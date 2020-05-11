#include "service_discoverer.h"

#include <iostream>
#include <sys/select.h>
#include <netdb.h>
#include <sstream>
#include <memory>

namespace sppvip {

const char *
errorString(DNSServiceErrorType error) {
    switch (error) {
        case kDNSServiceErr_NoError:
            return "no error";
        case kDNSServiceErr_Unknown:
            return "unknown";
        case kDNSServiceErr_NoSuchName:
            return "no such name";
        case kDNSServiceErr_NoMemory:
            return "no memory";
        case kDNSServiceErr_BadParam:
            return "bad param";
        case kDNSServiceErr_BadReference:
            return "bad reference";
        case kDNSServiceErr_BadState:
            return "bad state";
        case kDNSServiceErr_BadFlags:
            return "bad flags";
        case kDNSServiceErr_Unsupported:
            return "unsupported";
        case kDNSServiceErr_NotInitialized:
            return "not initialized";
        case kDNSServiceErr_AlreadyRegistered:
            return "already registered";
        case kDNSServiceErr_NameConflict:
            return "name conflict";
        case kDNSServiceErr_Invalid:
            return "invalid";
        case kDNSServiceErr_Firewall:
            return "firewall";
        case kDNSServiceErr_Incompatible:
            return "incompatible";
        case kDNSServiceErr_BadInterfaceIndex:
            return "bad interface index";
        case kDNSServiceErr_Refused:
            return "refused";
        case kDNSServiceErr_NoSuchRecord:
            return "no such record";
        case kDNSServiceErr_NoAuth:
            return "no auth";
        case kDNSServiceErr_NoSuchKey:
            return "no such key";
        case kDNSServiceErr_NATTraversal:
            return "NAT traversal";
        case kDNSServiceErr_DoubleNAT:
            return "double NAT";
        case kDNSServiceErr_BadTime:
            return "bad time";
#ifdef kDNSServiceErr_BadSig
        case kDNSServiceErr_BadSig:
            return "bad sig";
#endif
#ifdef kDNSServiceErr_BadKey
        case kDNSServiceErr_BadKey:
            return "bad key";
#endif
#ifdef kDNSServiceErr_Transient
        case kDNSServiceErr_Transient:
            return "transient";
#endif
#ifdef kDNSServiceErr_ServiceNotRunning
        case kDNSServiceErr_ServiceNotRunning:
            return "service not running";
#endif
#ifdef kDNSServiceErr_NATPortMappingUnsupported
        case kDNSServiceErr_NATPortMappingUnsupported:
            return "NAT port mapping unsupported";
#endif
#ifdef kDNSServiceErr_NATPortMappingDisabled
        case kDNSServiceErr_NATPortMappingDisabled:
            return "NAT port mapping disabled";
#endif
#ifdef kDNSServiceErr_NoRouter
        case kDNSServiceErr_NoRouter:
            return "no router";
#endif
#ifdef kDNSServiceErr_PollingMode
        case kDNSServiceErr_PollingMode:
            return "polling mode";
#endif
#ifdef kDNSServiceErr_Timeout
        case kDNSServiceErr_Timeout:
            return "timeout";
#endif
        default:
            return "unknown error code";
    }
}

ServiceDiscoverer::ServiceDiscoverer(DiscoveredNodeList& discoveredList)
    : m_discoveredList(discoveredList)
    , m_browseSrv()
    , m_browseMutex()
    , m_browseThread(nullptr)
    , m_browseRunning(false)
{}

ServiceDiscoverer::~ServiceDiscoverer()
{
    assert(m_browseThread->joinable());

    m_browseMutex.lock();
    m_browseRunning = false;
    m_browseMutex.unlock();

    m_browseThread->join();

    DNSServiceRefDeallocate(m_browseSrv);
}

void ServiceDiscoverer::start()
{
    if (!m_browseRunning) {
        m_browseThread = std::make_unique<std::thread>([this]() constexpr {
            (*this)();
        });
    }
}

void ServiceDiscoverer::onBrowse(const std::string& serviceName,
                                 const std::string& regType,
                                 const std::string& replayDomain)
{
    DNSServiceRef resolveSrv;
    auto err = DNSServiceResolve(&resolveSrv,
                                 0,
                                 0,
                                 serviceName.c_str(),
                                 regType.c_str(),
                                 replayDomain.c_str(),
                                 resolveCallback,
                                 this);

    if (err) {
        std::cerr << "Failed resolving: "
            << serviceName
            << " Error:"
            <<errorString(err) << std::endl;
    }
    else {
        DNSServiceProcessResult(resolveSrv);
        DNSServiceRefDeallocate(resolveSrv);
    }
}

void ServiceDiscoverer::browseCallback(DNSServiceRef sdRef, 
                                       DNSServiceFlags flags, 
                                       uint32_t interfaceIndex, 
                                       DNSServiceErrorType errorCode, 
                                       const char *serviceName, 
                                       const char *regtype, 
                                       const char *replyDomain, 
                                       void *context)
{
    assert(context);

    if (errorCode == kDNSServiceErr_NoError) {
        static_cast<ServiceDiscoverer*>(context)->onBrowse(
            std::string(serviceName),
            std::string(regtype),
            std::string(replyDomain));
    }
}

void ServiceDiscoverer::resolveCallback(DNSServiceRef sdRef,
                                        DNSServiceFlags flags,
                                        uint32_t interfaceIndex,
                                        DNSServiceErrorType errorCode,
                                        const char* fullname,
                                        const char* hosttarget,
                                        uint16_t port,
                                        uint16_t txtLen,
                                        const unsigned char* txtRecord,
                                        void* context)
{
    assert(context);

    if (errorCode == kDNSServiceErr_NoError) {
        static_cast<ServiceDiscoverer*>(context)->onResolve(
            std::string(fullname),
            std::string(hosttarget),
            ntohs(port),
            std::string(txtRecord, txtRecord + txtLen)
        );
    }
}

/// @brief instance mDNS resolve callback
void ServiceDiscoverer::onResolve(const std::string& fullName,
                                  const std::string& host,
                                  uint16_t port,
                                  const std::string& txtRecord)
{
    addrinfo* addrs = nullptr;
    addrinfo hints;
    bzero(&hints, sizeof(hints));

    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;

    std::stringstream ss;
    ss << port;
    int rv = getaddrinfo(host.c_str(), ss.str().c_str(), &hints, &addrs);

    if (rv == -1) {
        std::cerr << "Failed to resolve " << host << std::endl;
    }
    else {
        struct Node serviceNode;
        serviceNode.m_id = fullName;
        serviceNode.m_ioStream = std::make_shared<UdpIOStream>(addrs->ai_addr);

        m_discoveredList.append(std::move(serviceNode));
    }
}

void ServiceDiscoverer::operator()()
{
    m_browseRunning = true;

    auto err = DNSServiceBrowse(&m_browseSrv,
                                0, 
                                0,
                                "_sspvip._udp", 
                                NULL,
                                browseCallback,
                                this);

    if (err) {
        std::cerr << "Service Browse Error: " << errorString(err) << std::endl;
        exit(9);
    }

    int sockFd = DNSServiceRefSockFD(m_browseSrv);
    int nSockFd = sockFd + 1;

    bool bInError = false;

    while (true) {
        fd_set rfds;
        struct timeval readTv;

        FD_ZERO(&rfds);
        FD_SET(sockFd, &rfds);

        readTv.tv_sec = 1;
        readTv.tv_usec = 0;

        int retVal = select(nSockFd, &rfds, NULL, NULL, &readTv);

        if (retVal > 0) {
            err = kDNSServiceErr_NoError;
            if (FD_ISSET(sockFd, &rfds)) {
                err = DNSServiceProcessResult(m_browseSrv);

                if (err) {
                    bInError = true;
                }
            }
        }

        if (bInError) {
            break;
        }

        std::scoped_lock<std::mutex> lock(m_browseMutex);
        if (!m_browseRunning) {
            break;
        }
    }

    std::cout << "Done browse thread" << std::endl;
}
};
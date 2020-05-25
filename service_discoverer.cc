#include "service_discoverer.h"

#include "service_common.h"

#include <iostream>
#include <memory>
#include <netdb.h>
#include <sstream>
#include <sys/select.h>

namespace sppvip {

namespace {
void
browse_callback(DNSServiceRef sdRef,
                DNSServiceFlags flags,
                uint32_t interfaceIndex,
                DNSServiceErrorType errorCode,
                const char* serviceName,
                const char* regtype,
                const char* replyDomain,
                void* context)
{
    assert(context);

    if (errorCode == kDNSServiceErr_NoError) {
        static_cast<service_discoverer*>(context)->onBrowse(
            std::string(serviceName),
            std::string(regtype),
            std::string(replyDomain));
    }
}

void
resolveCallback(DNSServiceRef sdRef,
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
        static_cast<service_discoverer*>(context)->onResolve(
            std::string(fullname),
            std::string(hosttarget),
            ntohs(port),
            std::string(txtRecord, txtRecord + txtLen));
    }
}
}

service_discoverer::service_discoverer(
    discovered_node_list<sppvip_node>& discoveredList)
    : m_discoveredList(discoveredList)
    , m_browseSrv()
    , m_browseMutex()
    , m_browseThread(nullptr)
    , m_browseRunning(false)
    , m_host_name()
{
    char host_name[256];
    gethostname(host_name, sizeof(host_name));
    m_host_name = host_name;
}

service_discoverer::~service_discoverer()
{
    if (m_browseThread && m_browseThread->joinable()) {

        if (m_browseRunning) {
            std::scoped_lock<decltype(m_browseMutex)> lock(m_browseMutex);
            m_browseRunning = false;
        }

        m_browseThread->join();
    }

    DNSServiceRefDeallocate(m_browseSrv);
}

void
service_discoverer::start()
{
    if (!m_browseRunning) {
        m_browseThread = std::make_unique<std::thread>([this]() { (*this)(); });
    }
}

void
service_discoverer::stop()
{
    if (m_browseRunning) {
        std::scoped_lock<decltype(m_browseMutex)> lock(m_browseMutex);
        m_browseRunning = false;
    }
}

void
service_discoverer::onBrowse(const std::string& serviceName,
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
        std::cerr << "Failed resolving: " << serviceName
                  << " Error:" << mdns_error_string(err) << std::endl;
    } else {
        DNSServiceProcessResult(resolveSrv);
        DNSServiceRefDeallocate(resolveSrv);
    }
}

/// @brief instance mDNS resolve callback
void
service_discoverer::onResolve(const std::string& fullName,
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
    } else {
        if (addrs) {
            sppvip_node serviceNode;
            serviceNode.m_id = fullName;
            serviceNode.m_ioStream =
                std::make_shared<sppvip_node::stream_type>(addrs->ai_addr);

            m_discoveredList.append(std::move(serviceNode));
        } else {
            std::cerr << "getaddrinfo failed on " << host << std::endl;
        }
    }
}

void
service_discoverer::operator()()
{
    m_browseRunning = true;

    auto err = DNSServiceBrowse(
        &m_browseSrv, 0, 0, SPPVIP_SERVICE_NAME, NULL, browse_callback, this);

    if (err) {
        std::cerr << "Service Browse Error: " << mdns_error_string(err)
                  << std::endl;
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
}
};

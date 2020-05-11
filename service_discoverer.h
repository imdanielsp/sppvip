#ifndef SERVICE_DISCOVERER_H
#define SERVICE_DISCOVERER_H

#include "node.h"

#include <dns_sd.h>

#include <memory>
#include <thread>

namespace sppvip {

/// @brief Service discoverer
class ServiceDiscoverer
{
public:
    /// @brief Constructor
    ///
    /// @param[in,out] discoveredList thread-safe list of discovered nodes
    ServiceDiscoverer(DiscoveredNodeList& discoveredList);

    ~ServiceDiscoverer();

    /// @brief Start the browse thread
    void start();

private:
    /// @brief Browse thread entry point
    void operator()();

    /// @brief mDNS browse callback
    static void browseCallback(DNSServiceRef sdRef, 
                               DNSServiceFlags flags, 
                               uint32_t interfaceIndex, 
                               DNSServiceErrorType errorCode, 
                               const char* serviceName, 
                               const char* regtype, 
                               const char* replyDomain, 
                               void *context);

    /// @brief instance mDNS browse callback
    void onBrowse(const std::string& serviceName,
                  const std::string& regType,
                  const std::string& replayDomain);

    /// @brief mDNS resolve callback
    static void resolveCallback(DNSServiceRef sdRef,
                                DNSServiceFlags flags,
                                uint32_t interfaceIndex,
                                DNSServiceErrorType errorCode,
                                const char* fullname,
                                const char* hosttarget,
                                uint16_t port,
                                uint16_t txtLen,
                                const unsigned char* txtRecord,
                                void* context);

    /// @brief instance mDNS resolve callback
    void onResolve(const std::string& fullName,
                   const std::string& host,
                   uint16_t port,
                   const std::string& txtRecord);

    DiscoveredNodeList& m_discoveredList;
    DNSServiceRef m_browseSrv;

    std::mutex m_browseMutex;
    std::unique_ptr<std::thread> m_browseThread;
    bool m_browseRunning;
};

};

#endif

#ifndef SERVICE_DISCOVERER_H
#define SERVICE_DISCOVERER_H

#include "node.h"

#include <dns_sd.h>

#include <memory>
#include <thread>

namespace sppvip {

/// @brief Service discoverer
class service_discoverer
{
public:
    /// @brief Constructor
    ///
    /// @param[in,out] discoveredList thread-safe list of discovered nodes
    service_discoverer(discovered_node_list<sppvip_node>& discovered_list);

    ~service_discoverer();

    /// @brief Start the browse thread
    void start();

    void stop();

    /// @brief instance mDNS browse callback
    void onBrowse(const std::string& serviceName,
                  const std::string& regType,
                  const std::string& replayDomain);

    /// @brief instance mDNS resolve callback
    void onResolve(const std::string& fullName,
                   const std::string& host,
                   uint16_t port,
                   const std::string& txtRecord);

private:
    /// @brief Browse thread entry point
    void operator()();

    discovered_node_list<sppvip_node>& m_discoveredList;
    DNSServiceRef m_browseSrv;

    std::mutex m_browseMutex;
    std::unique_ptr<std::thread> m_browseThread;
    bool m_browseRunning;
    std::string m_host_name;
};

};

#endif

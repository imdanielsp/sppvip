#ifndef NODE_H
#define NODE_H

#include "net_io_stream.h"

#include <map>
#include <mutex>
#include <string>

namespace sppvip {

/// @brief represents a peer node running the SPPVIP
/// (Simple Peer-to-Peer Voice over IP) service
struct sppvip_node
{
    using stream_type = udp_stream;
    using stream_type_base = io_stream;

    std::string m_id;
    std::shared_ptr<stream_type_base> m_ioStream;
};

/// @brief thread-safe discovered node list
template<typename node_type>
class discovered_node_list
{
public:
    using const_iterator =
        typename std::map<std::string, node_type>::const_iterator;

    discovered_node_list()
        : m_nodesMutex()
        , m_discoveredNodes()
    {}

    ~discovered_node_list() = default;

    void append(node_type&& node)
    {
        std::scoped_lock<decltype(m_nodesMutex)> lock(m_nodesMutex);
        m_discoveredNodes.insert({ node.m_id, node });
    }

    bool remove(const node_type& node)
    {
        bool bDidRemove = false;
        std::scoped_lock<decltype(m_nodesMutex)> lock(m_nodesMutex);
        if (m_discoveredNodes.count(node.m_id) > 0) {
            m_discoveredNodes.erase(node.m_id);
            bDidRemove = true;
        }

        return bDidRemove;
    }

    size_t size() const { return m_discoveredNodes.size(); }

    const_iterator get_node(const std::string& id) const
    {
        return m_discoveredNodes.find(id);
    }

    const_iterator begin() { return m_discoveredNodes.cbegin(); }

    const_iterator end() { return m_discoveredNodes.cend(); }

private:
    std::mutex m_nodesMutex;
    std::map<std::string, node_type> m_discoveredNodes;
};

};
#endif

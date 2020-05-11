#ifndef NODE_H
#define NODE_H

#include "net_io_stream.h"

#include <string>
#include <map>
#include <mutex>

namespace sppvip {

/// @brief represents a peer node running the SPPVIP
/// (Simple Peer-to-Peer Voice over IP) service
struct Node
{
    std::string m_id;
    std::shared_ptr<IOStreamIntf> m_ioStream;
};

/// @brief thread-safe discovered node list
class DiscoveredNodeList
{
public:
    using const_iterator = std::map<std::string, Node>::const_iterator;

    DiscoveredNodeList();

    ~DiscoveredNodeList() = default;

    void append(Node&& node);

    bool remove(const Node& node);

    size_t size() const { return m_discoveredNodes.size(); }

    const_iterator getNode(const std::string& id) const;

    const_iterator begin() { return m_discoveredNodes.cbegin(); }

    const_iterator end() { return m_discoveredNodes.cend(); }

private:
    std::mutex m_nodesMutex;
    std::map<std::string, Node> m_discoveredNodes;
};

};
#endif
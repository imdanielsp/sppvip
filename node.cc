
#include "node.h"

namespace sppvip {

DiscoveredNodeList::DiscoveredNodeList()
    : m_nodesMutex()
    , m_discoveredNodes()
{}

void DiscoveredNodeList::append(Node&& node)
{
    std::scoped_lock<std::mutex> lock(m_nodesMutex);
    m_discoveredNodes.insert({node.m_id, node});
}

bool DiscoveredNodeList::remove(const Node& node)
{
    bool bDidRemove = false;
    std::scoped_lock<std::mutex> lock(m_nodesMutex);
    if (m_discoveredNodes.count(node.m_id) > 0) {
        m_discoveredNodes.erase(node.m_id);
        bDidRemove = true;
    }

    return bDidRemove;
}

DiscoveredNodeList::const_iterator DiscoveredNodeList::getNode(const std::string& id) const
{
    return m_discoveredNodes.find(id);
}

}

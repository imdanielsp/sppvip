#include "net_io_stream.h"

#include <assert.h>
#include <iostream>
#include <netinet/in.h>

namespace sppvip {

udp_stream::udp_socket udp_stream::m_inSocket = 0;
udp_stream::sock_addr_in udp_stream::m_inAddr = sock_addr_in();

udp_stream::udp_stream(std::string&& ip, int port)
    : m_outSocket(socket(AF_INET, SOCK_DGRAM, 0))
{
    assert(m_outSocket);

    bzero(&m_outAddr, sizeof(m_outAddr));
    m_outAddr.sin_family = AF_INET;
    m_outAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    m_outAddr.sin_port = htons(port);

    initInSocket();
}

udp_stream::udp_stream(sock_addr* outSckAddr)
    : m_outSocket(socket(AF_INET, SOCK_DGRAM, 0))
    , m_outAddr(*(reinterpret_cast<sock_addr_in*>(outSckAddr)))
{
    assert(outSckAddr);
    assert(m_outSocket);

    initInSocket();
}

void
udp_stream::initInSocket()
{
    if (!m_inSocket) {
        return;
    }

    m_inSocket = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&m_inAddr, sizeof(m_inAddr));
    m_inAddr.sin_family = AF_INET;
    m_inAddr.sin_addr.s_addr = INADDR_ANY;
    m_inAddr.sin_port = htons(52999);

    auto rv = bind(m_inSocket,
                   reinterpret_cast<const sock_addr*>(&m_inAddr),
                   sizeof(m_inAddr));

    assert(rv >= 0);
}

std::string
udp_stream::read()
{
    socklen_t addrLen = sizeof(m_inSocket);

    char buff[4096];
    buff[0] = '\0';
    // TODO: This blocks indefinetly, do a select loop here
    auto szRcv = recvfrom(m_inSocket,
                          buff,
                          sizeof(buff),
                          0,
                          reinterpret_cast<struct sockaddr*>(&m_inSocket),
                          &addrLen);

    return std::string(buff);
}

bool
udp_stream::write(AudioFrame&& frame)
{
    auto rv = sendto(m_outSocket,
                     &frame,
                     sizeof(frame),
                     0,
                     reinterpret_cast<struct sockaddr*>(&m_outAddr),
                     sizeof(m_outAddr));

    return rv > 0;
}

} // namespace sppvip

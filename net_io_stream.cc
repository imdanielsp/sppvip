#include "net_io_stream.h"

#include <assert.h>
#include <netinet/in.h>
#include <iostream>

namespace sppvip {

UdpIOStream::UDPSocket UdpIOStream::m_inSocket = 0;
UdpIOStream::SockAddrIn UdpIOStream::m_inAddr = SockAddrIn();

UdpIOStream::UdpIOStream(std::string&& ip, int port)
    : m_outSocket(socket(AF_INET, SOCK_DGRAM, 0))
{
    assert(m_outSocket);

    bzero(&m_outAddr, sizeof(m_outAddr));
    m_outAddr.sin_family = AF_INET;
    m_outAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    m_outAddr.sin_port = htons(port);

    initInSocket();
}

UdpIOStream::UdpIOStream(SockAddr* outSckAddr)
    : m_outSocket(socket(AF_INET, SOCK_DGRAM, 0))
    , m_outAddr(*(reinterpret_cast<SockAddrIn*>(outSckAddr)))
{
    assert(outSckAddr);
    assert(m_outSocket);

    initInSocket();
}

void UdpIOStream::initInSocket()
{
    if (!m_inSocket) {
        return;
    }

    m_inSocket = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&m_inAddr, sizeof(m_inAddr));
    m_inAddr.sin_family = AF_INET;
    m_inAddr.sin_addr.s_addr = INADDR_ANY;
    m_inAddr.sin_port = htons(52999);

    auto rv = bind(m_inSocket, reinterpret_cast<const SockAddr*>(&m_inAddr), sizeof(m_inAddr));

    assert(rv >= 0);
}

std::string UdpIOStream::read()
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

bool UdpIOStream::write(AudioFrame&& frame)
{
    auto rv = sendto(m_outSocket,
                     &frame,
                     sizeof(frame),
                     0,
                     reinterpret_cast<struct sockaddr *>(&m_outAddr),
                     sizeof(m_outAddr));

    return rv > 0;
}

}

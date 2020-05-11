#ifndef NET_IO_STREAM_H
#define NET_IO_STREAM_H

#include "audio_frame.h"

#include <string>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>

namespace sppvip {

class IOStreamIntf
{
public:
    IOStreamIntf() = default;

    virtual ~IOStreamIntf() = default;

    virtual std::string read() = 0;

    virtual bool write(AudioFrame&& frame) = 0;
};

class UdpIOStream : public IOStreamIntf
{
    using UDPSocket = int;
    using SockAddr = sockaddr;
    using SockAddrIn = sockaddr_in;

public:
    UdpIOStream(std::string&& ip, int port);

    UdpIOStream(SockAddr* outSckAddr);

    virtual ~UdpIOStream() = default;

    std::string read() override;

    bool write(AudioFrame&& frame) override;

private:
    void initInSocket();

private:
    UDPSocket m_outSocket;
    SockAddrIn m_outAddr;

    static UDPSocket m_inSocket;
    static SockAddrIn m_inAddr;
};

};

#endif

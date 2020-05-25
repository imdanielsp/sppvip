#ifndef NET_IO_STREAM_H
#define NET_IO_STREAM_H

#include "audio_frame.h"

#include <arpa/inet.h>
#include <string>
#include <sys/socket.h>
#include <vector>

namespace sppvip {

class io_stream
{
public:
    io_stream() = default;

    virtual ~io_stream() = default;

    virtual std::string read() = 0;

    virtual bool write(AudioFrame&& frame) = 0;
};

class udp_stream : public io_stream
{
    using udp_socket = int;
    using sock_addr = sockaddr;
    using sock_addr_in = sockaddr_in;

public:
    udp_stream(std::string&& ip, int port);

    udp_stream(sock_addr* outSckAddr);

    virtual ~udp_stream() = default;

    std::string read() override;

    bool write(AudioFrame&& frame) override;

private:
    void initInSocket();

private:
    udp_socket m_outSocket;
    sock_addr_in m_outAddr;

    static udp_socket m_inSocket;
    static sock_addr_in m_inAddr;
};

};

#endif

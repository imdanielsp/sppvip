#include "audio_frame.h"
#include "audio_stream_player.h"
#include "audio_stream_recorder.h"
#include "net_io_stream.h"
#include "service_advertiser.h"
#include "service_discoverer.h"

#include <dns_sd.h>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <thread>
#include <vector>

using namespace sppvip;

class audio_stream_writer : public audio_stream_handler
{
public:
    audio_stream_writer(io_stream& stream)
        : m_stream(stream)
    {}

    ~audio_stream_writer() = default;

    void on_audio_data(AudioFrame&& frame) override
    {
        m_stream.write(std::move(frame));
    }

private:
    io_stream& m_stream;
};

class file_stream_writer : public audio_stream_handler
{
public:
    file_stream_writer(std::string&& name)
        : file_stream(name, std::ios::out | std::ios::app | std::ios::binary)
    {}

    ~file_stream_writer() { file_stream.close(); }

    void on_audio_data(AudioFrame&& frame) override
    {
        static int max_write = 100;
        static int written = 0;

        if (max_write > (written++)) {
            file_stream.write(reinterpret_cast<const char*>(frame.payload),
                              sizeof(frame.payload));
        } else {
            std::cout << "not writting..." << std::endl;
        }
    }

private:
    std::ofstream file_stream;
};

class audio_file_reader : io_stream
{
public:
    audio_file_reader(std::string&& name)
        : file_stream(name, std::ios::in)
    {}

    ~audio_file_reader() { file_stream.close(); }

    std::string read() override
    {
        char* buffer = new char[1 << 16];
        auto size = file_stream.readsome(buffer, sizeof(buffer));
        std::string s(buffer, size);
        delete[] buffer;
        return s;
    }

    bool write(AudioFrame&& frame) override { return false; }

private:
    std::ifstream file_stream;
};

template<typename stream, typename player>
class audio_streamer
{
public:
    audio_streamer(stream& stm, player& ply)
        : s(stm)
        , p(ply)
    {}

    void stream_audio() { s.read(); }

private:
    stream& s;
    player& p;
};

int
main(int argc, char** argv)
{
    discovered_node_list<sppvip_node> discovered_nodes;
    service_discoverer srv_discoverer(discovered_nodes);
    srv_discoverer.start();

    service_advertiser advertiser;
    advertiser.advertise();

    file_stream_writer file_writer("raw_output.bin");
    audio_stream_recorder recorder(file_writer);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto node_id = "someone._sspvip._udp.local.";
    auto node = discovered_nodes.get_node(node_id);

    if (node == discovered_nodes.end()) {
        std::cerr << "someone is not available" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    advertiser.stop();
    srv_discoverer.stop();

    return 0;
}

#ifndef AUDIO_STREAM_RECORDER_H
#define AUDIO_STREAM_RECORDER_H

#include "audio_frame.h"
#include <condition_variable>
#include <memory>
#include <thread>

namespace sppvip {

static const int AQ_NUM_OF_BUFFER = 3;

class audio_stream_handler
{
public:
    audio_stream_handler() = default;

    ~audio_stream_handler() = default;

    virtual void on_audio_data(AudioFrame&& frame) = 0;
};

class audio_stream_recorder
{
public:
    audio_stream_recorder(audio_stream_handler& audio_handler);

    ~audio_stream_recorder();

    bool start();

    bool stop();

    void on_audio(AudioFrame&& frame);

private:
    enum recorder_state
    {
        IDLE,
        RECORDING,
    };

    void worker_func();

private:
    recorder_state state_;
    bool terminate_thread_;
    std::unique_ptr<std::thread> recorder_thrd_;
    std::condition_variable recorder_cond_;
    std::mutex recorder_mtx_;
    audio_stream_handler& audio_handler_;
};
}

#endif

#include "audio_stream_recorder.h"
#include "audio_frame.h"
#include <iostream>
#include <memory>

namespace sppvip {

audio_stream_recorder::audio_stream_recorder(
    audio_stream_handler& audio_handler)
    : state_(IDLE)
    , terminate_thread_(false)
    , recorder_thrd_(nullptr)
    , recorder_cond_()
    , recorder_mtx_()
    , audio_handler_(audio_handler)
    , in_stream_params_()
{
    recorder_thrd_ = std::make_unique<decltype(recorder_thrd_)::element_type>(
        [this]() { worker_func(); });
}

audio_stream_recorder::~audio_stream_recorder()
{
    if (!terminate_thread_) {
        recorder_mtx_.lock();

        state_ = IDLE;
        terminate_thread_ = true;
        std::cout << "terminate thread true" << std::endl;

        recorder_mtx_.unlock();
        std::cout << "released mtx joining" << std::endl;

        if (recorder_thrd_ && recorder_thrd_->joinable()) {
            recorder_thrd_->join();
        }
    }
}

void
audio_stream_recorder::worker_func()
{
    while (true) {
        switch (state_) {
            case IDLE: {
                // std::unique_lock<decltype(recorder_mtx_)>
                // lock(recorder_mtx_); recorder_cond_.wait(lock,
                //                     [this]() { return state_ == RECORDING;
                //                     });
                std::cout << "idle" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } break;
            case RECORDING: {
                std::cout << "recording" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } break;
            default:
                assert(false);
        }

        std::scoped_lock<decltype(recorder_mtx_)> lock(recorder_mtx_);
        std::cout << "got lock in thread" << std::endl;
        if (terminate_thread_) {
            std::cout << "breaking" << std::endl;
            break;
        }
    }

    std::cout << "finished" << std::endl;
}

bool
audio_stream_recorder::start()
{
    auto rv = false;

    if (state_ != RECORDING) {
        std::lock_guard<decltype(recorder_mtx_)> lock(recorder_mtx_);
        state_ = RECORDING;
        recorder_cond_.notify_all();
        rv = true;
    }

    return rv;
}

bool
audio_stream_recorder::stop()
{
    auto rv = false;

    if (state_ != IDLE) {
        std::lock_guard<decltype(recorder_mtx_)> lock(recorder_mtx_);
        state_ = IDLE;
        rv = true;
    }

    return rv;
}

void
audio_stream_recorder::on_audio(AudioFrame&& frame)
{
    audio_handler_.on_audio_data(std::move(frame));
}

}

#ifndef AUDIO_STREAM_PLAYER_H
#define AUDIO_STREAM_PLAYER_H

#include "audio_frame.h"

namespace sppvip {

class stream_player
{
public:
    stream_player() = default;
    virtual ~stream_player() = default;

    virtual void on_audio(AudioFrame&& audio) = 0;
};

class audio_stream_player : public stream_player
{
public:
    audio_stream_player();

    ~audio_stream_player();

    void compute_output_buffer_size(float seconds);

    void on_audio(AudioFrame&& audio) override;

    bool m_isRunning;
};
}
#endif

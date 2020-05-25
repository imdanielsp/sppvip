#include "audio_stream_player.h"
#include "audio_frame.h"

#include <iostream>

namespace sppvip {

audio_stream_player::audio_stream_player()
    : m_isRunning(false)
{}

audio_stream_player::~audio_stream_player() {}

void
audio_stream_player::on_audio(AudioFrame&& audio)
{}

void
audio_stream_player::compute_output_buffer_size(float seconds)
{}

}

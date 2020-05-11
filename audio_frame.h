#ifndef AUDIO_FRAME_H
#define AUDIO_FRAME_H

#include <cstdint>

namespace sppvip {

struct AudioFrame
{
    AudioFrame()
        : rsv(0)
        , length(0)
        , payload()
    {}

    // Header
    uint8_t rsv;
    uint32_t length;

    uint8_t payload[4096];
} __attribute__((packed));

};
#endif
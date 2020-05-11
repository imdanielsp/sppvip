#ifndef AUDIO_STREAM_PLAYER_H
#define AUDIO_STREAM_PLAYER_H

#include <CoreFoundation/CFURL.h>
#include <AudioToolbox/AudioQueue.h>
#include <AudioToolbox/AudioFile.h>
#include <CoreAudioTypes/CoreAudioTypes.h>

class OutputAudioQueue
{
public:
    OutputAudioQueue() = default;
    virtual ~OutputAudioQueue() = default;

    void deriveOutputBufferSize(uint32_t packetSize, float seconds);
public:
    static const int AQ_NUM_OF_BUFFERS = 3;

    AudioStreamBasicDescription m_dataFormat;
    AudioQueueRef m_queue;
    AudioQueueBufferRef m_buffers[AQ_NUM_OF_BUFFERS];
    AudioFileID m_audioFile;
    uint32_t m_bufferByteSize;
    int64_t m_currentPacket;
    int32_t m_numPacketsToRead;
    AudioStreamPacketDescription* m_packetDescs;
    bool m_isRunning;
};

// Audio Player: https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioQueueProgrammingGuide/AQPlayback/PlayingAudio.html#//apple_ref/doc/uid/TP40005343-CH3-SW1

class AudioStreamPlayer
    : public OutputAudioQueue
{
public:
    AudioStreamPlayer()
        : OutputAudioQueue()
    {}

    virtual ~AudioStreamPlayer() = default;

    static void onOutputBuffer(void* pAudioStreamPlayer,
                               AudioQueueRef inAQ,
                               AudioQueueBufferRef inBuffer);

private:
    void internalOnOutputBuffer(AudioQueueRef inAQ,
                                AudioQueueBufferRef inBuffer);

};

#endif

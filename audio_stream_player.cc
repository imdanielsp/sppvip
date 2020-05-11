#include "audio_stream_player.h"

#include <iostream>

void OutputAudioQueue::deriveOutputBufferSize(uint32_t packetSize,
                                              float seconds)
{
    static const int maxBufferSize = 0x50000;
    static const int minBufferSize = 0x4000;

    if (m_dataFormat.mFramesPerPacket != 0) {
        Float64 numPacketsForTime =
            m_dataFormat.mSampleRate / m_dataFormat.mFramesPerPacket * seconds;
        m_bufferByteSize = numPacketsForTime * packetSize;
    } else {
        m_bufferByteSize =
            maxBufferSize > packetSize ?
                maxBufferSize : packetSize;
    }

    if (m_bufferByteSize > maxBufferSize && m_bufferByteSize > packetSize)
        m_bufferByteSize = maxBufferSize;
    else {
        if (m_bufferByteSize < minBufferSize)
            m_bufferByteSize = minBufferSize;
    }

    m_numPacketsToRead = m_bufferByteSize / packetSize;
}

void AudioStreamPlayer::onOutputBuffer(void* pAudioStreamPlayer,
                                       AudioQueueRef inAQ,
                                       AudioQueueBufferRef inBuffer)
{
    static_cast<AudioStreamPlayer*>(
        pAudioStreamPlayer)->internalOnOutputBuffer(inAQ, inBuffer);
}

void AudioStreamPlayer::internalOnOutputBuffer(AudioQueueRef inAQ,
                                               AudioQueueBufferRef inBuffer)
{
    if (m_isRunning == 0) return; 

    // Read data
    uint32_t numPackets = m_numPacketsToRead;
    uint32_t numBytesReadFromFile = 0;

    OSStatus result = AudioFileReadPackets(m_audioFile,
                                           false,
                                           &numBytesReadFromFile,
                                           m_packetDescs,
                                           m_currentPacket,
                                           &numPackets,
                                           inBuffer->mAudioData);

    std::cout << result << ": AudioFileReadPackets, numPackets: " << numPackets << std::endl;

    if (numPackets > 0) {
        // Enque buffer
        inBuffer->mAudioDataByteSize = numBytesReadFromFile;
        AudioQueueEnqueueBuffer(m_queue, inBuffer, m_packetDescs ? numPackets : 0, m_packetDescs);
        m_currentPacket += numPackets;
    } else {
        AudioQueueStop(m_queue, false);
        m_isRunning = false;
    }
}

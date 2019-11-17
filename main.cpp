#include <iostream>
#include <stdint.h>

#include <CoreFoundation/CFURL.h>
#include <AudioToolbox/AudioQueue.h>
#include <AudioToolbox/AudioFile.h>
#include <CoreAudioTypes/CoreAudioTypes.h>

static const int AQ_NUM_OF_BUFFER = 3;

struct AQRecoderState
{
    AudioStreamBasicDescription m_dataFormat;
    AudioQueueRef m_queue;
    AudioQueueBufferRef m_buffers[AQ_NUM_OF_BUFFER];
    AudioFileID m_audioFile;
    uint32_t m_bufferByteSize;
    int64_t m_currentPacket;
    bool m_isRunning;
};

static void onAudioBuffer(void *pAQData,
                          AudioQueueRef audioQueue,
                          AudioQueueBufferRef buffer,
                          const AudioTimeStamp *pStartTime,
                          uint32_t numPackets,
                          const AudioStreamPacketDescription *pPacketDesc)
{
    AQRecoderState *pAqData = static_cast<AQRecoderState *>(pAQData);

    if (numPackets == 0 && pAqData->m_dataFormat.mBytesPerPacket != 0)
    {
        numPackets = buffer->mAudioDataByteSize / pAqData->m_dataFormat.mBytesPerPacket;
    }

    if (AudioFileWritePackets(pAqData->m_audioFile,
                              false,
                              buffer->mAudioDataByteSize,
                              pPacketDesc,
                              pAqData->m_currentPacket,
                              &numPackets,
                              buffer->mAudioData) == noErr)
    {
        pAqData->m_currentPacket += numPackets;
    }

    if (pAqData->m_isRunning == 0)
    {
        return;
    }

    AudioQueueEnqueueBuffer(pAqData->m_queue,
                            buffer,
                            0,
                            NULL);
}

void deriveBufferSize(AudioQueueRef audioQueue,
                      AudioStreamBasicDescription &asbDesc,
                      float_t seconds,
                      uint32_t *pBufferSize)
{
    assert(pBufferSize);

    static const int maxBufferSize = 0x5000;

    int maxPacketSize = asbDesc.mBytesPerPacket;
    if (maxPacketSize == 0)
    {
        uint32_t maxVBRPacketSize = sizeof(maxPacketSize);
        AudioQueueGetProperty(audioQueue,
                              kAudioQueueProperty_MaximumOutputPacketSize,
                              &maxPacketSize,
                              &maxVBRPacketSize);
    }

    float numBytesForTime = asbDesc.mSampleRate * maxPacketSize * seconds;
    *pBufferSize = numBytesForTime < maxBufferSize ? numBytesForTime : maxBufferSize;
}

static void initAQRecorder(AQRecoderState &aqRecorder)
{
    aqRecorder.m_dataFormat.mFormatID = kAudioFormatMPEGLayer3;
    aqRecorder.m_dataFormat.mSampleRate = 44100.0;
    aqRecorder.m_dataFormat.mChannelsPerFrame = 2;
    aqRecorder.m_dataFormat.mBitsPerChannel = 16;
    aqRecorder.m_dataFormat.mBytesPerPacket =
        aqRecorder.m_dataFormat.mBytesPerFrame =
            aqRecorder.m_dataFormat.mChannelsPerFrame * sizeof(int16_t);
    aqRecorder.m_dataFormat.mFramesPerPacket = 1;
    aqRecorder.m_dataFormat.mFormatFlags = kLinearPCMFormatFlagIsBigEndian
                                        | kLinearPCMFormatFlagIsSignedInteger
                                        | kLinearPCMFormatFlagIsPacked;
}

static void allocateQueueBuffer(AQRecoderState &aqRecorder)
{
    for (int i = 0; i < AQ_NUM_OF_BUFFER; i++)
    {
        AudioQueueAllocateBuffer(aqRecorder.m_queue,
                                 aqRecorder.m_bufferByteSize,
                                 &aqRecorder.m_buffers[i]);

        AudioQueueEnqueueBuffer(aqRecorder.m_queue,
                                aqRecorder.m_buffers[i],
                                0,
                                NULL);
    }
}

int main(int argc, char **argv)
{
    AQRecoderState aqData;

    aqData.m_dataFormat.mFormatID = kAudioFormatLinearPCM;
    aqData.m_dataFormat.mSampleRate = 44100.0;
    aqData.m_dataFormat.mChannelsPerFrame = 2;
    aqData.m_dataFormat.mBitsPerChannel = 16;
    aqData.m_dataFormat.mBytesPerPacket =
        aqData.m_dataFormat.mBytesPerFrame =
            aqData.m_dataFormat.mChannelsPerFrame * sizeof(int16_t);
    aqData.m_dataFormat.mFramesPerPacket = 1;
    aqData.m_dataFormat.mFormatFlags = kLinearPCMFormatFlagIsBigEndian
                                        | kLinearPCMFormatFlagIsSignedInteger
                                        | kLinearPCMFormatFlagIsPacked;

    OSStatus status = AudioQueueNewInput(&aqData.m_dataFormat,
                                        onAudioBuffer,
                                        &aqData,
                                        NULL,
                                        kCFRunLoopCommonModes,
                                        0,
                                        &aqData.m_queue);
    
    if (status) {
        std::cout << status << ": AudioQueueNewInput failed" << std::endl;
        exit(1);
    }

    const char *filepath = "/Users/daniel.santos/cpp/sppvip/build/test_audio.aiff";
    CFURLRef audioFileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                                    (const uint8_t *)(filepath),
                                                                    strlen(filepath),
                                                                    false);
    if (!audioFileURL)
    {
        std::cout << "CFURLCreateFromFileSystemRepresentation failed" << std::endl;
        exit(1);
    }

    AudioFileTypeID fileType = kAudioFileAIFFType;
    status = AudioFileCreateWithURL(audioFileURL,
                                    fileType,
                                    &aqData.m_dataFormat,
                                    kAudioFileFlags_EraseFile,
                                    &aqData.m_audioFile);

    if (status) {
        std::cout << status << ": Failed creating the file" << std::endl;
        exit(1);
    }

    deriveBufferSize(aqData.m_queue,
                     aqData.m_dataFormat,
                     0.5,
                     &aqData.m_bufferByteSize);

    allocateQueueBuffer(aqData);

    std::cout << "Press enter to start recording..." << std::endl;

    char input;
    std::cin >> input;

    aqData.m_currentPacket = 0;
    aqData.m_isRunning = true;
    status = AudioQueueStart(aqData.m_queue, NULL);

    if (status) {
        std::cout << status << ": Failed creating starting the queue" << std::endl;
        exit(1);
    }

    std::cout << "Recording..." << std::endl
              << "Press enter to stop recording..." << std::endl;
    std::cin >> input;

    if (AudioQueueStop(aqData.m_queue, true) != noErr)
    {
        std::cout << "Failed stopping the queue" << std::endl;
        exit(1);
    }

    aqData.m_isRunning = false;

    if (AudioQueueDispose(aqData.m_queue, true) != noErr)
    {
        std::cout << "Failed disposing the audio queue" << std::endl;
        exit(1);
    }

    if (AudioFileClose(aqData.m_audioFile) != noErr)
    {
        std::cout << "Failed closing the audio file" << std::endl;
        exit(1);
    }

    return 0;
}
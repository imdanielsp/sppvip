#include <iostream>
#include <stdint.h>

#include <CoreFoundation/CFURL.h>
#include <AudioToolbox/AudioQueue.h>
#include <AudioToolbox/AudioFile.h>
#include <CoreAudioTypes/CoreAudioTypes.h>

#include <thread>
#include <vector>
#include <dns_sd.h>

#include "net_io_stream.h"
#include "service_discoverer.h"
#include "audio_frame.h"

// Audio recorder: https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioQueueProgrammingGuide/AQRecord/RecordingAudio.html#//apple_ref/doc/uid/TP40005343-CH4-SW15

sppvip::DiscoveredNodeList::const_iterator itr;

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
    // TODO: Implement framer object

    // Creates 4096 chunks
    if (buffer->mAudioDataByteSize > 0) {

        // Havoc framer
        for (uint32_t szConsumed = 0; szConsumed < buffer->mAudioDataByteSize;) {
            sppvip::AudioFrame frame;

            uint32_t szPayload = 0;
            if ((buffer->mAudioDataByteSize - szConsumed) >= sizeof(frame.payload)) {
                szPayload = sizeof(frame.payload);
            }
            else {
                szPayload = buffer->mAudioDataByteSize - szConsumed;
            }

            memcpy(frame.payload,
                   static_cast<uint8_t* const>(buffer->mAudioData) + szConsumed,
                   szPayload);

            frame.length = szPayload;
            szConsumed += szPayload;

            itr->second.m_ioStream->write(std::move(frame));
        }
    }

    AQRecoderState *pAqData = static_cast<AQRecoderState *>(pAQData);

    // if (numPackets == 0 && pAqData->m_dataFormat.mBytesPerPacket != 0)
    // {
    //     numPackets = buffer->mAudioDataByteSize / pAqData->m_dataFormat.mBytesPerPacket;
    // }

    // if (AudioFileWritePackets(pAqData->m_audioFile,
    //                           false,
    //                           buffer->mAudioDataByteSize,
    //                           pPacketDesc,
    //                           pAqData->m_currentPacket,
    //                           &numPackets,
    //                           buffer->mAudioData) == noErr)
    // {
    //     pAqData->m_currentPacket += numPackets;
    // }

    // std::cout << "Audio buffer size: " << buffer->mAudioDataByteSize << std::endl;
    // char rgBuffer[pAqData->m_dataFormat.mBytesPerPacket];
    // memcpy(rgBuffer, buffer->mAudioData, pAqData->m_dataFormat.mBytesPerPacket);

    // std::cout << "First frame: ";
    // for (int i = 0; i < pAqData->m_dataFormat.mBytesPerPacket; i++) {
    //     std::cout << +rgBuffer[i];
    //     if (i != pAqData->m_dataFormat.mBytesPerPacket - 1) std::cout << "-";
    //     else std::cout << std::endl;
    // }

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
    *pBufferSize =
        numBytesForTime < maxBufferSize ? numBytesForTime : maxBufferSize;
}

static void initAQRecorder(AQRecoderState &aqRecorder)
{
    aqRecorder.m_dataFormat.mFormatID = kAudioFormatLinearPCM;
    aqRecorder.m_dataFormat.mSampleRate = 44100.0;
    aqRecorder.m_dataFormat.mChannelsPerFrame = 2;
    aqRecorder.m_dataFormat.mBitsPerChannel = 16;
    aqRecorder.m_dataFormat.mBytesPerPacket =
        aqRecorder.m_dataFormat.mBytesPerFrame =
            aqRecorder.m_dataFormat.mChannelsPerFrame * sizeof(int16_t);
    aqRecorder.m_dataFormat.mFramesPerPacket = 1;
    aqRecorder.m_dataFormat.mFormatFlags =
        kLinearPCMFormatFlagIsBigEndian |
        kLinearPCMFormatFlagIsSignedInteger |
        kLinearPCMFormatFlagIsPacked;
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

static void registerCallback(DNSServiceRef sdRef,
                             DNSServiceFlags flags,
                             DNSServiceErrorType ec,
                             const char* name,
                             const char* regtype,
                             const char* domain,
                             void* context)
{
    std::cout << "here" << std::endl;
}

static void browseCallback(DNSServiceRef sdRef, 
                           DNSServiceFlags flags, 
                           uint32_t interfaceIndex, 
                           DNSServiceErrorType errorCode, 
                           const char *serviceName, 
                           const char *regtype, 
                           const char *replyDomain, 
                           void *context)
{
    std::cout << serviceName << std::endl;
};

int main(int argc, char **argv)
{
    sppvip::DiscoveredNodeList discoveredNodes;
    sppvip::ServiceDiscoverer srvDiscoverer(discoveredNodes);
    srvDiscoverer.start();

    DNSServiceRef sppvIpRegService;
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, 0, NULL);

    DNSServiceErrorType err =
        DNSServiceRegister(&sppvIpRegService,
                           0,
                           0,
                           NULL,
                           "_sspvip._udp",
                           NULL, NULL,
                           htons(9999),
                           TXTRecordGetLength(&txtRecord),
                           &txtRecord,
                           registerCallback,
                           NULL);


    if (err) {
        // std::cout << "Service Register Error: "
        //     << errorString(err) << std::endl;
        exit(9);
    }

    DNSServiceProcessResult(sppvIpRegService);

    std::mutex browseSrvMutex;
    bool bBrowseTerminated = false;

    std::this_thread::sleep_for(std::chrono::seconds(5));

    if (discoveredNodes.size() > 0) {
        for (itr = discoveredNodes.begin(); itr != discoveredNodes.end(); itr++) {
            if (itr->first == "SPPVIP-Test._sspvip._udp.local.") {
                break;
            }
        }
    }

    std::cout << "out with " << itr->first << std::endl;

    // Recorder
    AQRecoderState aqData;
    initAQRecorder(aqData);

    OSStatus status = AudioQueueNewInput(&aqData.m_dataFormat,
                                         onAudioBuffer,
                                         &aqData,
                                         NULL,
                                         kCFRunLoopCommonModes,
                                         0,
                                         &aqData.m_queue);

    if (status)
    {
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

    if (status)
    {
        std::cout << status << ": Failed creating the file" << std::endl;
        exit(1);
    }

    deriveBufferSize(aqData.m_queue,
                     aqData.m_dataFormat,
                     0.5,
                     &aqData.m_bufferByteSize);

    allocateQueueBuffer(aqData);

    // std::cout << "Press enter to start recording..." << std::endl;

    char input;
    // std::cin >> input;

    aqData.m_currentPacket = 0;
    aqData.m_isRunning = true;
    status = AudioQueueStart(aqData.m_queue, NULL);

    if (status)
    {
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

    // Player
    // const char* filePath = "/Users/daniel.santos/cpp/sppvip/build/test_audio.aiff";
    // CFURLRef audioFileURL =
    //     CFURLCreateFromFileSystemRepresentation (           // 1
    //         NULL,                                           // 2
    //         (const UInt8 *) filePath,                       // 3
    //         strlen (filePath),                              // 4
    //         false                                           // 5
    //     );

    // AudioStreamPlayer asPlayer;
    // OSStatus result = AudioFileOpenURL(audioFileURL, kAudioFileReadPermission, 0, &asPlayer.m_audioFile);
    // std::cout << result << ": AudioFileOpenURL" << std::endl;
    // CFRelease(audioFileURL);

    // uint32_t dataFormatSize = sizeof(asPlayer.m_dataFormat);
    // result = AudioFileGetProperty(asPlayer.m_audioFile,
    //                      kAudioFilePropertyDataFormat,
    //                      &dataFormatSize,
    //                      &asPlayer.m_dataFormat);
    // std::cout << result << ": AudioFileGetProperty" << std::endl;
    // result = AudioQueueNewOutput(&asPlayer.m_dataFormat,
    //                     AudioStreamPlayer::onOutputBuffer,
    //                     &asPlayer,
    //                     CFRunLoopGetCurrent(),
    //                     kCFRunLoopCommonModes,
    //                     0,
    //                     &asPlayer.m_queue);

    // std::cout << result << ": AudioQueueNewOutput" << std::endl;

    // uint32_t maxPacketSize;
    // uint32_t propertySize = sizeof(maxPacketSize);
    // result = AudioFileGetProperty(asPlayer.m_audioFile,
    //                      kAudioFilePropertyPacketSizeUpperBound,
    //                      &propertySize,
    //                      &maxPacketSize);
    // std::cout << result << ": AudioFileGetProperty" << std::endl;
    // asPlayer.deriveOutputBufferSize(maxPacketSize, 0.5);

    // bool isFormatVBR  = (asPlayer.m_dataFormat.mBytesPerPacket == 0 ||
    //                      asPlayer.m_dataFormat.mFramesPerPacket == 0);

    // if (isFormatVBR) {
    //     asPlayer.m_packetDescs = new AudioStreamPacketDescription [asPlayer.m_numPacketsToRead];
    // }
    // else {
    //     asPlayer.m_packetDescs = NULL;
    // }
    // asPlayer.m_isRunning = true;
    // asPlayer.m_currentPacket = 0;
    // for (int i = 0; i < 3; i++) {
    //     AudioQueueAllocateBuffer(asPlayer.m_queue,
    //                              asPlayer.m_bufferByteSize,
    //                              &asPlayer.m_buffers[i]);

    //     AudioStreamPlayer::onOutputBuffer(&asPlayer,
    //                                       asPlayer.m_queue,
    //                                       asPlayer.m_buffers[i]);
    // }

    // float gain = 1;
    // AudioQueueSetParameter(asPlayer.m_queue, kAudioQueueParam_Volume, gain);

    // AudioQueueStart(asPlayer.m_queue, NULL);

    // do {
    //     std::cout << "here" << std::endl;
    //     CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.25, false);
    // } while (asPlayer.m_isRunning);

    // CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1, false);

    // AudioQueueDispose(asPlayer.m_queue, true);

    // AudioFileClose(asPlayer.m_audioFile);

    // delete [] asPlayer.m_packetDescs;

    {
        std::lock_guard<std::mutex> lock(browseSrvMutex);
        bBrowseTerminated = true;
    }

    // TODO: This leaks if the application terminates in a error case
    DNSServiceRefDeallocate(sppvIpRegService);

    return 0;
}
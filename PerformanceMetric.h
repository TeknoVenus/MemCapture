//
// Created by Stephen F on 24/11/22.
//

#pragma once

#include "IMetric.h"

#include <thread>
#include <condition_variable>
#include <map>
#include <mutex>

#include "Procrank.h"


class PerformanceMetric : public IMetric
{
public:
    PerformanceMetric();

    ~PerformanceMetric();

    void StartCollection(std::chrono::seconds frequency) override;

    void StopCollection() override;

    void PrintResults() override;

private:
    void CollectData(std::chrono::seconds frequency);

    void GetDecoderStats();
    void GetAudioHalStats();
    void GetAvSessionStats();

private:
    std::thread mCollectionThread;
    bool mQuit;
    std::condition_variable mCv;
    std::mutex mLock;

    // Decoder raw counters
    int mFrameDrop = 0;
    int mFrameError = 0;
    int mHwError = 0;

    Measurement mAudioLatency;
    Measurement mAvDifference;

    bool mAvSessionInProgress;
    bool mAudioHalExists;
};

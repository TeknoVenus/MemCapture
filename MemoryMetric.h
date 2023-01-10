//
// Created by Stephen F on 24/11/22.
//

#pragma once

#include "IMetric.h"

#include <thread>
#include <condition_variable>
#include <map>
#include <mutex>
#include "Platform.h"

#include "Procrank.h"


class MemoryMetric : public IMetric
{
public:
    MemoryMetric(Platform platform);

    ~MemoryMetric();

    void StartCollection(std::chrono::seconds frequency) override;

    void StopCollection() override;

    void PrintResults() override;

private:
    void CollectData(std::chrono::seconds frequency);

    void GetLinuxMemoryUsage();

    void GetCmaMemoryUsage();

    void GetGpuMemoryUsage();

    void GetContainerMemoryUsage();

    void GetMemoryBandwidth();

private:
    struct cmaMeasurement
    {
        cmaMeasurement(long _size, Measurement &_used, Measurement &unused)
                : sizeKb(_size),
                  Used(std::move(_used)),
                  Unused(std::move(unused))
        {

        }

        long sizeKb;
        Measurement Used;
        Measurement Unused;
    };

    struct memoryFragmentation
    {
        memoryFragmentation(Measurement &_freePages, Measurement &fragmentation)
                : FreePages(std::move(_freePages)),
                  Fragmentation(std::move(fragmentation))
        {

        }

        Measurement FreePages;
        Measurement Fragmentation;
    };

    struct memoryBandwidth
    {
        long maxKBps;
        double maxUsagePercent;

        long averageKBps;
        double averageUsagePercent;
    };

    std::thread mCollectionThread;
    bool mQuit;
    std::condition_variable mCv;
    std::mutex mLock;

    size_t mPageSize;

    std::map<std::string, cmaMeasurement> mCmaMeasurements;
    std::map<std::string, Measurement> mLinuxMemoryMeasurements;
    std::map<pid_t, Measurement> mGpuMemoryUsage;
    std::map<std::string, Measurement> mContainerMeasurements;

    Measurement mCmaFree;
    Measurement mCmaBorrowed;

    memoryBandwidth mMemoryBandwidth;
    bool mMemoryBandwidthSupported;

    // Position in vector reflects order
    std::map<std::string, std::vector<memoryFragmentation>> mMemoryFragmentation;

    Platform mPlatform;

    std::map<std::string, std::string> mCmaNames;

    void CalculateFragmentation();
};

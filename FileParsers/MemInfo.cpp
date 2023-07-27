//
// Created by Stephen F on 27/07/23.
//

#include "MemInfo.h"

#include <fstream>
#include "Log.h"

MemInfo::MemInfo() : mTotal(0), mFree(0), mAvailable(0), mUsed(0), mBuffers(0), mCached(0), mSlab(0), mSReclaimable(0),
                     mSUnreclaimable(0), mSwapTotal(0), mSwapFree(0), mCmaTotal(0), mCmaFree(0)
{
    parseMemInfo();
}

void MemInfo::parseMemInfo()
{
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo) {
        LOG_WARN("Failed to open /proc/meminfo");
        return;
    }

    std::string line;
    long value;
    while (std::getline(meminfo, line)) {
        if (sscanf(line.c_str(), "MemTotal: %ld kB", &value) != 0) {
            mTotal = value;
        } else if (sscanf(line.c_str(), "MemFree: %ld kB", &value) != 0) {
            mFree = value;
        } else if (sscanf(line.c_str(), "MemAvailable: %ld kB", &value) != 0) {
            mAvailable = value;
        } else if (sscanf(line.c_str(), "Buffers: %ld kB", &value) != 0) {
            mBuffers = value;
        } else if (sscanf(line.c_str(), "Cached: %ld kB", &value) != 0) {
            mCached = value;
        } else if (sscanf(line.c_str(), "Slab: %ld kB", &value) != 0) {
            mSlab = value;
        } else if (sscanf(line.c_str(), "SReclaimable: %ld kB", &value) != 0) {
            mSReclaimable = value;
        } else if (sscanf(line.c_str(), "SUnreclaim: %ld kB", &value) != 0) {
            mSUnreclaimable = value;
        } else if (sscanf(line.c_str(), "SwapTotal: %ld kB", &value) != 0) {
            mSwapTotal = value;
        } else if (sscanf(line.c_str(), "SwapFree: %ld kB", &value) != 0) {
            mSwapFree = value;
        } else if (sscanf(line.c_str(), "CmaTotal: %ld kB", &value) != 0) {
            mCmaTotal = value;
        }
    }

    if (mTotal < (mFree + mBuffers + mCached + mSlab)) {
        LOG_WARN("MemTotal too small, something went wrong calculating memory");
        return;
    }

    mUsed = mTotal - (mFree + mBuffers + mCached + mSReclaimable);
}

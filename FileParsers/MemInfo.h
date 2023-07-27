//
// Created by Stephen F on 27/07/23.
//

#pragma once

/**
 * @brief Utility wrapper over the /proc/meminfo file to pull data from it easily
 */
class MemInfo
{
public:
    MemInfo();

    long MemTotalKb() const
    {
        return mTotal;
    }

    long MemFreeKb() const
    {
        return mFree;
    }

    long MemAvailableKb() const
    {
        return mAvailable;
    }

    long MemUsedKb() const
    {
        return mUsed;
    }

    long BuffersKb() const
    {
        return mBuffers;
    }

    long CachedKb() const
    {
        return mCached;
    }

    long SlabKb() const
    {
        return mSlab;
    }

    long SlabReclaimable() const
    {
        return mSReclaimable;
    }

    long SlabUnreclaimable() const
    {
        return mSUnreclaimable;
    }

    long SwapTotal() const
    {
        return mSwapTotal;
    }

    long SwapFree() const
    {
        return mSwapFree;
    }

    long CmaTotal() const
    {
        return mCmaTotal;
    }

    long CmaFree() const
    {
        return mCmaFree;
    }

private:
    void parseMemInfo();


private:
    long mTotal;
    long mFree;
    long mAvailable;
    long mUsed;
    long mBuffers;
    long mCached;
    long mSlab;
    long mSReclaimable;
    long mSUnreclaimable;
    long mSwapTotal;
    long mSwapFree;
    long mCmaTotal;
    long mCmaFree;
};

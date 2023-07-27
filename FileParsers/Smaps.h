//
// Created by Stephen F on 27/07/23.
//

#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <string_view>

/**
 * If smaps_rollup is available, will use that. Otherwise will use smaps and sum everything manually.
 */
class Smaps
{
public:
    Smaps(pid_t pid);

    long Rss() const
    {
        return mRss;
    }

    long Pss() const
    {
        return mPss;
    }

    long Swap() const
    {
        return mSwap;
    }

    long SwapPss() const
    {
        return mSwapPss;
    }

    long Locked() const
    {
        return mLocked;
    }

    long Uss() const
    {
        return mPrivateClean + mPrivateDirty;
    }

    long Vss() const
    {
        return mSize;
    }

private:
    enum class SmapsField
    {
        Pss,
        Rss,
        Swap,
        SwapPss,
        Locked,
        PrivateClean,
        PrivateDirty,
        Size,
        Ignore
    };

private:
    void parseSmaps();

    void parseSmapsRollup();

    std::pair<SmapsField, long> parseSmapsLine(std::string_view line);

private:
    pid_t mPid;

    long mRss;
    long mPss;
    long mSwap;
    long mSwapPss;
    long mLocked;
    long mPrivateClean;
    long mPrivateDirty;
    long mSize;
};

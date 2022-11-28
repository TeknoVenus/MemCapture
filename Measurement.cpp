//
// Created by Stephen F on 22/11/22.
//

#include "Measurement.h"
#include <limits>
#include <utility>
#include <cmath>

Measurement::Measurement(std::string name)
        : mName(std::move(name)),
          mCount(0),
          mMin(std::numeric_limits<double>::max()),
          mMax(std::numeric_limits<double>::min()),
          mAverage(0),
          mTotal(0)
{

}

void Measurement::AddDataPoint(long double value)
{
    if (value < mMin) {
        mMin = value;
    }

    if (value > mMax) {
        mMax = value;
    }

    mTotal += value;
    mCount++;

    mAverage = mTotal / mCount;
}

long double Measurement::GetMin() const
{
    return mMin;
}

int Measurement::GetMinRounded() const
{
    return (int)std::round(mMin);
}

long double Measurement::GetMax() const
{
    return mMax;
}

int Measurement::GetMaxRounded() const
{
    return (int)std::round(mMax);
}

long double Measurement::GetAverage() const
{
    return mAverage;
}

int Measurement::GetAverageRounded() const
{
    return (int)std::round(mAverage);
}

std::string Measurement::GetName() const
{
    return mName;
}

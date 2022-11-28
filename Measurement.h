//
// Created by Stephen F on 22/11/22.
//

#pragma once

#include <string>

class Measurement
{
public:
    explicit Measurement(std::string name);

public:
    void AddDataPoint(long double value);

    long double GetMin() const;
    int GetMinRounded() const;

    long double GetMax() const;
    int GetMaxRounded() const;

    long double GetAverage() const;
    int GetAverageRounded() const;

    std::string GetName() const;

private:
    const std::string mName;

    int mCount;
    long double mMin;
    long double mMax;

    long double mAverage;
    long double mTotal;

};

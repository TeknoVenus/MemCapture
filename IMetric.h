//
// Created by Stephen F on 24/11/22.
//

#ifndef MEMCAPTURE_IMETRIC_H
#define MEMCAPTURE_IMETRIC_H

#include <chrono>

class IMetric
{
public:
    virtual void StartCollection(std::chrono::seconds frequency) = 0;
    virtual void StopCollection() = 0;
    virtual void PrintResults() = 0;
};


#endif //MEMCAPTURE_IMETRIC_H

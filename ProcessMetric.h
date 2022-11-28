//
// Created by Stephen F on 24/11/22.
//

#pragma once

#include "IMetric.h"

#include <thread>
#include <condition_variable>
#include <map>
#include <mutex>
#include <utility>

#include "Procrank.h"


class ProcessMetric : public IMetric
{
public:
    ProcessMetric();

    ~ProcessMetric();

    void StartCollection(std::chrono::seconds frequency) override;

    void StopCollection() override;

    void PrintResults() override;

private:
    void CollectData(std::chrono::seconds frequency);

private:
    struct processMeasurement
    {
        processMeasurement(std::string name, Measurement _pss, Measurement _rss, Measurement _uss)
                : ProcessName(std::move(name)),
                  Pss(std::move(_pss)),
                  Rss(std::move(_rss)),
                  Uss(std::move(_uss))
        {

        }

        std::string ProcessName;
        Measurement Pss;
        Measurement Rss;
        Measurement Uss;
    };

    struct Result
    {
        explicit Result(pid_t _pid, std::string  _name, Measurement  pss, Measurement  rss, Measurement  uss)
            :pid(_pid),
            Name(std::move(_name)),
            Pss(std::move(pss)),
            Rss(std::move(rss)),
            Uss(std::move(uss))
        {

        }


        pid_t pid;
        std::string Name;
        Measurement Pss;
        Measurement Rss;
        Measurement Uss;
    };

    std::thread mCollectionThread;
    bool mQuit;
    std::condition_variable mCv;
    std::mutex mLock;

    std::map<pid_t, processMeasurement> mMeasurements;
};

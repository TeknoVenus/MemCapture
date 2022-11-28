//
// Created by Stephen F on 24/11/22.
//

#include "ProcessMetric.h"
#include "Utils.h"

#include <tabulate/table.hpp>
#include <algorithm>


ProcessMetric::ProcessMetric()
        : mQuit(false),
          mCv()
{

}

ProcessMetric::~ProcessMetric()
{
    if (!mQuit) {
        StopCollection();
    }
}

void ProcessMetric::StartCollection(const std::chrono::seconds frequency)
{
    mQuit = false;
    mCollectionThread = std::thread(&ProcessMetric::CollectData, this, frequency);
    mCollectionThread.detach();
}

void ProcessMetric::StopCollection()
{
    std::unique_lock<std::mutex> locker(mLock);
    mQuit = true;
    mCv.notify_all();
    locker.unlock();

    if (mCollectionThread.joinable()) {
        LOG_INFO("Waiting for ProcessMetric collection thread to terminate");
        mCollectionThread.join();
    }
}

void ProcessMetric::PrintResults()
{
    printf("======== Processes ===========\n");

    tabulate::Table memoryResults;

    memoryResults.add_row({"PID", "Process", "Min_RSS_KB", "Max_RSS_KB", "Average_RSS_KB", "Min_PSS_KB", "Max_PSS_KB", "Average_PSS_KB", "Min_USS_KB", "Max_USS_KB", "Average_USS_KB"});

    for (const auto& result : mMeasurements) {
        memoryResults.add_row({
            std::to_string(result.first),
            result.second.ProcessName,
            std::to_string(result.second.Rss.GetMinRounded()),
            std::to_string(result.second.Rss.GetMaxRounded()),
            std::to_string(result.second.Rss.GetAverageRounded()),
            std::to_string(result.second.Pss.GetMinRounded()),
            std::to_string(result.second.Pss.GetMaxRounded()),
            std::to_string(result.second.Pss.GetAverageRounded()),
            std::to_string(result.second.Uss.GetMinRounded()),
            std::to_string(result.second.Uss.GetMaxRounded()),
            std::to_string(result.second.Uss.GetAverageRounded()),
        });
    }

    Utils::PrintTable(memoryResults);
}

void ProcessMetric::CollectData(const std::chrono::seconds frequency)
{
    std::unique_lock<std::mutex> lock(mLock);

    do {
        LOG_INFO("Collecting process data");
        auto start = std::chrono::high_resolution_clock::now();

        Procrank procrank;
        auto processMemory = procrank.GetMemoryUsage();

        for (const auto &process: processMemory) {

            auto itr = mMeasurements.find(process.pid);
            if (itr != mMeasurements.end()) {
                // Already got a measurement for this PID
                auto &measurement = itr->second;
                measurement.Pss.AddDataPoint(process.memoryUsage.pss / (long double) 1024.0);
                measurement.Rss.AddDataPoint(process.memoryUsage.rss / (long double) 1024.0);
                measurement.Uss.AddDataPoint(process.memoryUsage.uss / (long double) 1024.0);
            } else {
                // Store in KB
                auto pss = Measurement("PSS");
                pss.AddDataPoint(process.memoryUsage.pss / (long double) 1024.0);

                auto rss = Measurement("RSS");
                rss.AddDataPoint(process.memoryUsage.rss / (long double) 1024.0);

                auto uss = Measurement("USS");
                uss.AddDataPoint(process.memoryUsage.uss / (long double) 1024.0);

                processMeasurement measurements(process.name, pss, rss, uss);
                mMeasurements.insert(std::pair<int, processMeasurement>(process.pid, measurements));
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        LOG_INFO("PerformanceMetric completed in %ld us",
                 std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

        // Wait for period before doing collection again, or until cancelled
        mCv.wait_for(lock, frequency);
    } while (!mQuit);

    LOG_INFO("Collection thread quit");
}
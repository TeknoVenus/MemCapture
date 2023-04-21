/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2023 Stephen Foulds
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "ProcessMetric.h"
#include <algorithm>


ProcessMetric::ProcessMetric(std::shared_ptr<ReportGeneratorFactory> reportGeneratorFactory,
                             std::optional<std::shared_ptr<GroupManager>> groupManager)
        : mQuit(false),
          mCv(),
          mReportGeneratorFactory(std::move(reportGeneratorFactory)),
          mGroupManager(std::move(groupManager))
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
    DeduplicateData();

    std::vector<std::string> columns = {"PID", "Process", "Group", "Systemd Service", "Container", "Cmdline",
                                        "Min_RSS_KB",
                                        "Max_RSS_KB", "Average_RSS_KB", "Min_PSS_KB", "Max_PSS_KB", "Average_PSS_KB",
                                        "Min_USS_KB", "Max_USS_KB", "Average_USS_KB"};
    auto memoryResults = mReportGeneratorFactory->getReportGenerator("Process Memory", columns);

    for (const auto &result: mMeasurements) {
        std::optional<std::string> group = std::nullopt;
        if (mGroupManager.has_value()) {
            group = result.ProcessInfo.group(mGroupManager.value());
        }

        memoryResults->addRow({
                                      std::to_string(result.ProcessInfo.pid()),
                                      result.ProcessInfo.name(),
                                      group.has_value() ? group.value() : "Unknown",
                                      result.ProcessInfo.systemdService().has_value()
                                      ? result.ProcessInfo.systemdService().value() : "-",
                                      result.ProcessInfo.container().has_value()
                                      ? result.ProcessInfo.container().value() : "-",
                                      tabulate::Format::word_wrap(result.ProcessInfo.cmdline(), 200, "", false),
                                      std::to_string(result.Rss.GetMinRounded()),
                                      std::to_string(result.Rss.GetMaxRounded()),
                                      std::to_string(result.Rss.GetAverageRounded()),
                                      std::to_string(result.Pss.GetMinRounded()),
                                      std::to_string(result.Pss.GetMaxRounded()),
                                      std::to_string(result.Pss.GetAverageRounded()),
                                      std::to_string(result.Uss.GetMinRounded()),
                                      std::to_string(result.Uss.GetMaxRounded()),
                                      std::to_string(result.Uss.GetAverageRounded()),
                              });
    }

    memoryResults->printReport();
}

void ProcessMetric::CollectData(const std::chrono::seconds frequency)
{
    std::unique_lock<std::mutex> lock(mLock);

    do {
        //LOG_INFO("Collecting process data");
        auto start = std::chrono::high_resolution_clock::now();

        // Use procrank to get the memory usage for all processes in the system at this moment in time
        // Won't capture every spike in memory usage, but over time should smooth out into a decent average
        Procrank procrank;

        // This can take 0.5 - 1 second...
        auto processMemory = procrank.GetMemoryUsage();

        for (const auto &procrankMeasurement: processMemory) {
            auto itr = std::find_if(mMeasurements.begin(), mMeasurements.end(), [&](const processMeasurement &m)
            {
                return m.ProcessInfo == procrankMeasurement.process;
            });

            if (itr != mMeasurements.end()) {
                // Already got a measurement for this PID
                auto &measurement = *itr;
                measurement.Pss.AddDataPoint(procrankMeasurement.memoryUsage.pss / (long double) 1024.0);
                measurement.Rss.AddDataPoint(procrankMeasurement.memoryUsage.rss / (long double) 1024.0);
                measurement.Uss.AddDataPoint(procrankMeasurement.memoryUsage.uss / (long double) 1024.0);
            } else {
                // Store in KB
                auto pss = Measurement("PSS");
                pss.AddDataPoint(procrankMeasurement.memoryUsage.pss / (long double) 1024.0);

                auto rss = Measurement("RSS");
                rss.AddDataPoint(procrankMeasurement.memoryUsage.rss / (long double) 1024.0);

                auto uss = Measurement("USS");
                uss.AddDataPoint(procrankMeasurement.memoryUsage.uss / (long double) 1024.0);

                processMeasurement measurement(procrankMeasurement.process, pss, rss, uss);
                mMeasurements.emplace_back(measurement);
            }
        }

        // Update process dead/alive flag
        for (auto &process: mMeasurements) {
            process.ProcessInfo.updateAliveStatus();
        }

        auto end = std::chrono::high_resolution_clock::now();
        LOG_INFO("ProcessMetric completed in %lld ms",
                 (long long) std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

        // Wait for period before doing collection again, or until cancelled
        mCv.wait_for(lock, frequency);
    } while (!mQuit);

    LOG_INFO("Collection thread quit");
}

/**
 * @brief Analyse the collected data and prevent any duplicate processes
 *
 * For example, if a bash script executed 'sleep 10' once a minute, over an hour capture we'd have 60 instances of
 * sleep 10. Providing the processes have the same parent and cmdline (and the other instances are dead), then remove the duplicates
 *
 * This is really only here to prevent sleep's in some RDK scripts from artificially inflating the results over long runs.
 * In an ideal world we wouldn't need this.
 */
void ProcessMetric::DeduplicateData()
{
    // Warning:: This is quite crude. Can be disabled at runtime if you want to handle this manually later on in Excel/similar
    std::map<std::string, std::vector<processMeasurement>> duplicates;

    for (const auto &measurement: mMeasurements) {
        if (!measurement.ProcessInfo.isDead()) {
            continue;
        }

        auto hasDuplicate = std::count_if(mMeasurements.begin(), mMeasurements.end(),
                                          [&](const processMeasurement &m)
                                          {
                                              // Duplicate processes have the same cmdline and same parent PID (and are dead)
                                              return m.ProcessInfo.isDead() &&
                                                     m.ProcessInfo.cmdline() == measurement.ProcessInfo.cmdline() &&
                                                     m.ProcessInfo.ppid() == measurement.ProcessInfo.ppid();
                                          }) > 1;


        if (hasDuplicate) {
            auto itr = duplicates.find(measurement.ProcessInfo.cmdline());

            if (itr != duplicates.end()) {
                itr->second.emplace_back(measurement);
            } else {
                duplicates.insert(std::make_pair(measurement.ProcessInfo.cmdline(), std::vector<processMeasurement>()));
                duplicates[measurement.ProcessInfo.cmdline()].emplace_back(measurement);
            }
        }
    }

    if (!duplicates.empty()) {
        LOG_INFO("%zu Duplicates", duplicates.size());

        // For simplicity, keep the duplicate that had the highest average and remove the rest
        for (const auto &duplicate: duplicates) {
            // Sort
            auto d = duplicate.second;
            std::sort(d.begin(), d.end(), [](const processMeasurement &a, const processMeasurement &b)
            {
                return a.Pss.GetAverageRounded() < b.Pss.GetAverageRounded();
            });

            // Highest PSS will be last, remove it
            d.pop_back();

            // Remove other duplicates from measurements
            LOG_INFO("Removing %zu duplicates for %s", d.size(), duplicate.first.c_str());

            for (const auto &toRemove: d) {
                mMeasurements.erase(
                        std::remove_if(mMeasurements.begin(), mMeasurements.end(), [&](const processMeasurement &m)
                        {
                            return m.ProcessInfo == toRemove.ProcessInfo;
                        }), mMeasurements.end());
            }
        }
    }
}

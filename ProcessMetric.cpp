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
#include "Utils.h"

#include <tabulate/table.hpp>
#include <algorithm>


ProcessMetric::ProcessMetric(std::shared_ptr<ReportGeneratorFactory> reportGeneratorFactory)
        : mQuit(false),
          mCv(),
          mReportGeneratorFactory(std::move(reportGeneratorFactory))
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
    std::vector<std::string> columns = {"PID", "Process", "Systemd Service", "Container", "Cmdline", "Min_RSS_KB",
                                        "Max_RSS_KB", "Average_RSS_KB", "Min_PSS_KB", "Max_PSS_KB", "Average_PSS_KB",
                                        "Min_USS_KB", "Max_USS_KB", "Average_USS_KB"};
    auto memoryResults = mReportGeneratorFactory->getReportGenerator("Process Memory", columns);

    for (const auto &result : mMeasurements) {
        memoryResults->addRow({
                                      std::to_string(result.first),
                                      result.second.ProcessName,
                                      result.second.SystemdService,
                                      result.second.Container,
                                      tabulate::Format::word_wrap(result.second.Cmdline, 200, "", false),
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

    memoryResults->printReport();
}

void ProcessMetric::CollectData(const std::chrono::seconds frequency)
{
    std::unique_lock<std::mutex> lock(mLock);

    do {
        LOG_INFO("Collecting process data");
        auto start = std::chrono::high_resolution_clock::now();

        // Use procrank to get the memory usage for all processes in the system at this moment in time
        // Won't capture every spike in memory usage, but over time should smooth out into a decent average
        Procrank procrank;

        // This can take 0.5 - 1 second...
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

                std::string systemdService = Utils::getSystemdServiceName(process.pid);
                std::string containerName = Utils::getContainerName(process.pid);
                std::string cmdline = Procrank::GetProcessCmdline(process.pid);

                processMeasurement measurement(process.name, cmdline, systemdService, containerName, pss, rss, uss);
                mMeasurements.insert(std::pair<int, processMeasurement>(process.pid, measurement));
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        LOG_INFO("ProcessMetric completed in %lld us",
                 (long long) std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

        // Wait for period before doing collection again, or until cancelled
        mCv.wait_for(lock, frequency);
    } while (!mQuit);

    LOG_INFO("Collection thread quit");
}
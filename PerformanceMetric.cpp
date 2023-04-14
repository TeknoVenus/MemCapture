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

#include "PerformanceMetric.h"
#include <thread>
#include <fstream>
#include <filesystem>
#include <tabulate/table.hpp>
#include <stdio.h>

#include "Utils.h"

PerformanceMetric::PerformanceMetric(Platform platform, std::shared_ptr<ReportGeneratorFactory> reportGeneratorFactory)
        : mQuit(false),
          mCv(),
          mAudioLatency("Audio Latency"),
          mAvDifference("AV Difference"),
          mAvSessionInProgress(false),
          mAudioHalExists(false),
          mPlatform(platform),
          mReportGeneratorFactory(std::move(reportGeneratorFactory)),
          mSupportedPlatform(false)
{
    if (mPlatform != Platform::AMLOGIC) {
        LOG_WARN("Performance metrics only supported on Amlogic for now");
        mSupportedPlatform = false;
    } else {
        mSupportedPlatform = true;
    }

    if (std::filesystem::exists("/usr/bin/hal_dump")) {
        mAudioHalExists = true;
    } else {
        mAudioHalExists = false;
    }
}

PerformanceMetric::~PerformanceMetric()
{
    if (!mQuit) {
        StopCollection();
    }
}

void PerformanceMetric::StartCollection(const std::chrono::seconds frequency)
{
    if (mSupportedPlatform) {
        mQuit = false;
        mCollectionThread = std::thread(&PerformanceMetric::CollectData, this, frequency);
    }
}

void PerformanceMetric::StopCollection()
{
    std::unique_lock<std::mutex> locker(mLock);
    mQuit = true;
    mCv.notify_all();
    locker.unlock();

    if (mCollectionThread.joinable()) {
        LOG_INFO("Waiting for PerformanceMetric collection thread to terminate");
        mCollectionThread.join();
    }
}

void PerformanceMetric::CollectData(std::chrono::seconds frequency)
{
    std::unique_lock<std::mutex> lock(mLock);

    do {
        auto start = std::chrono::high_resolution_clock::now();

        GetDecoderStats();
        GetAudioHalStats();
        GetAvSessionStats();

        auto end = std::chrono::high_resolution_clock::now();
        LOG_INFO("PerformanceMetric completed in %lld us",
                 (long long) std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

        // Wait for period before doing collection again, or until cancelled
        mCv.wait_for(lock, frequency);
    } while (!mQuit);

    LOG_INFO("Collection thread quit");
}

void PerformanceMetric::PrintResults()
{
    if (!mSupportedPlatform) {
        return;
    }

    auto decoderResults = mReportGeneratorFactory->getReportGenerator("Video Decoder", {"Statistic", "Value"});
    decoderResults->addRow({"Frame Drop Count", std::to_string(mFrameDrop)});
    decoderResults->addRow({"Frame Error Count", std::to_string(mFrameError)});
    decoderResults->addRow({"HW Error Count", std::to_string(mHwError)});

    if (mAvSessionInProgress) {
        auto sessionResults = mReportGeneratorFactory->getReportGenerator("A/V Session", {"Statistic", "Min_ms", "Max_ms", "Average_ms"});
        sessionResults->addRow({"A/V Sync Difference",
                                std::to_string(mAvDifference.GetMin()),
                                std::to_string(mAvDifference.GetMax()),
                                std::to_string(mAvDifference.GetAverage())
                               });

        sessionResults->printReport();
    }

    if (mAudioHalExists) {
        auto audioHalResults = mReportGeneratorFactory->getReportGenerator("Audio HAL", {"Statistic", "Min_ms", "Max_ms", "Average_ms"});
        audioHalResults->addRow({"Total Latency",
                                 std::to_string(mAudioLatency.GetMin()),
                                 std::to_string(mAudioLatency.GetMax()),
                                 std::to_string(mAudioLatency.GetAverage())
                                });
        audioHalResults->printReport();
    }
}

void PerformanceMetric::GetDecoderStats()
{
    std::ifstream decoderStatusFile("/sys/class/vdec/vdec_status");

    if (!decoderStatusFile) {
        LOG_WARN("No vdec_status file");
        return;
    }

    // I'm not convinced how accurate the values reported here are, but get them anyway

    std::string line;
    int metricValue;
    while (std::getline(decoderStatusFile, line)) {
        if (sscanf(line.c_str(), "   drop count : %d", &metricValue) != 0) {
            mFrameDrop = metricValue;
        } else if (sscanf(line.c_str(), "fra err count : %d", &metricValue) != 0) {
            mFrameError = metricValue;
        } else if (sscanf(line.c_str(), " hw err count : %d", &metricValue) != 0) {
            mHwError = metricValue;
        }
    }
}

void PerformanceMetric::GetAudioHalStats()
{
    if (!mAudioHalExists) {
        LOG_WARN("hal_dump binary not found");
        return;
    }

    FILE *fp = popen("/usr/bin/hal_dump", "r");
    if (!fp) {
        LOG_WARN("Failed to execute hal_dump");
        return;
    }

    // hal_dump reports many stats, the one we care about is the total audio latency of the system

    char *line = nullptr;
    size_t len = 0;

    int latency;
    while ((getline(&line, &len, fp)) != -1) {
        if (sscanf(line, "[AML_HAL]      audio total latency         :    %d ms", &latency) != 0) {
            mAudioLatency.AddDataPoint(latency);
        }
    }

    pclose(fp);
    if (line) {
        free(line);
    }
}

void PerformanceMetric::GetAvSessionStats()
{
    std::ifstream avSessionFile("/sys/class/avsync_session0/session_stat");

    // This file will only exist during playback, but that's expected
    if (!avSessionFile) {
        LOG_WARN("No A/V session in progress");
        return;
    }

    mAvSessionInProgress = true;

    // This will tell us the difference between the video and audio in milliseconds (>100ms either way is noticeable as a lip sync issue)

    std::string line;
    int avDifference;
    while (std::getline(avSessionFile, line)) {
        if (sscanf(line.c_str(), "diff-ms a-w %*d v-w %*d a-v %d", &avDifference) != 0) {
            mAvDifference.AddDataPoint(avDifference);
        }
    }
}


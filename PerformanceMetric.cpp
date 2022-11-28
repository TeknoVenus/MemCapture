//
// Created by Stephen F on 24/11/22.
//

#include "PerformanceMetric.h"
#include <thread>
#include <fstream>
#include <filesystem>
#include <tabulate/table.hpp>
#include <climits>
#include <stdio.h>

#include "Utils.h"

PerformanceMetric::PerformanceMetric()
        : mQuit(false),
          mCv(),
          mAudioLatency("Audio Latency"),
          mAvDifference("AV Difference"),
          mAvSessionInProgress(false)
{
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
    mQuit = false;
    mCollectionThread = std::thread(&PerformanceMetric::CollectData, this, frequency);
    mCollectionThread.detach();
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
        LOG_INFO("PerformanceMetric completed in %ld us",
                 std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

        // Wait for period before doing collection again, or until cancelled
        mCv.wait_for(lock, frequency);
    } while (!mQuit);

    LOG_INFO("Collection thread quit");
}

void PerformanceMetric::PrintResults()
{
    tabulate::Table decoderResults;
    tabulate::Table sessionResults;
    tabulate::Table audioHalResults;

    printf("======== Video Decoder ===========\n");

    decoderResults.add_row({"Statistic", "Value"});
    decoderResults.add_row({"Frame Drop Count", std::to_string(mFrameDrop)});
    decoderResults.add_row({"Frame Error Count", std::to_string(mFrameError)});
    decoderResults.add_row({"HW Error Count", std::to_string(mHwError)});

    Utils::PrintTable(decoderResults);

    printf("\n======== A/V Session ===========\n");

    if (mAvSessionInProgress) {
        sessionResults.add_row({"Statistic", "Min_ms", "Max_ms", "Average_ms"});
        sessionResults.add_row({"A/V Sync Difference",
                                std::to_string(mAvDifference.GetMin()),
                                std::to_string(mAvDifference.GetMax()),
                                std::to_string(mAvDifference.GetAverage())
                               });

        Utils::PrintTable(sessionResults);
    } else {
        printf("No A/V session in progress\n");
    }

    printf("\n======== Audio HAL ===========\n");

    if (mAudioHalExists) {
        audioHalResults.add_row({"Statistic", "Min_ms", "Max_ms", "Average_ms"});
        audioHalResults.add_row({"Total Latency",
                                 std::to_string(mAudioLatency.GetMin()),
                                 std::to_string(mAudioLatency.GetMax()),
                                 std::to_string(mAudioLatency.GetAverage())
                                });
    } else {
        printf("Could not find hal_dump binary\n");
    }

    Utils::PrintTable(audioHalResults);
}

void PerformanceMetric::GetDecoderStats()
{
    std::ifstream decoderStatusFile("/sys/class/vdec/vdec_status");

    if (!decoderStatusFile) {
        LOG_WARN("No vdec_status file");
        return;
    }

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
    if (!mAudioHalExists)
    {
        LOG_WARN("hal_dump binary not found");
        return;
    }

    FILE *fp = popen("/usr/bin/hal_dump", "r");
    if (!fp) {
        LOG_WARN("Failed to execute hal_dump");
        return;
    }

    char *line = nullptr;
    size_t len = 0;

    int latency;
    while ((getline(&line, &len, fp)) != -1) {
        if (sscanf(line, "[AML_HAL]      audio total latency         :    %d ms", &latency) != 0) {
            mAudioLatency.AddDataPoint(latency);
        }
    }

    fclose(fp);
    if (line) {
        free(line);
    }
}

void PerformanceMetric::GetAvSessionStats()
{
    std::ifstream avSessionFile("/sys/class/avsync_session0/session_stat");

    if (!avSessionFile) {
        LOG_WARN("No A/V session in progress");
        return;
    }

    mAvSessionInProgress = true;

    std::string line;
    int avDifference;
    while (std::getline(avSessionFile, line)) {
        if (sscanf(line.c_str(), "diff-ms a-w %*d v-w %*d a-v %d", &avDifference) != 0) {
            mAvDifference.AddDataPoint(avDifference);
        }
    }
}


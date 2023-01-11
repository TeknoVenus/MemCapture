//
// Created by Stephen F on 24/11/22.
//

#include "MemoryMetric.h"
#include <thread>
#include <fstream>
#include <filesystem>
#include <tabulate/table.hpp>
#include <cmath>

#include "Utils.h"

MemoryMetric::MemoryMetric(Platform platform)
        : mQuit(false),
          mCv(),
          mLinuxMemoryMeasurements{},
          mCmaFree("CmaFree"),
          mCmaBorrowed("CmaBorrowedKernel"),
          mMemoryBandwidth({0, 0, 0, 0}),
          mMemoryFragmentation{},
          mPlatform(platform)
{

    mPageSize = sysconf(_SC_PAGESIZE);

    if (platform == Platform::AMLOGIC) {
        mCmaNames = {
                std::make_pair("cma-0", "secmon_reserved"),
                std::make_pair("cma-1", "logo_reserved"),
                std::make_pair("cma-2", "codec_mm_cma"),
                std::make_pair("cma-3", "ion_cma_reserved"),
                std::make_pair("cma-4", "vdin1_cma_reserved"),
                std::make_pair("cma-5", "demod_cma_reserved"),
                std::make_pair("cma-6", "kernel_reserved")
        };
    } else if (platform == Platform::REALTEK) {
        mCmaNames = {
                std::make_pair("cma-0", "cma-0"),
                std::make_pair("cma-1", "cma-1"),
                std::make_pair("cma-2", "cma-2"),
                std::make_pair("cma-3", "cma-3"),
                std::make_pair("cma-4", "cma-4"),
                std::make_pair("cma-5", "cma-5"),
                std::make_pair("cma-6", "cma-6"),
                std::make_pair("cma-7", "cma-7"),
                std::make_pair("cma-8", "cma-8"),
        };
    }

    // Create static measurements - store in KB
    Measurement total("Total");
    mLinuxMemoryMeasurements.insert(std::make_pair(total.GetName(), total));

    Measurement used("Used");
    mLinuxMemoryMeasurements.insert(std::make_pair(used.GetName(), used));

    Measurement buffered("Buffered");
    mLinuxMemoryMeasurements.insert(std::make_pair(buffered.GetName(), buffered));

    Measurement cached("Cached");
    mLinuxMemoryMeasurements.insert(std::make_pair(cached.GetName(), cached));

    Measurement free("Free");
    mLinuxMemoryMeasurements.insert(std::make_pair(free.GetName(), free));

    Measurement available("Available");
    mLinuxMemoryMeasurements.insert(std::make_pair(available.GetName(), available));

    Measurement slabTotal("SlabTotal");
    mLinuxMemoryMeasurements.insert(std::make_pair(slabTotal.GetName(), slabTotal));

    Measurement slabReclaimable("SlabReclaimable");
    mLinuxMemoryMeasurements.insert(std::make_pair(slabReclaimable.GetName(), slabReclaimable));

    Measurement slabUnreclaimable("SlabUnreclaimable");
    mLinuxMemoryMeasurements.insert(
            std::make_pair(slabUnreclaimable.GetName(), slabUnreclaimable));

    if (platform == Platform::AMLOGIC) {
        mMemoryBandwidthSupported = true;
        // Enable memory bandwidth monitoring
        std::ofstream ddrMode("/sys/class/aml_ddr/mode", std::ios::binary);
        ddrMode << "1";
    } else if (platform == Platform::REALTEK) {
        mMemoryBandwidthSupported = false;
    }

}

MemoryMetric::~MemoryMetric()
{
    if (!mQuit) {
        StopCollection();
    }

    // Disable memory bandwidth monitoring
    std::ofstream ddrMode("/sys/class/aml_ddr/mode", std::ios::binary);
    ddrMode << "0";
}

void MemoryMetric::StartCollection(const std::chrono::seconds frequency)
{
    mQuit = false;
    mCollectionThread = std::thread(&MemoryMetric::CollectData, this, frequency);
    mCollectionThread.detach();
}

void MemoryMetric::StopCollection()
{
    std::unique_lock<std::mutex> locker(mLock);
    mQuit = true;
    mCv.notify_all();
    locker.unlock();

    if (mCollectionThread.joinable()) {
        LOG_INFO("Waiting for MemoryMetric collection thread to terminate");
        mCollectionThread.join();
    }
}

void MemoryMetric::CollectData(std::chrono::seconds frequency)
{
    std::unique_lock<std::mutex> lock(mLock);

    do {

        auto start = std::chrono::high_resolution_clock::now();

        GetLinuxMemoryUsage();
        GetCmaMemoryUsage();
        GetGpuMemoryUsage();
        GetContainerMemoryUsage();
        GetMemoryBandwidth();
        CalculateFragmentation();

        auto end = std::chrono::high_resolution_clock::now();
        LOG_INFO("MemoryMetric completed in %ld us",
                 std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

        // Wait for period before doing collection again, or until cancelled
        mCv.wait_for(lock, frequency);
    } while (!mQuit);

    LOG_INFO("Collection thread quit");
}

void MemoryMetric::PrintResults()
{
    tabulate::Table memoryResults;
    tabulate::Table gpuResults;
    tabulate::Table cmaResults;
    tabulate::Table cmaSummary;
    tabulate::Table containerResults;
    tabulate::Table memoryBandwidth;

    printf("======== Linux Memory ===========\n");

    memoryResults.add_row({"Value", "Min_KB", "Max_KB", "Average_KB"});

    for (const auto &result: mLinuxMemoryMeasurements) {
        memoryResults.add_row({
                                      result.first,
                                      std::to_string(result.second.GetMinRounded()),
                                      std::to_string(result.second.GetMaxRounded()),
                                      std::to_string(result.second.GetAverageRounded()),
                              });
    }

    Utils::PrintTable(memoryResults);

    printf("\n======== GPU Memory ===========\n");


    gpuResults.add_row({"PID", "Process", "Min_KB", "Max_KB", "Average_KB"});

    for (const auto &result: mGpuMemoryUsage) {
        gpuResults.add_row({
                                   std::to_string(result.first),
                                   result.second.GetName(),
                                   std::to_string(result.second.GetMinRounded()),
                                   std::to_string(result.second.GetMaxRounded()),
                                   std::to_string(result.second.GetAverageRounded()),
                           });
    }

    Utils::PrintTable(gpuResults);

    printf("\n======== CMA Memory ===========\n");

    cmaResults.add_row(
            {"Region", "Size_KB", "Used_Min_KB", "Used_Max_KB", "Used_Average_KB", "Unused_Min_KB", "Unused_Max_KB",
             "Unused_Average_KB"});

    for (const auto &result: mCmaMeasurements) {
        cmaResults.add_row({
                                   result.first,
                                   std::to_string(result.second.sizeKb),
                                   std::to_string(result.second.Used.GetMinRounded()),
                                   std::to_string(result.second.Used.GetMaxRounded()),
                                   std::to_string(result.second.Used.GetAverageRounded()),
                                   std::to_string(result.second.Unused.GetMinRounded()),
                                   std::to_string(result.second.Unused.GetMaxRounded()),
                                   std::to_string(result.second.Unused.GetAverageRounded())
                           });
    }

    Utils::PrintTable(cmaResults);

    cmaSummary.add_row({"", "Min_KB", "Max_KB", "Average_KB"});
    cmaSummary.add_row({
                               "CMA Free",
                               std::to_string(mCmaFree.GetMinRounded()),
                               std::to_string(mCmaFree.GetMaxRounded()),
                               std::to_string(mCmaFree.GetAverageRounded()),
                       });

    cmaSummary.add_row({
                               "CMA Borrowed by Kernel",
                               std::to_string(mCmaBorrowed.GetMinRounded()),
                               std::to_string(mCmaBorrowed.GetMaxRounded()),
                               std::to_string(mCmaBorrowed.GetAverageRounded()),
                       });

    printf("\n");
    Utils::PrintTable(cmaSummary);

    printf("\n======== Container Memory ===========\n");

    containerResults.add_row(
            {"Container", "Used_Min_KB", "Used_Max_KB", "Used_Average_KB"});

    for (const auto &result: mContainerMeasurements) {
        containerResults.add_row({
                                         result.first,
                                         std::to_string(result.second.GetMinRounded()),
                                         std::to_string(result.second.GetMaxRounded()),
                                         std::to_string(result.second.GetAverageRounded())
                                 });
    }

    Utils::PrintTable(containerResults);

    printf("\n======== Memory Bandwidth ===========\n");

    if (mMemoryBandwidthSupported) {
        memoryBandwidth.add_row(
                {"", "Bandwidth_KB/s", "Usage_%"});

        memoryBandwidth.add_row(
                {"Max", std::to_string(mMemoryBandwidth.maxKBps), std::to_string(mMemoryBandwidth.maxUsagePercent)});
        memoryBandwidth.add_row({"Average", std::to_string(mMemoryBandwidth.averageKBps),
                                 std::to_string(mMemoryBandwidth.averageUsagePercent)});

        Utils::PrintTable(memoryBandwidth);
    } else {
        printf("Not supported\n");
    }


    for (const auto &memoryZone: mMemoryFragmentation) {
        tabulate::Table memoryFragmentation;

        printf("\n======== Memory Fragmentation - %s ===========\n", memoryZone.first.c_str());

        memoryFragmentation.add_row(
                {"Order", "Min_Free_Pages", "Max_Free_Pages", "Average_Free_Pages", "Min_Fragmentation_%",
                 "Max_Fragmentation_%", "Average_Fragmentation_%"});

        int i = 0;
        for (const auto &measurement: memoryZone.second) {
            memoryFragmentation.add_row(
                    {std::to_string(i),
                     std::to_string(measurement.FreePages.GetMinRounded()),
                     std::to_string(measurement.FreePages.GetMaxRounded()),
                     std::to_string(measurement.FreePages.GetAverageRounded()),
                     std::to_string(measurement.Fragmentation.GetMin() * 100),
                     std::to_string(measurement.Fragmentation.GetMax() * 100),
                     std::to_string(measurement.Fragmentation.GetAverage() * 100)
                    });
            i++;
        }

        Utils::PrintTable(memoryFragmentation);
    }
}

void MemoryMetric::GetLinuxMemoryUsage()
{
    LOG_INFO("Getting memory usage");

    std::ifstream meminfo("/proc/meminfo");

    if (!meminfo) {
        LOG_WARN("Failed to open /proc/meminfo");
        return;
    }

    long memTotal = 0;
    long memUsed = 0;
    long memBuffered = 0;
    long memCached = 0;
    long memFree = 0;
    long memAvailable = 0;
    long memSlabTotal = 0;
    long memSlabReclaimable = 0;
    long memSlabUnreclaimable = 0;

    std::string line;
    long value;

    while (std::getline(meminfo, line)) {
        if (sscanf(line.c_str(), "MemTotal: %ld kB", &value) != 0) {
            memTotal = value;
        } else if (sscanf(line.c_str(), "MemFree: %ld kB", &value) != 0) {
            memFree = value;
        } else if (sscanf(line.c_str(), "MemAvailable: %ld kB", &value) != 0) {
            memAvailable = value;
        } else if (sscanf(line.c_str(), "Buffers: %ld kB", &value) != 0) {
            memBuffered = value;
        } else if (sscanf(line.c_str(), "Cached: %ld kB", &value) != 0) {
            memCached = value;
        } else if (sscanf(line.c_str(), "Slab: %ld kB", &value) != 0) {
            memSlabTotal = value;
        } else if (sscanf(line.c_str(), "SReclaimable: %ld kB", &value) != 0) {
            memSlabReclaimable = value;
        } else if (sscanf(line.c_str(), "SUnreclaim: %ld kB", &value) != 0) {
            memSlabUnreclaimable = value;
        }
    }

    if (memTotal < (memFree + memBuffered + memCached + memSlabTotal)) {
        LOG_WARN("MemTotal too small, something went wrong calculating memory");
        return;
    }

    memUsed = memTotal - (memFree + memBuffered + memCached + memSlabReclaimable);

    mLinuxMemoryMeasurements.at("Total").AddDataPoint(memTotal);
    mLinuxMemoryMeasurements.at("Used").AddDataPoint(memUsed);
    mLinuxMemoryMeasurements.at("Buffered").AddDataPoint(memBuffered);
    mLinuxMemoryMeasurements.at("Cached").AddDataPoint(memCached);
    mLinuxMemoryMeasurements.at("Free").AddDataPoint(memFree);
    mLinuxMemoryMeasurements.at("Available").AddDataPoint(memAvailable);
    mLinuxMemoryMeasurements.at("SlabTotal").AddDataPoint(memSlabTotal);
    mLinuxMemoryMeasurements.at("SlabReclaimable").AddDataPoint(memSlabReclaimable);
    mLinuxMemoryMeasurements.at("SlabUnreclaimable").AddDataPoint(memSlabUnreclaimable);
}

void MemoryMetric::GetCmaMemoryUsage()
{
    LOG_INFO("Getting CMA memory usage");

    long double countKb = 0;
    long double usedKb = 0;
    long double unusedKb = 0;

    long double cmaTotalKb = 0;
    long double cmaTotalUsed = 0;

    // Start by getting CMA breakdown
    try {
        for (const auto &dirEntry: std::filesystem::directory_iterator(
                "/sys/kernel/debug/cma")) {

            // Read CMA metrics
            auto countFile = std::ifstream(dirEntry.path() / "count");
            countFile >> countKb;
            countKb = (countKb * mPageSize) / (long double) 1024;

            auto usedPagesFile = std::ifstream(dirEntry.path() / "used");
            usedPagesFile >> usedKb;
            usedKb = (usedKb * mPageSize) / (long double) 1024;

            unusedKb = countKb - usedKb;

            // Calculate some totals
            cmaTotalKb += countKb;
            cmaTotalUsed += usedKb;

            std::string cmaName;
            try {
                cmaName = mCmaNames.at(dirEntry.path().filename());
            }
            catch (const std::exception &ex) {
                LOG_ERROR("Could not find CMA name for directory %s", dirEntry.path().filename().string().c_str());
                break;
            }


            // Add to measurements
            auto itr = mCmaMeasurements.find(cmaName);

            if (itr != mCmaMeasurements.end()) {
                auto &measurement = itr->second;

                measurement.sizeKb = countKb;
                measurement.Used.AddDataPoint(usedKb);
                measurement.Unused.AddDataPoint(unusedKb);
            } else {
                auto used = Measurement("Used");
                used.AddDataPoint(usedKb);

                auto unused = Measurement("Unused");
                unused.AddDataPoint(unusedKb);

                auto measurement = cmaMeasurement(countKb, used, unused);
                mCmaMeasurements.insert(std::make_pair(cmaName, measurement));
            }
        }

        // Work out how much CMA is borrowed by the kernel
        std::ifstream meminfo("/proc/meminfo");

        if (!meminfo) {
            LOG_WARN("Failed to open /proc/meminfo");
            return;
        }

        long double totalUnused = cmaTotalKb - cmaTotalUsed;
        int cmaFree = 0;
        std::string line;
        while (std::getline(meminfo, line)) {
            if (sscanf(line.c_str(), "CmaFree: %d kB", &cmaFree) != 0) {
                mCmaFree.AddDataPoint(cmaFree);
            }
        }

        long double borrowed = totalUnused - cmaFree;
        mCmaBorrowed.AddDataPoint(borrowed);
    } catch (std::filesystem::filesystem_error &error) {
        LOG_WARN("Failed to open CMA debug with error %s", error.what());
    }
}

void MemoryMetric::GetGpuMemoryUsage()
{
    LOG_INFO("Getting GPU memory usage");

    std::ifstream gpuMem("/sys/kernel/debug/mali0/gpu_memory");

    if (!gpuMem) {
        LOG_WARN("Could not open gpu_memory file");
        return;
    }

    std::string line;
    long gpuPages;
    pid_t pid;

    if (mPlatform == Platform::AMLOGIC) {
        // Amlogic example
        /* root@sky-llama-panel:~# cat /sys/kernel/debug/mali0/gpu_memory
            mali0            total used_pages      25939
            ----------------------------------------------------
            kctx             pid              used_pages
            ----------------------------------------------------
            f1dbf000      14880       4558
            f1c19000      14438        135
            f1bb1000      14292      16359
            f18c0000      10899       4887
         */
        while (std::getline(gpuMem, line)) {
            if (sscanf(line.c_str(), "%*x %d %ld", &pid, &gpuPages) != 0) {
                unsigned long gpuBytes = gpuPages * mPageSize;

                auto itr = mGpuMemoryUsage.find(pid);

                if (itr != mGpuMemoryUsage.end()) {
                    // Already got a measurement for this PID
                    auto &measurement = itr->second;
                    measurement.AddDataPoint(gpuBytes / (long double) 1024.0);
                } else {
                    std::string processName;
                    Procrank::GetProcessName(pid, processName);

                    Measurement measurement(processName);
                    measurement.AddDataPoint(gpuBytes / (long double) 1024.0);
                    mGpuMemoryUsage.insert(std::make_pair(pid, measurement));
                }
            }
        }
    } else if (mPlatform == Platform::REALTEK) {
        // Realtek example
        // First column = pages, second column = PID
        /* root@skyxione:/sys/kernel/debug/mali0# cat gpu_memory
        mali0                  45605
          kctx-0xfa847000      14102      15898
          kctx-0xf7953000         42      15833
          kctx-0xff0b0000       3316       9134
          kctx-0xfec18000      20929       8344
          kctx-0xfb9df000        135       6235
          kctx-0xfb12e000       7081       4962
        */
        while (std::getline(gpuMem, line)) {
            if (sscanf(line.c_str(), "  kctx-0x%*x %ld %d", &gpuPages, &pid) != 0) {
                unsigned long gpuBytes = gpuPages * mPageSize;

                auto itr = mGpuMemoryUsage.find(pid);

                if (itr != mGpuMemoryUsage.end()) {
                    // Already got a measurement for this PID
                    auto &measurement = itr->second;
                    measurement.AddDataPoint(gpuBytes / (long double) 1024.0);
                } else {
                    std::string processName;
                    Procrank::GetProcessName(pid, processName);

                    Measurement measurement(processName);
                    measurement.AddDataPoint(gpuBytes / (long double) 1024.0);
                    mGpuMemoryUsage.insert(std::make_pair(pid, measurement));
                }
            }
        }
    }


}

void MemoryMetric::GetContainerMemoryUsage()
{
    LOG_INFO("Getting Container memory usage");

    long double memoryUsageKb = 0;
    for (const auto &dirEntry: std::filesystem::directory_iterator(
            "/sys/fs/cgroup/memory")) {
        if (!dirEntry.is_directory()) {
            continue;
        }

        auto containerName = dirEntry.path().filename().string();

        auto memoryUsageFile = std::ifstream(dirEntry.path() / "memory.usage_in_bytes");
        memoryUsageFile >> memoryUsageKb;
        memoryUsageKb /= (long double) 1024.0;

        auto itr = mContainerMeasurements.find(containerName);

        if (itr != mContainerMeasurements.end()) {
            auto &measurement = itr->second;
            measurement.AddDataPoint(memoryUsageKb);
        } else {
            Measurement measurement(containerName);
            measurement.AddDataPoint(memoryUsageKb);
            mContainerMeasurements.insert(std::make_pair(containerName, measurement));
        }
    }
}

void MemoryMetric::GetMemoryBandwidth()
{
    LOG_INFO("Getting memory bandwidth usage");

    if (mMemoryBandwidthSupported) {
        if (mPlatform == Platform::AMLOGIC) {
            std::ifstream memBandwidthFile("/sys/class/aml_ddr/usage_stat");

            if (!memBandwidthFile) {
                LOG_WARN("Cannot get DDR usage");
                return;
            }

            std::string line;
            long kbps = 0;
            double percent = 0;

            // Know the data we need is in the first two lines, save effort by only reading those lines
            int i = 0;
            while (std::getline(memBandwidthFile, line) && i < 2) {
                if (sscanf(line.c_str(), "MAX bandwidth:  %ld KB/s, usage: %lf%%, tick:%*d us", &kbps, &percent) != 0) {
                    mMemoryBandwidth.maxKBps = kbps;
                    mMemoryBandwidth.maxUsagePercent = percent;
                } else if (
                        sscanf(line.c_str(), "AVG bandwidth:  %ld KB/s, usage: %lf%%, samples:%*d", &kbps, &percent) !=
                        0) {
                    mMemoryBandwidth.averageKBps = kbps;
                    mMemoryBandwidth.averageUsagePercent = percent;
                }
                i++;
            }
        }
    } else {
        // DDR bandwidth not supported
        return;
    }


}

void MemoryMetric::CalculateFragmentation()
{
    LOG_INFO("Getting memory fragmentation");

    std::ifstream buddyInfo("/proc/buddyinfo");

    if (!buddyInfo) {
        LOG_WARN("Could not open buddyinfo");
        return;
    }

    std::string line;
    std::string segment;
    // Get fragmentation for all zones
    while (std::getline(buddyInfo, line)) {
        std::stringstream lineStream(line);
        std::vector<std::string> segments;
        // Split line on space
        while (std::getline(lineStream, segment, ' ')) {
            if (!segment.empty()) {
                segments.emplace_back(segment);
            }
        }

        std::string zoneName = segments[3];
        std::map<int, int> freePages;
        std::map<int, double> fragmentationPercent;

        size_t columnCount = 0;
        if (mPlatform == Platform::AMLOGIC) {
            columnCount = 15;
        } else if (mPlatform == Platform::REALTEK) {
            columnCount = 17;
        }

        if (segments.size() != columnCount) {
            LOG_WARN("Failed to parse buddyinfo - invalid number of columns (got %zd, expected %zd)", segments.size(), columnCount);
        } else {
            // Calculate fragmentation % for this node
            int totalFreePages = 0;

            //  Get all free page values, and work out total free pages
            for (int i = 4; i < (int) columnCount; i++) {
                int order = i - 4;

                int freeCount = std::stoi(segments[i]);
                totalFreePages += std::pow(2, order) * freeCount;
                freePages[order] = freeCount;
            }

            // Now find out the fragmentation percentages (see https://www.stb.bskyb.com/confluence/display/2016/Memory+Fragmentation)
            double fragPercentage;
            for (int i = 0; i < (int) freePages.size(); i++) {
                fragPercentage = 0;

                // Seems inefficient...
                for (int j = i; j < (int) freePages.size(); j++) {
                    fragPercentage += (std::pow(2, j)) * freePages[j];
                }
                fragPercentage = (totalFreePages - fragPercentage) / totalFreePages;
                fragmentationPercent[i] = fragPercentage;
            }

            // Update measurements
            auto itr = mMemoryFragmentation.find(zoneName);
            if (itr != mMemoryFragmentation.end()) {
                auto &measurements = itr->second;

                for (int i = 0; i < (int) freePages.size(); i++) {
                    measurements[i].FreePages.AddDataPoint(freePages[i]);
                    measurements[i].Fragmentation.AddDataPoint(fragmentationPercent[i]);
                }
            } else {
                std::vector<memoryFragmentation> measurements = {};
                for (int i = 0; i < (int) freePages.size(); i++) {
                    Measurement fp("FreePages");
                    fp.AddDataPoint(freePages[i]);

                    Measurement frag("Fragmentation");
                    frag.AddDataPoint(fragmentationPercent[i]);
                    memoryFragmentation fragMeasurement(fp, frag);
                    measurements.emplace_back(fragMeasurement);
                }

                mMemoryFragmentation.insert(std::make_pair(zoneName, measurements));
            }
        }
    }
}
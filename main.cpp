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

#include <unistd.h>
#include <getopt.h>

#include "Platform.h"
#include "Log.h"
#include "ProcessMetric.h"
#include "MemoryMetric.h"
#include "PerformanceMetric.h"

static int gDuration = 30;
static Platform gPlatform = Platform::AMLOGIC;

static void displayUsage()
{
    printf("Usage: MemCapture <option(s)>\n");
    printf("    Utility to capture memory statistics\n\n");
    printf("    -h, --help          Print this help and exit\n");
    printf("    -d, --duration      Amount of time (in seconds) to capture data for. Default 30 seconds\n");
    printf("    -p, --platform      Platform we're running on. Supported options = ['AMLOGIC', 'REALTEK', 'BROADCOM']. Defaults to Amlogic\n");
}

static void parseArgs(const int argc, char **argv)
{
    struct option longopts[] = {
            {"help",     no_argument,       nullptr, (int) 'h'},
            {"duration", required_argument, nullptr, (int) 'd'},
            {"platform", required_argument, nullptr, (int) 'p'},
            {nullptr, 0,                    nullptr, 0}
    };

    opterr = 0;

    int option;
    int longindex;

    while ((option = getopt_long(argc, argv, "hd:p:", longopts, &longindex)) != -1) {
        switch (option) {
            case 'h':
                displayUsage();
                exit(EXIT_SUCCESS);
                break;
            case 'd':
                gDuration = std::atoi(optarg);
                if (gDuration < 0) {
                    fprintf(stderr, "Error: duration (s) must be > 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p': {
                std::string platform(optarg);

                if (platform == "AMLOGIC") {
                    gPlatform = Platform::AMLOGIC;
                } else if (platform == "REALTEK") {
                    gPlatform = Platform::REALTEK;
                } else if (platform == "BROADCOM") {
                    gPlatform = Platform::BROADCOM;
                } else {
                    fprintf(stderr, "Warning: Unsupported platform %s\n", platform.c_str());
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case '?':
                if (optopt == 'c')
                    fprintf(stderr, "Warning: Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Warning: Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Warning: Unknown option character `\\x%x'.\n", optopt);

                exit(EXIT_FAILURE);
                break;
            default:
                exit(EXIT_FAILURE);
                break;
        }
    }
}


int main(int argc, char *argv[])
{
    parseArgs(argc, argv);

    // Lower our priority to avoid getting in the way
    if (nice(10) < 0) {
        LOG_WARN("Failed to set nice value");
    }

    LOG_INFO("** About to start memory capture for %d seconds **", gDuration);

    // Create all our metrics
    ProcessMetric processMetric;
    MemoryMetric memoryMetric(gPlatform);
    PerformanceMetric performanceMetric;

    // Start data collection
    // Capture procrank output less often to reduce CPU load
    processMetric.StartCollection(std::chrono::seconds(5));
    memoryMetric.StartCollection(std::chrono::seconds(3));

    // We can only get performance metrics for now on Amlogic
    if (gPlatform == Platform::AMLOGIC) {
        performanceMetric.StartCollection(std::chrono::seconds(3));
    }

    // Block main thread for the collection duration
    std::this_thread::sleep_for(std::chrono::seconds(gDuration));

    // Done! Stop data collection
    processMetric.StopCollection();
    memoryMetric.StopCollection();

    if (gPlatform == Platform::AMLOGIC) {
        performanceMetric.StopCollection();
    }

    // Print results to stdout
    processMetric.PrintResults();
    printf("\n");
    memoryMetric.PrintResults();

    if (gPlatform == Platform::AMLOGIC) {
        printf("\n");
        performanceMetric.PrintResults();
    }

    return 0;
}

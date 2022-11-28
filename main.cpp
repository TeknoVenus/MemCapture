#include <unistd.h>
#include <getopt.h>

#include "Log.h"
#include "ProcessMetric.h"
#include "MemoryMetric.h"
#include "PerformanceMetric.h"

static int gDuration = 30;

static void displayUsage()
{
    printf("Usage: MemCapture <option(s)>\n");
    printf("    Utility to capture memory statistics\n\n");
    printf("    -h, --help          Print this help and exit\n");
    printf("    -d, --duration      Amount of time (in seconds) to capture data for. Default 30 seconds\n");
}

static void parseArgs(const int argc, char **argv)
{
    struct option longopts[] = {
            {"help",     no_argument,       nullptr, (int) 'h'},
            {"duration", required_argument, nullptr, (int) 'd'},
            {nullptr, 0,                    nullptr, 0}
    };

    opterr = 0;

    int option;
    int longindex;

    while ((option = getopt_long(argc, argv, "hd:", longopts, &longindex)) != -1) {
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


int main(int argc, char* argv[])
{
    parseArgs(argc, argv);

    // Lower our priority to avoid getting in the way
    if (nice(10) < 0) {
        LOG_WARN("Failed to set nice value");
    }

    LOG_INFO("** About to start memory capture for %d seconds **", gDuration);

    ProcessMetric processMetric;
    MemoryMetric memoryMetric;
    PerformanceMetric performanceMetric;

    // Start data collection
    // Capture procrank output less often to reduce CPU load
    processMetric.StartCollection(std::chrono::seconds(5));
    memoryMetric.StartCollection(std::chrono::seconds(3));
    performanceMetric.StartCollection(std::chrono::seconds(3));

    std::this_thread::sleep_for(std::chrono::seconds(gDuration));

    // Stop data collection
    processMetric.StopCollection();
    memoryMetric.StopCollection();
    performanceMetric.StopCollection();

    // Print results to stdout
    processMetric.PrintResults();
    printf("\n");
    memoryMetric.PrintResults();
    printf("\n");
    performanceMetric.PrintResults();

    return 0;
}

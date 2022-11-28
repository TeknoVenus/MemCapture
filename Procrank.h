//
// Created by Stephen F on 23/11/22.
//

#pragma once

#include "Log.h"
#include "Measurement.h"

extern "C" {
#include "pagemap/pagemap.h"
}

#include <vector>
#include <string>


class Procrank
{
public:
    struct ProcessInfo
    {
        pid_t pid;
        std::string name;
        pm_memusage_t memoryUsage;
    };

public:
    Procrank();

    ~Procrank();

    std::vector<ProcessInfo> GetMemoryUsage() const;

    static void GetProcessName(pid_t pid, std::string &name);


private:
    pm_kernel_t *mKernel;

    std::vector<Measurement> mMeasurements;
};

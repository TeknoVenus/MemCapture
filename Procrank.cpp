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

#include "Procrank.h"

#include <climits>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>

/**
 * C++ wrapper over Procrank (originally from Android): https://github.com/csimmonds/procrank_linux
 */
Procrank::Procrank()
{
    auto error = pm_kernel_create(&mKernel);
    if (error != 0) {
        LOG_SYS_WARN(error, "Failed to create kernel interface");
        mKernel = nullptr;
    }
}

Procrank::~Procrank()
{
    if (mKernel) {
        pm_kernel_destroy(mKernel);
    }
}


std::vector<Procrank::ProcessMemoryUsage> Procrank::GetMemoryUsage() const
{
    if (mKernel == nullptr) {
        return {};
    }

    pid_t *pids;
    size_t num_procs;
    auto error = pm_kernel_pids(mKernel, &pids, &num_procs);

    if (error != 0) {
        LOG_SYS_WARN(error, "Failed to list processes");
        return {};
    }

    std::vector<Procrank::ProcessMemoryUsage> processes;
    std::string cmdline;
    pm_process_t *proc = {};
    pid_t pid;

    for (size_t i = 0; i < num_procs; i++) {
        pid = pids[i];

        Process process(pid);
        ProcessMemoryUsage memoryUsage(process);

        if (process.name().empty()) {
            continue;
        }

        error = pm_process_create(mKernel, pid, &proc);
        if (error != 0) {
            LOG_WARN("Could not create process interface for %d", pid);
            continue;
        }

        error = pm_process_usage(proc, &memoryUsage.memoryUsage);
        if (error != 0) {
            LOG_WARN("Could not get memory usage for PID %d", pid);
        } else {
            processes.emplace_back(memoryUsage);
        }

        pm_process_destroy(proc);
    }

    free(pids);
    LOG_INFO("Got memory usage for %zd PIDs", processes.size());

    return processes;
}
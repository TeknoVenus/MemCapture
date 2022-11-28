//
// Created by Stephen F on 23/11/22.
//

#include "Procrank.h"

#include <climits>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>

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


std::vector<Procrank::ProcessInfo> Procrank::GetMemoryUsage() const
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

    std::vector<Procrank::ProcessInfo> processes;
    std::string cmdline;
    pm_process_t *proc = {};
    pid_t pid;

    for (size_t i = 0; i < num_procs; i++) {
        pid = pids[i];

        Procrank::ProcessInfo info = {};
        info.pid = pid;
        GetProcessName(pid, info.name);

        if (info.name.empty()) {
            continue;
        }

        error = pm_process_create(mKernel, pid, &proc);
        if (error != 0) {
            LOG_WARN("Could not create process interface for %d", pid);
            continue;
        }

        info.memoryUsage = {};
        error = pm_process_usage(proc, &info.memoryUsage);
        if (error != 0) {
            LOG_WARN("Could not get memory usage for PID %d", pid);
        } else {
            processes.emplace_back(info);
        }

        pm_process_destroy(proc);
    }

    free(pids);
    LOG_INFO("Got memory usage for %zd PIDs", processes.size());

    return processes;
}

void Procrank::GetProcessName(const pid_t pid, std::string &name)
{
    char procPath[PATH_MAX];

    sprintf(procPath, "/proc/%u/cmdline", pid);

    std::ifstream cmdFile(procPath);

    if (!cmdFile) {
        name = "<Unknown>";
        return;
    }

    name.assign((std::istreambuf_iterator<char>(cmdFile)),
                (std::istreambuf_iterator<char>()));

    name.erase(std::find(name.begin(), name.end(), '\0'), name.end());
}
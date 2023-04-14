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
    static std::string GetProcessCmdline(pid_t pid);


private:
    pm_kernel_t *mKernel;

    std::vector<Measurement> mMeasurements;
};

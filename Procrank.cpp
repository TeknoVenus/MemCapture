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
#include <iostream>

/**
 * C++ wrapper over Procrank (originally from Android): https://github.com/csimmonds/procrank_linux
 */
Procrank::Procrank()
{

}

Procrank::~Procrank()
{

}


std::vector<Procrank::ProcessMemoryUsage> Procrank::GetMemoryUsage() const
{
    // Get running processes
    std::set<pid_t> pids;
    if (!::android::smapinfo::get_all_pids(&pids)) {
        LOG_ERROR("Failed to get running processes");
        return {};
    }

    uint64_t pgflags = 0;
    uint64_t pgflags_mask = 0;

    ::android::smapinfo::run_procrank(pgflags, pgflags_mask, pids, false,
                                                     false, android::smapinfo::SortOrder::BY_PSS, false, nullptr,
                                                     std::cout, std::cerr);

    return {};
}
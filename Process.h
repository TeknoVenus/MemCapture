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

#include <sys/types.h>
#include <string>
#include "GroupManager.h"
#include <memory>
#include <optional>

/**
 * Represent a running process on the system
 *
 * All info about the process is loaded in the constructor and cached to help avoid issues with short-lived processes.
 * As such, there is no guarantee the process actually exists in system any more when calling the various methods!
 *
 */
class Process
{
public:
    explicit Process(pid_t pid);

    bool operator==(const Process &rhs) const
    {
        return mPid == rhs.mPid;
    }

    pid_t pid() const;

    std::string name() const;

    std::string cmdline() const;

    std::optional<std::string> systemdService() const;

    std::optional<std::string> container() const;

    std::optional<std::string> group(const std::shared_ptr<GroupManager> &groupManager) const;

private:
    std::string getName() const;

    std::string getCmdline() const;

    std::string getSystemdService();

    std::string getContainer();

    std::string getNameWithoutPath() const;

    std::string GetCgroupPathByCgroupControllerAndPid(const std::string &cgroup_controller, pid_t pid);

private:
    pid_t mPid;

    std::string mName;
    std::string mCmdline;
    std::string mSystemdService;
    std::string mContainer;
};

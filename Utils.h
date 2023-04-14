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

#ifndef MEMCAPTURE_UTILS_H
#define MEMCAPTURE_UTILS_H

#include <tabulate/table.hpp>
#include <string>
#include <fstream>
#include "Measurement.h"

class Utils
{
public:
    static std::string getContainerName(pid_t pid)
    {
        return GetCgroupPathByCgroupControllerAndPid("cpuset", pid);
    }

    static std::string getSystemdServiceName(pid_t pid)
    {
        std::string systemdSlice = GetCgroupPathByCgroupControllerAndPid("pids", pid);

        auto pos = systemdSlice.find("system.slice/");
        if (pos == std::string::npos) {
            return "Unknown";
        }

        return systemdSlice.substr(pos + 13);
    }

private:
    /**
     * Extract cgroup name from /proc/<pid>/cgroup (if any) for specified cgroup_controller and pid and return. Otherwise return '-'.
     *
     * Example of process which is part of gpu cgroup. Here /proc/<pid>/cgroup will have a 'gpu' entry followed by name of cgroup which is also the name of the container:
     *
     * root@xione-sercomm:~# cat /proc/8619/cgroup
     * 10:gpu:/com.sky.as.apps_com.bskyb.epgui
     * 9:pids:/com.sky.as.apps_com.bskyb.epgui
     * 8:cpu,cpuacct:/com.sky.as.apps_com.bskyb.epgui
     * 7:freezer:/com.sky.as.apps_com.bskyb.epgui
     * 6:memory:/com.sky.as.apps_com.bskyb.epgui
     * 5:blkio:/com.sky.as.apps_com.bskyb.epgui
     * 4:devices:/com.sky.as.apps_com.bskyb.epgui
     * 3:cpuset:/com.sky.as.apps_com.bskyb.epgui
     * 2:debug:/com.sky.as.apps_com.bskyb.epgui
     * 1:name=systemd:/com.sky.as.apps_com.bskyb.epgui
     * root@xione-sercomm:~#
     *
     * Example of process which is not part of gpu cgroup and therefore not part of a container. Here the 'gpu' entry is not followed by anything:
     *
     * root@xione-sercomm:~# cat /proc/7539/cgroup
     * 10:gpu:/
     * 9:pids:/system.slice/sky-appsservice.service
     * 8:cpu,cpuacct:/system.slice/sky-appsservice.service
     * 7:freezer:/
     * 6:memory:/system.slice/sky-appsservice.service
     * 5:blkio:/
     * 4:devices:/system.slice/sky-appsservice.service
     * 3:cpuset:/
     * 2:debug:/
     * 1:name=systemd:/system.slice/sky-appsservice.service
     * root@xione-sercomm:~#
     *
     * @param cgroup_controller name of cgroup controller e.g. 'gpu'
     * @param pid pid of process
     * @return name of cgroup (which is also name of container)
     */
    static std::string GetCgroupPathByCgroupControllerAndPid(const std::string &cgroup_controller, pid_t pid)
    {
        std::string cgrp_path("-");
        std::string cgrp_file_path = "/proc/" + std::to_string(pid) + "/cgroup";

        std::ifstream cgrp_strm(cgrp_file_path.c_str());
        if (cgrp_strm) {
            std::string cgrp_line;
            int hierarchy_id;
            char cgroup_path[128];

            std::string sscanf_format = std::string("%d:") + cgroup_controller + ":/%s";

            // Doesn't feel very efficient (need to memset cgroup_path each time round the loop) but the alternatives are std::string.find (clunky) or std::regex (inefficient).
            // Besides, the gpu group always appears to be the first line.
            while (std::getline(cgrp_strm, cgrp_line)) {
                memset(cgroup_path, 0, sizeof(cgroup_path));
                if (sscanf(cgrp_line.c_str(), sscanf_format.c_str(), &hierarchy_id, cgroup_path) == 2) {
                    cgrp_path = cgroup_path;
                    break;
                }
            }
        } else {
            LOG_WARN("Could not open process cgroup file \"%s\"", cgrp_file_path.c_str());
        }

        return cgrp_path;
    }
};

#endif //MEMCAPTURE_UTILS_H

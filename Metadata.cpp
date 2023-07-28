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


#include "Metadata.h"
#include "Procrank.h"

#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

Metadata::Metadata() : mDuration(0)
{

}

void Metadata::SetDuration(long seconds)
{
    mDuration = seconds;
}

std::string Metadata::Platform() const
{
    std::ifstream deviceProperties("/etc/device.properties");
    if (!deviceProperties) {
        return "Unknown";
    }

    std::string line;
    char platform[512];

    while (std::getline(deviceProperties, line)) {
        if (sscanf(line.c_str(), "FRIENDLY_ID=\"%s\"", platform) != 0) {
            return std::string(platform);
        }
    }

    return "Unknown";
}

std::string Metadata::Image() const
{
    std::ifstream versionFile("/version.txt");
    if (!versionFile) {
        return "Unknown";
    }

    std::string line;
    char image[512];

    while (std::getline(versionFile, line)) {
        if (sscanf(line.c_str(), "imagename:%256s", image) != 0) {
            return std::string(image);
        }
    }

    return "Unknown";
}

std::string Metadata::Mac() const
{
    std::ifstream macFile("/sys/class/net/eth0/address");
    if (!macFile) {
        return "Unknown";
    }

    std::stringstream buffer;
    buffer << macFile.rdbuf();

    return buffer.str();
}

std::string Metadata::ReportTimestamp() const
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::stringstream buffer;
    buffer << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    return buffer.str();
}

long Metadata::Duration() const
{
    return mDuration;
}

bool Metadata::SwapEnabled() const
{
    Procrank procrank;
    return procrank.swapTotalKb() > 0;
}

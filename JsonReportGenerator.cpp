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


#include "JsonReportGenerator.h"

#include <utility>

JsonReportGenerator::JsonReportGenerator(std::shared_ptr<Metadata> metadata, std::optional<std::shared_ptr<GroupManager>> groupManager)
        : mMetadata(std::move(metadata)), mGroupManager(std::move(groupManager)), mJson()
{
    mJson["processes"] = nlohmann::json::array();
    mJson["metadata"] = {};

    mJson["grandTotal"]["linuxUsage"] = 0.0;
    mJson["grandTotal"]["calculatedUsage"] = 0.0;
}

void JsonReportGenerator::addDataset(const std::string &name, const std::vector<std::string> &columns,
                                     const std::vector<row> &rows)
{
    nlohmann::json dataSet;

    dataSet["name"] = name;
    dataSet["columns"] = columns;

    dataSet["rows"] = nlohmann::json::array();

    for (const auto &row: rows) {
        nlohmann::json rowJson = nlohmann::json::array();

        for (const auto &value: row) {
            std::visit(overload{
                    [&](const std::string &value)
                    {
                        rowJson.emplace_back(value);
                    },
                    [&](const Measurement &value)
                    {
                        rowJson.emplace_back(value.GetMinRounded());
                        rowJson.emplace_back(value.GetMaxRounded());
                        rowJson.emplace_back(value.GetAverageRounded());
                    }
            }, value);
        }

        dataSet["rows"].emplace_back(rowJson);
    }

    mJson["data"].emplace_back(dataSet);
}

nlohmann::json JsonReportGenerator::getJson()
{
    mJson["metadata"] = {
            {"image",     mMetadata->Image()},
            {"platform",  mMetadata->Platform()},
            {"mac",       mMetadata->Mac()},
            {"timestamp", mMetadata->ReportTimestamp()},
            {"duration",  mMetadata->Duration()}
    };

    return mJson;
}

void JsonReportGenerator::addProcesses(std::vector<processMeasurement> &processes)
{
    // Sort by PSS desc
    std::sort(processes.begin(), processes.end(), [](const processMeasurement &a, const processMeasurement &b)
    {
        return a.Pss.GetAverageRounded() > b.Pss.GetAverageRounded();
    });

    for (const auto &process: processes) {
        nlohmann::json processJson;

        processJson["pid"] = process.ProcessInfo.pid();
        processJson["ppid"] = process.ProcessInfo.ppid();
        processJson["name"] = process.ProcessInfo.name();
        processJson["cmdline"] = process.ProcessInfo.cmdline();

        processJson["systemdService"] = process.ProcessInfo.systemdService().has_value()
                                        ? process.ProcessInfo.systemdService().value() : "";
        processJson["container"] = process.ProcessInfo.container().has_value() ? process.ProcessInfo.container().value()
                                                                               : "";

        if (mGroupManager.has_value()) {
            processJson["group"] = process.ProcessInfo.group(mGroupManager.value()).has_value()
                                   ? process.ProcessInfo.group(
                            mGroupManager.value()).value() : "";
        } else {
            processJson["group"] = "";
        }

        processJson["rss"]["min"] = process.Rss.GetMinRounded();
        processJson["rss"]["max"] = process.Rss.GetMaxRounded();
        processJson["rss"]["average"] = process.Rss.GetAverageRounded();

        processJson["pss"]["min"] = process.Pss.GetMinRounded();
        processJson["pss"]["max"] = process.Pss.GetMaxRounded();
        processJson["pss"]["average"] = process.Pss.GetAverageRounded();

        processJson["uss"]["min"] = process.Uss.GetMinRounded();
        processJson["uss"]["max"] = process.Uss.GetMaxRounded();
        processJson["uss"]["average"] = process.Uss.GetAverageRounded();

        mJson["processes"].emplace_back(processJson);
    }


    // Calculate PSS memory per group
    if (mGroupManager.has_value()) {
        std::map<std::string, long double> pssPerGroup;
        mJson["pssByGroup"] = nlohmann::json::array();

        for (const auto &process : processes) {
            auto group = process.ProcessInfo.group(mGroupManager.value());
            if (group.has_value()) {
                pssPerGroup[group.value()] += process.Pss.GetAverage();
            }
        }

        // Sort the map by PSS desc so the pie chart appears nicely
        std::vector<std::pair<std::string, long double>> pairs;
        pairs.reserve(pssPerGroup.size());
        for (auto & itr : pssPerGroup) {
            pairs.emplace_back(itr);
        }

        std::sort(pairs.begin(), pairs.end(), [](std::pair<std::string, long double>& a, std::pair<std::string, long double>& b)
        {
            return a.second > b.second;
        });

        for (const auto &group : pairs) {
            nlohmann::json tmp;
            tmp["groupName"] = group.first;
            tmp["pss"] = std::round((int)group.second);

            mJson["pssByGroup"].emplace_back(tmp);
        }
    } else {
        mJson["pssByGroup"] = nullptr;
    }

}

void JsonReportGenerator::setAverageLinuxMemoryUsage(int valueKb)
{
    // Store in MB
    mJson["grandTotal"]["linuxUsage"] = valueKb / 1024.0L;
}

void JsonReportGenerator::addToAccumulatedMemoryUsage(long double valueKb)
{
    // Store in MB
    long double usage = mJson["grandTotal"]["calculatedUsage"];
    usage += valueKb / 1024.0L;

    mJson["grandTotal"]["calculatedUsage"] = usage;
}


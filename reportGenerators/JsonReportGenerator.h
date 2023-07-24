//
// Created by Stephen F on 06/07/23.
//

#pragma once

#include <variant>

#include "nlohmann/json.hpp"

#include "../ProcessMeasurement.h"
#include "../GroupManager.h"
#include "../Metadata.h"

template<class... Ts>
struct overload : Ts ...
{
    using Ts::operator()...;
};
template<class... Ts> overload(Ts...) -> overload<Ts...>;

class JsonReportGenerator
{
public:
    using row = std::vector<std::variant<std::string, Measurement>>;

    JsonReportGenerator(Metadata metadata, std::optional<std::shared_ptr<GroupManager>> groupManager);

    void addDataset(const std::string &name, const std::vector<std::string> &columns, const std::vector<row> &rows);

    void addProcesses(std::vector<processMeasurement> &processes);

    void setAverageLinuxMemoryUsage(int valueKb);

    void addToAccumulatedMemoryUsage(long double valueKb);

    nlohmann::json getJson() const;

private:
    const Metadata mMetadata;
    const std::optional<std::shared_ptr<GroupManager>> mGroupManager;

    nlohmann::json mJson;

    std::vector<Process> mProcesses;
};

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

#include "IReportGenerator.h"
#include <filesystem>

class CsvReportGenerator : public IReportGenerator
{
public:
    CsvReportGenerator(std::filesystem::path directory, std::string name, std::vector<std::string> columnNames);

    ~CsvReportGenerator() override = default;

    void addRow(const std::vector<std::string> &values) override;

    void printReport() override;

private:
    const std::filesystem::path mOutputDirectory;
    const std::string mReportName;
    std::vector<std::string> mColumnNames;

    std::vector<std::vector<std::string>> mRows;
    std::filesystem::path mFilename;
};

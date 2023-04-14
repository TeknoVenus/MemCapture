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

#include <string>
#include "IReportGenerator.h"
#include "TableReportGenerator.h"
#include "CsvReportGenerator.h"
#include <filesystem>

/**
 * Factory for report generators
 */
class ReportGeneratorFactory
{
public:
    enum class ReportType
    {
        TABLE,
        CSV
    };

    /**
     * Create report generators of a specified type
     *
     * @param type Type of report to generate
     * @param directory Where to save the reports (should be a unique directory)
     */
    explicit ReportGeneratorFactory(ReportType type, std::filesystem::path directory) : mType(type), mOutputDirectory(
            std::move(directory))
    {

    }

    /**
     * Get a report generator for a report with the given name
     *
     * @param reportName Name of the report
     * @param columnNames Columns to display in the report
     * @return Report generator instance
     */
    std::unique_ptr<IReportGenerator> getReportGenerator(const std::string &reportName,
                                                         const std::vector<std::string> &columnNames)
    {
        switch (mType) {
            case ReportType::TABLE:
                return std::make_unique<TableReportGenerator>(mOutputDirectory, reportName, columnNames);
            case ReportType::CSV:
                return std::make_unique<CsvReportGenerator>(mOutputDirectory, reportName, columnNames);
            default:
                return nullptr;
        }
    }

private:
    const ReportType mType;
    const std::filesystem::path mOutputDirectory;
};

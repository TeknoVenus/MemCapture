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

#include "CsvReportGenerator.h"
#include <algorithm>
#include <fstream>

/**
 * Report generator that saves the data into a CSV file with the name of the report
 *
 * @param directory Directory to save the CSV file in
 * @param name Name of the report - will be the name of the CSV file
 * @param columnNames Vector of columns in the CSV file
 */
CsvReportGenerator::CsvReportGenerator(std::filesystem::path directory, std::string name,
                                       std::vector<std::string> columnNames) : mOutputDirectory(std::move(directory)),
                                                                               mReportName(std::move(name)),
                                                                               mColumnNames(std::move(columnNames))
{
    // Add suffix
    auto csvName = mReportName + ".csv";
    mFilename = mOutputDirectory / csvName;
}

void CsvReportGenerator::addRow(const std::vector<std::string> &values)
{
    mRows.emplace_back(values);
}

void CsvReportGenerator::printReport()
{
    // Each report gets it own CSV file
    std::ofstream csvFile(mFilename, std::ios::out | std::ios::trunc);

    // Headings
    for (const auto &col: mColumnNames) {
        csvFile << col;
        if (&col != &mColumnNames.back()) {
            csvFile << ",";
        }
    }

    csvFile << "\n";

    // Data
    for (const auto &row: mRows) {
        for (const auto &col: row) {
            // Strip new-lines from the data
            csvFile << '"' << sanitise(col) << '"';

            // Don't add comma on final item
            if (&col != &row.back()) {
                csvFile << ",";
            }
        }

        csvFile << "\n";
    }
}

std::string CsvReportGenerator::sanitise(const std::string &str)
{
    // Remove new lines
    std::string tmp(str);
    tmp.erase(std::remove(tmp.begin(), tmp.end(), '\n'), tmp.cend());

    // If string starts with =, erase = to avoid Excel treating as a formula
    if (tmp.rfind('=', 0) == 0)
    {
        tmp.erase(0, 1);
    }

    return tmp;
}

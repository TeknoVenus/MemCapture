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

#include "TableReportGenerator.h"
#include "tabulate/table.hpp"
#include <fstream>

/**
 * Report generator that saves all reports to a single file called "report.txt" in the output directory
 *
 * Uses the tabulate library to format everything into nicely presented tables
 *
 * This is the default output for MemCapture as it's a fairly easy to read single file
 *
 * @param directory Directory to save the report into. The file will always be called "report.txt"
 * @param name Name of this particular report - will be used to section the txt file with headers
 * @param columnNames Names of the columns for this table
 */
TableReportGenerator::TableReportGenerator(std::filesystem::path directory, std::string name,
                                           std::vector<std::string> columnNames) :
        mOutputDirectory(std::move(directory)),
        mReportName(std::move(name)),
        mColumnNames(std::move(columnNames)),
        mRows()
{
    mFilename = mOutputDirectory / "report.txt";
}

void TableReportGenerator::addRow(const std::vector<std::string> &values)
{
    mRows.emplace_back(values);
}

void TableReportGenerator::printReport()
{
    // Save report in directory
    // Always append since we want to save everything to a single file
    std::ofstream outputFile(mFilename, std::ios::app);

    outputFile << "======== " << mReportName.c_str() << " ========";

    tabulate::Table resultsTable;

    // Get column names and print heading
    tabulate::Table::Row_t columnNames;
    for (const auto &col: mColumnNames) {
        columnNames.emplace_back(col);
    }

    resultsTable.add_row(columnNames);

    // Print results
    for (const auto &row: mRows) {
        tabulate::Table::Row_t rowData;

        for (const auto &col: row) {
            rowData.emplace_back(col);
        }

        resultsTable.add_row(rowData);
    }

    resultsTable.format()
            .padding(0)
            .border_top(" ")
            .border_bottom(" ")
            .border_left(" ")
            .border_right(" ")
            .hide_border_bottom()
            .hide_border_top()
            .corner(" ");

    resultsTable.print(outputFile);
    outputFile << "\n";
}


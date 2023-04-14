# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 Stephen Foulds
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.10)

project(MemCapture)

# The following disables an annoying warning "<blah> will change in GCC X.XX"
add_compile_options(-Wno-psabi)
add_compile_options(-Wall -Wextra)

find_package(Threads REQUIRED)

add_subdirectory(procrank)

add_executable(${PROJECT_NAME}
        main.cpp
        Measurement.cpp
        Procrank.cpp

        reportGenerators/TableReportGenerator.cpp
        reportGenerators/CsvReportGenerator.cpp

        ProcessMetric.cpp
        MemoryMetric.cpp
        PerformanceMetric.cpp
        )

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        )

target_include_directories(${PROJECT_NAME}
        PRIVATE
        3rdparty
        )

target_link_libraries(${PROJECT_NAME}
        procrank
        Threads::Threads
        )

cmake_minimum_required(VERSION 3.10)

project(MemCapture)

# The following disables an annoying warning "<blah> will change in GCC X.XX"
add_compile_options(-Wno-psabi)
add_compile_options(-Wall)

find_package(Threads REQUIRED)

add_subdirectory(procrank)

add_executable(${PROJECT_NAME}
        main.cpp
        Measurement.cpp
        Procrank.cpp

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
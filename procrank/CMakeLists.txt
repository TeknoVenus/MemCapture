cmake_minimum_required(VERSION 3.10)

project(procrank)

add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE)

add_library(${PROJECT_NAME}
        pm_kernel.c
        pm_map.c
        pm_process.c
        pm_memusage.c
        strlcpy.c
        )

target_include_directories(${PROJECT_NAME}
        PUBLIC
        include
)
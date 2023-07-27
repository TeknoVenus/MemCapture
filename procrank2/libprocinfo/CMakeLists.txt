project(procinfo)

add_library(${PROJECT_NAME}
        ./process.cpp
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ./include
)

target_link_libraries(${PROJECT_NAME}
        AndroidCommon
)
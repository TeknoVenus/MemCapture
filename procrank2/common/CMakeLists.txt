project(AndroidCommon)

add_library(${PROJECT_NAME}
        STATIC
        ./file.cpp
        ./stringprintf.cpp
        ./strings.cpp
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        .)
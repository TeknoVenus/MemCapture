project(meminfo)

add_library(${PROJECT_NAME}
        pageacct.cpp
        procmeminfo.cpp
        sysmeminfo.cpp

        libsmapinfo/smapinfo.cpp
        libsmapinfo/processrecord.cpp

        libdmabufinfo/dmabufinfo.cpp
        libdmabufinfo/dmabuf_sysfs_stats.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ./include
        ./libsmapinfo/include
        ./libdmabufinfo/include

        PRIVATE
        .
)

target_link_libraries(${PROJECT_NAME}
        AndroidCommon
        procinfo
)


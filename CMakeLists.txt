cmake_minimum_required(VERSION 3.1.0)

project(SingleApplication)

find_package(Qt5Core)
find_package(Qt5Network)

add_library(${PROJECT_NAME} STATIC
    singleapplication.cpp
    singleapplication_p.cpp
    )

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Network
    )

if(WIN32)
    target_link_libraries(${PROJECT_NAME} advapi32)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

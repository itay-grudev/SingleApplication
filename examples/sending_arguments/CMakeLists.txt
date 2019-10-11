cmake_minimum_required(VERSION 3.1.0)

project(sending_arguments)

set(CMAKE_AUTOMOC ON)

# SingleApplication base class
set(QAPPLICATION_CLASS QCoreApplication CACHE STRING "Inheritance class for SingleApplication")

add_executable(${PROJECT_NAME}
    main.cpp
    messagereceiver.cpp
    messagereceiver.h
    main.cpp
    )

find_package(Qt5 COMPONENTS Core REQUIRED)
add_subdirectory(../.. SingleApplication)
target_link_libraries(${PROJECT_NAME} Qt5::Core SingleApplication)

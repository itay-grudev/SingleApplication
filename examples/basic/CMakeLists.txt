cmake_minimum_required(VERSION 3.1.0)

project(basic)

# SingleApplication base class
set(QAPPLICATION_CLASS QCoreApplication CACHE STRING "Inheritance class for SingleApplication")

add_executable(basic
    main.cpp
    )

find_package(Qt5 COMPONENTS Core REQUIRED)
add_subdirectory(../.. SingleApplication)
target_link_libraries(${PROJECT_NAME} Qt5::Core SingleApplication)


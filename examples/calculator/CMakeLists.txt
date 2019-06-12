cmake_minimum_required(VERSION 3.1.0)

project(calculator)

set(CMAKE_AUTOMOC ON)

# SingleApplication base class
set(QAPPLICATION_CLASS QApplication CACHE STRING "Inheritance class for SingleApplication")

add_executable(${PROJECT_NAME}
    button.h
    calculator.h
    button.cpp
    calculator.cpp
    main.cpp
    )

find_package(Qt5 COMPONENTS Widgets REQUIRED)
add_subdirectory(../.. SingleApplication)
target_link_libraries(${PROJECT_NAME} Qt5::Widgets SingleApplication)

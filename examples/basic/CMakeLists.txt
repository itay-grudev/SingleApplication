cmake_minimum_required(VERSION 3.7.0)

project(basic LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

IF(DEFINED $ENV{QT6_BUILD})
    set(QT_DEFAULT_MAJOR_VERSION 6)
ENDIF()

# SingleApplication base class
set(QAPPLICATION_CLASS QCoreApplication)

add_executable(basic main.cpp)

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS Core REQUIRED)
add_subdirectory(../.. SingleApplication)
target_link_libraries(${PROJECT_NAME} SingleApplication::SingleApplication)

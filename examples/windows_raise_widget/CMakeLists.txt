cmake_minimum_required(VERSION 3.7.0)

project(windows_raise_widget LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_WIN32_EXECUTABLE TRUE)

if (NOT DEFINED ENV{CMAKE_PREFIX_PATH})
  set (ENV{CMAKE_PREFIX_PATH} "C:/Qt/Qt5.12.10/5.12.10/msvc2017_64")
endif ()

# SingleApplication base class
set(QAPPLICATION_CLASS QApplication)
add_subdirectory(../.. SingleApplication)

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS Core Widgets REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} SingleApplication::SingleApplication user32.lib)

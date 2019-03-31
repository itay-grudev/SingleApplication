cmake_minimum_required(VERSION 3.12.4)

project(SingleApplication)

set(QAPPLICATION_CLASS QCoreApplication CACHE STRING "Inheritance class for SingleApplication")
set_property(CACHE QAPPLICATION_CLASS PROPERTY STRINGS QApplication QGuiApplication QCoreApplication)

find_package(Qt5Network)
if(QAPPLICATION_CLASS STREQUAL QApplication)
    find_package(Qt5Widgets)
elseif(QAPPLICATION_CLASS STREQUAL QGuiApplication)
    find_package(Qt5Gui)
else()
    find_package(Qt5Core)
endif()
add_compile_definitions(QAPPLICATION_CLASS=${QAPPLICATION_CLASS})

add_library(${PROJECT_NAME} STATIC
    singleapplication.cpp
    singleapplication_p.cpp
    )

target_link_libraries(${PROJECT_NAME} Qt5::Network)
if(QAPPLICATION_CLASS STREQUAL QApplication)
    target_link_libraries(${PROJECT_NAME} Qt5::Widgets)
elseif(QAPPLICATION_CLASS STREQUAL QGuiApplication)
    target_link_libraries(${PROJECT_NAME} Qt5::Gui)
else()
    target_link_libraries(${PROJECT_NAME} Qt5::Core)
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} advapi32)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

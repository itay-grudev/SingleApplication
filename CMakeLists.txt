cmake_minimum_required(VERSION 3.12.0)

project(SingleApplication VERSION 3.6.0 LANGUAGES CXX DESCRIPTION "Replacement for QtSingleApplication")

set(CMAKE_AUTOMOC ON)

add_library(${PROJECT_NAME} STATIC
    singleapplication.cpp
    singleapplication_p.cpp
)

# User configurable options
if(NOT QT_DEFAULT_MAJOR_VERSION)
    set(QT_DEFAULT_MAJOR_VERSION 5 CACHE STRING "Qt version to use (5 or 6), defaults to 5")
endif()

if(NOT QAPPLICATION_CLASS)
    set(QAPPLICATION_CLASS QCoreApplication CACHE STRING "Qt application base class or FreeStandingSingleApplication")
endif()

option(SINGLEAPPLICATION_INSTALL OFF "Enable freestanding mode install including config files")

if(SINGLEAPPLICATION_INSTALL AND NOT QAPPLICATION_CLASS STREQUAL "FreeStandingSingleApplication")
    message(FATAL_ERROR "SINGLEAPPLICATION_INSTALL requires QAPPLICATION_CLASS == FreeStandingSingleApplication")
endif()

# Find dependencies
set(QT_COMPONENTS Core Network)
set(QT_LIBRARIES Qt${QT_DEFAULT_MAJOR_VERSION}::Core Qt${QT_DEFAULT_MAJOR_VERSION}::Network)

if(QAPPLICATION_CLASS STREQUAL QApplication)
    list(APPEND QT_COMPONENTS Widgets)
    list(APPEND QT_LIBRARIES Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets)
elseif(QAPPLICATION_CLASS STREQUAL QGuiApplication)
    list(APPEND QT_COMPONENTS Gui)
    list(APPEND QT_LIBRARIES Qt${QT_DEFAULT_MAJOR_VERSION}::Gui)
endif()

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS ${QT_COMPONENTS} REQUIRED)

option(SINGLEAPPLICATION_DOCUMENTATION "Generate Doxygen documentation" OFF)
if(SINGLEAPPLICATION_DOCUMENTATION)
    find_package(Doxygen)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ${QT_LIBRARIES})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE advapi32)
endif()

if(SINGLEAPPLICATION_INSTALL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE QAPPLICATION_CLASS=${QAPPLICATION_CLASS})
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(${PROJECT_NAME} INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC QAPPLICATION_CLASS=${QAPPLICATION_CLASS})
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    QT_NO_CAST_TO_ASCII
    QT_NO_CAST_FROM_ASCII
    QT_NO_URL_CAST_FROM_STRING
    QT_NO_CAST_FROM_BYTEARRAY
    QT_USE_QSTRINGBUILDER
    QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
    QT_NO_KEYWORDS
    QT_NO_FOREACH
)

if(DOXYGEN_FOUND)
    # Doxygen theme
    include(FetchContent)
    FetchContent_Declare(DoxygenAwesome
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css
        GIT_TAG 4cd62308d825fe0396d2f66ffbab45d0e247724c # 2.0.3
    )
    FetchContent_MakeAvailable(DoxygenAwesome)
    FetchContent_GetProperties(DoxygenAwesome SOURCE_DIR DoxygenAwesome_SOURCE_DIR)

    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_HTML_HEADER ${DoxygenAwesome_SOURCE_DIR}/doxygen-custom/header.html)
    set(DOXYGEN_HTML_EXTRA_STYLESHEET ${DoxygenAwesome_SOURCE_DIR}/doxygen-awesome.css)
    set(DOXYGEN_HTML_EXTRA_FILES
        ${DoxygenAwesome_SOURCE_DIR}/doxygen-awesome-fragment-copy-button.js
        ${DoxygenAwesome_SOURCE_DIR}/doxygen-awesome-paragraph-link.js
        ${DoxygenAwesome_SOURCE_DIR}/doxygen-awesome-darkmode-toggle.js
    )

    doxygen_add_docs(${PROJECT_NAME}Documentation
        singleapplication.h
        CHANGELOG.md
        Windows.md
        README.md
    )
endif()

if(SINGLEAPPLICATION_INSTALL)
    # Create a header veriant where QAPPLICATION_CLASS is replaced with FreeStandingSingleApplication
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/singleapplication.h" SINGLEAPPLICATION_H_CONTENT)

    string(REGEX REPLACE
        "#ifndef QAPPLICATION_CLASS[^\n]*\n[ \t]*#define QAPPLICATION_CLASS QCoreApplication[^\n]*\n[ \t]*#endif[^\n]*\n"
        ""
        SINGLEAPPLICATION_H_CONTENT
        "${SINGLEAPPLICATION_H_CONTENT}")

    string(REGEX REPLACE
        "#include QT_STRINGIFY\\(QAPPLICATION_CLASS\\)"
        "#include \"FreeStandingSingleApplication\""
        SINGLEAPPLICATION_H_CONTENT
        "${SINGLEAPPLICATION_H_CONTENT}")

    string(REPLACE
        "QAPPLICATION_CLASS"
        "FreeStandingSingleApplication"
        SINGLEAPPLICATION_H_CONTENT
        "${SINGLEAPPLICATION_H_CONTENT}")

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/singleapplication.h" "${SINGLEAPPLICATION_H_CONTENT}")

    # CMake install
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/singleapplication.h" "SingleApplication" "FreeStandingSingleApplication"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "SingleApplicationConfigVersion.cmake"
        VERSION "${PACKAGE_VERSION}"
        COMPATIBILITY SameMajorVersion)

    configure_file("SingleApplicationConfig.cmake.in" "SingleApplicationConfig.cmake" @ONLY)
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/SingleApplicationConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/SingleApplicationConfigVersion.cmake"
        DESTINATION "lib/cmake/SingleApplication")

    install(TARGETS SingleApplication EXPORT SingleApplicationTargets)
    install(EXPORT SingleApplicationTargets
        FILE "SingleApplicationTargets.cmake"
        NAMESPACE "SingleApplication::"
        DESTINATION "lib/cmake/SingleApplication")
else()
    add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
endif()

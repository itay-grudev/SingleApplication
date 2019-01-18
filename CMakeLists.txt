cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_CXX_STANDARD 11)

include(Version.cmake)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

option(BUILD_SHARED_LIBS "Build shared library" ON)

if(NOT QAPPLICATION_CLASS)
  set(QAPPLICATION_CLASS "QApplication")
endif()

if (QAPPLICATION_CLASS STREQUAL "QApplication")
  set(SINGLEAPPLICATION_NAME "singleapplication")
  set(SINGLEAPPLICATION_CLASS "SingleApplication")
elseif (QAPPLICATION_CLASS STREQUAL "QCoreApplication")
  set(SINGLEAPPLICATION_NAME "singlecoreapplication")
  set(SINGLEAPPLICATION_CLASS "SingleCoreApplication")
elseif (QAPPLICATION_CLASS STREQUAL "QGuiApplication")
  set(SINGLEAPPLICATION_NAME "singleguiapplication")
  set(SINGLEAPPLICATION_CLASS "SingleGuiApplication")
else()
  message(FATAL_ERROR "Application class must be QApplication, QCoreApplication or QGuiApplication")
endif()

project(${SINGLEAPPLICATION_NAME})

set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "The subdirectory to the binaries prefix (default prefix/bin)")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE PATH "The subdirectory relative to the install prefix where libraries will be installed (default is /lib${LIB_SUFFIX})")
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "The subdirectory to the header prefix")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/singleapplication.h.in" "${CMAKE_CURRENT_BINARY_DIR}/${SINGLEAPPLICATION_NAME}.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/singleapplication_p.h.in" "${CMAKE_CURRENT_BINARY_DIR}/singleapplication_p.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/singleapplication.pc.cmake" "${CMAKE_CURRENT_BINARY_DIR}/${SINGLEAPPLICATION_NAME}.pc" @ONLY)

set(QT_MIN_VERSION 5.5)
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Widgets Network)

set(SINGLEAPP-SOURCES singleapplication.cpp singleapplication_p.cpp)
set(SINGLEAPP-MOC-HEADERS ${CMAKE_CURRENT_BINARY_DIR}/${SINGLEAPPLICATION_NAME}.h ${CMAKE_CURRENT_BINARY_DIR}/singleapplication_p.h)
QT5_WRAP_CPP(SINGLEAPP-SOURCES-MOC ${SINGLEAPP-MOC-HEADERS})
add_library(${SINGLEAPPLICATION_NAME} ${SINGLEAPP-SOURCES} ${SINGLEAPP-SOURCES-MOC})
target_link_libraries(${SINGLEAPPLICATION_NAME} Qt5::Core Qt5::Widgets Qt5::Network)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${SINGLEAPPLICATION_NAME}.pc" DESTINATION "${LIB_INSTALL_DIR}/pkgconfig")

set_target_properties(${SINGLEAPPLICATION_NAME} PROPERTIES
  VERSION ${SINGLEAPPLICATION_SOVERSION_MAJOR}.${SINGLEAPPLICATION_SOVERSION_MINOR}.${SINGLEAPPLICATION_SOVERSION_PATCH}
  SOVERSION ${SINGLEAPPLICATION_SOVERSION_MAJOR}
  INSTALL_NAME_DIR ${LIB_INSTALL_DIR}
  DEFINE_SYMBOL MAKE_SINGLEAPPLICATION_LIB
  LINK_INTERFACE_LIBRARIES ""
  PUBLIC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/${SINGLEAPPLICATION_NAME}.h"
)

install(TARGETS ${SINGLEAPPLICATION_NAME}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  PUBLIC_HEADER DESTINATION ${INCLUDE_INSTALL_DIR}
)

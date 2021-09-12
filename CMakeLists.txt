cmake_minimum_required(VERSION 3.12...3.16)

project(xcb-task
  DESCRIPTION "AIStatefulTask submodule - xcb support"
  LANGUAGES CXX
)

# Require support for C++20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#==============================================================================
# DEPENDENCIES
#

include(AICxxProject)
include(FindPackageHandleStandardArgs)

find_package(magic_enum REQUIRED)

find_library(
  XCB_LIBRARY
  NAMES xcb
  PATH_SUFFIXES ${LIBRARY_PATH_PREFIX}
)

set(XCB_LIBRARIES ${XCB_LIBRARY})
find_package_handle_standard_args(xcb DEFAULT_MSG XCB_LIBRARY)
mark_as_advanced(XCB_LIBRARY)

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(xcb-task_ObjLib OBJECT)

# Set SOURCE_DIR to the current directory.
set_property(
  TARGET xcb-task_ObjLib
  PROPERTY SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

# The list of source files.
target_sources(xcb-task_ObjLib
  PRIVATE
    "XcbConnection.cxx"
    "XcbConnection.h"
    "Connection.cxx"
    "Connection.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(xcb-task_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
    "${Libsystemd_INCLUDE_DIRS}"
)

# Set link dependencies.
target_link_libraries(xcb-task_ObjLib
  INTERFACE
    ${XCB_LIBRARY} 
  PUBLIC
    AICxx::statefultask
    AICxx::xcb-task::OrgFreedesktopXcbError
    farmhash::farmhash
    magic_enum::magic_enum
)

# Create an ALIAS target.
add_library(AICxx::xcb-task ALIAS xcb-task_ObjLib)

# Error categories.
add_subdirectory(org.freedesktop.Xcb.Error)

# Tests.
add_subdirectory(tests)

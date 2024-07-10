cmake_minimum_required(VERSION 3.15...3.30)

project(xcb-task
  DESCRIPTION "AIStatefulTask submodule - xcb support"
  LANGUAGES CXX
)

#==============================================================================
# DEPENDENCIES
#

include(AICxxProject)
include(FindPackageHandleStandardArgs)

find_package(magic_enum REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(Libxcb xcb IMPORTED_TARGET)
pkg_check_modules(Libxkb xkbcommon-x11 IMPORTED_TARGET)
pkg_check_modules(Libxcb-xkb xcb-xkb IMPORTED_TARGET)

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(xcb-task_ObjLib OBJECT)

# Require at least support for C++20.
target_compile_features(xcb-task_ObjLib PUBLIC cxx_std_20)

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
    PkgConfig::Libxcb-xkb
    PkgConfig::Libxcb
    PkgConfig::Libxkb
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

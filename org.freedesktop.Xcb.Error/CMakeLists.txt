# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(Xcb_OrgFreedesktopXcbError_ObjLib OBJECT)

# The list of source files.
target_sources(Xcb_OrgFreedesktopXcbError_ObjLib
  PRIVATE
    "Errors.cxx"
    "Errors.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(Xcb_OrgFreedesktopXcbError_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

# Require support for C++20.
target_compile_features(Xcb_OrgFreedesktopXcbError_ObjLib
  PUBLIC cxx_std_20
)

target_link_libraries(Xcb_OrgFreedesktopXcbError_ObjLib
  PUBLIC
    magic_enum::magic_enum
)

# Create an ALIAS target.
add_library(AICxx::xcb-task::OrgFreedesktopXcbError ALIAS Xcb_OrgFreedesktopXcbError_ObjLib)

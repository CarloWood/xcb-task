add_executable(xcb_error_test xcb_error_test.cxx)
target_link_libraries(xcb_error_test PRIVATE AICxx::xcb-task AICxx::xcb-task::OrgFreedesktopXcbError ${AICXX_OBJECTS_LIST})

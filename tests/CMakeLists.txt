add_executable(error_test error_test.cxx)
target_link_libraries(error_test PRIVATE AICxx::xcb-task AICxx::xcb-task::OrgFreedesktopXcbError ${AICXX_OBJECTS_LIST})

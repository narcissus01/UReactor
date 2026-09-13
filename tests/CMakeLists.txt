file(GLOB UREACTOR_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
list(SORT UREACTOR_TEST_SOURCES)

foreach(TEST_SOURCE IN LISTS UREACTOR_TEST_SOURCES)
    get_filename_component(TEST_NAME "${TEST_SOURCE}" NAME_WE)
    add_executable(${TEST_NAME} "${TEST_SOURCE}")
    target_link_libraries(${TEST_NAME} PRIVATE UReactor::ureactor)
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()



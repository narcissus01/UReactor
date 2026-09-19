set(INNER_TEST_NAME test_log_buffer)
set(INNER_TEST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/test_log_buffer.cpp")


add_executable("${INNER_TEST_NAME}" "${INNER_TEST_SOURCE}")
target_link_libraries("${INNER_TEST_NAME}" PRIVATE ureactor Threads::Threads)
target_include_directories("${INNER_TEST_NAME}" PRIVATE "${PROJECT_SOURCE_DIR}/ureactor/src")
set_target_properties("${INNER_TEST_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME "${INNER_TEST_NAME}" COMMAND "${INNER_TEST_NAME}" 10000)

add_executable(test_formatter test_formatter.cpp)
target_link_libraries(test_formatter PRIVATE ureactor Threads::Threads)
target_include_directories(test_formatter PRIVATE "${PROJECT_SOURCE_DIR}/ureactor/src")
add_test(NAME test_formatter COMMAND test_formatter)
set_tests_properties(test_formatter PROPERTIES ENVIRONMENT "TZ=UTC")

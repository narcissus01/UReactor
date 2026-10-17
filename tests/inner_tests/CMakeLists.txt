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

add_executable(test_context test_context.cpp)
target_link_libraries(test_context PRIVATE UReactor::ureactor)
add_test(NAME test_context COMMAND test_context)
set_tests_properties(test_context PROPERTIES TIMEOUT 20)

add_executable(test_fiber test_fiber.cpp)
target_link_libraries(test_fiber PRIVATE UReactor::ureactor)
add_test(NAME test_fiber COMMAND test_fiber)
set_tests_properties(test_fiber PROPERTIES TIMEOUT 30)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(test_config test_config.cpp)
    target_link_libraries(test_config PRIVATE UReactor::ureactor)
    add_test(NAME test_config COMMAND test_config)
    set_tests_properties(test_config PROPERTIES TIMEOUT 30)
endif()

add_executable(test_backtrace test_backtrace.cpp)
target_link_libraries(test_backtrace PRIVATE UReactor::ureactor)
add_test(NAME test_backtrace COMMAND test_backtrace)

foreach(mode IN ITEMS debug release)
    add_executable(test_assertion_${mode} test_assertion_diagnostics.cpp)
    target_link_libraries(test_assertion_${mode} PRIVATE UReactor::ureactor)
    if(mode STREQUAL "release")
        target_compile_definitions(test_assertion_${mode} PRIVATE UREACTOR_TEST_RELEASE)
    endif()
    add_test(NAME test_assertion_${mode}
        COMMAND "${CMAKE_COMMAND}"
            "-DPROGRAM=$<TARGET_FILE:test_assertion_${mode}>"
            "-DRELEASE_MODE=$<STREQUAL:${mode},release>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/check_assertion_output.cmake")
endforeach()

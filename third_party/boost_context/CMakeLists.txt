# UReactor wrapper. The vendored libs/* CMake files are unchanged upstream files.
set(BOOST_SUPERPROJECT_VERSION 1.92.0)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(context_modules assert config core integer mp11 pool predef smart_ptr
    throw_exception type_traits winapi context)
foreach(module IN LISTS context_modules)
    add_subdirectory(libs/${module} SYSTEM)
    # Official source archives use a shared boost/ tree instead of modular include/.
    set_target_properties(boost_${module} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>;$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
    install(TARGETS boost_${module} EXPORT UReactorTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endforeach()
target_include_directories(boost_context PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
install(DIRECTORY include/boost DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES LICENSE_1_0.txt manifest.json
    DESTINATION "${CMAKE_INSTALL_DATADIR}/UReactor/boost_context")

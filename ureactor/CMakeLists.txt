# 按教程递归收集源码；CONFIGURE_DEPENDS 会检查文件列表的变化。
file(GLOB_RECURSE UREACTOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE UREACTOR_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_library(ureactor STATIC ${UREACTOR_SOURCES} ${UREACTOR_HEADERS})
add_library(UReactor::ureactor ALIAS ureactor)
set_target_properties(ureactor PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(ureactor
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(ureactor PUBLIC ureactor_options Threads::Threads)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ureactor PRIVATE -Werror)
elseif(MSVC)
    target_compile_options(ureactor PRIVATE /WX)
endif()

install(TARGETS ureactor ureactor_options EXPORT UReactorTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
# 保留公开头文件的目录层次，避免同名头文件覆盖。
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

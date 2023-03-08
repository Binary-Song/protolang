cmake_minimum_required(VERSION 3.23.1)
project(Protolang LANGUAGES CXX)

# 设置CMake变量 
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
option(BUILD_TESTING "是否创建单元测试目标" OFF)

if (MSVC)
    set(CMAKE_CXX_FLAGS "/utf-8")
endif ()

# 包含CMake模块
include(StandardFlags)

# 寻找依赖
# reflex: https://github.com/Genivia/RE-flex
# 如果找不到，尝试将REFLEX_ROOT环境变量到reflex的根目录
# 或将reflex二进制程序的位置加入到PATH环境变量，
find_path(REFLEX_INCLUDE_DIR reflex/matcher.h PATHS "$ENV{REFLEX_ROOT}/include" REQUIRED)
message("Found Reflex include dir: ${REFLEX_INCLUDE_DIR}")
set(REFLEX_ROOT "${REFLEX_INCLUDE_DIR}/..")

find_program(REFLEX_EXECUTABLE_PATH NAMES "reflex" PATHS "${REFLEX_INCLUDE_DIR}/../bin/win32" DOC "reflex 程序的路径" REQUIRED)
message("Found Reflex executable: ${REFLEX_EXECUTABLE_PATH}")


# 创建target：Protolang
file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
add_executable(Protolang
        ${SOURCE_FILES}
        src/exceptions.h src/env.h src/namedobject.h src/ast.cpp src/typechecker.h)

# 设置reflex输出目录
set(REFLEX_OUTPUT_DIR "${PROJECT_BINARY_DIR}/src_generated")
file(MAKE_DIRECTORY "${REFLEX_OUTPUT_DIR}")
# 设置reflex输入输出文件
set(REFLEX_SRC "${PROJECT_SOURCE_DIR}/src/lexer.l")
set(REFLEX_OUT_CPP "${REFLEX_OUTPUT_DIR}/lex.yy.cpp")
set(REFLEX_OUT_H "${REFLEX_OUTPUT_DIR}/lex.yy.h")
# 设置reflex命令
add_custom_command(
        OUTPUT "${REFLEX_OUT_CPP}" "${REFLEX_OUT_H}" # 在别人依赖这两个文件时，会自动调用该命令生成它们
        COMMAND "${REFLEX_EXECUTABLE_PATH}" "${REFLEX_SRC}"
        MAIN_DEPENDENCY "${REFLEX_SRC}"
        WORKING_DIRECTORY ${REFLEX_OUTPUT_DIR}
        VERBATIM)

# 创建target：lexer
file(GLOB_RECURSE LEXER_SOURCE_FILES
        "${REFLEX_ROOT}/lib/*.cpp"
        "${REFLEX_ROOT}/unicode/*.cpp")
add_library(lexer STATIC "${REFLEX_OUT_CPP};${LEXER_SOURCE_FILES}")
target_include_directories(lexer PUBLIC "${REFLEX_OUTPUT_DIR}" "${REFLEX_INCLUDE_DIR}" "${PROJECT_SOURCE_DIR}/src")

# link
target_link_libraries(Protolang PUBLIC lexer)

#target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt5::Widgets)
set_standard_flags(${CMAKE_PROJECT_NAME})


# Google test
#set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
#add_subdirectory("3rdparty/googletest-release-1.12.1")
#enable_testing()
#add_executable(
#        hello_test
#        "./test/test.cpp"
#)
#target_link_libraries(
#        hello_test
#        GTest::gtest_main
#)
#include(GoogleTest)
#gtest_discover_tests(hello_test)
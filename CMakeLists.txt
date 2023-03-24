cmake_minimum_required(VERSION 3.23.1)
project(Protolang LANGUAGES CXX)


# 设置CMake变量
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BUILD_TESTING "是否创建单元测试目标" OFF)
option(PROTOLANG_USE_WCHAR
        "在调用系统API时（写文件、输出等）使用是wchar_t还是char。在Windows上，wchar_t即utf16，char即ansi。在Linux等支持UTF-8的平台上一般不使用wchar_t"
        OFF)

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

# llvm:
find_package(LLVM REQUIRED)
message("Found LLVM @ ${LLVM_CONFIG}")

# fmt:
set(FMT_INCLUDE
        "${CMAKE_CURRENT_LIST_DIR}/src/3rdparty/fmt/include")
set(FMT_SRC
        "${CMAKE_CURRENT_LIST_DIR}/src/3rdparty/fmt/src/format.cc"
        )

# 创建target：Protolang
file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/*.h")
add_executable(Protolang
        ${SOURCE_FILES}
        ${FMT_SRC})

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
set_standard_flags(Protolang)
target_include_directories(Protolang PUBLIC
        ${LLVM_INCLUDE_DIRS}
        "src/"
        ${FMT_INCLUDE}
        )
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
target_compile_definitions(Protolang PUBLIC ${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs core irreader x86codegen x86asmparser)
target_link_libraries(Protolang PUBLIC lexer ${llvm_libs})

# add defs
set(DEFS "")
if (MSVC)
    list(APPEND DEFS "PROTOLANG_HOST_MSVC")
endif ()
if (PROTOLANG_USE_WCHAR)
    list(APPEND DEFS "PROTOLANG_USE_WCHAR")
endif ()
# set defs
target_compile_definitions(Protolang PUBLIC "${DEFS}")


option(BUILD_DOC "Generate documentation" FALSE)
if (BUILD_DOC)
    include(Doc)
    add_doc()
endif ()

# play
add_executable(Playground "./test/playground.cpp"
        "./src/encoding/win/encoding_win.cpp" ${FMT_SRC})
target_include_directories(Playground PUBLIC ${FMT_INCLUDE} ${LLVM_INCLUDE_DIRS} ./src/)
target_link_libraries(Playground PUBLIC ${llvm_libs})
target_compile_definitions(Playground PUBLIC "${DEFS} ")

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
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Test REQUIRED)
enable_testing(true)


# 添加 qt 测试目标
# 语法：
#   add_qt_test(NAME <测试名> SOURCES <源文件> ... INCLUDE_DIRECTORIES <包含文件> ... LINK_LIBRARIES <链接> ...) 
macro(add_qt_test)
    cmake_parse_arguments("ARG"
            ""
            "NAME"
            "SOURCES;INCLUDE_DIRECTORIES;LINK_LIBRARIES"
            ${ARGN})
    add_executable(${ARG_NAME} "${ARG_SOURCES}")
    add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})
    target_link_libraries(${ARG_NAME} PRIVATE Qt5::Test ${ARG_LINK_LIBRARIES})
    target_include_directories(${ARG_NAME} PRIVATE ${ARG_INCLUDE_DIRECTORIES})
endmacro()
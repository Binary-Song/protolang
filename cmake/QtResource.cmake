# 自动生成 qrc 文件
# 语法：
#   generate_qrc(<RESOURCE_DIR 资源文件根目录> <OUTPUT_PATH 输出文件路径> [RELATIVE 相对路径根目录])
# 参数：
#   RESOURCE_DIR （绝对路径）资源文件文件夹，里面的东西都会被rcc打包
#   OUTPUT_PATH （绝对路径）输出的qrc文件路径
#   RELATIVE （绝对路径）在代码中访问资源时，使用的路径是相对于哪里。默认等于RESOURCE_DIR。例如，RESOURCE_DIR 为 "D:/rsc" ，RELATIVE 为"D:/"，则
#   在代码中访问"D:/rsc/img.png"时，需要使用":/rsc/img.png"。
function(generate_qrc)

    cmake_parse_arguments("ARG"
            ""
            "RESOURCE_DIR;OUTPUT_PATH;RELATIVE"
            ""
            ${ARGN})

    set(RESOURCE_DIR ${ARG_RESOURCE_DIR})
    set(OUTPUT_PATH ${ARG_OUTPUT_PATH})
    set(RELATIVE ${ARG_RELATIVE})

    if (NOT IS_ABSOLUTE ${RESOURCE_DIR})
        message(FATAL_ERROR "arg RESOURCE_DIR must be an absolute path (RESOURCE_DIR=${RESOURCE_DIR})")
    endif ()

    if (NOT RELATIVE)
        set(RELATIVE "${RESOURCE_DIR}")
    endif ()

    if (NOT IS_ABSOLUTE ${RELATIVE})
        message(FATAL_ERROR "arg RELATIVE must be an absolute path (RELATIVE=${RELATIVE})")
    endif ()

    file(GLOB_RECURSE RESOURCE_FILES
            LIST_DIRECTORIES false
            RELATIVE "${RELATIVE}"
            "${RESOURCE_DIR}/*")

    foreach (FILE_REL ${RESOURCE_FILES})
        get_filename_component(FILE_ABS "${FILE_REL}"
                REALPATH BASE_DIR "${RELATIVE}")
        set(QRC_FILE_PLACEHOLDER "${QRC_FILE_PLACEHOLDER} <file alias=\"${FILE_REL}\">${FILE_ABS}</file>")
    endforeach ()

    set(CONTENT "<!DOCTYPE RCC><RCC version=\"1.0\"><qresource>${QRC_FILE_PLACEHOLDER}</qresource></RCC>")
    write_file("${OUTPUT_PATH}" "${CONTENT}")
endfunction()

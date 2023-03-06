# 打包发布Qt应用程序
# 语法：
#   qt_deploy([RUN_7ZIP] [AUTO_RUN] <TARGET 目标> [DEPLOY_ARGS arg1 arg2...])
# 参数：
#   RUN_7ZIP    是否用7zip打包成.7z文件
#   AUTO_RUN    是否在目标生成后自动运行打包程序（否则创建单独的Package_xxx目标）
#   TARGET      要打包的目标
#   DEPLOY_ARGS 传给windeployqt.exe的额外参数
# 说明：
#   默认运行windeployqt，效果是将所需dll复制到qt程序所在目录。
#   若指定 RUN_7ZIP，则还会将dll、exe等文件压缩成7z文件，便于发布
#   若指定 AUTO_RUN，则上述过程会作为目标的构建后事件，在每次构建完成后自动进行。若不指定，则会创建一个“Package_xxx”目标，需手动构建该目标来完成上述过程。
function(qt_deploy)

    cmake_parse_arguments("ARG"
            "RUN_7ZIP;AUTO_RUN" # 是否运行7zip
            "TARGET"
            "DEPLOY_ARGS"
            ${ARGN})

    set(TARGET ${ARG_TARGET})
    set(RUN_7ZIP ${ARG_RUN_7ZIP})
    set(AUTO_RUN ${ARG_AUTO_RUN})
    set(DEPLOY_ARGS ${ARG_DEPLOY_ARGS})

    # 寻找程序
    find_program(WINDEPLOY_EXE windeployqt PATHS "${Qt5_ROOT}/bin")
    find_program(ZIP_EXE "7z")

    # 找不到 windeployqt.exe
    if (NOT WINDEPLOY_EXE)
        message("windeployqt not found. Packaging disabled.")
        return()
    endif ()

    # 找不到 7zip.exe
    if (RUN_7ZIP AND NOT ZIP_EXE)
        message("7zip not found. Packaging disabled.")
        return()
    endif ()

    # target 不是目标
    if (NOT TARGET ${TARGET})
        message("Main target not set. Packaging disabled.")
        return()
    endif ()

    # PACKAGING_CWD 是我们打包的文件夹，会把所有东西拷贝到这里
    set(PACKAGING_CWD "${CMAKE_BINARY_DIR}/PackagingTemp")
    file(MAKE_DIRECTORY "${PACKAGING_CWD}") # 确保该路径存在
    get_filename_component(BINARY_DIR_NAME "${CMAKE_BINARY_DIR}" NAME) # 用二进制目录的名字作为压缩文件名的一部分
    set(PACKAGE_TARGET "Package_${TARGET}") # 打包目标的名字
    set(ZIP_OUTPUT "${CMAKE_BINARY_DIR}/${TARGET}-${BINARY_DIR_NAME}.7z") # 输出压缩文件的路径

    if (RUN_7ZIP)
        set(COMMAND
                COMMAND "${WINDEPLOY_EXE}" "$<TARGET_FILE:${TARGET}>" "--dir" "${PACKAGING_CWD}" ${DEPLOY_ARGS} # 将dll拷贝到cwd
                COMMAND "${WINDEPLOY_EXE}" "$<TARGET_FILE:${TARGET}>" "--dir" "$<TARGET_FILE_DIR:${TARGET}>" ${DEPLOY_ARGS}# 将dll拷贝到exe所在目录
                COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${TARGET}>" "${PACKAGING_CWD}" # 将exe拷贝到cwd
                COMMAND "${ZIP_EXE}" a -mx=9 ${ZIP_OUTPUT} "${PACKAGING_CWD}/*" # 7zip 打包
                )
        set(BYPRODUCTS BYPRODUCTS ${ZIP_OUTPUT} ${PACKAGING_CWD})
    else ()
        set(COMMAND
                COMMAND "${WINDEPLOY_EXE}" "$<TARGET_FILE:${TARGET}>" "--dir" "$<TARGET_FILE_DIR:${TARGET}>" ${DEPLOY_ARGS}# 将dll拷贝到exe所在目录
                )
        set(BYPRODUCTS "")
    endif ()

    if (AUTO_RUN)
        add_custom_command(
                TARGET ${TARGET}
                POST_BUILD
                ${COMMAND}
                ${BYPRODUCTS}
        )
    else ()
        add_custom_target(
                ${PACKAGE_TARGET}
                ${COMMAND}
                DEPENDS ${TARGET}
                ${BYPRODUCTS}
                WORKING_DIRECTORY "${PACKAGING_CWD}"
        )
    endif ()

endfunction()

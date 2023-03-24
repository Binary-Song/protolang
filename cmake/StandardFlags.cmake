function(set_standard_flags TARGET)
    if (MSVC)

        set(GENERAL_FLAGS
                /utf-8 # 源文件编码 utf-8
                /W4 # 警告等级
                #/WX # 警告=错误
                /Zc:preprocessor # 预处理器符合标准
                /permissive- # 各种符合标准
                #/GR- # 禁用RTII
                /EHsc
                /external:anglebrackets # 尖括号include被认为是外部代码
                /external:W0 # 外部代码不要警告
                )
        target_compile_options(${TARGET} PRIVATE ${GENERAL_FLAGS})
    endif ()
endfunction()


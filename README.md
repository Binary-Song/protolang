# 文件监视器

## 编译流程

编译**必需**的依赖：

| 依赖项 | 安装方式 | 备注 |
| --- | --- | --- |
| Visual Studio | 见官网 | 需要安装MSVC |
| Qt 5.9.3（或兼容版本） | 需指定环境变量 | 安装后在 CMakeUserPresets.json 中更改 `environment` 中 `Qt5_MSVC_64_DIR` 等变量，安装了哪些库就指定哪些变量，没有安装的留空，示例：`"Qt5_MSVC_64_DIR": "C:/Qt/5.9.3/msvc2017_64/lib/cmake/Qt5/"` |
| googletest 1.12.1 | 下载源码后放在 3rdparty 目录 | 源码文件夹名称：googletest-release-1.12.1 |

编译**可选**的依赖：

| 依赖项 | 安装方式 | 说明 | 
| --- | --- | --- |
| 7 zip | 安装后确保可执行文件在PATH下 | 用于打包部署 |
 

配置

```
cmake -S . --preset msvc-x64-release 
```

构建

```
cmake --build --preset msvc-x64-release
```

## CMakeUserPresets 示例

其中的环境变量需要根据自己环境的情况修改

```json
{
    "version": 3,
    "configurePresets": [
        {
            "name": "base",
            "hidden": true,
            "warnings": {
                "dev": false
            },
            "binaryDir": "build/${presetName}",
            "environment": {
                "Qt5_MSVC_64_DIR": "C:/Qt/5.9.3/msvc2017_64/lib/cmake/Qt5/",
                "Qt5_MSVC_32_DIR": "",
                "Qt5_MINGW_64_DIR": "",
                "Qt5_MINGW_32_DIR": "",
                "MINGW_PATH": "" 
            }
        },
        {
            "name": "msvc",
            "hidden": true,
            "inherits": [
                "base"
            ],
            "generator": "Visual Studio 17 2022",
            "toolset": "v143", 
            "vendor": {
                "jetbrains.com/clion": {
                    "toolchain": "Visual Studio 2022 (MSVC 2017 v141)"
                }
            }
        },
        {
            "name": "msvc-x64",
            "hidden": true,
            "inherits": [
                "msvc"
            ],
            "architecture": "x64",
            "environment": {
                "Qt5_DIR": "$env{Qt5_MSVC_64_DIR}"
            },
            "condition": {
                "type": "notEquals",
                "lhs": "$env{Qt5_MSVC_64_DIR}",
                "rhs": ""
            }
        },
        {
            "name": "msvc-x86",
            "hidden": true,
            "inherits": [
                "msvc"
            ],
            "architecture": "Win32",
            "environment": {
                "Qt5_DIR": "$env{Qt5_MSVC_32_DIR}"
            },
            "condition": {
                "type": "notEquals",
                "lhs": "$env{Qt5_MSVC_32_DIR}",
                "rhs": ""
            }
        },
        {
            "name": "msvc-x64-debug",
            "inherits": [
                "msvc-x64"
            ]
        },
        {
            "name": "msvc-x64-release",
            "inherits": [
                "msvc-x64"
            ]
        },
        {
            "name": "msvc-x86-debug",
            "inherits": [
                "msvc-x86"
            ]
        },
        {
            "name": "msvc-x86-release",
            "inherits": [
                "msvc-x86"
            ]
        },
        {
            "name": "mingw",
            "inherits": [
                "base"
            ],
            "hidden": true,
            "generator": "MinGW Makefiles"
        },
        {
            "name": "mingw-x64",
            "inherits": [
                "mingw"
            ],
            "hidden": true,
            "environment": {
                "PATH": "$env{MINGW_PATH}",
                "Qt5_DIR": "$env{Qt5_MINGW_64_DIR}"
            },
            "vendor": {
                "jetbrains.com/clion": {
                    "toolchain": "MinGW x64"
                }
            },
            "condition": {
                "type": "notEquals",
                "lhs": "$env{Qt5_MINGW_64_DIR}",
                "rhs": ""
            }
        },
        {
            "name": "mingw-x86",
            "inherits": [
                "mingw"
            ],
            "hidden": true,
            "environment": {
                "PATH": "$env{MINGW_PATH}",
                "Qt5_DIR": "$env{Qt5_MINGW_32_DIR}"
            },
            "vendor": {
                "jetbrains.com/clion": {
                    "toolchain": "MinGW x86"
                }
            },
            "condition": {
                "type": "notEquals",
                "lhs": "$env{Qt5_MINGW_32_DIR}",
                "rhs": ""
            }
        },
        {
            "name": "mingw-x64-debug",
            "inherits": [
                "mingw-x64"
            ],
            "environment": {
                "CMAKE_BUILD_TYPE": "Debug"
            }
        },
        {
            "name": "mingw-x64-release",
            "inherits": [
                "mingw-x64"
            ],
            "environment": {
                "CMAKE_BUILD_TYPE": "Release"
            }
        },
        {
            "name": "mingw-x86-debug",
            "inherits": [
                "mingw-x86"
            ],
            "environment": {
                "CMAKE_BUILD_TYPE": "Debug"
            }
        },
        {
            "name": "mingw-x86-release",
            "inherits": [
                "mingw-x86"
            ],
            "environment": {
                "CMAKE_BUILD_TYPE": "Release"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "mingw-x64-debug",
            "configurePreset": "mingw-x64-debug"
        },
        {
            "name": "mingw-x64-release",
            "configurePreset": "mingw-x64-release"
        },
        {
            "name": "mingw-x86-debug",
            "configurePreset": "mingw-x86-debug"
        },
        {
            "name": "mingw-x86-release",
            "configurePreset": "mingw-x86-release"
        },
        {
            "name": "msvc-x64-debug",
            "configurePreset": "msvc-x64-debug",
            "configuration": "Debug"
        },
        {
            "name": "msvc-x64-release",
            "configurePreset": "msvc-x64-release",
            "configuration": "Release"
        },
        {
            "name": "msvc-x86-debug",
            "configurePreset": "msvc-x86-debug",
            "configuration": "Debug"
        },
        {
            "name": "msvc-x86-release",
            "configurePreset": "msvc-x86-release",
            "configuration": "Release"
        }
    ]
}
```
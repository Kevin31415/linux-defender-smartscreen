cmake_minimum_required(VERSION 3.16)
project(linux-defender-smartscreen LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 公共源文件
set(COMMON_SOURCES
    src/common/tagging.cpp
    src/common/logger.cpp
)

# smartscreend (守护进程 + CLI)
add_executable(smartscreend
    src/daemon/main.cpp
    src/daemon/watcher.cpp
    ${COMMON_SOURCES}
)
target_include_directories(smartscreend PRIVATE src)
target_link_libraries(smartscreend PRIVATE pthread)

# smartscreen-dialog (Qt 弹窗)
find_package(Qt6 COMPONENTS Widgets REQUIRED)
add_executable(smartscreen-dialog
    src/dialog/main.cpp
    src/dialog/dialogwindow.cpp
    ${COMMON_SOURCES}
)
set_target_properties(smartscreen-dialog PROPERTIES AUTOMOC ON)
target_include_directories(smartscreen-dialog PRIVATE src)
target_link_libraries(smartscreen-dialog PRIVATE Qt6::Widgets)

# libsmartscreen_hook.so (LD_PRELOAD exec 拦截)
add_library(smartscreen_hook SHARED
    src/hook/hook_exec.c
)
target_include_directories(smartscreen_hook PRIVATE src/hook)
target_link_libraries(smartscreen_hook PRIVATE dl)
set_target_properties(smartscreen_hook PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "smartscreen_hook"
    POSITION_INDEPENDENT_CODE ON
)

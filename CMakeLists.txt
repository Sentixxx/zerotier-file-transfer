cmake_minimum_required(VERSION 3.10)
project(vport_project CXX)

# 检查编译器支持
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        message(FATAL_ERROR "GCC version must be at least 7.0!")
    endif()
    # GCC 7/8 需要链接 stdc++fs
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        link_libraries(stdc++fs)
    endif()
endif()

# C++17 要求
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加编译选项
add_compile_options(-Wall -Wextra -Wpedantic)

# 添加 filesystem 库支持
link_libraries(stdc++fs)

# 检查必要的头文件
include(CheckIncludeFileCXX)
check_include_file_cxx("linux/if.h" HAVE_IF_H)
check_include_file_cxx("linux/if_tun.h" HAVE_IF_TUN_H)

if(NOT HAVE_IF_H OR NOT HAVE_IF_TUN_H)
    message(FATAL_ERROR "Missing required Linux headers!")
endif()

# 添加头文件路径
include_directories(${PROJECT_SOURCE_DIR}/include)

# 添加源文件
add_executable(vport
    src/main.cpp
    src/tap_device.cpp
    src/vport.cpp
    src/file_transfer.cpp
)

# 查找并链接pthread库
find_package(Threads REQUIRED)
target_link_libraries(vport PRIVATE Threads::Threads)

# 安装目标
install(TARGETS vport
    RUNTIME DESTINATION bin
) 
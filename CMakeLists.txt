cmake_minimum_required(VERSION 3.10)
project(vport_project CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加头文件路径
include_directories(${PROJECT_SOURCE_DIR}/include)

# 添加源文件
add_executable(vport
    src/main.cpp
    src/tap_device.cpp
    src/vport.cpp
)

# 链接pthread库
target_link_libraries(vport pthread) 
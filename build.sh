#!/bin/bash

# 检查是否安装了cmake
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake is not installed"
    exit 1
fi

# 创建项目结构
mkdir -p include src build

# 移动源文件到正确的位置
mv tap_device.hpp include/ 2>/dev/null || true
mv vport.hpp include/ 2>/dev/null || true
mv tap_device.cpp src/ 2>/dev/null || true
mv vport.cpp src/ 2>/dev/null || true
mv main.cpp src/ 2>/dev/null || true

# 进入build目录
cd build

# 运行cmake
cmake .. || {
    echo "CMake configuration failed"
    exit 1
}

# 编译
make -j$(nproc) || {
    echo "Build failed"
    exit 1
}

echo "Build successful!"
echo "You can find the executable at: build/vport"

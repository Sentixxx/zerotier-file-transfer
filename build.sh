#!/bin/bash

# 检查是否安装了cmake
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake is not installed"
    exit 1
fi

# 清理上次编译结果
rm -rf build/*

# 创建项目结构
mkdir -p include src build

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

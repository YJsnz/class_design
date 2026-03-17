#!/bin/bash
 
cd ..
# 创建build目录
mkdir -p build
cd build
 
# 运行CMake
cmake ..
 
# 编译项目
make
 

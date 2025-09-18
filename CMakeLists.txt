# 设置最低版本号
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

# 设置项目名称
project(serdes VERSION 0.0.1 LANGUAGES CXX)

# 设置编译器
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})

# 强制修改 Release 标志，添加调试信息
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -g -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

# 输出系统信息
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Install: ${CMAKE_INSTALL_PREFIX}")

# add_subdirectory(test)
add_subdirectory(example)

# install(DIRECTORY scripts DESTINATION output)
# install(DIRECTORY etc DESTINATION output)
# install(DIRECTORY weights/ DESTINATION output/models)

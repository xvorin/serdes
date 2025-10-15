# 设置最低版本号
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

# 设置项目名称
project(serdes VERSION 0.0.1 LANGUAGES CXX)

# 设置编译器
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 强制修改 Release 标志，添加调试信息
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -g -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

# 输出系统信息
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Install: ${CMAKE_INSTALL_PREFIX}")

option(BUILD_EXAMPLES "Build the example programs" OFF)

if(BUILD_EXAMPLES)
    message(STATUS "Building example")
    add_subdirectory(example)
endif()

install(DIRECTORY types serdes validator utils 3rd DESTINATION include/serdes)
install(FILES serdes.h DESTINATION include/serdes) 

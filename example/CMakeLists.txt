cmake_minimum_required(VERSION 3.5)

set(3RDPATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rd/)

# 文件监听使用 std::thread
find_package(Threads REQUIRED)

# 用来搜索头文件的目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${3RDPATH}
    ${3RDPATH}/protobuf/install/include
)

# protobuf库存在时启用protobuf后端并链接(Importer位于libprotoc, 故两个库都链); 不存在时保持ENABLE_PROTOBUF默认关闭
set(PROTOBUF_INSTALL ${3RDPATH}/protobuf/install)
set(PROTOBUF_LIB ${PROTOBUF_INSTALL}/lib/libprotobuf.a ${PROTOBUF_INSTALL}/lib/libprotoc.a)
if(EXISTS ${PROTOBUF_INSTALL}/lib/libprotobuf.a)
    add_definitions(-DENABLE_PROTOBUF=1)
else()
    set(PROTOBUF_LIB "")
endif()

add_library(definition STATIC definition.cpp)

add_executable(test_serdes main.cpp)
target_link_libraries(test_serdes
    "-Wl,--whole-archive" definition "-Wl,--no-whole-archive"
    ${PROTOBUF_LIB}
    Threads::Threads
)

add_executable(simple simple.cpp)
target_link_libraries(simple
    ${PROTOBUF_LIB}
    Threads::Threads
)

add_executable(subtree_demo subtree.cpp)
target_link_libraries(subtree_demo
    ${PROTOBUF_LIB}
    Threads::Threads
)

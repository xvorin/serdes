
set(3RDPATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rd/)

# 用来搜索头文件的目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${3RDPATH}
    ${3RDPATH}/protobuf/install/include
)

# protobuf库不存在时不链接(ENABLE_PROTOBUF默认关闭, 代码不再依赖libprotobuf)
set(PROTOBUF_LIB ${3RDPATH}/protobuf/install/lib/libprotobuf.a)
if(NOT EXISTS ${PROTOBUF_LIB})
    set(PROTOBUF_LIB "")
endif()

add_library(definition STATIC definition.cpp)

add_executable(test_serdes main.cpp)
target_link_libraries(test_serdes
    "-Wl,--whole-archive" definition "-Wl,--no-whole-archive"
    ${PROTOBUF_LIB}
)

add_executable(simple simple.cpp)
target_link_libraries(simple
    ${PROTOBUF_LIB}
)

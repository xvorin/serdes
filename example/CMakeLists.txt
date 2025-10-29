set(3RDPATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rd/)

# 用来搜索头文件的目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${3RDPATH}
    ${3RDPATH}/protobuf/install/include
)

add_library(definition STATIC definition.cpp)

add_executable(test_serdes main.cpp)
target_link_libraries(test_serdes
    "-Wl,--whole-archive" definition "-Wl,--no-whole-archive"
    ${3RDPATH}/protobuf/install/lib/libprotobuf.a
)

add_executable(simple simple.cpp)
target_link_libraries(simple
    ${3RDPATH}/protobuf/install/lib/libprotobuf.a
)

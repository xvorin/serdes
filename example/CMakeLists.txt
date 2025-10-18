set(3RDPATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rd/)

# 用来搜索头文件的目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${3RDPATH}
    ${3RDPATH}/protobuf/install/include
)

add_executable(test_serdes main.cpp definition.cpp)
target_link_libraries(test_serdes
    ${3RDPATH}/protobuf/install/lib/libprotobuf.a
)

add_executable(simple simple.cpp)
target_link_libraries(simple
    ${3RDPATH}/protobuf/install/lib/libprotobuf.a
)

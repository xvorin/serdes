set(3RDPATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rd/)

# 用来搜索头文件的目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${3RDPATH}
    ${3RDPATH}/yaml-cpp/install/include
    ${3RDPATH}/protobuf/install/include
)

file(GLOB_RECURSE SRC *.cpp)

add_executable(test_serdes ${SRC})
# add_executable(test_serdes "../test.cpp")

target_link_libraries(test_serdes
    # ${3RDPATH}/yaml-cpp/install/lib/libyaml-cpp.a
    ${3RDPATH}/protobuf/install/lib/libprotobuf.a
)

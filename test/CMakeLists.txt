# serdes 单元测试: cmake -DBUILD_TESTS=ON 时由根 CMakeLists 引入
cmake_minimum_required(VERSION 3.11)
project(serdes_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rd
)

# protobuf 子库(3rd/protobuf/install)存在时启用 protobuf 后端并链接
set(PROTOBUF_INSTALL ${CMAKE_CURRENT_SOURCE_DIR}/../3rd/protobuf/install)
if(EXISTS ${PROTOBUF_INSTALL}/lib/libprotobuf.a)
    add_definitions(-DENABLE_PROTOBUF=1)
    include_directories(${PROTOBUF_INSTALL}/include)
    set(PB_LIBS ${PROTOBUF_INSTALL}/lib/libprotobuf.a ${PROTOBUF_INSTALL}/lib/libprotoc.a)
    message(STATUS "serdes test: protobuf backend enabled")
endif()

find_package(Threads REQUIRED)

# 公共注册库(独立 TU, 跨 TU 注册场景)
add_library(testdefs STATIC definitions.cpp)

# 每个测试一个可执行 + ctest 用例; 注册库须 whole-archive 保留
function(add_serdes_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name}
        "-Wl,--whole-archive" testdefs "-Wl,--no-whole-archive"
        Threads::Threads
        ${PB_LIBS}
    )
    add_test(NAME ${name} COMMAND ${name})
    # 并发用例若死锁应判定失败, 而不是无限挂住
    set_tests_properties(${name} PROPERTIES TIMEOUT 120)
endfunction()

add_serdes_test(basic_types)
add_serdes_test(enum)
add_serdes_test(composite)
add_serdes_test(tree_api)
add_serdes_test(backends)
add_serdes_test(subtree)
add_serdes_test(concurrency)
if(EXISTS ${PROTOBUF_INSTALL}/lib/libprotobuf.a)
    add_serdes_test(protobuf)
endif()

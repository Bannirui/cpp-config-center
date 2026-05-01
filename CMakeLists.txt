cmake_minimum_required(VERSION 3.20)
project(config-center VERSION 1.0.0 LANGUAGES CXX)

# 关掉cmake的warning
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 自定义find_xxx.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── Build options ──────────────────────────────────────────────
option(BUILD_SERVER "Build config center server" ON)
option(BUILD_SDK "Build client SDK" ON)
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# ── Sanitizers ─────────────────────────────────────────────────
if (ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif ()
if (ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
endif ()

# ── Dependencies ───────────────────────────────────────────────
include(boost)            # find_package(Boost REQUIRED)
include(openssl)          # find_package(OpenSSL REQUIRED)
include(protobuf_grpc)    # find_package(Protobuf + gRPC)

include(nlohmann_json)    # header-only JSON, FetchContent
include(yaml-cpp)         # YAML parser, FetchContent
include(jwt-cpp)          # JWT (header-only), FetchContent
include(sqlite_orm)       # SQLite ORM (header-only), FetchContent

if (BUILD_TESTS)
    enable_testing()
    include(googletest)
endif ()

# ── Project targets ───────────────────────────────────────────
add_subdirectory(proto)

# 在顶层把头文件编成单独的库给实现去依赖
set(HEADER_ONLY_NAME ${PROJECT_NAME}-core)
add_library(${HEADER_ONLY_NAME} INTERFACE)
# 整个项目自己的头文件路径
target_include_directories(${HEADER_ONLY_NAME} INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# boost的头文件 在源码中直接#include <boost/beast.hpp>
# jwt-cpp is header-only — its include dir is in the source tree
target_include_directories(${HEADER_ONLY_NAME} INTERFACE
        ${Boost_INCLUDE_DIRS}
        ${jwt-cpp_SOURCE_DIR}/include
)

add_subdirectory(src)
add_subdirectory(sdk)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()

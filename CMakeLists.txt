cmake_minimum_required(VERSION 3.20)
project(config-center VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── Build options ──────────────────────────────────────────────
option(BUILD_SERVER "Build config center server" ON)
option(BUILD_SDK "Build client SDK" ON)
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# ── Sanitizers ─────────────────────────────────────────────────
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
endif()

# ── Dependencies ───────────────────────────────────────────────
include(boost)            # find_package(Boost REQUIRED)
include(openssl)          # find_package(OpenSSL REQUIRED)
include(protobuf_grpc)    # find_package(Protobuf + gRPC)

include(nlohmann_json)    # header-only JSON, FetchContent
include(yaml-cpp)         # YAML parser, FetchContent
include(jwt-cpp)          # JWT (header-only), FetchContent
include(sqlite_orm)       # SQLite ORM (header-only), FetchContent

if(BUILD_TESTS)
    enable_testing()
    include(googletest)
endif()

# ── Project targets ───────────────────────────────────────────
add_subdirectory(proto)

add_library(config-center-core STATIC)
target_include_directories(config-center-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(config-center-core PUBLIC
    Boost::system
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    sqlite_orm::sqlite_orm
    OpenSSL::SSL
    OpenSSL::Crypto
    config-center-proto
)
# jwt-cpp is header-only — its include dir is in the source tree
target_include_directories(config-center-core PUBLIC
    ${jwt-cpp_SOURCE_DIR}/include
)

add_subdirectory(src)
add_subdirectory(sdk)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

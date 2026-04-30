file(GLOB_RECURSE SDK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(config-center-sdk STATIC ${SDK_SOURCES})
target_include_directories(config-center-sdk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(config-center-sdk PUBLIC
    config-center-core
    Boost::beast
)

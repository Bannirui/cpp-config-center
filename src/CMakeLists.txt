file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(config-center-server ${SERVER_SOURCES})
target_link_libraries(config-center-server PRIVATE
    config-center-core
    gRPC::grpc++
    Boost::beast
)

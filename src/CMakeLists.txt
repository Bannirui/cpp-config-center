file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME}-server ${SERVER_SOURCES})
target_link_libraries(${PROJECT_NAME}-server PRIVATE
        ${HEADER_ONLY_NAME} # 这个项目的头文件
        gRPC::grpc++
        Boost::beast
)

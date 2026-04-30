set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTO_FILE ${PROTO_DIR}/config_service.proto)
set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# ── Find protoc and grpc_cpp_plugin ────────────────────────────
if(gRPC_FOUND AND TARGET gRPC::grpc_cpp_plugin)
    # gRPC CONFIG mode — plugin is a target
    get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
else()
    # FindGRPC / manual — locate the binary
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
endif()

find_program(PROTOC_BIN protoc)
if(NOT PROTOC_BIN AND TARGET protobuf::protoc)
    get_target_property(PROTOC_BIN protobuf::protoc LOCATION)
endif()
if(NOT PROTOC_BIN)
    message(FATAL_ERROR "protoc not found")
endif()

# ── Generate sources ───────────────────────────────────────────
set(PROTO_SRCS
    ${PROTO_BINARY_DIR}/config_service.pb.cc
    ${PROTO_BINARY_DIR}/config_service.pb.h
    ${PROTO_BINARY_DIR}/config_service.grpc.pb.cc
    ${PROTO_BINARY_DIR}/config_service.grpc.pb.h
)

add_custom_command(
    OUTPUT ${PROTO_SRCS}
    COMMAND ${PROTOC_BIN}
        --proto_path=${PROTO_DIR}
        --cpp_out=${PROTO_BINARY_DIR}
        --grpc_out=${PROTO_BINARY_DIR}
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
        ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
)

# ── Library ────────────────────────────────────────────────────
add_library(config-center-proto ${PROTO_SRCS})

target_include_directories(config-center-proto PUBLIC ${PROTO_BINARY_DIR})

if(TARGET protobuf::libprotobuf)
    target_link_libraries(config-center-proto PUBLIC protobuf::libprotobuf)
else()
    target_link_libraries(config-center-proto PUBLIC ${Protobuf_LIBRARIES})
    target_include_directories(config-center-proto PUBLIC ${Protobuf_INCLUDE_DIRS})
endif()

if(TARGET gRPC::grpc++)
    target_link_libraries(config-center-proto PUBLIC gRPC::grpc++)
else()
    target_link_libraries(config-center-proto PUBLIC ${GRPC_LIBRARIES})
    target_include_directories(config-center-proto PUBLIC ${GRPC_INCLUDE_DIRS})
endif()

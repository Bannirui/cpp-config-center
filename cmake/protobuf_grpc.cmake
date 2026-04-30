# Protobuf — try CONFIG first, fallback to Find
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
    find_package(Protobuf REQUIRED)
endif()

# gRPC — try CONFIG first, fallback to Find
find_package(gRPC CONFIG QUIET)
if(NOT gRPC_FOUND)
    find_package(gRPC REQUIRED)
endif()

cmake_minimum_required(VERSION 3.10)
project(GraphService CXX)

set(CMAKE_CXX_STANDARD 17)

# Paths to gRPC + protobuf on RCE
set(GRPC_INSTALL_DIR "/home/apps/grpc-1.74.1")
set(CMAKE_PREFIX_PATH ${GRPC_INSTALL_DIR}) # Tell find_package where to look

# Find the packages using their provided configuration
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# === Proto generation ===
set(PROTO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/graph.proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})
set(PROTO_SRCS ${PROTO_GEN_DIR}/graph.pb.cc ${PROTO_GEN_DIR}/graph.grpc.pb.cc)
add_custom_command(
    OUTPUT ${PROTO_SRCS}
    COMMAND ${GRPC_INSTALL_DIR}/bin/protoc
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
        --cpp_out=${PROTO_GEN_DIR}
        --grpc_out=${PROTO_GEN_DIR}
        --plugin=protoc-gen-grpc=${GRPC_INSTALL_DIR}/bin/grpc_cpp_plugin
        ${PROTO_SRC}
    DEPENDS ${PROTO_SRC}
)
include_directories(${PROTO_GEN_DIR})

# === Define a function to simplify creating executables ===
function(add_grpc_executable name source_file)
    add_executable(${name} ${source_file} ${PROTO_SRCS})
    # This is the "smart" linking that includes all dependencies automatically
    target_link_libraries(${name} PRIVATE gRPC::grpc++ protobuf::libprotobuf systemd)
endfunction()

# === Build ALL executables using the function ===
add_grpc_executable(server server.cpp)
add_grpc_executable(client client.cpp)
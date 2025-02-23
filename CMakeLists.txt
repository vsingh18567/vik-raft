cmake_minimum_required(VERSION 3.15)
project(grpc_example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(GENERATED_PROTO_DIR ${CMAKE_BINARY_DIR}/proto)
file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

# Find gRPC first (it will find its own compatible protobuf)
find_package(gRPC CONFIG REQUIRED)

# Generate protobuf and gRPC code
set(PROTO_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/service.proto"
)

add_library(proto_lib STATIC)

# Set include directories for the library
target_include_directories(proto_lib
    PUBLIC
    ${GENERATED_PROTO_DIR}
)

target_link_libraries(proto_lib
    PUBLIC
    gRPC::grpc++
    gRPC::grpc++_reflection
)

# Generate the gRPC and protobuf sources
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

# Configure protobuf generation to output to the generated directory
protobuf_generate(
    TARGET proto_lib 
    LANGUAGE cpp 
    PROTOS ${PROTO_FILES}
    IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/proto
    PROTOC_OUT_DIR ${GENERATED_PROTO_DIR}
)

protobuf_generate(
    TARGET proto_lib 
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
    PROTOS ${PROTO_FILES}
    IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/proto
    PROTOC_OUT_DIR ${GENERATED_PROTO_DIR}
)

# Server executable
add_executable(server src/main.cpp src/raft_node.cpp src/raft_network.cpp src/timer.cpp)
target_link_libraries(server
    PRIVATE
    proto_lib
)
target_include_directories(server PRIVATE ${GENERATED_PROTO_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
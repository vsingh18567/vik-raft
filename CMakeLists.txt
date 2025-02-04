cmake_minimum_required(VERSION 3.10)
project(vikraft VERSION 1.0)

# Set C++17 as the standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Protocol Buffers
find_package(Protobuf REQUIRED)

# Set the output directory for generated protobuf files
set(PROTO_GEN_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

# Generate protobuf files
protobuf_generate_cpp(
    PROTO_SRCS 
    PROTO_HDRS 
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/messages.proto
)

# Add release executable
add_executable(${PROJECT_NAME}_node 
    src/node/main.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

# Add debug executable
add_executable(${PROJECT_NAME}_node_debug
    src/node/main.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

add_executable(${PROJECT_NAME}_gateway
    src/gateway/main.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

target_compile_options(${PROJECT_NAME}_node_debug PRIVATE -g -O0 -Wall -Wextra -Wpedantic)

# Include directories (add for both targets)
target_include_directories(${PROJECT_NAME}_node PUBLIC
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/node
    ${CMAKE_CURRENT_SOURCE_DIR}/src/networking
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/include
    ${Protobuf_INCLUDE_DIRS}
    ${PROTO_GEN_DIR}
)
target_include_directories(${PROJECT_NAME}_node_debug PUBLIC
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/node
    ${CMAKE_CURRENT_SOURCE_DIR}/src/networking
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/include
    ${Protobuf_INCLUDE_DIRS}
    ${PROTO_GEN_DIR}
)

target_include_directories(${PROJECT_NAME}_gateway PUBLIC
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gateway
    ${CMAKE_CURRENT_SOURCE_DIR}/src/networking
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/include
    ${Protobuf_INCLUDE_DIRS}
    ${PROTO_GEN_DIR}
)

# Link libraries (add for both targets)
target_link_libraries(${PROJECT_NAME}_node PUBLIC
    ${Protobuf_LIBRARIES}
    absl::log
    absl::check
)
target_link_libraries(${PROJECT_NAME}_node_debug PUBLIC
    ${Protobuf_LIBRARIES}
    absl::log
    absl::check
)

target_link_libraries(${PROJECT_NAME}_gateway PUBLIC
    ${Protobuf_LIBRARIES}
    absl::log
    absl::check
)

# Add Abseil dependency
find_package(absl REQUIRED)
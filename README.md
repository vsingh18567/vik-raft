# Vikraft

A distributed key-value store using the Raft consensus algorithm. Just for fun and educational purposes.

## Prerequisites

- CMake (3.10 or higher)
- Protocol Buffers
- C++17 compatible compiler
- abseil (https://abseil.io/docs/cpp/quickstart-cmake.html) (todo: get rid of dependency)

## Building 

```bash
$ ./build.sh
```

## Design
1. node: the actual node that participates in the Raft consensus algorithm.
2. gateway: a proxy that allows clients to interact with the cluster.

Config is in JSON format (example: `config.json`).

## Running

```bash
$ ./build/vikraft_node <config_file> <node_id>
```
```bash
$ ./build/vikraft_gateway <config_file> <gateway_id>
```
Example interaction with the gateway:
```bash
$ echo '{"data" : "yeehaw"}' | nc 127.0.0.1 8083
{"success":true}
```

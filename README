# Sparser: High-Performance JSON Querying

## Overview

Sparser is an optimized JSON querying framework that minimizes full JSON parsing using cascade filtering. It efficiently processes newline-delimited JSON (NDJSON) records with substring filtering before detailed evaluation.

Original repository: [Sparser GitHub](https://github.com/stanford-futuredata/sparser)  
Research paper: [VLDB 2018](https://www.vldb.org/pvldb/vol11/p1576-palkar.pdf)

## Build Instructions

### Dependencies (vcpkg)

The project uses `vcpkg` for dependency management.

### Configure with CMake Presets

```sh
cmake --preset <preset>
```

### Build

```sh
cmake --build build
```

### Run

```sh
./build/SparserMain <input_file>
```

## Testing

To run tests:

```sh
cmake --build build --target test
ctest --test-dir build --output-on-failure
```

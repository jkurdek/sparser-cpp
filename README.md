# Sparser: High-Performance JSON Querying

## Overview

Sparser is an optimized JSON querying framework that minimizes full JSON parsing using cascade filtering. It efficiently processes newline-delimited JSON (NDJSON) records with substring filtering before detailed evaluation.

Original repository: [Sparser GitHub](https://github.com/stanford-futuredata/sparser)  
Research paper: [VLDB 2018](https://www.vldb.org/pvldb/vol11/p1576-palkar.pdf)

## Build Instructions

### Dependencies (vcpkg)

The project uses `vcpkg` for dependency management.

#### Install vcpkg
Follow the following instructions to install `vcpkg`:

```sh
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.sh
```

Add the following line to your shell configuration file (e.g., `~/.bashrc` or `~/.zshrc`):

```sh
export VCPKG_ROOT=/path/to/vcpkg
export PATH="$VCPKG_ROOT:$PATH"
```

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

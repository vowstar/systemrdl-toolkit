# SystemRDL Library Example

This directory contains a complete working example demonstrating the **modern SystemRDL API** usage.

## Building the Example

### Prerequisites

1. **Install the SystemRDL library first**:

```bash
# From the project root directory
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install
```

### Build and Run

```bash
# From the example/ directory
mkdir build && cd build
cmake ..
make

# Run the example
./example_app
```

The main project builds this same source as its `example` target and runs it
through `make test-example`. That is the in-tree smoke test; this standalone
project is the consumer-style build against an installed package.

## What the Example Does

It walks the API section by section: parse, hierarchical model JSON
elaboration, simplified JSON elaboration, advanced elaboration with arrays,
CSV conversion, file operations, stream operations, and error handling. Each
step prints its own status line, and the program exits non-zero if a step fails.

## Files

- `CMakeLists.txt` - CMake configuration for the example
- `example.cpp` - Main example source code demonstrating all API features

The file-based section writes its sample to the system temp directory, so
running the example leaves nothing behind.

## Integration in Your Project

To use the SystemRDL library in your own project, add this to your `CMakeLists.txt`:

```cmake
find_package(SystemRDL REQUIRED)
target_link_libraries(your_target SystemRDL::systemrdl)
```

Then include the modern API header:

```cpp
#include <systemrdl_api.h>
```

For the library API, see [doc/API.md](../doc/API.md).

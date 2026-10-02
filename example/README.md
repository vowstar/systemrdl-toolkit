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

## What the Example Does

It walks the API section by section: parse, hierarchical model JSON
elaboration, simplified JSON elaboration, advanced elaboration with arrays,
CSV conversion, file operations, stream operations, and error handling. Each
section prints an `[OK]` line. The program does not report a failure status, so
read its output.

## Files

- `CMakeLists.txt` - CMake configuration for the example
- `example.cpp` - Main example source code demonstrating all API features
- `test_example.rdl` - Sample SystemRDL file for testing

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

For complete API documentation and usage patterns, see the main project README.md.

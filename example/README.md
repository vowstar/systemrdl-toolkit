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

## What the Example Demonstrates

The example showcases all major features of the modern SystemRDL API:

- **🚀 String-based Operations**: Parse and elaborate SystemRDL content directly from strings
- **📁 File Operations**: Read SystemRDL files using convenient file wrappers
- **🌊 Stream Processing**: Input/output using standard C++ streams
- **📊 CSV Integration**: Convert CSV register data to SystemRDL format
- **🛡️ Error Handling**: Robust error management with Result types
- **🏗️ Advanced Elaboration**: Complex SystemRDL designs with arrays and hierarchies
- **⚡ Modern C++**: Clean API without ANTLR4 header exposure

## Expected Output

When you run the example, you should see output like:

```text
🚀 SystemRDL Modern API Example

📋 Example 1: Parse SystemRDL content
✅ Parse successful!

🚀 Example 2: Simple Elaboration
✅ Elaboration successful!

🎯 Example 3: Advanced Elaboration (Arrays & Complex Features)
✅ Advanced elaboration successful!

📊 Example 4: Convert CSV to SystemRDL
✅ CSV conversion successful!

📁 Example 5: File-based operations
✅ File parse successful!

🌊 Example 6: Stream operations
✅ Stream processing successful!

❗ Example 7: Error handling
✅ Error handling working correctly!
```

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
#include <systemrdl/systemrdl_api.h>
```

For complete API documentation and usage patterns, see the main project README.md.

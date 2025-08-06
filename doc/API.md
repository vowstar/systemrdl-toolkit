# Library Usage

This toolkit has been refactored to provide both a standalone library (`libsystemrdl`) and command-line tools.
The library enables easy integration of SystemRDL parsing and elaboration capabilities into other C++ projects.

## Library Features

- **Library Support**: Use SystemRDL functionality as a library in your C++ projects
- **Command-line Tools**: Traditional command-line tools for parsing and elaboration
- **Flexible Build**: Choose between shared/static libraries and optional components
- **Modern CMake**: Full CMake package support with `find_package()` integration
- **Code Quality**: Comprehensive testing and code quality tools

## Build Options

The project provides several build options to customize what gets built:

| Option | Default | Description |
|--------|---------|-------------|
| `SYSTEMRDL_BUILD_SHARED` | `ON` | Build shared library |
| `SYSTEMRDL_BUILD_STATIC` | `ON` | Build static library |
| `SYSTEMRDL_BUILD_TOOLS` | `ON` | Build command-line tools |
| `SYSTEMRDL_BUILD_TESTS` | `ON` | Build tests |
| `USE_SYSTEM_ANTLR4` | `OFF` | Use system ANTLR4 instead of downloading |

## Building the Library

### Basic Library Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Library-Only Build

If you only need the library (no command-line tools):

```bash
cmake .. -DSYSTEMRDL_BUILD_TOOLS=OFF -DSYSTEMRDL_BUILD_TESTS=OFF
make -j$(nproc)
```

### Installation

```bash
# Install to default location (/usr/local)
sudo make install

# Or install to custom location
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/systemrdl
make install
```

## Using the Library in Your Project

### Method 1: CMake find_package (Recommended)

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)

# Find SystemRDL library
find_package(SystemRDL REQUIRED)

# Create your application
add_executable(my_app main.cpp)

# Link against SystemRDL
target_link_libraries(my_app PRIVATE SystemRDL::systemrdl)
```

### Method 2: pkg-config

```bash
# Check if library is found
pkg-config --exists systemrdl && echo "SystemRDL library found"

# Compile with pkg-config
g++ -std=c++17 main.cpp $(pkg-config --cflags --libs systemrdl) -o my_app
```

### Method 3: Direct linking

```cmake
target_link_libraries(my_app PRIVATE systemrdl)
target_include_directories(my_app PRIVATE /usr/local/include/systemrdl)
```

## Library API Usage

The SystemRDL library provides both a **traditional API** for advanced users who need direct ANTLR4 access, and a
**modern API** for easy integration without ANTLR4 complexity.

### Modern API (Recommended)

The modern API provides a clean, simple interface without exposing ANTLR4 headers. It supports string-based
operations, file operations, and stream processing.

#### String-based Operations

```cpp
#include <systemrdl/systemrdl_api.h>
#include <iostream>

int main() {
    // Parse SystemRDL content
    std::string rdl_content = R"(
        addrmap simple_chip {
            reg {
                field {
                    sw = rw;
                    hw = r;
                    desc = "Control bit";
                } ctrl[0:0] = 0;
            } control_reg @ 0x0000;
        };
    )";

    // Parse to AST JSON
    auto parse_result = systemrdl::parse(rdl_content);
    if (parse_result.ok()) {
        std::cout << "Parse successful!" << std::endl;
        std::cout << "AST JSON: " << parse_result.value() << std::endl;
    } else {
        std::cerr << "Parse failed: " << parse_result.error() << std::endl;
        return 1;
    }

    // Elaborate SystemRDL design (hierarchical AST JSON)
    auto elaborate_result = systemrdl::elaborate(rdl_content);
    if (elaborate_result.ok()) {
        std::cout << "Elaboration successful!" << std::endl;
        std::cout << "Elaborated JSON: " << elaborate_result.value() << std::endl;
    } else {
        std::cerr << "Elaboration failed: " << elaborate_result.error() << std::endl;
        return 1;
    }

    // Elaborate SystemRDL design (simplified flattened JSON)
    auto simplified_result = systemrdl::elaborate_simplified(rdl_content);
    if (simplified_result.ok()) {
        std::cout << "Simplified elaboration successful!" << std::endl;
        std::cout << "Simplified JSON: " << simplified_result.value() << std::endl;
    } else {
        std::cerr << "Simplified elaboration failed: " << simplified_result.error() << std::endl;
        return 1;
    }

    return 0;
}
```

#### File-based Operations

```cpp
#include <systemrdl/systemrdl_api.h>

int main() {
    // Parse SystemRDL file
    auto parse_result = systemrdl::file::parse("design.rdl");
    if (parse_result.ok()) {
        std::cout << "File parsed successfully!" << std::endl;
        // process parse_result.value()
    }

    // Elaborate SystemRDL file (hierarchical AST JSON)
    auto elaborate_result = systemrdl::file::elaborate("design.rdl");
    if (elaborate_result.ok()) {
        std::cout << "File elaborated successfully!" << std::endl;
        // process elaborate_result.value()
    }

    // Elaborate SystemRDL file (simplified flattened JSON)
    auto simplified_result = systemrdl::file::elaborate_simplified("design.rdl");
    if (simplified_result.ok()) {
        std::cout << "File elaborated to simplified JSON successfully!" << std::endl;
        // process simplified_result.value()
    }

    return 0;
}
```

#### CSV to SystemRDL Conversion

```cpp
#include <systemrdl/systemrdl_api.h>

int main() {
    std::string csv_content =
        "addrmap_offset,addrmap_name,reg_offset,reg_name,reg_width,"
        "field_name,field_lsb,field_msb,reset_value,sw_access,hw_access,description\n"
        "0x0000,DEMO,0x0000,CTRL,32,ENABLE,0,0,0,RW,RW,Enable control bit\n"
        "0x0000,DEMO,0x0000,CTRL,32,MODE,2,1,0,RW,RW,Operation mode\n";

    auto result = systemrdl::csv_to_rdl(csv_content);
    if (result.ok()) {
        std::cout << "SystemRDL output:\n" << result.value() << std::endl;
    }

    return 0;
}
```

#### Stream Operations

```cpp
#include <systemrdl/systemrdl_api.h>
#include <fstream>
#include <sstream>

int main() {
    std::ifstream input_file("input.rdl");
    std::ofstream output_file("output.json");

    // Parse from stream to stream
    if (systemrdl::stream::parse(input_file, output_file)) {
        std::cout << "Stream processing successful!" << std::endl;
    }

    return 0;
}
```

#### Error Handling

The modern API uses a `Result` type that encapsulates success/error states:

```cpp
auto result = systemrdl::parse(rdl_content);

// Check if operation succeeded
if (result.ok()) {
    // Access the successful result
    std::string json_output = result.value();
    // process json_output...
} else {
    // Handle the error
    std::string error_message = result.error();
    std::cerr << "Error: " << error_message << std::endl;
}

// Alternative pattern
if (result.has_error()) {
    std::cerr << "Operation failed: " << result.error() << std::endl;
    return 1;
}
```

#### API Features

- **Clean Interface**: No ANTLR4 headers exposed to user code
- **String-based**: Work with `std::string` and `std::string_view`
- **File Support**: Direct file input/output operations
- **Stream Support**: Standard C++ stream processing
- **Performance**: Uses modern C++17 features like `string_view`
- **Type Safety**: Strong typing with Result pattern
- **Flexible Input**: Multiple ways to provide SystemRDL content
- **CSV Integration**: Built-in CSV to SystemRDL conversion

#### JSON Output Formats

The SystemRDL library provides two different JSON output formats to suit different use cases:

**1. Hierarchical AST JSON (`elaborate()`)**

- Maintains the original hierarchical structure of the SystemRDL design
- Preserves parent-child relationships between address maps, regfiles, registers, and fields
- Suitable for tools that need to understand the full design hierarchy
- Compatible with existing SystemRDL toolchains and templates

**2. Simplified Flattened JSON (`elaborate_simplified()`)**

- Flattens the hierarchical structure into separate arrays for registers and regfiles
- Includes full path information for each register and field
- More user-friendly for register documentation and code generation
- Easier to process for applications that don't need hierarchy details

**Example comparison:**

```cpp
// Hierarchical JSON - preserves structure
auto ast_result = systemrdl::elaborate(rdl_content);
// Output: {"addrmap": {"children": [{"reg": {"children": [{"field": ...}]}}]}}

// Simplified JSON - flattened structure  
auto simplified_result = systemrdl::elaborate_simplified(rdl_content);
// Output: {"registers": [...], "regfiles": [...], "fields": [...]}
```

### Traditional API (Advanced Users)

For users who need direct access to ANTLR4 features or fine-grained control:

```cpp
#include <systemrdl/elaborator.h>
#include <systemrdl/SystemRDLLexer.h>
#include <systemrdl/SystemRDLParser.h>
#include <antlr4-runtime.h>

using namespace antlr4;
using namespace systemrdl;

int main() {
    // Parse SystemRDL file
    std::ifstream stream("design.rdl");
    ANTLRInputStream input(stream);
    SystemRDLLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    SystemRDLParser parser(&tokens);
    auto tree = parser.root();

    // Elaborate the design
    Elaborator elaborator;
    auto root_node = elaborator.elaborate(tree);

    if (elaborator.has_errors()) {
        // Handle errors
        for (const auto& error : elaborator.get_errors()) {
            std::cerr << "Error: " << error.message << std::endl;
        }
        return 1;
    }

    // Use the elaborated design
    std::cout << "Design: " << root_node->inst_name << std::endl;
    std::cout << "Type: " << root_node->get_node_type() << std::endl;
    std::cout << "Size: " << root_node->size << " bytes" << std::endl;

    return 0;
}
```

### Working with Address Maps

```cpp
// Create address map visitor
AddressMapVisitor addr_visitor;
root_node->accept_visitor(addr_visitor);

// Get address layout
const auto& address_map = addr_visitor.get_address_map();
for (const auto& entry : address_map) {
    std::cout << "0x" << std::hex << entry.address
              << ": " << entry.name
              << " (" << std::dec << entry.size << " bytes)"
              << std::endl;
}
```

## Available Targets

### Library Targets

- **`SystemRDL::systemrdl`** - Generic target (shared if available, otherwise static)
- **`SystemRDL::systemrdl_shared`** - Shared library
- **`SystemRDL::systemrdl_static`** - Static library

### Command-line Tools (if enabled)

- **`systemrdl_parser`** - Parse SystemRDL files and generate AST
- **`systemrdl_elaborator`** - Parse and elaborate SystemRDL designs
- **`systemrdl_csv2rdl`** - Convert CSV files to SystemRDL format

## Library Components

### Modern API Components

| Component | Header | Description |
|-----------|--------|-------------|
| `systemrdl::Result` | `systemrdl_api.h` | Result type for error handling |
| `systemrdl::parse()` | `systemrdl_api.h` | Parse SystemRDL content to AST JSON |
| `systemrdl::elaborate()` | `systemrdl_api.h` | Elaborate SystemRDL content to hierarchical JSON |
| `systemrdl::elaborate_simplified()` | `systemrdl_api.h` | Elaborate SystemRDL content to simplified flattened JSON |
| `systemrdl::csv_to_rdl()` | `systemrdl_api.h` | Convert CSV to SystemRDL format |
| `systemrdl::file::*` | `systemrdl_api.h` | File-based operations namespace |
| `systemrdl::stream::*` | `systemrdl_api.h` | Stream-based operations namespace |

### Traditional API Components

#### Core Classes

| Class | Description |
|-------|-------------|
| `Elaborator` | Main elaboration engine |
| `ElaboratedNode` | Base class for elaborated elements |
| `ElaboratedAddrmap` | Address map component |
| `ElaboratedRegfile` | Register file component |
| `ElaboratedReg` | Register component |
| `ElaboratedField` | Field component |
| `ElaboratedMem` | Memory component |

#### Visitors

| Visitor | Purpose |
|---------|---------|
| `ElaboratedNodeVisitor` | Base visitor interface |
| `AddressMapVisitor` | Generate address map |

#### Utilities

| Component | Description |
|-----------|-------------|
| `PropertyValue` | Property value container |
| `ParameterDefinition` | Parameter definitions |
| `EnumDefinition` | Enumeration definitions |
| `StructDefinition` | Structure definitions |

### Common Usage Patterns

#### Pattern 1: Simple Register Map Processing

```cpp
#include <systemrdl/systemrdl_api.h>
#include <iostream>
#include <fstream>

int main() {
    // Read and process a SystemRDL file
    auto result = systemrdl::file::elaborate("chip_registers.rdl");

    if (!result.ok()) {
        std::cerr << "Failed to process register map: " << result.error() << std::endl;
        return 1;
    }

    // Save elaborated model to file
    std::ofstream output("chip_elaborated.json");
    output << result.value();

    std::cout << "Successfully processed register map!" << std::endl;
    return 0;
}
```

#### Pattern 2: CSV Register Database Import

```cpp
#include <systemrdl/systemrdl_api.h>
#include <iostream>

// Convert CSV register database to SystemRDL
bool convert_csv_database(const std::string& csv_file, const std::string& rdl_file) {
    auto result = systemrdl::file::csv_to_rdl(csv_file);

    if (!result.ok()) {
        std::cerr << "CSV conversion failed: " << result.error() << std::endl;
        return false;
    }

    // Save SystemRDL output
    std::ofstream output(rdl_file);
    output << result.value();

    std::cout << "Successfully converted " << csv_file << " to " << rdl_file << std::endl;
    return true;
}

int main() {
    return convert_csv_database("registers.csv", "registers.rdl") ? 0 : 1;
}
```

#### Pattern 3: Build System Integration

```cpp
#include <systemrdl/systemrdl_api.h>
#include <filesystem>

// Process all SystemRDL files in a directory
void process_rdl_directory(const std::string& input_dir, const std::string& output_dir) {
    std::filesystem::create_directories(output_dir);

    for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
        if (entry.path().extension() == ".rdl") {
            std::string input_file = entry.path().string();
            std::string output_file = output_dir + "/" +
                                    entry.path().stem().string() + "_elaborated.json";

            auto result = systemrdl::file::elaborate(input_file);
            if (result.ok()) {
                std::ofstream output(output_file);
                output << result.value();
                std::cout << "Processed: " << input_file << std::endl;
            } else {
                std::cerr << "Failed to process " << input_file
                         << ": " << result.error() << std::endl;
            }
        }
    }
}
```

#### Pattern 4: Error Validation and Reporting

```cpp
#include <systemrdl/systemrdl_api.h>
#include <vector>
#include <string>

struct ValidationResult {
    std::string filename;
    bool success;
    std::string error_message;
};

std::vector<ValidationResult> validate_rdl_files(const std::vector<std::string>& files) {
    std::vector<ValidationResult> results;

    for (const auto& file : files) {
        ValidationResult result;
        result.filename = file;

        auto parse_result = systemrdl::file::parse(file);
        if (parse_result.ok()) {
            // Try elaboration as well
            auto elab_result = systemrdl::file::elaborate(file);
            result.success = elab_result.ok();
            result.error_message = elab_result.ok() ? "" : elab_result.error();
        } else {
            result.success = false;
            result.error_message = parse_result.error();
        }

        results.push_back(result);
    }

    return results;
}
```

#### Pattern 5: Dual JSON Output Generation

```cpp
#include <systemrdl/systemrdl_api.h>
#include <iostream>
#include <fstream>

// Generate both hierarchical and simplified JSON outputs
bool generate_dual_outputs(const std::string& rdl_file, const std::string& output_dir) {
    // Generate hierarchical AST JSON
    auto ast_result = systemrdl::file::elaborate(rdl_file);
    if (!ast_result.ok()) {
        std::cerr << "AST elaboration failed: " << ast_result.error() << std::endl;
        return false;
    }

    // Generate simplified flattened JSON
    auto simplified_result = systemrdl::file::elaborate_simplified(rdl_file);
    if (!simplified_result.ok()) {
        std::cerr << "Simplified elaboration failed: " << simplified_result.error() << std::endl;
        return false;
    }

    // Save both outputs
    std::ofstream ast_file(output_dir + "/design_ast.json");
    std::ofstream simplified_file(output_dir + "/design_simplified.json");

    ast_file << ast_result.value();
    simplified_file << simplified_result.value();

    std::cout << "Generated both AST and simplified JSON outputs" << std::endl;
    return true;
}

int main() {
    return generate_dual_outputs("chip_design.rdl", "output") ? 0 : 1;
}
```

## Example Project

The `example/` directory contains a complete working example demonstrating the **modern API** usage in a real C++
project. This example shows how to integrate SystemRDL functionality into your applications without dealing with
ANTLR4 complexity.

### What the Example Demonstrates

- **Modern API Usage**: Complete demonstration of string-based operations
- **File Operations**: Reading SystemRDL files using convenient wrappers
- **Stream Processing**: Input/output using standard C++ streams
- **CSV Integration**: Converting CSV data to SystemRDL format
- **Error Handling**: Robust error management with Result types
- **Elaboration**: Advanced SystemRDL design processing with arrays and hierarchies
- **Performance**: Modern C++17 patterns with `string_view`

To build and run the example:

```bash
# First build and install the library
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install

# Then build and run the example
cd ../example
mkdir build && cd build
cmake ..
make

# Run the modern API demonstration
./example_app
```

### Example Output

The example produces output demonstrating all modern API features:

```text
SystemRDL Modern API Example

Example 1: Parse SystemRDL content
Parse successful!

Example 2: Simple Elaboration
Elaboration successful!

Example 3: Advanced Elaboration (Arrays & Complex Features)
Advanced elaboration successful!
This demonstrates:
   - Array instantiation (mem_ctrl[4])
   - Complex address mapping with strides
   - Hierarchical regfile structures
   - Automatic gap filling and validation

Example 4: Convert CSV to SystemRDL
CSV conversion successful!

Example 5: File-based operations
File parse successful!
File elaboration successful!

Example 6: Stream operations
Stream processing successful!

Example 7: Error handling
Error handling working correctly!

Key features demonstrated:
   - Clean interface without ANTLR4 header exposure
   - String-based input/output for ease of use
   - Consistent error handling pattern
   - Multiple input/output methods supported
   - Modern C++ design patterns
```

## Library Dependencies

### Required

- **CMake** 3.16 or later
- **C++17** compatible compiler
- **ANTLR4 C++ runtime** (automatically downloaded if not using system version)

### Optional

- **Python 3** (for testing and validation)
- **pkg-config** (for pkg-config support)
- **clang-format** (for code formatting)
- **cppcheck** (for static analysis)

## Library Configuration

### Custom ANTLR4 Version

```bash
# Use specific ANTLR4 version
cmake .. -DANTLR4_VERSION=4.12.0

# Use system ANTLR4
cmake .. -DUSE_SYSTEM_ANTLR4=ON
```

### Install Locations

```bash
# Custom install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/systemrdl

# Custom library directory
cmake .. -DCMAKE_INSTALL_LIBDIR=lib64
```

### Build Types

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Release with debug info
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

## Library Troubleshooting

### Library Not Found

```bash
# Check installation
find /usr/local -name "*systemrdl*"

# Set custom search path
cmake .. -DCMAKE_PREFIX_PATH=/opt/systemrdl

# Verify with pkg-config
pkg-config --exists systemrdl && echo "Found"
```

### Compilation Issues

```bash
# Verbose build output
make VERBOSE=1

# Check compiler requirements
g++ --version  # Should support C++17

# Check ANTLR4 installation
find /usr -name "antlr4-runtime*"
```

### Runtime Issues

```bash
# Check shared library path
ldd your_app

# Set LD_LIBRARY_PATH if needed
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### Development

#### Code Quality

```bash
# Check code formatting
make format-check

# Auto-format code
make format

# Run static analysis
make cppcheck

# Run all quality checks
make quality-check
```

#### Testing

```bash
# Run all tests
make test

# Run specific test categories
make test-parser
make test-elaborator
make test-json
```

### Version Information

- **Version**: 0.1.0
- **SystemRDL Standard**: 2.0
- **ANTLR4 Version**: 4.13.2 (default)
- **C++ Standard**: C++17

### Private Headers (Implementation Details)

- `cmdline_parser.h` - Command-line argument parsing utilities

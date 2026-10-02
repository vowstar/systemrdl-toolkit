# Library Usage

`libsystemrdl` is the parser and elaborator as a linkable library; the
command-line tools are thin wrappers over the same code.

## Build Options

The project provides several build options to customize what gets built:

| Option | Default | Description |
|--------|---------|-------------|
| `SYSTEMRDL_BUILD_SHARED` | `ON` | Build shared library |
| `SYSTEMRDL_BUILD_STATIC` | `ON` | Build static library |
| `SYSTEMRDL_BUILD_TOOLS` | `ON` | Build command-line tools |
| `SYSTEMRDL_BUILD_TESTS` | `ON` | Build tests |
| `USE_SYSTEM_ANTLR4` | `OFF` | Use system ANTLR4 instead of downloading |

## Using the Library in Your Project

Build and install the library first, as [BUILD.md](BUILD.md) describes. The
installed package supports any of the three integration paths below.

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

The installed header `<systemrdl/systemrdl_api.h>` is the entry point for
embedding the toolkit: string, file and stream operations, none of which expose
ANTLR4 types. The internal elaborator API is described at the end of this
document and is only usable inside the source tree.

### Modern API (Recommended)

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
- Easier to read when the register map is maintained by hand
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

### Working against the elaborator directly

The elaborator and the generated parser are internal. They are not installed,
so this only applies inside the source tree, where `src/` and `src/generated/`
are on the include path.

```cpp
#include "elaborator.h"
#include "SystemRDLLexer.h"
#include "SystemRDLParser.h"
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

## Library Components

### Modern API Components

Everything below is declared in `<systemrdl/systemrdl_api.h>`.

| Component | Description |
| -- | -- |
| `systemrdl::Result` | Result type for error handling |
| `systemrdl::parse()` | Parse SystemRDL content to AST JSON |
| `systemrdl::elaborate()` | Elaborate SystemRDL content to hierarchical JSON |
| `systemrdl::elaborate_simplified()` | Elaborate SystemRDL content to simplified flattened JSON |
| `systemrdl::csv_to_rdl()` | Convert CSV to SystemRDL format |
| `systemrdl::file::*` | File-based operations namespace |
| `systemrdl::stream::*` | Stream-based operations namespace |

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

A complete working example, built against the installed library, lives in
[`example/`](../example/README.md).

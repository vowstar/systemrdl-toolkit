# SystemRDL Toolkit

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CI](https://github.com/vowstar/systemrdl-toolkit/actions/workflows/ci.yml/badge.svg)](https://github.com/vowstar/systemrdl-toolkit/actions)

A comprehensive SystemRDL 2.0 toolkit based on ANTLR4 that provides parsing,
elaboration, and conversion capabilitiesfor SystemRDL register description files.

## Features

- **SystemRDL 2.0 Support**: Implementation of SystemRDL 2.0 specification
- **AST & Elaboration**: Parse and elaborate SystemRDL designs with semantic analysis
- **AST Export**: Export AST and elaborated models to JSON format
- **CSV Conversion**: Convert CSV specifications to SystemRDL format
- **Template Rendering**: Documentation generation using Jinja2 templates
- **Validation Testing**: Integration with Python SystemRDL compiler
- **C++ API**: Library interface without ANTLR4 header dependencies
- **Build Flexibility**: Support for multiple ANTLR4 versions

## Tools Included

|          Tool          |                       Description                        |
| ---------------------- | -------------------------------------------------------- |
| `systemrdl_parser`     | Parse SystemRDL files and generate Abstract Syntax Trees |
| `systemrdl_elaborator` | Elaborate parsed designs with semantic analysis          |
| `systemrdl_csv2rdl`    | Convert CSV register specifications to SystemRDL         |
| `systemrdl_render`     | Generate documentation using Jinja2 templates            |

## Quick Start

### Prerequisites

**Ubuntu/Debian:**

```bash
sudo apt-get install cmake build-essential pkg-config python3 python3-venv
```

**Gentoo:**

```bash
sudo emerge cmake dev-util/cmake python:3.13
```

### Build

```bash
# Clone and build
git clone <repository-url>
cd systemrdl-toolkit
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run tests
make test-fast
```

### Example Usage

```bash
# Parse SystemRDL file
./systemrdl_parser design.rdl --json

# Elaborate design
./systemrdl_elaborator design.rdl --json

# Convert CSV to SystemRDL
./systemrdl_csv2rdl registers.csv -o design.rdl

# Generate documentation
./systemrdl_render design.rdl -t template.j2 -o output.html
```

## Documentation

|                   Document                   |                 Description                  |
| -------------------------------------------- | -------------------------------------------- |
| [BUILD.md](doc/BUILD.md)                     | Detailed build instructions and dependencies |
| [TOOLS.md](doc/TOOLS.md)                     | Command-line tools usage guide              |
| [API.md](doc/API.md)                         | C++ library API reference and examples       |
| [TESTING.md](doc/TESTING.md)                 | Testing framework and validation procedures  |
| [DEVELOPMENT.md](doc/DEVELOPMENT.md)         | Development setup and contribution guide     |
| [TROUBLESHOOTING.md](doc/TROUBLESHOOTING.md) | Common issues and solutions                  |
| [ARCHITECTURE.md](doc/ARCHITECTURE.md)       | Project structure and components             |

## Library Usage

The toolkit provides a modern C++ library for integrating SystemRDL functionality:

```cpp
#include "systemrdl_api.h"

// Parse SystemRDL content
auto result = systemrdl::parse_string(rdl_content);
if (result.success) {
    std::cout << "AST JSON: " << result.json_output << std::endl;
}

// Elaborate design
auto elab_result = systemrdl::elaborate_string(rdl_content);
if (elab_result.success) {
    std::cout << "Elaborated JSON: " << elab_result.json_output << std::endl;
}
```

See [API.md](doc/API.md) for complete library documentation.

## Testing

The project includes comprehensive validation using both C++ and Python SystemRDL tools:

```bash
# Run all tests
make test

# Quick validation tests
make test-fast

# Specific test categories
make test-parser test-elaborator test-csv2rdl
```

## Installation

```bash
# Install to system
sudo make install

# Use in your project
find_package(SystemRDL REQUIRED)
target_link_libraries(your_target SystemRDL::systemrdl)
```

## Configuration

### ANTLR4 Version Control

```bash
# Use specific version
cmake .. -DANTLR4_VERSION=4.12.0

# Use system ANTLR4
cmake .. -DUSE_SYSTEM_ANTLR4=ON

# Environment variable
export ANTLR4_VERSION=4.11.1
```

### Build Options

```bash
# Build components
cmake .. -DSYSTEMRDL_BUILD_TOOLS=ON -DSYSTEMRDL_BUILD_TESTS=ON

# Library types
cmake .. -DSYSTEMRDL_BUILD_SHARED=ON -DSYSTEMRDL_BUILD_STATIC=ON
```

## Version Information

- **Version**: 0.1.0
- **SystemRDL Standard**: 2.0
- **C++ Standard**: C++17
- **ANTLR4 Version**: 4.13.2 (default)

All tools support `--version` flag for detailed version information including Git commit and build status.

## Acknowledgments

The SystemRDL grammar file (`SystemRDL.g4`) is derived from the
[SystemRDL Compiler](https://github.com/SystemRDL/systemrdl-compiler) project.
We express our sincere gratitude to the SystemRDL organization and contributors
for providing the comprehensiveSystemRDL 2.0 specification and grammar implementation.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Issues & Support

- **Documentation**: Check the [doc/](doc/) directory for detailed guides
- **Bug Reports**: [Use the issue tracker](https://github.com/vowstar/systemrdl-toolkit/issues)
- **Questions**: See [TROUBLESHOOTING.md](doc/TROUBLESHOOTING.md) for common issues
- **Contributing**: Read [DEVELOPMENT.md](doc/DEVELOPMENT.md) for contribution guidelines

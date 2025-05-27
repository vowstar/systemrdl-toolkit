# SystemRDL Toolbox

This is a comprehensive SystemRDL toolbox based on ANTLR4 that provides parsing and elaboration capabilities for SystemRDL files. The toolbox includes:

- **Parser**: Parses SystemRDL files and generates Abstract Syntax Trees (AST)
- **Elaborator**: Processes and elaborates the parsed SystemRDL descriptions for further analysis and code generation

## Dependencies

Before building, please ensure the following dependencies are installed:

### Ubuntu/Debian

```bash
sudo apt-get install cmake build-essential pkg-config libantlr4-runtime-dev
```

### Gentoo

```bash
sudo emerge cmake dev-util/cmake dev-libs/antlr-cpp
```

## Building

The project now features flexible ANTLR4 version management with automatic downloading and building capabilities.

### ANTLR4 Version Control

You can control which ANTLR4 version to use in several ways:

#### 1. Default Version (4.13.2) - Automatic Download

```bash
mkdir build && cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF
make
```

#### 2. Environment Variable

```bash
export ANTLR4_VERSION=4.11.1
mkdir build && cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF
make
```

#### 3. Command Line Parameter

```bash
mkdir build && cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF -DANTLR4_VERSION=4.12.0
make
```

#### 4. Use System-Installed ANTLR4

```bash
# Install system ANTLR4 first (e.g., sudo apt install antlr4-cpp-runtime-dev)
mkdir build && cd build
cmake .. -DUSE_SYSTEM_ANTLR4=ON
make
```

### Version Selection Priority

Version selection follows this priority order:

1. Command line parameter (`-DANTLR4_VERSION=x.y.z`)
2. Environment variable (`ANTLR4_VERSION=x.y.z`)
3. Default version (4.13.2)

### Code Generation

The build system now includes integrated targets for ANTLR4 JAR download and C++ code generation:

```bash
# Download ANTLR4 JAR (uses configured version)
make download-antlr4-jar

# Generate C++ files from SystemRDL.g4
make generate-antlr4-cpp
```

The generated files will include relative paths in comments:

```cpp
// Generated from SystemRDL.g4 by ANTLR 4.x.y
```

### Manual Installation (Legacy)

For manual ANTLR4 setup, you can still follow the traditional approach:

#### Download ANTLR4 JAR

```bash
# Download ANTLR4 JAR file
wget https://www.antlr.org/download/antlr-4.13.2-complete.jar

# Set up ANTLR4 environment (optional, for convenience)
export CLASSPATH=".:antlr-4.13.2-complete.jar:$CLASSPATH"
alias antlr4='java -jar antlr-4.13.2-complete.jar'
alias grun='java org.antlr.v4.gui.TestRig'
```

#### Install ANTLR4 C++ Runtime from Source

```bash
# Download ANTLR4 source code
wget https://www.antlr.org/download/antlr4-cpp-runtime-4.13.2-source.zip
unzip antlr4-cpp-runtime-4.13.2-source.zip
cd antlr4-cpp-runtime-4.13.2-source

# Build and install
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

#### Generate C++ Files Manually

```bash
# Generate C++ lexer, parser, and visitor files from the grammar
java -jar antlr-4.13.2-complete.jar -Dlanguage=Cpp -no-listener -visitor SystemRDL.g4

# This will generate the following files:
# - SystemRDLLexer.h/cpp
# - SystemRDLParser.h/cpp
# - SystemRDLBaseVisitor.h/cpp
# - SystemRDLVisitor.h
```

**Note:** The generated C++ files are required for compilation. With the new build system, these are automatically managed, but if you modify the `SystemRDL.g4` grammar file, you can regenerate them using `make generate-antlr4-cpp`.

### Standard Build

```bash
mkdir build
cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF  # Use automatic ANTLR4 download
make -j$(nproc)
```

### Build with Tests

The project includes comprehensive testing capabilities through CMake's CTest framework:

```bash
mkdir build
cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF
make -j$(nproc)

# Run fast tests (JSON + semantic validation)
make test-fast

# Run all tests
make test-all

# Or using CTest directly
ctest --output-on-failure --verbose
```

### Generated Files

The following files are generated from `SystemRDL.g4`:

- `SystemRDLLexer.cpp/h`
- `SystemRDLParser.cpp/h`
- `SystemRDLBaseVisitor.cpp/h`
- `SystemRDLVisitor.cpp/h`

## Usage

After successful build, the executables are located in the `build/` directory.

### Parser Usage

The parser can display the Abstract Syntax Tree (AST) and optionally export it to JSON format:

```bash
# Parse and print AST to console
./build/systemrdl_parser input.rdl

# Parse and generate JSON output with default filename (input_ast.json)
./build/systemrdl_parser input.rdl --json

# Parse and generate JSON output with custom filename
./build/systemrdl_parser input.rdl --json=my_ast.json

# Short option variant
./build/systemrdl_parser input.rdl -j=output.json
```

### Elaborator Usage

The elaborator processes SystemRDL files through semantic analysis and can export the elaborated model to JSON:

```bash
# Elaborate SystemRDL file and display to console
./build/systemrdl_elaborator input.rdl

# Elaborate and generate JSON output with default filename (input_elaborated.json)
./build/systemrdl_elaborator input.rdl --json

# Elaborate and generate JSON output with custom filename
./build/systemrdl_elaborator input.rdl --json=my_model.json

# Short option variant
./build/systemrdl_elaborator input.rdl -j=output.json
```

### Command Line Options

Both tools support the following options:

- `-j, --json[=<filename>]` - Enable JSON output, optionally specify custom filename
- `-h, --help` - Show help message

If no filename is specified with `--json`, the tools automatically generate a filename based on the input file:

- Parser: `<input_basename>_ast.json`
- Elaborator: `<input_basename>_elaborated.json`

### Input/Output Examples

**Input file** (`example.rdl`):

```systemrdl
addrmap simple_chip {
    reg {
        field {
            sw = rw;
        } data[31:0];
    } reg1 @ 0x0;

    reg {
        field {
            sw = rw;
        } status[7:0];
    } reg2 @ 0x4;
};
```

**Parser console output**:

```bash
✅ Parsing successful!

=== Abstract Syntax Tree ===
📦 Component Definition
    🔧 Type: addrmap
        🔧 Type: reg
            🔧 Type: field
              ⚙️ Property: sw=rw
          📋 Instance: data[31:0]
            📏 Range: [31:0]
      📋 Instance: reg1@0x0
        📍 Address: @0x0
        ...
```

**Elaborator console output**:

```bash
🔧 Parsing SystemRDL file: example.rdl
✅ Parsing successful!

🚀 Starting elaboration...
✅ Elaboration successful!

=== Elaborated SystemRDL Model ===
📦 addrmap: simple_chip @ 0x0
  🔧 reg: reg1 (size: 4 bytes)
    🔧 field: data [31:0]
      📝 width: 32
      📝 lsb: 0
      📝 sw: "rw"
      📝 msb: 31
  🔧 reg: reg2 @ 0x4 (size: 4 bytes)
    🔧 field: status @ 0x4 [7:0]
      📝 width: 8
      📝 lsb: 0
      📝 sw: "rw"
      📝 msb: 7

📊 Address Map:
Address     Size    Name      Path
------------------------------------
0x00000000  4       reg1      simple_chip.reg1
0x00000004  4       reg2      simple_chip.reg2
```

**JSON output** (example of elaborated model):

```json
{
  "format": "SystemRDL_ElaboratedModel",
  "version": "1.0",
  "model": [
    {
      "node_type": "addrmap",
      "inst_name": "simple_chip",
      "absolute_address": "0x0",
      "size": 0,
      "children": [
        {
          "node_type": "reg",
          "inst_name": "reg1",
          "absolute_address": "0x0",
          "size": 4,
          "children": [
            {
              "node_type": "field",
              "inst_name": "data",
              "absolute_address": "0x0",
              "size": 0,
              {
                "_section": "properties",
                "width": 32,
                "lsb": 0,
                "sw": "rw",
                "msb": 31
              }
            }
          ]
        }
      ]
    }
  ]
}
```

## Testing

The project includes automatic testing of all SystemRDL files in the `test/` directory with enhanced testing capabilities:

### Quick Testing

```bash
# Run fast tests (JSON + semantic validation)
cd build
make test-fast

# Run JSON output tests only
make test-json

# Run semantic validation tests only
make test-semantic
```

### Comprehensive Testing

```bash
# Run all tests (parser + elaborator + JSON + semantic)
make test-all

# Standard CTest execution
make test

# Verbose output with details
ctest --output-on-failure --verbose

# Custom target for comprehensive testing
make run-tests
```

### Test Categories

```bash
# Test only the parser
make test-parser

# Test only the elaborator
make test-elaborator

# Or using CTest labels
ctest -L parser --output-on-failure
ctest -L elaborator --output-on-failure
ctest -L json --output-on-failure
ctest -L semantic --output-on-failure
```

### Individual Test Execution

```bash
# Test specific file with parser
ctest -R "parser_test_minimal" --output-on-failure

# Test specific file with elaborator
ctest -R "elaborator_test_minimal" --output-on-failure

# Test specific JSON output
ctest -R "json_test_minimal" --output-on-failure
```

### Available Test Targets

- `test-fast` - Quick tests (JSON + semantic validation) for rapid development
- `test-json` - JSON output validation tests
- `test-semantic` - RDL semantic validation using Python SystemRDL compiler
- `test-parser` - SystemRDL parser tests
- `test-elaborator` - SystemRDL elaborator tests
- `test-all` - Complete test suite

## File Description

- `parser_main.cpp` - Main program for the SystemRDL parser
- `elaborator_main.cpp` - Main program for the SystemRDL elaborator
- `elaborator.cpp/.h` - Elaboration engine implementation
- `CMakeLists.txt` - CMake build configuration with integrated testing and ANTLR4 management
- `SystemRDL.g4` - ANTLR4 grammar file
- `SystemRDLLexer.*` - Generated lexer (auto-generated)
- `SystemRDLParser.*` - Generated parser (auto-generated)
- `SystemRDLBaseVisitor.*` - Generated base visitor class (auto-generated)
- `SystemRDLVisitor.*` - Generated visitor interface (auto-generated)
- `test/*.rdl` - Test SystemRDL files for validation
- `script/*.py` - Python validation and testing scripts

## Features

- **Complete SystemRDL 2.0 Support**: Full implementation of the SystemRDL 2.0 specification
- **Flexible ANTLR4 Version Management**: Support for multiple ANTLR4 versions with automatic download
  - Environment variable support (`ANTLR4_VERSION`)
  - Command line parameter support (`-DANTLR4_VERSION=x.y.z`)
  - Automatic JAR download and C++ code generation
  - System ANTLR4 runtime integration
- **AST Generation**: Detailed Abstract Syntax Tree representation of SystemRDL designs
- **Elaboration Engine**: Semantic analysis and elaboration of SystemRDL descriptions
- **JSON Export**: Export parsed AST and elaborated model to structured JSON format
  - Standardized JSON schema for interoperability
  - Support for both AST and elaborated model formats
  - Automatic filename generation or custom file specification
- **Flexible Command Line Interface**: Modern CLI with optional parameters
  - Support for `--json` (default filename) and `--json=custom.json` formats
  - Short options (`-j`) and long options (`--json`) support
  - Future-ready for multiple output formats (e.g., `--yaml`, `--xml`)
- **Comprehensive Error Reporting**: Detailed error detection and reporting with line/column information
- **Address Map Generation**: Automatic generation of memory address maps from elaborated models
- **Extensible Architecture**: Modular design for easy extension and customization
- **Integrated Testing**: CMake-based testing framework with comprehensive test coverage
  - JSON output validation tests
  - Semantic validation using Python SystemRDL compiler
  - Fast test targets for rapid development
- **Cross-platform Support**: Compatible with Linux, macOS, and Windows
- **Unicode Emoji Interface**: User-friendly console output with emoji indicators
- **Property Analysis**: Complete property inheritance and evaluation system

## Troubleshooting

1. **ANTLR4 runtime library not found**
   - If using system ANTLR4 (`USE_SYSTEM_ANTLR4=ON`): Ensure ANTLR4 C++ runtime library is installed
   - If using downloaded ANTLR4 (`USE_SYSTEM_ANTLR4=OFF`): The system will automatically download and build it
   - Check if the library is in standard paths, or use `cmake .. -DUSE_SYSTEM_ANTLR4=OFF` for automatic management

2. **Version conflicts**
   - Use `cmake .. -DUSE_SYSTEM_ANTLR4=OFF -DANTLR4_VERSION=4.13.2` to specify exact version
   - Clear build directory if switching between system and downloaded ANTLR4: `rm -rf build/*`

3. **Compilation errors**
   - Ensure using C++17 or higher version compiler
   - If using custom ANTLR4 version, ensure it's compatible (4.9.0+)
   - Regenerate C++ files if needed: `make generate-antlr4-cpp`

4. **Network issues during download**
   - Check internet connection for ANTLR4 JAR and runtime download
   - Use proxy if necessary: `export https_proxy=your_proxy`
   - Fallback to system ANTLR4: `cmake .. -DUSE_SYSTEM_ANTLR4=ON`

5. **Runtime errors**
   - Ensure input SystemRDL file has correct syntax
   - Check if file path is correct
   - Use verbose mode for debugging: `./systemrdl_parser file.rdl --verbose`

6. **Test failures**
   - Run individual tests to identify specific issues: `ctest -R "test_name" --output-on-failure`
   - Check test file syntax and ensure it complies with SystemRDL 2.0 specification
   - Use `make test-fast` for quick validation during development

## Acknowledgments

The SystemRDL grammar file (`SystemRDL.g4`) used in this project is derived from the [SystemRDL Compiler](https://github.com/SystemRDL/systemrdl-compiler) project. We would like to express our sincere gratitude to the SystemRDL organization and all contributors to the SystemRDL Compiler project for providing the comprehensive SystemRDL 2.0 language specification and grammar implementation.

The SystemRDL Compiler project is an open-source, MIT-licensed compiler front-end for Accellera's SystemRDL 2.0 register description language, which serves as an excellent foundation for SystemRDL language processing and tool development.

For more information about the SystemRDL Compiler project, please visit: [https://github.com/SystemRDL/systemrdl-compiler](https://github.com/SystemRDL/systemrdl-compiler)

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

### Download ANTLR4 JAR (for grammar generation)

```bash
# Download ANTLR4 JAR file
wget https://www.antlr.org/download/antlr-4.13.2-complete.jar

# Set up ANTLR4 environment (optional, for convenience)
export CLASSPATH=".:antlr-4.13.2-complete.jar:$CLASSPATH"
alias antlr4='java -jar antlr-4.13.2-complete.jar'
alias grun='java org.antlr.v4.gui.TestRig'
```

### Install ANTLR4 C++ Runtime from Source (if not available in package manager)

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

## Generating C++ Files from Grammar

Before building the project, you need to generate the C++ source files from the SystemRDL grammar file using ANTLR4.

### Prerequisites

Make sure you have downloaded the ANTLR4 JAR file as described in the dependencies section above.

### Generate C++ Files

```bash
# Generate C++ lexer, parser, and visitor files from the grammar
java -jar antlr-4.13.2-complete.jar -Dlanguage=Cpp -no-listener -visitor SystemRDL.g4

# This will generate the following files:
# - SystemRDLLexer.h/cpp
# - SystemRDLParser.h/cpp
# - SystemRDLBaseVisitor.h/cpp
# - SystemRDLVisitor.h
```

### Alternative (using alias if set up)

If you have set up the ANTLR4 alias as shown in the dependencies section:

```bash
antlr4 -Dlanguage=Cpp -no-listener -visitor SystemRDL.g4
```

**Note:** The generated C++ files are required for compilation. If you modify the `SystemRDL.g4` grammar file, you must regenerate these files before building.

## Building

### Using Build Script (Recommended)

```bash
./build.sh
```

### Manual Build

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## Usage

After successful build, the executables are located in the `build/` directory.

### Parser Usage

```bash
# Parse and print AST
./build/systemrdl_parser example.rdl

# Or parse your own SystemRDL file
./build/systemrdl_parser your_file.rdl
```

### Elaborator Usage

```bash
# Elaborate SystemRDL file
./build/systemrdl_elaborator example.rdl

# Or elaborate your own SystemRDL file
./build/systemrdl_elaborator your_file.rdl
```

## File Description

- `main.cpp` - Main program containing AST printing and elaboration logic
- `CMakeLists.txt` - CMake build configuration
- `SystemRDL.g4` - ANTLR4 grammar file
- `SystemRDLLexer.*` - Generated lexer
- `SystemRDLParser.*` - Generated parser
- `SystemRDLBaseVisitor.*` - Generated base visitor class
- `example.rdl` - Example SystemRDL file
- `build.sh` - Automated build script

## Features

- **Complete SystemRDL 2.0 Support**: Full implementation of the SystemRDL 2.0 specification
- **AST Generation**: Detailed Abstract Syntax Tree representation of SystemRDL designs
- **Elaboration Engine**: Semantic analysis and elaboration of SystemRDL descriptions
- **Error Reporting**: Comprehensive error detection and reporting
- **Extensible Architecture**: Modular design for easy extension and customization

## Troubleshooting

1. **ANTLR4 runtime library not found**
   - Ensure ANTLR4 C++ runtime library is installed
   - Check if the library is in standard paths, or modify paths in CMakeLists.txt

2. **Compilation errors**
   - Ensure using C++17 or higher version compiler
   - Check if all generated ANTLR4 files exist

3. **Runtime errors**
   - Ensure input SystemRDL file has correct syntax
   - Check if file path is correct

## Acknowledgments

The SystemRDL grammar file (`SystemRDL.g4`) used in this project is derived from the [SystemRDL Compiler](https://github.com/SystemRDL/systemrdl-compiler) project. We would like to express our sincere gratitude to the SystemRDL organization and all contributors to the SystemRDL Compiler project for providing the comprehensive SystemRDL 2.0 language specification and grammar implementation.

The SystemRDL Compiler project is an open-source, MIT-licensed compiler front-end for Accellera's SystemRDL 2.0 register description language, which serves as an excellent foundation for SystemRDL language processing and tool development.

For more information about the SystemRDL Compiler project, please visit: [https://github.com/SystemRDL/systemrdl-compiler](https://github.com/SystemRDL/systemrdl-compiler)

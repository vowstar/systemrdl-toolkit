# Build Instructions

## Dependencies

Before building, please ensure the following dependencies are installed:

### System Dependencies

#### Ubuntu/Debian

```bash
sudo apt-get install cmake build-essential pkg-config libantlr4-runtime-dev python3 python3-venv python3-pip
```

#### Gentoo

```bash
sudo emerge cmake dev-util/cmake dev-libs/antlr-cpp python:3.13
```

### Python Dependencies

The project includes Python scripts for validation and testing that require additional dependencies:

#### Set up Python Virtual Environment

The project includes a `requirements.txt` file with all necessary Python dependencies. Follow these steps to set up the
Python environment:

```bash
# 1. Create virtual environment
python3 -m venv .venv

# 2. Activate virtual environment
source .venv/bin/activate

# 3. Upgrade pip to latest version (recommended)
pip install --upgrade pip

# 4. Install all required dependencies from requirements.txt
pip install -r requirements.txt

# 5. Verify installation
python3 -c "import systemrdl; print('SystemRDL compiler version:', systemrdl.__version__)"
```

**For Windows users:**

```cmd
# 2. Activate virtual environment (Windows)
.venv\Scripts\activate

# Other steps remain the same
```

#### Manual Installation (Alternative)

If you prefer to install dependencies manually:

```bash
# Create and activate virtual environment
python3 -m venv .venv
source .venv/bin/activate

# Install specific version (as specified in requirements.txt)
pip install systemrdl-compiler==1.29.3
```

#### Deactivating Virtual Environment

When you're done working with the project:

```bash
# Deactivate virtual environment
deactivate
```

#### Required Python Packages

The `requirements.txt` file contains:

- **systemrdl-compiler==1.29.3**: Official SystemRDL 2.0 compiler for semantic validation
  - Used by `rdl_semantic_validator.py` for validating SystemRDL files
  - Used by the test framework for semantic validation tests
  - Provides the Python API for SystemRDL elaboration and compilation

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

**Note:** The generated C++ files are required for compilation. With the new build system, these are automatically
managed, but if you modify the `SystemRDL.g4` grammar file, you can regenerate them using `make generate-antlr4-cpp`.

### Standard Build

```bash
mkdir build
cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF  # Use automatic ANTLR4 download
make -j$(nproc)
```

### Build with Tests

The project includes testing capabilities through CMake's CTest framework:

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

# Build Instructions

## Dependencies

### System Dependencies

#### Ubuntu/Debian

```bash
sudo apt-get install cmake build-essential pkg-config libantlr4-runtime-dev python3 python3-venv python3-pip
```

#### Gentoo

```bash
sudo emerge cmake dev-util/cmake dev-libs/antlr-cpp python:3.10
```

### Python Dependencies

Python 3.10 or newer is required. The versions of `systemrdl-compiler` and the validation tools are pinned in
`requirements.txt`.

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt
```

On Windows, activate the environment with `.venv\Scripts\activate`.

## Building

ANTLR4 version selection uses this priority order:

1. Command line parameter (`-DANTLR4_VERSION=x.y.z`)
2. Environment variable (`ANTLR4_VERSION=x.y.z`)
3. Default version (`4.13.2`)

Set `-DUSE_SYSTEM_ANTLR4=ON` to link a system-installed ANTLR4 C++ runtime instead of the downloaded build:

```bash
mkdir build && cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF
make -j$(nproc)
```

### Code Generation

```bash
make download-antlr4-jar    # download the ANTLR4 JAR for the configured version
make generate-antlr4-cpp    # generate C++ sources from grammar/SystemRDL.g4
```

Regenerate after editing `grammar/SystemRDL.g4`. Generated sources carry a `// Generated from SystemRDL.g4 by ANTLR
<version>` header:

- `src/generated/SystemRDLLexer.cpp/h`
- `src/generated/SystemRDLParser.cpp/h`
- `src/generated/SystemRDLBaseVisitor.cpp/h`
- `src/generated/SystemRDLVisitor.cpp/h`

### Tests

Tests are built by default for a top-level project (`SYSTEMRDL_BUILD_TESTS=ON`):

```bash
make test-fast    # AST, JSON, semantic, CSV2RDL, template and example tests
make test-all     # all registered CTest tests
ctest --output-on-failure --verbose   # CTest directly
```

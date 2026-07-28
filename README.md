# SystemRDL Toolkit

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CI](https://github.com/vowstar/systemrdl-toolkit/actions/workflows/ci.yml/badge.svg)](https://github.com/vowstar/systemrdl-toolkit/actions)

SystemRDL Toolkit is a C++17 parser and elaborator for SystemRDL register
descriptions. It emits JSON models, converts RCSV to SystemRDL, and renders
Inja templates supplied by the user. The project implements a tested subset
of SystemRDL 2.0 semantics.

## Validated Scope

The test corpus exercises parsing and elaboration paths for address maps,
registers, register files, arrays with one dimension, parameters, enums,
field access, reset values, and address calculation.

Negative tests cover invalid field bounds, field overlaps, and instance address
overlaps. The elaborator also fills unused register bits with reserved fields.

## Known Limits

- Dynamic property assignments and property modifiers are not elaborated.
- Only the first array dimension is elaborated. A dimension that cannot be
  evaluated falls back to four elements.
- Memory size semantics are incomplete. A memory without a recognized size is
  assigned 4096 bytes.
- Struct definitions can be parsed but are not used by the elaborated model.
- Register widths must be `2^N` with `N >= 3`, as required by SystemRDL 2.0
  clauses 10.1-f and 10.6.1-a. Earlier releases accepted any width.
- RCSV is the schema defined by this project. It is not an arbitrary CSV
  register format, and each file describes one address map.
- The templates under `test/` are test fixtures. They are not qualified C
  header or RTL generators.
- JSON documents contain a format name and version. The elaborated models are
  at `2.0` and the AST model is at `1.0`. A compatibility policy has not been
  defined.
- The installed C++ package currently fails a standalone consumer build. Its
  public header set and generic CMake target are incomplete.
- The system dependency path used for offline builds is not covered by CI.

## Data Flow

```text
RCSV -> systemrdl_csv2rdl -> SystemRDL
SystemRDL + systemrdl_parser --ast -> AST JSON
SystemRDL + systemrdl_elaborator --ast -> hierarchical JSON
SystemRDL + systemrdl_elaborator --json -> simplified JSON
SystemRDL or RCSV + Inja template -> systemrdl_render -> user-defined output
```

## Quick Start

The default configuration downloads ANTLR4, nlohmann/json, and Inja.

```bash
git clone https://github.com/vowstar/systemrdl-toolkit.git
cd systemrdl-toolkit
cmake -B build
cmake --build build --parallel
```

Elaborate the register description used by the smoke tests:

```bash
./build/systemrdl_elaborator test/test_minimal.rdl --json=build/minimal.json
```

The generated file identifies itself as `SystemRDL_SimplifiedModel` version
`1.0` and contains the resolved address map, registers, fields, access
properties, and reset values. Source build options are documented in
[Build](doc/BUILD.md).

## Command-Line Tools

| Tool | Input | Output | JSON format |
| -- | -- | -- | -- |
| `systemrdl_parser` | SystemRDL | Printed parse tree and optional JSON through `--ast` | `SystemRDL_AST` |
| `systemrdl_elaborator --ast` | SystemRDL | Hierarchical model | `SystemRDL_ElaboratedModel` |
| `systemrdl_elaborator --json` | SystemRDL | Flattened register model | `SystemRDL_SimplifiedModel` |
| `systemrdl_csv2rdl` | RCSV | SystemRDL source | None |
| `systemrdl_render` | SystemRDL or RCSV plus an Inja template | Text produced by the supplied template | None |

Run each tool with `--help`, or see [Command-Line Tools](doc/TOOLS.md). JSON
consumers must validate both `format` and `version`; the three models use
different schemas.

## Documentation

Read [Build](doc/BUILD.md) for dependencies, [Command-Line Tools](doc/TOOLS.md)
for CLI options, [RCSV](doc/RCSV.md) for the input schema, and
[Testing](doc/TESTING.md) before changing parser or elaborator behavior. The
C++ entry points in the source tree are declared in
[`systemrdl_api.h`](systemrdl_api.h).

The grammar in `SystemRDL.g4` is derived from the
[SystemRDL Compiler](https://github.com/SystemRDL/systemrdl-compiler) project.

License: [MIT](LICENSE). Bugs:
[Issues](https://github.com/vowstar/systemrdl-toolkit/issues). Changes:
[CONTRIBUTING.md](CONTRIBUTING.md).

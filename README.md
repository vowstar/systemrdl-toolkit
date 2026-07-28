# SystemRDL Toolkit

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CI](https://github.com/vowstar/systemrdl-toolkit/actions/workflows/ci.yml/badge.svg)](https://github.com/vowstar/systemrdl-toolkit/actions)

SystemRDL Toolkit is a C++17 parser and elaborator for SystemRDL register
descriptions. It emits JSON models, converts RCSV to SystemRDL, and renders
Inja templates supplied by the user. The project implements a tested subset
of SystemRDL 2.0 semantics.

## Conformance

The worked examples in SystemRDL 2.0 state the addresses and bit positions they
produce. Those examples are part of the test suite with the expected values
recorded beside them, and every build checks the elaborator against them. The
examples covered are the three addressing modes of 5.1.2.2.2, the allocation
operators of 5.1.2.5, the field packing of 10.7.2 in both bit orderings, and the
alignment property of 12.3.2.

Elaboration rejects a description that breaks a rule the standard states with
"shall", naming the clause in the message. This includes register and access
widths (10.1-f, 10.6.1), a register with no field (10.1-c), a register file with
no register (12.2-c), field overlaps and bit ranges (10.1-d, 10.1-e), and mixing
both bit ordering forms in one register (10.7.1-a).

One behaviour goes beyond the standard: the bits between fields become real
fields carrying `reserved = true`, so that generators do not each have to derive
them. Filter on that property to ignore them.

## Not Implemented

- Dynamic property assignments and property modifiers.
- Array dimensions beyond the first.
- Struct definitions, which parse but do not reach the elaborated model.

## Scope

- RCSV is the schema defined by this project. It is not an arbitrary CSV
  register format, and each file describes one address map.
- The templates under `test/` are test fixtures. They are not qualified C
  header or RTL generators.
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

The generated file identifies itself as `SystemRDL_SimplifiedModel` and
contains the resolved address map, registers, fields, access properties, and
reset values. Source build options are documented in [Build](doc/BUILD.md).

## Command-Line Tools

| Tool | Input | Output | JSON format |
| -- | -- | -- | -- |
| `systemrdl_parser` | SystemRDL | Printed parse tree and optional JSON through `--ast` | `SystemRDL_AST` |
| `systemrdl_elaborator --ast` | SystemRDL | Hierarchical model | `SystemRDL_ElaboratedModel` |
| `systemrdl_elaborator --json` | SystemRDL | Flattened register model | `SystemRDL_SimplifiedModel` |
| `systemrdl_csv2rdl` | RCSV | SystemRDL source | None |
| `systemrdl_render` | SystemRDL or RCSV plus an Inja template | Text produced by the supplied template | None |

Run each tool with `--help`, or see [Command-Line Tools](doc/TOOLS.md). Each
document names its `format`; the three share no schema. A field reset is a
lowercase hex string such as `"0xff"`, and a field the source gives no reset
carries no `reset` key.

## Documentation

Read [Build](doc/BUILD.md) for dependencies, [Command-Line Tools](doc/TOOLS.md)
for CLI options, [RCSV](doc/RCSV.md) for the input schema, and
[Testing](doc/TESTING.md) before changing parser or elaborator behavior.

## Using the Library

The whole interface is one header that takes and returns strings.

```cmake
find_package(SystemRDL REQUIRED)
target_link_libraries(your_target PRIVATE SystemRDL::systemrdl)
```

```cpp
#include <systemrdl/systemrdl_api.h>

const auto result = systemrdl::elaborate_simplified(rdl_text);
if (result.ok()) {
    use(result.value());   // simplified JSON model
}
```

The elaborator internals and the generated parser are not installed.

The grammar is derived from the
[SystemRDL Compiler](https://github.com/SystemRDL/systemrdl-compiler) project.

License: [MIT](LICENSE). Bugs:
[Issues](https://github.com/vowstar/systemrdl-toolkit/issues). Changes:
[CONTRIBUTING.md](CONTRIBUTING.md).

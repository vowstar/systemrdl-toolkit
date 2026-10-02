# Command-Line Tools Usage

The executables land in `build/` after a successful build. This document
covers their command line options.

## Overview

|          Tool          |                       Description                        |
| ---------------------- | -------------------------------------------------------- |
| `systemrdl_parser`     | Parse SystemRDL files and generate Abstract Syntax Trees |
| `systemrdl_elaborator` | Elaborate parsed designs with semantic analysis          |
| `systemrdl_csv2rdl`    | Convert CSV register specifications to SystemRDL         |
| `systemrdl_render`     | Generate documentation using Jinja2 templates            |

---

## Parser

The parser can display the Abstract Syntax Tree (AST) and optionally export it to AST JSON format:

```bash
# Parse and print AST to console
./build/systemrdl_parser input.rdl

# Parse and generate AST JSON output with default filename (input_ast.json)
./build/systemrdl_parser input.rdl --ast

# Parse and generate AST JSON output with custom filename
./build/systemrdl_parser input.rdl --ast=my_ast.json

# Short option variant
./build/systemrdl_parser input.rdl -a=output.json
```

### Parser Command Line Options

- `-a, --ast[=<filename>]` - Enable AST JSON output, optionally specify custom filename
- `-h, --help` - Show help message

If no filename is specified with `--ast`, the tool automatically generates: `<input_basename>_ast.json`

---

## Elaborator

The elaborator processes SystemRDL files through semantic analysis and can export the elaborated model to AST JSON format:

```bash
# Elaborate SystemRDL file and display to console
./build/systemrdl_elaborator input.rdl

# Elaborate and generate AST JSON output with default filename (input_ast_elaborated.json)
./build/systemrdl_elaborator input.rdl --ast

# Elaborate and generate AST JSON output with custom filename
./build/systemrdl_elaborator input.rdl --ast=my_model.json

# Elaborate and generate simplified JSON output with default filename (input_simplified.json)
./build/systemrdl_elaborator input.rdl --json

# Elaborate and generate simplified JSON output with custom filename
./build/systemrdl_elaborator input.rdl --json=my_simplified.json

# Short option variants
./build/systemrdl_elaborator input.rdl -a=ast_output.json
./build/systemrdl_elaborator input.rdl -j=json_output.json
```

### Elaborator Command Line Options

- `-a, --ast[=<filename>]` - Enable AST JSON output, optionally specify custom filename
- `-j, --json[=<filename>]` - Enable simplified JSON output, optionally specify custom filename
- `-h, --help` - Show help message

If no filename is specified:

- `--ast` generates: `<input_basename>_ast_elaborated.json`
- `--json` generates: `<input_basename>_simplified.json`

### Elaborator Gap Detection

Bits left unspecified in a register are filled with reserved fields:

```bash
./build/systemrdl_elaborator test/test_auto_reserved_fields.rdl
```

- A gap wider than one bit becomes `RESERVED_<msb>_<lsb>`; a one-bit gap
  becomes `RESERVED_<bit>`.
- Reserved fields are emitted after the fields that were declared, with
  `sw = r`, `hw = na` and `reserved = true`.
- Coverage runs over the whole `regwidth`, whatever it is set to.

---

## CSV2RDL Converter

`systemrdl_csv2rdl` reads an RCSV file and writes SystemRDL text.

### CSV2RDL Basic Usage

```bash
# Convert CSV file to SystemRDL (auto-generate output filename)
./build/systemrdl_csv2rdl input.csv

# Specify custom output filename
./build/systemrdl_csv2rdl input.csv -o output.rdl

# Display help information
./build/systemrdl_csv2rdl --help
```

### CSV2RDL Command Line Options

- `-o, --output <filename>` - Specify output SystemRDL filename
- `-h, --help` - Show help message

### CSV2RDL Format Requirements (RCSV Specification)

The converter reads CSV files following the RCSV specification, which describes
a register map in three layers: addrmap, then reg, then field.

> [INFO] **Complete Specification**: See [RCSV.md](RCSV.md) for the full RCSV specification

CSV files should contain the following columns (header names are case-insensitive with fuzzy matching support):

| Column | Required | Description | Example |
|--------|----------|-------------|---------|
| `addrmap_offset` | Yes | Address map base offset | `0x0000` |
| `addrmap_name` | Yes | Address map name | `DEMO` |
| `reg_offset` | Yes | Register offset within address map | `0x0000` |
| `reg_name` | Yes | Register name | `CTRL` |
| `reg_width` | Yes | Register width in bits | `32` |
| `field_name` | Yes | Field name | `ENABLE` |
| `field_lsb` | Yes | Field least significant bit | `0` |
| `field_msb` | Yes | Field most significant bit | `0` |
| `reset_value` | Yes | Field reset value | `0` |
| `sw_access` | Yes | Software access type | `RW`/`RO`/`WO` |
| `hw_access` | Yes | Hardware access type | `RW`/`RO`/`WO` |
| `description` | Optional | Field/register description | `Enable control bit` |

### RCSV Structure Rules

> [WARN] **Important**: CSV files must comply with RCSV structural requirements for successful conversion

#### Row Hierarchy Definition

1. **Header Row** (Line 1): Column names defining the structure
2. **Address Map Row**: Contains `addrmap_offset` and `addrmap_name`
3. **Register Row**: Contains `reg_offset`, `reg_name` and `reg_width`, and may
   include `description`
4. **Field Row**: Contains `field_name`, `field_lsb`, `field_msb`,
   `reset_value`, `sw_access`, `hw_access`, and optionally `description`

#### RCSV Compliance Rules

1. **Sequential Processing**: Address map -> Register -> Fields sequence must be maintained
2. **Complete Hierarchy**: Every field must have a parent register
3. **Required Columns**: All 11 mandatory RCSV columns must be present
4. **Valid Values**: Access control must use RW/RO/WO/NA values
5. **Bit Range Validation**: Field ranges must not overlap within registers
6. **Address Alignment**: Registers should align to natural boundaries

#### Multi-line and Quoting Support

- **Multi-line descriptions**: Supported with proper CSV double-quote escaping
- **Special characters**: Commas, quotes, newlines handled per RFC 4180
- **Logical vs Physical rows**: Multi-line CSV cells count as single logical rows

### CSV2RDL Example (RCSV-Compliant)

**Correct RCSV Structure:**

```csv
addrmap_offset,addrmap_name,reg_offset,reg_name,reg_width,field_name,field_lsb,field_msb,reset_value,sw_access,hw_access,description
0x0000,DEMO,,,,,,,,,,Demo chip address map
,,0x0000,CTRL,32,,,,,,,Control register
,,,,,ENABLE,0,0,0,RW,RW,Enable control bit
,,,,,MODE,1,2,0,RW,RW,"Operation mode
- 0: Disabled
- 1: Normal
- 2: Debug"
,,0x0004,STATUS,32,,,,,,,Status register
,,,,,READY,0,0,0,RO,WO,System ready flag
,,,,,ERROR,1,1,0,RO,WO,Error status
```

### CSV2RDL Features (RCSV Implementation)

#### Header Matching

- **Case-insensitive**: `AddrmapOffset` -> `addrmap_offset`
- **Fuzzy matching**: Handles typos with Levenshtein distance <=3
- **Abbreviation support**: `sw_acc` -> `sw_access`, `hw_acc` -> `hw_access`
- **RCSV Standard Names**: Recognizes all 11 required RCSV column names

#### Multi-line Field Support

The converter handles multi-line descriptions properly:

```csv
field_name,description
MODE,"Operation mode selection
- 0x0: Mode0: Foo bar
- 0x1: Mode1: Foz baz
- 0x2: Mode2: Fooo baar
- 0x3: Reserved"
```

#### Flexible Delimiters

Automatically detects and supports:

- Comma-separated values (`,`)
- Semicolon-separated values (`;`)

#### String Processing

- **Name fields** (addrmap_name, reg_name, field_name): Remove all newlines and trim whitespace
- **Description fields**: Preserve internal newlines, collapse multiple consecutive newlines
- **Regular fields**: Basic trim operations

---

## Renderer

`systemrdl_render` parses and elaborates a design, converts it to JSON, and
renders an Inja template (Jinja2 syntax) against that JSON.

### Renderer Basic Usage

```bash
# Simplified JSON (default) with a matching template
./build/systemrdl_render design.rdl -t test/test_j2_json_header.h.j2

# Full AST JSON with a matching template
./build/systemrdl_render design.rdl -t test/test_j2_ast_header.h.j2 --ast

# Custom output name, with progress output
./build/systemrdl_render design.rdl -t test/test_j2_json_doc.md.j2 -o design_documentation.md --verbose
```

A template reads the JSON shape it was written for. The `test_j2_json_*`
templates read the simplified model, which is the default; the `test_j2_ast_*`
templates read the full model and need `--ast`. Mixing them fails at render
time with `variable 'model' not found`.

### Renderer Command Line Options

| Option | Description |
|--------|-------------|
| `-t, --template <file>` | Jinja2 template file (`.j2`). Required. |
| `-o, --output[=<file>]` | Output file. Defaults to the design name plus a suffix taken from the template name. |
| `--ast` | Feed the full AST JSON model instead of the simplified one. |
| `--verbose` | Print the JSON structure preview and progress. |
| `-h, --help` | Show help. |

### Renderer Data Structure

Templates receive one of two JSON shapes.

Simplified (`SystemRDL_SimplifiedModel`, the default): a top-level `addrmap`
object plus flat `registers` and `regfiles` arrays. Each register carries a
flat `fields` array, its numeric `offset`, `path` and `path_abs`, `size`,
`register_width` and `register_reset_value`.

```json
{
  "format": "SystemRDL_SimplifiedModel",
  "addrmap": {"absolute_address": "0x0", "base": "0x0", "inst_name": "chip"},
  "registers": [
    {
      "absolute_address": "0x0",
      "fields": [
        {"inst_name": "data", "lsb": 0, "msb": 31, "sw": "rw", "width": 32}
      ],
      "inst_name": "control_reg",
      "offset": 0,
      "register_width": 32,
      "size": 4
    }
  ]
}
```

Full AST (`SystemRDL_ElaboratedModel`, with `--ast`): the same data nested, with
`children` arrays and a `properties` object per node.

```json
{
  "format": "SystemRDL_ElaboratedModel",
  "model": [
    {
      "node_type": "addrmap",
      "inst_name": "chip",
      "absolute_address": "0x0",
      "size": 4096,
      "children": [
        {
          "node_type": "reg",
          "inst_name": "control_reg",
          "absolute_address": "0x0",
          "size": 4,
          "children": [
            {
              "node_type": "field",
              "inst_name": "data",
              "absolute_address": "0x0",
              "properties": {"lsb": 0, "msb": 31, "sw": "rw", "width": 32}
            }
          ]
        }
      ]
    }
  ]
}
```

### Renderer Available Templates

`test/` holds one template per output format, in both JSON variants. The
`ast_` and `json_` infix selects the JSON shape, so the files are
`test_j2_ast_*` and `test_j2_json_*`:

| Template | Output |
| -- | -- |
| `*_header.h.j2` | C header: base address, register offsets, field mask and shift macros |
| `*_doc.md.j2` | Markdown register documentation |
| `*_verilog.v.j2` | Verilog register block with an APB-like `psel`/`penable`/`pwrite` interface |

Pick the variant that matches the JSON you feed it: `json_` with the default
simplified model, `ast_` with `--ast`.

### Renderer Custom Templates

#### Template Language

Templates use Jinja2 syntax with the following features:

**Variables:**

```jinja2
{{ addrmap.inst_name }}              <!-- Address map name -->
{{ node.absolute_address }}          <!-- Node address -->
{{ field.properties.msb }}           <!-- Field MSB -->
{{ field.properties.lsb }}           <!-- Field LSB -->
{{ field.properties.sw }}            <!-- Software access -->
{{ field.properties.hw }}            <!-- Hardware access -->
```

**Loops:**

```jinja2
{% for addrmap in model %}
  {% for node in addrmap.children %}
    {% if node.node_type == "reg" %}
      Register: {{ node.inst_name }}
      {% for field in node.children %}
        {% if field.node_type == "field" %}
          Field: {{ field.inst_name }}
        {% endif %}
      {% endfor %}
    {% endif %}
  {% endfor %}
{% endfor %}
```

**Built-in Filters:**

```jinja2
{{ addrmap.inst_name | upper }}               <!-- Convert to uppercase -->
{{ node.inst_name | lower }}                  <!-- Convert to lowercase -->
{{ field.inst_name | replace("[", "_") }}     <!-- Replace characters -->
{{ node.absolute_address }}                   <!-- Address value -->
```

**Conditionals:**

```jinja2
{% if field.properties.sw == "rw" %}
  Read-write field
{% elif field.properties.sw == "r" %}
  Read-only field
{% endif %}
```

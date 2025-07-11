# Command-Line Tools Usage

After successful build, the executables are located in the `build/` directory. This document covers all command-line tools included in the SystemRDL Toolkit.

## Overview

|          Tool          |                       Description                        |
| ---------------------- | -------------------------------------------------------- |
| `systemrdl_parser`     | Parse SystemRDL files and generate Abstract Syntax Trees |
| `systemrdl_elaborator` | Elaborate parsed designs with semantic analysis          |
| `systemrdl_csv2rdl`    | Convert CSV register specifications to SystemRDL         |
| `systemrdl_render`     | Generate documentation using Jinja2 templates            |

---

## Parser

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

### Parser Command Line Options

- `-j, --json[=<filename>]` - Enable JSON output, optionally specify custom filename
- `-h, --help` - Show help message

If no filename is specified with `--json`, the tool automatically generates: `<input_basename>_ast.json`

---

## Elaborator

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

### Elaborator Command Line Options

- `-j, --json[=<filename>]` - Enable JSON output, optionally specify custom filename
- `-h, --help` - Show help message

If no filename is specified with `--json`, the tool automatically generates: `<input_basename>_elaborated.json`

### Elaborator Gap Detection

The elaborator automatically detects and fills gaps in register field definitions with reserved fields:

```bash
# Test automatic gap detection with a register containing field gaps
./build/systemrdl_elaborator test/test_auto_reserved_fields.rdl
```

**Example SystemRDL input with gaps**:

```systemrdl
addrmap test_auto_reserved_fields {
    reg gap_reg {
        regwidth = 32;

        field {
            sw = rw;
            hw = r;
            desc = "Control bit";
        } ctrl[0:0];        // bit 0

        // Gap: bits 1-3 are unspecified

        field {
            sw = rw;
            hw = r;
            desc = "Status bits";
        } status[7:4];      // bits 4-7

        // Gap: bits 8-15 are unspecified

        field {
            sw = rw;
            hw = r;
            desc = "Data field";
        } data[23:16];      // bits 16-23

        // Gap: bits 24-30 are unspecified

        field {
            sw = rw;
            hw = r;
            desc = "Enable bit";
        } enable[31:31];    // bit 31
    };

    gap_reg test_reg @ 0x0000;
};
```

**Elaborator output with automatic reserved fields**:

```bash
reg: test_reg (size: 4 bytes)
  field: ctrl [0:0]
    width: 1, lsb: 0, msb: 0, sw: "rw"
  field: RESERVED_3_1 [3:1]    # Automatically generated
    width: 3, lsb: 1, msb: 3, sw: "r"
  field: status [7:4]
    width: 4, lsb: 4, msb: 7, sw: "rw"
  field: RESERVED_15_8 [15:8]  # Automatically generated
    width: 8, lsb: 8, msb: 15, sw: "r"
  field: data [23:16]
    width: 8, lsb: 16, msb: 23, sw: "rw"
  field: RESERVED_30_24 [30:24] # Automatically generated
    width: 7, lsb: 24, msb: 30, sw: "r"
  field: enable [31:31]
    width: 1, lsb: 31, msb: 31, sw: "rw"
```

**Features of automatic gap detection**:

- **Gap Detection**: Analyzes all field bit ranges to identify unspecified bits
- **Naming Convention**: Reserved fields use `RESERVED_<msb>_<lsb>` naming convention
- **Read-Only Properties**: Reserved fields are automatically set to `sw=r, hw=na`
- **Complete Coverage**: Ensures all register bits from 0 to (regwidth-1) are covered
- **Register Width Support**: Works with 8-bit, 16-bit, 32-bit, 64-bit, and custom register widths
- **Performance Optimized**: Only processes registers with detected gaps

---

## CSV2RDL Converter

The toolkit includes a CSV to SystemRDL converter with parsing capabilities and validation features.

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

### CSV2RDL Format Requirements

The converter supports a three-layer structure: **addrmap → reg → field**. CSV files should contain the following columns (header names are case-insensitive with fuzzy matching support):

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

### CSV2RDL Structure Specification

**Important: CSV files must follow a strict logical row structure**

#### Row Type Definitions

1. **Header Row** (Line 1): Column names defining the structure
2. **Addrmap Row**: Contains `addrmap_offset` and `addrmap_name`, all other fields should be empty
3. **Register Row**: Contains `reg_offset`, `reg_name`, and `reg_width`, may include `description`
4. **Field Row**: Contains `field_name`, `field_lsb`, `field_msb`, `reset_value`, `sw_access`, `hw_access`, and optionally `description`

#### Structural Rules

1. **First data row** (Line 2) **MUST** be an addrmap row
2. **After an addrmap row**, the next row **MUST** be a register row
3. **After a register row**, subsequent rows **MUST** be field rows until a new register or addrmap is encountered
4. **Mixed row types are forbidden**: Each row can only contain one type of information (addrmap, register, or field)
5. **Logical vs Physical rows**: Multi-line CSV cells (quoted with newlines) count as single logical rows

### CSV2RDL Example Structure

**Correct Structure:**

```csv
addrmap_offset,addrmap_name,reg_offset,reg_name,reg_width,field_name,field_lsb,field_msb,reset_value,sw_access,hw_access,description
0x0000,DEMO,,,,,,,,,,                           # Line 2: Addrmap row
,,0x0000,CTRL,32,,,,,,,Control register         # Line 3: Register row  
,,,,,ENABLE,0,0,0,RW,RW,Enable control bit      # Line 4: Field row
,,,,,MODE,1,2,0,RW,RW,Operation mode            # Line 5: Field row
,,0x0004,STATUS,32,,,,,,,Status register        # Line 6: New register row
,,,,,READY,0,0,0,RO,RO,Ready status             # Line 7: Field row
```

### CSV2RDL Features

#### Header Matching

- **Case-insensitive**: `AddrmapOffset` → `addrmap_offset`
- **Fuzzy matching**: Handles typos with Levenshtein distance ≤3
- **Abbreviation support**: `sw_acc` → `sw_access`, `hw_acc` → `hw_access`

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

The SystemRDL Template Renderer generates various output formats from SystemRDL designs using Jinja2 templates.

### Renderer Overview

**systemrdl_render** is a command-line tool that:

1. Parses and elaborates SystemRDL files
2. Converts the elaborated design to JSON data
3. Renders the data using Jinja2 templates
4. Generates output files in any desired format

### Renderer Basic Usage

```bash
# Generate C header file
./systemrdl_render design.rdl -t test/test_j2_header.h.j2

# Generate documentation with custom output name
./systemrdl_render design.rdl -t test/test_j2_doc.md.j2 -o design_documentation.md

# Generate Verilog RTL
./systemrdl_render design.rdl -t test/test_j2_verilog.v.j2

# Verbose output
./systemrdl_render design.rdl -t test/test_j2_header.h.j2 -v
```

### Renderer Command Line Options

| Option | Description | Example |
|--------|-------------|---------|
| `-t, --template` | **Required.** Jinja2 template file (.j2) | `-t test/test_j2_header.h.j2` |
| `-o, --output` | Output file (auto-generated if not specified) | `-o my_output.h` |
| `-v, --verbose` | Enable verbose output | `-v` |
| `-h, --help` | Show help message | `-h` |

### Renderer Data Structure

The tool provides the following JSON data structure to templates:

```json
{
  "format": "SystemRDL_ElaboratedModel",
  "model": [
    {
      "absolute_address": "0x0",
      "children": [
        {
          "absolute_address": "0x0",
          "children": [
            {
              "absolute_address": "0x0",
              "inst_name": "data",
              "node_type": "field",
              "properties": {
                "hw": "r",
                "lsb": 0,
                "msb": 31,
                "sw": "rw",
                "width": 32
              }
            }
          ],
          "inst_name": "control_reg",
          "node_type": "reg",
          "size": 4
        }
      ],
      "inst_name": "chip",
      "node_type": "addrmap",
      "size": 4096
    }
  ]
}
```

### Renderer Available Templates

#### C Header Template (`test/test_j2_header.h.j2`)

Generates C header files with register and field definitions:

```c
/*
 * Auto-generated C header file from SystemRDL
 * Generated by SystemRDL Template Renderer
 */

#ifndef CHIP_H
#define CHIP_H

#include <stdint.h>

/* Address Map: chip */
#define CHIP_BASE_ADDR  0x00000000
#define CHIP_SIZE       4096

/* Register: control_reg */
#define CHIP_CONTROL_REG_OFFSET  0x0000
#define CHIP_CONTROL_REG_ADDR    (CHIP_BASE_ADDR + CHIP_CONTROL_REG_OFFSET)

/* Field: control_reg.data */
#define CHIP_CONTROL_REG_DATA_MASK    0xFFFFFFFF
#define CHIP_CONTROL_REG_DATA_SHIFT   0
```

**Usage:**

```bash
./systemrdl_render your_design.rdl -t test/test_j2_header.h.j2
# Generates: your_design_header.h
```

#### Markdown Documentation Template (`test/test_j2_doc.md.j2`)

Generates register documentation with formatting.

**Usage:**

```bash
./systemrdl_render your_design.rdl -t test/test_j2_doc.md.j2
# Generates: your_design_doc.md
```

#### Verilog RTL Template (`test/test_j2_verilog.v.j2`)

Generates complete Verilog RTL modules with APB-like interface:

**Features:**

- Complete RTL module with parameterized design
- APB-like CPU interface (psel, penable, pwrite, etc.)
- Hardware interface signals based on access permissions
- Register definitions and address decode
- Write/read logic with async reset
- Hardware output assignments

**Usage:**

```bash
./systemrdl_render your_design.rdl -t test/test_j2_verilog.v.j2
# Generates: your_design_verilog.v
```

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

---

## Examples

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

### Parser Output

```bash
Parsing successful!

=== Abstract Syntax Tree ===
Component Definition
    Type: addrmap
        Type: reg
            Type: field
              Property: sw=rw
          Instance: data[31:0]
            Range: [31:0]
      Instance: reg1@0x0
        Address: @0x0
        ...
```

### Elaborator Output

```bash
Parsing SystemRDL file: example.rdl
Parsing successful!

Starting elaboration...
Elaboration successful!

=== Elaborated SystemRDL Model ===
addrmap: simple_chip @ 0x0
  reg: reg1 (size: 4 bytes)
    field: data [31:0]
      width: 32
      lsb: 0
      sw: "rw"
      msb: 31
  reg: reg2 @ 0x4 (size: 4 bytes)
    field: status @ 0x4 [7:0]
      width: 8
      lsb: 0
      sw: "rw"
      msb: 7

Address Map:
Address     Size    Name      Path
------------------------------------
0x00000000  4       reg1      simple_chip.reg1
0x00000004  4       reg2      simple_chip.reg2
```

### JSON Output

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
              "properties": {
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

---

## Validation and Testing

### CSV2RDL Validation

Run validation from any directory:

```bash
# Run complete validation suite
python3 script/csv2rdl_validator.py

# The validator performs three levels of testing:
# 1. CSV2RDL conversion success
# 2. SystemRDL syntax validation (using parser)
# 3. Content pattern validation
```

### Manual Testing

```bash
# Convert and validate manually
./build/systemrdl_csv2rdl test/test_csv_basic_example.csv
./build/systemrdl_parser test/test_csv_basic_example.rdl
```

### General Testing

```bash
# Run all tests
make test

# Quick validation tests
make test-fast

# Specific test categories
make test-parser test-elaborator test-csv2rdl
```

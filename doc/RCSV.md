# RCSV (Register-CSV) Specification v0.4

RCSV is a practical, field-oriented CSV format for SystemRDL Toolkit. It provides a standardized way to represent register maps that can be converted to SystemRDL without semantic loss.

**Design Principles:**

- **Simplicity**: Easy to understand and create manually or programmatically
- **Direct Mapping**: CSV structure maps directly to SystemRDL syntax
- **Completeness**: Preserves all essential register/field information
- **Validation**: Clear error messages for malformed input

---

## 1. Purpose and Scope

RCSV addresses the need for a standardized CSV format for register map interchange:

- **Primary Use**: Converting CSV register specifications to SystemRDL
- **Target Audience**: Hardware engineers, verification engineers, documentation teams
- **Scope**: Elaborated register maps with resolved addresses, widths, and properties
- **Single Address Map**: One RCSV file describes exactly one address map (for multiple address maps, use separate files)
- **Compatibility**: Designed to work with SystemRDL Toolkit's existing CSV2RDL converter

---

## 2. File Format and Encoding

RCSV follows standard CSV conventions with specific requirements:

- **Encoding**: UTF-8 with Unix line endings (`\n`)
- **Structure**: Comma-separated values with **mandatory header row**
- **Delimiter**: Standard comma (`,`) - semicolon (`;`) support is optional for compatibility
- **Quoting**: Multi-line cells supported with double quotes (`"`)
- **Escaping**: Double quotes in cells escaped as `""` (RFC 4180 compliant)

---

## 3. Structure Overview

RCSV uses a **row-based hierarchical structure** compatible with existing CSV tools:

- **Header Row**: Defines column names. Column names MUST match exactly and are case-sensitive.
- **Address Map Row**: Defines the top-level address map container
- **Register Rows**: Define individual registers within the address map
- **Field Rows**: Define fields within each register (one row per field)

This structure maintains compatibility with the SystemRDL Toolkit's current CSV2RDL implementation while providing standardization.

---

## 4. Required Columns

RCSV defines a standard set of columns compatible with existing CSV2RDL tools:

### 4.1 Core Identification Columns

|      Column      | Required |                   Description                    |   Example   |
| ---------------- | -------- | ------------------------------------------------ | ----------- |
| `addrmap_offset` | Yes      | Address map base offset (hex/decimal)            | `0x0000`    |
| `addrmap_name`   | Yes      | Address map instance name                        | `DEMO_CHIP` |
| `reg_offset`     | Yes      | Register offset within address map (hex/decimal) | `0x1000`    |
| `reg_name`       | Yes      | Register instance name                           | `CTRL_REG`  |
| `reg_width`      | Yes      | Register width in bits                           | `32`        |

### 4.2 Field Definition Columns

|    Column     | Required |             Description              |   Example    |
| ------------- | -------- | ------------------------------------ | ------------ |
| `field_name`  | Yes      | Field instance name                  | `ENABLE`     |
| `field_lsb`   | Yes      | Field least significant bit position | `0`          |
| `field_msb`   | Yes      | Field most significant bit position  | `3`          |
| `reset_value` | Yes      | Field reset value (decimal/hex)      | `5` or `0x5` |

### 4.3 Access Control Columns

|   Column    | Required |         Description         |      Valid Values      |
| ----------- | -------- | --------------------------- | ---------------------- |
| `sw_access` | Yes      | Software access permissions | `RW`, `RO`, `WO`, `NA` |
| `hw_access` | Yes      | Hardware access permissions | `RW`, `RO`, `WO`, `NA` |

### 4.4 Read/Write Behavior Columns

|  Column   | Required |        Description         |                             Valid Values                              |
| --------- | -------- | -------------------------- | --------------------------------------------------------------------- |
| `onread`  | No       | Read side-effect behavior  | `rclr`, `rset`, `ruser`                                               |
| `onwrite` | No       | Write side-effect behavior | `woclr`, `woset`, `wot`, `wzs`, `wzc`, `wzt`, `wclr`, `wset`, `wuser` |

### 4.5 Optional Documentation Column

|    Column     | Required |                Description                |       Example        |
| ------------- | -------- | ----------------------------------------- | -------------------- |
| `description` | No       | Human-readable field/register description | `Enable control bit` |

### 4.6 Column Name Rules

All column headers MUST match the standard names EXACTLY and are case-sensitive:

- `addrmap_offset`
- `addrmap_name`
- `reg_offset`
- `reg_name`
- `reg_width`
- `field_name`
- `field_lsb`
- `field_msb`
- `reset_value`
- `sw_access`
- `hw_access`
- `onread` (optional)
- `onwrite` (optional)
- `description` (optional)

### 4.7 Array Support

Register arrays are specified directly in the `reg_name` column using SystemRDL syntax:

```csv
,,0x0000,BUFFER[8],32,,,,,,,8-element buffer array
```

This generates SystemRDL: `BUFFER[8] @ 0x0000` which expands to 8 registers with automatic address calculation. No additional columns needed.

---

## 5. Row Structure and Hierarchy

RCSV uses a **row-based approach** to define the three-level hierarchy: Address Map -> Register -> Field.

### 5.1 Row Type Identification

Rows are identified by which columns contain data:

|    Row Type     |                                Populated Columns                                |         Empty Columns         |
| --------------- | ------------------------------------------------------------------------------- | ----------------------------- |
| **Address Map** | `addrmap_offset`, `addrmap_name`                                                | All register/field columns    |
| **Register**    | `reg_offset`, `reg_name`, `reg_width`                                           | Address map and field columns |
| **Field**       | `field_name`, `field_lsb`, `field_msb`, `reset_value`, `sw_access`, `hw_access` | Address map columns           |

### 5.2 Structural Rules

1. **Header Row**: First row must contain column names
2. **Address Map Row**: Second row must define the address map
3. **Register Row**: Must appear before its associated field rows
4. **Field Rows**: Must immediately follow their parent register row
5. **Sequential Processing**: Rows processed in order, maintaining hierarchy

### 5.3 Example Structure

```csv
addrmap_offset,addrmap_name,reg_offset,reg_name,reg_width,field_name,field_lsb,field_msb,reset_value,sw_access,hw_access,description
0x0000,DEMO,,,,,,,,,,
,,0x0000,CTRL,32,,,,,,,"Control register"
,,,,,ENABLE,0,0,0,RW,RW,"Enable control bit"
,,,,,MODE,1,2,0,RW,RW,"Operation mode"
,,0x0004,STATUS,32,,,,,,,"Status register"
,,,,,READY,0,0,0,RO,RO,"Ready status"
```

**Row Type Explanation:**

- Row 2: Address Map Row (defines DEMO address map)
- Row 3: Register Row (defines CTRL register)
- Rows 4-5: Field Rows (ENABLE and MODE fields in CTRL register)
- Row 6: Register Row (defines STATUS register)
- Row 7: Field Row (READY field in STATUS register)

---

## 6. Access Control Semantics

RCSV uses separate `sw_access` and `hw_access` columns to specify field access permissions.

### 6.1 Software Access Values (`sw_access`)

| Value |    Meaning     | SystemRDL Equivalent |
| ----- | -------------- | -------------------- |
| `RW`  | Read/Write     | `sw = rw`            |
| `RO`  | Read Only      | `sw = r`             |
| `WO`  | Write Only     | `sw = w`             |
| `NA`  | Not Accessible | `sw = na`            |

### 6.2 Hardware Access Values (`hw_access`)

| Value |    Meaning     | SystemRDL Equivalent |
| ----- | -------------- | -------------------- |
| `RW`  | Read/Write     | `hw = rw`            |
| `RO`  | Read Only      | `hw = r`             |
| `WO`  | Write Only     | `hw = w`             |
| `NA`  | Not Accessible | `hw = na`            |

### 6.3 Common Access Patterns

| sw_access | hw_access |                Use Case                |
| --------- | --------- | -------------------------------------- |
| `RW`      | `RW`      | Control register                       |
| `RO`      | `WO`      | Status register (HW writes, SW reads)  |
| `WO`      | `RO`      | Command register (SW writes, HW reads) |
| `RO`      | `RO`      | Configuration constant                 |

---

## 7. Side-Effect Behaviors

**OnRead**: `rclr` (clear on read), `rset` (set on read), `ruser` (user-defined)

**OnWrite**: `woclr` (W1C), `woset` (W1S), `wot` (W1T), `wzs` (W0S), `wzc` (W0C), `wzt` (W0T), `wclr` (write clear), `wset` (write set), `wuser` (user-defined)

---

## 8. Reset Values

Support decimal (`42`), hex (`0x2A`), or empty (no reset). Value must fit within field width.

---

## 9. Addresses

Absolute address = `addrmap_offset + reg_offset`. Register width in bits (8, 16, 32, 64).

---

## 10. Validation

- Field ranges must not overlap within registers
- MSB >= LSB, ranges fit within register width
- Access values: RW/RO/WO/NA (case insensitive)
- Names must be valid SystemRDL identifiers
- Row order: Address map -> Register -> Fields

---

## 11. Minimal Compliance Set

For full RCSV compliance, all CSV files must include these columns:

**Required Columns (11 total):**

```csv
addrmap_offset, addrmap_name, reg_offset, reg_name, reg_width,
field_name, field_lsb, field_msb, reset_value, sw_access, hw_access
```

**Optional Columns:**

```csv
onread, onwrite, description
```

This minimal set ensures complete SystemRDL generation without information loss.

---

## 12. Complete Example

Here's a practical RCSV example demonstrating all features:

```csv
addrmap_offset,addrmap_name,reg_offset,reg_name,reg_width,field_name,field_lsb,field_msb,reset_value,sw_access,hw_access,onread,onwrite,description
0x0000,DEMO_CHIP,,,,,,,,,,,,"Demo chip register map"
,,0x0000,SYS_CTRL,32,,,,,,,,,"System control register"
,,,,,ENABLE,0,0,1,RW,RW,,,"System enable bit"
,,,,,MODE,1,3,2,RW,RW,,,"3-bit operation mode (0-7)"
,,,,,RESERVED_7_4,4,7,0,RO,NA,,,"Reserved bits"
,,,,,IRQ_EN,8,8,0,RW,RW,,,"Interrupt enable"
,,,,,DEBUG,9,9,0,RW,RW,,,"Debug mode enable"
,,,,,RESET_REQ,31,31,0,WO,RO,,"woset","Write 1 to trigger reset"
,,0x0004,STATUS,32,,,,,,,,,"Status register"
,,,,,READY,0,0,0,RO,WO,,,"System ready flag"
,,,,,ERROR,1,1,0,RO,WO,,"woclr","Error status (W1C)"
,,,,,INT_STATUS,8,15,0,RO,WO,"rclr",,"Interrupt status (clear on read)"
,,,,,DEVICE_ID,16,31,0xDEAD,RO,RO,,,"Device identification"
,,0x0008,DATA,32,,,,,,,,,"Data register"
,,,,,VALUE,0,31,0,RW,RW,,,"32-bit data value"
```

**Key Features Demonstrated:**

- Standard three-tier hierarchy (address map -> registers -> fields)
- Mixed access patterns (RW/RO/WO combinations)
- Hexadecimal values (addresses and reset values)
- Reserved field naming convention with recommended RO/NA access
- Complete bit coverage within registers
- Corrected field width descriptions (3-bit MODE field: 0-7)
- Read/write side-effects: Write-one-set (woset), Write-one-clear (woclr), Read-clear (rclr)

### 12.1 Array Example

Register arrays are specified directly in the `reg_name` column:

```csv
addrmap_offset,addrmap_name,reg_offset,reg_name,reg_width,field_name,field_lsb,field_msb,reset_value,sw_access,hw_access,description
0x0000,ARRAY_DEMO,,,,,,,,,,Array demonstration
,,0x0000,BUFFER[8],32,,,,,,,8-element buffer array
,,,,,DATA,0,31,0,RW,RW,Buffer data value
```

This generates SystemRDL `BUFFER[8] @ 0x0000` which expands to 8 registers: `BUFFER[0]` through `BUFFER[7]`.

---

## 13. SystemRDL Output

RCSV maps directly to SystemRDL syntax. Arrays like `BUFFER[8]` become `BUFFER[8] @ address` and auto-expand to individual register instances.

---

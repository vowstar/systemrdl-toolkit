# RCSV (Register-CSV) Specification v0.4

RCSV is a field-oriented CSV format for SystemRDL Toolkit that describes
register maps in a spreadsheet and converts them to SystemRDL.

---

## 1. Purpose and Scope

RCSV addresses the need for a standardized CSV format for register map interchange:

- **Primary Use**: Converting CSV register specifications to SystemRDL
- **Target Audience**: Hardware engineers, verification engineers, documentation teams
- **Scope**: Elaborated register maps with resolved addresses, widths, and properties
- **Single Address Map**: One RCSV file describes exactly one address map (for multiple address maps, use separate files)

---

## 2. File Format and Encoding

RCSV follows standard CSV conventions with specific requirements:

- **Encoding**: UTF-8 with Unix line endings (`\n`)
- **Structure**: Comma-separated values with **mandatory header row**
- **Delimiter**: Standard comma (`,`); semicolon (`;`) is also accepted (auto-detected per file)
- **Quoting**: Multi-line cells supported with double quotes (`"`)
- **Escaping**: Double quotes in cells escaped as `""` (RFC 4180 compliant)
- **Comments**: A line whose first non-blank character is `#` is ignored

---

## 3. Structure Overview

RCSV uses a **row-based hierarchical structure**:

- **Header Row**: Defines column names; matching rules are in section 4.6.
- **Address Map Row**: Defines the top-level address map container
- **Register Rows**: Define individual registers within the address map
- **Field Rows**: Define fields within each register (one row per field)

---

## 4. Required Columns

RCSV defines the following columns:

### 4.1 Core Identification Columns

|      Column      | Required |                   Description                    |   Example   |
| ---------------- | -------- | ------------------------------------------------ | ----------- |
| `addrmap_offset` | Yes      | Address map base offset (hex/decimal)            | `0x0000`    |
| `addrmap_name`   | Yes      | Address map instance name                        | `DEMO_CHIP` |
| `reg_offset`     | Yes      | Register offset within address map (hex/decimal) | `0x1000`    |
| `reg_name`       | Yes      | Register instance name                           | `CTRL_REG`  |
| `reg_width`      | Yes      | Register width in bits, `2^N` with `N >= 3`      | `32`        |

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

| Column | Required | Description | Valid Values |
| -- | -- | -- | -- |
| `onread` | No | Read side-effect behavior | `rclr`, `rset`, `ruser` |
| `onwrite` | No | Write side-effect behavior | `woclr`, `woset`, `wot`, `wzs`, `wzc`, `wzt`, `wclr`, `wset`, `wuser` |

### 4.5 Optional Documentation Column

|    Column     | Required |                Description                |       Example        |
| ------------- | -------- | ----------------------------------------- | -------------------- |
| `description` | No       | Human-readable field/register description | `Enable control bit` |

### 4.6 Column Name Rules

Header names are matched case-insensitively against the standard names in
sections 4.1 to 4.5. The abbreviation map `sw_acc`, `hw_acc`, `access`,
`addr_offset`, `addr_name`, `lsb`, `msb`, `desc` and `width` is applied first;
otherwise the closest standard name within an edit distance of 3 is used. A
header that matches no standard name is ignored.

### 4.7 Array Support

Register arrays are specified directly in the `reg_name` column using SystemRDL syntax:

```csv
,,0x0000,BUFFER[8],32,,,,,,,8-element buffer array
```

This generates `BUFFER[8] @ 0x0000`, which expands to 8 registers with
addresses calculated automatically. No additional columns are needed.

---

## 5. Row Structure and Hierarchy

RCSV uses a **row-based approach** to define the three-level hierarchy: Address Map -> Register -> Field.

### 5.1 Row Type Identification

Rows are identified by which columns contain data:

| Row Type | Populated Columns | Empty Columns |
| -- | -- | -- |
| **Address Map** | `addrmap_offset`, `addrmap_name` | All register/field columns |
| **Register** | `reg_offset`, `reg_name`, `reg_width` | Address map and field columns |
| **Field** | `field_name`, `field_lsb`, `field_msb`, `reset_value`, `sw_access`, `hw_access` | Address map columns |

### 5.2 Structural Rules

1. **Header Row**: First row must contain column names
2. **Address Map Row**: Second row must define the address map
3. **Register Row**: Must appear before its associated field rows
4. **Field Rows**: Must immediately follow their parent register row
5. **Sequential Processing**: Rows processed in order, maintaining hierarchy
6. **Register Width**: `reg_width` must be `2^N` with `N >= 3`, that is 8, 16,
   32, 64 and so on. SystemRDL 2.0 requires this in clauses 10.1-f and
   10.6.1-a, and elaboration rejects any other value. A register that holds
   fewer meaningful bits is written at the next legal width with the unused
   bits left to the automatic reserved field generation.

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

What the hardware does when software touches the field.

`onread`:

| Value | Effect |
| -- | -- |
| `rclr` | Clear the field |
| `rset` | Set the field |
| `ruser` | User defined |

`onwrite`:

| Value | Also known as | Effect |
| -- | -- | -- |
| `woclr` | W1C | Writing 1 clears the bit |
| `woset` | W1S | Writing 1 sets the bit |
| `wot` | W1T | Writing 1 toggles the bit |
| `wzc` | W0C | Writing 0 clears the bit |
| `wzs` | W0S | Writing 0 sets the bit |
| `wzt` | W0T | Writing 0 toggles the bit |
| `wclr` | | Any write clears the field |
| `wset` | | Any write sets the field |
| `wuser` | | User defined |

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

A file is RCSV-compliant when it supplies every column marked Required in
section 4; the columns marked No are optional.

---

## 12. Complete Example

The example below exercises the full column set:

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

### 12.1 Array Example

```csv
addrmap_offset,addrmap_name,reg_offset,reg_name,reg_width,field_name,field_lsb,field_msb,reset_value,sw_access,hw_access,description
0x0000,ARRAY_DEMO,,,,,,,,,,Array demonstration
,,0x0000,BUFFER[8],32,,,,,,,8-element buffer array
,,,,,DATA,0,31,0,RW,RW,Buffer data value
```

This generates SystemRDL `BUFFER[8] @ 0x0000` which expands to 8 registers: `BUFFER[0]` through `BUFFER[7]`.

#pragma once

#include "SystemRDLParser.h"
#include "systemrdl_bitvector.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace systemrdl {

// Forward declarations
class ElaboratedNode;
class ElaboratedAddrmap;
class ElaboratedRegfile;
class ElaboratedReg;
class ElaboratedField;
class ElaboratedMem;

// Basic type definitions
using Address         = uint64_t;
using Size            = uint64_t;
using ArrayDimensions = std::vector<size_t>;

// Property value type
struct PropertyValue
{
    // BITVECTOR carries a value of a known width that need not fit in int64_t.
    // A 128-bit reset value has no other home: INTEGER would silently keep only
    // its low bits.
    enum Type { STRING, INTEGER, BOOLEAN, ENUM, BITVECTOR } type;
    std::string string_val;
    int64_t     int_val;
    bool        bool_val;
    BitVector   bits_val;

    PropertyValue()
        : type(STRING)
        , string_val("")
        , int_val(0)
        , bool_val(false)
    {}
    explicit PropertyValue(const std::string &s)
        : type(STRING)
        , string_val(s)
        , int_val(0)
        , bool_val(false)
    {}
    explicit PropertyValue(int64_t i)
        : type(INTEGER)
        , string_val("")
        , int_val(i)
        , bool_val(false)
    {}
    explicit PropertyValue(bool b)
        : type(BOOLEAN)
        , string_val("")
        , int_val(0)
        , bool_val(b)
    {}
    explicit PropertyValue(BitVector bits)
        : type(BITVECTOR)
        , string_val("")
        , int_val(0)
        , bool_val(false)
        , bits_val(std::move(bits))
    {}
};

// Parameter definition
struct ParameterDefinition
{
    std::string   name;
    std::string   data_type;
    PropertyValue default_value;
    bool          has_default = false;
    bool          is_array    = false;
};

// Parameter instantiation
struct ParameterAssignment
{
    std::string   name;
    PropertyValue value;
};

// Enum definition
struct EnumEntry
{
    std::string name;
    int64_t     value;
};

struct EnumDefinition
{
    std::string            name;
    std::vector<EnumEntry> entries;
};

// Struct definition
struct StructMember
{
    std::string   name;
    std::string   type;
    PropertyValue default_value;
    bool          has_default = false;
};

struct StructDefinition
{
    std::string               name;
    std::vector<StructMember> members;
};

// Base class for elaborated nodes
class ElaboratedNode
{
public:
    virtual ~ElaboratedNode() = default;

    // Basic information
    std::string inst_name;
    std::string type_name;
    Address     absolute_address = 0;
    Size        size             = 0;

    // Source location information for better error reporting
    antlr4::ParserRuleContext *source_ctx = nullptr;

    // Array information
    ArrayDimensions      array_dimensions;
    std::vector<Address> array_strides;
    std::vector<size_t>  array_indices; // Current instance indices in array

    // Properties
    std::unordered_map<std::string, PropertyValue> properties;

    // Hierarchical relationships
    ElaboratedNode                              *parent = nullptr;
    std::vector<std::unique_ptr<ElaboratedNode>> children;

    // Utility methods
    virtual std::string    get_hierarchical_path() const;
    virtual void           add_child(std::unique_ptr<ElaboratedNode> child);
    virtual PropertyValue *get_property(const std::string &name);
    virtual void           set_property(const std::string &name, const PropertyValue &value);

    // Pure virtual functions
    virtual std::string get_node_type() const                                = 0;
    virtual void        accept_visitor(class ElaboratedNodeVisitor &visitor) = 0;
};

// Address map node
class ElaboratedAddrmap : public ElaboratedNode
{
public:
    std::string get_node_type() const override { return "addrmap"; }
    void        accept_visitor(ElaboratedNodeVisitor &visitor) override;

    // Find child nodes
    ElaboratedNode *find_child_by_name(const std::string &name) const;
    ElaboratedNode *find_child_by_address(Address addr) const;
};

// Register file node
class ElaboratedRegfile : public ElaboratedNode
{
public:
    std::string get_node_type() const override { return "regfile"; }
    void        accept_visitor(ElaboratedNodeVisitor &visitor) override;

    // Find child nodes
    ElaboratedNode *find_child_by_name(const std::string &name) const;
    ElaboratedNode *find_child_by_address(Address addr) const;

    // regfile-specific properties
    Address alignment = 4; // Alignment requirement
};

// Register node
class ElaboratedReg : public ElaboratedNode
{
public:
    std::string get_node_type() const override { return "reg"; }
    void        accept_visitor(ElaboratedNodeVisitor &visitor) override;

    // Register-specific properties
    uint32_t    register_width = 32; // Register bit width (10.1-e: default is 32)
    std::string register_reset_hex;  // Register reset value in 0x format

    // Minimum software access width. Zero means the source did not specify one,
    // in which case it defaults to register_width (10.6.1-d).
    uint32_t access_width = 0;

    // Bit ordering of this register (10.7). Fields without an explicit range
    // are packed from bit 0 upward when false, and from regwidth-1 downward
    // when true.
    bool msb0 = false;

    uint32_t effective_access_width() const
    {
        return access_width != 0 ? access_width : register_width;
    }

    // Find fields
    ElaboratedField *find_field_by_name(const std::string &name) const;
    ElaboratedField *find_field_by_bit_range(size_t msb, size_t lsb) const;
};

// Field node
class ElaboratedField : public ElaboratedNode
{
public:
    std::string get_node_type() const override { return "field"; }
    void        accept_visitor(ElaboratedNodeVisitor &visitor) override;

    // Field-specific properties
    size_t msb   = 0; // Most significant bit
    size_t lsb   = 0; // Least significant bit
    size_t width = 0; // Bit width

    // Reset value at the literal's natural width until the field geometry is
    // final, then normalized to the field width. There is no 64-bit ceiling.
    BitVector reset_value;

    // Whether the source actually specified a reset. A field with no reset is
    // undefined at power-up and must not be reported as resetting to zero:
    // that difference decides whether reset logic is generated at all.
    bool has_reset = false;

    // The source wrote the bit range as [low:high], which selects msb0 ordering
    // (10.7 b). msb and lsb above always hold the normalised positions, so this
    // only records how the range was written.
    bool declared_msb0 = false;

    // Access types
    enum AccessType { RW, R, W, W1C, W1S, W1T, W0C, W0S, W0T, NA };
    AccessType sw_access = RW;
    AccessType hw_access = RW;
};

// Memory node
class ElaboratedMem : public ElaboratedNode
{
public:
    std::string get_node_type() const override { return "mem"; }
    void        accept_visitor(ElaboratedNodeVisitor &visitor) override;

    // Memory-specific properties
    Size        memory_size   = 0;     // Memory size (bytes)
    size_t      data_width    = 32;    // Data bit width
    size_t      address_width = 32;    // Address bit width
    std::string memory_type   = "ram"; // Memory type: ram, rom, etc.

    // Find functionality (memory usually doesn't contain child components, but keeps interface consistency)
    ElaboratedNode *find_child_by_name(const std::string &name) const;
    ElaboratedNode *find_child_by_address(Address addr) const;
};

// Visitor pattern interface
class ElaboratedNodeVisitor
{
public:
    virtual ~ElaboratedNodeVisitor()            = default;
    virtual void visit(ElaboratedAddrmap &node) = 0;
    virtual void visit(ElaboratedRegfile &node) = 0;
    virtual void visit(ElaboratedReg &node)     = 0;
    virtual void visit(ElaboratedField &node)   = 0;
    virtual void visit(ElaboratedMem &node)     = 0;
};

// Main elaborator class
class SystemRDLElaborator
{
public:
    SystemRDLElaborator();
    ~SystemRDLElaborator();

    // Main interface
    std::unique_ptr<ElaboratedAddrmap> elaborate(SystemRDLParser::RootContext *ast_root);

    // Error handling
    struct ElaborationError
    {
        std::string message;
        size_t      line   = 0;
        size_t      column = 0;
    };

    const std::vector<ElaborationError> &get_errors() const { return errors_; }
    bool                                 has_errors() const { return !errors_.empty(); }

private:
    std::vector<ElaborationError> errors_;

    // Symbol table: stores named component definitions
    struct ComponentDefinition
    {
        std::string                                  name;
        std::string                                  type;
        SystemRDLParser::Component_named_defContext *def_ctx;
        std::vector<ParameterDefinition>             parameters; // Parameter definition list
    };
    std::unordered_map<std::string, ComponentDefinition> component_definitions_;

    // Enum and struct definitions
    std::unordered_map<std::string, EnumDefinition>   enum_definitions_;
    std::unordered_map<std::string, StructDefinition> struct_definitions_;

    // Parameter scopes, innermost last.
    //
    // Instantiation nests: a parameterized regfile instantiates parameterized
    // registers inside its own body, and the inner instance must not destroy
    // the outer parameters. A single flat map made "regs[NUM_REGS] @ BASE_ADDR"
    // unresolvable as soon as the inner instance bound its own parameters.
    std::vector<std::unordered_map<std::string, PropertyValue>> parameter_scopes_;

    // Internal elaboration methods
    void elaborate_component_body(
        SystemRDLParser::Component_bodyContext *body_ctx, ElaboratedNode *parent);

    void elaborate_component_definition(
        SystemRDLParser::Component_defContext *comp_def,
        ElaboratedNode                        *parent,
        Address                               &current_address);

    void elaborate_component_instance(
        SystemRDLParser::Component_anon_defContext *def_ctx,
        SystemRDLParser::Component_instContext     *inst_ctx,
        ElaboratedNode                             *parent,
        Address                                    &current_address,
        const std::string                          &comp_type);

    void elaborate_array_instance(
        SystemRDLParser::Component_anon_defContext *def_ctx,
        SystemRDLParser::Component_instContext     *inst_ctx,
        ElaboratedNode                             *parent,
        Address                                    &current_address,
        const std::string                          &comp_type);

    // Handle named component definitions and instantiation
    void collect_component_definitions(SystemRDLParser::RootContext *ast_root);

    void collect_component_definitions_from_body(SystemRDLParser::Component_bodyContext *body_ctx);

    void register_component_definition(SystemRDLParser::Component_named_defContext *named_def);

    void elaborate_named_component_instance(
        const std::string                      &type_name,
        SystemRDLParser::Component_instContext *inst_ctx,
        ElaboratedNode                         *parent,
        Address                                &current_address);

    void elaborate_named_array_instance(
        const std::string                      &type_name,
        SystemRDLParser::Component_instContext *inst_ctx,
        ElaboratedNode                         *parent,
        Address                                &current_address);

    void elaborate_explicit_component_inst(
        SystemRDLParser::Explicit_component_instContext *explicit_inst,
        ElaboratedNode                                  *parent,
        Address                                         &current_address);

    // Property handling methods
    void elaborate_local_property_assignment(
        SystemRDLParser::Local_property_assignmentContext *local_prop, ElaboratedNode *parent);

    void elaborate_dynamic_property_assignment(
        SystemRDLParser::Dynamic_property_assignmentContext *dynamic_prop, ElaboratedNode *parent);

    PropertyValue evaluate_property_value(SystemRDLParser::ExprContext *expr_ctx);

    PropertyValue evaluate_property_value(SystemRDLParser::Prop_assignment_rhsContext *rhs_ctx);

    // Enhanced expression evaluator
    PropertyValue evaluate_expression(SystemRDLParser::ExprContext *expr_ctx);

    PropertyValue evaluate_expression_primary(SystemRDLParser::Expr_primaryContext *primary_ctx);

    int64_t evaluate_integer_expression_enhanced(SystemRDLParser::ExprContext *expr_ctx);

    // Parse one SystemRDL numeric literal into int64_t.
    //
    // This is the only way numeric literal text becomes a number. std::stoll
    // and std::stoull must not be used on literal text: they stop at the
    // apostrophe of 8'hFF and at the separator of 0x1_0000_0000, returning the
    // prefix as if it were the value.
    //
    // Returns false when the text is not a literal or does not fit in int64_t,
    // so a caller can never mistake a failure for a small value.
    static bool parse_literal_int64(const std::string &text, int64_t &out);

    // Record a field reset value coming from either syntax that can carry one:
    // the instance initializer "f[7:0] = 0xAB" and the property assignment
    // "reset = 0xAB". Both must reach reset_value or the register reset image
    // silently drops the field.
    static void assign_field_reset(ElaboratedField *field, const PropertyValue &value);

    // Field bit range handling
    void elaborate_field_bit_range(
        SystemRDLParser::Component_instContext *inst_ctx, ElaboratedField *field_node);

    // Automatic field positioning
    void resolve_register_bit_order(ElaboratedReg *reg_node, const ElaboratedNode *parent);
    void assign_automatic_field_positions(ElaboratedReg *reg_node);

    // Gap detection and reserved field generation methods
    void                                   detect_and_fill_register_gaps(ElaboratedReg *reg_node);
    std::vector<std::pair<size_t, size_t>> find_register_gaps(ElaboratedReg *reg_node);
    std::unique_ptr<ElaboratedField>       create_reserved_field(
        size_t msb, size_t lsb, const std::string &name);
    std::string generate_reserved_field_name(size_t msb, size_t lsb);

    // Addressing mode from clause 5.1.2.2.2. regalign is the default.
    enum class AddressingMode { Compact, RegAlign, FullAlign };

    // Nearest addressing mode in scope. The property is inherited from the
    // enclosing address map (Table 26).
    AddressingMode addressing_mode_for(const ElaboratedNode *node) const;

    // Nearest explicit alignment property in scope, or 0 when none is set.
    // Inherited by all of a container's non-addrmap children (5.1.2.2.1).
    Address alignment_for(const ElaboratedNode *node) const;

    // Byte alignment an instance of this size must satisfy under the given mode.
    Address mode_alignment_for(
        AddressingMode        mode,
        const ElaboratedNode *node,
        const ElaboratedNode *parent,
        Size                  size) const;

    // Place an instance whose address was not given explicitly, honouring the
    // addressing mode, the alignment property and the %= operator. Returns the
    // offset relative to the parent, and moves the subtree if it shifted.
    Address place_instance(
        ElaboratedNode                         *node,
        const ElaboratedNode                   *parent,
        SystemRDLParser::Component_instContext *inst_ctx,
        Address                                 candidate_offset);

    // Move a node and everything below it by the given number of bytes.
    // Alignment can only be applied once the size is known, which is after the
    // body has been elaborated and the children have taken their addresses.
    static void shift_subtree_address(ElaboratedNode *node, Address delta);

    // Register property validation. Checks the constraints the standard states
    // with "shall", which the elaborator previously accepted silently.
    void validate_register_properties(ElaboratedReg *reg_node);

    // Field validation methods
    void validate_register_fields(ElaboratedReg *reg_node);
    void check_field_overlaps(ElaboratedReg *reg_node);
    void check_field_boundaries(ElaboratedReg *reg_node);
    bool fields_overlap(const ElaboratedField *field1, const ElaboratedField *field2);

    // Field validation error reporting
    void report_field_overlap_error(
        const std::string         &field1_name,
        const std::string         &field2_name,
        size_t                     overlap_start,
        size_t                     overlap_end,
        antlr4::ParserRuleContext *ctx = nullptr);
    void report_field_boundary_error(
        const std::string         &field_name,
        size_t                     field_msb,
        size_t                     reg_width,
        antlr4::ParserRuleContext *ctx = nullptr);

    // Instance address validation methods
    void validate_instance_addresses(ElaboratedNode *parent);
    void check_instance_address_overlaps(ElaboratedNode *parent);
    bool instances_overlap(const ElaboratedNode *instance1, const ElaboratedNode *instance2);
    void report_instance_overlap_error(
        const std::string         &instance1_name,
        const std::string         &instance2_name,
        Address                    addr1_start,
        Address                    addr1_end,
        Address                    addr2_start,
        Address                    addr2_end,
        antlr4::ParserRuleContext *ctx = nullptr);

    // Parameter handling methods
    std::vector<ParameterDefinition> parse_parameter_definitions(
        SystemRDLParser::Param_defContext *param_def_ctx);

    std::vector<ParameterAssignment> parse_parameter_assignments(
        SystemRDLParser::Param_instContext *param_inst_ctx);

    void apply_parameter_assignments(
        const std::vector<ParameterDefinition> &param_defs,
        const std::vector<ParameterAssignment> &param_assignments);

    void clear_parameter_context();

    PropertyValue resolve_parameter_reference(const std::string &param_name);

    PropertyValue evaluate_expression_from_string(const std::string &expr_text);

    // Enum and struct handling methods
    void collect_enum_and_struct_definitions(SystemRDLParser::RootContext *ast_root);

    void collect_enum_and_struct_definitions_from_body(
        SystemRDLParser::Component_bodyContext *body_ctx);

    void register_enum_definition(SystemRDLParser::Enum_defContext *enum_def);

    void register_struct_definition(SystemRDLParser::Struct_defContext *struct_def);

    EnumDefinition   *find_enum_definition(const std::string &name);
    StructDefinition *find_struct_definition(const std::string &name);

    std::unique_ptr<ElaboratedNode> create_elaborated_node(const std::string &type);

    std::string get_component_type(SystemRDLParser::Component_typeContext *type_ctx);

    Address evaluate_address_expression(SystemRDLParser::ExprContext *expr_ctx);

    size_t evaluate_integer_expression(SystemRDLParser::ExprContext *expr_ctx);

    void calculate_node_size(ElaboratedNode *node, const ElaboratedNode *parent);

    // Register reset value calculation methods
    void calculate_register_reset_value(ElaboratedReg *reg_node);
    void validate_register_reset_value(ElaboratedReg *reg_node);

    // Error reporting
    void report_error(const std::string &message, antlr4::ParserRuleContext *ctx = nullptr);
};

// Utility class: elaborated model traverser
class ElaboratedModelTraverser : public ElaboratedNodeVisitor
{
public:
    virtual void traverse(ElaboratedNode &root);

protected:
    virtual void pre_visit(ElaboratedNode &node) {}
    virtual void post_visit(ElaboratedNode &node) {}

    void visit(ElaboratedAddrmap &node) override;
    void visit(ElaboratedRegfile &node) override;
    void visit(ElaboratedReg &node) override;
    void visit(ElaboratedField &node) override;
    void visit(ElaboratedMem &node) override;
};

// Utility class: address map generator
class AddressMapGenerator : public ElaboratedModelTraverser
{
public:
    struct AddressEntry
    {
        Address     address;
        Size        size;
        std::string name;
        std::string path;
        std::string type;
    };

    std::vector<AddressEntry> generate_address_map(ElaboratedAddrmap &root);

protected:
    void visit(ElaboratedRegfile &node) override;
    void visit(ElaboratedReg &node) override;
    void visit(ElaboratedMem &node) override;

private:
    std::vector<AddressEntry> address_map_;
};

} // namespace systemrdl

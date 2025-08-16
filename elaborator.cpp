#include "elaborator.h"
#include <algorithm>
#include <climits>
#include <map>
#include <set>
#include <sstream>

namespace systemrdl {

// ElaboratedNode implementation
std::string ElaboratedNode::get_hierarchical_path() const
{
    if (parent == nullptr) {
        return inst_name;
    }
    return parent->get_hierarchical_path() + "." + inst_name;
}

void ElaboratedNode::add_child(std::unique_ptr<ElaboratedNode> child)
{
    child->parent = this;
    children.push_back(std::move(child));
}

PropertyValue *ElaboratedNode::get_property(const std::string &name)
{
    auto it = properties.find(name);
    return (it != properties.end()) ? &it->second : nullptr;
}

void ElaboratedNode::set_property(const std::string &name, const PropertyValue &value)
{
    properties[name] = value;
}

// ElaboratedAddrmap implementation
void ElaboratedAddrmap::accept_visitor(ElaboratedNodeVisitor &visitor)
{
    visitor.visit(*this);
}

ElaboratedNode *ElaboratedAddrmap::find_child_by_name(const std::string &name) const
{
    for (const auto &child : children) {
        if (child->inst_name == name) {
            return child.get();
        }
    }
    return nullptr;
}

ElaboratedNode *ElaboratedAddrmap::find_child_by_address(Address addr) const
{
    for (const auto &child : children) {
        if (child->absolute_address <= addr && addr < child->absolute_address + child->size) {
            return child.get();
        }
    }
    return nullptr;
}

// ElaboratedRegfile implementation
void ElaboratedRegfile::accept_visitor(ElaboratedNodeVisitor &visitor)
{
    visitor.visit(*this);
}

ElaboratedNode *ElaboratedRegfile::find_child_by_name(const std::string &name) const
{
    for (const auto &child : children) {
        if (child->inst_name == name) {
            return child.get();
        }
    }
    return nullptr;
}

ElaboratedNode *ElaboratedRegfile::find_child_by_address(Address addr) const
{
    for (const auto &child : children) {
        if (child->absolute_address <= addr && addr < child->absolute_address + child->size) {
            return child.get();
        }
    }
    return nullptr;
}

// ElaboratedReg implementation
void ElaboratedReg::accept_visitor(ElaboratedNodeVisitor &visitor)
{
    visitor.visit(*this);
}

ElaboratedField *ElaboratedReg::find_field_by_name(const std::string &name) const
{
    for (const auto &child : children) {
        if (auto field = dynamic_cast<ElaboratedField *>(child.get())) {
            if (field->inst_name == name) {
                return field;
            }
        }
    }
    return nullptr;
}

ElaboratedField *ElaboratedReg::find_field_by_bit_range(size_t msb, size_t lsb) const
{
    for (const auto &child : children) {
        if (auto field = dynamic_cast<ElaboratedField *>(child.get())) {
            if (field->msb == msb && field->lsb == lsb) {
                return field;
            }
        }
    }
    return nullptr;
}

// ElaboratedField implementation
void ElaboratedField::accept_visitor(ElaboratedNodeVisitor &visitor)
{
    visitor.visit(*this);
}

// ElaboratedMem implementation
void ElaboratedMem::accept_visitor(ElaboratedNodeVisitor &visitor)
{
    visitor.visit(*this);
}

ElaboratedNode *ElaboratedMem::find_child_by_name(const std::string &name) const
{
    for (const auto &child : children) {
        if (child->inst_name == name) {
            return child.get();
        }
    }
    return nullptr;
}

ElaboratedNode *ElaboratedMem::find_child_by_address(Address addr) const
{
    for (const auto &child : children) {
        if (child->absolute_address <= addr && addr < child->absolute_address + child->size) {
            return child.get();
        }
    }
    return nullptr;
}

// SystemRDLElaborator implementation
SystemRDLElaborator::SystemRDLElaborator()  = default;
SystemRDLElaborator::~SystemRDLElaborator() = default;

std::unique_ptr<ElaboratedAddrmap> SystemRDLElaborator::elaborate(
    SystemRDLParser::RootContext *ast_root)
{
    errors_.clear();
    component_definitions_.clear();
    enum_definitions_.clear();
    struct_definitions_.clear();

    if (!ast_root) {
        report_error("AST root is null");
        return nullptr;
    }

    // First pass: collect enum and struct definitions
    collect_enum_and_struct_definitions(ast_root);

    // Second pass: collect all named component definitions (recursive)
    collect_component_definitions(ast_root);

    // Third pass: find top-level addrmap definition and elaborate
    for (auto root_elem : ast_root->root_elem()) {
        if (auto comp_def = root_elem->component_def()) {
            if (auto named_def = comp_def->component_named_def()) {
                if (auto addrmap_def = named_def->component_type()->component_type_primary()) {
                    if (addrmap_def->getText() == "addrmap") {
                        // Found addrmap definition, start elaboration
                        auto elaborated              = std::make_unique<ElaboratedAddrmap>();
                        elaborated->inst_name        = named_def->ID()->getText();
                        elaborated->type_name        = "addrmap";
                        elaborated->absolute_address = 0;

                        // Process addrmap content
                        if (auto body = named_def->component_body()) {
                            elaborate_component_body(body, elaborated.get());
                        }

                        // Validate instance addresses after elaboration is complete
                        validate_instance_addresses(elaborated.get());

                        return elaborated;
                    }
                }
            }
        }
    }

    report_error("No top-level addrmap found");
    return nullptr;
}

void SystemRDLElaborator::elaborate_component_body(
    SystemRDLParser::Component_bodyContext *body_ctx, ElaboratedNode *parent)
{
    Address current_address = 0;

    for (auto body_elem : body_ctx->component_body_elem()) {
        if (auto comp_def = body_elem->component_def()) {
            // Process component definitions and instantiation
            elaborate_component_definition(comp_def, parent, current_address);
        } else if (auto explicit_inst = body_elem->explicit_component_inst()) {
            // Process explicit component instantiation (named component instantiation)
            elaborate_explicit_component_inst(explicit_inst, parent, current_address);
        } else if (auto local_prop = body_elem->local_property_assignment()) {
            // Process local property assignment
            elaborate_local_property_assignment(local_prop, parent);
        } else if (auto dynamic_prop = body_elem->dynamic_property_assignment()) {
            // Process dynamic property assignment
            elaborate_dynamic_property_assignment(dynamic_prop, parent);
        }
    }
}

void SystemRDLElaborator::elaborate_component_definition(
    SystemRDLParser::Component_defContext *comp_def,
    ElaboratedNode                        *parent,
    Address                               &current_address)
{
    if (auto anon_def = comp_def->component_anon_def()) {
        // Anonymous definition + instantiation
        std::string comp_type = get_component_type(anon_def->component_type());

        if (auto insts = comp_def->component_insts()) {
            for (auto inst : insts->component_inst()) {
                elaborate_component_instance(anon_def, inst, parent, current_address, comp_type);
            }
        }
    }
}

void SystemRDLElaborator::elaborate_component_instance(
    SystemRDLParser::Component_anon_defContext *def_ctx,
    SystemRDLParser::Component_instContext     *inst_ctx,
    ElaboratedNode                             *parent,
    Address                                    &current_address,
    const std::string                          &comp_type)
{
    std::string inst_name = inst_ctx->ID()->getText();

    // Check if it's an array
    auto array_suffixes = inst_ctx->array_suffix();
    if (!array_suffixes.empty()) {
        elaborate_array_instance(def_ctx, inst_ctx, parent, current_address, comp_type);
    } else {
        // Single instance
        auto node = create_elaborated_node(comp_type);
        if (!node)
            return;

        node->inst_name  = inst_name;
        node->type_name  = comp_type;
        node->source_ctx = inst_ctx; // Save source context for error reporting

        // Calculate address
        Address instance_address = current_address;
        if (auto fixed_addr = inst_ctx->inst_addr_fixed()) {
            instance_address = evaluate_address_expression(fixed_addr->expr());
        }

        node->absolute_address = parent->absolute_address + instance_address;

        // Process field bit range
        if (comp_type == "field") {
            if (auto field_node = dynamic_cast<ElaboratedField *>(node.get())) {
                elaborate_field_bit_range(inst_ctx, field_node);
            }
        }

        // Process component body
        if (auto body = def_ctx->component_body()) {
            elaborate_component_body(body, node.get());
        }

        // Calculate size
        calculate_node_size(node.get());

        // Save size, because node is about to be moved
        Size node_size = node->size;

        parent->add_child(std::move(node));
        current_address = instance_address + node_size;
    }
}

void SystemRDLElaborator::elaborate_array_instance(
    SystemRDLParser::Component_anon_defContext *def_ctx,
    SystemRDLParser::Component_instContext     *inst_ctx,
    ElaboratedNode                             *parent,
    Address                                    &current_address,
    const std::string                          &comp_type)
{
    std::string base_name = inst_ctx->ID()->getText();

    // Parse array dimensions
    auto                array_suffixes = inst_ctx->array_suffix();
    std::vector<size_t> dimensions;

    if (!array_suffixes.empty()) {
        auto array_suffix = array_suffixes[0]; // Take the first array suffix
        if (auto expr = array_suffix->expr()) {
            // Get array size from expression
            size_t dim = evaluate_integer_expression(expr);
            dimensions.push_back(dim > 0 ? dim : 4); // Default to 4 if parsing fails
        } else {
            dimensions.push_back(4); // Default size
        }
    }

    // Calculate base address
    Address base_address = current_address;
    if (auto fixed_addr = inst_ctx->inst_addr_fixed()) {
        base_address = evaluate_address_expression(fixed_addr->expr());
    }

    // Calculate stride
    Address stride = 4; // Default 4-byte alignment
    if (auto stride_addr = inst_ctx->inst_addr_stride()) {
        stride = evaluate_address_expression(stride_addr->expr());
    }

    // Generate array instances
    for (size_t i = 0; i < dimensions[0]; ++i) {
        auto node = create_elaborated_node(comp_type);
        if (!node)
            continue;

        node->inst_name        = base_name + "[" + std::to_string(i) + "]";
        node->type_name        = comp_type;
        node->source_ctx       = inst_ctx; // Save source context for error reporting
        node->absolute_address = parent->absolute_address + base_address + i * stride;
        node->array_dimensions = dimensions;
        node->array_indices    = {i};

        // Process field bit range for field components
        if (comp_type == "field") {
            if (auto field_node = dynamic_cast<ElaboratedField *>(node.get())) {
                elaborate_field_bit_range(inst_ctx, field_node);
            }
        }

        // Process component body
        if (auto body = def_ctx->component_body()) {
            elaborate_component_body(body, node.get());
        }

        calculate_node_size(node.get());
        parent->add_child(std::move(node));
    }

    current_address = base_address + dimensions[0] * stride;
}

std::unique_ptr<ElaboratedNode> SystemRDLElaborator::create_elaborated_node(const std::string &type)
{
    if (type == "addrmap") {
        return std::make_unique<ElaboratedAddrmap>();
    } else if (type == "regfile") {
        return std::make_unique<ElaboratedRegfile>();
    } else if (type == "reg") {
        return std::make_unique<ElaboratedReg>();
    } else if (type == "field") {
        return std::make_unique<ElaboratedField>();
    } else if (type == "mem") {
        return std::make_unique<ElaboratedMem>();
    }

    report_error("Unknown component type: " + type);
    return nullptr;
}

std::string SystemRDLElaborator::get_component_type(SystemRDLParser::Component_typeContext *type_ctx)
{
    if (auto primary = type_ctx->component_type_primary()) {
        return primary->getText();
    }
    return "unknown";
}

Address SystemRDLElaborator::evaluate_address_expression(SystemRDLParser::ExprContext *expr_ctx)
{
    // Use enhanced expression evaluator
    auto result = evaluate_expression(expr_ctx);
    if (result.type == PropertyValue::INTEGER) {
        return static_cast<Address>(result.int_val);
    }

    // If unable to evaluate, try parsing as a number
    std::string text = expr_ctx->getText();
    if (!text.empty()) {
        try {
            Address addr_result = 0;
            if (text.substr(0, 2) == "0x" || text.substr(0, 2) == "0X") {
                addr_result = std::stoull(text, nullptr, 16);
            } else {
                addr_result = std::stoull(text, nullptr, 10);
            }
            return addr_result;
        } catch (...) {
            // Parsing failed, return 0
        }
    }

    return 0;
}

size_t SystemRDLElaborator::evaluate_integer_expression(SystemRDLParser::ExprContext *expr_ctx)
{
    return static_cast<size_t>(evaluate_integer_expression_enhanced(expr_ctx));
}

void SystemRDLElaborator::calculate_node_size(ElaboratedNode *node)
{
    if (!node)
        return;

    if (auto reg_node = dynamic_cast<ElaboratedReg *>(node)) {
        // Assign automatic positions to fields that need them
        assign_automatic_field_positions(reg_node);
        // Validate register fields first
        validate_register_fields(reg_node);
        // Detect and fill register gaps before calculating size
        detect_and_fill_register_gaps(reg_node);
        reg_node->size = (reg_node->register_width + 7) / 8; // Byte count (round up)
        // Calculate register reset value after all fields are processed
        calculate_register_reset_value(reg_node);
        // Validate register reset value consistency
        validate_register_reset_value(reg_node);
    } else if (auto field_node = dynamic_cast<ElaboratedField *>(node)) {
        field_node->size = 0; // Field does not occupy independent address space
    } else if (auto regfile_node = dynamic_cast<ElaboratedRegfile *>(node)) {
        // regfile size is the address range of all its children
        Address max_addr = 0;
        for (const auto &child : regfile_node->children) {
            Address child_end = child->absolute_address + child->size;
            if (child_end > max_addr) {
                max_addr = child_end;
            }
        }
        regfile_node->size = max_addr - regfile_node->absolute_address;
        if (regfile_node->size == 0) {
            regfile_node->size = 4; // Minimum size
        }
    } else if (auto mem_node = dynamic_cast<ElaboratedMem *>(node)) {
        // Memory size can be obtained from parameter or attribute
        // First, try MEM_SIZE parameter
        auto mem_size_param = resolve_parameter_reference("MEM_SIZE");
        if (mem_size_param.type == PropertyValue::INTEGER && mem_size_param.int_val > 0) {
            mem_node->size        = static_cast<Size>(mem_size_param.int_val);
            mem_node->memory_size = static_cast<Size>(mem_size_param.int_val);
        } else {
            // Try SIZE parameter
            auto size_param = resolve_parameter_reference("SIZE");
            if (size_param.type == PropertyValue::INTEGER && size_param.int_val > 0) {
                mem_node->size        = static_cast<Size>(size_param.int_val);
                mem_node->memory_size = static_cast<Size>(size_param.int_val);
            } else if (mem_node->memory_size > 0) {
                mem_node->size = mem_node->memory_size;
            } else {
                // Default memory size: 4KB
                mem_node->size        = 4096;
                mem_node->memory_size = 4096;
            }
        }

        // Set memory type parameter
        auto type_param = resolve_parameter_reference("TYPE");
        if (type_param.type == PropertyValue::STRING) {
            mem_node->memory_type = type_param.string_val;
        }

        // Set alignment parameter
        auto align_param = resolve_parameter_reference("ALIGN");
        if (align_param.type == PropertyValue::INTEGER && align_param.int_val > 0) {
            // Can handle alignment requirements here, temporarily store as attribute
            mem_node->set_property("alignment", align_param);
        }

        // Set KB_SIZE parameter (if exists)
        auto kb_size_param = resolve_parameter_reference("KB_SIZE");
        if (kb_size_param.type == PropertyValue::INTEGER) {
            mem_node->set_property("kb_size", kb_size_param);
        }
    } else {
        // For addrmap, simplified calculation: default size
        node->size = 4; // Default 4 bytes
    }
}

// Helper function to convert uint64_t to binary string
std::string SystemRDLElaborator::uint64_to_binary_string(uint64_t value, size_t width)
{
    std::string binary(width, '0');
    for (size_t i = 0; i < width && i < 64; ++i) {
        if (value & (1ULL << i)) {
            binary[width - 1 - i] = '1';
        }
    }
    return binary;
}

// Helper function to convert binary string to hexadecimal
std::string SystemRDLElaborator::binary_string_to_hex(const std::string &binary)
{
    std::string hex_result;

    // Process 4 bits at a time (1 hex digit = 4 binary bits)
    for (size_t i = 0; i < binary.length(); i += 4) {
        // Extract 4-bit nibble, pad with leading zeros if needed
        std::string nibble = binary.substr(i, 4);
        while (nibble.length() < 4) {
            nibble = "0" + nibble;
        }

        // Convert 4-bit binary to decimal value
        int val = 0;
        for (char c : nibble) {
            val = (val << 1) + (c - '0');
        }

        // Convert to hex character (lowercase)
        char hex_char = (val < 10) ? ('0' + val) : ('a' + val - 10);
        hex_result += hex_char;
    }

    return hex_result;
}

// Calculate register reset value using pure string operations
void SystemRDLElaborator::calculate_register_reset_value(ElaboratedReg *reg_node)
{
    if (!reg_node) {
        return;
    }

    // Initialize binary string with all zeros
    std::string bits(reg_node->register_width, '0');

    // Set bits for each field
    for (const auto &child : reg_node->children) {
        if (auto field = dynamic_cast<ElaboratedField *>(child.get())) {
            // Skip fields with invalid bit positions
            if (field->lsb >= reg_node->register_width || field->msb >= reg_node->register_width) {
                continue;
            }

            // Convert field reset value to binary string
            size_t      field_width  = field->msb - field->lsb + 1;
            std::string field_binary = uint64_to_binary_string(field->reset_value, field_width);

            // Set bits from LSB position
            for (size_t i = 0; i < field_width && (field->lsb + i) < reg_node->register_width; ++i) {
                size_t bit_pos       = reg_node->register_width - 1 - (field->lsb + i);
                size_t field_bit_pos = field_width - 1 - i;
                bits[bit_pos]        = field_binary[field_bit_pos];
            }
        }
    }

    // Convert binary string to hexadecimal and store
    std::string hex_str          = binary_string_to_hex(bits);
    reg_node->register_reset_hex = "0x" + hex_str;
}

// Validate register reset value consistency and bounds
void SystemRDLElaborator::validate_register_reset_value(ElaboratedReg *reg_node)
{
    if (!reg_node) {
        return;
    }

    // Check if any field reset values exceed their bit width
    for (const auto &child : reg_node->children) {
        if (auto field = dynamic_cast<ElaboratedField *>(child.get())) {
            // Skip reserved fields (auto-generated)
            auto reserved_prop = field->get_property("reserved");
            if (reserved_prop && reserved_prop->type == PropertyValue::BOOLEAN
                && reserved_prop->bool_val) {
                continue;
            }

            // Calculate maximum value for field width
            size_t field_width = field->msb - field->lsb + 1;
            if (field_width < 64) { // Avoid overflow for very large fields
                uint64_t max_field_value = (1ULL << field_width) - 1;
                if (field->reset_value > max_field_value) {
                    report_error(
                        "Field '" + field->inst_name + "' reset value "
                            + std::to_string(field->reset_value) + " exceeds maximum value "
                            + std::to_string(max_field_value) + " for "
                            + std::to_string(field_width) + "-bit field",
                        field->source_ctx);
                }
            }
        }
    }
}

void SystemRDLElaborator::report_error(const std::string &message, antlr4::ParserRuleContext *ctx)
{
    ElaborationError error;
    error.message = message;
    if (ctx) {
        error.line   = ctx->getStart()->getLine();
        error.column = ctx->getStart()->getCharPositionInLine();
    }
    errors_.push_back(error);
}

// ElaboratedModelTraverser implementation
void ElaboratedModelTraverser::traverse(ElaboratedNode &root)
{
    pre_visit(root);
    root.accept_visitor(*this);
    post_visit(root);
}

void ElaboratedModelTraverser::visit(ElaboratedAddrmap &node)
{
    for (auto &child : node.children) {
        traverse(*child);
    }
}

void ElaboratedModelTraverser::visit(ElaboratedRegfile &node)
{
    for (auto &child : node.children) {
        traverse(*child);
    }
}

void ElaboratedModelTraverser::visit(ElaboratedReg &node)
{
    for (auto &child : node.children) {
        traverse(*child);
    }
}

void ElaboratedModelTraverser::visit(ElaboratedField &node)
{
    // Field usually has no children
}

void ElaboratedModelTraverser::visit(ElaboratedMem &node)
{
    for (auto &child : node.children) {
        traverse(*child);
    }
}

// AddressMapGenerator implementation
std::vector<AddressMapGenerator::AddressEntry> AddressMapGenerator::generate_address_map(
    ElaboratedAddrmap &root)
{
    address_map_.clear();
    traverse(root);

    // Sort by address
    std::sort(
        address_map_.begin(), address_map_.end(), [](const AddressEntry &a, const AddressEntry &b) {
            return a.address < b.address;
        });

    return address_map_;
}

void AddressMapGenerator::visit(ElaboratedRegfile &node)
{
    AddressEntry entry;
    entry.address = node.absolute_address;
    entry.size    = node.size;
    entry.name    = node.inst_name;
    entry.path    = node.get_hierarchical_path();
    entry.type    = node.get_node_type();

    address_map_.push_back(entry);

    // Continue traversing child nodes
    ElaboratedModelTraverser::visit(node);
}

void AddressMapGenerator::visit(ElaboratedReg &node)
{
    AddressEntry entry;
    entry.address = node.absolute_address;
    entry.size    = node.size;
    entry.name    = node.inst_name;
    entry.path    = node.get_hierarchical_path();
    entry.type    = node.get_node_type();

    address_map_.push_back(entry);

    // Continue traversing child nodes
    ElaboratedModelTraverser::visit(node);
}

void AddressMapGenerator::visit(ElaboratedMem &node)
{
    AddressEntry entry;
    entry.address = node.absolute_address;
    entry.size    = node.size;
    entry.name    = node.inst_name;
    entry.path    = node.get_hierarchical_path();
    entry.type    = node.get_node_type();

    address_map_.push_back(entry);

    // Continue traversing child nodes
    ElaboratedModelTraverser::visit(node);
}

// New method implementation
void SystemRDLElaborator::collect_component_definitions(SystemRDLParser::RootContext *ast_root)
{
    // Collect top-level definitions
    for (auto root_elem : ast_root->root_elem()) {
        if (auto comp_def = root_elem->component_def()) {
            if (auto named_def = comp_def->component_named_def()) {
                register_component_definition(named_def);

                // Recursively collect internal definitions
                if (auto body = named_def->component_body()) {
                    collect_component_definitions_from_body(body);
                }
            }
        }
    }
}

void SystemRDLElaborator::collect_component_definitions_from_body(
    SystemRDLParser::Component_bodyContext *body_ctx)
{
    for (auto body_elem : body_ctx->component_body_elem()) {
        if (auto comp_def = body_elem->component_def()) {
            if (auto named_def = comp_def->component_named_def()) {
                register_component_definition(named_def);

                // Recursively collect internal definitions
                if (auto body = named_def->component_body()) {
                    collect_component_definitions_from_body(body);
                }
            }
        }
    }
}

void SystemRDLElaborator::register_component_definition(
    SystemRDLParser::Component_named_defContext *named_def)
{
    std::string comp_name = named_def->ID()->getText();
    std::string comp_type = get_component_type(named_def->component_type());

    ComponentDefinition def;
    def.name    = comp_name;
    def.type    = comp_type;
    def.def_ctx = named_def;

    // Parse parameter definitions
    if (auto param_def = named_def->param_def()) {
        def.parameters = parse_parameter_definitions(param_def);
    }

    component_definitions_[comp_name] = def;
}

void SystemRDLElaborator::elaborate_explicit_component_inst(
    SystemRDLParser::Explicit_component_instContext *explicit_inst,
    ElaboratedNode                                  *parent,
    Address                                         &current_address)
{
    std::string type_name = explicit_inst->ID()->getText();

    // Find named component definition
    auto it = component_definitions_.find(type_name);
    if (it == component_definitions_.end()) {
        report_error("Undefined component type: " + type_name, explicit_inst);
        return;
    }

    const ComponentDefinition &comp_def = it->second;

    // Process parameter instantiation
    std::vector<ParameterAssignment> param_assignments;
    if (auto param_inst = explicit_inst->component_insts()->param_inst()) {
        param_assignments = parse_parameter_assignments(param_inst);
    }

    // Apply parameter values
    apply_parameter_assignments(comp_def.parameters, param_assignments);

    if (auto insts = explicit_inst->component_insts()) {
        for (auto inst : insts->component_inst()) {
            elaborate_named_component_instance(type_name, inst, parent, current_address);
        }
    }

    // Clear parameter context
    clear_parameter_context();
}

void SystemRDLElaborator::elaborate_named_component_instance(
    const std::string                      &type_name,
    SystemRDLParser::Component_instContext *inst_ctx,
    ElaboratedNode                         *parent,
    Address                                &current_address)
{
    // Find component definition
    auto it = component_definitions_.find(type_name);
    if (it == component_definitions_.end()) {
        report_error("Undefined component type: " + type_name, inst_ctx);
        return;
    }

    const ComponentDefinition &comp_def  = it->second;
    std::string                inst_name = inst_ctx->ID()->getText();

    // Check if it's an array
    auto array_suffixes = inst_ctx->array_suffix();
    if (!array_suffixes.empty()) {
        elaborate_named_array_instance(type_name, inst_ctx, parent, current_address);
    } else {
        // Single instance
        auto node = create_elaborated_node(comp_def.type);
        if (!node)
            return;

        node->inst_name  = inst_name;
        node->type_name  = comp_def.type;
        node->source_ctx = inst_ctx; // Save source context for error reporting

        // Calculate address
        Address instance_address = current_address;
        if (auto fixed_addr = inst_ctx->inst_addr_fixed()) {
            instance_address = evaluate_address_expression(fixed_addr->expr());
        }

        node->absolute_address = parent->absolute_address + instance_address;

        // Process component body (from named definition)
        if (auto body = comp_def.def_ctx->component_body()) {
            elaborate_component_body(body, node.get());
        }

        // Calculate size
        calculate_node_size(node.get());

        // Save size, because node is about to be moved
        Size node_size = node->size;

        parent->add_child(std::move(node));
        current_address = instance_address + node_size;
    }
}

void SystemRDLElaborator::elaborate_named_array_instance(
    const std::string                      &type_name,
    SystemRDLParser::Component_instContext *inst_ctx,
    ElaboratedNode                         *parent,
    Address                                &current_address)
{
    // Find component definition
    auto it = component_definitions_.find(type_name);
    if (it == component_definitions_.end()) {
        report_error("Undefined component type: " + type_name, inst_ctx);
        return;
    }

    const ComponentDefinition &comp_def  = it->second;
    std::string                base_name = inst_ctx->ID()->getText();

    // Parse array dimensions
    auto                array_suffixes = inst_ctx->array_suffix();
    std::vector<size_t> dimensions;

    if (!array_suffixes.empty()) {
        auto array_suffix = array_suffixes[0]; // Take the first array suffix
        if (auto expr = array_suffix->expr()) {
            // Get array size from expression
            size_t dim = evaluate_integer_expression(expr);
            dimensions.push_back(dim > 0 ? dim : 4); // Default to 4 if parsing fails
        } else {
            dimensions.push_back(4); // Default size
        }
    }

    // Calculate base address
    Address base_address = current_address;
    if (auto fixed_addr = inst_ctx->inst_addr_fixed()) {
        base_address = evaluate_address_expression(fixed_addr->expr());
    }

    // Calculate stride
    Address stride = 4; // Default 4-byte alignment
    if (auto stride_addr = inst_ctx->inst_addr_stride()) {
        stride = evaluate_address_expression(stride_addr->expr());
    }

    // Generate array instances
    for (size_t i = 0; i < dimensions[0]; ++i) {
        auto node = create_elaborated_node(comp_def.type);
        if (!node)
            continue;

        node->inst_name        = base_name + "[" + std::to_string(i) + "]";
        node->type_name        = comp_def.type;
        node->source_ctx       = inst_ctx; // Save source context for error reporting
        node->absolute_address = parent->absolute_address + base_address + i * stride;
        node->array_dimensions = dimensions;
        node->array_indices    = {i};

        // Process component body (from named definition)
        if (auto body = comp_def.def_ctx->component_body()) {
            elaborate_component_body(body, node.get());
        }

        calculate_node_size(node.get());
        parent->add_child(std::move(node));
    }

    current_address = base_address + dimensions[0] * stride;
}

// Property processing method implementation
void SystemRDLElaborator::elaborate_local_property_assignment(
    SystemRDLParser::Local_property_assignmentContext *local_prop, ElaboratedNode *parent)
{
    if (auto normal_prop = local_prop->normal_prop_assign()) {
        std::string prop_name;

        // Get property name
        if (auto prop_keyword = normal_prop->prop_keyword()) {
            prop_name = prop_keyword->getText();
        } else if (auto id = normal_prop->ID()) {
            prop_name = id->getText();
        }

        // Get property value
        if (auto rhs = normal_prop->prop_assignment_rhs()) {
            PropertyValue value = evaluate_property_value(rhs);
            parent->set_property(prop_name, value);

            // Special handling for regwidth property
            if (prop_name == "regwidth" && value.type == PropertyValue::INTEGER) {
                if (auto reg_node = dynamic_cast<ElaboratedReg *>(parent)) {
                    reg_node->register_width = static_cast<uint32_t>(value.int_val);
                }
            }
            // Special handling for encode attribute
            else if (prop_name == "encode" && value.type == PropertyValue::STRING) {
                // Check if it's an enum type
                auto enum_def = find_enum_definition(value.string_val);
                if (enum_def) {
                    // Store enum information
                    parent->set_property("encode_type", PropertyValue(std::string("enum")));
                    parent->set_property("encode_name", value);

                    // Store enum value mapping
                    std::string enum_values = "";
                    for (const auto &entry : enum_def->entries) {
                        if (!enum_values.empty())
                            enum_values += ",";
                        enum_values += entry.name + "=" + std::to_string(entry.value);
                    }
                    parent->set_property("encode_values", PropertyValue(enum_values));
                }
            }
        } else {
            // No value assigned, set to true (for boolean attributes)
            parent->set_property(prop_name, PropertyValue(true));
        }
    } else if (auto encode_prop = local_prop->encode_prop_assign()) {
        // Process encode attribute assignment
        std::string enum_name;
        if (auto id = encode_prop->ID()) {
            // Get enum name
            enum_name = id->getText();
        }

        // Set encode attribute
        parent->set_property("encode", PropertyValue(enum_name));

        // Find enum definition
        auto enum_def = find_enum_definition(enum_name);
        if (enum_def) {
            // Store enum information
            parent->set_property("encode_type", PropertyValue(std::string("enum")));
            parent->set_property("encode_name", PropertyValue(enum_name));

            // Store enum value mapping
            std::string enum_values = "";
            for (const auto &entry : enum_def->entries) {
                if (!enum_values.empty())
                    enum_values += ",";
                enum_values += entry.name + "=" + std::to_string(entry.value);
            }
            parent->set_property("encode_values", PropertyValue(enum_values));
        }
    }
    // TODO: Handle prop_mod_assign
}

void SystemRDLElaborator::elaborate_dynamic_property_assignment(
    SystemRDLParser::Dynamic_property_assignmentContext *dynamic_prop, ElaboratedNode *parent)
{
    // Dynamic property assignment: instance_ref -> property = value
    // This requires finding the target instance, temporarily skipping complex implementation
    // TODO: Implement dynamic property assignment
}

PropertyValue SystemRDLElaborator::evaluate_property_value(
    SystemRDLParser::Prop_assignment_rhsContext *rhs_ctx)
{
    if (auto precedence = rhs_ctx->precedencetype_literal()) {
        return PropertyValue(precedence->getText());
    } else if (auto expr = rhs_ctx->expr()) {
        return evaluate_property_value(expr);
    }

    return PropertyValue(std::string(""));
}

PropertyValue SystemRDLElaborator::evaluate_property_value(SystemRDLParser::ExprContext *expr_ctx)
{
    // Use enhanced expression evaluator
    return evaluate_expression(expr_ctx);
}

// Enhanced expression evaluator implementation
PropertyValue SystemRDLElaborator::evaluate_expression(SystemRDLParser::ExprContext *expr_ctx)
{
    if (!expr_ctx) {
        return PropertyValue(std::string(""));
    }

    // Process unary expression
    if (auto unary_ctx = dynamic_cast<SystemRDLParser::UnaryExprContext *>(expr_ctx)) {
        auto        operand = evaluate_expression_primary(unary_ctx->expr_primary());
        std::string op      = unary_ctx->op->getText();

        if (operand.type == PropertyValue::INTEGER) {
            int64_t val = operand.int_val;
            if (op == "+")
                return PropertyValue(val);
            else if (op == "-")
                return PropertyValue(-val);
            else if (op == "~")
                return PropertyValue(~val);
            else if (op == "!")
                return PropertyValue(static_cast<int64_t>(val == 0 ? 1 : 0));
        }
        return PropertyValue(expr_ctx->getText());
    }

    // Process binary expression
    if (auto binary_ctx = dynamic_cast<SystemRDLParser::BinaryExprContext *>(expr_ctx)) {
        auto        left  = evaluate_expression(binary_ctx->expr(0));
        auto        right = evaluate_expression(binary_ctx->expr(1));
        std::string op    = binary_ctx->op->getText();

        // If both operands are integers, perform numerical calculation
        if (left.type == PropertyValue::INTEGER && right.type == PropertyValue::INTEGER) {
            int64_t l = left.int_val;
            int64_t r = right.int_val;

            // Arithmetic operations
            if (op == "+")
                return PropertyValue(l + r);
            else if (op == "-")
                return PropertyValue(l - r);
            else if (op == "*")
                return PropertyValue(l * r);
            else if (op == "/")
                return PropertyValue(r != 0 ? l / r : 0);
            else if (op == "%")
                return PropertyValue(r != 0 ? l % r : 0);
            else if (op == "**") {
                // Simple power operation implementation
                int64_t result = 1;
                for (int64_t i = 0; i < r && i < 64; ++i) {
                    result *= l;
                }
                return PropertyValue(result);
            }
            // Bitwise operations
            else if (op == "&")
                return PropertyValue(l & r);
            else if (op == "|")
                return PropertyValue(l | r);
            else if (op == "^")
                return PropertyValue(l ^ r);
            else if (op == "<<")
                return PropertyValue(l << (r & 63)); // Limit shift bit count
            else if (op == ">>")
                return PropertyValue(l >> (r & 63));
            // Comparison operations
            else if (op == "<")
                return PropertyValue(static_cast<int64_t>(l < r ? 1 : 0));
            else if (op == "<=")
                return PropertyValue(static_cast<int64_t>(l <= r ? 1 : 0));
            else if (op == ">")
                return PropertyValue(static_cast<int64_t>(l > r ? 1 : 0));
            else if (op == ">=")
                return PropertyValue(static_cast<int64_t>(l >= r ? 1 : 0));
            else if (op == "==")
                return PropertyValue(static_cast<int64_t>(l == r ? 1 : 0));
            else if (op == "!=")
                return PropertyValue(static_cast<int64_t>(l != r ? 1 : 0));
            // Logical operations
            else if (op == "&&")
                return PropertyValue(static_cast<int64_t>((l != 0 && r != 0) ? 1 : 0));
            else if (op == "||")
                return PropertyValue(static_cast<int64_t>((l != 0 || r != 0) ? 1 : 0));
        }

        // String concatenation
        if (op == "+"
            && (left.type == PropertyValue::STRING || right.type == PropertyValue::STRING)) {
            std::string l_str = (left.type == PropertyValue::STRING) ? left.string_val
                                                                     : std::to_string(left.int_val);
            std::string r_str = (right.type == PropertyValue::STRING)
                                    ? right.string_val
                                    : std::to_string(right.int_val);
            return PropertyValue(l_str + r_str);
        }

        return PropertyValue(expr_ctx->getText());
    }

    // Process ternary expression (condition ? true_val : false_val)
    if (auto ternary_ctx = dynamic_cast<SystemRDLParser::TernaryExprContext *>(expr_ctx)) {
        auto condition = evaluate_expression(ternary_ctx->expr(0));
        bool cond_true = false;

        if (condition.type == PropertyValue::INTEGER) {
            cond_true = (condition.int_val != 0);
        } else if (condition.type == PropertyValue::BOOLEAN) {
            cond_true = condition.bool_val;
        }

        if (cond_true) {
            return evaluate_expression(ternary_ctx->expr(1)); // true branch
        } else {
            return evaluate_expression(ternary_ctx->expr(2)); // false branch
        }
    }

    // Process simple expression (NOPContext)
    if (auto nop_ctx = dynamic_cast<SystemRDLParser::NOPContext *>(expr_ctx)) {
        return evaluate_expression_primary(nop_ctx->expr_primary());
    }

    // If unable to evaluate, return expression text
    return PropertyValue(expr_ctx->getText());
}

// Enhanced integer expression evaluation
int64_t SystemRDLElaborator::evaluate_integer_expression_enhanced(
    SystemRDLParser::ExprContext *expr_ctx)
{
    auto result = evaluate_expression(expr_ctx);
    if (result.type == PropertyValue::INTEGER) {
        return result.int_val;
    }

    // Try parsing string as a number
    if (result.type == PropertyValue::STRING) {
        try {
            std::string str = result.string_val;
            if (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X") {
                return std::stoll(str, nullptr, 16);
            } else {
                return std::stoll(str, nullptr, 10);
            }
        } catch (...) {
            return 0;
        }
    }

    return 0;
}

// Expression primary part evaluation implementation
PropertyValue SystemRDLElaborator::evaluate_expression_primary(
    SystemRDLParser::Expr_primaryContext *primary_ctx)
{
    if (!primary_ctx) {
        return PropertyValue(std::string(""));
    }

    if (auto literal = primary_ctx->literal()) {
        // Process number literal
        if (auto number = literal->number()) {
            std::string num_str = number->getText();
            int64_t     result  = 0;
            if (num_str.substr(0, 2) == "0x" || num_str.substr(0, 2) == "0X") {
                result = std::stoll(num_str, nullptr, 16);
            } else {
                result = std::stoll(num_str, nullptr, 10);
            }
            return PropertyValue(result);
        }
        // Process string literal
        else if (auto string_lit = literal->string_literal()) {
            std::string str = string_lit->getText();
            // Remove quotes
            if (str.length() >= 2 && str[0] == '"' && str.back() == '"') {
                str = str.substr(1, str.length() - 2);
            }
            return PropertyValue(str);
        }
        // Process boolean literal
        else if (auto bool_lit = literal->boolean_literal()) {
            bool value = (bool_lit->getText() == "true");
            return PropertyValue(value);
        }
        // Process access type literal
        else if (auto access_lit = literal->accesstype_literal()) {
            return PropertyValue(access_lit->getText());
        }
        // Process other literal types
        else {
            return PropertyValue(literal->getText());
        }
    }
    // Process parentheses expression
    else if (auto paren = primary_ctx->paren_expr()) {
        return evaluate_expression(paren->expr());
    }
    // Process identifier (possibly parameter reference)
    else if (
        primary_ctx->getText().find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")
        == std::string::npos) {
        // This is an identifier, check if it's a parameter reference
        std::string identifier  = primary_ctx->getText();
        auto        param_value = resolve_parameter_reference(identifier);
        if (param_value.type != PropertyValue::STRING || param_value.string_val != identifier) {
            return param_value; // Found parameter value
        }
    }

    // If unable to evaluate, return expression text
    return PropertyValue(primary_ctx->getText());
}

// Field bit range processing method implementation
void SystemRDLElaborator::elaborate_field_bit_range(
    SystemRDLParser::Component_instContext *inst_ctx, ElaboratedField *field_node)
{
    if (!field_node)
        return;

    // Check if there's bit range definition
    if (auto range_suffix = inst_ctx->range_suffix()) {
        auto exprs = range_suffix->expr();
        if (exprs.size() == 2) {
            // Parse [msb:lsb] format
            size_t msb = evaluate_integer_expression_enhanced(exprs[0]);
            size_t lsb = evaluate_integer_expression_enhanced(exprs[1]);

            field_node->msb   = msb;
            field_node->lsb   = lsb;
            field_node->width = (msb >= lsb) ? (msb - lsb + 1) : 0;

            // Verify the reasonability of the bit range
            if (msb < lsb) {
                report_error(
                    "Invalid bit range: MSB (" + std::to_string(msb) + ") is less than LSB ("
                        + std::to_string(lsb) + ")",
                    inst_ctx);
            }

            // Set bit range attribute
            field_node->set_property("msb", PropertyValue(static_cast<int64_t>(msb)));
            field_node->set_property("lsb", PropertyValue(static_cast<int64_t>(lsb)));
            field_node->set_property("width", PropertyValue(static_cast<int64_t>(field_node->width)));
        }
    } else {
        // No bit range definition - field needs automatic positioning
        size_t field_width = 1; // Default width

        // Check if fieldwidth property is defined
        auto fieldwidth_prop = field_node->get_property("fieldwidth");
        if (fieldwidth_prop && fieldwidth_prop->type == PropertyValue::INTEGER) {
            field_width = static_cast<size_t>(fieldwidth_prop->int_val);
        }

        // Mark this field as needing automatic positioning
        field_node->msb   = SIZE_MAX; // Use SIZE_MAX as marker for auto-positioning
        field_node->lsb   = SIZE_MAX;
        field_node->width = field_width;

        field_node->set_property("msb", PropertyValue(static_cast<int64_t>(SIZE_MAX)));
        field_node->set_property("lsb", PropertyValue(static_cast<int64_t>(SIZE_MAX)));
        field_node->set_property("width", PropertyValue(static_cast<int64_t>(field_width)));
        field_node->set_property("auto_position", PropertyValue(true));
    }

    // Process field reset value if specified (applies to both bit range and auto-positioned fields)
    if (auto field_reset = inst_ctx->field_inst_reset()) {
        if (auto reset_expr = field_reset->expr()) {
            PropertyValue reset_value = evaluate_property_value(reset_expr);

            // Store reset value in both the field member and properties
            if (reset_value.type == PropertyValue::INTEGER) {
                field_node->reset_value = static_cast<uint64_t>(reset_value.int_val);
            } else if (reset_value.type == PropertyValue::STRING) {
                // Try to parse string as integer (for hex values like "0x1A")
                try {
                    field_node->reset_value = std::stoull(reset_value.string_val, nullptr, 0);
                } catch (...) {
                    field_node->reset_value = 0;
                }
            }

            // Always store in properties for JSON export
            field_node->set_property("reset", reset_value);
        }
    }
}

// Parameter processing method implementation
std::vector<ParameterDefinition> SystemRDLElaborator::parse_parameter_definitions(
    SystemRDLParser::Param_defContext *param_def_ctx)
{
    std::vector<ParameterDefinition> parameters;

    for (auto param_elem : param_def_ctx->param_def_elem()) {
        ParameterDefinition param;

        // Get parameter name
        param.name = param_elem->ID()->getText();

        // Get data type
        if (auto data_type = param_elem->data_type()) {
            if (auto basic_type = data_type->basic_data_type()) {
                param.data_type = basic_type->getText();
            } else {
                param.data_type = data_type->getText();
            }
        }

        // Check if it's an array type
        param.is_array = (param_elem->array_type_suffix() != nullptr);

        // Get default value (temporarily store as string, will evaluate later when applying parameters)
        if (auto default_expr = param_elem->expr()) {
            // First try direct evaluation, if failed then store expression text
            auto value = evaluate_expression(default_expr);
            if (value.type != PropertyValue::STRING || value.string_val != default_expr->getText()) {
                param.default_value = value;
            } else {
                // Include parameter reference, store expression text for later evaluation
                param.default_value = PropertyValue(default_expr->getText());
            }
            param.has_default = true;
        }

        parameters.push_back(param);
    }

    return parameters;
}

std::vector<ParameterAssignment> SystemRDLElaborator::parse_parameter_assignments(
    SystemRDLParser::Param_instContext *param_inst_ctx)
{
    std::vector<ParameterAssignment> assignments;

    for (auto param_assign : param_inst_ctx->param_assignment()) {
        ParameterAssignment assignment;

        // Get parameter name
        assignment.name = param_assign->ID()->getText();

        // Get parameter value
        assignment.value = evaluate_expression(param_assign->expr());

        assignments.push_back(assignment);
    }

    return assignments;
}

void SystemRDLElaborator::apply_parameter_assignments(
    const std::vector<ParameterDefinition> &param_defs,
    const std::vector<ParameterAssignment> &param_assignments)
{
    // Clear current parameter context
    current_parameter_values_.clear();

    // First apply default values (multi-round evaluation to handle parameter dependencies)
    std::set<std::string> resolved_params;
    bool                  progress = true;

    while (progress && resolved_params.size() < param_defs.size()) {
        progress = false;

        for (const auto &param_def : param_defs) {
            if (!param_def.has_default || resolved_params.count(param_def.name)) {
                continue; // Skip already resolved parameters
            }

            if (param_def.default_value.type != PropertyValue::STRING) {
                // Direct value, no need to re-evaluate
                current_parameter_values_[param_def.name] = param_def.default_value;
                resolved_params.insert(param_def.name);
                progress = true;
            } else {
                // Try re-evaluating expression
                auto value = evaluate_expression_from_string(param_def.default_value.string_val);
                if (value.type != PropertyValue::STRING
                    || value.string_val != param_def.default_value.string_val) {
                    // Evaluation succeeded
                    current_parameter_values_[param_def.name] = value;
                    resolved_params.insert(param_def.name);
                    progress = true;
                }
            }
        }
    }

    // Process remaining unparsed parameters (may exist circular dependencies)
    for (const auto &param_def : param_defs) {
        if (param_def.has_default && !resolved_params.count(param_def.name)) {
            current_parameter_values_[param_def.name] = param_def.default_value;
        }
    }

    // Then apply instantiation-time assignments
    for (const auto &assignment : param_assignments) {
        // Check if parameter exists
        bool param_exists = false;
        for (const auto &param_def : param_defs) {
            if (param_def.name == assignment.name) {
                param_exists = true;
                break;
            }
        }

        if (param_exists) {
            current_parameter_values_[assignment.name] = assignment.value;
        } else {
            report_error("Unknown parameter: " + assignment.name);
        }
    }

    // Check if all required parameters have values
    for (const auto &param_def : param_defs) {
        if (!param_def.has_default
            && current_parameter_values_.find(param_def.name) == current_parameter_values_.end()) {
            report_error("Missing required parameter: " + param_def.name);
        }
    }
}

void SystemRDLElaborator::clear_parameter_context()
{
    current_parameter_values_.clear();
}

PropertyValue SystemRDLElaborator::resolve_parameter_reference(const std::string &param_name)
{
    auto it = current_parameter_values_.find(param_name);
    if (it != current_parameter_values_.end()) {
        return it->second;
    }

    // If parameter not found, return original string
    return PropertyValue(param_name);
}

PropertyValue SystemRDLElaborator::evaluate_expression_from_string(const std::string &expr_text)
{
    // Enhanced expression evaluator: support more complex expression patterns

    // Check if it's a simple parameter reference
    if (expr_text.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")
        == std::string::npos) {
        return resolve_parameter_reference(expr_text);
    }

    // Process "(1<<BASE_WIDTH)-1" pattern
    if (expr_text.find("(1<<") != std::string::npos && expr_text.find(")-1") != std::string::npos) {
        size_t start = expr_text.find("(1<<") + 4;
        size_t end   = expr_text.find(")-1");
        if (start < end) {
            std::string param_name = expr_text.substr(start, end - start);
            auto        param_val  = resolve_parameter_reference(param_name);
            if (param_val.type == PropertyValue::INTEGER) {
                return PropertyValue(static_cast<int64_t>((1LL << param_val.int_val) - 1));
            }
        }
    }

    // Process "param*number" or "number*param" pattern
    if (expr_text.find('*') != std::string::npos) {
        size_t      pos   = expr_text.find('*');
        std::string left  = expr_text.substr(0, pos);
        std::string right = expr_text.substr(pos + 1);

        // Remove spaces
        left.erase(std::remove(left.begin(), left.end(), ' '), left.end());
        right.erase(std::remove(right.begin(), right.end(), ' '), right.end());

        auto left_val  = resolve_parameter_reference(left);
        auto right_val = resolve_parameter_reference(right);

        // If both are integers, directly multiply
        if (left_val.type == PropertyValue::INTEGER && right_val.type == PropertyValue::INTEGER) {
            return PropertyValue(left_val.int_val * right_val.int_val);
        }

        // If left is parameter, right is number
        if (left_val.type == PropertyValue::INTEGER && right_val.type == PropertyValue::STRING) {
            try {
                int64_t right_num = std::stoll(right);
                return PropertyValue(left_val.int_val * right_num);
            } catch (...) {
            }
        }

        // If right is parameter, left is number
        if (left_val.type == PropertyValue::STRING && right_val.type == PropertyValue::INTEGER) {
            try {
                int64_t left_num = std::stoll(left);
                return PropertyValue(left_num * right_val.int_val);
            } catch (...) {
            }
        }
    }

    // Process "param+number" or "number+param" pattern
    if (expr_text.find('+') != std::string::npos) {
        size_t      pos   = expr_text.find('+');
        std::string left  = expr_text.substr(0, pos);
        std::string right = expr_text.substr(pos + 1);

        // Remove spaces
        left.erase(std::remove(left.begin(), left.end(), ' '), left.end());
        right.erase(std::remove(right.begin(), right.end(), ' '), right.end());

        auto left_val  = resolve_parameter_reference(left);
        auto right_val = resolve_parameter_reference(right);

        // If both are integers, directly add
        if (left_val.type == PropertyValue::INTEGER && right_val.type == PropertyValue::INTEGER) {
            return PropertyValue(left_val.int_val + right_val.int_val);
        }

        // If left is parameter, right is number
        if (left_val.type == PropertyValue::INTEGER && right_val.type == PropertyValue::STRING) {
            try {
                int64_t right_num = std::stoll(right);
                return PropertyValue(left_val.int_val + right_num);
            } catch (...) {
            }
        }

        // If right is parameter, left is number
        if (left_val.type == PropertyValue::STRING && right_val.type == PropertyValue::INTEGER) {
            try {
                int64_t left_num = std::stoll(left);
                return PropertyValue(left_num + right_val.int_val);
            } catch (...) {
            }
        }
    }

    // Process "param-number" pattern
    if (expr_text.find('-') != std::string::npos) {
        size_t      pos   = expr_text.find('-');
        std::string left  = expr_text.substr(0, pos);
        std::string right = expr_text.substr(pos + 1);

        // Remove spaces
        left.erase(std::remove(left.begin(), left.end(), ' '), left.end());
        right.erase(std::remove(right.begin(), right.end(), ' '), right.end());

        auto left_val  = resolve_parameter_reference(left);
        auto right_val = resolve_parameter_reference(right);

        // If both are integers, directly subtract
        if (left_val.type == PropertyValue::INTEGER && right_val.type == PropertyValue::INTEGER) {
            return PropertyValue(left_val.int_val - right_val.int_val);
        }

        // If left is parameter, right is number
        if (left_val.type == PropertyValue::INTEGER && right_val.type == PropertyValue::STRING) {
            try {
                int64_t right_num = std::stoll(right);
                return PropertyValue(left_val.int_val - right_num);
            } catch (...) {
            }
        }
    }

    // Process "TOTAL_WIDTH-1" type expression (for bit range)
    if (expr_text.find("-1") != std::string::npos) {
        size_t      pos        = expr_text.find("-1");
        std::string param_part = expr_text.substr(0, pos);
        param_part.erase(std::remove(param_part.begin(), param_part.end(), ' '), param_part.end());

        auto param_val = resolve_parameter_reference(param_part);
        if (param_val.type == PropertyValue::INTEGER) {
            return PropertyValue(param_val.int_val - 1);
        }
    }

    // Process complex expression: like "8 * 1024 + 512"
    if (expr_text.find("*") != std::string::npos && expr_text.find("+") != std::string::npos) {
        size_t mult_pos = expr_text.find("*");
        size_t plus_pos = expr_text.find("+");

        if (mult_pos < plus_pos) {
            // Format: A * B + C
            std::string a_str = expr_text.substr(0, mult_pos);
            std::string b_str = expr_text.substr(mult_pos + 1, plus_pos - mult_pos - 1);
            std::string c_str = expr_text.substr(plus_pos + 1);

            // Remove spaces
            a_str.erase(std::remove(a_str.begin(), a_str.end(), ' '), a_str.end());
            b_str.erase(std::remove(b_str.begin(), b_str.end(), ' '), b_str.end());
            c_str.erase(std::remove(c_str.begin(), c_str.end(), ' '), c_str.end());

            auto a_val = resolve_parameter_reference(a_str);
            auto b_val = resolve_parameter_reference(b_str);
            auto c_val = resolve_parameter_reference(c_str);

            // Try parsing as a number
            int64_t a_num = 0, b_num = 0, c_num = 0;
            bool    a_ok = false, b_ok = false, c_ok = false;

            if (a_val.type == PropertyValue::INTEGER) {
                a_num = a_val.int_val;
                a_ok  = true;
            } else {
                try {
                    a_num = std::stoll(a_str);
                    a_ok  = true;
                } catch (...) {
                }
            }

            if (b_val.type == PropertyValue::INTEGER) {
                b_num = b_val.int_val;
                b_ok  = true;
            } else {
                try {
                    b_num = std::stoll(b_str);
                    b_ok  = true;
                } catch (...) {
                }
            }

            if (c_val.type == PropertyValue::INTEGER) {
                c_num = c_val.int_val;
                c_ok  = true;
            } else {
                try {
                    c_num = std::stoll(c_str);
                    c_ok  = true;
                } catch (...) {
                }
            }

            if (a_ok && b_ok && c_ok) {
                return PropertyValue(a_num * b_num + c_num);
            }
        }
    }

    // Process parentheses expression: like "(2 * 1024) * 2"
    if (expr_text.find("(") != std::string::npos && expr_text.find(")") != std::string::npos) {
        size_t open_paren  = expr_text.find("(");
        size_t close_paren = expr_text.find(")");

        if (open_paren < close_paren) {
            std::string inner_expr = expr_text.substr(open_paren + 1, close_paren - open_paren - 1);
            std::string remaining  = expr_text.substr(close_paren + 1);

            // Recursively evaluate expression inside parentheses
            auto inner_val = evaluate_expression_from_string(inner_expr);

            if (inner_val.type == PropertyValue::INTEGER && !remaining.empty()) {
                // Process operator after parentheses
                remaining
                    .erase(std::remove(remaining.begin(), remaining.end(), ' '), remaining.end());

                if (remaining.substr(0, 1) == "*") {
                    std::string right_part = remaining.substr(1);
                    auto        right_val  = resolve_parameter_reference(right_part);

                    if (right_val.type == PropertyValue::INTEGER) {
                        return PropertyValue(inner_val.int_val * right_val.int_val);
                    } else {
                        try {
                            int64_t right_num = std::stoll(right_part);
                            return PropertyValue(inner_val.int_val * right_num);
                        } catch (...) {
                        }
                    }
                }
            }
        }
    }

    // If unable to parse, return original string
    return PropertyValue(expr_text);
}

// Enum and struct processing method implementation
void SystemRDLElaborator::collect_enum_and_struct_definitions(SystemRDLParser::RootContext *ast_root)
{
    // Collect top-level definitions
    for (auto root_elem : ast_root->root_elem()) {
        if (auto enum_def = root_elem->enum_def()) {
            register_enum_definition(enum_def);
        } else if (auto struct_def = root_elem->struct_def()) {
            register_struct_definition(struct_def);
        } else if (auto comp_def = root_elem->component_def()) {
            if (auto named_def = comp_def->component_named_def()) {
                // Recursively collect internal definitions
                if (auto body = named_def->component_body()) {
                    collect_enum_and_struct_definitions_from_body(body);
                }
            }
        }
    }
}

void SystemRDLElaborator::collect_enum_and_struct_definitions_from_body(
    SystemRDLParser::Component_bodyContext *body_ctx)
{
    for (auto body_elem : body_ctx->component_body_elem()) {
        if (auto enum_def = body_elem->enum_def()) {
            register_enum_definition(enum_def);
        } else if (auto struct_def = body_elem->struct_def()) {
            register_struct_definition(struct_def);
        } else if (auto comp_def = body_elem->component_def()) {
            if (auto named_def = comp_def->component_named_def()) {
                // Recursively collect internal definitions
                if (auto body = named_def->component_body()) {
                    collect_enum_and_struct_definitions_from_body(body);
                }
            }
        }
    }
}

void SystemRDLElaborator::register_enum_definition(SystemRDLParser::Enum_defContext *enum_def)
{
    std::string enum_name = enum_def->ID()->getText();

    EnumDefinition def;
    def.name = enum_name;

    // Parse enum entries
    int64_t current_value = 0;

    for (auto entry : enum_def->enum_entry()) {
        EnumEntry enum_entry;
        enum_entry.name = entry->ID()->getText();

        // Check if there's an explicit value
        if (auto expr = entry->expr()) {
            enum_entry.value = evaluate_integer_expression_enhanced(expr);
            current_value    = enum_entry.value + 1;
        } else {
            enum_entry.value = current_value++;
        }

        def.entries.push_back(enum_entry);
    }

    enum_definitions_[enum_name] = def;
}

void SystemRDLElaborator::register_struct_definition(SystemRDLParser::Struct_defContext *struct_def)
{
    // Get struct name (first ID)
    auto ids = struct_def->ID();
    if (ids.empty())
        return;

    std::string struct_name = ids[0]->getText();

    StructDefinition def;
    def.name = struct_name;

    // Parse struct members
    for (auto elem : struct_def->struct_elem()) {
        StructMember struct_member;
        struct_member.name = elem->ID()->getText();

        // Get member type
        if (auto struct_type = elem->struct_type()) {
            if (auto data_type = struct_type->data_type()) {
                if (auto basic_type = data_type->basic_data_type()) {
                    struct_member.type = basic_type->getText();
                } else {
                    struct_member.type = data_type->getText();
                }
            } else if (auto comp_type = struct_type->component_type()) {
                struct_member.type = comp_type->getText();
            }
        }

        // Note: SystemRDL struct members usually have no default values, here temporarily skipping

        def.members.push_back(struct_member);
    }

    struct_definitions_[struct_name] = def;
}

EnumDefinition *SystemRDLElaborator::find_enum_definition(const std::string &name)
{
    auto it = enum_definitions_.find(name);
    return (it != enum_definitions_.end()) ? &it->second : nullptr;
}

StructDefinition *SystemRDLElaborator::find_struct_definition(const std::string &name)
{
    auto it = struct_definitions_.find(name);
    return (it != struct_definitions_.end()) ? &it->second : nullptr;
}

// Field validation implementation
void SystemRDLElaborator::validate_register_fields(ElaboratedReg *reg_node)
{
    if (!reg_node)
        return;

    // Check field boundaries
    check_field_boundaries(reg_node);

    // Check field overlaps
    check_field_overlaps(reg_node);
}

void SystemRDLElaborator::check_field_boundaries(ElaboratedReg *reg_node)
{
    if (!reg_node)
        return;

    for (const auto &child : reg_node->children) {
        if (auto field = dynamic_cast<ElaboratedField *>(child.get())) {
            // Skip reserved fields (auto-generated)
            auto reserved_prop = field->get_property("reserved");
            if (reserved_prop && reserved_prop->type == PropertyValue::BOOLEAN
                && reserved_prop->bool_val) {
                continue;
            }

            // Check if field exceeds register width
            if (field->msb >= reg_node->register_width) {
                report_field_boundary_error(
                    field->inst_name, field->msb, reg_node->register_width, field->source_ctx);
            }

            // Check if field LSB exceeds register width
            if (field->lsb >= reg_node->register_width) {
                report_field_boundary_error(
                    field->inst_name, field->lsb, reg_node->register_width, field->source_ctx);
            }
        }
    }
}

void SystemRDLElaborator::check_field_overlaps(ElaboratedReg *reg_node)
{
    if (!reg_node)
        return;

    // Get all non-reserved fields
    std::vector<ElaboratedField *> fields;
    for (const auto &child : reg_node->children) {
        if (auto field = dynamic_cast<ElaboratedField *>(child.get())) {
            // Skip reserved fields (auto-generated)
            auto reserved_prop = field->get_property("reserved");
            if (reserved_prop && reserved_prop->type == PropertyValue::BOOLEAN
                && reserved_prop->bool_val) {
                continue;
            }
            fields.push_back(field);
        }
    }

    // Check for overlaps between all field pairs
    for (size_t i = 0; i < fields.size(); ++i) {
        for (size_t j = i + 1; j < fields.size(); ++j) {
            if (fields_overlap(fields[i], fields[j])) {
                // Calculate overlap range
                size_t overlap_start = std::max(fields[i]->lsb, fields[j]->lsb);
                size_t overlap_end   = std::min(fields[i]->msb, fields[j]->msb);

                report_field_overlap_error(
                    fields[i]->inst_name,
                    fields[j]->inst_name,
                    overlap_start,
                    overlap_end,
                    fields[i]->source_ctx);
            }
        }
    }
}

bool SystemRDLElaborator::fields_overlap(const ElaboratedField *field1, const ElaboratedField *field2)
{
    if (!field1 || !field2)
        return false;

    // Two fields overlap if their bit ranges intersect
    // Field 1: [field1->lsb, field1->msb]
    // Field 2: [field2->lsb, field2->msb]
    // They overlap if: max(lsb1, lsb2) <= min(msb1, msb2)

    size_t max_lsb = std::max(field1->lsb, field2->lsb);
    size_t min_msb = std::min(field1->msb, field2->msb);

    return max_lsb <= min_msb;
}

void SystemRDLElaborator::report_field_overlap_error(
    const std::string         &field1_name,
    const std::string         &field2_name,
    size_t                     overlap_start,
    size_t                     overlap_end,
    antlr4::ParserRuleContext *ctx)
{
    std::ostringstream oss;
    oss << "Field overlap detected: '" << field1_name << "' and '" << field2_name
        << "' both use bits [" << overlap_end << ":" << overlap_start << "]";
    report_error(oss.str(), ctx);
}

void SystemRDLElaborator::report_field_boundary_error(
    const std::string         &field_name,
    size_t                     field_msb,
    size_t                     reg_width,
    antlr4::ParserRuleContext *ctx)
{
    std::ostringstream oss;
    oss << "Field '" << field_name << "' bit position " << field_msb
        << " exceeds register width of " << reg_width << " bits (valid range: 0-" << (reg_width - 1)
        << ")";
    report_error(oss.str(), ctx);
}

// Gap detection and reserved field generation implementation
void SystemRDLElaborator::detect_and_fill_register_gaps(ElaboratedReg *reg_node)
{
    if (!reg_node)
        return;

    // Find gaps in the register
    auto gaps = find_register_gaps(reg_node);

    // Generate reserved fields for each gap
    for (const auto &gap : gaps) {
        size_t gap_msb = gap.first;
        size_t gap_lsb = gap.second;

        // Generate reserved field name
        std::string reserved_name = generate_reserved_field_name(gap_msb, gap_lsb);

        // Create reserved field
        auto reserved_field = create_reserved_field(gap_msb, gap_lsb, reserved_name);

        if (reserved_field) {
            // Set parent relationship
            reserved_field->parent           = reg_node;
            reserved_field->absolute_address = reg_node->absolute_address;

            // Add to register's children
            reg_node->add_child(std::move(reserved_field));
        }
    }
}

std::vector<std::pair<size_t, size_t>> SystemRDLElaborator::find_register_gaps(
    ElaboratedReg *reg_node)
{
    std::vector<std::pair<size_t, size_t>> gaps;

    if (!reg_node)
        return gaps;

    // Create a bit coverage vector for the register
    std::vector<bool> bit_coverage(reg_node->register_width, false);

    // Mark bits covered by existing fields
    for (const auto &child : reg_node->children) {
        if (auto field = dynamic_cast<ElaboratedField *>(child.get())) {
            // Mark bits from lsb to msb as covered
            for (size_t bit = field->lsb; bit <= field->msb && bit < reg_node->register_width;
                 ++bit) {
                bit_coverage[bit] = true;
            }
        }
    }

    // Find continuous gaps (uncovered bits)
    size_t gap_start = 0;
    bool   in_gap    = false;

    for (size_t bit = 0; bit < reg_node->register_width; ++bit) {
        if (!bit_coverage[bit]) {
            // Uncovered bit - start or continue gap
            if (!in_gap) {
                gap_start = bit;
                in_gap    = true;
            }
        } else {
            // Covered bit - end gap if we were in one
            if (in_gap) {
                gaps.emplace_back(bit - 1, gap_start); // MSB, LSB format
                in_gap = false;
            }
        }
    }

    // Handle gap extending to the end of register
    if (in_gap) {
        gaps.emplace_back(reg_node->register_width - 1, gap_start); // MSB, LSB format
    }

    return gaps;
}

std::unique_ptr<ElaboratedField> SystemRDLElaborator::create_reserved_field(
    size_t msb, size_t lsb, const std::string &name)
{
    auto field = std::make_unique<ElaboratedField>();

    // Set basic properties
    field->inst_name   = name;
    field->type_name   = "field";
    field->msb         = msb;
    field->lsb         = lsb;
    field->width       = (msb >= lsb) ? (msb - lsb + 1) : 0;
    field->reset_value = 0;

    // Set access properties for reserved fields
    field->sw_access = ElaboratedField::R;  // Software read-only
    field->hw_access = ElaboratedField::NA; // Hardware no access

    // Set properties
    field->set_property("msb", PropertyValue(static_cast<int64_t>(msb)));
    field->set_property("lsb", PropertyValue(static_cast<int64_t>(lsb)));
    field->set_property("width", PropertyValue(static_cast<int64_t>(field->width)));
    field->set_property("sw", PropertyValue(std::string("r")));
    field->set_property("hw", PropertyValue(std::string("na")));
    field->set_property("reset", PropertyValue(static_cast<int64_t>(0)));
    field->set_property("desc", PropertyValue(std::string("Reserved field - auto-generated")));
    field->set_property("reserved", PropertyValue(true));

    return field;
}

std::string SystemRDLElaborator::generate_reserved_field_name(size_t msb, size_t lsb)
{
    if (msb == lsb) {
        // Single bit: RESERVED_X
        return "RESERVED_" + std::to_string(lsb);
    } else {
        // Multiple bits: RESERVED_MSB_LSB
        return "RESERVED_" + std::to_string(msb) + "_" + std::to_string(lsb);
    }
}

// Automatic field positioning implementation
void SystemRDLElaborator::assign_automatic_field_positions(ElaboratedReg *reg_node)
{
    if (!reg_node)
        return;

    // Collect fields that need automatic positioning (in order of appearance)
    std::vector<ElaboratedField *> auto_position_fields;

    for (const auto &child : reg_node->children) {
        if (auto field = dynamic_cast<ElaboratedField *>(child.get())) {
            auto auto_pos_prop = field->get_property("auto_position");
            if (auto_pos_prop && auto_pos_prop->type == PropertyValue::BOOLEAN
                && auto_pos_prop->bool_val) {
                auto_position_fields.push_back(field);
            }
        }
    }

    // Group fields by base name to handle arrays correctly
    std::map<std::string, std::vector<ElaboratedField *>> field_groups;

    for (auto field : auto_position_fields) {
        std::string base_name = field->inst_name;

        // Extract base name from array field name (e.g., "enable[3]" -> "enable")
        size_t bracket_pos = base_name.find('[');
        if (bracket_pos != std::string::npos) {
            base_name.resize(bracket_pos);
        }

        field_groups[base_name].push_back(field);
    }

    // Assign positions starting from the next available bit
    size_t current_bit = calculate_next_available_bit(reg_node);

    // Process each field group
    for (auto &group : field_groups) {
        auto &fields = group.second;

        // Sort array fields by their index to ensure proper ordering
        std::sort(fields.begin(), fields.end(), [](ElaboratedField *a, ElaboratedField *b) {
            // Extract array index from field name (e.g., "enable[3]" -> 3)
            auto extract_index = [](const std::string &name) -> int {
                size_t bracket_pos = name.find('[');
                if (bracket_pos != std::string::npos) {
                    size_t end_pos = name.find(']', bracket_pos);
                    if (end_pos != std::string::npos) {
                        std::string index_str
                            = name.substr(bracket_pos + 1, end_pos - bracket_pos - 1);
                        try {
                            return std::stoi(index_str);
                        } catch (...) {
                        }
                    }
                }
                return 0;
            };

            return extract_index(a->inst_name) < extract_index(b->inst_name);
        });

        // Assign consecutive bit positions to array elements
        for (auto field : fields) {
            // Calculate position for this field (each field is 1 bit by default)
            size_t field_width = 1; // Default width for array elements

            // Check if fieldwidth property is defined and override
            auto fieldwidth_prop = field->get_property("fieldwidth");
            if (fieldwidth_prop && fieldwidth_prop->type == PropertyValue::INTEGER) {
                field_width = static_cast<size_t>(fieldwidth_prop->int_val);
            }

            size_t field_lsb = current_bit;
            size_t field_msb = current_bit + field_width - 1;

            // Check if field would exceed register width
            if (field_msb >= reg_node->register_width) {
                report_error(
                    "Auto-positioned field '" + field->inst_name
                        + "' would exceed register width. Field needs " + std::to_string(field_width)
                        + " bits but only " + std::to_string(reg_node->register_width - current_bit)
                        + " bits available from position " + std::to_string(current_bit),
                    field->source_ctx);
                continue;
            }

            // Assign the calculated position
            field->lsb   = field_lsb;
            field->msb   = field_msb;
            field->width = field_width;

            // Update properties
            field->set_property("lsb", PropertyValue(static_cast<int64_t>(field_lsb)));
            field->set_property("msb", PropertyValue(static_cast<int64_t>(field_msb)));
            field->set_property("width", PropertyValue(static_cast<int64_t>(field_width)));
            field->set_property("auto_position", PropertyValue(false)); // Clear auto-position flag

            // Move to next available position
            current_bit = field_msb + 1;
        }
    }
}

size_t SystemRDLElaborator::calculate_next_available_bit(ElaboratedReg *reg_node)
{
    if (!reg_node)
        return 0;

    size_t next_bit = 0;

    // Find the highest used bit among explicitly positioned fields
    for (const auto &child : reg_node->children) {
        if (auto field = dynamic_cast<ElaboratedField *>(child.get())) {
            auto auto_pos_prop = field->get_property("auto_position");
            // Skip fields that need auto-positioning
            if (auto_pos_prop && auto_pos_prop->type == PropertyValue::BOOLEAN
                && auto_pos_prop->bool_val) {
                continue;
            }

            // Only consider fields with valid bit positions
            if (field->msb != SIZE_MAX && field->lsb != SIZE_MAX) {
                size_t field_end = field->msb + 1;
                if (field_end > next_bit) {
                    next_bit = field_end;
                }
            }
        }
    }

    return next_bit;
}

// Instance address validation implementation
void SystemRDLElaborator::validate_instance_addresses(ElaboratedNode *parent)
{
    if (!parent)
        return;

    // Check address overlaps among child instances
    check_instance_address_overlaps(parent);

    // Recursively validate address spaces in child containers
    for (const auto &child : parent->children) {
        if (dynamic_cast<ElaboratedAddrmap *>(child.get())
            || dynamic_cast<ElaboratedRegfile *>(child.get())) {
            validate_instance_addresses(child.get());
        }
    }
}

void SystemRDLElaborator::check_instance_address_overlaps(ElaboratedNode *parent)
{
    if (!parent)
        return;

    // Collect addressable child instances
    std::vector<ElaboratedNode *> addressable_children;

    for (const auto &child : parent->children) {
        // Only check addressable components (regs, regfiles, memories)
        // Fields are handled separately by field validation
        if (dynamic_cast<ElaboratedReg *>(child.get())
            || dynamic_cast<ElaboratedRegfile *>(child.get())
            || dynamic_cast<ElaboratedMem *>(child.get())) {
            addressable_children.push_back(child.get());
        }
    }

    // Check for overlaps between all instance pairs
    for (size_t i = 0; i < addressable_children.size(); ++i) {
        for (size_t j = i + 1; j < addressable_children.size(); ++j) {
            if (instances_overlap(addressable_children[i], addressable_children[j])) {
                // Calculate address ranges for error reporting
                Address addr1_start = addressable_children[i]->absolute_address;
                Address addr1_end   = addr1_start + addressable_children[i]->size - 1;
                Address addr2_start = addressable_children[j]->absolute_address;
                Address addr2_end   = addr2_start + addressable_children[j]->size - 1;

                report_instance_overlap_error(
                    addressable_children[i]->inst_name,
                    addressable_children[j]->inst_name,
                    addr1_start,
                    addr1_end,
                    addr2_start,
                    addr2_end,
                    addressable_children[i]->source_ctx);
            }
        }
    }
}

bool SystemRDLElaborator::instances_overlap(
    const ElaboratedNode *instance1, const ElaboratedNode *instance2)
{
    if (!instance1 || !instance2)
        return false;

    // Skip instances with invalid addresses or sizes
    if (instance1->size == 0 || instance2->size == 0)
        return false;

    // Calculate address ranges
    Address addr1_start = instance1->absolute_address;
    Address addr1_end   = addr1_start + instance1->size - 1;
    Address addr2_start = instance2->absolute_address;
    Address addr2_end   = addr2_start + instance2->size - 1;

    // Two ranges overlap if: max(start1, start2) <= min(end1, end2)
    Address max_start = std::max(addr1_start, addr2_start);
    Address min_end   = std::min(addr1_end, addr2_end);

    return max_start <= min_end;
}

void SystemRDLElaborator::report_instance_overlap_error(
    const std::string         &instance1_name,
    const std::string         &instance2_name,
    Address                    addr1_start,
    Address                    addr1_end,
    Address                    addr2_start,
    Address                    addr2_end,
    antlr4::ParserRuleContext *ctx)
{
    std::ostringstream oss;
    oss << "Instance address overlap detected: '" << instance1_name << "' at address range 0x"
        << std::hex << std::uppercase << addr1_start << "-0x" << addr1_end << " overlaps with '"
        << instance2_name << "' at address range 0x" << addr2_start << "-0x" << addr2_end;

    report_error(oss.str(), ctx);
}

} // namespace systemrdl

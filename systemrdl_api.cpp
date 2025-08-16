#include "systemrdl_api.h"

#include "SystemRDLLexer.h"
#include "SystemRDLParser.h"
#include "antlr4-runtime.h"
#include "elaborator.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

namespace systemrdl {

// Helper structure to keep ANTLR objects alive
struct ParseContext
{
    std::unique_ptr<antlr4::ANTLRInputStream>  input;
    std::unique_ptr<SystemRDLLexer>            lexer;
    std::unique_ptr<antlr4::CommonTokenStream> tokens;
    std::unique_ptr<SystemRDLParser>           parser;
    SystemRDLParser::RootContext              *tree;

    ParseContext(std::string_view content)
    {
        std::string        content_str(content);
        std::istringstream content_stream(content_str);

        input  = std::make_unique<antlr4::ANTLRInputStream>(content_stream);
        lexer  = std::make_unique<SystemRDLLexer>(input.get());
        tokens = std::make_unique<antlr4::CommonTokenStream>(lexer.get());
        parser = std::make_unique<SystemRDLParser>(tokens.get());
        tree   = parser->root();
    }

    bool hasErrors() const { return parser->getNumberOfSyntaxErrors() > 0; }
};

// Helper function to convert ANTLR parse tree to JSON using nlohmann/json
static nlohmann::json convert_ast_to_json(antlr4::tree::ParseTree *tree, SystemRDLParser *parser)
{
    if (auto *ruleContext = dynamic_cast<antlr4::ParserRuleContext *>(tree)) {
        nlohmann::json node;

        std::string ruleName = parser->getRuleNames()[ruleContext->getRuleIndex()];
        std::string text     = ruleContext->getText();

        node["type"]         = "rule";
        node["rule_name"]    = ruleName;
        node["text"]         = text;
        node["start_line"]   = ruleContext->getStart()->getLine();
        node["start_column"] = ruleContext->getStart()->getCharPositionInLine();
        node["stop_line"]    = ruleContext->getStop()->getLine();
        node["stop_column"]  = ruleContext->getStop()->getCharPositionInLine();

        if (ruleContext->children.size() > 0) {
            nlohmann::json children = nlohmann::json::array();
            for (auto *child : ruleContext->children) {
                children.push_back(convert_ast_to_json(child, parser));
            }
            node["children"] = children;
        }

        return node;
    } else if (auto *terminal = dynamic_cast<antlr4::tree::TerminalNode *>(tree)) {
        nlohmann::json node;
        node["type"]   = "terminal";
        node["text"]   = terminal->getText();
        node["line"]   = terminal->getSymbol()->getLine();
        node["column"] = terminal->getSymbol()->getCharPositionInLine();
        return node;
    }

    return nlohmann::json::object();
}

// Helper function to convert property value to JSON
static nlohmann::json convert_property_to_json(const systemrdl::PropertyValue &prop)
{
    switch (prop.type) {
    case systemrdl::PropertyValue::STRING:
        return prop.string_val;
    case systemrdl::PropertyValue::INTEGER:
        return prop.int_val;
    case systemrdl::PropertyValue::BOOLEAN:
        return prop.bool_val;
    case systemrdl::PropertyValue::ENUM:
        return prop.string_val; // Treat enum as string
    default:
        return "unknown_type";
    }
}

// Helper function to convert elaborated node to JSON using nlohmann/json
static nlohmann::json convert_elaborated_node_to_json(systemrdl::ElaboratedNode &node)
{
    nlohmann::json json_node;

    json_node["node_type"] = node.get_node_type();
    json_node["inst_name"] = node.inst_name;

    // Format address as hex string
    std::ostringstream hex_addr;
    hex_addr << "0x" << std::hex << node.absolute_address;
    json_node["absolute_address"] = hex_addr.str();

    json_node["size"] = node.size;

    if (!node.array_dimensions.empty()) {
        nlohmann::json array_dims = nlohmann::json::array();
        for (size_t dim : node.array_dimensions) {
            nlohmann::json dim_obj;
            dim_obj["size"] = dim;
            array_dims.push_back(dim_obj);
        }
        json_node["array_dimensions"] = array_dims;
    }

    if (!node.properties.empty()) {
        nlohmann::json props = nlohmann::json::object();
        for (const auto &prop : node.properties) {
            props[prop.first] = convert_property_to_json(prop.second);
        }
        json_node["properties"] = props;
    }

    if (node.children.size() > 0) {
        nlohmann::json children = nlohmann::json::array();
        for (auto &child : node.children) {
            children.push_back(convert_elaborated_node_to_json(*child));
        }
        json_node["children"] = children;
    }

    return json_node;
}

// Helper function to convert elaborated node to simplified JSON
static void extract_registers_simplified(
    systemrdl::ElaboratedNode &node,
    nlohmann::json            &registers_array,
    nlohmann::json            &regfiles_array,
    std::vector<std::string>  &path,
    std::vector<std::string>  &path_abs)
{
    // Format current absolute address as hex string
    std::ostringstream hex_addr;
    hex_addr << "0x" << std::hex << node.absolute_address;
    std::string current_addr = hex_addr.str();

    if (node.get_node_type() == "regfile") {
        // Add regfile to regfiles array
        nlohmann::json regfile_obj;
        regfile_obj["inst_name"] = node.inst_name;
        if (!node.properties.empty()) {
            auto name_prop = node.properties.find("name");
            if (name_prop != node.properties.end()) {
                regfile_obj["name"] = convert_property_to_json(name_prop->second);
            }
            auto desc_prop = node.properties.find("desc");
            if (desc_prop != node.properties.end()) {
                regfile_obj["desc"] = convert_property_to_json(desc_prop->second);
            }
        }
        regfile_obj["absolute_address"] = current_addr;
        regfile_obj["path"]             = nlohmann::json::array();
        for (const auto &p : path) {
            regfile_obj["path"].push_back(p);
        }
        regfile_obj["size"] = node.size;
        regfiles_array.push_back(regfile_obj);

        // Add current regfile to path for children
        path.push_back(node.inst_name);
        path_abs.push_back(current_addr);
    } else if (node.get_node_type() == "reg") {
        // This is a register - add it to the registers array
        nlohmann::json register_obj;
        register_obj["inst_name"] = node.inst_name;

        // Add name and desc from properties if available
        if (!node.properties.empty()) {
            auto name_prop = node.properties.find("name");
            if (name_prop != node.properties.end()) {
                register_obj["name"] = convert_property_to_json(name_prop->second);
            }
            auto desc_prop = node.properties.find("desc");
            if (desc_prop != node.properties.end()) {
                register_obj["desc"] = convert_property_to_json(desc_prop->second);
            }
        }

        register_obj["absolute_address"] = current_addr;
        register_obj["offset"]           = static_cast<int>(node.absolute_address);
        register_obj["size"] = static_cast<int>(node.size); // Add size field for convenience

        // Add register-specific information if this is an ElaboratedReg
        if (auto reg_node = dynamic_cast<systemrdl::ElaboratedReg *>(&node)) {
            register_obj["register_width"] = static_cast<int>(reg_node->register_width);
            if (!reg_node->register_reset_hex.empty()) {
                register_obj["register_reset_value"] = reg_node->register_reset_hex;
            }
        }

        register_obj["path"]     = nlohmann::json::array();
        register_obj["path_abs"] = nlohmann::json::array();

        // Add path information
        for (const auto &p : path) {
            register_obj["path"].push_back(p);
        }
        for (const auto &pa : path_abs) {
            register_obj["path_abs"].push_back(pa);
        }

        // Extract fields
        nlohmann::json fields = nlohmann::json::array();
        for (const auto &child : node.children) {
            if (child->get_node_type() == "field") {
                nlohmann::json field_obj;
                field_obj["inst_name"] = child->inst_name;

                // Add field properties
                if (!child->properties.empty()) {
                    auto name_prop = child->properties.find("name");
                    if (name_prop != child->properties.end()) {
                        field_obj["name"] = convert_property_to_json(name_prop->second);
                    } else {
                        field_obj["name"] = child->inst_name; // fallback to inst_name
                    }
                    auto desc_prop = child->properties.find("desc");
                    if (desc_prop != child->properties.end()) {
                        field_obj["desc"] = convert_property_to_json(desc_prop->second);
                    }

                    // Add other important properties
                    for (const auto &prop : child->properties) {
                        if (prop.first == "lsb" || prop.first == "msb" || prop.first == "width"
                            || prop.first == "sw" || prop.first == "hw" || prop.first == "reserved"
                            || prop.first == "reset" || prop.first == "onwrite"
                            || prop.first == "onread") {
                            field_obj[prop.first] = convert_property_to_json(prop.second);
                        }
                    }
                }

                std::ostringstream field_hex_addr;
                field_hex_addr << "0x" << std::hex << child->absolute_address;
                field_obj["absolute_address"] = field_hex_addr.str();

                fields.push_back(field_obj);
            }
        }
        register_obj["fields"] = fields;

        registers_array.push_back(register_obj);
    } else {
        // For addrmap and other node types, continue recursing but don't add to path for addrmap
        if (node.get_node_type() != "addrmap") {
            path.push_back(node.inst_name);
            path_abs.push_back(current_addr);
        }
    }

    // Recurse through children
    for (auto &child : node.children) {
        extract_registers_simplified(*child, registers_array, regfiles_array, path, path_abs);
    }

    // Remove current node from path when done (except for addrmap)
    if (node.get_node_type() == "regfile"
        || (node.get_node_type() != "addrmap" && node.get_node_type() != "reg")) {
        if (!path.empty())
            path.pop_back();
        if (!path_abs.empty())
            path_abs.pop_back();
    }
}

static nlohmann::json convert_elaborated_node_to_simplified_json(systemrdl::ElaboratedNode &node)
{
    nlohmann::json result;
    result["format"]  = "SystemRDL_SimplifiedModel";
    result["version"] = "1.0";

    // Extract addrmap information (should be the root node)
    nlohmann::json addrmap_obj;
    addrmap_obj["inst_name"] = node.inst_name;

    // Add addrmap properties
    if (!node.properties.empty()) {
        auto name_prop = node.properties.find("name");
        if (name_prop != node.properties.end()) {
            addrmap_obj["name"] = convert_property_to_json(name_prop->second);
        }
        auto desc_prop = node.properties.find("desc");
        if (desc_prop != node.properties.end()) {
            addrmap_obj["desc"] = convert_property_to_json(desc_prop->second);
        }
    }

    std::ostringstream hex_addr;
    hex_addr << "0x" << std::hex << node.absolute_address;
    addrmap_obj["absolute_address"] = hex_addr.str();
    addrmap_obj["base"]             = hex_addr.str(); // Alternative name for base address

    result["addrmap"] = addrmap_obj;

    // Extract registers and regfiles
    nlohmann::json           registers_array = nlohmann::json::array();
    nlohmann::json           regfiles_array  = nlohmann::json::array();
    std::vector<std::string> path;
    std::vector<std::string> path_abs;

    // Start with addrmap in path
    path.push_back(node.inst_name);
    path_abs.push_back(hex_addr.str());

    for (auto &child : node.children) {
        extract_registers_simplified(*child, registers_array, regfiles_array, path, path_abs);
    }

    // Add regfiles array if not empty
    if (!regfiles_array.empty()) {
        result["regfiles"] = regfiles_array;
    }

    result["registers"] = registers_array;

    return result;
}

// CSV Row structure
struct CSVRow
{
    std::string addrmap_offset;
    std::string addrmap_name;
    std::string reg_offset;
    std::string reg_name;
    std::string reg_width;
    std::string field_name;
    std::string field_lsb;
    std::string field_msb;
    std::string reset_value;
    std::string sw_access;
    std::string hw_access;
    std::string onread;  // New: onread behavior (rclr, rset, ruser)
    std::string onwrite; // New: onwrite behavior (woclr, woset, wot, etc.)
    std::string description;
};

// CSV Parser class (full implementation from csv2rdl_main.cpp)
class CSVParser
{
private:
    // Standard column names (lowercase for comparison)
    const std::vector<std::string> standard_columns
        = {"addrmap_offset",
           "addrmap_name",
           "reg_offset",
           "reg_name",
           "reg_width",
           "field_name",
           "field_lsb",
           "field_msb",
           "reset_value",
           "sw_access",
           "hw_access",
           "onread",
           "onwrite",
           "description"};

    // Trim whitespace
    std::string trim(const std::string &str)
    {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
            return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    // Remove all newlines from a string (for names)
    std::string remove_all_newlines(const std::string &str)
    {
        std::string result = str;
        result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
        result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
        return result;
    }

    // Process description: trim leading/trailing newlines, preserve internal ones, collapse multiple consecutive newlines
    std::string process_description(const std::string &str)
    {
        if (str.empty())
            return str;

        // First trim leading and trailing whitespace and newlines
        std::string trimmed = trim(str);
        if (trimmed.empty())
            return trimmed;

        // Replace multiple consecutive newlines with single newlines
        std::string result;
        bool        prev_was_newline = false;

        for (char c : trimmed) {
            if (c == '\n' || c == '\r') {
                if (!prev_was_newline) {
                    result += '\n';
                    prev_was_newline = true;
                }
            } else {
                result += c;
                prev_was_newline = false;
            }
        }

        return result;
    }

    // Remove outer quotes from a field if they exist and match
    std::string remove_outer_quotes(const std::string &str)
    {
        if (str.length() < 2) {
            return str;
        }

        // Check for matching outer quotes (double quotes)
        if (str.front() == '"' && str.back() == '"') {
            return str.substr(1, str.length() - 2);
        }

        // Check for matching outer quotes (single quotes)
        if (str.front() == '\'' && str.back() == '\'') {
            return str.substr(1, str.length() - 2);
        }

        // No matching outer quotes found
        return str;
    }

    // Enhanced field processing that handles various quoting styles
    std::string process_field_with_quotes(const std::string &str, bool is_description = false)
    {
        if (str.empty()) {
            return str;
        }

        // First trim whitespace
        std::string trimmed = trim(str);
        if (trimmed.empty()) {
            return trimmed;
        }

        // Remove outer quotes if they exist and match
        std::string unquoted = remove_outer_quotes(trimmed);

        // Apply appropriate processing based on field type
        if (is_description) {
            return process_description(unquoted);
        } else {
            return unquoted;
        }
    }

    // Convert to lowercase
    std::string to_lower(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // Convert to uppercase
    std::string to_upper(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    // Calculate Levenshtein distance
    int levenshtein_distance(const std::string &s1, const std::string &s2)
    {
        const size_t                  len1 = s1.size(), len2 = s2.size();
        std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

        for (size_t i = 1; i <= len1; ++i)
            d[i][0] = i;
        for (size_t i = 1; i <= len2; ++i)
            d[0][i] = i;

        for (size_t i = 1; i <= len1; ++i) {
            for (size_t j = 1; j <= len2; ++j) {
                d[i][j] = std::min(
                    {d[i - 1][j] + 1,
                     d[i][j - 1] + 1,
                     d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1)});
            }
        }

        return d[len1][len2];
    }

    // Find best matching column
    int find_best_match(const std::string &header, const std::vector<std::string> &standards)
    {
        std::string lower_header = to_lower(header);

        // First try exact match
        for (size_t i = 0; i < standards.size(); ++i) {
            if (lower_header == standards[i]) {
                return static_cast<int>(i);
            }
        }

        // Check for common abbreviations
        std::map<std::string, std::string> abbrev_map
            = {{"sw_acc", "sw_access"},
               {"hw_acc", "hw_access"},
               {"access", "sw_access"}, // fallback for ambiguous case
               {"addr_offset", "addrmap_offset"},
               {"addr_name", "addrmap_name"},
               {"lsb", "field_lsb"},
               {"msb", "field_msb"},
               {"desc", "description"},
               {"width", "reg_width"}};

        // Try abbreviation mapping
        auto abbrev_it = abbrev_map.find(lower_header);
        if (abbrev_it != abbrev_map.end()) {
            for (size_t i = 0; i < standards.size(); ++i) {
                if (abbrev_it->second == standards[i]) {
                    return static_cast<int>(i);
                }
            }
        }

        // Then try fuzzy match with edit distance <= 3
        int best_match   = -1;
        int min_distance = 4; // Maximum allowed distance

        for (size_t i = 0; i < standards.size(); ++i) {
            int distance = levenshtein_distance(lower_header, standards[i]);
            if (distance < min_distance) {
                min_distance = distance;
                best_match   = static_cast<int>(i);
            }
        }

        return best_match;
    }

    // Detect CSV delimiter (, or ;)
    char detect_delimiter(const std::string &line)
    {
        size_t comma_count     = std::count(line.begin(), line.end(), ',');
        size_t semicolon_count = std::count(line.begin(), line.end(), ';');
        return semicolon_count > comma_count ? ';' : ',';
    }

    // Split CSV line respecting quoted fields (supports both single and double quotes)
    std::vector<std::string> split_csv_line(const std::string &line, char delimiter)
    {
        std::vector<std::string> fields;
        std::string              current_field;
        bool                     in_double_quotes = false;
        bool                     in_single_quotes = false;

        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];

            if (c == '"' && !in_single_quotes) {
                if (in_double_quotes && i + 1 < line.length() && line[i + 1] == '"') {
                    // Double quote - escaped quote within quoted field
                    current_field += '"';
                    i++; // Skip the next quote
                } else {
                    // Toggle double quote state
                    in_double_quotes = !in_double_quotes;
                }
            } else if (c == '\'' && !in_double_quotes) {
                if (in_single_quotes && i + 1 < line.length() && line[i + 1] == '\'') {
                    // Single quote - escaped quote within quoted field
                    current_field += '\'';
                    i++; // Skip the next quote
                } else {
                    // Toggle single quote state
                    in_single_quotes = !in_single_quotes;
                }
            } else if (c == delimiter && !in_double_quotes && !in_single_quotes) {
                fields.push_back(current_field); // Don't trim here, let field processors handle it
                current_field.clear();
            } else {
                current_field += c;
            }
        }

        fields.push_back(current_field); // Don't trim here either
        return fields;
    }

    // Create column mapping from header to standard columns
    std::vector<int> create_column_mapping(const std::vector<std::string> &headers)
    {
        std::vector<int> mapping;

        std::cout << "[MAP] Column mapping:" << std::endl;
        for (size_t i = 0; i < headers.size(); ++i) {
            int match = find_best_match(headers[i], standard_columns);
            mapping.push_back(match);

            if (match >= 0) {
                std::cout << "  [" << i << "] \"" << headers[i] << "\" -> "
                          << standard_columns[match] << std::endl;
            } else {
                std::cout << "  [" << i << "] \"" << headers[i] << "\" -> (ignored)" << std::endl;
            }
        }

        return mapping;
    }

    // Parse a data row using column mapping
    CSVRow parse_row(const std::vector<std::string> &fields, const std::vector<int> &mapping)
    {
        CSVRow row;

        for (size_t i = 0; i < fields.size() && i < mapping.size(); ++i) {
            if (mapping[i] < 0)
                continue; // Skip unmapped columns

            std::string value = fields[i];

            switch (mapping[i]) {
            case 0:
                row.addrmap_offset = process_field_with_quotes(value);
                break;
            case 1:
                row.addrmap_name = remove_all_newlines(process_field_with_quotes(value));
                break;
            case 2:
                row.reg_offset = process_field_with_quotes(value);
                break;
            case 3:
                row.reg_name = remove_all_newlines(process_field_with_quotes(value));
                break;
            case 4:
                row.reg_width = process_field_with_quotes(value);
                break;
            case 5:
                row.field_name = remove_all_newlines(process_field_with_quotes(value));
                break;
            case 6:
                row.field_lsb = process_field_with_quotes(value);
                break;
            case 7:
                row.field_msb = process_field_with_quotes(value);
                break;
            case 8:
                row.reset_value = process_field_with_quotes(value);
                break;
            case 9:
                row.sw_access = process_field_with_quotes(value);
                break;
            case 10:
                row.hw_access = process_field_with_quotes(value);
                break;
            case 11:
                row.onread = process_field_with_quotes(value);
                break;
            case 12:
                row.onwrite = process_field_with_quotes(value);
                break;
            case 13:
                row.description = process_field_with_quotes(value, true);
                break;
            }
        }

        return row;
    }

    // Parse CSV content handling multiline quoted fields (supports both single and double quotes)
    std::vector<std::string> parse_csv_content(const std::string &content)
    {
        std::vector<std::string> lines;
        std::string              current_line;
        bool                     in_double_quotes = false;
        bool                     in_single_quotes = false;

        for (size_t i = 0; i < content.length(); ++i) {
            char c = content[i];

            if (c == '"' && !in_single_quotes) {
                in_double_quotes = !in_double_quotes;
                current_line += c;
            } else if (c == '\'' && !in_double_quotes) {
                in_single_quotes = !in_single_quotes;
                current_line += c;
            } else if (c == '\n' && !in_double_quotes && !in_single_quotes) {
                // End of line outside quotes
                if (!current_line.empty()) {
                    lines.push_back(current_line);
                    current_line.clear();
                }
            } else {
                current_line += c;
            }
        }

        // Add last line if not empty
        if (!current_line.empty()) {
            lines.push_back(current_line);
        }

        return lines;
    }

public:
    // Validate CSV structure according to SystemRDL CSV format specification
    std::string validate_csv_structure(const std::vector<CSVRow> &rows)
    {
        if (rows.empty()) {
            return "Error: CSV file is empty";
        }

        enum class ExpectedRowType { ADDRMAP, REG, FIELD };

        ExpectedRowType expected     = ExpectedRowType::ADDRMAP;
        size_t          logical_line = 2; // Start from line 2 (after header)

        for (const auto &row : rows) {
            bool is_addrmap_row = !row.addrmap_offset.empty() && !row.addrmap_name.empty();
            bool is_reg_row     = !row.reg_offset.empty() && !row.reg_name.empty();
            bool is_field_row   = !row.field_name.empty();

            // Count how many types this row appears to be
            int type_count = (is_addrmap_row ? 1 : 0) + (is_reg_row ? 1 : 0)
                             + (is_field_row ? 1 : 0);

            if (type_count == 0) {
                return "Error: Line " + std::to_string(logical_line)
                       + " does not contain valid addrmap, register, or field information";
            }

            if (type_count > 1) {
                std::string error = "Error: Line " + std::to_string(logical_line)
                                    + " contains mixed information types: ";
                if (is_addrmap_row)
                    error += "addrmap ";
                if (is_reg_row)
                    error += "register ";
                if (is_field_row)
                    error += "field ";
                return error;
            }

            // Check if the row type matches expectations
            if (is_addrmap_row) {
                expected = ExpectedRowType::REG;
            } else if (is_reg_row) {
                if (expected == ExpectedRowType::ADDRMAP) {
                    return "Error: Line " + std::to_string(logical_line)
                           + " defines a register but no addrmap was defined first";
                }
                expected = ExpectedRowType::FIELD;
            } else if (is_field_row) {
                if (expected == ExpectedRowType::ADDRMAP) {
                    return "Error: Line " + std::to_string(logical_line)
                           + " defines a field but no addrmap was defined first";
                }
                if (expected == ExpectedRowType::REG) {
                    return "Error: Line " + std::to_string(logical_line)
                           + " defines a field but no register was defined for this addrmap";
                }
                // After fields, we can have more fields, new registers, or new addrmaps
                // expected stays as FIELD, but we allow transitions
            }

            logical_line++;
        }

        // Validate field properties values
        for (const auto &row : rows) {
            if (!row.field_name.empty()) {
                // Validate access values
                if (!row.sw_access.empty()) {
                    std::string sw_upper = to_upper(row.sw_access);
                    if (sw_upper != "RW" && sw_upper != "RO" && sw_upper != "WO"
                        && sw_upper != "NA") {
                        return "Error: Invalid sw_access value '" + row.sw_access
                               + "' (use RW/RO/WO/NA)";
                    }
                }
                if (!row.hw_access.empty()) {
                    std::string hw_upper = to_upper(row.hw_access);
                    if (hw_upper != "RW" && hw_upper != "RO" && hw_upper != "WO"
                        && hw_upper != "NA") {
                        return "Error: Invalid hw_access value '" + row.hw_access
                               + "' (use RW/RO/WO/NA)";
                    }
                }

                // Validate onread values
                if (!row.onread.empty()) {
                    std::string onread_upper = to_upper(row.onread);
                    if (onread_upper != "RCLR" && onread_upper != "RSET"
                        && onread_upper != "RUSER") {
                        return "Error: Invalid onread value '" + row.onread
                               + "' (use rclr/rset/ruser)";
                    }
                }

                // Validate onwrite values
                if (!row.onwrite.empty()) {
                    std::string onwrite_upper = to_upper(row.onwrite);
                    if (onwrite_upper != "WOCLR" && onwrite_upper != "WOSET"
                        && onwrite_upper != "WOT" && onwrite_upper != "WZS"
                        && onwrite_upper != "WZC" && onwrite_upper != "WZT"
                        && onwrite_upper != "WCLR" && onwrite_upper != "WSET"
                        && onwrite_upper != "WUSER") {
                        return "Error: Invalid onwrite value '" + row.onwrite
                               + "' (use woclr/woset/wot/wzs/wzc/wzt/wclr/wset/wuser)";
                    }
                }
            }
        }

        return ""; // Empty string means validation passed
    }

    // Parse CSV content and return rows
    std::vector<CSVRow> parse_content(const std::string &csv_content)
    {
        std::vector<CSVRow> rows;
        bool                first_line = true;
        std::vector<int>    column_mapping;

        // Parse CSV content line by line, handling multiline quoted fields
        std::vector<std::string> csv_lines = parse_csv_content(csv_content);

        for (const auto &csv_line : csv_lines) {
            if (csv_line.empty())
                continue;

            // Detect delimiter from first data line
            char                     delimiter = detect_delimiter(csv_line);
            std::vector<std::string> fields    = split_csv_line(csv_line, delimiter);

            if (first_line) {
                // Parse header and create column mapping
                column_mapping = create_column_mapping(fields);
                first_line     = false;
                continue;
            }

            // Parse data row
            CSVRow row = parse_row(fields, column_mapping);
            rows.push_back(row);
        }

        return rows;
    }
};

// RDL Generator class
class RDLGenerator
{
private:
    std::string current_addrmap_offset;
    std::string current_reg_offset;

    std::string format_address(const std::string &addr)
    {
        if (addr.empty())
            return "0x0000";
        if (addr.substr(0, 2) == "0x" || addr.substr(0, 2) == "0X") {
            return addr;
        }
        return "0x" + addr;
    }

    std::string escape_string(const std::string &str)
    {
        std::string result;
        for (char c : str) {
            if (c == '"') {
                // Escape double quotes for SystemRDL string literals
                result += "\\\"";
            } else if (c == '\\') {
                // Escape backslashes
                result += "\\\\";
            } else if (c == '\n') {
                // SystemRDL supports multiline strings, preserve real newlines
                result += c;
            } else if (c == '\r') {
                // Skip carriage returns (just use \n for line endings)
                // Don't add anything for \r
            } else if (c == '\t') {
                // Convert tabs to \t escape sequence
                result += "\\t";
            } else if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) >= 127) {
                // Escape other control characters and non-ASCII characters as hex
                std::ostringstream hex_escape;
                hex_escape << "\\x" << std::hex << std::setw(2) << std::setfill('0')
                           << static_cast<unsigned int>(static_cast<unsigned char>(c));
                result += hex_escape.str();
            } else {
                // Regular printable ASCII characters (including single quotes) are safe
                result += c;
            }
        }
        return result;
    }

    std::string to_lower(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // Helper function to map CSV access values to SystemRDL access values
    std::string map_access_to_systemrdl(const std::string &csv_access)
    {
        std::string upper_access;
        std::transform(
            csv_access.begin(), csv_access.end(), std::back_inserter(upper_access), ::toupper);

        if (upper_access == "RW")
            return "rw";
        if (upper_access == "RO")
            return "r";
        if (upper_access == "WO")
            return "w";
        if (upper_access == "NA")
            return "na";
        return csv_access; // fallback to original if unknown
    }

public:
    std::string generate(const std::vector<CSVRow> &rows)
    {
        std::ostringstream rdl;

        std::string current_addrmap;
        std::string current_reg;
        bool        in_reg = false;

        for (const auto &row : rows) {
            // Handle address map definition
            if (!row.addrmap_offset.empty() && !row.addrmap_name.empty()) {
                if (in_reg) {
                    rdl << "    } " << current_reg << " @ " << format_address(current_reg_offset)
                        << ";\n\n";
                    in_reg = false;
                }
                if (!current_addrmap.empty()) {
                    rdl << "};\n\n";
                }

                current_addrmap        = row.addrmap_name;
                current_addrmap_offset = row.addrmap_offset;

                rdl << "addrmap " << current_addrmap << " {\n";
                rdl << "    name = \"" << escape_string(row.addrmap_name) << "\";\n";
                if (!row.description.empty()) {
                    rdl << "    desc = \"" << escape_string(row.description) << "\";\n";
                }
                rdl << "\n";
            }
            // Handle register definition
            else if (!row.reg_offset.empty() && !row.reg_name.empty()) {
                if (in_reg) {
                    rdl << "    } " << current_reg << " @ " << format_address(current_reg_offset)
                        << ";\n\n";
                }

                current_reg        = row.reg_name;
                current_reg_offset = row.reg_offset;
                in_reg             = true;

                rdl << "    reg {\n";
                rdl << "        name = \"" << escape_string(row.reg_name) << "\";\n";
                if (!row.description.empty()) {
                    rdl << "        desc = \"" << escape_string(row.description) << "\";\n";
                }
                if (!row.reg_width.empty()) {
                    rdl << "        regwidth = " << row.reg_width << ";\n";
                }
                rdl << "\n";
            }
            // Handle field definition
            else if (!row.field_name.empty() && in_reg) {
                rdl << "        field {\n";
                rdl << "            name = \"" << escape_string(row.field_name) << "\";\n";
                if (!row.description.empty()) {
                    rdl << "            desc = \"" << escape_string(row.description) << "\";\n";
                }

                // Add access properties
                if (!row.sw_access.empty()) {
                    rdl << "            sw = " << map_access_to_systemrdl(row.sw_access) << ";\n";
                }
                if (!row.hw_access.empty()) {
                    rdl << "            hw = " << map_access_to_systemrdl(row.hw_access) << ";\n";
                }

                // Add onread/onwrite properties
                if (!row.onread.empty()) {
                    rdl << "            onread = " << to_lower(row.onread) << ";\n";
                }
                if (!row.onwrite.empty()) {
                    rdl << "            onwrite = " << to_lower(row.onwrite) << ";\n";
                }

                rdl << "        } " << row.field_name;

                // Add bit range
                if (!row.field_lsb.empty() && !row.field_msb.empty()) {
                    rdl << "[" << row.field_msb << ":" << row.field_lsb << "]";
                }

                // Add reset value
                if (!row.reset_value.empty()) {
                    rdl << " = " << row.reset_value;
                }

                rdl << ";\n\n";
            }
        }

        // Close remaining structures
        if (in_reg) {
            rdl << "    } " << current_reg << " @ " << format_address(current_reg_offset)
                << ";\n\n";
        }
        if (!current_addrmap.empty()) {
            rdl << "};\n";
        }

        return rdl.str();
    }
};

// Main API functions
Result parse(std::string_view rdl_content)
{
    try {
        ParseContext ctx(rdl_content);

        if (ctx.hasErrors()) {
            return Result::error("Syntax errors found during parsing");
        }

        // Convert AST to JSON
        nlohmann::json ast_result = convert_ast_to_json(ctx.tree, ctx.parser.get());

        // Create full JSON structure
        nlohmann::json json_result;
        json_result["format"]  = "SystemRDL_AST";
        json_result["version"] = "1.0";
        json_result["ast"]     = nlohmann::json::array();
        json_result["ast"].push_back(ast_result);

        return Result::success(json_result.dump(2)); // Pretty print with 2 spaces
    } catch (const std::exception &e) {
        return Result::error(std::string("Parse error: ") + e.what());
    }
}

Result elaborate(std::string_view rdl_content)
{
    try {
        ParseContext ctx(rdl_content);

        if (ctx.hasErrors()) {
            return Result::error("Syntax errors found during parsing");
        }

        // Create elaborator and elaborate the design
        systemrdl::SystemRDLElaborator elaborator;
        auto                           elaborated_model = elaborator.elaborate(ctx.tree);

        if (elaborator.has_errors()) {
            std::string error_details = "Elaboration errors:\n";
            for (const auto &err : elaborator.get_errors()) {
                error_details += "  " + err.message + "\n";
            }
            return Result::error(error_details);
        }

        if (!elaborated_model) {
            return Result::error("Failed to elaborate design");
        }

        // Convert elaborated model to JSON
        nlohmann::json elaborated_result = convert_elaborated_node_to_json(*elaborated_model);

        // Create full JSON structure
        nlohmann::json json_result;
        json_result["format"]  = "SystemRDL_ElaboratedModel";
        json_result["version"] = "1.0";
        json_result["model"]   = nlohmann::json::array();
        json_result["model"].push_back(elaborated_result);

        return Result::success(json_result.dump(2)); // Pretty print with 2 spaces
    } catch (const std::exception &e) {
        return Result::error(std::string("Elaboration error: ") + e.what());
    }
}

Result elaborate_simplified(std::string_view rdl_content)
{
    try {
        ParseContext ctx(rdl_content);

        if (ctx.hasErrors()) {
            return Result::error("Syntax errors found during parsing");
        }

        // Create elaborator and elaborate the design
        systemrdl::SystemRDLElaborator elaborator;
        auto                           elaborated_model = elaborator.elaborate(ctx.tree);

        if (elaborator.has_errors()) {
            std::string error_details = "Elaboration errors:\n";
            for (const auto &err : elaborator.get_errors()) {
                error_details += "  " + err.message + "\n";
            }
            return Result::error(error_details);
        }

        if (!elaborated_model) {
            return Result::error("Failed to elaborate design");
        }

        // Convert elaborated model to simplified JSON
        nlohmann::json simplified_result = convert_elaborated_node_to_simplified_json(
            *elaborated_model);

        return Result::success(simplified_result.dump(2)); // Pretty print with 2 spaces
    } catch (const std::exception &e) {
        return Result::error(std::string("Elaboration error: ") + e.what());
    }
}

Result csv_to_rdl(std::string_view csv_content)
{
    try {
        // Parse CSV content
        CSVParser           parser;
        std::vector<CSVRow> rows = parser.parse_content(std::string(csv_content));

        // Validate CSV structure
        std::string validation_result = parser.validate_csv_structure(rows);
        if (!validation_result.empty()) {
            return Result::error(validation_result);
        }

        // Generate RDL
        RDLGenerator generator;
        std::string  rdl_content = generator.generate(rows);

        return Result::success(rdl_content);
    } catch (const std::exception &e) {
        return Result::error(std::string("CSV conversion error: ") + e.what());
    }
}

// File-based API functions
namespace file {

Result parse(const std::string &filename)
{
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return Result::error("Cannot open file: " + filename);
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        return systemrdl::parse(content);
    } catch (const std::exception &e) {
        return Result::error(std::string("File read error: ") + e.what());
    }
}

Result elaborate(const std::string &filename)
{
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return Result::error("Cannot open file: " + filename);
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        return systemrdl::elaborate(content);
    } catch (const std::exception &e) {
        return Result::error(std::string("File read error: ") + e.what());
    }
}

Result elaborate_simplified(const std::string &filename)
{
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return Result::error("Cannot open file: " + filename);
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        return systemrdl::elaborate_simplified(content);
    } catch (const std::exception &e) {
        return Result::error(std::string("File read error: ") + e.what());
    }
}

Result csv_to_rdl(const std::string &filename)
{
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return Result::error("Cannot open file: " + filename);
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        return systemrdl::csv_to_rdl(content);
    } catch (const std::exception &e) {
        return Result::error(std::string("File read error: ") + e.what());
    }
}

} // namespace file

// Stream-based API functions
namespace stream {

bool parse(std::istream &input, std::ostream &output)
{
    try {
        std::string
            content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

        auto result = systemrdl::parse(content);
        if (result.ok()) {
            output << result.value();
            return true;
        } else {
            output << "Error: " << result.error();
            return false;
        }
    } catch (const std::exception &e) {
        output << "Stream error: " << e.what();
        return false;
    }
}

bool elaborate(std::istream &input, std::ostream &output)
{
    try {
        std::string
            content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

        auto result = systemrdl::elaborate(content);
        if (result.ok()) {
            output << result.value();
            return true;
        } else {
            output << "Error: " << result.error();
            return false;
        }
    } catch (const std::exception &e) {
        output << "Stream error: " << e.what();
        return false;
    }
}

bool elaborate_simplified(std::istream &input, std::ostream &output)
{
    try {
        std::string
            content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

        auto result = systemrdl::elaborate_simplified(content);
        if (result.ok()) {
            output << result.value();
            return true;
        } else {
            output << "Error: " << result.error();
            return false;
        }
    } catch (const std::exception &e) {
        output << "Stream error: " << e.what();
        return false;
    }
}

bool csv_to_rdl(std::istream &input, std::ostream &output)
{
    try {
        std::string
            content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

        auto result = systemrdl::csv_to_rdl(content);
        if (result.ok()) {
            output << result.value();
            return true;
        } else {
            output << "Error: " << result.error();
            return false;
        }
    } catch (const std::exception &e) {
        output << "Stream error: " << e.what();
        return false;
    }
}

} // namespace stream

} // namespace systemrdl

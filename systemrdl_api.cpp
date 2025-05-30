#include <cstdio>
#include <fstream>
#include <sstream>

#include <antlr4-runtime.h>
#include <nlohmann/json.hpp>

#include "SystemRDLLexer.h"
#include "SystemRDLParser.h"
#include "elaborator.h"
#include "systemrdl_api.h"
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

Result csv_to_rdl(std::string_view csv_content)
{
    // For now, return a simple conversion
    // This would need to be implemented based on the CSV format specification
    try {
        std::string rdl_result = "// Generated from CSV\n";
        rdl_result += "addrmap generated_from_csv {\n";
        rdl_result += "    // TODO: Implement CSV to RDL conversion\n";
        rdl_result += "    // CSV content length: " + std::to_string(csv_content.length())
                      + " bytes\n";
        rdl_result += "};\n";

        return Result::success(std::move(rdl_result));
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

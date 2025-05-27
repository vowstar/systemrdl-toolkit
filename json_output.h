#ifndef JSON_OUTPUT_H
#define JSON_OUTPUT_H

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <iomanip>
#include "antlr4-runtime.h"
#include "SystemRDLParser.h"
#include "elaborator.h"

// Simple JSON builder without external dependencies
class JsonBuilder {
private:
    std::ostringstream json_;
    int indent_level_;
    bool first_item_;
    std::vector<bool> is_array_stack_;

    void write_indent() {
        for (int i = 0; i < indent_level_; i++) {
            json_ << "  ";
        }
    }

    std::string escape_string(const std::string& str) {
        std::string escaped = "\"";
        for (char c : str) {
            switch (c) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += c; break;
            }
        }
        escaped += "\"";
        return escaped;
    }

public:
    JsonBuilder() : indent_level_(0), first_item_(true) {}

    void start_object() {
        if (!first_item_) json_ << ",\n";
        write_indent();
        json_ << "{\n";
        indent_level_++;
        first_item_ = true;
        is_array_stack_.push_back(false);
    }

    void end_object() {
        json_ << "\n";
        indent_level_--;
        write_indent();
        json_ << "}";
        first_item_ = false;
        is_array_stack_.pop_back();
    }

    void start_array(const std::string& key = "") {
        if (!key.empty()) {
            if (!first_item_) json_ << ",\n";
            write_indent();
            json_ << escape_string(key) << ": ";
        } else if (!first_item_) {
            json_ << ",\n";
            write_indent();
        }
        json_ << "[\n";
        indent_level_++;
        first_item_ = true;
        is_array_stack_.push_back(true);
    }

    void end_array() {
        json_ << "\n";
        indent_level_--;
        write_indent();
        json_ << "]";
        first_item_ = false;
        is_array_stack_.pop_back();
    }

    void add_string(const std::string& key, const std::string& value) {
        if (!first_item_) json_ << ",\n";
        write_indent();
        json_ << escape_string(key) << ": " << escape_string(value);
        first_item_ = false;
    }

    void add_number(const std::string& key, long long value) {
        if (!first_item_) json_ << ",\n";
        write_indent();
        json_ << escape_string(key) << ": " << value;
        first_item_ = false;
    }

    void add_bool(const std::string& key, bool value) {
        if (!first_item_) json_ << ",\n";
        write_indent();
        json_ << escape_string(key) << ": " << (value ? "true" : "false");
        first_item_ = false;
    }

    void add_hex(const std::string& key, uint64_t value) {
        if (!first_item_) json_ << ",\n";
        write_indent();
        json_ << escape_string(key) << ": \"0x" << std::hex << value << std::dec << "\"";
        first_item_ = false;
    }

    std::string to_string() const {
        return json_.str();
    }
};

// AST to JSON converter
class ASTToJsonConverter {
private:
    JsonBuilder json_;
    SystemRDLParser* parser_;

    void convert_node(antlr4::tree::ParseTree* tree, int depth = 0) {
        if (auto* ruleContext = dynamic_cast<antlr4::ParserRuleContext*>(tree)) {
            json_.start_object();

            std::string ruleName = parser_->getRuleNames()[ruleContext->getRuleIndex()];
            std::string text = ruleContext->getText();

            json_.add_string("type", "rule");
            json_.add_string("rule_name", ruleName);
            json_.add_string("text", text);
            json_.add_number("start_line", ruleContext->getStart()->getLine());
            json_.add_number("start_column", ruleContext->getStart()->getCharPositionInLine());
            json_.add_number("stop_line", ruleContext->getStop()->getLine());
            json_.add_number("stop_column", ruleContext->getStop()->getCharPositionInLine());

            if (ruleContext->children.size() > 0) {
                json_.start_array("children");
                for (auto* child : ruleContext->children) {
                    convert_node(child, depth + 1);
                }
                json_.end_array();
            }

            json_.end_object();
        } else if (auto* terminal = dynamic_cast<antlr4::tree::TerminalNode*>(tree)) {
            json_.start_object();
            json_.add_string("type", "terminal");
            json_.add_string("text", terminal->getText());
            json_.add_number("line", terminal->getSymbol()->getLine());
            json_.add_number("column", terminal->getSymbol()->getCharPositionInLine());
            json_.end_object();
        }
    }

public:
    std::string convert_to_json(antlr4::tree::ParseTree* tree, SystemRDLParser* parser) {
        parser_ = parser;
        json_.start_object();
        json_.add_string("format", "SystemRDL_AST");
        json_.add_string("version", "1.0");

        json_.start_array("ast");
        convert_node(tree);
        json_.end_array();

        json_.end_object();
        return json_.to_string();
    }
};

// Elaborated model to JSON converter
class ElaboratedModelToJsonConverter : public systemrdl::ElaboratedModelTraverser {
private:
    JsonBuilder json_;
    bool first_node_;

    void write_property(const std::string& name, const systemrdl::PropertyValue& prop) {
        switch (prop.type) {
            case systemrdl::PropertyValue::STRING:
                json_.add_string(name, prop.string_val);
                break;
            case systemrdl::PropertyValue::INTEGER:
                json_.add_number(name, prop.int_val);
                break;
            case systemrdl::PropertyValue::BOOLEAN:
                json_.add_bool(name, prop.bool_val);
                break;
            case systemrdl::PropertyValue::ENUM:
                json_.add_string(name, prop.string_val);  // Treat enum as string
                break;
            default:
                json_.add_string(name, "unknown_type");
                break;
        }
    }

protected:
    void pre_visit(systemrdl::ElaboratedNode& node) override {
        json_.start_object();

        json_.add_string("node_type", node.get_node_type());
        json_.add_string("inst_name", node.inst_name);
        json_.add_hex("absolute_address", node.absolute_address);
        json_.add_number("size", node.size);

        if (!node.array_dimensions.empty()) {
            json_.start_array("array_dimensions");
            for (size_t dim : node.array_dimensions) {
                json_.start_object();
                json_.add_number("size", dim);
                json_.end_object();
            }
            json_.end_array();
        }

        if (!node.properties.empty()) {
            json_.start_object();
            json_.add_string("_section", "properties");
            for (const auto& prop : node.properties) {
                write_property(prop.first, prop.second);
            }
            json_.end_object();
        }

        if (node.children.size() > 0) {
            json_.start_array("children");
            first_node_ = true;
        }
    }

    void post_visit(systemrdl::ElaboratedNode& node) override {
        if (node.children.size() > 0) {
            json_.end_array();
        }
        json_.end_object();
        first_node_ = false;
    }

public:
    std::string convert_to_json(systemrdl::ElaboratedAddrmap& root) {
        first_node_ = true;
        json_.start_object();
        json_.add_string("format", "SystemRDL_ElaboratedModel");
        json_.add_string("version", "1.0");

        json_.start_array("model");
        traverse(root);
        json_.end_array();

        json_.end_object();
        return json_.to_string();
    }
};

// Helper function to write JSON to file
inline bool write_json_to_file(const std::string& json_content, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot write to file " << filename << std::endl;
        return false;
    }
    file << json_content;
    file.close();
    std::cout << "✅ JSON output written to: " << filename << std::endl;
    return true;
}

// Helper function to generate default JSON filename
inline std::string get_default_json_filename(const std::string& input_file, const std::string& suffix = "") {
    // Simple basename extraction without filesystem dependency
    size_t last_slash = input_file.find_last_of("/\\");
    size_t last_dot = input_file.find_last_of('.');

    std::string basename;
    if (last_slash != std::string::npos) {
        basename = input_file.substr(last_slash + 1);
    } else {
        basename = input_file;
    }

    if (last_dot != std::string::npos && last_dot > last_slash) {
        basename = basename.substr(0, last_dot - (last_slash != std::string::npos ? last_slash + 1 : 0));
    }

    return basename + suffix + ".json";
}

#endif // JSON_OUTPUT_H

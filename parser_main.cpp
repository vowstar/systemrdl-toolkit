#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "antlr4-runtime.h"
#include "SystemRDLLexer.h"
#include "SystemRDLParser.h"

using namespace antlr4;

// Recursive function to print AST (optimized alignment version)
void printAST(tree::ParseTree* tree, SystemRDLParser* parser, int depth = 0) {
    if (ParserRuleContext* ruleContext = dynamic_cast<ParserRuleContext*>(tree)) {
        std::string ruleName = parser->getRuleNames()[ruleContext->getRuleIndex()];
        std::string text = ruleContext->getText();

        // Print indentation
        auto indent = [depth]() {
            for (int i = 0; i < depth; i++) std::cout << "  ";
        };

        // Function to handle multi-line text alignment
        auto printAligned = [&](const std::string& prefix, const std::string& content) {
            indent();
            std::cout << prefix;

            // Calculate alignment position (indentation + prefix length)
            size_t alignPos = depth * 2 + prefix.length();
            std::string alignSpaces(alignPos, ' ');

            // Split text into lines
            std::istringstream iss(content);
            std::string line;
            bool firstLine = true;

            while (std::getline(iss, line)) {
                if (firstLine) {
                    std::cout << line << std::endl;
                    firstLine = false;
                } else {
                    std::cout << alignSpaces << line << std::endl;
                }
            }
        };

        // Different processing based on rule type
        if (ruleName == "component_named_def") {
            indent();
            std::cout << "📦 Component Definition" << std::endl;
        }
        else if (ruleName == "component_type_primary") {
            printAligned("🔧 Type: ", text);
        }
        else if (ruleName == "component_inst") {
            printAligned("📋 Instance: ", text);
        }
        else if (ruleName == "local_property_assignment") {
            printAligned("⚙️  Property: ", text);
        }
        else if (ruleName == "range_suffix") {
            printAligned("📏 Range: ", text);
        }
        else if (ruleName == "inst_addr_fixed") {
            printAligned("📍 Address: ", text);
        }

        // Recursively process child nodes
        for (size_t i = 0; i < ruleContext->children.size(); i++) {
            printAST(ruleContext->children[i], parser, depth + 1);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file.rdl>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];

    try {
        // Read input file
        std::ifstream stream(inputFile);
        if (!stream.is_open()) {
            std::cerr << "Error: Cannot open file " << inputFile << std::endl;
            return 1;
        }

        // Create ANTLR input stream
        ANTLRInputStream input(stream);

        // Create lexer
        SystemRDLLexer lexer(&input);

        // Create token stream
        CommonTokenStream tokens(&lexer);

        // Create parser
        SystemRDLParser parser(&tokens);

        // Parse, starting from root rule
        tree::ParseTree* tree = parser.root();

        // Check for syntax errors
        if (parser.getNumberOfSyntaxErrors() > 0) {
            std::cerr << "Syntax errors found: " << parser.getNumberOfSyntaxErrors() << std::endl;
        }

        // Print AST
        std::cout << "=== Abstract Syntax Tree ===" << std::endl;
        printAST(tree, &parser);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

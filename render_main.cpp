#include "SystemRDLLexer.h"
#include "SystemRDLParser.h"
#include "cmdline_parser.h"
#include "systemrdl_api.h"
#include <fstream>
#include <inja/inja.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace antlr4;

int main(int argc, char *argv[])
{
    // Setup command line parser
    CmdLineParser cmdline(
        "SystemRDL Template Renderer - Render SystemRDL designs using Jinja2 templates");
    cmdline.add_option("t", "template", "Jinja2 template file (.j2)", true);
    cmdline
        .add_option_with_optional_value("o", "output", "Output file (default: auto-generated name)");
    cmdline.add_option("v", "verbose", "Enable verbose output");
    cmdline.add_option("h", "help", "Show this help message");

    if (!cmdline.parse(argc, argv)) {
        return argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") ? 0
                                                                                               : 1;
    }

    const auto &args = cmdline.get_positional_args();
    if (args.empty()) {
        std::cerr << "Error: No input RDL file specified" << std::endl;
        cmdline.print_help();
        return 1;
    }

    if (!cmdline.is_set("template")) {
        std::cerr << "Error: Template file not specified (use -t/--template)" << std::endl;
        cmdline.print_help();
        return 1;
    }

    std::string rdl_file      = args[0];
    std::string template_file = cmdline.get_value("template");
    bool        verbose       = cmdline.is_set("verbose");

    if (verbose) {
        std::cout << "Processing RDL file: " << rdl_file << std::endl;
        std::cout << "Using template: " << template_file << std::endl;
    }

    try {
        // Use systemrdl_api.h to elaborate the RDL file and get JSON
        auto elaborate_result = systemrdl::file::elaborate(rdl_file);
        if (!elaborate_result.ok()) {
            std::cerr << "Elaboration failed: " << elaborate_result.error() << std::endl;
            return 1;
        }

        if (verbose) {
            std::cout << "Successfully elaborated SystemRDL design" << std::endl;
        }

        // Parse the JSON string to nlohmann::json for Inja
        json elaborated_json;
        try {
            elaborated_json = json::parse(elaborate_result.value());
        } catch (const json::parse_error &e) {
            std::cerr << "Failed to parse elaborated JSON: " << e.what() << std::endl;
            return 1;
        }

        if (verbose) {
            std::cout << "JSON structure preview:" << std::endl;
            std::cout << elaborated_json.dump(2).substr(0, 500) << "..." << std::endl;
        }

        // Setup Inja environment
        inja::Environment env;
        // Disable line statements
        env.set_line_statement("");
        // Render template
        std::string rendered_content = env.render_file(template_file, elaborated_json);

        if (verbose) {
            std::cout << "Successfully rendered template" << std::endl;
        }

        // Generate output filename if not specified
        std::string output_file = cmdline.get_value("output");
        if (output_file.empty()) {
            std::string rdl_basename = rdl_file.substr(rdl_file.find_last_of("/\\") + 1);
            size_t      dot_pos      = rdl_basename.find_last_of('.');
            if (dot_pos != std::string::npos) {
                rdl_basename.resize(dot_pos);
            }

            std::string template_basename = template_file.substr(
                template_file.find_last_of("/\\") + 1);

            // Extract purpose and extension from template name (e.g., test_j2_header.h.j2 -> header.h)
            size_t j2_pos = template_basename.find("_j2_");
            if (j2_pos != std::string::npos) {
                size_t start = j2_pos + 4;
                size_t end   = template_basename.find(".j2");
                if (end != std::string::npos) {
                    std::string purpose_and_ext = template_basename.substr(start, end - start);
                    output_file                 = rdl_basename + "_" + purpose_and_ext;
                }
            }

            if (output_file.empty()) {
                output_file = rdl_basename + "_rendered.txt";
            }
        }

        // Write output
        std::ofstream out_file(output_file);
        if (!out_file) {
            std::cerr << "Error: Cannot write to output file: " << output_file << std::endl;
            return 1;
        }

        out_file << rendered_content;
        out_file.close();

        if (verbose) {
            std::cout << "Output written to: " << output_file << std::endl;
        } else {
            std::cout << output_file << std::endl;
        }

        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

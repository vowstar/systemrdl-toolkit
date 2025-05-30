#include "cmdline_parser.h"
#include "systemrdl_version.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

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
    std::string description;
};

// CSV Parser class
class CSVParser
{
public:
    CSVParser() = default;

    // Parse CSV file and return rows
    std::vector<CSVRow> parse(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        std::vector<CSVRow> rows;
        bool                first_line = true;
        std::vector<int>    column_mapping;

        // Read all content and parse multiline CSV
        std::string content;
        std::string temp_line;
        while (std::getline(file, temp_line)) {
            content += temp_line + "\n";
        }

        // Parse CSV content line by line, handling multiline quoted fields
        std::vector<std::string> csv_lines = parse_csv_content(content);

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
           "description"};

    // Detect CSV delimiter (, or ;)
    char detect_delimiter(const std::string &line)
    {
        size_t comma_count     = std::count(line.begin(), line.end(), ',');
        size_t semicolon_count = std::count(line.begin(), line.end(), ';');
        return semicolon_count > comma_count ? ';' : ',';
    }

    // Split CSV line respecting quoted fields
    std::vector<std::string> split_csv_line(const std::string &line, char delimiter)
    {
        std::vector<std::string> fields;
        std::string              current_field;
        bool                     in_quotes = false;

        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];

            if (c == '"') {
                if (in_quotes && i + 1 < line.length() && line[i + 1] == '"') {
                    // Double quote - escaped quote within quoted field
                    current_field += '"';
                    i++; // Skip the next quote
                } else {
                    // Toggle quote state
                    in_quotes = !in_quotes;
                }
            } else if (c == delimiter && !in_quotes) {
                fields.push_back(current_field); // Don't trim here, let field processors handle it
                current_field.clear();
            } else {
                current_field += c;
            }
        }

        fields.push_back(current_field); // Don't trim here either
        return fields;
    }

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

    // Process name fields (remove all newlines and trim)
    std::string process_name_field(const std::string &str)
    {
        if (str.empty())
            return str;
        std::string trimmed = trim(str);
        return remove_all_newlines(trimmed);
    }

    // Process regular fields (just trim)
    std::string process_regular_field(const std::string &str) { return trim(str); }

    // Convert to lowercase
    std::string to_lower(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
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

    // Create column mapping from header to standard columns
    std::vector<int> create_column_mapping(const std::vector<std::string> &headers)
    {
        std::vector<int> mapping;

        std::cout << "📋 Column mapping:" << std::endl;
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
                row.addrmap_offset = process_regular_field(value);
                break;
            case 1:
                row.addrmap_name = process_name_field(value);
                break;
            case 2:
                row.reg_offset = process_regular_field(value);
                break;
            case 3:
                row.reg_name = process_name_field(value);
                break;
            case 4:
                row.reg_width = process_regular_field(value);
                break;
            case 5:
                row.field_name = process_name_field(value);
                break;
            case 6:
                row.field_lsb = process_regular_field(value);
                break;
            case 7:
                row.field_msb = process_regular_field(value);
                break;
            case 8:
                row.reset_value = process_regular_field(value);
                break;
            case 9:
                row.sw_access = process_regular_field(value);
                break;
            case 10:
                row.hw_access = process_regular_field(value);
                break;
            case 11:
                row.description = process_description(value);
                break;
            }
        }

        return row;
    }

    // Parse CSV content handling multiline quoted fields
    std::vector<std::string> parse_csv_content(const std::string &content)
    {
        std::vector<std::string> lines;
        std::string              current_line;
        bool                     in_quotes = false;

        for (size_t i = 0; i < content.length(); ++i) {
            char c = content[i];

            if (c == '"') {
                in_quotes = !in_quotes;
                current_line += c;
            } else if (c == '\n' && !in_quotes) {
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
};

// RDL Generator class
class RDLGenerator
{
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
                    rdl << "            sw = " << to_lower(row.sw_access) << ";\n";
                }
                if (!row.hw_access.empty()) {
                    rdl << "            hw = " << to_lower(row.hw_access) << ";\n";
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
                result += "\\\"";
            } else if (c == '\\') {
                result += "\\\\";
            } else {
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
};

int main(int argc, char *argv[])
{
    // Setup command line parser
    CmdLineParser cmdline(
        "CSV to SystemRDL Converter - Convert CSV register definitions to SystemRDL format");
    cmdline.set_version(systemrdl::get_detailed_version());
    cmdline.add_option_with_optional_value("o", "output", "Output RDL file (default: <input>.rdl)");
    cmdline.add_option("h", "help", "Show this help message");

    if (!cmdline.parse(argc, argv)) {
        return argc == 2
                       && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"
                           || std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v")
                   ? 0
                   : 1;
    }

    const auto &args = cmdline.get_positional_args();
    if (args.empty()) {
        std::cerr << "Error: No input CSV file specified" << std::endl;
        cmdline.print_help();
        return 1;
    }

    std::string input_file  = args[0];
    std::string output_file = cmdline.get_value("output");

    // Generate default output filename if not specified
    if (output_file.empty()) {
        size_t dot_pos = input_file.find_last_of('.');
        if (dot_pos != std::string::npos) {
            output_file = input_file.substr(0, dot_pos) + ".rdl";
        } else {
            output_file = input_file + ".rdl";
        }
    }

    try {
        std::cout << "🔧 Parsing CSV file: " << input_file << std::endl;

        // Parse CSV
        CSVParser parser;
        auto      rows = parser.parse(input_file);

        std::cout << "✅ Successfully parsed " << rows.size() << " rows" << std::endl;

        // Generate RDL
        std::cout << "\n🚀 Generating SystemRDL output..." << std::endl;

        RDLGenerator generator;
        std::string  rdl_content = generator.generate(rows);

        // Write output file
        std::ofstream output_stream(output_file);
        if (!output_stream.is_open()) {
            std::cerr << "Error: Cannot create output file " << output_file << std::endl;
            return 1;
        }

        output_stream << rdl_content;
        output_stream.close();

        std::cout << "✅ SystemRDL file generated: " << output_file << std::endl;
        std::cout << "\n🎉 Conversion completed successfully!" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

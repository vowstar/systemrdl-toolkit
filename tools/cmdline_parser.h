#ifndef CMDLINE_PARSER_H
#define CMDLINE_PARSER_H

#include <iostream>
#include <map>
#include <string>
#include <vector>

class CmdLineParser
{
public:
    explicit CmdLineParser(const std::string &description = "")
        : description_(description)
        , version_string_("")
    {}

    void set_version(const std::string &version) { version_string_ = version; }

    void add_option(
        const std::string &short_opt,
        const std::string &long_opt,
        const std::string &help,
        bool               has_value     = false,
        const std::string &default_value = "")
    {
        Option opt;
        opt.short_opt          = short_opt;
        opt.long_opt           = long_opt;
        opt.help               = help;
        opt.has_value          = has_value;
        opt.has_optional_value = false;
        opt.default_value      = default_value;
        opt.is_set             = false;
        options_.push_back(opt);
    }

    void add_option_with_optional_value(
        const std::string &short_opt,
        const std::string &long_opt,
        const std::string &help,
        const std::string &default_value = "")
    {
        Option opt;
        opt.short_opt          = short_opt;
        opt.long_opt           = long_opt;
        opt.help               = help;
        opt.has_value          = true;
        opt.has_optional_value = true;
        opt.default_value      = default_value;
        opt.is_set             = false;
        options_.push_back(opt);
    }

    bool parse(int argc, const char *const argv[])
    {
        program_name_ = argv[0];

        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                print_help();
                return false;
            }

            if (arg == "--version" || arg == "-v") {
                print_version();
                return false;
            }

            bool option_found = false;

            // Check for --option=value format
            size_t eq_pos = arg.find('=');
            if (eq_pos != std::string::npos) {
                std::string option_part  = arg.substr(0, eq_pos);
                std::string option_value = arg.substr(eq_pos + 1);

                for (auto &opt : options_) {
                    if (option_part == "--" + opt.long_opt || option_part == "-" + opt.short_opt) {
                        opt.is_set   = true;
                        opt.value    = option_value;
                        option_found = true;
                        break;
                    }
                }
            }
            // Check for short and long options
            else {
                for (auto &opt : options_) {
                    if (arg == "--" + opt.long_opt || arg == "-" + opt.short_opt) {
                        opt.is_set = true;
                        if (opt.has_value && !opt.has_optional_value) {
                            // Mandatory value
                            if (i + 1 >= argc) {
                                std::cerr << "Error: Option " << arg << " requires a value"
                                          << std::endl;
                                return false;
                            }
                            opt.value = argv[++i];
                        } else if (opt.has_optional_value) {
                            // Optional value - check if next arg is a value or another option
                            if (i + 1 < argc && argv[i + 1][0] != '-') {
                                opt.value = argv[++i];
                            } else {
                                opt.value = opt.default_value; // Use default if no value provided
                            }
                        }
                        option_found = true;
                        break;
                    }
                }
            }

            if (!option_found) {
                // Assume it's a positional argument
                positional_args_.push_back(arg);
            }
        }

        return true;
    }

    bool is_set(const std::string &long_opt) const
    {
        for (const auto &opt : options_) {
            if (opt.long_opt == long_opt) {
                return opt.is_set;
            }
        }
        return false;
    }

    std::string get_value(const std::string &long_opt) const
    {
        for (const auto &opt : options_) {
            if (opt.long_opt == long_opt) {
                return opt.is_set ? opt.value : opt.default_value;
            }
        }
        return "";
    }

    const std::vector<std::string> &get_positional_args() const { return positional_args_; }

    void print_help() const
    {
        std::cout << description_ << std::endl;

        // Extract program name from path
        std::string prog_name  = program_name_;
        size_t      last_slash = prog_name.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            prog_name = prog_name.substr(last_slash + 1);
        }

        std::cout << "\nUsage: " << prog_name << " <input_file.rdl> [options]" << std::endl;
        std::cout << "\nOptions:" << std::endl;

        for (const auto &opt : options_) {
            if (opt.short_opt.empty()) {
                std::cout << "      --" << opt.long_opt;
            } else {
                std::cout << "  -" << opt.short_opt << ", --" << opt.long_opt;
            }
            if (opt.has_value && !opt.has_optional_value) {
                std::cout << " <value>";
            } else if (opt.has_optional_value) {
                std::cout << "[=<value>]";
            }
            std::cout << "\t" << opt.help;
            if (!opt.default_value.empty()) {
                std::cout << " (default: " << opt.default_value << ")";
            }
            std::cout << std::endl;
        }
    }

    void print_version() const
    {
        if (!version_string_.empty()) {
            std::cout << version_string_ << std::endl;
        } else {
            // Extract program name from path
            std::string prog_name  = program_name_;
            size_t      last_slash = prog_name.find_last_of("/\\");
            if (last_slash != std::string::npos) {
                prog_name = prog_name.substr(last_slash + 1);
            }
            std::cout << prog_name << " version information not available" << std::endl;
        }
    }

private:
    struct Option
    {
        std::string short_opt;
        std::string long_opt;
        std::string help;
        bool        has_value;
        bool        has_optional_value;
        std::string default_value;
        bool        is_set;
        std::string value;
    };

    std::string              description_;
    std::string              program_name_;
    std::string              version_string_;
    std::vector<Option>      options_;
    std::vector<std::string> positional_args_;
};

#endif // CMDLINE_PARSER_H

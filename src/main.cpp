#include <iostream>
#include <fstream>
#include <string>
#include "json.hpp"
#include "yaml.hpp"
#include "minja.hpp"
#include <sstream>

using json = nlohmann::ordered_json;

void print_help() {
    std::cout << "Usage: cminja <flags>\n"
              << "\t-h prints help\n"
              << "\t-v prints versions\n"
              << "\t-j json\n"
              << "\t-y yaml\n"
              << "\t-i path to minja file\n"
              << "\t-d path to data\n"
              << "\t-s read data from stdin\n"
              << "\t-o save to file\n";
}

void print_version() {
    std::cout << "cMinja v1.0.0\nlightweight-yaml-parser v1.0.0\nminja 78bf4a5\nnlohmann/json v3.11.3";
}

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    return std::string(std::istreambuf_iterator<char>(file),
                      std::istreambuf_iterator<char>());
}

std::string read_stdin() {
    return std::string(std::istreambuf_iterator<char>(std::cin),
                      std::istreambuf_iterator<char>());
}

void write_output(const std::string& content, const std::string& output_path) {
    if (!output_path.empty()) {
        std::ofstream out(output_path);
        if (!out.is_open()) {
            throw std::runtime_error("Could not open output file: " + output_path);
        }
        out << content;
    } else {
        std::cout << content;
    }
}

int main(int argc, char* argv[]) {
    bool use_json = false;
    bool use_yaml = false;
    bool use_stdin = false;
    std::string template_path;
    std::string data_path;
    std::string output_path;

    // CLI Args.
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg[0] == '-') {
            for (size_t j = 1; j < arg.length(); j++) {
                switch (arg[j]) {
                    case 'h':
                        print_help();
                        return 0;
                    case 'v':
                        print_version();
                        return 0;
                    case 'j':
                        use_json = true;
                        break;
                    case 'y':
                        use_yaml = true;
                        break;
                    case 's':
                        use_stdin = true;
                        break;
                    case 'i':
                        if (j == arg.length() - 1 && i + 1 < argc) {
                            template_path = argv[++i];
                        } else {
                            std::cerr << "Error: -i requires a path argument\n";
                            return 1;
                        }
                        break;
                    case 'd':
                        if (j == arg.length() - 1 && i + 1 < argc) {
                            data_path = argv[++i];
                        } else {
                            std::cerr << "Error: -d requires a path argument\n";
                            return 1;
                        }
                        break;
                    case 'o':
                        if (j == arg.length() - 1 && i + 1 < argc) {
                            output_path = argv[++i];
                        } else {
                            std::cerr << "Error: -o requires a path argument\n";
                            return 1;
                        }
                        break;
                    default:
                        std::cerr << "Error: Unknown flag -" << arg[j] << "\n";
                        print_help();
                        return 1;
                }
            }
        } else {
            std::cerr << "Error: Arguments must start with -\n";
            print_help();
            return 1;
        }
    }

    // Arg validations.
    if (!use_json && !use_yaml) {
        std::cerr << "Error: Must specify either -j for JSON or -y for YAML\n";
        return 1;
    }
    if (use_json && use_yaml) {
        std::cerr << "Error: Cannot specify both JSON and YAML\n";
        return 1;
    }
    if (template_path.empty()) {
        std::cerr << "Error: Must specify template path with -i\n";
        return 1;
    }
    if (!use_stdin && data_path.empty()) {
        std::cerr << "Error: Must specify data path with -d or use -s for stdin\n";
        return 1;
    }

    try {
        std::string template_content = read_file(template_path);
        
        std::string data_content;
        if (use_stdin) {
            data_content = read_stdin();
        } else {
            data_content = read_file(data_path);
        }

        nlohmann::ordered_json data;
        if (use_json) {
            data = json::parse(data_content);
        } else {
            // Parse YAML and convert to JSON.
            data = nlohmann::ordered_json::object();
            
            // Create temporary file for stdin if needed, I chose the wrong library lol.
            std::string yaml_path = use_stdin ? "temp.yaml" : data_path;
            if (use_stdin) {
                std::ofstream temp(yaml_path);
                temp << data_content;
                temp.close();
            }

            yaml::YAML yaml_file(yaml_path);

            // Helper function to convert YAML value to JSON, fml.
            std::function<json(yaml::TypedValue&)> convert_value;
            convert_value = [&convert_value](yaml::TypedValue& value) -> json {
                switch (value.type) {
                    case yaml::YAMLType::Int_:
                        return value.cast<int>();
                    case yaml::YAMLType::Double_:
                        return value.cast<double>();
                    case yaml::YAMLType::String_:
                        return value.cast<std::string>();
                    case yaml::YAMLType::Bool_:
                        return value.cast<bool>();
                    case yaml::YAMLType::Array_: {
                        auto arr = value.cast<yaml::Array>();
                        json result = json::array();
                        for (auto& elem : arr) {
                            result.push_back(convert_value(elem));
                        }
                        return result;
                    }
                    case yaml::YAMLType::Object_: {
                        auto obj = value.cast<yaml::Object>();
                        json result = json::object();
                        for (auto& [k, v] : obj) {
                            result[k] = convert_value(v);
                        }
                        return result;
                    }
                    default:
                        return nullptr;
                }
            };

            // Convert YAML to JSON.
            for (auto& [key, value] : yaml_file.data) {
                data[key] = convert_value(value);
            }

            // Remove the temporary file if used, lmao.
            if (use_stdin) {
                std::remove(yaml_path.c_str());
            }
        }

        // Process the template.
        minja::Options options;
        options.trim_blocks = true;
        options.lstrip_blocks = true;
        auto tmpl = minja::Parser::parse(template_content, options);
        auto context = minja::Context::make(minja::Value(data));
        std::ostringstream out;
        tmpl->render(out, context);
        std::string result = out.str();

        write_output(result, output_path);

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

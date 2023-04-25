#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <regex>

using namespace std;

void print_usage() {
    std::cout << "Usage: ascii_art [mask_file] [source_file]\n";
}

int count_char(std::ifstream& file, char c) {
    int count = 0;
    char current_char;
    while (file.get(current_char)) {
        if (current_char == c) {
            count++;
        }
    }
    return count;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage();
        return 1;
    }

    std::ifstream mask_file(argv[1]);
    std::ifstream source_file(argv[2]);

    if (!mask_file || !source_file) {
        std::cerr << "Error opening files\n";
        return 1;
    }

    int mask_width = 0;
    int mask_height = 0;
    std::string mask_line;
    while (std::getline(mask_file, mask_line)) {
        int line_length = mask_line.length();
        if (line_length > mask_width) {
            mask_width = line_length;
        }
        mask_height++;
    }
    mask_file.clear();
    mask_file.seekg(0, std::ios::beg);

    std::string source_str;
    source_file.seekg(0, std::ios::end);
    source_str.reserve(source_file.tellg());
    source_file.seekg(0, std::ios::beg);
    source_str.assign((std::istreambuf_iterator<char>(source_file)),
        std::istreambuf_iterator<char>());
    source_file.clear();
    source_file.seekg(0, std::ios::beg);

    source_str.erase(remove(source_str.begin(), source_str.end(), '\t'), source_str.end());

    source_str.erase(remove(source_str.begin(), source_str.end(), '\n'), source_str.end());

    source_str.erase(remove(source_str.begin(), source_str.end(), '\r'), source_str.end());

    source_str = regex_replace(source_str, regex("\\s{2,}"), " ");

    int source_length = source_str.length();

    int num_hash = count_char(mask_file, '#');
    mask_file.clear();
    mask_file.seekg(0, std::ios::beg);
    if (num_hash < source_length) {
        std::cerr << "Error: Source code too long for mask\n";
        return 1;
    }

    int source_index = 0;
    std::string result_line;
    while (std::getline(mask_file, mask_line)) {
        int line_length = mask_line.length();
        for (int i = 0; i < line_length; i++) {
            if (mask_line[i] == '#') {
                if (source_index < source_length) {
                    result_line += source_str[source_index++];
                }
                else {
                    result_line += ' ';
                }
            }
            else {
                result_line += mask_line[i];
            }
        }
        std::cout << result_line << std::endl;
        result_line.clear();
    }

    mask_file.close();
    source_file.close();

    return 0;
}

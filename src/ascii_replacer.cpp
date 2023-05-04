#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <regex>

void print_usage() {
    printf("Usage: ascii_art [mask_file] [source_file]\n"); 
    // cout is an eyesore and i refuse to use it 
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
    source_str.assign((std::istreambuf_iterator<char>(source_file)), std::istreambuf_iterator<char>());
    source_file.clear();
    source_file.seekg(0, std::ios::beg);

    // removing any whitespaces present in code
    source_str.erase(remove(source_str.begin(), source_str.end(), '\t'), source_str.end());
    source_str.erase(remove(source_str.begin(), source_str.end(), '\n'), source_str.end());
    source_str.erase(remove(source_str.begin(), source_str.end(), '\r'), source_str.end());

    // reduce n space characters to just one
    source_str = regex_replace(source_str, std::regex("\\s{2,}"), " ");

    int source_length = source_str.length();
    int num_hash = count_char(mask_file, '#');
    mask_file.clear();
    mask_file.seekg(0, std::ios::beg);
    if (num_hash < source_length) {
        std::cerr << "Error: Source code too long for mask\n";
        return 1;
    }




    int source_index = 0;
    int mask_index = 0;

    std::string result_line; // this should be a result string // to be removed

    std::string shaped_output; // this is where the output goes
    
    // load the mask from a file into a string
    std::stringstream buffer;
    buffer << mask_file.rdbuf();
    std::string mask_str;
    mask_str = buffer.str();
    int source_code_index;

    for(auto maskchar : mask_str) {

        // look ahead in mask and find number until next ' '
        constexpr int mask_current_remaining_word_length =
            mask_str.find(' ', maskchar.index());
        // compare to source_code_str and look if the next word fits in
        constexpr int source_code_current_remaining_word_length =
                source_str.find(' ', source_code_index);
            count_char(source_str.substr(source_code_index), ' ');
            /* this can be expanded to include other opportunities to split 
            *  the source code tokens
            *  space characters are obviously earsed from the source code as
            *  this would othewise lead to concatenation
            *  for something like "func()" the split to "func ()" is optional
            *  if the source code contains "func ()" it can obviously be merged together
            *  but that is harder to implement
            */
        
        // if not call edge case function with the string and position 
        
        // and try to fill in a comment this can also fail as the comment may reduce
        // availible characters which may result in the string not fitting in       
    }
    

    [[deprecated]] {
        while (std::getline(mask_file, mask_line)) {
    
        int line_length = mask_line.length(); // no
    
        for (int i = 0; i < line_length; i++) { // do for global
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
        std::cout << result_line << "\n"; // :|
        result_line.clear();
        }
    }

    mask_file.close();
    source_file.close();

    return 0;
}

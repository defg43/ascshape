#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <regex>
#include <assert.h>

#define DEBUG 0

#if DEBUG == 1
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...) 
#endif

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
        std::cerr << (!mask_file) << " and the source is " << (!mask_file) << '\n';
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

    printf(source_str.c_str());
    printf("\n");
    shaped_output = mask_str;

    // main replacing logic
    for(int index = 0; index < mask_str.length(); index++) {
        // if a newline is encountered in the mask copy it over to the output
        if (mask_str[index] == '\n') {
                shaped_output[index] = '\n';
                index++;
                debug("[DEBUG] whitespace, skipping\n");
            }
        
        // look ahead in mask and find number until next ' '
        // this is the space that is still avaible in the mask
        int mask_word_length =
            std::min(mask_str.find(' ', index), mask_str.find('\n', index)) - index;
        // get length of the next token from the source code
        int source_code_word_length = (source_str[source_code_index] == ' ')?
            source_str.find(' ', source_code_index + 1) - source_code_index:
            source_str.find(' ', source_code_index) - source_code_index;
        
        // if DEBUG is set to 1 this just logs indexes
        debug("[DEBUG] index: %d, word_lenght: %d, src_word_len: %d, src_index: %d, src_len: %d\n", 
                index, mask_word_length, source_code_word_length, 
                source_code_index, source_str.length());

            // this means we havent reached the end of the source code
            if(source_code_index < source_str.length()) {
                // try to fit source code token into mask
                if (mask_word_length >= source_code_word_length) { 
                // enough space for token
                     // copy from src to ouput
                    shaped_output[index] = source_str[source_code_index]; 
                    source_code_index++;
                } else { // not enough space for token handle edge case here

                    debug("[DEBUG] mask: %d, src: %d ", mask_word_length, source_code_word_length);
                    // edgecase function here and check if we are inside of a comment
                    debug("[DEBUG] we had %d places left\n", mask_word_length);
                    bool we_have_more_problems = false;
                    switch (mask_word_length) {
                    
                    case 1:
                        shaped_output[index] = ' ';
                        index++;
                        break;
                    #if 0
                    case 2:
                        shaped_output[index] = '/';
                        shaped_output[index + 1] = '*';
                        index += 2;
                        we_have_more_problems = false;
                        printf("\n\nentering loop\n\n");
                        do {
                            while(mask_str[index] == ' ' || mask_str[index] == '\n' 
                                && index < mask_str.length()) {
                                index += 2;
                            }
                            if(index == mask_str.length()) {
                                std::cerr << "exceded lenght" << '\n';
                                exit(EXIT_FAILURE);
                            }
                            int section_lenght = mask_str.find(index, ' ') - index;
                            assert(section_lenght > 0);

                            if (source_str[source_code_index] == ' ') source_code_index++;
                            int next_token_lenght = source_str.find(source_code_index, ' ') -
                                source_code_index + 2; // +2 because we want to fir "* /"

                            if (section_lenght >= next_token_lenght) {
                                shaped_output[index] = '*';
                                shaped_output[index + 1] = '/';
                                index += 2; 
                            } else {
                                // fill section with comments *
                                while (mask_str[index] == '#')
                                {
                                    if (mask_str[index] != '\n') 
                                        shaped_output[index] = '*';                                
                                    index++;                             
                                }  
                                we_have_more_problems = true;                   
                            }
                        } while(we_have_more_problems);
                        break;
                    #endif
                    default:
                        break;
                    }
                } 
            }

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
    // when done output the string to the terminal
    printf("\n[RESULT] result: \n%s", shaped_output.c_str());

    // cleanup
    mask_file.close();
    source_file.close();

    return 0;
}

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <regex>
#include <assert.h>

#define DEBUG 1

#if DEBUG == 1
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...) 
#endif
typedef struct token {
    std::string token;
    int count;
    bool valid;
}token;
typedef struct globalvars {
    std::string src;
        int src_ind;
        int src_len;
    std::string mask;
        int mask_ind;
        int mask_len;
        int mask_slot_amount;
    std::string output;
        int output_ind;
    std::string current_token;
        int token_len;
        int token_pos;
    int newline_distance;
    int slotend_distance;
    bool outofspace;
} globalvars;

#pragma region util_functions

void print_usage() {
    printf("Usage: ascii_art [mask_file] [source_file]\n"); 
    // cout is an eyesore and i refuse to use it 
}
int count_char(const std::string& str, char c) {
    int count = 0;
    for (char current_char : str) {
        if (current_char == c) {
            count++;
        }
    }
    return count;
}

token pop_token(std::string& input) {
    token info;
    info.count = 0;
    info.valid = false;    
    // Remove leading spaces
    while (!input.empty() && input[0] == ' ') {
        input.erase(0, 1);
    }    
    // Check if input is empty
    if (input.empty()) {
        return info;
    }    
    // Extract token
    size_t token_end = input.find(' ');
    if (token_end == std::string::npos) {
        info.token = input;
        input.clear();
    } else {
        info.token = input.substr(0, token_end);
        input.erase(0, token_end);
    }
    // Remove trailing spaces
    while (!info.token.empty() && info.token.back() == ' ') {
        info.token.pop_back();
    }
    // Count characters in token
    for (char c : info.token) {
        if (c != ' ') {
            info.count++;
        }
    }    
    // Set valid flag
    info.valid = true;    
    return info;
}

int getNextTokenLength(const std::string& str, int index) {
    int start = -1, end = -1;
    int len = str.length();
    for (int i = index; i < len; i++) {
        if (str[i] == ' ') {
            if (start >= 0 && end < 0) {
                end = i;
            }
        }
        else {
            if (start < 0) {
                start = i;
            }
        }
        if (i == len - 1 && start >= 0 && end < 0) {
            end = len;
        }
        if (start >= 0 && end >= 0) {
            return end - start;
        }
    }
    return -1; // No more tokens found
}

int nextTokenStart(const std::string& str, int startIndex) {
    // Skip any spaces before the next token
    while (startIndex < str.length() && std::isspace(str[startIndex])) {
        ++startIndex;
    }
    // Find the start of the next token
    while (startIndex < str.length() && !std::isspace(str[startIndex])) {
        ++startIndex;
    }
    // Skip any spaces after the current token
    while (startIndex < str.length() && std::isspace(str[startIndex])) {
        ++startIndex;
    }
    return startIndex == str.length() ? -1 : startIndex;
}

bool insertcomment(std::string &output_str, std::string &mask_str, std::string &src_str, 
                    int &mask_ind, int &src_ind) {
    assert(src_str.length() - 1 > src_ind);
    // start comment, we can assume there is ineeded enough space
    // insert comment start
    output_str[mask_ind] = '/';
    mask_ind++;
    output_str[mask_ind] = '*';
    mask_ind++;
    while (mask_ind < mask_str.length()) {
        if(mask_str[mask_ind] == ' ') // || mask_str[mask_ind] == '\n') // just skip
            mask_ind++;
        else { // we have reached a section where code can be placed, check if we can stop comment here
            bool can_finish_comment = getNextTokenLength(mask_str, mask_ind - 1) >= getNextTokenLength(src_str, src_ind - 1) + 2;
            if(can_finish_comment) {
                output_str[mask_ind] = '*';
                output_str[mask_ind + 1] = '/';
                mask_ind++;
                exit(EXIT_SUCCESS);
                return true; 
            } else { // we are trapped on an "island" and need to fill it with *
                while (mask_ind < mask_str.length() && src_ind < src_str.length() && mask_str[mask_ind] != '\0' && mask_str[mask_ind] != ' ') // && mask_str[mask_ind] != '\n')
                {
                    mask_str[mask_ind] = '*';
                    mask_ind++;
                }
                if(mask_ind >= mask_str.length() || src_ind >= src_str.length()) {
                    exit(EXIT_FAILURE);
                    return false;
                }
            }
        }
    }
    return false;
}

#pragma endregion

#pragma region replacer
bool replace(globalvars &vars) {
    vars.mask_ind = 0;
    vars.output_ind = 0;
    token current_token = {};
    bool valid = false;
    bool can_continue = false;

    do {
        vars.newline_distance = vars.mask.find('\n', vars.mask_ind);
        vars.slotend_distance = vars.mask.find(' ', vars.mask_ind);
        current_token = pop_token(vars.src);
        valid = current_token.valid;

    } while(can_continue);
    return false;
} 
#pragma endregion

#pragma region main
int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage();
        return 1;
    }

    std::ifstream mask_file(argv[1]);
    std::ifstream source_file(argv[2]);

    if (!mask_file || !source_file) {
        std::cerr << "Error opening files\n";
        std::cerr << (!mask_file) << " and the source is " << (!source_file) << '\n';
        return 1;
    }

    globalvars vars = {};

    std::stringstream mask_buffer;
    mask_buffer << mask_file.rdbuf();
    vars.mask = mask_buffer.str();

    std::stringstream src_buffer;
    src_buffer << src_file.rdbuf();
    vars.src = src_buffer.str();

    // format the source code properly
    vars.src.erase(remove(vars.src.begin(), vars.src.end(), '\t'), vars.src.end());
    vars.src.erase(remove(vars.src.begin(), vars.src.end(), '\n'), vars.src.end());
    vars.src.erase(remove(vars.src.begin(), vars.src.end(), '\r'), vars.src.end());
    vars.src = regex_replace(vars.src, std::regex("\\s{2,}"), " ");

    // init vars
    vars.mask_slot_amount = count_char(vars.mask, '#');
    vars.mask_len = vars.mask.length();
    vars.src_len = vars.src.length();
    vars.outofspace = vars.src_len > vars.mask_slot_amount;

    if (vars.outofspace) {
        printf("not enough space\n");
        exit(EXIT_FAILURE);
    }

    debug("[DEBUG]\n%s\n", vars.src);
    debug("[DEBUG]\n%s\n", vars.mask);
    debug("[DEBUG]\n%s\n", vars.output);

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
            if(source_code_index < source_str.length() - 1) {
                // try to fit source code token into mask
                if (mask_word_length >= source_code_word_length) { 
                // enough space for token
                    // copy from src to ouput
                    source_code_index++;
                    shaped_output[index] = source_str[source_code_index];
                    
                    
                } else { // not enough space for token handle edge case here

                    debug("[DEBUG] mask: %d, src: %d ", mask_word_length, source_code_word_length);
                    // edgecase function here and check if we are inside of a comment
                    debug("[DEBUG] we had %d places left\n", mask_word_length);
                    bool we_have_more_problems = false;
                    switch (mask_word_length) {
                    
                    case 1:
                        shaped_output[index] = '@';
                        // index++;
                        break;
                    case 2:
                        /*
                        shaped_output[index] = '/';
                        shaped_output[index + 1] = '*';
                        index++;*/
                        insertcomment(shaped_output, mask_str, source_str, index, source_code_index);
                        break;
                        /*
                    case 3: 
                        shaped_output[index] = '/';
                        shaped_output[index + 1] = '*';
                        shaped_output[index + 2] = '*';
                        index += 2;
                        // finishcomment();
                        break;
                        */
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
#pragma endregion

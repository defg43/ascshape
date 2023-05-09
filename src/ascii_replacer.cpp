#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <regex>
#include <assert.h>
#include <stdint.h>

#define DEBUG 1

#if DEBUG == 1
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...) 
#endif
typedef struct token {
    std::string data;
    int len;
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
    token current_token;
    int newline_distance;
    int slotend_distance;
    bool outofspace;
} globalvars;

#pragma region function_prototypes
    bool parsingEdgecase(globalvars &vars);
    token generateCommentToken(uint32_t comment_length, bool end_of_line = false);
#pragma endregion

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
    info.len = 0;
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
        info.data = input;
        input.clear();
    } else {
        info.data = input.substr(0, token_end);
        input.erase(0, token_end);
    }
    // Remove trailing spaces
    while (!info.data.empty() && info.data.back() == ' ') {
        info.data.pop_back();
    }
    // Count characters in token
    for (char c : info.data) {
        if (c != ' ') {
            info.len++;
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
    bool ok_to_continue = false;
    bool token_fits = false;

    vars.newline_distance = vars.mask.find('\n', vars.mask_ind);
    vars.slotend_distance = vars.mask.find(' ', vars.mask_ind);

    do {
        vars.current_token = pop_token(vars.src);

        vars.newline_distance = vars.mask.find('\n', vars.mask_ind) - vars.mask_ind;
        vars.slotend_distance = std::min(
                vars.mask.find(' ', vars.mask_ind) - vars.mask_ind,
                vars.mask.find('\n', vars.mask_ind) - vars.mask_ind);

        token_fits = vars.current_token.len <= vars.slotend_distance;
        
        printf("token fits: %d, remaining slot: %d, newline_in: %d, mask_ind: %d\n", 
                token_fits, vars.slotend_distance, vars.newline_distance, 
                vars.mask_ind);
        
        vars.outofspace = vars.mask_ind >= vars.mask_len;
        
        ok_to_continue = token_fits && vars.current_token.valid &&
                         !vars.outofspace;

        if(token_fits) {
            vars.output += vars.current_token.data;
            vars.mask_ind += vars.current_token.len;

            vars.newline_distance = vars.mask.find('\n', vars.mask_ind);
            vars.slotend_distance = vars.mask.find(' ', vars.mask_ind);
        } else {
            // token obviously doesnt fit
            // handle edge case
            // take along the entire context
            vars.outofspace = true;
            bool edgecase_success = false;
            edgecase_success = parsingEdgecase(vars);
            if(!edgecase_success) {
                printf("there was an error while dealing with an edgecase: \n");
                printf("\033[1;31m%s\033[0m", vars.output.c_str());
                printf("%s", vars.mask.substr(vars.mask_ind + 1).c_str());
                exit(EXIT_FAILURE);
            }
            debug("[DEBUG]\n%s", vars.output.c_str());
            // todo pull forward string

            // resume loop since edgecase got handled
            vars.newline_distance = vars.mask.find('\n', vars.mask_ind);
            vars.slotend_distance = vars.mask.find(' ', vars.mask_ind);
            token_fits = vars.current_token.len <= vars.slotend_distance;
            vars.outofspace = vars.mask_ind >= vars.mask_len;    
            ok_to_continue = token_fits && vars.current_token.valid &&
                                                    !vars.outofspace;
        }

        // find way to insert string

    } while(ok_to_continue);

    return false;
} 

bool parsingEdgecase(globalvars &vars) {
    debug("[DEBUG]\noutput:\n%s\n", vars.output.c_str());
    // establish if we are on the end of a line
    bool end_of_line = false;
    assert(vars.newline_distance >= vars.slotend_distance);
    end_of_line = vars.newline_distance == vars.slotend_distance;

    // generate comment token
    token comment = generateCommentToken(vars.slotend_distance, end_of_line);
    if(comment.valid) {
        vars.output += comment.data;
        vars.mask_ind += comment.len;
        return true;
    // insert comment token and append whatever whitespace is neccesary
    }
    return false;
}

[[nodiscard]] token generateCommentToken(uint32_t comment_length, bool end_of_line) {
    union switcher_util
    {
        struct { uint32_t part1; uint32_t part2; };
        struct { uint64_t required_token_type; };
    };
    switcher_util switcher = {
        .part1 = comment_length,
        .part2 = end_of_line 
    }; 

    printf("\n\ncomment_length is 0x%016X\n", (switcher.part1));
    printf("end_of_line is 0x%016X\n", (switcher.part2));
    printf("the switcher(value) is 0x%016lX\n", (switcher.required_token_type));
    
    token comment;
    switch(switcher.required_token_type) {
        case 0x0000'0001'0000'0000:
            comment = { 
                .data = "\n",
                .len = 1,
                .valid = true 
            };
            break;
        case 0x0000'0001'0000'0001:
            comment = { 
                .data = " \n",
                .len = 2,
                .valid = true 
            };
            break;
        case 0x0000'0001'0000'0002:
            comment = { 
                .data = "//\n",
                .len = 3,
                .valid = true 
            };
            break;
        case 0x0000'0001'0000'0003:
            comment = { 
                .data = " //\n",
                .len = 4,
                .valid = true 
            };
            break;
        case 0x0000'0001'0000'0004 ... 0x0000'0001'FFFF'FFFF:
            comment = { 
                .data = "/*" + std::string(comment_length - 4, '*') + "*/\n",
                .len = comment_length + 1,
                .valid = true 
            };
            break;
        case 0x0000000000000000:
            comment = { 
                .data = " ",
                .len = 1,
                .valid = true 
            };
            break;
        case 0x0000'0000'0000'0001:
            comment = { 
                .data = " ",
                .len = 1,
                .valid = false // this means the previous token  
            };                 // shall be pulled forward by one char
            break;
        case 0x0000'0000'0000'0002:
            // this might be more complicated to handle
            break;
        case 0x0000'0000'0000'0004 ... 0x0000'0000'FFFF'FFFF:
            comment = { 
                .data = "/*" + std::string(comment_length - 4, '*') + "*/",
                .len = comment_length,
                .valid = true 
            };
            break;
        default:
            printf("not impleted yet\n");
            printf("implement:\ncase 0x%016lX:\n\tbreak;\n", 
                (switcher.required_token_type));
            comment = { 
                .data = "»not implemented yet«",
                .len = 21,
                .valid = false 
            };
            break;
    }
    return comment;
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
    src_buffer << source_file.rdbuf();
    vars.src = src_buffer.str();

    debug("[DEBUG]\n%s\n", vars.src.c_str());
    debug("[DEBUG]\n%s\n", vars.mask.c_str());
    debug("[DEBUG]\n%s\n", vars.output.c_str());

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

    debug("[DEBUG]\n%s\n", vars.src.c_str());
    debug("[DEBUG]\n%s\n", vars.mask.c_str());
    debug("[DEBUG]\n%s\n", vars.output.c_str());
    
    replace(vars);
    printf("\n[RESULT]:\n%s\n",vars.output.c_str());
    // cleanup
    mask_file.close();
    source_file.close();

    return 0;
}
#pragma endregion

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <regex>
#include <assert.h>
#include <stdint.h>

#define DEBUG 0

#if DEBUG == 1
#define debug(...) fprintf(stderr,"[DEBUG] " __VA_ARGS__)
#else
#define debug(...) 
#endif
typedef struct token {
    std::string data;
    uint32_t len;
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
    bool defer_token_popping;
    bool outofspace;
} globalvars;

#pragma region function_prototypes
    bool parsingEdgecase(globalvars &vars);
    void purgeRegionDirectives(std::string& input);
    void purgeComments(std::string& input);
    token pop_token(std::string &input, globalvars &vars);
    token generateCommentToken(uint32_t comment_length, bool end_of_line = false);
    bool currentTokenNeedsSeperator(std::string &check_end, std::string &check_front);
    bool contains(const std::string &str, const std::string &substr);
    token token_post_processing(globalvars &vars, token current_token);
    int isOperatorOrDelimiter(const std::string &str);
    bool isTokenChar(char c);
#pragma endregion

#pragma region util_functions 

void print_usage() {
    printf("Usage: ascii_art [mask_file] [source_file]\n"); 
}

#if 1
[[nodiscard]]
bool isTokenChar(char c) {
    static const std::string TOKEN_CHARS = 
            "_.abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    return TOKEN_CHARS.find(c) != std::string::npos;
}
#else
[[nodiscard]]
bool isTokenChar(char c) {
    const std::string compare = std::string(1, c);
    return !isOperatorOrDelimiter(compare);
}
#endif

void purgeComments(std::string& input) {
    std::regex 
        commentRegex(R"((?:\/\*(?:[^*]|(?:\*+[^*\/]))*\*+\/)|(?:\/\/.*$))");
    input = std::regex_replace(input, commentRegex, "");
}

void purgeRegionDirectives(std::string& input) {
    std::istringstream iss(input);
    std::ostringstream oss;
    std::string line;

    // lines starting with these string shall be removed
    const std::string strings_to_remove[] = {
        "#pragma region",
        "#pragma endregion",
        "#region",
        "#endregion"
    };

    const int numStringsToRemove = sizeof(strings_to_remove) / 
        sizeof(strings_to_remove[0]);

    while (std::getline(iss, line)) {
        bool removeLine = false;
        for (int i = 0; i < numStringsToRemove; ++i) {
            if (line.find(strings_to_remove[i]) == 0) {
                removeLine = true;
                break;
            }
        }
        if (!removeLine) {
            oss << line << "\n";
        }
    }
    input = oss.str();
}

[[nodiscard]]
int isOperatorOrDelimiter(const std::string &str) {
    static const std::string operators[] = {
        "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "::"
        "<<=", ">>=",
        "++", "--", "<<", ">>",
        "==", "!=", "&&", "||",
        "<=", ">=",
        "->",
        "[[", "]]", "({", "})",
        "(", ")", "{", "}", "[", "]",
        ";", ":", ",", "+", "-", "*",
        "/", "%", "=", "<", ">", "&", "|",
        "!", "^", "~", "?"
    };
    for (const std::string &op : operators) {
        if (str.substr(0, op.length()) == op) {
            return op.length();
        }
    }
    return false;
}

[[nodiscard]]
int count_char(const std::string &str, char c) {
    int count = 0;
    for (char current_char : str) {
        if (current_char == c) {
            count++;
        }
    }
    return count;
}

[[nodiscard]]
bool contains(const std::string &str, const std::string &substr) {
    return str.find(substr) != std::string::npos;
}

bool currentTokenNeedsSeperator(std::string &check_end, std::string &check_front) {
    if(check_end.empty())
        return false;
    std::string seperatorless_characters = "+-*/%&|^!~<>?:\"=\'\n#\\{}()[];,";
    std::string end_pre, front_cur;
    bool pre_is_string_token, cur_is_string_token;

    end_pre = check_end.back();
    front_cur = check_front[0];
    
    debug("comparison: %c«»%c\n", end_pre[0], front_cur[0]);

    pre_is_string_token = !contains(seperatorless_characters, end_pre);
    cur_is_string_token = !contains(seperatorless_characters, front_cur);

    return pre_is_string_token && cur_is_string_token;
}

[[deprecated]] [[nodiscard]]
token pop_token_old(std::string &input, globalvars &vars) {
    bool seperation_needed = false;
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

    seperation_needed = currentTokenNeedsSeperator(vars.output, info.data);
    // std::cout << vars.output[vars.output.size()] << "«»" << info.data <<'\n';
    if(seperation_needed) {
        if (vars.slotend_distance < info.len + 1) {
            // determine if token is *really* going to be pasted 
            // behind another string_token
            // if not we dont append the space because that will be provided
            // by the mask
            /*nothing*/
        } else {
            info.data = " " + info.data;
            info.len++;
        } 
    }

    // Set valid flag
    info.valid = true;    
    return info;
}

[[nodiscard]]
token token_post_processing(globalvars &vars, token current_token) {
    bool seperation_needed;
    if (vars.output.empty() != true) {
        current_token.valid = true;
        return current_token;
    }
    seperation_needed = currentTokenNeedsSeperator(vars.output, 
        current_token.data);
    if(seperation_needed) {
        if (vars.slotend_distance < current_token.len + 1) {
            /*nothing*/
        } else {
            current_token.data = " " + current_token.data;
            current_token.len++;
        } 
    }
    // Set valid flag
    current_token.valid = true;
    assert(current_token.len != 0);
    return current_token;
}

[[deprecated]]
token pop_token_unstable(std::string &str, globalvars &vars) {
    // Skip any leading whitespace
    bool seperation_needed = false;
    token current_token = {
        .len = 0,
        .valid = false
    };

    std::size_t startPos = str.find_first_not_of(" \t\n\r\f\v");

    // Return an empty string if there's no token left
    if (startPos == std::string::npos) {
        return current_token;
    }

    // Check if the token is a comment
    if (str[startPos] == '/' && str[startPos + 1] == '/') {
        std::size_t endPos = str.find('\n', startPos + 2);
        if (endPos == std::string::npos) {
            endPos = str.length();
        }
        current_token.data = str.substr(startPos, endPos - startPos); 
        current_token.len = current_token.data.length();
        str.erase(startPos, endPos - startPos);
        return token_post_processing(vars, current_token);

    } else if (str[startPos] == '/' && str[startPos + 1] == '*') {
        std::size_t endPos = str.find("*/", startPos + 2);
        if (endPos == std::string::npos) {
            endPos = str.length();
        }
        current_token.data = str.substr(startPos, endPos - startPos + 2);
        current_token.len = current_token.data.length();
        str.erase(startPos, endPos - startPos + 2);
        return token_post_processing(vars, current_token);
    }

    if (str[startPos] == '"' || str[startPos] == '\'') {
        char quote = str[startPos];
        std::size_t endPos = str.find_first_of(quote, startPos + 1);
        while (str[endPos - 1] == '\\' && str[endPos - 2] != '\\') {
            endPos = str.find_first_of(quote, endPos + 1);
        }
        std::cout << "end: " << endPos << '\n';
        std::cout << "start: " << startPos << '\n';
        current_token.data = str.substr(startPos, endPos - startPos + 1);
        std::cout << "tok: " << current_token.data << '\n';
        exit(EXIT_SUCCESS);
        current_token.len = current_token.data.length();
        str.erase(startPos, endPos - startPos + 1);
        return token_post_processing(vars, current_token);
    }

    // Check if the token is a operator or a delimiter
    int opLength = isOperatorOrDelimiter(str.substr(startPos));
    if (opLength > 0) {
        current_token.data = str.substr(startPos, opLength);
        current_token.len = current_token.data.length();
        str.erase(startPos, opLength);
        return token_post_processing(vars, current_token);
    }

    // Find the end position of the token
    std::size_t endPos = startPos + 1;
    while (endPos < str.length() && isTokenChar(str[endPos]) && 
            !isOperatorOrDelimiter(str.substr(startPos, endPos - startPos))) {
        endPos++;
    }

    // Extract and return the token
    current_token.data = str.substr(startPos, endPos - startPos);
    current_token.len = current_token.data.length();
    str.erase(startPos, endPos - startPos);
    return token_post_processing(vars, current_token);
}

[[nodiscard]]
token pop_token(std::string &str, globalvars &vars) {
    bool seperation_needed = false;
    token current_token = {
        .len = 0,
        .valid = false
    };

    // erase any whitespace at the front of the string
    std::size_t startPos = 0;
    while (str[startPos] == ' ' || str[startPos] == '\n' || 
        str[startPos] == '\t') {
        str.erase(0, 1);
    }    

    // Return an empty string if there's no token left
    if (startPos == std::string::npos) {
        return current_token; // hm
    }

    // Check if the token is a comment
    if (str[startPos] == '/' && str[startPos + 1] == '/') {
        std::size_t endPos = str.find('\n', startPos + 2);
        if (endPos == std::string::npos) {
            endPos = str.length();
        }
        current_token.data = str.substr(startPos, endPos - startPos); 
        current_token.len = current_token.data.length();
        str.erase(startPos, endPos - startPos);
        return token_post_processing(vars, current_token);

    } else if (str[startPos] == '/' && str[startPos + 1] == '*') {
        std::size_t endPos = str.find("*/", startPos + 2);
        if (endPos == std::string::npos) {
            endPos = str.length();
        }
        current_token.data = str.substr(startPos, endPos - startPos + 2);
        current_token.len = current_token.data.length();
        str.erase(startPos, endPos - startPos + 2);
        return token_post_processing(vars, current_token);
    }

    if (str[startPos] == '"' || str[startPos] == '\'') {
        char quote = str[startPos];
        std::size_t endPos = str.find_first_of(quote, startPos + 1);
        while (str[endPos - 1] == '\\' && str[endPos - 2] != '\\') {
            endPos = str.find_first_of(quote, endPos + 1);
        }
        if (endPos > vars.slotend_distance && vars.slotend_distance >= 3)
        {
            endPos = vars.slotend_distance - 2;
            current_token.data = str.substr(startPos, endPos - startPos + 1) + "\"";
            current_token.len = current_token.data.length();
            str.erase(startPos, endPos - startPos + 1);
            str.insert(0, "\"");
                debug("start-end, slotend position: %d-%d, %d\n", startPos, endPos,
                    vars.slotend_distance); 
                debug("current literal: %s\n", current_token.data.c_str());
            return token_post_processing(vars, current_token);
        } else if (endPos > vars.slotend_distance && vars.slotend_distance == 0) {
            assert(vars.slotend_distance != 0);
            current_token.len = current_token.data.length();
            return token_post_processing(vars, current_token);
        }
        
        current_token.data = str.substr(startPos, endPos - startPos + 1);
        current_token.len = current_token.data.length();
        str.erase(startPos, endPos - startPos + 1);
            debug("start-end, slotend position: %d-%d, %d\n", startPos, endPos,
                    vars.slotend_distance);
            // exit(EXIT_SUCCESS); 
        return token_post_processing(vars, current_token);
    }  

    // Check if the token is a operator or a delimiter
    int opLength = isOperatorOrDelimiter(str.substr(startPos));
    if (opLength > 0) {
        current_token.data = str.substr(startPos, opLength);
        current_token.len = current_token.data.length();
        str.erase(startPos, opLength);
        return token_post_processing(vars, current_token);
    }

    // Find the end position of the token
    std::size_t endPos = startPos + 1;
    while (endPos < str.length() && isTokenChar(str[endPos]) && 
            !isOperatorOrDelimiter(str.substr(startPos, endPos - startPos))) {
        endPos++;
    }

    // Extract and return the token
    current_token.data = str.substr(startPos, endPos - startPos);
    current_token.len = current_token.data.length();
    str.erase(startPos, endPos - startPos);
    return token_post_processing(vars, current_token);
}

[[deprecated]]
int getNextTokenLength(const std::string &str, int index) {
    int start = -1, end = -1;
    int len = str.length();
    for (int i = index; i < len; i++) {
        if (str[i] == ' ') {
            if (start >= 0 && end < 0) {
                end = i;
            }
        } else {
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

[[deprecated]]
int nextTokenStart(const std::string &str, int startIndex) {
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

[[deprecated]]
bool insertcomment(std::string &output_str, std::string &mask_str, 
                    std::string &src_str, 
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
        else { // we have reached a section where code can be placed, 
        // check if we can stop comment here
            bool can_finish_comment = getNextTokenLength(mask_str, mask_ind - 1) >= 
            getNextTokenLength(src_str, src_ind - 1) + 2;
            if(can_finish_comment) {
                output_str[mask_ind] = '*';
                output_str[mask_ind + 1] = '/';
                mask_ind++;
                exit(EXIT_SUCCESS);
                return true; 
            } else { // we are trapped on an "island" and need to fill it with *
                while (mask_ind < mask_str.length() && src_ind < src_str.length() && 
                mask_str[mask_ind] != '\0' && mask_str[mask_ind] != ' ') 
                // && mask_str[mask_ind] != '\n')
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
        if (vars.slotend_distance == 0)
        {
            if (vars.slotend_distance == vars.newline_distance || 
                    vars.mask[vars.mask_ind] == '\n') {
                vars.output += "\n";
                vars.mask_ind++;
            } else {
                vars.output += " ";
                vars.mask_ind++;
            }
            vars.newline_distance = vars.mask.find('\n', vars.mask_ind) - vars.mask_ind;
            vars.slotend_distance = std::min(
                vars.mask.find(' ', vars.mask_ind) - vars.mask_ind,
                vars.mask.find('\n', vars.mask_ind) - vars.mask_ind);
            continue;
        }
        
        if (!vars.defer_token_popping) {
            assert(vars.slotend_distance != 0);
            vars.current_token = pop_token(vars.src, vars);
            if (vars.current_token.len == 0) {
                debug("corrupt token with length 0 detected: %s\n", 
                    vars.current_token.data.c_str());
                debug("the index of the mask is: %d\n", 
                    vars.mask_ind);
                debug("rest of the src string is:\n %s\n", vars.src.c_str());
                exit(EXIT_FAILURE);
            }
            assert(vars.current_token.len != 0);
            assert(vars.current_token.data.length() == 
                vars.current_token.len);

        }            

        vars.newline_distance = vars.mask.find('\n', vars.mask_ind) - vars.mask_ind;
        vars.slotend_distance = std::min(
                vars.mask.find(' ', vars.mask_ind) - vars.mask_ind,
                vars.mask.find('\n', vars.mask_ind) - vars.mask_ind);

        token_fits = vars.current_token.len <= vars.slotend_distance && 
            vars.slotend_distance != 0;
        
        debug("token fits: %d, remaining slot: %d, newline_in: %d, mask_ind: %d\n", 
                token_fits, vars.slotend_distance, vars.newline_distance, 
                vars.mask_ind);
        
        vars.outofspace = vars.mask_ind >= vars.mask_len;
        
        ok_to_continue = token_fits && vars.current_token.valid &&
                         !vars.outofspace;

        if(token_fits) {
            vars.output += vars.current_token.data;
            vars.mask_ind += vars.current_token.len;

            vars.newline_distance = vars.mask.find('\n', vars.mask_ind);
            vars.slotend_distance = std::min(
                vars.mask.find(' ', vars.mask_ind) - vars.mask_ind,
                vars.mask.find('\n', vars.mask_ind) - vars.mask_ind);
            vars.defer_token_popping = false;
        } else {
            // token obviously doesnt fit
            // handle edge case
            // take along the entire context
            vars.outofspace = true;
            bool edgecase_success = false;
            
            edgecase_success = parsingEdgecase(vars);
            if(!edgecase_success) { // this swallows the last poped token 
                                    // and replaces it with a comment
                                    // prevent token poping
                printf("there was an error while dealing with an edgecase: \n");
                printf("\033[1;31m%s\033[0m", vars.output.c_str());
                printf("%s", vars.mask.substr(vars.mask_ind).c_str());
                exit(EXIT_FAILURE);
            }
            
            // todo pull forward string

            // defer next token popping
            vars.defer_token_popping = true;
            // resume loop since edgecase got handled
            vars.newline_distance = vars.mask.find('\n', vars.mask_ind);
            vars.slotend_distance = vars.mask.find(' ', vars.mask_ind);
            token_fits = vars.current_token.len <= vars.slotend_distance;
            vars.outofspace = vars.mask_ind >= vars.mask_len;    
            ok_to_continue = token_fits && vars.current_token.valid &&
                                                    !vars.outofspace;
        }
    } while(ok_to_continue);

    return false;
} 

bool parsingEdgecase(globalvars &vars) {
    // for debugging
    // debug("\noutput:\n%s\n", vars.output.c_str());
    
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

[[nodiscard]]
token generateCommentToken(uint32_t comment_length, bool end_of_line) {
    union switcher_util
    {
        struct { uint32_t part1; uint32_t part2; };
        struct { uint64_t required_token_type; };
    };
    switcher_util switcher = {
        .part1 = comment_length,
        .part2 = end_of_line 
    }; 

    debug("comment_length is 0x%016X\n", (switcher.part1));
    debug("end_of_line is 0x%016X\n", (switcher.part2));
    debug("the switcher(value) is 0x%016lX\n", (switcher.required_token_type));
    
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
                .valid = true//false // this means the previous token  
            };                 // shall be pulled forward by one char
            break;
        case 0x0000'0000'0000'0002:
            // this might be more complicated to handle
            comment = { 
                .data = std::string(comment_length, '#'),
                .len = comment_length,
                .valid = true 
            };
            break;
        case 0x0000'0000'0000'0003:
            comment = { 
                .data = std::string(comment_length, '#'),
                .len = comment_length,
                .valid = true 
            };
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

    printf("\n%s\n", vars.src.c_str());
    debug("\n%s\n", vars.mask.c_str());
    debug("\n%s\n", vars.output.c_str());

    // format the source code properly
    vars.src.erase(remove(vars.src.begin(), vars.src.end(), '\t'), vars.src.end());
    vars.src.erase(remove(vars.src.begin(), vars.src.end(), '\n'), vars.src.end());
    vars.src.erase(remove(vars.src.begin(), vars.src.end(), '\r'), vars.src.end());
    vars.src = regex_replace(vars.src, std::regex("\\s{2,}"), " ");
    // purgeComments(vars.src); // this seems to cut source code at some point
    // purgeRegionDirectives(vars.src);

    // init vars
    vars.mask_slot_amount = count_char(vars.mask, '#');
    vars.mask_len = vars.mask.length();
    vars.src_len = vars.src.length();
    vars.outofspace = vars.src_len > vars.mask_slot_amount;

    if (vars.outofspace) {
        printf("not enough space\n");
        printf("src_len: %d\n", vars.src_len);
        printf("slot amount: %d\n", vars.mask_slot_amount);
        // exit(EXIT_FAILURE);
    }

    printf("\n%s\n", vars.src.c_str());
    debug("\n%s\n", vars.mask.c_str());
    debug("\n%s\n", vars.output.c_str());
    
    replace(vars);

    printf("\n[SRC]%s\n", vars.src.c_str());
    printf("\n[RESULT]:\n%s\n",vars.output.c_str());
    printf("src_len: %d\n", vars.src_len);
    printf("mask_slot_amount: %d\n", vars.mask_slot_amount);
    // cleanup
    mask_file.close();
    source_file.close();

    return 0;
}
#pragma endregion

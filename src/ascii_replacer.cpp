#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <regex>
#include <assert.h>
#include <stdint.h>
#include <utility>

#include "../witc/matching.h"

// std::string test = R"(abc this is a test string)";

#define DEBUG 1

#if DEBUG == 1
#define debug(...) fprintf(stderr,"[DEBUG] " __VA_ARGS__)
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
    bool defer_token_popping;
    bool outofspace;
} globalvars;

#pragma region function_prototypes
    bool parsingEdgecase(globalvars &vars);
    void purgeRegionDirectives(std::string& input);
    void removeComments(std::string& sourceCode);
    void purgeComments(std::string& input);
    token pop_token(std::string &input, globalvars &vars);
    void transformRawStrings(std::string& sourceCode);
    token generateCommentToken(int comment_length, bool end_of_line = false);
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

void removeComments(std::string& sourceCode) {
    std::vector<std::string> lines;

    // Split the source code into lines
    size_t start = 0;
    size_t end = sourceCode.find("\n");
    while (end != std::string::npos) {
        lines.push_back(sourceCode.substr(start, end - start));
        start = end + 1;
        end = sourceCode.find("\n", start);
    }
    lines.push_back(sourceCode.substr(start));

    // Process each line to remove comments
    bool insideComment = false;
    sourceCode.clear();
    for (const std::string& line : lines) {
        std::string processedLine;
        for (size_t i = 0; i < line.length(); ++i) {
            if (!insideComment && line[i] == '/' && i + 1 < line.length() && line[i + 1] == '/')
                break;
            else if (!insideComment && line[i] == '/' && i + 1 < line.length() && line[i + 1] == '*') {
                insideComment = true;
                ++i;
            } else if (insideComment && line[i] == '*' && i + 1 < line.length() && line[i + 1] == '/') {
                insideComment = false;
                ++i;
            } else if (!insideComment) {
                processedLine += line[i];
            }
        }

        sourceCode += processedLine + "\n";
    }
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

void transformRawStrings(std::string& sourceCode) {
    std::string rawStringStart = "R\"(";
    std::string rawStringEnd = ")\"";

    std::pair<std::string, std::string> conversionPairs[] = {
        {"\\", "\\\\"}, {"\t", "\\t"},
        {"\n", "\\n"}, {"\r", "\\r"},
        {"\e", "\\e"}, {"\a", "\\a"},
        {"\b", "\\b"}, {"\"", "\\\""},
        {"\'", "\\\'"},
    };

    std::size_t searchStart = sourceCode.find(rawStringStart);
    while (searchStart != std::string::npos) {
        std::size_t searchEnd = sourceCode.find(rawStringEnd, searchStart);
        if (searchEnd != std::string::npos) {
            std::string rawString = sourceCode.substr(searchStart + rawStringStart.length(), searchEnd - searchStart - rawStringStart.length());
            for (const auto& pair : conversionPairs) {
                std::size_t escapePos = rawString.find(pair.first);
                while (escapePos != std::string::npos) {
                    rawString.replace(escapePos, pair.first.length(), pair.second);
                    escapePos = rawString.find(pair.first, escapePos + pair.second.length());
                }
            }
            sourceCode.replace(searchStart, searchEnd - searchStart + rawStringEnd.length(), "\"" + rawString + "\"");
        } else {
            break;  // Found an opening delimiter but no closing delimiter, stop searching
        }
        searchStart = sourceCode.find(rawStringStart, searchStart + rawStringStart.length());
    }
}

bool currentTokenNeedsSeperator(std::string &check_end, std::string &check_front) {
    debug("calling function currentTokenNeedsSeperator\n");
    if(check_end.empty())
        return false;
    std::string seperatorless_characters = "+-*/%&|^!~<>?:\"=\'\n#\\{}()[];,";
    std::string end_pre, front_cur;
    bool pre_is_string_token, cur_is_string_token;

    end_pre = check_end.back();
    front_cur = check_front[0];
    
    pre_is_string_token = !contains(seperatorless_characters, end_pre);
    cur_is_string_token = !contains(seperatorless_characters, front_cur);
    
    debug("comparison: %c«»%c\n", end_pre[0], front_cur[0]);
    debug("separation was needed: %s\n", 
        (pre_is_string_token && cur_is_string_token)?
        "true" : "false");
   
    return pre_is_string_token && cur_is_string_token;
}


[[nodiscard]]
token token_post_processing(globalvars &vars, token current_token) {
    debug("call from token_post_processing\n");
    //exit(EXIT_SUCCESS);
    bool seperation_needed;
    if (vars.output.empty()) { // what was the purpose of this again?
        debug("post processing is returning token as is\n");
        current_token.valid = true; // this seems very wrong
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
        // slot must at least be 3 because at least 1 charcter should be popped
        if (endPos > vars.slotend_distance && vars.slotend_distance >= 3) {
            endPos = vars.slotend_distance - 2;
            current_token.data = 
                str.substr(startPos, endPos - startPos + 1) + "\"";
            current_token.len = current_token.data.length();
            str.erase(startPos, endPos - startPos + 1);
            str.insert(0, "\"");
                debug("start-end, slotend position: %d-%d, %d\n", 
                startPos, endPos, vars.slotend_distance); 
                debug("current literal: %s\n", current_token.data.c_str());
            return token_post_processing(vars, current_token);
        // in case there is 0 length left in the slot left
        } else if (endPos > vars.slotend_distance && vars.slotend_distance == 0) {
            assert(vars.slotend_distance != 0);
            current_token.len = current_token.data.length();
            return token_post_processing(vars, current_token);
        }

        current_token.data = 
                str.substr(startPos, endPos - startPos + 1); 

        current_token.len = current_token.data.length();
        str.erase(startPos, endPos - startPos + 1);
            debug("start-end, slotend position: %d-%d, %d\n", startPos, endPos,
                    vars.slotend_distance);
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
        
        debug("token fits: %d, token length: %d, remaining slot: %d, newline_in: %d, mask_ind: %d\n", 
                token_fits, vars.current_token.len, vars.slotend_distance, 
                    vars.newline_distance, vars.mask_ind);
        debug("the token is: %s", vars.current_token.data.c_str());

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
token generateCommentToken(int comment_length, bool end_of_line) {
    match(comment_length, end_of_line) {
        pattern(1, false) return { 
            .data = "\n",
            .len = 1,
            .valid = true 
        };
        pattern(1, true) return { 
            .data = " \n",
            .len = 2,
            .valid = true 
        };
        pattern(2, true) return { 
            .data = "//\n",
            .len = 3,
            .valid = true 
        };
        pattern(3, true) return { 
            .data = " //\n",
            .len = 4,
            .valid = true 
        };
        pattern(_, true when comment_length > 3) return { 
            .data = "/*" + std::string(comment_length - 4, '*') + "*/\n",
            .len = comment_length + 1,
            .valid = true 
        };
        pattern(0, false) return { 
            .data = " ",
            .len = 1,
            .valid = true 
        };
        pattern(1, false) return { 
            .data = " ",
            .len = 1,
            .valid = true // false // this means the previous token  
        };                // should be pulled forward by one char
        pattern(2, false) return { 
            .data = std::string(comment_length, '#'),
            .len = comment_length,
            .valid = true // this might be more complicated to handle
        };
        pattern(3, false) return { 
            .data = std::string(comment_length, '#'),
            .len = comment_length,
            .valid = true 
        };
        pattern(_, false when comment_length > 3) return { 
            .data = "/*" + std::string(comment_length - 4, '*') + "*/",
            .len = comment_length,
            .valid = true 
        };
        else {
            printf("unimplemented case caugth\n");
            printf("implement: \n pattern(%d, %s)", 
                comment_length, end_of_line ? "true" : "false");
            return {
                .data = "»not implemented yet«",
                .len = 21,
                .valid = false 
            };
        }
    } 
    printf("this part of part of the code should not be reachable\n");
    *(int *)0;
    return { 0 };
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
    purgeRegionDirectives(vars.src); // this appears to be messing with 
            // string literal splitting

    removeComments(vars.src);
    // printf("\n[SRC]:\n%s", vars.src.c_str());
    transformRawStrings(vars.src);
    // newline characters are removede, everything after a // is 
        // considered a comment
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
        printf("src_len: %d\n", vars.src_len);
        printf("slot amount: %d\n", vars.mask_slot_amount);
        // exit(EXIT_FAILURE);
    }

    printf("\n%s\n", vars.src.c_str());
    debug("\n%s\n", vars.mask.c_str());
    debug("\n%s\n", vars.output.c_str());
    if (vars.src.empty()) {
        std::cout << "the src string was already empty before it got processed\n";
    }
    replace(vars);

    printf("\n[SRC]:\n%s\n", vars.src.c_str());
    printf("\n[RESULT]:\n%s\n",vars.output.c_str());
    printf(!vars.src.length() ? "src is empty\n" : "src is not empty\n");
    printf("src_len: %d\n", vars.src_len);
    printf("mask_slot_amount: %d\n", vars.mask_slot_amount);
    // cleanup
    mask_file.close();
    source_file.close();

    return 0;
}
#pragma endregion
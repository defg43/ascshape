
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

[[deprecated]]
void purgeComments(std::string& input) {
    std::regex 
        commentRegex(R"((?:\/\*(?:[^*]|(?:\*+[^*\/]))*\*+\/)|(?:\/\/.*$))");
    input = std::regex_replace(input, commentRegex, "");
}


[[deprecated]]
void transformRawStrings_old(std::string& sourceCode) {
    std::string searchString = "R\"(";
    std::string replaceString = "\"";
    std::pair<char, std::string> conversion_pairs[] = {
        {'\t', R"(\t)"}, {'\n', R"(\n)"},
        {'\r', R"(\r)"}, {'\e', R"(\e)"},
        {'\a', R"(\a)"}, {'\b', R"(\b)"},
        {'\\', R"(\\)"}, {'\"', R"(\")"},
        {'\'', R"(\')"}
    };

    std::size_t searchStart = sourceCode.find(searchString);
    while (searchStart != std::string::npos) {
        std::size_t searchEnd = sourceCode.find(")\"", searchStart);
        if (searchEnd != std::string::npos) {
            sourceCode.replace(searchStart, searchEnd - searchStart + searchString.length(), replaceString);
        } else {
            break;  // Found an opening delimiter but no closing delimiter, stop searching
        }
        searchStart = sourceCode.find(searchString, searchStart + replaceString.length());
    }
}

[[deprecated]]
void transformRawStrings_unstable(std::string& sourceCode) {
    std::string searchString = "R\"(";
    std::string replaceString = "\"";
    std::pair<char, std::string> conversion_pairs[] = {
        {'\t', R"(\t)"}, {'\n', R"(\n)"},
        {'\r', R"(\r)"}, {'\e', R"(\e)"},
        {'\a', R"(\a)"}, {'\b', R"(\b)"},
        {'\\', R"(\\)"}, {'\"', R"(\")"},
        {'\'', R"(\')"}
    };

    std::size_t searchStart = sourceCode.find(searchString);
    while (searchStart != std::string::npos) {
        std::size_t searchEnd = sourceCode.find(")\"", searchStart);
        if (searchEnd != std::string::npos) {
            std::string rawString = sourceCode.substr(searchStart, searchEnd - searchStart + searchString.length());
            for (const auto& pair : conversion_pairs) {
                std::size_t escapePos = rawString.find(pair.second);
                while (escapePos != std::string::npos) {
                    rawString.replace(escapePos, pair.second.length(), 1, pair.first);
                    escapePos = rawString.find(pair.second, escapePos + 1);
                }
            }
            sourceCode.replace(searchStart, searchEnd - searchStart + searchString.length(), rawString);
        } else {
            break;  // Found an opening delimiter but no closing delimiter, stop searching
        }
        searchStart = sourceCode.find(searchString, searchStart + replaceString.length());
    }
}

#include "parser.hpp"
#include <sstream>
#include <stdexcept>

namespace tinyshell {

Pipeline Parser::parse(const std::string& line) {
    Pipeline pipeline;
    
    // Split by pipe
    std::istringstream stream(line);
    std::string cmdStr;
    
    while (std::getline(stream, cmdStr, '|')) {
        // Trim whitespace
        size_t start = cmdStr.find_first_not_of(" \t");
        size_t end = cmdStr.find_last_not_of(" \t");
        
        if (start == std::string::npos) {
            continue;
        }
        
        cmdStr = cmdStr.substr(start, end - start + 1);
        
        if (!cmdStr.empty()) {
            pipeline.commands.push_back(parseCommand(cmdStr));
        }
    }
    
    return pipeline;
}

Command Parser::parseCommand(const std::string& cmdStr) {
    Command cmd;
    auto tokens = tokenize(cmdStr);

    // Check for background operator
    if (!tokens.empty() && tokens.back() == "&") {
        cmd.background = true;
        tokens.pop_back();
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];

        if (token == "<") {
            if (i + 1 < tokens.size()) {
                cmd.inputFile = tokens[++i];
            } else {
                throw std::runtime_error("Syntax error: missing input file");
            }
        } else if (token == ">") {
            if (i + 1 < tokens.size()) {
                cmd.outputFile = tokens[++i];
                cmd.appendOutput = false;
            } else {
                throw std::runtime_error("Syntax error: missing output file");
            }
        } else if (token == ">>") {
            if (i + 1 < tokens.size()) {
                cmd.outputFile = tokens[++i];
                cmd.appendOutput = true;
            } else {
                throw std::runtime_error("Syntax error: missing output file");
            }
        } else {
            cmd.args.push_back(token);
        }
    }

    return cmd;
}

std::vector<std::string> Parser::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    char quoteChar = 0;
    
    for (char c : line) {
        if (inQuotes) {
            if (c == quoteChar) {
                inQuotes = false;
            } else {
                current += c;
            }
        } else if (c == '"' || c == '\'') {
            inQuotes = true;
            quoteChar = c;
        } else if (c == ' ' || c == '\t') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        tokens.push_back(current);
    }
    
    return tokens;
}

} // namespace tinyshell

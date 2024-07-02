#pragma once

#include <string>
#include <sstream>
#include <algorithm>
#include <exception>
#include <memory>
#include <vector>

struct AST;
typedef std::unique_ptr<AST> AST_ptr;

struct AST
{
    enum NodeType
    {
        COMMAND = 0,
        REGULAR,
        SQ_STRING,
        SUBCOMMAND,
        VAR,
        TILDE,
        ESCAPE,
        WORD,
        PIPE,
        LOGICAL,
        COMMA
    } type;

    std::string data;
    std::vector<AST_ptr> children;

    AST(NodeType _type, const std::string& _data)
        : type(_type), data(_data) {}

    AST(NodeType _type, const std::string& _data, std::vector<AST_ptr>&& _children)
        : type(_type), data(_data), children(std::move(_children)) {}

    void print(int tabs = 0);
};

AST_ptr parse_shell_input(std::string& input);

AST_ptr parse_command_list(std::string& input);
AST_ptr parse_command_logical(std::string& input);
AST_ptr parse_command_pipe(std::string& input);
AST_ptr parse_command(std::string& input);

AST_ptr parse_word(std::string& input);
AST_ptr parse_regular(std::string& input);
AST_ptr parse_subcommand(std::string& input);
AST_ptr parse_sq_string(std::string& input);
AST_ptr parse_var(std::string& input);
AST_ptr parse_tilde(std::string& input);
AST_ptr parse_escape(std::string& input);

AST_ptr parse_comma(std::string& input);
AST_ptr parse_logic(std::string& input);
AST_ptr parse_pipe(std::string& input);
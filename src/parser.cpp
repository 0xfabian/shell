#include <parser.h>

using namespace std;

void AST::print(int tabs)
{
    for (int i = 0; i < tabs; i++)
        printf("    ");

    switch (type)
    {
    case COMMAND:       printf("com");          break;
    case REGULAR:       printf("reg");          break;
    case SQ_STRING:     printf("sq");           break;
    case SUBCOMMAND:    printf("subcom");       break;
    case VAR:           printf("var");          break;
    case TILDE:         printf("tilde");        break;
    case ESCAPE:        printf("esc");          break;
    case WORD:          printf("word");         break;
    case PIPE:          printf("pipe");         break;
    case LOGICAL:       printf("logic");        break;
    case COMMA:         printf("comma");        break;
    }

    printf(": \"%s\"\n", data.c_str());

    if (!children.empty())
    {
        for (int i = 0; i < tabs; i++)
            printf("    ");

        printf("{\n");

        for (const auto& child : children)
            child->print(tabs + 1);

        for (int i = 0; i < tabs; i++)
            printf("    ");

        printf("}\n");
    }
}

AST_ptr parse_shell_input(string& input)
{
    AST_ptr ret = parse_command_list(input);

    if (!input.empty() && input.front() == ')')
        throw runtime_error("expected ( before subcommand");

    return ret;
}

AST_ptr parse_command_list(string& input)
{
    AST_ptr ret = parse_command_logical(input);

    while (true)
    {
        AST_ptr sep = parse_comma(input);

        if (sep)
        {
            if (!ret)
                throw runtime_error("expected command before ;");

            AST_ptr com = parse_command_logical(input);

            if (!com)
                throw runtime_error("expected command after ;");

            sep->children.push_back(move(ret));
            sep->children.push_back(move(com));

            ret = move(sep);
        }
        else
            break;
    }

    return ret;
}

AST_ptr parse_command_logical(string& input)
{
    AST_ptr ret = parse_command_pipe(input);

    while (true)
    {
        AST_ptr sep = parse_logic(input);

        if (sep)
        {
            if (!ret)
                throw runtime_error("expected command before " + sep->data);

            AST_ptr com = parse_command_pipe(input);

            if (!com)
                throw runtime_error("expected command after " + sep->data);

            sep->children.push_back(move(ret));
            sep->children.push_back(move(com));

            ret = move(sep);
        }
        else
            break;
    }

    return ret;
}

AST_ptr parse_command_pipe(string& input)
{
    AST_ptr ret = parse_command(input);

    while (true)
    {
        AST_ptr sep = parse_pipe(input);

        if (sep)
        {
            if (!ret)
                throw runtime_error("expected command before |");

            AST_ptr com = parse_command(input);

            if (!com)
                throw runtime_error("expected command after |");

            if (ret->type == AST::PIPE)
                ret->children.push_back(move(com));
            else
            {
                sep->children.push_back(move(ret));
                sep->children.push_back(move(com));

                ret = move(sep);
            }
        }
        else
            break;
    }

    return ret;
}

void trim_left(string& input)
{
    while (!input.empty() && isspace(input.front()))
        input.erase(input.begin());
}

AST_ptr parse_command(string& input)
{
    vector<AST_ptr> children;

    trim_left(input);

    while (!input.empty())
    {
        AST_ptr child = parse_word(input);

        if (!child)
            break;

        children.push_back(move(child));

        trim_left(input);
    }

    if (children.empty())
        return nullptr;

    return make_unique<AST>(AST::COMMAND, "", move(children));
}

AST_ptr parse_word(string& input)
{
    vector<AST_ptr> children;

    AST_ptr a = nullptr;

    while (true)
    {
        a = parse_regular(input);

        if (!a)
            a = parse_subcommand(input);

        if (!a)
            a = parse_sq_string(input);

        if (!a)
            a = parse_var(input);

        if (!a)
            a = parse_tilde(input);

        if (!a)
            a = parse_escape(input);

        if (!a)
            break;

        children.push_back(move(a));
    }

    if (children.empty())
        return nullptr;

    return make_unique<AST>(AST::WORD, "", move(children));
}

AST_ptr parse_regular(string& input)
{
    AST_ptr  regular = nullptr;

    while (!input.empty())
    {
        if (isspace(input.front()) || string(";|&()\'~$\\#").find(input.front()) != string::npos)
            break;

        if (!regular)
            regular = make_unique<AST>(AST::REGULAR, "");

        regular->data += input.front();

        input.erase(input.begin());
    }

    return regular;
}

AST_ptr parse_subcommand(string& input)
{
    if (input.front() != '(')
        return nullptr;

    // string sub = input.substr(1);
    // int len = sub.size();

    // AST_ptr subcom = parse_command_list(sub);

    // if (sub.empty() || sub.front() != ')')
    //     throw runtime_error("expected ) after subcommand");

    // string res = input.substr(1, len - sub.size());
    // input = input.substr(2 + res.size());

    // return make_unique<AST>(AST::SUBCOMMAND, res);

    input.erase(input.begin());
    string res = input;

    AST_ptr subcom = parse_command_list(input);

    if (input.empty() || input.front() != ')')
        throw runtime_error("expected ) after subcommand");

    res.resize(res.size() - input.size());

    input.erase(input.begin());

    return make_unique<AST>(AST::SUBCOMMAND, res);
}

AST_ptr parse_sq_string(string& input)
{
    if (input.front() != '\'')
        return nullptr;

    input.erase(input.begin());

    AST_ptr sq = make_unique<AST>(AST::SQ_STRING, "");

    while (true)
    {
        if (input.empty())
            throw runtime_error("expected '");

        if (input.front() == '\'')
            break;

        sq->data += input.front();

        input.erase(input.begin());
    }

    input.erase(input.begin());

    return sq;
}

AST_ptr parse_var(string& input)
{
    if (input.front() != '$')
        return nullptr;

    input.erase(input.begin());

    AST_ptr var = nullptr;

    while (!input.empty())
    {
        if (!isalnum(input.front()) && input.front() != '_')
            break;

        if (!var)
            var = make_unique<AST>(AST::VAR, "");

        var->data += input.front();

        input.erase(input.begin());
    }

    if (!var)
        throw runtime_error("expected variable name after $");

    return var;
}

AST_ptr parse_tilde(string& input)
{
    if (input.front() != '~')
        return nullptr;

    input.erase(input.begin());

    return make_unique<AST>(AST::TILDE, "~");
}

AST_ptr parse_escape(string& input)
{
    if (input.front() != '\\')
        return nullptr;

    input.erase(input.begin());

    if (input.empty())
        throw runtime_error("expected character after \\");

    char esc = input[0];

    input.erase(input.begin());

    return make_unique<AST>(AST::ESCAPE, string(1, esc));
}

AST_ptr parse_comma(string& input)
{
    if (input.size() > 0 && input.front() == ';')
    {
        input = input.substr(1);
        return make_unique<AST>(AST::COMMA, ";");
    }

    return nullptr;
}

AST_ptr parse_logic(string& input)
{
    if (input.size() > 1 && input[0] == '&' && input[1] == '&')
    {
        input = input.substr(2);
        return make_unique<AST>(AST::LOGICAL, "&&");
    }

    if (input.size() > 1 && input[0] == '|' && input[1] == '|')
    {
        input = input.substr(2);
        return make_unique<AST>(AST::LOGICAL, "||");
    }

    return nullptr;
}

AST_ptr parse_pipe(string& input)
{
    if (input.size() > 0 && input[0] == '|')
    {
        if (input.size() > 1 && input[1] == '|')
            return nullptr;

        input = input.substr(1);

        return make_unique<AST>(AST::PIPE, "|");
    }

    return nullptr;
}
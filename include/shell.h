#pragma once

#include <unistd.h>
#include <sys/stat.h>
#include <wait.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <functional>
#include <cstring>
#include <vars.h>
#include <alias.h>
#include <parser.h>

struct Shell
{
    bool print_tree = false;

    std::string name;
    Vars vars;
    Aliases aliases;
    std::unordered_map < std::string, std::function<int(int, char**)>> builtins;

    Shell(const std::string& _name) : name(_name) {}

    void run();
    void run_file(int argc, char** argv);
    void execute(std::string input, bool save_status = true);

    void sync_vars();
    void prompt();
    void init();

    std::vector<std::string> get_sub_lines(const std::string& input);

    void make_regular(AST_ptr& leaf);
    std::vector<AST_ptr> expand_word(const AST_ptr& word);
    void sub_commands(AST_ptr& tree);

    int execute_tree(const AST_ptr& tree);
    int execute_command(const AST_ptr& command);
    int execute_pipeline(const AST_ptr& pipeline);

    bool is_builtin(const std::string& name);
    bool is_executable(const std::string& name);
    bool is_script(const std::string& name, std::string& interpreter);
    int exec_and_return(int argc, char** argv);
    void exec_and_exit(int argc, char** argv);

    int __exit(int argc, char** argv);
    int __cd(int argc, char** argv);
    int __set(int argc, char** argv);
    int __unset(int argc, char** argv);
    int __export(int argc, char** argv);
    int __alias(int argc, char** argv);
    int __unalias(int argc, char** argv);
};
#include <shell.h>

using namespace std;

void Shell::run()
{
    while (true)
    {
        prompt();

        if (!input.get())
            execute("echo exit ; exit");

        execute(input.data);
    }
}

void Shell::run_file(int argc, char** argv)
{
    const char* file_path = argv[1];

    ifstream file(file_path);

    if (!file.is_open())
    {
        cerr << name << ": failed to open file '" << file_path << "'\n";
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++)
        vars.set(to_string(i - 1), argv[i]);

    string line;

    while (getline(file, line))
        execute(line);

    file.close();
}

void Shell::execute(string input, bool save_status)
{
    string start_input = input;

    try
    {
        AST_ptr tree = parse_shell_input(input);

        if (!tree)
            return;

        make_regular(tree);
        sub_commands(tree);

        if (print_tree)
            tree->print();

        int status = execute_tree(tree);

        if (save_status)
            vars.set("status", to_string(status));
    }
    catch (const runtime_error& e)
    {
        // can catch pipe failed

        cout << start_input << endl;

        for (int i = 0; i < start_input.size() - input.size(); i++)
            cout << " ";

        cout << "^\n";

        cerr << name << ": " << e.what() << endl;
    }
}

void Shell::sync_vars()
{
    for (int i = 0; environ[i] != nullptr; i++)
    {
        string entry = environ[i];
        size_t pos = entry.find('=');

        if (pos != string::npos)
        {
            string name = entry.substr(0, pos);
            string value = entry.substr(pos + 1);

            vars.set(name, value);
            vars.export_(name);
        }
    }
}

void Shell::add_history(string str)
{
    if (str.empty())
        return;

    if (!history.empty() && str == history.back())
        return;

    int hist_size = 100;
    string hist_size_str;

    if (vars.get("HISTSIZE", hist_size_str))
    {
        try
        {
            hist_size = stoi(hist_size_str);
        }
        catch (...)
        {

        }
    }

    if (history.size() >= hist_size)
        history.erase(history.begin());

    history.push_back(str);
}

std::string Shell::get_history(int index)
{
    if (index < 0 || index >= history.size())
        return "";

    return history[history.size() - index - 1];
}

void Shell::prompt()
{
    string prompt;

    if (!vars.get("prompt", prompt))
        cout << "> " << flush;
    else
        execute(prompt, false);
}

#define ADD_BUILTIN(x) builtins[#x] = [this](int argc, char** argv) { return this->__##x(argc, argv); };

void Shell::init()
{
    ADD_BUILTIN(exit);
    ADD_BUILTIN(cd);
    ADD_BUILTIN(set);
    ADD_BUILTIN(unset);
    ADD_BUILTIN(export);
    ADD_BUILTIN(alias);
    ADD_BUILTIN(unalias);
    ADD_BUILTIN(history);

    sync_vars();

    vars.set("status", "0");

    const char* home = getenv("HOME");

    if (home)
    {
        ifstream file(string(home) + "/.config/shell/init");
        string line;

        if (file.is_open())
        {
            while (getline(file, line))
                execute(line);

            file.close();
        }
    }
}

vector<string> Shell::get_sub_lines(const string& input)
{
    int pipefd[2];

    if (pipe(pipefd) == -1)
        throw runtime_error("pipe failed");

    pid_t pid = fork();

    if (pid == 0)
    {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        print_tree = false;
        execute(input);
        exit(EXIT_SUCCESS);
    }
    else
    {
        close(pipefd[1]);

        char buffer[512];
        ssize_t bytes_read;
        string output = "";

        while ((bytes_read = read(pipefd[0], buffer, 512)) > 0)
            output.append(buffer, bytes_read);

        wait(nullptr);
        close(pipefd[0]);

        istringstream iss(output);
        vector<string> lines;
        string line;

        while (getline(iss, line))
            lines.push_back(line);

        return lines;
    }
}

void Shell::make_regular(AST_ptr& leaf)
{
    if (leaf->type == AST::TILDE)
    {
        leaf->type = AST::REGULAR;

        string home;
        vars.get("HOME", home);

        leaf->data = home;
    }
    else if (leaf->type == AST::VAR)
    {
        leaf->type = AST::REGULAR;

        string var;
        vars.get(leaf->data, var);

        leaf->data = var;
    }
    else if (leaf->type == AST::SQ_STRING)
    {
        leaf->type = AST::REGULAR;
    }
    else if (leaf->type == AST::ESCAPE)
    {
        leaf->type = AST::REGULAR;

        if (leaf->data == "a") leaf->data = "\a";
        if (leaf->data == "b") leaf->data = "\b";
        if (leaf->data == "e") leaf->data = "\e";
        if (leaf->data == "f") leaf->data = "\f";
        if (leaf->data == "n") leaf->data = "\n";
        if (leaf->data == "r") leaf->data = "\r";
        if (leaf->data == "t") leaf->data = "\t";
        if (leaf->data == "v") leaf->data = "\v";
    }
    else
    {
        for (auto& child : leaf->children)
            make_regular(child);
    }
}

void cartesian_prod(const vector<vector<string>>& vecs, vector<string>& current, vector<vector<string>>& result, size_t depth = 0)
{
    if (depth == vecs.size())
    {
        result.push_back(current);
        return;
    }

    for (const string& s : vecs[depth])
    {
        current.push_back(s);
        cartesian_prod(vecs, current, result, depth + 1);
        current.pop_back();
    }
}

vector<vector<string>> cartesian_prod(const vector<vector<string>>& vecs)
{
    vector<vector<string>> result;
    vector<string> current;

    cartesian_prod(vecs, current, result);

    return result;
}

vector<AST_ptr> Shell::expand_word(const AST_ptr& word)
{
    vector<vector<string>> results;

    for (int i = 0; i < word->children.size(); i++)
        if (word->children[i]->type == AST::SUBCOMMAND)
            results.push_back(get_sub_lines(word->children[i]->data));

    vector<vector<string>> cartesian = cartesian_prod(results);

    vector<AST_ptr> ret;

    for (const auto& config : cartesian)
    {
        int i = 0;
        ret.push_back(make_unique<AST>(AST::WORD, ""));

        for (const auto& child : word->children)
            if (child->type == AST::SUBCOMMAND)
                ret.back()->children.push_back(make_unique<AST>(AST::REGULAR, config[i++]));
            else
                ret.back()->children.push_back(make_unique<AST>(child->type, child->data));
    }

    // if (ret.empty())
    //     ret.push_back(make_unique<AST>(AST::WORD, ""));

    return ret;
}

void Shell::sub_commands(AST_ptr& tree)
{
    if (tree->type != AST::COMMAND)
    {
        for (auto& child : tree->children)
            sub_commands(child);
    }
    else
    {
        vector<AST_ptr> new_children;

        for (const auto& word : tree->children)
        {
            vector<AST_ptr> new_words = expand_word(word);

            new_children.insert(new_children.end(), make_move_iterator(new_words.begin()), make_move_iterator(new_words.end()));
        }

        tree->children = move(new_children);

        for (const auto& word : tree->children)
        {
            for (const auto& reg : word->children)
                word->data += reg->data;

            word->children.clear();
        }

        if (!tree->children.empty())
        {
            string alias;

            if (aliases.get(tree->children[0]->data, alias))
            {
                istringstream iss(alias);
                vector<string> words;
                string word;

                while (iss >> word)
                    words.push_back(word);

                tree->children.erase(tree->children.begin());

                for (int i = 0; i < words.size(); i++)
                    tree->children.insert(tree->children.begin() + i, make_unique<AST>(AST::WORD, words[i]));
            }

            tree->data = tree->children[0]->data;
        }
    }
}

int Shell::execute_tree(const AST_ptr& tree)
{
    if (tree->type == AST::COMMAND)
        return execute_command(tree);

    if (tree->type == AST::PIPE)
        return execute_pipeline(tree);

    if (tree->type == AST::COMMA)
    {
        int status = 0;

        for (const auto& child : tree->children)
            status = execute_tree(child);

        return status;
    }

    if (tree->type == AST::LOGICAL && tree->data == "&&")
    {
        int status = execute_tree(tree->children[0]);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            return execute_tree(tree->children[1]);

        return status;
    }

    if (tree->type == AST::LOGICAL && tree->data == "||")
    {
        int status = execute_tree(tree->children[0]);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            return status;

        return execute_tree(tree->children[1]);
    }

    return 0;
}

char** get_argv(const AST_ptr& command)
{
    int argc = command->children.size();
    char** argv = new char* [argc + 1];

    for (int i = 0; i < argc; ++i)
    {
        argv[i] = new char[command->children[i]->data.size() + 1];
        strcpy(argv[i], command->children[i]->data.c_str());
    }

    argv[argc] = nullptr;

    return argv;
}

void free_argv(char** argv)
{
    if (!argv)
        return;

    for (int i = 0; argv[i] != nullptr; i++)
        delete[] argv[i];

    delete[] argv;
}

int Shell::execute_command(const AST_ptr& command)
{
    if (command->children.empty())
        return 0;

    int argc = command->children.size();
    char** argv = get_argv(command);

    int status = exec_and_return(argc, argv);

    free_argv(argv);

    return status;
}

int Shell::execute_pipeline(const AST_ptr& pipeline)
{
    size_t n = pipeline->children.size();
    vector<int[2]> pipes(n - 1);
    pid_t last_pid;

    for (auto& fds : pipes)
        if (pipe(fds) == -1)
            throw runtime_error("pipe failed");

    for (int i = 0; i < n; i++)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            if (i > 0)
                dup2(pipes[i - 1][0], STDIN_FILENO);

            if (i < n - 1)
                dup2(pipes[i][1], STDOUT_FILENO);

            for (const auto& fds : pipes)
            {
                close(fds[0]);
                close(fds[1]);
            }

            if (pipeline->children[i]->children.empty())
                exit(EXIT_SUCCESS);

            int argc = pipeline->children[i]->children.size();
            char** argv = get_argv(pipeline->children[i]);

            exec_and_exit(argc, argv);
        }
        else if (pid > 0 && i == n - 1)
            last_pid = pid;
    }

    for (const auto& fds : pipes)
    {
        close(fds[0]);
        close(fds[1]);
    }

    int status;
    int last_status = 0;

    for (int i = 0; i < n; i++)
    {
        pid_t pid = wait(&status);

        if (pid == last_pid)
            last_status = status;
    }

    if (WIFEXITED(last_status))
        return WEXITSTATUS(last_status);

    return 1;
}

bool Shell::is_builtin(const string& name)
{
    return builtins.find(name) != builtins.end();
}

bool absolute_or_relative(const string& name)
{
    return name.find('/') != string::npos;
}

bool Shell::is_executable(const string& name)
{
    struct stat file_stat;

    if (absolute_or_relative(name))
        return (stat(name.c_str(), &file_stat) == 0 && S_ISREG(file_stat.st_mode) && (file_stat.st_mode & S_IXUSR));

    const char* path = getenv("PATH");

    if (!path)
        return false;

    istringstream iss(path);
    string directory;

    while (getline(iss, directory, ':'))
    {
        string file_path = directory + '/' + name;

        if (stat(file_path.c_str(), &file_stat) == 0 && S_ISREG(file_stat.st_mode) && (file_stat.st_mode & S_IXUSR))
            return true;
    }

    return false;
}

bool Shell::is_script(const string& name, string& interpreter)
{
    ifstream file(name);

    if (!file.is_open())
        return false;

    string first_line;
    getline(file, first_line);

    file.close();

    if (first_line.rfind("#!", 0) != 0)
        return false;

    interpreter = first_line.substr(2);

    return true;
}

int Shell::exec_and_return(int argc, char** argv)
{
    if (is_builtin(argv[0]))
        return builtins[argv[0]](argc, argv);

    if (is_executable(argv[0]))
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            execvp(argv[0], argv);
            cerr << name << ": exec and return failed\n";
            exit(EXIT_FAILURE);
        }

        int status;
        wait(&status);

        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }

    string interpreter;
    if (!is_script(argv[0], interpreter))
    {
        cerr << argv[0] << ": command not found\n";
        return EXIT_FAILURE;
    }

    if (access(argv[0], X_OK) != 0)
    {
        cerr << name << ": ";
        perror(argv[0]);
        return EXIT_FAILURE;
    }

    if (!is_executable(interpreter))
    {
        cerr << argv[0] << ": interpreter not found\n";
        return EXIT_FAILURE;
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        vector<char*> new_argv;

        new_argv.push_back(const_cast<char*>(interpreter.c_str()));

        for (int i = 0; i < argc; i++)
            new_argv.push_back(argv[i]);

        new_argv.push_back(nullptr);

        execvp(new_argv[0], new_argv.data());
        cerr << name << ": script exec and return failed\n";
        exit(EXIT_FAILURE);
    }

    int status;
    wait(&status);

    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

void Shell::exec_and_exit(int argc, char** argv)
{
    if (is_builtin(argv[0]))
        exit(builtins[argv[0]](argc, argv));

    if (is_executable(argv[0]))
    {
        execvp(argv[0], argv);
        cerr << name << ": exec and exit failed\n";
        exit(EXIT_FAILURE);
    }

    string interpreter;
    if (!is_script(argv[0], interpreter))
    {
        cerr << argv[0] << ": command not found\n";
        exit(EXIT_FAILURE);
    }

    if (access(argv[0], X_OK) != 0)
    {
        cerr << name << ": ";
        perror(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!is_executable(interpreter))
    {
        cerr << argv[0] << ": interpreter not found\n";
        exit(EXIT_FAILURE);
    }

    vector<char*> new_argv;

    new_argv.push_back(const_cast<char*>(interpreter.c_str()));

    for (int i = 0; i < argc; i++)
        new_argv.push_back(argv[i]);

    new_argv.push_back(nullptr);

    execvp(new_argv[0], new_argv.data());
    cerr << name << ": script exec and exit failed\n";
    exit(EXIT_FAILURE);
}

int Shell::__exit(int argc, char** argv)
{
    if (argc > 2)
    {
        cerr << "exit: too many arguments\n";
        return 1;
    }

    int status = 0;

    if (argc == 2)
    {
        try
        {
            status = stoi(argv[1]);
        }
        catch (...)
        {
            cerr << "exit: status is invalid\n";
            return 1;
        }
    }

    exit(status);
}

int Shell::__cd(int argc, char** argv)
{
    if (argc > 2)
    {
        cerr << "cd: too many arguments\n";
        return 1;
    }

    char* path;

    if (argc == 1)
    {
        path = getenv("HOME");

        if (!path)
        {
            cerr << "cd: $HOME is not defined\n";
            return 1;
        }
    }
    else
        path = argv[1];

    if (chdir(path) == -1)
    {
        perror("cd");
        return 1;
    }

    char* pwd = getcwd(nullptr, 0);

    vars.set("PWD", string(pwd));

    free(pwd);

    return 0;
}

int Shell::__set(int argc, char** argv)
{
    if (argc > 3)
    {
        cerr << "set: too many arguments\n";
        return 1;
    }

    if (argc == 1)
    {
        for (const auto& pair : vars.data)
            if (!vars.is_exported(pair.first))
                cout << pair.first << " = " << pair.second << "\e[0m\n";
    }
    else if (argc == 2)
        vars.set(argv[1], "");
    else if (argc == 3)
        vars.set(argv[1], argv[2]);

    return 0;
}

int Shell::__unset(int argc, char** argv)
{
    if (argc == 2)
        return !vars.unset(argv[1]);

    return 0;
}

int Shell::__export(int argc, char** argv)
{
    if (argc == 2)
        return !vars.export_(argv[1]);

    return 0;
}

int Shell::__alias(int argc, char** argv)
{
    if (argc > 3)
    {
        cerr << "alias: too many arguments\n";
        return 1;
    }

    if (argc == 1)
    {
        for (const auto& pair : aliases.data)
            cout << pair.first << " = " << pair.second << "\n";
    }
    else if (argc == 2)
        aliases.set(argv[1], "");
    else if (argc == 3)
        aliases.set(argv[1], argv[2]);

    return 0;
}

int Shell::__unalias(int argc, char** argv)
{
    if (argc == 2)
        return !aliases.unset(argv[1]);

    return 0;
}

int Shell::__history(int argc, char** argv)
{
    if (argc == 1)
    {
        int max_size = to_string(history.size()).size();

        for (int i = 0; i < history.size(); i++)
        {
            string num = to_string(history.size() - i);
            num = string(max_size - num.size(), ' ') + num;

            cout << "  " << num << "  " << history[i] << endl;
        }
    }
    else if (argc == 2 && string(argv[1]) == "-c")
        history.clear();

    return 0;
}
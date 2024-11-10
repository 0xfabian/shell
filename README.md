# Basic Shell

A simple command-line shell inspired by the [Fish shell](https://fishshell.com/), offering basic shell functionality such as executing commands, managing variables, and supporting common operations like piping and command substitution.

![Shell Demo](shell.gif)

## Features

- **Variables**: Supports regular shell and environment variables.
- **Autocompletion**: Supports autocompletion for commands, file names, and variables, making navigation faster.
- **History**: Maintains a history of commands, so you can easily recall previously executed commands.
- **Text Selection**: Provides basic text selection capabilities within the shell for convenience.
- **Aliases**: Enables defining simple aliases for frequently used commands, saving time on repetitive tasks.
- **Operators**: Implements standard shell operators such as `|`, `||`, `&&`, and `;` with correct precedence.
- **Tilde Expansion**: Expands the `~` symbol to represent the user’s home directory.
- **Command Substitution**: Supports command substitution using `()` for dynamic command execution.
- **Argument Expansion**: Handles argument expansion, allowing the output of commands to automatically become arguments when necessary.

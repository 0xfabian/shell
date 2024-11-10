# Basic Shell

A simple command-line shell inspired by the [fish](https://fishshell.com/) shell, offering basic shell functionality such as executing commands, managing variables, and supporting common operations like piping and command substitution.

![Shell Demo](shell.gif)

## Features

- Supports regular shell and environment variables.
- Autocompletion for commands, file names, and variables.
- Maintains a history of commands.
- Line editor with basic text selection capabilities.
- Enables defining simple aliases for frequently used commands.
- Implements standard shell operators such as `|`, `||`, `&&`, and `;` with correct precedence.
- Supports command substitution using `()`.
- Expands the `~` symbol to represent the user’s home directory.
- Handles argument expansion, allowing the output of commands to automatically become arguments when necessary.

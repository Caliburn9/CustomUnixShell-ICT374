# Custom Unix Shell

A fully functional Unix shell written in C, supporting pipelines, background execution, redirection, command history, and a custom command-line parser.

## Features

- **Command Execution** – Runs external programs via `execvp()` with PATH resolution
- **Pipelines (`|`)** – Chains multiple commands together, connecting stdout to stdin
- **Background Jobs (`&`)** – Execute commands without blocking the shell
- **Sequential Execution (`;`)** – Runs commands one after another
- **Redirection** – Standard input (`<`), output (`>`), and error (`2>`)
- **Wildcard Expansion** – `*` and `?` using `glob()`
- **Built-in Commands** – `cd`, `pwd`, `history`, `prompt`, `exit`
- **Command History** – Navigate with arrow keys (Up/Down), repeat with `!!`, `!n`, `!string`
- **Custom Parser** – Handles quotes (`'`/`"`), backslash escapes, and special characters per Extended BNF grammar
- **Signal Handling** – Ignores `SIGINT`, `SIGQUIT`, `SIGTSTP` for the shell; restores defaults for child processes

## Quick Start

### Prerequisites
- Linux or WSL (Ubuntu recommended)
- GCC and Make

### Build
```bash
git clone https://github.com/Caliburn9/CustomUnixShell-ICT374.git
cd CustomUnixShell-ICT374/shell
make
```

### Run
```bash
./myshell
```

### Example Usage
```bash
# Pipeline with redirection
% ls *.c | grep "shell" > output.txt

# Background job
% sleep 30 &

# Sequential commands
% echo first ; echo second

# History navigation
% !!
% !42
% !echo
```

## Project Structure

| File | Purpose |
|------|---------|
| `myshell.c` | Main loop, signal handling, terminal input |
| `token.c` | Tokenizer per Extended BNF definition |
| `command.c` | Separates tokens into command structures |
| `shell_builtins.c` | Built-in commands (cd, pwd, history, prompt, exit) |
| `shell_utils.c` | Helper functions for execution, redirection, pipelines |
| `input.c` | Termios raw mode input with line editing and history |

## Technical Deep Dive

- **Terminal Raw Mode** – `termios` library enables character-by-character reading, arrow key support, and custom line editing without ncurses
- **Process Management** – `fork()`, `execvp()`, `pipe()`, `dup2()` for redirection and pipelines
- **Signal Handling** – Custom handlers for `SIGCHLD` (zombie reaping) and ignored signals for shell process
- **Parser** – Two-pass tokenization to expand history commands (`!`) before final parsing
- **History** – Circular static array with shift-upon-full strategy; arrow key navigation via raw input codes

## Limitations & Future Work

- History does not persist between sessions (file-based storage planned)
- No job control beyond background/sequential
- Tab completion not yet implemented

## License

MIT License – see [LICENSE](LICENSE) file for details.

## Author

**Nabeel Aziz** – [GitHub](https://github.com/Caliburn9)

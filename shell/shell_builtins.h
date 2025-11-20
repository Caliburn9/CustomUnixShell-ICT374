// shell_builtins.h - header file for module that handles builtin commands for myshell project

void builtinPrompt(char *curr_pr, char *new_pr, int max_size);

void builtinPwd();

void builtinCd(char *path);

void builtinHistory(char *history[], int history_count);

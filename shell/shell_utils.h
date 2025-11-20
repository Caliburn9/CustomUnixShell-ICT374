// shell_utils.h - header for utility functions for myshell project

void catchChild(int signo);

void setupRedirection(Command *command);

int globErrHandler(const char *path, int err);

int hasWildcard(char *token);

int expandWildcards(char *token[], int *size, int max_size);

int findPipeCommands(Command commands[], int start, int size);

void executeCommand(Command *cmd);

void executePipeline(Command commands[], int start, int end);

void freeStringArray(char *arr[], int n);

char *expandHistory(char *token, char *history[], int history_count);
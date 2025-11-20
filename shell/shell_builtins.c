// shell_builtins.c - implementation file for the module that handles builtin commands for myshell project

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "shell_builtins.h"
#include "command.h"

void builtinPrompt(char *curr_pr, char *new_pr, int max_size) {
	if (new_pr == NULL) {
		fprintf(stderr, "prompt: missing argument\n");
		return;
	}
	if (strlen(new_pr) >= max_size) {
		fprintf(stderr, "prompt: argument too long\n");
		return;
	}
	strcpy(curr_pr, new_pr);
}

void builtinPwd() {
	char cwd[256];

	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("%s\n", cwd);
		fflush(stdout);
	} else {
		fprintf(stderr, "pwd: error getting current directory\n");
	}
}

void builtinCd(char *path) {
	if (path == NULL) {
		path = getenv("HOME");
	}
	if (chdir(path) != 0) {
		fprintf(stderr, "cd: error changing directory\n");
	}
}

void builtinHistory(char *history[], int history_count) {
	int i;
	for (i = 0; i < history_count; ++i) {
		printf("%d\t%s\n", i+1, history[i]);
	}
}
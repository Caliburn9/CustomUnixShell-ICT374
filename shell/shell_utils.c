// shell_utils.c - implementation file for utility functions for myshell project

#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <ctype.h>
#include "command.h"

void catchChild(int signo) {
	int more = 1;	// more zombies to claim
	pid_t pid;	// pid of the zombie
	int status;	// termination status of the zombie

	while (more) {
		pid = waitpid(-1, &status, WNOHANG);
		if (pid <= 0) {
			more = 0;
		}
	}
}

void setupRedirection(Command *command) {
	int fd_in, fd_out, fd_err;

	// input redirection
	if (command->stdin_file != NULL) {
		fd_in = open(command->stdin_file, O_RDONLY, 0666);
		if (fd_in < 0) {
			fprintf(stderr, "open: unable to open stdin file\n");
			exit(1);
		}
		if ((dup2(fd_in, 0)) < 0) {
			fprintf(stderr, "dup2: unable to duplicate to new fd\n");
			exit(1);
		}
		close(fd_in);
	}
	// output redirection
	if (command->stdout_file != NULL) {
		fd_out = open(command->stdout_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd_out < 0) {
			fprintf(stderr, "open: unable to open stdin file\n");
			exit(1);
		}
		if ((dup2(fd_out, 1)) < 0) {
			fprintf(stderr, "dup2: unable to duplicate to new fd\n");
			exit(1);
		}
		close(fd_out);
	}
	// error redirection
	if (command->stderr_file != NULL) {
		fd_err = open(command->stderr_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd_err < 0) {
			fprintf(stderr, "open: unable to open stdin file\n");
			exit(1);
		}
		if ((dup2(fd_err, 2)) < 0) {
			fprintf(stderr, "dup2: unable to duplicate to new fd\n");
			exit(1);
		}
		close(fd_err);
	}
}

int globErrHandler(const char *path, int err) {
	fprintf(stderr, "glob: error accessing %s: %s\n", path, strerror(err));
	return 0;
}

int hasWildcard(char *token) {
	if (strchr(token, '*') != NULL || strchr(token, '?') != NULL) {
		return 1;
	}
	return 0;
}

void expandWildcards(char *token[], int *size, int max_size) {
	glob_t results;
	int i, j, new_size = 0;
	char *temp_token[max_size];

	// loop through all tokens
	for (i = 0; token[i] != NULL; ++i) {
		// clear results
		memset(&results, 0, sizeof(results));

		// if token has a wildcard
		if (hasWildcard(token[i])) {
			// expand wildcard
			if (glob(token[i], 0, globErrHandler, &results) == 0) {
				// copy all matches
				for (j = 0; j < results.gl_pathc; j++) {
					if (new_size >= max_size - 1) {
						break;
					}
					temp_token[new_size] = strdup(results.gl_pathv[j]);
					new_size++;
				}
				globfree(&results);
			} else {
				// else, insert token to temp token array
				temp_token[new_size] = strdup(token[i]);
				new_size++;
			}
		} else {
		// not a wildcard, copy token as is
			temp_token[new_size] = strdup(token[i]);
			new_size++;
		}
	}

	temp_token[new_size] = NULL;

	// copy temp token contents to token array and update token size
	for (i = 0; i < new_size; ++i) {
		token[i] = temp_token[i];
	}
	token[new_size] = NULL;
	*size = new_size;
}

int findPipeCommands(Command commands[], int start, int size) {
	int curr = start;
	while (curr < size - 1 && strcmp(commands[curr].sep, "|") == 0) {
		curr++;
	}
	return curr;
}

void executeCommand(Command *cmd) {
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	setupRedirection(cmd);
	execvp(cmd->argv[0], cmd->argv);
	fprintf(stderr, "myshell: %s: command not found\n", cmd->argv[0]);
	exit(1);
}

void executePipeline(Command commands[], int start, int end) {
	int n_pipes = end - start;
	int p[n_pipes][2], i, j, status;
	pid_t pid, child_pids[n_pipes + 1];

	// open pipes
	for (i = 0; i < n_pipes; ++i) {
		if (pipe(p[i]) < 0) {
			fprintf(stderr, "execute pipeline: pipe call");
			return;
		}
	}

	for (i = start; i <= end; ++i) {
		if ((pid = fork()) < 0) {
			fprintf(stderr, "execute pipeline: fork call");
			return;
		}

		if (pid == 0) { // child
			// close all pipe ends in child
			for (j = 0; j < n_pipes; ++j) {
				if (i > start && j == i - start - 1) {
					// this is the pipe we read from - keep [0] open
					close(p[j][1]);  // Close write end only
				} else if (i < end && j == i - start) {
					// this is the pipe we write to - keep [1] open  
					close(p[j][0]);  // Close read end only
				} else {
					// close both ends of all other pipes
					close(p[j][0]);
					close(p[j][1]);
				}
			}

			// open necessary pipe end
			if (i != start) {
				if ((dup2(p[i - start - 1][0], 0)) < 0) {
					fprintf(stderr, "dup2: unable to duplicate pipe read\n");
					exit(1);
				}
				close(p[i - start - 1][0]);
			}
			if (i != end) {
				if ((dup2(p[i - start][1], 1)) < 0) {
					fprintf(stderr, "dup2: unable to duplicate pipe write\n");
					exit(1);
				}
				close(p[i - start][1]);
			}

			// execute command
			executeCommand(&commands[i]);
		} else {
			child_pids[i - start] = pid;
		}
	}

	// close all pipe ends in parent after forking
	for (i = 0; i < n_pipes; ++i) {
		close(p[i][0]);
		close(p[i][1]);
	}

	// if the last command is a background job
	if (strcmp(commands[end].sep, "&") == 0) {
		// return to main
		return;
	} else {
	// wait for the jobs to finish
		for (i = 0; i < n_pipes + 1; ++i) {
			waitpid(child_pids[i], &status, 0);
		}
	}
}

void freeStringArray(char *arr[], int n) {
	int i;
	for (i = 0; i < n; ++i) {
		free(arr[i]);
	}
}

char *expandHistory(char *command, char *history[], int history_count) {
	int i;
	
	// check if history is not empty
	if (history_count <= 0) {
		return NULL;
	}
	
	// last command from history
	if (strcmp(command, "!!") == 0) {
		return history[history_count - 1];
	// command by number from history
	} else if (isdigit(command[1])) {
		i = atoi(&command[1]);
		if (i > 0 && i <= history_count) {
			return history[i - 1];
		} else {
			return NULL;
		}
	// command by string from history
	} else {
		char *search_str = &command[1];
		for (i = history_count - 1; i >= 0; --i) {
			if (strncmp(history[i], search_str, strlen(search_str)) == 0) {
				return history[i];
			}
		}
		return NULL;
	}
	
	return NULL;
}
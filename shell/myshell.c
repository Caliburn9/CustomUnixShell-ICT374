// myshell.c main program

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "token.h"
#include "command.h"
#include "shell_builtins.h"
#include "shell_utils.h"
#include "input.h"

#define MAX_NUM_CMDLINE MAX_NUM_TOKENS
#define MAX_PROMPT_SIZE	256
#define MAX_HISTORY		1000

int main(void) {
	char message[MAX_NUM_CMDLINE], *tokens[MAX_NUM_TOKENS], *history[MAX_HISTORY];
	char prompt[MAX_PROMPT_SIZE] = "%", expanded_message[MAX_NUM_CMDLINE] = {0}, *linept;
	Command commands[MAX_NUM_COMMANDS];
	int tokenNum, commandNum, again, status, i, pipe_start, pipe_end, len;
	int history_count = 0, exp_pos = 0;
	pid_t pid;
	struct sigaction intr_act, quit_act, tstp_act, chld_act;

	// signal handler
	intr_act.sa_handler = SIG_IGN;
	quit_act.sa_handler = SIG_IGN;
	tstp_act.sa_handler = SIG_IGN;
	chld_act.sa_handler = catchChild;

	// signal flags
	intr_act.sa_flags = 0;
	quit_act.sa_flags = 0;
	tstp_act.sa_flags = 0;
	chld_act.sa_flags = 0;

	// signal masks
	sigemptyset(&(intr_act.sa_mask));
	sigemptyset(&(quit_act.sa_mask));
	sigemptyset(&(tstp_act.sa_mask));
	sigemptyset(&(chld_act.sa_mask));

	// sigaction error handling
	if (sigaction(SIGINT, &intr_act, NULL) != 0) {
		perror("sigaction error: SIGINT");
		exit(1);
	}
	if (sigaction(SIGQUIT, &quit_act, NULL) != 0) {
		perror("sigaction error: SIGQUIT");
		exit(1);
	}
	if (sigaction(SIGTSTP, &tstp_act, NULL) != 0) {
		perror("sigaction error: SIGTSTP");
		exit(1);
	}
	if (sigaction(SIGCHLD, &chld_act, NULL) != 0) {
		perror("sigaction error: SIGCHLD");
		exit(1);
	}

	while (1) {
		// print prompt
		printf("%s ", prompt);
		fflush(stdout);
		
		enable_raw_mode();

		// read line from user
		again = 1;
		while (again) {
			again = 0;
			linept = read_command_line(message, MAX_NUM_CMDLINE, prompt, history, history_count);
			if (linept == NULL) {
				if (errno == EINTR) {
					again = 1; // signal interruption, read again
				}
			}
		}
		printf("\r\n");

		// tokenize line
		if ((tokenNum = tokenise(message, tokens)) < 0) {
			fprintf(stderr, "myshell: error in tokenising message\n");
			continue;
		}
		
		// expand history
		expanded_message[0] = '\0';
		exp_pos = 0;
		for (i = 0; i < tokenNum; ++i) {
			// check if token is ! command
            if (tokens[i][0] == '!' && tokens[i][1] != '\0') {
                char *found = expandHistory(tokens[i], history, history_count);
				if (found == NULL) {
					len = strlen(tokens[i]);
					if (exp_pos + len < MAX_NUM_CMDLINE - 1) {
						// insert token as is to expanded message if not found
						strcpy(&expanded_message[exp_pos], tokens[i]);
						exp_pos += len;
					}
                } else {
                    len = strlen(found);
					if (exp_pos + len < MAX_NUM_CMDLINE - 1) {
						// insert found history to expanded message
						strcpy(&expanded_message[exp_pos], found);
						exp_pos += len;
					}
                }
            } else {
				// insert non-! token as is to expanded message
				len = strlen(tokens[i]);
				if (exp_pos + len < MAX_NUM_CMDLINE - 1) {
					strcpy(&expanded_message[exp_pos], tokens[i]);
					exp_pos += len;
				}
			}
			
			// add space between each token
			if (i < tokenNum - 1 && exp_pos < MAX_NUM_CMDLINE - 1) {
				expanded_message[exp_pos] = ' ';
				exp_pos++;
			}
        }
		expanded_message[exp_pos] = '\0';
		
		// copy expanded message and re-tokenize
		strcpy(message, expanded_message);
		freeStringArray(tokens, tokenNum);
		if ((tokenNum = tokenise(message, tokens)) < 0) {
			fprintf(stderr, "myshell: error in tokenising message\n");
			continue;
		}

		// expand wildcards
		expandWildcards(tokens, &tokenNum, MAX_NUM_TOKENS);

		// identify and separate commands
		if ((commandNum = separateCommands(tokens, commands)) < 0) {
			fprintf(stderr, "myshell: error in separating commands\n");
			continue;
		}
		
		// add line to history
		if (history_count < MAX_HISTORY) {
			history[history_count] = strdup(message);
			history_count++;
		} else {
			free(history[0]);
			// shift commands in history up
			for (i = 0; i < MAX_HISTORY - 1; ++i) {
				history[i] = history[i + 1];
			}
			history[MAX_HISTORY - 1] = strdup(message);
		}
		
		disable_raw_mode();

		// analyse the command line
		for (i = 0; i < commandNum; ++i) {
		// for each job in the command line
			// if the job is exit command, then terminate the program
			if (strcmp(commands[i].argv[0], "exit") == 0) {
				freeStringArray(tokens, tokenNum);
				freeStringArray(history, history_count);
				exit(0);
			}
			if (strcmp(commands[i].sep, "|") == 0) {
				// identify commands connected by pipes
				pipe_start = i;
				pipe_end = findPipeCommands(commands, pipe_start, commandNum);
				// execute pipeline commands
				executePipeline(commands, pipe_start, pipe_end);
				// skip all commands in the pipeline and continue to next command in loop
				i = pipe_end;
				continue;
			}
			// handle history
			if (strcmp(commands[i].argv[0], "history") == 0) {
				builtinHistory(history, history_count);
				continue;
			}
			// handle prompt change
			if (strcmp(commands[i].argv[0], "prompt") == 0) {
				builtinPrompt(prompt, commands[i].argv[1], MAX_PROMPT_SIZE);
				continue;
			}
			// handle pwd
			if (strcmp(commands[i].argv[0], "pwd") == 0) {
				builtinPwd();
				continue;
			}
			// handle cd
			if (strcmp(commands[i].argv[0], "cd") == 0) {
				builtinCd(commands[i].argv[1]);
				continue;
			}
			// create child processes (and redirections) to execute that job
			pid = fork();
			if (pid < 0) {
				perror("fork error");
				exit(1);
			}
			if (pid == 0) { // execute command in child
				executeCommand(&commands[i]);
			}
			// if the job is a background job (ie. ended with &)
			if (strcmp(commands[i].sep, "&") == 0) {
				// go back to for loop
				continue;
			} else {
			// wait for the job to finish
				waitpid(pid, &status, 0);
			}
			fflush(stdout);
		}
		freeStringArray(tokens, tokenNum);
	}

	exit(0);
}
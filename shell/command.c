// file:		command.c
// purpose;		to separate a list of tokens into a sequence of commands.
// assumptions:		any two successive commands in the list of tokens are separated 
//			by one of the following command separators:
//				"|"  - pipe to the next command
//				"&"  - shell does not wait for the proceeding command to terminate
//				";"  - shell waits for the proceeding command to terminate

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"

// return 1 if the token is a command separator
// return 0 otherwise
//
int separator(char *token)
{
	int i=0;
	char *commandSeparators[] = {pipeSep, conSep, seqSep, NULL};

	while (commandSeparators[i] != NULL) {
		if (strcmp(commandSeparators[i], token) == 0) {
			return 1;
		}
		++i;
	}

	return 0;
}

// fill one command structure with the details
//
void fillCommandStructure(Command *cp, int first, int last, char *sep)
{
	cp->first = first;
	cp->last = last - 1;
	cp->sep = sep;
	cp->argv = NULL;
	cp->stdin_file = NULL;
	cp->stdout_file = NULL;
	cp->stderr_file = NULL;
}

int separateCommands(char *token[], Command command[])
{
	int i;
	int nTokens;

	// find out the number of tokens
	i = 0;
	while (token[i] != NULL) {
		++i;
	}
	nTokens = i;

	// if empty command line
	if (nTokens == 0) {
		return 0;
	}

	// check the first token
	if (separator(token[0])) {
		return -3;
	}

	// check last token, add ";" if necessary
	int lastTokenIsSeparator = separator(token[nTokens-1]);

	int first=0;   // points to the first tokens of a command
	int last;      // points to the last  tokens of a command
	char *sep;     // command separator at the end of a command
	int c = 0;         // command index
	
	for (i=0; i<nTokens; ++i) {
		last = i;
		if (separator(token[i])) {
			sep = token[i];
			if (first==last) { // two consecutive separators
				return -2;
			}
			if (c >= MAX_NUM_COMMANDS) {
				return -1;
			}
			fillCommandStructure(&(command[c]), first, last, sep);
			++c;
			first = i + 1;
		}
	}

	if (!lastTokenIsSeparator) {
		if (first < nTokens) {
			if (c >= MAX_NUM_COMMANDS) {
				return -1;
			}
			fillCommandStructure(&(command[c]), first, nTokens, seqSep);
			++c;
		}
	}

	// check the last token of the last command
	if (lastTokenIsSeparator && strcmp(token[nTokens - 1], pipeSep) == 0) { // last token is pipe separator
		return -4;
	}

	// calculate the number of commands
	int nCommands = c;

	command[nCommands].first = -1;
	command[nCommands].last = -1;
	command[nCommands].sep = NULL;
	command[nCommands].argv = NULL;
	command[nCommands].stdin_file = NULL;
	command[nCommands].stdout_file = NULL;
	command[nCommands].stderr_file = NULL;

	// check for stdin/stdout redirection in each command struct
	for (int i = 0; i < nCommands; ++i) {
		searchRedirection(token, &(command[i]));
		buildCommandArgumentArray(token, &(command[i]));
	}

	return nCommands;
}

void searchRedirection(char *token[], Command *cp) {
	for (int i = cp->first; i <= cp->last; ++i) {
		if (strcmp(token[i], "<") == 0) {
			// check if token after redirection symbol is a valid token
			if (i+1 <= cp->last && token[i+1] != NULL && !separator(token[i+1])) {
				strip_quotes(token[i+1]);
				remove_escape(token[i+1]);
				cp->stdin_file = token[i+1];
			}
		} else if (strcmp(token[i], ">") == 0) {
			// check if token after redirection symbol is a valid token
			if (i+1 <= cp->last && token[i+1] != NULL && !separator(token[i+1])) {
				strip_quotes(token[i+1]);
				remove_escape(token[i+1]);
				cp->stdout_file = token[i+1];
			}
		} else if (strcmp(token[i], "2>") == 0) {
			// check if token after redirection symbol is a valid token
			if (i+1 <= cp->last && token[i+1] != NULL && !separator(token[i+1])) {
				strip_quotes(token[i+1]);
				remove_escape(token[i+1]);
				cp->stderr_file = token[i+1];
			}
		}
	}
}

void buildCommandArgumentArray(char *token[], Command *cp) {
	int n, i, k = 0;

	// n = number of tokens in command (last - first + 1)
	//   - 2 tokens for stdin redirection
	//   - 2 tokens for stdout redirection
	//	 - 2 tokens for stderr redirection
	//   + 1 the last element in argv must be a NULL
	n = cp->last - cp->first + 2;
	if (cp->stdin_file != NULL) {
		n -= 2;
	}
	if (cp->stdout_file != NULL) {
		n -= 2;
	}
	if (cp->stderr_file != NULL) {
		n -= 2;
	}

	// dynamically allocate space for argv
	cp->argv = (char**)realloc(cp->argv, sizeof(char *) * n);
	if (cp->argv == NULL) {
		perror("realloc");
		exit(1);
	}

	// build argv
	for (i = cp->first; i <= cp->last; ++i) {
		if (strcmp(token[i], ">") == 0 || strcmp(token[i], "<") == 0 || strcmp(token[i], "2>") == 0) {
			++i; // skip off the std in/out/err redirection
		} else {
			cp->argv[k] = token[i];
			strip_quotes(cp->argv[k]);
			remove_escape(cp->argv[k]);
			++k;
		}
	}
	cp->argv[k] = NULL;
}

void strip_quotes(char *str) {
	int len = strlen(str);
    if (len < 1) return;

    // Remove opening quote if present
    if (str[0] == '\'' || str[0] == '"') {
        for (int i = 0; i < len; i++) {
            str[i] = str[i + 1];
        }
        len--; // String is now shorter
    }

    // Remove closing quote if present (check the new last character)
    if (len > 0 && (str[len - 1] == '\'' || str[len - 1] == '"')) {
        str[len - 1] = '\0';
    }
}

void remove_escape(char *str) {
    int i, j;
	
	// read index
	i = 0;
	// write index	
    j = 0; 
    
    while (str[i] != '\0') {
		// found a backslash
        if (str[i] == '\\') {
            // skip the backslash (i++) and copy the next character
            i++;
            if (str[i] != '\0') {
                str[j] = str[i];
                i++;
                j++;
            }
        } else {
            // normal character, just copy it
            str[j] = str[i];
            i++;
            j++;
        }
    }
    str[j] = '\0'; 
}

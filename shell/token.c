// token.c - implementation for token module for tokenizing a string

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

int tokenise(char line[], char *token[]) {
	int tok_index = 0, pos = 0, tok_count = 0, len = strlen(line);
	TokenState state = STATE_NORMAL, prev_state;
	char c, tok[256];
	
	while (pos < len) {
		c = line[pos];
		
		switch (state) {
			case STATE_NORMAL:
				if (strchr(tokenSeparators, c)) {
					buildToken(token, tok, &tok_index, &tok_count);	
				} else if (c == '\\') {
					addCharToToken(tok, &tok_index, c); 
					prev_state = state;
					state = STATE_ESCAPED;
				} else if (c == '\'') {
					buildToken(token, tok, &tok_index, &tok_count);
					addCharToToken(tok, &tok_index, c); 
					state = STATE_SINGLE_QUOTE;
				} else if (c == '\"') {
					buildToken(token, tok, &tok_index, &tok_count);
					addCharToToken(tok, &tok_index, c); 
					state = STATE_DOUBLE_QUOTE;
				} else if (strchr(specialChars, c)) {
					    // check for 2> special case
						if (tok_index == 1 && tok[0] == '2' && c == '>') {
							// we have lone "2" followed by ">" - merge into "2>"
							addCharToToken(tok, &tok_index, c);  // tok becomes "2>"
							buildToken(token, tok, &tok_index, &tok_count);  // Store "2>"
						} else {
							// normal case - finish current token, then store special char
							buildToken(token, tok, &tok_index, &tok_count);
							addCharToToken(tok, &tok_index, c); 
							buildToken(token, tok, &tok_index, &tok_count);
						}
				} else {
					// regular character
					// add current char to token buffer
					addCharToToken(tok, &tok_index, c);
				}
				break;

			case STATE_SINGLE_QUOTE:
				if (c == '\'') {
					addCharToToken(tok, &tok_index, c); 
					state = STATE_NORMAL;
				} else if (c == '\\') {
					addCharToToken(tok, &tok_index, c);
					prev_state = state;
					state = STATE_ESCAPED;
				} else {
					// any other character
					// add to token literally (even spaces, even special chars)
					addCharToToken(tok, &tok_index, c);
				}
				break;

			case STATE_DOUBLE_QUOTE:
				if (c == '\"') {
					addCharToToken(tok, &tok_index, c); 
					state = STATE_NORMAL;
				} else if (c == '\\') {
					addCharToToken(tok, &tok_index, c);
					prev_state = state;
					state = STATE_ESCAPED;
				} else {
					// any other character
					// add to token literally (even spaces, even special chars)
					addCharToToken(tok, &tok_index, c);
				}
				break;

			case STATE_ESCAPED:
				// add character literally to current token
				addCharToToken(tok, &tok_index, c);
				state = prev_state;
				break;

			default:
				fprintf(stderr, "tokenise: invalid state\n");
				for (int i = 0; i < tok_count; i++) {
					free(token[i]);
				}
				return -1;
		}
		pos++;
	}
	
	buildToken(token, tok, &tok_index, &tok_count);
	token[tok_count] = NULL;
	return tok_count;
}

void addCharToToken(char *tok, int *tok_index, char c) {
	tok[*tok_index] = c;
	(*tok_index)++;
}

void buildToken(char *token[], char *tok, int *tok_index, int *tok_count) {
    if (*tok_index > 0) {  // if building a token
        tok[*tok_index] = '\0';  // null-terminate
        token[*tok_count] = strdup(tok);  // copy to token array
		(*tok_count)++;
        *tok_index = 0;  // reset builder
    }
}
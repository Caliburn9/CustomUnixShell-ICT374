// token.h - header file for token module for tokenizing a string

#define MAX_NUM_TOKENS 100000
#define tokenSeparators " \t\n"
#define specialChars 	"&;|<>"

typedef enum {
	STATE_NORMAL,
	STATE_SINGLE_QUOTE,
	STATE_DOUBLE_QUOTE,
	STATE_ESCAPED
} TokenState;

int tokenise(char line[], char *token[]);

void addCharToToken(char *tok, int *tok_index, char c);

void buildToken(char *token[], char *tok, int *tok_index, int *tok_count);
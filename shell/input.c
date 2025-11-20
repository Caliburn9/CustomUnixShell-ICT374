// input.c - implementation file for input module for terminal input handling

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <ctype.h>
#include "input.h"

static struct termios original;

void enable_raw_mode() {
	struct termios raw;
	tcgetattr(STDIN_FILENO, &original);
	raw = original;
	
	// disable flags
	// no echo, no line buffering, no signals
	raw.c_lflag &= ~(ECHO | ICANON | ISIG);
	// no flow control, no CR->NL conversion
	raw.c_iflag &= ~(IXON | ICRNL);
	// no output processing
	raw.c_oflag &= ~(OPOST);
	// read 1 char at a time
	raw.c_cc[VMIN] = 1;
	// no timeout
	raw.c_cc[VTIME] = 0;
	
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
}

char *read_command_line(char *buf, int buf_size, char *prompt, char *hist[], int hist_count) {
	char c, temp[buf_size];
	int n, cur_pos = 0, hist_ind = -1, len;
	
	// start with empty buffer
	buf[0] = '\0';
	temp[0] = '\0';
	
	while(1) {
		// read one character at a time
		n = read(STDIN_FILENO, &c, 1);
		
		if (n <= 0) {
			if (errno == EINTR) {
				continue;
			}
			return NULL;
		}
		
		// newline or carriage return
		if (c == '\n' || c == '\r') {
			len = strlen(buf);
			if (len < buf_size) {
				buf[len] = '\0';
			}
			return buf;
		// arrow keys (ESC[<C>)
		} else if (c == 27) {
			handle_arrow_keys(buf, buf_size, temp, &cur_pos, hist, hist_count, &hist_ind);
		// backspace
		} else if (c == 127 || c == 8) {
			handle_backspace(buf, &cur_pos);
		// regular character
		} else if (isprint(c)) {
			handle_character(buf, buf_size, &cur_pos, c);
		}
		redraw_line(buf, cur_pos, prompt);
	}
}

void handle_arrow_keys(char *buf, int buf_size, char *temp, int *cur_pos, char *hist[], int hist_count, int *hist_ind) {
	char seq[2];
	
	// read the next two characters to verify its an arrow key
	if (read(STDIN_FILENO, &seq[0], 1) != 1) {
		return;
	}
	if (read(STDIN_FILENO, &seq[1], 1) != 1) {
		return;
	}
	
	if (seq[0] == '[') {
		switch(seq[1]) {
			// up
			case 'A':
				if (hist_count > 0) {
					// first time - save current line
					if (*hist_ind == -1) {
						strcpy(temp, buf);
						*hist_ind = hist_count - 1;
					} else if (*hist_ind > 0) {
						(*hist_ind)--;
					}
					strcpy(buf, hist[*hist_ind]);
					*cur_pos = strlen(buf);
				}
				break;
			// down
			case 'B':
				if (*hist_ind != -1) {
					(*hist_ind)++;
					if (*hist_ind >= hist_count) {
						// reached newest - restore original line
						(*hist_ind) = -1;
						strcpy(buf, temp);
					} else {
						strcpy(buf, hist[*hist_ind]);
					}
					*cur_pos = strlen(buf);
				}
				break;
			// right
			case 'C':
				if (*cur_pos < buf_size) {
					(*cur_pos)++;
				}
				break;		
			// left
			case 'D':
				if (*cur_pos > 0) {
					(*cur_pos)--;
				}
				break;			
		}
	}
}

void handle_backspace(char *buf, int *cur_pos) {
	if (*cur_pos > 0) {
		memmove(&buf[*cur_pos - 1], &buf[*cur_pos], strlen(buf) - *cur_pos + 1);
		(*cur_pos)--;
	}
} 

void handle_character(char *buf, int buf_size, int *cur_pos, char c) {
	if (*cur_pos < buf_size - 1) {
		memmove(&buf[*cur_pos + 1], &buf[*cur_pos], strlen(buf) - *cur_pos + 1);
		buf[*cur_pos] = c;
		(*cur_pos)++;
	}
}

void redraw_line(char *buf, int cur_pos, char *prompt) {
	// clear entire line (\033[K) and move to start (\r)
	printf("\r\033[K");
	
	// print prompt and current buffer
	printf("%s %s", prompt, buf);
	
	// move cursor to current position: prompt + space + cur_pos
	// moves cursor to the start of line (\r)
	// move cursor forward N colums (\033[%dC
	printf("\r\033[%dC", (int)(strlen(prompt) + 1 + cur_pos));
	
	fflush(stdout);
}
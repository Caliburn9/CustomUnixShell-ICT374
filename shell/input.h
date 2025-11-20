// input.h - header file for input module for terminal input handling

#define MAX_INPUT 100000

void enable_raw_mode();

void disable_raw_mode();

char *read_command_line(char *buf, int buf_size, char *prompt, char *hist[], int hist_count);

void handle_arrow_keys(char *buf, int buf_size, char *temp, int *cur_pos, char *hist[], int hist_count, int *hist_ind);

void handle_backspace(char *buf, int *cur_pos);

void handle_character(char *buf, int buf_size, int *cur_pos, char c);

void redraw_line(char *buf, int cur_pos, char *prompt);
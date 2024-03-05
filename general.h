#include <stdlib.h>
#include <ctype.h>

#define MAX_STRING 400
#define MC_MAX 10
#define HISTORY_MAX 20
#define WILDCARDS_MAX 10
#define ALIASES_MAX 15
#define FDS_NUMBER 5
// You can change the above if you want

#define READ 0
#define WRITE 1

#ifndef GENERAL
#define GENERAL

typedef struct{
    char aliasname[MAX_STRING];
    char command[MAX_STRING];
} alias;

typedef struct wc_info{
    char* wildcard;
    int result;
} wc_info;

typedef struct minicommand{
    char* command;
    char* arguments[50];
    char* red_in_args[50];
    char* red_out_trunc_args[50];
    char* red_out_append_args[50];
    bool pipe_input, pipe_output, red_in, red_out_trunc, red_out_append, bg;
    int arg_count, red_in_args_count, red_out_trunc_args_count, red_out_append_args_count, wc_counter;
    wc_info* wc[WILDCARDS_MAX];
} minicommand;
#endif
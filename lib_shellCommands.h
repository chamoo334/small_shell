#ifndef LIB_SHELLCOMMANDS_H_INCLUDED
#define LIB_SHELLCOMMANDS_H_INCLUDED

#include "lib_linkedProcesses.h"

typedef struct redir {
    int change_in;
    int change_out;
    char *in_file;
    char *out_file;
} redir;

extern int shellRun;
extern LinkedList all_proc;
extern int last_fore_proc[];
extern int background;
extern int backgroundPermitted;
extern redir r_data;
extern struct sigaction ignore_sig, default_sig, tstp_sig;
extern sigset_t tstp_mask;

void exit_command(void);
void change_directory(char **, int);
void get_status(void);
void execute_command(char **, int);
void background_handler(void);
void redirection_handler(void);
// void clean_redir_data(void);

#endif
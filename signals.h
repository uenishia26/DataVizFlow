#ifndef CS410_PROJECT_2_SIGNALS_H
#define CS410_PROJECT_2_SIGNALS_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define MAXLINE 1024

extern int childpids_size;
extern pid_t childpids[];


void sigint_handler(int signum);

void sigchld_handler(int signum);


#endif

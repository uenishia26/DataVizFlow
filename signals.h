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
extern volatile sig_atomic_t signal_received;

void sigint_handler();

void sigchld_handler();


#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include "signals.h"

#define MAXLINE 1024

pid_t childpids[MAXLINE];
int childpids_size = 0;

volatile sig_atomic_t signal_received = 0;


void sigint_handler() {

    int i;
    for(i = 0; i < childpids_size; i++) {
        if(childpids[i] != 0) {
            kill(childpids[i], SIGINT);
        }
    }
    printf("\nStopped all foreground processes.\n");
    printf("myshell> ");
    signal_received = 1;
    fflush(stdout);
}

void sigchld_handler() {
    // Wait for any child without blocking
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}



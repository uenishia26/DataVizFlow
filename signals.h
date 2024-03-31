//
// Created by nickgalis on 3/30/24.
//

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

pid_t childpids[MAXLINE];
int childpids_size = 0;

void sigint_handler(int signum) {
    int i;
    for(i = 0; i < childpids_size; i++) {
        if(childpids[i] != 0) {
            kill(childpids[i], SIGINT);
        }
    }
    printf("\nStopped all foreground processes.\n");
    printf("myshell> ");
    fflush(stdout);
}

void sigchld_handler(int signum) {
    // Wait for any child without blocking
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}


#endif //CS410_PROJECT_2_SIGNALS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include "execute.h"

#define MAXLINE 1024

pid_t childpids[MAXLINE];
int childpids_size = 0;

void sigint_handler() {
    signal(SIGINT, sigint_handler);
    for(int i = 0; i < childpids_size; i++) {
        if(childpids[i] != 0) {
            kill(childpids[i], SIGINT);
        }
    }
    printf("\nStopped all foreground processes.\n");
    fflush(stdout);
}

void sigchld_handler() {
    // Wait for any child without blocking
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}

int main() {
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    char cmdline[MAXLINE];

    if (isatty(STDIN_FILENO)) {
        printf("myshell> ");
    }

    while (fgets(cmdline, sizeof(cmdline), stdin) != NULL) {
        cmdline[strlen(cmdline) - 1] = '\0';

        execute_separated_commands(cmdline);

        if (isatty(STDIN_FILENO)) {
            printf("myshell> ");
        }
    }
    printf("\n");
    exit(0);
}



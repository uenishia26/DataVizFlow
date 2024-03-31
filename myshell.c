#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "signals.h"
#include "execute.h"

int main() {

    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);


    char cmdline[MAXLINE];

    if (isatty(STDIN_FILENO)) {
        printf("myshell> ");
    }

    while (fgets(cmdline, sizeof(cmdline), stdin) != NULL) {
        cmdline[strlen(cmdline) - 1] = '\0';

        // Check if cmdline is empty
        if(strlen(cmdline) > 0){
            // cmdline is nonempty, safe to parse and execute commands
            execute_separated_commands(cmdline);
        }

        if (isatty(STDIN_FILENO)) {
            printf("myshell> ");
        }
    }
    printf("\n");
    exit(0);
}
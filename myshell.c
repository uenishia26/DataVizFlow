#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "signals.h"
#include "execute.h"


int main(int argc, char *argv[]) {

    if(argc>1){
        freopen(argv[1], "r", stdin);
    }
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

        // check signal_received flag before printing the prompt
        if (isatty(STDIN_FILENO) && !signal_received) {
            printf("myshell> ");
        }
        signal_received = 0;

    }
    printf("\n");
    exit(0);
}
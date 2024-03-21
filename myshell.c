#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define MAXLINE 1024
#define MAXARGS 128

/* Function to parse user commands */
void parser(char* cmdline, char** argv) {
    char *token;
    token = strtok(cmdline, " ");
    int i=0;
    while(token!=NULL) {
        argv[i]=token;
        i++;
        token = strtok(NULL," ");
    }
    argv[i] = NULL;
}

/* Function to execute user commands */
void execute(char** argv) {
    int out = 0; // Output redirection flag
    int file_descriptor; // File Descriptor for output file if redirection is intended
    pid_t pid;
    int status;
    char *out_file = NULL; // Output file name if any

    // Check if the command contains '>' for redirection
    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], ">") == 0) {
            argv[i] = NULL; // Remove '>' from command
            out_file = argv[i + 1]; // The next string in the command is the output file name
            out = 1;
        }
    }

    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
        exit(1);
    }
    else if (pid == 0) {
        if (out) {
            file_descriptor = open(out_file, O_WRONLY | O_CREAT, S_IRWXU);
            dup2(file_descriptor, STDOUT_FILENO);
            close(file_descriptor);
        }

        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "*** ERROR: exec failed\n");
            exit(1);
        }
    }
    else {
        while(wait(&status) != pid);
    }
}


int main() {
    char cmdline[MAXLINE];
    char *argv[MAXARGS];

    if (isatty(STDIN_FILENO)) {
        printf("myshell> ");
    }

    while (fgets(cmdline, sizeof(cmdline), stdin) != NULL) {

        cmdline[strlen(cmdline) - 1] = '\0';

        parser(cmdline, argv);

        if (isatty(STDIN_FILENO)) {
            printf("myshell> ");
        }

        if (strcmp(argv[0], "exit") == 0) {
            exit(0);
        }

        execute(argv);
    }
    printf("\n");
    exit(0);
}

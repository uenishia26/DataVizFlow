
#ifndef CS410_PROJECT_2_EXECUTE_H
#define CS410_PROJECT_2_EXECUTE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

/* Command structure */
typedef struct command {
    char **argv;
    struct command *next;
} Command;

/* Function to execute user commands */
void execute(char** argv) {
    int out = 0; // Output redirection flag
    int err = 0; // Error redirection flag
    int in = 0;  // Input redirection flag
    int file_descriptor; // File Descriptor for output file if redirection is intended
    char *out_file = NULL; // Output file name if any
    char *err_file = NULL; // Output file for error
    char *in_file = NULL; // Input file name if any
    char *both_file = NULL; // Both stout and stdin go here if directed

    // Check if the command contains '>' for redirection or '1>' for standard output redirection
    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], ">") == 0 || (strcmp(argv[i], "1>") == 0)) {
            out_file = argv[i + 1]; // The next string is the output file name
            argv[i] = NULL; // Remove '>' from command
            out = 1;
        }
    }

    // Check if the command contains '2>' for error redirection
    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "2>") == 0) {
            err_file = argv[i + 1];
            argv[i] = NULL;
            err = 1;
        }
    }

    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "&>") == 0) {
            both_file = argv[i + 1];
            argv[i] = NULL;
            out = 1;
            err = 1;
        }
    }

    // Check if the command contains '<' for input redirection
    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "<") == 0) {
            in_file = argv[i + 1];
            argv[i] = NULL;
            in = 1;
        }
    }


    if (out) {
        if (both_file) {
            file_descriptor = open(both_file, O_WRONLY | O_CREAT, S_IRWXU);
        } else {
            file_descriptor = open(out_file, O_WRONLY | O_CREAT, S_IRWXU);
        }
        dup2(file_descriptor, STDOUT_FILENO);
        close(file_descriptor);
    }
    if (err) {
        if (both_file) {
            file_descriptor = open(both_file, O_WRONLY | O_CREAT, S_IRWXU);
        } else {
            file_descriptor = open(err_file, O_WRONLY | O_CREAT, S_IRWXU);
        }
        dup2(file_descriptor, STDERR_FILENO);
        close(file_descriptor);
    }
    if (in) {
        file_descriptor = open(in_file, O_RDONLY);
        dup2(file_descriptor, STDIN_FILENO);
        close(file_descriptor);
    }

    if (execvp(argv[0], argv) < 0) {
        fprintf(stderr, "*** ERROR: exec failed\n");
        exit(1);
    }
}

/* Function to parse user command */
Command* parser(char* cmdline) {
    // Count spaces for the number of arguments
    int spaces = 0;
    for (int i = 0; cmdline[i]; i++)
        spaces += (cmdline[i] == ' ');

    // Allocate memory for arguments
    char **argv = malloc((spaces + 2) * sizeof(char*));

    // Tokenize cmd_line by spaces and store the tokens in argv
    int idx = 0;
    char *token = strtok(cmdline, " ");
    while (token != NULL) {
        argv[idx++] = token;
        token = strtok(NULL, " ");
    }
    argv[idx] = NULL;

    // Create and return the Command
    Command *cmd = malloc(sizeof(Command));
    cmd->argv = argv;
    cmd->next = NULL;
    return cmd;
}


void execute_separated_commands(char* cmdline) {
    char *line;
    Command *command_list = NULL, *last_command = NULL;
    char *rest = cmdline;
    // This will split the command line input by semicolons
    while ((line = strsep(&rest, ";")) != NULL) {
        Command *current_command = parser(line);

        // Append commands to a linked list
        if (last_command == NULL) {
            command_list = current_command;
            last_command = current_command;
        } else {
            last_command->next = current_command;
            last_command = current_command;
        }
    }

    Command *current = command_list;
    while(current != NULL){
        int status;
        pid_t pid;

        if (strcmp(current->argv[0], "exit") == 0)
            exit(0);

        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Fork Failed");
            exit(1);
        } else if (pid == 0) {
            execute(current->argv);
            exit(0); //important to exit otherwise it will continue to execute the remaining commands in the child
        } else {
            if (!strcmp(current->argv[0], "cd")) { // if the command is "cd"
                if (current->argv[1]) {
                    if (chdir(current->argv[1]) < 0) { // try to change the directory
                        fprintf(stderr, "chdir failed\n"); // print an error message if it fails
                    }
                }
            } else {
                waitpid(pid, &status, 0); //Ensure you don't have zombie process
            }
        }

        Command *nextCommand = current->next;

        free(current->argv);
        free(current);

        current = nextCommand;
    }
}


#endif //CS410_PROJECT_2_EXECUTE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "execute.h"
#include "signals.h"

/* Function to execute user commands */
void execute(Command *command)  {
    char **argv = command->argv;
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

    pid_t pid = fork();
    if (pid == 0) {
        if (out) {
            if (both_file) {
                file_descriptor = open(both_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
            } else {
                file_descriptor = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
            }
            dup2(file_descriptor, STDOUT_FILENO);
            close(file_descriptor);
        }

        if (err) {
            if (both_file) {
                file_descriptor = open(both_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
            } else {
                file_descriptor = open(err_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
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
        } else {
            printf("executing: %s\n", argv[0]);
        }
    } else if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else {
        int status;
        waitpid(pid, &status, 0);
    }
}

void execute_pipe_commands(Command *cmd) {
    int prev_pipe = STDIN_FILENO;
    int pfds[2];
    pid_t *pids_arr; // Created an array to hold pid of all child processes
    int cmd_count = 0; // Count of total commands
    Command *tmp = cmd;

    // Count total commands
    while (tmp) {
        cmd_count++;
        tmp = tmp->next;
    }

    pids_arr = (pid_t*)malloc(cmd_count * sizeof(pid_t));

    pid_t pid;
    Command *current_command = cmd;
    int i = 0; // Counter to store pid in the array

    while (current_command) {
        if (pipe(pfds) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid = fork();
        if (pid == 0) {
            if (prev_pipe != STDIN_FILENO) {
                dup2(prev_pipe, STDIN_FILENO);
                close(prev_pipe);
            }

            if (current_command->next) {
                dup2(pfds[1], STDOUT_FILENO);
            }

            close(pfds[0]);
            close(pfds[1]);

            execute(current_command);
            _exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (prev_pipe != STDIN_FILENO) {
            close(prev_pipe);
        }

        close(pfds[1]);

        prev_pipe = pfds[0];
        current_command = current_command->next;

        pids_arr[i++] = pid; // Store the pid of this child
    }

    if (prev_pipe != STDIN_FILENO) {
        close(prev_pipe);
    }

    // Waiting for child processes in the order they were created
    for (i = 0; i < cmd_count; ++i) {
        waitpid(pids_arr[i], NULL, 0);
    }

    free(pids_arr);
}

void execute_separated_commands(char* cmdline) {
    char *line;
    Command *command_list = NULL, *last_command = NULL;
    char *rest = cmdline;
    // This will split the command line input by semicolons and pipes
    while ((line = strsep(&rest, ";")) != NULL) {
        // Check if it is a piped command
        if (strchr(line, '|') != NULL) {
            Command *piped_command = pipe_parser(line);
            execute_pipe_commands(piped_command);
            continue;
        }
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

        signal(SIGINT, sigint_handler);

        if (current->argv[0] && strcmp(current->argv[0], "exit") == 0)
            exit(0);

        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Fork Failed");
            exit(1);
        } else if (pid == 0) {

            signal(SIGINT, SIG_DFL);

            execute(current);
            exit(0); //important to exit otherwise it will continue to execute the remaining commands in the child
        } else {

            // keep track of child pid
            childpids[childpids_size++] = pid;

            if (current->background) {
                printf("Background task running... pid: %d, Command: %s\n", pid, current->argv[0]);
                usleep(50000);
            }

            else {
                waitpid(pid, &status, 0); //Ensure you don't have zombie process
            }

            // Reassign SIGINT handler in parent shell
            signal(SIGINT, sigint_handler);
        }

        Command *next_command = current->next;

        free(current->argv);
        free(current);

        current = next_command;
    }
}

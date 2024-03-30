#ifndef CS410_PROJECT_2_EXECUTE_H
#define CS410_PROJECT_2_EXECUTE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>

/* Command structure */
typedef struct command {
    char **argv;
    int background;
    struct command *next;

} Command;

void trim(char** str) {
    char* ptr = *str;
    int len;

    // Remove leading white-spaces
    while(isspace((unsigned char)*ptr)) ptr++;

    *str = ptr;

    // Remove trailing white-spaces
    len = strlen(ptr);
    while(len > 0 && isspace((unsigned char)ptr[len-1])) len--;

    ptr[len] = '\0';
}


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
    cmd->background = (argv[idx - 1] && strcmp(argv[idx - 1], "&") == 0);
    if (cmd->background) {
        argv[idx - 1] = NULL; // Remove '&'
    }
    cmd->next = NULL;

    return cmd;
}

Command* pipe_parser(char *cmdline) {
    Command *head = NULL;
    Command **node = &head;

    char* saveptr;
    char* line = strtok_r(cmdline, "|", &saveptr);

    while (line != NULL) {
        trim(&line);
        *node = parser(line);
        node = &((*node)->next);
        line = strtok_r(NULL, "|", &saveptr);
    }

    return head;
}

/*
void execute_pipe_commands(Command *cmd) {
    int fd[2];
    pid_t pid;
    int fd_in = 0;

    while (cmd != NULL) {
        pipe(fd);
        if ((pid = fork()) == -1) {
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            dup2(fd_in, 0);
            if (cmd->next != NULL)
                dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            execute(cmd);
            exit(EXIT_SUCCESS);
        } else {
            wait(NULL);
            close(fd[1]);
            fd_in = fd[0];
            cmd = cmd->next;
        }
    }
}
*/

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
    Command *currentCommand = cmd;
    int i = 0; // Counter to store pid in the array

    while (currentCommand) {
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

            if (currentCommand->next) {
                dup2(pfds[1], STDOUT_FILENO);
            }

            close(pfds[0]);
            close(pfds[1]);

            execute(currentCommand);
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
        currentCommand = currentCommand->next;

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

        if (strcmp(current->argv[0], "exit") == 0)
            exit(0);

        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Fork Failed");
            exit(1);
        } else if (pid == 0) {
            execute(current);
            exit(0); //important to exit otherwise it will continue to execute the remaining commands in the child
        } else {

            if (current->background) {
                printf("Background task running... pid: %d, Command: %s\n", pid, current->argv[0]);
            }

            else {
                waitpid(pid, &status, 0); //Ensure you don't have zombie process
            }
        }

        Command *next_command = current->next;

        free(current->argv);
        free(current);

        current = next_command;
    }
}


#endif //CS410_PROJECT_2_EXECUTE_H
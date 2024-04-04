#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

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
    cmd->background = (idx > 0 && argv[idx - 1] && strcmp(argv[idx - 1], "&") == 0);
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



#ifndef CS410_PROJECT_2_PARSER_H
#define CS410_PROJECT_2_PARSER_H

/* Command structure */
typedef struct command {
    char **argv;
    int background;
    struct command *next;

} Command;

void trim(char** str);

Command* parser(char* cmdline);

Command* pipe_parser(char *cmdline);

#endif


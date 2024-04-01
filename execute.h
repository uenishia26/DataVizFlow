#ifndef CS410_PROJECT_2_EXECUTE_H
#define CS410_PROJECT_2_EXECUTE_H

#include "parser.h"


void execute(Command *command);

void execute_pipe_commands(Command *cmd);

void execute_separated_commands(char* cmdline);

#endif


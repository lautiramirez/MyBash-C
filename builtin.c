#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "builtin.h"
#include "tests/syscall_mock.h"

bool builtin_is_exit(pipeline pipe){
    assert(pipe != NULL);
    bool command_compare;

    scommand pipe_front = pipeline_front(pipe);
    char *command_front = scommand_front(pipe_front);
    //Comparo si el comando es un exit.
    bool compare_value = strcmp(command_front, "exit");
    command_compare = (compare_value == 0);

    return (command_compare);
}

bool builtin_is_cd(pipeline pipe){
    assert(pipe != NULL);
    bool command_compare;

    scommand pipe_front = pipeline_front(pipe);
    char *command_front = scommand_front(pipe_front);
    //Comparo si el comando es un cd.
    bool compare_value = strcmp(command_front, "cd");
    command_compare = (compare_value == 0);

    return (command_compare);
}

bool builtin_is_internal(pipeline pipe){
    assert(pipe != NULL);
    return builtin_is_exit(pipe) || builtin_is_cd(pipe);
}

void builtin_exec(pipeline pipe){
    assert(builtin_is_internal(pipe));

    scommand command = pipeline_front(pipe);
    if(builtin_is_cd(pipe)){
        scommand_pop_front(command);
        if(scommand_length(command) > 0){
            char *temp = scommand_front(command);
            chdir(temp);
        } 
        else{
            fprintf(stderr, "Faltan argumentos \n");
        }
    }
    else if(builtin_is_exit(pipe)){
        close(STDIN_FILENO);
    }
}

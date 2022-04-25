#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>

#include "builtin.h"
#include "execute.h"
#include "tests/syscall_mock.h"

static int config_redir(pipeline apipe){
    if(apipe != NULL){
        // Tomamos el primer scommand del pipeline.
        scommand pipe_front = pipeline_front(apipe);
        // Redireccionamos la entrada estandar (<) por el fichero correspondiente si es necesario:
        char *redir_in = scommand_get_redir_in(pipe_front);
        if(redir_in){
            int in = open(redir_in, O_RDONLY, S_IRWXU);
            // Comprobamos el fichero de entrada existe:
            if(in == -1){
                fprintf(stderr, "Error, el fichero '%s' no existe. \n", redir_in);
                return -1;
            }
            dup2(in, STDIN_FILENO);
            close(in);
        }
        // Redireccionamos la salida estandar (>) por el fichero correspondiente si es necesario:
        char *redir_out = scommand_get_redir_out(pipe_front);
        if(redir_out){
            int out = open(redir_out,  O_CREAT | O_WRONLY | O_TRUNC , S_IRWXU);
            dup2(out, STDOUT_FILENO);
            close(out);
        }
    }
    return 0;
}

static int execute_simple_scommand(pipeline apipe){
    // Con status_execute controlaremos si un proceso se creó correctamente.
    int status_execute = 0;
    // Si el comando es interno (cd ó exit), lo corremos.
    if(builtin_is_internal(apipe)){
        builtin_exec(apipe);
    }
    // Si no es interno, creamos un array para llamar a execvp.
    else{
        scommand pipe_front = pipeline_front(apipe);
        unsigned int length_command = scommand_length(pipe_front);
        char **cmd = calloc(length_command + 1, sizeof(char *));
        // Desarmamos el comando para insertarlo en el array.
        for(unsigned int i = 0u; i < length_command; ++i){
            char *tmp = scommand_front(pipe_front);
            cmd[i] = strdup(tmp);
            scommand_pop_front(pipe_front);
        }
        // Si execvp falla, cambiamos el estado de status_execute para informar que hay un error.
        if(execvp(cmd[0], cmd) == -1){
            fprintf(stderr, "Comando inexistente\n");
            status_execute = -1;
        }
        // Liberamos la memoria utilizada.
        for(unsigned int i = 0u; i < length_command; ++i){
            free(cmd[i]);
            cmd[i] = NULL;
        }
        free(cmd);
        cmd = NULL;
    }
    return status_execute;
}

static void config_pipe(int fd_close, int fd_op, int fd_std){
    // Cerramos el file descriptor que no necesitamos.
    close(fd_close);
    // Redireccionamos la salida/entrada estandar por el file descriptor que necesitamos.
    dup2(fd_op, fd_std);
    // Cerramos la punta de lectura o escritura.
    close(fd_op);
}

void execute_pipeline(pipeline apipe){
    assert(apipe != NULL);

    // Variable por si hay que esperar o no que un proceso termine.
    bool pipe_wait = pipeline_get_wait(apipe);

    // Si el pipe es vacio, no hacemos nada.
    if(pipeline_is_empty(apipe)){
        return;
    }

    // Si el comando es interno, lo ejecutamos.
    if(builtin_is_internal(apipe)){
        builtin_exec(apipe);
        return;
    }

    // Si es el unico comando, creamos un nuevo proceso y lo ejecutamos.
    if(pipeline_length(apipe) == 1){
        pid_t pid = fork();
        if(pid == 0){
            // Configuramos los redirecionamientos, en caso que falle finalizamos el proceso.
            if(config_redir(apipe) == -1){
                exit(1);
            }
            // Ejecutamos el comando, en caso de error el proceso termina.
            if(execute_simple_scommand(apipe) == -1){
                exit(1);
            }
        }
        // Mensaje de error si el proceso no se creo correctamente.
        else if(pid == -1){
            fprintf(stderr, "Error con el fork\n");
        }
        // El proceso padre espera que el proceso del hijo termine si es necesario.
        else{
            if(pipe_wait){
                waitpid(pid, NULL, 0);
            }
        }
        return;
    }

    // Si el pipeline tiene mas de un comando:
    else {
        // Creamos un array de tuberias (pipes) para comunicar los procesos.
        unsigned int len_pipeline = pipeline_length(apipe);
        int **vec_pipe = calloc(len_pipeline - 1, sizeof(int*));
        for(unsigned int i = 0u; i < len_pipeline - 1; ++i){
            vec_pipe[i] = calloc(2, sizeof(int));
            pipe(vec_pipe[i]);
        }

        // Creamos un array para guardar los pid de los procesos que se crean.
        unsigned int *copy_fork = calloc(len_pipeline, sizeof(int));

        // Creamos un nuevo proceso por cada comando del pipeline.
        for(unsigned int i = 0u; i < len_pipeline; ++i){
            pid_t pid = fork();
            if (pid == 0){
                // Si no es el ultimo comando, cerramos la punta  de lectura, y redireccionamos la salida al pipe.
                if(i < len_pipeline - 1){
                    config_pipe(vec_pipe[i][0], vec_pipe[i][1], STDOUT_FILENO);
                }
                // Si no es el primer comando, cerramos la punta de escritura, y redireccionamos la entrada al pipe.
                if(i > 0){
                    config_pipe(vec_pipe[i-1][1], vec_pipe[i-1][0], STDIN_FILENO);
                }
                // Si es necesario congiguramos los redireccionamientos (<) y (>). En caso que la entrada no exista, el proceso termina.
                if(config_redir(apipe) == -1){
                    exit(1);
                }

                // Ejecutamos el comando.
                if(execute_simple_scommand(apipe) == -1){
                    exit(1);
                }
            }
            // Mensaje de error si el proceso no se pudo crear correctamente.
            else if(pid == -1){
                fprintf(stderr, "Error con el fork\n");
            }

            // En el proceso padre:
            else {
                // Cerramos la punta de lectura del proceso padre si no es el primer comando.
                if(i > 0){
                    close(vec_pipe[i-1][0]);
                }
                // Cerramos la punta de escritura del proceso padre si no es el ultimo comando.
                if(i < len_pipeline -1){
                    close(vec_pipe[i][1]);
                }
                // Guardamos el pid del proceso.
                copy_fork[i] = pid;
                // Eliminamos del pipeline el comando recien ejecutado.
                pipeline_pop_front(apipe);
            }
        }
        // Esperamos que terminen los procesos si es necesario.
        if(pipeline_get_wait(apipe)){
            for (unsigned int i = 0u; i < len_pipeline; ++i){
                waitpid(copy_fork[i], NULL, 0);
            }
        }

        // Liberamos la memoria utilizada.
        free(copy_fork);
        for (unsigned int i = 0u; i < len_pipeline - 1; ++i){
            free(vec_pipe[i]);
            vec_pipe[i] = NULL;
        }
        free(vec_pipe);
        vec_pipe = NULL;
    }
}

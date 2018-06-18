/* main.c
Copyright (c) 2018,

Igor Guilherme Bianchi   igorgbianchi@gmail.com
Lucas Martins Macedo
Marco Alcassio Uski			uski.marco@gmail.com
Willian Kaminobo Takaezu		takaezu.ufscar@gmail.com

This file is part of RoyalDutchShell.
RoyalDutchShell is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "tparse.h"
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "functions.h"

int main(int argc, char **argv) {
    buffer_t *command_line;
    int aux, pid = 0;

    pipeline_t *pipeline;
    command_line = new_command_line();
    pipeline = new_pipeline();

    /* Set shell to ingore most signals */
    set_signals(SIG_IGN);

    while (true) {
        /* Prompt. */
        printf("%s ", PROMPT);
        fflush(stdout);
        aux = read_command_line(command_line);
        if (aux == 0) {
            break;
        }
        assert (aux > 0);

        /* Parse */
        if (!parse_command_line(command_line, pipeline)) {
            int pipes[2];
            int in_file = STDIN_FILENO;
            int out_file = STDOUT_FILENO;
            int i;

            if (pipeline->ncommands > 0 && strcmp("exit", pipeline->command[0][0]) == 0) {
                /* exit builtin to leave the shell */
                break;
            }

            /* change input file */
            if (REDIRECT_STDIN(pipeline)) {
                in_file = open(pipeline->file_in, O_RDONLY);
                assert(in_file != -1);
            }

            for (i = 0; i < pipeline->ncommands; i++) {
                if (i == pipeline->ncommands - 1) { /* is last process? */
                    /* change output file */
                    out_file = STDOUT_FILENO;
                    if (REDIRECT_STDOUT(pipeline)) {
                        out_file = open(pipeline->file_out, O_RDWR | O_CREAT | O_TRUNC, 0664);
                        assert(out_file != -1);
                    }
                } else {
                    /* pipe between processes */
                    assert(pipe(pipes) == 0);
                    out_file = pipes[1];
                }

                pid = launch(pipeline->command[i], in_file, out_file);

                /* clear already redirected IO */
                if (in_file != STDIN_FILENO) {
                    close(in_file);
                }
                if (out_file != STDOUT_FILENO) {
                    close(out_file);
                }

                /* next input is piped from this output */
                in_file = pipes[0];
            }
        }

        /* Parent */
        if (RUN_FOREGROUND(pipeline)) {
            /* Wait for the last procces of the pipeline */
            int status;
            waitpid(pid, &status, 0);
        } else {
            printf("Background job control not yet implemented\n");
            fflush(stdout);
        }
    }

    release_command_line(command_line);
    release_pipeline(pipeline);

    return EXIT_SUCCESS;
}

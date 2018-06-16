/* main.c
Copyright (c) 2018,

Igor Guilherme Bianchi
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


#include <stdlib.h>
#include <stdio.h>
#include "tparse.h"
#include "debug.h"
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PROMPT "$"

int main(int argc, char **argv) {
    int aux;
    buffer_t *commandLine = new_command_line();
    pipeline_t *pipeline = new_pipeline();

    while (true) {
        /* Prompt */
        printf("%s ", PROMPT);
        fflush(stdout);
        aux = read_command_line(commandLine);
        sysfatal(aux < 0);

        if (!parse_command_line(commandLine, pipeline) && pipeline->ncommands > 0) {
            int ret;
            int prevOut = dup(1);
            int prevIn = dup(0);
            /*TODO error redirection*/

            if (pipeline->ncommands > 1) {
                printf("Hold the door! Pipes are not yet available in this version.\n"
                       "Running only the first command from the pipeline.");
            }

            if (REDIRECT_STDOUT(pipeline)) {
                int out;

                out = open(pipeline->file_out, O_RDWR | O_CREAT | O_TRUNC, 0664);
                if (out != -1) {
                    dup2(out, 1);
                    close(out);
                } else {
                    debug(true && "open failed", "Couldn\'t open file for output redirection");
                }
            }

            if (REDIRECT_STDIN(pipeline)) {
                int in;

                in = open(pipeline->file_in, O_RDONLY);
                if (in != -1) {
                    dup2(in, 0);
                    close(in);
                } else {
                    debug(true && "open failed", "Couldn\'t open file for input redirection");
                }
            }

            ret = fork();
            if (ret > 0) {
                /* Parent process */
                int status; /* <- Use this for child information */
                waitpid(ret, &status, 0);
                dup2(1, prevOut);
                dup2(0, prevIn);
            } else if (ret == 0) {
                /* Child process  */
                execvp(pipeline->command[0][0], pipeline->command[0]);

                /* Error on changing process image */
                sysfatal("execvp failed"); /* Fail for now */
            } else {
                /* Error on fork */
                sysfatal("fork failed");
            }
        }
    }

    release_command_line(commandLine);
    release_pipeline(pipeline);


    return EXIT_SUCCESS;
}

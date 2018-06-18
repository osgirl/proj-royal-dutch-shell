/* functions.h
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

#define PROMPT "RoyalDutch$"
// Print formated error string (from errno) to output
void print_error(char *message) {
    printf("%s %s failed: %s\n", PROMPT, message, strerror(errno));
    fflush(stdout);
}

// Set handler value to SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU and SIGCHLD
void set_signals(__sighandler_t handler) {
    signal(SIGINT, handler);
    signal(SIGQUIT, handler);
    signal(SIGTSTP, handler);
    signal(SIGTTIN, handler);
    signal(SIGTTOU, handler);
    signal(SIGCHLD, handler);
}


//  Launch a process with io redirection
int launch(char **command, int in_file, int out_file) {
    int pid;
    pid = fork();
    if (pid == 0) {
        /* Child */
        set_signals(SIG_DFL);   /* Reset signals the shell is ignoring */

        if (in_file != STDIN_FILENO) {
            /* Redirect input */
            dup2(in_file, STDIN_FILENO);
            close(in_file);
        }

        if (out_file != STDOUT_FILENO) {
            /* Redirect output */
            dup2(out_file, STDOUT_FILENO);
            close(out_file);
        }

        execvp(command[0], command);
        print_error("execvp");
        exit(1);
    } else if (pid == -1) {
        /* Error */
        print_error("fork");
    }
    return pid;
}

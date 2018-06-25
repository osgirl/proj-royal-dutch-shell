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
#include <unistd.h>
#include <string.h>

#include "tparse.h"
#include "job.h"
#include "royaldutch.h"

job* jobs_head;
bool on_terminal;
int shell_in = STDIN_FILENO;
int shell_pgid;
struct termios io_flags;

#define BUILTIN_CONDITION(name) (job->number_procs > 0 && strcmp(#name, job->procs[0].argv[0]) == 0)
#define BUILTIN_ON_FUNCTION(name) {if(BUILTIN_CONDITION(name)) {builtin_ ## name(job); continue;}}

int main(int argc, char** argv) {
    buffer_t* command_line;

    shell_init();
    command_line = new_command_line();

    while (true) {
        int read;
        job* job;

        notify_background_jobs();
        read = prompt(command_line, &job);
        if (!job) { if (read == 0) { break; } else { continue; }}
        put_job(job);

        BUILTIN_ON_FUNCTION(cd);
        BUILTIN_ON_FUNCTION(jobs);
        BUILTIN_ON_FUNCTION(fg);
        BUILTIN_ON_FUNCTION(bg);
        if (BUILTIN_CONDITION(exit)) { break; }

        launch_job(job);

        if (job->background) {
            printf("[%d] started\n", job->pgid);
        } else {
            wait_foreground_job(job);
        }
    }

    shell_release();

    release_command_line(command_line);

    return EXIT_SUCCESS;
}

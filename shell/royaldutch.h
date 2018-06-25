/* royaldutch.h
Copyright (c) 2018,

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


#ifndef IMP_BUILTIN_H
#define IMP_BUILTIN_H

#include <signal.h>
#include <termios.h>
#include "job.h"

#define PROMPT "RoyalDutch$"

extern job* jobs_head;
extern bool on_terminal;
extern int shell_in;
extern int shell_pgid;
extern struct termios io_flags;

void shell_init();

void shell_release();

void print_error(char* message);

int prompt(buffer_t* buffer, struct job** job);

job* job_from_pipeline(pipeline_t* pipeline, char* command_line);

void wait_foreground_job(struct job* job);
void notify_background_jobs();

void set_signals(__sighandler_t handler, bool include_sigtstp);
void sigtstp_handler(int signo);

char* last_dir(char* path);

/* *
 * BUILT-IN FUNCTIONS
 * */
void builtin_cd(struct job* job);

void builtin_jobs(struct job* job);

void builtin_fg(struct job* job);
void builtin_bg(struct job* job);

#endif

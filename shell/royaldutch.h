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


/*****************************
 * Globals
 *****************************/
/* List of child jobs */
extern job* jobs_head;

/* Can interact with user */
extern bool on_terminal;

/* File descriptor for shell stdin */
extern int shell_in;

/* Shell's own PGID */
extern int shell_pgid;

/* Shell IO modes */
extern struct termios io_flags;

/******************************
 * Shell functions
 ******************************/
/* Initialize the shell globals */
void shell_init();

/* Release the shell globals */
void shell_release();

/* Print a default formatted error message (uses errno) */
void print_error(char* message);

/* Gets the name of the last folder in hierarchy */
char* last_dir(char* path);

/* Prompt user for commands */
int prompt(buffer_t* buffer, struct job** job);

/* Set child as foreground process and wait for it to finish */
void wait_foreground_job(struct job* job);

/* Update status about background process and notifies the user */
void notify_background_jobs();

/* Set handler value to SIGINT, SIGQUIT, SIGTTIN, SIGTTOU
 * and override SIGTSTP if needed */
void set_signals(__sighandler_t handler, bool override_sigtstp);

/* Default shell SIGTSTP (^Z) handler
 * propagate the signal to lastest running job */
void sigtstp_handler(int signo);

/**************************************************
 * Built-in functions
 **************************************************/

/** Move the working directory */
void builtin_cd(struct job* job);

/** List current jobs */
void builtin_jobs(struct job* job);

/** Resume stopped job on foreground */
void builtin_fg(struct job* job);

/** Resume stopped job on background */
void builtin_bg(struct job* job);

/**************************************************
 * Built-in help functions
 **
 ** Print the help text for the command
 **************************************************/
void builtin_help_cd();
void builtin_help_jobs();
void builtin_help_fg();
void builtin_help_bg();
void builtin_help_exit();

#endif

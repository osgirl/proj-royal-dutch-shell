/* royaldutch.c
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

#include "royaldutch.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <assert.h>
#include <fcntl.h>

job* jobs_head;
bool on_terminal;
int shell_in = STDIN_FILENO;
int shell_pgid;
struct termios io_flags;

void shell_init() {
    jobs_head = calloc(1, sizeof(*jobs_head));

    on_terminal = (bool) isatty(shell_in);
    if (on_terminal) { /* input on user terminal? */

        set_signals(SIG_IGN, false); /* set shell to ignore most signals */
        signal(SIGTSTP, sigtstp_handler);

        shell_pgid = getpid();
        assert(setpgid(shell_pgid, shell_pgid) >= 0); /* make us a process group leader */

        tcsetpgrp(shell_in, shell_pgid); /* put stdin on foreground */
        tcgetattr(shell_in, &io_flags); /* save terminal mode */
    }
}


void shell_release() {
    job* j, * next;
    for (j = jobs_head; j; j = next) {
        next = j->next;
        release_job(j);
    }
}

void print_error(char* message) {
    printf("%s (%s) %s\n", PROMPT, message, strerror(errno));
    fflush(stdout);
}

char* last_dir(char* path) {
    char* tokstr, * token, * last = 0;
    bool first = true;
    if (!path) { return NULL; }
    while ((token = strtok_r(first ? path : NULL, "/", &tokstr))) {
        first = false;
        last = token;
    }

    return (last ? last : "/");
}

int prompt(buffer_t* buffer, struct job** job) {
    int read;
    pipeline_t* pipeline = new_pipeline();

    /* show prompt */
    char* cwd = getcwd(0, 0);
    char* dir = last_dir(cwd);
    printf("%s [%s] ", PROMPT, dir);
    fflush(stdout);
    free(cwd);

    read = read_command_line(buffer);

    /* parse and build job */
    *job = NULL;
    if (read > 0 && !parse_command_line(buffer, pipeline)) {
        *job = job_from_pipeline(pipeline, buffer->buffer);
    }

    release_pipeline(pipeline);

    return read;
}

void wait_foreground_job(struct job* job) {
    int status, pid;

    tcsetpgrp(shell_in, job->pgid); /* bring group foreground */

    do {
        pid = waitpid(-1, &status, WUNTRACED);
    } while (jobs_update_status(pid, status) && !job_stopped(job)); /* Wait until job is stopped */

    /* If the job is done, remove it from the list */
    if (job_completed(job)) {
        remove_job(job);
    }

    /* bring shell to foreground */
    tcsetpgrp(shell_in, shell_pgid);
    tcsetattr(shell_in, TCSADRAIN, &io_flags);
}

void notify_background_jobs() {
    job* j, * next;
    int pid, status;

    do {
        pid = waitpid(-1, &status, WUNTRACED | WNOHANG);
    } while (jobs_update_status(pid, status)); /* Update all pending job signals */

    for (j = jobs_head->next; j; j = next) {
        next = j->next;

        /* Notify user of job completed or stopped */
        if (job_completed(j)) {
            printf("[%d] completed\n", j->pgid);
            remove_job(j); /* Completed jobs are removed from the list */

        } else if (job_stopped(j) && !j->notified) {
            j->notified = true;
            printf("\n[%d] suspended\n", j->pgid);
        }
    }
}

void set_signals(__sighandler_t handler, bool override_sigtstp) {
    signal(SIGINT, handler);
    signal(SIGQUIT, handler);
    signal(SIGTTIN, handler);
    signal(SIGTTOU, handler);
    /* if not overriding SIGTSTP, reset it to shell default handler */
    signal(SIGTSTP, override_sigtstp ? handler : sigtstp_handler);
    /*signal(SIGCHLD, handler);*/
}

void sigtstp_handler(int signo) {
    job* j = find_latest_job(SEARCH_RUNNING);
    if (j) {
        if (killpg(j->pgid, SIGTSTP) < 0) { /* Send suspend to latest running child */
            print_error("killpg");
        }
    }
}

/**************************************************
 * Built-in functions
 **************************************************/
void builtin_cd(struct job* job) {
    process* proc = &job->procs[0];
    if (proc->argc > 1) {
        int ret = chdir(proc->argv[1]);
        if (ret == -1) {
            print_error("chdir");
        }
    }
}

void builtin_jobs(struct job* job) {
    struct job* j;
    int pid, status;

    /* TODO: This was supposed to update job status, but using kill <pgid> doesn't get us a signal here :( */
    /*do {
        pid = waitpid(-1, &status, WEXITED | WUNTRACED | WNOHANG);
        *//*printf("pid: %d, status: %d", pid, status);*//*
    } while (jobs_update_status(pid, status));*/

    for (j = jobs_head->next; j; j = j->next) {
        if (j->background || job_stopped(j)) {
            printf("[%d]\t%s\t(%s)\n", j->pgid, j->command_line, job_str_status(j));
        }
    }
}

/* Find latest job stopped job or by pgid*/
static job* find_stopped(size_t argc, char** argv) {
    job* target;
    if (argc <= 1) {
        target = find_latest_job(SEARCH_STOPPED);
    } else {
        target = find_job(atoi(argv[1]));
    }
    return target;
}


void builtin_fg(struct job* job) {
    process* proc = &job->procs[0];
    struct job* target = find_stopped(proc->argc, proc->argv);

    /* Continue job */
    if (!target || !continue_job(target)) {
        print_error("SIGCONT");
    } else {
        /* set as foreground and wait */
        target->background = false;
        wait_foreground_job(target);
    }
}


void builtin_bg(struct job* job) {
    process* proc = &job->procs[0];
    struct job* target = find_stopped(proc->argc, proc->argv);

    /* Continue job */
    if (!target || !continue_job(target)) {
        print_error("SIGCONT");
    } else {
        /* set as background and notify resume */
        target->background = true;
        printf("[%d] continued\n", target->pgid);
    }
}

/***************************************************************
 * User help functions
 ***************************************************************/
void builtin_help_cd() {
    printf("cd <path>\tChange the working directory.\n");
}

void builtin_help_jobs() {
    printf("jobs\t\tDisplay status of jobs in the current session.\n");
}

void builtin_help_fg() {
    printf("fg <pgid>\tRun jobs in the foreground.\n");
}

void builtin_help_bg() {
    printf("bg <pgid>\tRun jobs in the background.\n");
}

void builtin_help_exit() {
    printf("exit\t\tCause the shell to exit.\n");
}

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

int prompt(buffer_t* buffer, struct job** job) {
    int read;
    pipeline_t* pipeline = new_pipeline();

    char* cwd = getcwd(0, 0);
    char* dir = last_dir(cwd);
    printf("%s [%s] ", PROMPT, dir);
    fflush(stdout);
    free(cwd);

    read = read_command_line(buffer);

    *job = NULL;
    /* parse */
    if (read > 0 && !parse_command_line(buffer, pipeline)) {
        *job = job_from_pipeline(pipeline, buffer->buffer);
    }

    release_pipeline(pipeline);

    return read;
}

job* job_from_pipeline(pipeline_t* pipeline, char* command_line) {
    int i, j;
    job* job = calloc(1, sizeof(*job));

    if (REDIRECT_STDIN(pipeline)) {
        job->in = open(pipeline->file_in, O_RDONLY);
        assert(job->in != -1);
    } else {
        job->in = STDIN_FILENO;
    }

    if (REDIRECT_STDOUT(pipeline)) {
        job->out = open(pipeline->file_out, O_RDWR | O_CREAT | O_TRUNC, 0664);
        assert(job->out != -1);
    } else {
        job->out = STDOUT_FILENO;
    }
    job->err = STDERR_FILENO;

    job->background = RUN_BACKGROUND(pipeline);

    job->command_line = stringdup(command_line);

    job->number_procs = (size_t) pipeline->ncommands;
    job->procs = calloc(job->number_procs, sizeof(*job->procs));

    for (i = 0; i < job->number_procs; ++i) {
        process* proc = &job->procs[i];
        proc->argc = (size_t) pipeline->narguments[i];
        proc->argv = malloc((proc->argc + 1) * sizeof(*proc->argv));
        proc->argv[proc->argc] = 0;
        for (j = 0; j < proc->argc; ++j) {
            proc->argv[j] = stringdup(pipeline->command[i][j]);
        }
    }

    return job;
}
/* not sure why these aren't getting defined by <sys/wait.h>*/
#ifndef P_PGID
#define P_PGID 2
#endif
#ifndef P_ALL
#define P_ALL 0
#endif

void wait_foreground_job(struct job* job) {
    int status, pid;
    /*siginfo_t siginfo;*/

    tcsetpgrp(shell_in, job->pgid); /* bring group foreground */

    do {
        pid = waitpid(-1, &status, WUNTRACED);
    } while (jobs_update_status(pid, status) && !job_stopped(job));
    if (job_completed(job)) {
        remove_job(job);
    }

    tcsetpgrp(shell_in, shell_pgid);
    tcsetattr(shell_in, TCSADRAIN, &io_flags);
}

void notify_background_jobs() {
    job* j, * next;
    int pid, status;
    /*siginfo_t siginfo;*/

    do {
        pid = waitpid(-1, &status, WUNTRACED | WNOHANG);
    } while (jobs_update_status(pid, status));

    for (j = jobs_head->next; j; j = next) {
        next = j->next;
        if (job_completed(j)) {
            printf("[%d] completed\n", j->pgid);
            remove_job(j);
        } else if (job_stopped(j) && !j->notified) {
            j->notified = true;
            printf("\n{%d} stopped\n", j->pgid);
            /*remove_job(j);*/
        }
    }
}

/**
 * Set handler value to SIGINT, SIGQUIT, SIGTSTP, SIGTTIN and SIGTTOU
 */
void set_signals(__sighandler_t handler, bool include_sigtstp) {
    signal(SIGINT, handler);
    signal(SIGQUIT, handler);
    signal(SIGTTIN, handler);
    signal(SIGTTOU, handler);
    if(include_sigtstp) {
        signal(SIGTSTP, handler);
    }
    /*signal(SIGCHLD, handler);*/
}

void sigtstp_handler(int signo) {
    job* j = find_latest_job(SEARCH_RUNNING);
    if(j) {
        if(killpg(j->pgid, SIGTSTP) < 0) {
            print_error("killpg");
        }
    }
}

char* last_dir(char* path) {
    char* tokstr, * token, * last = 0;
    bool first = true;
    if(!path) { return NULL; }
    while ((token = strtok_r(first ? path : NULL, "/", &tokstr))) {
        first = false;
        last = token;
    }

    return (last? last : "/");
}

/**
 * BUILT-IN FUNCTIONS
 */
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
    for (j = jobs_head->next; j; j = j->next) {
        if (j->background || job_stopped(j)) {
            printf("[%d]\t%s\t(%s)\n", j->pgid, j->command_line, job_str_status(j));
        }
    }
}

void builtin_fg(struct job* job) {
    struct job* target;
    process* proc = &job->procs[0];
    if (proc->argc <= 1) {
        target = find_latest_job(SEARCH_STOPPED);
    } else {
        target = find_job(atoi(proc->argv[1]));
    }

    if(!target || !continue_job(target)) {
        print_error("SIGCONT");
    } else {
        target->background = false;
        wait_foreground_job(target);
    }
}

void builtin_bg(struct job* job) {
    struct job* target;
    process* proc = &job->procs[0];
    if (proc->argc <= 1) {
        target = find_latest_job(SEARCH_STOPPED);
    } else {
        target = find_job(atoi(proc->argv[1]));
    }

    if(!target || !continue_job(target)) {
        print_error("SIGCONT");
    } else {
        target->background = true;
        printf("[%d] continued\n", target->pgid);
    }
}

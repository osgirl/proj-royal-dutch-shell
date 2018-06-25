/* job.c
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

#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>

#include "job.h"
#include "royaldutch.h"

/**
 * Launch a process with io redirection
 */
void launch_process(struct job* job, process* proc, int in_file, int out_file) {
    int pid;
    pid = fork();
    if (pid == 0) {
        /* Child */
        if (on_terminal) {
            pid = getpid();
            if (job->pgid == 0) {
                job->pgid = pid; /*NOTE: remember this is the child process, it doesn't change the value in the parent*/
            }
            setpgid(pid, job->pgid); /* move to the appropriate process group (maybe become leader) */
            if (!job->background && job->procs == proc) { /* this is first process and the job is on foreground */
                tcsetpgrp(shell_in, job->pgid); /* move it foreground*/
            }
            set_signals(SIG_DFL, true);   /* Reset signals the shell is ignoring */
        }

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

        execvp(proc->argv[0], proc->argv);
        print_error("execvp");
        exit(1);
    } else if (pid == -1) {
        /* Error */
        print_error("fork");
        exit(1);
    } else {
        /* Parent */
        proc->pid = pid;
        if (on_terminal) {
            if (job->pgid == 0) {
                job->pgid = pid;
            }
            setpgid(pid, job->pgid);
        }
    }
}

void launch_job(struct job* job) {
    int i;
    int pipes[2] = {0, 0};
    int in_file, out_file;

    if (!job) { return; }

    in_file = job->in;
    for (i = 0; i < job->number_procs; i++) {
        if (i == job->number_procs - 1) {  /*is last process?*/
            /*set output file*/
            out_file = job->out;
        } else {
            /*pipe between processes*/
            assert(pipe(pipes) == 0);
            out_file = pipes[1];
        }

        launch_process(job, &job->procs[i], in_file, out_file);

        /* close access to already redirected files */
        if (in_file != STDIN_FILENO) {
            close(in_file);
        }
        if (out_file != STDOUT_FILENO) {
            close(out_file);
        }

        /* next input is piped from this output */
        in_file = pipes[0];
    }
    job->time_run = time(NULL);
}

void release_job(struct job* job) {
    int i, j;
    for (i = 0; i < job->number_procs; ++i) {
        process* proc = job->procs + i;
        for (j = 0; j < proc->argc; j++) {
            free(proc->argv[j]);
        }
        free(proc->argv);
    }
    free(job->command_line);
    free(job->procs);
    free(job);
}

bool job_stopped(job* j) {
    int i;
    for (i = 0; i < j->number_procs; i++) {
        process* proc = j->procs + i;
        if (!proc->completed && !proc->stopped) {
            return false;
        }
    }

    return true;
}

bool job_completed(job* j) {
    int i;
    for (i = 0; i < j->number_procs; i++) {
        process* proc = j->procs + i;
        if (!proc->completed) {
            return false;
        }
    }

    return true;
}

void put_job(job* new_job) {
    struct job* j;
    for (j = jobs_head; j->next; j = j->next);
    j->next = new_job;
}

void remove_job(struct job* toRemove) {
    struct job* j;
    for (j = jobs_head; j->next; j = j->next) {
        if (j->next == toRemove) {
            j->next = toRemove->next;
            release_job(toRemove);
            break;
        }
    }
}

bool jobs_update_status(int pid, int status) {
    job* j;
    if (pid <= 0) { return false; }
    for (j = jobs_head; j; j = j->next) {
        int i;

        for (i = 0; i < j->number_procs; i++) {
            process* proc = &j->procs[i];

            if (pid == proc->pid) {
                proc->status = status;
                if (WIFSTOPPED(status)) {
                    proc->stopped = true;
                } else {
                    proc->completed = true;
                }
                return true;
            }
        }
    }

    return false;
}

char* job_str_status(job* j) {
    assert(j);
    if (job_completed(j))
        return "Completed";
    if (job_stopped(j))
        return "Stopped";
    return "Running";
}

job* find_job(int pgid) {
    job* j;
    if (pgid == 0) return NULL;
    for (j = jobs_head->next; j; j = j->next) {
        if (j->pgid == pgid) {
            return j;
        }
    }

    return NULL;
}

job* find_latest_job(job_search search) {
    job* j, * latest;
    for (latest = j = jobs_head->next; j; j = j->next) {
        if (j->time_run > latest->time_run) {
            bool stopped = job_stopped(j);
            if((search == SEARCH_RUNNING && stopped)
              ||search == SEARCH_STOPPED && !stopped) {
                continue;
            } else {
                latest = j;
            }
        }
    }
    return latest;
}

bool continue_job(struct job* job) {
    if (killpg(job->pgid, SIGCONT) < 0) {
        return false;
    } else {
        int i;
        for (i = 0; i < job->number_procs; ++i) {
            process* proc = &job->procs[i];
            proc->stopped = false;
        }
        job->notified = false;
        job->time_run = time(NULL);
        return true;
    }
}

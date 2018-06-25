/* job.h
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

#ifndef IMP_JOB_H
#define IMP_JOB_H

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include "tparse.h"
#include <time.h>

typedef struct {
    char** argv;
    size_t argc;
    pid_t pid;
    bool completed, stopped;
    int status;
} process;

typedef struct job {
    struct job* next;
    process* procs;
    char* command_line;
    size_t number_procs;
    int pgid;
    bool notified;
    bool background;
    int in, out, err;
    time_t time_run;
} job;

void launch_job(struct job* job);

void release_job(struct job* job);

bool job_stopped(job* j);

bool job_completed(job* j);

void put_job(job* new_job);

void remove_job(struct job* toRemove);

job* find_job(int pgid);

typedef enum {
    SEARCH_ALL,
    SEARCH_STOPPED,
    SEARCH_RUNNING
} job_search;
job* find_latest_job(job_search search);

bool continue_job(struct job* job);

bool jobs_update_status(int pid, int status);

char* job_str_status(job* j);

#endif

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

/* Struct representing a single process from a job */
typedef struct {
    char** argv;                /* Process arguments, including program name */
    size_t argc;                /* Number of arguments */
    pid_t pid;                  /* Process id */
    bool completed, stopped;    /* Process status flag */
    int status;                 /* Returned status on exit */
} process;

/* Struct representing a single job in a linked list */
typedef struct job {
    struct job* next;           /* Next job in list */
    process* procs;             /* List of processes */
    size_t number_procs;        /* Number of processes */
    char* command_line;         /* Command line to run this job */
    int pgid;                   /* Process group id, equals shell pid if foreground */
    bool notified;              /* Stopped job has already been notified */
    bool background;            /* Is running in background? */
    time_t time_run;            /* Last time the job was run or continued */
    int in, out, err;           /* Input, output and error file descriptors */
} job;

/* Initialize a job struct from a (foo shell) pipeline object */
job* job_from_pipeline(pipeline_t* pipeline, char* command_line);

/* Launch all process from the job */
void launch_job(struct job* job);

/* Release job structure resources */
void release_job(struct job* job);

/* Return true if all job's processes are marked stopped or completed, false otherwise */
bool job_stopped(job* j);

/* Return true if all job's processes are marked completed, false otherwise */
bool job_completed(job* j);

/* Put job on the end of jobs list */
void put_job(job* new_job);

/* Remove job from jobs list */
void remove_job(struct job* toRemove);

/* Find job by pgid in jobs list */
job* find_job(int pgid);

/* Enum used to filter jobs on find_lastest_job() */
typedef enum {
    SEARCH_ALL,
    SEARCH_STOPPED,
    SEARCH_RUNNING
} job_search;

/* Find the job with latest time_run, filter by the job_search enum */
job* find_latest_job(job_search search);

/* Continue the stopped job's processes */
bool continue_job(struct job* job);

/* Mark job and process as stopped, completed etc. */
bool jobs_update_status(int pid, int status);

/* Get a string representing job status */
const char* job_str_status(job* j);

#endif

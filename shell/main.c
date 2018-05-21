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

#define PROMPT "RoyalDutchShell$"


typedef struct process
{
	int pid; 		// process id
	int status;		// 0 - stopped | 1 - running
	int foreground; 	// 0 - background | 1 - foreground | 2 - not running
	char *name;
} process;

/* void test(void); */

int go_on = 1;			/* This variable controls the main loop. */

int main (int argc, char **argv)
{
	buffer_t *command_line;
	int i, j, aux;

	pipeline_t *pipeline;

	command_line = new_command_line (); 

	pipeline = new_pipeline ();

	/* This is the main loop. */

	while (go_on)
	{
	/* Prompt. */

		printf ("%s ", PROMPT);
		fflush (stdout);
		if(read_command_line(command_line) < 0)	      	//NO INPUT YET
			getchar();
		else
		{
			if (!parse_command_line (command_line, pipeline) || 1)
			{
				//THERE'S SOMETHING ON command_line OR pipeline TO RUN
				printf ("  Pipeline has %d command(s)\n", pipeline->ncommands);

				for (i=0; pipeline->command[i][0]; i++)
				{
					printf ("  Command %d has %d argument(s): ", i, pipeline->narguments[i]);
  					for (j=0; pipeline->command[i][j]; j++)
						printf ("%s \n", pipeline->command[i][j]);
 			 
				}


				/*if ( RUN_FOREGROUND(pipeline))
				{
					printf ("  Run pipeline in foreground\n");
					process->foreground = 1;
				}
				else
				{
					printf ("  Run pipeline in background\n");
					process->foreground = 0;
				}*/

			}
			if ( REDIRECT_STDIN(pipeline))
				printf ("  Redirect input from %s\n", pipeline->file_in);
			if ( REDIRECT_STDOUT(pipeline))
				printf ("  Redirect output to  %s\n", pipeline->file_out);

			/* This is where we would fork and exec. */

		}	

		release_command_line (command_line);
		release_pipeline (pipeline);

		return EXIT_SUCCESS;
	}
}
 

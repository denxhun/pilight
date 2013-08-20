/*
	Copyright (C) 2013 CurlyMo

	This file is part of pilight.

    pilight is free software: you can redistribute it and/or modify it under the 
	terms of the GNU General Public License as published by the Free Software 
	Foundation, either version 3 of the License, or (at your option) any later 
	version.

    pilight is distributed in the hope that it will be useful, but WITHOUT ANY 
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR 
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>

#include "gc.h"

static sigjmp_buf gc_cleanup;

/* The gc uses a observer pattern to
   easily call function when exiting
   the daemon */

void gc_handler(int sig) {
    siglongjmp(gc_cleanup, sig);
}

/* Add function to gc */
void gc_attach(int (*fp)(void)) {
	struct collectors_t *gnode = malloc(sizeof(struct collectors_t));
	gnode->listener=fp;
	gnode->next = gc;
	gc = gnode;
}

/* Run the GC manually */
int gc_run(void) {
    unsigned int i, s;
	struct collectors_t *temp = gc;
	while(temp) {
		if(temp->listener() != 0) {
			s=1;
		}
		temp = temp->next;
	}
	if(s)
		return EXIT_FAILURE;
	else
		return EXIT_SUCCESS;
}

/* Initialize the catch all gc */
void gc_catch(void) {

    struct sigaction act, old;

    memset(&act,0,sizeof(act));
    act.sa_handler = gc_handler;
    sigemptyset(&act.sa_mask);
    sigaction(SIGINT,  &act, &old);
    sigaction(SIGQUIT, &act, &old);
    sigaction(SIGTERM, &act, &old);

    sigaction(SIGABRT, &act, &old);
    sigaction(SIGTSTP, &act, &old);

    sigaction(SIGBUS,  &act, &old);
    sigaction(SIGILL,  &act, &old);
    sigaction(SIGSEGV, &act, &old);
	sigaction(SIGFPE,  &act, &old);	

    if(sigsetjmp(gc_cleanup, 0) == 0)
		return;

	/* Call all GC functions */
	exit(gc_run());
}

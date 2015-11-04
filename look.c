/******************************************************************************
*
*	look.c
*
*
*	by Mark R Boyns
*
*/

#include <sys/types.h>
#include <sys/file.h>
#include <utmp.h>
#include <stdio.h>

#define	MAXUSERS	200
#define	USERSIZE	8   /* MAXUSERS*USERSIZE+1 must be <= MESSAGE_SIZE */

int look(buf)
char *buf;
{
	register int nusers, i, fd;
	char names[MAXUSERS][USERSIZE];
	struct utmp utmp;
	extern int strcmp();

	if ((fd = open("/etc/utmp", O_RDONLY, 0)) < 0) {
		perror("open");
		return -1;
	}

	nusers = 0;

	while(read(fd, &utmp, sizeof(utmp))) {
		if (*utmp.ut_name) {
			strncpy(names[nusers++], utmp.ut_name, USERSIZE);
			if (nusers == MAXUSERS) break; /* ignore users > MAXUSERS */
		}
	}
	close(fd);

	if (nusers == 0) {
		sprintf(buf, "(no one logged on)");
		return 0;
	}

	qsort(names, nusers, USERSIZE, strcmp);

	*buf = '\0';
	i = 0;
	for(;;) {
		strncat(buf, names[i++], USERSIZE);
		if (i == nusers) break; 
		strcat(buf, " ");
	}

	return 0;
}

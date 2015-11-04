/******************************************************************************
*
*	getPass.c
*
*	by Mark R Boyns
*	August 1991
*
*/

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>

char *getPass(prompt)
char *prompt;
{
	struct sgttyb oldsg, newsg;
	register char *cp;
	char c;
	static char password[128];

	ioctl(0, TIOCGETP, &oldsg);
	newsg = oldsg;
	newsg.sg_flags |= CBREAK;
	newsg.sg_flags &= ~ECHO;
	ioctl(0, TIOCSETP, &newsg);
	fprintf(stderr, "%s", prompt);
	fflush(stderr);

	cp = password;

	for(;;) {
		read(0, &c, 1);
		if (c == '\n' || c == EOF) 
			break;
		if (cp < &password[127])
			*cp++ = c;
	}

	*cp = '\0';

	fprintf(stderr, "\n");
	fflush(stderr);

	ioctl(0, TIOCSETP, &oldsg);

	return password;
}

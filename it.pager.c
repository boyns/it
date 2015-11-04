/******************************************************************************
*
*	it.pager.c
*
*	Internet Talk Pager
*
*	by Mark R Boyns
*	August 1991
*
*	*** chmod 4711 it.pager ***
*/

#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include "defs.h"

char msg[] = "Message from the Internet Talk Server..."; 

main(argc, argv)
int argc;
char **argv;
{
	struct passwd *pwd;
	char *tty;
	FILE *fp;

	if (isatty(0)) /* a little protection! */
		exit(1);

	if ((pwd = getpwuid(geteuid())) == 0) {
		perror("getpwuid");
		exit(1);
	}

	if ((tty = checkUsers(pwd->pw_name)) == 0) {
		fprintf(stderr, "?user just logged out\n");
		exit(1);
	}

	if ((fp = fopen(tty, "w")) == 0) {
		perror("fopen");
		exit(1);
	}

	fprintf(fp, "\n\r%c%s\n\r%s is paging you...\n\r\n\r", 0x07, msg, argv[1]);
	fclose(fp); /* necessary */
}

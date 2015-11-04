/******************************************************************************
*
*	checkUsers.c
*
*	by Mark R Boyns
*	August 1991
*
*/

#include <sys/types.h>
#include <sys/file.h>
#include <utmp.h>

char *checkUsers(user)
char *user;
{
	int  fd, found = 0;
	static char tty[16];
	struct utmp u;

	if ((fd = open("/etc/utmp", O_RDONLY, 0)) < 0) {
		return 0;
	}

	while (read(fd, &u, sizeof(u)) > 0) {
		if (u.ut_name[0] == '\0') continue;
		u.ut_name[8] = '\0';
		if (strcmp(u.ut_name, user) == 0) {
			sprintf(tty, "/dev/%s", u.ut_line);
			found = 1;
			break;
		}
	}

	close(fd);

	if (found) 
		return tty;
	else
		return 0;
} 

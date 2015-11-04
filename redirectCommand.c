/******************************************************************************
*
*	redirectCommand.c
*
*	by Mark R Boyns
*
*/

#include <signal.h>
#include "defs.h"

redirectCommand(fd, command, status, ident)
int fd;
char *command;
int status;
char *ident;
{
	char *shell, *getenv();
	char *defaultShell = "/bin/sh";
	char message[MESSAGE_SIZE], ciphertext[MESSAGE_SIZE];
	char buf[MESSAGE_SIZE];
	int child, line[2], size;

	if ((shell = getenv("SHELL")) == 0) {
		shell = defaultShell;
	}

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	pipe(line);

	if ((child = fork()) == 0) {
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		dup2(line[1], 1);
		close(line[0]);
		execl(shell, shell, "-c", command, 0);
		perror("execl");
		exit(1);
	}

	close(line[1]);

	sleep(1);  /* let pipe fill up... */

	while((size = read(line[0], message+1, MESSAGE_SIZE-IDENT_SIZE-4-32))>0) {
		*message = '\n'; /* replaces last \n, believe me */
		if (status == SECURE) {
			sprintf(buf, "[%s(%s)]%s", ident, command, message);
			size = encryptMessage(size+strlen(ident)+strlen(command)+4,
				buf, ciphertext);
			writemsg(fd, ciphertext, size, SECURE);
		}
		else {
			writemsg(fd, message, size, UNSECURE);
		}
	}

	wait(0); /* clean up after that zombie */

	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
}

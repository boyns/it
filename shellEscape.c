/******************************************************************************
*
*	shellEscape.c
*
*	by Mark R Boyns
*
*/

#include <signal.h>

shellEscape(command)
char *command;
{
	char *shell, *getenv();
	char *defaultShell = "/bin/sh";
	int child;

	if ((shell = getenv("SHELL")) == 0) {
		shell = defaultShell;
	}

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	if ((child = fork()) == 0) {
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		execl(shell, shell, "-c", command, 0);
		perror("execl");
		exit(1);
	}

	while (wait(0) != child)
		;

	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
}

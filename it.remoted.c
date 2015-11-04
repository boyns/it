/******************************************************************************
*
*	it.remoted.c
*
*	Internet Talk Remote Daemon
*
*	by Mark R Boyns
*	August 1991
*
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <netdb.h>
#include <netinet/in.h>
#include "defs.h"
#include "remotePort.h"

extern int errno;
extern int sigchld();
extern int sighup();

main(argc, argv)
int  argc;
char **argv;
{
	char hostname[HOST_SIZE+1];
	char message[MESSAGE_SIZE];
	struct hostent *host;
	struct sockaddr_in sin;
	int sinlen, server;
	int pid, fd, size, status;
	char request;
	
	daemonize();

	if (gethostname(hostname, HOST_SIZE) < 0) {
		perror("gethostname");
		exit(1);
	}

	if ((host = gethostbyname(hostname)) == 0) {
		herror("gethostbyname");
		exit(1);
	}

	bcopy(host->h_addr, &sin.sin_addr, host->h_length);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(REMOTE_PORT);
	sinlen = sizeof(sin);	
	
	if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	if (bind(server, (struct sockaddr *) &sin, sinlen) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(server, 0) < 0) {
		perror("listen");
		exit(1);
	}

	signal(SIGCHLD, sigchld);
	signal(SIGHUP, sighup);

	for(;;) {

		if ((fd = accept(server, &sin, &sinlen)) < 0) {
   			if (errno == EINTR) continue;
			exit(1);
		}

		pid = fork();
		if (pid == 0) {
			if ((size = readmsg(fd, message, &status)) < 0) {
				exit(1);		
			}
			if (size != 1) 
				exit(1);
			switch(*message) {
			case RPAGE:
				doRemotePage(fd); exit(0);
			case RLOOK:
				doRemoteLook(fd); exit(0);
			default:
				exit(1);
			}
		}
		close(fd);
	} 

}

doRemotePage(fd)
int fd;
{
	char message[MESSAGE_SIZE];
	char to[USER_SIZE+1];
	char from[USER_SIZE+HOST_SIZE+1];
	char buf[MESSAGE_SIZE];
	char *tty;
	char pagerPath[256];
	int status, size;
	int pid, child;
	struct passwd *pwd;
	struct stat st;

	signal(SIGCHLD, SIG_DFL);

	if ((size = readmsg(fd, message, &status)) < 0) {
		exit(1);
	}

	strncpy(from, message, size);
	from[size] = '\0';
 
	if ((size = readmsg(fd, message, &status)) < 0) {
		exit(1);
	}

	strncpy(to, message, size);
	to[size] = '\0';

	if ((tty = checkUsers(to)) == 0) {
		writemsg(fd, "?user not logged in\n", 20, UNSECURE);
		exit(1);
	}

	setpwent();
	pwd = getpwnam(to);
	sprintf(pagerPath, "%s/%s", pwd->pw_dir, PAGER);
	endpwent();

	if (stat(pagerPath, &st) < 0) {
		sprintf(buf, "?%s does not exist\n", pagerPath);
		writemsg(fd, buf, strlen(buf), UNSECURE);
		exit(1);
	}

	if ((st.st_mode & 0000001) == 0) {
		sprintf(buf, "?%s is not world executable\n", pagerPath);
		writemsg(fd, buf, strlen(buf), UNSECURE);
		exit(1);
	}

	if ((st.st_mode & 0004000) == 0) {
		sprintf(buf, "?%s is not setuid\n", pagerPath);
		writemsg(fd, buf, strlen(buf), UNSECURE);
		exit(1);
	}

	child = fork();

	if (child < 0) {
		writemsg(fd, "?cannot fork, page failed\n", 26, UNSECURE);
		exit(1);
	}

	if (child == 0) {
		sprintf(buf, "+paging %s on %s...\n", to, tty);
		writemsg(fd, buf, strlen(buf), UNSECURE);
		close(0);
		execl(pagerPath, PAGER, from, 0);
		sprintf(buf, "?exec error activating %s\n", pagerPath);
		writemsg(fd, buf, strlen(buf), UNSECURE);
		exit(1);
	}

	while ((pid = wait(0)) != child) {
		;
	}
	exit(0);
}

doRemoteLook(fd)
int fd;
{
	char lookbuf[2048];

	if (look(lookbuf+1) < 0) {
		writemsg(fd, "?remote look error\n", 19, UNSECURE);
		exit(1);
	}
	*lookbuf = '+';
	strcat(lookbuf, "\n");
	writemsg(fd, lookbuf, strlen(lookbuf), UNSECURE);
	exit(0);
}

sigchld()
{
	int pid;

	pid = wait3(0, WNOHANG | WUNTRACED, 0);
}

sighup()
{
	exit(0);
}

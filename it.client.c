/******************************************************************************
*
*	it.client.c
*
*	Internet Talk Client
*
*	by Mark R Boyns
*	August 1991
*
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include "defs.h"
#include "serverPort.h"
#include "serverAddress.h"
#include "DESdefs.h"

struct sgttyb oldttysg, newttysg;
struct tchars oldttytc, newttytc; 
struct ltchars oldttylt, newttylt;

char **av;

int clientStatus = UNSECURE;
char ident[IDENT_SIZE];

char helpMessage[] = "\
Internet Talk Commands\n\
\thelp - this message\n\
\tquit - exit Internet Talk (also ^D,^C)\n\
\tlook - show users on local host\n\
\trlook [hostname] - show users on remote host\n\
\tnick [newnickname] - change nickname\n\
\tpage [username] - page a local user\n\
\trpage [username@hostname] - page a remote user\n\
\tsecure - enter secure mode using DES\n\
\tunsecure - exit secure mode\n\
\twho - display Internet Talk users\n\
\t![shellcommand] - execute a shell command\n\
\t<[shellcommand] - execute a shell command and send output to the server\
";

main(argc, argv)
int argc;
char **argv;
{
	struct sockaddr_in sin;
	fd_set rfd, mask;
	int index, server, size, x, status;
	char c;
	char input[MESSAGE_SIZE];
	char message[MESSAGE_SIZE];
	char buf[MESSAGE_SIZE];
	char plaintext[MESSAGE_SIZE];
	char ciphertext[MESSAGE_SIZE];

	if (!isatty(0)) {
		fprintf(stderr, "%s: stdin must be a terminal\n", argv[0]);
		exit(1);
	}

	if (argc > 2) {
		fprintf(stderr, "usage: %s [nickname]\n", argv[0]);
		exit(1);
	}

	av = argv;
	sin.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SERVER_PORT);

	if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	if (connect(server, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		fprintf(stderr, "%s: can't connect to it.server at %s\n", 
			argv[0], SERVER_ADDR);
		exit(1);
	}

	sendIdent(server);

	if ((size = readmsg(server, message, &status)) < 0) {
		quit();
	}

	write(1, message, size);

	setTerm();

	FD_ZERO(&mask);
	FD_ZERO(&rfd);
	FD_SET(server, &mask);
	FD_SET(0, &mask);

	write(1, "-", 1);
	index = 0;

	for(;;) {
		rfd = mask;
		select(server+1, &rfd, (fd_set *)0, (fd_set *)0, 0);
		if (FD_ISSET(0, &rfd)) {
			read(0, &c, 1);
			if (index > INPUT_SIZE) {
				write(1, "\n?line too long\n", 16);
				index = 0;
				write(1, "-", 1);
				continue;
			}
			switch(c) {
			case	CTRL(c):
			case	CTRL(d):
				quit();
			case CTRL(u):
				for (x = 0; x < index; x++) 
					write(1, "\b \b", 3);
				index = 0;
				break;
			case CTRL(w):
				/* this is ugly but it works */
				if (index == 0) break;
				while(--index != -1 && input[index] == ' ')
					write(1, "\b \b", 3);
				if (index == -1) {
					index = 0;
					break;
				}
				do {
					write(1, "\b \b", 3);
				} while(--index != -1 && input[index] != ' ');
				if (index == -1)
					index = 0;
				else
					index++;
				break;
			case CTRL(z):
				resetTerm();
				kill(getpid(), SIGSTOP);
				setTerm();
				write(1, "-", 1);
				write(1, input, index);
				break;
			case CTRL(r):
				write(1, "\n-", 2);
				write(1, input, index);
				break;
			case 0x08:
			case	0x7f:
				if (index == 0) break;
				write(1, "\b \b", 3);
				index--;
				break;
			case '\n':
				write(1, &c, 1);
				if (index > 0) {
					if (*input == '/') {
						doCommand(server, input+1, index-1);
					}
					else if (*input == '!') {
						input[index] = '\0';
						resetTerm();
						shellEscape(input+1);
						setTerm();
					}
					else if (*input == '<') {
						input[index] = '\0';
						resetTerm();
						redirectCommand(server, input+1, clientStatus,
							ident);
						setTerm();
					}
					else if (clientStatus == SECURE) {
						sprintf(buf, "[%s] %s", ident, input);
						size = encryptMessage(strlen(ident)+index+3,
							buf, ciphertext);
						writemsg(server, ciphertext, size, SECURE);
					}
					else {
						writemsg(server, input, index, UNSECURE);
					}
				}
				index = 0;
				write(1, "-", 1);
				break;
			default:
				write(1, &c, 1);
				input[index++] = c;
			}
		}
		
		if (FD_ISSET(server, &rfd)) {
			if ((size = readmsg(server, message, &status)) < 0) {
				quit();
			}
			write(1, "\n", 1);
			if (status == SECURE) {
				decryptMessage(size, message, plaintext);
				write(1, plaintext, size);
			}
			else {
				write(1, message, size);
			}
			write(1, "\n-", 2);
			if (index != 0) {
				write(1, input, index);
			}
		}
	}	
}

doCommand(server, cmd, size)
int server;
char *cmd;
int size;
{
	char *tty;
	char key[128], *one, *two, *p;
	char pagerPath[USER_SIZE+256+1];
	char buf[MESSAGE_SIZE];
	int child, pid;
	struct passwd *pwd;
	struct stat st;

	cmd[size] = '\0';

	if (strncmp(cmd, "page", 4) == 0 && (cmd[4] == ' ' || cmd[4] == '\0' || 
		cmd[4] == '\t')) { 
		for (p = cmd+4; *p != '\0' && (*p == ' ' || *p == '\t'); p++);
		if (*p == '\0') {
			write(1, "?must specify username\n", 23);
			return;
		}
		if (strlen(p) > USER_SIZE) {
			write(1, "?username too long\n", 19);
			return;
		} 
		if ((tty = checkUsers(p)) == 0) {
			printf("?%s not logged on your local host\n", p);
			return;
		}
		pwd = getpwnam(p);
		sprintf(pagerPath, "%s/%s", pwd->pw_dir, PAGER);
		if (stat(pagerPath, &st) < 0) {
			fprintf(stderr, "?%s does not exist\n", pagerPath);
			return;
		}
		if ((st.st_mode & 0000001) == 0) {
			fprintf(stderr, "?%s is not world executable\n", pagerPath);
			return;
		}
		if ((st.st_mode & 0004000) == 0) {
			fprintf(stderr, "?%s is not setuid\n", pagerPath);
			return;
		}
		child = fork();
		if (child < 0) {
			write(1, "?cannot fork, page failed\n", 26);
			return;
		}
		if (child == 0) {
			printf("+paging %s on %s...\n", p, tty);
			close(0);
			execl(pagerPath, PAGER, ident, 0);
			fprintf(stderr, "?exec error activating %s\n", pagerPath);
			exit(1);
		}
		while ((pid = wait(0)) != child) {
			;
		}
	}
	else if (strncmp(cmd, "rpage", 5) == 0 && (cmd[5] == ' ' || 
		cmd[5] == '\0' || cmd[5] == '\t')) {
		for (p = cmd+5; *p != '\0' && (*p == ' ' || *p == '\t'); p++);
		if (*p == '\0') {
			write(1, "?must specify user@address\n", 27);
			return;
		}
		remotePage(p, ident);
	}
	else if (strncmp(cmd, "look", 4) == 0 && (cmd[4] == ' ' || 
		cmd[4] == '\0' || cmd[4] == '\t')) {
		if (look(buf) < 0) {
			write(1, "?look error\n", 12);
			return;
		}
		printf("+%s\n", buf);
	}
	else if (strncmp(cmd, "rlook", 5) == 0 && (cmd[5] == ' ' || 
		cmd[5] == '\0' || cmd[5] == '\t')) {
		for (p = cmd+5; *p != '\0' && (*p == ' ' || *p == '\t'); p++);
		if (*p == '\0') {
			write(1, "?must specify hostname\n", 23);
			return;
		}
		remoteLook(p);
	}
	else if (strncmp(cmd, "secure", 6) == 0 && (cmd[6] == ' ' ||
		cmd[6] == '\0' || cmd[6] == '\t')) {
		one = getPass("key:");
		if (strlen(one) < 8) {
			write(1, "?key must be 8 characters\n", 26);
			return;
		}
		strcpy(key, one);
		two = getPass("verify:");
		if (strcmp(key, two) != 0) {
			write(1, "?sorry\n", 7);
			return;
		}
		key[8] = '\0';
		loadkey(key, SHIFT); /* loadkey() erases key! */
		strncpy(two, "XXXXXXXX", 8); /* erase this too */
		writemsg(server, "/secure", 7, UNSECURE);
		clientStatus = SECURE;
	}
	else if (strncmp(cmd, "unsecure", 8) == 0 && (cmd[8] == ' ' ||
		cmd[8] == '\0' || cmd[8] == '\t')) {
		writemsg(server, "/unsecure", 9, UNSECURE);
		clientStatus = UNSECURE;
	}
	else if (strncmp(cmd, "help", 4) == 0 && (cmd[4] == ' ' || 
		cmd[4] == '\0' || cmd[4] == '\t')) {
		printf("%s\n", helpMessage);
	}
	else if (strncmp(cmd, "quit", 4) == 0 && (cmd[4] == ' ' || 
		cmd[4] == '\0' || cmd[4] == '\t')) {
		quit();
	}
	else {
		/* Send command to the server, put '/' back on */
		writemsg(server, cmd-1, size+1, UNSECURE);
	}
}

setTerm()
{
	ioctl(0, TIOCGETP, &oldttysg);
	ioctl(0, TIOCGETC, &oldttytc);
	ioctl(0, TIOCGLTC, &oldttylt);

	newttysg = oldttysg;
	newttytc = oldttytc;
	newttylt = oldttylt;

	newttysg.sg_flags |= CBREAK;
	newttysg.sg_flags &= ~ECHO;
	newttytc.t_intrc = -1; /* disable ^C */
	newttylt.t_suspc = -1; /* disable ^Z */
	
	ioctl(0, TIOCSETP, &newttysg);
	ioctl(0, TIOCSETC, &newttytc);
	ioctl(0, TIOCSLTC, &newttylt);
}

resetTerm()
{
	ioctl(0, TIOCSETP, &oldttysg);
	ioctl(0, TIOCSETC, &oldttytc);
	ioctl(0, TIOCSLTC, &oldttylt);
}

sendIdent(server)
int server;
{
	int size;
	char *getenv(), *nick;
	char hostname[HOST_SIZE+1];
	struct passwd *pwd;

	pwd = getpwuid(getuid());
	
	while (writemsg(server, pwd->pw_name, strlen(pwd->pw_name, 
		UNSECURE)) < 0) {
		write(1, "?cannot write to the server\n", 28);
		exit(1);
	}

	if (av[1] != NULL) {
		if (strlen(av[1]) > NICK_SIZE) {
			write(1, "?nick too long\n", 15);
			exit(1);
		}
		if (writemsg(server, av[1], strlen(av[1]), UNSECURE) < 0) {
			write(1, "?cannot write to the server\n", 28);
			exit(1);
		}
	}
	else {
		if ((nick = getenv("NICK")) == NULL) {
			if (writemsg(server, pwd->pw_name, strlen(pwd->pw_name,
				UNSECURE)) < 0) {
				write(1, "?cannot write to the server\n", 28);
				exit(1);
			}
		}
		else {
			if (strlen(nick) > NICK_SIZE) {
				write(1, "?nick too long\n", 14);
				exit(1);
			}
			if (writemsg(server, nick, strlen(nick), UNSECURE) < 0) {
				write(1, "?cannot write to the server\n", 28);
				exit(1);
			}
		}
	}

	if (gethostname(hostname, HOST_SIZE) < 0) {
		perror("gethostname");
		exit(1);
	}

	sprintf(ident, "%s@%s", pwd->pw_name, hostname);
}
		
quit()
{
	printf("\nConnection closed.\n");
	resetTerm();
	exit(0);
}

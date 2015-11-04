/******************************************************************************
*
*	it.server.c
*
*	Internet Talk Server
*
*	by Mark R Boyns
*	August 1991
*
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include "defs.h"
#include "serverPort.h"

struct clientInfo clientList[MAX_CLIENTS];
extern int sighup();

main()
{
	char hostname[HOST_SIZE+1];
	struct hostent *host;
	struct sockaddr_in sin;
	int server;
	fd_set mask, rfd;
	register int client;
	
	signal(SIGHUP, sighup);

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
	sin.sin_port = htons(SERVER_PORT);
	
	if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	if (bind(server, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(server, 5) < 0) {
		perror("listen");
		exit(1);
	}

	for (client = 0; client < MAX_CLIENTS; client++)
		clientList[client].status = INVALID;

	FD_ZERO(&mask);
	FD_ZERO(&rfd);
	FD_SET(server, &mask);

	for(;;) {
		rfd = mask;
		select(FD_SETSIZE, &rfd, (fd_set *)0, (fd_set *)0, 0);
		if (FD_ISSET(server, &rfd)) {
			addClient(server, &mask);
		}
		for (client = 0; client < MAX_CLIENTS; client++) {
			if (clientList[client].status != INVALID &&
				FD_ISSET(clientList[client].fd, &rfd)) {
				if (doClient(client) < 0) {
					delClient(client, &mask);
				}
			}
		}
	}
}

addClient(server, mask)
int server;
fd_set *mask;
{
	register int client;
	int addrlen, fd, size, status;
	char message[MESSAGE_SIZE];
	struct sockaddr_in peer;
	struct hostent *hp;

	addrlen = sizeof(peer);

	if ((fd = accept(server, &peer, &addrlen)) < 0) {
		return -1;
	}	

	if ((hp = gethostbyaddr(&peer.sin_addr, sizeof(peer.sin_addr),
		AF_INET)) == 0) {
		return -1;
	}

	for (client = 0; client < MAX_CLIENTS; client++) {
		if (clientList[client].status == INVALID) break;
	}

	if (client == MAX_CLIENTS) {
	/* this message does not get sent to the user? */
	/* the user just gets ignored */
/*
		write(fd, "!client limit exceeded\n", 22);
*/
		close(fd);
		return -1;
	}

	clientList[client].fd = fd;
	clientList[client].status = NEED_USER;
	strncpy(clientList[client].addr, inet_ntoa(peer.sin_addr), ADDR_SIZE);
	clientList[client].addr[ADDR_SIZE-1] = '\0';
	strcpy(clientList[client].host, hp->h_name, HOST_SIZE);
	clientList[client].addr[HOST_SIZE-1] = '\0';
	FD_SET(clientList[client].fd, mask);
}	

int doClient(client)
register int client;
{
	char message[MESSAGE_SIZE];
	char buf[MESSAGE_SIZE];
	int size;
	int status;

	printf("debug: waiting for client message\n");
	if ((size = readmsg(clientList[client].fd, message, &status)) < 0) {
		return -1;
	}
	printf("debug: waiting for client message\n");

	switch(clientList[client].status) {
	case	NEED_USER:
		strncpy(clientList[client].user, message, size);
		clientList[client].user[size] = '\0';
		clientList[client].status = NEED_NICK;
		break;
	case	NEED_NICK:
		strncpy(clientList[client].nick, message, size);
		clientList[client].nick[size] = '\0';
		clientList[client].status = UNSECURE;
		sprintf(buf, "!<%s> is here", clientList[client].nick);
		broadCast(client, buf, strlen(buf), UNSECURE);
		sendMotd(clientList[client].fd);
		break;
	case UNSECURE:
		if (*message == '/') {
			doCommand(client, message+1, size-1);
			break;
		}
		sprintf(buf, "<%s> %s", clientList[client].nick, message);
		broadCast(client, buf, strlen(clientList[client].nick)+size+3, 
			UNSECURE);
		break;
	case SECURE:
		if (status == UNSECURE) /* it must be a command! */
			doCommand(client, message+1, size-1);
		else
			broadCast(client, message, size, SECURE);
		break;
	}

	return 1;
}

broadCast(from, message, size, status)
int from;
char *message;
int size;
int status;
{
	register int to;

	if (status == SECURE) {
		for (to = 0; to < MAX_CLIENTS; to++) {
			if (clientList[to].status == SECURE && to != from) 
				writemsg(clientList[to].fd, message, size, SECURE);
		}
	}
	else if (status == UNSECURE) {
		for (to = 0; to < MAX_CLIENTS; to++) {
			if (clientList[to].status > VALID && to != from)
				writemsg(clientList[to].fd, message, size, UNSECURE);
		}
	}
}

doCommand(client, cmd, size)
int client;
char *cmd;
int size;
{
	char buf[MESSAGE_SIZE];
	char output[MESSAGE_SIZE];
	register int x;
	int len;
	char c, *p;

	cmd[size] = '\0';

	if (strncmp(cmd, "who", 3) == 0 && (cmd[3] == ' ' || 
		cmd[3] == '\0' || cmd[3] == '\t')) {
		*output = '\0';
		for (x = 0; x < MAX_CLIENTS; x++) {
			if (clientList[x].status > VALID) {
				c = (clientList[x].status == SECURE) ? '*' : ' ';
				sprintf(buf, "+[%d] %s@%s <%s> %s%c\n", x+1,
					clientList[x].user, clientList[x].host, 
					clientList[x].nick, clientList[x].addr, c);
				strcat(output, buf);
			}
		}
		writemsg(clientList[client].fd, output, strlen(output)-1, UNSECURE);
	}
	else if (strncmp(cmd, "nick", 4) == 0 && (cmd[4] == ' ' || 
		cmd[4] == '\0' || cmd[4] == '\t')) {
		for (p = cmd+4; *p != '\0' && (*p == ' ' || *p == '\t'); p++);
		if (*p == '\0') {
			writemsg(clientList[client].fd, "?no change", 10, UNSECURE);
			return;
		}
		if (strlen(p) > NICK_SIZE) {
			writemsg(clientList[client].fd, "?nick too long", 14, UNSECURE);
			return;
		}
		sprintf(output, "!<%s> is now <%s>", clientList[client].nick, p);
		broadCast(client, output, strlen(output), UNSECURE);
		strcpy(clientList[client].nick, p);
	}
	else if (strncmp(cmd, "secure", 6) == 0 && (cmd[6] == ' ' ||
		cmd[6] == '\0' || cmd[6] == '\t')) {
		clientList[client].status = SECURE;
		sprintf(output, "!<%s> is now secure", clientList[client].nick);
		broadCast(client, output, strlen(output), UNSECURE);
	}
	else if (strncmp(cmd, "unsecure", 8) == 0 && (cmd[8] == ' ' ||
		cmd[8] == '\0' || cmd[8] == '\t')) {
		clientList[client].status = UNSECURE;
		sprintf(output, "!<%s> is now unsecure", clientList[client].nick);
		broadCast(client, output, strlen(output), UNSECURE);
	}
	else {
		writemsg(clientList[client].fd, "?no such command", 16, UNSECURE);
	}
}

sendMotd(client)
register int client;
{
	int fd, size;
	char message[MESSAGE_SIZE];

	if ((fd = open(MOTD, O_RDONLY, 0)) < 0) {
		writemsg(client, "\0", 1, UNSECURE); /* don't rely on MOTD file */
		return;
	}

	/* Only read upto MESSAGE_SIZE characters from MOTD */
	if ((size = read(fd, message, MESSAGE_SIZE)) < 0) {
		writemsg(client, "\0", 1, UNSECURE); /* don't rely on MOTD file */
		return;
	}

	close(fd);

	writemsg(client, message, size, UNSECURE);
}

delClient(client, mask) 
register int client;
fd_set *mask;
{
	char message[MESSAGE_SIZE];

	FD_CLR(clientList[client].fd, mask);
	clientList[client].status = INVALID;
	(void) close(clientList[client].fd);
	sprintf(message, "!<%s> is gone", clientList[client].nick);
	broadCast(client, message, strlen(message), UNSECURE);
}

/* a nice way for the server to die... */
sighup()
{
	register int client;

	broadCast(-1, "!it.server received SIGHUP... shutting down", 43, UNSECURE);

	for (client = 0; client < MAX_CLIENTS; client++) {
		if (clientList[client].status > VALID)
			close(clientList[client].fd);
	}

	exit(0);
}

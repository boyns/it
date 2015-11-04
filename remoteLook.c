/******************************************************************************
*
*	remoteLook.c
*
*	Internet Talk remote look interface
*
*	by Mark R Boyns
*	August 1991
*
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include "defs.h"
#include "remotePort.h"

int remoteLook(host)
char *host;
{
	struct hostent *hp;
	struct sockaddr_in sin;
	int sock;
	int size, status;
	unsigned long inaddr;
	char message[MESSAGE_SIZE];
	char *p, *index(), request;

	if ((inaddr = inet_addr(host)) != INADDR_NONE) {
		bcopy(&inaddr, &sin.sin_addr, sizeof(inaddr));
		if ((hp = gethostbyaddr(&sin.sin_addr, sizeof(sin.sin_addr),
			AF_INET)) == 0) {
			herror("?rlook");
			return;
		}
	}
	else {
		if ((hp = gethostbyname(host)) == 0) {
			herror("?rlook");
			return;
		}
		bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);
	}

	sin.sin_family = AF_INET;
	sin.sin_port = htons(REMOTE_PORT);
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return;
	}

	if ((connect(sock, &sin, sizeof(sin))) < 0) {
		fprintf(stderr, "?can't connect to it.remoted at %s\n", hp->h_name);
		return;
	}

	request = RLOOK;
	if (writemsg(sock, &request, 1, UNSECURE) < 0) {
		fprintf(stderr, "?remote look failed\n");
		return;
	}

	if ((size = readmsg(sock, message, &status)) < 0) {
		fprintf(stderr, "?remote look failed\n");
		return;
	}
	else {
		write(1, message, size);
	}
}

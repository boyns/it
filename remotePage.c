/******************************************************************************
*
*	remotePage.c
*
*	Internet Talk remote page interface
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

int remotePage(address, from)
char *address;
char *from;
{
	struct hostent *hp;
	struct sockaddr_in sin;
	int sock;
	int size, status;
	unsigned long inaddr;
	char message[MESSAGE_SIZE];
	char touser[USER_SIZE+1];
	char host[HOST_SIZE+1];
	char *p, *index(), request;

	if (strlen(address) < 3) {
		fprintf(stderr, "?must specify user@address\n");
		return;
	}

	if ((p = index(address, '@')) == 0) {
		fprintf(stderr, "?must specify user@address\n");
		return;
	}

	*p = '\0';

	if (strlen(address) > USER_SIZE) {
		fprintf(stderr, "?username too long\n");
		return;
	}

	if (*(p+1) == '\0' || *address == '\0') {
		fprintf(stderr, "?must specify user@address\n");
		return;
	}

	if (strlen(p+1) > HOST_SIZE) {
		fprintf(stderr, "?hostname too long\n");
		return;
	}

	strcpy(touser, address);
	strcpy(host, p+1);

	if ((inaddr = inet_addr(host)) != INADDR_NONE) {
		bcopy(&inaddr, &sin.sin_addr, sizeof(inaddr));
		if ((hp = gethostbyaddr(&sin.sin_addr, sizeof(sin.sin_addr),
			AF_INET)) == 0) {
			herror("?rpage");
			return;
		}
	}
	else {
		if ((hp = gethostbyname(host)) == 0) {
			herror("?rpage");
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

	request = RPAGE;
	if (writemsg(sock, &request, 1, UNSECURE) < 0) {
		fprintf(stderr, "?remote page failed\n");
		return;
	}

	if (writemsg(sock, from, strlen(from), UNSECURE) < 0) {
		fprintf(stderr, "?remote page failed\n");
		return;
	}

	if (writemsg(sock, touser, strlen(touser), UNSECURE) < 0) {
		fprintf(stderr, "?remote page failed\n");
		return;
	}

	if ((size = readmsg(sock, message, &status)) < 0) {
		fprintf(stderr, "?remote page failed\n");
		return;
	}
	else {
		write(1, message, size);
	}
}

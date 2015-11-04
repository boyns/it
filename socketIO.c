/******************************************************************************
*
*	socketIO.c
*
*	Internet Talk socket input/output routines
*
*	by Mark R Boyns
*	August 1991
*
*/

#include "defs.h"

/*
 *	Read "n" bytes from a stream socket.
 */
int nread(fd, buf, nbytes)
register int fd;
register char *buf;
register int nbytes;
{
	register int nleft, nread;

	nleft = nbytes;
	while (nleft > 0) {
		nread = read(fd, buf, nleft);
		if (nread <= 0)
			return -1;
		nleft -= nread;
		buf += nread;
	}
	return 1;
}

/*
 *	Write "n" bytes to a stream socket.
 */
int nwrite(fd, buf, nbytes)
register int fd;
register char *buf;
register int nbytes;
{
	register int nleft, nwritten;

	nleft = nbytes;
	while (nleft > 0) {
		nwritten = write(fd, buf, nleft);
		if (nwritten <= 0)
			return -1;
		nleft -= nwritten;
		buf += nwritten;
	}
	return 1;
}

int readmsg(fd, message, status)
int fd;
char *message;
int *status;
{
	int size;

	if (nread(fd, &size, INT_SIZE) < 0) {
		return -1;
	}

	if (nread(fd, status, INT_SIZE) < 0) {
		return -1;
	}

	if (nread(fd, message, size) < 0) {
		return -1;
	}

	return size;
}

int writemsg(fd, message, size, status)
int fd;
char *message;
int size;
int status;
{
	if (nwrite(fd, &size, INT_SIZE) < 0) {
		return -1;
	}

	if (nwrite(fd, &status, INT_SIZE) < 0) {
		return -1;
	}

	if (nwrite(fd, message, size) < 0) {
		return -1;
	}

	return 0;
}

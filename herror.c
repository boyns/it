/******************************************************************************
* 
*	herror.c
*
*	Internet Talk host error function
*
*	by Mark R Boyns
*	August 1991
*
*/

#include <netdb.h>
#include <stdio.h>

extern int h_errno;	

char *h_errlist[] = {
	"no error, h_errno=0",
	"host not found",
	"try again",
	"no recovery",
	"no data",
	"no address"
};

herror(msg)
char *msg;
{
	fprintf(stderr, "%s: %s\n", msg, h_errlist[h_errno]);
}

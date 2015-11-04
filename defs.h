/**************************************************************************
*
*	defs.h
*
*	Internet Talk definitions
*
*	by Mark R Boyns
*	August 1991
*
*/

#define	INT_SIZE		4	/* # of bytes/int */
#define	MESSAGE_SIZE	2048
#define	HOST_SIZE		32
#define	USER_SIZE		8
#define	NICK_SIZE		16
#define	ADDR_SIZE		16

#define	IDENT_SIZE	(HOST_SIZE+USER_SIZE+2) /* user@host */
#define	INPUT_SIZE	(MESSAGE_SIZE-IDENT_SIZE-3) /* "[%s] %s" */

#define	INVALID		-1
#define	NEED_USER		0
#define	NEED_NICK		1
#define	VALID		3
#define	UNSECURE		4
#define	SECURE		5

#define	MAX_CLIENTS	10

#define	PAGER		"it.pager" /* name of pager found in $HOME */
#define	MOTD			"/home/students/boyns/src/it/it.motd"

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif

/* used with it.remoted */
#define	RPAGE		1
#define	RLOOK		2

#define	GOD			"boyns"

struct clientInfo {
	int fd;
	int status;
	char user[USER_SIZE+1];
	char nick[NICK_SIZE+1];
	char host[HOST_SIZE+1];
	char addr[ADDR_SIZE+1];
};

extern char *checkUsers();
extern char *getPass();

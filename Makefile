###############################################################################
#
#	Makefile
#
#	Internet Talk
#
#	by Mark Boyns
#	August 1991
#
COBJ=	it.client.o socketIO.o checkUsers.o DES.o crypt.o\
		remotePage.o getPass.o look.o remoteLook.o shellEscape.o\
		redirectCommand.o herror.o
SOBJ=	it.server.o socketIO.o daemonize.o herror.o
POBJ=	it.pager.o checkUsers.o
ROBJ=	it.remoted.o checkUsers.o socketIO.o daemonize.o look.o herror.o
LIB=		-lresolv
CFLAGS=	-O
CC=		cc

all:	it.server it it.pager it.remoted

it.server: $(SOBJ)
	$(CC) -o it.server $(SOBJ) $(LIB)

it: $(COBJ)
	$(CC) -o it $(COBJ) $(LIB)

it.pager:	$(POBJ)
	$(CC) -o it.pager $(POBJ)

it.remoted:	$(ROBJ)
	$(CC) -o it.remoted $(ROBJ) $(LIB)

clean:
	rm -f core $(COBJ) $(SOBJ) $(POBJ) $(ROBJ) a.out

it.client.o:	it.client.c
	cc -O -c it.client.c

it.server.o:	it.server.c
	cc -O -c it.server.c

it.pager.o:	it.pager.c
	cc -O -c it.pager.c

it.remoted.o:	it.remoted.c
	cc -O -c it.remoted.c

socketIO.o:	socketIO.c
	cc -O -c socketIO.c

checkUsers.o:	checkUsers.c
	cc -O -c checkUsers.c

DES.o:		DES.c
	cc -O -c DES.c

crypt.o:		crypt.c
	cc -O -DFASTDES -DCIPHER_BLOCK_CHAIN -c crypt.c

getPass.o:	getPass.c
	cc -O -c getPass.c

remotePage.o:	remotePage.c
	cc -O -c remotePage.c

daemonize.o:	daemonize.c
	cc -O -c daemonize.c

herror.o:		herror.c
	cc -O -c herror.c

look.o:		look.c
	cc -O -c look.c

remoteLook.o:	remoteLook.c
	cc -O -c remoteLook.c

shellEscape.o:	shellEscape.c
	cc -O -c shellEscape.c

redirectCommand.o:	redirectCommand.c
	cc -O -c redirectCommand.c

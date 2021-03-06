apue/opend.fe/Makefile                                                                                 664     340      24          303  5211773605  10156                                                                                                                                                                                                                                                                                                                                                                      include ../Make.defines

PROGS =	opend
OBJS = cliargs.o main.o request.o

all:	${PROGS}

${OBJS}:opend.h

opend:	${OBJS}
		${LINK.c} -o $@ ${OBJS} ${LDLIBS}

clean:
		rm -f ${PROGS} ${TEMPFILES}
.c M  streams   0  &  termios      R  test os ) {
		line[strlen(line) - 1] = 0;	/* replace newline with null */

					/* open the file */
		if ( (fd = csopen(line, O_RDONLY)) < 0)
			continue;	/* csopen() prints error from server */

					/* and cat to stdout */
		while ( (n = read(fd, buf, BUFFSIZE)) > 0apue/opend.fe/main.c                                                                                   664     340      24          676  5211773605   7623                                                                                                                                                                                                                                                                                                                                                                      #include	"opend.h"

			/* define global variables */
char	 errmsg[MAXLINE];
int		 oflag;
char	*pathname;

int
main(void)
{
	int		nread;
	char	buf[MAXLINE];

	for ( ; ; ) {	/* read arg buffer from client, process request */
		if ( (nread = read(STDIN_FILENO, buf, MAXLINE)) < 0)
			err_sys("read error on stream pipe");
		else if (nread == 0)
			break;		/* client has closed the stream pipe */

		request(buf, nread, STDIN_FILENO);
	}
	exit(0);
}
* and cat to stdout */
		while ( (n = read(fd, buf, BUFFSIZE)) > 0apue/opend.fe/opend.h                                                                                  664     340      24          654  5211773605  10005                                                                                                                                                                                                                                                                                                                                                                      #include	<sys/types.h>
#include	<errno.h>
#include	"ourhdr.h"

#define	CL_OPEN "open"				/* client's request for server */

			/* declare global variables */
extern char	 errmsg[];	/* error message string to return to client */
extern int	 oflag;		/* open() flag: O_xxx ... */
extern char	*pathname;	/* of file to open() for client */

			/* function prototypes */
int		 cli_args(int, char **);
void	 request(char *, int, int);
);
	}
	exit(0);
}
* and cat to stdout */
		while ( (n = read(fd, buf, BUFFSIZE)) > 0apue/opend.fe/cliargs.c                                                                                664     340      24          701  5211773777  10322                                                                                                                                                                                                                                                                                                                                                                      #include	"opend.h"

/* This function is called by buf_args(), which is called by
 * request().  buf_args() has broken up the client's buffer
 * into an argv[] style array, which we now process. */

int
cli_args(int argc, char **argv)
{
	if (argc != 3 || strcmp(argv[0], CL_OPEN) != 0) {
		strcpy(errmsg, "usage: <pathname> <oflag>\n");
		return(-1);
	}

	pathname = argv[1];		/* save ptr to pathname to open */
	oflag = atoi(argv[2]);
	return(0);
}
nd cat to stdout */
		while ( (n = read(fd, buf, BUFFSIZE)) > 0apue/opend.
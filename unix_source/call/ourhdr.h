apue/call/call.c                                                                                       664     340      24         1364  5211773645   7046                                                                                                                                                                                                                                                                                                                                                                      #include	"call.h"
#include	<sys/uio.h>		/* struct iovec */

/* Place the call by sending the "args" to the calling server,
 * and reading a file descriptor back. */

int
call(const char *args)
{
	int				csfd, len;
	struct iovec	iov[2];

					/* create connection to conn server */
	if ( (csfd = cli_conn(CS_CALL)) < 0)
		err_sys("cli_conn error");

	iov[0].iov_base = CL_CALL " ";
	iov[0].iov_len  = strlen(CL_CALL) + 1;
	iov[1].iov_base = (char *) args;
	iov[1].iov_len  = strlen(args) + 1;
							/* null at end of args always sent */
	len = iov[0].iov_len + iov[1].iov_len;
	if (writev(csfd, &iov[0], 2) != len)
		err_sys("writev error");

					/* read back descriptor */
					/* returned errors handled by write() */
	return( recv_fd(csfd, write) );
}
 size of output file */
	if (lseek(fdout, statbuf.st_size - 1, SEEK_SET) == -1)
		err_sys("lseek error");
	if (write(fdout, "", 1) != 1)
		err_sys("write error");

	if ( (src = mmap(0, statbuf.st_size, PROT_READ,
					 MAP_FILE | MAP_SHARED, fdin, 0)) == (caddr_t) -1)apue/call/call.h                                                                                       664     340      24         1156  5211773542   7046                                                                                                                                                                                                                                                                                                                                                                      #include	<sys/types.h>
#include	<sys/time.h>
#include	<errno.h>
#include	<termios.h>
#include	"ourhdr.h"

#define	CS_CALL	"/home/stevens/calld"	/* well-known server name */
#define	CL_CALL	"call"					/* command for server */

				/* declare global variables */
extern char	 escapec;	/* tilde for local commands */
extern char	*src;		/* for take and put commands */
extern char	*dst;		/* for take and put commands */

				/* function prototypes */
int		call(const char *);
int		doescape(int);
void	loop(int);
int		prompt_read(char *, int (*)(int, char **));
void	put(int);
void	take(int);
int		take_put_args(int, char **);
ys("writev error");

					/* read back descriptor */
					/* returned errors handled by write() */
	return( recv_fd(csfd, write) );
}
 size of output file */
	if (lseek(fdout, statbuf.st_size - 1, SEEK_SET) == -1)
		err_sys("lseek error");
	if (write(fdout, "", 1) != 1)
		err_sys("write error");

	if ( (src = mmap(0, statbuf.st_size, PROT_READ,
					 MAP_FILE | MAP_SHARED, fdin, 0)) == (caddr_t) -1)apue/call/escape.c                                                                                     664     340      24         2303  5211773646   7366                                                                                                                                                                                                                                                                                                                                                                      #include	"call.h"
#include	<signal.h>

/* Called when first character of a line is the escape character
 * (tilde).  Read the next character and process.  Return -1
 * if next character is "terminate" charcter, 0 if next character
 * is valid command character (that's been processed), or next
 * character itself (if the next character is not special). */

int
doescape(int remfd)
{
	char	c;

	if (read(STDIN_FILENO, &c, 1) != 1)		/* next input char */
		err_sys("read error from stdin");

	if (c == escapec)		/* two in a 
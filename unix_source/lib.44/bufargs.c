apue/lib.44/recvfd.c                                                                                   664     340      24         3760  5211773727   7470                                                                                                                                                                                                                                                                                                                                                                      #include	<sys/types.h>
#include	<sys/socket.h>		/* struct msghdr */
#include	<sys/uio.h>			/* struct iovec */
#include	<stddef.h>
#include	"ourhdr.h"

static struct cmsghdr	*cmptr = NULL;		/* malloc'ed first time */
#define	CONTROLLEN	(sizeof(struct cmsghdr) + sizeof(int))
		 /* size of control buffer to send/recv one file descriptor */
/* Receive a file descriptor from another process (a server).
 * In addition, any data received from the server is passed
 * to (*userfunc)(STDERR_FILENO, buf, nbytes).  We have a
 * 2-byte protocol for receiving the fd from send_fd(). */
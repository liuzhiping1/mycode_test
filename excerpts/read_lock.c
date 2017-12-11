#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sdio.h>
#include <stdlib.h>
#include <string.h>

void lock_set(int fd ,int type);

int main (void)
{
	int fd,nwrite,nread,len;
	char *buff = "hello\n";
	char buf_r[100];
	len = strlen(buff);
	fd = open("hello",O_RDWR | O_CREAT,0666);
	if(fd < 0)
	{
		perror("open failed!");
		exit(1);
	}
	//lock write
	lock_set(fd,F_WRCLK);
	if(len == (nwrite = write(fd,buff,len)))
	{
		printf("write success!");
	}
	getchar();
	//unlock
	lock_set(fd,F_UNLCK);
	getchar();
	//lock read
	lock_set(fd,F_RDCLK);
	lseek(fd,0,SEEK_SET);
	if((nread = read(fd,buf_r)) == len)
	{
		printf("read :%s \n",buf_r);
	}
	getchar();
	//unlock
	lock_set(fd,F_UNLCK);
	getchar();
	close(fd);
	exit(0);
}
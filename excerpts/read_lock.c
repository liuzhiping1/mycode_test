#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
	if(len == (nwrite = write(fd,buff,len)))
	{
		printf("write success!");
	}
	getchar();
	//unlock
	getchar();
	//lock read
	lseek(fd,0,SEEK_SET);
	if((nread = read(fd,buf_r,len)) == len)
	{
		printf("read :%s \n",buf_r);
	}
	getchar();
	//unlock
	getchar();
	close(fd);
	exit(0);
}

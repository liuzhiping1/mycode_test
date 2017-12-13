#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define BUFSZ    2048

int main()
{
	int shmid ,i,fd,nwrite,nread;
	char *shmadd;
	char buff[5];
	
	//create share mem
	if((shmid=shmget(IPC_PRIVATE,BUFSZ,0666))<0)
	{
		perror("shmget");
		exit(1);
	}
	else
	printf("created shared-memory\n");
	//at share mem
	if(shmadd=shmat(shmid,0,0)< (char *)0)
	{
		perror("shmat");
		exit(1);
	}
	else
		printf("attached share-mem\n");
	//open
	if(fd = open("share",O_CREAT|O_RDWR,0666) < 0 )
	{
		perror("open");
		exit(1);
	}
	else
		printf("open success !\n");
	//write
	if(nwrite=write(fd,shmadd,5) < 0)
	{
		perror("write");
		exit(1);
	}
	else
		printf("write success!\n");
	lseek(fd,0,SEEK_SET);
	//read
	if((nread=read(fd,buf,5)) <0)
	{
		perror("read");
		exit(1);
	}
	else
		printf("read %d from file:%s\n",nread,buf);
	//delete share mem
	if((shmdt(shmadd))<0)
	{
		perror("shmdt");
		exit(1);
	}
	else
		printf("delete shared-memory\n");
	exit(0);
}

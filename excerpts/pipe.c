#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define BUFFSIZE    100

int main ()
{
    FILE *fp;
    char *cmd = "ps -ef"	;
    char buf[BUFFSIZE];

    if((fp=popen(cmd,"r")) == NULL )
        perror("open failed"); 
    while ((fgets(buf,BUFFSIZE,fp)) != NULL)
    {
	printf("%s \n",buf);		
    }
    pclose(fp);
    exit(0);
}

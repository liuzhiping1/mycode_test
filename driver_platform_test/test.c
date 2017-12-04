#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <stdio.h>  
  
main()  
{  
    int fd,i,lednum;  
  
    fd = open("/dev/beep",O_RDWR);  
    if(fd<0)  
    {  
        perror("open fail \n");  
        return ;  
    }  
      
    sleep(10);  
    close(fd);  
}  
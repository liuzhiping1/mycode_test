#include <sys/mman.h>
#include <stdio.h>
#include <asm-generic/fcntl.h>
#include <string.h>
#include <unistd.h>
void main()
{
int fb;
 unsigned char *fb_mem;
fb = open("/dev/fb0",O_RDWR);
fb_mem = mmap(NULL , 1024*768,PROT_READ|PROT_WRITE,MAP_SHARED,fb,0);
sleep(3);
memset(fb_mem,1,1024*768);
sleep(3);

munmap(NULL,1024*768);

}

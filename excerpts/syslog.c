#include <syslog.h>

void main (void)
{
    openlog("lzp",LOG_PID,LOG_PERROR);
    syslog(LOG_INFO,"that is my test!!");
    syslog(LOG_DEBUG,"this is debug message!");
    closelog();
}

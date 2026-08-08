#include <stdio.h>
#include <time.h>
#include <sys/utsname.h>
#include <unistd.h>

int main(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", t);

    struct utsname sys;
    uname(&sys);

    printf("webserv cgi runner\n");
    printf("-------------------\n");
    printf("status       OK\n");
    printf("pid          %d\n", getpid());
    printf("os           %s %s\n", sys.sysname, sys.release);
    printf("host         %s\n", sys.nodename);
    printf("compiler     gcc\n");
    printf("source       c.c\n");
    printf("executed at  %s\n", timestr);
    printf("-------------------\n");
    printf("compiled and executed via CGI. output captured and returned by run_c.py.\n");

    return 0;
}
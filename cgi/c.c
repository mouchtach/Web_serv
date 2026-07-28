#include <stdio.h>
#include <stdlib.h>
int main(void) {
    char *len_str = getenv("CONTENT_LENGTH");
    int len = len_str ? atoi(len_str) : 0;
    char body[4096] = {0};
    if (len > 0) fread(body, 1, len > 4095 ? 4095 : len, stdin);
    printf("Status: 200 OK\r\nContent-Type: text/plain\r\n\r\n");
    printf("received %d bytes: %s\n", len, body);
    return 0;
}
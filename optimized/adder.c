/*
* adder.c - the adder function which add two numbers together
*/
#include "csapp.h"

void adder(int fd, char* cgiargs){
    char *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE], buf[MAXLINE];
    int n1=0,n2=0;

    if(cgiargs != NULL){
        p = strchr(cgiargs,'&');
	*p = '\0';
	strcpy(arg1,cgiargs);
	strcpy(arg2,p+1);
	n1 = atoi(arg1);
	n2 = atoi(arg2);
    }

    sprintf(content, "Welcome to add.com:");
    sprintf(content, "%sThe Internet addition portal.\r\n<p>", content);
    sprintf(content, "%sThe answer is:%d + %d = %d \r\n<p>",
	    content, n1, n2, n1 + n2);
    sprintf(content, "%sThanks for visiting!\r\n", content);

    sprintf(buf, "Content-length: %d\r\n", (int)strlen(content));
    sprintf(buf, "%sContent-type: text/html\r\n\r\n", buf);
    sprintf(buf, "%s%s",buf, content);
    Rio_writen(fd, buf, strlen(buf));
    return;
}

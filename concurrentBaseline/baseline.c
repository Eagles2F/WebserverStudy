/*
* baseline.c - A simple HTTP/1.0 concurrent Web server that uses
* the GET method to serve static and dynamic content.
*
*  - threads are used to support the concurrency.
*  - giving credit to the tiny webser on the csapp book. 
*  @Author: Yifan Li
*  @Date: Feb 2015
*/
#include "csapp.h"

void serve(int fd);
void *thread(void *vargp);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char* filename, char *filetype);
void serve_dynamic(int fd, char* filename, char *cgiargs);
void clienterror(int fd, char*cause, char* errnum,
 		char* shortmsg, char* longmsg);
// start of main
int main(int argc, char**argv){
    int listenfd, *connfdp, port;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;
   
    //check argv
    if(argc !=2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);    
    }
    port =  atoi(argv[1]);

    listenfd = Open_listenfd(port);
    while(1){
	connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
 	Pthread_create(&tid, NULL, thread, connfdp);
    }
}
//end of main

// Thread routines
void *thread(void *vargp){
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    serve(connfd);
    Close(connfd);
    return NULL;
}

//serve - serve one HTTP/1.0 GET request 
void serve(int fd){
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE],method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    // read request line and headers
    Rio_readinitb(&rio,fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    if(strcasecmp(method, "GET")){
       clienterror(fd, method, "501", "Not Implemented",
      		 "Baseline server doesn't implement this method");
       return;
    }
    read_requesthdrs(&rio);

    //Parse URI from GET request
    is_static  = parse_uri(uri, filename, cgiargs);
    if(stat(filename, &sbuf) < 0){
        clienterror(fd, filename, "404", "Not found",
		    "Baseline server couldn't find this file");
        return ;
    }
    
    //serving static content
    if (is_static) {
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
	    clienterror(fd, filename, "403", "Forbidden",
			"Baseline server couldn't read the file");
	    return;
	}
       serve_static(fd, filename, sbuf.st_size);
    }
    else {//serving dynamic content
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
	    clienterror(fd, filename, "403", "Forbidden",
			"Baselien server couldn't run the CGI program");
	    return;
	}
	serve_dynamic(fd, filename, cgiargs);
    }
}

//read_requesthdrs - read and parse HTTP request headers
void read_requesthdrs(rio_t* rp){
    char buf[MAXLINE];
    
    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")){
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
    return;
}

//parse_uri - parse URI into filename and CGI args
//	      return 0 if dynamic content, 1 if static
int parse_uri(char *uri, char *filename, char *cgiargs){
    char *ptr;
    if(!strstr(uri, "cgi-bin")){ // static content
    	strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
  	if(uri[strlen(uri)-1] == '/')
	    strcat(filename, "home.html");
	return 1;
    }
    else{// dynamic content
        ptr = index(uri, '?');
        if(ptr) {
            strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
	} 
        else
	    strcpy(cgiargs, "");
 	strcpy(filename, ".");
	strcat(filename, uri);
	return 0;
    }
}

// serve_static - copy a file back to the client
void serve_static(int fd, char *filename, int filesize){
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    //send response header to the client
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Baseline Server\r\n",buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type:%s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    //send response body to client
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}

//get_filetype - derive file type from file name
void get_filetype(char *filename, char *filetype){
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else
	strcpy(filetype, "text/plain");
}

//serve_dynamic - run a CGI program on behalf of the client
void serve_dynamic(int fd, char *filename, char *cgiargs){
    char buf[MAXLINE], *emptylist[] = {NULL};

    // return first part of HTTP response
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Baseline Server\r\n"); 
    Rio_writen(fd, buf, strlen(buf));

    if(Fork()==0){//child
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
	Execve(filename, emptylist, environ);
    }
    Wait(NULL);
}

//clienterror - returns an error message to the client
void clienterror(int fd, char *cause, char *errnum,
		char *shortmsg, char *longmsg){
    char buf[MAXLINE], body[MAXLINE];
    
    //build the HTTP response body
    sprintf(body, "<html><title>Baseline Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n",body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Baseline server</em>\r\n", body);
	
    //Print the HTTP response
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}


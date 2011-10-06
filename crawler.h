#ifndef _CRAWLER_H
#define _CRAWLER_H 

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<netdb.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<sys/select.h>
#include<string.h>
#include<pthread.h>
#include<pcre.h>

#include"hiredis.h"

#define HOST "www.8684.cn"
#define NEXT_LINE "\r\n"
#define GET_CMD "GET %s HTTP/1.1\r\nhost:%s\r\n\r\n"
#define END_OF_HTML "</html>\r\n"

#define MAXLINE 8192
#define MAXFILES 128 
#define SERV "80"
#define URL_LENGTH 1024
#define HOST_LENGTH 32

static struct redisContext *context;
static char setName[]="url";

struct file{
	char f_name[URL_LENGTH];	
	char f_host[HOST_LENGTH];
	int f_fd;
	int f_flags;
	pthread_t f_tid;
}file[MAXFILES];

#define F_CONNECTING 1
#define F_READING 2
#define F_DONE 4
#define F_JOINED 8

int nconn, nfiles, nlefttoconn, nlefttoread;

int ndone;
static pthread_mutex_t ndone_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ndone_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

//int tcpConnect(const char*, const char*); 
//void *do_get_read(void*);
//void write_get_cmd(struct file *);

#endif

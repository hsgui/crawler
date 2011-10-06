#include"crawler.h"
#include"queue.h"
#include"parse.h"
#include"rio.h"

char localhost[]="127.0.0.1";
int port=6379;

void *do_get_read(void*);
void home_page(char *, char*);
void write_get_cmd(struct file *);
void *thread_work(void*);

static struct redisContext *select_database(redisContext *c);
static redisContext *connectRedis(char* host, int port);
static void disconnectRedis(redisContext *c);


int main(int argc, char **argv)
{
	int			i, maxnconn;
	pthread_t	tid;
	struct file	*fptr;
	int current;
	listNode *listNode;

	context = connectRedis(localhost,port);
	queue = listCreate();
	home_page(HOST, "/");

	nlefttoread = nlefttoconn = queue->len;
	nconn = 0;
	for(i = 0; i< MAXFILES; i++)
	{
		pthread_create(&tid,NULL,thread_work,NULL);
		pthread_join(tid,NULL);
	}

	listRelease(queue);
	disconnectRedis(context);
	exit(0);
}

void print_pthread(pthread_t pt)
{
	size_t i;
	unsigned char *ptc = (unsigned char*)(void*)(&pt);
	printf("0x");
	for (i = 0; i< sizeof(pt); i++)
	{
		printf("%02x",(unsigned)(ptc[i]));
	}
	printf("\n");
}

void *thread_work(void *ptr)
{
	pthread_detach(pthread_self());
	pthread_t pid = pthread_self();
	int fd=-1, n,length;
	char line[MAXLINE];
	char current_host[HOST_LENGTH];
	char current_url[URL_LENGTH];

	struct file thread_file;
	rio_t rio;

	current_host[0]=0;
	current_url[0]=0;

	while (1)
	{
		pthread_mutex_lock(&queue_mutex);
		while (queue->len == 0)
			pthread_cond_wait(&queue_cond,&queue_mutex);
		printf("fetch a url:%s to crawl,queue size:%d, thread id:",queue->head->value,queue->len);
		print_pthread(pid);
		length = strlen(queue->head->value);
		strncpy(current_url,queue->head->value,length);
		current_url[length]=0;
		listDelNodeHead(queue);
		pthread_mutex_unlock(&queue_mutex);

		setFileNameAndHost(current_url,thread_file.f_name,thread_file.f_host);
		if (strcmp(current_host,thread_file.f_host) != 0)
		{
			close(fd);
			fd = tcpConnect(thread_file.f_host, SERV);
			thread_file.f_fd = fd;
			length = strlen(thread_file.f_host);
			strncpy(current_host,thread_file.f_host,length);
			current_host[length]=0;
		}
		write_get_cmd(&thread_file);
		rio_readinitb(&rio, fd);
		length = 0;
		while ( (n = rio_readlineb(&rio,line,sizeof(line))) > 1)
		{
			length++;
			if (strcmp(line, END_OF_HTML) == 0)
			{
				printf("fetch a url:%s is over,total lines:%d, thread_id:",current_url,length);
				print_pthread(pid);
				break;
			}
			parseUrl(line,thread_file.f_host);
		}
	}
}


void write_get_cmd(struct file *fptr)
{
	int		n;
	char	line[MAXLINE];

	n = snprintf(line, sizeof(line), GET_CMD, fptr->f_name, fptr->f_host);
	rio_writen(fptr->f_fd, line, n);
	printf("wrote %d bytes for name:%s host:%s\n", n, fptr->f_name,fptr->f_host);

	fptr->f_flags = F_READING;			/* clears F_CONNECTING */
}

void home_page(char *host, char *fname)
{
	int		fd, n;
	char	line[MAXLINE];
	redisReply *reply;
	rio_t rio;

	fd = tcpConnect(host, SERV);	/* blocking connect() */
	reply = redisCommand(context,"sadd %s %s",setName,"www.8684.cn/");

	n = snprintf(line, sizeof(line), GET_CMD, fname,host);
	rio_writen(fd, line, n);

	rio_readinitb(&rio, fd);
	while ((n = rio_readlineb(&rio, line, sizeof(line))) > 1)
	{
		if (strcmp(line,END_OF_HTML) == 0)
		{
			break;
		}
		parseUrl(line,host);
	}
	printf("end-of-file on home page\n");
	close(fd);
}

static struct redisContext *select_database(redisContext *c) {
    redisReply *reply;

    reply = redisCommand(c,"SELECT 9");
    freeReplyObject(reply);

    reply = redisCommand(c,"DBSIZE");
	if (reply == NULL)
		printf("reply is null\n");
    if (reply->type == REDIS_REPLY_INTEGER && reply->integer == 0) {
        /* Awesome, DB 9 is empty and we can continue. */
        freeReplyObject(reply);
    } else {
        printf("Database #9 is not empty, test can not continue\n");
        exit(1);
    }

    return c;
}

static redisContext *connectRedis(char* host, int port) {
    redisContext *c = NULL;

    c = redisConnect(host,port);

    if (c->err) {
        printf("Connection error: %s\n", c->errstr);
        exit(1);
    }

    return select_database(c);
}
static void disconnectRedis(redisContext *c) {
    redisReply *reply;

    /* Make sure we're on DB 9. */
    reply = redisCommand(c,"SELECT 9");
    freeReplyObject(reply);
    //reply = redisCommand(c,"FLUSHDB"); //flushdb myself
    //freeReplyObject(reply);

    /* Free the context as well. */
    redisFree(c);
}

void *do_get_read(void *vptr)
{
	int					fd, n;
	char				line[MAXLINE];
	struct file			*fptr;
	rio_t rio;

	fptr = (struct file *) vptr;

	printf("connect for name:%s host:%s\n",fptr->f_name,fptr->f_host);
	fd = tcpConnect(fptr->f_host, SERV);
	fptr->f_fd = fd;
	printf("do_get_read for name:%s host:%s, fd %d, thread %d\n",
			fptr->f_name,fptr->f_host, fd, fptr->f_tid);

	write_get_cmd(fptr);	/* write() the GET command */

	rio_readinitb(&rio,fd);
	while((n = rio_readlineb(&rio,line,sizeof(line))) > 1)
	{
		if (strcmp(line, END_OF_HTML) == 0)
		{
			break;
		}
		parseUrl(line,fptr->f_host);
	}
	printf("end-of-file on name:%s host:%s\n", fptr->f_name,fptr->f_host);
	close(fd);
	fptr->f_flags = F_DONE;		/* clears F_READING */

	pthread_mutex_lock(&ndone_mutex);
	ndone++;
	pthread_cond_signal(&ndone_cond);
	pthread_mutex_unlock(&ndone_mutex);

	return(fptr);		/* terminate thread */
}

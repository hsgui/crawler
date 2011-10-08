#include"crawler.h"
#include"queue.h"
#include"parse.h"
#include"rio.h"
#include"tcpConnect.h"

char localhost[]="127.0.0.1";
int port=6379;

void home_page(char *, char*);
void write_get_cmd(struct file *);
void *thread_work(void*);

static struct redisContext *select_database(redisContext *c);
static redisContext *connectRedis(char* host, int port);
static void disconnectRedis(redisContext *c);

int main(int argc, char **argv)
{
	int			i;

	pthread_t pids[MAXFILES];
	char logStr[LOG_LENGTH];

	context = connectRedis(localhost,port);
	queue = listCreate();
	home_page(HOST, "/");

	for(i = 0; i< MAXFILES; i++)
	{
		pthread_create(&pids[i],NULL,thread_work,NULL);
	}
	for (i=0; i< MAXFILES; i++)
	{
		pthread_join(pids[i],NULL);
		snprintf(logStr,sizeof(logStr),"the thread is over");
		logger(logStr);
	}

	listRelease(queue);
	disconnectRedis(context);
	exit(0);
}

void *thread_work(void *ptr)
{
	pthread_t pid = pthread_self();
	int fd=-1, n,length;
	char line[MAXLINE];
	char current_host[HOST_LENGTH];
	char current_url[URL_LENGTH];
	char logStr[LOG_LENGTH];

	struct file thread_file;
	rio_t rio;

	current_host[0]=0;
	current_url[0]=0;

	snprintf(logStr,sizeof(logStr),"start the thread");
	logger(logStr);

	while (1)
	{
		pthread_mutex_lock(&queue_mutex);
		while (queue->len == 0 || queue->head == NULL)
			pthread_cond_wait(&queue_cond,&queue_mutex);

		snprintf(logStr,sizeof(logStr),"fetch a url:%s to crawl, queue size:%d",queue->head->value,queue->len);
		logger(logStr);

		length = strlen(queue->head->value);
		strncpy(current_url,queue->head->value,length);
		current_url[length]=0;
		listDelNodeHead(queue);
		pthread_mutex_unlock(&queue_mutex);

		setFileNameAndHost(current_url,thread_file.f_name,thread_file.f_host);
		fd = tcpConnect(thread_file.f_host, SERV);
		if (fd == -1)
		{
			continue;
		}
		thread_file.f_fd = fd;

		write_get_cmd(&thread_file);
		rio_readinitb(&rio, fd);
		length = 0;
		while ( (n = rio_readlineb(&rio,line,sizeof(line))) > 1)
		{
			if (length == 0)
			{
				snprintf(logStr,sizeof(logStr),"first reading data from url:%s",current_url);
				logger(logStr);
			}
			length++;
			if (strcmp(line, END_OF_HTML) == 0)
			{
				break;
			}
			parseUrl(line,thread_file.f_host);
		}
		snprintf(logStr,sizeof(logStr),"crawl url:%s overs, total lines:%d",current_url,length);
		logger(logStr);

		close(fd);
	}
	pthread_exit(NULL);
}


void write_get_cmd(struct file *fptr)
{
	int		n;
	char	line[MAXLINE];
	char logStr[LOG_LENGTH];

	n = snprintf(line, sizeof(line), GET_CMD, fptr->f_name, fptr->f_host);
	rio_writen(fptr->f_fd, line, n);

	snprintf(logStr,sizeof(logStr),"writes for host:%s, name:%s",fptr->f_name,fptr->f_host);
	logger(logStr);

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

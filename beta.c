#include"crawler.h"
#include"queue.h"
#include"parse.h"
#include"rio.h"

char localhost[]="127.0.0.1";
int port=6379;

void *do_get_read(void*);
void home_page(char *, char*);
void write_get_cmd(struct file *);

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

int main(int argc, char **argv)
{
	int			i, maxnconn;
	pthread_t	tid;
	struct file	*fptr;
	int current;
	listNode *listNode;

	for (i = 0; i < MAXFILES; i++) {
		file[i].f_flags = 0;
	}
	context = connectRedis(localhost,port);
	queue = listCreate();
	home_page(HOST, "/");

	nlefttoread = nlefttoconn = queue->len;
	nconn = 0;
	
	while (nlefttoconn > 0 || nconn > 0) {
		printf("lefttoconn:%d nconn:%d\n",nlefttoconn,nconn);
		current = 0;
		while (nconn < MAXFILES && nlefttoconn > 0) {
			for (i = current ; i < MAXFILES; i++)
				if (file[i].f_flags == 0)
				{
					current = i;
					break;
				}

			pthread_mutex_lock(&queue_mutex);
			listNode = queue->head;
			if (listNode == NULL)
			{
				printf("have some but no!\n");
				exit(1);
			}
			file[i].f_flags = F_CONNECTING;
			setFileNameAndHost(listNode->value,file[i].f_name,file[i].f_host);
			listDelNodeHead(queue);
			//printList(queue);
			pthread_create(&tid, NULL, &do_get_read, &file[i]);
			file[i].f_tid = tid;
			nconn++;
			nlefttoconn = queue->len;
			pthread_mutex_unlock(&queue_mutex);
		}

		pthread_mutex_lock(&ndone_mutex);
		while (ndone == 0)
			pthread_cond_wait(&ndone_cond, &ndone_mutex);

		for (i = 0; i < MAXFILES; i++) {
			if (file[i].f_flags & F_DONE) {
				pthread_join(file[i].f_tid, (void **) &fptr);

				if (&file[i] != fptr)
				{
					printf("file[i]!=ptr\n");
					exit(1);
				}
				fptr->f_flags = 0;	/* clears F_DONE */
				ndone--;
				nconn--;
				printf("thread %d for name:%s host:%s done\n", fptr->f_tid, fptr->f_name,fptr->f_host);
			}
		}
		pthread_mutex_unlock(&ndone_mutex);
		nlefttoconn = queue->len;
	}

	listRelease(queue);
	disconnectRedis(context);
	exit(0);
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

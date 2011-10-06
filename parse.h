#include"queue.h"
#include"crawler.h"

#define OVECCOUNT 30
#define BUFLEN 1024
#define MAX_URL_LENGTH 8192
#define EBUFLEN 128

const char pattern[] = "<a\\s[^>]*?href\\s*=\\s*[\"']?([^\"'\\s>]*)[^>]*>";

int isContainHost(char *url)
{
	int i, flag = 0;
	int urllen = strlen(url);
	if (url[0] == '/')
		return 0;
	else
		return 1;
}

void setFileNameAndHost(char *url, char* name, char* host)
{
	int urllen = strlen(url);
	int i, flag=0;
	for (i = 0; i<urllen; i++)
	{
		if (flag == 0 && url[i]=='/')
		{
			flag = 1;
			break;
		}
	}
	if (flag == 0)	//no /	
	{
		name[0]='/';
		name[1]=0;
		strncpy(host, url, i);
		host[i] = 0;
	}
	else if (flag == 1 && i <= urllen-1 && i > 0)
	{
		strncpy(host, url, i);
		strncpy(name, url+i, urllen-i);
		host[i] = 0;
		name[urllen-i]=0;
	}
}

int substr(char *des, const char*str, unsigned start, unsigned end)
{
	unsigned n;
	unsigned i=start;
	char exam[1000];

	strncpy(exam,str+start,end-start);
	exam[end-start]=0;
	
	if ((str[start]=='"' && str[end]=='"') )
	{
		start++;
		end--;
		i = start;
	}
	if((i+3) <= end && str[i]=='h'&&str[i+1]=='t'&&str[i+2]=='t'&&str[i+3]=='p') 
		start += 7;
	n = end - start;
	strncpy(des, str+start, n);
	des[n] = 0;
	printf("match content:%s,and the sub url is:%s\n",exam,des);
	return 0;
}

int parseUrl(char* content, char* host)
{
	char url[MAX_URL_LENGTH];
	char* temp;
	redisReply *reply;
	pcre *re;
	const char *error;
	int erroffset;
	int ovector[OVECCOUNT];
	int rc, i;
	unsigned int offset = 0;
	size_t urllen;
	unsigned int contentlen;
	int hostlen = strlen(host);

	re = pcre_compile(pattern, 0,&error, &erroffset,NULL);
	if (re == NULL)
	{
		printf("pcre compile failed at offset %d: \n",erroffset);
		return 1;
	}

	offset = 0;
	contentlen = strlen(content);
	while (offset < contentlen && (rc = pcre_exec(re,0,content,contentlen,offset,0,ovector,sizeof(ovector))) == 2)
	{
		substr(url,content,ovector[2],ovector[3]);	//get the url
		urllen = strlen(url); //because url is just the only var,so malloc new
		if (isContainHost(url) == 0)
		{
			temp = (char*)malloc( urllen + hostlen + 1);
			strncpy(temp,host,hostlen);
			strncpy(temp + hostlen, url, urllen);
			temp[ urllen + hostlen ] = 0;
		}
		else
		{
			temp = (char*)malloc(urllen + 1);
			strncpy(temp, url, urllen);		//temp is the url,just the length of the url,not the sizeof
			temp[urllen] = 0;				//the last char is 0,for default
		}
		if (strstr(temp,"8684") != NULL)	//the url is in the 8684 domain
		{
			pthread_mutex_lock(&queue_mutex);
			reply = redisCommand(context,"sismember %s %s",setName,temp);
			if (reply->type == REDIS_REPLY_INTEGER && reply->integer == 0)
			{
				listAddNodeTail(queue, temp);	//add to the tail,always.lock
				freeReplyObject(reply);
				reply = redisCommand(context,"sadd %s %s",setName,temp);
				freeReplyObject(reply);
				printf("add new url:%s into the set,queue size:%d\n",temp,queue->len);
			}
			else if (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1)
			{
				printf("the url:%s exits in the set,queue size:%d\n",temp,queue->len);
				free(temp);
				freeReplyObject(reply);
			}
			else
			{
				free(temp);
				freeReplyObject(reply);
				printf("some error occurs!\n");
			}

			pthread_mutex_unlock(&queue_mutex);
		}
		else
		{
			printf("the url:%s is not the 8684 domain\n",temp);
			free(temp);				//is not the domain, free the array;
		}
		offset = ovector[3];
	}

	pcre_free(re);
	return 0;
}
/*
int main()
{
	char content[]="<a test=\"dfsdfd\" href=\"www.eee.com/sfdsds\">xxx</a>dfsd<a href=\"www.dfd.com\"></a><a href=\"/aas\">";
	printf("%s\n",content);
	parseUrl(content,"www.jjj.com");
}
*/

#ifndef _RIO_H_
#define _RIO_H_

#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>

#define RIO_BUFSIZE 8192

typedef struct
{
	int rio_fd;					/*descriptor for this internal buf*/
	int rio_cnt;				/*unread bytes in internal buf */
	char* rio_pbuf;				/*next unread byte in internal buf*/
	char rio_buf[RIO_BUFSIZE];	/*internal buffer */
}rio_t;

//without buffer
ssize_t rio_readn(int fd, char* usrbuf, size_t n);
ssize_t rio_writen(int fd, char* usrbuf, size_t n);

//with buffer
void rio_readinitb(rio_t *pr, int fd);
ssize_t rio_read(rio_t *pr, char *usrbuf, size_t n);
ssize_t rio_readnb(rio_t *pr,char *usrbuf, size_t n);
ssize_t rio_readlineb(rio_t *pr, char *usrbuf, size_t n);

//无缓冲输入函数
//成功返回输入的字节数，若EOF返回0，出错返回-1
ssize_t rio_readn(int fd, char *usrbuf, size_t n)
{
	size_t nleft = n;
	ssize_t nread;
	char *pbuf = usrbuf;

	while (nleft > 0)
	{
		//在某些系统中，当处理程序捕捉到一个信号时，被中断的慢系统调用（read,write,accept)
		//在信号处理程序返回时不再继续，而是立即返回给客户一个错误条件，并将errno设置为EINTR
		if ((nread = read(fd, pbuf,nleft)) < 0)
		{
			if (errno == EINTR)
				nread = 0;	//中断造成的，再次调用read
			else
				return -1;	//出错
		}
		else if (nread == 0)	//到了文件末尾
			break;
		nleft -= nread;
		pbuf += nread;
	}
	return (n - nleft);
}

ssize_t rio_writen(int fd, char* usrbuf, size_t n)
{
	size_t nleft = n;
	ssize_t nwritten;
	char *pbuf = usrbuf;

	while (nleft > 0)
	{
		if ((nwritten = write(fd, pbuf, nleft)) <= 0)
		{
			if (errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}
		nleft -= nwritten;
		pbuf += nwritten;
	}
	return n;
}

//初始化rio_t结构
void rio_readinitb(rio_t *pr, int fd)
{
	pr->rio_fd = fd;
	pr->rio_cnt = 0;
	pr->rio_pbuf = pr->rio_buf;
}
//带缓存函数，供内部调用
ssize_t rio_read(rio_t *pr, char *usrbuf,size_t n)
{
	int cnt;
	while (pr->rio_cnt <= 0)	//内部缓冲空了，进行重填
	{
		pr->rio_cnt = read(pr->rio_fd, pr->rio_buf,sizeof(pr->rio_buf));	//读到内部缓冲区中
		if(pr->rio_cnt < 0)
		{
			if(errno != EINTR)	//出错返回
			{
				return -1;
			}
		}
		else if (pr->rio_cnt == 0)	//到文件末尾
		{
			return 0;
		}
		else
		{
			pr->rio_pbuf = pr->rio_buf;	//重置指针位置
		}
	}
	//从内部缓冲区拷贝到用户缓冲区中
	cnt = (pr->rio_cnt < n)?pr->rio_cnt: n;
	memcpy(usrbuf, pr->rio_pbuf,cnt);
	pr->rio_pbuf += cnt;
	pr->rio_cnt -= cnt;
	return cnt;
}

//带缓冲输入函数
ssize_t rio_readnb(rio_t *pr, char *usrbuf, size_t n)
{
	size_t nleft = n;
	size_t nread;
	char *pbuf = usrbuf;
	while (nleft > 0)
	{
		//与无缓冲的区别在于这里调用的是rio_read而不是read
		if ((nread = rio_read(pr,pbuf,nleft)) < 0)
		{
			if (errno == EINTR)
				nread = 0;		//中断造成的，再次调用read
			else
				return -1;		//出错
		}
		else if (nread == 0)	//到了文件尾
		{
			break;
		}
		nleft -= nread;
		pbuf += nread;
	}
	return n-nleft;
}

//带缓冲输入函数，每次输入一行
ssize_t rio_readlineb(rio_t *pr, char *usrbuf, size_t maxlen)
{
	int i,n;
	char c, *pbuf = usrbuf;
	for (i=1; i<maxlen; i++)
	{
		if ((n = rio_read(pr,&c,1)) == 1)	//读一个字符
		{
			*pbuf++ = c;
			if (c == '\n')	//读到换行符
				break;
		}
		else if(n == 0)
		{
			if (i == 1)	//空文件
				return 0;
			else		//读到了部分数据
				break;
		}
		else
			return -1;
	}

	*pbuf = 0;		//添加字符串结束符
	return i;
}
#endif

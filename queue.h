#ifndef _QUEUE_H_
#define _QUEUE_H_

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>

typedef struct listNode{
	struct listNode *next;
	char *value;
}listNode;

typedef struct list{
	listNode *head;
	listNode *tail;
	unsigned int len;
}list;

static list *queue;

#define listLength(l) ((l)->len)
#define listFirst(l) ((l)->head)
#define listLast(l) ((l)->tail)

list *listCreate(void)
{
	struct list *list;
	if ((list = malloc(sizeof(*list))) == NULL)
		return NULL;
	list->head = list->tail = NULL;
	list->len = 0;

	return list;
}

void listRelease(list *list)
{
	unsigned int len;
	listNode *current, *next;
	current = list->head;
	len = list->len;
	while (len--)
	{
		next = current->next;
		free(current->value);
		free(current);
		current = next;
	}
	free(list);
}

list *listAddNodeTail(list *list, char *value)
{
	listNode *node;
	if ((node = malloc(sizeof(*node))) == NULL)
		return NULL;
	node->value = value;
	if (list->len == 0)
	{
		list->head = list->tail = node;
		node->next = NULL;
	}
	else 
	{
		node->next = NULL;
		list->tail->next = node;
		list->tail = node;
	}
	list->len++;
	return list;
}

void listDelNodeHead(list *list)
{
	listNode *oldHead;
	oldHead = list->head;
	list->head = oldHead->next;
	if (list->head == NULL)
		list->tail = NULL;
	free(oldHead->value);
	free(oldHead);
	list->len--;
}

void printList(list *list)
{
	listNode *current;
	unsigned int len;

	current = list->head;
	len = list->len;
	printf("the queue contains %d nodes:\n",len);
	while ( len--)
	{
		printf("%s\n",current->value);
		current = current->next;
	}
}
#endif

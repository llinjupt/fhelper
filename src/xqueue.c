#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "xqueue.h"

static unsigned int __xqueue_enqueue(xqueue_t *head, xqnode_t *qnode)
{
  xlist_add_tail((struct xlist_head *)qnode, &head->node);
  head->qnode_num++;
  
  return head->qnode_num;
}

xqnode_t *xqueue_dequeue(xqueue_t *head)
{
  if(head->qnode_num == 0)
    return NULL;
  
  xqnode_t *node = (xqnode_t *)xlist_get(&head->node);  
  if(node)
    head->qnode_num--;    
  
  return node;
}

void xqueue_traverse(xqueue_t *head, xqueue_traverse_f handle)
{
  xqnode_t *tmp;
  
  assert(head != NULL && handle);
  xlist_for_each_entry(tmp, &head->node, node)
  {
    handle(tmp->data);
  }
}

void xqueue_traverse_fromto(xqueue_t *head, xqueue_traverse_f handle, 
                            int from_idx, int to_idx)
{
  int idx = 0;
  xqnode_t *tmp;
  
  assert(head != NULL && handle);
  xlist_for_each_entry(tmp, &head->node, node)
  {
    if(idx >= from_idx && idx <= to_idx)
      handle(tmp->data);
    
    idx++;
  }
}

/* if size is 0 then no limit, -1 with default */
xqueue_t *xqueue_create(int size, xqueue_free_f free)
{
  xqueue_t *head = malloc(sizeof(xqueue_t));
  if(!head)
  {
    perror("malloc");
    return NULL;
  }
  
  memset(head, 0, sizeof(xqueue_t));
  INIT_XLIST_HEAD(&head->node);  
  
  if(size < 0)
    head->max_qnode_num = AQUEUE_MAX_NODES;
  else  
    head->max_qnode_num = size;
    
  head->qnode_num = 0;
  head->free = free;
  
  return head;
}

int xqueue_enqueue(xqueue_t *head, void *data)
{ 
  xqnode_t *qnode = NULL;
  if(head->max_qnode_num 
    && head->qnode_num >= head->max_qnode_num)
    return -1;
  
  /* alloc xqueue node */
  if((qnode = (xqnode_t *)malloc(sizeof(xqnode_t)))==NULL)
  {
    perror("malloc");
    return -1;
  }
  
  memset(qnode, 0, sizeof(xqnode_t));    
  qnode->data = data;

  /* push request into ipc queue */  
  return __xqueue_enqueue(head, qnode);
}

void xqueue_handle(xqueue_t *head, xqueue_handle_f handle)
{  
  xqnode_t *node = NULL;
  
  assert(handle != NULL);
  
  while((node = xqueue_dequeue(head)) != NULL)
  {
    handle(node->data);
    if(head->free)
      head->free(node->data);
    free(node);
  }
}

void xqueue_flush(xqueue_t *head)
{
  xqnode_t *node = NULL;
  
  if(!head)
    return;
  
  while((node = xqueue_dequeue(head)) != NULL)
  {
    if(head->free)
      head->free(node->data);
    free(node);
  }
}

void xqueue_destroy(xqueue_t *head)
{
  if(!head)
    return;
  
  xqueue_flush(head);
  free(head);
}

unsigned int xqueue_nodes(xqueue_t *head)
{
  assert(head);
  
  return head->qnode_num;
}

#define TEST 1
#ifdef TEST
static void dump_string(void *str)
{
  if(!str)
    return;
    
  printf("%s\n", (char *)str);
}

void test_queue()
{
  xqueue_t *queue = xqueue_create(0, free);
  xqueue_enqueue(queue, strdup("hello"));
  printf("xqueue_nodes %d\n", xqueue_nodes(queue));
  
  xqueue_enqueue(queue, strdup("world"));
  printf("xqueue_nodes %d\n", xqueue_nodes(queue));
  
  /* call the dump hook function to work on every node */
  xqueue_traverse(queue, dump_string);
  
  /* destroy the queue */
  xqueue_destroy(queue);
}

#endif

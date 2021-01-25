/*
 * Fhelper, powered by Eastforest Co., Ltd 
 *
 * Copyright (C) 2018-2021 Reid Liu  <lli_njupt@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
 
#ifndef XQUEUE_H
#define XQUEUE_H

#include "xlist.h"

#define AQUEUE_MAX_NODES 1024

typedef void (*xqueue_dump_f)(void *data);
typedef void (*xqueue_traverse_f)(void *data);
typedef void (*xqueue_free_f)(void *data);
typedef void (*xqueue_handle_f)(void *data);

/* asynchronous queue node */
typedef struct
{
  struct xlist_head node;
  void *data;
}xqnode_t;

/* asynchronous queue header */
typedef struct
{
  struct xlist_head node;  
  
  unsigned int qnode_num;    /* current nodes */
  unsigned int max_qnode_num;
  
  xqueue_traverse_f traverse;
  xqueue_free_f free;
}xqueue_t;

/* if size is 0 then no limit, -1 with default */
xqueue_t *xqueue_create(int size, xqueue_free_f free);
void xqueue_traverse(xqueue_t *head, xqueue_traverse_f traverse);
void xqueue_traverse_fromto(xqueue_t *head, xqueue_traverse_f traverse, int from_idx, int to_idx);

void xqueue_dump(xqueue_t *head);

int xqueue_enqueue(xqueue_t *head, void *data);
void xqueue_destroy(xqueue_t *head);

/* flush away all data nodes */
void xqueue_flush(xqueue_t *head);

unsigned int xqueue_nodes(xqueue_t *head);

void xqueue_handle(xqueue_t *head, xqueue_handle_f handle);

#endif /* XQUEUE_H */

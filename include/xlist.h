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

/* based on linux kernel list implementation */

#ifndef _XLIST_H
#define _XLIST_H

#include <stdio.h>

#ifndef LIST_POISON1
#define LIST_POISON1  NULL
#define LIST_POISON2  NULL
#endif

/*
 * Simple doubly linked list implementation.
 * Stolen from Linux kernel list.h file
 */

struct xlist_head {
  struct xlist_head *next, *prev;
};

#define XLIST_HEAD_INIT(name) { &(name), &(name) }

#define XLIST_HEAD(name) \
  struct xlist_head name = XLIST_HEAD_INIT(name)

static inline void INIT_XLIST_HEAD(struct xlist_head *list)
{
  list->next = list;
  list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */

static inline void __xlist_add(struct xlist_head *new,
            struct xlist_head *prev,
            struct xlist_head *next)
{
  next->prev = new;
  new->next = next;
  new->prev = prev;
  prev->next = new;
}


/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void xlist_add(struct xlist_head *new, struct xlist_head *head)
{
  __xlist_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void xlist_add_tail(struct xlist_head *new, struct xlist_head *head)
{
  __xlist_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __xlist_del(struct xlist_head * prev, struct xlist_head * next)
{
  next->prev = prev;
  prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */

static inline void xlist_del(struct xlist_head *entry)
{
  __xlist_del(entry->prev, entry->next);
  entry->next = LIST_POISON1;
  entry->prev = LIST_POISON2;
}

#if 0

/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void xlist_replace(struct xlist_head *old,
        struct xlist_head *new)
{
  new->next = old->next;
  new->next->prev = new;
  new->prev = old->prev;
  new->prev->next = new;
}

static inline void xlist_replace_init(struct xlist_head *old,
          struct xlist_head *new)
{
  xlist_replace(old, new);
  INIT_XLIST_HEAD(old);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void xlist_del_init(struct xlist_head *entry)
{
  __xlist_del(entry->prev, entry->next);
  INIT_XLIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void xlist_move(struct xlist_head *list, struct xlist_head *head)
{
  __xlist_del(list->prev, list->next);
  xlist_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void xlist_replace_tail(struct xlist_head *list,
          struct xlist_head *head)
{
  __xlist_del(list->prev, list->next);
  xlist_add_tail(list, head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int xlist_is_last(const struct xlist_head *list,
        const struct xlist_head *head)
{
  return list->next == head;
}

#endif

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int xlist_empty(const struct xlist_head *head)
{
  return head->next == head;
}

#if 1

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 */
static inline int xlist_empty_careful(const struct xlist_head *head)
{
  struct xlist_head *next = head->next;
  return (next == head) && (next == head->prev);
}

/**
 * list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int xlist_is_singular(const struct xlist_head *head)
{
  return !xlist_empty(head) && (head->next == head->prev);
}

static inline void __xlist_cut_position(struct xlist_head *list,
    struct xlist_head *head, struct xlist_head *entry)
{
  struct xlist_head *new_first = entry->next;
  list->next = head->next;
  list->next->prev = list;
  list->prev = entry;
  entry->next = list;
  head->next = new_first;
  new_first->prev = head;
}

/**
 * list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *  and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void xlist_cut_position(struct xlist_head *list,
    struct xlist_head *head, struct xlist_head *entry)
{
  if (xlist_empty(head))
    return;
  if (xlist_is_singular(head) &&
    (head->next != entry && head != entry))
    return;
  if (entry == head)
    INIT_XLIST_HEAD(list);
  else
    __xlist_cut_position(list, head, entry);
}

static inline void __xlist_splice(const struct xlist_head *list,
         struct xlist_head *prev,
         struct xlist_head *next)
{
  struct xlist_head *first = list->next;
  struct xlist_head *last = list->prev;

  first->prev = prev;
  prev->next = first;

  last->next = next;
  next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void xlist_splice(const struct xlist_head *list,
        struct xlist_head *head)
{
  if (!xlist_empty(list))
    __xlist_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void xlist_splice_tail(struct xlist_head *list,
        struct xlist_head *head)
{
  if (!xlist_empty(list))
    __xlist_splice(list, head->prev, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void xlist_splice_init(struct xlist_head *list,
            struct xlist_head *head)
{
  if (!xlist_empty(list)) {
    __xlist_splice(list, head, head->next);
    INIT_XLIST_HEAD(list);
  }
}

/**
 * list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void xlist_splice_tail_init(struct xlist_head *list,
           struct xlist_head *head)
{
  if (!xlist_empty(list)) {
    __xlist_splice(list, head->prev, head);
    INIT_XLIST_HEAD(list);
  }
}

#endif

#ifndef offset_of
#define offset_of(type, memb) \
  ((unsigned long)(&((type *)0)->memb))
#endif
#ifndef container_of
#define container_of(obj, type, memb) \
  ((type *)(((char *)obj) - offset_of(type, memb)))
#endif

/**
 * list_entry - get the struct for this entry
 * @ptr:  the &struct list_head pointer.
 * @type:  the type of the struct this is embedded in.
 * @member:  the name of the list_struct within the struct.
 */
#define xlist_entry(ptr, type, member) \
  container_of(ptr, type, member)

#ifndef ARCH_HAS_PREFETCH
#define prefetch(x) __builtin_prefetch(x)
#endif

/**
 * list_for_each  -  iterate over a list
 * @pos:  the &struct list_head to use as a loop cursor.
 * @head:  the head for your list.
 */
#define xlist_for_each(pos, head) \
  for (pos = (head)->next; prefetch(pos->next), pos != (head); \
          pos = pos->next)

/**
 * list_for_each_entry  -  iterate over list of given type
 * @pos:  the type * to use as a loop cursor.
 * @head:  the head for your list.
 * @member:  the name of the list_struct within the struct.
 */
#define xlist_for_each_entry(pos, head, member)        \
  for (pos = xlist_entry((head)->next, typeof(*pos), member);  \
       prefetch(pos->member.next), &pos->member != (head);   \
       pos = xlist_entry(pos->member.next, typeof(*pos), member))

extern struct xlist_head *xlist_get(struct xlist_head *head);

#endif

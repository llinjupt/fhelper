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
 
/* based on xen/libxc/freearray.c and modified */

#ifndef XARRAY_H
#define XARRAY_H

typedef struct xarray
{
  /* size of the arrary */
  unsigned int size;
  
  /* appendable: 0 or 1 */
  unsigned char append_enable;
  
  /* current nodes */
  unsigned int count;
  
  /* function to realse a node data */
  void (*free_f)(void *data);
  
  /* compare the node */
  int (*cmp_f)(void *data1, void *data2);
  
  /* dump a node */
  void (*dump_f)(void *data);
  
  /* real data node array hook */
  void **data; 
} xarray_t;

xarray_t *xarray_create(unsigned int size, unsigned char append_enable, 
                        void (*free_f)(void *data),
                        int (*cmp_f)(void *data1, void *data2),
                        void (*dump_f)(void *data));
void xarray_destroy(xarray_t *array);

int xarray_append(xarray_t *array, int extents);
int xarray_fset_index(xarray_t *array, unsigned int index, void *ptr);
int xarray_set_index(xarray_t *array, unsigned int index, void *ptr);

int xarray_set(xarray_t *array, void *ptr);
int xarray_vset(xarray_t *array, ...);

void *xarray_get(xarray_t *array, int index);
void *xarray_remove_index(xarray_t *array, int index);
void *xarray_remove(xarray_t *array);
int xarray_remove_data(xarray_t *array, void *data);

void *xarray_find(xarray_t *array, void *data);
int xarray_find_index(xarray_t *array, void *data);
void *xarray_find_byfunc(xarray_t *array, void *args, 
                         int (*cmp_f)(void *data, void *args));
void xarray_dump(xarray_t *array);

void xarray_traverse(xarray_t *array, void (*func)(void *));
void xarray_traverse2(xarray_t *array, void *para, 
                      void (*func)(void *node, void *para));

unsigned int xarray_getcount(xarray_t *array);
void **xarray_contents(xarray_t *array);

/**** below is a sample for str array ****/

/* create a new str array */
#define XSTRARRAY_SPLIT     '\3'
#define XSTRARRAY_SPLIT_STR "\3"

xarray_t *xstrarray();
int xstrarrayadd(xarray_t *array, const char *str);
char *xarray2str(xarray_t *array, const char split);

/* dump [from, to] items into a string */
char *xarray2str_fromto_index(xarray_t *array, int from_index, 
                              int to_index, const char split);
char *xarray2str_from_index(xarray_t *array, int from_index, const char split);
xarray_t *xstr2array(const char *instr, const char *split);

#endif /* XARRAY_H */

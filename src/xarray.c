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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <alloca.h>
#include <assert.h>

#include "xarray.h"

/****************************************************************************
 *desc :  create the array
 *input¡êo size, orignial size
          append_enable: appendable 0, no; !0 yes
****************************************************************************/
xarray_t *xarray_create(unsigned int size, unsigned char append_enable,
                        void (*free_f)(void *data),
                        int (*cmp_f)(void *data1, void *data2),
                        void (*dump_f)(void *data))
{
  assert(size > 0);

  xarray_t *array = malloc(sizeof(struct xarray));
  if(array == NULL)
  {
    perror("malloc array");
    return NULL;
  }

  array->size = size;
  array->append_enable = append_enable;
  array->count = 0;
  array->data = calloc(size, sizeof(void *));
  
  array->free_f = free_f;
  array->cmp_f = cmp_f;
  array->dump_f = dump_f;

  return array;
}

void xarray_dump(xarray_t *array)
{
  int i = 0;
  
  if(array == NULL)
  {
    printf("array is NULL!\n");
    return;
  }
  
  if(array->dump_f == NULL)
  {
    printf("array's dump func is NULL!\n");
    return;
  }
    
  for(; i < array->count; i++)
  {
    printf("slot[%d]:\n", i);
    array->dump_f(array->data[i]);
    printf("\n");
  }
}

void xarray_traverse(xarray_t *array, void (*func)(void *))
{
  int i = 0;
  
  if(array == NULL)
  {
    printf("array is NULL!\n");
    return;
  }

  for(; i < array->count; i++)
    func(array->data[i]);
}

/* traverse func with another parameter */
void xarray_traverse2(xarray_t *array, void *para, void (*func)(void *node, void *para))
{
  int i = 0;
  
  if(array == NULL)
  {
    printf("array is NULL!\n");
    return;
  }

  for(; i < array->count; i++)
    func(array->data[i], para);
}

/* release an xarray_t object */
void xarray_destroy(xarray_t *array)
{
  int i = 0;
  
  if(array == NULL)
    return;
  
  if(array->free_f)
  {
    for(; i < array->count; i++)
      array->free_f(array->data[i]);
  }
  
  free(array->data);
  free(array);
}

/****************************************************************************
 *desc :  append the array size
 *return: -1 failed, >=0 return the new size of the array
****************************************************************************/
int xarray_append(xarray_t *array, int extents)
{
  void **data;
  int newsize;
  
  newsize = array->size + extents;
  if(newsize <= 0)
  {
    array->size = 0;
    array->count = 0;
    free(array->data);
    array->data = NULL;
    
    return 0;
  }
  
  data = realloc(array->data, sizeof(void *) * newsize);
  if(data == NULL)
  {
    perror("realloc array");
    return -1;
  }
  
  array->data = data;
  if(extents > 0)
    memset(&array->data[array->size], 0, sizeof(void *) * extents);

  array->size = newsize;
  
  return newsize;
}

/****************************************************************************
 *desc :  set [index] point to as ptr
 *input¡êo array 
          index of the array
          ptr   new data 
          force if [index] is not null, still replace it with ptr

 *output: None
 *return: -1 failed, 0 ok. Make sure the [index] is null
****************************************************************************/
static int __xarray_set_index(xarray_t *array, unsigned int index,
                              void *ptr, int force)
{
  if(index < array->size)
  {
    if(array->data[index] != NULL
      && force == 0)
    {
      printf("Error: slot(%d) is not empty!\n", index);
      return -1;
    }
  }
  else
  {
    int newsize = 0;
    if(array->append_enable == 0)
    {
      printf("Error: index %d is too big but array not support append_enable!\n", index);
      return -1;
    }
    
    newsize = (array->size * 2 < index) ? index + 1 : array->size * 2;
    if(xarray_append(array, newsize - array->size) < 0)
      return -1;
  }
  
  if(index + 1 > array->count)
    array->count = index + 1;

  array->data[index] = ptr;

  return 0;
}

/****************************************************************************
 *desc :  set [index] = ptr forcedly
 *return: -1 failed, 0 ok.
****************************************************************************/
int xarray_fset_index(xarray_t *array, unsigned int index, void *ptr)
{
  return __xarray_set_index(array, index, ptr, 1);
}

/* unlike xarray_fset_index, don't set [index] forcedly */
int xarray_set_index(xarray_t *array, unsigned int index, void *ptr)
{
  return __xarray_set_index(array, index, ptr, 0);
}

/****************************************************************************
 *desc :  set data to the last index          
****************************************************************************/
int xarray_set(xarray_t *array, void *ptr)
{
  return xarray_set_index(array, array->count, ptr);
}

/****************************************************************************
 *desc :  add mulplite elements
  example: xarray_vset(new, strdup("world"), NULL);

 *return: the number of new added elements
****************************************************************************/
int xarray_vset(xarray_t *array, ...)
{
  va_list va;
  void *ptr;
  int ret;

  va_start(va, array);

  for(ret = 0; (ptr = va_arg(va, void *)); ret++)
  {
    if(xarray_set(array, ptr))
      break;
  }
  
  va_end(va);
  return ret;
}

void *xarray_find(xarray_t *array, void *data)
{
  int i = 0;
  
  if(array == NULL || data == NULL)
    return NULL;
  
  if(array->cmp_f)
  {
    for(; i < array->count; i++)
    {
      if(array->cmp_f(array->data[i], data) == 0)
        return array->data[i];
    }
  }

  return NULL;
}

void *xarray_find_byfunc(xarray_t *array, void *args, 
                         int (*cmp_f)(void *data, void *args))
{ 
  int i = 0;
    
  if(array == NULL || cmp_f == NULL)
    return NULL;
  
  for(; i < array->count; i++)
  {
    if(cmp_f(array->data[i], args) == 0)
      return array->data[i];
  }

  return NULL;
}

int xarray_find_index(xarray_t *array, void *data)
{
  int i = 0;
  
  if(array == NULL || data == NULL)
    return -1;

  if(array->cmp_f)
  {
    for(; i < array->count; i++)
    {
      if(array->cmp_f(array->data[i], data) == 0)
        return i;
    }
  }
  
  return -1;
}

/****************************************************************************
 *desc :  remove data from [index] and return it
 *input¡êo array
          index
          
 *output: None
 *return: return the deleted data
****************************************************************************/
void *xarray_remove_index(xarray_t *array, int index)
{
  void *ret = NULL;
  
  if(index >= array->size)
    return NULL;
  
  ret = array->data[index];
  array->data[index] = NULL;
  
  /* removed is at last */
  if(index == array->count - 1)
  {
    array->data[index] = NULL;
    array->count--;
  }
  else /* use memmove to keep the order */
  {
    memmove(array->data + index, array->data + index + 1, 
            (array->count - index - 1) * sizeof(void *));
    array->count--;
    array->data[array->count] = NULL;
  }

  if(array->append_enable
     && array->count <= array->size / 2)
    xarray_append(array, -array->count);  
  
  return ret;
}

int xarray_remove_data(xarray_t *array, void *data)
{
  int i = 0, index = 0;
  void *ret = NULL;

  if(array == NULL || data == NULL)
    return -1;

  if(index >= array->size)
    return -1;
  
  if(array->cmp_f)
  {
    for(i = 0; i < array->count; i++)
    {
      if(array->cmp_f(array->data[i], data) != 0)
        continue;
      
      ret = array->data[i];
      index = i;
      break;
    }
  }
  
  if(ret == NULL)
    return -1;

  /* removed is at last */
  if(index == array->count - 1)
  {
    array->data[index] = NULL;
    array->count--;
  }
  else /* use memmove to keep the order */
  {
    memmove(array->data + index, array->data + index + 1, 
            (array->count - index - 1) * sizeof(void *));
    array->count--;
    array->data[array->count] = NULL;
  }

  if(array->append_enable
     && array->count <= array->size / 2)
    xarray_append(array, -array->count);
  
  if(array->free_f)
    array->free_f(ret);

  return 0;
}

/****************************************************************************
 *desc :  remove the last !null data
 *input¡êo array

 *output: None
 *return: the pointer of the data
****************************************************************************/
void *xarray_remove(xarray_t *array)
{
  return xarray_remove_index(array, array->count - 1);
}

/****************************************************************************
 *desc :  return [index]
 *input¡êo array 
          index 
      
 *output: None
 *return: return [index]
****************************************************************************/
void *xarray_get(xarray_t *array, int index)
{ 
  if(index >= array->size)
    return NULL;

  return array->data[index];
}

/****************************************************************************
 *desc :  return the data array and release array
 *input¡êo array

 *output: None
 *return: the data array pointer
****************************************************************************/
void **xarray_contents(xarray_t *array)
{
  void **data;

  if(array == NULL)
    return NULL;
  
  data = array->data;
  free(array);
  
  return data;
}

unsigned int xarray_getcount(xarray_t *array)
{
  if(array == NULL)
    return 0;

  return array->count;
}

/* below is a sample for str array */
void strarray_dump(void *data)
{
  printf("%s", data ? (char *)data : "NULL");
}

static int strarray_cmp(void *a, void *b)
{
  return strcmp((char *)a, (char *)b);
}

static void strarray_free(void *data)
{
  if(data)
    free(data);
}

/* don't forget to free the returned str */
char *xarray2str(xarray_t *array, const char split)
{
  /* -1 means to the end */
  return xarray2str_fromto_index(array, 0, -1, split);
}

char *xarray2str_from_index(xarray_t *array, int from_index, const char split)
{
  return xarray2str_fromto_index(array, from_index, -1, split);
}

char *xarray2str_fromto_index(xarray_t *array, int from_index, int to_index, const char split)
{
  int i = 0;
  int len = 0, offset = 0;
  int size = 256;
  int to = 0;
  
  char *str = NULL;
  
  if(array == NULL)
    return NULL;
  
  if(from_index >= array->count)
    return NULL;
  
  to = array->count;
  if(to_index > 0 
     && to_index < array->count)
    to = to_index + 1;

  str = malloc(size);
  memset(str, 0, size);
  if(!str)
  {
    perror("strarray2str malloc");
    return NULL;
  }

  for(i = from_index; i < to; i++)
  {
    char *tmp = (char *)array->data[i];

    if(tmp == NULL 
      || !(len = strlen(tmp)))
      continue;
          
    if(size - offset <= len - 1)
    {
      char *data = NULL;
      
      if(size <= len - 1)
        size = size + len + 1;
      else
        size = size * 2;

      data = realloc(str, size);
      if(data == NULL)
      {
        perror("realloc array");
        free(str);
        return NULL;
      }
      
      str = data;
    }
    
    sprintf(str + offset, "%s%c", tmp, split);
    offset = offset + len + 1;
  }
  
  if(offset)
    str[offset - 1] = '\0';
  
  return str;
}

xarray_t *xstrarray()
{
  return xarray_create(8, 1, strarray_free, strarray_cmp, strarray_dump);
}

int xstrarrayadd(xarray_t *array, const char *str)
{
  char *tmp = NULL;

  if(!str || !array)
    return -1;

  tmp = strdup(str);
  if(!tmp)
    return -1;
  
  if(xarray_vset(array, tmp, NULL) == 1)
    return 0;

  free(tmp); 
  return -1;
}

xarray_t *xstr2array(const char *instr, const char *split)
{
  int len = 0;
  int i = 0, j = 0;

  char *tmp = NULL;
  xarray_t *_new = NULL;

  len = strlen(instr);
  if(len == 0)
    return NULL;

  tmp = strdup(instr); 
  if(tmp == NULL)
  {
    perror("xtr2array strdup");
    return NULL;
  }
  
  _new = xarray_create(8, 1, strarray_free, strarray_cmp, strarray_dump);
  if(_new == NULL)
  {
    printf("xarray_create error\n");
    goto err;
  }

  for(; i <= len; i++)
  {
    if(strchr(split, instr[i]) == NULL 
      && instr[i] != '\0')
    {
      tmp[j++] = instr[i]; 
    }
    else
    {
      char *newstr = NULL;
      
      tmp[j] = '\0';
      j = 0;
      
      newstr = strdup(tmp);
      if(newstr == NULL 
        || xarray_vset(_new, newstr, NULL) != 1)
        goto err;
    }
  }
  free(tmp);
  return _new;

err:
  free(tmp);
  xarray_destroy(_new);
  
  return NULL;
}

#ifdef TEST
void test_xarray()
{
  int ret = 0;
  
  xarray_t *str_array = xstr2array("0-1-2-3-4-5-6", "-");
  printf("count :%d\n", xarray_getcount(str_array));
  xarray_dump(str_array);
  
  char *str = xarray2str_fromto_index(str_array, 1, 1, ' ');
  printf(str); 
  
  char *removed = xarray_remove_index(str_array, 2);
  printf("\nafter remove index [2]:\n");
  printf("count :%d\n", xarray_getcount(str_array));
  xarray_dump(str_array);
  strarray_free(removed);
  
  removed = xarray_remove(str_array);
  printf("\nafter remove the last index:\n");
  printf("count :%d\n", xarray_getcount(str_array));
  xarray_dump(str_array);
  strarray_free(removed);

  ret = xarray_remove_data(str_array, "0");
  if(ret == 0)
  {
    printf("\nafter remove string \"0\":\n");
    printf("count :%d\n", xarray_getcount(str_array));
    xarray_dump(str_array);
  }
  else
    printf("\nremove string \"4\" failed!\n");

  xarray_vset(str_array, strdup("hello"), NULL);
  printf("\nafter add string \"hello\":\n");
  printf("count :%d\n", xarray_getcount(str_array));
  xarray_dump(str_array);

  xarray_destroy(str_array);
}
#endif

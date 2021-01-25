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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>

/* for mkfifo */
#include <sys/types.h>
#include <sys/stat.h>
/* for file open */
#include <fcntl.h>

/* see /usr/include/unistd.h 
 * Standard file descriptors.
 */ 
//#define STDIN_FILENO  0 /* Standard input. */
//#define STDOUT_FILENO 1 /* Standard output. */
//#define STDERR_FILENO 2 /* Standard error output. */

#include "xdebug.h"
#include "xarray.h"
#include "xqueue.h"
#include "terminal.h"

#define RECV_BUFSIZE 1024
#define FHELPER_PIPE "/tmp/fhelper"

static int fhelper_pipe_create()
{
  int fd = 0;

  unlink(FHELPER_PIPE);
  if(mkfifo(FHELPER_PIPE, 0644) < 0)
  {
    perror("mkfifo\n");
    return -1;
  }

  fd = open(FHELPER_PIPE, O_RDWR);
  if(fd < 0)
  {
    perror("open");
    return -1;
  }

  return fd;
}

static void fhelper_pipe_close(int fd)
{
  if(fd > 0)
    close(fd);

  unlink(FHELPER_PIPE);  
}

/* add sockets[] with size sockets to fdset 
 * Note: fdset will be inited firstly. Return the maxfd
 */
int xfdset(fd_set *fds, int sockets[], int size)
{
  int i = 0, maxfd = 0;

  if(size <= 0)
    return -1;

  FD_ZERO(fds);

  for(; i < size; i++)
  {
    if(sockets[i] < 0)
    {
      printf("Wrong sockets\n");
      return -1;
    }

    if(sockets[i] > maxfd)
      maxfd = sockets[i];

    FD_SET(sockets[i], fds);
  }

  return maxfd;
}

ssize_t safe_read(int fd, void *buf, size_t count)
{
  ssize_t n;
  
  do 
  {
    n = read(fd, buf, count);
  }while (n < 0 && errno == EINTR);
  
  return n;
}

static int g_auto_refresh = 1;
#if 0
static void auto_refresh_set(int yes)
{
  g_auto_refresh = yes;
}
#endif

static void auto_refresh_reverse()
{
  g_auto_refresh = !g_auto_refresh;
}

static int auto_refresh_get()
{
  return g_auto_refresh;
}

static void fhelper_logo()
{
  printf(
      "    ________         __               \n"
      "   / ____/ /_  ___  / /___  ___  _____\n"
      "  / /_  / __ \\/ _ \\/ / __ \\/ _ \\/ ___/\n"
      " / __/ / / / /  __/ / /_/ /  __/ /    \n"
      "/_/   /_/ /_/\\___/_/ .___/\\___/_/     \n"
      "                  /_/                 \n"
      "    Powered by Eastforest Co., Ltd    \n");
  printf(
      " Generated at %s v%s\n\n", MAKE_TIME, VERSION);
}   
      
static void usage(void)
{
  printf("Usage:\n"
          "\n"
          "  fhelper <options>\n"
          "\n"
          "where options may include:\n"
          "\n"
          "  --help -h        to output this message.\n"
          "  D or D           refresh the screen.\n"
          "  S or s           enable or disable refresh .\n"
          "  Arrows/pagedn/up scroll the list.\n"
          "  Q or q           quit.\n"
          
          );
}

/* need to free the returned string */
static char *fhelper_shrink_path(const char *path)
{
  int i = 0;
  char *newpath = NULL;
  
  if(!path)
    return NULL;

#define SHOW_LAST_PATH_PARTS (2)  
  xarray_t *paths = xstr2array(path, "/");
  int path_parts = xarray_getcount(paths);
  i = path_parts - SHOW_LAST_PATH_PARTS - 1;
  if(i < 0)
    i = 0;
  
  newpath = xarray2str_from_index(paths, i, '/');
    
  xarray_destroy(paths);
  return newpath;
}

/* It is perfectly ok to pass in a NULL for either width or for
 * height, in which case that value will not be set.  */
int get_terminal_width_height(int fd, int *width, int *height)
{
  struct winsize win = { 0, 0, 0, 0 };
  int ret = ioctl(fd, TIOCGWINSZ, &win);
  
  if (height)
  {
    if (!win.ws_row) 
    {
      char *s = getenv("LINES");
      if (s) win.ws_row = atoi(s);
    }
    if (win.ws_row <= 1 || win.ws_row >= 30000)
      win.ws_row = 24;
    *height = (int) win.ws_row;
  }
  
  if (width) 
  {
    if (!win.ws_col) 
    {
      char *s = getenv("COLUMNS");
      if (s) win.ws_col = atoi(s);
    }
    if (win.ws_col <= 1 || win.ws_col >= 30000)
            win.ws_col = 80;
    *width = (int) win.ws_col;
  }
  
  return ret;
}

typedef enum
{
  INFO_TYPE_ERROR,
  INFO_TYPE_WARN,
  INFO_TYPE_NOTE,
  
  INFO_TYPE_UNKNOWN,
}info_type_t;

static info_type_t info_type_get(const char *typestr)
{
  if(!typestr)
    return INFO_TYPE_UNKNOWN;

#define TYPE_ERROR_STR  "error"
#define TYPE_WARN_STR   "warning"
#define TYPE_NOTE_STR   "note"
  
  if(strstr(typestr, TYPE_ERROR_STR))
    return INFO_TYPE_ERROR;
  
  if(strstr(typestr, TYPE_WARN_STR))
    return INFO_TYPE_WARN;
  
  if(strstr(typestr, TYPE_NOTE_STR))
    return INFO_TYPE_NOTE;
  
  return INFO_TYPE_UNKNOWN;
}

static void info_type_printstr(info_type_t type, int align, const char *str)
{
  char fstr[16] = "%s";
  if(align)
    sprintf(fstr, "%%-%ds", align);
  
  switch(type)
  {
    case INFO_TYPE_ERROR:
      xwprintf(fstr, str);
      break;
    case INFO_TYPE_WARN:
      xnprintf(fstr, str);
      break;
    case INFO_TYPE_NOTE:
    default:
      xiprintf(fstr, str);
      break;            
  }
}

xqueue_t *err_queue = NULL, *other_queue = NULL;

/* sure the fist entry of errors is path, shrink it */
#define PATH_INDEX       0
#define LINE_NUM_INDEX   1
#define OFFSET_NUM_INDEX 2
#define INFO_TYPE_INDEX  3
#define INFO_DESC_INDEX  4
static void dump_infos(void *in)
{
  xarray_t *errors = (xarray_t *)in;
  if(!errors)
    return;

  const char *path = (char *)xarray_get(errors, PATH_INDEX);
  
  char *newpath = fhelper_shrink_path(path);
  int lines = 0, col = 0;
  int aligned = 50;
  
  get_terminal_width_height(1, &col, &lines);

  char *type = (char *)xarray_get(errors, INFO_TYPE_INDEX);
  info_type_t info_type = info_type_get(type);  
  
  info_type_printstr(info_type, 30, newpath);
  info_type_printstr(info_type, 4, (char *)xarray_get(errors, LINE_NUM_INDEX));
  info_type_printstr(info_type, 10, type);
  
#define WIDTH_CHARS (50)
  aligned = col - WIDTH_CHARS - 4;
  if(aligned < WIDTH_CHARS)
    aligned = WIDTH_CHARS;
  
  char *desc = (char *)xarray_get(errors, INFO_DESC_INDEX);
  int desclen = strlen(desc);
  
  if(desclen > aligned) /* need to split it */
  {
    int j = 0;
    xarray_t *descs = xstr2array(desc, " ");
    int size = xarray_getcount(descs);
    int len = 0, first_line = 1;
    int from = 0, to = 0;
    
    for(; j < size; j++)
    {
      int item_len = strlen((char *)xarray_get(descs, j));
      if(len + item_len > aligned || j == size - 1)
      {
        if(j == size - 1)
          to = j;

        char *aligned_desc = xarray2str_fromto_index(descs, from, to, ' ');
        
        if(first_line != 1)
          printf("%-44s", "");  
        info_type_printstr(info_type, 0, aligned_desc);  
        printf("\n");
        
        /* need to check */
        free(aligned_desc);
        
        from = j;
        len = 0;
        first_line--;
      }

      to = j;
      len += item_len;
    }
    
    xarray_destroy(descs);
  }
  else
  {
    info_type_printstr(info_type, 0, desc); 
    printf("\n");
  }
  
  if(newpath)
    free(newpath);
}

typedef enum{
  SCROLL_UP,
  SCROLL_DOWN,
  SCROLL_PAGEUP,
  SCROLL_PAGEDOWN,
}scroll_type_t;

/* 1: up, 0: down */
static unsigned int refresh_scroll(unsigned int current_offset, scroll_type_t type)
{
  int lines = 0, col = 0;
  unsigned int others = xqueue_nodes(other_queue);
  unsigned int errors = xqueue_nodes(err_queue);
  get_terminal_width_height(1, &col, &lines);

  /* first two lines are used by statitics */
  lines -= 2;
  
  if(others == 0)
    return 0;
  
  switch(type)
  {
    case SCROLL_DOWN:
      if(current_offset >= others + errors - 1)
        return others + errors - 1;
      
      return current_offset + 1;
      break;
    case SCROLL_UP:
      if(current_offset)
        return current_offset - 1;
      
      return 0;
      break;
    case SCROLL_PAGEDOWN:
      if(current_offset + lines >= others + errors - 1)
        return others + errors - 1;
      return current_offset + lines - 1;
      break;
    case SCROLL_PAGEUP:
      if(current_offset > lines)
        return current_offset - lines + 1;
      return 0;  
      break;
    default:
      break;
  }
      
  return 0;
}

static void refresh_infos(unsigned int offset)
{
  int lines = 0, col = 0;
  unsigned int errors = xqueue_nodes(err_queue);
  unsigned int others = xqueue_nodes(other_queue);
  
  get_terminal_width_height(1, &col, &lines);
  printf(SCREEN_CLEAR); /* clear the screen */
  
  /* show statitics */
  xwprintf("%-10s%-5u-%5s", "errors", errors, "");
  xnprintf("%-10s%-5u-%5s", "others", others, "");
  xnprintf("%-15s%-5u-%5s", "auto refresh", g_auto_refresh, "");
  xnprintf("%-10s%-5u\n\n", "scroll", offset);

  /* at least show 20 lines */
  if(lines <= 20)
    return;

  /* first two lines are used by statitics */
  lines -= 3; /* last line can't show out, why? */
  
  if(offset > errors) /* no need to show errors */
  {
    offset -= errors;
    xqueue_traverse_fromto(other_queue, dump_infos, offset, offset + lines - 1); 
  }
  else //if(offset <= errors)/* only need to show part of errors */
  {
    xqueue_traverse_fromto(err_queue, dump_infos, offset, offset + lines - 1); 
    
    if(lines + offset > errors) /* need to show part of other queue */
      xqueue_traverse_fromto(other_queue, dump_infos, 0, lines + offset - errors - 1); 
  }
}

int main(int argc, char *argv[])
{
  int ret = 0;

  int pipe_fd = -1; 
  int option_index = 0;
  unsigned char screen_offset = 0;
  
  static struct option long_options[] =
  {
    /* These options set a flag. */
    {"help",      no_argument,       0, 'h'},
    {0, 0, 0, 0}
  };

  while(1)
  {
    ret = getopt_long(argc, argv, "h",
                      long_options, &option_index);

     /* Detect the end of the options. */
    if(ret == -1)
      break;

    switch (ret)
    {
      case 'h':
        usage();
        return 0;
      default:
        break;
    }
  }
  
  fhelper_logo();
  usleep(5000);
  terminal_init();

  err_queue = xqueue_create(0, (xqueue_free_f)xarray_destroy);
  other_queue = xqueue_create(0, (xqueue_free_f)xarray_destroy);
  if(!err_queue || !other_queue)
  {
    printf("faile to create info queue");
    goto end;
  }
    
  /* only under scan mode, create pipe */
  pipe_fd = fhelper_pipe_create();
  if(pipe_fd < 0)
  {
    printf("faile to create pipe file.\n");
    goto end;
  }

  /* set the pipe fd and stdin into the fds_set */
  fd_set fds_set, read_set;
  
  int fds[2] = {0};
  int fds_count = 0;
  int max_fd = 0;
  char recvbuf[RECV_BUFSIZE] = "";

  /* add stdin to accept quit key 'q' */
  fds[fds_count++] = STDIN_FILENO;
  fds[fds_count++] = pipe_fd;
  max_fd = xfdset(&fds_set, fds, fds_count);

  /* every 3 second refresh the screen */
  unsigned interval = 1;
  struct timeval tv;
  
  do
  {
    tv.tv_sec = interval;
    tv.tv_usec = 0;
  
    /* reset the fd set */
    read_set = fds_set;
    ret = select(max_fd + 1, &read_set, NULL, NULL, &tv);
    if(ret < 0)
    {
      if(errno != EINTR)
        perror("select");
  
      continue;
    }
    
    /* time out, refresh the screen */
    if(ret == 0)
    {
      if(auto_refresh_get())
        refresh_infos(screen_offset);
      continue;
    }

    /* handle quit key */
    if(FD_ISSET(STDIN_FILENO, &read_set)) 
    {
      unsigned char c = 0;
      if(safe_read(STDIN_FILENO, &c, 1) != 1)
        continue;
      
      /* Ctrl+C */
      if(c == terminal_ctrlc())
        break;
      
      /* 'Q' or 'q' to quit */
      c = (char)tolower((int)c);
      if(c == 'q')
        break;
      
      if(c == 'd')
        refresh_infos(screen_offset);
      
      /* enable or disable auto refresh */
      if(c == 's')
        auto_refresh_reverse();

      /* 27 means a ctrl command */
      if(c == '\033')
      {
        if(safe_read(STDIN_FILENO, &c, 1) != 1)
          continue;
        
        /* the follow character must be '[' */
        if(c != '[')
          continue;
        
        if(safe_read(STDIN_FILENO, &c, 1) != 1)
          continue;

        //printf("c 0x%02x, %d, %d, %c\n", c, (int)c, '\033', c);
        unsigned int old_offset = screen_offset;
        switch(c) 
        { 
          case 'A': /* code for up */
          case 'D': /* code for arrow left */
            screen_offset = refresh_scroll(screen_offset, SCROLL_UP);
            break;
          case 'B': /* code for arrow down */
          case 'C': /* code for arrow right */
            screen_offset = refresh_scroll(screen_offset, SCROLL_DOWN);
            break;
          case '5': /* page up */
            screen_offset = refresh_scroll(screen_offset, SCROLL_PAGEUP);
            break;
          case '6': /* page down */
            screen_offset = refresh_scroll(screen_offset, SCROLL_PAGEDOWN);
            break;  
          default:
            break;
        }
        
        if(old_offset != screen_offset)
          refresh_infos(screen_offset);
      }
    }
    
    /* handle pipe request */
    if(!FD_ISSET(pipe_fd, &read_set))
      continue;
    
    ret = read(pipe_fd, recvbuf, RECV_BUFSIZE);
    recvbuf[ret] = '\0';
    
    if(ret <= 0)
    {
      perror("read");
      continue;
    }
    
    /*
     * only take care of such error/warning lines:
     * /xxx/xxx.c:73:27: warning: unused variable 'list' [-Wunused-variable]
     * Format: file.c:lineno:offset:reasonDesc [-Wreason] 
     *       
     */
    //printf("fheler:%s\n", recvbuf);       
    if(recvbuf[0] != '/')
      continue;
    
    /* check private command */
    if(strstr(recvbuf, "/flush/"))
    {
      xqueue_flush(err_queue);
      xqueue_flush(other_queue);
      continue;
    }
    
    xarray_t *gccinfo = xstr2array(recvbuf, "\n");
    int count = xarray_getcount(gccinfo);
    int i = 0;
    
    for(; i < count; i++)
    {
      /* find the warning and error info entry */
      const char *info = xarray_get(gccinfo, i);
      info_type_t info_type = info_type_get(info);

      if(info_type == INFO_TYPE_UNKNOWN)
        continue;

      /* now analyse the entry */
      xarray_t *errors = xstr2array(info, ":");
      //xarray_dump(errors);
      
      if(xarray_getcount(errors) >= 5)
      {
        char *type = (char *)xarray_get(errors, INFO_TYPE_INDEX);
        info_type_t info_type = info_type_get(type);
        if(info_type == INFO_TYPE_ERROR)
            xqueue_enqueue(err_queue, errors);
        else
            xqueue_enqueue(other_queue, errors);
      }
      else
        xarray_destroy(errors);
    }
    
    xarray_destroy(gccinfo);
  }while(1);
  
  fhelper_pipe_close(pipe_fd);

end:
  xqueue_destroy(err_queue);
  xqueue_destroy(other_queue);
  
  terminal_reset();
	return 0;
}

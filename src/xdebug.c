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
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <syslog.h>
#include <limits.h>
#include <errno.h>

#include "xdebug.h"

#ifdef XDEBUG 

/* default log size is 64K */
#define XLOG_LIMIT_SIZE (1024 * 64)

/* default log file path */
#define XLOG_PATH         "/var/log"
#define XLOG_FILE_SUFFIX  ".log"
#define XLOG_SWAP_SUFFIX  ".swap"

/* 
 * xlog proxy, if log file is not defined, use syslog.
 * On some embeded system, the ram and flash are limited, so XLOG_LIMIT_SIZE
 * is used to limit the log file size.
 *
 * When the log file size > .log, it will be rename as .log.swp. and the oldest
 * .log.swp will be covered. xlogfsize is used to set the file limitation size.
 *
 * The lowest log priority is LOG_DEBUG, using xloglevel to change it.
 *
 * Log messages are able to be redicted to syslog/stdout/stderr or /dev/null,
 * using xlogfile to set the way.
 */
typedef struct xlogctl
{
  char file[PATH_MAX];  /* log file */
  FILE *fp;             /* fp point to log file */

  long long fsize; /* max log fsize, 0 means no limit  */
  int level;       /* if priority <= level then log it otherwise ignore */
  
  char syslog;     /* 0/1 is to syslog */
}xlogctl_t;

static xlogctl_t xlogctl = {"syslog", NULL, XLOG_LIMIT_SIZE, LOG_DEBUG, 1};

/* for debugging */
void xlogdump()
{
  printf("xlogctl.file:\t%s\n", xlogctl.file);
  printf("xlogctl.fp:\t%p\n", xlogctl.fp);
  printf("xlogctl.fsize:\t%lld\n", xlogctl.fsize);
  printf("xlogctl.level:\t%d\n", xlogctl.level);
  printf("xlogctl.syslog:\t%d\n", xlogctl.syslog);
}

void _xprintf(FILE *ofp,
              const char *filename,
              const int line,
              const char *funcname,
              const char *fmt, ...)
{
  char *msg = NULL;
  va_list arg;
  FILE *fp = ofp;
  
  /* just try to print out to console */
  if(fp == NULL)
  {
    fp = fopen("/dev/console", "a+");
    if(fp == NULL)
    {
      fprintf(stderr, "open /dev/console error %s\n", funcname);
      return;
    }
  }
  
  va_start(arg, fmt);
#ifndef XDEBUG_PURE  
  if(filename && funcname)
  {
    char *tmp = rindex(filename, '/');
    fprintf(fp, "=%s/%s(%d)= ", tmp ? tmp + 1 : filename, funcname, line);
  }
#endif  

  if(vasprintf(&msg, fmt, arg) < 0)
  {
    fprintf(fp, "Xdebug Memory exhausted!\n");
    if(fp != stderr)
      fprintf(stderr, "Xdebug Memory exhausted!\n");
      
    fflush(fp);
    va_end(arg);
    goto out;
  }

  fprintf(fp, "%s", msg);
  fflush(fp);
  va_end(arg);
  
out:  
  if(fp && fp != stdout && fp != stderr)
    fclose(fp);
  if(msg)
    free(msg);
    
  return;
}

void _xassert(FILE *fp,
        const char *filename,
              const int line,
              const char *funcname,
              const char *expstr)
{
  _xprintf(fp, filename, line, funcname,
              "Assertion `%s' failed.\n", expstr);
  _xsyslog(LOG_ERR, filename, line, funcname, "Assertion `%s' failed.\n", expstr);

  abort();
}

#define XDUMP_LINE_LEN (16)
static void xdumpline(FILE *out, int lineno, const unsigned char *addr, unsigned int len)
{
  int i = 0;
  int split = (XDUMP_LINE_LEN + 1) / 2;
  
  fprintf(out, "[%04x] ", lineno);
  for(i = 0; i < XDUMP_LINE_LEN; i++)
  {
    if(i < len)
      fprintf(out, " %02x", addr[i]);
    else
      fprintf(out, "   ");
    
    if(((i + 1) % split) == 0) 
      fprintf(out, " ");
  }

  fprintf(out, "  ");
  
  for(i = 0; i < XDUMP_LINE_LEN; i++)
  {
    char c = 0x7f & ((char *) addr)[i];
    if (i < len)
      fprintf(out, "%c", isprint(c) ? c : '.');
    else
      fprintf(out, " ");
  }
}

/* 
* A useful function for debugging.
* Guess... the name Ohh. Even through the len is defined as unsigned int
* but should be limited by inbuf's size.
*/
void xdumphex(const void *addr, unsigned int len)
{
  FILE *out = stdout;
  
  int i = 0, j = 0;
  int tail =  len % XDUMP_LINE_LEN;
  int lineno = len / XDUMP_LINE_LEN;

  /* just try to print out to console */
#ifdef XDEBUG_CGI  
  out = fopen("/dev/console", "a+");
  if(out == NULL)
  {
    fprintf(stderr, "open /dev/console error\n");
    return;
  }
#endif

  for(i = 0; j < lineno; i += XDUMP_LINE_LEN, j++)
  {
    xdumpline(out, j, addr + i, XDUMP_LINE_LEN);
    fprintf(out, "\n");
  }
  
  if(tail)
  {
    xdumpline(out, j, addr + i, tail);
    fprintf(out, "\n");
  }
  
  fflush(out);

#ifdef XDEBUG_CGI  
  fclose(out);
#endif  
}

/******************** below are for xlog ********************************/

/*
 * print the timestam to the log
 * NOTE: need to set TZ= enviroment in the bash or your program with setevn
 * to let localtime() work correctly.
 */
static void xlogtimestamp()
{
  int len = 0;
  
  struct timeval tv;
  char *tstr;
  
  if(xlogctl.fp == NULL
    || xlogctl.fp == stdout
    || xlogctl.fp == stderr)
    return;

  memset(&tv, 0, sizeof(tv));
  gettimeofday(&tv, NULL);
  //tzset(); /* update time zone */
  tstr = asctime(localtime(&tv.tv_sec));
  
  len = strlen(tstr);
  if(len <= 0)
    return;
    
  tstr[len - 1] = '\0';
  
  fprintf(xlogctl.fp, "[%s] ", tstr);
}

/****************************************************************************
 *desc :  the log priority, default is LOG_DEBUG
 *input only when <= the log level the log messages will be recorded 
 
 *output: None
 *return: -1 failed; 0 OK
****************************************************************************/
int xloglevel(int level)
{  
  if(level < LOG_EMERG || level > LOG_DEBUG)
  {
    fprintf(stderr, "Loglevel should between LOG_EMERG and LOG_DEBUG\n");
    return -1;
  }
  
  xlogctl.level = level;
  return 0;
}

/****************************************************************************
 *desc : the log file size, default is XLOG_LIMIT_SIZE
 *input  size of the log file, 0 means no limited, the minimum size is 512
****************************************************************************/
int xlogfsize(long long fsize)
{  
  if(xlogctl.file[0] != '\0' 
     && xlogctl.fsize < 512)
  {
    xerror("Logfize is too short, should be bigger than 512Bytes!");
    return -1;
  }
  
  xlogctl.fsize = fsize;
  return 0;
}

/****************************************************************************
 *desc :  the swap log file, when log file is full, it will be renamed as .sawp
          Actually, it is a trash buffer of log, meanwhile the old log sway file 
          will be flushed away.
          
 *input: None
 
 *output: None
 *return: -1 failed; 0 OK
****************************************************************************/
static int xlogswap()
{
  struct stat stat;
  
  if(xlogctl.fsize == 0  /* no limit */
    || xlogctl.fp == NULL
    || xlogctl.fp == stdout
    || xlogctl.fp == stdout)
    return 0;

  if(fstat(fileno(xlogctl.fp), &stat) < 0
     || stat.st_size == (off_t)-1)
  {
    xerror("fstat");
    return -1;
  }

  if(stat.st_size >=  xlogctl.fsize)
  {
    char swap[PATH_MAX] = "";

    // ftruncate(fileno(xlogctl.fp), 0);
    snprintf(swap, PATH_MAX, "%s%s", xlogctl.file, 
             XLOG_SWAP_SUFFIX);
    unlink(swap);
    
    /* FixME. with cp will not need xlogfile, but no c desc */
    rename(xlogctl.file, swap);

    /* reset log file */
    return xlogfile(xlogctl.file);
  }

  return 0;
}

/****************************************************************************
 *desc :  when the application quits, the atexit will be called to do last
          jobs
 *input: None
 
 *output: None
 *return: None
****************************************************************************/
static void xlogexit()
{  
    /* close old fp */
  if(xlogctl.fp != NULL
    && xlogctl.fp != stdout
    && xlogctl.fp != stderr)
  {
    fclose(xlogctl.fp);
    xlogctl.fp = NULL;
  }
}

/****************************************************************************
 *desc :  Set the log file, all below file types are avaliable:
          1. a noremal file, /xx/xxx.log or it will be recorded under dir XLOG_PATH
          2. stdout/stderr, dump the log to the standard terminal
          3. syslog send to syslogd
          4. NULL don't record any logs

 *input: None

 *output: None
 *return: -1 failed; 0 OK
****************************************************************************/
int xlogfile(const char *newfile)
{  
  char *tmp = NULL;
  char logfile[PATH_MAX] = "";
    
  /* close old fp */
  if(xlogctl.fp != NULL
    && xlogctl.fp != stdout
    && xlogctl.fp != stderr)
  {
    fclose(xlogctl.fp);
    xlogctl.fp = NULL;
  }
  
  if(newfile == NULL)
    return -1;
  
  /* 
   * if input is xlogctl.file will cause error when do snrpintf 
   * so copy it first 
   */
  snprintf(logfile, PATH_MAX, "%s", newfile);
  xlogctl.syslog = 0;
  
  if(strcmp(logfile, "syslog") == 0)
    xlogctl.syslog = 1;
  else if(strcmp(logfile, "stdout") == 0)
    xlogctl.fp = stdout;
  else if(strcmp(logfile, "stderr") == 0)
    xlogctl.fp = stderr;
  else
  {
    /* add file suffix */
    tmp = strstr(logfile, ".log");
    if(NULL == tmp)
      snprintf(xlogctl.file, PATH_MAX, "%s.log", logfile);
  else
      snprintf(xlogctl.file, PATH_MAX, "%s", logfile);

    /* abs path */
    if(logfile[0] != '/')
    {
      snprintf(logfile, PATH_MAX, "%s/%s", XLOG_PATH, xlogctl.file);
      snprintf(xlogctl.file, PATH_MAX, "%s", logfile);
    }

    xlogctl.fp = fopen(xlogctl.file, "a+");
    if(xlogctl.fp == NULL)
    {
      xerror("fopen");
      return -1;
    }
    
    atexit(xlogexit);
  }
  
  return 0;
}

/****************************************************************************
 *desc :  record log, if XDEBUG_PURE is defined the log will ignore lineno 
          and func info
****************************************************************************/
void _xsyslog(int priority,
              const char *filename,
              const int line,
              const char *funcname,
              const char *fmt, ...)
{
  char *msg = NULL;
  
#define MARK_SIZE 1024 /* must be enough */
  char mark[MARK_SIZE] = "";
  va_list arg;
  
  if(priority > xlogctl.level)
    return;
  
  if(xlogctl.syslog == 0
    && xlogctl.fp == NULL)
    return;
  
  va_start(arg, fmt);
#ifndef XDEBUG_PURE
  if(filename && funcname)
  {
    char *tmp = rindex(filename, '/');

    if(priority <= LOG_ERR)
      snprintf(mark, MARK_SIZE, "=%s/%s(%d,%s)= ", 
               tmp ? tmp + 1 : filename, funcname, line, strerror(errno));
    else
      snprintf(mark, MARK_SIZE, "=%s/%s(%d)= ", tmp ? tmp + 1 : filename, funcname, line);
    mark[MARK_SIZE - 1] = '\0';
  }
#endif

  if(vasprintf(&msg, fmt, arg) < 0)
  {
    fprintf(stderr, "Memory exhausted!\n");
    va_end(arg);
    goto out;
  }

  if(xlogctl.fp)
  {
    xlogswap();
    xlogtimestamp();
    fprintf(xlogctl.fp, "%s%s", mark, msg);
    fflush(xlogctl.fp);
  }
  else
  {
    openlog("xsyslog", 0, 0);
    syslog(priority, "%s%s", mark, msg);
    closelog();
  }

#ifdef XLOG_WITH_PRINT
  if(xlogctl.fp != stdout
     && xlogctl.fp != stderr)
    fprintf(stderr, "%s%s", mark, msg);
#endif

  va_end(arg);
  
out:  
  if(msg)
    free(msg);
    
  return;
}

/* below funcs are for color print stole from git-1.7.2.2 color.c */
/* dump with colors */
static int parse_color(const char *name, int len)
{
  static const char * const color_names[] = 
  {
    "normal", "black", "red", "green", "yellow",
    "blue", "magenta", "cyan", "white"
  };
  
  char *end;
  int i;
  
  for(i = 0; i < ARRAY_SIZE(color_names); i++)
  {
    const char *str = color_names[i];
    if (!strncasecmp(name, str, len) && !str[len])
      return i - 1;
  }
  
  i = strtol(name, &end, 10);
  if(end - name == len && i >= -1 && i <= 255)
    return i;
    
  return -2;
}

static int parse_attr(const char *name, int len)
{
  static const int attr_values[] = { 1, 2, 4, 5, 7 };
  static const char * const attr_names[] =
  {
    "bold", "dim", "ul", "blink", "reverse"
  };
  
  int i;
  for(i = 0; i < ARRAY_SIZE(attr_names); i++)
  {
    const char *str = attr_names[i];
    if(!strncasecmp(name, str, len) && !str[len])
      return attr_values[i];
  }
  
  return -1;
}

/* try to parse color restrictions: vlaue ([fg [bg]] [attr]... ) 
 * to color code: dst
 */
static void color_parse(char *dst, const char *value)
{
  int value_len = strlen(value);
  const char *ptr = value;
  int len = value_len;
  unsigned int attr = 0;
  int fg = -2;
  int bg = -2;

  if (!strncasecmp(value, "reset", len)) {
    strcpy(dst, COLOR_RESET);
    return;
  }

  /* [fg [bg]] [attr]... */
  while (len > 0) {
    const char *word = ptr;
    int val, wordlen = 0;

    while (len > 0 && !isspace(word[wordlen]))
    {
      wordlen++;
      len--;
    }

    ptr = word + wordlen;
    while (len > 0 && isspace(*ptr))
    {
      ptr++;
      len--;
    }

    val = parse_color(word, wordlen);
    if (val >= -1) {
      if (fg == -2) {
        fg = val;
        continue;
      }
      if (bg == -2) {
        bg = val;
        continue;
      }
      goto bad;
    }
    val = parse_attr(word, wordlen);
    if (0 <= val)
      attr |= (1 << val);
    else
      goto bad;
  }

  if(attr || fg >= 0 || bg >= 0)
  {
    int sep = 0;
    int i;

    *dst++ = '\033';
    *dst++ = '[';

    for (i = 0; attr; i++)
    {
      unsigned bit = (1 << i);
      if (!(attr & bit))
        continue;
      attr &= ~bit;
      if (sep++)
        *dst++ = ';';
      *dst++ = '0' + i;
    }
    if (fg >= 0)
    {
      if (sep++)
        *dst++ = ';';
      if (fg < 8)
      {
        *dst++ = '3';
        *dst++ = '0' + fg;
      } 
      else
        dst += sprintf(dst, "38;5;%d", fg);      
    }
    if (bg >= 0) 
    {
      if (sep++)
        *dst++ = ';';
      if (bg < 8)
      {
        *dst++ = '4';
        *dst++ = '0' + bg;
      }
      else
        dst += sprintf(dst, "48;5;%d", bg);      
    }
    *dst++ = 'm';
  }
  *dst = 0;
  return;
  
bad:
  xdie("bad color value for variable '%s'\n", value);
}

static int color_vfprintf(FILE *fp, const char *color_code, const char *fmt,
                          va_list args, const char *trail)
{
  int r = 0;

  if (*color_code)
    r += fprintf(fp, "%s", color_code);
  r += vfprintf(fp, fmt, args);
  if (*color_code)
    r += fprintf(fp, "%s", COLOR_RESET);
  if (trail)
    r += fprintf(fp, "%s", trail);
  
  /* add this line for color strictly outputing */
  fflush(fp);
  return r;
}

void color_fprintf(FILE *fp, const char *color, const char *fmt, ...)
{
  char color_code[16] = "";
  va_list args;

  va_start(args, fmt);
  color_parse(color_code, color);
  color_vfprintf(fp, color_code, fmt, args, NULL);
  va_end(args);
  
  fflush(fp);
}

#endif /* XDEBUG */


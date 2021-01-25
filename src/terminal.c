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
#include <termios.h>
#include <signal.h>
#include "xdebug.h"
#include "terminal.h"

static void terminal_curosr_hide(int yes)
{
  if(yes)
    printf(CURSOR_HIDE);/* hidden the cursor */
  else
    printf(CURSOR_SHOW); /* show the cursor */
}

/* for terminal ctl */
static struct termios initial_settings;
void terminal_reset(void)
{
  tcsetattr(STDIN_FILENO, TCSANOW, &initial_settings);
  terminal_curosr_hide(0);
}

static void sig_catcher(int sig)
{
  terminal_reset();
}

enum {
 /* 
  * (FATAL_SIGS, handler) catches all signals which
  * otherwise would kill us, except for those resulting from bugs:
  * SIGSEGV, SIGILL, SIGFPE.
  * Other fatal signals not included (TODO?):
  * SIGBUS   Bus error (bad memory access)
  * SIGPOLL  Pollable event. Synonym of SIGIO
  * SIGPROF  Profiling timer expired
  * SIGSYS   Bad argument to routine
  * SIGTRAP  Trace/breakpoint trap
  *
  * The only known arch with some of these sigs not fitting
  * into 32 bits is parisc (SIGXCPU=33, SIGXFSZ=34, SIGSTKFLT=36).
  * Dance around with long long to guard against that...
  */
  FATAL_SIGS = (int)(0
        + (1LL << SIGHUP)
        + (1LL << SIGINT)
        + (1LL << SIGTERM)
        + (1LL << SIGPIPE)   // Write to pipe with no readers
        + (1LL << SIGQUIT)   // Quit from keyboard
        + (1LL << SIGABRT)   // Abort signal from abort(3)
        + (1LL << SIGALRM)   // Timer signal from alarm(2)
        + (1LL << SIGVTALRM) // Virtual alarm clock
        + (1LL << SIGXCPU)   // CPU time limit exceeded
        + (1LL << SIGXFSZ)   // File size limit exceeded
        + (1LL << SIGUSR1)   // Yes kids, these are also fatal!
        + (1LL << SIGUSR2)
        + 0),
};

static void install_signals(int sigs, void (*f)(int))
{
  int sig_no = 0;
  int bit = 1;

  while (sigs) 
  {
    if (sigs & bit) 
    {
      sigs &= ~bit;
      signal(sig_no, f);
    }
    
    sig_no++;
    bit <<= 1;
  }
}

/* Ctrl+C code */
char terminal_ctrlc()
{
  return initial_settings.c_cc[VINTR];
}

void terminal_init()
{
  /* init terminal cfg */
  struct termios new_settings;
  tcgetattr(STDIN_FILENO, (void *) &initial_settings);
  memcpy(&new_settings, &initial_settings, sizeof(new_settings));

  /* unbuffered input, turn off echo */
  new_settings.c_lflag &= ~(ISIG | ICANON | ECHO | ECHONL);

  install_signals(FATAL_SIGS, sig_catcher);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
  
  
  terminal_curosr_hide(1);
}

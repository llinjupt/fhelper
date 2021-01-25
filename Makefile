# A generic used Makefile, which is to provide an instant building 
# environment for both C and C++ programs.
#
# Powered by Eastforest Co., Ltd
# Author: Red Liu lli_njupt@163.com
#

include ./make.def

# Parameters are transfered to the program while compiling
# for program version instead of the Makefile version
VERSION      = 0.1
# The compile time, spread to the program from the FLAGS
MAKE_TIME = $(shell date +%F\ %H:%M:%S)

.PHONY = all install clean

# global switches
GDB          = 0
TOPDIR       = $(shell pwd)
SRCDIR       = $(TOPDIR)/src
TARGET_DIR   = $(TOPDIR)/
LIB_DIR      = $(TOPDIR)/lib  
OBJECTDIR    = $(TOPDIR)/build
INCLUDEDIR   = $(TOPDIR)/include 

# target application name
TARGETMAIN  = fhelper
MAINDOTC    = $(TARGETMAIN).c

# find all source file located dirs
VPATH 	   = $(shell ls -AxR $(SRCDIR)|grep ":"|grep -v "\.git"|tr -d ':')
SOURCEDIRS = $(VPATH)

# search source file in the current dir
CSOURCES      = $(foreach subdir,$(SOURCEDIRS),$(wildcard $(subdir)/*.c))
CPPSOURCES  = $(foreach subdir,$(SOURCEDIRS),$(wildcard $(subdir)/*.cpp))
COBJS 	   = $(patsubst %.c,%.o,$(CSOURCES))
CPPOBJS 	   = $(patsubst %.cpp,%.o,$(CPPSOURCES))
BUILDCOBJS      = $(subst $(SRCDIR),$(OBJECTDIR),$(COBJS))
BUILDCPPOBJS  = $(subst $(SRCDIR),$(OBJECTDIR),$(CPPOBJS))

DEPS	   = $(patsubst %.o,%.d,$(BUILDOBJS))
SRCCXXS    = $(filter-out %.c,$(SOURCES))

# Common flags for both C and C++
# Good to enable -Wall, -Wextra and maybe even -pedantic and -Werror 
# to make the compiler as picky as possible. 
CPPFLAGS  = -Wall

# default is little endian for both linux and windows
# CPPFLAGS += -DBIGENDIAN

# enable test
CPPFLAGS  += #-DTEST

# run on WIN32 Linux or MacOS 
CPPFLAGS  += -D_WIN32 

# Compatilbe with -D_GUN_SOURCE or POSIX 
CPPFLAGS  += -D_GNU_SOURCE 
CPPFLAGS  += -DMAKE_TIME='"$(MAKE_TIME)"'
CPPFLAGS  += -DVERSION='"$(VERSION)"'

# for c and c++, -g used to debug, and -O2 that optimizes more
# man gcc to see what's going on with -Ox.
# -MD create dependency files
CFLAGS   = -g -O2 -MD
CXXFLAGS = -g  -MD 

# To lower the g++ error threshhold for old style code, but 
# it is stronly not recommended
CXXFLAGS   += -fpermissive

# external include file define
CFLAGS	 += $(foreach dir,$(INCLUDEDIR),-I$(dir))
CXXFLAGS += $(foreach dir,$(INCLUDEDIR),-I$(dir))

ifeq ($(GDB),1)
CFLAGS  += -ggdb3
CXXFLAGS += -ggdb3
endif

# enable load dynamic lib, always put it as the last parameter 
# of the link command
LDFLAGS	   = -ldl -lm 
LDFLAGS	   += 
LDFLAGS	   += $(foreach lib,$(LIB_DIR),-L $(lib))

COMPILE.c   = $(CC)  $(CFLAGS)   $(CPPFLAGS) -c
COMPILE.cxx = $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c
LINK.c      = $(CC)  $(CFLAGS)   $(CPPFLAGS)
LINK.cxx    = $(CXX) $(CXXFLAGS) $(CPPFLAGS)

# if use c++ to compile .c, use cxxflags instead of cflags
ifeq ($(CC),$(CXX))
COMPILE.c   = $(COMPILE.cxx)
LINK.c      = $(LINK.cxx)
endif

# defaut target:compile the currrent dir file and sub dir 
all: __prepare $(TARGETMAIN)

# for .h header files dependence
#-include $(DEPS)

# internal targets always with prefix "__"
__prepare:
#	@./scripts/chkutf8.sh
#	@touch ./src/$(MAINDOTC)
	@echo $(COMPILE.c) > make.time

$(TARGETMAIN) : $(BUILDCOBJS)  $(BUILDCPPOBJS)
ifeq ($(SRC_CXX),)  # C program
	@$(LINK.c) $(subst $(SRCDIR),$(OBJECTDIR),$^) -o $@ $(LDFLAGS) 
else
	@$(LINK.cxx) $(subst $(SRCDIR),$(OBJECTDIR),$^) -o $@ $(LDFLAGS)
endif
#	@$(STRIP)  --strip-unneeded $(TARGETMAIN)

# use cc to compile .c files
$(OBJECTDIR)%.o: $(SRCDIR)%.c
	@[ ! -d $(dir $(subst $(SRCDIR),$(OBJECTDIR),$@)) ] & $(MKDIR) -p $(dir $(subst $(SRCDIR),$(OBJECTDIR),$@))
	@$(COMPILE.c) -o $(subst $(SRCDIR),$(OBJECTDIR),$@) -c $<

# use cxx to compile .cpp files
$(OBJECTDIR)%.o: $(SRCDIR)%.cpp
	@[ ! -d $(dir $(subst $(SRCDIR),$(OBJECTDIR),$@)) ] & $(MKDIR) -p $(dir $(subst $(SRCDIR),$(OBJECTDIR),$@))
	@$(COMPILE.cxx) -o $(subst $(SRCDIR),$(OBJECTDIR),$@) -c $<

install:
	cp -f $(TARGETMAIN) $(INSTALLDIR)/usr/sbin/
	ln -snf /usr/sbin/$(TARGETMAIN) $(INSTALLDIR)/usr/sbin/client

clean:
	@$(RM) -rf $(OBJECTDIR)
	@$(RM) -f *.d *.o
	@$(RM) -f $(TARGETMAIN)
	@$(RM) -f make_time

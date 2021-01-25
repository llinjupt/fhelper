# Fhelper

Fhelper is a useful tool, which is designed to analyse the output of gcc/cc compiler and show in lines and rows automatically. Cowork with the make.sh, you just need to code and check the error/warning lists without typing make again and again.

## How to compile

1. Edit make.def and update this line. Normally if your target platform is yor PC, just leave it here empty.

  CROSS_COMPILE ?=

2. $ make

After compiling successfully, you will find a new file named fhelper (Linux) or fhelper.exe (Win).  

## How to use it

1. fhelper.exe -h will tell you how to implement it.

$ ./fhelper.exe -h

  Usage:

    fhelper <options>

  where options may include:

    --help -h        to output this message.
    D or D           refresh the screen.
    S or s           enable or disable refresh .
    Arrows/pagedn/up scroll the list.
    Q or q           quit.

2. Run fhelper without -h, it will create a pipe file named /tmp/fhelper then runs as a daemon.
  
  $./fhelper 

3. Put scripts/make.sh to your own program's dir where the Makefile located.
then run ./make.sh or you just need run "make > /tmp/fhelper 2>&1" without the coworker make.sh, 
but you need to tell fhelper to flush the last info manually.

## More hints
It helped me and my team a lot and I hope it will help you.
I highly recommend ConEmu (on Win) or Tmux (On linux) to open multiple console workbenches in one window.
One workbench is for fhelper, one is for make.sh and the last one is for you to code and debug.

#!/bin/sh
# comworker with fhelper.
# put it in your project dir where the Makefile is located.

export LANG=en_US
csum="1234567890"

while true
# using cusm to make sure if it needs a new make
do
  newcsum=$(find . -name "*.c" -o -name "*.h" -type f | xargs md5sum | md5sum)
  if [ "$csum" = "$newcsum" ] ; then
    sleep 5
    continue
  fi

  csum="$newcsum"
  echo "compiling at $(date +%F\ %H:%M:%S) ..."
  
# uncomment this line if need
# make clean

# tell fhelper flush away all stuff and be ready for a new round
  echo "/flush/" > /tmp/fhelper

# dump all err/warning messages to fhelper 
  make >/tmp/fhelper 2>&1
  echo "auto run..."

# check your program if it's recreated and run it here automatically 

done


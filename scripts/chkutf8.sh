#/bin/sh

CHECK_FILES="./src/capdev/cap_lang.c"
CHECK_FILES=${CHECK_FILES}" ./src/sms/sms.c"
CHECK_FILES=${CHECK_FILES}" ./src/capwifi/hb1682/hb1682_tab.c"

#echo $CHECK_FILES

for i in $CHECK_FILES
do
  format=`file --mime-encoding $i | cut -d' ' -f2`
  if [ "$format" != "us-ascii" -a "$format" != "utf-8" ]
  then
    echo "Error! The encode of $i is $format, should be utf-8 or us-ascii!" 
    exit 1
  fi
done

exit 0

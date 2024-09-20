:
# simple script to find all regular files and creates a sha1 signiture for that file and places it 
# into the single file sha1chklist.txt
# A check of all the files can then be performed via
# sha1sum -c sha1chklist.txt
# this line starts from the currecnt work directory
# find . -type f -exec sha1sum {} > sha1chklist.txt \;

#
# RHEl, Ubuntu sha1sum, INterix sha1  (note output is reversed name then sha1)
#
if [ "$(uname -s)" = "Darwin" ]; then
  if [ -x /sbin/md5 ]; then
     SHA1SUM="/sbin/md5"
  else
     echo "No md5 application, exiting."
     exit
  fi
elif [ -x /usr/bin/sha1sum ]; then
  SHA1SUM="sha1sum"
elif [ -x /bin/sha1 ]; then
  SHA1SUM="sha1"
else
  echo "No sha1 application, exiting."
  exit
fi

# this one would go to the working vnmr directory and start there.
if [ ! -d ../../vnmr/adm/sha1 ]
then
   mkdir -p ../../vnmr/adm/sha1
fi
cd ../../vnmr; 

## regular files 
# find . -type f -exec sha1sum {} > sha1chklist.txt \;
## regular files  and symlinks
## have find follow the symlink to real type of file.
## since the sha1sum will follow the symlink to the file to create the sha1 anyway
##
# find . -follow \( -type f -o -type l \) -exec sha1sum {} > adm/sha1/sha1chklist.txt \;
# 
#  some attempt was to remove pubsmanual html files out of the sha1 list
#  however this prove to be problematic since some names have control chars embeeded in them, thus refusing
# attempt to regex them the closest obtain was the following but good enough...
#  find ../../vnmr  ! -regex '.*/help/html/.*'  -print > /workspace/greg/master/git-repo/scripts/tst.txt
#   GMB
# find . -follow \( -type f -o -type l \) ! -name ".git*" -exec sha1sum {} > adm/sha1/sha1chklist.txt \;
find . -follow \( -type f -o -type l \) ! -name ".git*" -exec $SHA1SUM {} > adm/sha1/sha1chklist.txt \;
if [ -d ../options ]; then
   cd ../options
   find standard -follow \( -type f -o -type l \)  ! -name ".git*" -exec $SHA1SUM {} > ../vnmr/adm/sha1/sha1chklistOptionsStd.txt \;
   find console -follow \( -type f -o -type l \)  ! -name ".git*" -exec $SHA1SUM {} > ../vnmr/adm/sha1/sha1chklistOptionsConsole.txt \;
fi
if [ -d ../console ]; then
   cd ../console
   find . -follow \( -type f -o -type l \) ! -name ".git*" -exec $SHA1SUM {} > ../vnmr/adm/sha1/sha1chklistConsole.txt \;
fi

cd ../vnmr
$SHA1SUM ./adm/sha1/Build_Id.txt ./adm/sha1/sha1chklist.txt ./adm/sha1/sha1chklistOptionsStd.txt ./adm/sha1/sha1chklistOptionsConsole.txt ./adm/sha1/sha1chklistConsole.txt > adm/sha1/sha1chklistFiles.txt

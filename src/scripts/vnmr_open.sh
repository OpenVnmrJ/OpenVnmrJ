#! /bin/sh
#
# open a file based on type and default application for that type
# e.g. open a pdf bring up Acroreader, a html file the browser
#
if [ -z "$vnmrsystem" ] ; then
   vnmrsystem="/vnmr"
fi
ostype=`uname -s`
path="$1"
url=`perl $vnmrsystem/bin/addhttp.pl $1`
if [ ! -z "$url" ] ; then
   path=$url
fi
case "$ostype" in
   cygwin | Interix )
      opencmd="cmd /c start"
      if [ -z "$url" ]; then 
         path=`unixpath2win "$1"`
      fi
        ;;
   *inux*)   # Linux, linux, etc.
      # echo "linux"
      # opencmd="gnome-open"  depricated
      # check for the replacement, if not present fail-back to gnome-open
      if [ -x /usr/bin/xdg-open ]; then
         opencmd="xdg-open"
      else 
         opencmd="gnome-open"
      fi
        
        ;;
   *arwin*)  # Darwin, darwin
      opencmd="open"
        ;;
      *)
       # echo "default"
       if [ -x /usr/bin/xdg-open ]; then
         opencmd="xdg-open"
       else 
         opencmd="gnome-open"
       fi
       ;;
esac

$opencmd "$path"


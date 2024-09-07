#! /bin/bash
#
# open a file based on type and default application for that type
#
if [ -z "$vnmrsystem" ] ; then
   vnmrsystem="/vnmr"
fi
ostype=$(uname -s)
path="$1"
url=$(perl $vnmrsystem/bin/addhttp.pl $1)
if [ ! -z "$url" ] ; then
   path=$url
fi
case "$ostype" in
   *inux*)   # Linux, linux, etc.
      # echo "linux"
      # opencmd="gnome-open"  deprecated
      # check for the replacement, if not present fail-back to gnome-open
      if [ -x /usr/bin/gio ]; then
         opencmd="gio open"
      elif [ -x /usr/bin/xdg-open ]; then
         opencmd="xdg-open"
      else 
         opencmd="gnome-open"
      fi
      path=$(readlink -f $path)
        
        ;;
   *arwin*)  # Darwin, darwin
      opencmd="open"
        ;;
      *)
       # echo "default"
       if [ -x /usr/bin/gio ]; then
         opencmd="gio open"
       elif [ -x /usr/bin/xdg-open ]; then
         opencmd="xdg-open"
       else 
         opencmd="gnome-open"
       fi
       ;;
esac

$opencmd "$path"


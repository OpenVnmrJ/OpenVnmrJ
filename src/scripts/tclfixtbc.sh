: '@(#)tclfixtbc.sh 22.1 03/24/08 1999-2000 '
# 
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
#!/bin/sh

ostype=`uname -s`
if [ x$ostype = "xSunOS" ]
then
    osver=`uname -r`
    case $osver in
       5*) ostype="SOLARIS"
           tbcloadlib="libtbcload13.so"
           ;;
        *)
           ;;
    esac
fi
if [ x$ostype = "xIRIX64" ]
then
   ostype="IRIX"
fi

if [ x$ostype = "xLinux" ]
then
   tbcloadlib="libtbcload1.3.so"
fi

#ostype="IRIX"
if [ $# -ne 0 ]
then
    loadstrg="load [file join \$env(vnmrsystem) lib $tbcloadlib]"
    tmpfile="/tmp/if_see_please_remove_it"

    for File
    do 
        #name=`echo $File | cut -d. -f1`
        #tbcFile=${name}.tbc

        linenum=`grep -n Include $File | cut -d: -f1`
        linenum=`expr $linenum + 1`
        case $ostype in

            IRIX )
                   fr_line=`expr $linenum + 1`
                   to_line=`expr $linenum + 3`
                   sed "$fr_line,${to_line}d" $File > $tmpfile

                   mv -f $tmpfile $File
                   ;;

         SOLARIS | Linux)
                   (sed ${linenum}'i\
'"$loadstrg"'' $File > $tmpfile)
                   mv -f $tmpfile $File
                   ;;

               * )
                   echo "Nothing done to $File"
                   ;;
       esac
    done
else
    echo "\n    Usage: $0  myfile.tbc [myfile1.tbc] [myfile2.tbc] [myfil...]\n"
    exit 1
fi

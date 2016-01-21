: '@(#)updateuser.sh 22.1 03/24/08 1991-1996 '
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
# Usage: updateuser
# Each user must run this script (or makeuser) before running VNMR 5.1

: /bin/sh

if test $HOME = "/"
then
   echo "no home directory, cannot proceed with $0"
   exit 1
fi
userhome=$HOME
uservnmr=$userhome/vnmrsys

if test `ls $uservnmr/seqlib | wc -l` -ne 0
then
  echo "Removing old pulse sequences..."
  rm $uservnmr/seqlib/*
fi

if test -d $uservnmr/psg
then
    for filespec in "*.a" "*.so.*" "*.ln" "*.o" "makeuserpsg"
    do
        tmpval=`(cd $uservnmr/psg; ls $filespec 2>/dev/null) | wc -c`
        if test $tmpval != "0"
        then
            echo "  removing '$filespec' from psg subdirectory"
            (cd $uservnmr/psg; rm -f $filespec)
        fi
    done 
fi

# Check for modification of globals
echo -n "Adding parameters to global file..."
if test `grep -c parstyle $uservnmr/global` -eq 0
then
  echo -n "parstyle..."
  echo "parstyle 2 2 8 0 0 4 1 0 1 64" >> $uservnmr/global
  echo "1 \"ppa\"" >> $uservnmr/global
  echo "0" >> $uservnmr/global
fi

if test `grep -c pkpick $uservnmr/global` -eq 0
then
  echo -n "pkpick..."
  echo "pkpick 2 2 8 0 0 4 1 0 1 64" >> $uservnmr/global
  echo "1 \"\"" >> $uservnmr/global
  echo "0" >> $uservnmr/global
fi

if test `grep -c wysiwyg $uservnmr/global` -eq 0
then
  echo -n "wysiwyg..."
  echo "wysiwyg 2 2 8 0 0 4 1 0 1 64" >> $uservnmr/global
  echo "1 \"y\"" >> $uservnmr/global
  echo "2 \"n\" \"y\"" >> $uservnmr/global
fi

if test `grep -c dsp $uservnmr/global` -eq 0
then
  echo -n "dsp..."
  echo "dsp 2 2 8 0 0 2 1 9 1 64" >> $uservnmr/global
  echo "1 \"n\"" >> $uservnmr/global
  echo "3 \"i\" \"r\" \"n\"" >> $uservnmr/global
fi

if test `grep -c probe_protection $uservnmr/global` -eq 0
then
  echo -n "probe_protection..."
  echo "probe_protection 2 2 8 0 0 2 1 0 1 64" >> $uservnmr/global
  echo "1 \"y\"" >> $uservnmr/global
  echo "2 \"n\" \"y\"" >> $uservnmr/global
fi
echo ""

# .login 
echo -n "Adding definitions to .login file..."
if test `grep -c vnmreditor $userhome/.login` -eq 0
then
  echo -n "vnmreditor..."
  awk '{ print
         if (($1=="setenv") && ($2=="vnmrsystem")) {
          printf "setenv vnmreditor vi\n" }
  }' < $userhome/.login > /vnmr/tmp/.login
  mv /vnmr/tmp/.login $userhome/.login
fi

if test `grep -c glide $userhome/.login` -eq 0
then
  echo -n "glide..."
  awk '{ print
         if (($1=="setenv") && ($2=="vnmrsystem")) {
          printf "if -f $vnmrsystem/glide/vnmrmenu then\n"
          printf "  setenv vnmrmenu $vnmrsystem/glide/vnmrmenu\n"
          printf "endif\n" }
  }' < $userhome/.login > /vnmr/tmp/.login
  mv /vnmr/tmp/.login $userhome/.login
fi

if test `grep -c BROWSERDIR $userhome/.login` -eq 0
then
  echo -n "BROWSERDIR..."
  awk '{ print
         if (($1=="setenv") && ($2=="memsize")) {
          printf "setenv BROWSERDIR $home/ib_initdir\n" }
  }' < $userhome/.login > /vnmr/tmp/.login
  mv /vnmr/tmp/.login $userhome/.login
fi

echo ""

# .xlogin 
echo -n "Adding definitions to .xlogin file..."
if test `grep -c vnmreditor $userhome/.xlogin` -eq 0
then
  echo -n "vnmreditor..."
  awk '{ print
         if (($1=="setenv") && ($2=="vnmrsystem")) {
          printf "setenv vnmreditor vi\n" }
  }' < $userhome/.xlogin > /vnmr/tmp/.xlogin
  mv /vnmr/tmp/.xlogin $userhome/.xlogin
fi

if test `grep -c glide $userhome/.xlogin` -eq 0
then
  echo -n "glide..."
  awk '{ print
         if (($1=="setenv") && ($2=="vnmrsystem")) {
          printf "if -f $vnmrsystem/glide/vnmrmenu then\n"
          printf "  setenv vnmrmenu $vnmrsystem/glide/vnmrmenu\n"
          printf "endif\n" }
  }' < $userhome/.xlogin > /vnmr/tmp/.xlogin
  mv /vnmr/tmp/.xlogin $userhome/.xlogin
fi
echo ""

# .Xdefaults 
echo -n "Adding definitions to .Xdefaults file..."
if test `grep -c errorLines $userhome/.Xdefaults` -eq 0
then
  echo -n "*VNMR*errorLines..."
  awk '{ print
         if ($1=="*VNMR*fontColor:") {
          printf "*VNMR*errorLines:\t3\n" }
  }' < $userhome/.Xdefaults > /vnmr/tmp/.Xdefaults
  mv /vnmr/tmp/.Xdefaults $userhome/.Xdefaults
fi

if test `grep -c "button\*font" $userhome/.Xdefaults` -eq 0
then
  echo -n "*VNMR*button*font..."
  awk '{ print
         if ($1=="*VNMR*font:") {
          printf "*VNMR*button*font:\t9x15bold\n" }
  }' < $userhome/.Xdefaults > /vnmr/tmp/.Xdefaults
  mv /vnmr/tmp/.Xdefaults $userhome/.Xdefaults
fi

if test `grep -c "olwm\.MinimalDecor" $userhome/.Xdefaults` -eq 0
then
  echo -n "olwm.MinimalDecor..."
  awk '{ print
         if ($1=="Scrollbar.JumpCursor:") {
          printf "olwm.MinimalDecor:\tVNMR\n" }
  }' < $userhome/.Xdefaults > /vnmr/tmp/.Xdefaults
  mv /vnmr/tmp/.Xdefaults $userhome/.Xdefaults
fi

echo ""

# New additions
if test -f /vnmr/user_templates/Acqi
then
  updateAcqi=y
  if test -f $userhome/Acqi
  then
    if test `diff /vnmr/user_templates/Acqi $userhome/Acqi | wc -l` -gt 0
    then
      echo "Acqi definitions file exists and is different than the new one."
      echo -n "  Replace it with the new one? [y] "
      read answer
      testanswer=X$answer
      if test $testanswer != "X"
      then
        updateAcqi=$answer
      fi
    else
      updateAcqi=n
    fi
  fi
  if test $updateAcqi = "y"
  then
    echo "Adding Acqi definitions file..."
    cp /vnmr/user_templates/Acqi $userhome
  fi
fi

if test -d /vnmr/user_templates/ib_initdir 
then
  updateIB=y
  if test -d $userhome/ib_initdir
  then
    echo "Image Browser definitions directory exists."
    echo -n "  Replace it with the new one? [y] "
    read answer
    testanswer=X$answer
    if test $testanswer != "X"
    then
      updateIB=$answer
    fi
  fi
  if test $updateIB = "y"
  then
    echo "Adding Image Browser definitions file..."
    cp -r /vnmr/user_templates/ib_initdir $userhome
  fi
fi
echo "Done!"

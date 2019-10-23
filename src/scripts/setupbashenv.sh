#!/bin/bash
#
# script to modify the .profile and .bashrc to source the
# .vnmrenvbash file
# .profile is read for the desktop and non-interactive shell
#    important for OpenVnmrJ adm use and for desktop icon to be able to
#    start OpenVnmrJ
#  
# .bashrc is read for interactive bash shell
#
#
#set -x

if [ "x$vnmrsystem" = "x" ]
then
   vnmrsystem="/vnmr"
fi
userpath=$1
if [ -z $userpath ] ; then
  userpath=$HOME
fi 

# if path valid then proceed with checking/modifing .profile & .bash_profile
if [ -d  $userpath ] 
then
   if [ -f $userpath/.bash_profile ] ; then
      envFound=$(grep vnmrenvbash $userpath/.bash_profile)
      if [ -z "$envFound" ] ; then
         cat "$vnmrsystem/user_templates/profile" >> $userpath/.bash_profile
      fi
   elif [ -f $userpath/.profile ] ; then
      envFound=$(grep vnmrenvbash $userpath/.profile)
      if [ -z "$envFound" ] ; then
         cat "$vnmrsystem/user_templates/profile" >> $userpath/.profile
      fi
   fi
   if [ ! -f $userpath/.vnmrenvbash ] ; then
      cp $vnmrsystem/user_templates/.vnmrenvbash $userpath/.
   fi
else
  echo "Invalid user path: $userpath"
fi

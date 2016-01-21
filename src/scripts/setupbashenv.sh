: /bin/sh
#
# script to modify the .profile and .bashrc to source the
# .vnmrenvbash file
# .profile is read for the desktop and non-interactive shell
#    important for VnmrJ adm use and for desktop icon to be able to
#    start VnmrJ
#  
# .bashrc is read for interactive bash shell
#
#    Author:  Greg Brissey    5/06/2009
#
#set -x

SUDO="/usr/bin/sudo"
ED="/bin/ed"

userpath=$1
if [ -z $userpath ] ; then
  echo "Enter user path: "
  read userpath
fi 

# if path valid then proceed with checking/modifing .profile & .bashrc
if [ -d  $userpath ] 
then
#   echo "cd: $userpath"

#  $SUDO cd $userpath ;  won't work because cd is a builtin shell cmd

   cd $userpath ;

#
# test if the .profile has already has been modified
# if not then append the text to the end of the file
#
profilefind=`grep .vnmrenvbash .profile`
if [ -z "$profilefind" ] ; then
   $SUDO $ED -s ".profile" <<+
a
#
# source Varian VnmrJ envs for the desktop
#
if [ -f "\$HOME/.vnmrenvbash" ]; then
    . "\$HOME/.vnmrenvbash"
fi

.
w
q
+
fi

#
# test if the .bashrc has already has been modified
# if not then append the text to the end of the file
#
bashrcfind=`grep .vnmrenvbash .bashrc`
if [ -z "$bashrcfind" ] ; then
   $SUDO $ED -s ".bashrc" <<+
a
#
# source the VnmrJ envs for interactive bash/sh shells
#
if [ -f "\$HOME/.vnmrenvbash" ]; then
    . "\$HOME/.vnmrenvbash"
fi

.
w
q
+
fi

else
  echo "Invalid user path: $userpath"
fi

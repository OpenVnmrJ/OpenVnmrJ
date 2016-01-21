#!/bin/sh

#---------------------------------------------------------------
#Usage:  vnmremail <-m> <"-s subject"> "filename" address
#  without -m option the file specified by filename
#  is sent after uuencode.  A directory will be 
#  zipped.
#  with -m option the file will be cat'ed to the body
#  of the email
#  multiple files/directories can be attached by supplying
#  all of them as "arg1" - each seperated by a space
#---------------------------------------------------------------

echo=`which echo`
MAIL=mailx
if [ `uname` = Linux ]; then
  MAIL=mail
  echo="$echo -e"
fi
tmp1=/vnmr/tmp/emailtmp.$$
tmp2=/vnmr/tmp/emailtmp2.$$

if [ $# -lt 1 ]; then
  $echo "Usage: vnmremail \"arg1\" \"arg2\""
  $echo "   arg1 is filenames/dirnames"
  $echo "   arg2 is email addresses"
  $echo "   Multiple filenames within arg1 seperated by a blank space"
  $echo "   Multiple eaddress within arg2 seperated by a blank space"
  exit
fi

subject=''
catfile=uuencode
if [ `$echo $1 | awk '{print $1}'` = "-m" ]; then
   catfile=cat
   shift
fi
if [ `$echo $1 | awk '{print $1}'` = "-s" ]; then
   subject=`$echo $1 | awk '{print $2}'`
   shift
fi

Files=$1
eaddress=$2

curdir=`pwd` 
for subfiles in $Files
do
  if [ -d $subfiles ]; then
    catfile=uuencode
  fi
done
mailarg=""
if [ $catfile = uuencode ]; then
  if [ `which mutt | wc -w` -eq 1 ]; then
    catfile=mutt
    MAIL=mutt
    if [ -f $HOME/.muttrc ]; then
      muttrc=$HOME/.muttrc
    else
      cmddir=`dirname $0`
      muttrc=${cmddir}/muttrc
      if [ ! -f $muttrc ]; then
        muttrc=/vnmr/bin/muttrc
      fi
    fi
    if [ -f $muttrc ]; then
      MAIL="$MAIL -F $muttrc"
    fi
  fi
fi

rm -rf $tmp1
rm -rf $tmp2

Uname=`uname -n`
if [ "x$subject" = "x" ]; then
    systemname=$Uname
else
    systemname="$Uname ($subject)"
fi

tmpfiles=""
mailarg=""

if [ "$catfile" = "cat" ]; then

  for subfiles in $Files
  do
     if [ `$echo $Files | wc -w` -gt 1 ]; then
       $echo " " >> $tmp1
       $echo "*********************************************" >> $tmp1
       $echo "     $subfiles" >> $tmp1
       $echo "*********************************************" >> $tmp1
     fi
     cat $subfiles >> $tmp1
  done
  touch $tmp2

else

  $echo " " > $tmp1
  $echo "***************************************************** " >> $tmp1
  $echo "DO NOT REPLY TO THIS SYSTEM GENERATED EMAIL  " >> $tmp1
  $echo "NO ONE WILL READ YOUR REPLY!!!!             " >> $tmp1
  $echo "Contact your administrator, if you have questions   " >> $tmp1

  $echo " " >> $tmp1

  if [ `$echo $Files | wc -w` -gt 1 ]; then
      $echo "Requested Files are enclosed" >> $tmp1
  else
	$echo "Requested File is enclosed" >> $tmp1
  fi
  $echo " " >> $tmp1
  $echo "****************************************************" >> $tmp1

  for subfiles in $Files
  do
     dir=`dirname $subfiles`
     Name=`basename $subfiles`
     cd $dir
     if [ -d $Name ]; then
	zip -ryq /vnmr/tmp/$Name.zip $Name
#	tar cf - $Name | gzip > /vnmr/tmp/$Name.tgz
#	Name="$Name.tgz"
	Name="$Name.zip"
     else
# Do not make a copy if the file was sent in the /vnmr/tmp directory
	if [ "$subfiles" != "/vnmr/tmp/$Name" ]; then
	    cp $Name /vnmr/tmp
	fi
     fi
     cd /vnmr/tmp
     if [ $catfile = "uuencode" ]; then
        uuencode $Name $Name > $tmp2
        cat $tmp2 >> $tmp1
        rm -rf $Name
     else
        mailarg="$mailarg -a /vnmr/tmp/$Name"
        tmpfiles="$tmpfiles /vnmr/tmp/$Name"
     fi
     cd $curdir
  done

fi


for subadd in $eaddress
do
   if [ $catfile = "uuencode" ]; then
      cd /vnmr/tmp
      $MAIL -s "message from $systemname" $subadd < $tmp1
      cd $curdir
   else
     if [ $catfile = "mutt" ]; then
      $MAIL -s "message from $systemname" $mailarg -- $subadd < $tmp1
     else
      $MAIL -s "message from $systemname" $mailarg $subadd < $tmp1
     fi
   fi
done
rm -rf $tmp2 $tmp1
rm -rf $tmpfiles

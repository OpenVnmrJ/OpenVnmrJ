: '@(#)loadvnmr.sh 22.1 03/24/08 1991-1996 '
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
: /bin/sh

common_env() {

#  ostype:  IBM: AIX  Sun: SunOS or SOLARIS  SGI: IRIX
#  the system this script is running on defines the ostype
#  vnmros has a similar meaning, except the tape defines it

    ostype=`uname -s`
    if [ x$ostype = "xIRIX64" ]
    then
       ostype="IRIX"
    fi
    vnmros="SunOS"
    if (test -f VnmrI_5.1a)
    then
	vnmros="AIX"
    else if (test -f VnmrSGI_5.1a)
	 then
	     vnmros="IRIX"
         else if (test -f VnmrSOL_5.1a)
             then
                 vnmros="SOLARIS"
             fi
         fi
    fi

    if [ x$ostype = "xAIX" ]
    then
        osver=`uname -v`.`uname -r`
        sysV="y"
        ktype="AIX"
        atype="ibm"
        user=`whoami`
    else if [ x$ostype = "xIRIX" ]
	 then
             osver=`uname -r`
             sysV="y"
             ktype="IRIX"
             atype="sgi"
             user=`whoami`
         else
             osver=`uname -r`
             osmajor=`echo $osver | awk 'BEGIN { FS = "." } { print $1 }'`
             atype=`uname -m | awk '
                 /^sun4/ { print "sun4" }
                 /^sun3/ { print "sun3" }
             '`
             if test $osmajor = "5"
             then
           	sysV="y"
                ostype="SOLARIS"
                ktype="solaris"
             else
          	sysV="n"
                ktype=`uname -m`
             fi
#  get the user name according to the protocol

	     case $osver in
	      4*) user=`whoami`
		  ;;
	      5*) user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
		  ;;
	     esac
	 fi
    fi
}

nnl_echo() {
    if test x$sysV = "x"
    then
        echo "error in echo-no-new-line: sysV not defined"
        exit 1
    fi

    if test $sysV = "y"
    then
        if test $# -lt 1
        then
            echo
        else
            echo "$*\c"
        fi
    else
        if test $# -lt 1
        then
            echo
        else
            echo -n $*
        fi
    fi
}

change_procpar() {
 
   for proc in $nproc
   do
      cp $proc proc_tmp
      cat proc_tmp | sed 's/4 "a" "n" "s" "y"/2 "n" "y"/' | \
        sed 's/9 "c" "f" "g" "m" "p" "r" "u" "w" "x"/5 "c" "f" "n" "p" "w"/' > $proc
   done
   rm proc_tmp 
 
}

cmdname=`basename $0`
common_env

if [ x$ostype != x$vnmros ]
then
   echo " $cmdname must be run in the $vnmros system "
   exit 1
fi

if [ x$user != "xroot" ]
then
   echo "$cmdname must be run by root"
   exit 1
fi

#  Do not verify OS version
#  prevents annoying problems with 4.1.1_IPX, 4.1.3C, etc.

if ( test $ktype != $atype )
then
    if ( test  "(" $ktype != "sun4c" -a  $ktype != "sun4m" -a $ktype != "solaris" ")" )
    then
        ktype="none"
    fi
fi

#  Now get display type from architecture type

case x$atype in
   xsun3 )
        if ( test $ktype = "sun3x" )
	then
            dtype="Sun-3/80"
        else
            dtype="Sun-3"
        fi
	;;
   xsun4 )
        if ( test $ktype = "sun4c" )
	then
            dtype="Sun-4c (SPARCstation)"
        elif (test $ktype = "sun4m" )
        then
            dtype="Sun-4m (SPARCstation)"
        else
            dtype="Sun-4"
        fi
	;;
   xibm )
        dtype="IBM"
	;;
   xsgi )
        dtype="SGI"
	;;
     *)
        echo "$ostype system type not supported.";
        exit 1;
	;;
esac

echo ""
echo "  During the loadvnmr sequence, the system asks several questions."
echo "  The questions often provide the possible responses enclosed in"
echo "  parentheses. The default response is enclosed in square brackets."
echo "  Pressing the Enter key selects the default response."
echo ""


test4acq="n"
if [ x$ostype = "xSunOS" -o x$ostype = "xSOLARIS" ]
then
    test4acq="y"
    pid=$$
    if [ x$sysV = "xy" ]
    then
        npids=`ps -ef | grep -v awk | awk '/Acqproc/ { printf( "%d ", $2) }' $*`
    else
        npids=`ps axc | awk '/Acqproc/ { printf("%d ",$1) }' $*`
    fi

    for acqpid in $npids
    do
        nnl_echo "An Acquisition process is running. Kill Acqproc ? ( y or n ) [y]: "
        read a
        if [ x$a != "xn" ]
        then
            if [ x$user = "xroot" ]
            then
                echo "Killing Acqproc."
                kill -2 $acqpid
                sleep 5		# give time for kill message to show up.
            else
                echo "Not root, login as root if you wish to kill Acqproc."
            fi
        fi
    done
fi

#

echo " "
echo "System Architecture is a $dtype running $ostype Version $osver."
echo " "
if [ $ktype = "none" -a $vnmros = "SunOS" ] 
then
    echo "No kernel programs (required for use as a spectrometer)"
    echo "will be loaded, because this is a `uname -m`"
    echo " "
fi

ctype=$atype
if [ $ostype = "SOLARIS" ]
then
    ctype="sol"
fi

a="y"
if ( test x$vnmros = "xSunOS" )
then
    nnl_echo "Load Varian Software for this architecture/$vnmros? (y or n) [y]: "
    read a
fi

if ( test x$a = "xn" )
then
    echo " "
    nnl_echo "Specify Architecture required ( 4, 4c or 4m ) [4c]? "
    read a
    case x$a in
        x4 )
            atype="sun4"
            ktype="sun4"
            ;;
        x4c )
            atype="sun4"
            ktype="sun4c"
            ;;
        x4m )
            atype="sun4"
            ktype="sun4m"
            ;;
	x)
            atype="sun4"
            ktype="sun4c"
	    ;;
        *)
            echo "Only Sun-4, Sun-4c or Sun-4m system types supported."
            exit 1;
            ;;
    esac
    default="n"
else
    default="y"
fi

uplus="n"
if [ x$test4acq = "xy" ]
then
    echo " "
    nnl_echo "Is this a UNITYplus spectrometer (y or n) [y]: "
    read uplus
    if ( test x$uplus != "xn" )
    then
	uplus="y"
    fi
    if [ $ostype = "SOLARIS" -a x$uplus != "xy" ]
    then
       echo " "
       nnl_echo "Is this a GEMINI 2000 spectrometer (y or n) [y]: "
       read g2000
       if ( test x$g2000 != "xn" )
       then
	   g2000="y"
           mv sol.toc uni.toc
           mv common.toc unicommon.toc
           mv gem.toc sol.toc
           mv gemcommon.toc common.toc
       fi
    else
       g2000="n"
    fi

    if ( test $ostype = "SOLARIS" )
    then
      ktype="none"
    else
      echo " "
      nnl_echo "Load software necessary for use as a spectrometer (y or n) [y]: "
      read a
      if ( test x$a != "xn" )
      then
        if ( test $default != "y" )
        then
	    if ( test $ktype = "sun4m" )
	    then
	        echo "Note: For a Sun-4m only the 4.1.3 and the 4.1.4 kernel are supported."
            fi
            echo " "
            echo "Specify SunOS required"
            echo "   1)  4.1.1"
            echo "   2)  4.1.2"
            echo "   3)  4.1.3"
            echo "   4)  4.1.4"
            nnl_echo "Enter 1, 2, 3 or 4: "
            read a
            case x$a in
                x1 )
                    osver="4.1.1"
                    ;;
                x2 )
                    osver="4.1.2"
                    ;;
                x3 )
                    osver="4.1.3_U1"
                    ;;
                x4 )
                    osver="4.1.4"
                    ;;
                *)
                    echo "You must enter a 1, 2 or a 3"
                    if ( test x$g2000 = "xy" )
                    then
                       mv sol.toc gem.toc
                       mv common.toc gemcommon.toc
                       mv uni.toc sol.toc
                       mv unicommon.toc common.toc
                    fi
                    exit 1
                    ;;
            esac
        else

# If default is yes, use osver as determined automatically by this script
# not-solaris is equivalent to SunOS.  For SunOS 4.1.3 and its flavors,
# substitute 4.1.3_U1 for the current osver.  The script variable is
# only used to compute the name of the kernel archive.  For SunOS 4.1.3
# we support all flavors with the final version of the kernel.

           osver_2=`echo $osver | awk '{ print substr( $0, 1, 5 ) }'`
           if ( test $osver_2 = "4.1.3" )
           then
               osver="4.1.3_U1"
           fi
        fi
      else
        ktype="none"
      fi
   fi
fi

#  getchoices is a C program.  The script must select the correct
#  (architecture) version to run.  The atype variable is not valid;
#  the user may be loading for an alternate architecture.

./getchoices.${ctype} ${ctype} $cmdname

if [ $ostype = "AIX" ]
then
    tdrive="rmt0"
    ndrive="rmt0.1"
    tapecmd="tctl -f"
    tapeop=xvfB
    blksize=""
else if [ $ostype = "IRIX" ]
     then
        tdrive="tapens"
        ndrive="nrtapens"
	tapecmd="mt -t"
	tapeop=xvfb
	blksize=20
     else if [ $ostype = "SOLARIS" ]
        then
    	   tdrive="rmt/0mb"
	   ndrive="rmt/0mbn"
	   tapecmd="mt -f"
	   tapeop=xvf
	   blksize=""
        else
    	   tdrive="rst8"
	   ndrive="nrst8"
	   tapecmd="mt -f"
	   tapeop=xvfb
	   blksize=2000
        fi
     fi
fi

TAPE_LOC=""
REMOTE_HOST="NA"
REMOTE=0
while [ "x$TAPE_LOC" =  "x" ]
do
    echo ""
    nnl_echo "Enter Drive Location (local or remote) [local]: "
    read TAPE_LOC
    case "$TAPE_LOC" in
        r*) 
            echo ""
            nnl_echo "Enter hostname of remote tape drive: "
            read REMOTE_HOST
            echo ""
            nnl_echo "Login name for $REMOTE_HOST [vnmr1]: "
            read REMOTE_LOG
	    if [ x$REMOTE_LOG = "x" ]
 	    then
		REMOTE_LOG=vnmr1
	    fi
            REMOTE=1
	    rsh $REMOTE_HOST -l $REMOTE_LOG -n "echo 0 > /dev/null"
            if [ "$?" -ne 0 ]
            then
               echo "$cmdname : Problem with reaching remote host $REMOTE_HOST"
               if ( test x$g2000 = "xy" )
               then
                  mv sol.toc gem.toc
                  mv common.toc gemcommon.toc
                  mv uni.toc sol.toc
                  mv unicommon.toc common.toc
               fi
               exit 1
            fi
	    rtype=`rsh $REMOTE_HOST -l $REMOTE_LOG  uname -s`
	    if [ x$rtype = "xIRIX64" ]
	    then
		rtype="IRIX"
	    fi
	    if [ x$rtype = "xSunOS" ]
	    then
	       rosver=`rsh $REMOTE_HOST -l $REMOTE_LOG  uname -r`
	       case "x$rosver" in
		x4*)
		    ;;
		x*)
		    rtype="SOLARIS"
		    ;;
	       esac
	    fi
	    if [ x$ostype != "xAIX" ]
	    then
	       tapeop="B"$tapeop
	    fi
            if [ x$rtype = "xAIX" ]
            then
               tapecmd="tctl -f"
               tdrive="rmt0"
               ndrive="rmt0.1"
            else if [ x$rtype = "xIRIX" ]
                 then
                     tapecmd="mt -t"
                     tdrive="tapens"
                     ndrive="nrtapens"
                 else if [ x$rtype = "xSOLARIS" ]
		     then
                         tapecmd="mt -f"
                         tdrive="rmt/0mb"
                         ndrive="rmt/0mbn"
		     else
                         tapecmd="mt -f"
                         tdrive="rst8"
                         ndrive="nrst8"
                     fi
                 fi
            fi;;
        l*) ;;
        *)
           TAPE_LOC="local" ;;
    esac
done

echo " "
nnl_echo "Which tape drive for loading? [$tdrive]: "
read a
if ( test ! x$a = "x")
then
      tdrive=$a
      case $tdrive in
        rst*)
            ndrive="n"$tdrive ;;
        rmt*)
	    if ( test x$ostype = "xSOLARIS" -o \
                 "(" x$TAPE_LOC != "xlocal" -a x$rtype = "xSOLARIS" ")" )
            then
                ndrive=$tdrive"n"
            else
                ndrive=$tdrive".1"
            fi
            ;;
        tape*)
            ndrive="nr"$tdrive ;;
        *)
            echo " "
            nnl_echo "Which non-rewind tape drive for loading? [$ndrive]: "
            read a
            if ( test ! x$a = "x")
            then
                ndrive=$a
            fi ;;
      esac
fi
tapedrive="/dev/"$tdrive
ntapedrive="/dev/"$ndrive

#  export some variables for finish_load use

nmr_adm="vnmr1"

export atype ctype ktype osver uplus
export REMOTE_HOST REMOTE REMOTE_LOG
export tapedrive ntapedrive tapecmd tapeop blksize
export ostype vnmros nmr_adm

if [ x$ostype = "xIRIX" ]
then
   ./makevnmr1 $nmr_adm nmr /usr/people
else
   if [ x$ostype = "xSOLARIS" ]
   then
       ./makevnmr1 $nmr_adm nmr /export/home
   else
       ./makevnmr1 $nmr_adm nmr /home
   fi
fi
./finish_load
if ( test $? -ne 0 )
then
    if ( test x$g2000 = "xy" )
    then
       mv sol.toc gem.toc
       mv common.toc gemcommon.toc
       mv uni.toc sol.toc
       mv unicommon.toc common.toc
    fi
    echo "Load VNMR process aborted"
    exit 1
fi
rm makevnmr1
rm getchoices.*
rm *.toc

if ( test x$uplus != "xy" -a x$g2000 != "xy"  )
then
   nnl_echo "  9"
   cp user_templates/global user_templates/tmp
   cat user_templates/tmp | sed 's/lockgain 1 1 48/lockgain 1 1 70/' | \
       sed 's/lockpower 1 1 68/lockpower 1 1 63/' > user_templates/global
   rm user_templates/tmp
   nproc=`ls par??0/stdpar/*/procpar`
   change_procpar
   nproc=`ls par??0/tests/*/procpar`
   change_procpar
   nproc=`ls parlib/*/procpar`
   change_procpar
   nproc=`ls fidlib/*/procpar`
   change_procpar
fi
if ( test x$g2000 = "xy" )
then
   nnl_echo "  9"
   cp user_templates/global user_templates/tmp
   cat user_templates/tmp | sed 's/lockgain 1 1 48 0 1/lockgain 1 1 30 0 10/' | \
       sed 's/lockpower 1 1 68 0 1/lockpower 1 1 40 0 1/' > user_templates/global
   rm user_templates/tmp
   nproc=`ls fidlib/*/procpar`
   change_procpar
   if (test -f glide/adm/public.env)
   then
      cp  glide/adm/public.env glide/adm/public
      cat glide/adm/public | sed 's/unity/gem/' > glide/adm/public.env
      rm glide/adm/public
   fi
fi
./makevnmr2
rm makevnmr2

# special stuff for Solaris
# "install" GNU C (for seqgen, etc) by creating a symbolic link
# between /vnmr/gnu/cygnus-sol2-2.0 (the /vnmr/gnu directory has
# only 1 entry, the GNU C compiler directory in it) and /opt
#
# GNU C is considered already installed if the cygnus* entry is
# present in /opt, is a directory and is not a symbolic link


if (test x$ostype = "xSOLARIS")
then
   if (test -d gnu)
   then
      cwd=`pwd`
      gnudir=`(cd gnu; echo cygnus*)`
      (cd /opt; if (test -d $gnudir -a ! -h $gnudir); then		\
         echo "GNU C compiler in /opt/$gnudir already installed";	\
         echo "Ignoring version loaded with VNMR";                      \
       else								\
         rm -f $gnudir;							\
         ln -s $cwd/gnu/$gnudir $gnudir;				\
       fi)
   fi
fi
#

rm $cmdname
echo "VNMR Software Load Complete."

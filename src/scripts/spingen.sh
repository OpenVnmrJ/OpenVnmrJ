: '@(#)spingen.sh 22.1 03/24/08 1999-2002 '
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
#
#*********************************************************************
#** spingen:  shell script for performing pulse sequence generation **
#**          at the UNIX level.   (Bourne shell)                    **
#**                                                                 **
#**          The pulse sequence file to be compiled is supplied as  **
#**          the argument to the seqgen shell script, e.g.,         **
#**          "spingen test".                                        **
#*********************************************************************

if [ $# != 0 ]; then

echo " "
echo " "
echo "Beginning SpinCAD Pulse Sequence Compilation Process:"
echo " "

# Check to establish USER and LOGNAME equivalence
# (Prevents problems with one user using UNIX command "su"
# to become some other user)

if [ ! $USER = $LOGNAME ]; then
   echo "Username: "$USER"  and Logname: "$LOGNAME"  do not agree."
   echo ""
   exit
fi

osmajor=`uname -r | awk 'BEGIN { FS = "." } { print $1 }'`
ostype=`uname -s`
# Test for Solaris (SunOS, but not SunOS 4.X) and IRIX; they use a version
# of basename which interprets the 2nd argument as a Regular Expression
# vnmr_uname is our script that emits SunOS, SOLARIS, IRIX or AIX.

if test `vnmr_uname` = "SunOS"
then
    svr4="n"
else
    svr4="y"
fi


#  Check for USER directory for compiled pulse sequences (SEQLIB). If
#  it is not present, create it.

if  [ ! -d "$vnmruser"/seqlib ]; then
   mkdir "$vnmruser"/seqlib
fi

# Check for USER pulse sequence directory (PSGLIB)

if [ ! -d "$vnmruser"/spincad/psglib ]; then
   mkdir -p "$vnmruser"/spincad/psglib
   if [ ! -d "$vnmruser"/spincad/psglib -o ! -d "$vnmruser"/seqlib ]; then
      echo "Cannot create user's 'psglib' and/or 'seqlib' Directories."
      exit
   fi
fi

# Check for USER spincad info directory (INFO)

if [ ! -d "$vnmruser"/spincad/info ]; then
   mkdir "$vnmruser"/spincad/info
   if [ ! -d "$vnmruser"/spincad/info ]; then
      echo "Cannot create user's 'spincad/info' directory."
      exit
   fi
fi

# Check for USER spincad classes directory (CLASSES)

if [ ! -d "$vnmruser"/spincad/classes ]; then
   mkdir "$vnmruser"/spincad/classes
   if [ ! -d "$vnmruser"/spincad/classes ]; then
      echo "Cannot create user's 'spincad/info' directory."
      exit
   else
      cp /vnmr/spincad/classes/* "$vnmruser"/spincad/classes/
   fi
fi


# options for psg only or dps only

dpsFlag=1
psgFlag=1
allFlag=0

# -dps option: compile only dps version
if [ $1 = "-dps" ]; then
  psgFlag=0
  file=`basename $2`
  if [ $# != 1 ]; then
     shift
  else
     echo "spingen: no pulse sequence name argument provided"
     exit
  fi
fi

# -psg option: compile only go version
if [ $1 = "-psg" ]; then
  dpsFlag=0
  file=`basename $2`
  if [ $# != 1 ]; then
    shift
  else
    echo "spingen: no pulse sequence name argument provided"
    exit
  fi
fi

# -all option: compile dps, go and also any loaded Java classes
if [ $1 = "-all" ]; then
  allFlag=1
  psgFlag=1
  dpsFlag=1
  file=`basename $2`
  if [ $# != 1 ]; then
     shift
  else
     echo "spingen: no pulse sequence name argument provided"
     exit
  fi
fi


#**********************************************************
#**  Compile all sequences given as arguments in a loop  **
#**********************************************************

   psglib="$vnmruser"/spincad/psglib
   seqlib="$vnmruser"/seqlib

 while [ $# != 0 ]; do

   # get list of pulse sequence file args
   file=`basename $1`

   cd "$vnmruser"/spincad/psglib
   if [ ! -f $file ]; then
      if [ -f "$vnmrsystem"/spincad/psglib/$file ]; then
         cp "$vnmrsystem"/spincad/psglib/$file "$psglib"
         echo "File $file found in $vnmrsystem/spincad/psglib;"
         echo "    compiling a local copy in $psglib"
      fi
   fi


   if [ ! -f "$psglib"/$file ]; then

      echo "File $file not found in $psglib or $vnmrsystem/spincad/psglib"
      echo ""			# move on to the next file in the arg list
   else

      echo "Compiling SpinCAD sequence $file  ..."

      compiledOK=1
      # If present, remove file for error and warning messages (ERRMSG)

      if [ -f $file.error ]; then
         rm $file.error
      fi


      # Following line commented out
      # If present, remove compiled dps pulse sequence from user seqlib
      # if [ -f $seqlib/$file.psg ]; then
      #    rm $seqlib/$file.psg
      # fi
      # if [ -f $seqlib/${file}_dps.psg ]; then
      #    rm $seqlib/${file}_dps.psg
      # fi

      # remove the source files , message and error files from seqlib
      rm -f "$seqlib"/${file}.spincad
      rm -f "$seqlib"/${file}.error
      rm -f "$seqlib"/${file}.msg%
      rm -f "$seqlib"/${file}_dps.msg%


      # Compile the spincad sequence

     if [ $psgFlag = 1 ]; then
      
      rm -f "$seqlib"/$file.psg
      "$vnmrsystem"/tcl/bin/spingen  $file > "$seqlib"/${file}.msg%

      # Check for compilation result.
      if [ -f "$seqlib"/${file}.psg ]; then

         # Check to see if the file ERRMSG contains any text.
         if [ -s "$seqlib"/$file.error ]; then
            echo ""
            echo "Pulse Sequence $file did compile but may not function properly."
            echo ""
            echo "The following comments can also be found in the"
            echo "file $seqlib/$file.error:"
            echo ""
            echo "<<-----  detailed compile messages & warnings  ----->>"
            cat "$seqlib"/$file.error
            echo "<<-----  end  of  compile messages & warnings  ----->>"
            echo ""
         else
            echo "Pulse Sequence $file  go version compiled."
            rm -f "$seqlib"/${file}.error
         fi
         cp "$psglib"/$file "$seqlib"/${file}.spincad

         # now delete the C error/warn message file, so user knows which sequence was run
         rm -f "$seqlib"/${file}.msg%

      else

         compiledOK=0
         echo ""
         echo "Pulse Sequence $file did not compile."
         if [ -f "$seqlib"/${file}.error ]; then
           echo "The following errors can also be found in the"
           echo "file $seqlib/$file.error:"
           echo ""
           cat "$seqlib"/$file.error
           echo ""
           echo "<<-----  detailed compile error messages: ----->>"
           cat "$seqlib"/${file}.msg%
           echo "<<-----  end  of  compile error messages  ----->>"
         fi
         echo " "

      fi

     fi
     # end psgFlag if



      # Compile the spincad dps sequence

     if [ $dpsFlag = 1 ]; then

      rm -f "$seqlib"/${file}_dps.psg
      "$vnmrsystem"/tcl/bin/spingen -dps $file > "$seqlib"/${file}_dps.msg%

      # Check for output dps psg file
      # If no _dps.psg file this means the
      # dps  Pulse Sequence failed to compile
      # recompile only the normal sequence

      if [ ! -f "$seqlib"/${file}_dps.psg ] ; then
        compiledOK=0
        echo "Compile with DPS extensions failed"
        if [ -f "$seqlib"/${file}_dps.error ]; then
          cat "$seqlib"/${file}_dps.error
        fi
        if [ -f "$seqlib"/${file}_dps.msg% ]; then
          echo " "
          echo "<<-----  detailed dps compile error messages: ----->>"
          cat "$seqlib"/${file}_dps.msg%
          echo "<<-----  end  of  dps compile error messages  ----->>"
          echo " "
        fi
      else
        echo "Pulse Sequence $file dps version compiled."
        rm -f "$seqlib"/${file}_dps.msg%
      fi

     fi
     # above fi ends the dpsFlag if


     compiledJOK=1

     # if both compilations successful? check for any loaded java programs to be compiled
     if [ $allFlag = 1 -a $compiledOK = 1 ]; then

        javalist=`awk ' /<psgLoadUserMethod>/,/<\/psgLoadUserMethod>/ { print } ' "$seqlib"/${file}.psg \
            | awk ' /<jload>/,/<\/jload>/ { print } ' | awk ' { for ( i = 2; i < NF; ++i ) print $i } ' ` 

        for jf in $javalist 
        do
        if [ x$svr4 = "xy" ]; then
            jf=`basename $jf \\\\.java`
            jf=`basename $jf \\\\.class`
        else
            jf=`basename $jf .java`
            jf=`basename $jf .class`
        fi

        # check for the .java and .class file in user and system directories and do appropriate copying
        okToDelete=0
        okToCompile=1
        if [ -f "${vnmruser}"/spincad/classes/"${jf}".java ]; then
          okToDelete=1
        else

          if [ -f /vnmr/spincad/classes/"${jf}".java ]; then
            cp /vnmr/spincad/classes/"${jf}".java "${vnmruser}"/spincad/classes/
            echo "copied ${jf}.java from /vnmr/spincad/classes to ${vnmruser}/spincad/classes"
            okToDelete=1
          else
            okToCompile=0
            if [ ! -f "${vnmruser}"/spincad/classes/"${jf}".class ]; then
               if [ -f /vnmr/spincad/classes/"${jf}".class ]; then
                  cp /vnmr/spincad/classes/"${jf}".class "${vnmruser}"/spincad/classes/
                  echo "copied ${jf}.class from /vnmr/spincad/classes to ${vnmruser}/spincad/classes"
               else
                  # no files found anywhere ? 
                  echo "${jf}.java or ${jf}.class file not found in user or system class directories"
                  compiledJOK=0
               fi
            fi
          fi

        fi

          if [ $okToDelete = 1 ]; then
             rm -f "${vnmruser}"/spincad/classes/"${jf}".class
          fi

        if [ $okToCompile = 1 ]; then
          javac -classpath /vnmr/jpsg/lib/Jpsg.jar:"$vnmruser"/spincad/classes:"$CLASSPATH" \
                 "${vnmruser}"/spincad/classes/"${jf}".java
          if [ ! -f "${vnmruser}"/spincad/classes/"${jf}".class ]; then
            echo  "Compiling Java program ${jf}.java ... ERROR: DID NOT COMPILE CORRECTLY \
                     \n possibly additional class files may be needed to compile the program. \
                     \n Check for \"cannot resolve symbol\" error messages and identify the classes"
            compiledJOK=0
          else
            echo  "Compiling Java program ${jf}.java ... done"
          fi
        fi
        done
        
    fi
    # above fi ends allFlag if

        if [ $compiledJOK = 1 ]; then
          echo "Done!  Pulse sequence \""$file"\" now ready to use."
        else
          echo "Done!  Pulse sequence \""$file"\" compiled,                  \
               \n      however one or more of Java programs did not compile."
        fi
        echo " "


    fi
    # above fi ends the "if file found"

   shift
   # shift to the next argument in the list

 done
 # end of while loop

else

  echo "spingen: no pulse sequence name argument provided"

fi
# end

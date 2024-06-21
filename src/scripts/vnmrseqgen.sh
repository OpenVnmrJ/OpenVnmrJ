#! /bin/bash 
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
#*********************************************************************
#** seqgen:  shell script for performing pulse sequence generation  **
#**          at the UNIX level.                                     **
#**                                                                 **
#**          The pulse sequence file to be compiled is supplied as  **
#**          the argument to the seqgen shell script, e.g.,         **
#**          "seqgen test".  Note that the ".c" file extension is   **
#**          NOT necessary when specifying the pulse sequence file, **
#**          although if it is, it is removed, so test or test.c    **
#**          are both acceptable.                                   **
#**   Modified:							    **
#**    01/25/96   Rolf K.    allows compilation of sequences in     **
#**             $vnmrsystem/psglib; the sequence will be copied     **
#**		into the local psglib and treated as a local file   **
#**             allow for local makefile $vnmruser/psg/seqgenmake.  **
#**             made error file sequence-specific, to allow for     **
#**             simultaneous compilation by the same user.          **
#**             allows compilation of multiple sequences, e.g.:     **
#**                     cd ~/vnmrsys/psglib; seqgen *.c             **
#*********************************************************************
# Main MAIN main

echo "Beginning Pulse Sequence Generation Process:"

# Check to establish USER and LOGNAME equivalence
# (Prevents problems with one user using UNIX command "su"
# to become some other user)

if [[ ! -z $LOGNAME ]] && [[ $USER != $LOGNAME ]]; then
   echo "Username: "$USER"  and Logname: "$LOGNAME"  do not agree."
   echo ""
   exit
fi

#  Check for USER pulse sequence generation directory (PSG).  If
#  one does not exist, set the PSG directory to the SYSTEM PSG
#  directory (/VNMR/PSG).

cd "$vnmruser"
osname=$(vnmr_uname)

rpath="-Wl,-rpath $vnmrsystem/lib"
psdir="$vnmrsystem"/lib
makefile="$vnmrsystem"/psg/seqgenmake
if [ -d "$vnmruser"/psg ]; then
   # If USER does not have the appropriate PSG library, the PSG
   # directory is set to the SYSTEM PSG directory (/VNMR/PSG).
   if [ x$osname = "xLinux" ] && [ -r "$vnmruser"/psg/libpsglib.so.6.0 ]; then
      rpath="-Wl,-rpath $vnmruser/psg -Wl,-rpath $vnmrsystem/lib"
      psdir="$vnmruser"/psg
   fi
   if [ x$osname = "xDarwin" ] && [ -r "$vnmruser"/psg/libpsglib.a ]; then
      rpath="-Wl,-rpath $vnmruser/psg -Wl,-rpath $vnmrsystem/lib"
      psdir="$vnmruser"/psg
   fi
   # test for local make file for seqgen
   if [ -f "$vnmruser"/psg/seqgenmake ]; then
      makefile="$vnmruser"/psg/seqgenmake
   fi
fi

arch=""
# Silence warnings from newer gcc compilers
if [ x$osname = "xLinux" ]; then
   Wextra=""
   gcc -Q --help=warning | grep Wunused-but-set-variable >& /dev/null
   if [[ $? -eq 0 ]]
   then
      Wextra="-Wno-unused-but-set-variable"
   fi
   gcc -Q --help=warning | grep Wunused-result >& /dev/null
   if [[ $? -eq 0 ]]
   then
      Wextra=${Wextra}" -Wno-unused-result"
   fi
   gcc -Q --help=warning | grep Wmisleading-indentation >& /dev/null
   if [[ $? -eq 0 ]]
   then
      Wextra=${Wextra}" -Wno-misleading-indentation"
   fi
   gcc -Q --help=warning | grep Wstringop-truncation >& /dev/null
   if [[ $? -eq 0 ]]
   then
      Wextra=${Wextra}" -Wno-stringop-truncation"
   fi
   gcc -Q --help=warning | grep Wformat-overflow >& /dev/null
   if [[ $? -eq 0 ]]
   then
      Wextra=${Wextra}" -Wno-format-overflow"
   fi
   file $vnmrsystem/lib/libpsglib.so | grep "32-bit" $file >& /dev/null
   if [[ $? -eq 0 ]]; then
      arch="-m32"
   fi
fi

#  Check for USER directory for compiled pulse sequences (SEQLIB). If
#  it is not present, create it.

if  [ ! -d "$vnmruser"/seqlib ]; then
   mkdir "$vnmruser"/seqlib
fi

# Check for USER pulse sequence directory (PSGLIB)

if [ ! -d "$vnmruser"/psglib ]; then
   mkdir "$vnmruser"/psglib
   if [ ! -d "$vnmruser"/psglib ] || [ ! -d "$vnmruser"/seqlib ]; then
      echo "Cannot create user's 'psglib' and/or 'seqlib' Directories."
      exit
   fi
fi

# If DPS gen program present then execute and set dps postfix name to "dps"

if [ -x "$vnmrsystem"/bin/dps_ps_gen ]; then
   dpspost="dps"
else
   dpspost=""
   echo "DPS extensions skipped,  $vnmrsystem/bin/dps_ps_gen not found."
fi

incl=""
# Set include directories
while [ $# != 0 ]; do
   if [ x$1 = "x-I" ]; then
     shift
     incl="$incl -I$1"
     shift
   else
     break
   fi
done

if [ "x$incl" = "x" ]; then
   incl="-I$vnmruser/psg -I$vnmrsystem/psg"
fi

#**********************************************************
#**  Compile all sequences given as arguments in a loop  **
#**********************************************************

LANG=C
while [ $# != 0 ]; do

   # Remove the *.c file extension (if present) from the pulse sequence name
   basetest=$(basename filec .c)
   if [ x$basetest = "xfil" ]; then
      file=$(basename $1 \\\\.c)
   else
      file=$(basename $1 .c)
   fi

   cd "$vnmruser"/psglib
   psglib="$vnmruser"/psglib
   seqlib="$vnmruser"/seqlib

   if [ ! -f $file.c ]; then
      if [ -f "$vnmrsystem"/psglib/$file.c ]
      then
         cp "$vnmrsystem"/psglib/$file.c "$psglib"
         echo "File $file.c found in $vnmrsystem/psglib;"
         echo "    compiling a local copy in $psglib"
      elif [ -f "$vnmrsystem"/imaging/psglib/$file.c ]
      then
         cp "$vnmrsystem"/imaging/psglib/$file.c "$psglib"
         echo "File $file.c found in $vnmrsystem/imaging/psglib;"
         echo "    compiling a local copy in $psglib"
      fi
   fi
   if [ ! -f $file.c ]; then
      echo "File $file.c not found."
      echo ""			# move on to the next file in the arg list
   else

      # If present, remove file for error and warning messages (ERRMSG)

      if [ -f $file.errors ]; then
         rm $file.errors
      fi

      # If present, remove pulse sequence object file from USER PSGLIB.
      if [ -f $file.o ]; then
         rm $file.o
      fi

      # If present, remove compiled pulse sequence from USER SEQLIB.
      if [ -f "$seqlib"/$file ]; then
         rm "$seqlib"/$file
      fi
      rm -f "$seqlib"/$file.c

      # If DPS gen program found then execute and set dps postfix name to "dps"
      if [ "x$dpspost" = "xdps" ]; then
         echo "Adding DPS extensions to Pulse Sequence \""$file"\"."
         dpspost="dps"
         dps_dummy_obj=
         if [ -f "$vnmrsystem"/psg/psgmain.cpp ]; then
            dps_ps_gen -DDPS -DNVPSG $incl $file.c
         else
            dps_ps_gen -DDPS $incl $file.c
         fi
         if [ ! -f "$psglib"/${file}${dpspost}.c ]; then
            echo "Adding DPS extensions failed"
            dpspost=""
            dps_dummy_obj="$vnmrsystem"/lib/x_ps.o
            rm -f "$psglib"/${file}.i
         fi
      else
         dps_dummy_obj="$vnmrsystem"/lib/x_ps.o
      fi

      # Compile either the combined normal & dps or just
      #    the normal Pulse Sequence if dps_ps_gen was not found.

      if [ x$osname = "xLinux" ]; then
         ( make -e -s -f $makefile PS=${file}${dpspost} \
           CPPFLAGS="$incl" \
           LIB="$psdir" CFLAGS="-O2 ${arch} ${Wextra}" LFLAGS="$rpath" \
           SHELL="/bin/sh" DPS_DUMMY_OBJ="$dps_dummy_obj" \
           SEQLIB="$seqlib" psgLinux) 2>> /dev/null
      elif [ x$osname = "xDarwin" ]; then
         ( make -e -s -f $makefile PS=${file}${dpspost} \
           CPPFLAGS="$incl" \
           LIB="$psdir" CFLAGS="-Os -arch x86_64 -DMACOS" LFLAGS="" \
           CC="clang" CCC="clang++" \
           SHELL="/bin/sh" DPS_DUMMY_OBJ="$dps_dummy_obj" \
           SEQLIB="$seqlib" psgMac) 2>> /dev/null
      else
         ( make -e -s -f $makefile PS=${file}${dpspost} \
           CPPFLAGS="$incl" \
           LIB="$psdir" CFLAGS="-O2 -m32 -D_ALL_SOURCE" LFLAGS="$rpath" \
           CC="gcc" CCC="g++" \
           SHELL="/bin/sh" DPS_DUMMY_OBJ="$dps_dummy_obj" \
           SEQLIB="$seqlib" psgInterix) 2>> /dev/null
      fi

      # Check for extraneous psg and object file
      # If no .o file and dps postfix is not null this means the combined
      # normal & dps Pulse Sequence failed to compile therfore now try to 
      # recompile only the normal sequence

      if [ -f "$psglib"/${file}${dpspost}.o ]; then
         rm "$psglib"/${file}${dpspost}.o
         if [ "x$dpspost" != "x" ] && [ -f ${file}${dpspost}.errors ]; then
            cat ${file}${dpspost}.errors >> ${file}.errors
            rm ${file}${dpspost}.errors
         fi

         # Rename pulse sequence back to standard name if required
         #					(eg. s2puldps -> s2pul)
         if [ "x$dpspost" != "x" ] && [ -f "$seqlib"/${file}${dpspost} ]; then
            mv -f "$seqlib"/${file}${dpspost} "$seqlib"/$file
         fi
      elif [ "x$dpspost" != "x" ]; then
         if [ -f ${file}${dpspost}.errors ]; then
            rm ${file}${dpspost}.errors
         fi

         # attempt recompile if dps failed
         echo "Compile with DPS extensions failed"
         echo "Retrying without DPS extensions"
         dps_dummy_obj="$vnmrsystem"/lib/x_ps.o

         if [ x$osname = "xLinux" ]; then
            ( make -e -s -f $makefile PS=$file \
              CPPFLAGS="$incl" \
              LIB="$psdir" CFLAGS="-O2 ${arch} ${Wextra}" LFLAGS="$rpath" \
              SHELL="/bin/sh" DPS_DUMMY_OBJ="$dps_dummy_obj" \
              SEQLIB="$seqlib" psgLinux) 2>> /dev/null
         elif [ x$osname = "xDarwin" ]; then
            ( make -e -s -f $makefile PS=$file \
              CPPFLAGS="$incl" \
              LIB="$psdir" CFLAGS="-Os -DMACOS -arch x86_64" LFLAGS="" \
              CC="clang" CCC="clang++" \
              SHELL="/bin/sh" DPS_DUMMY_OBJ="$dps_dummy_obj" \
              SEQLIB="$seqlib" psgMac) 2>> /dev/null
         else
            ( make -e -s -f $makefile PS=$file \
              CPPFLAGS="$incl" \
              LIB="$psdir" CFLAGS="-O2 -m32 -D_ALL_SOURCE" LFLAGS="$rpath" \
              CC="gcc" CCC="g++" \
              SHELL="/bin/sh" DPS_DUMMY_OBJ="$dps_dummy_obj" \
              SEQLIB="$seqlib" psgInterix) 2>> /dev/null
         fi
         if [ -f "$psglib"/$file.o ]; then
            rm "$psglib"/$file.o
         fi
      fi

      # Delete dps .c file if present
      if [ "x$dpspost" != "x" ] && [ -f "$psglib"/${file}${dpspost}.c ]; then
         rm "$psglib"/${file}${dpspost}.c
      fi
      rm -f "$psglib"/psg

      # Check for compilation result.
      if [ -f "$seqlib"/$file ]; then

         # Check to see if the file ERRMSG contains any text.
         if [ -s "$psglib"/$file.errors ]; then
            echo ""
            echo "Pulse Sequence did compile but may not function properly."
            echo ""
            echo "The following comments can also be found in the"
            echo "file $psglib/$file.errors:"
            echo ""
            mv "$psglib"/$file.errors "$psglib"/$file.dpserr
            cat "$psglib"/$file.dpserr | sed 's/dps.c/.c/' > "$psglib"/$file.errors
            rm -f "$psglib"/$file.dpserr
            cat "$psglib"/$file.errors
            echo ""
         else
            echo "Done!  Pulse sequence \""$file"\" now ready to use."
            rm -f "$psglib"/$file.errors
            echo ""
         fi
         cp "$psglib"/$file.c "$seqlib"/.
         rm -f "$seqlib"/${file}.psg "$seqlib"/${file}_dps.psg
      else
         echo ""
         echo "Pulse Sequence did not compile."
         echo "The following errors can also be found in the"
         echo "file $psglib/$file.errors:"
         echo ""
         mv "$psglib"/$file.errors "$psglib"/$file.dpserr
         cat "$psglib"/$file.dpserr | sed 's/dps.c/.c/' > "$psglib"/$file.errors
         rm -f "$psglib"/$file.dpserr
         cat "$psglib"/$file.errors
         echo ""
      fi
   fi

   shift
done

#!/bin/sh
# '@(#)patchmake.sh 15.1 08/29/00 1991-1998 '
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
#!/usr/bin/sh

#-----------------------------------------------------------------
# patchmake < -s > patched filename
#
#Argument to the patchmake is a very stricted patched filename 
#which consist of 16 characters, Ex.:  5.3BacqSOLino101
#Where :
#      5.3B  is the Vnmr software version (digit dot digit upper-case-character)
#      acq   is the category under $vnmrsystem such as acq, psg, adm ...
#               special case, "gen" (generic) is for jumbo patch
#                                               (3 lower-case characters)
#
#      SOL   for SOLARIS could be AIX, IRI ...  (3 upper-case characters)
#      ino   for inova could be gem for geminy  (3 lower-case characters)
#                               mer for mercury
#                               mvx for mercury-vx
#                               upl for unity+
#                               uni for unity
#                               all for all console types
#      101   is the count, you could start from anywhere within 3 digit range
#            the best is from 101 for the first patch          (3 digits)
#The patchmake will append .tar.Z to the end of the patched file during the making
#process, the final product would be,  Ex.:  5.3BacqSOLino101.tar.Z 
# 
#The patchmake assumes that you have all the necessary bug fixed files for the
#particular patched category in your current directory 
#The patchmake looks for the "Readme" file for that patch, in the same directory
#otherwise it will terminate itself . It also checks for the "p_install" and
#the "p_remove", these two files together will go in the final patched file,
#if you have the customized one, that one will be used, else, the patchmake 
#will get the standard one from SCCS 
#
#If you want to make a jumbo-patch, which consist of several small patches
#(several .tar.Z files) you place these .tar.Z files in your current directory
#together with the Readme file for this jumbo-patch
#              Ex.: 1)  5.3BacqSOLino101.tar.Z 
#                   2)  5.3BpsgSOLino101.tar.Z 
#                   3)  5.3BadmSOLino101.tar.Z 
#                   4)  Readme
#then give the patchmake the filename argument with "gen" for the category
#The p_install and p_remove do not go with jumbo-patch, only with small-patch
#
#The patchmake will place two final files
#              Ex.:  5.3BacqSOLino101.tar.Z
#                    5.3BacqSOLino101.Readme
#to the vnmr patched ftp site which set by the env variable $vnmrpatch_dir
#
#At the end, your current directory left with the original states .
#
#The patchmake with -s option (save) will keep patched file .tar.Z stay in
#your current directory, and do nothing with the patched ftp site
#-------------------------------------------------------------------

#-----------------------------------------------------------
# named_check
#-----------------------------------------------------------
named_check () { 

   is_beta=`echo "$1" | cut -c3`

   if [ x$is_beta != "xb" ]
   then
      if [ `echo -n "$1" | wc -c` -ne 16 ]
      then
          echo -e "\nPatched filename must be 16 character long and corrected format"
          echo -e "Ex.:  patchmake < -s > 5.3BacqSOLino101\n"
          exit 1
      fi
 
      echo $1 | grep '[0-9]\.[0-9][A-Z][a-z][a-z][a-z][A-Z][A-Z][A-Z][a-z][a-z][a-z][0-9][0-9][0-9]' > /dev/null
    
      if [ "$?" -ne 0 ]
      then
          echo -e "\npatchmake:  Wrong patched filename format"
          echo -e "Ex.:  patchmake < -s > 5.3BacqSOLino101\n"
          exit 1
      fi
   fi
} 

#-----------------------------------------------------------
# make_jumbo_patch <patched filename> <Readme file location>
#                  5.3BacqSOLino101   $vnmrsystem/tmp/$1.Readme
#-----------------------------------------------------------
make_jumbo_patch () {

    ###Making tar file of all .Z files
    echo "Tar all .Z files ----------"
    tar czvf $1.tar.Z *.Z
    #compress -f $1.tar
    if [ $save_patch -eq 0 ]
    then
        cp_to_ftp_site $1.tar.Z $2
    fi

    return 0
}

#-----------------------------------------------------------
# make_small_patch <patched filename> <Readme file location>
#                  5.3BacqSOLino101   $vnmrsystem/tmp/$1.Readme
#-----------------------------------------------------------
make_small_patch () {
    if [ -f *.Z ]
    then
        echo -e "\n	.Z file exist in this directory, Can not make !\n"
        return 1
    fi

    echo "Making patch $1 ----------"
    ###Someone might already add more information to p_install or p_remove .
    ###Copy p_install and p_remove to here if not yet exist
    if [ ! -f p_install ]
    then
        p_install_exist=0
        echo "Get p_install ----------"
        Sget scripts p_install.sh #2>/dev/null
        make p_install
        if [ $? -eq 0 ]
        then
            rm -f p_install.sh
        fi
    else
        p_install_exist=1
        chmod +x p_install
    fi

    if [ ! -f p_remove ]
    then
        p_remove_exist=0
        echo "Get p_remove ----------"
        Sget scripts p_remove.sh #2>/dev/null
        make  p_remove
        if [ $? -eq 0 ]
        then
            rm -f p_remove.sh
        fi
    else
        p_remove_exist=1
        chmod +x p_remove
    fi

    ###Making tar file of the current directory
    echo "Tar current directory ----------"
    tar czvf $1.tar.Z *
    #compress -f $1.tar
    if [ $save_patch -eq 0 ]
    then
        cp_to_ftp_site $1.tar.Z $2
    fi

    return 0
}

#-----------------------------------------------------------
# cp_to_ftp_site <.tar.Z_file>  <$vnmrsystem/tmp/$1.Readme>
#-----------------------------------------------------------
cp_to_ftp_site () {
    ### move .tar.Z file to $VNMR_PATCH_DEST_DIR
    echo "Copy patch to $VNMR_PATCH_DEST_DIR ----------"
    cp $1 $VNMR_PATCH_DEST_DIR
    cp $2 $VNMR_PATCH_DEST_DIR

    rm -f $1

    return 0
}

#-----------------------------------------------------------
#                    Main
#
# patchmake < -s > <patched-filename> 
#-----------------------------------------------------------

VNMR_PATCH_DEST_DIR=${vnmrpatch_dir:-/scratch/vnmrpatch}

###argument check
if [ $# -lt 1 -o $# -gt 2 ]
then
    echo -e "\nUsage:   $0  < option >  \"Patched filename\" "
    echo -e "Ex.:  $0  < -s >  5.3BacqSOLino101\n"
    exit 1
fi
 
case $1 in
         -s ) save_patch=1
              shift
              named_check $1
              vnmrsystem=''
              break
              ;;   
 
          * ) save_patch=0
              named_check $1
              if [ ! -d $VNMR_PATCH_DEST_DIR ]
              then
                  echo -e "\nThe directory $VNMR_PATCH_DEST_DIR does not exist"
                  echo -e "You need to set the env variable \"vnmrpatch_dir\" to"
                  echo -e "the Vnmr patched \"ftp\" site \n"
                  exit 1
              fi
              ###do not want to tar the "Readme" file
              if [ -r Readme ]
              then
                  mv Readme $vnmrsystem/tmp/$1.Readme
              else
                  echo -e "\nMissing \"Readme\" file for patch $1, Can not make !\n"
                  exit 1
              fi
              break
              ;;   
esac
 
###variable vnmrsystem check
if [ $save_patch -eq 0 ]
then
    if [ -z "$vnmrsystem" ]
    then
        echo -e "\nYou need to set the env variable \"vnmrsystem\" to"
        echo -e "where the Vnmr software resides .\n"
        exit 1
    fi
fi
 
###The patch name with "gen" (generic) in it is the Jumbo patch
patch_category=`echo "$1" | cut -c5-7`
if [ "$patch_category" = "gen" ]
then
    make_jumbo_patch $1 $vnmrsystem/tmp/$1.Readme
    if [ $? -ne 0 ]
    then
        exit 1
    fi
else
    make_small_patch $1 $vnmrsystem/tmp/$1.Readme
    if [ $? -ne 0 ]
    then
        exit 1
    fi
fi

###cleaning
echo "Cleaning ----------"
if [ -r Readme ]
then
    mv $vnmrsystem/tmp/$1.Readme Readme
fi

if [ "$patch_category" != "gen" ]
then
    if [ $p_install_exist -ne 1 ]
    then
        rm -f p_install
    fi

    if [ $p_remove_exist -ne 1 ]
    then
        rm -f p_remove
    fi
fi

echo -e "\npatchmake:  $1  - DONE -\n"

#!/bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#

# set -x
#-----------------------------------------------------------
#  Get value of vnmrsystem
#  If not defined, ask for its value
#  Use /vnmr as the default
#  make sure directory exists
#-----------------------------------------------------------
get_vnmrsystem() {

    if [ "x$vnmrsystem" = "x" ]
    then
        vnmrsystem=/vnmr
    fi
 
    if test ! -d "$vnmrsystem"
    then
        echo "$vnmrsystem does not exist, exit upgrade"
        exit
    fi
    export vnmrsystem
}

get_vnmrsystem
savePath=$(sed -n '2,2p' vnmrrev | tr -s " ," _)
upgrade_temp_dir="$vnmrsystem"/tmp/upgrade
upgrade_adm_dir="$vnmrsystem"/adm/upgrade
upgrade_save_dir=$upgrade_adm_dir/$savePath
sysdDir=""
if [[ ! -z $(type -t systemctl) ]] ; then
   sysdDir=$(pkg-config systemd --variable=systemdsystemunitdir)
fi

# This section is run as root to set acquisition files
if [[ $# -gt 0 ]]; then
    tmp_save=$upgrade_save_dir
    if [[ $(stat -c %a "$vnmrsystem"/bin/execkillacqproc) -eq 500 ]]; then
        chmod 777 "$vnmrsystem"/bin/execkillacqproc
        if [[ ! -d $tmp_save/bin ]]; then
            mkdir -p $tmp_save/bin
        fi
        mv "$vnmrsystem"/bin/execkillacqproc $tmp_save/bin
        mv "$vnmrsystem"/bin/execkillacqproc.new "$vnmrsystem"/bin/execkillacqproc
    fi
    if [[ $(stat -c %a "$vnmrsystem"/bin/sudoins) -eq 500 ]]; then
        chmod 777 "$vnmrsystem"/bin/sudoins
        if [[ ! -d $tmp_save/bin ]]; then
            mkdir -p $tmp_save/bin
        fi
        mv "$vnmrsystem"/bin/sudoins $tmp_save/bin
        mv "$vnmrsystem"/bin/sudoins.new "$vnmrsystem"/bin/sudoins
        /vnmr/bin/sudoins >& /dev/null
    elif [[ -f  $tmp_save/bin/sudoins ]]; then
        /vnmr/bin/sudoins >& /dev/null
    fi
    if [[ -f "/etc/init.d/rc.vnmr" ]]; then
        diff --brief "$vnmrsystem"/acqbin/rc.vnmr /etc/init.d/rc.vnmr >& /dev/null
        if [[ $? -ne 0 ]]; then
            cp -f "$vnmrsystem"/acqbin/rc.vnmr /etc/init.d/rc.vnmr
        fi
    fi
    if [[ -f "$sysdDir/vnmr.service" ]]; then
        diff --brief "$vnmrsystem"/acqbin/vnmr.service $sysdDir/vnmr.service >& /dev/null
        if [[ $? -ne 0 ]]; then
            cp "$vnmrsystem"/acqbin/vnmr.service $sysdDir/vnmr.service
        fi
    fi
    if [[ -f "$sysdDir/bootpd.service" ]] &&
       [[ -f "$vnmrsystem"/acqbin/bootpd.service ]]; then
        diff --brief "$vnmrsystem"/acqbin/bootpd.service $sysdDir/bootpd.service >& /dev/null
        if [[ $? -ne 0 ]]; then
            cp "$vnmrsystem"/acqbin/bootpd.service $sysdDir/bootpd.service
        fi
    fi
    if [ -f /etc/debian_version ]; then
       if [ "$(dpkg --get-selections openjdk-8-jre 2>&1 |
         grep -w 'install' > /dev/null;echo $?)" != "0" ]
       then
          echo "Updating java"
          apt-get -y install openjdk-8-jre > /dev/null 2>&1
       fi
    else
       if [ "$(rpm -q java-1.8.0-openjdk |
             grep 'not installed' > /dev/null;echo $?)" = "0" ]
       then
          echo "Updating java"
          yum -y install java-1.8.0-openjdk > /dev/null 2>&1
       fi
    fi
    exit 0
fi

# Check if upgrade can be installed on this system
checkRev () {

    if [[ ! -f "$vnmrsystem"/vnmrrev ]]; then
        echo "$vnmrsystem/vnmrrev is not present. Cannot do upgrade"
        return -1
    fi

    if [[ $(uname -s) != "Linux" ]]; then
        echo "Upgrade is only possible on Linux systems"
        return -1
    fi

#  vnmrrev file information
    vnmr_rev=$(grep VERSION "$vnmrsystem"/vnmrrev)
    if [[ $(echo $vnmr_rev | awk '{ print $1 }') != "OpenVnmrJ" ]]; then
        echo "Upgrade is only possible with previous OpenVnmrJ installations"
        return -1
    fi

    vnmr_rev_1=$(echo $vnmr_rev | awk '{ print tolower($3) }')
    console_name=$(sed -n '3,3p' "$vnmrsystem"/vnmrrev)
    console_type=$(echo $console_name | cut -c1-2)
    spectrometer=$console_name
    case $console_type in
        me )    spectrometer="MERCURYplus/-Vx"
                ;;
        mv )    spectrometer="MERCURY-Vx"
                ;;
        mp )    spectrometer="MERCURYplus"
                ;;
        in )    spectrometer="UNITY INOVA"
                ;;
        vn )    spectrometer="VNMRS"
                ;;
        mr )    spectrometer="MR-400"
                ;;
        mv )    spectrometer="MERCURY-Vx"
                ;;
        pr )    spectrometer="ProPulse"
                ;;
        b1 )    spectrometer="Bridge12"
                ;;
        * )
                ;;
    esac
    if [ $(echo $console_name | grep -ic dd2) -gt 0 ]; then
        spectrometer="$spectrometer DD2"
    fi
    if [[ ! -f code/rht/${console_name}.rht ]]; then
        echo "Upgrading a $spectrometer system is not possible with this DVD image"
        return -1
    fi
    return 0
}

isInstalled() {
    case "x$category" in

        "xVNMR" )
            return 1
            ;;
        "xBackprojection" )
            if [[ -f $vnmrsystem/bin/bp_ball ]]; then
                return 1
            else
                return 0
            fi
            ;;
        "xBiosolidspack" )
            if [[ -d $vnmrsystem/biosolidspack ]]; then
                return 1
            else
                return 0
            fi
            ;;
        "xChinese" )
            if [[ -f $vnmrsystem/templates/vnmrj/properties/vjLabels_zh_CN.properties ]]; then
                return 1
            else
                return 0
            fi
            ;;
        "xDOSY_for_VnmrJ" )
            if [[ -f $vnmrsystem/maclib/dosy ]]; then
                return 1
            else
                return 0
            fi
            ;;
        "xVAST" )
            if [[ -f $vnmrsystem/bin/gilalign ]]; then
                return 1
            else
                return 0
            fi
            ;;
        "xImaging_or_Triax" )
            if [[ -d $vnmrsystem/imaging ]]; then
                return 1
            else
                return 0
            fi
            ;;
        "xJapanese" )
            if [[ -f $vnmrsystem/templates/vnmrj/properties/vjLabels_ja.properties ]]; then
                return 1
            else
                return 0
            fi
            ;;
        "xLC-NMR" )
            if [[ -d $vnmrsystem/lc ]]; then
                return 1
            else
                return 0
            fi
            ;;
        "xSecure_Environments" )
            if [[ -d $vnmrsystem/p11 ]]; then
                return 1
            else
                return 0
            fi
            ;;
        "xSTARS" )
            if [[ -f $vnmrsystem/bin/starsprg ]]; then
                return 1
            else
                return 0
            fi
            ;;
            * )
                return 0
            ;;
    esac
}

#-----------------------------------------------------------
# doUpgrade Installs the files.
#-----------------------------------------------------------
doUpgrade () {

    # This is where the upgrade record is kept
    tmp_save=$upgrade_save_dir
    mkdir "$tmp_save"

#   ignoreCategory2=$(cat code/rht/${console_name}.ignore)
    ignoreCategory=(
        code/tarfiles/bootup_message.tar
        code/tarfiles/conpar.tar
        code/tarfiles/conpar.400mr.tar
        code/tarfiles/devicenames.tar
        code/tarfiles/devicetable.tar
        code/tarfiles/pgsql.tar
        code/tarfiles/jre.tar
        code/tarfiles/fidlib.tar
        code/tarfiles/spinapi.tar
        )
    ignoreFiles="
        acq/info
        p11/part11Config
        adm/accounting/loggingParamList
        adm/users/profiles/accPolicy
        adm/users/userDefaults
        pulsecal
        imaging/gradtables
        solventlist
        solventppm
        solvents
        "

    OLDIFS=$IFS
    IFS=$'\n'
    tarFiles=$(cat code/rht/${console_name}.rht)
    num=$(wc -l code/rht/${console_name}.rht | awk '{print $1}')
    index=0
    for tarFile in $tarFiles
    do
        index=$((index+1))
        printf "Upgrading category $index of $num\r"
        category=$(echo $tarFile | awk '{ print $1 }')
        isInstalled
        if [ $? -ne 1 ]; then
            # echo "$category not installed"
            continue
        fi
        File=$(echo $tarFile | awk '{ print $3 }')
        skip=0
        for ignore in "${ignoreCategory[@]}"
        do
            if [[ $ignore = $File ]]; then
                # echo "ignore $File"
                skip=1
                break
            fi
        done
        if [[ $skip -eq 1 ]]; then
            continue
        fi

        upgrade_temp_dir="$vnmrsystem"/tmp/upgrade
        rm -rf $upgrade_temp_dir/*
        contents=$(tar tf $File)
        cp $File $upgrade_temp_dir/file.tar
        (cd $upgrade_temp_dir && tar -xf file.tar)
        for content in $contents
        do
            if [[ -d $upgrade_temp_dir/$content ]]; then
                dir=$content
                continue
            fi
            if [[ -L $upgrade_temp_dir/$content ]]; then
                if [[ -e $vnmrsystem/$content ]]; then
                    continue
                fi
            fi
            echo $ignoreFiles | grep -q $content >& /dev/null
            if [[ $? -eq 0 ]]; then
                # echo "ignore file $content"
                continue
            fi
            if [[ -f $vnmrsystem/$content ]]; then
                diff --brief $upgrade_temp_dir/$content $vnmrsystem/$content >& /dev/null
                if [[ $? -ne 0 ]]; then
                    dirContent=$(dirname $content)
                    if [[ ! -d $tmp_save/$dirContent ]]; then
                        mkdir -p $tmp_save/$dirContent
                    fi
                    if [[ $doAcq -eq 1 ]] &&
                       [[ $content = "bin/execkillacqproc" ]]; then
                        mv $upgrade_temp_dir/$content $vnmrsystem/$content.new
                    elif [[ $doAcq -eq 1 ]] &&
                         [[ $content = "bin/sudoins" ]]; then
                        mv $upgrade_temp_dir/$content $vnmrsystem/$content.new
                    else
                        mv $vnmrsystem/$content $tmp_save/$dirContent/.
                        mv $upgrade_temp_dir/$content $vnmrsystem/$content
                    fi
                    echo $content | grep -w psglib  >& /dev/null
                    if [[ $? -eq 0 ]]; then
                        seq=$(echo $content | sed 's/psglib/seqlib/' | sed 's/\.c$//')
                        dirContent=$(dirname $seq)
                        if [[ ! -d $tmp_save/$dirContent ]]; then
                            mkdir -p $tmp_save/$dirContent
                        fi
                        mv $vnmrsystem/$seq $tmp_save/$dirContent/.
                        mv $upgrade_temp_dir/$seq $vnmrsystem/$seq
                    fi
                fi
            else
                if [[ ! -d $vnmrsystem/$dir ]]; then
                    mkdir -p $vnmrsystem/$dir
                fi
                mv $upgrade_temp_dir/$content $vnmrsystem/$content
                echo $content | grep -w psglib  >& /dev/null
                if [[ $? -eq 0 ]]; then
                    seq=$(echo $content | sed 's/psglib/seqlib/' | sed 's/\.c$//')
                    mv $upgrade_temp_dir/$seq $vnmrsystem/$seq
                fi
            fi
        done
    done
    IFS=$OLDIFS

    #successfuly installed
    grep -w $console_name "$vnmrsystem"/vnmrrev >& /dev/null
    if [[ $? -ne 0 ]]; then
        echo $console_name >> "$vnmrsystem"/vnmrrev
    fi
    if [[ -f $tmp_save/bin/Vnmrbg ]]; then
        cp -f "$vnmrsystem"/bin/Vnmrbg "$vnmrsystem"/bin/Vnmr
    fi
    if [[ -d $vnmrsystem/jre ]]; then
       mv $vnmrsystem/jre $tmp_save/.
    fi
    if [[ -d $vnmrsystem/pgsql/bin_ver7 ||
          -f $vnmrsystem/pgsql/bin/pg_id ]]; then
       mv $vnmrsystem/pgsql $tmp_save/.
       cp code/tarfiles/pgsql.tar $vnmrsystem/.
       (cd $vnmrsystem && tar -xf pgsql.tar)
       rm -f $vnmrsystem/pgsql.tar
       if [[ -d $tmp_save/pgsql/bin ]]; then
          mv $vnmrsystem/pgsq/bin_ver9 $vnmrsystem/pgsq/bin
       fi
       if [[ -f $tmp_save/pgsql/persistence/LocatorOff ]]; then
          touch /vnmr/pgsql/persistence/LocatorOff
       else
          dbsetup
       fi
    fi
    echo ""
   
    return 0
}

# Update checksums if it is an SE system
fixSE () {
    cmd="/vnmr/p11/sbin/makeP11checksums"
    if [ -f $cmd ]
    then
        nmr_adm=$(ls -l $vnmrsystem/vnmrrev | awk '{print $3}')
        nmr_group=$(ls -l $vnmrsystem/vnmrrev | awk '{print $4}')
        echo "Updating SE checksums"
        $cmd $vnmrsystem $nmr_adm $nmr_group
    fi
}

#-----------------------------------------------------------
# clean_up
#-----------------------------------------------------------
clean_up () {

    #Just go somewhere else, for removing $upgrade_temp_dir
    cd "$vnmrsystem"
    rm -rf "$upgrade_temp_dir"
}

 
#-----------------------------------------------------------
#
#                Main
#-----------------------------------------------------------

user=$(id -un)
if [ x$user = "xroot" ]
then
    echo "Upgrade cannot be done by root."
    exit 1
fi


if touch "$vnmrsystem"/testpermission 2>/dev/null
then
    rm -f "$vnmrsystem"/testpermission
else
    echo "$user does not have permission to write into $vnmrsystem directory"
    echo ""
    echo "Please, login as the OpenVnmrJ admin and then rerun $0"
    echo ""
    exit 1
fi
checkRev
if [[ $? -ne 0 ]]; then
    exit 1
fi

newRev=$(head -n1 vnmrrev)
if [[ -d "$upgrade_save_dir" ]]; then
    echo ""
    echo "This system has already been upgraded with $newRev"
    echo ""
    exit 0
fi
echo ""
echo "-- Upgrading to $newRev --"
echo "                $(sed -n '2,2p' vnmrrev) "
echo ""
 

rm -rf "$upgrade_temp_dir"
mkdir "$upgrade_temp_dir"
if [ ! -d "$upgrade_adm_dir" ]
then
    mkdir -p "$upgrade_adm_dir"
fi

doAcq=0
if [[ $(stat -c %a "$vnmrsystem"/bin/execkillacqproc) -eq 500 ]]; then
    doAcq=1
fi

noWeb=0
if [[ ! -d /vnmr/web ]]; then
   noWeb=1
fi

doUpgrade

if [[ $noWeb -eq 1  ]]; then
    rm -rf $vnmrsystem/web
fi
if [[ $doAcq -eq 0 ]]; then
    if [[ -f $upgrade_save_dir/bin/sudoins ]]; then
        doAcq=1
    fi
fi

# Has setacq been run?
if [[ -f "$vnmrsystem"/acqbin/acqpresent ]]; then
    if [[ $doAcq -eq 0 ]] && [[ -f /etc/init.d/rc.vnmr ]]; then
        diff --brief "$vnmrsystem"/acqbin/rc.vnmr /etc/init.d/rc.vnmr >& /dev/null
        if [[ $? -ne 0 ]]; then
            doAcq=1
        fi
    fi
    if [[ $doAcq -eq 0 ]] && [[ -f $sysdDir/vnmr.service ]]; then
        diff --brief "$vnmrsystem"/acqbin/vnmr.service $sysdDir/vnmr.service >& /dev/null
        if [[ $? -ne 0 ]]; then
            doAcq=1
        fi
    fi
    if [[ $doAcq -eq 0 ]] && [[ -f $sysdDir/bootpd.service ]] &&
        [[ -f "$vnmrsystem"/acqbin/bootpd.service ]]; then
        diff --brief "$vnmrsystem"/acqbin/bootpd.service $sysdDir/bootpd.service >& /dev/null
        if [[ $? -ne 0 ]]; then
            doAcq=1
        fi
    fi
fi

if [ -f /etc/debian_version ]; then
   if [ "$(dpkg --get-selections openjdk-8-jre 2>&1 |
         grep -w 'install' > /dev/null;echo $?)" != "0" ]
   then
      doAcq=1
   fi
else
   if [ "$(rpm -q java-1.8.0-openjdk |
         grep 'not installed' > /dev/null;echo $?)" = "0" ]
   then
      doAcq=1
   fi
fi

if [[ $doAcq -eq 1 ]]; then
    echo ""
    echo "Final step of the upgrade requires elevated permissions"
    if [ -f /etc/debian_version ]; then
       echo "Please enter the $user sudo password when requested"
    else
       echo "Please enter the root password when requested"
    fi
    echo ""
    if [ -f /etc/debian_version ]; then
        sudo $0 doAcq ;
    else
        su root -c "$0 doAcq";
    fi
fi

fixSE
clean_up

echo ""
echo "-- Upgrade to $newRev complete --"
echo ""

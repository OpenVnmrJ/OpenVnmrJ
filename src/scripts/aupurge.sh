: '@(#)aupurge.sh 22.1 03/24/08 1999-2002 '
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
#aupurge.sh

    if test x"$vnmrsystem" = "x"
    then
        # with 5.3 the vnmrsystem env is not set when VnmrJ adm sudo is executed
        # thus test if this has been envoke as a sudo command
        # could test SUDO_USER, SUDO_GID, or SUDO_COMMAND
        # if SUDO_USER has a value then don't ask for vnmrsystem just default 
        # to /vnmr     GMB/GRS 8/10/2009
        if [ "x" == "x$SUDO_USER" ]; then
           nnl_echo  "Please enter location of VNMR system directory [/vnmr]: "
           read vnmrsystem
           if test x"$vnmrsystem" = "x"
           then
               vnmrsystem="/vnmr"
           fi
        else
           vnmrsystem="/vnmr"
        fi
        export vnmrsystem

    fi

    if test ! -d "$vnmrsystem"
    then
        echo "$vnmrsystem does not exist, cannot proceed:"
        exit
    fi

    aucontrl=/etc/security/audit_control
    p11conf=$vnmrsystem/p11/part11Config

    bsm_audit_dir=`grep dir: $aucontrl | cut -d: -f2,3`
    #for testing only
    #bsm_audit_dir=/usr25/chin/test-au

    #will remove this dir at the end, no need to save .praud files
    dot_praud_tmp_dir=/tmp/uaudit

    if [ ! -d $dot_praud_tmp_dir ]
    then
        mkdir -p $dot_praud_tmp_dir
    fi

    aa=

    if [ -r $p11conf ]
    then
        #This is where .cond files would go
        aa=`grep auditDir: $p11conf | cut -d: -f2,3`
    else
        exit
    fi

    if [ x$1 != "x" ]
    then
        dot_cond_tmp_dir=$1
    else 
        dot_cond_tmp_dir=/tmp/uaut
    fi

    if [ ! -d $dot_cond_tmp_dir ]
    then
        mkdir -p $dot_cond_tmp_dir
    else
	rm -rf $dot_cond_tmp_dir/*.cond
    fi

    #for testing only
    #dot_cond_dest_dir=/usr25/chin/test-au/d-dir

    /usr/varian/sbin/auconvert $bsm_audit_dir $dot_praud_tmp_dir
    #/usr25/chin/part11/auconvert.sh $bsm_audit_dir $dot_praud_tmp_dir


    rm -f $dot_cond_tmp_dir/*.not_terminated.*
    /usr/varian/sbin/aureduce $dot_praud_tmp_dir $dot_cond_tmp_dir
    #/usr25/chin/part11/aureduce.sh $dot_praud_tmp_dir $dot_cond_dest_dir 

    #do not want to keep these files 
    rm -rf $dot_praud_tmp_dir

: '@(#)userdel.sh 22.1 03/24/08 2003-2004 '
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
#!/bin/ksh
#
# $Id: userdel,v 1.4 2003/07/31 20:36:36 mark Exp $
#
#---------------------
# Written by Mark Funkenhauser. 
#
# This is sample code only.   Provided "AS IS". 
# There are no Warranties.
#
# It is intended only for use on Interix 3.0.
#
# Redistribution and use in source form, 
# with or without modification, are permitted.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#---------------------
#
#
#usage: userdel [-W] [-d] username
#    where
#	-W :  delete a Domain user account (without -W delete local user)
#	-d :  delete user's home directory
#

# make sure we only look for utils in well known places
#
PATH=/bin:/usr/contrib/win32/bin

PROG=$(basename $0)


# some temp files
#
T1=${TMPDIR}/.userdel1-$$
T2=${TMPDIR}/.userdel2-$$


cleanup()
{
    rm -f ${T1} ${T2}
}

error()
{
	print -r "$PROG: Error: " "$*" 1>&2
	cleanup
	exit 1
}

warn()
{
	print -r "$PROG: Warning: " "$*" 1>&2
}


usage()
{
echo "\
usage: userdel [-W] [-d] username
    where
	-W :  delete Windows domain account (without -W delete local acct)
	-d :  delete user's home directory
"
	exit 2
}


#
# MAIN PROGRAM
#

delOptions="/DELETE"
opt_domain=""
dir_remove=FALSE
debug=off
homedir=""

#
# Parse the options
#
while getopts ":dWx" opt; do
    case "$opt" in

	d)  dir_remove="TRUE"
	    ;;

	W)
	    # delete a domain account
	    opt_domain="/DOMAIN"
	    ;;

	x)  debug=on
	    set -x
	    ;;

	':')
	     warn "missing argument after -${OPTARG}."
	     usage
	     ;;
	'?')
	     warn "unknown option -${OPTARG}."
	     usage
	     ;;
	*)
	     warn "unknown option -${opt}."
	     usage
	     ;;
    esac
done
shift $OPTIND-1

#
# get the user name that we want to delete
#
USER="${1}"

if [ "$USER" = "" ]; then
   warn "User name missing"
   usage
fi


#
# Make sure that the user exists
#
net user "${USER}" ${opt_domain} > ${T1} 2>&1

if [ $? != 0 ]; then
   error "User '${USER}' does not exist"
fi


# if necessary, get home directory information 
# from tmp file T1
#
if [ $dir_remove = "TRUE" ]; then
    #
    # ---- Figure out user's home directory
    #      Save it in 'homedir' variable
    #


    # convert data from "net user" above into UNIX text format
    flip -u ${T1}

    dir=""
    homedir=""

    # find user's home directory from info in $T1 file
    # Store this directory name in 'dir'
    # Remember that this directory will be in Windows path format
    #
    # Assume format in this file is something like:
    #   Home Directory  c:/users home directory/
    #
    grep -i '^Home directory' ${T1} > ${T2}

    if [ $? = 0 ]; then
       read -r a b dir < ${T2}
    fi

    cleanup


    # convert Windows pathanme to Interix format
    # and store in 'homedir'
    #
    homedir=$(winpath2unix "${dir}" 2> /dev/null)

    #
    # make sure Interix recognizes this directory
    if [ ! -d "$homedir" ]; then
	# directory does not exist.
	#
	warn "User ${USER}'s homedir '${homedir}' does not exist"
    fi
fi

#
# ---- try to delete the user
#


net USER "${USER}" ${delOptions} ${opt_domain} >/dev/null 2> ${T1}

if [ $? != 0 ]; then
	#
	# double check, sometimes the win32 exec fails but
	# it really has succeeded.
	#
	net user "${USER}" ${opt_domain} > /dev/null 2>&1
	if [ $? = 0 ]; then
	    # user still exists !
	    cat ${T1}
	    error "Could not remove user ${USER}"; 
	fi
fi

print "account '${USER}' successfully removed."


#
# --- remove user's home directory if requested
#
if [ $dir_remove = "TRUE" ]; then

	# remove user's home directory and contents 

	#
	# note: skip homedir processing if homedir field
	#       is empty from the data provided by "net user ..." above
	#       or if directory is "/"
	#
	if [ "${homedir}" != "" -a "${homedir}" != "/" ]; then

	    if [ -d "${homedir}" ]; then
		#
		# directory exists.
		# Make sure admin really wants to do this
		#
		echo "Remove ${USER}'s home directory '${homedir}'?[y/n] (n) \c"
		read ans

		if [ "${ans}" = "Y" -o "${ans}" = "y" ]; then
		    rm -rf ${homedir}
		fi
	    fi
	fi
fi

cleanup

: '@(#)useradd.sh 22.1 03/24/08 2003-2004 '
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
# $Id: useradd,v 1.4 2003/07/31 20:36:36 mark Exp $
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
# useradd: This script is similar to the System V useradd command
#
#
#    usage: 
# 	useradd [-W] [-v] 
#		[-g group ] [-G group[[,group]...]] 
#		[-d dir ]
#               [-c comment ] 
#		[-s shell ]
#		[-p password ] username"
#
# NOTES:
#  1) if you (as admin of this program) have a default shell profile
#     template that you'd like installed in each user's home directory
#     then change the DEFAULT_PROFILE variable below to point
#     to this file.
#
DEFAULT_PROFILE=This_file_can_not_possibly_exist

#
# reset PATH so we can always find the utils
# that we need in this script
#
PATH=/bin:/usr/contrib/win32/bin:$PATH

#
# ---- We need a couple temporary files
#
T1=${TMPDIR}/.useradd1.$$
T2=${TMPDIR}/.useradd2.$$


# ---- global variables
#

opt_domain=		# empty by default - means create local user

# special options for the "net use" command line
opt_shell=/USERCOMMENT:
opt_homedir="/HOMEDIR:"		# use no directory as default value
opt_comment="/COMMENT:"


hflag=FALSE		 # -h flag : display help/usage message
debug=off		 #internal debugging (off by default)
verbose=FALSE		 # Verbose

_primarygroup=""
_grouplist=""
_password=""
_shell=""

unset GROUPLIST
_DEST_TYPE="Local"


#
# ---- local functions
#

#
# Print out error message and exit
#
error()
{
	print -r "${_PROG}: Error: " $* 1>&2
	#print -r "${_PROG}: Exiting"
	cleanup
	exit 1
}

#
# print out a warning message
#
warn()
{
	print -r "${_PROG}: Warning: " $* 1>&2
}


usage()
{
    echo "\
usage: ${_PROG} [-h] [-W] [-v] [-c comment ]
               [-g group ] [-G group[[,group]...]]
               [-d dir_in_UNIX_format ]
               [-s login_shell ]
               [-p password ]
               username
"

if [ "${hflag}" = TRUE ]; then
        echo "\
    where:
	-W 		:  use Windows domain not local system
	-v 		:  verbose mode
	-c comment 	:  add 'comment' to user information
	-g group   	:  add this user to 'group'
	-G group,... 	:  add this user to several groups
	-d directory    :  create 'directory' as user's home directory
	-s shell    	:  use 'shell' as user's login shell
	-p passwd	:  save 'passwd' as user's password
"

fi
	exit 2
}



check_valid_group()
{
	typeset GRP	# local variable

	# a simple string search in the list of all possible groupnames.
	# this does not accurately test if the group name is correct
	# but its better than nothing.
	#
	IFS=","		# group names may be separated by ','
	for GRP in ${1}; do
	    echo ${systemgroups} | grep -i "\"${GRP}\"" > /dev/null 2>&1
	    if [ $? != 0 ] ; then
		error "${_DEST_TYPE} group '${GRP}' does not exist"
	    fi
	    set -A GROUPLIST -- "${GROUPLIST[@]}" "${GRP}"
	done
	IFS=" "		# reset IFS
}

#
# This function will add a user to a group.
# Also, we don't want to fail at this point, so we just issue warnings
# not fatal problems.
#
add_user_to_group()
{

	typeset GRP	# local variable
	typeset ret

	#
	GRP="$1"

	# choose the correct 'net' command 
	# depending on whether this is a local or Domain request
	#
	if [ "$opt_domain" = "" ]; then
	    net localgroup "${GRP}" ${USER} /ADD  > ${T2} 2>&1
	else
	    net group "${GRP}" ${USER} /ADD /DOMAIN  > ${T2} 2>&1
	fi
	ret=$?

	# check for errors.
	#  0 - success
	#  2 - user already in this group
	# 
	if [ $ret = 0 ] ; then
	    if [ "$verbose" = TRUE ]; then
		echo "    $USER added to group '${GRP}'"
    	    fi
	elif [ $ret != 2 ]; then
	    cat ${T1}
	    echo "Couldn't add $USER to '${NGRP}'"
	fi
}



#
# usage:   check_homedir  pathname 
# returns: 
#    Sets the 'unixdir'and 'windir' global variables
#    Also sets the 'opt_homedir' global variable
#
# if 'pathname' exists then update 'opt_homedir' variable to 
# the appropriate option for the 'net.exe' command
# 
check_homedir()
{
    unixdir="$1"

    # check home directory
    #
    if [ "$unixdir" != "" ]; then

	    # convert to windows absolute pathname
	    windir=$(unixpath2win ${unixdir}) > /dev/null 2>&1

	    if [ $? = 1 ]; then
		error "Directory '${unixdir}' is not allowed."
	    fi

	    opt_homedir="/HOMEDIR:${windir}"

    fi
}

create_homedir ()
{
    #
    # Create the directory for the newly created user if a directory
    # was specified.
    #
    if [ "$unixdir" != "" ]; then
	if [ -e "$unixdir" ]; then
	    warn "Pathname '$unixdir' already exists."
	    ls -ldD "$unixdir"
	    print "\nDo you wish to remove it? [y/n] (n) \c"
	    read ans
	    if [ "${ans}" = "Y" -o "${ans}" = "y" ]; then
	        rm -rf "${unixdir}"
		if [ $? != 0 ]; then
		    error "Could not remove '$unixdir'"
		fi
	    fi
	fi

	mkdir -p "${unixdir}"
	if [ $? != 0 ]; then
		error "Couldn't create directory ${unixdir}"
	fi
	if [ -f ${DEFAULT_PROFILE} ]; then
	      cp -p ${DEFAULT_PROFILE} "${unixdir}"
	fi

	#
	# get user's primary group name so we can assign it properly
	# to the home directory
	#
	if [ "$opt_domain" = "" ]; then
	    # NOTE: the '+' in front of ${USER} is to indicate this is a
	    #       local username. (not a domain name).
	    gid=$(id -gn +"${USER}")
	    chown -R +"${USER}:$gid" "${unixdir}" >/dev/null 2>&1
	else
	    gid=$(id -gn "${USER}")
	    chown -R "${USER}:$gid" "${unixdir}" >/dev/null 2>&1
	fi

	if [ $? != 0 ] ; then
	     warn "Could not assign ownership (${USER}) to '${unixdir}'"
	fi
	chmod 775 "${unixdir}" >/dev/null 2>&1
    fi
}

	
check_comment()
{
    _comment="$1"
    # Add a comment into the user's information database
    if [ "${_comment}" != "" ]; then
	opt_comment="/COMMENT:${_comment}"
    fi
}

cleanup()
{
    rm -f ${T1} ${T2}
}


#
# get the list of all valid system groups
# and store them in 'system_groups' variable
#
enum_systemgroups () {

    if [ "${opt_domain}" = "" ]; then
	#
	# Create the list of local groups and store into 'systemgroups' variable
	# Ignore any extra stuff that "net localgroup" outputs
	#
	net localgroup > ${T1} 2>/dev/null
    else
	net group ${opt_domain} > ${T1} 2>/dev/null
    fi

    # find all group names (names that begin with "*")
    # - then convert DOS text format line endings to UNIX format
    # - then replace leading "*" with a double quote (")
    # - then add a double quote to the end of each group name
    # - then remove any trailing blanks in the group name
    # - then replace multiple blanks with a single blank
    grep "\*" ${T1} \
	    | flip -u - \
	    | sed "s/\*//g" \
	    | sed 's/^/"/' \
	    | sed 's/$/"/' \
	    | sed 's/  *"$/"/' \
	    | sed 's/   */" "/g' > ${T2}

    systemgroups=$(cat ${T2})
    cleanup
}


#
#
# --- MAIN PROGRAM
#

#
# What is this program's name ?
# Check if _PROG is already set (ie if usermod sources this file)
#
_PROG=${_PROG:-useradd}

case "${_PROG}" in
    useradd)
	_ADMIN_OPERATION=/ADD
	_ADMIN_DESCRIPTION="create"
	;;
    usermod)
	_ADMIN_OPERATION=""
	_ADMIN_DESCRIPTION="change"
	;;
esac


#
# Process the options
#
while getopts ":c:d:g:G:p:s:hVvWx" opt; do
	case "$opt" in
	p) 
	    _password="$OPTARG"
	    ;;
	s)
	    _shell="$OPTARG"
	    opt_shell="/USERCOMMENT:$_shell"
	    ;;
	g) 
	    # NOTE: we provide the -g option 
	    #      but it really doesn't work the same as on UNIX.
	    #      I haven't figured out how to tell NT to assign
	    #      the primpary group to a specific value.
	    #    So for now, just accept this group name and add the
	    #    username to the group.
	    #
	    _primarygroup="$OPTARG"
	    ;;
	d) 
	    check_homedir "$OPTARG"
	    ;;
	c) 
	    check_comment "$OPTARG"
	    ;;
	G) 
	    _grouplist="$OPTARG"
	    ;;
	h) 
	    hflag=TRUE
	    usage
	    ;;
	v) 
  	    verbose="TRUE"
	    ;;
	V) 
	    verbose="TRUE" 
 	    ;;
	W)
	    # try to create a Windows domain account
	    opt_domain="/DOMAIN"
	    _DEST_TYPE="Domain"
	    ;;
	x) 
	    set -x 
	    debug=on
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
# get the user, notice that we only take one user at a time
#
USER=${1}
if [ "$USER" = "" ] ; then
     warn "Missing username"
     usage
fi

if [ "$2" != "" ]; then
    warn "Arguemnts after username are not allowed"
    usage
fi

#
# Find out whether the user exists
#
net user ${USER} ${opt_domain} > /dev/null 2>&1
result=$?

# if useradd command, then user cannot already exist.
# if usermod command, then user must already exist.
#
case "${_PROG}" in
    useradd)
	if [ $result = 0 ]; then
	    error "User '${USER}' already exists"
	fi
	;;
    usermod)
	if [ $result != 0 ]; then
	    error "User '${USER}' does not exist"
	fi
	;;
esac

# find all the system group names
#
enum_systemgroups

#echo "SYSTEM GROUP LIST: " $localgroups

# validate the -g and/or -G options
#
check_valid_group "${_primarygroup}"
check_valid_group "${_grouplist}"


# check that shell specified actually exists
#
if [ "$_shell" != "" -a  ! -f "$_shell" ]; then
    error "The filename '$_shell' does not exist"
fi

if [ "$verbose" = TRUE ]; then
    if [ "$_password" != "" ]; then
	print -r "    ${USER}: Setting password to ${_password}"
    fi
    if [ "$_comment" != "" ]; then
	print -r "    ${USER}: Setting comment to ${_comment}"
    fi
    if [ "$windir" != "" ]; then 
	print -r "    ${USER}: Setting home directory to ${windir}"
    fi
    if [ "$_shell" != "" ]; then
	echo "    Setting login shell to ${_shell}"
    fi
fi

#
# Create the new account.
# Then separatly add the user to the specified groups below
#
# NOTE: we insist that password cannot contain any spaces.
#      That's why we don't use double quotes around the
#      ${_password} variable below.
#      If _password was empty, then the double quotes would cause
#      this arg to be empty and the net.exe command to not work properly
#    Same with the _ADMIN_OPERATION variable.
#
net user ${USER} ${_password} ${_ADMIN_OPERATION} "${opt_comment}" \
	"${opt_homedir}" "${opt_shell}" ${opt_domain} > ${T1} 2>&1

if [ $? != 0 ]; then
  cat ${T1}
  error "Failed to ${_ADMIN_DESCRIPTION} account ${USER}"
fi

create_homedir

#
# Now finish up by adding the user to any specified groups
#
for grp in "${GROUPLIST[@]}" ; do
	add_user_to_group "${grp}"
done

#
# indicate success
#
if [ "$verbose" = TRUE ]; then
    echo "account '${USER}' ${_ADMIN_DESCRIPTION}d successfully"
fi

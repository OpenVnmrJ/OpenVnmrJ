#
# Copyright (C) 2018  Michael Tesch
#
# This file is a part of the OpenVnmrJ project.  You may distribute it
# under the terms of either the GNU General Public License or the
# Apache 2.0 License, as specified in the LICENSE file.
#
# For more information, see the OpenVnmrJ LICENSE file.
#

# this is a shell-function library.  To use the functions
# in your shell script, first include this by "sourceing"
# this file:
# 'source loglib.sh' or '. loglib.sh'
# The latter syntax with '.' is POSIX.
#
# CMDLINE="$0 $*"
# SCRIPT=$(basename "$0")
# SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#

: ${CMDLINE=loglib.sh}
: ${VERBOSE=3}
: ${ERRCOUNT=0}

LEVELNAMES=( ERROR MSG WARN INFO DEBUG )

# call this before calling any log commands.  can be called repeatedly
# from one script to change the log file.
log_setup () {
    # directory for log files, will try
    # to mkdir -p if non-existant
    local logdir="$2"
    [ -z "$logdir" ] && logdir=.
    LOGFILE="${logdir}/$1"       # typically $(basename "$0" .sh).txt

    # colors & names of the log levels
    # check if stdout is a terminal...
    if test -t 1; then
        # see if it supports colors...
        ncolors=$(tput colors)
        if test -n "$ncolors" && test "$ncolors" -ge 8; then
            normal="$(tput sgr0)"
            bold="$(tput bold)"
            red="$(tput setaf 1)"
            green="$(tput setaf 2)"
            yellow="$(tput setaf 3)"
            cyan="$(tput setaf 6)"
            white="$(tput setaf 7)"
            magenta="$(tput setaf 5)"
        fi
    fi
    #set -x
    LEVELCOLOR=( "$red" "$white$bold" "$yellow" "$green" "$cyan" )

    if [ ! -d "$logdir" ]; then
        echo "creating log directory '$logdir'"
        mkdir -p "$logdir" || return $?
    fi
    if [ ! -d "$logdir" ]; then
        echo "${red}Unable to create log directory '$logdir':${normal}"
        echo "  ${red}log messages will be printed to the terminal.${normal}"
        return
    fi
    if [ -t 3 ] && [ -t 4 ]; then
        # restore stdin and stdout before opening new logfile
        exec 1>&3 2>&4
    fi
    exec 3>&1 4>&2
    trap 'exec 1>&3 2>&4' 0
    trap 'exec 1>&3 2>&4; exit 1' 1 2 3
    #trap 'onerror' 0 1 2 3
    # move old LOGFILE
    if [ -f "${LOGFILE}" ]; then
        for SEQ in $(seq -w 1 10); do
            if [ ! -f "${LOGFILE}.${SEQ}" ] || [ $SEQ -eq 10 ]; then
                echo "Moving old logfile ${LOGFILE} to ${LOGFILE}.${SEQ}"
                mv "${LOGFILE}" "${LOGFILE}.${SEQ}"
                break
            fi
        done
    fi
    # redirect output to LOGFILE
    if [ ${VERBOSE} -gt 3 ]; then
        # at VERBOSE >= DEBUG level, also send cmd output to terminal
        exec 1> >(tee -a "${LOGFILE}") 2>&1
    else
        exec 1> "$LOGFILE" 2>&1
    fi
    # how & when this script was called
    log_debug "$CMDLINE"
    log_info "Logfile: $LOGFILE"
}
log_msg_ () {
    local level=$1
    shift
    #local datestring=$(date +"%Y-%m-%d %H:%M:%S")
    local message="$*"
    echo "${LEVELCOLOR[level]}${LEVELNAMES[level]}:${message}${normal}"
    if [ -t 3 ] && [ ${VERBOSE} -le 3 ] && [ "$level" -le ${VERBOSE} ]; then
        echo "${LEVELCOLOR[level]}${LEVELNAMES[level]}:${message}${normal}" >&3
    fi
}
log_error () { log_msg_ 0 "$*" ; ERRCOUNT=$(( ERRCOUNT + 1 )) ; }
log_msg   () { log_msg_ 1 "$*" ; }
log_warn  () { log_msg_ 2 "$*" ; }
log_info  () { log_msg_ 3 "$*" ; }
log_debug () { log_msg_ 4 "$*" ; }
log_cmd   () { log_info "\$ $*" ; "$@" ; }
cmdspin () {
    #
    # Run a command, spin a wheelie while it's running
    #
    log_info "Cmd started $(date)"
    log_info "\$ $*"
    # spinner
    local sp='/-\|'
    if [ -t 3 ]; then printf ' ' >&3 ; fi
    while : ; do
        sleep 1;
        sp=${sp#?}${sp%???}
        if [ -t 3 ]; then printf '\b%.1s' "$sp" >&3 ; fi
    done &
    SPINNER_PID=$!
    # Kill the spinner if we die prematurely
    trap "kill $SPINNER_PID" EXIT
    # command runs here
    "$@"
    retval=$?
    # Kill the loop and unset the EXIT trap
    kill -PIPE $SPINNER_PID
    trap " " EXIT
    if [ -t 3 ]; then printf '\b.\n' >&3 ; fi
    log_info "Cmd finished $(date), returned: $retval"
    return $retval
}

if [ x$LOGLIBTEST = y ]; then
    log_warn "bad"
    log_msg "bold"
    log_info "info"
    log_error "error"
    log_debug "debug"
    log_setup "lib.log1"
    log_warn "hello1"
    log_setup "lib.log2"
    log_warn "hello2"
    log_warn "bad"
    log_msg "bold"
    log_info "info"
    log_error "error"
    log_debug "debug"
fi

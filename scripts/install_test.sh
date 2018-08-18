#!/bin/bash
#
# Copyright (C) 2018  Michael Tesch
#
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
#
# For more information, see the LICENSE file.
#

#
# install and test OpenVnmrJ from the command-line
# based on email from Dan Iverson
#

CMDLINE="$0 $*"
SCRIPT=$(basename "$0")
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ERRCOUNT=0
onerror() {
    echo "$(tput setaf 1)$SCRIPT: Error on line ${BASH_LINENO[0]}, exiting.$(tput sgr0)"
    exit 1
}
trap onerror ERR

: "${ACTIONS="install test"}"
: "${FORCE=no}"
: "${OVJ_VERSION=openvnmrj_1.1_A}"
: "${VERBOSE=3}"
: "${OVJ_SUPERCLEAN=no}"
: "${OVJ_PASSWD=vnmrx}"
: "${OVJ_SYSTEM=Spectroscopy}"
: "${OVJ_LOGFILE=install_test.log}"
: "${OVJ_CONSOLE=propulse}" # inova
: "${OVJ_OS=rht}"
: "${OVJ_NMRADMIN=vnmr1}"
: "${OVJ_NMRGROUP=nmr}"
: "${OVJ_HOME=/home}"
: "${OVJ_SETVNMRLINK=yes}"
tmp=(VNMR Backprojection Biosolidspack DOSY_for_VnmrJ Imaging_or_Triax STARS)
OVJ_INS_LIST=("${OVJ_INS_LIST[@]:-${tmp[@]}}")
tmp=()
OVJ_INS_OPTS=("${OVJ_INS_OPTS[@]:-${tmp[@]}}")

# set OVJ_TOOLS if necessary
if [ "x${OVJ_TOOLS}" = "x" ] ; then
    if [ -d "$SCRIPTDIR/../../ovjTools/vms" ]; then
        cd "$SCRIPTDIR/../../ovjTools"
        OVJ_TOOLS="$(pwd)"
        echo "Setting OVJ_TOOLS to '${OVJ_TOOLS}'"
        export OVJ_TOOLS
    elif [ -d "$(pwd)/vms/box_defs" ]; then
        OVJ_TOOLS="$(pwd)"
        echo "Setting OVJ_TOOLS to '${OVJ_TOOLS}'"
        export OVJ_TOOLS
    else
        echo "set OVJ_TOOLS environment variable to the ovjTools directory"
        exit 1
    fi
else
    # make sure it's set right
    if ! [ -d "${OVJ_TOOLS}/vms/centos7" ]; then
        echo "set OVJ_TOOLS environment variable to the ovjTools directory"
        exit 1
    fi
fi

# set ovjBuildDir if necessary
if [ "x${ovjBuildDir}" = "x" ] ; then
    cd "${OVJ_TOOLS}/.."
    ovjBuildDir="$(pwd)"
fi

usage_msg() {
    cat <<EOF
usage:
  $SCRIPT [actions...]

actions:
  install               - install OpenVnmrJ, kludge a default configuration
  uninstall             - uninstall OpenVnmrJ version '${OVJ_VERSION}'
  test                  - run vjqa tests

options:
  -a <package>          - add package to package list (${OVJ_INS_LIST[@]})
  -A                    - clear package list
  -o <option>           - add install option (${OVJ_INS_OPTS[@]})
  -O                    - clear options list
  -c <console>          - which console to install for (${OVJ_CONSOLE}):
                          Mercury/INOVA : inova,mercplus[,g2000?]
                          VNMRS         : propulse,mr400,mr400dd2,vnmrs,vnmrsdd2
  -p <password>         - set password for vnmr1 and testuser (${OVJ_PASSWD})
  -d                    - install as datastation (TODO!)
  --nolink              - dont set the /vnmr link (${OVJ_SETVNMRLINK})
  -f|--force            - force [un]install (TODO: prompt if unset)
  -s                    - super-clean uninstall, remove vnmr1&testuser home dirs too
  -v|--verbose          - be more verbose (can add multiple times)
  -q|--quiet            - be more quiet   (can add multiple times)
  -h|--help             - print this message and exit

packages (-a) :
 VNMR Backprojection Biosolidspack DOSY_for_VnmrJ Imaging_or_Triax
 STARS Secure_Environments ... (TODO: fill this list)

install options (-o) :
 VAST

Environment Variables:
  OVJ_TOOLS     -> path to ovjTools
  ovjBuildDir   -> path to the build directory (OVJ_TOOLS/..)
EOF
}

usage() {
    usage_msg
    if [ -t 3 ]; then usage_msg >&3 ; fi
    exit 1
}

#######################################################################
#
# process flag args
#
while [ $# -gt 0 ]; do
    key="$1"
    case $key in
        install)                ACTIONS=install             ;;
        uninstall)              ACTIONS=uninstall           ;;
        test)                   ACTIONS="test"              ;;
        -a)                     OVJ_INS_LIST+=("$2"); shift ;;
        -A)                     OVJ_INS_LIST=()             ;;
        -o)                     OVJ_INS_OPTS+=("$2"); shift ;;
        -O)                     OVJ_INS_OPTS=()             ;;
        -c)                     OVJ_CONSOLE="$2"; shift      ;;
        -p)                     OVJ_PASSWD="$2"; shift      ;;
        -f|--force)             FORCE=yes                   ;;
        -s)                     OVJ_SUPERCLEAN=yes          ;;
        -d)                     OVJ_SYSTEM=Datastation      ;;
        -v|--verbose)           VERBOSE=$(( VERBOSE + 1 )) ;;
        -q|--quiet)             VERBOSE=$(( VERBOSE - 1 )) ;;
        -h|--help)              usage                       ;;
        *)
            echo "unrecognized arg: $key"
            usage
            ;;
    esac
    shift
done

if [ ${VERBOSE} -gt 4 ]; then
    set -x
fi

#######################################################################
#
# helper functions
#

# import logging functions
# shellcheck source=loglib.sh
. "$SCRIPTDIR/loglib.sh"


#######################################################################
#
# functions
#

add_appdir() {
    local user="$1"    # name of vnmr user
    local appdir="$2"  # path under $vnmruser
    local vnmrsystem=/vnmr
    local vnmruser="$(eval echo "~$user")/vnmrsys"
    local appdirfile="${vnmruser}/persistence/appdir_${user}"
    local appmode="${OVJ_SYSTEM}"
    if [ ! -d "$vnmruser" ]; then
        log_error "nmr user '$user' doesn't have a ~/vnmrsys/ directory"
        return 1
    fi
    if [ ! -f "${appdirfile}" ]; then
        log_cmd sudo -u "$user" mkdir -p "$(dirname "$appdirfile")" || return $?
        log_cmd sudo -u "$user" cp "${vnmrsystem}/adm/users/userProfiles/appdir${appmode}.txt" \
                "${appdirfile}" || return $?
    fi
    appdirpath="${vnmruser}/${appdir}"
    applabel="$(basename "$appdir")"
    if ! grep "$applabel" "$appdirfile" > /dev/null ; then
        log_info "Enabling appdir '${appdirpath}' for user '${user}' as '$applabel'"
        echo "1;${appdirpath};${applabel}" >> "${appdirfile}"
    fi
}

vnmr_cmd() {
    #local cmd="$1"
    #cmdspin sudo -i -u vnmr1 Vnmr -d -mserver -exec "$cmd" -exec exit
    #log_cmd sudo -i -u vnmr1 Vnmr -d -xserver -t vjqa
    #cmdspin sudo -i -u vnmr1 Vnmr -n0 -mback "$1"
    cmdspin sudo -i -u vnmr1 Vnmr -mback "$1"
}

is_ovj_installed() {
    # does ovj appear to be installed already?
    # todo: could check more thoroughly!
    [ -d /vnmr ] && [ -d /usr/varian ] && \
        [ -d "${OVJ_HOME}/${OVJ_VERSION}" ] && \
        [ -d ~vnmr1 ] && \
        [ -d ~testuser ] && \
        getent passwd testuser && \
        getent passwd vnmr1 > /dev/null 2>&1
}

join_by() { local IFS="$1"; shift; echo "$*"; }

#######################################################################
#
# Main script starts here
#

# setup the logfile
log_setup "${OVJ_LOGFILE}" "${ovjBuildDir}/logs"

# verify ovjBuildDir and that build completed
if ! [ -f "${ovjBuildDir}/vnmr/adm/sha1/sha1chklistFiles.txt" ]; then
    log_error "missing sha1chklistFiles.txt in \${ovjBuildDir}, invalid build directory"
    log_error " did the build complete in '${ovjBuildDir}'?"
    exit 1
fi

# do the things
for ACTION in $ACTIONS ; do

    if [ $ACTION = install ]; then
        log_info "Installing ${OVJ_VERSION}"

        # check if maybe already installed
        if is_ovj_installed ; then
            log_warn "Looks like OpenVnmrJ is already installed, skipping install"
            continue
        fi

        # Make sure that /bin/sh is bash or bash-like!  Many scripts
        # need this:
        if [ -z "$(/bin/sh -c 'echo $BASH_VERSION')" ]; then
            log_msg "It looks like /bin/sh is NOT bash, OpenVnmrJ scripts"
            log_msg "require some non-POSIX features missing from, ie dash"
            log_msg "particularly shell redirects in the form '>&' and '&>'"
            if [ "$FORCE" != yes ]; then
                log_error "Aborting install, to force installation use the -f flag."
                exit 1
            else
                log_msg "Continuing forced install anyway, consider yourself warned."
            fi
        fi

        # First, create the user vnmr1.  Also, before running the OVJ
        # installation script, you may need to create the "nmr" group.
        log_cmd groupadd nmr || log_error "'groupadd nmr' failed, ignoring..."

        # create two users - vnmr1 is special, testuser is normal
        for user in vnmr1 testuser ; do
            log_info "Creating unix user '$user'"
            # bug workaround: ubuntu sometimes doesnt reasonably set login shell
            useradd -s /bin/bash -m "$user"
            log_info "Setting default password for $user"
            echo -e "${OVJ_PASSWD}\\n${OVJ_PASSWD}" | passwd "$user"
            log_info "Adding '$user' to group 'nmr'"
            usermod -a -G nmr "$user"
        done

        # Then, as root, cd into the dvdimageOVJ and run the script.
        #
        # The part in bold should be changed to the correct directory
        # where the dvdimageOVJ is at or put the dvdimageOVJ into the /tmp
        # directory.
        case "${OVJ_CONSOLE}" in
            inova|mercplus|g2000)                   SUFX=OVJMI ;;
            propulse|mr400|mr400dd2|vnmrs|vnmrsdd2) SUFX=OVJ   ;;
            *)
                log_error "unrecognized OVJ_CONSOLE: ${OVJ_CONSOLE}"
                usage
                ;;
        esac
        cmdspin ${ovjBuildDir}/dvdimage${SUFX}_*/code/ins_vnmr "${OVJ_OS}" "${OVJ_CONSOLE}" \
                ${ovjBuildDir}/dvdimage${SUFX}_*/code "${OVJ_HOME}/${OVJ_VERSION}" \
                "${OVJ_NMRADMIN}" "${OVJ_NMRGROUP}" "${OVJ_HOME}" "${OVJ_SETVNMRLINK}" no \
                "+$(join_by + "${OVJ_INS_LIST[@]}")" "+$(join_by + "${OVJ_INS_OPTS[@]}")"
        retval=$?

        if [ $retval = 0 ]; then
            log_warn "install script ins_vnmr success."
        else
            log_warn "install script ins_vnmr failed, tail log:"
            tail -50 /tmp/ovjlog
            exit $retval
        fi

        # Create users vnmr1 and testuser
        for user in vnmr1 testuser ; do
            log_info "Setting up user '$user'"

            # The script /vnmr/bin/makeuser <username> (from
            # OpenVnmrJ/src/scripts) will create users and configure
            # their home accounts.
            log_cmd /vnmr/bin/makeuser "$user" /home nmr y /vnmr

            # The script /vnmr/bin/ovjUser <username> (also from
            # OpenVnmrJ/src/scripts) will add the necessary files to
            # /vnmr/adm.
            log_cmd sudo -u vnmr1 /vnmr/bin/ovjUser $user

            # make sure .vnmrenvsh is there & run on login (should
            # move this to /vnmr/bin/makeuser)
            if [ ! -f "$(eval echo "~$user")"/.vnmrenvsh ]; then
                sudo -u $user cp /vnmr/user_templates/.vnmrenvsh "$(eval echo "~$user")/"
                echo "[ -f ~/.vnmrenvsh ] && source ~/.vnmrenvsh" >> "$(eval echo "~$user")/.bashrc"
            fi
        done

    elif [ $ACTION = uninstall ]; then
        log_info "Uninstalling ${OVJ_VERSION}"

        if [ $FORCE != yes ]; then
            log_error "Unforced uninstall, skipping"
            continue
        fi

        # delete system varian stuff
        rm -f /vnmr
        rm -rf /usr/varian
        rm -rf "/home/${OVJ_VERSION:?}"

        # remove users and groups
        killall -u vnmr1     && log_warn "Killed procs belonging to 'vnmr1'" || true
        killall -u testuser  && log_warn "Killed procs belonging to 'testuser'" || true
        gpasswd -d vnmr1 nmr || log_error "Unable to remove 'vnmr1' from 'nmr' group"
        userdel vnmr1        || log_error "Unable to delete user 'vnmr1'"
        userdel testuser     || log_error "Unable to delete user 'testuser'"
        groupdel nmr         || log_error "Unable to delete group 'nmr'"
        #vrun 'sudo groupdel vnmr1' || return $?

        # remove the associated home directories
        if [ ${OVJ_SUPERCLEAN} = yes ]; then
            rm -rf ~vnmr1
            rm -rf ~testuser
        fi

        log_warn "Successfully uninstalled '${OVJ_VERSION}'"

    elif [ $ACTION = test ]; then
        log_info "Testing ${OVJ_VERSION}"

        # make sure there's an installation
        if ! is_ovj_installed ; then
            log_error "Cant run tests without OpenVnmrJ installation"
            return 1
        fi

        # The test suite is in OpenVnmrJ/src/vjqa. This is not included in
        # the dvdimageOVJ. It has been a while since I have used
        # it. First,

        cd "${ovjBuildDir}/OpenVnmrJ/src/vjqa" && scons

        # This will create a ovj_qa directory with the correct stuff in
        # it. Then, follow the instructions in the README file.

        # Put the ovj_qa directory in the ~vnmr1/vnmrsys directory.
        log_cmd sudo -u vnmr1 cp -r "${ovjBuildDir}/OpenVnmrJ/src/vjqa/ovj_qa" ~vnmr1/vnmrsys/

        for user in vnmr1 testuser ; do
            vnmruser="/home/$user/vnmrsys"

            # Enable appdirs
            # /home/vnmr1/vnmrsys/ovj_qa/ovjtest and
            # /home/vnmr1/vnmrsys/ovj_qa/OVJQA
            add_appdir "$user" ovj_qa/ovjtest || log_error "unable to add appdir ovjtest"
            add_appdir "$user" ovj_qa/OVJQA || log_error "unable to add appdir OVJQA"

            # copy default pulsecal, create and activate a probe!
            log_cmd sudo -u "$user" cp /vnmr/pulsecal "${vnmruser}/"
            log_cmd sudo -u "$user" cp /vnmr/CoilTable "${vnmruser}/"
        done

        # manual setup kludges
        # appmode=?
        # will these even give a non-zero return val if they fail???
        vnmr_cmd "addprobe('main')"
        vnmr_cmd "probe='main'"
        vnmr_cmd "bootwalkup"
        # set appmode?

        # Run the macro vjqa to execute the QA tests. Results will be put
        # into ~vnmr1/vnmrsys/ovj_qa/ovjtests/reports
        vnmr_cmd vjqa || log_error "vjqa failed"

    else
        log_error "Unknown ACTION: '$ACTION'"
    fi

done

log_info "$SCRIPT done, ${ERRCOUNT} errors.  Logfile: $LOGFILE"
exit ${ERRCOUNT}

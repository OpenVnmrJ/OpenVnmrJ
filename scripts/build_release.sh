#!/bin/bash
#
# Copyright (C) 2018  Michael Tesch
#
# This file is a part of the OpenVnmrJ project.  You may distribute it
# under the terms of either the GNU General Public License or the
# Apache 2.0 License, as specified in the LICENSE file.
#
# For more information, see the OpenVnmrJ LICENSE file.
#

#
# Build script that takes all parameters/control on the command line
# or inherits from the environment.  Originally based on buildovj+makeovj.
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

# helper
numcpus() {
    local ncpu=1
    if [ "x$(uname -s)" = "xDarwin" ]; then
        ncpu=$(sysctl -n hw.ncpu)
    else
        ncpu=$(nproc)
    fi
    echo $ncpu
}

# defaults
: ${OVJ_DO_CHECKOUT=no}
: ${OVJ_DO_BUILD=no}
: ${OVJ_DO_PACKAGE=no}
: ${OVJ_CODESIGN="3rd Party Mac Developer Application:"}
: ${OVJ_BUILDDIR=${ovjBuildDir}}
: ${OVJ_DEVELOPER=OpenVnmrJ}
: ${OVJ_GITBRANCH=master}
: ${OVJ_GITURL="https://github.com/OpenVnmrJ/OpenVnmrJ.git"}
: ${OVJ_GITDEPTH=3}
: ${OVJT_GITBRANCH=master}
: ${OVJT_GITURL="https://github.com/OpenVnmrJ/ovjTools.git"}
: ${OVJT_GITDEPTH=3}
: ${OVJ_SRCRESET=no}
: ${OVJ_PACK_DDR=yes}
: ${OVJ_PACK_MINOVA=yes}
: ${OVJ_SCONSFLAGS="-j $(( $(numcpus) + 1 ))"}
: ${VERBOSE=3}

usage() {
    if [ -t 3 ]; then echo "$SCRIPT failed, see log '$LOGFILE'" >&3 ; fi
    cat <<EOF

Either of the environment variables OVJ_TOOLS or ovjBuildDir MUST be set.

usage:
  ${SCRIPT} [checkout] [build] [package] [options...]

where [options...] are:
  -d|--bindir dir           - target directory for build [\$ovjBuildDir]
  -u|--gitname github_user  - github account to clone from [${OVJ_DEVELOPER}]
  -b|--branch branch_name   - branch to clone, [${OVJ_GITBRANCH}]
  -g|--giturl url           - url for OpenVnmrJ git repository, overrides -u/-b
                              [${OVJ_GITURL}]
  --tbranch branch_name     - branch to clone for ovjTools, ie [${OVJT_GITBRANCH}]
  -t|--tgiturl url          - url for ovjTools git repository, overrides -u/-b
                              [${OVJT_GITURL}]
  --gitdepth num            - argument for --depth in git clone [${OVJ_GITDEPTH}]
  -r|--srcreset             - reset the OpenVnmr/src directory before compiling
  -s scons_options          - flags to pass to scons [${OVJ_SCONSFLAGS}]
  --ddr yes|no              - enable Direct-Drive (VnmrS/DD2/ProPulse) DVD build
                              [${OVJ_PACK_DDR}]
  --inova yes|no            - enable Mercury/Inova DVD build [${OVJ_PACK_MINOVA}]
  --codesign id|none        - set code signing identity [${OVJ_CODESIGN}]
  -v|--verbose              - be more verbose (can add multiple times)
  -q|--quiet                - be more quiet   (can add multiple times)
  -h|--help                 - print this message and exit

EOF
    exit 1
}

# process flag args
while [ $# -gt 0 ]; do
    key="$1"
    case $key in
        checkout)               OVJ_DO_CHECKOUT=yes           ;;
        build)                  OVJ_DO_BUILD=yes              ;;
        package)                OVJ_DO_PACKAGE=yes            ;;
        -u|--gitname)
            OVJ_DEVELOPER="$2"
            OVJ_GITURL="https://github.com/${OVJ_DEVELOPER}/OpenVnmrJ.git"
            OVJT_GITURL="https://github.com/${OVJ_DEVELOPER}/ovjTools.git"
            shift
            ;;
        -b|--branch)            OVJ_GITBRANCH="$2"; shift     ;;
        --tbranch)              OVJT_GITBRANCH="$2"; shift    ;;
        -g|--giturl)            OVJ_GITURL="$2"; shift        ;;
        -d|--bindir)            OVJ_BUILDDIR="$2"; shift      ;;
        --gitdepth)             OVJ_GITDEPTH="$2"; shift      ;;
        -r|--srcreset)          OVJ_SRCRESET=yes              ;;
        -s)                     OVJ_SCONSFLAGS="$2"; shift    ;;
        --ddr)                  OVJ_PACK_DDR="$2"; shift      ;;
        --inova)                OVJ_PACK_MINOVA="$2"; shift   ;;
        --codesign)             OVJ_CODESIGN="$2"; shift      ;;
        -h|--help)              usage                         ;;
        -v|--verbose)           VERBOSE=$(( VERBOSE + 1 )) ;;
        -q|--quiet)             VERBOSE=$(( VERBOSE - 1 )) ;;
        *)
            # unknown option
            echo "unrecognized arg: $key"
            usage
            ;;
    esac
    shift # shift out flag
done

#######################################################################
#
# helper functions
#

# import logging functions
# shellcheck source=loglib.sh
. "$SCRIPTDIR/loglib.sh"

#######################################################################
#
# checkout sources into ${OVJ_BUILDDIR}
#
do_checkout () {
    # check that the requested git repo exists

    # make directory if necessary, then cd there
    if [ ! -d "${OVJ_BUILDDIR}" ]; then
        log_info "checkout: build directory ${OVJ_BUILDDIR} doesnt exist, creating..."
        log_cmd mkdir -p "${OVJ_BUILDDIR}" || return $?
    fi
    log_cmd cd "${OVJ_BUILDDIR}" || return $?

    # check if the OpenVnmrJ directory already exists in ${OVJ_BUILDDIR}
    if [ -d "${OVJ_BUILDDIR}/OpenVnmrJ" ]; then
        log_info "checkout: OpenVnmrJ source directory: ${OVJ_BUILDDIR}/OpenVnmrJ already exists."
        log_info "checkout: checking out requested branch '${OVJ_GITBRANCH}'"
        log_cmd cd "${OVJ_BUILDDIR}/OpenVnmrJ" || return $?
        log_cmd git checkout "${OVJ_GITBRANCH}" || return $?
        log_cmd cd "${OVJ_BUILDDIR}"
        # do something, like delete it?
        #log_error "checkout: source directory ${OVJ_BUILDDIR}/OpenVnmrJ already exists"
    else
        # clone the requested git repo 
        log_cmd git clone --branch "${OVJ_GITBRANCH}" --depth "${OVJ_GITDEPTH}" "${OVJ_GITURL}" || return $?
    fi

    # check that the thing actually checked out an OpenVnmrJ directory
    if [ ! -d "${OVJ_BUILDDIR}/OpenVnmrJ/src/vnmr" ]; then
        log_error "checkout: git clone of '${OVJ_GITURL}' didnt create valid OpenVnmrJ source directory"
        exit 1
    fi

    # clone the requested git repo -- if ovjTools doesnt exist, clone it too
    if [ ! -d "${OVJ_BUILDDIR}/ovjTools" ]; then
        log_cmd git clone --branch "${OVJT_GITBRANCH}" --depth "${OVJT_GITDEPTH}" "${OVJT_GITURL}" || return $?
    fi

    # get the git tag for ovjTools
    log_cmd cd "${OVJ_BUILDDIR}/ovjTools/" || return $?
    log_info "building with ovjTools: $(git describe --exact-match --tags 2>/dev/null || git log -n1 --pretty='%h')"

    # ok done
}

#######################################################################
#
# build the sources in
#
do_build () {
    # check that the source, OpenVnmrJ, exists in ${OVJ_BUILDDIR}
    # check that the thing actually checked out an OpenVnmrJ directory
    if [ ! -d "${OVJ_BUILDDIR}/OpenVnmrJ/src/vnmr" ]; then
        log_error "build: '${OVJ_BUILDDIR}/OpenVnmrJ/' is not a valid OpenVnmrJ source directory"
        exit 1
    fi

    # go where the action is
    log_cmd cd "${OVJ_BUILDDIR}/OpenVnmrJ/" || return $?

    # if src/ reset requested, reset it
    if [ "${OVJ_SRCRESET}" = yes ]; then
        log_info "build: Removing: ===>>> dvdimage* options vnmr console <<<==="
        log_cmd cd "${OVJ_BUILDDIR}/" || return $?
        log_cmd rm -rf dvdimage* options vnmr console

        log_warn "build: wiping OpenVnmrJ/src directory to fresh state"
        log_cmd cd "${OVJ_BUILDDIR}/OpenVnmrJ/" || return $?
        log_cmd rm -rf src
        log_cmd git checkout ./src || return $?
    fi

    # run scons
    cmdspin scons ${OVJ_SCONSFLAGS}
    retval=$?
    if [ $retval -ne 0 ]; then
        # scons failed, dump a little something useful to stdout
        if [ -t 3 ]; then
            echo "--- scons failed. tail LOGFILE: ---" >&3
            tail -30 "${LOGFILE}" >&3 ;
            echo "--- --- --- --- --- --- --- --- ---" >&3
        fi
        return $retval
    fi
    log_info "build done."
}

#######################################################################
#
# package the build into a directory suitable for a DVD image
#
do_package () {
    # args
    local PACK_SCRIPT="$1"
    local OUTPUT_PREFIX="$2"
    local PACK_SCRIPT_SRC="${OVJ_BUILDDIR}/OpenVnmrJ/src/scripts/${PACK_SCRIPT}"
    # used by a sub-script (?)
    local workspacedir="${OVJ_BUILDDIR}"

    # get the git tag for this version
    log_cmd cd "${OVJ_BUILDDIR}/OpenVnmrJ/"
    OVJ_VERSION_STR="$(git describe --exact-match --tags 2>/dev/null || git log -n1 --pretty='%h')"

    log_info "package: packing using ${PACK_SCRIPT} -> ${OUTPUT_PREFIX}_${OVJ_VERSION_STR}"

    local dvdBuildName1=${OUTPUT_PREFIX}_${OVJ_VERSION_STR}  # used in ovjmacout.sh,ovjddrout.sh
    local dvdBuildName2=${OUTPUT_PREFIX}_${OVJ_VERSION_STR}  # used in ovjmiout.sh
    local ovjAppName=OpenVnmrJ_${OVJ_VERSION_STR}.app
    #local shortDate
    #shortDate=$(date +%F)

    if [ ! -x "${PACK_SCRIPT_SRC}" ]; then
        log_error "package: invalid packaging script requeted: '${PACK_SCRIPT_SRC}'"
        exit 1
    fi

    # copy and run the packing script
    log_cmd mkdir -p "${OVJ_BUILDDIR}/bin/"
    log_cmd cd "${OVJ_BUILDDIR}/bin/"
    #log_cmd make ${PACK_SCRIPT} # what does this do?

    # export vars used by the ovj???out.sh ($PACK_SCRIPT) scripts
    export workspacedir dvdBuildName1 dvdBuildName2 ovjAppName OVJ_TOOLS
    cmdspin "${PACK_SCRIPT_SRC}/${PACK_SCRIPT}" || return $?

    # make a second copy? make an iso? todo...
    #dvdCopyName1=OVJ_$shortDate
    #dvdCopyName2=OVJ_MI_$shortDate
    log_info "package [${PACK_SCRIPT} -> ${dvdBuildName1}] done."
}

#######################################################################
#
# main part of the script
#

#
# check validity of arguments
#

# it's ok to run this command without setting either OVJ_TOOLS or
# OVJ_BUILDDIR is from OpenVnmrJ/ or ovjTools/, but we might guess
# what you really want...
if [ x"${OVJ_BUILDDIR}" = x ] && [ x"${OVJ_TOOLS}" = x ]; then
    if [ -d OpenVnmrJ ] && [ -d ovjTools ] ; then
        OVJ_BUILDDIR="$(pwd)"
    else
        OVJ_BUILDDIR="$(dirname "$(dirname "$SCRIPTDIR")")"
    fi
    OVJ_TOOLS="${OVJ_BUILDDIR}/ovjTools"
    echo "${yellow}OVJ_BUILDDIR=${OVJ_BUILDDIR}${normal}"
    echo "${yellow}OVJ_TOOLS=${OVJ_TOOLS}${normal}"
elif [ -d "${OVJ_BUILDDIR}" ] && [ -d "${OVJ_TOOLS}" ]; then
    echo "OVJ_BUILDDIR=${OVJ_BUILDDIR}"
    echo "OVJ_TOOLS=${OVJ_TOOLS}"
elif [ x"${OVJ_BUILDDIR}" = x ] && [ -d "${OVJ_TOOLS}" ]; then
    # create OVJ_BUILDDIR if it's set and doesn't exist
    OVJ_BUILDDIR=$(dirname "${OVJ_TOOLS}")
elif [ -d "${OVJ_BUILDDIR}" ] && [ x"${OVJ_TOOLS}" = x ]; then
    echo "OVJ_TOOLS not set, assuming OVJ_TOOLS=[${OVJ_TOOLS}]"
    OVJ_TOOLS="${OVJ_BUILDDIR}/ovjTools"
fi

if [ ! -d "${OVJ_BUILDDIR}" ]; then
    echo "OVJ_BUILDDIR [${OVJ_BUILDDIR}] doesn't exist, creating..."
    echo mkdir -p "OVJ_BUILDDIR"
fi

if [ "${OVJ_BUILDDIR}/ovjTools" != "${OVJ_TOOLS}" ]; then
    echo "\$OVJ_TOOLS must be a direct subdir of \$OVJ_BUILDDIR"
    exit 1
fi

export OVJ_TOOLS

#
# setup log file
#
log_setup "$(basename "$SCRIPT" .sh).txt" "${OVJ_BUILDDIR}/logs"

# disallow Mercury/Inova build
if [ "x$(uname -s)" = "xDarwin" ]; then
    log_warn todo
fi

# make sure filesystem is case-sensitive
touch "${OVJ_BUILDDIR}/CaSeSeNsItIvE"
touch "${OVJ_BUILDDIR}/casesensitive"
rm "${OVJ_BUILDDIR}/casesensitive"
if [ -f "${OVJ_BUILDDIR}/CaSeSeNsItIvE" ]; then
    log_debug "Filesystem appears to be case sensitive.  good."
    rm "${OVJ_BUILDDIR}/CaSeSeNsItIvE"
else
    log_warn "FILESYSTEM APPEARS TO BE CASE-INSENSITIVE! this probably wont work!!!"
fi

# do the requested actions... (in their own subshells)
if [ "${OVJ_DO_CHECKOUT}" = yes ]; then
    do_checkout
fi

# make sure the checkout is ok
if [ ! -f "${OVJ_TOOLS}/OSX.md" ]; then
    log_warn "variable OVJ_TOOLS set to '${OVJ_TOOLS}'"
    log_error 'check environment: missing OSX.md from "${OVJ_TOOLS}/"'
    usage
fi

if [ "${OVJ_DO_BUILD}" = yes ]; then
    do_build
fi
if [ "${OVJ_DO_PACKAGE}" = yes ]; then
    if [ "${OVJ_PACK_DDR}" = yes ]; then
        log_info "Packaging DDR/DDR2"
        if [ "x$(uname -s)" = "xDarwin" ]; then
            do_package ovjmacout.sh dvdimageOVJ
        else
            do_package ovjddrout.sh dvdimageOVJ
        fi
    fi
    if [ "${OVJ_PACK_MINOVA}" = yes ]; then
        log_info "Packaging MERCURY/INOVA"
        do_package ovjmiout.sh dvdimageOVJMI
    fi
    log_info "Packaging done."
fi

# let the user know they didn't ask for anything, in case they wonder why nothing happend
if [[ "${OVJ_DO_CHECKOUT}" == no && "${OVJ_DO_BUILD}" == no && "${OVJ_DO_PACKAGE}" == no ]] ; then
    set +x
    log_error "No action specified:"
    usage
fi

log_info "$SCRIPT done, ${ERRCOUNT} errors.  Logfile: $LOGFILE"
exit ${ERRCOUNT}

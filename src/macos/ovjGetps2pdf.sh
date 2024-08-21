#!/bin/bash
#
# Copyright (C) 2017  Dan Iverson
#
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
#
# For more information, see the LICENSE file.
#
#Uncomment next line for debugging output
#set -x


ovj_usage() {
    cat <<EOF

usage:

    $SCRIPT will install the Postscript to PDF conversion (ps2pdf)
    tool for MacOS. It will first install brew if needed. The ps2pdf
    script is part of Ghostscript.
EOF
    exit 1
}

# process flag args
while [ $# -gt 0 ]; do
    key="$1"
    case $key in
        -h|--help)              ovj_usage                   ;;
        -vv|--debug)            set -x ;;
        *)
            # unknown option
            echo "unrecognized argument: $key"
            ovj_usage
            ;;
    esac
    shift
done

ostype=$(uname -s)
if [[ $ostype != "Darwin" ]] && [[ $ostype != "darwin" ]]; then
   echo "$0 only works on MacOS systems"
   exit
fi

if [[ ! -z $(type -t ps2pdf) ]]; then
   echo "Postscript to PDF conversion tool already installed"
   exit
fi

if [[ ! -z $(type -t pstopdf) ]]; then
   echo "Postscript to PDF conversion tool already installed"
   exit
fi

if [[ -z $(type -t brew) ]]; then
   echo "Installing brew"
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

if [ -f /opt/homebrew/bin/brew ]; then
   eval "$(/opt/homebrew/bin/brew shellenv)"
elif [ -f /usr/local/bin/brew ]; then
   eval "$(/usr/local/bin/brew shellenv)"
fi
echo "Installing Postscript to PDF conversion tool"

brew install ghostscript

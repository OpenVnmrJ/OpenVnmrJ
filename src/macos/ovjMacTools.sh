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
# set -x

SCRIPT=$(basename "$0")

ovj_usage() {
    cat <<EOF

usage:

    $SCRIPT will install the Postscript to PDF conversion (ps2pdf)
    tool for MacOS and / or ImageMagick and / or xterm. It will first
    install brew if needed. The ps2pdf script is part of Ghostscript.

    $SCRIPT
       Installs ghostScript, ImageMagick, and xterm and brew if needed

    $SCRIPT -h
       Displays this help information

    $SCRIPT -b
       Installs brew only

    $SCRIPT -g
       Installs ghostScript (i.e., ps2pdf) and brew if needed

    $SCRIPT -i
       Installs ImageMagick and brew if needed

    $SCRIPT -x
       Installs xterm and brew if needed
EOF
    exit 1
}

brewOnly="n"
ghostOnly="n"
imageOnly="n"
xtermOnly="n"
# process flag args
while [ $# -gt 0 ]; do
    key="$1"
    case $key in
        -h|--help)              ovj_usage    ;;
        -vv|--debug)            set -x ;;
        -b)                     brewOnly="y"  ;;
        -i)                     imageOnly="y"; ghostOnly="n"; xtermOnly="n" brewOnly="n"  ;;
        -g)                     ghostOnly="y"; imageOnly="n"; xtermOnly="n" brewOnly="n"  ;;
        -x)                     xtermOnly="y"; imageOnly="n"; ghostOnly="n" brewOnly="n"  ;;
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

if [[ -z $(type -t brew) ]]; then
   echo "Installing brew"
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

if [ -f /opt/homebrew/bin/brew ]; then
   eval "$(/opt/homebrew/bin/brew shellenv)"
elif [ -f /usr/local/bin/brew ]; then
   eval "$(/usr/local/bin/brew shellenv)"
fi
if [[ $brewOnly = "n" ]]; then
   if [[ $ghostOnly = "n" ]] && [[ $imageOnly = "n" ]] &&
      [[ $xtermOnly = "n" ]]; then
      echo "Installing Postscript to PDF conversion tool, ImageMagick and xterm"
      brew install ghostscript imagemagick xterm
      brew install --cask xquartz
   elif [[ $ghostOnly = "y" ]]; then
      echo "Installing Postscript to PDF conversion tool"
      brew install ghostscript
   elif [[ $imageOnly = "y" ]]; then
      echo "Installing ImageMagick"
      brew install imagemagick
   elif [[ $xtermOnly = "y" ]]; then
      echo "Installing xterm"
      brew install xterm
      brew install --cask xquartz
   fi
fi


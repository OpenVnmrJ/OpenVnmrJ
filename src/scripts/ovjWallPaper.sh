#!/bin/bash
#
# Copyright (C) 2015  University of Oregon
#
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
#
# For more information, see the LICENSE file.
#
# set -x


if [ x$vnmrsystem = "x" ]
then
   vnmrsystem=/vnmr
fi
if [ x$vnmruser = "x" ]
then
   source $vnmrsystem/user_templates/.vnmrenvsh
fi

DISPLAY=:0
export DISPLAY
icon="$vnmrsystem/iconlib/OVJ_Gray.png"
style='spanned'
if [[ $# -ge 1 ]]; then
  icon=$1
  if [[ $# -ge 2 ]]; then
    style=$2
  fi
fi
if hash gsettings 2> /dev/null; then
    schema="org.gnome.desktop.background"
    if [[ -z $DBUS_SESSION_BUS_ADDRESS ]]; then
       dbus-launch gsettings set $schema picture-uri $icon 2> /dev/null
       dbus-launch gsettings set $schema picture-options $style 2> /dev/null
    else
       gsettings set $schema picture-uri $icon 2> /dev/null
       gsettings set $schema picture-options $style 2> /dev/null
    fi
else
    schema="/desktop/gnome/background/"
    gconftool-2 --type=string --set ${schema}/picture_filename $icon
    gconftool-2 --type=string --set ${schema}/picture_options $style
fi

#! /bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
#-----------------------------------------------

wkhtmltopdf --no-footer-line --footer-font-size 8 --footer-right "[page]/[topage]" --page-size Letter $1 $2

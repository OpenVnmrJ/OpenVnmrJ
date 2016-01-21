: '@(#)usermod.sh 22.1 03/24/08 2003-2004 '
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

# $Id: usermod,v 1.4 2003/07/31 20:36:36 mark Exp $
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
# usermod  - a script to allow modifications to user's account characteristics
#           (like home directory, login shell, comments, ...)
#

# set the _PROG variable. This tells the 'useradd' script
# what mode to run in.
#
_PROG=usermod

# now "source" the useradd script.
# Both usermod and useradd are so similar that 
# all the logic is in the useradd script.
# 
. $(dirname $0)/useradd

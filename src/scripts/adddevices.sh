#! /bin/csh
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#


set user = `id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`


   if ( $?vnmrsystem == 0)  then
      setenv vnmrsystem "/vnmr"
   endif

   if ( $user == "root" ) then
      if ("x$vnmrsystem" == "x")  then
         setenv vnmrsystem   "/vnmr"
      endif

      if ( ! -d "$vnmrsystem" ) then
          echo "$vnmrsystem does not exist, cannot proceed:"
          exit
      endif

      setenv TCLDIR        "$vnmrsystem/tcl"
      setenv VNMR_ADM_DIR  "$vnmrsystem/adm"
      set path = ($path "$vnmrsystem/tcl/bin")

      setenv TCL_LIBRARY   "$TCLDIR/tcllibrary"
      setenv TK_LIBRARY    "$TCLDIR/tklibrary"
      set WISH = "$vnmrsystem/tcl/bin/vnmrWish"

      "$WISH" "$TCLDIR/bin/add_printer" &

   else
      echo ""
      echo Be root then restart ./$0
      echo ""
   endif

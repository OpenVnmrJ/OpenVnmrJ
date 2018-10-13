#! /bin/csh
# '@(#)adddevices.sh 22.1 03/24/08 1991-1996 '
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
      if ( -r "$vnmrsystem/p11/p11Config" ) then
          set aa = `/usr/bin/cksum "$vnmrsystem/tcl/bin/add_printer" | awk '{print $1}'`
          set bb = `/bin/cat /usr/varian/sbin/add_printer.sum | awk '{print $1}'`

          if ( "x$aa" != "x$bb" ) then
             echo "The add printer tool had been altered since last installed"
             exit
          endif
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

if did_filetype() " found one already
   finish	  " no need to check for more
endif
if getline(1) =~ '^" \<magical\|macro\|Agilent\|Varian\>'
   setfiletype magical
endif

# 

#! /bin/sh

#  version for Solaris
#  output from SunOS (BSD?) and Solaris (SVR4?) are completely different

nm $1 | awk '
$7 == "|UNDEF" {

    #  Send to stdout a C program with each undefined symbol defined as a stub

               symbol = substr( $8, 2, length( $8 ) -1 )
               print symbol "()"
               print "{"
               print "}"
               print ""
          }
' >dummycmds.c

#!
#
# give a URL return the URL with the http:// added if missing
# if not a URL i.e. www. the return a null string 
# used by vnmr_open to handle URLs
#
use strict;

my $s = $ARGV[0];
#if    ($s =~ /^http:\/\/(.+)$/) { $s = $1; }   would remove the http:// from string
if    ($s =~ /^http:.+$/) { }
elsif ($s =~ /^www\..+$/)  { $s = "http://".$s; }
else                      { $s = ""; }
print  $s;

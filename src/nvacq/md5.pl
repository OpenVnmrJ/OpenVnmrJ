#!/usr/bin/env perl
# 
# Copyright (c) 1999-2000 Varian,Inc. All Rights Reserved. 
# This software contains proprietary and confidential
# information of Varian, Inc. and its contributors.
# Use, disclosure and reproduction is prohibited without
# prior consent.
#
use warnings;
use integer;
use Getopt::Long;
use Digest::MD5;

my $usage = "$0 [-output <file>] <file>\n";

my $output_file = "-";
my $obj_name = undef;
my $checksum = undef;

GetOptions( "output=s" => \$output_file) or die $usage;

open CFILE,">$output_file" or die "$output_file: $!\n";

$#ARGV >= 0 or die $usage;

if (!defined $obj_name) {
  $obj_name = $ARGV[0]
}

open BITFILE,$ARGV[0] or die "$ARGV[0]: $!\n";

my $mem;
my $cnt = 0;
while (read(BITFILE,$mem,1,$cnt++)) {}

my $ctx = Digest::MD5->new;
$ctx->add($mem);

#printf CFILE ("%s\n",
#	      $ctx->hexdigest);
printf CFILE ("%s %s\n",
	      $ctx->hexdigest,$ARGV[0]);
;

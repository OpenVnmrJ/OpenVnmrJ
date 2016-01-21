#!/bin/csh

# Copyright (C) Jaravine VA, Orekhov VYu (24.01.2007), Swedish NMR Centre, Gothenburg

#
# grep the params from input file  and arrange them into a command calling 'spheader'
# args:  rname.in  [rname.hdr_3]
# 
set nargs=${#argv}
 
if ( $nargs < 1 ) then
echo " $0  requires 1 or 2 filename arguments "
echo " Usage: "
echo " $0  rname.in  [rname.hdr_3]"
echo " "
echo " arg1 requied  : the file name is used as root for the header files "
echo " arg2 optional : the indices from the file will be used (without arg2 - the indices are randomly generated) "
exit;
endif


# 24.01.2007 : note temporary setting of the parameter for repair holes here ; meant to say how many is the minimal number of holes allowed (0, 1, 2 ...); -1 means no checks is made
set minhole=0

# check infile and access to split_dims.sh
set in=$1;  
if( ! -e $in ) then
  ls $in ; exit 1;
endif
#which split_dims.sh > /dev/null
#if( $status ) then
#  which split_dims.sh
#  exit 1
#endif

# the default base for splitting dimensions is '2'
set base=2

# set the root name from the *.in path
set rname=$in:r
#get all params after a keyword
set s=(`grep '^file'   $in`); if ( $#s<2 ) echo "error in field 'file'";   set expfile=$s[2]; 
set s=(`grep '^NDIM'   $in`); if ( $#s<2 ) echo "error in field 'NDIM'";   set mode=$s[2];
set s=(`grep '^seed'   $in`); if ( $#s<2 ) echo "error in field 'seed'";   set seed=$s[2];
set s=(`grep '^sptype' $in`); if ( $#s<2 ) echo "error in field 'sptype'"; set sptype=$s[2];
set s=(`grep '^SPARSE' $in`); if ( $#s<2 ) echo "error in field 'SPARSE'"; set sparse=$s[2];
# split string 'nyn' into 3 fields and rename it (for spheader syntax)
set s=(`grep '^f180'   $in`); if ( $#s<2 ) echo "error in field 'f180'";   set f180=(`echo "$s[2]" | sed -e '1,1 s/y/ y /g'  | sed -e '1,1 s/n/ n /g' `);
set s=(`grep '^CT_SP'  $in`); if ( $#s<2 ) echo "error in field 'CT_EXP'"; set ctsp=(`echo "$s[2]" | sed -e '1,1 s/y/ CT /g' | sed -e '1,1 s/n/ 0 /g' `);
set s=(`grep '^CEXP'   $in`); if ( $#s<2 ) echo "error in field 'CEXP'";   set cexp=(`echo "$s[2]" | sed -e '1,1 s/y/ y /g'  | sed -e '1,1 s/n/ n /g' `);
# just remove keyword from the read line
set s=(`grep '^NIMAX'  $in`); if ( $#s<2 ) echo "error in field 'NIMAX'";  set nimax=(`echo "$s" | sed -e '1,1 s/NIMAX//' `); 
set s=(`grep '^NIMIN'  $in`); if ( $#s<2 ) echo "error in field 'NIMIN'";  set nimin=(`echo "$s" | sed -e '1,1 s/NIMIN//' `); 
set s=(`grep '^NI '    $in`); if ( $#s<2 ) echo "error in field 'NI '";    set ni   =(`echo "$s" | sed -e '1,1 s/NI//' `);    
set s=(`grep '^SW'     $in`); if ( $#s<2 ) echo "error in field 'SW'";     set sw   =(`echo "$s" | sed -e '1,1 s/SW//' `);    
set s=(`grep '^T2'     $in`); if ( $#s<2 ) echo "error in field 'T2'";     set t2   =(`echo "$s" | sed -e '1,1 s/T2//' `);    
set s=(`grep '^Jsp'    $in`); if ( $#s<2 ) echo "error in field 'Jsp'";    set jsp  =(`echo "$s" | sed -e '1,1 s/Jsp//' `);   
set s=(`grep '^MINHOLE'    $in`); if ( $#s > 1 ) then 
  set minhole=$s[2] 
endif

# nimin-ni-nimax checks
set d=1;
while ($d <= $mode )
 if( $ni[$d] <= 0  ) then
  echo "$0 : ERROR: ni <= 0 for one of the dimensions " ;  exit
 endif
 if( $nimax[$d] < $ni[$d]  ) then
  echo "$0 : ERROR: nimax < ni for one of the dimensions " ;  exit
 endif
 if( $nimax[$d] <= $nimin[$d] ) then
  echo "$0 : ERROR: nimax <= nimin for one of the dimensions " ;  exit
 endif
 if( $d > 2 && $cexp[1] == n && $cexp[$d] == n && $nimax[$d] != $ni[$d] ) then
  echo "$0 : Warning: nimax2 must be equal to ni2 for cexp=nnn " ;  
 endif
 @ d++
end


 set opt1="$rname"
 set opt2=("$mode $sptype");
 set opt3="";
 set opt4="";
 set opt5="";
 set opt6="";
 set opt7="";
 set opt8="";
 set opt9="";

 if ( $nargs == 2 ) then
 set opt10=("file=$2 minhole=$minhole");
 else 
 set opt10=("seed=$seed minhole=$minhole");  
 endif 
 set opt11="";

#strcat param for each dim
set d=1; 
while ($d <= $mode )
 set a=$nimin[$d];	 					set opt3=("$opt3 $a");
 set a=$nimax[$d];	 					set opt4=("$opt4 $a");
 set a=$ni[$d];          					set opt5=("$opt5 $a");
 set a=(`$3 $nimax[$d] $base NDSIZES $cexp[$d]`);  	set opt6=("$opt6 $a");
 set a=(`$3 $nimax[$d] $base  DSIZES $cexp[$d]`);  	set opt7=("$opt7 $a");
 set a=$sw[$d];          					set opt8=("$opt8 $a");
 set a=$t2[$d];          					set opt9=("$opt9 $a");
 if ( $ctsp[$d] == CT ) then
 set a=(J=$jsp[$d],f180=$f180[$d],$ctsp[$d]);
 else 
 set a=(J=$jsp[$d]);
 endif
 								set opt11=("$opt11 $a");
 @ d++
end


 echo ./spheader " '$opt1' '$opt2' '$opt3' '$opt4' '$opt5' '$opt6' '$opt7' '$opt8' '$opt9' '$opt10' '$opt11' "
      $2   "$opt1" "$opt2" "$opt3" "$opt4" "$opt5" "$opt6" "$opt7" "$opt8" "$opt9" "$opt10" "$opt11"


 if( $sparse != 'y' ) echo "Warning: SPARSE!='y', SPARSE mode is off"


exit  

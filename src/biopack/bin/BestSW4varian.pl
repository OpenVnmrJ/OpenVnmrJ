#! /usr/bin/perl  
#-------------------------------------------------------------------------------
#  Script to calculate the best Spectral width from a peak list given by VNMRJ.
#  The input should be only the linewidth $RH and $RN.
#  The ouput is the file score.dat containing all SW values giving no more overlaps
#  compared with the full spectral width.
# To be executed as "./BestSW4varian.pl RH RN" where RH and RN are in Hz
#-----------------------------------------------------------------

# Input parameters for line width


$RH=$ARGV[0];	#in Hz
$RN=$ARGV[1];	#in Hz
$nucl=$ARGV[2];	#nucleus in indirect dimension


$k=0;
if ($nucl eq "15N") {$gamma=0.1;$k=1;}
if ($nucl eq "13C") {$gamma=0.25;$k=1;}
if ($nucl eq "1H")  {$gamma=1;$k=1;}
if ($k==0) {printf "!!!!!!!!!! No nucleus has been given in indirect dimension. Should be 1H, 15N or 13C  !!!!!!!!!!!!!!!!!\n";}

# Set some parameters for files
$score_file = "bestSW.dat";
$output_file = "output.dat";
$ass_file = "peaks.txt";

$type_fold=-1; #=1 if the folding occurs on the same side of the spectrum =-1 if on other side
$sign_fold=1; #=1 if one folding step keep the sign, =-1 if change sign

# grid search flags
$minSW=0.4; $maxSW=30; $dof_fin=120; 
$Npts_ref = 1000*30/$RN;  #number of points in indirect dimension for sw1p=$maxSW; ok for 15& 15
$Hpts = 1000*30/$RH;
$nb_dof=8;			#if =1, the increment in dof2 equals the resolution in N dimension (30/Npts_ref)
$nb_sw=$Npts_ref/$maxSW/10;			#if =1, the increment in SW equals the resolution in N dimension (SWmax/Npts_ref*nb_sw)



# End parameters settings
#-----------------------------------------------------------------
#-----------------------------------------------------------------

open (SCO,"> $score_file");
open (OUT,"> $output_file");


&READ_ASSIGNMENTS;
$field=$field_pp;	#in MHz


############### Check data

$diffN=$maxiN-$miniN;
$swHp=$maxiH-$miniH+1;

############### Main 
&filtre;
&OPTIMIZE_SW_DOF2;
printf SCO "%3.2f %s\n",$sw_min,$unique_peak_max;

##############################################################################################
sub OPTIMIZE_SW_DOF2
{



$sw_trial=$maxSW;
$r=0;

while ($sw_trial>=$minSW)
	{$r=$r+1;
	&OPTIMIZE_DOF2($sw_trial);
	$sw_trial=$sw_trial-0.1;
	}
}

##############################################################################################
sub OPTIMIZE_DOF2
{
my ($sw1p)=@_;



$dof2_def=120;

$mini_dof=round($dof2_def-$sw1p/2);
$maxi_dof=round($dof2_def+$sw1p/2);


$d=0;
if ($type_fold==1)
	{
	$dof2_trial=$mini_dof;
	while ($dof2_trial<=$maxi_dof)
		{
		$d++;
		$dof2_trial=$dof2_trial+$resN*$nb_dof;
		}
	}
else 
	{$dof2_trial=$dof_fin;
	&CREATE_SPECTRUM($dof2_def,$sw1p,$Npts,$grid_search);
	$indice_score++;
	$d++;
	}
} #OPTIMIZE_DOF2;


##############################################################################################
sub CREATE_SPECTRUM
{

my ($dof2p,$sw1p,$grid_search)=@_;
$newlistppm=NEWLIST($dof2p,$sw1p);

foreach $v (0..$peak_num)
	{	
	$i=1;
	$prox=abs(($newlistppm[2]{$v}-($dof2p+$sw1p/2))); if (abs(($newlistppm[2]{$v}-($dof2p-$sw1p/2)))<$prox) {$prox=abs(($newlistppm[2]{$v}-($dof2p+$sw1p)))}
	if ($prox<=$RN) {$k=1;} else {$k=0;} #prox=1 if the peak is close to the limit of the spectrum
	while (($tab[$v]==1) && ($i<=$contact[$v][0]))
	  {$w=$contact[$v][$i];

	$dH=($newlistppm[1]{$v}-$newlistppm[1]{$w})*$field;		#Hz
	$dX=($newlistppm[2]{$v}-$newlistppm[2]{$w})*$field*$gamma;	#Hz

	if ($type_fold==1  || $k==0) #this if and the following check for overlapping peak that may have their center on different sides of the border of the spectrum
			{
			if (distance($dH,$RH,$dX,$RN)<1) {$tab[$v]=0;$tab[$w]=0;}
			}
	if ($type_fold==-1 || $k==1)
			{
			$dw=$sw1p*$field*$gamma;
if ((distance($dH,$RH,$dX,$RN)<1)||(distance($dH,$RH,$dX+$dw,$RN)<1)||(distance($dH,$RH,$dX-$dw,$RN)<1)) {$tab[$v]=0;$tab[$w]=0;}
				}
	   $i=$i+1;	
     	   }	
	}

$unique_peak=0;
foreach $v (0..$peak_num)
	{
	if ($listppm[20]{$v}==1)
		{	
		$unique_peak=$tab[$v]+$unique_peak;
		}
	}

if ($r==1) {$unique_peak_max=$unique_peak;$sw_min=$sw1p;}
	 else 
	{
	if ($unique_peak>$unique_peak_max) 
		{$sw_min=$sw1p;$unique_peak_max=$unique_peak;
		}
	if ($unique_peak==$unique_peak_max) 
		{$sw_min=$sw1p;
		}
	
	}
printf OUT "%4s sw_trial= %3.3f unique= %3s over %3s  RN=%3.3f Hz RH=%3.3f Hz\n",$r,$sw1p,$unique_peak,$peak_num,$RN,$RH;			
return;
}	#CREATE_SPECTRUM


##############################################################################################
sub INITIALIZE_SPECTR
{
my ($Hpts,$Npts)=@_;
undef @spectr;
foreach $i (0..($Hpts-1))
	{
	foreach $j (0..($Npts-1))
		{
		$spectr[$i][$j]=0;
		}
	}

}	#INITIALIZE_SPECTR


##############################################################################################
sub distance
{
my ($dH,$RH,$dX,$RN)=@_;
$dist=($dH/$RH)*($dH/$RH)+($dX/$RN)*($dX/$RN);
return $dist;
}
##############################################################################################
##############################################################################################

sub filtre
{

$RH_max=$RH/$field;

foreach $v (0..$peak_num)
	{$i=1;$contact[$v][0]=0;
	foreach $w (0..$peak_num)
		{
		$d=$listppm[1]{$v}-$listppm[1]{$w};
		if ($d<0) {$d=-$d;}
		if ($d<$RH_max && $v!=$w)
			{
			$contact[$v][$i]=$w;
			$contact[$v][0]=$contact[$v][0]+1;
			$i=$i+1;
			
			}
		}
	}
}

##############################################################################################


sub round {
my($number) = shift;
return int($number + .5 * ($number <=> 0));
}	#round

##############################################################################################

sub NEWLIST
{
my ($dof2p,$sw1p)=@_;
$maxN=$dof2p+$sw1p/2;$minN=$dof2p-$sw1p/2;

foreach $v (0..$peak_num)
	{
	$newlistppm[1]{$v}=$listppm[1]{$v};
	$newlistppm[2]{$v}=$listppm[2]{$v};
	$newlistppm[3]{$v}=$listppm[3]{$v};
	$newlistppm[4]{$v}=1;	# number of times folded
	$newlistppm[5]{$v}=1;	# any overlap with other residue
	$tab[$v]=1;
	while ($newlistppm[2]{$v} >$maxN or $newlistppm[2]{$v} <$minN)
		{
		if ($newlistppm[2]{$v} > $maxN)
			{
			if ($type_fold==1)
				{
				$newlistppm[2]{$v}=2.0*$maxN-$newlistppm[2]{$v};
				$newlistppm[4]{$v}=$sign_fold*$newlistppm[4]{$v};
				}
			if ($type_fold==-1)
				{
				$newlistppm[2]{$v}=$newlistppm[2]{$v}-$sw1p;
				$newlistppm[4]{$v}=$sign_fold*$newlistppm[4]{$v};
				}
			}

		if ($newlistppm[2]{$v} < $minN)
			{
			if ($type_fold==1)
				{
				$newlistppm[2]{$v}=2.0*$minN-$newlistppm[2]{$v};
				$newlistppm[4]{$v}=$sign_fold*$newlistppm[4]{$v};
				}
			if ($type_fold==-1)
				{
				$newlistppm[2]{$v}=$newlistppm[2]{$v}+$sw1p;
				$newlistppm[4]{$v}=$sign_fold*$newlistppm[4]{$v};
				}
			}
		
		$i++;	
		}
}
return $newlistppm;
}	#NEWLIST

##############################################################################################
sub READ_ASSIGNMENTS
{
open (ASS,$ass_file) or die "Can't open $ass_file\n";
$peak_num=1;
$miniN=100000;$maxiN=-10000;
$miniH=100000;$maxiH=-10000;
$p=0;$lign=0;
$RN_pp=0;
$RH_pp=0;
while ($line = <ASS>) {
	        $line =~ s/^\s+//;	
		$lign=$lign+1;
		($residue, $csN, $csH, $vol) = split(/\s+/,$line);
if ($lign==1)	{$field_pp=$csH;}
		

if ($lign>7)	{		
		if ($residue==$peak_num )
		{
		$listppm[1]{$peak_num} = $csH; 
		$listppm[2]{$peak_num} = $csN;
		$listppm[20]{$peak_num} = 1;
		if ($csN>$maxiN)	{$maxiN=$csN;}
		if ($csN<$miniN)	{$miniN=$csN;}
		if ($csH>$maxiH)	{$maxiH=$csH;}
		if ($csH<$miniH)	{$miniH=$csH;}
		$listppm[3]{$peak_num} = $residue_number;
		$peak_num=$peak_num+1;
		$p=0;
		}
		else {
		     if ($p==0)	
				{
				$RN_pp=$RN_pp+$residue;
				$RH_pp=$RH_pp+$csN;
				$p=1;
				}
		     }
		}
}	
$peak_num=$peak_num-1;
$RN_pp=$RN_pp/$peak_num*$field_pp*0.1;
$RH_pp=$RH_pp/$peak_num*$field_pp;
close (ASS);
print OUT "# Read assignments from $ass_file\n";
print OUT "# Your spectrum a 1H-$nucl like spectrum \n";
print OUT "# number of peaks in the spectrum: $peak_num\n";
print OUT "# min: $miniN ppm ... max: $maxiN ppm \n";
printf OUT "# RN_from_file=$RN_pp RH_from_file=$RH_pp \n";
} #READ_ASSIGNMENTS

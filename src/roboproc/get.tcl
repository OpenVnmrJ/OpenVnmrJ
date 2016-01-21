#
#

gPuts "--------------- get.tcl-------------------\n"

set not_return_yet 1

source $env(vnmrsystem)/asm/tcl/wash.tcl
source $env(vnmrsystem)/asm/tcl/mix.tcl
source $env(vnmrsystem)/asm/tcl/transfer.tcl   
source $env(vnmrsystem)/asm/tcl/retrieve.tcl
source $env(vnmrsystem)/asm/info/default
source $env(vnmrsystem)/asm/info/racks

gPuts "----------- get.tcl: source $env(vnmrsystem)/asm/info/current\n"

# if no current just return, i.e. skip get.tcl 
if {[catch {source $env(vnmrsystem)/asm/info/current }] == 1} {
      gPuts "----------- get.tcl: No Current, Skipping get.tcl\n"
      return 0
}

gPuts "----------- get.tcl: source $getProtocol\n"

# first check the user's protocols then the systems
if {[file exists $userdir/asm/protocols/$getProtocol] == 1} {
      source $userdir/asm/protocols/$getProtocol
} elseif {[file exists $env(vnmrsystem)/asm/protocols/$getProtocol] == 1} {
      source $env(vnmrsystem)/asm/protocols/$getProtocol
}

if ($not_return_yet)  {  # putprotocol could of already injected
    gPuts "calling return $RackLoc $SampleNumber $SampleZone $SampleVolume $SampleHeight $SampleDepth\n"
   retrieve $RackLoc $SampleNumber $SampleZone $SampleVolume $SampleHeight $SampleDepth
}

# finish get so remove current to indicate no sample in probe
exec rm -f $env(vnmrsystem)/asm/info/current


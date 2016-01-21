#
#

gPuts "--------------- put.tcl-------------------\n"
set not_injected_yet 1

source $env(vnmrsystem)/asm/tcl/wash.tcl
source $env(vnmrsystem)/asm/tcl/mix.tcl
source $env(vnmrsystem)/asm/tcl/transfer.tcl   
source $env(vnmrsystem)/asm/tcl/inject.tcl
source $env(vnmrsystem)/asm/info/racks
source $env(vnmrsystem)/asm/info/default
catch { source $env(vnmrsystem)/asm/info/sampInfo }

gPuts "----------- put.tcl: source $SampleInfoFile\n"

if {[catch {source $SampleInfoFile }] == 1} {
    if {[catch {source $env(vnmrsystem)/asm/info/current }] == 1} {
      gPuts "----------- put.tcl: No Current, Skipping put.tcl\n"
      return 0
     }   
}

# ------  message to gilson's display -----------
set msg [format "R%dZ%dS%d" $RackLoc $SampleZone $SampleNumber ]
gWriteDisplay "$msg"

gPuts "------------ put.tcl: source $putProtocol \n"

# first check the user's protocols then the systems
if {[file exists $userdir/asm/protocols/$putProtocol] == 1} {
      source $userdir/asm/protocols/$putProtocol
} elseif {[file exists $env(vnmrsystem)/asm/protocols/$putProtocol] == 1} {
      source $env(vnmrsystem)/asm/protocols/$putProtocol
}


if ($not_injected_yet)  {  # putprotocol could of already injected

#  ----- Switch gas valve to vent NMR Probe to Atmosphere  ----------
   gSetContacts 2 0

   gPuts " -------------- calling wash needle \n"
   wash needle

   gPuts " -------------- calling wash probe \n"
   wash probe

   set msg [format "R%dZ%dS%d" $RackLoc $SampleZone $SampleNumber ]
   gWriteDisplay "$msg"

   gPuts "calling inject $RackLoc $SampleNumber $SampleZone $SampleVolume $SampleHeight $SampleDepth\n"
   inject $RackLoc $SampleNumber $SampleZone $SampleVolume $SampleHeight $SampleDepth
}

if { [file exist $SampleInfoFile] == 1} {
    exec rm -f -f $env(vnmrsystem)/asm/info/current
#   copy sample in magnet into current
    exec mv $SampleInfoFile $env(vnmrsystem)/asm/info/current
#   copy sample flows and volumes of sample in magnet into current
    exec cat $env(vnmrsystem)/asm/info/sampInfo >> $env(vnmrsystem)/asm/info/current
}

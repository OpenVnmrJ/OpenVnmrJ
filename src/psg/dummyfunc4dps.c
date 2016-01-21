/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>

/* the following SISCO functions are temperarily deactivated */
S_shapedpulse()  {};
S_decshapedpulse() {};
S_simshapedpulse() {};
S_vgradient() {};
S_incgradient() {};
S_shapedgradient() {};
S_oblique_gradient() {};
S_oblique_shapedgradient() {};
S_phase_encode_gradient() {};
S_phase_encode_shapedgradient() {};
S_phase_encode3_gradient() {};
S_phase_encode3_shapedgradient() {};
S_position_offset() {};
S_position_offset_list() {};
S_getarray() {};
sli() {};
vsli() {};
init_vscan() {};
Init_vscan() {};
vscan() {};

/****************************/

/*** gemini ***/
indirect() {};

/**************/

lk_hold() {};
lk_sample() {};
blankingoff() {};
blankingon() {};
blankon() {};
blankoff() {};

Create_delay_list() {};
Create_freq_list() {};
create_delay_list() {};
create_freq_list() {};
create_offset_list() {};


dec3phase() {};
dec3prgoff() {};
decoffset() {};
dec2offset() {};
dec3offset() {};

decshapedpulse() {};
decstepsize() {};
dec2stepsize() {};
dec3stepsize() {};



genpulse() {};
genrgpulse() {};
gensaphase() {};

incgradient() {};
init_gpattern() {};
init_gradpattern() {};
init_rfpattern() {};
ioffset() {};

obl_gradient() {};
obl_shapedgradient() {};
oblique_gradient() {};
oblique_shapedgradient() {};
obsoffset() {};
obsstepsize() {};

pe_gradient() {};
pe_shapedgradient() {};
pe2_gradient() {};
pe2_shapedgradient() {};
pe3_gradient() {};
pe3_shapedgradient() {};

phase_encode_gradient() {};
phase_encode_shapedgradient() {};
phase_encode3_gradient() {}
phase_encode3_shapedgradient() {};

poffset() {};
poffset_list() {};
position_offset() {};
position_offset_list() {};

pwrm() {};
ipwrm() {};
rlpwrm() {};
rfon() {};
rfoff() {};

setstatus() {};
shapedincgradient() {};
shaped_INC_gradient() {};
shaped_V_gradient() {};
shaped2Dgradient() {};
shapedvgradient() {};
simshapedpulse() {};

vfreq() {};
voffset() {};

zgradpulse() {};

APsetreceiver()  {};

APSHAPED_DEC2PULSE()  {};

APSHAPED_DECPULSE()  {};

APSHAPED_PULSE()  {};

DCPLR2PHASE()  {};

DCPLRPHASE()  {};

DEC2OFF()  {};

DEC2ON()  {};

DEC2PHASE()  {};

DEC2PRGOFF()  {};

DEC2PRGON()  {};

DEC2RGPULSE()  {};

DEC2SHAPED_PULSE()  {};

DEC2SPINLOCK()  {};

DECOFF()  {};

DECON()  {};

DECPHASE()  {};

DECPRGOFF()  {};

DECPRGON()  {};

DECPULSE()  {};

DECRGPULSE()  {};

DECSHAPED_PULSE()  {};

DECSPINLOCK()  {};

DELAY()  {};

GENPULSE()  {};

GENQDPHASE()  {};

GENRGPULSE()  {};

GENSAPHASE()  {};

G_Delay()  {};

G_Offset() {};

G_Power()  {};

G_Pulse()  {};

G_RTDelay()  {};

G_Sweep()  {};

HSgate()  {};

IDEC2RGPULSE()  {};

IDECPULSE()  {};

IDECRGPULSE()  {};

IDELAY()  {};

INCDELAY()  {};

INITDELAY()  {};

IOBSPULSE()  {};

IOFFSET()  {};

IPULSE()  {};

IPWRF()  {};

IRGPULSE()  {};

Mlintfile()  {};

OBSPRGOFF()  {};

OBSPRGON()  {};

OBSPULSE()  {};

ObjectNew()  {};

PULSE()  {};

PWRF()  {};

RFOFF()  {};

RFON()  {};

RGPULSE()  {};

ROTORPERIOD()  {};

ROTORSYNC()  {};

SETAUTOINCREMENT()  {};

SETDIVNFACTOR()  {};

SHAPED_PULSE()  {};

SIM3SHAPED_PULSE()  {};

SIMPULSE()  {};

SIMSHAPED_PULSE()  {};

SPINLOCK()  {};

SetAPAttr()  {};

SetAPBit()  {};

SetAttnAttr()  {};

SetDeviceAttr()  {};

SetEventAttr()  {};

TSADD()  {};

TSDIV()  {};

TSMULT()  {};

TSSUB()  {};

TTADD()  {};

TTDIV()  {};

TTMULT()  {};

TTSUB()  {};

TXPHASE()  {};

VDELAY()  {};

XGATE()  {};

XMTROFF()  {};

XMTRON()  {};

XMTRPHASE()  {};

abort()  {};

acquire()  {};

add()  {};

all_grad_reset() {};

apcodes()  {};

apovrride()  {};

apshaped_decpulse() {};

apshaped_dec2pulse() {};

apshaped_pulse() {};

assign()  {};

calc_amp_tc() {};

check_bounds()  {};

checkforcomments()  {};

checktable()  {};

clearapdatatable()  {};

command_wg()  {};

convertdbl()  {};

dbl()  {};

dec2blank()  {};

dec2unblank()  {};

dec3blank()  {};

dec3unblank()  {};

decblank()  {};

decblankoff()  {};

decblankon()  {};

declvloff()  {};

declvlon()  {};

decouplepower()  {};

decouplerattn()  {};

decpwr()  {};

decr()  {};

decunblank()  {};

divn()  {};

dofiltercontrol()  {};

donoisecalc()  {};

dowlfiltercontrol()  {};

ecc_handle() {};

elemindex()  {};

elemvalues()  {};

elsenz()  {};

endhardloop()  {};

endif()  {};

endloop()  {};

eventovrhead() {};

gate()  {};

gatedecoupler()  {};

gatedmmode()  {};

gen2spinlock()  {};

gen_apshaped_pulse() {};

genshaped_pulse()  {};

gensim2pulse()  {};

gensim2shaped_pulse()  {};

gensim3pulse()  {};

gensim3shaped_pulse()  {};

genspinlock()  {};

getelem()  {};

getorientation()  {};

getstr2nwarn() {};

gradient()  {};

hlv()  {};

hsdelay()  {};

ifmi()  {};

ifnz()  {};

ifzero()  {};

incdelay()  {};

incr()  {};

initHSlines()  {};

initdecmodfreq()  {};

initialRF()  {};

initval()  {};

load_element()  {};

loadtable()  {};

loop()  {};

mod2()  {};

mod4()  {};

modn()  {};

mult()  {};

no_grad_wg()  {};

notinhwloop()  {};

obsblank()  {};

observepower()  {};

obsunblank()  {};

offset()  {};

okinhwloop()  {};

open_table()  {};

orr()  {};

pfg_12s() {};

pfg_20() {};

pfg_blank() {};

pfg_enable() {};

pfg_quick_zero() {};

pfg_reg() {};

pfg_reset(where) {};

pfg_select_addr() {};

pgradient() {};

phaseshift()  {};

phase_var_type() {};

point_wg()  {};

pop()  {};

pow()  {};

power()  {};

preacqdelay()  {};

prg_dec_off()  {};

prg_dec_on()  {};

ptsconv()  {};

ptsofs()  {};

pulseampfilter()  {};

pulse_phase_type() {};

push()  {};

pwrf()  {};

rcvroff()  {};

rcvron()  {};

read_number()  {};

reparse_release()  {};

reset_table()  {};

rfbandselect()  {};

rgradient()  {};

rlpower()  {};

setPTS()  {};

setSISPTS()  {};

set_observech()  {};

setapchip()  {};

setdirectPTS()  {};

setlkfrqflt()  {};

setoffsetsyn()  {};

setparm()  {};

setprgmode()  {};

setreceiver()  {};

setrfattenuation()  {};

settable()  {};

shaped_2D_Vgradient()  {};

shaped_2D_gradient()  {};

shapedgradient()  {};

sp1off()  {};

sp1on()  {};

sp2off()  {};

sp2on()  {};

spareoff()  {};

spareon()  {};

starthardloop()  {};

status()  {};

stepsize()  {};

store_in_table()  {};

sub()  {};

sync_on_event()  {};

table_math()  {};

tabsetreceiver() {};

tablertv()  {};

tablesop()  {};

tabletop()  {};

test4acquire()  {};

timerwords()  {};

validrtvar()  {};

var1off()  {};

var1on()  {};

var2off()  {};

var2on()  {};

vgradient()  {};

wfg_remap()  {};

wgtb()  {};

write_to_acqi()  {};

writedebug()  {};

writetable()  {};

zero_all_gradients() {};

startfifo() {};

peloop() {};

msloop() {};

endmsloop() {};

endpeloop() {};

initparms_sis() {};

Create_offset_list() {};

setHSLdelay() {};

vget_elem() {};

loop_check() {};



/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif

/**********************************************************

	z gradient mapping sequence based on the Oneshot pulse sequence
	allows measurement of phase sensitive or absolute value profiles

	from MCDoneKshotread5_new500.c

	Doneshot - corrected dosytimecubed 04iv01 (previous version saved as Doneshot_04iv01) CLE
	Corrected GAM/MJS 10iv01 - * not + for Dtau in dosytimecubed calculation
	Add explicit rcvron GAM  7v04
	Doneshot - Oneshot DOSY sequence based on modifications to the
	Bipolar pulse stimulated echo (BPPSTE) sequence
	Adapted by P Sandor from Vnmr6.1B MDP 26v99.
	MC29iv04 - Modified to include tweek parameter to minimise phase disturbances.
	MC6v04 - Modified to include tweek2 and tweek3 parameters to minimise phase disturbances.
	MC3xii04 - remove tweek 2 and tweek3 and fix original 'tweak' to be defined as a % change
		in last gradient pulse
	MC3xii04 - move heating pulses to 0.1 s (delay gss) before 1st 90
	MC6xii04 - change tweak to no. of DAC points added on to total gzlvl to correct imbalance
	MC10xii04 - alternate sign of gzlvl3 on successive transients
	MC10ii05 - exorcycle all 5 pulses in phasecycling
	MC11ii05 - remove phasecycleflag and replace with STEflag to give an option of STE or STAE
	MC2iii05 - now able to calculate phase of any pathway by using num.
	MC2iii05 - now able to set RF pulses to zero to follow specific CTP's
	MC16iii05 - fix gradient signs for STE or STAE like those in 1shot paper

	GM22ii09 - new name

Parameters:
        delflag   - 'y' runs Doneshot_nugmap sequence
                    'n' runs a normal s2pul sequence
        avflag    - 'n' runs Doneshot sequence with a read gradient
                  - 'y' as above plus an increased gradient pulse top move the echo to the middle of at
	del       -  the actual diffusion delay.
	gt1       - total diffusion-encoding pulse width.
        gzlvl1    - diffusion-encoding pulse strength
        gstab     - gradient stabilization delay
        gt3       - spoiling gradient duration
        gzlvl3    - spoiling gradient strength
        gzlvl_max - maximun accepted gradient strength
                       32767 with PerformaII, 2047 with PerformaI
        gzlvl_read- gradient strength during acquisition, typically around 25 DAC points
        kappa     - unbalancing factor between bipolar pulses as a
                       proportion of gradient strength (~0.2)
        tweak     - correction to final gradient pulse, typically around -1 DAC point
        awm       - 0 selects absolute value experiment
                    1 selects phase sensitive experiment
     nugcal_[1-5] - a 5 membered parameter array summarising the results of a
                        calibration of non-uniform field gradients.
                        Created by the Doneshot_nugmap sequence
           probe_ - stores the probe name used for the dosy experiment

Select tof to be on-resonance on the hdo signal.
The parameters for the heating gradients (gt4, gzlvl4) are calculated
in the sequence. They cannot be set directly.

	Constant energy dissipation calculation corrected 8vii99

**********************************************************/

#include<standard.h>

pulsesequence()
{
int     selectCTP = getval("selectCTP");
double	kappa = getval("kappa"), 
	gzlvl1 = getval("gzlvl1"),
	gzlvl3 = getval("gzlvl3"),
        gzlvl_max = getval("gzlvl_max"),
	gt1 = getval("gt1"),
	gt3 = getval("gt3"),
	del = getval("del"),
        gstab = getval("gstab"),
        gss = getval("gss"),
	tweak = getval("tweak"),
	gzlvl_read=getval("gzlvl_read"),
	num=getval("num"),
        hsgt = getval("hsgt"),
        gzlvlhs =  getval("gzlvlhs"),
	avm,gzlvl4,gt4,Dtau,Ddelta,dosytimecubed,dosyfrq;
char alt_grd[MAXSTR],avflag[MAXSTR],delflag[MAXSTR],STEflag[MAXSTR],sspul[MAXSTR];

gt4 = 2.0*gt1;
getstr("alt_grd",alt_grd);
getstr("delflag",delflag);
getstr("avflag",avflag);
getstr("STEflag",STEflag);
getstr("sspul",sspul);

avm=0.0;
if(avflag[0]=='y')
{
	avm=1.0;
}

/* Decrement gzlvl4 as gzlvl1 is incremented, to ensure constant 
   energy dissipation in the gradient coil 
   Current through the gradient coil is proportional to gzlvl */

gzlvl4=sqrt(2.0*gt1*(1+3.0*kappa*kappa)/gt4)*sqrt(gzlvl_max*gzlvl_max/((1+kappa)*(1+kappa))-gzlvl1*gzlvl1);

/* In pulse sequence, del>4.0*pw+3*rof1+2.0*gt1+5.0*gstab+gt3 */

if ((del-(4*pw+3.0*rof1+2.0*gt1+5.0*gstab+gt3)) < 0.0)
   { del=(4*pw+3.0*rof1+2.0*gt1+5.0*gstab+gt3);
     printf("Warning: del too short; reset to minimum value\n");}

if ((d1 - (gt3+gstab) -2.0*(gt4/2.0+gstab)) < 0.0)
   { d1 = (gt3+gstab) -2.0*(gt4/2.0+gstab);
     printf("Warning: d1 too short;  reset to minimum value\n");}

if ((abs(gzlvl1)*(1+kappa)) > gzlvl_max)
   { abort_message("Max. grad. amplitude exceeded: reduce either gzlvl1 or kappa\n");
     }

if (ni > 1.0)
   { abort_message("This is a 2D, not a 3D dosy sequence: please set ni to 0 or 1\n");
     }

Ddelta=gt1;
Dtau=2.0*pw+gstab+gt1/2.0+rof1;
dosyfrq = getval("sfrq");
dosytimecubed=del+(gt1*((kappa*kappa-2)/6.0))+Dtau*((kappa*kappa-1.0)/2.0);
dosytimecubed=(gt1*gt1)*dosytimecubed;
putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);

/* This section determines the phase calculation for any number of transients   */
 
  initval(num,v14);
  add(v14,ct,v13);

/* phase cycling calculation */

if(delflag[0]=='y')
{
	mod4(v13,v3);	/* 1st 180,  0 1 2 3	*/
	hlv(v13,v9);	
	hlv(v9,v9);		
	mod4(v9,v4);	/* 2nd 90,  (0)4 (1)4 (2)4 (3)4  */
	hlv(v9,v9);
	hlv(v9,v9);
	mod4(v9,v1);	/* 2nd 180, (0)16 (1)16 (2)16 (3)16  */ 
	hlv(v9,v9);
	hlv(v9,v9);
	mod4(v9,v2);	/* 1st 90,  (0)64 (1)64 (2)64 (3)64  */
	hlv(v9,v9);
	hlv(v9,v9);
	mod4(v9,v5);	/* 3rd 90,  (0)256 (1)256 (2)256 (3)256  */

	if(STEflag[0]=='y')
				{
	dbl(v2,v6); assign(v6,oph); sub(oph,v1,oph);
        sub(oph,v3,oph); sub(oph,v4,oph); dbl(v5,v6);
        add(v6,oph,oph); mod4(oph,oph);                /* receiver phase for STE */
				}
	else

				{
 	assign(v1,oph); dbl(v2,v6); sub(oph,v6,oph);
        add(v3,oph,oph); sub(oph,v4,oph); dbl(v5,v6);
        add(v6,oph,oph); mod4(oph,oph);                /* receiver phase for STAE */
				}
	
}
   mod2(ct,v7);		/* change sign of gradients on alternate transients */
   status(A);

   if(delflag[0]=='y')
	{

	if (sspul[0]=='y')
       		{
         	zgradpulse(gzlvlhs,hsgt);
         	rgpulse(pw,zero,rof1,rof1);
         	zgradpulse(gzlvlhs,hsgt);
       	}

	delay(d1 - (gt3+gstab) -2.0*(gt4/2.0+gstab)-gss); /* Move d1 to here */

      	zgradpulse(-1.0*gzlvl4,gt4/2.0); /* 1st dummy heating pulse */
   	delay(gstab);

	zgradpulse(gzlvl4,gt4/2.0); /* 2nd dummy heating pulse */
	delay(gstab);

	delay(gss); /* Short delay to acheive steady state */

        if (alt_grd[0] == 'y')
        {
                ifzero(v7);
                zgradpulse(-1.0*gzlvl3,gt3);
                elsenz(v7);
                zgradpulse(gzlvl3,gt3);
                endif(v7);
        }
	else	zgradpulse(-1.0*gzlvl3,gt3); /* Spoiler gradient balancing pulse */
	delay(gstab); 
        }
	else delay(d1);

   status(B); /* first part of sequence */
   if(delflag[0]=='y')
   {
   	if(gt1>0 && abs(gzlvl1)>0)
   	{
		if(selectCTP==2)
		{
   		rgpulse(0.0, v1, rof1, rof2);		/* first 90, v1 */
		}
	else
   		rgpulse(pw, v1, rof1, rof2);		/* first 90, v1 */

	zgradpulse(gzlvl1*(1.0+kappa),gt1/2.0); /*1st main gradient pulse*/
   	delay(gstab);
		if(selectCTP==2)
		{
		rgpulse(0.0, v2, rof1, rof2);		/* first 180, v2 */
		}
	else
		rgpulse(pw*2.0, v2, rof1, rof2);		/* first 180, v2 */

	zgradpulse(-1.0*gzlvl1*(1.0-kappa),gt1/2.0); /*2nd main grad. pulse*/
   	delay(gstab);
		if((selectCTP==1)||(selectCTP==2))
		{
   		rgpulse(0.0, v3, rof1, rof2);		/* second 90, v3 */
		}
	else
   		rgpulse(pw, v3, rof1, rof2);		/* second 90, v3 */

	zgradpulse(-gzlvl1*2.0*kappa,gt1/2.0); /*Lock refocussing pulse*/
   	delay(gstab);

                if (alt_grd[0] == 'y')
                {
                        ifzero(v7);
                        zgradpulse(gzlvl3,gt3); /* Spoiler gradient */
                        elsenz(v7);
                        zgradpulse(-1.0*gzlvl3,gt3);
                        endif(v7);
                }
	else	zgradpulse(gzlvl3,gt3); /* Spoiler gradient  */

   	delay(gstab);

	delay(del-4.0*pw-3.0*rof1-2.0*gt1-5.0*gstab-gt3); /* diffusion delay */

		if(STEflag[0]=='y')
		{
		zgradpulse(2.0*kappa*gzlvl1,gt1/2.0); /*Lock refocussing pulse*/
   		delay(gstab);
		}
	else
		{
		zgradpulse(-2.0*kappa*gzlvl1,gt1/2.0); /*Lock refocussing pulse*/
   		delay(gstab);
		}
		if(selectCTP==1)
		{
   		rgpulse(0.0, v4, rof1, rof2);		/* third 90, v4 */
		}
	else
   		rgpulse(pw, v4, rof1, rof2);		/* third 90, v4 */

		if(STEflag[0]=='y')
		{
		zgradpulse(-gzlvl1*(1.0+kappa),gt1/2.0); /*3rd main gradient pulse*/
   		delay(gstab);
		}
	else
		{
		zgradpulse(gzlvl1*(1.0+kappa),gt1/2.0); /*3rd main gradient pulse*/
   		delay(gstab);
		}

	rgpulse(pw*2.0, v5, rof1, rof2);	/* second 180, v5 */
rcvron();
		if(STEflag[0]=='y')
		{
		zgradpulse(1.0*(1.0-kappa)*gzlvl1+tweak-avm*at*gzlvl_read/gt1,gt1/2.0); /*4th main grad. pulse*/
   		delay(gstab);
		}
	else
		{
		zgradpulse(-1.0*(1.0-kappa)*gzlvl1-tweak-avm*at*gzlvl_read/gt1,gt1/2.0); /*4th main grad. pulse*/
   		delay(gstab);
		}
	rgradient('z',gzlvl_read);
   	}
   }
   else rgpulse(pw,oph,rof1,rof2);
   status(C);

}

/****************************************************************************
1024 (or 16,32,64,128,256,512) steps phase cycle
Order of cycling: v2 0,1,2,3; v4 [0]4,[1]4,[2]4,[3]4; v5 [0]16,[1]16,[2]16,[3]16; v1 [0]64,[1]64,[2]64,[3]64; v3 [0]256,[1]256,[2]256,[3]256;

receiver = v1-2*v2+v3-v4+2*v5

Coherence pathway for Stimulated Anti-Echo (STAE):

	90	180	 90		90	180	  Acq.
+2
                  ________                _______
+1               |        |              |       |
                 |        |              |       |
 0------|        |        |--------------|       |
        |        |                               |
-1      |________|                               |____________________

-2
	v1       v2        v3             v4      v5
****************************************************************************/

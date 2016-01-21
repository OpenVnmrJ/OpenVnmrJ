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
/* 
 */
/****************************************************************************  
    Dcosyidosy.c - pulsed field gradient COSY (magnitude mode) 
	with coherence transfer pathway selection pulses 
	separated by delay del to give diffusion encoding 
	with full gcosy sensitivity 
 
    Nilsson M, Gil AM, Delgadillo I, Morris GA. Chem. Commun. 2005 1737-1739. 
	 
Parameters: 
	del - the actual diffusion delay. 
	gt1 - total diffusion-encoding pulse width. 
     gzlvl1 - gradient amplitude (-32768 to +32768) 
     gzlvl2 - gradient amplitude for coherence selection (-32768 to +32768) 
	gt2 - gradient duration for coherence selection in seconds (0.001) 
      gstab - optional delay for stability 
  alt_grd   - flag to invert gradient sign on alternating scans
                        (default = 'n')
 lkgate_flg - flag to gate the lock sampling off during    
                              gradient pulses
       del2 - delay parameter that can shift the convection compensation
                    sequence elements off the center of the pulse sequence
                    allowing to run a velocity profile
                    can also be negative but in absolute value cannot exceed
                    del/2 minus the gradient and gradient-stabilization delays
                    (default value for diffusion measurements is zero)
    satmode - 'yn' turns on presaturation during satdly
	      'yy' turns on presaturation during satdly and del
	      the presauration happens at the transmitter position
	      (set tof right if presat option is used)
     satdly - presatutation delay (part of d1) 
     satpwr - presaturation power 
        wet - flag for optional wet solvent suppression
      sspul - flag for a GRD-90-GRD homospoil block
    gzlvlhs - gradient level for sspul
       hsgt - gradient duration for sspul
      phase - 1 (selects echo N-type coherence selection; default) 
              2 (selects antiecho P-type coherence selection)  
   convcomp - 'y': selects convection compensated cosyidosy
              'n': normal cosyidosy
  nugflag   - 'n' uses simple mono- or multi-exponential fitting to
                  estimate diffusion coefficients
              'y' uses a modified Stejskal-Tanner equation in which the
                  exponent is replaced by a power series
nugcal_[1-5] - 5 membered parameter array summarising the results of a
                  calibration of non-uniform field gradients. Used if
                  nugcal is set to 'y'
                  requires a preliminary NUG-calibration by the 
                  Deoneshot_nugmap sequence
 dosy3Dproc - 'ntype' calls dosy with 3D option with N-type selection
              'ptype' calls dosy with 3D option with P-type selection 
     probe_ - stores the probe name used for the dosy experiment

*********************************************************************************/ 
 
#include <standard.h> 
#include <chempack.h>
 
pulsesequence() 
{ 
 double gstab = getval("gstab"),
        gt1 = getval("gt1"),
        gzlvl1 = getval("gzlvl1"),
        gt2 = getval("gt2"),
        gzlvl2 = getval("gzlvl2"),
        del=getval("del"),
        del2=getval("del2"),
        dosyfrq=getval("sfrq"),
        phase = getval("phase"),
        satpwr = getval("satpwr"),
        satdly = getval("satdly"),
        gzlvlhs = getval("gzlvlhs"),
        hsgt = getval("hsgt"),
        Ddelta,dosytimecubed,icosel,delcor1,delcor2,delcor3;
 char convcomp[MAXSTR],satmode[MAXSTR],alt_grd[MAXSTR],lkgate_flg[MAXSTR],
      sspul[MAXSTR]; 
 
  getstr("convcomp",convcomp); 
  getstr("satmode",satmode);
  getstr("alt_grd",alt_grd);
  getstr("lkgate_flg",lkgate_flg);
  getstr("sspul",sspul);
 
       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",1.0);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);
          gt2 = syncGradTime("gt2","gzlvl2",1.0);
          gzlvl2 = syncGradLvl("gt2","gzlvl2",1.0);

/* phase cycling calculation */ 
 
  icosel=1;    /* Default to N-type experiment */ 
 
  if (phase == 2.0)   
     { icosel=-1;  
	  mod4(ct,v1); 
	  hlv(ct,v2); 
	  hlv(v2,v2); 
	  mod4(v2,v2);  
	  if (ix==1) fprintf(stdout,"P-type COSY\n"); 
     }  
  else 
     {	  mod4(ct,v1); 
	  hlv(ct,v2); 
	  hlv(v2,v2); 
	  mod4(v2,v2); 
 	  dbl(v2,v3); 
	  sub(v3,v1,oph); 
	  mod4(oph,oph); 
          if (ix==1) fprintf(stdout,"N-type COSY\n"); 
     } 
 
  mod2(ct,v10);        /* gradients change sign at odd transients */
  Ddelta=gt1;/*the diffusion-encoding pulse width is gt1*/ 
  if (convcomp[A]=='y') 
  	 dosytimecubed=Ddelta*Ddelta*(del - (4.0*Ddelta/3.0)); 
  else 
  	 dosytimecubed=Ddelta*Ddelta*(del - (Ddelta/3.0)); 
   
  if (phase==2.0)
      putCmd("makedosyparams(%e,%e) dosy3Dproc='ptype'\n",dosytimecubed,dosyfrq);
  else
      putCmd("makedosyparams(%e,%e) dosy3Dproc='ntype'\n",dosytimecubed,dosyfrq);

  delcor1=(del+del2)/4.0-gt1;
  delcor2=(del-del2)/4.0-gt1;
  if ((del < 4.0*gt1)&&(convcomp[0] == 'y'))
     {
         abort_message("Dcosyidosy: increase diffusion delay at least to  %f seconds",
                     (gt1*4.0));
     }
  if ((delcor2 < 0.0)&&(convcomp[0] == 'y'))
     {
         abort_message("Dcosyidosy: decrease del2 to less than %f to fit into the diffusion delay",
                     (del-gt1*4.0));
     }
  delcor3=(del-gt1-2*rof1-pw)/2;
  if ((delcor3 < 0.0)&&(convcomp[0] == 'n'))
     {
         abort_message("Dcosyidosy: increase the diffusion delay at least to %f second",
                     (gt1*2+rof1+pw));
     }
 
  /* BEGIN ACTUAL PULSE SEQUENCE */ 
  status(A);  
     if (sspul[0]=='y')
       {
         zgradpulse(gzlvlhs,hsgt);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(gzlvlhs,hsgt);
       }
     if (satmode[0] == 'y')
      {
        if (d1 - satdly > 0) delay(d1 - satdly);
        obspower(satpwr);
        rgpulse(satdly,zero,rof1,rof1);
        obspower(tpwr);
      }
     else
     {  delay(d1); }

 if (getflag("wet")) wet4(zero,one);

  status(B); 
  if (convcomp[A]=='y')  
    {
         if (lkgate_flg[0] == 'y') lk_hold();   /* turn lock sampling off */
          if (icosel > 0)
           {
            if (alt_grd[0] == 'y')
            {  ifzero(v10);
                 zgradpulse(-gzlvl2,2*gt2);
               elsenz(v10);
                 zgradpulse(gzlvl2,2*gt2);
               endif(v10);
            }
            else zgradpulse(-gzlvl2,2*gt2);
            delay(gstab);    
          }
      rgpulse(pw,v1,rof1,rof1); 
         delay(d2+gstab);
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(-gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1);
          if (satmode[1] == 'y')
           { obspower(satpwr);
             rgpulse(delcor1,zero,rof1,rof1);
             obspower(tpwr);
           }
          else delay(delcor1);
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(-gzlvl1,gt1);
          delay(gstab); 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(-gzlvl1,gt1);
          if (satmode[1] == 'y')
           { obspower(satpwr);
             rgpulse(delcor2,zero,rof1,rof1);
             obspower(tpwr);
           }
          else delay(delcor2);
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(-gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1);
	 delay(gstab); 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl2,gt2);
            elsenz(v10);
              zgradpulse(-gzlvl2,gt2);
            endif(v10);
         }
         else zgradpulse(gzlvl2,gt2);
         delay(gstab);
     rgpulse(pw,v2,rof1,rof2);
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(icosel*gzlvl2,gt2);
            elsenz(v10);
              zgradpulse(-1.0*icosel*gzlvl2,gt2);
            endif(v10);
         }
         else zgradpulse(icosel*gzlvl2,gt2);
          delay(gstab);
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(-gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1);
          if (satmode[1] == 'y')
           { obspower(satpwr);
             rgpulse(delcor2,zero,rof1,rof1);
             obspower(tpwr);
           }
          else delay(delcor2);
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(-gzlvl1,gt1);
         delay(gstab); 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(-gzlvl1,gt1);
          if (satmode[1] == 'y')
           { obspower(satpwr);
             rgpulse(delcor1,zero,rof1,rof1);
             obspower(tpwr);
           }
          else delay(delcor1);
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(-gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1);
	  delay(gstab); 
         if (lkgate_flg[0] == 'y') lk_sample();     /* turn lock on */ 
    }   
  else 
    {
         if (lkgate_flg[0] == 'y') lk_hold();   /* turn lock off */
          if (icosel > 0) 
           {
            if (alt_grd[0] == 'y')
            {  ifzero(v10);
                 zgradpulse(-gzlvl1,2.0*gt1);
               elsenz(v10);
                 zgradpulse(gzlvl1,2.0*gt1);
               endif(v10);
            }
            else zgradpulse(-gzlvl1,2.0*gt1);
            delay(gstab);    
          }
    rgpulse(pw,v1,rof1,rof1); 
         delay(d2); 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(-gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1);
          if (satmode[1] == 'y')
           { obspower(satpwr);
             rgpulse(delcor3,zero,rof1,rof1);
             obspower(tpwr);
           }
          else delay(delcor3);
    rgpulse(pw,v2,rof1,rof2);	 
          if (satmode[1] == 'y')
           { obspower(satpwr);
             rgpulse(delcor3,zero,rof1,rof1);
             obspower(tpwr);
           }
          else delay(delcor3);
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(icosel*gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(-icosel*gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(icosel*gzlvl1,gt1);
         delay(gstab); 
         if (lkgate_flg[0] == 'y') lk_sample();     /* turn lock on */
    }	 
 status(C); 
}  

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
/****************************************************************************

  
    cpmg1 - Single point CPMG T2 pulse sequence

		p1 - {r - p2 -r}*n 
		
            ticks=1 enables external trigger  
            p1pat - pulse shape (=hard)
            tpwr1  - pulse power (dB)
            p1,p2  - 90,180deg pulses Note: hard pulses used
            bt    - t2 delay; set to btarray
            te	  - user defined delay unit, r 
                    equivalent to te/2
            
 Version: 20070511 
  SGL version

****************************************************************************/

#include <standard.h>
#include "sgl.c"
/* Phase cycling of 180 degree pulse */
static int ph180[2] = {1,3};

pulsesequence()
{ 
  double  aqtm     = getval("aqtm");      /* "extra" sampling time for filters */
  double  acqdelay = getval("acqdelay");  /* minimum delay between echoes */
  double  r1,r2,r3,r,pd,seqtime,dw;
  double restol, resto_local;
	  
  int     vnp      = v1;
  int     vnp_ctr  = v2;
  int  vph180     = v7;  /* Phase of 180 pulse */

  /* Single point acquisition */
  setacqmode(WACQ|NZ);

  init_mri();              /****needed ****/

  restol=getval("restol");   //local frequency offset

  dw = 1.0/sw;

  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);   /* hard pulse */
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  init_rf(&p2_rf,p2pat,p2,flip2,rof1,rof2);   /* hard pulse */
  calc_rf(&p2_rf,"tpwr2","tpwr2f");

   r1 = (te/2)-(p1/2)-rof2-rof1-(p2/2)-alfa;  /* p1-r1-p2-    */
   r2 = (te/2)-(p2/2)-rof2-(dw/2)-alfa;
   r3 = (te/2)-(p2/2)-rof1-(dw/2);
   r = (p1/2)-rof2-rof1-(p2/2)-alfa;   /* correct delay for pulse width; assume rof1=rof2 */
   if ((r1<=0)||(r2<=0)||(r3<=0))
     abort_message("%s: Inter pulse delay(te) too short. r1,r2,r3 = %f,%f,%f ms",seqfil,r1,r2,r3);
   seqtime = p1+rof1+r1+alfa+(p2+rof1+rof2+r2+r3+dw)*np/2;
   
   pd = tr - seqtime;  /* predelay based on tr */
   if (pd <= 0.0) {
      abort_message("%s: Requested tr too short.  Min tr = %f ms",seqfil,seqtime*1e3);
    }

  resto_local=resto-restol;

  /* PULSE SEQUENCE *************************************/
  	
  status(A);
  initval(np/2.0, vnp);
  settable(t2,2,ph180);        /* initialize phase tables and variables */
  getelem(t2,ct,v6);  /* 180 deg pulse phase alternates +/- 90 off the rcvr */
  add(oph,v6,vph180);      /* oph=zero */

  obsoffset(resto_local); 
  obspower(p1_rf.powerCoarse);
  obspwrf(p1_rf.powerFine);
  xgate(ticks);
  delay(pd);
  rgpulse(p1,oph,rof1,rof2);    /* oph=zero */
  obspower(p2_rf.powerCoarse);
  obspwrf(p2_rf.powerFine);   
  startacq(alfa);
  delay(r1);
      loop(vnp,vnp_ctr);        
        rgpulse(p2,vph180,rof1,rof2);
        delay(r2);
        startacq(alfa);	
	sample(dw);
        endacq();
        delay(r3);  
	
      endloop(vnp_ctr);
//      if (aqtm > at) sample(aqtm-at); /* "extra" sampling time for filters */
  endacq();
  delay(acqdelay);                /* minimum delay for end of acqn. */

}


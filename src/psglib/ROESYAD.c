// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* ROESYAD - 

	Features included:
		States-TPPI in F1
		Solvent suppression during relaxation & mixNing delays
		
	Parameters:
		sspul   :	selects magnetization randomization option
		satmode :	yn - presaturation during relax delay
				ny - presaturation during mixing
				yy - presaturation during relax
					and mixing delays
                wet     :       yn - wet after relax delay
                                ny - wet after mixing
                                yy - wet after relax mixing delays
		mixR	:	ROESY mixing time

KrishK	-	Last revision	: June 1997
KrishK	-	Revised - July 2004
HaitaoH	-	ZQ suppression - Sept. 2004
KrishK	-	Revised for Cp3 - May 2005
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***

*/


#include <standard.h>
#include <chempack.h>

static shape mixsh;

static char shapename1[MAXSTR];

static int phs1[32] = {0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,1,1,1,1,3,3,3,3},
           phs2[32] = {2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1},
           phs3[32] = {0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,1,1,1,1,3,3,3,3},
           phs4[32] = {2,0,2,0,0,2,0,2,3,1,3,1,1,3,1,3,3,1,3,1,1,3,1,3,0,2,0,2,2,0,2,0},
           phs5[32] = {0,0,2,2,0,0,2,2,1,1,3,3,1,1,3,3,1,1,3,3,1,1,3,3,2,2,0,0,2,2,0,0},
           phs6[32] = {0,2,2,0,2,0,0,2,1,3,3,1,3,1,1,3,1,3,3,1,3,1,1,3,2,0,0,2,0,2,2,0};
		
pulsesequence()
{
   double          mixR = getval("mixR"),
		   
		   gzlvlz = getval("gzlvlz"),
		   gtz = getval("gtz"),tpos1,tpos2,
		   gstab = getval("gstab"),
                   slpwrR = getval("slpwrR"),
                   spinlockR = getval("spinlockR"),
                   tiltfactor = getval("tiltfactor"),
                   tpwr_cf = getval("tpwr_cf"),
                   tof = getval("tof"); 
                   
   char		   satmode[MAXSTR],
                   zfilt[MAXSTR],
                   cmd[MAXSTR],
		   wet[MAXSTR];
   int		   phase1 = (int)(getval("phase")+0.5),
		   prgcycle = (int)(getval("prgcycle")+0.5);


/* LOAD VARIABLES */
   getstr("satmode",satmode);
   getstr("wet",wet);
   getstr("zfilt",zfilt);

  
   tpos1 = tof + (spinlockR/tiltfactor);
   tpos2 = tof - (spinlockR/tiltfactor);
 

  assign(ct,v17);
  assign(zero,v18);
  assign(zero,v19);

  if (getflag("prgflg") && (satmode[0] == 'y') && (prgcycle > 1.5))
    {
        hlv(ct,v17);
        mod2(ct,v18); dbl(v18,v18);
        if (prgcycle > 2.5)
           {
                hlv(v17,v17);
                hlv(ct,v19); mod2(v19,v19); dbl(v19,v19);
           }
     }

   settable(t1,32,phs1);
   settable(t2,32,phs2);
   settable(t3,32,phs3);
   settable(t5,32,phs5);
   settable(t4,32,phs4);
   settable(t6,32,phs6);
  
   getelem(t1,v17,v6);   /* v6 - presat */
   getelem(t2,v17,v1);   /* v1 - first 90 */
   getelem(t3,v17,v2);   /* v2 - 2nd 90 */
   getelem(t4,v17,v7);   /* v7 - presat during mixN */
   getelem(t5,v17,v3);   /* v3 - 3rd 90 */
   getelem(t6,v17,oph);  /* oph - receiver */

   add(oph,v18,oph);
   add(oph,v19,oph);

 
   /*  Make adiabatic roesy mixing shape */
     if(FIRST_FID)
       {
         opx("Pbox");
         pboxpar("steps", 1000.0);
         mixsh = pbox_shape("roesweep", "amwu", 0.0, 0.0, 0.0,0.0);
       }
   

/*
   mod2(id2,v14);
   dbl(v14,v14);
*/

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   if (phase1 == 2)                                        /* hypercomplex */
     {  incr(v1); incr(v6); }

   add(v1, v14, v1);
   add(oph,v14,oph);
    

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
   obspower(tpwr);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
	if (getflag("slpsat"))
	     {
		shaped_satpulse("relaxD",satdly,v6);
               	if (getflag("prgflg"))
                   shaped_purge(v1,v6,v18,v19);
             }

	else
	     {
        	satpulse(satdly,v6,rof1,rof1);
		if (getflag("prgflg"))
		   purge(v1,v6,v18,v19);
	     }
     }
   else
        delay(d1);

   if (wet[0] == 'y')
     wet4(zero,one);


   status(B);
      rgpulse(pw, v1, rof1, 2.0e-6);
      if (d2>0)
       delay(d2- rof1 -(4*pw/PI));  /*corrected evolution time */
      else
       delay(d2);
      rgpulse(pw, v2, rof1, rof1);
      if (zfilt[0] == 'y')
         zgradpulse(gzlvlz,gtz);
      delay(gstab);
      obsoffset(tpos1);
      obspower(slpwrR);
      shapedpulse("roesweep",mixR/2.0,v2,rof1,rof2);
      
      obsoffset(tpos2);
      shapedpulse("roesweep",mixR/2.0,v2,rof1,rof2);
      obspower(tpwr);
      obsoffset(tof);
      if (zfilt[0] == 'y')
         zgradpulse(gzlvlz*0.62,gtz);
      delay(gstab);
   status(C);
      rgpulse(pw, v3, rof1, rof2);
}

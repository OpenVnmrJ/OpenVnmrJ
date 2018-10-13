// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* NOESY - 

	Features included:
		States-TPPI in F1
		Solvent suppression during relaxation & mixNing delays
		
	Paramters:
		sspul   :	selects magnetization randomization option
		satmode :	yn - presaturation during relax delay
				ny - presaturation during mixing
				yy - presaturation during relax
					and mixing delays
                wet     :       yn - wet after relax delay
                                ny - wet after mixing
                                yy - wet after relax mixing delays
		mixN	:	NOESY mixing time

KrishK	-	Last revision	: June 1997
KrishK	-	Revised - July 2004
HaitaoH	-	ZQ suppression - Sept. 2004
KrishK	-	Revised for Cp3 - May 2005
*/


#include <standard.h>
#include <chempack.h>

static int	phs1[16] = {0,2,0,2,0,2,0,2,1,3,1,3,1,3,1,3},
		phs2[32] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
                            2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3},
		phs3[16] = {0,0,1,1,2,2,3,3,1,1,2,2,3,3,0,0},
                phs5[16] = {1,1,0,0,1,1,0,0,2,2,3,3,2,2,3,3},
		phs4[32] = {0,2,1,3,2,0,3,1,1,3,2,0,3,1,0,2,
                            2,0,3,1,0,2,1,3,3,1,0,2,1,3,2,0};
		
pulsesequence()
{
   double          mixN = getval("mixN"),
		   mixNcorr,
		   gzlvl1 = getval("gzlvl1"),
		   gt1 = getval("gt1"),
		   gstab = getval("gstab"),
                   zqfpw1 = getval("zqfpw1"),
                   zqfpwr1 = getval("zqfpwr1"),
                   gzlvlzq1 = getval("gzlvlzq1");
   char		   satmode[MAXSTR],
		   zqfpat1[MAXSTR],
		   wet[MAXSTR];
   int		   phase1 = (int)(getval("phase")+0.5);

/* LOAD VARIABLES */
   getstr("satmode",satmode);
   getstr("wet",wet);
   getstr("zqfpat1",zqfpat1);
   mixNcorr = 0.0;
   if (getflag("PFGflg"))
   {
	mixNcorr = gt1 + gstab;
	if (getflag("Gzqfilt"))
		mixNcorr += gstab + zqfpw1;
   	if (wet[1] == 'y')
		mixNcorr += 4*(getval("pwwet")+getval("gtw")+getval("gswet"));
   }

   if (mixNcorr > mixN)
	mixN=mixNcorr;

   settable(t1,16,phs1);
   settable(t2,32,phs2);
   settable(t3,16,phs3);
   settable(t5,16,phs5);
   settable(t4,32,phs4);
   
   getelem(t1,ct,v1);
   getelem(t4,ct,oph);
   assign(oph,v7);
   assign(zero,v6);
   getelem(t2,ct,v2);
   getelem(t3,ct,v3);
/*
   mod2(id2,v14);
   dbl(v14,v14);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   if (phase1 == 2)                                        /* hypercomplex */
      incr(v1);

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
        satpulse(satdly,v6,rof1,rof1);
     }
   else
        delay(d1);

   if (wet[0] == 'y')
     wet4(zero,one);

   status(B);
      rgpulse(pw, v1, rof1, rof1);
      if (d2>0)
       delay(d2- 2*rof1 -(4*pw/PI));  /*corrected evolution time */
      else
       delay(d2);
      rgpulse(pw, v2, rof1, rof1);

      if (satmode[1] == 'y')
	satpulse((mixN-mixNcorr)*0.7,v7,rof1,rof1);
      else
	delay((mixN - mixNcorr)*0.7);

      if (getflag("PFGflg"))
      {
        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr1);
         rgradient('z',gzlvlzq1);
         delay(100.0e-6);
         shaped_pulse(zqfpat1,zqfpw1,zero,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(gstab);
	 obspower(tpwr);
        }
        zgradpulse(gzlvl1,gt1);
        delay(gstab);
      }
      if (satmode[1] == 'y')
        satpulse((mixN-mixNcorr)*0.3,v7,rof1,rof1);
      else
	delay((mixN - mixNcorr)*0.3);
      if (wet[1] == 'y')
	wet4(zero,one);
   status(C);
      rgpulse(pw, v3, rof1, rof2);
}

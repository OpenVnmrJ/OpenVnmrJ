// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* e.cosy - r. kyburz, 1988-04-22
	    r. kyburz, 1991-09-21 (updated)
	    r. kyburz, 2002-12-13 (adjusted for VNMR 6.1C) */

/* references: 
       R. Brueschweiler, C. Griesinger, O.W. Sorensen & R.R.Ernst,
             private communication (1987).
       C. Griesinger, O.W. Sorensen & R.R. Ernst,
             J.Am.Chem.Soc. 107, 6394 (1985).

   Parameters:	see man('ecosy')
*/
/*
   KrishK - Modified to work with chempack
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/

#include <standard.h>
#include <chempack.h>

pulsesequence()
{   
   int	phase1 = (int)(getval("phase")+0.5),
  	prgcycle=(int)(getval("prgcycle")+0.5);


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

/* CONSTANTS FOR PHASE CALCULATIONS */
    initval( 8.0,v13);
    initval( 32.0,v12);
    initval( 20.0,v11);
    initval( 192.0,v10);

/* CALCULATE PHASECYCLE */
   assign(zero,v14);	/* phase of first pulse */
   mod2(v17,v1);
   dbl(v1,v1);		/* 0202 */
			/* even/odd flag */
   hlv(v17,v2);
   hlv(v2,v3);
   dbl(v3,v3);		/* 0000222244446666 */
			/* phase for transients 3 + 4*n */
			/* 1+4*n = 0 */
   mod2(v2,v2);		/* 0011 */
			/* switch for pairs */
   assign(v13,v4);	/* 8 */
   ifzero(v2);
      incr(v4);
   elsenz(v2);
      decr(v4);
   endif(v2);
   modn(v4,v13,v4);	/* 1177 */
			/* standard phases for even transients */
			/*      1 for 2+4*n, 7 for 4*n         */
   hlv(v13,v8);		/* 4 */
   add(v17,v8,v5);	/* (ct+4) */
   divn(v5,v12,v5);	/* (ct+4)/32 */
   divn(v17,v12,v6);	/* ct/32 */
   sub(v5,v6,v5);	/* ((ct+4)/32-ct/32 */
			/* 00000000 00000000 00000000 00001111 */
   add(v17,v11,v6);	/* (ct+20) */
   divn(v6,v10,v6);	/* (ct+20)/192 */
   sub(v11,v7,v7);	/* 16 */
   add(v17,v7,v7);	/* (ct+16) */
   divn(v7,v10,v7);	/* (ct+16)/192 */
   add(v5,v6,v5);
   sub(v5,v7,v5);	/* ((ct+4)/32-ct/32)+((ct+20)/192-(ct+16)/192) */
                        /* flag for exceptions on even transients */
   dbl(v2,v6);		/* 0022 */
   add(v6,three,v6);	/* 3355 */
   ifzero(v1);		/* for odd transients */
      ifzero(v2);       /* 1+4n:                             */
         assign(zero,v3);             /* 0xxx 0xxx 0xxx 0xxx */
      endif(v2);        /* 3+4n:         xx0x xx2x xx4x xx6x */
   elsenz(v1);		/* for even transients */
      ifzero(v5);	/* normal case:        */
         assign(v4,v3); /*                x1x7 */
      elsenz(v5);	/* exceptions:         */
         assign(v6,v3); /*                x3x5 */
      endif(v5);	/* v3 = phase of first and second pulse */
			/*      in 45 degrees steps:            */
                        /* 01070127 01470167 01070127 01470365  */
                        /* 01070127 01470167 01070127 01470365  */
                        /* 01070127 01470167 01070127 01470365  */
                        /* 01070127 01470167 01070127 01470365  */
                        /* 01070127 01470167 01070127 01470365  */
                        /* 01070127 01470365 01070127 01470365  */
   endif(v1);
   assign(two,v4);	/* v4 = phase of last 90 degrees pulse */
   assign(v1,oph);	/* oph = 0202 */

   assign(zero,v20);
  if (getflag("prgflg") && (satmode[0] == 'y'))
        assign(v14,v20);
  add(oph,v18,oph);
  add(oph,v19,oph);

   if (phase1 == 2) 
	incr(v14); /* States - Habercorn */

/*
      mod2(id2,v9);
      dbl(v9,v9);
*/
	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v9);

      add(v14,v9,v14);
      add(oph,v9,oph);
 
/* BEGIN ACTUAL PULSE SEQUENCE CODE */
   status(A);

   obsstepsize(45.0);
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
		if (getflag("prgflg"))
			xmtrphase(v3);
                shaped_satpulse("relaxD",satdly,v20);
                if (getflag("prgflg"))
		{
                   shaped_purge(v14,v20,v18,v19);
		   xmtrphase(zero);
		}
           }
        else
           {
                if (getflag("prgflg"))
			xmtrphase(v3);
                satpulse(satdly,v20,rof1,rof1);
                if (getflag("prgflg"))
		{
                   purge(v14,v20,v18,v19);
		   xmtrphase(zero);
		}
           }
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   status(B);
      xmtrphase(v3);
      if (getflag("cpmgflg"))
      {
         rgpulse(pw, v14, rof1, 0.0);
         cpmg(v14, v15);
      }
      else
         rgpulse(pw, v14, rof1, 2.0e-6);
      if (d2 > 0.0)
         delay(d2 - (4.0*pw/PI) - 4.0e-6);
      else
	delay(d2);
      rgpulse(pw, zero, 2.0e-6, rof1);
      xmtrphase(zero);
      rgpulse(pw, v4, rof1, rof2);
   status(C);
} 

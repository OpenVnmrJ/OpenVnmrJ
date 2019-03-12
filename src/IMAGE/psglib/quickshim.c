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

 quickshim - Fast x,y,z shimming sequence 

****************************************************************************/

#include <standard.h>
static int ph1[4] = {0,2,1,3},              /* 90 deg pulse phase */
	   phr[4] = {0,2,1,3};              /* receiver phase */

void pulsesequence()
{
	double  predelay,seqtime,glimit,gshimlim;
	double  gbasex,gbasey,gbasez,del;
	double  shimno;
	double  gdaclim,shimscale;
	double  delx,dely,delz;

    	del = getval("del");
    	gbasex = getval("gbasex");
    	gbasey = getval("gbasey");
    	gbasez = getval("gbasez");
	shimno = getval("shimno");
	shimscale = getval("shimscale");
    	gshimlim = getval("gshimlim");  /* shim dac limit */
	gdaclim = gshimlim*shimscale;  /* gradient dac limit */

//	initparms_sis();

	initval(1, v1);   /* no of complex point segments to acquire */

        seqtime = at+pw+rof1+rof2+te+(2*trise);
        predelay = tr - seqtime;  /* predelay based on tr */
        if (predelay <= 0.0) {
          abort_message("%s: TR too short.  Min TR = %f ms",seqfil,seqtime*1e3);
        }

	/* check if shim limit is exceeded */
	if(shimno > 0) {	
	  glimit = abs(del)+abs(gbasex);
	  if((glimit > gshimlim) || (glimit >= 1600))
	  abort_message("X shim limit exceeded, %5.0f %5.0f %5.0f %5.3f", glimit,gshimlim,del,shimscale);
	  glimit = abs(del)+abs(gbasey);
	  if((glimit > gshimlim)  || (glimit >= 1600))
	    abort_message("Y shim limit exceeded, %5.0f", glimit);
	  glimit = abs(del)+abs(gbasez);
	  if((glimit > gshimlim) || (glimit >= 1600))
	    abort_message("Z shim limit exceeded, %5.0f", glimit);
	}

	delx = 0; dely = 0; delz = 0;
	if(shimno == 1) delx = del;
	if(shimno == 2) dely = del;
	if(shimno == 3) delz = del;

    	settable(t1,4,ph1);    /* initialize phase tables and variables */
    	getelem(t1,ct,v7);     /* 90 deg pulse phase */
	settable(t4,4,phr);
    	getelem(t4,ct,oph);    /* receiver phase */
	initval(1,v3);   
	initval(1,v4);
	initval(1,v5);
	initval(ssc,v6);
	assign(zero,v2);

	status(A);
        rotate();
        
	obspower(tpwr);
	delay(4e-6);
	loop(v6,v8);		/* steady state dummy scans */
	  delay(2*trise);
	  delay(predelay); 
	  pulse(pw,v7);
	  delay(te);
	  delay(at); 
	endloop(v8);

        loop(v1,v2);	        /* compressed acqn not used */
          sub(v2,v3,v9);	/* v9-v11 not used */
	  assign(zero,v10);
	  assign(zero,v11);
          gradient('x',gbasex+delx);
          gradient('y',gbasey+dely);
          gradient('z',gbasez+delz);
          delay(trise);
          delay(predelay);
          pulse(pw,v7);
          delay(te);

          startacq(alfa);
          acquire(np,1.0/sw);
          endacq();
        endloop(v2); 
	delay(trise);           
	zero_all_gradients();
}

/*******************************************************************
		Modification History
		
2005Oct07(ss) - sequence cleaned up; glimit,delx-z added

*******************************************************************/

/* water_WG -   experiment with solvent suppression by watergate.  
Literature reference: 
*/ 

#include <standard.h> 
#include <chempack.h> 

static int 

    ph1[1] = {0}, 
    ph4[4] = {2,3,0,1},
    phr[4] = {0,2,0,2};
  
pulsesequence() 
{ 
   double          prgtime = getval("prgtime"),
                   prgpwr = getval("prgpwr"), 
		   phincr2 = getval("phincr2"),
                   phincr1 = getval("phincr1"); 
 
 
/* LOAD VARIABLES */ 
 
   rof1 = getval("rof1"); if (rof1 > 2.0e-6) rof1=2.0e-6;
   if (phincr1 < 0.0) phincr1=360+phincr1;
   if (phincr2 < 0.0) phincr2=360+phincr2;
   initval(phincr1,v9); 
   initval(phincr2,v10);

/* CALCULATE PHASECYCLE */ 
         
   sub(ct,ssctr,v12);
   settable(t1,1,ph1);		getelem(t1,v12,v1); 
   settable(t4,4,ph4);		getelem(t4,v12,v4);
   settable(t6,4,phr); 	getelem(t6,v12,oph);
 
   if (getflag("alt_grd")) mod2(ct,v6);
               /* alternate gradient sign on every 2nd transient */
 
/* BEGIN THE ACTUAL PULSE SEQUENCE */ 
 status(A); 

   if (getflag("lkgate_flg"))  lk_sample(); /* turn lock sampling on */

   obspower(tpwr); delay(5.0e-5);

   if (getflag("sspul"))
        steadystate();

   delay(d1);

   if (getflag("lkgate_flg"))  lk_hold(); /* turn lock sampling off */

     status(B); 
      if (getflag("WGmode"))
      {
      	if (getflag("flipback")) 
            FlipBack(v1,v9);            /* water flipback pulse */
        if (getflag("cpmgflg"))
        {
          rgpulse(pw, v1, rof1, 0.0);
          cpmg(v1, v15);
        }
        else
      	   rgpulse(pw, v1, rof1, rof1); 
	WGpulse(v4,v10,v6);
       	if (getflag("prg_flg"))
           { obspower(prgpwr);
             add(v1,one,v1);
             rgpulse(prgtime,v1,rof1,rof2);
           }
       	else 
	    delay(rof2);
      }
      else
        if (getflag("cpmgflg"))
        {
          rgpulse(pw, v1, rof1, 0.0);
          cpmg(v1, v15);
        }
        else
	  rgpulse(pw,v1,rof1,rof2); 
     status(C);
} 

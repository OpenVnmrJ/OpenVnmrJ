/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* C13observe_BB  
     spinecho option following s2pul
     p1 and pw pulses are at pwClvl power level
     spinecho pulse is shp_pw at power shp_pwr/shp_pwrf

     Coil ringing and acoustic ringing requires delay before
     acquisition, particularly for cold probes (200-400 usec). This
     is done here by using a double spin-echo.  
  Evgeny Tischenko, Agilent, December 2011
*/

#include <standard.h>

static int  
                      
	     phi1[]  = {0,2,0,2},
             rec[]   = {0,2,0,2};
void pulsesequence()
{
    
char        gpurge[MAXSTR],shape[MAXSTR],   		       
	    phase_roll_comp[MAXSTR];	      


   double  shp_pw=getval("shp_pw"),
           shp_pwr=getval("shp_pwr"),
           shp_pwrf=getval("shp_pwrf"), 
           gstab=getval("gstab"),
           gt=getval("gt"), 
           gzlvl=getval("gzlvl");


getstr("gpurge",gpurge);
getstr("phase_roll_comp",phase_roll_comp);
getstr("shape",shape);
  

/*   LOAD PHASE TABLE    */
	settable(t1,4,phi1);
	settable(t12,4,rec);

    status(A);
      obspower(tpwr);
      obsoffset(tof);        
      obspwrf(4095.0);
      delay(d1);
    if(gpurge[A]=='y')
      {
       zgradpulse(0.777*gzlvl,gt); 
       delay(gstab);
      }
    status(B);   /*for T1 experiments*/
       rgpulse(p1,zero,rof1,rof1);
       delay(d2);

       rgpulse(pw,t1,rof1,rof1);
       obspower(shp_pwr);        obspwrf(shp_pwrf);

    if(phase_roll_comp[A]=='y')
        {
      	 delay(10.0e-6);
     	 shaped_pulse(shape,shp_pw,zero,rof1,rof1);
      	 delay(10.0e-6+2.0/3.1415*pw+rof1);
        }

     if(gpurge[B]=='y')
        {
         zgradpulse(gzlvl,gt);
        };

      delay(gstab);
      shaped_pulse(shape,shp_pw,zero,rof1,rof1);

      if(gpurge[B]=='y')
        {
         zgradpulse(gzlvl,gt);
        }

      delay(gstab);

      if(phase_roll_comp[A]!='y') {delay(2.0/3.1415*pw+rof1);} 

      setreceiver(t12);
   status(C);



     
}

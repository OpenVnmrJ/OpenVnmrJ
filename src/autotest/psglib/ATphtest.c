// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* phtest - test of phase shifts of observe or decouple transmitters

     Now works for direct synthesis observe, fixed freq or direct
     synthesis decouple 11/3/88.  Transmitter and receiver phase
     cycling work on type b transmitters 12/7/88
     D. J. Wilbur  	

     cancel=x: completes a full cycle of xmtr phase increments,
               with phase steps equal to 'stpsiz', which should
               sum to zero when transformed.  the receiver will
               phase cycle if nt is large enough to allow the
               transmitter to complete more than a full cycle.

     cancel=r: completes a full cycle of rcvr 90 degree steps,
               with the xmtr phase held constant at 'stpsiz'.

     cancel=n: performs a single experiment with phase equal
               to 'mphase*stpsiz'.  this is most useful when
               mphase is arrayed to cover a 360 cycle.

        dec=y: performs the above using the decoupler. use
               this only with a direct synthesis decoupler.

        dec=n: this is the normal mode, for use with a direct
               synthesis transmitter. */




#include "standard.h"


pulsesequence()
{

double mphase,nsteps,stpsiz;
char cancel[MAXSTR],dec[MAXSTR];
   
    getstr("cancel",cancel);
    stpsiz=getval("stpsiz");
    getstr("dec",dec);
    if (newtrans)
        obsstepsize(stpsiz);
    if (newdec)
        decstepsize(stpsiz);
    if (cancel[0] == 'x') 
    {   
        nsteps = 360.0/stpsiz;
        initval(nsteps,v2);
        modn(ct,v2,v3);       /* v3 = 0,1,2,...nsteps-1 */
        divn(ct,v2,v4);       /* v4=0,0,..,nsteps-1,1,1,..,nsteps-1,.. */
        mod4(v4,oph);       /* set receiver phase */
    
        status(A);
        delay(d1);
        rcvroff();
        if (dec[0] == 'y') 
        {   if (newdec)
            {   dcplrphase(v3);
                decrgpulse(pw,v4,rof1,rof2);
            }
             else

                decrgpulse(pw,v3,rof1,rof2);
        } 
    
        else 
        if (newtrans)
             
        {   xmtrphase(v3);
            pulse(pw,v4);
        } 
             else
                pulse(pw,v3);
        status(C);
    } 

    if (cancel[0] == 'r') 
    {   status(A);
        delay(d1);
        if (dec[0] == 'y') 
        {   if (newdec)
            {   dcplrphase(one);
                decrgpulse(pw,zero,rof1,rof2);
            }
            else
            {   decrgpulse(pw,zero,rof1,rof2);
            }
        } 
        else 
        if (newtrans)
        {   xmtrphase(v3);
            pulse(pw,v4);
        } 
             else
                pulse(pw,zero);

        status(C);
    } 

    if (cancel[0] == 'n') 
    {   
        mphase=getval("mphase");       /* stpsiz multiplier */
        initval(mphase,v1);
        if (dec[0] == 'y') 
          {  
            dcplrphase(v1);
            delay(d1);
            decrgpulse(pw,oph,rof1,rof2);
          }
        else
          {
            xmtrphase(v1);
            delay(d1);
            obspulse();
          }
    } 
} 

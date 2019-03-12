// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  ATgecho - gradient echo 
              with option of axis and halfsine shape
    Parameters: 
        gzlvl1 = gradient amplitude (-32768 to +32768)
        gt1 = gradient duration in seconds (e.g. 0.002)   
        gradaxis = direction of gradient
          shaped = 'y' produces a halfsine, 'n' a rectangular

	Since there are 20 elements in the gradient shape, 
	the gradient duration MUST BE greater than 173 us on Unityplus
	(the stepsize on the Unityplus must be greater
	than 8.65us = 6.9us(APBUS) + 1.75us(min for serialization); 
	The gradient duration for INOVA MUST be greater than
	115 us because the stepsize must be greater
	than 5.75us = 4.0(APBUS) + 1.75us(min for serialization);
         (debbie mattiello, varian, palo alto)
*/
#include <standard.h>

void halfsineshapegrad(char grad,double gval,double gtim)
{
   static double mgrad[20]= {0.0,0.1564,0.3090,0.4540,0.5878,0.7071,0.8090,
			0.8910,0.9510,0.9877,1.0,0.9877,0.9510,0.8910,
			0.8090,0.7071,0.5878,0.4540,0.3090,0.1564};
   double gstep,gampl;
   int jcnt;
   if ((gtim/20.0) < (8.65e-6))
     gstep=8.65e-6;
   else 
     gstep = ((gtim)/20.0);
   for (jcnt=0;jcnt<=19;jcnt++) 
     { gampl=(mgrad[jcnt]*gval);
      rgradient(grad,gampl);
      delay(gstep - GRADIENT_DELAY); }
   rgradient(grad,0.0);
}


void pulsesequence()
{
   double loops,gzlvl1,gt1,gzlvl2,gt2;
   char   shaped[MAXSTR],gradaxis[MAXSTR];

   getstr("shaped",shaped);
   getstrnwarn("gradaxis",gradaxis);
   if (( gradaxis[A] != 'x') && ( gradaxis[A] != 'y') && ( gradaxis[A] != 'z') )
      strcpy(gradaxis,"z");

   gzlvl1 = getval("gzlvl1");
   gt1 = getval("gt1");
   gzlvl2 = getval("gzlvl2");
   gt2 = getval("gt2");
   loops = getval("loops");
   if (loops<1.0) loops=1.0;
   initval(loops,v1);

   status(A);
   lk_sample();
   delay(d1);
   lk_hold();

   status(B);
   rgpulse(pw, zero,rof1,rof2);
   loop(v1,v2);
   if (shaped[A]=='y')
    {
     if (gt1<0.0002) 
      {
       printf("set gradient greater than 200usec for sine shape");
       psg_abort(1);
      }
     halfsineshapegrad(gradaxis[A],gzlvl1,gt1);
    }
   if (shaped[A]=='s')
    zgradpulse(gzlvl1,gt1);
   if (shaped[A]=='n')
    {
     rgradient(gradaxis[A],gzlvl1);
     delay(gt1);
     rgradient(gradaxis[A],0.0);
    }
   delay(d2);
   if (shaped[A]=='y')
    {
     if (gt2<0.0002) 
      {
       printf("set gradient greater than 200usec for sine shape");
       psg_abort(1);
      }
     halfsineshapegrad(gradaxis[A],gzlvl2,gt2);
    }
   if (shaped[A]=='s')
    zgradpulse(gzlvl2,gt2);
   if (shaped[A]=='n')
    {
     rgradient(gradaxis[A],gzlvl2);
     delay(gt2);
     rgradient(gradaxis[A],0.0);
    }
   delay(d3);
   endloop(v2);
   lk_sample();
}


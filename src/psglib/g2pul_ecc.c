// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* g2pul_ecc.c   combination of g2pul.c and p2pul.c  

   	0 <= amp <= 100  
   	tcmax[4]={0.235,2.35,2.36,23.5}            ECC amps and times
	tc[1]: 0.020915 - 0.235000
	tc[2]: 0.209150 - 2.350000       these units are millisec !!!
	tc[3]: 0.209    - 2.360000   This one modified in Highland!!
	tc[4]: 2.091500 - 23.50000        
	  tcmax/11 <= tc <= tcmax               */  


#include <standard.h>

extern int dps_flag;

static void shgradpulse(double gval, double gtim)
{
char gradshaping[MAXSTR];
getstr("gradshaping",gradshaping);

if (gradshaping[0]=='n')
{

 rgradient('Z',gval);
 delay(gtim);
 rgradient('Z',0.0);

}
else
{ 
   static double mgrad[21]= {0.0,0.07,0.24,0.42,0.68,0.94,1.0,
                        1.0,1.0,1.0,1.0,1.0,1.0,1.0,
                        1.0,0.94,0.68,0.42,0.24,0.07,0.0};
   double gstep,gampl;
   int jcnt;
   if ((gtim/21.0) < (8.65e-6))
     gstep=8.65e-6;
   else
     gstep = ((gtim)/21.0);
   for (jcnt=0;jcnt<=20;jcnt++)
     { gampl=(mgrad[jcnt]*gval);
      rgradient('Z',gampl);
      delay(gstep - GRADIENT_DELAY); }
   rgradient('Z',0.0);
}
}
  

static double tc1x,tc2x,tc3x,tc4x;
static double amp1x,amp2x,amp3x,amp4x;
static double tc1y,tc2y,tc3y,tc4y;
static double amp1y,amp2y,amp3y,amp4y;
static double tc1z,tc2z,tc3z,tc4z;
static double amp1z,amp2z,amp3z,amp4z;

static void read_in_eddys(char chan)
{
   if (chan == 'x')
   {
     tc1x = getval("tc1x");
     tc2x = getval("tc2x");
     tc3x = getval("tc3x");
     tc4x = getval("tc4x");
     amp1x = getval("amp1x");
     amp2x = getval("amp2x");
     amp3x = getval("amp3x");
     amp4x = getval("amp4x");
   }
   if (chan == 'y')
   {
     tc1y = getval("tc1y");
     tc2y = getval("tc2y");
     tc3y = getval("tc3y");
     tc4y = getval("tc4y");
     amp1y = getval("amp1y");
     amp2y = getval("amp2y");
     amp3y = getval("amp3y");
     amp4y = getval("amp4y");
   }
   if (chan == 'z')
   {
     tc1z = getval("tc1");
     tc2z = getval("tc2");
     tc3z = getval("tc3");
     tc4z = getval("tc4");
     amp1z = getval("amp1");
     amp2z = getval("amp2");
     amp3z = getval("amp3");
     amp4z = getval("amp4");
   }
}
 
static void send_eddys(char chan, double deddy)
{
   /* if deddy is set long enough 50 ms + each light with light */
   if (chan == 'x')
   {
     calc_amp_tc('x',1,amp1x,tc1x);
     delay(deddy);
     calc_amp_tc('x',2,amp2x,tc2x);
     delay(deddy);
     calc_amp_tc('x',3,amp3x,tc3x);
     delay(deddy);
     calc_amp_tc('x',4,amp4x,tc4x);
     delay(deddy);
   }
   if (chan == 'y')
   {
     calc_amp_tc('y',1,amp1y,tc1y);
     delay(deddy);
     calc_amp_tc('y',2,amp2y,tc2y);
     delay(deddy);
     calc_amp_tc('y',3,amp3y,tc3y);
     delay(deddy);
     calc_amp_tc('y',4,amp4y,tc4y);
     delay(deddy);
   }
   if (chan == 'z')
   {
     calc_amp_tc('z',1,amp1z,tc1z);
     delay(deddy);
     calc_amp_tc('z',2,amp2z,tc2z);
     delay(deddy);
     calc_amp_tc('z',3,amp3z,tc3z);
     delay(deddy);
     calc_amp_tc('z',4,amp4z,tc4z);
     delay(deddy);
   }
}


void pulsesequence()
{
   double gzlvl1,gt1;
   char gradaxis[MAXSTR];
   getstrnwarn("gradaxis",gradaxis);
   if (( gradaxis[0] != 'x') && ( gradaxis[0] != 'y') && ( gradaxis[0] != 'z') )
      strcpy(gradaxis,"z");

   gzlvl1 = getval("gzlvl1");
   gt1 = getval("gt1");

   
   status(A);
   read_in_eddys(gradaxis[0]);
   delay(d1);

   status(B);
   rgradient(gradaxis[0],0.0);
   send_eddys(gradaxis[0],0.00005);
   
   delay(2e-3);
   shgradpulse(gzlvl1,gt1);
     
   delay(d2);

   status(C);
   pulse(pw,oph);
}

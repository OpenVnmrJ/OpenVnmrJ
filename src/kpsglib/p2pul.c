// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  p2pul - gradient equipment two-pulse sequence */
/*  test sequence do all tests between p1 and pw  */
/*  no science, please */

#include <standard.h>

static double gzlvl1;
static double tc1x,tc2x,tc3x,tc4x; 
static double amp1x,amp2x,amp3x,amp4x; 
static double tc1y,tc2y,tc3y,tc4y; 
static double amp1y,amp2y,amp3y,amp4y; 
static double tc1z,tc2z,tc3z,tc4z; 
static double amp1z,amp2z,amp3z,amp4z; 
static char   gread,gslice,gphase,command[MAXSTR],gradshape[MAXSTR];

void pulsesequence()
{
   int flag;
   /* get variables */
   flag = getorientation(&gread,&gphase,&gslice,"orient");
   if (flag)
     printf("flag\n");
   gzlvl1=getval("gzlvl1");
   getstr("cmd",command);
   getstr("gshape",gradshape);
   read_in_eddys();
   /* equilibrium period */
   status(A);
   delay(d1);
   rgpulse(p1, zero,rof1,rof2);
   do_op();
   status(C);
   obspulse();
}

do_op()
{
   /* respond to command string */
   /* pulse make a pulse for d2 of gzlvl1 on gread */
   /* bipulse make a pulse of gzlvl1 for d2 of -gzlvl1 for d2 on gread */
   /* blank live */
   /* blank zero */
   /* intercept by aa or hardware */
   if (!strcmp(command,"help"))
   {
      fprintf(stdout,"Commands available:\n  pulse, bipulse, shape\n  twinkle smeddy\n");
      fprintf(stdout,"  blank0, blank1, enable, disable, reset\n");
      exit(0);
   }
   if (!strcmp(command,"pulse"))
   {
     rgradient(gread,gzlvl1);
     delay(d2);
     rgradient(gread,0.0);
   }
   if (!strcmp(command,"shape"))
   {
     shapedgradient(gradshape,d2,gzlvl1,gread,1,1);
   }
   if (!strcmp(command,"bipulse"))
   {
     rgradient(gread,gzlvl1);
     delay(d2);
     rgradient(gread,-1.0*gzlvl1);
     delay(d2);
     rgradient(gread,0.0);
   }
   if (!strcmp(command,"blank0"))
   {
     rgradient(gread,gzlvl1);
     delay(d2);
     rgradient(gread,0.0);
     pfg_blank(gread);
     delay(d2);
     pfg_enable(gread);
   }
   if (!strcmp(command,"blank1"))
   {
     rgradient(gread,gzlvl1);
     delay(d2);
     pfg_blank(gread);
     delay(d2);
     rgradient(gread,0.0);
     pfg_enable(gread);
   }
   if (!strcmp(command,"twinkle"))
   {
     send_eddys(gread,1.000);   
   }
   if (!strcmp(command,"smeddy"))
   {
     rgradient(gread,0.0);
     send_eddys(gread,0.00005);   
     rgradient(gread,0.0);
     rgradient(gread,gzlvl1);
     delay(d2);
     rgradient(gread,-1.0*gzlvl1);
     delay(d2);
     rgradient(gread,0.0);
   }
   if (!strcmp(command,"enable"))
   {
     pfg_enable(gread);
   }
   if (!strcmp(command,"disable"))
   {
     pfg_blank(gread);
   }
   if (!strcmp(command,"tc1"))
   {
     calc_amp_tc('z',1,amp1z,tc1z);
     delay(1.0);
   }
   if (!strcmp(command,"tc2"))
   {
     calc_amp_tc('z',2,amp2z,tc2z);
     delay(1.0);
   }
   if (!strcmp(command,"tc3"))
   {
     calc_amp_tc('z',3,amp3z,tc3z);
     delay(1.0);
   }
   if (!strcmp(command,"tc4"))
   {
     calc_amp_tc('z',4,amp4z,tc4z);
     delay(1.0);
   }
   if (!strcmp(command,"reset"))
   {
     pfg_reset(gread);
   }
}

read_in_eddys()
{
   if (gread == 'x') 
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
   if (gread == 'y') 
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
   if (gread == 'z') 
   {
     tc1z = getval("tc1z");
     tc2z = getval("tc2z");
     tc3z = getval("tc3z");
     tc4z = getval("tc4z");
     amp1z = getval("amp1z");
     amp2z = getval("amp2z");
     amp3z = getval("amp3z");
     amp4z = getval("amp4z");
   }
}

send_eddys(chan,deddy)
char chan;
double deddy;
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

/*
   pfg_quick_zero('z');
*/

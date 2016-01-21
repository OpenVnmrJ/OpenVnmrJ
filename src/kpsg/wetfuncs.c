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
/*  wet.c - 
		wet element for wet seqeunces
 */

#include "acqparms2.h"
#include "rfconst.h"

extern double getval();
extern int dpsTimer;
extern int dps_flag;

static void x_wet4(codeint phaseA, codeint phaseB)
{

  double gzlvlw,
                gtw,
                gswet,
                dmfwet,
                dpwrwet,
                dofwet,
                wetpwr1,
                wetpwr2,
                wetpwr3,
                wetpwr4,
                pwwet1,
                pwwet2,
                pwwet3,
                pwwet4,
                dz;
  int c13wet;
  char wetshape1[512],
                wetshape2[512],
                wetshape3[512],
                wetshape4[512],
                wetfly[512];
  c13wet=getflag("c13wet");
  getstr("wetshape1",wetshape1);
  getstr("wetshape2",wetshape2);
  getstr("wetshape3",wetshape3);
  getstr("wetshape4",wetshape4);
  getstr("wetfly",wetfly);
  wetpwr1=getval("wetpwr1");
  wetpwr2=getval("wetpwr2");
  wetpwr3=getval("wetpwr3");
  wetpwr4=getval("wetpwr4");
  pwwet1=getval("pwwet1");
  pwwet2=getval("pwwet2");
  pwwet3=getval("pwwet3");
  pwwet4=getval("pwwet4");
  dmfwet=getval("dmfwet");
  dpwrwet=getval("dpwrwet");
  dofwet=getval("dofwet");
  dz=getval("dz");
  gzlvlw=getval("gzlvlw");
  gtw=getval("gtw");
  gswet=getval("gswet");

  DPSprint(0.0, "53 rcvroff  1 2 0 off 0 1 1 \n");
  DPSprint(0.0, "12 obs_pw_ovr  0 0 0 \n");
  DPSprint(0.0, "60 rlpower  %d  0 1 wetpwr1 %.9f \n", (int)(1), (float)(wetpwr1));
  DPSprint(pwwet1, "77 shaped_pulse  1 1 1 1 20.0e-6 %.9f 10.0e-6 %.9f  1  ?%s phaseA %d pwwet1 %.9f \n", (float)(20.0e-6), (float)(10.0e-6), wetshape1, (int)(phaseA), (float)(pwwet1));
  DPSprint(gtw, "94 zgradpulse  11 0 2 z gtw %.9f gzlvlw %.9f \n", (float)(gtw), (float)(gzlvlw));
  DPSprint(gswet, "10 delay  1 0 1 gswet %.9f \n", (float)(gswet));

  DPSprint(0.0, "60 rlpower  %d  0 1 wetpwr2 %.9f \n", (int)(1), (float)(wetpwr2));
  DPSprint(pwwet2, "77 shaped_pulse  1 1 1 1 20.0e-6 %.9f 10.0e-6 %.9f  1  ?%s phaseB %d pwwet2 %.9f \n", (float)(20.0e-6), (float)(10.0e-6), wetshape2, (int)(phaseB), (float)(pwwet2));
  DPSprint(gtw, "94 zgradpulse  11 0 2 z gtw %.9f gzlvlw/2 %.9f \n", (float)(gtw), (float)(gzlvlw/2));
  DPSprint(gswet, "10 delay  1 0 1 gswet %.9f \n", (float)(gswet));

  DPSprint(0.0, "60 rlpower  %d  0 1 wetpwr3 %.9f \n", (int)(1), (float)(wetpwr3));
  DPSprint(pwwet3, "77 shaped_pulse  1 1 1 1 20.0e-6 %.9f 10.0e-6 %.9f  1  ?%s phaseB %d pwwet3 %.9f \n", (float)(20.0e-6), (float)(10.0e-6), wetshape3, (int)(phaseB), (float)(pwwet3));
  DPSprint(gtw, "94 zgradpulse  11 0 2 z gtw %.9f gzlvlw/4 %.9f \n", (float)(gtw), (float)(gzlvlw/4));
  DPSprint(gswet, "10 delay  1 0 1 gswet %.9f \n", (float)(gswet));

  DPSprint(0.0, "60 rlpower  %d  0 1 wetpwr4 %.9f \n", (int)(1), (float)(wetpwr4));
  DPSprint(pwwet4, "77 shaped_pulse  1 1 1 1 20.0e-6 %.9f 10.0e-6 %.9f  1  ?%s phaseB %d pwwet4 %.9f \n", (float)(20.0e-6), (float)(10.0e-6), wetshape4, (int)(phaseB), (float)(pwwet4));
  DPSprint(gtw, "94 zgradpulse  11 0 2 z gtw %.9f gzlvlw/8 %.9f \n", (float)(gtw), (float)(gzlvlw/8));
  DPSprint(gswet, "10 delay  1 0 1 gswet %.9f \n", (float)(gswet));

  DPSprint(0.0, "12 obs_pw_ovr  0 0 0 \n");
  DPSprint(0.0, "60 rlpower  %d  0 1 tpwr %.9f \n", (int)(1), (float)(tpwr));
  DPSprint(0.0, "53 rcvron  1 2 0 on 1 1 1 \n");
  DPSprint(dz, "10 delay  1 0 1 dz %.9f \n", (float)(dz));
}

static void x_comp90pulse(double width, codeint phase, double rx1, double rx2)
{

  DPSprint(0.0, "70 incr 59 1 0 phase %d \n", (int)(phase)); DPSprint(width, "48 rgpulse  1 0 1 1 rx1 %.9f rx1 %.9f  1 phase %d width %.9f \n", (float)(rx1), (float)(rx1), (int)(phase), (float)(width));
  DPSprint(0.0, "70 incr 59 1 0 phase %d \n", (int)(phase)); DPSprint(width, "48 rgpulse  1 0 1 1 rx1 %.9f rx1 %.9f  1 phase %d width %.9f \n", (float)(rx1), (float)(rx1), (int)(phase), (float)(width));
  DPSprint(0.0, "70 incr 59 1 0 phase %d \n", (int)(phase)); DPSprint(width, "48 rgpulse  1 0 1 1 rx1 %.9f rx1 %.9f  1 phase %d width %.9f \n", (float)(rx1), (float)(rx1), (int)(phase), (float)(width));
  DPSprint(0.0, "70 incr 59 1 0 phase %d \n", (int)(phase)); DPSprint(width, "48 rgpulse  1 0 1 1 rx1 %.9f rx2 %.9f  1 phase %d width %.9f \n", (float)(rx1), (float)(rx2), (int)(phase), (float)(width));
}

static void t_wet4(codeint phaseA, codeint phaseB)
{

  double gzlvlw,
                gtw,
                gswet,
                dmfwet,
                dpwrwet,
                dofwet,
                wetpwr1,
                wetpwr2,
                wetpwr3,
                wetpwr4,
                pwwet1,
                pwwet2,
                pwwet3,
                pwwet4,
                dz;
  int c13wet;
  char wetshape1[512],
                wetshape2[512],
                wetshape3[512],
                wetshape4[512],
                wetfly[512];
  c13wet=getflag("c13wet");
  getstr("wetshape1",wetshape1);
  getstr("wetshape2",wetshape2);
  getstr("wetshape3",wetshape3);
  getstr("wetshape4",wetshape4);
  getstr("wetfly",wetfly);
  wetpwr1=getval("wetpwr1");
  wetpwr2=getval("wetpwr2");
  wetpwr3=getval("wetpwr3");
  wetpwr4=getval("wetpwr4");
  pwwet1=getval("pwwet1");
  pwwet2=getval("pwwet2");
  pwwet3=getval("pwwet3");
  pwwet4=getval("pwwet4");
  dmfwet=getval("dmfwet");
  dpwrwet=getval("dpwrwet");
  dofwet=getval("dofwet");
  dz=getval("dz");
  gzlvlw=getval("gzlvlw");
  gtw=getval("gtw");
  gswet=getval("gswet");

  DPStimer(53,0,0,0);
  DPStimer(12,0,0,0);
  DPStimer(60,0,0,0);
  DPStimer(77,0,0,3,0,0,0,0 ,(double)pwwet1,(double)20.0e-6,(double)10.0e-6);
  DPStimer(94,0,0,1,0,0,0,0 ,(double)gtw);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gswet);

  DPStimer(60,0,0,0);
  DPStimer(77,0,0,3,0,0,0,0 ,(double)pwwet2,(double)20.0e-6,(double)10.0e-6);
  DPStimer(94,0,0,1,0,0,0,0 ,(double)gtw);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gswet);

  DPStimer(60,0,0,0);
  DPStimer(77,0,0,3,0,0,0,0 ,(double)pwwet3,(double)20.0e-6,(double)10.0e-6);
  DPStimer(94,0,0,1,0,0,0,0 ,(double)gtw);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gswet);

  DPStimer(60,0,0,0);
  DPStimer(77,0,0,3,0,0,0,0 ,(double)pwwet4,(double)20.0e-6,(double)10.0e-6);
  DPStimer(94,0,0,1,0,0,0,0 ,(double)gtw);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gswet);

  DPStimer(12,0,0,0);
  DPStimer(60,0,0,0);
  DPStimer(53,0,0,0);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)dz);
}

static void t_comp90pulse(double width, codeint phase, double rx1, double rx2)
{

  DPStimer(70,59,1,0,(int)phase,0,0,0); DPStimer(48,0,0,3,0,0,0,0 ,(double)width,(double)rx1,(double)rx1);
  DPStimer(70,59,1,0,(int)phase,0,0,0); DPStimer(48,0,0,3,0,0,0,0 ,(double)width,(double)rx1,(double)rx1);
  DPStimer(70,59,1,0,(int)phase,0,0,0); DPStimer(48,0,0,3,0,0,0,0 ,(double)width,(double)rx1,(double)rx1);
  DPStimer(70,59,1,0,(int)phase,0,0,0); DPStimer(48,0,0,3,0,0,0,0 ,(double)width,(double)rx1,(double)rx2);
}

/* wet4 - Water Elimination */
void wet4(codeint phaseA, codeint phaseB)
{
  double 	gzlvlw,
  		gtw,
  		gswet,
  		dmfwet,
  		dpwrwet,
  		dofwet,
  		wetpwr1,
  		wetpwr2,
  		wetpwr3,
  		wetpwr4,
  		pwwet1,
  		pwwet2,
  		pwwet3,
  		pwwet4,
  		dz;
  int 		c13wet;
  char   	wetshape1[MAXSTR],
  		wetshape2[MAXSTR],
  		wetshape3[MAXSTR],
  		wetshape4[MAXSTR],
  		wetfly[MAXSTR];

  if (dps_flag)
  {
     if (dpsTimer == 0)
        x_wet4(phaseA, phaseB);
     else
        t_wet4(phaseA, phaseB);
     return;
  }
  c13wet=getflag("c13wet");             /* Water suppression flag        */  
  getstr("wetshape1",wetshape1);    /* Selective pulse shape (base)  */
  getstr("wetshape2",wetshape2);
  getstr("wetshape3",wetshape3);
  getstr("wetshape4",wetshape4);
  getstr("wetfly",wetfly);
  wetpwr1=getval("wetpwr1");        
  wetpwr2=getval("wetpwr2");
  wetpwr3=getval("wetpwr3");
  wetpwr4=getval("wetpwr4");
  pwwet1=getval("pwwet1");        
  pwwet2=getval("pwwet2");
  pwwet3=getval("pwwet3");
  pwwet4=getval("pwwet4");
  dmfwet=getval("dmfwet");
  dpwrwet=getval("dpwrwet");
  dofwet=getval("dofwet");
  dz=getval("dz");
  gzlvlw=getval("gzlvlw");      /* Z-Gradient level              */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */

  rcvroff();
  obs_pw_ovr(TRUE);
  rlpower(wetpwr1,1);		/* obspower(wetpwr1); replaced */
  shaped_pulse(wetshape1,pwwet1,phaseA,20.0e-6,10.0e-6);
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
  
  rlpower(wetpwr2,1);		/* obspower(wetpwr2); replaced */
  shaped_pulse(wetshape2,pwwet2,phaseB,20.0e-6,10.0e-6);
  zgradpulse(gzlvlw/2,gtw);
  delay(gswet);

  rlpower(wetpwr3,1);		/* obspower(wetpwr3); replaced */
  shaped_pulse(wetshape3,pwwet3,phaseB,20.0e-6,10.0e-6);
  zgradpulse(gzlvlw/4,gtw);
  delay(gswet);

  rlpower(wetpwr4,1);		/* obspower(wetpwr4); replaced */
  shaped_pulse(wetshape4,pwwet4,phaseB,20.0e-6,10.0e-6);
  zgradpulse(gzlvlw/8,gtw);
  delay(gswet);

  obs_pw_ovr(FALSE);
  rlpower(tpwr,1);		/* obspower(tpwr); Reset to normal power level*/
  rcvron();
  delay(dz);
}


int getflag(char *str)
{
   char strval[MAXSTR];
 
   getstr(str,strval);
   if ((strval[0]=='y') || (strval[0]=='Y'))
      return(TRUE);
   else
      return(FALSE);
}

void comp90pulse(double width, codeint phase, double rx1, double rx2)
{
  if (dps_flag)
  {
     if (dpsTimer == 0)
        x_comp90pulse(width, phase, rx1, rx2);
     else
        t_comp90pulse(width, phase, rx1, rx2);
     return;
  }
  incr(phase); rgpulse(width,phase,rx1,rx1);  /*  Y  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -X  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -Y  */
  incr(phase); rgpulse(width,phase,rx1,rx2);  /*  X  */
}


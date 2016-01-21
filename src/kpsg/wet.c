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

/* wet4 - Water Elimination */
wet4(phaseA,phaseB)
  codeint phaseA,phaseB;
 
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
  obspower(wetpwr1);
  shaped_pulse(wetshape1,pwwet1,phaseA,20.0e-6,10.0e-6);
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
  
  obspower(wetpwr2);
  shaped_pulse(wetshape2,pwwet2,phaseB,20.0e-6,10.0e-6);
  zgradpulse(gzlvlw/2,gtw);
  delay(gswet);

  obspower(wetpwr3);
  shaped_pulse(wetshape3,pwwet3,phaseB,20.0e-6,10.0e-6);
  zgradpulse(gzlvlw/4,gtw);
  delay(gswet);

  obspower(wetpwr4);
  shaped_pulse(wetshape4,pwwet4,phaseB,20.0e-6,10.0e-6);
  zgradpulse(gzlvlw/8,gtw);
  delay(gswet);

  obs_pw_ovr(FALSE);
  obspower(tpwr);     /* Reset to normal power level   */
  rcvron();
  delay(dz);
}


int getflag(str)
char str[MAXSTR];
{
   char strval[MAXSTR];
 
   getstr(str,strval);
   if ((strval[0]=='y') || (strval[0]=='Y')) return(TRUE);
     else                                    return(FALSE);
}

comp90pulse(width,phase,rx1,rx2)
  double width,rx1,rx2;
  codeint phase;
{
  incr(phase); rgpulse(width,phase,rx1,rx1);  /*  Y  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -X  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -Y  */
  incr(phase); rgpulse(width,phase,rx1,rx2);  /*  X  */
}


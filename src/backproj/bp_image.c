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
#include <standard.h>
#include <math.h>

static   double gro,grof,gss,gssf,gr1,gr2,gr3;
static	 double asweep,atilt,sweep_inc,tilt_inc,sw1,sw2;
static	 double phi,theta,act_phi,act_theta;
static   double tph;
static   double te,trise,ti,tr,tix,tpe;
static   double ni,ni2,at,dx,dy,nsat,sd,sat;
static   double temptof,tofc,tpwr1,tpwri,tpwrj,pi,pj;
static   char gro1,grp1,grs1;
static   char bptype[MAXSTR],prep[MAXSTR];
static   char p1pat[MAXSTR],pwpat[MAXSTR];




pulsesequence()  
{
   get_parameters();
   check_parameters();
   set_powers();
   calc_gradients();
   if(!strcmp(bptype,"slice")) {
      slice_timing();
   } else if(!strcmp(bptype,"noslice")) {
      noslice_timing();
   }
   status(A);
   if (!strcmp(prep,"sr")) {
       saturation_pulse();
   } else if (!strcmp(prep,"ir")) {
       inversion_pulse();
   } else if (!strcmp(prep,"aps")) {
       aps_pulse();
   } else if (!strcmp(prep,"t1r")) {
       spinlock_pulse();
   } else {
       delay(tr-te-p1/2.0);
   }
   if(!strcmp(bptype,"slice")) {
       slice_offset();
       status(B);
       selective_90();
       slice_compensate();
       delay(dx);
       selective_180();
       delay(dy);
       read_out();
   } else if(!strcmp(bptype,"noslice")) {
       status(B);
       bp_noslice();
   }
}

get_parameters()
{ 
   if (getorientation(&gro1,&grp1,&grs1,"orient") < 0) 
       exit(1);
   tofc = getval("tofc");
   gro = getval("gro");
   grof = getval("grof");
   gss = getval("gss");
   gssf = getval("gssf");
   at   = getval("at");
   ni =  (int) getval("ni");  
   ni2 =  (int) getval("ni2");  
   phi = getval("phi");
   theta = getval("theta");
   sw1 = getval("sw1");
   sw2 = getval("sw2");
   trise = getval("trise");
   te = getval("te");
   tpe = getval("tpe");
   tph = getval("tph");
   ti = getval("ti");
   tr = getval("tr");
   tpwr1 = getval("tpwr1");
   tpwri = getval("tpwri"); 
   tpwrj = getval("tpwrj"); 
   pi = getval("pi");
   pj = getval("pj");
   nsat = getval("nsat");
   sat = getval("sat");
   getstr("p1pat",p1pat);
   getstr("pwpat",pwpat);
   getstr("bptype",bptype);
   getstr("prep",prep);
}

check_parameters()
{
/*
 *  do some error checking on string variables 
 */
   if (p1pat[0] == '\0') { 
     text_error("p1pat file spec error\n"); 
     exit(2); 
   }
   if (pwpat[0] == '\0') { 
     text_error("pwpat file spec error\n"); 
     exit(2); 
   }
   if ((ni2 > 1) && (!strcmp(bptype,"slice"))){
     text_error("can't do slice selection in 3D experiment\n");
     exit(2);
   }
}

slice_timing()
{
/* calculate dx: */
   if (tpe >= p1) {
       dx = (te/2.0) - tpe - trise - 0.5*(p1+pw) - rof1 - rof2;
   } else {
       dx = (te/2.0) - p1 - trise - 0.5*(p1+pw) - rof1 - rof2;
   }
   if (dx < 1.0e-6) { 
       text_error("cannot fit tpe trise p1 pw into te... ABORT");
       exit(2);
   }

/* calculate dy: */
   dy = (te/2.0) - at/2.0 - 2*trise - pw/2.0 - rof2;
   if (dy < 1.0e-6) { 
       dy = 1.0e-6;
   }
   tix = tr - pi/2.0 - rof2 - trise;
}

noslice_timing()
{
/* calculate dx: */
   dx = (te/2.0) - 0.5*(p1+pw) - rof1 - rof2;
   if (dx < 1.0e-6) { 
       text_error("cannot fit tpe trise p1 pw into te... ABORT");
       exit(2);
   }
/* calculate dy: */
   dy = (te/2.0) - at/2.0 - pw/2.0 - rof2;
   if (dy < 1.0e-6) { 
       dy = 1.0e-6;
   }
   tix = tr - pi/2.0 - rof2 - trise;
}

set_powers()
{
			/* set the powers */
   initval(tpwr,v1);
   initval(tpwr1,v2);
   initval(tpwri,v3);
   initval(tpwrj,v4);

			/* set the phase cycle */
   assign(oph,v5);
   add(one,v5,v6);
   add(two,v5,v7);
}

/* Calculate gradients
*/
calc_gradients()
{
  sweep_inc= d2*sw1;
  tilt_inc = d3*sw2;
  if (ni > 1)                           /* it is 2D at least */
  {
    asweep = M_PI * phi/(180.0 * (double) (ni));
    if (ni2 > 1)                        /* it is full 3D */
    {
      atilt  = M_PI * theta/(180.0 * (double) (ni2));
    }
    else                                /* it is only 2D */
      atilt  = 0.0;
  }
  else                                  /* it is a projection */
  {
    atilt  = 0.0;
    asweep = 0.0;
  }
  act_phi =  asweep * sweep_inc;
  act_theta = atilt  * tilt_inc;
 
/* satelite type of acquisition*/
  gr1 = gro * cos(act_phi);
  gr2 = gro * sin(act_phi) * cos(act_theta);
  gr3 = gro * sin(act_phi) * sin(act_theta);
 
}

slice_offset()
{
   temptof = tofc+tof;
   offset(temptof,TODEV);	/*Tx offset for slice */
}

inversion_pulse()
{
  power(v3,TODEV);
  delay(d1);
  rgpulse(pi,oph,rof1,0.0);
  delay(tix);                          /* inversion period ends */
}

saturation_pulse()
{
  power(v3,TODEV);
  delay(d1);
  rgpulse(pi,oph,rof1,0.0);
  delay(tix);                          /* saturation period ends */
}

aps_pulse()
{
  int i,npsat;
  npsat = (int) nsat;
  power(v3,TODEV);
  delay(d1);
  rgpulse(pi,oph,rof1,0.0);
  for(i=1;i<npsat;i++){
      sd = sat - rof1;
      delay(sd);
      sat /= 2.0;
      rgpulse(pi,oph,rof1,0.0);
  }
  delay(tix);                          /* saturation period ends */
}
selective_90()
{
   power(v2,TODEV);
   rgradient(grs1,gss);
   delay(trise);
   shaped_pulse(p1pat,p1,oph,rof1,rof2);
   rgradient(grs1,-gss*gssf);
   rgradient(gro1,grof*gr1);
   rgradient(grp1,grof*gr2);
}

slice_compensate()
{
  if (tpe >= p1)
  { 				/* calculate delays for all gradients  */
    delay(p1);
    rgradient(grs1,0.0);
    if ((tpe-p1) == 0.0)
    {
      rgradient(gro1,0.0); 
      rgradient(grp1,0.0);
    }
    else
    {
      delay(tpe-p1);
      rgradient(gro1,0.0);
      rgradient(grp1,0.0);
    }
  }
  else
  {
    delay(tpe);
    rgradient(gro1,0.0);
    rgradient(grp1,0.0);
    delay(p1-tpe);
    rgradient(grs1,0.0);
  }
}

selective_180()
{
   power(v1,TODEV);
   rgradient(grs1,gss); 
   delay(trise);
   status(C);
   shaped_pulse(pwpat,pw,v6,rof1,rof2);
   offset(tof,TODEV);		/*  go back to observe position */
   delay(trise);
   rgradient(grs1,0.0);
}

read_out()
{
  rgradient(gro1,gr1);
  rgradient(grp1,gr2);
  delay(trise);
  acquire(np,1.0/sw);		/* Acquire  */
  rgradient(gro1,0.0);
  rgradient(grp1,0.0);
}

bp_noslice()
{
   power(v2,TODEV);
   rgradient(gro1,gr1);
   rgradient(grp1,gr2);
   rgradient(grs1,gr3);
   delay(trise);
   rgpulse(p1,oph,rof1,rof2);
   power(v1,TODEV);
   delay(dx);
   rgpulse(pw,v6,rof1,rof2);
   delay(dy);
   delay(alfa+1.0/(beta*fb));
   acquire(np,1.0/sw);		/* Acquire  */
   rgradient(gro1,0.0);
   rgradient(grp1,0.0);
   rgradient(grs1,0.0);
}

 spinlock_pulse()
{
  power(v3,TODEV);
  delay(d1);
  rcvroff();
  txphase(v5);
  delay(rof1);
  xmtron();
  delay(pi);
  xmtroff();
  power(v4,TODEV);
  txphase(v6);
  delay(tph);
  xmtron();
  delay(pj);
  xmtroff();
  power(v3,TODEV);
  txphase(v7);
  delay(tph);
  xmtron();
  delay(pi);
  xmtroff();
  delay(rof2);
  rcvron();
  delay(tix);
}


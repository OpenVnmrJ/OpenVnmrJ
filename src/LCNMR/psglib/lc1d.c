// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* lc1d (subset of lc1d) */
/* modified for t1b1 case, dz, and waltz   */

#include <standard.h>

extern int lcsample();
extern void pbox_par(char *pxname, char *pxval);

double pwwet=0.0, wetpwr=0.0, wgfpwr=4095;
char   wetarr[MAXSTR];
shape  dumshape;


/************************************************/
/* CHESS - CHEmical Shift Selective Suppression */
/************************************************/
static void CHESS(double pulsepower, char *pulseshape, double duration, codeint phase,
           double rx1, double rx2, double gzlvlw, double gtw, double gswet, int c13wet)  
{
  obspwrf(pulsepower);
  if (c13wet) decon();
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  if (c13wet) decoff();
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
}


/****************************/
/* wet4 - Water Elimination */
/****************************/

static void WET(codeint phaseA, codeint phaseB)
{
  double finepwr,gzlvlw,gtw,gswet,dmfwet,dpwrwet,dofwet,wetpwr,pwwet,dz,
  ref_pw90 = getval("ref_pw90"),
         ref_pwr = getval("ref_pwr"),
        slp0bw = getval("slp0bw"),
        slpbw = getval("slpbw"),
        slp2bw = getval("slp2bw"),
        slp3bw = getval("slp3bw"),
        slp4bw = getval("slp4bw"),
        slp5bw = getval("slp5bw"),
        slp6bw = getval("slp6bw"),
        slp0,slp,slp2,
        slp3,slp4,slp5,slp6;
  int   slp0w,slpw,slp2w,slp3w,slp4w,slp5w,slp6w,c13wet;
  char  wetshape[MAXSTR];

  c13wet=getflag("c13wet");        /* C13 satellite suppression flag    */
  getstr("wetshape",wetshape);     /* Selective pulse shape (basename)  */
  wetpwr=getval("wetpwr");         /* User enters power for 90 deg.     */
  pwwet=getval("pwwet");           /* User enters power for 90 deg.     */
  dmfwet=getval("dmfwet");         /* dmf for the C13 decoupling        */
  dpwrwet=getval("dpwrwet");       /* Power fot the C13 decoupling      */
  dofwet=getval("dofwet");         /* Offset for the C13 decoupling     */
  dz=getval("dz");                 /* Post WET delay                    */
  slp0w=getflag("slp0w");          /* Flags whether user is requesting  */
  slpw=getflag("slpw");            /* WET suppression on each solvent   */
  slp2w=getflag("slp2w");          /* signal                            */
  slp3w=getflag("slp3w");
  slp4w=getflag("slp4w");
  slp5w=getflag("slp5w");
  slp6w=getflag("slp6w");

/*      On-the fly calculation of the WET shapes.
        d.a. March 2001
        First check if any of the WET related parameters are arrayed,
        in order to avoid extra pulse shaping  */

   if ((getval("arraydim") < 1.5) || (ix==1) || (isarry("ref_pwr")) || (isarry("ref_pw90")) || (isarry("tof")) || (isarry("slp0bw")) || (isarry("slp")) || (isarry("slpbw")) || (isarry("slp2bw")) || (isarry("slp3bw")) || (isarry("slp4bw")) || (isarry("slp5bw")) || (isarry("slp6bw")) || (isarry("slp0")) || (isarry("slp2")) || (isarry("slp3")) || (isarry("slp4")) || (isarry("slp5")) || (isarry("slp6")) || (isarry("slp0w")) || (isarry("slpw")) || (isarry("slp2w")) || (isarry("slp3w")) || (isarry("slp4w")) || (isarry("slp5w")) || (isarry("slp6w")))
{

/*      Set the name of the shape file to wetshape if not arrayed,
        for compatibility reasons with the other sequences of the LC
        and VAST package.
        If something is arrayed then the first elements named with
        wetshape and all subsequent with wetshape_n, where n is
        the array index    */

  if (ix==1)
  sprintf(wetarr, "%s", wetshape);
  else
  sprintf(wetarr, "%s_%d", wetshape, ix);

/* Open Pbox and start pulse shape calculation   */

  opx(wetarr);

/*      Explicitly check whether each one of the seven solvent lines
        is chosen to be suppressed. If the slpN parameter is set to
        'n' or the slpNw flag is set to 'n' then no wave is put into Pbox. 
        Otherwise the proper line with the SEDUCE shape is addded.
        var_active information can be found in /vnmr/psg/active.c    */

  if ((var_active("slp0",1)) && (slp0w))
        { slp0 = getval("slp0");
          putwave("seduce",slp0bw/2,slp0,0.0,0.0,90.0); }
  if ((var_active("slp",1)) && (slpw))
        { slp = getval("slp");
          putwave("seduce",slpbw/2,slp,0.0,0.0,90.0); }
  if ((var_active("slp2",1)) && (slp2w))
        { slp2 = getval("slp2");
          putwave("seduce",slp2bw/2,slp2,0.0,0.0,90.0); }
  if ((var_active("slp3",1)) && (slp3w))
        { slp3 = getval("slp3");
          putwave("seduce",slp3bw/2,slp3,0.0,0.0,90.0); }
  if ((var_active("slp4",1)) && (slp4w))
        { slp4 = getval("slp4");
          putwave("seduce",slp4bw/2,slp4,0.0,0.0,90.0); }
  if ((var_active("slp5",1)) && (slp5w))
        { slp5 = getval("slp5");
          putwave("seduce",slp5bw/2,slp5,0.0,0.0,90.0); }
  if ((var_active("slp6",1)) && (slp6w))
        { slp6 = getval("slp6");
          putwave("seduce",slp6bw/2,slp6,0.0,0.0,90.0); }

/*      Add additional control parameters, close Pbox and retrieve
        the shape parameters into the proper variables   */

  pbox_par("attn","i");
  pbox_par("reps","2");
  cpx(ref_pw90,ref_pwr);
  pbox_get();
  wetpwr = pbox_pwr;
  pwwet = pbox_pw;
}
else
{   /* Read the pbx.RF shape file and retrieve the wetpwr and pwwet values */
    dumshape=getRsh(wetshape);
    wetpwr = dumshape.pwr;
    pwwet = dumshape.pw;
}


finepwr=wetpwr-(int)wetpwr;  /* Adjust power to 152 deg. pulse */
  wetpwr=(double)((int)wetpwr);
  if (finepwr==0.0) {wetpwr=wetpwr+5; finepwr=4095.0; }
  else {wetpwr=wetpwr+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  if (c13wet)
    {
    decunblank(); decon();
    decoffset(dofwet);
    decpower(dpwrwet);
    if (rfwg[DECch-1]=='y')
         decprgon("garp1",1/dmfwet,1.0);
      else
         setstatus(DECch,FALSE,'g',FALSE,dmfwet);
    }
  obspower(wetpwr);         /* Set to low power level        */
  gzlvlw=getval("gzlvlw");      /* Z-Gradient level              */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
  CHESS(finepwr*0.5056,wetarr,pwwet,phaseA,20.0e-6,rof2,gzlvlw,gtw,gswet,c13wet);
  CHESS(finepwr*0.6298,wetarr,pwwet,phaseB,20.0e-6,rof2,gzlvlw/2.0,gtw,gswet,c13wet);
  CHESS(finepwr*0.4304,wetarr,pwwet,phaseB,20.0e-6,rof2,gzlvlw/4.0,gtw,gswet,c13wet);
  CHESS(finepwr*1.00,wetarr,pwwet,phaseB,20.0e-6,rof2,gzlvlw/8.0,gtw,gswet,c13wet);
  if (c13wet)
    {
    if (rfwg[DECch-1]=='y')
         decprgoff();
      else
         setstatus(DECch,FALSE,'c',FALSE,dmf);
    decoffset(dof);
    decpower(dpwr);
    decoff(); decblank();
    }
  obspower(tpwr); obspwrf(tpwrf);    /* Reset to normal power level   */
  rcvron();
  delay(dz);
}

/*************************/
/** SS Composite pulse  **/
/*************************/
static void sscomposite(codeint phase)
{
    char cmd[MAXSTR*2];
   shape compshape;

    sprintf(compshape.name,"cmp_1");

if((getval("arraydim") < 1.5) || (ix==1) || (isarry("pw")) || (isarry("tpwr")))
  {
   sprintf(compshape.name, "%s_%d","cmp",ix);
   sprintf(cmd, "Pbox %s -u %s -w \"square n %.2f n 65 360 n 0.4u 0.4u\" -s 0.2 -p %.0f -l %.2f -attn %.0fd -2",compshape.name,userdir,1.025/(4*pw),tpwr,pw*1.0e6,tpwr);
   system(cmd);
  }

   compshape = getRsh(compshape.name);
   pbox_pulse(&compshape,phase,rof1,rof2);
}

/**************************/
/** Hard Composite Pulse **/
/**************************/
static void composite_pulse(double width, codeint phasetable, double rx1, double rx2, codeint phase)
{
  getelem(phasetable,ct,phase); /* Extract observe phase from table */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /*  Y  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -X  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -Y  */
  incr(phase); rgpulse(width,phase,rx1,rx2);  /*  X  */
} 

void pulsesequence()
{
  /* DECLARE & READ IN NEW PARAMETERS */
  
  char   compshape[MAXSTR];
  getstr("compshape",compshape);    /* Composit pulse shape  */

  loadtable("lc1d");              /* Phase table                   */

  /* PULSE SEQUENCE */

  status(A);

    lcsample();         /* Inject sample, if necessary */
    hsdelay(d1);

  status(B);

    if (getflag("wet")) WET(t1,t2);
  status(C); 

    if (getflag("composit")) 
    {
       if (rfwg[OBSch-1] == 'y')
          /* shaped_pulse(compshape,4.0*pw+0.8e-6,t3,rof1,rof2); */
          sscomposite(t3);
       else
          composite_pulse(pw,t3,rof1,rof2,v1);
    }
    else
       rgpulse(pw,t3,rof1,rof2);
    setreceiver(t4);
}



/*
PAK 950523 - modified DODEV and TODEV-style line (rechecked)
SHS 950712 - modified for t1b1 case,dz, and waltz decoupling
d.a. Darmstadt March 2001-October 2001: Added on the fly calculation 
					of pulse shapes.
*/



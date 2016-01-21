/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef CHEMPACK_H
#define CHEMPACK_H

#ifndef DPS
#include "Pbox_psg.h"
#endif

/*
  Four functions:
  shaped_satpulse(shapename,satduration,frequencyparameter)
  	-  Creates a RF shape with square90 wave and executes shaped_pulse
  shaped_saturate(shapename,satduration,frequencyparameter)
	-  Creates a DEC shape with square90 wave and executes spinlock
 
*/ 

extern double getval(const char *name);
extern int dps_flag;
extern int getArrayparval(const char *parname, double *parval[]);

void shaped_satpulse(const char *shn, double saturation1, codeint sphse1)
{
  FILE *inpf;
  double *ofq, reps=0.0, uspw90;
  char str[MAXSTR], cmd[MAXSTR], shname[MAXSTR];
  extern char userdir[];
  int nlf, i;

  sprintf(shname, "%s_%s",seqfil,shn);
  if (FIRST_FID)
  {
  	uspw90 = (getval("pw90"))*1e6*(getval("tpwr_cf"));
  	sprintf(str, "%s/shapelib/Pbox.inp" , userdir);
  	inpf = fopen(str, "w");
  	nlf = getArrayparval("pstof",&ofq);
  	for (i=0; i<nlf; i++)
        	fprintf(inpf, "{ square90 %.4fs %.2f } \n", saturation1, ofq[i]-tof);
  	fprintf(inpf, " attn = e \n");
  	fclose(inpf);

  	sprintf(cmd, "Pbox %s.RF -u %s -%.0f -l %.2f -p %.0f \n", shname,userdir,reps,uspw90,tpwr);
  	system(cmd);
  }
  obspower(satpwr);
  shaped_pulse(shname,saturation1,sphse1,rof1,rof1);
  obspower(tpwr);
}

void shaped_saturate(const char *shn2, double saturation2, codeint sphse2)
{
  FILE *inpf;
  shape sh;
  double *ofq, reps=0.0, uspw90;
  char str[MAXSTR], cmd[MAXSTR], shname[MAXSTR];
  extern char userdir[];
  int nlf, i;

  sprintf(shname, "%s_%s",seqfil,shn2);
  if (FIRST_FID)
  {
  	uspw90 = (getval("pw90"))*1e6*(getval("tpwr_cf"));
  	sprintf(str, "%s/shapelib/Pbox.inp" , userdir);
  	inpf = fopen(str, "w");
  	nlf = getArrayparval("pstof",&ofq);
  	for (i=0; i<nlf; i++)
        	fprintf(inpf, "{ square90 %.4fs %.2f } \n", saturation2, ofq[i]-tof);
  	fprintf(inpf, " attn = e \n");
  	fclose(inpf);

  	sprintf(cmd, "Pbox %s.DEC -u %s -%.0f -l %.2f -p %.0f \n", shname,userdir,reps,uspw90,tpwr);
  	system(cmd);
  }

  sh = getDsh(shname);
  obspower(satpwr);
  spinlock(sh.name,1/sh.dmf,sh.dres,sphse2,1);
  obspower(tpwr);
}

void satpulse(double saturation, codeint phase, double rx1, double rx2)
{
  double satpwr,
	 satfrq;
  satpwr = getval("satpwr");
  satfrq = getval("satfrq");

       obspower(satpwr);
        if (satfrq != tof)
         obsoffset(satfrq);
        rgpulse(saturation,phase,rx1,rx2);
        if (satfrq != tof)
         obsoffset(tof);
       obspower(tpwr);
}

double syncGrad(char *gT, char *gL, double Mf,char *sR, char *Rv)
{
   double tt, ll, sr, garea, reps;
   sr = getval(sR);
   tt = getval(gT)*Mf;
   ll = getval(gL);
   garea = tt*ll;
   if (sr > 199)
   {
     reps = floor((tt*sr)+0.5);
     if ((reps == 0.0) || (reps < 0.0))
        reps = 1.0;
     tt = reps/sr;
     if (ll > 0.0)
       ll = floor((garea/tt)+0.5);
     else
       ll = -floor((-garea/tt)+0.5);
   }
   tt = tt/Mf;
   if (!strcmp(Rv,"duration"))
        return(tt);
   if (!strcmp(Rv,"level"))
        return(ll);

   abort_message("Illegal argument for syncGrad function");
   return(0.0);

}

double syncGradTime(char *gTime, char *gLevel, double Mfactor)
{
   double Tt = getval(gTime);
   char probetype[MAXSTR];
   P_getstring(GLOBAL,"probetype",probetype,1,255);
   if (!strcmp(probetype,"nano"))
        Tt = syncGrad(gTime,gLevel,Mfactor,"srate","duration");
   return(Tt);
}

double syncGradLvl(char *gTime, char *gLevel, double Mfactor)
{
   double Ll = getval(gLevel);
   char probetype[MAXSTR];
   P_getstring(GLOBAL,"probetype",probetype,1,255);
   if (!strcmp(probetype,"nano"))
        Ll = syncGrad(gTime,gLevel,Mfactor,"srate","level");
   return(Ll);
}

void ExcitationSculpting(codeint esph1, codeint esph2, codeint esph3)
{
  double espw180 = getval("espw180"),
         essoftpw = getval("essoftpw"),
         essoftpwr = getval("essoftpwr"),
         essoftpwrf = getval("essoftpwrf"),
         esgzlvl = getval("esgzlvl"),
         esgt = getval("esgt"),
         esgstab = getval("esgstab");
  char   esshape[MAXSTR],sgyflg[MAXSTR];

        (void) esgzlvl;
        (void) essoftpwr;

//synchronize gradients to srate for probetype='nano'
//  Preserve gradient "area"
        esgt = syncGradTime("esgt","esgzlvl",1.0);
        esgzlvl = syncGradLvl("esgt","esgzlvl",1.0);
        getstr("sgyflg",sgyflg);
        getstr("esshape",esshape);
        ifzero(esph3); zgradpulse(esgzlvl,esgt);
              elsenz(esph3); zgradpulse(-1.0*esgzlvl,esgt); endif(esph3);
        delay(esgstab);
        obspower(essoftpwr+6); obspwrf(essoftpwrf);
        shaped_pulse(esshape,essoftpw,esph1,2.0e-6,2.0e-6);
        add(esph1,two,esph1); mod4(esph1,esph1);
        obspower(tpwr); obspwrf(4095.0);
        if (sgyflg[0] == 'y')
         {
          rgpulse(espw180*81/180,esph1,2.0e-6,2.0e-6);
          add(esph1,two,esph1); mod4(esph1,esph1);
          rgpulse(espw180*81/180,esph1,2.0e-6,2.0e-6);
          add(esph1,two,esph1); mod4(esph1,esph1);
          rgpulse(espw180*342/180,esph1,2.0e-6,2.0e-6);
          add(esph1,two,esph1); mod4(esph1,esph1);
          rgpulse(espw180*162/180,esph1,2.0e-6,2.0e-6);
         }
         else
         {
          rgpulse(espw180,esph1,2.0e-6,2.0e-6);
         }
        ifzero(esph3); zgradpulse(esgzlvl,esgt);
              elsenz(esph3); zgradpulse(-1.0*esgzlvl,esgt); endif(esph3);
        delay(esgstab);

        ifzero(esph3); zgradpulse(0.8*esgzlvl,esgt);
              elsenz(esph3); zgradpulse(-0.8*esgzlvl,esgt); endif(esph3);
        delay(esgstab);
        obspower(essoftpwr+6); obspwrf(essoftpwrf);
        shaped_pulse(esshape,essoftpw,esph2,2.0e-6,2.0e-6);
        add(esph2,two,esph2); mod4(esph2,esph2);
        obspower(tpwr); obspwrf(4095.0);
        if (sgyflg[0] == 'y')
         {
          rgpulse(espw180*81/180,esph2,2.0e-6,2.0e-6);
          add(esph2,two,esph2); mod4(esph2,esph2);
          rgpulse(espw180*81/180,esph2,2.0e-6,2.0e-6);
          add(esph2,two,esph2); mod4(esph2,esph2);
          rgpulse(espw180*342/180,esph2,2.0e-6,2.0e-6);
          add(esph2,two,esph2); mod4(esph2,esph2);
          rgpulse(espw180*162/180,esph2,2.0e-6,0.0);
         }
         else
         {
          rgpulse(espw180,esph2,2.0e-6,0.0);
         }
        ifzero(esph3); zgradpulse(0.8*esgzlvl,esgt);
              elsenz(esph3); zgradpulse(-0.8*esgzlvl,esgt); endif(esph3);
        delay(esgstab);
}

void FlipBack(codeint fbph1, codeint smph1)
{
  double flippwr = getval("flippwr"),
         flippwrf = getval("flippwrf"),
         flippw = getval("flippw");
  char   flipshape[MAXSTR];

        getstr("flipshape",flipshape);

        obsstepsize(1.0);
        xmtrphase(smph1);
        add(fbph1,two,fbph1); mod4(fbph1,fbph1);
        obspower(flippwr+6); obspwrf(flippwrf);
        shaped_pulse(flipshape,flippw,fbph1,rof1,rof1);
        add(fbph1,two,fbph1); mod4(fbph1,fbph1);
        xmtrphase(zero);
        obspower(tpwr); obspwrf(4095.0);
}

void soggy(codeint sgyph)
{
  double sgypw180 = getval("sgypw180"),
	 sgysoftpw = getval("sgysoftpw"),
	 sgysoftpwr = getval("sgysoftpwr"),
	 sgygzlvl = getval("sgygzlvl"),
	 sgygt = getval("sgygt"),
	 sgygstab = getval("sgygstab");

        (void) sgygzlvl;
        (void) sgysoftpwr;
	zgradpulse(sgygzlvl,sgygt);
	delay(sgygstab);
	obspower(sgysoftpwr);
	rgpulse(sgysoftpw,sgyph,2.0e-6,2.0e-6);
	obspower(tpwr);
	rgpulse(sgypw180*81/180,sgyph,2.0e-6,2.0e-6);
	add(sgyph,two,sgyph);
        rgpulse(sgypw180*81/180,sgyph,2.0e-6,2.0e-6);
	sub(sgyph,two,sgyph);
        rgpulse(sgypw180*342/180,sgyph,2.0e-6,2.0e-6);
	add(sgyph,two,sgyph);
        rgpulse(sgypw180*162/180,sgyph,2.0e-6,2.0e-6);
	sub(sgyph,two,sgyph);
        zgradpulse(sgygzlvl,sgygt);
        delay(sgygstab);

        zgradpulse(0.3*sgygzlvl,sgygt);
        delay(sgygstab);
        obspower(sgysoftpwr);
        rgpulse(sgysoftpw,sgyph,2.0e-6,2.0e-6);

        add(sgyph,two,sgyph);
        obspower(tpwr);
        rgpulse(sgypw180*81/180,sgyph,2.0e-6,2.0e-6);
        add(sgyph,two,sgyph);
        rgpulse(sgypw180*81/180,sgyph,2.0e-6,2.0e-6);
        sub(sgyph,two,sgyph);
        rgpulse(sgypw180*342/180,sgyph,2.0e-6,2.0e-6);
        add(sgyph,two,sgyph);
        rgpulse(sgypw180*162/180,sgyph,2.0e-6,2.0e-6);
        sub(sgyph,two,sgyph);
        zgradpulse(0.3*sgygzlvl,sgygt);
        delay(sgygstab);

	sub(sgyph,two,sgyph);
}

void purge(codeint prgph1, codeint prgpph, codeint prgph2, codeint prgph3)
{
  double prgpw = getval("prgpw"),
	 prgd1 = getval("prgd1"),
	 prggt = getval("prggt"),
	 prggstab = getval("prggstab"),
	 prggzlvl = getval("prggzlvl"),
	 prgd2 = getval("prgd2");

        (void) prggzlvl;
	add(prgph2,prgph1,prgph2);
	add(prgph3,prgph1,prgph3);

	obspower(tpwr);
	rgpulse(prgpw,prgph1,2.0e-6,2.0e-6);
	satpulse(prgd1,prgpph,2.0e-6,2.0e-6);
        rgpulse(2*prgpw,prgph1,2.0e-6,2.0e-6);
	satpulse(prgd1,prgpph,2.0e-6,2.0e-6);
        rgpulse(prgpw,prgph3,2.0e-6,2.0e-6);

	zgradpulse(-0.132*prggzlvl,prggt);
	delay(prggstab);
	satpulse(prgd2,prgpph,2.0e-6,2.0e-6);
        zgradpulse(0.527*prggzlvl,prggt);
        delay(prggstab);

        rgpulse(prgpw,prgph1,2.0e-6,2.0e-6);
	satpulse(prgd1,prgpph,2.0e-6,2.0e-6);
        rgpulse(2*prgpw,prgph1,2.0e-6,2.0e-6);
	satpulse(prgd1,prgpph,2.0e-6,2.0e-6);
        rgpulse(prgpw,prgph2,2.0e-6,2.0e-6);

        zgradpulse(-0.171*prggzlvl,prggt);
        delay(prggstab);
	satpulse(prgd2,prgpph,2.0e-6,2.0e-6);
        zgradpulse(0.685*prggzlvl,prggt);
        delay(prggstab);
	obspower(tpwr);
}

void shaped_purge(codeint prgph1, codeint prgpph, codeint prgph2,
                  codeint prgph3)
{
  double prgpw = getval("prgpw"),
         prgd1 = getval("prgd1"),
         prggt = getval("prggt"),
         prggstab = getval("prggstab"),
         prggzlvl = getval("prggzlvl"),
         prgd2 = getval("prgd2");

        (void) prggzlvl;
        add(prgph2,prgph1,prgph2);
        add(prgph3,prgph1,prgph3);

        obspower(tpwr);
        rgpulse(prgpw,prgph1,2.0e-6,2.0e-6);
	shaped_satpulse("prgd1shp",prgd1,prgpph);
        rgpulse(2*prgpw,prgph1,2.0e-6,2.0e-6);
        shaped_satpulse("prgd1shp",prgd1,prgpph);
        rgpulse(prgpw,prgph3,2.0e-6,2.0e-6);

        zgradpulse(-0.132*prggzlvl,prggt);
        delay(prggstab);
        shaped_satpulse("prgd2shp",prgd2,prgpph);
        zgradpulse(0.527*prggzlvl,prggt);
        delay(prggstab);

        rgpulse(prgpw,prgph1,2.0e-6,2.0e-6);
        shaped_satpulse("prgd1shp",prgd1,prgpph);
        rgpulse(2*prgpw,prgph1,2.0e-6,2.0e-6);
        shaped_satpulse("prgd1shp",prgd1,prgpph);
        rgpulse(prgpw,prgph2,2.0e-6,2.0e-6);

        zgradpulse(-0.171*prggzlvl,prggt);
        delay(prggstab);
        shaped_satpulse("prgd2shp",prgd2,prgpph);
        zgradpulse(0.685*prggzlvl,prggt);
        delay(prggstab);
        obspower(tpwr);
}
	
void steadystate()
{
  double hsglvl,
	hsgt;
  char PFGflg[MAXSTR];

  getstr("PFGflg",PFGflg);
  hsglvl=getval("hsglvl");
  hsgt=getval("hsgt");

	if (PFGflg[0] == 'y')
        {
         zgradpulse(hsglvl,hsgt);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(hsglvl,hsgt);
        }
        else
        {
        obspower(tpwr-12);
        rgpulse(500*pw,zero,rof1,rof1);
        rgpulse(500*pw,one,rof1,rof1);
        obspower(tpwr);
	}
}

/*   Flip back pulse definition */
void FBpulse(codeint phase, codeint phaseinc)
{
  char fbshp[MAXSTR];
  double fbpw,
         fbpwr;

  getstr("fbshp",fbshp);
  fbpw = getval("fbpw");
  fbpwr = getval("fbpwr");
       obsstepsize(1.0);
       obspower(fbpwr);
       xmtrphase(phaseinc);
       shaped_pulse(fbshp,fbpw,phase,rof1,rof1);
       obspower(tpwr);
       xmtrphase(zero);
}

	
/*-----------------MLEV17c definition-----------------------------*/
void mlevc(double width, codeint phsA, codeint phsB)
{
   txphase(phsA); delay(width);
   xmtroff(); delay(width); xmtron();
   txphase(phsB); delay(2*width);
   xmtroff(); delay(width); xmtron();
   txphase(phsA); delay(width);
}


void mlev17c(double length, double width, codeint phsw, codeint phsx,
             codeint phsy, codeint phsz)
{
   double  cycles;
   cycles = length/(96.67*width);
   cycles = 2*(double) (int) (cycles/2);
   initval(cycles, v24);

  if (length > 0.0)
  {

   obsunblank();
   xmtron(); 

   starthardloop(v24);

    mlevc(width,phsy,phsz); 
    mlevc(width,phsw,phsx);
    mlevc(width,phsw,phsx); 
    mlevc(width,phsy,phsz);

    mlevc(width,phsy,phsz);
    mlevc(width,phsy,phsz); 
    mlevc(width,phsw,phsx);
    mlevc(width,phsw,phsx); 

    mlevc(width,phsw,phsx); 
    mlevc(width,phsy,phsz);
    mlevc(width,phsy,phsz); 
    mlevc(width,phsw,phsx);

    mlevc(width,phsw,phsx);
    mlevc(width,phsw,phsx); 
    mlevc(width,phsy,phsz);
    mlevc(width,phsy,phsz); 

    txphase(phsx); delay(0.67*width);

  endhardloop();

   xmtroff();  
   obsblank();

  }
}


/*-----------------MLEV17 definition-----------------------------*/
void mlev(double width, codeint phsA, codeint phsB)
{
   txphase(phsA); delay(width);
   txphase(phsB); delay(2*width);
   txphase(phsA); delay(width);
}


void mlev17(double length, double width, codeint phsw, codeint phsx,
            codeint phsy, codeint phsz)
{
   double  cycles;
   cycles = length/(64.67*width);
   cycles = 2*(double) (int) (cycles/2);
   initval(cycles, v24);

  if (length > 0.0)
  {

   obsunblank();
   xmtron();

   starthardloop(v24);

    mlev(width,phsy,phsz);
    mlev(width,phsw,phsx);
    mlev(width,phsw,phsx);
    mlev(width,phsy,phsz);

    mlev(width,phsy,phsz);
    mlev(width,phsy,phsz);
    mlev(width,phsw,phsx);
    mlev(width,phsw,phsx);

    mlev(width,phsw,phsx);
    mlev(width,phsy,phsz);
    mlev(width,phsy,phsz);
    mlev(width,phsw,phsx);

    mlev(width,phsw,phsx);
    mlev(width,phsw,phsx);
    mlev(width,phsy,phsz);
    mlev(width,phsy,phsz);

    txphase(phsx); delay(0.67*width);

   endhardloop();

   xmtroff(); 
   obsblank();

  }
}


/*------------------------dipsi2 spinlock definition-------------*/
void dips2(double width, codeint phsA, codeint phsB)
{
      txphase(phsA); delay(320*width/90);
      txphase(phsB); delay(410*width/90);
      txphase(phsA); delay(290*width/90);
      txphase(phsB); delay(285*width/90);
      txphase(phsA); delay(30*width/90);
      txphase(phsB); delay(245*width/90);
      txphase(phsA); delay(375*width/90);
      txphase(phsB); delay(265*width/90);
      txphase(phsA); delay(370*width/90);
}


void dipsi2(double length, double width, codeint phsx, codeint phsy)
{
  double cycles;
  cycles = length/(width*115.11);
  cycles = 2*(double)(int)(cycles/2);
  initval(cycles, v24);

  if (length > 0.0)
  {
    obsunblank();
    xmtron(); 
    starthardloop(v24);
       dips2(width,phsx,phsy); dips2(width,phsy,phsx); 
       dips2(width,phsy,phsx); dips2(width,phsx,phsy);
    endhardloop();
    xmtroff(); 
    obsblank();
   }
}

/*------------------------dipsi3 spinlock definition-------------*/
void dips3(double width, codeint phsA, codeint phsB)
{
      txphase(phsA); delay(245*width/90);
      txphase(phsB); delay(395*width/90);
      txphase(phsA); delay(250*width/90);
      txphase(phsB); delay(275*width/90);
      txphase(phsA); delay(30*width/90);
      txphase(phsB); delay(230*width/90);
      txphase(phsA); delay(90*width/90);
      txphase(phsB); delay(245*width/90);
      txphase(phsA); delay(370*width/90);
      txphase(phsB); delay(340*width/90);
      txphase(phsA); delay(350*width/90);
      txphase(phsB); delay(260*width/90);
      txphase(phsA); delay(270*width/90);
      txphase(phsB); delay(30*width/90);
      txphase(phsA); delay(225*width/90);
      txphase(phsB); delay(365*width/90);
      txphase(phsA); delay(255*width/90);
      txphase(phsB); delay(395*width/90);
}


void dipsi3(double length, double width, codeint phsx, codeint phsy)
{
  double cycles;
  cycles = length/(width*217.33);
  cycles = 2*(double)(int)(cycles/2);
  initval(cycles, v24);

  if (length > 0.0)
  {
    obsunblank();
    xmtron(); 
    starthardloop(v24);
       dips3(width,phsx,phsy); dips3(width,phsy,phsx); 
       dips3(width,phsy,phsx); dips3(width,phsx,phsy);
    endhardloop();
    xmtroff(); 
    obsblank();
   }
}

/*-------------transverse roesy spinlock definition---------------*/

void troesy(double length, double width, codeint phs1, codeint phs2)
{
  double cycles;
   cycles = length/(width*4);
   cycles = 2*(double) (int) (cycles/2);
   initval(cycles, v24);

  if (length > 0.0)
  {
   obsunblank();
   xmtron();
   starthardloop(v24);
	txphase(phs1); delay(2*width);
	txphase(phs2); delay(2*width);
   endhardloop();
   xmtroff();
   obsblank();
  }
}

/*------------------dante spinlock---------------------*/

void dante_spinlock(double length, double width, codeint phs1)
{
  double cycles,
  	 ratio;
   ratio = getval("ratio");
   cycles = length/(width/3*16*(ratio+1));
   cycles = 2*(double) (int) (cycles/2);
   initval(cycles, v24);

  if (length > 0.0)
  {
       delay(ratio*width/6);
      starthardloop(v24);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
      endhardloop();
       delay(ratio*width/6);
 } 
}

/*------------------cw spinlock---------------------*/

void cw_spinlock(double length, codeint phs1)
{
   (void) phs1;
   rgpulse(length,phs1,0.0,0.0);
} 

/*----------------- SpinLock definition---------------------*/

void SpinLock(char *pattern, double length, double width, codeint phsb)
{

   sub(phsb,one,v20);
   add(v20,one,v21);
   add(v21,one,v22);
   add(v22,one,v23);

   if (!strcmp(pattern,"mlev17c"))
      mlev17c(length,width,v20,v21,v22,v23);

   if (!strcmp(pattern,"dipsi2"))
      dipsi2(length,width,v21,v23);

   if (!strcmp(pattern,"mlev17"))
      mlev17(length,width,v20,v21,v22,v23);

   if (!strcmp(pattern,"dipsi3"))
      dipsi3(length,width,v21,v23);

   if (!strcmp(pattern,"cw"))
      cw_spinlock(length,v21);

   if (!strcmp(pattern,"dante"))
      dante_spinlock(length,width,v21);

   if (!strcmp(pattern,"troesy"))
      troesy(length,width,v21,v23);
}

/*--------STEP (Selective Tocsy Edited Preparation) definition----------*/

void steptrain(codeint stepph1, codeint stepph2)
{
        double  steppwr=getval("steppwr"),
                steppw=getval("steppw"),
		stepmix=getval("stepmix"),
                stepGlvl1=getval("stepGlvl1"),
                stepgt1=getval("stepgt1"),
                stepGlvl2=getval("stepGlvl2"),
                stepgt2=getval("stepgt2"),
                stepGlvl3=getval("stepGlvl3"),
                stepgt3=getval("stepgt3"),
                stepslpwr=getval("stepslpwr"),
                stepslpw=getval("stepslpw"),
                stepZpwr1=getval("stepZpwr1"),
                stepZpw1=getval("stepZpw1"),
                stepZpwr2=getval("stepZpwr2"),
                stepZpw2=getval("stepZpw2"),
                stepZglvl1=getval("stepZglvl1"),
                stepZglvl2=getval("stepZglvl2"),
                stepgstab=getval("stepgstab");
        char    stepshape[MAXSTR],
		steppat[MAXSTR],
		stepdm[MAXSTR],
                stepZpat1[MAXSTR],
                stepZpat2[MAXSTR];

        (void) stepZglvl2;
        (void) stepZglvl1;
        (void) stepZpwr2;
        (void) stepZpwr1;
        (void) stepslpwr;
        (void) stepGlvl3;
        (void) stepGlvl2;
        (void) stepGlvl1;
        (void) steppwr;

        getstr("stepshape",stepshape);
	getstr("steppat",steppat);
        getstr("stepZpat1",stepZpat1);
        getstr("stepZpat2",stepZpat2);
	getstr("stepdm",stepdm);

        zgradpulse(stepGlvl1,stepgt1);
        delay(stepgstab);
	obspower(steppwr);

	if (stepdm[0] == 'y')
	{
	    decpower(dpwr);
	    decprgon(dseq,1/dmf,dres);
	    decunblank();
	    decon();
	}
        shaped_pulse(stepshape,steppw,stepph2,rof1,rof1);
	if (stepdm[0] == 'y')
	{
	    decoff();
	    decblank();
	    decprgoff();
	}

        obspower(tpwr);
        zgradpulse(stepGlvl1,stepgt1);
        delay(stepgstab);

        zgradpulse(1.5*stepGlvl1,stepgt1);
        delay(stepgstab);
        obspower(steppwr);
	if (stepdm[0] == 'y')
        {
            decpower(dpwr);
            decprgon(dseq,1/dmf,dres);
            decunblank();
            decon();
        }
        shaped_pulse(stepshape,steppw,stepph1,rof1,rof1);
	if (stepdm[0] == 'y')
        {
            decoff();
            decblank();
            decprgoff();
        }

        obspower(tpwr);
        zgradpulse(1.5*stepGlvl1,stepgt1);
        delay(stepgstab);
        rgpulse(pw, stepph1, rof1, rof1);
         obspower(stepZpwr1);
         rgradient('z',stepZglvl1);
         delay(100.0e-6);
         shaped_pulse(stepZpat1,stepZpw1,stepph1,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(stepgstab);
        obspower(stepslpwr);

        zgradpulse(stepGlvl2,stepgt2);
        delay(stepgstab);

        add(stepph1,one,stepph1);
        if (stepmix > 0.0)
        {
          if (dps_flag)
                rgpulse(stepmix,stepph1,0.0,0.0);
          else
                SpinLock(steppat,stepmix,stepslpw,stepph1);
        }
        sub(stepph1,one,stepph1);

         obspower(stepZpwr2);
         rgradient('z',stepZglvl2);
         delay(100.0e-6);
         shaped_pulse(stepZpat2,stepZpw2,stepph1,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(stepgstab);
        obspower(tpwr);
        zgradpulse(stepGlvl3,stepgt3);
        delay(stepgstab);

        rgpulse(pw, stepph1, rof1,rof2);
}

#endif	// CHEMPACK_H

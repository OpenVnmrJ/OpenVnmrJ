/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* watergate element */


WGpulse(phase,phaseinc)

   codeint phase, phaseinc;

{
   double  wgtau,
	  wgspw,
	  wgspwr,
	  wgpw180,
	  gzlvlwg,
	  gtwg,
	  gstabwg;
   char flag3919[MAXSTR],
	wgsshape[MAXSTR],
	dblgate[MAXSTR];
    wgtau = getval("wgtau");
    gzlvlwg = getval("gzlvlwg");
    gtwg = getval("gtwg");
    gstabwg = getval("gstabwg");
    wgspw = getval("wgspw");
    wgspwr = getval("wgspwr");
    wgpw180 = getval("wgpw180");
    getstr("flag3919",flag3919);
    getstr("wgsshape",wgsshape);
    getstr("dblgate",dblgate);

   if (flag3919[0] == 'y')
     {
       zgradpulse(gzlvlwg,gtwg);
       delay(gstabwg);
       pulse(wgpw180*0.231/2,phase);
       delay(wgtau);
       pulse(wgpw180*0.692/2,phase);
       delay(wgtau);
       pulse(wgpw180*1.462/2,phase);
       add(phase,two,phase);
       delay(wgtau);
       pulse(wgpw180*1.462/2,phase);
       delay(wgtau);
       pulse(wgpw180*0.692/2,phase);
       delay(wgtau);
       pulse(wgpw180*0.231/2,phase);
       sub(phase,two,phase);
       zgradpulse(gzlvlwg,gtwg);
       delay(gstabwg);

       if (dblgate[0] == 'y')
       {
	add(phase,two,phase);
       zgradpulse(0.6*gzlvlwg,0.6*gtwg);
       delay(gstabwg);
       pulse(wgpw180*0.231/2,phase);
       delay(wgtau);
       pulse(wgpw180*0.692/2,phase);
       delay(wgtau);
       pulse(wgpw180*1.462/2,phase);
       sub(phase,two,phase);
       delay(wgtau);
       pulse(wgpw180*1.462/2,phase);
       delay(wgtau);
       pulse(wgpw180*0.692/2,phase);
       delay(wgtau);
       pulse(wgpw180*0.231/2,phase);
       zgradpulse(0.6*gzlvlwg,0.6*gtwg);
       delay(gstabwg);
       }

    }
  else
    {

       obsstepsize(1.0);

       zgradpulse(gzlvlwg,gtwg);
       delay(gstabwg-2.0*SAPS_DELAY);
       obspower(wgspwr);
       xmtrphase(phaseinc);
       add(phase,two,phase);
       shaped_pulse(wgsshape,wgspw,phase,rof1,rof1);
       sub(phase,two,phase);
       obspower(tpwr);
       xmtrphase(zero);
       rgpulse(wgpw180,phase,rof1,rof1);
       obspower(wgspwr);
       add(phase,two,phase);
       shaped_pulse(wgsshape,wgspw,phase,rof1,rof1);
       sub(phase,two,phase);
       obspower(tpwr);
       zgradpulse(gzlvlwg,gtwg);
       delay(gstabwg);


	if (dblgate[0] == 'y')
	{

       zgradpulse(0.6*gzlvlwg,0.6*gtwg);
       delay(gstabwg-2.0*SAPS_DELAY);
       obspower(wgspwr);
       xmtrphase(phaseinc);
       shaped_pulse(wgsshape,wgspw,phase,rof1,rof1);
       add(phase,two,phase);
       obspower(tpwr);
       xmtrphase(zero);
       rgpulse(wgpw180,phase,rof1,rof1);
       obspower(wgspwr);
       sub(phase,two,phase);
       shaped_pulse(wgsshape,wgspw,phase,rof1,rof1);
       obspower(tpwr);
       zgradpulse(0.6*gzlvlwg,0.6*gtwg);
       delay(gstabwg);

       }
    }

}

FBpulse(phase,phaseinc)
codeint phase, phaseinc;

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


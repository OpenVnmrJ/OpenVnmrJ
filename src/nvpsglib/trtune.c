// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/*  trtune - probe tuning sequence */
#ifdef OLDVERSIONS
void getRealSetDefault(int tree, const char *name, double *buf, double def)
{
	int index;
	if (tree != CURRENT)
	{
		if (P_getreal(tree, name,buf,1) < 0)
		*buf = def;
	}
	else
	{
		index = find(name);   /* hash table find */
	    if (index == NOTFOUND)
	       *buf = def;
	    else
	       *buf = *( (double *) cvals[index]);
	}
}
#endif

#include <standard.h>
void pulsesequence()
{
   double freq,fstart,fend;
   double attn,attnd,attnd2,attnd3,attnd4,tattn;/* 5 channels supported */
   double tunesw,tuneswd,tuneswd2,tuneswd3,tuneswd4,tsw;
   double gain,gaind,gaind2,gaind3,gaind4,tgain;
   int chan;
   double offset_sec;
   int np2;
   int nfv,index;

   nfv = (int) getval("nf");
   np2 = np / 2;
   status(A);
   /* getRealSetDefault reduces logic - not in Inova */
   getRealSetDefault(CURRENT,"tunesw",&tunesw,10000000.0);
   getRealSetDefault(CURRENT,"tuneswd",&tuneswd,tunesw);
   getRealSetDefault(CURRENT,"tuneswd2",&tuneswd2,tunesw);
   getRealSetDefault(CURRENT,"tuneswd3",&tuneswd3,tunesw);
   getRealSetDefault(CURRENT,"tuneswd4",&tuneswd4,tunesw);

   getRealSetDefault(CURRENT,"tupwr",&attn,10.0);
   getRealSetDefault(CURRENT,"tupwrd",&attnd,10.0);
   getRealSetDefault(CURRENT,"tupwrd2",&attnd2,10.0);
   getRealSetDefault(CURRENT,"tupwrd3",&attnd3,10.0);
   getRealSetDefault(CURRENT,"tupwrd4",&attnd4,10.0);
   getRealSetDefault(CURRENT,"gain",&gain,10.0);
   getRealSetDefault(CURRENT,"gaind",&gaind,gain);
   getRealSetDefault(CURRENT,"gaind2",&gaind2,gain);
   getRealSetDefault(CURRENT,"gaind3",&gaind3,gain);
   getRealSetDefault(CURRENT,"gaind4",&gaind4,gain);
   offset_sec = (0.5 / sw);
   setacqmode(WACQ|NZ); 
   for (index = 0; index < nf; index++)
   {
     switch(index) {
      case 0:  chan = OBSch; freq = sfrq; tattn = attn; 
                    tgain = gain; tsw = tunesw; break;
      case 1:  chan = DECch; freq = dfrq; tattn = attnd; 
                    tgain = gaind; tsw = tuneswd; break;
      case 2:  chan = DEC2ch; freq = dfrq2; tattn = attnd2; 
                    tgain = gaind2; tsw = tuneswd2; break;
      case 3:  chan = DEC3ch; freq = dfrq3; tattn = attnd3; 
                    tgain = gaind3; tsw = tuneswd3; break;
      case 4:  chan = DEC4ch; freq = dfrq4; tattn = attnd4; 
                    tgain = gaind4; tsw = tuneswd4; break;
      default:  exit(-1);
     }
     fstart = freq - (tsw/2) * 1e-6;
     fend = freq + (tsw/2) * 1.0e-6;
     //printf("channel = %d  frequency = %f\n",chan,freq);
     //printf("channel = %d  frequency span = %f\n",chan, tsw);
     //printf("start=%f  stop = %f\n",fstart,fend);
     //printf("gain = %f power = %f\n",tgain,tattn);
     hsdelay(d1);
     set4Tune(chan,tgain); 
     assign(zero,oph);
     genPower(tattn,chan);
     delay(0.001);
     startacq(alfa);
     SweepNOffsetAcquire(fstart, fend, np2, chan, offset_sec); 
     endacq();
     delay(0.001);
   }
}

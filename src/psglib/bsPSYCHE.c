/*   PSYCHE 

PureShift Yielded by CHirp Excitation

     Reference:
 	Foroozandeh, M; Adams, RW; Meharry, NJ; Jeannerat, D, Nilsson, M; Morris, GA, Angew. Chem., Int. Ed., 2014, 53, 6990.

*/

#include <standard.h>
#include <chempack.h>

static int	ph1[8] = {0,2,0,2,0,2,0,2},
		ph2[8] = {0,0,0,0,0,0,0,0},
		ph3[8] = {0,0,1,1,2,2,3,3},
		ph4[8] = {0,2,2,0,0,2,2,0};

pulsesequence()
{

   double	   gt1 = getval("gt1"),
		   gzlvl1=getval("gzlvl1"),
		   gt2 = getval("gt2"),
		   gzlvl2=getval("gzlvl2"),
		   selpwrPS = getval("selpwrPS"),
		   selpwPS = getval("selpwPS"),
		   gzlvlPS = getval("gzlvlPS"),
		   droppts=getval("droppts"),
		   selfrq=getval("selfrq"),
                   selpwrA = getval("selpwrA"),
                   selpwA = getval("selpwA"),
                   gzlvlA = getval("gzlvlA"),
                   gtA = getval("gtA"),
                   selpwrB = getval("selpwrB"),
                   selpwB = getval("selpwB"),
                   gzlvlB = getval("gzlvlB"),
                   gtB = getval("gtB"),
		   gstab = getval("gstab");
   int 		   prgcycle=(int)(getval("prgcycle")+0.5);
   char		   selshapePS[MAXSTR],
		   selshapeA[MAXSTR],
		   selshapeB[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gt1 = syncGradTime("gt1","gzlvl1",1.0);
        gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);
	gt2 = syncGradTime("gt2","gzlvl2",1.0);
        gzlvl2 = syncGradLvl("gt2","gzlvl2",1.0);
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("selshapePS",selshapePS);
   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);

  assign(ct,v17);
  assign(zero,v18);
  assign(zero,v19);

  if (getflag("prgflg") && (satmode[0] == 'y') && (prgcycle > 1.5))
    {
        hlv(ct,v17);
        mod2(ct,v18); dbl(v18,v18);
        if (prgcycle > 2.5)
           {
                hlv(v17,v17);
                hlv(ct,v19); mod2(v19,v19); dbl(v19,v19);
           }
     }

   settable(t1,8,ph1);
   settable(t2,8,ph2);
   settable(t3,8,ph3);
   settable(t4,8,ph4);

   getelem(t1,v17,v1);
   getelem(t2,v17,v2);
   getelem(t3,v17,v3);
   getelem(t4,v17,v4);
   assign(v4,oph);
   add(v1,two,v5);

   assign(v1,v6);
   add(oph,v18,oph);
   add(oph,v19,oph);

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
      obspower(tpwr);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        if (getflag("slpsat"))
           {
                shaped_satpulse("relaxD",satdly,v6);
                if (getflag("prgflg"))
                   shaped_purge(v1,v6,v18,v19);
           }
        else
           {
                satpulse(satdly,v6,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,v6,v18,v19);
           }
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);


   status(B);
      obspower(tpwr);
      rgpulse(pw, v1, rof1, rof2);

        zgradpulse(gzlvlA,gtA);
        delay(gstab);
        obspower(selpwrA);
        shaped_pulse(selshapeA,selpwA,v1,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlA,gtA);
        delay(gstab);

        zgradpulse(gzlvlB,gtB);
        delay(gstab);
        obspower(selpwrB);
        shaped_pulse(selshapeB,selpwB,v5,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);

	delay(d2/2.0);

	delay((0.25/sw1) - gt1 - gstab - 2*pw/PI - rof2 - 2*GRADIENT_DELAY - pw - rof1);
	zgradpulse(gzlvl1,gt1);
	delay(gstab);
	rgpulse(2*pw,v2,rof1,rof1);
	zgradpulse(gzlvl1,gt1);
	delay(gstab);
	delay((0.25/sw1) - gt1 - gstab - 2*GRADIENT_DELAY - pw - rof1);

	delay(gstab);
	zgradpulse(gzlvl2,gt2);
	delay(gstab);
	obspower(selpwrPS);
	rgradient('z',gzlvlPS);
	shaped_pulse(selshapePS,selpwPS,v3,rof1,rof1);
	rgradient('z',0.0);
	delay(gstab);
	obspower(tpwr);
	zgradpulse(gzlvl2,gt2);
	delay(gstab - droppts/sw);

	delay(d2/2.0);

   status(C);
}

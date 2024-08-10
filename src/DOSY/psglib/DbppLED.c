/*********************************************************************
 * DbppLED - Bipolar gradient pulse pair with composite pulses and
 * longitudinal eddy-current delay
 * ref : J. Magn. Reson. Ser. A, 115, 260-264 (1995).
 * June 19, 2014 Composite pulse option added by C. Xu, Y. Wan, D. Chen and
 * P.L.Rinaldi
 * parameters:
 * delflag - 'y' runs the DbppADled sequence, 'n' runs normal s2pul
 * del - the actual diffusion delay
 * delst - eddy-current storage delay
 * gt1 - total diffusion-encoding pulse width
 * gzlvl1 - diffusion-encoding pulse strength
 * gt2,gt3 - crusher gradient pulse width
 * gzlvl2,gzlvl3 - crusher gradient pulse strength
 * gstab - gradient stabilization delay (~0.0002-0.0003 sec)
 * fn2D - Fourier number to build up the 2D display in F2
 * wet - 'y' turns wet flag on
 * satmode - 'y' turns on presaturation
 * comppulflag - 'y' uses composite pulses instead of simple 180's
 * tau taken as time between the mid-points of the bipolar gradient pulses
 ***********************************************************/
#include <standard.h>
#include <chempack.h>
void pulsesequence()
{
double del = getval("del"),
gstab = getval("gstab"),
gt1 = getval("gt1"),
gzlvl1 = getval("gzlvl1"),
Dtau,Ddelta,dosytimecubed,dosyfrq,
gzlvlhs = getval("gzlvlhs"),
hsgt = getval("hsgt"),
satpwr = getval("satpwr"),
satdly = getval("satdly"),
satfrq = getval("satfrq"),
gt2 = getval("gt2"),
gzlvl2 = getval("gzlvl2"),
gt3 = getval("gt3"),
gzlvl3 = getval("gzlvl3"),
delst = getval("delst");
char
delflag[MAXSTR],wet[MAXSTR],satmode[MAXSTR],sspul[MAXSTR],comppulflag[MAXSTR];
getstr("delflag",delflag);
getstr("wet",wet);
getstr("satmode",satmode);
getstr("comppulflag",comppulflag);
getstr("sspul",sspul);
/* In pulse sequence, minimum del=4.0*pw+3*rof1+gt1+2.0*gstab+gt2+gstab */
if (del < (4*pw+3.0*rof1+gt1+2.0*gstab+gt2+gstab))
{ abort_message("DbppLED error: 'del' is less than %g, too short!",(4.0*pw+3*rof1+gt1+2.0*gstab+gt2+gstab));
}
Ddelta=gt1;
if(comppulflag[A] == 'y')
{Dtau=4.0*pw+rof1+gstab+gt1/2.0;
}
else
{Dtau=2.0*pw+rof1+gstab+gt1/2.0;
}
dosyfrq = sfrq;
dosytimecubed=Ddelta*Ddelta*(del-(Ddelta/3.0)-(Dtau/2.0));
putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);
/* Check for the validity of the number of transients selected ! */
if ( (nt != 16) && ((int)nt%64 != 0) )
{ abort_message("DbppLED error: nt must be 16, 64 or n*64 (n: integer)!");
}
/* phase cycling calculation */
/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR) or off of * CT */
ifzero(ssctr);
assign(ct,v13);
elsenz(ssctr);
sub(ssval, ssctr, v13); /* v13 = 0,...,ss-1 */
endif(ssctr);
assign(zero,v1);
mod2(v13,v4);
dbl(v4,v4);
hlv(v13,v13);
dbl(v13,v2);
hlv(v13,v13);
dbl(v13,v3);
add(v3,v4,v4);
hlv(v13,v13);
/* v1 = 0 */
/* v2 = 0 0 2 2 */
mod2(v13,v12);
add(v3,v12,v3);
add(v4,v12,v4);
assign(v3,v5);
add(v1,v2,oph);
add(v3,oph,oph);
add(v4,oph,oph);
add(v5,oph,oph);
assign(zero,v6);
assign(zero,v7);
/* v3 = 4x0 4x2 4x1 4x3 */
/* v4 = 0202 2020 1313 3131 */
/* v5 = 4x0 4x2 4x1 4x3 */
/* oph = v1 + v2 + v3 + v4 + v5 */
/* v6 =0 */
/* v7 =0 */
add(one,v6,v8);
add(one,v7,v9);
/* CYCLOPS */
hlv(v13,v14);
add(v1,v14,v1);
add(v2,v14,v2);
add(v3,v14,v3);
add(v4,v14,v4);
add(v5,v14,v5);
add(v6,v14,v6);
add(v7,v14,v7);
add(oph,v14,oph);
if (ni > 0.0)
{ abort_message("DbppLED is a 2D, not a 3D dosy sequence: please set ni to 0");
}
/* equilibrium period */
status(A);
if (sspul[0]=='y')
{
zgradpulse(gzlvlhs,hsgt);
rgpulse(pw,zero,rof1,rof1);
zgradpulse(gzlvlhs,hsgt);
}
if (satmode[0] == 'y')
{
if (d1 - satdly > 0)
delay(d1 - satdly);
else
delay(0.02);
obspower(satpwr);
txphase(v1);
if (satfrq != tof)
obsoffset(satfrq);
rgpulse(satdly,zero,rof1,rof1);
if (satfrq != tof)
obsoffset(tof);
obspower(tpwr);
delay(1.0e-5);
}
else
{ delay(d1); }
if (wet[0] == 'y')
wet4(zero,one);
status(B);
if (delflag[0]=='y')
{ if (gt1>0 && gzlvl1>0)
{ rgpulse(pw, v1, rof1, 0.0);
/* first 90, v1 */
zgradpulse(gzlvl1,gt1/2.0);
delay(gstab);
if(comppulflag[A]=='y')
{
rgpulse(pw,v6,rof1,0.0);
rgpulse(pw*2.0,v8,0.0,0.0);
rgpulse(pw,v6,0.0,rof1);
}
else rgpulse(pw*2.0, v7, rof1, 0.0); /* second 180, v6 */
zgradpulse(-1.0*gzlvl1,gt1/2.0);
delay(gstab);
rgpulse(pw, v2, rof1, 0.0);
/* second 90, v2 */
zgradpulse(gzlvl2,gt2);
delay(gstab);
if(comppulflag[A] == 'y')
{delay(del-6.0*pw-3.0*rof1-gt1-2.0*gstab-gt2-gstab); /*diffusion delay */;
}
else
delay(del-4.0*pw-3.0*rof1-gt1-2.0*gstab-gt2-gstab); /*diffusion delay */
rgpulse(pw, v3, rof1, 0.0);
/* third 90, v3 */
zgradpulse(gzlvl1,gt1/2.0);
delay(gstab);
if(comppulflag[A]=='y')
{
rgpulse(pw,v7,rof1,0.0);
rgpulse(pw*2.0,v9,0.0,0.0);
rgpulse(pw,v7,0.0,rof1);
}
else rgpulse(pw*2.0, v7, rof1, 0.0); /* second 180, v7 */
zgradpulse(-1.0*gzlvl1,gt1/2.0);
delay(gstab);
rgpulse(pw, v4, rof1, 0.0);
/* fourth 90, v4 */
zgradpulse(gzlvl3,gt3);
delay(delst);
/* eddy current storage delay */
rgpulse(pw, v5, rof1, rof2);
}
}
else
rgpulse(pw,oph,rof1,rof2);
status(C);
}

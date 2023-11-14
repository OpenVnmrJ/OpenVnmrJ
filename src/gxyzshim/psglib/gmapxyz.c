// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* gmapxyz.c  11 June 2003 */

/* 3 pulse STE sequence for 3D gradient shimming 	*/
/* uses x1 and y1 shim gradients for pulses		*/ 
/* uses z1 homospoil rather than z1 shim gradient	*/
/* because of acquisition software bug			*/ 
/* from VVKimag14 					*/

/* correct del to keep echo at constant time GAM 11vi03 */
/* Increase gt3 purge gradient for use with PFG PJB 11viii03 */

/* local parameters:					*/
/*	fov	transverse field of view in mm		*/
/*	gt1	delay for xy gradient settling, 200 ms	*/
/*	gt2	delay between x and y switching, 10 ms	*/
/*	gt3	purge pulse during del			*/
/*	d4	dephasing delay before acquisition	*/
/*	tau	arrayed delay for field mapping		*/
/*	hspcorr	correction for homospoil rise time	*/


/* global parameters:					*/
/*	gcalx	x1 shim G/cm per DAC point		*/
/*	gcaly	y1 shim G/cm per DAC point		*/
/*	gcalang	error in degrees in y1 shim angle	*/

#include <standard.h>
#define GLOBAL          0


static int ph1[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3};
static int ph2[4] = {0,1,2,3};
static int ph3[16] = {0,1,2,3,3,0,1,2,2,3,0,1,1,2,3,0};

 
void pulsesequence()
{
        int index1,index2,ni,ni2;
        double gt1,gt2,gt3,tau,d5,del,range,hspcorr,shimset,gzlvl1,gzlvl3,
        ginc,x1m,y1m,gamma,gstab,gzlvlhs,gths,
	gxlvl,gylvl, fov, gcalx, gcaly, gcalang,
	shapedpw90 = getvalnwarn("shapedpw90"),
	shapedpw180 = getvalnwarn("shapedpw180"),
	shapedpwr90 = getvalnwarn("shapedpwr90"),
	shapedpwr180 = getvalnwarn("shapedpwr180");

        char tn[MAXSTR], steflg[MAXSTR], shaped[MAXSTR], shapename90[MAXSTR], shapename180[MAXSTR];
        getstr("tn",tn);
        getstr("steflg",steflg);
	getstrnwarn("shaped", shaped);
	getstrnwarn("shapename90", shapename90);
	getstrnwarn("shapename180", shapename180);

        if (P_getreal(GLOBAL, "shimset", &shimset, 1) < 0)
           warn_message("shimset global parameter not found\n");
	gcalx = P_getval("gcalx");
	gcaly = P_getval("gcaly");   
	gcalang = P_getval("gcalang");   
	ni = getval("ni");   
	ni2 = getval("ni2");   
        gt1 = getval("gt1");   
        gt2 = getval("gt2");
        gt3 = getval("gt3");      
        gths = getval("gths");      
        gzlvlhs = getval("gzlvlhs");
        gzlvl1 = getval("gzlvl1");
	gzlvl3 = getval("gzlvl3");   
        del = getval("del");      
        gstab = getval("gstab");   
        d4 = getval("d4");   
        tau = getval("tau");   
        hspcorr = getval("hspcorr");   
        x1m = getval("x1");   
        y1m = getval("y1");  
        fov = getval("fov");  
  
        range=32767.0;
        if ((shimset<3.0) || ((shimset>9.5) && (shimset<11.5))) range = 2047;
	index1=(int)(d2*getval("sw1")+0.5);
	index2=(int)(d3*getval("sw2")+0.5);
	if (steflg[0] == 'n') {
          if (shaped[A] == 'y')
	    d5=d4+at+tau+2.0*gstab+shapedpw90/2.0+0.00001;
          else
	    d5=d4+at+tau+2.0*gstab+pw/2.0+0.00001;
	}
	else
	  d5=d4+at/2; 

/* assume mapping 1H unless tn='H2' or 'lk' or 'F19'	*/     
	gamma=267522128.0;
     	if (tn[0]=='F') gamma=251814800.0;
     	if (tn[0]=='l') gamma=41066279.0;
     	if (tn[1]=='2') gamma=41066279.0;

	settable(t1, 16, ph1);       
        settable(t2, 4, ph2);
        settable(t3, 16, ph3);  

    	ginc=200000.0*3.14159265358979323846/(gamma*d5*fov); /* in G/cm */
	gylvl=(index2-((ni2-1)/2.0))*ginc/(gcaly*cos(gcalang*3.14159265358979323846/180.0));
	gxlvl=((index1-((ni-1)/2.0))*ginc/gcalx)-(index2-((ni2-1)/2.0))*ginc*tan(gcalang*3.14159265358979323846/180.0)/gcalx;
        
if ((ni==0)||(ni==1))
{
        gxlvl=0; gylvl=0;
}

        if (((x1m+gxlvl)>range)||((x1m+gxlvl)<(-range)))
         {
             printf("X shim is taken out of valid range\n");
                psg_abort(1);  
         }

        if (((y1m+gylvl)>range)||((y1m+gylvl)<(-range)))
         {
             printf("Y shim is taken out of valid range\n");
                psg_abort(1);  
         }


/*----- start of actual pulse sequence -----*/
        status(A);
        rcvroff();

	if (steflg[0] == 'y') 
	{
		delay((d1-gt1));
	        rgradient('x',gxlvl);
	        delay(gt2);
	        rgradient('y',gylvl);
	        delay(gt1-2.0*gt2);
	        rgradient('z',gzlvl1);
	        delay(gt2);
	        if (shaped[A] == 'y') {
		  obspower(shapedpwr90);
		  shaped_pulse(shapename90,shapedpw90,t1,rof1,rof2);
		}
	        else
	          rgpulse(pw,t1,0.00001,0.00001);
	       	delay(d5);
	        if (shaped[A] == 'y')
		  shaped_pulse(shapename90,shapedpw90,t2,rof1,rof2);
	        else
	          rgpulse(pw,t2,0.00001,0.00001);
	        rgradient('z',0.0);
	        delay(gt2);
	        rgradient('x',0.0);
	        delay(gt2);
	        rgradient('y',0.0);
	        status(B);
	        rgradient('z',gzlvl3);
	        delay(gt3);
	        rgradient('z',0.0);
	        delay(del-2.0*gt2-gt3-tau);
	        if (shaped[A] == 'y') {
		  shaped_pulse(shapename90,shapedpw90,zero,rof1,rof2);
		  obspower(tpwr);
		}
	        else
	          rgpulse(pw,zero,0.00001,0.00001);
	        setreceiver(t3);
	        rcvron();
	        delay(tau);
	        rgradient('z',gzlvl1);
	        delay(d4+hspcorr);
	        acquire(np,1.0/sw);
	        rgradient('z',0.0);
	} else {
	        rgradient('x',gxlvl);
	        rgradient('y',gylvl);
	        delay(d1/2.0);
	        zgradpulse(gzlvlhs,gths);
	        delay(d1/2.0-gths);
	        if (shaped[A] == 'y') {
		  obspower(shapedpwr90);
		  shaped_pulse(shapename90,shapedpw90,t2,rof1,rof2);
		  obspower(tpwr);
		}
	        else
	          rgpulse(pw,t2,0.00001,0.00001);
	        zgradpulse(-1.0*gzlvl1,at/2.0+gstab);
	       	delay(d4+tau);
	        status(B);
	        setreceiver(t2);
	        rcvron();
	        rgradient('z',gzlvl1);
		delay(gstab);
	        acquire(np,1.0/sw);
	        rgradient('x',0.0);
	        rgradient('y',0.0);
	        rgradient('z',0.0);
	}
}

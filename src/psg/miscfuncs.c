/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "oopc.h"
#ifndef NVPSG
#include "acodes.h"
#include "rfconst.h"
#else
#include "ACode32.h"
#endif
#include "acqparms.h"
#include "group.h"
#include "macros.h"

#if defined(SOLARIS) || defined(AIX) || defined(LINUX)
#ifndef M_LN10
#define M_LN10          2.30258509299404568402
#endif
#define  exp10( x )	(exp( M_LN10 * (x) ))
#endif

extern int  fifolpsize; /* fifo loop size */
extern int  dps_flag;
extern double   dps_fval[];

#ifndef NVPSG

/*---------------------------------------------------------
| crtfifo_cnt()
|	correct the timer count for either old or new 
|	output board.
|				Greg B. 11/20/88
+----------------------------------------------------------*/
int crtfifo_cnt(count)
int count;
{
   if (fifolpsize < 65)	/* determine output board type */
   {
      if (count > 4095)
         count = 4095;	/* count larger than 12 bits */
   }
   else
   {
      if (count > 4096)	/* remember count of 4095 gives 4096 * counts */
         count = 4095;	/* count larger than 12 bits */
      else
         count -= 1;	/* new output board count of 0 equals 1 */
   }
   return(count);
}

/*
 *  Probe Protection
 */

static double totalE[] = {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 };
static double maxE[]   = {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 };
static double sumE[]   = {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 };
static double totalG[] = {0.0,  0.0,  0.0 };
static double sumG[]   = {0.0,  0.0,  0.0 };
/*
 *  The following four parameters represent an attempt to characterize the
 *  power handling capablilities of each channel of a specific probe.
 *  refpower is the calibration power level to deliver refwatt number of watts
 *  to the probe.  For example,  setting tpwr, or dpwr to 47 yields 2 watts
 *  at the probe.
 *  refdecay is a pseudo duty-cycle constant used in the damping equation
 *  for energy decay from the probe.
 *  refmax is the limit for the amount of energy each channel of the probe
 *  can handle.
 */
static double refpower[] = {0.0, 47.0, 47.0, 47.0, 47.0, 47.0, 47.0 };
static double refwatt[]  = {0.0,  2.0,  2.0,  2.0,  2.0,  2.0,  2.0 };
static double refdecay[] = {0.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0 };
static double refmax[]   = {0.0, 15.0, 15.0, 15.0, 15.0, 15.0, 15.0 };

static int do_power_check = TRUE;
static int  debug_power;	/* debug flag */
static int  debug_grad;		/* debug flag */
static int  do_grad_check, testxgrad, testygrad, testzgrad;
static double totalTime = 0.0;

/*
 *  Determine if the channel is gated on or off.
 */
static int pulseon(dev)
int dev;
{
   Msg_Set_Param param;
   Msg_Set_Result result;
   int HSmask;
   int error;
   char msge[128];

   param.setwhat=GET_XMTRGATE;
   error = Send(RF_Channel[dev],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
   if (error < 0)
   {
      sprintf(msge,"%s : %s\n",RF_Channel[dev]->objname,ObjError(error));
      text_error(msge);
   }
   HSmask = result.reqvalue;
   param.setwhat=GET_XMTRGATEBIT;
   error = Send(RF_Channel[dev],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
   if (error < 0)
   {
      sprintf(msge,"%s : %s\n",RF_Channel[dev]->objname,ObjError(error));
      text_error(msge);
   }
/*	
   if (debug_power)
      fprintf(stderr,"mask= 0x%x bit= 0x%x and= %d\n",
              HSmask,result.reqvalue,HSmask & result.reqvalue);
 */
   return((HSmask & result.reqvalue) == result.reqvalue);
}

/*
 *  Add energy into a probe channel.
 */
static double addE(dev,time,power,scl)
int dev;
double time;
int power;
double scl;
{
   double energy;

   energy = refwatt[dev] * exp10( (double) (power - refpower[dev]) / 10.0);
   energy *= time * scl;
   return(energy);
}

/*
 *  Dissipate energy from a probe channel.
 *  Where did the divisor of 20.0 come from?  I quessed.  My thinking was that
 *  refdecay could be a duty cycle, like 10 % or 20 %.  Therefore refdecay / 100
 *  seemed reasonable. Then I thought about a signal decaying to zero after
 *  5 T1 periods.  Therefore 5 * refdecay / 100 which is refdecay / 20.
 *  Maybe yes, maybe no.
 */
static double loseE(dev,time)
int dev;
double time;
{
   double loss;

   loss = totalE[dev] * ( 1.0 - exp10( -time / refdecay[dev] ) );
   return(loss);
}

static int testchan(dev,time)
int dev;
double time;
{
   int pwr_lvl = 0;
   double save_time = 0.0;
   int pulsed;
   int wavegen = 0;
   double scale_factor = 1.0;

   pulsed = pulseon(dev);
   if (!pulsed)
   {
      wavegen = waveon(dev,&scale_factor);
   }

   if (pulsed || wavegen)
   {
      Msg_Set_Param param;
      Msg_Set_Result result;
      int error;
      char msge[128];
      double add;
      double add_inc;
      double saveE;

      param.setwhat=GET_PWR;
      error = Send(RF_Channel[dev],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
      if (error < 0)
      {
         sprintf(msge,"%s : %s\n",RF_Channel[dev]->objname,ObjError(error));
         text_error(msge);
      }
      pwr_lvl = result.reqvalue;
      /*
       *  This may be overkill.  The amount of energy dissipated
       *  depends on the amount of energy present in the probe channel.
       *  During long events when the transmitter is on,  this allows
       *  some of the energy to be dissipated during the event.
       */
      save_time = time;
      saveE = totalE[dev];
      add = 0;
      while (time > 0.2)
      {
      
         totalE[dev] -= loseE(dev,0.2);
         add_inc = addE(dev,0.2,pwr_lvl,scale_factor);
         totalE[dev] += add_inc;
         sumE[dev] += add_inc;
         add += add_inc;
         time -= 0.2;
      }
      totalE[dev] -= loseE(dev,time);
      add_inc = addE(dev,time,pwr_lvl,scale_factor);
      totalE[dev] += add_inc;
      sumE[dev] += add_inc;
      add += add_inc;
      if (debug_power)
        fprintf(stdout," %1d   %2d    %1.6f    %1.6f    %1.6f   %1.6f ",
           dev,pwr_lvl,save_time,totalE[dev] - saveE - add,
            add,scale_factor);
   }
   else
   {
      if (debug_power)
         fprintf(stdout," %1d   off   %1.6f    %1.6f    none      %1.6f  ",
            dev,time,loseE(dev,time),scale_factor);
      totalE[dev] -=  loseE(dev,time);
   }
   if (totalE[dev] > maxE[dev])
   {
      maxE[dev] = totalE[dev];
   }
   if (totalE[dev] < 0.0)
      totalE[dev] = 0.0;
   if (maxE[dev] > refmax[dev])
   {
      /*
       *  This could be the PSG abort
       */
      if (debug_power)
         fprintf(stdout,"%1.6f   %1.6f   %1.6f    WARNING exceeds max %1.6f\n",
              totalE[dev], maxE[dev], sumE[dev], refmax[dev]);
      fprintf(stdout,"WARNING: power level of %d on channel %d for duration of %g sec. exceeds safety limit\n",pwr_lvl,dev,save_time);
      fprintf(stdout,"The 'nosafe' option to go, ga, or au will override this safety check\n");
      fprintf(stdout,"For example, go('nosafe')\n");
      psg_abort(1);
   }
   else
   {
      if (debug_power)
         fprintf(stdout,"%1.6f   %1.6f   %1.6f\n", totalE[dev], maxE[dev], sumE[dev]);
   }
}

/* MAXGRAD: maximum dac value for gradient is 32768. This yields
 * 10 amps.  The maximum duration for this pulse is 10 msec.
 * The duty cycle at this level is 2%. Therefore, if 
 */
/*
#define MAXGRAD (32767.0*0.010)
#define LOSSPERSEC (MAXGRAD*1000/(500 - 10))
*/

/*
 * Modified duty cycle calculation 18ix98 Michelle Pelta
 * Calculate duty cycle so that total sequence energy <= 40 Joules
 * WARNING:  this criterion is much less cautious than Varian's
 * original limits.  It _should_ be safe, but it's not guaranteed!
 */
#define MAXGRAD  40
#define LOSSPERSEC  2.5

static int testgrad(devchar,time,lvl)
int devchar;
double time,lvl;
{
   int pwr_lvl = lvl;
   int dev;
   double add_inc;

   dev = devchar - 'x';
   if (pwr_lvl)
   {
      if (pwr_lvl < 0)
         pwr_lvl = -pwr_lvl;
      add_inc = (pwr_lvl / 32768.0) *  (pwr_lvl / 32768.0) * 120.0 * time;
      totalG[dev] += add_inc;
      sumG[dev] += add_inc;
      if (debug_grad)
        fprintf(stdout," %c %5d     %1.6f    none    %1.6f    %1.6f   %1.6f\n",
           devchar,pwr_lvl,time, add_inc,totalG[dev],sumG[dev]);
   }
   else
   {
      add_inc = 0;
      totalG[dev] -=  time * LOSSPERSEC;
      if (totalG[dev] < 0.0)
         totalG[dev] = 0.0;
      if (debug_grad)
         fprintf(stdout," %c   off   %1.6f    %1.6f    none      %1.6f   %1.6f\n",
            devchar,time,time * LOSSPERSEC,totalG[dev],sumG[dev]);
   }
   if (totalG[dev] > MAXGRAD)
   {
      text_error("ERROR: Total gradient pulse energy exceeds safety limit (level of %d DAC units on gradient %c for duration of %g sec.)\n",
     pwr_lvl,devchar,time);
      text_error("Either reduce level of gradient to %d DAC units or reduce duration to %g sec.\n",
     (int) (MAXGRAD/time),(double) (MAXGRAD/(double)pwr_lvl));
      psg_abort(1);
   }
   else if (totalG[dev] > LOSSPERSEC)
   {
      text_error("ERROR: Average pulse power dissipation on gradient %c exceeds safety limit\n",
                      devchar);
      text_error("Maximum duty cycle is 2%% at full power.\n");
      psg_abort(1);
   }
}

/*
 * This procedure is called from delayer and acquire-  whenever a time
 * event is generated.
 */
checkpowerlevels(time)
double time;
{
   int i;
   extern double gradxval, gradyval, gradzval;

   totalTime += time;
   if (do_power_check)
   {
      if (debug_power)
        fprintf(stdout,
        "\ndev pwr      time        loss        gain        scale       total        max    sum\n");
      for (i=TODEV; i <= NUMch; i++)
         if (cattn[i] > 0.5)
            testchan(i,time);
   }
   if (do_grad_check)
   {
      if (debug_grad)
        fprintf(stdout,
        "\ngrad lvl      time        loss        gain        total      sum\n");
       if (testxgrad)
       {
          testgrad('x',time,gradxval);
       }
       if (testygrad)
       {
          testgrad('y',time,gradyval);
       }
       if (testzgrad)
       {
          testgrad('z',time,gradzval);
       }
   }
}

/*
 * This procedure is called from psg.c to initialize the probe scheme
 */

init_power_check()
{
    char   tmpstr[20];
    do_power_check = option_check("nosafe") ? FALSE : TRUE;
    if (P_getstring(GLOBAL,"probe_protection",tmpstr,1,12) < 0)
       tmpstr[0] == 'y';
    if (tmpstr[0] == 'n')
       do_power_check = FALSE;
    debug_power = option_check("debugprobe");
    debug_grad = option_check("debuggrad");
    if (P_getstring(GLOBAL,"gradtype",tmpstr,1,12) < 0)
       strcpy(tmpstr,"nnn");
    do_grad_check = testxgrad = testygrad = testzgrad = 0;
    if ((tmpstr[0] == 'u') || (tmpstr[0] == 't') ||
        (tmpstr[0] == 'p') || (tmpstr[0] == 'q'))
       do_grad_check = testxgrad = 1;
    if ((tmpstr[1] == 'u') || (tmpstr[1] == 't') ||
        (tmpstr[1] == 'p') || (tmpstr[1] == 'q'))
       do_grad_check = testygrad = 1;
    if ((tmpstr[2] == 'u') || (tmpstr[2] == 't') ||
        (tmpstr[2] == 'p') || (tmpstr[2] == 'q'))
       do_grad_check = testzgrad = 1;
    if (do_grad_check)
       do_grad_check = option_check("danger") ? FALSE : TRUE;
}

/*  This function is called at end of scan.  It is used to check
 *  gradient levels do not exceed 50% of MAX.  If they do, we
 *  simply add a delay to deal with it.
 */
endPowerCheck()
{
   int i;

   if (do_grad_check)
   {
      for (i=0; i<3; i++)
      {
         if (totalG[i] > ((MAXGRAD/2.0) + 1e-6))
         {
            delay( (totalG[i] - (MAXGRAD/2.0)) / LOSSPERSEC );
         }
         if (debug_grad)
            fprintf(stdout,
              "gradient %c duty cycle is %g. Must be less than 2%%\n",
              i+'x', sumG[i]/MAXGRAD/totalTime);
         if (sumG[i]/totalTime/MAXGRAD > 2.0)
         {
            delay( (sumG[i] / (MAXGRAD * 2.0)) - totalTime );
         }
      }
   }
}

/*
 * Temporary functions to turn gradient checking on and off
 * These should be removed in the next release
 */

gradCheckOn()
{
   do_grad_check = 1;
}
gradCheckOff()
{
   do_grad_check = 0;
}


/*
| check if the attenuator value exceeds the maximum value for the channel
*/

int safety_check(checkType, source, value, channel, attndevmsg)
char *checkType, *source, *attndevmsg ;
double value;
int channel;
{
  double maxattenvalue;
  char paramname[16];
  int result=0;     /* if 1 then safety_check is OK, if 0 safety_check failed */


  if (strcmp(checkType,"MAXATTEN") == 0 )
  {

    /* calls from setattn() pass a attndev objname which contains channel & Coarse/Fine atten info
       which has to be extracted from the attndevmsg string */
    if (channel == -1)             /* decode the attn device message */
    {
      char chanstr[8];

      if (attndevmsg[11] == 'F')   /* Fine attenuator, need not check */
      {
        return (1);
      }
      sprintf(chanstr,"%c",attndevmsg[8]);
      channel = atoi(chanstr);
    }

     sprintf(paramname,"maxattench%d",channel);
     if (P_getreal(GLOBAL,paramname,&maxattenvalue,1) < 0 )
     {
       /* not found */
        result=1;
     }
     else if (!var_active(paramname,GLOBAL))
     {
       /* found, but not active */
        result=1;
     }
     else
     {
       /* found and active */
       if (value <= maxattenvalue)
       {
         result=1;
       }
       else
       {
         char msge[128];
         result=0;
         sprintf(msge,"channel %d attenuation value %gdB exceeds %gdB safety limit (%s) \n",channel,value,maxattenvalue,paramname);
         text_error(msge);
       }
     }
  }
  return(result);
}


/*
| check if 5Mhz acquisition parameters are likely to result in a Fifo UnderFlow 
*/
static int warned_already=0;

check5MhzFoo()
{
    char dp[MAXSTR];
    double bufs,maxbufs,memuse;
    double cttime,ctcmplttime,buftime,npbuftime;
    double d0,d0n,diftim;
    int dpflag,d0flag;
    double arraydim;
    extern double interfidovhd;

    if (warned_already == 1) return;
    getstr("dp",dp);
    dpflag = (dp[0] == 'y') ? 4 : 2;	/* single - 2, double - 4 */
    if (P_getreal(CURRENT, "d0", &d0, 1) < 0)
    {
	/* if d0 not present then d0 = interfidovhd, and added twice because of real-time if */
        d0 = 2 * interfidovhd;	/* indicate not present by minus value */
	d0flag = 0;
    }
    else if (!var_active("d0",CURRENT))
    {
	/* if d0 not active then d0 = interfidovhd, and added twice because of real-time if */
        d0 = 2 * interfidovhd;
        d0flag = 1;
    }
    else
    {
      d0flag = 1;
    }

    if (getparm("arraydim","real",CURRENT,&arraydim,1))
	psg_abort(1);

    /* printf("check5MhzFoo: sw: %lf, np: %lf, bs: %lf d0: %lf, dpflag: %d\n",sw,np,bs,d0,dpflag); */
    /* printf("acquire time: %lf\n", (1/sw)*(np/2.0)); */
    /* printf("totaltime: %lf\n",totaltime); */
    bufs = (bs == 0) ? 1 : nt / bs;
    bufs *= arraydim;
    maxbufs = 2097152 / (np * 4);
    /* printf("bufs: %lf, maxbufs: %lf\n",bufs,maxbufs); */
    
    /* 1. remove pad */
    /* 2. remove 1st of two d0 delays in transient, (ifzero result in cps.c) */
    cttime = totaltime - pad - (d0-interfidovhd);  
    ctcmplttime = cttime * nt;
    buftime = ctcmplttime / bufs;
    npbuftime = ctcmplttime / (bufs * np * dpflag);

    /*printf("cttime: %lf, ctmcplttime: %lf, buftime: %lg, bytebuftime: %lg\n",
        cttime,ctcmplttime,buftime,npbuftime);
    */

    memuse = bufs * np * dpflag;
    /* printf("memory usage: %lf * %lf * %d = %lf bytes, max: %ld\n",bufs,np,dpflag,memuse,2097152); */
    
    if ( (memuse >= 2097152) && (npbuftime <= 1.02271E-6))
    {
	warned_already = 1;
        diftim = 1.02271E-6 - npbuftime;
        d0n = (diftim * (bufs * np * dpflag)) / nt;
	/*
        printf("(diftim * (bufs * np * dpflag)) / nt = d0n\n");
        printf("(%lg * (%lf * %lf * %d)) / %lf = %lg\n",diftim,bufs,np,dpflag,nt,d0n);
        printf("d0: %lf, d0n: %lf\n",d0,d0n);
        */
        text_error("WARNING: sw, np, d1, bs parameter settings are likey to result in a Fifo Underflow\n");
        text_error("         Options: reduce sw or np parameters\n");
        text_error("                  increase d1 or bs parameters\n");
        if (d0flag == 0) 
        {
          text_error("                  Create('d0') and Set 'd0' to approx. %lf\n",d0n);
				
        }
        else
        {
          text_error("                  Set 'd0' to approx. %lf\n",d0 + d0n);
        }
    }
}
#endif 
/*  lcsample()
 *  set triggers for injecting an LC sample.
 *  no longer used in vnmrs or inova.  -bfetler 5/25/07
 */
lcsample()
{
}

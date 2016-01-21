/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "variables.h"
#include "oopc.h"
#include "acqparms.h"
#include "rfconst.h"
#include "group.h"
#include "macros.h"
#include "pvars.h"
#include "decfuncs.h"
#include "delay.h"
#include "abort.h"
#include "rcvrfuncs.h"

#define MRBUGBIT 2
#define MAXNUMRCVRS 4

extern int bugmask;
extern int  acqiflag;		/* True if acodes are for Acqi */
extern int  bgflag;		/* debug flag */
extern int  NUMch;		/* number of channels in the system */
extern int  OBSch;		/* designates Observe Channel */
extern int  rcvroff_flag;	/* global receiver flag */
extern int  rcvr_hs_bit;	/* old =0x8000, new = 0x1 */
extern int  ap_interface;
extern int  newacq;

extern int okinhwloop();
extern void HSgate(int ch, int state);
extern int SetRFChanAttr(Object obj, ...);
extern int S_getarray (const char *parname, double array[], int arraysize);
extern int get_filter_max_bandwidth();

int rcvr2nt = 0;
static long  blank_is_on = 0xffffffff;
		/*
		   bit pattern to show channel is blanked off 
		   must be initialized for all receiver lines to be on
		*/

/*------------------------------------------------------------------
|	rcvron()
|	 turn on receiver  
+-----------------------------------------------------------------*/
void rcvron()
{
    okinhwloop();
    if (ap_interface < 4)
    {
       HSgate(rcvr_hs_bit,FALSE);
       rcvroff_flag = 0;
    }
    else
    {
       if (newacq)
       {
	   HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
       }
       rcvroff_flag = 0;
       blankon(OBSch);
    }
}
/*------------------------------------------------------------------
|	rcvroff()
|	 turn off receiver  
+-----------------------------------------------------------------*/
void rcvroff()
{
    okinhwloop();
    if (ap_interface < 4)
    {
       HSgate(rcvr_hs_bit,TRUE);
       rcvroff_flag = 1;
    }
    else
    {
       if (newacq)
       {
	   HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
       }
       rcvroff_flag = 1;
       blankoff(OBSch);
    }
}

/*------------------------------------------------------------------
|	recon()
|	 turn on only receiver  for inovar
+-----------------------------------------------------------------*/
void recon()
{
    if (newacq)
    {
	HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
	rcvroff_flag = 0;
    }
 }

/*------------------------------------------------------------------
|	recoff()
|	 turn off only receiver for inova
+-----------------------------------------------------------------*/
void recoff()
{
    if (newacq)
    {
	HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
	rcvroff_flag = 1;
    }
}

void blankon(int device)
{
char	msge[80];
   if ( (device<0) || (device>NUMch) )
   {  sprintf(msge,"blankon(): device #%d is not within bounds of 1 - %d\n",
		device, NUMch);
      abort_message(msge);
   }
   if (ap_interface!=4)
   {  
      if ((device == DECch) && (cattn[device] == SIS_UNITY_ATTN_MAX)  
						&& (ap_interface < 3))
      {
	decblankon();
	blank_is_on |= (1<<device);
      }
      else
      {
 	fprintf(stderr,"blankon() is not valid with this hardware configuration\nblankon() call is ignored\n");
      }
      return;
   }
   SetRFChanAttr(RF_Channel[device], SET_RCVRGATE, ON, 0);
   blank_is_on |= (1<<device);
}

void blankoff(int device)
{
char	msge[80];
   if ( (device<0) || (device>NUMch) )
   {  sprintf(msge,"blankoff(): device #%d is not within bounds of 1 - %d\n",
		device, NUMch);
      abort_message(msge);
   }
   if (ap_interface!=4)
   {  
      if ((device == DECch) && (cattn[device] == SIS_UNITY_ATTN_MAX)  
						&& (ap_interface < 3))
      {
	decblankoff();
	blank_is_on &= ~(1<<device);
      }
      else
      {
	fprintf(stderr,"blankoff() is not valid with this hardware configuration\nblankoff() call is ignored\n");
      }
      return;
   }
   SetRFChanAttr(RF_Channel[device], SET_RCVRGATE, OFF, 0);
   blank_is_on &= ~(1<<device);
}

int isBlankOn(int device)
{
   return( (blank_is_on & (1<<device)) ? 1 : 0 );
}


int parmToRcvrMask(char *parName)
{
    /*
     * This version assumes the rcvrs parameter is a string of "y"s and "n"s
     * specifying whether each receiver is on or off.
     * Returned mask is guaranteed to be non-zero.
     */
    char rcvrStr[MAXSTR];
    int nRcvrs;
    static int rtnMask = 0;	/* Make this persistent for speed */

    if (!rtnMask) {
	double tmp;
	if (P_getreal(GLOBAL, "numrcvrs", &tmp, 1) < 0){
	    nRcvrs = 1;
	} else {
	    nRcvrs = (int)tmp;
	}

	if ( P_getstring(CURRENT, parName, rcvrStr, 1, MAXSTR)) {
	    rtnMask = 1;
	} else {
	    int c;
	    int n;
	    char *pc = rcvrStr;
	    for (n=0; (c=*pc) && (n<nRcvrs); pc++, n++) {
		if (c == 'y' || c == 'Y') {
		    rtnMask |= 1 << n;
		    if (acqiflag) {
			break;	/* Only allow one rcvr if Acqi */
		    }
		}
	    }
	}
	if (!rtnMask) {
	    rtnMask = 1;	/* Don't allow all rcvrs to be off! */
	}
        if (rtnMask == 3)       /* First 2 receivers are used */
        {
	   if (P_getreal(CURRENT, "nt2", &tmp, 1) == 0)
           {
              rcvr2nt = (int) tmp;
              if (rcvr2nt < 0)
                 rcvr2nt = 0;
              if (rcvr2nt)
              {
                 if ( (int) nt % (rcvr2nt + 1) )
                 {
	           text_error("nt must be a multiple of (nt2 + 1) (%d)\n",rcvr2nt + 1);
	           psg_abort(1);
                 }
                 if ((bs < nt) && ( (int) bs % (rcvr2nt + 1) ) )
                 {
	           text_error("bs must be a multiple of (nt2 + 1) (%d)\n",rcvr2nt + 1);
	           psg_abort(1);
                 }
              }
           }
        }
    }
    return rtnMask;
}

int isRcvrActive(int n)
{
    int rtn;

    rtn = (parmToRcvrMask("rcvrs") >> n) & 1;
    return rtn;
}

int getFirstActiveRcvr()
{
    int mask;
    int i;

    for (i=0, mask=parmToRcvrMask("rcvrs"); (mask & 1) == 0; mask >>= 1, i++);
    return i;
}

/* PIC adddition, 9/13/02, to set ARRAY base on number of active receivers */
int numOfActiveRcvr()
{
    int num,maxRcvrs;
    int i;

    maxRcvrs = 8; /* actually 4 but never know the future */
    for (i=0, num = 0; i < maxRcvrs; i++)
    {
        if ( isRcvrActive(i) == 1)
	   num = num + 1;
    }
    return num;
}

/*
 * Translate gain value to receiver gain bits.
 * Copied from "console" s/w.
 */
static int
gain2bits(double dgain, int *preampattn)
{
    int rset;
    int gain;

    gain = (int)(dgain + 0.5);
    if (gain >= 18)  {
	*preampattn=0;
	gain = gain - 18;
    } else if (gain >= 12)  {
	*preampattn = 2;
	gain = gain - 12;
    } else if (gain >= 6)  {
	*preampattn = 1;
	gain = gain - 6;
    } else {
	*preampattn = 3;
    }

    rset = 0x3e;
    if (gain >= 14) {
	rset = rset & 0x3d;	/* set 14 dB input */
	gain -= 14;
    }
    if (gain >= 14) {
	rset = rset & 0x3b;	/* set 14 dB output */
	gain -= 14;
    }
    if (gain >= 8) {
	rset = rset & 0x37;	/* set 8 dB  */
	gain -= 8;
    }
    if (gain >= 4) {
	rset = rset & 0x2f;	/* set 4 dB output */
	gain -= 4;
    }
    if (gain >= 2) {
	rset = rset & 0x1f;	/* set 2 dB output */
	gain -= 2;
    }
    return rset;
}

/*
 * Returns 1 if "pname" is a parameter in the CURRENT tree with active status,
 * otherwise returns 0.
 */
static int
isactive(char *pname)
{
    vInfo varinfo;
    if (P_getVarInfo(CURRENT, pname, &varinfo) || varinfo.active != ACT_ON) {
	return 0;
    }
    return 1;
}

double getGain(int ircvr)
{
    double mrgain[MAXNUMRCVRS];
    double rtn;
    int nr;
    /* NB: "gain" is global variable */

    nr = isactive("mrgain") ? getarray("mrgain", mrgain) : 0;
    rtn = nr > ircvr ? mrgain[ircvr] : gain;
    return rint(rtn);
}

int getGainBits(int ircvr)
{
    int preampbits; /* dummy */
    return gain2bits(getGain(ircvr), &preampbits);
}

/*
 * Translate filter bandwidth to receiver filter bits.
 */
static int
fb2bits(double fb)
{
    double fbmax;
    double fbstep;
    int rtn;

    fbmax = get_filter_max_bandwidth();
    fbstep = fbmax / 256;
    rtn = (int)(((fb / fbstep) - 1) + 0.5);
    return rtn;
}

/*
 * Kluge to set Multiple Receiver gains.
 * Does not set the preamp attenuation in the "magnet leg";
 * rely on the normal gain setting to set same attenuation for all.
 */
void setMRGains()
{
    /*
     * If not a Mult Rcvr system, do nothing
     * Else
     *    If mult rcvr active and "gain" inactive,
     *       Error (No Mult Rcvr autogain)
     *    Else if "mrgain" active,
     *       Set individual gains specified in "mrgain".
     *    Set remaining gains to value of "gain".
     */
    int i;
    int igain;
    int nr;
    int numrcvrs;
    int rmask;
    int gainactive;
    int preampAttn;
    int stdPreampAttn;
    double tmp;
    double mrgain[MAXNUMRCVRS];
    static int gainaddr[MAXNUMRCVRS] = {0xb42, 0xb43, 0xb46, 0xb47};

    if (P_getreal(GLOBAL, "numrcvrs", &tmp, 1) < 0){
	numrcvrs = 1;
    } else {
	numrcvrs = (int)tmp;
    }
    if (numrcvrs <= 1) {
	return;
    }

    if (numrcvrs > MAXNUMRCVRS)
	numrcvrs = MAXNUMRCVRS;
    rmask = parmToRcvrMask("rcvrs");
    gainactive = isactive("gain");
    if (rmask != 1 && !gainactive) {
	text_error("No autogain allowed with multiple receivers active.\n");
	text_error("Set the \"gain\" parameter.");
	psg_abort(1);
    }

    gain2bits(getGain(getFirstActiveRcvr()), &stdPreampAttn);
    nr = isactive("mrgain") ? getarray("mrgain", mrgain) : 0;
    /* Don't set first 2 rcvrs.  They should already get set:
     * first rcvr through init_auto_pars(), and second through
     * SetAPAttr(RCVR_homo, SET_MASK, ...).
     * Setting the second one here would clobber the RCVR_homo bit.
     */
    for (i=0; i<numrcvrs; i++) {
	if (i >= nr) {
	    mrgain[i] = gain;
	}
	igain = gain2bits(mrgain[i], &preampAttn);
	if (isRcvrActive(i) && preampAttn != stdPreampAttn) {
	    text_error("All active rcvrs must have gain in same regions.");
	    text_error("Regions: 0 to 5, 6 to 11, 12 to 17, 18 or greater.\n");
	    psg_abort(1);
	}
	if (i >= 2) {
	    apsetbyte(igain, gainaddr[i]);
	    if (bugmask & MRBUGBIT) {
		fprintf(stderr,"gain[%d]=%.0f => apsetbyte 0x%x at 0x%x\n",
			i, mrgain[i], igain, gainaddr[i]);
	    }
	}
    }
}

/*
 * Kluge to set Multiple Receiver filter bandwidths.
 */
void setMRFilters()
{
    /*
     * If not a Mult Rcvr system, do nothing
     * Else
     *    If "mrfb" active,
     *       Set individual filter bandwidths specified in "mrfb".
     *    Set remaining bandwidths to value of "fb".
     */
    int i;
    int ifb;
    int nr;
    int numrcvrs;
    double tmp;
    double mrfb[MAXNUMRCVRS];
    static int fbaddr[MAXNUMRCVRS] = {0xb40, 0xb41, 0xb44, 0xb45};

    if (P_getreal(GLOBAL, "numrcvrs", &tmp, 1) < 0){
	numrcvrs = 1;
    } else {
	numrcvrs = (int)tmp;
    }
    if (numrcvrs <= 1) {
	return;
    }

    if (numrcvrs > MAXNUMRCVRS)
	numrcvrs = MAXNUMRCVRS;
    nr = isactive("mrfb") ? getarray("mrfb", mrfb) : 0;
    for (i=0; i<numrcvrs; i++) {
	if (i >= nr) {
	    mrfb[i] = fb;
	}
	ifb = fb2bits(mrfb[i]);
	if (bugmask & MRBUGBIT) {
	    fprintf(stderr,"fb[%d]=%.0f => apsetbyte 0x%x at 0x%x\n",
		    i, mrfb[i], ifb, fbaddr[i]);
	}
	apsetbyte(ifb, fbaddr[i]);
    }
}

void startacq(double ldelay)
{
   rcvron();
   delay(ldelay);
}

void endacq()
{
   rcvroff();
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "device.p"

/* RF Channel local Properities  */
int newxmtr;		/* rf type 'c', equiv to newtrans, newdec */
int newamp;		/* amptype 'a', equiv to newtranamp, newdecamp */
int *ap_ovrride; 	/* address of ap_ovrride flag */
int phasetbl[4];	/* quadrent phase numbers */
int phasebits;		/* quadrent phase bits */
int sphaseaddr;		/* small angle phase shifter 6-trans, 9-dec */
int sphasereg;		/* small angle phase register */
int sphasemode;		/* small angle phase type */
int phasestep;		/* small angle phase step */
int xmtrgateHSbit;	/* HSline bit to gate xmtr with */
int rcvrgateHSbit;	/* HSline bit to gate rcvr with */
int wg_gateHSbit;	/* HSline bit to gate WG with */
int *HSlineptr;		/* pointer to Global High Speed Lines */
int amphibandmin;	/* amplifier high band min freq MHz */
int xmtr2ampbit;	/* OBS_RELAY, DEC_RELAY, DEC2_RELAY */
int xmtrx2bit;		/* OBS_XMTR_HI_LOW_BAND,DEC_..,DEC2_.. */
Object FreqObj;		/* Frequncy Object for this channel (e.g., ObsTrans)*/
Object AttnObj;		/* Attenuator Object */
Object LnAttnObj;	/* Fine Linear Attenuator Object */
Object ModulObj;	/* Modulation mode byte for each channel */
Object HLBrelayObj;	/* Amp. HI/LOW Band Relay Object */
Object XmtrX2Obj;	/* Xmtr X2  Object */
Object WFG_Obj;		/* Wave Form Generator Object */
Object MixerObj;	/* XMTRs hi/lo mixer select */
Object HSObj;		/* XMTRs HS line select */
Object GateObj;		/* XMTR gate mode,			    */
			/* idle, liquids, solids, hetero, wfg, homo */

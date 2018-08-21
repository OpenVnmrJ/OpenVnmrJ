/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "chanstruct.h"
#include "oopc.h"
#include "rfconst.h"
#include "acqparms.h"
/*---------------------------------------------------------------------------
 PTS  	     	     adr 	reg 	bytes 	mode 	max 	min 	offset
 chan 1 pts	      7		7	5
 chan 2 pts	      7		15	5
 chan 3 pts	      7		23	5

 Coarse Attn 	     adr 	reg 	bytes 	mode 	max 	min 	offset
 chan 1 cattn,ap<2=   5  	0  	1 	-1 	63 	0 	0 
 chan 2 cattn,ap<2=   5  	4  	1 	-1 	63 	0 	0 
 chan 1 cattn,ap=>2   5  	12  	1 	-1 	63 	0 	63 
 chan 2 cattn,ap=>2   5  	16  	1 	-1 	63 	0 	63 
 chan 3 cattn,ap=>2   5  	15  	1 	-1 	63 	0 	63 

 Fine Linear Attn    adr 	reg 	bytes 	mode 	max 	min 	offset
 chan 1 fattn,ap=>2   5  	22  	2 	1 	4095 	0 	0 
 chan 2 fattn,ap=>2   5  	20  	2 	1 	4095 	0 	0 
+---------------------------------------------------------------------------*/

chanconst rfchanconst[] = {
	{ 0 },  /* blank [0] values so rfchan can be used as index directly */
	{ 7, 7, 5,
	  OBS_ATTN_AP_ADR,	OBS_ATTN_AP_REG500,	OBS_ATTN_AP_REG,
	  OBS_ATTN_BYTES,	OBS_ATTN_MODE,		OBS_ATTN_MAXVAL,
	  OBS_ATTN_MINVAL,	OBS_ATTN_OFSET500,	OBS_ATTN_OFSET,
	  LN_ATTN_AP_ADR,	TX_LN_ATTN_AP_REG,	LN_ATTN_BYTES,
	  OBS_ATTN_MODE,	LN_ATTN_MAXVAL,		LN_ATTN_MINVAL,
	  LN_ATTN_OFFSET,
	  OBS_RELAY,		OBS_XMTR_HI_LOW_BAND,	0,
	  RFP90,		RFP180,			RFP270, RFP270,
	  OBS_SMPHASE_APADR,	TXON,			WFG1
	},	/* Observe Channel */
	{ 7, 15, 5,
	  DEC_ATTN_AP_ADR,	DEC_ATTN_AP_REG500,	DEC_ATTN_AP_REG,
	  DEC_ATTN_BYTES,	DEC_ATTN_MODE,		DEC_ATTN_MAXVAL,
	  DEC_ATTN_MINVAL,	DEC_ATTN_OFSET500,	DEC_ATTN_OFSET,
	  LN_ATTN_AP_ADR,	DC_LN_ATTN_AP_REG,	LN_ATTN_BYTES,
	  OBS_ATTN_MODE,	LN_ATTN_MAXVAL,		LN_ATTN_MINVAL,
	  LN_ATTN_OFFSET,
	  DEC_RELAY,		DEC_XMTR_HI_LOW_BAND,	0,
	  DC90,			DC180,			DC270, DC270, 
	  DEC_SMPHASE_APADR,	DECUPLR,		WFG2
	},	/* Decoupler Channel */
	{ 7, 23, 5,  
	  DEC2_ATTN_AP_ADR,	DEC2_ATTN_AP_REG,	DEC2_ATTN_AP_REG,
	  DEC2_ATTN_BYTES,	DEC2_ATTN_MODE,		DEC2_ATTN_MAXVAL,
	  DEC2_ATTN_MINVAL,	DEC2_ATTN_OFSET,	DEC2_ATTN_OFSET,
	  0,			0,			LN_ATTN_BYTES,
	  0,			LN_ATTN_MAXVAL,		LN_ATTN_MINVAL,
	  LN_ATTN_OFFSET,
	  DEC2_RELAY,		DEC2_XMTR_HI_LOW_BAND,	0,
	  DC2_90,		DC2_180,		DC2_270, DC2_270, 
	  DEC2_SMPHASE_APADR,	DECUPLR2,		WFG3
	},	/* Second Decoupler Channel */
	{ 7, 20, 5,  
	  0,			0,			0,
	  DEC3_ATTN_BYTES,	0,			DEC3_ATTN_MAXVAL,
	  DEC3_ATTN_MINVAL,	DEC3_ATTN_OFSET,	DEC3_ATTN_OFSET,
	  0,			0,			LN_ATTN_BYTES,
	  0,			LN_ATTN_MAXVAL,		LN_ATTN_MINVAL,
	  LN_ATTN_OFFSET,
	  0,			0,			0,
	  0,			0,			0, 0, 
	  0,			0,			0
	},	/* Third Decoupler Channel */
	{ 7, 20, 5,  
	  0,			0,			0,
	  DEC3_ATTN_BYTES,	0,			DEC3_ATTN_MAXVAL,
	  DEC3_ATTN_MINVAL,	DEC3_ATTN_OFSET,	DEC3_ATTN_OFSET,
	  0,			0,			LN_ATTN_BYTES,
	  0,			LN_ATTN_MAXVAL,		LN_ATTN_MINVAL,
	  LN_ATTN_OFFSET,
	  0,			0,			0,
	  0,			0,			0, 0, 
	  0,			0,			0
	}	/* Fourth Decoupler Channel */
};

int RFCHCONSTsize = (sizeof(rfchanconst) / sizeof(struct _chanconst)) - 1;/* 0 notused */

static char tnname[] = "tn";
static char dnname[] = "dn";
static char dn2name[] = "dn2";
static char dn3name[] = "dn3";
static char dn4name[] = "dn4";
static char sfrqname[] = "sfrq";
static char dfrqname[] = "dfrq";
static char dfrq2name[] = "dfrq2";
static char dfrq3name[] = "dfrq3";
static char dfrq4name[] = "dfrq4";
static char tofname[] = "tof";
static char dofname[] = "dof";
static char dof2name[] = "dof2";
static char dof3name[] = "dof3";
static char dof4name[] = "dof4";

rfvarinfo RFInfo[] = {
   { 0 },  /* blank [0] values so rfchan can be used as index directly */
   { &sfrq, &tof, &tpwr, &tpwrf, tofname },	 /* Observe Channel */
   { &dfrq, &dof, &dpwr, &dpwrf, dofname },	 /* Decoupler Channel */
   { &dfrq2, &dof2, &dpwr2, &dpwrf2, dof2name }, /* Second Decoupler Channel */
   { &dfrq3, &dof3, &dpwr3, &dpwrf3, dof3name }, /* Third Decoupler Channel */
   { &dfrq4, &dof4, &dpwr4, &dpwrf4, dof4name }	 /* Fourth Decoupler Channel */
};


static void setRFInfoch(chan,frq,frqoffset,pwr,pwrf,frqoffsetname)
int chan;
double	*frq;
double	*frqoffset;
double	*pwr;
double	*pwrf;
char	*frqoffsetname;
{
   RFInfo[chan].rfsfrq = frq;
   RFInfo[chan].rftof = frqoffset;
   RFInfo[chan].rftpwr = pwr;
   RFInfo[chan].rftpwrf = pwrf;
   RFInfo[chan].rftofname = frqoffsetname;
}

void setRFInfo(hwchan,chan)
int hwchan;
int chan;
{
   switch (hwchan)
   {
      case TODEV:  
            setRFInfoch(chan, &sfrq, &tof, &tpwr, &tpwrf, tofname);
            break;
      case DODEV:  
            setRFInfoch(chan, &dfrq, &dof, &dpwr, &dpwrf, dofname);
            break;
      case DO2DEV:  
            setRFInfoch(chan, &dfrq2, &dof2, &dpwr2, &dpwrf2, dof2name);
            break;
      case DO3DEV:  
            setRFInfoch(chan, &dfrq3, &dof3, &dpwr3, &dpwrf3, dof3name);
            break;
      case DO4DEV:  
            setRFInfoch(chan, &dfrq4, &dof4, &dpwr4, &dpwrf4, dof4name);
            break;
      default:
            break;
   }
}

char *getoffsetname(chan)
int chan;
{
   return(RFInfo[chan].rftofname);
}

#define FREQ 1
#define NUC 2

char *getfreqname(chan)
{
   char *getname();
   return (getname(chan,FREQ));
}

char *getnucname(chan)
{
   char *getname();
   return (getname(chan,NUC));
}

char *getname(chan,nametype)
int chan;
int nametype;
{
   char *ptr;

   if ( strcmp(RFInfo[chan].rftofname,"tof") == 0 )
   {
     switch(nametype)
     {
        case FREQ:
		ptr = sfrqname;
		break;
	case NUC:
		ptr = tnname;
		break;
	default:
		ptr = NULL; 
		break;
     }
   }
   else if ( strcmp(RFInfo[chan].rftofname,"dof") == 0 )
   {
     switch(nametype)
     {
        case FREQ:
		ptr = dfrqname;
		break;
	case NUC:
		ptr = dnname;
		break;
	default:
		ptr = NULL; 
		break;
     }
   }
   else if ( strcmp(RFInfo[chan].rftofname,"dof2") == 0 )
   {
     switch(nametype)
     {
        case FREQ:
		ptr = dfrq2name;
		break;
	case NUC:
		ptr = dn2name;
		break;
	default:
		ptr = NULL; 
		break;
     }
   }
   else if ( strcmp(RFInfo[chan].rftofname,"dof3") == 0 )
   {
     switch(nametype)
     {
        case FREQ:
		ptr = dfrq3name;
		break;
	case NUC:
		ptr = dn3name;
		break;
	default:
		ptr = NULL; 
		break;
     }
   }
   else if ( strcmp(RFInfo[chan].rftofname,"dof4") == 0 )
   {
     switch(nametype)
     {
        case FREQ:
		ptr = dfrq4name;
		break;
	case NUC:
		ptr = dn4name;
		break;
	default:
		ptr = NULL; 
		break;
     }
   }
   else
   {
      	ptr = NULL;
   }

   return(ptr);
}

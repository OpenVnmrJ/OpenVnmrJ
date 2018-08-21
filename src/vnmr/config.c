/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------
|	config.c
|
|       05/19/87
|
|       Provides display of current configuration, and allows update
|	if current situation permits.
|
|	06/01/87
|
|	All parameters are displayed in non-interactive mode and this
|	mode can be forced with the argument "display".  Entries "tests"
|	and "stdpar" are created when the interactive mode exits, based
|	on proton frequency.
|
|	09/03/87
|
|	Added provision for configuring a Wideline system by changing
|	the "parmax" entry for maximum spectral window and maximum dlp
|
|	05/26/88
|
|	Added fifo size and audio filter entries.
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   1/3/89     Greg B.    1. Added Rotor Sync Selection
|                         2. Added Type 3 to AP Interface Type
|   3/1/89     Greg B.    1. Added pulsestepvals & delaystepvals 
|			     so that a change in the fifo type
|			     changes the step value of pulse & delay
|			     parameters.
|   5/22/89    Greg B.    1. Fixed access() call in  makelink() to pass required
|			     second argument.
|
|---------------------------------------------------------------------*/

#include  "vnmrsys.h"
#include  <stdio.h>
#include  <ctype.h>
#ifdef UNIX
#include  <sys/file.h>
#else
#define  R_OK	4
#endif
#include  "group.h"

#ifdef SUN
#include  <suntool/sunview.h>
#include  <suntool/panel.h>

/*  The granddaddy frame, created when VNMR starts.  */

extern Frame    Wframe;
#endif SUN

extern int      menuon;

static char     conparfil[MAXPATHL];
static int      makestdparlink;
static int      maketestslink;

/*  While the operator is configuring the system, selections are
    stored as indices into various tables, defined below.	*/

static int      systemindex;
static int      protonindex;
static int      trayindex;
static int      z5index;
static int      butterworth;
static int      vtindex;
static int      dmfindex;
static int      numsyn;		/* Number of synthesizers is an exception */
static int      fifoindex;
static int      pulseindex;
static int      delayindex;
static int      apinterface;
static int      rsyncindex;

#ifdef SUN
/*  SunView structures created by this command.  */

static Frame    configFrame;
static Panel    exitPanel,
                choicePanel;
static Panel_item exitButton,
                quitButton;

/*  The array of panels defined below control the various options available  */

#define  NUMPANELS	20
#define  SYSPANEL	0
#define  H1PANEL	1
#define  XMPANEL	2
#define  DCPANEL	3
#define  SYNPANEL	4
#define  PTS1PANEL	5
#define  PTS2PANEL	6
#define  XAMPPANEL	7
#define  DAMPPANEL	8
#define  TRAYPANEL	9
#define  Z5PANEL	10
#define  AFPANEL	11
#define  VTPANEL	12
#define  DMFPANEL	13
#define  TOFPANEL	14
#define  DOFPANEL	15
#define  SWPANEL	16
#define  APINFPANEL	17
#define  FIFOPANEL	18
#define  ROTORSYNCPANEL	19

static Panel_item paramPanels[NUMPANELS];
static int      showingPTS2 = 0;
#endif SUN

static struct
{
   int             rfindex;
   int             ptsindex;
   int             ampindex;
}               xmitter, decoupler;

#define  NDMFVALS	2

static int      dmfval[NDMFVALS] = {9900, 32700};

#define  NVTTYPES	2

static char    *vtlabel[NVTTYPES] = {
   " Not used ",
   " Present  "
};

#define  NTRAYSIZES	3

static int      traysize[NTRAYSIZES] = {0, 50, 100};

#define  NSYSTYPES	2

static struct
{
   char           *systype;
   char           *syslabel;
}               systable[NSYSTYPES] =

{
   {
      "VXR", " Spectrometer "
   },
   {
      "VXRD", " Data Station "
   }
};

static char    *z5label[] = {
   " Not available ",
   " Available  ",
};

static char    *filterlabel[] = {
   " Elliptical ",
   " Butterworth ",
};

#define  NPTSTYPES	 4

static char    *ptslabel[] = {
   " PTS 160 ",
   " PTS 200 ",
   " PTS 250 ",
   " PTS 500 "
};
static int      ptsvals[NPTSTYPES] = {160, 200, 250, 500};

#ifdef SIS
#define  NRFTYPES	 5
#else
#define  NRFTYPES	 4
#endif

#define  BBANDFIXEDH1	'd'

static struct
{
   char            rftype;
   char           *rflabel;
}               rftable[NRFTYPES] =

{
   {
      'a', " Fixed Frequency      "
   },
   {
      'b', " Broadband            "
   },
   {
      'c', " Direct Synthesis     "
   },
   {
      BBANDFIXEDH1, " Broadband + Fixed H1 "
   }

#ifdef SIS
   ,
   {
      'm', " Modulator"
   }				/* VIS only */
#endif
};

#define NAMPTYPES	2

static struct
{
   char            amptype;
   char           *amplabel;
}               amptable[NAMPTYPES] =

{
   {
      'c', " Class C "
   },
   {
      'a', " Linear  "
   }
};

#define NAPINFVALS 	 3
static int      apinfvals[NAPINFVALS] = {1, 2, 3};
static int      apinfflag = 0;

#define NFIFOVALS 	 2
static int      fifovals[NFIFOVALS] = {63, 1024};

#define NPULSEVALS	NFIFOVALS
static double   pulsestepvals[NPULSEVALS] = {0.1, 0.025};

#define NDELAYVALS	NFIFOVALS
static double   delaystepvals[NDELAYVALS] = {1.0e-7, 0.25e-7};

#define  NRSYNCVALS	2
static char    *rsynclabel[NRSYNCVALS] = {
   " Not Present ",
   " Present  "
};

#ifdef SIS
#define  NH1FREQS	 6
static int      h1vals[NH1FREQS] = {200, 300, 400, 500, 600, 85};

#else
#define  NH1FREQS	 5
static int      h1vals[NH1FREQS] = {200, 300, 400, 500, 600};

#endif

/*  The transmitter offset and decoupler offset can be incremented
    (in hardware) by either 0.1 or 100 Hz.  The operator thus need
    to control the step values for TOF and DOF, obtained from
    PARSTEP.  The variables "tofstep" and "dofstep" contain the
    index into the array "parstepvals".					*/

#define  TOFINDEX	 7	/* Index into PARSTEP array */
#define  DOFINDEX	 8	/* Index into PARSTEP array */
#define  DMFINDEX	 11	/* Index into PARSTEP array */

static int      tofstep;
static int      dofstep;

#ifdef SIS
static double   parstepvals[3] = {0.1, 1.0, 100.0};

#else
static double   parstepvals[2] = {0.1, 100.0};

#endif

/*  The maxmimum spectral window is either 100KHz (high resolution)
    or 2MHz (wideline solids).  This is reflected in the PARMAX
    table entry for "sw".  When this maximum spectral window value
    is changed, the maximum value for "dlp" must be changed also,
    39 for high-resolution; 63 for wideline solids.			*/

#define  SWINDEX	5
#define  DHPINDEX	9
#define  DLPINDEX	10

#define  NDHPVALS	3
#define  DHPCHNGFREQ	495

#define  PULSEINDEX	13
#define  DELAYINDEX	14
#define  LPULSEINDEX	15

static int      dhpval[NDHPVALS] = {49, 63, 255};

static int      maxswindex;
static double   dlpmaxvals[2] = {39.0, 63.0};
static double   swmaxvals[2] = {1.0e5, 2.0e6};

/*  These routines return the index into the indicated parameter
    data structure(s) given a possible value, or -1 if that value
    is not found.

    Important note:  String parameters at this time should have
    at most 40 characters.  Excess characters will be ignored.	*/

/*  Unlike the other routines, "verifystep" accepts any value and
    trys to guess what the actual configuration is.		*/

static int 
verifystep(sval)
double          sval;
{
   if (sval < 0.0)
      sval = -sval;
   if (sval < 1.0)
      return (0);

#ifndef SIS
   else
      return (1);
#else
   if (sval >= 100.0)
      return (2);
   return (1);
#endif
}

static int 
verifymaxsw(maxsw)
double          maxsw;
{
   if (maxsw > 2.0e5)
      return (1);		/* Wide-line */
   else
      return (0);		/* High-res */
}

static int 
verifyrf(rfchar)
char            rfchar;
{
   int             iter;

   for (iter = 0; iter < NRFTYPES; iter++)
      if (rfchar == rftable[iter].rftype)
	 return (iter);

   return (-1);
}

static int 
verifyamp(ampchar)
char            ampchar;
{
   int             iter;

   for (iter = 0; iter < NAMPTYPES; iter++)
      if (ampchar == amptable[iter].amptype)
	 return (iter);

   return (-1);
}

static int 
verifytray(ival)
int             ival;
{
   int             iter;

   for (iter = 0; iter < NTRAYSIZES; iter++)
      if (ival == traysize[iter])
	 return (iter);

   return (-1);
}

static int 
verifyPTS(ival)
int             ival;
{
   int             iter;

   for (iter = 0; iter < NPTSTYPES; iter++)
      if (ival == ptsvals[iter])
	 return (iter);

   return (-1);
}

/*  Changed so as to always return a valid index.  If more
    values are ever permitted for DMFMAX, this routine will
    have to be changed.  Currently it assumes only two valid
    values with the 1st smaller than the 2nd.			*/

static int 
verifyDMF(ival)
int             ival;
{
   if (ival <= dmfval[0])
      return (0);
   if (ival >= dmfval[1])
      return (1);
   if (ival > (dmfval[0] + dmfval[1]) / 2)
      return (1);
   else
      return (0);
}

static int 
verifysys(cptr)
{
   char            tbuf[42],
                   tchar;
   int             iter,
                   l;

   l = strlen(cptr);
   if (l < 1)
      return (-1);		/* Invalid length */
   if (l > 40)
      l = 40;			/* Truncate to 40 chars */
   strncpy(&tbuf[0], cptr, 40);
   tbuf[40] = '\0';		/* Verify string terminated */

   for (iter = 0; iter < l; iter++)	/* Cvt to upper case */
   {
      tchar = tbuf[iter];
      if ('a' <= tchar && tchar <= 'z')
	 tbuf[iter] = toupper(tchar);
   }

   for (iter = 0; iter < NSYSTYPES; iter++)
      if (strcmp(systable[iter].systype, &tbuf[0]) == 0)
	 return (iter);
 
#ifdef VMS
/*  VMS:  Allow "system" to have value of VMS.  Call this a
          data station (for now).                               */
 
        if (strncmp( "VMS", &tbuf[ 0 ], 3 ) == 0) return( 1 );
#endif

   return (-1);
}

#ifdef SUN
static 
putSysGlobalVals(path)
char           *path;
{
   char            rfval[4];
   int             r;
   double          dval,
                   dhpvalue;

   if (r = P_setstring(
		       SYSTEMGLOBAL,
		       "system",
		       systable[systemindex].systype, 1
		       ))
   {
      P_err(r, "SYSTEM GLOBAL", "system: ");
      ABORT;
   }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "h1freq",
		     (double) h1vals[protonindex], 1
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "h1freq: ");
      ABORT;
   }

   rfval[0] = rftable[xmitter.rfindex].rftype;
   rfval[1] = rftable[decoupler.rfindex].rftype;
   rfval[2] = rfval[3] = '\0';
   if (r = P_setstring(SYSTEMGLOBAL, "rftype", &rfval[0], 1))
   {
      P_err(r, "SYSTEM GLOBAL", "rftype: ");
      ABORT;
   }

   rfval[0] = amptable[xmitter.ampindex].amptype;
   rfval[1] = amptable[decoupler.ampindex].amptype;

/*********************************************************
*  Set maximum DHP value based upon AMPTYPE and H1FREQ.  *
*********************************************************/

   if (rfval[1] == 'a')
   {
      if (h1vals[protonindex] > DHPCHNGFREQ)
      {
	 dhpvalue = (double) (dhpval[1]);
      }
      else
      {
	 dhpvalue = (double) (dhpval[0]);
      }
   }
   else
   {
      dhpvalue = (double) (dhpval[2]);
   }

   if (r = P_setreal(SYSTEMGLOBAL, "parmax", dhpvalue, DHPINDEX))
   {
      P_err(r, "SYSTEM GLOBAL", "parmax: ");
      ABORT;
   }
   else
   {
      execString("dhp=dhp\n");
   }

   rfval[2] = rfval[3] = '\0';
   if (r = P_setstring(SYSTEMGLOBAL, "amptype", &rfval[0], 1))
   {
      P_err(r, "SYSTEM GLOBAL", "amptype: ");
      ABORT;
   }

   if (r = P_setstring(
		       SYSTEMGLOBAL,
		       "syn",
		       (numsyn == 2) ? "yy" : "yn", 1
		       ))
   {
      P_err(r, "SYSTEM GLOBAL", "syn: ");
      ABORT;
   }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "ptsval",
		     (double) ptsvals[xmitter.ptsindex], 1
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "ptsval[1]: ");
      ABORT;
   }

   if (numsyn == 2)
      dval = (double) ptsvals[decoupler.ptsindex];
   else
      dval = 160.0;
   if (r = P_setreal(SYSTEMGLOBAL, "ptsval", dval, 2))
   {
      P_err(r, "SYSTEM GLOBAL", "ptsval[2]: ");
      ABORT;
   }

/*  Set "traymax" active if value is non-zero; inactive otherwise.  */

   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "traymax",
		     (double) traysize[trayindex], 1
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "traymax: ");
      ABORT;
   }
   P_setactive(SYSTEMGLOBAL, "traymax",
	       (traysize[trayindex] == 0) ? ACT_OFF : ACT_ON
      );

   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "vttype",
		     (vtindex == 0) ? 0.0 : 2.0, 1))
   {
      P_err(r, "SYSTEM GLOBAL", "vttype: ");
      ABORT;
   }
   if (r = P_setstring(
		       SYSTEMGLOBAL,
		       "z5flag",
		       (z5index) ? "y" : "n", 1
		       ))
   {
      P_err(r, "SYSTEM GLOBAL", "z5flag: ");
      ABORT;
   }
   if (r = P_setstring(
		       SYSTEMGLOBAL,
		       "audiofilter",
		       (butterworth) ? "b" : "e", 1
		       ))
   {
      P_err(r, "SYSTEM GLOBAL", "butterworth: ");
      ABORT;
   }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "parmax",
		     (double) dmfval[dmfindex], DMFINDEX
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "parmax: ");
      ABORT;
   }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "parstep",
		     parstepvals[tofstep], TOFINDEX
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "parstep: ");
      ABORT;
   }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "parstep",
		     parstepvals[dofstep], DOFINDEX
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "parstep: ");
      ABORT;
   }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "parmax",
		     swmaxvals[maxswindex], SWINDEX
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "max sw: ");
      ABORT;
   }
   /*
    * change Spectral width, Pulse, Long_Pulse & Delay parameter Step Sizes,
    * based on FifoLoop Size
    */
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "parstep",
		     -delaystepvals[delayindex], SWINDEX
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "parstep: ");
      ABORT;
   }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "parstep",
		     pulsestepvals[pulseindex], PULSEINDEX
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "parstep: ");
      ABORT;
   }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "parstep",
		     delaystepvals[delayindex], DELAYINDEX
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "parstep: ");
      ABORT;
   }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "parstep",
		     pulsestepvals[pulseindex], LPULSEINDEX
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "parstep: ");
      ABORT;
   }
   if (apinfflag)
      if (r = P_setreal(
			SYSTEMGLOBAL,
			"apinterface",
			(double) apinfvals[apinterface], 1
			))
      {
	 P_err(r, "SYSTEM GLOBAL", " apinterface:");
	 ABORT;
      }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "fifolpsize",
		     (double) fifovals[fifoindex], 1
   /* (fifoindex == 0) ? "63" : "1024", 1 */
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", " fifolpsize: ");
      ABORT;
   }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "rotorsync",
		     (double) rsyncindex, 1
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", " rotorsync: ");
      ABORT;
   }
   if (r = P_setreal(
		     SYSTEMGLOBAL,
		     "parmax",
		     dlpmaxvals[maxswindex], DLPINDEX
		     ))
   {
      P_err(r, "SYSTEM GLOBAL", "max dlp: ");
      ABORT;
   }

   if (r = P_save(SYSTEMGLOBAL, path))
      Werrprintf("failed to write system global parameters to %s\n",
		 path
	 );
}
#endif

static 
getSysGlobalVals(fnptr)
char           *fnptr;
{
   char            tempbuf[42],
                   tchar,
                   dtype,
                   xtype;
   int             xmval,
                   dcval,
                   iter,
                   l,
                   r;
   double          dval;

/*	if (r = P_read( SYSTEMGLOBAL, fnptr )) {
		Werrprintf( "failed to read system global parameters from %s\n",
			fnptr
		);
		ABORT;
	}  Done at system bootup.  */

/*  Now extract specific items from the system globals tree.  */

   if (r = P_getstring(SYSTEMGLOBAL, "system", &tempbuf[0], 1, 40))
   {
      P_err(r, "SYSTEM GLOBAL", "system: ");
      ABORT;
   }
   else
   {
      systemindex = verifysys(&tempbuf[0]);

      if (systemindex < 0)
      {
	 Werrprintf(
		    "System type of %s not supported", &tempbuf[0]
	    );
	 ABORT;
      }
   }
   if (r = P_getreal(SYSTEMGLOBAL, "h1freq", &dval, 1))
   {
      P_err(r, "SYSTEM GLOBAL", "h1freq: ");
      ABORT;
   }
   else
   {
      protonindex = -1;		/* Not found */
      for (iter = 0; iter < NH1FREQS; iter++)
	 if (h1vals[iter] == (int) dval)
	    protonindex = iter;

      if (protonindex < 0)
      {
	 Werrprintf(
		    "Proton frequency (value of h1freq) not supported"
	    );
	 ABORT;
      }
   }

/*  Obtain information about the RF on this system.  */

   if (r = P_getstring(SYSTEMGLOBAL, "rftype", &tempbuf[0], 1, 10))
   {
      P_err(r, "SYSTEM GLOBAL", "rftype: ");
      ABORT;
   }
   xtype = tempbuf[0];
   dtype = tempbuf[1];

   if ('A' <= xtype && xtype <= 'Z')
      xtype = tolower(xtype);
   if ('A' <= dtype && dtype <= 'Z')
      dtype = tolower(dtype);
   if ((xmval = verifyrf(xtype)) < 0)
   {
      Werrprintf("Invalid rf type for transmitter");
      ABORT;
   }
   if ((dcval = verifyrf(dtype)) < 0)
   {
      Werrprintf("Invalid rf type for decoupler");
      ABORT;
   }
   if (dtype == BBANDFIXEDH1)
   {
      Werrprintf(
		 "Broadband + Fixed H1 not supported as a decoupler type"
	 );
      ABORT;
   }
   xmitter.rfindex = xmval;
   decoupler.rfindex = dcval;

   if (r = P_getstring(SYSTEMGLOBAL, "amptype", &tempbuf[0], 1, 10))
   {
      P_err(r, "SYSTEM GLOBAL", "amptype: ");
      ABORT;
   }
   xtype = tempbuf[0];
   dtype = tempbuf[1];

   if ('A' <= xtype && xtype <= 'Z')
      xtype = tolower(xtype);
   if ('A' <= dtype && dtype <= 'Z')
      dtype = tolower(dtype);
   if ((xmval = verifyamp(xtype)) < 0)
   {
      Werrprintf("Invalid amplifier type for transmitter");
      ABORT;
   }
   if ((dcval = verifyamp(dtype)) < 0)
   {
      Werrprintf("Invalid amplifier type for decoupler");
      ABORT;
   }
   xmitter.ampindex = xmval;
   decoupler.ampindex = dcval;

   if (r = P_getstring(SYSTEMGLOBAL, "syn", &tempbuf[0], 1, 40))
   {
      P_err(r, "SYSTEM GLOBAL", "syn: ");
      ABORT;
   }
   if (tempbuf[0] != 'y' && tempbuf[0] != 'Y')
   {
      Werrprintf(
	      "Check value of 'syn', at least 1 synthesizer must be present"
	 );
      ABORT;
   }
   if (tempbuf[1] == 'y' || tempbuf[1] == 'Y')
      numsyn = 2;
   else if (tempbuf[1] == 'n' || tempbuf[1] == 'N')
      numsyn = 1;
   else
   {
      Werrprintf(
		 "Second character of 'syn' not valid (should be 'y' or 'n')"
	 );
      ABORT;
   }
   if (r = P_getreal(SYSTEMGLOBAL, "ptsval", &dval, 1))
   {
      P_err(r, "SYSTEM GLOBAL", "ptsval[1]: ");
      ABORT;
   }
   xmval = verifyPTS((int) dval);
   if (xmval < 0)
   {
      Werrprintf("First element of 'ptsval' array is invalid");
      ABORT;
   }
   else
      xmitter.ptsindex = xmval;

   if (numsyn == 2)
   {
      if (r = P_getreal(SYSTEMGLOBAL, "ptsval", &dval, 2))
      {
	 P_err(r, "SYSTEM GLOBAL", "ptsval[2]: ");
	 ABORT;
      }
      dcval = verifyPTS((int) dval);
      if (dcval < 0)
      {
	 Werrprintf(
		    "Second element of 'ptsval' array is invalid"
	    );
	 ABORT;
      }
      else
	 decoupler.ptsindex = dcval;
   }
   else
      decoupler.ptsindex = 0;

/*  Miscellaneous parameters... vttype, z5flag, audiofilter, etc.  */

   if (r = P_getreal(SYSTEMGLOBAL, "traymax", &dval, 1))
   {
      P_err(r, "SYSTEM GLOBAL", "traymax: ");
      ABORT;
   }
   else
   {
      trayindex = verifytray((int) dval);
      if (trayindex < 0)
      {
	 Werrprintf(
		  "Sample changer tray size of %d not supported", (int) dval
	    );
	 ABORT;
      }
   }
   if (r = P_getreal(SYSTEMGLOBAL, "vttype", &dval, 1))
   {
      P_err(r, "SYSTEM GLOBAL", "vttype: ");
      ABORT;
   }
   else
      vtindex = (int) dval;
   if (vtindex != 0)
      vtindex = 1;
   if (r = P_getstring(SYSTEMGLOBAL, "z5flag", &tempbuf[0], 1, 10))
   {
      P_err(r, "SYSTEM GLOBAL", "z5flag: ");
      ABORT;
   }
   if (tempbuf[0] == 'y' || tempbuf[0] == 'Y')
      z5index = 1;
   else if (tempbuf[0] == 'n' || tempbuf[0] == 'N')
      z5index = 0;
   else
   {
      Werrprintf("z5flag has invalid value (should be 'y' or 'n')");
      ABORT;
   }

   if (r = P_getstring(SYSTEMGLOBAL, "audiofilter", &tempbuf[0], 1, 10))
   {
      P_err(r, "SYSTEM GLOBAL", " audiofilter: ");
      ABORT;
   }
   if (tempbuf[0] == 'e' || tempbuf[0] == 'E')
      butterworth = 0;
   else if (tempbuf[0] == 'b' || tempbuf[0] == 'B')
      butterworth = 1;
   else
   {
      Werrprintf(
	     "audiofilter should be 'b' (butterworth) or 'e' (elliptical)");
      ABORT;
   }
   if (r = P_getreal(SYSTEMGLOBAL, "apinterface", &dval, 1))
   {
      apinterface = 0;
      apinfflag = 0;
   }
   else
   {
      apinfflag = 1;
      apinterface = -1;		/* Not found */
      for (iter = 0; iter < NAPINFVALS; iter++)
	 if (apinfvals[iter] == (int) dval)
	    apinterface = iter;
      if (apinterface < 0)
      {
	 Werrprintf(
		    " AP Interface should be either  Type 1 or Type 2  ");
	 apinterface = 0;
	 P_setreal(SYSTEMGLOBAL, "apinterface",
		   (double) apinfvals[apinterface], 1);
      }
   }

   if (r = P_getreal( SYSTEMGLOBAL, "fifolpsize", &dval, 1))
   {
      fifoindex = -1;
      pulseindex = delayindex = 0;  /* default to .1 usec step */
   }
   else
   {
      fifoindex = -1;                         /* Not found */
      for (iter = 0; iter < NFIFOVALS; iter++)
        if ( fifovals[ iter ] == (int) dval )
          fifoindex = iter;
 
      if (fifoindex < 0)
      {
         Werrprintf("Fifo loop size should be either 63 or 1024 ");
         fifoindex = 0;
         P_setreal( SYSTEMGLOBAL, "fifolpsize",
                      (double) fifovals[fifoindex],1 );
         pulseindex = delayindex = 0;  /* default to .1 usec step */
      }
      if (fifoindex > 0)   /* stepsize according to fifo */
          pulseindex = delayindex = 1;
   }
 
   if (r = P_getreal(SYSTEMGLOBAL, "rotorsync", &dval, 1))
   {
      rsyncindex = 0;
   }
   else
   {
      if (dval >= 0.0 && dval <= 1.0)
      {
	 rsyncindex = (int) dval;	/* Not found */
      }
      else
      {
	 Werrprintf(
		" Rotor Sync should be either 'Not Present' or 'Present' ");
	 rsyncindex = 0;
      }
   }

/*  Maximum DMF value now determined by an entry in "parmax"  */

   if (r = P_getreal(SYSTEMGLOBAL, "parmax", &dval, DMFINDEX))
   {
      P_err(r, "SYSTEM GLOBAL", "dmfmax: ");
      ABORT;
   }
   else
      dmfindex = verifyDMF((int) dval);	/* Always works  */

/*  Get step values for TOF, DOF from PARSTEP array  */

   if (r = P_getreal(SYSTEMGLOBAL, "parstep", &dval, TOFINDEX))
   {
      P_err(r, "SYSTEM GLOBAL", "parstep: ");
      ABORT;
   }
   tofstep = verifystep(dval);
   if (r = P_getreal(SYSTEMGLOBAL, "parstep", &dval, DOFINDEX))
   {
      P_err(r, "SYSTEM GLOBAL", "parstep: ");
      ABORT;
   }
   dofstep = verifystep(dval);
   if (r = P_getreal(SYSTEMGLOBAL, "parmax", &dval, SWINDEX))
   {
      P_err(r, "SYSTEM GLOBAL", "maxsw: ");
      ABORT;
   }
   maxswindex = verifymaxsw(dval);
   RETURN;
}

#ifdef SUN
/*  NOTE WELL THAT THIS ROUTINE ONLY WORKS SO LONG AS THE PROTON
    FREQUENCY IN MEGAHERTZ IS AN INTEGER BETWEEN 0 AND 999.	*/

static 
makelink(lptr)
char           *lptr;
{
   char            basepath[MAXPATHL],
                   linkpath[MAXPATHL];
   int             ival,
                   len;

/*  Base path:  /jaws/par500/stdpar  */

   strcpy(&basepath[0], systemdir);
   strcat(&basepath[0], "/parXXX/");
   len = strlen(&basepath[0]) - 4;	/* replace X with digit */
   if (protonindex >= NH1FREQS)
      return;
   else
   {
      ival = h1vals[protonindex];
      basepath[len++] = '0' + ival / 100;
      basepath[len++] = '0' + ((ival % 100) / 10);
      basepath[len++] = '0' + (ival % 10);
   }
   strcat(&basepath[0], lptr);

/*  Verify the base path exists before proceeding...  */

   if (access(&basepath[0], F_OK) != 0)
   {
      Werrprintf("entry %s doesn't exist", &basepath[0]);
      ABORT;
   }

/*  Link path:  /jaws/stdpar  */

   strcpy(&linkpath[0], systemdir);
   strcat(&linkpath[0], "/");
   strcat(&linkpath[0], lptr);

/*  Only unlink if the link is present.  */

   if (access(&linkpath[0], F_OK) == 0)
      if (unlink(&linkpath[0]))
      {
	 perror("unlink failure in config");
	 Werrprintf("failed to unlink %s", &linkpath[0]);
	 ABORT;
      }
   if (symlink(&basepath[0], &linkpath[0]))
   {
      perror("link failure in config");
      Werrprintf("Failed to link %s to %s",
		 &linkpath[0], &basepath[0]
	 );
      ABORT;
   }
   RETURN;
}

/*  The following three routines remove the config frame.
    removeConfig() actually removes it.
    turnoff_config() is called when Wturnoff_buttons is called
      after using the config command interactively.
    configExit is called when the EXIT or QUIT button is selected.  */

static 
removeConfig()
{
   window_set(configFrame, FRAME_NO_CONFIRM, TRUE, 0);
   window_destroy(configFrame);
   configFrame = NULL;
   execString("menu\n");
}

/*  Since the user may already have pressed the EXIT button, it
    is necessary to check if the frame has already been removed.   */

static 
turnoff_config()
{
   if (configFrame != NULL)
      removeConfig();
}

/*  This routine is called when the EXIT or QUIT button is pressed.  */

static void 
configExit(item, e)
Panel_item      item;
Event          *e;
{
   if (item == exitButton)
   {
      if (makestdparlink)
	 makelink("stdpar");
      if (maketestslink)
	 makelink("tests");
      putSysGlobalVals(conparfil);
      execString("_h1freq\n");

/*  Following VNMR command adjusts spectral width and dlp
    parameters if the current values are no longer allowed.	*/

      execString("sw=sw dlp=dlp\n");
   }
   removeConfig();
}

static int 
selectPanel(item)
Panel_item      item;
{
   int             iter;

   for (iter = 0; iter < NUMPANELS; iter++)
      if (item == paramPanels[iter])
	 return (iter);

   return (-1);
}

/*--------------------------------------------------------------------
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   1/3/89     Greg B.    1. Added ROTORSYNCPANEL Case
|                         2. Modified FIFOPANEL Case to remove or show 
|			     RotorSync panel. (rotorsync only allowed 
|			     with new output board)
|			     The window is resized when the RotorSync 
|			     is shown or removed.
+---------------------------------------------------------------------*/
static void 
dispatchChoice(item, value, event)
Panel_item      item;
int             value;
Event          *event;
{
   int             pindex;

   pindex = selectPanel(item);
/*	Wscrprintf( "dispatchChoice called,  %x   %d   %d\n",
		item, value, pindex
	);				*/

   switch (pindex)
   {

      case SYSPANEL:
	 if (value < NSYSTYPES)
	    systemindex = value;
	 else
	    Werrprintf("sysChoice called with invalid value!!");
	 break;

      case H1PANEL:
	 if (value < NH1FREQS)
	    protonindex = value;
	 else
	    Werrprintf("h1Choice called with invalid value!!");
	 break;

      case XMPANEL:
	 if (value < NRFTYPES)
	    xmitter.rfindex = value;
	 else
	    Werrprintf("xmChoice called with invalid value!!");
	 break;

      case DCPANEL:
	 if (value < NRFTYPES)

#ifdef SIS
	 {
	    if (value == 3)
	       value = 4;
	    /* correct for blank in table */
	    decoupler.rfindex = value;
	 }
#else
	    decoupler.rfindex = value;
#endif

	 else
	    Werrprintf("dcChoice called with invalid value!!\n");
	 break;

      case SYNPANEL:
	 if (value < 2)
	 {
	    value++;
	    if (value == 2 && numsyn == 1)
	       showPTS2();
	    else if (value == 1 && numsyn == 2)
	       removePTS2();
	    numsyn = value;
	 }
	 else
	    Werrprintf("synChoice called with invalid value!!\n");
	 break;

      case PTS1PANEL:
	 if (value < NPTSTYPES)
	    xmitter.ptsindex = value;
	 else
	    Werrprintf("pts1Choice called with invalid value!!\n");
	 break;

      case PTS2PANEL:
	 if (value < NPTSTYPES)
	    decoupler.ptsindex = value;
	 else
	    Werrprintf("pts2Choice called with invalid value!!\n");
	 break;

      case XAMPPANEL:
	 if (value < NAMPTYPES)
	    xmitter.ampindex = value;
	 else
	    Werrprintf("xmtr amplifier type called with invalid value!!\n");
	 break;

      case DAMPPANEL:
	 if (value < NAMPTYPES)
	    decoupler.ampindex = value;
	 else
	    Werrprintf("dec amplifier type called with invalid value!!\n");
	 break;

      case TRAYPANEL:
	 if (value < NTRAYSIZES)
	    trayindex = value;
	 else
	    Werrprintf("trayChoice called with invalid value!!\n");
	 break;

      case Z5PANEL:
	 if (value < 2)
	    z5index = value;
	 else
	    Werrprintf("z5Choice called with invalid value!!\n");
	 break;

      case AFPANEL:
	 if (value < 2)
	    butterworth = value;
	 else
	    Werrprintf("filterChoice called with invalid value!!\n");
	 break;

      case VTPANEL:
	 if (value < NVTTYPES)
	    vtindex = value;
	 else
	    Werrprintf("vtChoice called with invalid value!!\n");
	 break;

      case DMFPANEL:
	 if (value < NDMFVALS)
	    dmfindex = value;
	 else
	    Werrprintf("dmfChoice called with invalid value!!\n");
	 break;

      case TOFPANEL:
	 tofstep = value;
	 break;

      case DOFPANEL:
	 dofstep = value;
	 break;

      case SWPANEL:
	 if (value < 2)
	    maxswindex = value;
	 else
	    Werrprintf("swChoice called with invalid value!!\n");
	 break;

      case APINFPANEL:
	 if (apinfflag)
	 {
	    if (value < 3)
	       apinterface = value;
	    else
	       Werrprintf("apinfChoice called with invalid value!!\n");
	 }
	 break;
      case FIFOPANEL:
	 if (value < 2)
         {
	    fifoindex = value;
	    if (value == 1)
	    {
               pulseindex = delayindex = 1;
	       showRsync();
	    }
	    else if (value == 0 )
		 {
                   pulseindex = delayindex = 0;
		   rsyncindex = 0;  /* no rotor sync */
   		   panel_set(paramPanels[ROTORSYNCPANEL],
		             PANEL_VALUE, rsyncindex,
	     		     0);
	           removeRsync();
	           }
         }
	 else
         {
	    Werrprintf("fifoChoice called with invalid value!!\n");
         }
	 break;
      case ROTORSYNCPANEL:
	 if (value < 2)
	    rsyncindex = value;
	 else
	    Werrprintf("rotorsyncChoice called with invalid value!!\n");
	 break;
      default:
	 Werrprintf("dispatchChoice called from unknown panel");
	 break;
   }
}

static 
makeH1Panel()
{
   paramPanels[H1PANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "Proton freq",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 40,
		     PANEL_CHOICE_STRINGS, " 200 ",
		     " 300 ",
		     " 400 ",
		     " 500 ",
		     " 600 ",

#ifdef SIS
		     "  85 ",
#endif

		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 41,
		     PANEL_VALUE, protonindex,
		     0
   );
}

static 
makeXMPanel()
{
   paramPanels[XMPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "Transmitter RF",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 70,
		     PANEL_CHOICE_STRINGS, rftable[0].rflabel,
		     rftable[1].rflabel,
		     rftable[2].rflabel,
		     rftable[3].rflabel,

#ifdef SIS
		     rftable[4].rflabel,
#endif

		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 71,
		     PANEL_VALUE, xmitter.rfindex,
		     0
   );
}

static 
makeDCPanel()
{
   int             temp;

   temp = decoupler.rfindex;
   if (temp == 4)
      temp = 3;
   /* correct for missing element */
   paramPanels[DCPANEL] =
      panel_create_item(
			choicePanel, PANEL_CHOICE,
			PANEL_LABEL_STRING, "Decoupler RF",
			PANEL_FEEDBACK, PANEL_INVERTED,
			PANEL_NOTIFY_PROC, dispatchChoice,
			PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			PANEL_SHOW_ITEM, TRUE,
			PANEL_LABEL_X, 10,
			PANEL_LABEL_Y, 100,
			PANEL_CHOICE_STRINGS, rftable[0].rflabel,
			rftable[1].rflabel,
			rftable[2].rflabel,

#ifdef SIS
			rftable[4].rflabel,
#endif

			NULL,
			PANEL_VALUE_X, 180,
			PANEL_VALUE_Y, 101,
			PANEL_VALUE, temp,
			0
      );
}

static 
makeSYSPanel()
{
   paramPanels[SYSPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "System Type",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 10,
		     PANEL_CHOICE_STRINGS, systable[0].syslabel,
		     systable[1].syslabel,
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 11,
		     PANEL_VALUE, systemindex,
		     0
   );
}

static 
makeSYNPanel()
{
   paramPanels[SYNPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "No of Synthesizers",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 130,
		     PANEL_CHOICE_STRINGS, "  1  ",
		     "  2  ",
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 131,
		     PANEL_VALUE, numsyn - 1,
		     0
   );
}

static 
makePTS1Panel()
{
   paramPanels[PTS1PANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "XM Synthesizer",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 160,
		     PANEL_CHOICE_STRINGS, ptslabel[0],
		     ptslabel[1],
		     ptslabel[2],
		     ptslabel[3],
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 161,
		     PANEL_VALUE, xmitter.ptsindex,
		     0
   );
}

static 
makePTS2Panel()
{
   paramPanels[PTS2PANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "DC Synthesizer",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, (numsyn == 2) ? TRUE : FALSE,
		     PANEL_LABEL_X, 320,
		     PANEL_LABEL_Y, 160,
		     PANEL_CHOICE_STRINGS, ptslabel[0],
		     ptslabel[1],
		     ptslabel[2],
		     ptslabel[3],
		     NULL,
		     PANEL_VALUE_X, 490,
		     PANEL_VALUE_Y, 161,
		     PANEL_VALUE, decoupler.ptsindex,
		     0
   );
   showingPTS2 = (numsyn == 2) ? 131071 : 0;
}

static 
removePTS2()
{
   panel_set(paramPanels[PTS2PANEL],
	     PANEL_SHOW_ITEM, FALSE,
	     0
   );
   showingPTS2 = 0;
}

static 
showPTS2()
{
   panel_set(paramPanels[PTS2PANEL],
	     PANEL_SHOW_ITEM, TRUE,
	     0
   );
   showingPTS2 = 131071;
}

static 
makeXampPanel()
{
   int             temp;

   temp = xmitter.ampindex;
   paramPanels[XAMPPANEL] =
      panel_create_item(
			choicePanel, PANEL_CHOICE,
			PANEL_LABEL_STRING, "XM Amplifier",
			PANEL_FEEDBACK, PANEL_INVERTED,
			PANEL_NOTIFY_PROC, dispatchChoice,
			PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			PANEL_SHOW_ITEM, TRUE,
			PANEL_LABEL_X, 10,
			PANEL_LABEL_Y, 190,
			PANEL_CHOICE_STRINGS, amptable[0].amplabel,
			amptable[1].amplabel,
			NULL,
			PANEL_VALUE_X, 180,
			PANEL_VALUE_Y, 191,
			PANEL_VALUE, temp,
			0
      );
}

static 
makeDampPanel()
{
   int             temp;

   temp = decoupler.ampindex;
   paramPanels[DAMPPANEL] =
      panel_create_item(
			choicePanel, PANEL_CHOICE,
			PANEL_LABEL_STRING, "DC Amplifier",
			PANEL_FEEDBACK, PANEL_INVERTED,
			PANEL_NOTIFY_PROC, dispatchChoice,
			PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			PANEL_SHOW_ITEM, TRUE,
			PANEL_LABEL_X, 10,
			PANEL_LABEL_Y, 220,
			PANEL_CHOICE_STRINGS, amptable[0].amplabel,
			amptable[1].amplabel,
			NULL,
			PANEL_VALUE_X, 180,
			PANEL_VALUE_Y, 221,
			PANEL_VALUE, temp,
			0
      );
}

static 
maketrayPanel()
{
   paramPanels[TRAYPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "Sample Tray Size",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 250,
		     PANEL_CHOICE_STRINGS, "  0    ",
		     "  50   ",
		     "  100  ",
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 251,
		     PANEL_VALUE, trayindex,
		     0
   );
}

static 
makeZ5Panel()
{
   paramPanels[Z5PANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "Z5 Shimming",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 280,
		     PANEL_CHOICE_STRINGS, z5label[0],
		     z5label[1],
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 281,
		     PANEL_VALUE, z5index,
		     0
   );
}

static 
makeAFPanel()
{
   paramPanels[AFPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "Audio Filter Type",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 310,
		     PANEL_CHOICE_STRINGS, filterlabel[0],
		     filterlabel[1],
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 311,
		     PANEL_VALUE, butterworth,
		     0
   );
}

static 
makeAPINFPanel()
{
   paramPanels[APINFPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "AP Interface Type",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 490,
		     PANEL_CHOICE_STRINGS, " Type 1",
		     " Type 2",
		     " Type 3",
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 491,
		     PANEL_VALUE, apinterface,
		     0
   );
}

static 
makeRotorSyncPanel()
{
   paramPanels[ROTORSYNCPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "Rotor Sync ",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, (fifoindex == 1) ? TRUE : FALSE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 550,
		     PANEL_CHOICE_STRINGS, rsynclabel[0],
		     rsynclabel[1],
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 551,
		     PANEL_VALUE, rsyncindex,
		     0
   );
}

static 
removeRsync()
{
   panel_set(paramPanels[ROTORSYNCPANEL],
	     PANEL_SHOW_ITEM, FALSE,
	     0
   );
   window_set(configFrame, WIN_ROWS, 37, 0);
}

static 
showRsync()
{
   window_set(configFrame, WIN_ROWS, 39, 0);
   panel_set(paramPanels[ROTORSYNCPANEL],
	     PANEL_SHOW_ITEM, TRUE,
	     0
   );
}

static 
makeFIFOPanel()
{
   paramPanels[FIFOPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "Fifo Loop Size",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 520,
		     PANEL_CHOICE_STRINGS, " 63   ",
		     " 1024 ",
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 521,
		     PANEL_VALUE, fifoindex,
		     0
   );
}

static 
makeVTPanel()
{
   paramPanels[VTPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "VT Controller",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 340,
		     PANEL_CHOICE_STRINGS, vtlabel[0],
		     vtlabel[1],
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 341,
		     PANEL_VALUE, vtindex,
		     0
   );
}

static 
makeDMFPanel()
{
   paramPanels[DMFPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "Maximum DMF",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 370,
		     PANEL_CHOICE_STRINGS, " 9900  ",
		     " 32700 ",
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 371,
		     PANEL_VALUE, dmfindex,
		     0
   );
}

static 
makeTOFPanel()
{
   paramPanels[TOFPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "TO Step Size (Hz)",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 400,
		     PANEL_CHOICE_STRINGS, " 0.1 ",

#ifdef SIS
		     " 1.0 ",
#endif SIS

		     " 100 ",
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 401,
		     PANEL_VALUE, tofstep,
		     0
   );
}

static 
makeDOFPanel()
{
   paramPanels[DOFPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "DO Step Size (Hz)",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 430,
		     PANEL_CHOICE_STRINGS, " 0.1 ",

#ifdef SIS
		     " 1.0 ",
#endif SIS

		     " 100 ",
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 431,
		     PANEL_VALUE, dofstep,
		     0
   );
}

static 
makeSWPanel()
{
   paramPanels[SWPANEL] =
   panel_create_item(
		     choicePanel, PANEL_CHOICE,
		     PANEL_LABEL_STRING, "Max Spectral Width",
		     PANEL_FEEDBACK, PANEL_INVERTED,
		     PANEL_NOTIFY_PROC, dispatchChoice,
		     PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		     PANEL_SHOW_ITEM, TRUE,
		     PANEL_LABEL_X, 10,
		     PANEL_LABEL_Y, 460,
		     PANEL_CHOICE_STRINGS, " 100KHz ",
		     "  2MHZ  ",
		     NULL,
		     PANEL_VALUE_X, 180,
		     PANEL_VALUE_Y, 461,
		     PANEL_VALUE, maxswindex,
		     0
   );
}

static 
makeInteractPanel()
{
   configFrame = window_create(
			       Wframe, FRAME,
			       FRAME_SHOW_LABEL, TRUE,
			       FRAME_LABEL, "VNMR CONFIG",
			       WIN_COLUMNS, 70,
			       WIN_ROWS, 37,
			       WIN_ROWS, (fifoindex == 1) ? 39 : 37,
			       0
   );
   exitPanel = window_create(configFrame, PANEL, 0);
   exitButton =
      panel_create_item(
			exitPanel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE,
		      panel_button_image(exitPanel, "EXIT and SAVE", 13, 0),
			PANEL_NOTIFY_PROC, configExit,
			0
      );
   quitButton =
      panel_create_item(
			exitPanel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE,
		      panel_button_image(exitPanel, "QUIT, no SAVE", 13, 0),
			PANEL_NOTIFY_PROC, configExit,
			0
      );

/*  Minimize height of panel containing exit button.  */

   window_fit_height(exitPanel);
   choicePanel = window_create(configFrame, PANEL, 0);

   makeSYSPanel();
   makeH1Panel();
   makeXMPanel();
   makeDCPanel();
   makeSYNPanel();
   makePTS1Panel();
   makePTS2Panel();
   makeXampPanel();
   makeDampPanel();
   maketrayPanel();
   makeZ5Panel();
   makeAFPanel();
   makeVTPanel();
   makeDMFPanel();
   makeTOFPanel();
   makeDOFPanel();
   makeSWPanel();
   if (apinfflag)
      makeAPINFPanel();
   makeFIFOPanel();
   makeRotorSyncPanel();
   window_set(configFrame, WIN_SHOW, TRUE, 0);
}

/*  The following function checks on "stdpar" and "tests", entries
    in $vnmrsystem which normally are symbolic links.  The value
    returned specifies whether interactive access is to be allowed.

    If the entry already exists and is not a symbolic link, then no
    change in the entry is allowed, although this will not prevent
    interactive access to the system configuration.			*/

static int 
checklink(linkptr)
char           *linkptr;
{
   char            linkpath[MAXPATHL];
   int             ival,
                   retval;

   strcpy(&linkpath[0], systemdir);
   strcat(&linkpath[0], "/");
   strcat(&linkpath[0], linkptr);

   if (access(&linkpath[0], F_OK) == 0)
      if (islink(&linkpath[0]) == 0)
	 if (access(&linkpath[0], W_OK) == 0)
	    retval = 131071;
	 else
	    retval = 0;
      else
      {				/* linkptr exists, but not a link! */
	 if (strcmp(linkptr, "stdpar") == 0)
	    makestdparlink = 0;
	 if (strcmp(linkptr, "tests") == 0)
	    maketestslink = 0;
	 retval = 131071;
      }
   else
      retval = 131071;

   return (retval);
}
#endif

int 
config(argc, argv, retc, retv)
int             argc;
char           *argv[];
int             retc;
char           *retv[];

{
   int             interact,
                   xmindex,
                   dcindex,
                   xmampindex,
                   dcampindex;

   /* Wscrprintf("config 3-24-88\n"); */
   makestdparlink = 131071;	/* Normally these links are */
   maketestslink = 131071;	/* to be made if the command */
   strcpy(conparfil, systemdir);/* is used interactively */

#ifdef UNIX
   strcat(conparfil, "/conpar");
#else
   strcat(conparfil, "conpar");
#endif

/*  If no read access to conpar, abort the command.  Prevent interactive
    use if no write access is allowed, or if the current termnial is not
    the SUN graphics console.						*/

   if (access(conparfil, R_OK))
   {
      Werrprintf("no access to %s", conparfil);
      ABORT;
   }

#ifdef SUN
   if (argc > 1)
      interact = strcmp(argv[1], "display") != 0;
   else
      interact = 131071;
   if (interact)
      interact = access(systemdir, W_OK) == 0;
   if (interact)
      interact = Wissun();
   if (interact)
      interact = access(conparfil, W_OK) == 0;
   if (interact)
      interact = checklink("stdpar");
   if (interact)
      interact = checklink("tests");
#else
   interact = 0;			/* Never interactive on VMS */
#endif
   if (getSysGlobalVals(conparfil))
      ABORT;


   /* interact = 1;	/* BY PASS CHECKS FOR TESTING  */


   if (!interact)
   {
      xmindex = xmitter.rfindex;
      dcindex = decoupler.rfindex;
      xmampindex = xmitter.ampindex;
      dcampindex = decoupler.ampindex;
      Wshow_text();
      Wscrprintf("\n");
      Wscrprintf("Instrument type:         %s\n",
		 systable[systemindex].syslabel);
      Wscrprintf("Proton frequency (MHz):   %d\n",
		 h1vals[protonindex]);
      Wscrprintf("Transmitter RF:          %s\n",
		 rftable[xmindex].rflabel
	 );
      Wscrprintf("Decoupler RF:            %s\n",
		 rftable[dcindex].rflabel
	 );
      Wscrprintf("Number of Synthesizers:   %d\n", numsyn);
      Wscrprintf("Transmitter Synthesizer: %s\n",
		 ptslabel[xmitter.ptsindex]
	 );
      if (numsyn > 1)
	 Wscrprintf("Decoupler Synthesizer:   %s\n",
		    ptslabel[decoupler.ptsindex]
	    );
      Wscrprintf("Transmitter Amplifier:   %s\n",
		 amptable[xmampindex].amplabel
	 );
      Wscrprintf("Decoupler Amplifier:     %s\n",
		 amptable[dcampindex].amplabel
	 );
      if (traysize[trayindex] == 0)
	 Wscrprintf("Sample Changer:           Not Used\n");
      else
	 Wscrprintf("Sample Tray Size:         %d\n",
		    traysize[trayindex]
	    );
      Wscrprintf("VT Controller:           %s\n",
		 vtlabel[vtindex]
	 );
      Wscrprintf("Z5 Shimming:             %s\n",
		 z5label[z5index]
	 );
      Wscrprintf("Audio Filter:            %s\n",
		 filterlabel[butterworth]
	 );
      Wscrprintf("Maximum DMF               %d\n",
		 dmfval[dmfindex]
	 );
      Wscrprintf("TOF step value (Hz)       %.12g\n",
		 parstepvals[tofstep]
	 );
      Wscrprintf("DOF step value (Hz)       %.12g\n",
		 parstepvals[dofstep]
	 );
      Wscrprintf("Max Spectral Width (Hz)   %.12g\n",
		 swmaxvals[maxswindex]
	 );
      Wscrprintf("AP Interface Type         %d\n",
		 apinfvals[apinterface]
	 );
      if (fifoindex != -1)
	 Wscrprintf("FIFO Loop Size            %d\n",
		    fifovals[fifoindex]
	    );
      if ( fifovals[fifoindex] > 64 )
         Wscrprintf("Rotor Sync               %s\n",
		 rsynclabel[rsyncindex]
	 );
      RETURN;
   }

#ifdef SUN

/*  Come here for interactive configuration.  Setting menuon fools the
    menu system into waiting until the user closes the config panel.
    Note that the exiting routines run the MENU command, so that the
    last menu will be redisplayed when the user terminates the config
    session.  However, this does not mean the last set of buttons will
    always be displayed, as some commands, such as DS, create their
    own set of buttons independent of the menu library.			*/

   else
   {
      menuon = 1;
      Wactivate_buttons(0, turnoff_config, "sysconfig");
      makeInteractPanel();
   }

   RETURN;
#endif
}

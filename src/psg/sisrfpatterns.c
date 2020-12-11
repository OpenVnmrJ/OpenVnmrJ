/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/************************************************************************
* RFpatterns.c
*	Routines that will generate rf patterns that will be
*	executed in a pulse sequence.
*************************************************************************/


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "oopc.h"
#include "group.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "vnmrsys.h"
#include "macros.h"




/* NOTE WELL THAT PATERR MUST BE < 0 FOR THIS STUFF TO WORK!! */
#define	PATERR	-1	/* NOTE WELL THAT MATT DEFINED THIS AS ALWAYS < 1
			(SOMETIMES == 0, SOMETIMES < 0, SOMETIMES <= 0) */
#define OK	0	/* NOTE WELL THAT MATT DEFINED THIS AS ALWAYS > 1 */
#define SYSERR	-1	/* compatible with most UNIX error returns */
#define COMMENTCHAR '#'	/* character that signifies the start of a comment */

extern int chgdeg2b();

extern int acqiflag;		/* True if acodes are for Acqi */
extern int bgflag;		/* print diagnostics if not 0 */
extern int rcvroff_flag;	/* global receiver off flag */
/* extern int decblankoff_flag;	/* global decoupler blanking flag */
extern char filexpath[];	/* ex: $vnmrsys/acqque/exp1.russ.012345 */
extern char fileRFpattern[];	/* absolute path name of WG RF load file */
extern char filegrad[]; 	/* path for old SIS RF modulator patterns */


/************************
*  pattern globals	*
*************************/

#define td_loc  30  
#define tc_loc  21 
#define ph_loc  0 
#define amp_loc 10 
#define td_len   2 
#define tc_len   9 
#define ph_len  10 
#define amp_len 11 
#define INPUTPATTRN_ARRAYSIZE 128
#define INPUTPTR_ARRAYSIZE 64
#define TIMTBLSIZE 16
#define PATWRDMAX 8192
#define TBNS 0.1
#define TBUS 1.0
#define TB10US 10.0
#define TB100US 100.0
#define	MODE 0666
#define MIN_DELAY 0.2e-6
#define MINROF1	  0.995e-5


/* definitions for linked list	*/
#define FREE 1		/* type for entry on freelist	 */
#define USED 2		/* type for entry on usedlist   */
#define DELARRAY 2	/* number of levels to check for existing patterns */


struct pmlist {
    int     type;		/*  entry type free or vec	 */
    int	    patadr;		/*  Pattern Memory physical address  */
    int     arrayno;		/*  array no last used		 */
    int     wordcount;		/*  Total size in words		 */
    char    pulsefile[MAXPATHL]; /*  relative path name of pattern template */
    double  pulsewidth;		/*  width of pattern		*/
    struct pmlist  *flink;	/*  forward link into next entry */
};


int	delarrayno;		/* set to the present array number	*/
				/* minus DELARRAY before allocating	*/
				/* pattern				*/


struct shapetbl {		/* structure of pattern table		*/
	short	apadr;
	short	garb1;		/* dummy to avoid offset clash bug */
	short	garb2;		/* dummy to avoid offset clash bug */
	short	spare;
	struct pmlist  *plhead;	/*  head of memory map list	  */
	};


struct shapetbl obstbl;		/* allocation of pattern tables		*/
struct shapetbl dectbl;	
struct shapetbl *otptr;		/* pointers to pattern tables		*/
struct shapetbl *dtptr;

/* end definitions for linked list	*/


struct patternpoint {		/* input pattern point structure	*/
	float	phase;
	float	amplitude;
	float	unitcnt;
	};
				/* size of memory allocation portion 	*/

struct patinarray {		/* when reading in pattern template	*/
	struct	patternpoint patpnt[INPUTPATTRN_ARRAYSIZE];
	};
				/* pointers to the allocated portions	*/
struct patinarray *inpatptr[INPUTPTR_ARRAYSIZE];


struct outpatfile {		/* output file descriptor structure	*/
	int	fd;		/* unit number		*/
	codelong *hptr;		/* header pointer 	*/		
	unsigned long *mptr;	/* data pointer		*/
	int	size;		/* size			*/
	} ;

struct	outpatfile rfpatfildesc;
struct	outpatfile *rfpatfilptr;


struct	dataheader {
		short apadr;	/* NOT A UNIQUE ELEMENT NAME!! */
		short patptr;
		short patwords;
		short spare;	/* NOT A UNIQUE ELEMENT NAME!! */
	} *hdrptr;


struct	ptimecodes {
		int timecnt;
		int timedur;
	};

int	patpntcnt;
int	arrayno;


/*** varibles for use with psg arrayed experiments	***/
codelong  *p_filhdrpos;


static char pulsepath[MAXPATHL]; /* absolute path name of template file */
static char paterrmsg[MAXPATHL]; /* for forming messages for text_error */


/************************************************************************
*	RF PATTERN ELEMENTS - mrh
*************************************************************************/


initshapedpatterns(dummyarg)
/************************************************************************
* Initshapedpatterns needs to be called once at the beginning of an
* experiment acquisition.  It initializes the tables used for tracking 
* patterns used in the acquisition.
* PATERR RETURN FROM THIS FUNCTION SHOULD ABORT PULSE SEQUENCE GENERATION !!
*************************************************************************/
int dummyarg;	/* MUST BE 1! will be arraydim when arrayed patterns work */
{
 int	i;
 int	mask;

 arrayno = 0;

 /*****						*********/
 /* initialize output file and header for patterns	*/
 /*****						*********/
 /* form load file absolute path name */
 strcpy(filegrad,filexpath);
 strcat(filegrad,".Grad");	/* $vnmrsystem/acqqueue/exp1.russ.012345.RF */
 if (bgflag) 
 {
	sprintf(paterrmsg,"initshapedpatterns: RF pattern file %s.\n",filegrad);
	text_error(paterrmsg);
 }
 rfpatfilptr = &rfpatfildesc;
 rfpatfilptr->size = ((dummyarg+1) * sizeof(codelong));
 rfpatfilptr->hptr = (codelong *)malloc(rfpatfilptr->size);
 if (rfpatfilptr->hptr == NULL)
 {
	text_error("could not allocate memory for RF pattern file header\n");
	return(PATERR);
 }
 p_filhdrpos = rfpatfilptr->hptr;
 *p_filhdrpos = rfpatfilptr->size;
 /* use either O_EXCL or O_TRUNC */
 if (acqiflag){
     /* RF pattern file may already exist. That's OK, we'll trash it. */
     mask = umask(002);		/* Write access for group */
     rfpatfilptr->fd = open(filegrad,O_RDWR | O_CREAT | O_TRUNC, MODE);
     umask(mask);
 }else{
     rfpatfilptr->fd = open(filegrad,O_RDWR | O_CREAT | O_EXCL, MODE);
 }
 if (rfpatfilptr->fd == SYSERR)
 {
	sprintf(paterrmsg,"RF pattern file %s not opened\n",filegrad);
	text_error(paterrmsg);
	return(PATERR);
 }
 if (write(rfpatfilptr->fd,rfpatfilptr->hptr,rfpatfilptr->size)
						!= rfpatfilptr->size)
 {
	text_error("RF pattern header not written\n");
	return(PATERR);
 }

 /*******							*********/
 /* initialize linklist to track patterns in Pat memory			*/
 /*******							*********/
 otptr = &obstbl;
 if (bgflag)
	fprintf(stderr,"otptr = %lx\n",otptr);
 if (initpl(otptr) == PATERR)
 {
	text_error("could not allocate memory for obs RF pulse table\n");
	return(PATERR);
 }
 otptr->apadr = OBSAPADR;

 dtptr = &dectbl;
 if (initpl(dtptr) == PATERR)
 {
	text_error("could not allocate memory for dec RF pulse table\n");
	return(PATERR);
 }
 dtptr->apadr = DECAPADR;


 return(OK);
}

p_endarrayelem()
/*************************************************************************
* p_endarrayelem is to be added to the end of each array element so 
* the array header at the beginning of the pulsesequence can be 
* updated.
*************************************************************************/
{
 struct pmlist  *pl;		/* memory map list pointer	    */


/* output for testing */
 if (bgflag) {
	fprintf(stderr,"obstable npat:\n");
	for (pl=otptr->plhead; pl!=NULL; pl=pl->flink)
		fprintf(stderr,"patname: %s width: %g ptr: %d length: %d",
		pl->pulsefile,pl->pulsewidth,
		pl->patadr,pl->wordcount);
	fprintf(stderr,"\n");
 }

 if (bgflag) {
	fprintf(stderr,"dectable npat:\n");
	for (pl=dtptr->plhead; pl!=NULL; pl=pl->flink)
		fprintf(stderr,"patname: %s width: %g ptr: %d length: %d",
		pl->pulsefile,pl->pulsewidth,
		pl->patadr,pl->wordcount);
	fprintf(stderr,"\n");
 }
/* end output for testing */

 /* write of size of file in words into file header */
 *++p_filhdrpos = rfpatfilptr->size;
 
 arrayno++;			/* increment array counter	*/
}

closeshapedpatterns(dummyarg)
/************************************************************************
* Frees memory that holds the pattern link list.  
* Writes the size of the pattern file in words and the last sector of
*  the pattern files.
* Closes the pattern files in the experiment library.
* This should be called
* after all the array elements have been generated at the end of the
* experiment.
* PATERR RETURN FROM THIS FUNCTION SHOULD ABORT PULSE SEQUENCE GENERATION !!
*************************************************************************/
int dummyarg;	/* MUST BE 1! will be arraydim when arrayed patterns work */
{
	struct pmlist  *pl;		/* memory map list pointer */
	struct pmlist  *nextpl;		/* memory map list pointer */

	if (rfpatfilptr->size > (long)(dummyarg+1) * sizeof(codelong)) {
	/* "rewind" file and update file header */
		if (lseek(rfpatfilptr->fd,0,0) == SYSERR) {
			text_error("RF lseek failed!\n");
			return(PATERR);
		}
		if (write(rfpatfilptr->fd,rfpatfilptr->hptr,
		(long)(dummyarg+1) * sizeof(codelong)) !=
		(long)(dummyarg+1) * sizeof(codelong)) {
			text_error("RF write failed!\n");
			return(PATERR);
		}
	}
	else {
		if (unlink(filegrad) == SYSERR) {
			text_error("RF unlink failed!\n");
			return(PATERR);
		}
		filegrad[0] = 0; /* set filename to null string */
	}

	if (close(rfpatfilptr->fd) == SYSERR) {
		text_error("RF close failed!\n");
		return(PATERR);
	}

	/* free all nodes in the observe linked list */
	pl = otptr->plhead;
	while (pl != NULL) {
		nextpl = pl->flink; /* grab forward link while valid */
		free(pl);
		pl = nextpl;	/* get saved value of forward link */
	}

	/* free all nodes in the decouple linked list */
	/*this "for" is equivalent to the "while" construct immediately above!*/
	for (pl = dtptr->plhead; pl != NULL; pl = nextpl) {
		nextpl = pl->flink; /* grab forward link while valid */
		free(pl);
	}

	return(OK);
}

void S_shapedpulse(char *pulsefile, double pulsewidth, codeint phaseptr,
                   double rx1, double rx2) 
/************************************************************************
* Arguments: 
*	pulsefile	: filname of shaped pulse in shplib 
*	pulsewidth	: width of pulse (in sec) 
*	phaseptr	: phase (actual phase will be selected by a 
*				modulo four of this argument). 
*	rx1,rx2		: prepulse & postpulse receiver gate on parms 
* 							
* NOTE: Because of delays in the rf phase shifter there must be 
*       at least 15us between pattern pulses.  This is taken care 
*       of by setting the pointer before each pulse (14.2us), and  
*       adding 1us of delay. 
*************************************************************************/
{
 double twidth;
 codeint patptr;
	if (bgflag) {
		fprintf(stderr,"pulsefile addr = %x\n",pulsefile);
		fprintf(stderr,"pulsefile = %s\n",pulsefile);
		fprintf(stderr,"pulsewidth = %lf\n",pulsewidth);
		fprintf(stderr,"phaseptr = %d\n",phaseptr);
		fprintf(stderr,"rx1 = %lf\n",rx1);
		fprintf(stderr,"rx2 = %lf\n",rx2);
	}

 if (pulsewidth > 0.0) 
 {
	validate_imaging_config("S_shapedpulse");

	/* find pulse in pulse shape table */

        okinhwloop();
        txphase(phaseptr);	/* set xmtr phase */
	HSgate(RFP270,FALSE);	/* rf phase = zero */
	HSgate(RXOFF,TRUE);
	/* Ensure a minimum receiver gating time in order to latch in 	*/
	/* the receiver gating.						*/
	if (rx1 < MIN_DELAY)
		rx1 = MIN_DELAY;
	G_Delay(DELAY_TIME,rx1,0);
	/* It takes 14us to set rf pattern ptr 6/18/86 */

	putcode(SETSHPTR);
	putcode(OBSAPADR);		/* obsaddr 3072 = c00	*/
	/**************************************************************
	* Look for shape in table.  If present, insert pointer to 
	* the shape; otherwise, get shape template and generate a
	* pattern to be downloaded, and insert new shape in table.
	***************************************************************/
	/* rfpatfilptr is an external initialized by initshapedpatterns */
	/* otptr is an external initialized by initshapedpatterns */
	if (bgflag)
		fprintf(stderr,"calling insertpattern\n");
	patptr = insertpattern(pulsefile,pulsewidth,rfpatfilptr,otptr);
	if (bgflag)
		fprintf(stderr,"shapepulse ptr = %d\n",patptr);
	putcode(patptr);
	HSgate(VAR1,TRUE);
	/**************************************************************
	* the time for an event can be up to 10-4 less than desired  
	* time this must be accounted for so the pulse will completely 
	* finish and not leave any steps unaccounted. 
	***************************************************************/
	twidth = pulsewidth + (pulsewidth*9e-5) + 1e-6;
	G_Delay(DELAY_TIME,twidth,0);
	HSgate(VAR1,FALSE);
	G_Delay(DELAY_TIME,rx2,0);
	if (!rcvroff_flag)		/* turn receiver on if not globally */
           HSgate(RXOFF, FALSE);		/* turned off	*/
 }	/* if width > 0.0 */
 return;
}	/* end shapedpulse */

void S_decshapedpulse(char *pulsefile, double pulsewidth, codeint phaseptr,
                   double rx1, double rx2) 
/************************************************************************
* Arguments: "
*	pulsefile	: filname of shaped pulse in shplib 
*	pulsewidth	: width of pulse (in sec) 
*	phaseptr	: phase (actual phase will be selected by a 
*				modulo four of this argument). 
*	rx1,rx2		: prepulse & postpulse amplifier gate on parms 
* 							
* NOTE: Because of delays in the rf phase shifter there must be 
*       at least 15us between pattern pulses.  This is taken care 
*       of by setting the pointer before each pulse (14.2us), and  
*       adding 1us of delay. 
* NOTE: The decshapedpulse does not provide any receiver gating.
*	This has been changed 4/24/91 to provide receiver gating. M. Howitt
*************************************************************************/
{
 double twidth;
 codeint patptr;
 if (pulsewidth > 0.0) 
 {
	validate_imaging_config("S_decshapedpulse");

	/* find pulse in pulse shape table */

        okinhwloop();
        decphase(phaseptr);	/* set decoupler phase */
	HSgate(DC270,FALSE);	/* rf phase = zero */
	HSgate(RXOFF,TRUE);
	sisdecblank(OFF);
	G_Delay(DELAY_TIME,rx1,0);
	/* It takes 14us to set rf pattern ptr 6-18-86 */
	putcode(SETSHPTR);
	putcode(DECAPADR);		/* obsaddr 3080 = c08*/
	/**************************************************************
	* Look for shape in table.  If present, insert pointer to 
	* the shape; otherwise, get shape template and generate a
	* pattern to be downloaded, and insert new shape in table.
	***************************************************************/
	patptr = insertpattern(pulsefile,pulsewidth,rfpatfilptr,dtptr);
	if (bgflag)
		fprintf(stderr,"decshapepulse ptr = %d\n",patptr);
	putcode(patptr);
	HSgate(VAR2,TRUE);
	/**************************************************************
	* the time for an event can be up to 10-4 less than desired  
	* time this must be accounted for so the pulse will completely 
	* finish and not leave any steps unaccounted. 
	***************************************************************/
	twidth = pulsewidth + (pulsewidth*9e-5) + 1e-6;
	G_Delay(DELAY_TIME, twidth, 0);
	HSgate(VAR2,FALSE);
	G_Delay(DELAY_TIME, rx2, 0);
	sisdecblank(ON);
	if (!rcvroff_flag)		/* turn receiver on if not globally */
           HSgate(RXOFF, FALSE);		/* turned off	*/
 }	/* if width > 0.0 */
 return;
}	/* end shapedpulse */


void S_simshapedpulse(char *fno, char *fnd, double transpw, double decpw,
                 codeint transphase, codeint decupphase,
                 double rx1, double rx2)
/************************************************************************
* Arguments: 
*	fno,fnd		: filname of shaped pulse in shplib 
*			: if filname is blank a square pulse is assumed
*	wo,wd		: width of pulse (in sec) 
*	pho,phd 	: phase (actual phase will be selected by a 
*				modulo four of this argument). 
*	rx1,rx2		: prepulse & postpulse receiver gate on parms 
* 
* NOTE: Because of delays in the rf phase shifter there must be 
*       at least 15us between pattern pulses.  This is taken care 
*       of by setting the pointer before each pulse (14.2us), and  
*       adding 1us of delay. However in this case if both are shaped
*	 the delay will be 28.4us to set the pointers. So the delay 
*	 time before the pulse will be either 15.2us or 28.4us depending
*	 on whether one or both will be shaped pulses.
* 
* the time for an event can be up to 10-4 less than desired time 
* this must be accounted for so the pulse will completely finish 
* and not leave any steps unaccounted. 
*************************************************************************/
{
 double centertime;
 double	twidth,two,twd;
 int	ogate,dgate;
 codeint patptr;
 char	*pb = "";

if ((transpw > 0.0) && (decpw > 0.0)) 
 {
	validate_imaging_config("S_simshapedpulse");

	okinhwloop();
	decphase(decupphase);	/* set decoupler phase */
	HSgate(DC270,FALSE);
	txphase(transphase);	/* set xmtr phase */
	HSgate(RFP270,FALSE);
	sisdecblank(OFF);
 	HSgate(RXOFF,TRUE); 
	twidth = 0.0;
	two = transpw; 
	twd = decpw;
	if (strcmp(fno,pb) > 0) 
	{
		ogate = VAR1;
		two = two + (two*9e-5) + 1e-6;
	}
	else
		ogate = TXON;
	if (strcmp(fnd,pb) > 0)
	{
		dgate = VAR2;
		twd = twd + (twd*9e-5) + 1e-6;
	}
	else
		dgate = DECUPLR;

	/* set fifo ptr to start of shaped pulse	*/
	/* Ensure a minimum receiver gating time in order to latch in 	*/
	/* the receiver gating.						*/
	if (rx1 < MIN_DELAY)
		rx1 = MIN_DELAY;
	G_Delay(DELAY_TIME, rx1, 0);
	if (ogate == VAR1) 
	{
		putcode(SETSHPTR);
		putcode(OBSAPADR);		/* OBSAPADR 3072 = c00	*/
		/* find shape in shape table and 
		*  if array index = 1 point to acode location for sndpat
		*  to fill in fifo location.
		*  else fill in fifo location.*/
		patptr = insertpattern(fno,transpw,rfpatfilptr,otptr);
		putcode(patptr);
	}
	else
	{
		/* check that receiver is gated off for 10us before 	*/
		/* start of obs pulse.					*/
		if ( ((two >= twd) && (rx1 < MINROF1)) || 
		     ((two < twd) && ( ((twd-two)/2)+rx1 < MINROF1 )) )
   		{
		  text_error
                    ("Error: Obs mplifier blanking delay < 10us minimum.\n");
		  text_error("       Check rof1 or specific blanking delay\n");
		psg_abort(1);
		}
	}
	if (dgate == VAR2) 
	{
		putcode(SETSHPTR);		/* 83"setshptr"	*/
		putcode(DECAPADR);		/*  DECAPADR 3080 = c08	*/
		patptr = insertpattern(fnd,decpw,rfpatfilptr,dtptr);
		putcode(patptr);
	}

	/* gate pulses on and off	*/
 	if (twd > two) 
  	{
		centertime = (twd - two)/2.0;
   		HSgate(dgate,TRUE);  
		G_Delay(DELAY_TIME, centertime, 0);
		HSgate(ogate,TRUE);   
		G_Delay(DELAY_TIME, two, 0);
   		HSgate(ogate,FALSE);  
		G_Delay(DELAY_TIME, centertime, 0);
   		HSgate(dgate,FALSE);
		twidth=centertime+centertime+two;
  	}
 	else
  	{
		centertime = (two - twd)/2.0;
   		HSgate(ogate,TRUE);   
		G_Delay(DELAY_TIME, centertime, 0);
   		HSgate(dgate,TRUE);  
		G_Delay(DELAY_TIME, twd,0);
   		HSgate(dgate,FALSE); 
		G_Delay(DELAY_TIME, centertime, 0);
   		HSgate(ogate,FALSE);
		twidth=centertime+centertime+twd;
  	}
	G_Delay(DELAY_TIME, rx2, 0);
	sisdecblank(ON);
	if (!rcvroff_flag)		/* turn receiver on if not globally */
           HSgate(RXOFF, FALSE);		/* turned off	*/
 }	/* if width > 0.0	*/
 return;
}	/* end simshpul		*/

insertpattern(pulsefile,pulsewidth,opfile,tblptr)
/************************************************************************
* Insertpattern searches shape pulse table for pulse. If found,  
* it assigns  the pattern memory pointer in the table to the acode 
* position.  If not found, it generates the pattern and inserts this
* new entry into the shape table. 
* FAILURE OF THIS FUNCTION SHOULD ABORT PULSE SEQUENCE GENERATION !!
*************************************************************************/
char	*pulsefile;
double	pulsewidth;
struct	outpatfile *opfile;
struct	shapetbl *tblptr;
{
	int	patternptr;
	
	if (bgflag) {
		fprintf(stderr,"insertpattern\n");
		fprintf(stderr,"pat= %s width = %lg\n",pulsefile,pulsewidth);
	}
	if (bgflag)
		fprintf(stderr,"otptr = %lx\n",tblptr);
	
	if ((patternptr = patsrch(pulsefile,pulsewidth * 1.0e6,tblptr))
	== PATERR)
		patternptr = genpattern(pulsefile,pulsewidth * 1.0e6,opfile,tblptr);
		
	if (bgflag)
		fprintf(stderr,"insertpattern patternptr = %d\n",patternptr);

	if (patternptr == PATERR)
		psg_abort(1);

	return(patternptr); /* 0 IS A VALID RETURN VALUE! */
}

genpattern(pulsefile,pulsewidth,opfile,tblptr)
/************************************************************************
* genpattern calls rdpulsefile to read in a pulse pattern template.
* It then calculates the unittime for each timeunit in the pulse template
* and calls genpatpts to generate a pulse and write the pulse into the
* experiment pattern file.  Genpattern then updates the pattern table with 
* the size and pointer location of the generated pulse.
* inputs:	
*  pulsefile -	pattern file name.
*  pulsewidth - width of the pattern to be generated.
*  patternfile -descripter to file that contains all the generated patterns 
*		that will be used in the experiment acquisition.
*  tblptr -	pointer to the table of patterns to be downloaded.
* output:	The pointer location of the pattern in pattern memory.
*		If there are errors a PATERR is returned.
*************************************************************************/
char	*pulsefile;
double	pulsewidth;
struct	outpatfile *opfile;
struct	shapetbl *tblptr;
{
	int	npatunits,patternptr,tsize;
	double unittime;

	if (bgflag) {
		fprintf(stderr,"genpattern\n");
		fprintf(stderr,"pat= %s width = %lg\n",pulsefile,pulsewidth);
	}
	
	/* read in shape (template) file */
	if ((npatunits = rdpulsefile(pulsefile)) == PATERR)
		return(PATERR);

	unittime = pulsewidth/npatunits;
	npatunits = genpatpts(unittime,opfile);
	if (npatunits <= 0)
		return(PATERR);

   	delarrayno = arrayno - DELARRAY;	/* WHAT IS GOING ON HERE? */
   	if (delarrayno < 0) delarrayno = 0;

   	patternptr = patalloc(npatunits,pulsefile,pulsewidth,tblptr);
   	if (patternptr == PATERR) {
		text_error("paterr: no space in memory\n");
		return(PATERR);
   	}
   
   	/* insert header values and write out the file */
	/* THE FOLLOWING INIT IS DONE IN genpatpts! */
/*   	hdrptr = (struct dataheader *) opfile->mptr; */
   	hdrptr->apadr = tblptr->apadr;
   	hdrptr->patptr = patternptr;
   	hdrptr->patwords = npatunits;
	hdrptr->spare = 0x4242; /* DEBUG ONLY */
   	tsize = ((npatunits)*(sizeof(long)))
	+ sizeof(struct dataheader);
   	if (write(opfile->fd,opfile->mptr,tsize) != tsize) {
		text_error("RF write failed\n");
		return(PATERR);
   	}
	/* adjust record of file size */
   	opfile->size = opfile->size + tsize;
   	free(opfile->mptr);
	opfile->mptr = NULL; /* avoid dangling pointers! */
	return(patternptr);
}

rdpulsefile(pulsefile)
/************************************************************************
* Rdpulsefile reads the pattern pulse file into memory and sums
* the total number of time units in the pattern.
* Returns the total number of time units in the pattern, or PATERR if template
* file empty, or PATERR if error in opening or closing template file
* also updates the global count of points in the pattern file
*************************************************************************/
char	*pulsefile;
{
	FILE	*fd;
	int	i,ip,unitcnt;
	char *templatedir;
	char *getenv();

	/* form path name to local pulse shape library */
	templatedir = getenv("vnmruser"); /* $vnmruser */
	if (templatedir == NULL) {
		text_error("can't get vnmruser from environment\n");
		return(PATERR);
	}
	strcpy(pulsepath,templatedir);
	strcat(pulsepath,"/shapelib/");
	/* form absolute path name of template file */
	strcat(pulsepath,pulsefile);
	strcat(pulsepath,".RF");
	if (bgflag) {
		fprintf(stderr,"pulse path = %s\n",pulsepath);
		fprintf(stderr,"opening pulse file %s\n",pulsepath);
	}
	fd = fopen(pulsepath,"r");
	if (fd == NULL) {
		if (bgflag)
			fprintf(stderr,"pat warning: file %s not opened\n",pulsepath);
		/* form path name to global pulse shape library */
		templatedir = getenv("vnmrsystem"); /* $vnmrsystem */
		if (templatedir == NULL) {
			text_error("can't get vnmrsystem from environment\n");
			return(PATERR);
		}
		strcpy(pulsepath,templatedir);
		strcat(pulsepath,"/shapelib/");
		/* form absolute path name of template file */
		strcat(pulsepath,pulsefile);
		strcat(pulsepath,".RF");
		if (bgflag) {
			fprintf(stderr,"pulse path = %s\n",pulsepath);
			fprintf(stderr,"opening pulse file %s\n",pulsepath);
		}
		fd = fopen(pulsepath,"r");
		if (fd == NULL) {
			text_error("can't find pulse template file\n");
			return(PATERR);
		}
	}
	
	i = 0;
	ip = 0;
	unitcnt = 0;
	patpntcnt = 0;
	if ((inpatptr[ip] = (struct patinarray *) malloc(sizeof(struct patinarray))) == NULL) {
		text_error("input pattern file malloc failed\n");
		return(PATERR);
	}
	while (getpatline(fd, ip, i) != EOF) {
		/* running sum of unitcount */
		unitcnt += inpatptr[ip]->patpnt[i].unitcnt;
		/* running sum of points in pattern */
		patpntcnt++;
		i = ++i % INPUTPATTRN_ARRAYSIZE;
		if (i == 0) {
			/* allocate next patinarray */
	   		ip++;
		   	if ((inpatptr[ip] = (struct patinarray *) 
			malloc(sizeof(struct patinarray))) == NULL) {
				text_error("input pattern file malloc failed\n");
				return(PATERR);
		   	}
		}
	}

	if (bgflag)
		fprintf(stderr,"closing pulse file %s\n",pulsepath);

	if (fclose(fd) == EOF) {
		text_error("input pattern file close failed\n");
		return(PATERR);
	}

	if (unitcnt <= 0) {
		text_error("input pattern file empty\n");
		return(PATERR);
	}

	return(unitcnt);
}

genpatpts(unittime,opfile)
/*************************************************************************
* Genpatpts actually generates the pattern words to be loaded into the
* acquisition pattern memory.  It tags on a 800ns word with zero amplitude
* and the same phase as the first point at the beginning.  At the end,
* a stop pattern word(xffe00000) is tagged on.  After a sector of points
* is filled, it is written out to the pattern file that will be sent to
* the acquisition computer.  The memory that contains the pattern template
* is freed after its points have been generated.
* 
* Returns the number of pattern words that have been generated if
* successful, and PATERR if there are errors.
*************************************************************************/
double	unittime;
struct	outpatfile *opfile;
{
	double	remtim;
	int	i,ip,ia,patternsiz,tnum,j;
	unsigned long *outpatptr;
	int	iamplitude,iphase;
	struct ptimecodes tpat[TIMTBLSIZE];


	if (bgflag) {
		fprintf(stderr,"genpatpts\n");
		fprintf(stderr,"unitim=%lg size=%d\n",unittime,patpntcnt);
	}
	
	patternsiz = 0;		/* start of new pattern 	*/

	i = -1;
	ia = -1;

	ip = 0;
	remtim = 0.0;
	/* opfile is rfpatfilptr at this point */
	/* malloc space for pattern header (not file header!) and pattern
	data */
	opfile->mptr = (unsigned long *) malloc(((PATWRDMAX) * sizeof(long)) +
				sizeof(struct dataheader));
	if (opfile->mptr == NULL) {
		text_error("output pattern file malloc failed\n");
		return(PATERR);
	}
	/* initialize ==> header */
	hdrptr = (struct dataheader *) opfile->mptr;
	/* initialize ==> pattern info */
	outpatptr = (unsigned long *)((char *)hdrptr + sizeof(struct dataheader));


	while (i <= patpntcnt) {
		/* add 1us word of zero amplitude to start pattern to account
		for delay in phase shifter */
		if (i < 0) {
	   		tnum = 1;
	   		iamplitude = 0;
	   		iphase = chgdeg2b(inpatptr[0]->patpnt[0].phase);
	   		tpat[0].timecnt = 0;
	   		tpat[0].timedur = 1;		/* 1 usec duration */
		}
		/* add stopcode to the end of a pattern */
		else if (i == patpntcnt) {
	   		tnum = 1;
	   		iamplitude = 0;
	   		iphase = 0;
	   		tpat[0].timecnt = 511;
	   		tpat[0].timedur = 3;		/* all timer bits on */
		}
		else {
			iamplitude = inpatptr[ip]->patpnt[ia].amplitude;
			iphase = chgdeg2b(inpatptr[ip]->patpnt[ia].phase);
			remtim = unittime * inpatptr[ip]->patpnt[ia].unitcnt + remtim;
			if ((tnum = scaletim(tpat,&remtim)) == PATERR)
				return(PATERR);
		}
		j = 0;
		/*if (ia >= 0)
		*if (bgflag)
		*	fprintf(stderr,"phase: %f amplitude: %f\n",inpatptr[ip]->patpnt[ia].phase,
		*		inpatptr[ip]->patpnt[ia].amplitude);
		*/
		while (tnum > 0) {
			/* insert each point definition field into the
			patternword	*/
			*outpatptr = 0;
			insbitl(iphase,outpatptr,ph_loc,ph_len);
			insbitl(iamplitude,outpatptr,amp_loc,amp_len);
			insbitl(tpat[j].timecnt,outpatptr,tc_loc,tc_len);
			insbitl(tpat[j].timedur,outpatptr,td_loc,td_len);
			tnum--;
			j++;
			patternsiz++;
			if ( patternsiz > PATWRDMAX) {
				text_error("Pattern memory limit exceeded\n");
				return(PATERR);
			}
			outpatptr++;
		}
		ia = ++i % INPUTPATTRN_ARRAYSIZE;
		if ((i > 0) && (ia == 0)) {
			free(inpatptr[ip]);
			inpatptr[ip] = NULL; /* avoid dangling pointers! */
			ip++;
		}
	}
	if (ia != 0) {
		free(inpatptr[ip]);
		inpatptr[ip] = NULL; /* avoid dangling pointers! */
	}
	if (patternsiz <= 0)
		text_error("genpatpts return value is FUBAR!\n");

	return(patternsiz);
}

insbitl(srcwrd, dstwrd, startbit, nbits)
/************************************************************************/
/* Description:								*/
/* 	insbitl inserts nbits from bit location 0 in srcwrd into 	*/
/* 	startbit location in dstwrd.					*/
/************************************************************************/
#define maxbits  32
long	srcwrd;
long	*dstwrd;
int	startbit, nbits;
{
long	msk;
int	temp;

temp = startbit + nbits;
if (0 <= temp & temp <= maxbits) {
	srcwrd = srcwrd << startbit;
	msk = (~0) >> (maxbits - temp);
	*dstwrd = (srcwrd & msk) | *dstwrd;
	}
return;
}

/* chgdeg2b(rphase) */
/************************************************************************
* chgdeg2b will change the input from degrees to bits.  It needs to
* make the change in accordance with the way the ram is set up.  The
* 9,8 msbs are the 180,90 deg lines. The 0-7 are 0.5 deg increments
* up to 89.5 deg which means 179 bits out of 255.
* It returns an integer value.
*************************************************************************/
/*float	rphase;
 *{
 * int iphase;
 * iphase = ((long) ((rphase + 0.25)*10)) % 3600;
 * if (iphase < 0) iphase = 3600 + iphase;
 * iphase = ((iphase/900)*256) + ((iphase % 900)/5);
 * return(iphase);
 *}
*/

scaletim(tpat,timeperiod)
/************************************************************************
* scaletime the unit duration time and calculates the timecnt and
* timedur.
* NOTE: times are in microsecs.
*************************************************************************/
struct	ptimecodes *tpat;
double	*timeperiod;
{
 int i,notdone;

 notdone = TRUE;
 i = 0;
 while ( notdone && (i < TIMTBLSIZE))
 {
	if (*timeperiod >= (2.0*TBNS))
	{
	   if (*timeperiod < 51.2) 
	   {
		tpat->timecnt = gettimedur(timeperiod,TBNS);
		tpat->timedur = 0;			/* 100 ns */
	   }
	   else if (*timeperiod < 512.0) 
	   {
		tpat->timecnt = gettimedur(timeperiod,TBUS);
		tpat->timedur = 1;			/* 1 us */
	   }		
	   else if (*timeperiod < 5120.0) 
	   {
		tpat->timecnt = gettimedur(timeperiod,TB10US);
		tpat->timedur = 2;			/* 10 us */
	   }		
	   else if (*timeperiod < 51100.0) 
	   {
		tpat->timecnt = gettimedur(timeperiod,TB100US);
		tpat->timedur = 3;			/* 100us */
	   }
	   else
	   {
		*timeperiod = *timeperiod - 51100.0;
		tpat->timecnt = 511;
		tpat->timedur = 3;			/* 100us */
	   }
	   /* subtract on timecnt to make up the patternboard	*/
	   /* hardware adding one timecnt			*/
	   tpat->timecnt = tpat->timecnt - 1;
	   i++;
	   tpat++;
	}
	else notdone = FALSE;
 }
 if (i == TIMTBLSIZE)
 {
	text_error("pattern point exceeds maximum time for a single entry\n");
	return(PATERR);
 }
 if (i == 0) {
	text_error("pulsewidth smaller than pattern size\n");
 	return(PATERR);
 }
 return(i); /* 0 < i < TIMTBLSIZE */
}

gettimedur(timeperiod,timebase)
double	*timeperiod;
double	timebase;
{
 int timecnt;
 timecnt = (*timeperiod/timebase) + 0.005;
 *timeperiod = *timeperiod - (timecnt*timebase);
 return(timecnt);
}
  
	 		
 
/************************************************************************
* link list routines
*************************************************************************/
/************************************************************************
 * initpl(tblptr);
 *
 * initpl initializes the free and used lists. It initializes
 * a free list entry with the first Pat Memory address  and sets
 * the bytecount to the size of accessible memory. Head & tail
 * are also set.
 *
 ************************************************************************/

initpl(tblptr)
struct shapetbl *tblptr; 
{	
	struct pmlist  *pl;

	pl = (struct pmlist *)malloc(sizeof(struct pmlist));
	if (pl == NULL)
		return (PATERR);


	pl->type = FREE;	/* create first entry in the list to init */
	if (bgflag)
		fprintf(stderr,"initpl hostmem = %lx mem_size = %ld\n",pl,pl->wordcount);
	pl->patadr = 0;
	pl->arrayno = 0;
	pl->wordcount = PATWRDMAX;
	/* no initializations of pulsefile or pulsewidth... */
	pl->flink = NULL;

	tblptr->plhead = pl;	/* it is the head of the linked list 	*/
	/*for (i = 0; i < HASHSIZE; i++)
		hashtab[i] = 0; */


	return(OK);	
}	  


/***********************************************************************
 * patalloc (size,pulsefile,pulsewidth,tblptr)
 *      size   - number of elements in pattern
 *     
 * Errors:
 *   Will not fit in memory
 *
 * Implementation:
 *   patalloc allocates the specified pattern in pattern board memory.
 *   The memory management list is searched for the 
 *   first free block of memory in which the pattern will fit. 
 *   Upon placement in memory, an entry into a linked list is made
 *   that contains physical address, and element size. 
 *
 **********************************************************************/

patalloc(size,pulsefile,pulsewidth,tblptr)
int     size;
char	*pulsefile;
double	pulsewidth;
struct shapetbl *tblptr; 
{
	struct pmlist  *pl;	/* memory map list pointer	    */
	struct pmlist  *new;	/* new entry into memory map list   */
	struct pmlist  *prev;	/* previous entry into mem map list */

	int    diff;		/* difference between new entry and existing */
	int   resolved;		/* flag whether entry is found	 */
	int    vector,delpat;	/* returns ap address		 */


	if (bgflag)
		fprintf(stderr,"patalloc words = %d\n",size);
	/*  word align all allocations */
	vector = PATERR;
	resolved = FALSE;

	pl = tblptr->plhead;
	do {
		if (bgflag)
			fprintf(stderr," memsize = %d\n",pl->wordcount);

		if ((pl->wordcount >= size) && (pl->type == FREE)) {
			diff = pl->wordcount - size;
	
			vector = pl->patadr;
			if (bgflag)
				fprintf(stderr," allocated memptr = %x\n",vector);

			new = (struct pmlist *)malloc(sizeof(struct pmlist));
			if (new == NULL)
				return(PATERR);

			new->type = USED;
			new->patadr = pl->patadr;
			new->arrayno = arrayno;
			new->wordcount = size;
			strcpy(new->pulsefile,pulsefile);
			new->pulsewidth = pulsewidth;

			/* insert new node ahead of current (pl) node! */
			if (tblptr->plhead == pl)
				tblptr->plhead = new;
			else
				prev->flink = new;
				/* NOTE WELL THAT prev IS UNINITIALIZED THE
				FIRST TIME THROUGH THIS DO-WHILE LOOP!! */

			if (diff == 0) {
				/* used up the whole current node */
				new->flink = pl->flink;
				/* free the unlinked node!! */
				free(pl);
				/* POSSIBLE dangling PTR ?? */
			}
			else {
				new->flink = pl;
				pl->patadr = pl->patadr + size;
				pl->wordcount = diff;
			}
			resolved = TRUE;
		}
		else {			/* It  won't fit  -- try again  */
			prev = pl;
			pl = pl->flink;
		}
	} while ((resolved != TRUE) && (pl != NULL));

	if (resolved != TRUE) {
		/* delete all the patterns not used in last array	*/
		/* DOES THIS FORCE RELOADING OF THE PATTERN BOARD? */
		pl = tblptr->plhead; 
		if ((arrayno > delarrayno) && (vector == PATERR)) {
			while (pl != NULL) {
	        		if (pl->arrayno < delarrayno) {
					delpat = pl->patadr;
					pl = pl->flink;
					if (patdeall(delpat,tblptr) == PATERR)
						return(PATERR);
				}
	        		else
					pl = pl->flink;
			}
			delarrayno++;
			/* RECURSION LIMITED BY DECREMENTING GLOBAL
			delarrayno?? */
			vector = patalloc(size,pulsefile,pulsewidth,tblptr);
		}
	}
	return(vector);
}

patdeall(patadr,tblptr)
/***********************************************************************
* seaches linked list to find the entry associated with the pattern 	
* address.  It then deletes the entry.
***********************************************************************/

int    patadr;
struct shapetbl *tblptr; 
{

	struct pmlist  *pl;
	struct pmlist  *prev;
	struct pmlist  *next;

	prev = NULL;
	pl = tblptr->plhead;

	/* the list must be searched to find the host address, keeping 
	track of the backlink */

	if (patadr < 0) {
		text_error("patdeall with patadr < 0\n");
		return(PATERR);
	}


	while ((pl->patadr != patadr) && (pl->flink != NULL)) {
		prev = pl;
		pl = pl->flink;
	}

	if ((pl->patadr != patadr) && (pl->flink == NULL)) {
		/* should NOT be REACHED */
		sprintf(paterrmsg,"paterr : patadr = %x not found\n",patadr);
		text_error(paterrmsg);
		return(PATERR);
	}

	/* change the node into a free node- remove template name and width */
	pl->type = FREE;
	/* leave pl->patadr intact! */
	pl->arrayno = 0;
	/* leave pl->wordcount intact! */
	pl->pulsefile[0] = '\0';
	pl->pulsewidth = 0.0;
	/* leave pl->flink intact! */


	next = pl->flink;	/* save forward link */
	if (next != NULL) {
		/* current node not tail of list */
		if (next->type == FREE) {    
			/* "current node" = "current node" and "next node" */
			pl->wordcount += next->wordcount;
			pl->flink = next->flink;
			if (bgflag)
				fprintf(stderr,"free next = %x\n",next);
			free(next);
		}
	}

	/* node "next" is guaranteed not to be FREE at this point! */
	if (prev != NULL) {
		/* current node not head of list */
		if (prev->type == FREE) {
			/* "previous node" = "previous node" & "current node" */
			prev->wordcount += pl->wordcount;
			prev->flink = pl->flink;
			if (bgflag)
				fprintf(stderr,"free current = %x\n",pl);
			free(pl);
		}
	}
	return(OK);
}

/*
find active node with specified shape and width and return its address (>= 0)
or PATERR if not found
NOTE WELL THAT ONLY THE RELATIVE PATH NAME OF THE TEMPLATE FILE, NOT THE
ABSOLUTE PATH NAME, IS COMPARED; THUS IF A TEMPLATE FILE EXISTS IN THE
GLOBAL TEMPLATE DIRECTORY AND NOT IN THE LOCAL TEMPLATE DIRECTORY AT THE
BEGINNING OF PULSE SEQUENCE GENERATION FOR AN EXPERIMENT THERE IS THE
POSSIBILITY THAT THE SAME PULSE SHAPE (RELATIVE PATHNAME) WITH MORE THAN ONE
PULSE WIDTH MAY USE TWO DIFFERENT TEMPLATES IF A TEMPLATE FILE OF THE SAME
NAME IS CREATED IN THE LOCAL TEMPLATE DIRECTORY DURING PULSE SEQUENCE
GENERATION FOR THAT EXPERIMENT!!
*/
patsrch(pulsefile,pulsewidth,tblptr)
char	*pulsefile;
double	pulsewidth;
struct shapetbl *tblptr; 
{
	int	patadr;
	struct pmlist  *pl;

	if (bgflag)
		fprintf(stderr,"patsrch:\n");

	for (pl = tblptr->plhead; pl != NULL; pl = pl->flink) {
		if (pl->type != USED) 
			continue;
	   	if (!strcmp(pulsefile,pl->pulsefile) &&
	     	(pulsewidth >= pl->pulsewidth - (pl->pulsewidth*1e-6)) &&
	     	(pulsewidth <= pl->pulsewidth + (pl->pulsewidth*1e-6)) ) {
	      		patadr = pl->patadr;
	      		pl->arrayno = arrayno;
			if (bgflag)
				fprintf(stderr,"pattern found in linked list\n");
			return(patadr); /* now returns 1st occurrance of
			matching node rather than last occurrance of
			matching node as was previously (inadvertently?)
			done */
	   	}
	}
	/* pattern not found */
	if (bgflag)
		fprintf(stderr,"pattern not found in linked list\n");
	return(PATERR);
}
 


/* this is an expansion of a fscanf in rdpulsefile which allows the
commenting out of lines in the RF template file via a COMMENTCHAR
(currently '#') in the first column of a line.  It returns EOF or !EOF
depending on whether the end of the file has been reached or not */
getpatline(fd, ip, i)
FILE *fd;
int ip;
int i;
{
	char col1;

	do {
		col1 = getc(fd);
		if (col1 == EOF)
			return(EOF);
		else if (col1 != COMMENTCHAR) {
			ungetc(col1, fd);	/* replace the character */
			fscanf(fd,"%f%f%f\n",&inpatptr[ip]->patpnt[i].phase,
			     &inpatptr[ip]->patpnt[i].amplitude,
			     &inpatptr[ip]->patpnt[i].unitcnt);
			return(!EOF);
		}
		else {	/* a comment line; eat it up, including \n */
			do {
				col1 = getc(fd);
			} while (col1 != '\n');
		}
	} while (1);
}



/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/************************************************************************
* freq_list.c
*	Routines that will generate and use global lists of acodes as
*	for realtime variable access.
*************************************************************************/


#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#ifdef AIX
#include <sys/ioctl.h>
#else
#include <sys/fcntl.h>
#endif

#include "vnmrsys.h"
#include "acodes.h"
#include "oopc.h"
#include "ACQ_SUN.h"
#include "acqparms.h"
#include "group.h"
#include "rfconst.h"
#include "aptable.h"
#include "abort.h"
#include "macros.h"




#define OK	0	
#define SYSERR	-1	/* compatible with most UNIX error returns */
#define COMMENTCHAR '#'	/* character that signifies the start of a comment */
#define	MODE 0666

#define HEADER_ARRAYSIZE 256
#define MAXGLOBALSEG 10
#define MAXFREQVALS 2048
#define MAXFREQACODE 24
#define MAXDELAYVALS 2048
#define MAXDELAYACODE 6

extern int newacq;
extern int acqiflag;		/* True if acodes are for Acqi */
extern int bgflag;		/* print diagnostics if not 0 */
extern int rcvroff_flag;	/* global receiver off flag */
extern int decblankoff_flag;	/* global decoupler blanking flag */
extern char filexpath[];	/* ex: $vnmrsys/acqque/exp1.russ.012345 */
extern char filexpan[];	        /* absolute path name of xpansion load file */
extern char fileRFpattern[];    /* absolute path name of RF load file */
extern int freq_global_table;
extern int gen_apbcout();
extern char *ObjError(int wcode);

/************************
*  pattern globals	*
*************************/

struct outpatfile {		/* output file descriptor structure	*/
	int	fd;		/* unit number		*/
	codelong *hptr;		/* header pointer 	*/		
	codeint *mptr;		/* data pointer		*/
	int	size;		/* size			*/
	} ;

struct acode_globindx {
	int	gseg;		/* number of global segment to use */
	int	listcnt;
				/* number of acodes for each frequency 	*/
				/* setting.				*/
	int	acodes_forelem[HEADER_ARRAYSIZE];
	int	tblnum[HEADER_ARRAYSIZE];
	} gfreq = { -1, 0};

struct	outpatfile freqfildesc;
struct	outpatfile *freqfilptr;

codeint	*GTABptr[MAXGLOBALSEG];
codeint *GtabStart,*GtabEnd;

/*** varibles for use with psg arrayed experiments	***/
codelong  *p_frqfilhdrpos;


/*  Used by the Qtune program ...  */

int
get_maxfreqacode()
{
	return( MAXFREQACODE );
}

/************************************************************************
*	GLOBAL LIST ELEMENTS - mrh
*************************************************************************/
int get_freqlist_table()
{
	return(gfreq.gseg);
}


init_global_list(globaltable)
/************************************************************************
* init_global_list needs to be called once at the beginning of an
* experiment acquisition.  It initializes the freq tables used  
* in the acquisition.
*************************************************************************/
int globaltable;	
{
 int	i;
 int	mask;
 char paterrmsg[MAXPATHL];

 /*---------------------------------------------------------------------*/
 /* Initialize variables to be used in pulse sequence elements.		*/
 /*---------------------------------------------------------------------*/

 gfreq.gseg = globaltable;
 gfreq.listcnt = 0;
 for (i=0; i<HEADER_ARRAYSIZE; i++)
	gfreq.acodes_forelem[i] = 0;
 
 if (globaltable < 0)	return(1);

 /*****						*********/
 /* initialize output file and header for patterns	*/
 /*****						*********/

 freqfilptr = &freqfildesc;
 freqfilptr->size = ((256+2) * sizeof(codelong));
 freqfilptr->hptr = (codelong *)malloc(freqfilptr->size);
 if (freqfilptr->hptr == NULL)
 {
	text_error("could not allocate memory for RF pattern file header\n");
	psg_abort(1);
 }
 p_frqfilhdrpos = freqfilptr->hptr;
 *p_frqfilhdrpos++ = freqfilptr->size;	/* current file size */
 *p_frqfilhdrpos++ = freqfilptr->size;	/* current header size */

 /*** Do not create file if INOVA system ***/
 if (newacq) return(OK);

 /* form load file absolute path name */
 if (gfreq.gseg == ACQ_XPAN_GSEG)
 {
 	strcpy(filexpan,filexpath);
 	strcat(filexpan,".XPAN"); /* $vnmrsystem/acqqueue/exp1.russ.012345.RF */
 	/* use either O_EXCL or O_TRUNC */
 	if (acqiflag){
 	    /* RF pattern file may already exist. That's OK, we'll trash it. */
 	    mask = umask(002);		/* Write access for group */
	    freqfilptr->fd = open(filexpan,O_RDWR | O_CREAT | O_TRUNC, MODE);
 	    umask(mask);
 	}else{
 	    freqfilptr->fd = open(filexpan,O_RDWR | O_CREAT | O_EXCL, MODE);
 	}
 	if (freqfilptr->fd == SYSERR)
 	{
	   sprintf(paterrmsg,"init_global_list: file %s not opened\n",filexpan);
	   text_error(paterrmsg);
	   psg_abort(1);
 	}
 }
 else if (gfreq.gseg == ACQ_RF_GSEG)
 {
	strcpy(fileRFpattern,filexpath);
 	strcat(fileRFpattern,".RF");	
				/*$vnmrsystem/acqqueue/exp1.russ.012345.RF */
 	/* use either O_EXCL or O_TRUNC */
 	if (acqiflag){
 	    /* RF pattern file may already exist. That's OK, we'll trash it. */
 	    mask = umask(002);		/* Write access for group */
	    freqfilptr->fd = 
		open(fileRFpattern,O_RDWR | O_CREAT | O_TRUNC, MODE);
 	    umask(mask);
 	}else{
 	    freqfilptr->fd = 
		open(fileRFpattern,O_RDWR | O_CREAT | O_EXCL, MODE);
 	}
 	if (freqfilptr->fd == SYSERR)
 	{
	    sprintf(paterrmsg,
		 "init_global_list: file %s not opened\n",fileRFpattern);
	    text_error(paterrmsg);
	    psg_abort(1);
 	}
 }
 else
 {
	text_error("init_global_list: Invalid Global Segment\n");
	psg_abort(1);
 }

 if (write(freqfilptr->fd,freqfilptr->hptr,freqfilptr->size)
						!= freqfilptr->size)
 {
	text_error("init_global_list: file header not written\n");
	psg_abort(1);
 }


 return(OK);
}

close_global_list()
/************************************************************************
* Writes the size of the freq list file in the header
* Closes the freq list files in the experiment library.
* This should be called at the end of the experiment.
*************************************************************************/
{

 if (gfreq.gseg >= 0)
 {
	/*** Do not write to file if INOVA system ***/
 	if (newacq) return(OK);

	if (freqfilptr->size > (long)(256+2) * sizeof(codelong)) {
 		p_frqfilhdrpos = freqfilptr->hptr;
 		*p_frqfilhdrpos = freqfilptr->size; /* update file size */

		/* "rewind" file and update file header */
		if (lseek(freqfilptr->fd,0,0) == SYSERR) {
			text_error("Freq List lseek failed!\n");
			psg_abort(1);
		}
		if (write(freqfilptr->fd,freqfilptr->hptr,
		(long)(256+2) * sizeof(codelong)) !=
		(long)(256+2) * sizeof(codelong)) {
			text_error("RF write failed!\n");
			psg_abort(1);
		}

		/* close file */
		if (close(freqfilptr->fd) == SYSERR) {
		    text_error("Freq List close failed!\n");
		    psg_abort(1);
		}
	}
	else {
 		if (gfreq.gseg == ACQ_XPAN_GSEG)
 		{
		    if (unlink(filexpan) == SYSERR) {
			text_error("Freq list XPAN unlink failed!\n");
			/* psg_abort(1); */
		    }
		    filexpan[0] = 0; /* set filename to null string */
		} else if (gfreq.gseg == ACQ_RF_GSEG) {
		    if (unlink(fileRFpattern) == SYSERR) {
			text_error("Freq list RF unlink failed!\n");
			/* psg_abort(1); */
		    }
		    fileRFpattern[0] = 0; /* set filename to null string */
		}
	}

 }
 return(OK);
}

free_global_list()
{
	if (freqfilptr->hptr != NULL) {
		free( freqfilptr->hptr );
		freqfilptr->hptr = NULL;
	}

	if (freqfilptr->mptr != NULL) {
		free( freqfilptr->mptr );
		freqfilptr->mptr = NULL;
	}
}

void vget_elem(list_no, vindex)
/************************************************************************
* Description:
*	Element creates the acodes for indexing into a table of previously
*	created frequency offsets or frequencies.   
*
* Arguments: 
*	list_no		: 0-255 should match will a generated list using
*			  create_offset_list or create_freq_list. 
*	vindex		: realtime variable v1-v14 etc., table T1-T60
*			: used to index to the desired frequency.
* 							
* Return value: 
*	NONE 
*  
*************************************************************************/

int 	list_no;
codeint	vindex;
{
   char msge[MAXPATHL];

   /* check to make sure a valid global segment is available	*/
   if (gfreq.gseg < 0) 
   {
  	text_error("Global segment not available for frequency list.\n");
	psg_abort(1);
   }

   /* test valid apbus range for vindex */
   if (((vindex < v1) || (vindex > v14)) && ((vindex < t1) || 
	(vindex > t60))) 
   {
       sprintf(msge,"voffset: vindex illegal dynamic %d \n",
			       vindex);
       text_error(msge);
       psg_abort(1); 
   }
   if ((vindex >= t1) && (vindex <= t60))
	vindex = tablertv(vindex);

   /* test valid list number */
   if (gfreq.acodes_forelem[list_no] == 0)
   {
	sprintf(msge, "List number: %d is incorrect.\n",list_no);
	text_error(msge);
	psg_abort(1);
   }
   /* sprintf(msge,"voffset: list[%d] gseg: %d  freq_addr: %d vindex: %d\n", */
   /*		list_no,gfreq.gseg,gfreq.acodes_forelem[list_no],vindex); */
   /* text_error(msge); */

   putcode((codeint)GTABINDX);
   putcode((codeint)gfreq.gseg);
   if (newacq)
   	putcode((codeint)gfreq.tblnum[list_no]);
   else
   	putcode((codeint)list_no);
   putcode((codeint)gfreq.acodes_forelem[list_no]);
   putcode((codeint)vindex);

   /* do we need to update this ? */
   curfifocount += gfreq.acodes_forelem[list_no]-4;

}

/*----------------------------------------------------------------------*/
/*	putgtab()/1							*/
/*	puts integer word into Global table array defined by the 	*/
/* 	table number (currently not used), increments pointer		*/
/*									*/
/*   Modified   Author     Purpose					*/
/*   --------   ------     -------					*/
/*    6/19/89   Greg B.    1. Speed Optimizations, removed incrementing	*/ 
/*			     Codeoffset. It Now calculated when needed.	*/
/*									*/
/*----------------------------------------------------------------------*/
putgtab(table,word)
int table;			/* currently not used */
codeint word;
{
    if (bgflag)
	fprintf(stderr,"GTAB: %d Code(%lx) = %d(dec) or %4x(hex) \n",
		table,GTABptr,word,word);
    *GTABptr[table]++ = word; 		/* Put word into Codes array */

    /* test for acodes overflowing malloc acode memory */
    if ((long)GTABptr[table] > (long)GtabEnd)  
    {    
        char msge[128];
	sprintf(msge,"Acode overflow, %ld words generated.",
		(long) (GTABptr[table] - GtabStart));
	text_error(msge);
	psg_abort(0); 
    } 
}

condense_apbout(beginaddr, endaddr)
  /************************************************************************
   * Description:
   *	Element will condense all the apbouts found into a single apbout,
   *     and set the GTAB end pointer appropiately.
   *
   *    The count for each apbout is one less that the number of acodes.  So
   *    each time the routine encounters an apbout (after the first one), it
   *    
   *
   * Arguments: 
   *	beginaddr	: pointer to first apbout.
   *	endaddr		: pointer to the end pointer of the codes to condense 
   * 							
   * Return value: 
   *	None.
   *************************************************************************/
codeint *beginaddr;
codeint **endaddr;
{
   codeint *apbcntr,*newendaddr; 

   /* printf("condense_apbout: beginaddr: %d  endaddr: %d acode: %d\n", */
   /* 			beginaddr,*endaddr,(int)*beginaddr);	*/
   if (*beginaddr != APBOUT)
   {
	text_error("condense_apbout: Passed incorrect acode stream");
	psg_abort(1);
   }
   beginaddr++;			/* increment past apbout acode		*/
   apbcntr = beginaddr;		/* set address of apbout counter	*/
   newendaddr = beginaddr = beginaddr + *beginaddr + 2;
   while ((long)beginaddr < (long)*endaddr)
   {
	int tmpapbcntr;
   	if (*beginaddr != APBOUT)
   	{
	   text_error("condense_apbout: Only accepts APBOUTS in acode stream.");
	   psg_abort(1);
   	}
	beginaddr++;		/* increment past apbout acode	*/
	tmpapbcntr = *beginaddr++;	/* set address of apbout counter*/
	tmpapbcntr++; 		/* Increment because count = n-1 acodes	*/
	*apbcntr = *apbcntr + (codeint)tmpapbcntr;
	while (tmpapbcntr > 0)
	{
	   *newendaddr++ = *beginaddr++;
	   tmpapbcntr--;
	}
   }
   *endaddr = newendaddr;
   /* printf("endaddr: %d  apbcntr: %d \n",*endaddr,(int)*apbcntr); */
}

int
Create_offset_list(list,nvals,device,list_no)
/************************************************************************
* Description:
*	Element will generate offsets for whatever the current base
*	frequency is.  Will create the list if the list specified by
*	the list number has not been created before.
*
* Arguments: 
*	list		: pointer to a list of frequency offsets 
*	nvals		: number of values in the list 
*	device		: TODEV, DODEV 
*	list_no		: 0-255
* 							
* Return value: 
*	>= 0, Number of the list it has created. 
*        < 0, No list made. 
*************************************************************************/

double *list;
int	nvals;
int	device;
int	list_no;
{
 int	size;			/* size in bytes of completed list */
 int	i;
 codeint *outputbuf;
 char   msge[128];

 /* check to make sure a valid global segment is available	*/
 if (gfreq.gseg < 0) 
 {
	text_error("Global segment not available for offset list.\n");
	psg_abort(1);
 }

 if (list_no < 0)
 {
	list_no = gfreq.listcnt;
 }

 if (gfreq.listcnt == list_no)
 {
	size = nvals*MAXFREQACODE*2;	/* size of buffer in bytes */
 	outputbuf = (codeint *)malloc(size);
	GTABptr[gfreq.gseg] = GtabStart = outputbuf;
	GtabEnd = &outputbuf[(nvals*MAXFREQACODE)-1];
	gfreq.acodes_forelem[list_no] = Create_mem_offset_list(list, 
						nvals, device, GtabStart);

	/* when list completed, update the size in file header		*/
	size = (int)GTABptr[gfreq.gseg]-(int)outputbuf;
	freqfilptr->size += size;
	*p_frqfilhdrpos++ = freqfilptr->size;

	if (newacq)
	{
	   /*-----------------------------------------------------------*/
	   /* convert old acode list to table list for TAPBCOUT		*/
	   /*-----------------------------------------------------------*/
	   codeint *newacqbuf,*nacqptr,*addr,*nend;
	   int len,nacodes;
	   int newacodes = 0;

 	   nacqptr = newacqbuf = (codeint *)malloc(nvals*MAXFREQACODE*2);
	   addr = GtabStart;
	   nend = newacqbuf + nvals*MAXFREQACODE;
	   for (i=0; i<nvals; i++) 
	   {
		addr++;		/* skip acode */
		len = *addr++;
	   	nacodes = gen_apbcout(nacqptr,addr,len);
		if (newacodes == 0){
		    newacodes = nacodes;
		} else {
		    if (newacodes != nacodes) {
		    sprintf(msge,"freq_list: Mismatch in acodes %d, is %d\n",
			newacodes,nacodes);
	    	    text_error(msge);
		    psg_abort(1);
		   }
	    	}
	   	nacqptr = nacqptr+nacodes;
		addr = addr+len+1;
	   }
	   gfreq.tblnum[list_no] = createglobaltable(nvals, newacodes*2, 
						(char *)newacqbuf);
	   free((char *)newacqbuf);

	} 
	else {
	   /* output the acodes to the global segment file	*/
   	   if (write(freqfilptr->fd,(char *)outputbuf,size) != size) {
		text_error("RF write failed\n");
		psg_abort(1);
	   }
	}
	free(outputbuf);
 	gfreq.listcnt++;
	return(list_no);
 }
 else
 {
	if (gfreq.listcnt > list_no)
	{
	   if (bgflag)
	   {
	     sprintf(msge, "Warning: List [%d] already created.\n",list_no);
	     text_error(msge);
	   }
	}
	if (gfreq.listcnt < list_no)
	{
	   sprintf(msge, "Error: List [%d] not in order.\n",list_no);
	   text_error(msge);
	   sprintf(msge, 
	    "Make sure to start at 0, and to have unique list\n"); 
	   text_error(msge);
	   sprintf(msge,"numbers for each list.\n");
	   text_error(msge);
	   psg_abort(1);
	}
	return(-1);
 }
}

int
Create_mem_offset_list(list, nvals, device, pbuf)
  /************************************************************************
   * Description:
   *	Element will generate a list of frequency setting Acodes.
   *
   * Arguments: 
   *	list		: pointer to a list of frequencys
   *	nvals		: number of values in the list 
   *	device		: TODEV, DODEV
   *	pbuf		: (RETURN) address of list in memory
   * 							
   * Return value: 
   *	Number of Acodes in each list element.
   *************************************************************************/

  double *list;
  int	nvals;
  int	device;
  codeint *pbuf;
{
    int	acodes_forelem;
    int	i;
    codeint *beginaddr;
    char   msge[128];

    /* check for a valid device.	*/
    if ((device < 1) || (device > NUMch)){
	sprintf(msge, "freq_list: device #%d is not within bounds 1 - %d\n",
		device, MAX_RFCHAN_NUM);
        text_error(msge);
        psg_abort(1);
    }
    if (RF_Channel[device] == NULL){
	if (ix < 2){
	    sprintf(msge, 
		    "offset: Warning RF Channel device #%d is not present.\n",
		    device);
	    text_error(msge);
	}
        return(0);
    }

    acodes_forelem = 0;

    /* step through frequency list and make the acodes for each	*/
    /* frequency.						*/
    for (i=0; i<nvals; i++){
	Msg_Set_Param   param;
	Msg_Set_Result  result;
	int error,num_acodes_forelem;

	/* Generate acodes for requested offset */
	beginaddr = GTABptr[gfreq.gseg];
	okinhwloop();

	if (bgflag)
	{
	   sprintf(msge,"num: %d  freq: %12.4lf\n",i,list[i]);
	   text_error(msge);
	}			

	SetRFChanAttr(RF_Channel[device], SET_OFFSETFREQ, list[i], 
			SET_FREQLIST, gfreq.gseg, NULL);

	/* Get RF band of Observe Transmitter  */
	param.setwhat = GET_XMTRTYPE;
	error = Send(RF_Channel[device], MSG_GET_RFCHAN_ATTR_pr, 
						&param, &result);
	if (error < 0)
	{
	        sprintf(msge, 
	       "%s : %s\n", RF_Channel[device]->objname, ObjError(error));
	        text_error(msge);
	}

	/* delay in order for frequency switching to settle		*/
	if ( (result.reqvalue != TRUE) &&  !newacq )
	{
	        G_Delay(DELAY_TIME, 2.0e-6, SET_GTAB, gfreq.gseg, 0);	
	}

	if (!newacq) {
	    putgtab(gfreq.gseg,GTABRTN);
	}
	num_acodes_forelem = (int)((long)GTABptr[gfreq.gseg] - 
				   (long)beginaddr)/2;
	if (acodes_forelem == 0){
	    acodes_forelem = num_acodes_forelem;
	}else{
	    if (acodes_forelem != num_acodes_forelem){
		sprintf(msge,"freq_list: Mismatch in freq acodes %d, is %d\n",
			acodes_forelem,num_acodes_forelem);
	        text_error(msge);
		psg_abort(1);
	    }
	}
    }

    return(acodes_forelem);
}

int
Create_delay_list(list,nvals,list_no)
/************************************************************************
* Description:
*	Element will generate a list of delays. 
*	Will create the list if the list specified by
*	the list number has not been created before.
*
* Arguments: 
*	list		: pointer to a list of delays. 
*	nvals		: number of values in the list 
*	list_no		: 0-255
* 							
* Return value: 
*	>= 0, Number of the list it has created. 
*        < 0, No list created.
*************************************************************************/

double *list;
int	nvals;
int	list_no;
{
 int	size;			/* size in bytes of completed list */
 int	i,j;
 codeint *outputbuf;
 char   msge[128];


 /* check to make sure a valid global segment is available	*/
 if (gfreq.gseg < 0) 
 {
	text_error("Global segment not available for delay list.\n");
	psg_abort(1);
 }

 if (list_no < 0)
 {
	list_no = gfreq.listcnt;
 }

 if (gfreq.listcnt == list_no)
 {
	size = nvals*MAXDELAYACODE*2;	/* size of buffer in bytes */
 	outputbuf = (codeint *)malloc(size);
	GTABptr[gfreq.gseg] = GtabStart = outputbuf;
	GtabEnd = &outputbuf[(nvals*MAXDELAYACODE)-1];
	gfreq.acodes_forelem[list_no] = Create_mem_delay_list(list, 
						nvals, GtabStart);

	/* when list completed, update the size in file header		*/
	size = (int)GTABptr[gfreq.gseg]-(int)outputbuf;
	freqfilptr->size += size;
	*p_frqfilhdrpos++ = freqfilptr->size;

	if (newacq)
	{
	   /*-----------------------------------------------------------*/
	   /* convert old acode list to table list for DELAYS		*/
	   /*-----------------------------------------------------------*/
	   codeint *newacqbuf,*nacqptr,*addr,*nend,delayacode;
	   int len,nacodes;
	   int newacodes = 0;

 	   nacqptr = newacqbuf = (codeint *)malloc(nvals*MAXDELAYACODE*2);
	   addr = GtabStart;
	   nend = newacqbuf + nvals*MAXDELAYACODE;
	   for (i=0; i<nvals; i++) 
	   {
		delayacode = *addr++;		/* skip acode */
		if (delayacode == EVENT1_TWRD)
			len = 2;
		else if (delayacode == EVENT2_TWRD)
			len = 4;
		else {
		    sprintf(msge,"Delay_list: unknown acode %d\n",
			delayacode);
	    	    text_error(msge);
		    psg_abort(1);
		}

		for (j=0; j<len; j++)
		{
		   *nacqptr++ = *addr++;
		}
	   	nacodes = len;
		if (newacodes == 0){
		    newacodes = nacodes;
		} else {
		    if (newacodes != nacodes) {
		    sprintf(msge,"Delay_list: Mismatch in acodes %d, is %d\n",
			newacodes,nacodes);
	    	    text_error(msge);
		    psg_abort(1);
		   }
	    	}
	   }
	   gfreq.tblnum[list_no] = createglobaltable(nvals, newacodes*2, 
						(char *)newacqbuf);
	   gfreq.acodes_forelem[list_no] = newacodes; /* Update */
	   free((char *)newacqbuf);

	} 
	else
	{
	   /* output the acodes to the global segment file	*/
   	   if (write(freqfilptr->fd,(char *)outputbuf,size) != size) {
		text_error("delay list write failed\n");
		psg_abort(1);
	   }
	}
	free(outputbuf);
 	gfreq.listcnt++;
	return(list_no);
 }
 else
 {
	/** checks to make sure lists are created in order  **/
	if (gfreq.listcnt > list_no)
	{
	   if (bgflag)
	   {
	     sprintf(msge, "Warning: List [%d] already created.\n",list_no);
	     text_error(msge);
	   }
	}
	if (gfreq.listcnt < list_no)
	{
	   sprintf(msge, "Error: List [%d] not in order.\n",list_no);
	   text_error(msge);
	   sprintf(msge, 
	    "Make sure to start at 0, and to have unique list\n"); 
	   text_error(msge);
	   sprintf(msge,"numbers for each list.\n");
	   text_error(msge);
	   psg_abort(1);
	}
	return(-1);
 }
}

int
Create_mem_delay_list(list, nvals, pbuf)
  /************************************************************************
   * Description:
   *	Element will generate a list of frequency setting Acodes.
   *
   * Arguments: 
   *	list		: pointer to a list of frequencys
   *	nvals		: number of values in the list 
   *	pbuf		: (RETURN) address of list in memory
   * 							
   * Return value: 
   *	Number of Acodes in each list element.
   *************************************************************************/

  double *list;
  int	nvals;
  codeint *pbuf;
{
    int	acodes_forelem;
    int	i;
    codeint *beginaddr;
    char   msge[128];


    acodes_forelem = 0;
    for (i=0; i<nvals; i++)
    {
	   int num_acodes_forelem;
	   /* Generate acodes for requested delay */

	    beginaddr = GTABptr[gfreq.gseg];
	    okinhwloop();
	    G_Delay(DELAY_TIME, list[i], SET_GTAB, gfreq.gseg, 0);	


	    if (!newacq) {
	    	putgtab(gfreq.gseg,GTABRTN);
	    }
	    num_acodes_forelem = (int)((long)GTABptr[gfreq.gseg] - 
							(long)beginaddr)/2;

	    if (acodes_forelem == 0)
		acodes_forelem = num_acodes_forelem;
	    else
	    {
	    	if (acodes_forelem != num_acodes_forelem)
	    	{
	   	 sprintf(msge,"freq_list: Mismatch in freq acodes %d, is %d\n",
			acodes_forelem,num_acodes_forelem);
	        text_error(msge);
		 psg_abort(1);
	    	}
	    }

    }

    return(acodes_forelem);
}

int
Create_freq_list(list,nvals,device,list_no)
/************************************************************************
* Description:
*	Element will generate frequencies for whatever the current base
*	frequency is.  Will create the list if the list specified by
*	the list number has not been created before.
*
* Arguments: 
*	list		: pointer to a list of frequency offsets 
*	nvals		: number of values in the list 
*	device		: TODEV, DODEV 
*	list_no		: 0-255
* 							
* Return value: 
*	>= 0, Number of the list it has created. 
*        < 0, No list created.
*************************************************************************/

double *list;
int	nvals;
int	device;
int	list_no;
{
 int	size;			/* size in bytes of completed list */
 int acodes_per_elem;
 codeint *outputbuf;
 char   msge[128];


 /* check to make sure a valid global segment is available	*/
 if (gfreq.gseg < 0) 
 {
	text_error("Global segment not available for frequency list.\n");
	psg_abort(1);
 }

 if (list_no < 0)
 {
	list_no = gfreq.listcnt;
 }

 if (gfreq.listcnt == list_no)
 {
	size = nvals*MAXFREQACODE*2;	/* size of buffer in bytes */
 	outputbuf = (codeint *)malloc(size);
	gfreq.acodes_forelem[list_no] = acodes_per_elem =
	  Create_mem_freq_list(list, nvals, device, outputbuf);

	if (newacq){
	    gfreq.tblnum[list_no] = createglobaltable(nvals, acodes_per_elem*2, 
						      (char *)outputbuf);
	}else{
	    /* when list completed, update the size in file header */
	    size = (int)GTABptr[gfreq.gseg]-(int)outputbuf;
	    freqfilptr->size += size;
	    *p_frqfilhdrpos++ = freqfilptr->size;
	    
	   /* output the acodes to the global segment file	*/
   	   if (write(freqfilptr->fd,(char *)outputbuf,size) != size) {
		text_error("RF write failed\n");
		psg_abort(1);
	   }
	}
	free(outputbuf);
 	gfreq.listcnt++;
	return(list_no);
 }
 else
 {
	if (gfreq.listcnt > list_no)
	{
	   if (bgflag)
	   {
	     sprintf(msge, "Warning: List [%d] already created.\n",list_no);
	     text_error(msge);
	   }
	}
	if (gfreq.listcnt < list_no)
	{
	   sprintf(msge, "Error: List [%d] not in order.\n",list_no);
	   text_error(msge);
	   sprintf(msge, 
	    "Make sure to start at 0, and to have unique list\n"); 
	   text_error(msge);
	   sprintf(msge,"numbers for each list.\n");
	   text_error(msge);
	   psg_abort(1);
	}
	return(-1);
 }
}

int
Create_mem_freq_list(list, nvals, device, outputbuf)
  /************************************************************************
   * Description:
   *	Element will generate a list of frequency setting Acodes.
   *
   * Arguments: 
   *	list		: pointer to a list of frequencys
   *	nvals		: number of values in the list 
   *	device		: TODEV, DODEV
   *	outputbuf	: address of list in memory
   * 							
   * Return value: 
   *	Number of Acodes in each list element.
   *************************************************************************/

  double *list;
  int	nvals;
  int	device;
  codeint *outputbuf;
{
    int	acodes_forelem;
    int	i;
    codeint *beginaddr;
    char   msge[128];

    /* check for a valid device.	*/
    if ((device < 1) || (device > NUMch)){
	sprintf(msge, "freq_list: device #%d is not within bounds 1 - %d\n",
		device, MAX_RFCHAN_NUM);
        text_error(msge);
        psg_abort(1);
    }
    if (RF_Channel[device] == NULL){
	if (ix < 2){
	    sprintf(msge, 
		    "offset: Warning RF Channel device #%d is not present.\n",
		    device);
	    text_error(msge);
	}
        return 0;
    }

    GTABptr[gfreq.gseg] = GtabStart = outputbuf;
    GtabEnd = &outputbuf[(nvals*MAXFREQACODE)-1];
    acodes_forelem = 0;

 /* Set Mixer in Magnet Leg to Hi or Low Band.  This depends on */
 /* on the frequency sweep since the mixers in the magnet leg	*/
 /* have overlapping ranges.  Select the mixer based on the	*/
 /* frequency at the middle of the sweep.			*/

    SetRFChanAttr(RF_Channel[device], SET_SPECFREQ, list[nvals/2-1], 
		      SET_OFFSETFREQ, 0.0, NULL);
    if ( !rfchan_getampband(OBSch) )
      SetAPBit(RF_MLrout, SET_TRUE, MAGLEG_HILO, NULL);
    else
      SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_HILO, NULL);

/* Since the magnet leg routing is constant for the entire
   sweep, it seems appropriate to set it here, before starting
   the actual frequency sweep.  Unfortunately this does not work
   on the Uplus for some unknown reason.  So we set the magnet
   leg routing for each individual frequency in the sweep. 	*/

    /* step through frequency list and make the acodes for each	*/
    /* frequency.						*/
    for (i=0; i<nvals; i++){
	Msg_Set_Param   param;
	Msg_Set_Result  result;
	int error,num_acodes_forelem;
	/* Generate acodes for requested offset */
	beginaddr = GTABptr[gfreq.gseg];
	okinhwloop();

	SetRFChanAttr(RF_Channel[device], SET_SPECFREQ, list[i], 
		      SET_OFFSETFREQ, 0.0, NULL);
	SetRFChanAttr(RF_Channel[device],SET_MIXER_VALUE, gfreq.gseg, NULL);
	SetRFChanAttr(RF_Channel[device], SET_FREQLIST, gfreq.gseg, NULL);
	SetAPBit(RF_MLrout, SET_GTAB, gfreq.gseg, NULL);

	/* Get RF band of Observe Transmitter  */

	param.setwhat = GET_XMTRTYPE;
	error = Send(RF_Channel[device], MSG_GET_RFCHAN_ATTR_pr, 
		     &param, &result);
	if (error < 0){
	    sprintf(msge, 
		    "%s : %s\n", RF_Channel[device]->objname, ObjError(error));
	    text_error(msge);
	}
	/* delay in order for frequency switching to settle		*/
	if ((result.reqvalue != TRUE) && !newacq) {
	    G_Delay(DELAY_TIME, 2.0e-6, SET_GTAB, gfreq.gseg, 0);	
	}
	if (!newacq) {
	    putgtab(gfreq.gseg,GTABRTN);
	}
	else 
	{
	    /* To be able to interpret the apbout's generated into 	*/
	    /* a table driven TAPBCOUT, all the abouts generated above	*/
	    /* have to be condensed into a single apbout.		*/
	    condense_apbout((codeint *)beginaddr, &GTABptr[gfreq.gseg]);
	}
	num_acodes_forelem = (int)((long)GTABptr[gfreq.gseg] - 
				   (long)beginaddr)/2;
	if (acodes_forelem == 0){
	    acodes_forelem = num_acodes_forelem;
	}else{
	    if (acodes_forelem != num_acodes_forelem){
		sprintf(msge,"freq_list: Mismatch in freq acodes %d, is %d\n",
			acodes_forelem, num_acodes_forelem);
	        text_error(msge);
		psg_abort(1);
	    }
	}
    }
    if (newacq){
	/* Convert acode list to table list for TAPBCOUT */
	codeint *newacqbuf, *nacqptr, *addr, *nend;
	int len, nacodes;
	int newacodes = 0;

	nacqptr = newacqbuf = (codeint *)malloc(nvals*MAXFREQACODE*2);
	addr = GtabStart;
	nend = newacqbuf + nvals*MAXFREQACODE;
	for (i=0; i<nvals; i++){
	    addr++;		/* skip acode */
	    len = *addr++;
	    nacodes = gen_apbcout(nacqptr,addr,len);
	    if (newacodes == 0){
		newacodes = nacodes;
	    }else{
		if (newacodes != nacodes){
		    sprintf(msge,"freq_list: Mismatch in acodes %d, is %d\n",
			    newacodes, nacodes);
	    	    text_error(msge);
		    psg_abort(1);
		}
	    }
	    nacqptr = nacqptr+nacodes;
	    addr = addr+len+1;
	}
	memcpy(outputbuf, newacqbuf, nvals * newacodes * 2);
#ifdef LINUX
        {
           codeint *ptr;
           int i;
           ptr = outputbuf;
           for (i=0; i < nvals * newacodes; i++)
           {
              *ptr = htons( *ptr );
              ptr++;
           }
        }
#endif
	acodes_forelem = newacodes;
	free((char *)newacqbuf);
    } 
    return acodes_forelem;
}

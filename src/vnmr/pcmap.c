/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------*/
/* File pcmap.c:  							*/
/*	Contains routines that implement generating and applying Phase	*/
/*	Correction Map information.  The command were developed for	*/
/*	EPI applications, but can be used for any type of data.		*/
/*	The following commands are implemented in this file.		*/
/*	- pcmapopen([<filename>][,<max_index>])				*/
/*	- pcmapclose()							*/
/*	- pcmapgen([<filename>,]<index>[,<options>])		       	*/
/*	- pcmapapply([<filename>,]<index>)				*/
/* Table Convert							*/
/*	- tcopen(petable)						*/
/*	- tcclose()							*/
/*	- tcapply([<filename>])						*/
/* Reverse Spectrum							*/
/*	- rsapply(<index>)						*/
/*----------------------------------------------------------------------*/

#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mfileObj.h"
#include "data.h"
#include "group.h"
#include "tools.h"
#include "vnmrsys.h"
#include "variables.h"
#include "phase_correct.h"
#include "pvars.h"
#include "wjunk.h"

#define PCMAPGEN 	0
#define PCMAPAPPLY 	1
#define PCMAPOPEN 	2

#define OLD_S_COMPLEX	0x40	/* Complex data status bit for old fid data */

#define TRUE	1
#define FALSE	0

#define OK	0
#define ERROR		1

extern void Vperror(char *);
extern void gaussj(double **a, int n, double **b, int m);

static  MFILE_ID pcmap_md = NULL;
static  int explicit_open = FALSE;
static	int pcmap_maxindx = 0;

/* index array is created from the phase encoding table (tcopen/tcapply) but used by pcmapgen too */
static int *index_array = NULL;
static int tablesize = 0;

static int pcmapgen(MFILE_ID pcmap_md, int blk_index, int pc_option);
static int pcmapapply(MFILE_ID pcmap_md, int blk_index);
static  int intcompare(const void *i, const void *j);
static int estdimsiz(int *fnvar, int *fn1var);
int phaseunwrap(double *input, int npts, double *output);
int pc_calc(float *data, float *result, int echosize, int etl, int method, int transflag);
int polyfit(double *input, int npts, double *output, int order);
   int getphase(int iecho, int echolength, int nviews, double *phsptr, float *data, int transposed);



static int putphase(iecho, echolength, nviews, phsptr, result, transposed)
   int iecho, echolength, nviews, transposed;
   double *phsptr;
   float *result;
   {
       int i,offset;
       
       if(transposed)
       {
	   for (i=0;i<echolength;i++)      
	   {
	       offset=iecho+nviews*i; /* data stored column-wise */
	       offset=offset*2;       /* since data is complex   */
	       *(result+offset)=(float)cos(*(phsptr+i));
	       *(result+offset+1)=(float)-sin(*(phsptr+i));
	   }	
       }
       else
       {
	   for (i=0;i<echolength;i++)      
	   {
	       offset=iecho*echolength+i; /* data stored row-wise */
	       offset=offset*2;       /* since data is complex   */
	       *(result+offset)=(float)cos(*(phsptr+i));
	       *(result+offset+1)=(float)-sin(*(phsptr+i));
	   }	
       }	
	   
       return(OK);
}  

/******************************************************************************/
int pcmap ( argc, argv, retc, retv )
/* 
Purpose:

Arguments:
---------
argc  :  (   )  Argument count.
argv  :  (   )  Command line arguments.
retc  :  (   )  Return argument count.  Not used here.
retv  :  (   )  Return arguments.  Not used here.
*/


int argc, retc;
char *argv[], *retv[];
/*ARGSUSED*/

{ /* Begin function */
   /*
   Local Variables:
   ---------------
   pcmap_file	:  Complete file path to the source fid file .
   dest_fid  	:  Complete file path to the destination fid file .
   a_opt	: Defines source exp option 1=arg specified 0=not specfied
   */
   char   pcmap_file[MAXPATHL];
  int	a_opt, action, permit, status;
   int  pcmap_blkindx = 0;
   int nargs;
   int pc_option=POINTWISE;  /* the default phase map method */

   /*---------------------------*/
   /* Begin Executable Code:	*/
   /*---------------------------*/
   /* Verify that the user has passed correct number of arguments */
   /* note that index is required! */

   a_opt = 0;
   nargs=argc-1;
   if ( !strcmp(argv[0],"pcmapgen") ) 
   {
	if ((nargs < 1) || (nargs > 3))
	{ 
	   Werrprintf("Usage: pcmapgen([<filename>,] <index> [,<options>]).");
	   ABORT;
	}
	action = PCMAPGEN;
	(void)strcpy ( pcmap_file, curexpdir );
	(void)strcat ( pcmap_file, "/datdir/" );
	if (nargs==1)
	{
	    (void)strcat ( pcmap_file, "pcmap" ); /*  use default */
	   a_opt=1; /* index option number */
	}
	else if(nargs==2)
	{
	    a_opt=2;
	    if(isReal(argv[1])) /* have option and index */
	    {    
		(void)strcat (pcmap_file, "pcmap" );  /* use default */
		pc_option=(int)stringReal(argv[1]);
		if((pc_option < MIN_OPTION)||(pc_option>MAX_OPTION))
		{
		       Werrprintf("pcmapgen: Illegal map generation option %d",
				  pc_option);
		       ABORT;	
		}
	    }
	    else  /* have filename and index */
	    {
		(void)strcat ( pcmap_file, argv[1] );  
	    }
	}
	else /* have all 3 arguments */
	{
		(void)strcat ( pcmap_file, argv[1] );
		pc_option=(int)stringReal(argv[3]);
		if((pc_option < MIN_OPTION)||(pc_option>MAX_OPTION))
		   {
		       Werrprintf("pcmapgen: Illegal map generation option %d",
				  pc_option);
		       ABORT;
		   }
		a_opt=2;
	}


	if (isReal(argv[a_opt])) 
	   pcmap_blkindx=(int)stringReal(argv[a_opt]);
	else {
	   Werrprintf ( "pcmapgen: Illegal index block number.");
	   ABORT;
	}
   }
   
   else if ( !strcmp(argv[0],"pcmapapply") )
   {
	if ((argc < 2) || (argc > 3))
	{ 
	   Werrprintf("Usage: pcmapapply([<filename>,]<index>).");
	   ABORT;
	}
	action = PCMAPAPPLY;
	(void)strcpy ( pcmap_file, curexpdir );
	(void)strcat ( pcmap_file, "/datdir/" );
	if (argc < 3)
	{
	   (void)strcat ( pcmap_file, "pcmap" );
	}
	else
	{
	   (void)strcat ( pcmap_file, argv[1] );
	   a_opt = 1;
	}
	if (isReal(argv[1+a_opt]))  
	   pcmap_blkindx=(int)stringReal(argv[1+a_opt]);
	else {
	   Werrprintf ( "pcmapapply: Illegal index block number.");
	   ABORT;
	}
   }
   else if ( !strcmp(argv[0],"pcmapopen") )
   {
	if ((argc < 2) || (argc > 3))
	{ 
	   Werrprintf ( "Usage: pcmapopen(file,max_index) or pcmapopen(max_index)." );
	   ABORT;
	}
	explicit_open = TRUE;
	action = PCMAPOPEN;
	(void)strcpy ( pcmap_file, curexpdir );
	(void)strcat ( pcmap_file, "/datdir/" );
	if (argc < 3)
	{
	   (void)strcat ( pcmap_file, "pcmap" );
	}
	else
	{
	   (void)strcat ( pcmap_file, argv[1] );
	   a_opt = 1;
	}
	if (isReal(argv[1+a_opt]))  
	   pcmap_maxindx = (int)stringReal(argv[1+a_opt]);
	else {
	   Werrprintf ( "pcmapopen: Illegal max index number '%s'.",argv[1+a_opt]);
	   ABORT;
	}
   }
   else if (  !strcmp(argv[0],"pcmapclose") )
   {
	if (argc > 1)
	{ 
	   Werrprintf ( "Usage: pcmapclose()." );
	   ABORT;
	}

	if (pcmap_md->newByteLen < pcmap_md->byteLen)
		pcmap_md->newByteLen = pcmap_md->byteLen;
	mClose(pcmap_md);
	pcmap_md = NULL;
	explicit_open = FALSE;
	pcmap_maxindx = 0;
   	/* Normal, successful return */
   	RETURN;
   }
   else
   {
	   Werrprintf ( "pcmap: Incorrect command." );
	   ABORT;
   }


   /*---------------------------*/
   /* Open pcmap file	 	*/
   /*---------------------------*/

   /* Only open if file pointer is NULL otherwise assume they are 	*/
   /* already open.							*/
   if (pcmap_md == NULL)
   {
	int bytelen,local_fn,local_fn1,res;
	struct datafilehead	tmpdatahead;
	if (action == PCMAPAPPLY)
	{
	   bytelen = 0;
	   permit = O_RDONLY;
	}
	else if (action == PCMAPGEN)
	{
	   /* Get datafile info */
	   if ( (res = D_gethead(D_DATAFILE, &tmpdatahead)) )
	   {
	     D_error(res);
	     return(ERROR);
	   }
   	   permit = O_RDWR | O_CREAT;
	   bytelen = sizeof(struct datafilehead);
	   bytelen += tmpdatahead.bbytes * tmpdatahead.ebytes * pcmap_blkindx;
	}
	else  /* action == PCMAPOPEN */
	{
   	   permit = O_RDWR | O_CREAT;
	   if (estdimsiz(&local_fn,&local_fn1) != OK)
	   {
   		Vperror ( "pcmapopen: estdimsiz" );
   	   	ABORT;
	   }
	   bytelen = sizeof(struct datafilehead) +
			(sizeof(struct datablockhead)*pcmap_maxindx);
	   bytelen += local_fn * local_fn1 * sizeof(float) * pcmap_maxindx;
	}
   	if ( ( pcmap_md = mOpen ( pcmap_file,bytelen,permit)) == NULL )
   	{  Vperror ( "pcmap: mOpen (pcmap_file)" );
   	   ABORT;
   	}

   }

   status = OK;
   switch (action) {

	case PCMAPGEN:
	   status = pcmapgen(pcmap_md,pcmap_blkindx,pc_option);
	   break;
	case PCMAPAPPLY:
	   status = pcmapapply(pcmap_md,pcmap_blkindx);
	   break;
	case PCMAPOPEN:
	   break;
	default:
	   Werrprintf( "pcmap: Should not get to this default switch stmt." );
	   break;
   }

   if ((!explicit_open) || (status == ERROR))
   {
   	mClose(pcmap_md);
	pcmap_md = NULL;
   }
   

   /* Normal, successful return */
   RETURN;


} /* End function pcmap */



/*----------------------------------------------------------------------*/
/* pcmapgen								*/
/*	This routine will generate the complex phase correction values	*/
/*	cos(theta),sin(theta) from the current datafile data and store 	*/
/*	the correction values into pcmap.				*/
/*----------------------------------------------------------------------*/
static int pcmapgen(MFILE_ID pcmap_md, int blk_index, int pc_option)
{
   struct datafilehead *pcmap_file_head;
   struct datablockhead *block_head;
   struct datafilehead	tmpdatahead;
   dpointers	outblock;
   int cblock;
   int res,newfile = FALSE;				/* flags */

   /* Set pcmap file header */
   pcmap_file_head = (struct datafilehead *)pcmap_md->mapStrtAddr;
   pcmap_md->offsetAddr = pcmap_md->mapStrtAddr;

   /* Get datafile info */
   if ( (res = D_gethead(D_DATAFILE, &tmpdatahead)) )
   {
     D_error(res);
     return(ERROR);
  }
 

   pcmap_md->newByteLen = pcmap_md->byteLen;

   if (pcmap_md->byteLen == 0)
   {
	if (blk_index != 1)
	{
	   Werrprintf( "pcmapgen: Inconsistent index into pcmap file." );
	   return(ERROR);
	}
	else
	{
	   int bbytes;
   	   memcpy(pcmap_md->offsetAddr,&tmpdatahead,
					(sizeof(struct datafilehead)));
	   pcmap_file_head->nblocks = 1;
	   pcmap_file_head->ntraces = tmpdatahead.nblocks*tmpdatahead.ntraces;
	   bbytes = tmpdatahead.bbytes;
	   if (tmpdatahead.nblocks > 1)
		bbytes += (tmpdatahead.nblocks-1)*tmpdatahead.ntraces*
							tmpdatahead.tbytes;
	   pcmap_file_head->bbytes = bbytes;
	   pcmap_md->offsetAddr += sizeof(struct datafilehead);
	}
	newfile=TRUE;
   }
   else
   {
	int bbytes;
     if (blk_index == 1) 
     {
	int bbytes;
   	memcpy(pcmap_md->offsetAddr,&tmpdatahead,
					(sizeof(struct datafilehead)));
	pcmap_file_head->nblocks = 1;
	pcmap_file_head->ntraces = tmpdatahead.nblocks*tmpdatahead.ntraces;
	bbytes = tmpdatahead.bbytes;
	if (tmpdatahead.nblocks > 1)
	    bbytes += (tmpdatahead.nblocks-1)*tmpdatahead.ntraces*
							tmpdatahead.tbytes;
	pcmap_file_head->bbytes = bbytes;
	pcmap_md->offsetAddr += sizeof(struct datafilehead);
     }
     else
     {
	/* check for file consistentcy */
	if (pcmap_file_head->np != tmpdatahead.np)
	{
	   Werrprintf("pcmapgen: Inconsistent np between pcmap and data.");
	   return(ERROR);
	}
	if (pcmap_file_head->ntraces != tmpdatahead.ntraces*tmpdatahead.nblocks)
	{
	   Werrprintf("pcmapgen: Inconsistent ntraces between pcmap and data.");
	   return(ERROR);
	}
	bbytes = tmpdatahead.bbytes;
	if (tmpdatahead.nblocks > 1)
	    bbytes += (tmpdatahead.nblocks-1)*tmpdatahead.ntraces*
							tmpdatahead.tbytes;
	if (pcmap_file_head->bbytes != bbytes)
	{
	   Werrprintf("pcmapgen: Inconsistent bbytes between pcmap and data.");
	   return(ERROR);
	}
	if (blk_index > (pcmap_file_head->nblocks + 1))
	{
	   Werrprintf( "pcmapgen: blk_index out of range." );
	   return(ERROR);
	}
	/* advance pointer locations to start of data */
 	pcmap_md->offsetAddr += sizeof(struct datafilehead);
 	pcmap_md->offsetAddr += pcmap_file_head->bbytes*(blk_index - 1);
     }
   }


   /*----------------------------------------------*/
   /* Copy header info and advance pcmap pointer .	*/
   /*----------------------------------------------*/
   block_head = (struct datablockhead *)pcmap_md->offsetAddr;
   memcpy(pcmap_md->offsetAddr,&outblock.head,
					sizeof(struct datablockhead));
   pcmap_md->offsetAddr += sizeof(struct datablockhead);

  /**********************************
   *  Start processing data blocks.  *
   **********************************/

   for (cblock = 0; cblock < tmpdatahead.nblocks; cblock++)
   {
	float *d1, *pcdata;
	int transposed = TRUE;

	if ( (res = D_getbuf(D_DATAFILE, tmpdatahead.nblocks, cblock, &outblock)) )
	{
           D_error(res); 
           return(ERROR); 
 	}

	/*------------------------------*/
	/*  Start processing data.  	*/
	/*------------------------------*/
	
	pcdata = (float *)pcmap_md->offsetAddr;
	d1 = (float *)outblock.data;
	
	(void)pc_calc(d1, pcdata, tmpdatahead.np/2, tmpdatahead.ntraces, pc_option, transposed);

	pcmap_md->offsetAddr += 
			(sizeof(float)*tmpdatahead.np*tmpdatahead.ntraces);
	if ( (res = D_release(D_DATAFILE, cblock)) )
	{
	   D_error(res);
           return(ERROR); 
	}
   }
   block_head->index = blk_index;
 
   /*-------------------------------------------------------------*/
   /* Update file and file header with new info.			*/
   /*-------------------------------------------------------------------*/

   /* If the block is greater than num of blocks in the destination	*/
   /*  file, the source block will be appended to the dest file.	*/
   if (blk_index > pcmap_file_head->nblocks)
	pcmap_file_head->nblocks++;
   pcmap_md->newByteLen = sizeof(struct datafilehead);
   pcmap_md->newByteLen += 
			(pcmap_file_head->bbytes*pcmap_file_head->nblocks);

   return(OK);
}

int pc_calc(float *data, float *result, int echosize, int etl, int method, int transflag)
{
    if(method==POINTWISE) 
    {    
	float tmp;
	int j;
	for (j=0; j<etl*echosize*2; j+=2)
	{
	    tmp = (float)sqrt(data[j]*data[j] + data[j+1]*data[j+1]);
	    if (tmp > 0.0)
	    {
		result[j] = (float) data[j]/tmp;
		result[j+1] = (float) -data[j+1]/tmp;
	    }
	    else
	    {
		result[j] = 0; 
		result[j+1] = 0; 
	    }
	}
    }
    else /* fit to unwrapped phase and do something with it */
    {
	int cntr,i, iecho;
	double *cntr_phs,*uphs,*fitphs;
	double *phsline;
	
	cntr_phs = (double *)malloc(echosize*sizeof(double));
	phsline = (double *)malloc(echosize*sizeof(double));
	uphs = (double *)malloc(echosize*sizeof(double));
	fitphs =  (double *)malloc(echosize*sizeof(double));	  	   
	
	/* get central profile phase */
	/* use index_array which comes from tcopen or tcapply*/
	if(index_array == NULL)
	{
	    cntr=etl/2;  /* a guess */
	}
	else
	{
	    /* this ought to be right even for multishot EPI? */
	    cntr=index_array[(int)fmod((double)(tablesize/2),(double)etl)];	
	}
	(void)getphase(cntr,echosize, etl,cntr_phs,data,transflag);
	
	
	switch(method)
	{
	  case LINEAR:
	  case QUADRATIC:
	  {
	      for(iecho=0;iecho<etl;iecho++)
	      {
		  /* fetch the phase of the profile */     
		  (void)getphase(iecho,echosize,etl,phsline,data,transflag);
		  for(i=0;i<echosize;i++)
		      phsline[i]=phsline[i]-cntr_phs[i];
		  (void)phaseunwrap(phsline,echosize,uphs); 
		  /* fit to polynomial and return polynomial evaluation */
		  (void)polyfit(uphs,echosize,fitphs,method);	       
		  (void)putphase(iecho,echosize,etl,fitphs,result,transflag); 		
	      }
	  }
	  break;
	  case CENTER_PAIR:
	  {
	      int estart=1; 
	      if(fmod((double)cntr, (double)2) != 0.0)
		  estart=0;
	      /* zero the phase map */
	      for(i=0;i<etl*echosize;i++)
	      {
		  result[2*i]=1.0;
		  result[2*i+1]=0.0;
	      }
	      (void)getphase((cntr+1),echosize,etl,phsline,data,transflag);
	      for(i=0;i<echosize;i++)
		  phsline[i]=phsline[i]-cntr_phs[i];
	      (void)phaseunwrap(phsline,echosize,uphs); 
	      /* fit to polynomial and return polynomial evaluation */
	      (void)polyfit(uphs,echosize,fitphs,1);  /* zero order means only first order here */
	      for(iecho=estart;iecho<etl;iecho+=2)
	      {
		  (void)putphase(iecho,echosize,etl,fitphs,result,transflag); 
	      }
	  }
	  break;
	  case PAIRWISE:
	  {
	      int estart=1; 
	      if(fmod((double)cntr, (double)2) != 0.0)
		  estart=0;
	      /* zero the phase map */
	      for(i=0;i<etl*echosize;i++)
	      {
		  result[2*i]=1.0;
		  result[2*i+1]=0.0;
	      }
	      for(iecho=estart;iecho<(etl-1);iecho+=2)
	      {
		  (void)getphase(iecho,echosize,etl,phsline,data,transflag);
		  (void)getphase((iecho+1),echosize,etl,cntr_phs,data,transflag);
		  for(i=0;i<echosize;i++)
		      phsline[i]=phsline[i]-cntr_phs[i];
		  (void)phaseunwrap(phsline,echosize,uphs); 
		  /* fit to polynomial and return polynomial evaluation */
		  (void)polyfit(uphs,echosize,fitphs,1);		  
		  (void)putphase(iecho,echosize,etl,fitphs,result,transflag); 
	      }
	  }
	  break;
	  case FIRST_PAIR:
	  {
	      int estart=1; 
	      cntr=0;
	      if(fmod((double)cntr, (double)2) != 0.0)
		  estart=0;
	      
	      (void)getphase(cntr,echosize, etl,cntr_phs,data,transflag);

	      /* zero the phase map */
	      for(i=0;i<etl*echosize;i++)
	      {
		  result[2*i]=1.0;
		  result[2*i+1]=0.0;
	      }
	      (void)getphase((cntr+1),echosize,etl,phsline,data,transflag);
	      for(i=0;i<echosize;i++)
		  phsline[i]=phsline[i]-cntr_phs[i];
	      (void)phaseunwrap(phsline,echosize,uphs); 
	      /* fit to polynomial and return polynomial evaluation */
	      (void)polyfit(uphs,echosize,fitphs,1); /* zero order means only first order here */
	      for(iecho=estart;iecho<etl;iecho+=2)
	      {
		  (void)putphase(iecho,echosize,etl,fitphs,result,transflag); 
	      }
	  }
	  break;	  
	} /* end of switch statement */
	
	(void)free((void *)cntr_phs);
	(void)free((void *)phsline);
	(void)free((void *)uphs);
	(void)free((void *)fitphs);
	
    }
    return(OK);
}  

   int getphase(int iecho, int echolength, int nviews, double *phsptr, float *data, int transposed)
   {
       int i,offset;   
    
       if(transposed)
       {
	   for (i=0;i<echolength;i++)      
	   {
	       offset=iecho+nviews*i; /* data stored column-wise */
	       offset=offset*2;       /* since data is complex   */
	       *(phsptr+i)=atan2((double)data[offset+1],(double)data[offset]); 
	   }
       }
       else
       {
	   for (i=0;i<echolength;i++)      
	   {
	       offset=iecho*echolength+i; /* data stored row-wise */
	       offset=offset*2;       /* since data is complex   */
	       *(phsptr+i)=atan2((double)data[offset+1],(double)data[offset]); 
	   }
       }

       return(OK);
   }  

  int phaseunwrap(double *input, int npts, double *output)
   {
       int i;
       int j=0;
       double cutoff = M_PI*.99;
       double diff;
       double min=input[0];

       for(i=1;i<npts;i++)
       {
	   if(input[i]<min)min=input[i];
       }
       for(i=0;i<npts;i++)
       {
	   input[i]=(double)fmod((float)(input[i]-min),2*M_PI);
	   input[i]=input[i]+min;
       }

       output[0]=input[0];
       for (i=1;i<npts;i++)  
       {
	   diff=*(input+i)-*(input+i-1);
	   if(diff>cutoff)
	       j--;
	   else if(diff<-1*cutoff)
	       j++;
	   output[i]=input[i]+j*2*M_PI;
       }
       
       return(OK);
   }  

int polyfit(double *input, int npts, double *output, int order)
   {
       int i;
       int center=npts/2;
       int width;
       int nfit;
       double sx=0.0;
       double sy=0.0;
       double ssx=0.0;
       double sxy=0.0;         
       double x,y;
       double b0, b1;
       
       /*        width=PC_FIT_WIDTH; */
       width=PC_FIT_WIDTH*npts/100;
       nfit=2*width;

       if((order==0)||(order==1))    /* linear fit */
       {
	   for (i=center-width;i<center+width;i++)
	   {
	       x=(double)i;
	       y=input[i];
	       sx += x;
	       sy += y;
	       ssx += x*x;
	       sxy += x*y;
	   }
	   b1=(nfit*sxy-sx*sy)/(nfit*ssx-sx*sx);
	   b0=(sy-b1*sx)/nfit;

	   /* generate evaluation */
	   if(order==0)  /* skip zeroth order for M. Gyngell */
	     for(i=0;i<npts;i++)
	       output[i]=b1*i;
	   if(order==1)
	     for(i=0;i<npts;i++)
	       output[i]=b0+b1*i;

       }
       else /* 2nd order */
       {
	   double b11;
	   double sx4=0.0;
	   double sx2y = 0.0;
	   double sx3=0.0;
	   double x2;
	   double a[3][3];	
	   double b[3][1];
	   double **aa, **bb;

	   for (i=center-width;i<center+width;i++)
	   {
	       x=(double)i;
	       y=input[i];
	       x2=x*x;
	       sx += x;
	       sy += y;
	       ssx += x2;
	       sxy += x*y;
	       sx3 += x2*x;
	       sx2y += x2*y;
	       sx4 += x2*x2;
	   }
	   /* set up pointers to pointers for NR routine */
	  aa=(double **)malloc(3*sizeof(double*));
	  bb=(double **)malloc(3*sizeof(double*));
	  for (i=0;i<3;i++)
	  {
	      aa[i]=&(a[i][0])-1;
	      bb[i]=&(b[i][0])-1;
	  }
	  /* fill in the matrices */
	  a[0][0]=(double)nfit;
	  a[0][1]=sx;
	  a[0][2]=ssx;
	  a[1][0]=sx;
	  a[1][1]=ssx;
	  a[1][2]=sx3;
	  a[2][0]=ssx;
	  a[2][1]=sx3;
	  a[2][2]=sx4;
	  b[0][0]=sy;
	  b[1][0]=sxy;
	  b[2][0]=sx2y;

	  /* fire it up !*/
	  (void)gaussj(aa-1, 3, bb-1,1);
	  /* bb has the coefficients */
	  b0=b[0][0]; b1=b[1][0]; b11=b[2][0];
       	   /* generate evaluation */
	   for(i=0;i<npts;i++)
	   {
	       output[i]=b0+b1*i+b11*i*i;	   
	   }
  
       }
    
       
       return(OK);
   }  


/*----------------------------------------------------------------------*/
/* pcmapapply								*/
/*	This routine will apply the complex phase correction values	*/
/*	cos(theta),sin(theta) from the indexed location in pcmap to 	*/
/*	the current datafile data values.				*/
/*----------------------------------------------------------------------*/
static int pcmapapply(MFILE_ID pcmap_md, int blk_index)
{
   struct datafilehead *pcmap_file_head;
   struct datafilehead	tmpdatahead;
   dpointers	outblock;
   int cblock;
   int res;				/* flags */

   /* Set pcmap file header */
   pcmap_file_head = (struct datafilehead *)pcmap_md->mapStrtAddr;
   pcmap_md->offsetAddr = pcmap_md->mapStrtAddr;

   /* Get datafile info */
   if ( (res = D_gethead(D_DATAFILE, &tmpdatahead)) )
   {
     D_error(res);
     return(ERROR);
   }

   pcmap_md->newByteLen = pcmap_md->byteLen;

   if (pcmap_md->byteLen == 0)
   {
	Werrprintf( "pcmapapply: No correction values in pcmap file." );
	return(ERROR);
   }
   else
   {
	int bbytes;
	/* check for file consistentcy */
	if (pcmap_file_head->np != tmpdatahead.np)
	{
	   Werrprintf("pcmapapply: Inconsistent np between pcmap and data.");
	   return(ERROR);
	}
	if (pcmap_file_head->ntraces != tmpdatahead.ntraces*tmpdatahead.nblocks)
	{
	   Werrprintf("pcmapapply: Inconsistent ntraces between pcmap and data.");
	   return(ERROR);
	}
	bbytes = tmpdatahead.bbytes;
	if (tmpdatahead.nblocks > 1)
		bbytes += (tmpdatahead.nblocks-1)*tmpdatahead.ntraces*
							tmpdatahead.tbytes;
	if (pcmap_file_head->bbytes != bbytes)
	{
	   Werrprintf("pcmapapply: Inconsistent bbytes between pcmap and data.");
	   return(ERROR);
	}
	if (blk_index > (pcmap_file_head->nblocks))
	{
	   Werrprintf( "pcmapapply: blk_index out of range." );
	   return(ERROR);
	}
	/* advance pointer locations to start of data */
 	pcmap_md->offsetAddr += sizeof(struct datafilehead);
 	pcmap_md->offsetAddr += pcmap_file_head->bbytes*(blk_index - 1);
   }


   /*-------------------------------------------*/
   /* Advance pcmap pointer .			*/
   /*-------------------------------------------*/
   pcmap_md->offsetAddr += sizeof(struct datablockhead);

  /**********************************
   *  Start processing data blocks.  *
   **********************************/

   for (cblock = 0; cblock < tmpdatahead.nblocks; cblock++)
   {
	double d1real,d1complex;
	float *d1, *pcdata;
	int j;

	if ( (res = D_getbuf(D_DATAFILE, tmpdatahead.nblocks, cblock, &outblock)) )
	{
           D_error(res); 
           return(ERROR); 
 	}

	/*------------------------------*/
	/*  Start processing data.  	*/
	/*------------------------------*/
	pcdata = (float *)pcmap_md->offsetAddr;
	d1 = (float *)outblock.data;

	for (j=0; j<tmpdatahead.np*tmpdatahead.ntraces; j+=2)
	{
	    /* rcos(a+b) = rcos(a)*cos(b) - rsin(a)*sin(b)	*/
	    /* rsin(a+b) = rsin(a)*cos(b) + rcos(a)*sin(b)	*/

	    d1real = d1[j]*pcdata[j] - pcdata[j+1]*d1[j+1];
	    d1complex =  pcdata[j]*d1[j+1] + d1[j]*pcdata[j+1];
	    d1[j] = (float)d1real;
	    d1[j+1] = (float)d1complex;
	
	}
	pcmap_md->offsetAddr += 
			(sizeof(float)*tmpdatahead.np*tmpdatahead.ntraces);
	if ( (res = D_markupdated(D_DATAFILE, cblock)) )
	{
	    D_error(res);
            return(ERROR); 
	}
	if ( (res = D_release(D_DATAFILE, cblock)) )
	{
	    D_error(res);
            return(ERROR); 
	}
   }
   return(OK);
}


/*----------------------------------------------------------------------*/
/* estdimsiz								*/
/*	Used to estimate dimension size for mmap open of cpmap file	*/
/*----------------------------------------------------------------------*/
static int estdimsiz(int *fnvar, int *fn1var)
{
double rval;
int r;
vInfo        info;
/******************************************
*  Get appropriate 1D or 2D-F2 FN values  *
******************************************/
 
   if ( (r = P_getreal(CURRENT, "fn", &rval,1)) )
   {
      P_err(r, "current ", "fn:");
      return(ERROR);
   }     
   else if ( (r = P_getVarInfo(CURRENT, "fn", &info)) )
   {
      P_err(r, "info?", "fn:");
      return(ERROR);
   }
   else
   {
	if (info.active)
	{
	   *fnvar = (int) (rval + 0.5);
	}
	else
	{
	   if ( (r = P_getreal(PROCESSED, "np", &rval, 1)) )
   	   {
   	      P_err(r, "processed ", "np:");
   	      return(ERROR);
   	   }
   	   else
   	   {
      	      *fnvar = (int) (rval + 0.5);
   	   }
	}
   }
 

/************************************
*  Get appropriate 2D-F1 FN values  *
************************************/
 
   if ( (r = P_getreal(CURRENT, "fn1", &rval, 1)) )
   {
      P_err(r, "current ", "fn1");
      return(ERROR);
   }
   else if ( (r = P_getVarInfo(CURRENT, "fn1", &info)) )
   {
      P_err(r, "info? ", "fn1");
      return(ERROR);
   }
   else
   {
	if (info.active)
	{
	   *fn1var = (int) (rval + 0.5);
	}
	else
	{
	   /* Determine fn1 dimension */
	   if ( (r = P_getreal(PROCESSED, "nv", &rval, 1)) )
   	   {
		double r1val;
	   	r = P_getreal(PROCESSED, "nf", &rval, 1);
	   	r = P_getreal(PROCESSED, "ni", &r1val, 1);
		if (r1val > rval) rval=r1val;
   	   }
   	   else
   	   {
	     if (rval < 0.5)
	     {
		double r1val;
	   	r = P_getreal(PROCESSED, "nf", &rval, 1);
	   	r = P_getreal(PROCESSED, "ni", &r1val, 1);
		if (r1val > rval) rval=r1val;
	     }
   	   }
	   /* set fn1var to rval */
	   *fn1var = 2 * (int) (rval + 0.5);
	   if (*fn1var <= 0) *fn1var = 1;
	}
   }

   return(OK);

}


#define TABSCOPEN	1
#define TABSCAPPLY	2

#define NOMINALTABLESIZE	1024
#define MAXTABLESIZE		1024*8

#define getdatatype(status)						\
  	( (status & S_HYPERCOMPLEX) ? HYPERCOMPLEX :			\
    	( (status & S_COMPLEX) ? COMPLEX : REAL ) )


static int *table = NULL;

static  int tabexplicit_open = FALSE;
/******************************************************************************/
int tcdata ( argc, argv, retc, retv )
/* 
Purpose:

Arguments:
---------
argc  :  (   )  Argument count.
argv  :  (   )  Command line arguments.
retc  :  (   )  Return argument count.  Not used here.
retv  :  (   )  Return arguments.  Not used here.
*/


int argc, retc;
char *argv[], *retv[];
/*ARGSUSED*/

{ /* Begin function */
   /*
   Local Variables:
   ---------------
   tcdata_file	:  Complete file path to the source fid file .
   dest_fid  	:  Complete file path to the destination fid file .
   a_opt	: Defines source exp option 1=arg specified 0=not specfied
   destsize	: Size value to use when opening destination file.
   */
   FILE    *table_ptr;
   char   tcdata_file[MAXPATHL];
   int	a_opt, action, tempsize,status;
   int  i,res,done;

   /*---------------------------*/
   /* Begin Executable Code:	*/
   /*---------------------------*/
   /* Verify that the user has passed correct number of arguments */
   a_opt = 0;
   if ( !strcmp(argv[0],"tcapply") )
   {
	if ((argc < 1) || (argc > 2))
	{ 
	   Werrprintf("Usage: tcapply([<tblfilename>]).");
	   ABORT;
	}
	(void)strcpy ( tcdata_file, userdir );
	(void)strcat ( tcdata_file, "/tablib/" );
	if (argc > 1)
	{
	   (void)strcat ( tcdata_file, argv[1] );
	   a_opt = 1;
	}
	else
	{
	   if (index_array == NULL)
	   {
	   	Werrprintf ( "Supply tablename as argument or use tcopen." );
	   	ABORT;
	   }
	}

	action = TABSCAPPLY;

   }
   else if ( !strcmp(argv[0],"tcopen") )
   {
	if (argc != 2)
	{ 
	   Werrprintf ( "Usage: tcopen(<filename>)." );
	   ABORT;
	}
	tabexplicit_open = TRUE;
	(void)strcpy ( tcdata_file, userdir );
	(void)strcat ( tcdata_file, "/tablib/" );
	(void)strcat ( tcdata_file, argv[1] );
 	action = TABSCOPEN;
  }
   else if (  !strcmp(argv[0],"tcclose") )
   {
	if (argc > 1)
	{ 
	   Werrprintf ( "Usage: tcclose." );
	   ABORT;
	}
	tabexplicit_open = FALSE;
	free(index_array);
	index_array = NULL;
   	/* Normal, successful return */
   	RETURN;
   }
   else
   {
	   Werrprintf ( "tcdata: Incorrect command." );
	   ABORT;
   }



   /*------------------------------------*/
   /* Open and Read in Table file	 */
   /*------------------------------------*/

    /****
    * Open table file:
    ****/
   if (index_array == NULL)
   {
       if ((table_ptr = fopen(tcdata_file, "r")) == NULL) 
       {
	   (void)strcpy ( tcdata_file, systemdir );
	   (void)strcat ( tcdata_file, "/tablib/" );
	   (void)strcat ( tcdata_file, argv[1] );
	   if ((table_ptr = fopen(tcdata_file, "r")) == NULL)
	   {
	       Werrprintf("tcdata: can't open table file: %s.\n",tcdata_file);
	       ABORT;   
	   }
       }	


       /****
	* Read in the table.  A separate index array is constructed at this
	* time.  This index array is sorted by qsort, using as criteria the
	* values pointed to in the table array by the entries in index.
	****/
       tempsize = NOMINALTABLESIZE;
       table = (int *)malloc(tempsize*sizeof(int));
       index_array = (int *)malloc(tempsize*sizeof(int));
       while (fgetc(table_ptr) != '=') ;
       i = 0;
       done = 0;
       while ((i<MAXTABLESIZE) && !done)
       {
	   while ((i < tempsize) && (fscanf(table_ptr, "%d", &table[i]) == 1)) 
	   {
	       index_array[i] = i;
	       i++;
	   }
	   if (i == tempsize)
	   {
	       tempsize += NOMINALTABLESIZE;
	       table = (int *)realloc(table,tempsize*sizeof(int));
	       index_array = (int *)realloc(index_array,tempsize*sizeof(int)); 
	   }
	   else done = 1;
       }
       if (i >= MAXTABLESIZE)
       {
	   Werrprintf("tcdata: table length overrun.\n");
	   free(table);
	   table = NULL;
	   free(index_array);
	   index_array = NULL;
	   ABORT;   
       }
       tablesize = i;
       /* fprintf(stderr,"tabsc: tablesize = %d\n",tablesize); */
       if (fclose(table_ptr))
       {
	   Werrprintf("tabc: trouble closing table file.\n");
	   free(table);
	   table = NULL;
	   free(index_array);
	   index_array = NULL;
	   ABORT;   
       }
       qsort(index_array, tablesize, sizeof(int), intcompare);
       free(table);
       table = NULL;
   }  /* end of if index_array == NULL */

    /****
    * At this point the index array has been rearranged so that its
    * entries, in ascending order, hold the index values required to 
    * unscramble the table array, and therefore the data array.
    *
    * Example:           table =  2   0  -1   1  -2
    * index starts as:   index =  0   1   2   3   4
    * After sorting,     index =  4   2   1   3   0
    *
    * Now, we can write out the 4th element of data, followed by the
    * 2nd, 1st, 3rd, and 0th, resulting in data which appears as if it 
    * had been acquired in the order: -2  -1  0  1  2.
    ****/

   status = OK;
   switch (action) {

	case TABSCAPPLY:
	  {
	   /*---------------------------*/
	   /*  Process data blocks.	*/
	   /*---------------------------*/
	   /*-----------------------------------------------------------*/
	   /* It is important to note that the data is stored in an	*/
	   /* inverted order for the second dimension data processing.	*/
	   /* In other words the transformed fids are stored in column	*/
	   /* order.  np/2 is the number of spectrum traces and 	*/
	   /* ntraces*2 is the number of points.			*/
	   /*-----------------------------------------------------------*/

	   float *dataptr;
	   float *sdata = NULL;
	   int malloc_size, trace_size, j, block, numblocks, dstatus;
	   dpointers tmpdatablock;
	   dfilehead	tmpdatahead;

	   dstatus = OK;
           numblocks = trace_size = 0;
	   if ( (res = D_gethead(D_DATAFILE, &tmpdatahead)) )
	   {
	     	D_error(res);
		Werrprintf("tcapply: Unable to get data header\n");
		dstatus = ERROR;
		status=dstatus;
	   }
	   if(dstatus==OK)
	   {
	       numblocks = tmpdatahead.nblocks;
	       trace_size = (tmpdatahead.ntraces*2)*sizeof(float);
	       if ((tmpdatahead.np/2) < tablesize)
	       {
		   Werrprintf("tcapply: Num spectra less than tablesize\n");
		   dstatus = ERROR;
		   status=dstatus;
	       }
	       else
	       {
		   malloc_size = 
		       (tmpdatahead.ntraces*2)*numblocks*sizeof(float)*tablesize;
		   sdata = (float *) malloc(malloc_size);
		   if (sdata == NULL)
		   {
		       Werrprintf("tcapply: Unable to alloc temp buffer, size=%d\n",
						malloc_size);
		       dstatus = ERROR;
		       status=dstatus;
		   }
	       }
	   }
	   /*-------------------*/
	   /*  Sort Traces.  	*/
	   /*-------------------*/
	   for (block = 0; (block < numblocks) && (dstatus==OK); block++)
	   {
	     if (D_getbuf(D_DATAFILE,numblocks,block,&tmpdatablock))
	     {
		if (tmpdatablock.head == NULL) 
		{
		   dstatus = ERROR;
		   status=dstatus;
		}
	     }

	     j = 0;
	     while ((j<tablesize) && (dstatus==OK))
             {
		int sdataindex;
		/* index into the correct trace (row) */
		sdataindex = j*tmpdatahead.ntraces*COMPLEX*numblocks
					+block*tmpdatahead.ntraces*COMPLEX;
		/* index into the correct trace (column) */
		dataptr = tmpdatablock.data + index_array[j]*COMPLEX;
		movmem((char *)dataptr,(char *)(&sdata[sdataindex]),trace_size,
						tmpdatahead.np/2,8);
		j++;
             }
	     if ( (res = D_release(D_DATAFILE, block)) )
	     {
	        D_error(res);
	        dstatus = ERROR;
		status=dstatus;
	     }
	   }
	   /*------------------------------*/
	   /*  Copy sorted data back.  	*/
	   /*------------------------------*/
	   for (block = 0; (block < numblocks) && (dstatus==OK); block++)
	   {
	     if (D_getbuf(D_DATAFILE,numblocks,block,&tmpdatablock))
	     {
		if (tmpdatablock.head == NULL) 
		{
		   dstatus = ERROR;
		   status=dstatus;
		}
	     }
	     trace_size = tablesize*COMPLEX*sizeof(float);
	     j = 0;
	     while ((j<tmpdatahead.ntraces) && (dstatus==OK))
             {
		int sdataindex;
		/* pointsperinc = pointsperspec; */
		sdataindex = j*COMPLEX+block*tmpdatahead.ntraces*COMPLEX;
		dataptr = tmpdatablock.data + j*tmpdatahead.np;
		movmem((char *)(&sdata[sdataindex]),(char *)dataptr,trace_size,
			tmpdatahead.ntraces*numblocks,8);
		j++;
	     }
	     if ( (res = D_markupdated(D_DATAFILE, block)) )
	     {
	        D_error(res);
	        dstatus = ERROR;
		status=dstatus;
	     }
	     if ( (res = D_release(D_DATAFILE, block)) )
	     {
	        D_error(res);
	        dstatus = ERROR;
		status=dstatus;
	     }
	   }
	   if(sdata != NULL)
	       {
		   free(sdata);
	       }
	   }
	   break;
	case TABSCOPEN:
	   break;
	default:
	   Werrprintf( "tcdata: Should not get to this default switch stmt." );
	   break;
   }

   if ((!tabexplicit_open) || (status == ERROR))
   {
	free(index_array);
	(index_array) = NULL;
   }
   

   /* Normal, successful return */
   RETURN;


} /* End function tcdata */

/*****************************************************************
*  This routine is used by the qsort function to provide the
*  comparison for sorting.  It compares the two entries in
*  "table" array which have indeces defined by the values of the 
*  elements in the "index" array.
*****************************************************************/
static  int intcompare(const void *i, const void *j)
{
    return(table[*((int *)i)] - table[*((int *)j)]);
}


/******************************************************************************/
int rsdata ( argc, argv, retc, retv )
/* 
Purpose:

Arguments:
---------
argc  :  (   )  Argument count.
argv  :  (   )  Command line arguments.
retc  :  (   )  Return argument count.  Not used here.
retv  :  (   )  Return arguments.  Not used here.
*/


int argc, retc;
char *argv[], *retv[];
/*ARGSUSED*/

{ /* Begin function */
   /*
   Local Variables:
   ---------------
   a_opt	: Defines source exp option 1=arg specified 0=not specfied
   */
   int	a_opt, status;
   int  res,spectrum_index;
   float *sdata,*dataptr;
   int malloc_size, trace_size, j, block, numblocks, dstatus, datatype;
   dpointers tmpdatablock;
   dfilehead	tmpdatahead;


   /*---------------------------*/
   /* Begin Executable Code:	*/
   /*---------------------------*/
   /* Verify that the user has passed correct number of arguments */
   a_opt = 0;
   spectrum_index = 1;
   if ( !strcmp(argv[0],"rsapply") )
   {
	if (argc != 2)
	{ 
	   Werrprintf("Usage: rsapply(<index>).");
	   ABORT;
	}
	if (isReal(argv[1+a_opt])) 
	   spectrum_index=(int)stringReal(argv[1]);
	else {
	   Werrprintf ( "rsapply: Illegal index number.");
	   ABORT;
	}
	if (spectrum_index <= 0)
	{
	   Werrprintf ( "rsapply: Illegal index number.");
	   ABORT;
	}

   }
   else
   {
	   Werrprintf ( "rsdata: Incorrect command." );
	   ABORT;
   }


   status = OK;
   /*---------------------------*/
   /*  Process data blocks.	*/
   /*---------------------------*/
   /*-----------------------------------------------------------*/
   /* It is important to note that the data is stored in an	*/
   /* inverted order for the second dimension data processing.	*/
   /* In other words the transformed fids are stored in column	*/
   /* order.  np/2 is the number of spectrum traces and 	*/
   /* ntraces*2 is the number of points.			*/
   /*-----------------------------------------------------------*/

   dstatus = OK;
   if ( (res = D_gethead(D_DATAFILE, &tmpdatahead)) )
   {
    	D_error(res);
	dstatus = ERROR;
   }
   numblocks = tmpdatahead.nblocks;
   datatype = (tmpdatahead.status & S_COMPLEX) ? COMPLEX : REAL; /* complex=2 */
								 /* real=1 */

   malloc_size =(tmpdatahead.ntraces*datatype*sizeof(float))*numblocks;
   trace_size = (tmpdatahead.ntraces*datatype)*sizeof(float);
   sdata = (float *) malloc(malloc_size);
   if (sdata == NULL)
   {
	  Werrprintf("rsapply: Unable to alloc temp buffer, size=%d\n",
						malloc_size);
	  dstatus = ERROR;
   }
   /*-------------------*/
   /*  Sort Traces.  	*/
   /*-------------------*/
   for (block = 0; (block < numblocks) && (dstatus==OK); block++)
   {
	int sdataindex;
	if (D_getbuf(D_DATAFILE,numblocks,block,&tmpdatablock))
	{
	   if (tmpdatablock.head == NULL) 
	   {
		dstatus = ERROR;
	   }
	}

	/* index into the correct trace (column) */
	sdataindex = block*tmpdatahead.ntraces*datatype;
	dataptr = tmpdatablock.data + spectrum_index*datatype;
        movmem((char *)dataptr,(char *)(&sdata[sdataindex]),trace_size,tmpdatahead.np/2,
					datatype*tmpdatahead.ebytes);
	if ( (res = D_release(D_DATAFILE, block)) )
	{
	        D_error(res);
	        dstatus = ERROR;
	}
   }
   /*--------------------------------------*/
   /*  Copy back in reverse order.		*/
   /*--------------------------------------*/
   for (block = 0; (block < numblocks) && (dstatus==OK); block++)
   {
	int sdataindex;
	float *datastart;
	if (D_getbuf(D_DATAFILE,numblocks,block,&tmpdatablock))
	{
	   if (tmpdatablock.head == NULL) 
	   {
		dstatus = ERROR;
	   }
	}
	sdataindex = numblocks*tmpdatahead.ntraces*datatype - datatype;
	sdataindex = sdataindex - block*tmpdatahead.ntraces*datatype;
	datastart = tmpdatablock.data + spectrum_index*datatype;
	j = 0;
	while ((j<tmpdatahead.ntraces) && (dstatus==OK))
        {
	    dataptr = datastart + j*tmpdatahead.np ;
	    memcpy(dataptr,&sdata[sdataindex],datatype*tmpdatahead.ebytes);
	    j++;
	    sdataindex = sdataindex-datatype;
	 }
	 if ( (res = D_markupdated(D_DATAFILE, block)) )
	 {
	    D_error(res);
	    dstatus = ERROR;
	 }
	 if ( (res = D_release(D_DATAFILE, block)) )
	 {
	    D_error(res);
	    dstatus = ERROR;
	 }
   }
   free(sdata);

  /* Normal, successful return */
   RETURN;

} /* End function tcdata */

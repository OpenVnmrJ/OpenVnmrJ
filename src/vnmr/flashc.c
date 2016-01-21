/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/******************************************************************************
* File flashc.c:  Contains code for the Vnmr "flash convert" function.	      *
*      --------   This is a temporary piece of code which will eventually be  *
* replaced by a more elegant solution.  The fid file in the current	      *
* experiment is converted from a (temporary?) "flash" format into the format  *
* of a standard	two-dimensional fid file.  Also, process parameters nf, ni,   *
* arraydim, arrayelemts, and cf are edited.  The result of all this is that   *
* the current experiment will appear to contain the results of a standard     *
* two dimensional experiment.						      *

* Modified to accept a single argument, ncomp, defining the number of         *
* compressed traces to retain for each "ni".  Allows compressed phase         *
* encoding and compressed multislice and/or multiecho simultaneously.         *
* 9-4-92  A. Rath                                                             *
******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include "data.h"
#include "group.h"
#include "mfileObj.h"
#include "symtab.h"
#include "vnmrsys.h"
#include "variables.h"
#include "pvars.h"
#include "wjunk.h"


/* A very incomplete list of the routines contained in this file */
static int flash_interlock();
static int get_flashc_parms (double *parraydim, double *pnf, double *pni );
static int flash_convert (char *default_fid, char *temp_fid, double arraydim,
                          int format, double ncomp, double necho, double nf, double *pni );
static int nf_flash_convert (char *default_fid, char *temp_fid,
           double arraydim, int format, double ncomp, double necho,
           double nf, double *pni );
void Vperror (char *usr_string );
extern int fidversion(void *headptr, int headertype, int version);

/******************************************************************************/
int flashc (int argc, char *argv[], int retc, char *retv[] )
/* 
Purpose:
-------
     Routine flashc is the main routine for the "flash convert" function.  It
edits the fid file in the current experiment, as well as process parameters nf,
ni, arraydim, arrayelemts, and cf to do the flash data format to standard 
format conversion.

Arguments:
---------
argc  :  (   )  Argument count.
argv  :  (   )  Command line arguments.
retc  :  (   )  Return argument count.  Not used here.
retv  :  (   )  Return arguments.  Not used here.
*/

/*  9-4-92
A single argument is allowed at this time to specify the number
of compressed traces to be left as a compressed nf block.  The use
for this is to allow compressed multislice or multiecho simultaneous
with compressed phase encoding.

    9-10-92
Argument list expanded to allow both compressed multi-slice, and
compressed multi-image formats.  The difference in format is:
(numerical values indicate acquisition order of traces)

  pe    1  2  3  4  5    ns ==>
  ||    6  7  8  9  10
  ||    11 12 13 14 15
  \/    ...
	..........  XX  where XX = phase encode * slices


  pe    1   pe+1  2pe+1  compressed images ==>
  ||    2   pe+2  2pe+2
  ||    3   pe+3  2pe+3
  \/    ...
	pe  pe*2 ... XX  where XX = phase encode * images


"rare" type data acquisition.

  pe    1   ne+1  2ne+1  ns ==>
  ||    2   ne+2  2ne+2
  ||    3   ne+3  2ne+3
  ||    ...
  ||	ne  ne*2 ... XX  where XX = num echoes (ne) * slices (ns)
  ||	
  ||	ne*ns+1  ne*ns+ne+1  ne*ns+2ne+1
  ||	ne*ns+2  ne*ns+ne+2  ne*ns+2ne+2
  ||	...
  ||	ne*ns+ne ne*ns+ne*2  ne*ns+ne*ns
  ||	.
  \/	2(ne*ns)+ne 2(ne*ns)+ne*2  2(ne*ns)+ne*ns
	.	
	... 		XX where XX = nv/ne*(ne * ns)

	


Examples of use from vnmr:
    flashc              simple compressed phase-encode
    flashc('ms',ns)     compressed phase-encode and compressed multi-slice
    flashc('mi',ns)     compressed multi-image and compressed phase-encode
    flashc('rare',ns,ne) compressed echo-train, multi-slice, and phase-encode

 'nf' argument added to reformat the data into the following:
    slice1 image, slice2 image, slice3 image,..., slicen image.

    flashc('nf')         simple compressed phase-encode
    flashc('nf','ms',ns) compressed phase-encode and compressed multi-slice
    flashc('nf','mi',ns) compressed multi-image and compressed phase-encode
    flashc('nf','rare',ns,ne) compressed echo-train, multi-slice, and 
								phase-encode
*/
{ /* Begin function flashc */
   /*
   Local Variables:
   ---------------
   default_fid  :  Complete file path to the fid file in the current experiment.
   temp_fid     :  Complete file path to a temporary version of the fid file in
		   the current experiment.
   nf		:  The Vnmr process parameter of the same name.
   ni		:  Ditto.
   ncomp	:  Number of traces to keep in compressed nf form.
   arraydim     :  Value of Vnmr process parameter arraydim.
   arrayelemts  :  Value of Vnmr process parameter arrayelemts.
   r            :  Return value of a function.
   format       :  image compression format; 0 = multi-slice, 1 = multi-image.
   */
   char   default_fid[MAXPATHL], temp_fid[MAXPATHL];
   int    format, r, nfarg;
   double ncomp, necho, nf, ni, arraydim, arrayelemts;

/* A convenience macro */
#define P_SETPAR(name,value)			\
   if (( r=P_setreal(PROCESSED,name,value,1)))	\
   {  P_err(r,name,":");			\
      ABORT;					\
   }						\
   if ( ( r=P_setreal(CURRENT,name,value,1)) )	\
   {  P_err(r,name,":");			\
      ABORT;					\
   }
   /*
   Begin Executable Code:
   --------------------
   */
   /* Verify that the user has passed correct number of arguments */
   if ( argc > 5 )
   {  Werrprintf ( "Flashc: wrong number of arguments." );
      ABORT;
   }

   nfarg = 0;
   /* Look for 'nf' argument */
   if (argc > 1)
   {
      if ( !strcmp(argv[1],"nf") )
      {
	nfarg = 1;
      }
   }
   /* Look for any arguments */
   if ( argc == 4+nfarg) {
      if ( !strcmp(argv[1+nfarg],"rare") )
      {
	  format = 2;
	  ncomp = atof(argv[2+nfarg]);
	  necho = atof(argv[3+nfarg]); /* necho= echo train of phase encode steps */
      }
      else
      {  Werrprintf ( "Flashc: unrecognized conversion type." );
         ABORT;
      }
   }
   else if ( argc == 3+nfarg ) {
      ncomp = atof(argv[2+nfarg]);
      necho = 1.0;
      if ( !strcmp(argv[1+nfarg],"ms") )
	  if (nfarg)
		format = 1;
	  else
		format = 0;
      else if ( !strcmp(argv[1+nfarg],"mi") )
	  if (nfarg)
		format = 0;
	  else
	  	format = 1;
      else
      {  Werrprintf ( "Flashc: unrecognized conversion type." );
         ABORT;
      }
   }
   else {
      ncomp = 1.0;
      necho = 1.0;
      format = 0;
   }


   /* Do an "interlock" to insure that flashc is called only once */
   if ( flash_interlock() != 0 )
      RETURN;


   /* Generate paths to the fid file and a parallel, temporary file. */
   (void)strcpy ( default_fid, curexpdir );
   (void)strcpy ( temp_fid, curexpdir );
#ifdef UNIX
   (void)strcat ( default_fid, "/acqfil/fid" );
   (void)strcat ( temp_fid, "/acqfil/flashc.temp" );
#else 
   vms_fname_cat ( default_fid, "[.acqfil]fid" );
   vms_fname_cat ( temp_fid, "[.acqfil]flashc.temp" );
#endif 

   /* Obtain the values of the process parameters */
   if ( get_flashc_parms ( &arraydim, &nf, &ni ) != 0 )
   {
      P_deleteVar ( PROCESSED, "flash_converted" );
      P_deleteVar ( CURRENT,   "flash_converted" );
      ABORT;
   }

   if (nfarg && (ncomp == 1.0)) ncomp = nf;

   /* Check nf to make sure it is divisible by ncomp */
   if ( (int)floor(nf+0.5) % (int)floor(ncomp+0.5) > 0 )
   {  Werrprintf ( "Flashc: ncomp is not a factor of nf." );
      P_deleteVar ( PROCESSED, "flash_converted" );
      P_deleteVar ( CURRENT,   "flash_converted" );
      ABORT;
   }
   /* Display status on screen */
   disp_status ( "FLASHC" );

   if (nfarg)
   {
   	/* Edit the fid file, may also revise parameters nf and ni. */
   	if ( nf_flash_convert ( default_fid, temp_fid, arraydim,
     	         format, ncomp, necho, nf, &ni ) != 0 )
        {
           P_deleteVar ( PROCESSED, "flash_converted" );
           P_deleteVar ( CURRENT,   "flash_converted" );
           disp_status ( "      " );
   	   ABORT;
        }
   }
   else
   {
   	/* Edit the fid file, may also revise parameter ni. */
   	if ( flash_convert ( default_fid, temp_fid, arraydim,
   	         format, ncomp, necho, nf, &ni ) != 0 )
        {
           P_deleteVar ( PROCESSED, "flash_converted" );
           P_deleteVar ( CURRENT,   "flash_converted" );
           disp_status ( "      " );
   	   ABORT;
        }
   }

   disp_status ( "      " );
   /* Write out the new parameter values */
   if (nfarg)
   {
   	P_SETPAR ( "nf", floor (ni*(nf / ncomp)+0.5) );
   	P_SETPAR ( "ni", 1.0 );
   }
   else
   {
   	P_SETPAR ( "nf", ncomp );
   	P_SETPAR ( "ni", floor ((nf / ncomp)+0.5) );
   }

   if ( arraydim > 1 )
      arrayelemts = 2;
   else
      arrayelemts = 1;
   P_SETPAR ( "arrayelemts", arrayelemts );

   if (nfarg)
      arraydim = floor(((arraydim/ni)*ncomp)+0.5);
   else
      arraydim *= floor ((nf / ncomp)+0.5);
   P_SETPAR ( "arraydim", arraydim );
   if ( ( r = P_setreal ( CURRENT, "cf", (float)1, 1 ) ) )
   {  P_err ( r, "cf", ":" );
      ABORT;
   }

   /* Display new parameters, and clear status on screen */
   appendvarlist("nf,ni,arrayelemts,arraydim,cf");

   Winfoprintf("Flash Convert reformatting completed.");
   /* Normal, successful return */
   RETURN;

/* Since this item was defined within this routine, get rid of it now */
#undef P_SETPAR

} /* End function flashc */

/******************************************************************************/
static int flash_interlock()
/* 
Purpose:
-------
     Routine flash_interlock is used to insure that the "flashc" command is
called only once for a given experiment.  In particular, this routine checks
for the existence of a parameter called flash_converted.  If this parameter
exists, an error condition is returned.  If this parameter does not exist, it
is created.
     To complete this interlock on flashc, the flash_converted parameter should
be deleted (if it exists) whenever an acquisition takes place.

Arguments:  (none)
---------
flash_interlock  :  (  O)  Returns 0 for success, 1 to indicate an error.
*/
{ /* Begin function flash_interlock */
   /*
   Local Variables:
   ---------------
   root  :  The root node of the "processed" variable tree.
   */
   symbol **root;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Obtain the root of the processed tree */
   if ( ( root = getTreeRoot ( "processed" ) ) == NULL )
   {  Werrprintf ( "flashc: getTreeRoot: programming error" );
      ABORT;
   }

   /* Try to create the interlock variable. This call serves */
   /* to indicate whether or not it existed already.	     */
   if ( RcreateVar ( "flash_converted", root, T_REAL ) == NULL )
   {
      /* The interlock already existed, this is an error */
      ABORT;
   }

   /* Normal successful return */
   RETURN;

} /* End function flash_interlock */

/******************************************************************************/
static int get_flashc_parms (double *parraydim, double *pnf, double *pni )
/* 
Purpose:
-------
     Routine get_flashc_parms shall obtain some Vnmr process parameters for 
flashc.

Arguments:
---------
parraydim     :  (  O)  Pointer to value of Vnmr process parameter arraydim.
pnf	      :  (  O)  Pointer to value of Vnmr process parameter nf.
pni	      :  (  O)  Pointer to value of Vnmr process parameter ni.
*/
{ /* Begin function get_flashc_parms */
   /*
   Local Variables:
   ---------------
   r	 :  Return value of a function.
   */
   int r;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Get existing values */
   if ( ( r = P_getreal ( PROCESSED, "arraydim", parraydim, 1 ) ) != 0 )
   {  P_err ( r, "arraydim", ":" );
      ABORT;
   }
   if ( ( r = P_getreal ( PROCESSED, "nf", pnf, 1 ) ) != 0 )
   {  P_err ( r, "nf", ":" );
      ABORT;
   }
   /* "ni" is allowed to not exist */
   /* Value "-1" codes for "not set" */
   if ( P_getreal ( PROCESSED, "ni", pni, 1 ) != 0 )
      *pni = -1;
   if ( *pni == 0 )
      *pni = -1;

   /* Normal successful return */
   RETURN;

} /* End function get_flashc_parms */

/******************************************************************************/
static int flash_convert (char *default_fid, char *temp_fid, double arraydim,
                          int format, double ncomp, double necho, double nf, double *pni )
/* 
Purpose:
-------
     Routine flash_convert shall replace a fid file in flash format with one in
the format of a standard two dimensional image fid.
     This routine also sets the "file" parameter in the CURRENT tree.

Arguments:
---------
default_fid  :  (I  )  The path of the fid file to be edited.
temp_fid     :  (I  )  The path of a parallel scratch file.
arraydim     :  (I  )  (Old) value of the process parameter of the same name.
format       :  (I  )  image compression format; 
			0 = multi-slice,
			1 = multi-image.
			2 = rare-type with pe - echo train.
ncomp        :  (I  )  Number of traces to keep in compressed nf form.
necho	     :  (I  )  Number of echos. (in echo train for format 2)
nf	     :  (I  )  Ditto.
pni	     :  (I/O)  Ditto.
*/

{ /* Begin function flash_convert */
   /*
   Local Variables:
   ---------------
   fid_file    :  Stdio file indicator for the (input) fid file.
   tmp_file    :  Stdio file indicator for the (output) temporary file.
   file_head   :  A datafilehead struct for holding file header.
   block_head  :  A datablockhead struct for holding block header.
   ptrace      :  Allocated space to hold one trace of fid data.
   trace_size  :  Number of bytes per trace of fid data.
   slice_ctr   :  Loop control variable for slices in a multi-slice experiment,
		  or echoes in a multi-echo experiment.
   trace_ctr   :  Loop control variable for traces.
   old_bbytes  :  Block size (including block header) of input (flash format)
		  fid file.
   offset      :  Offset into input file, for reading next trace.
   nbytes      :  Number of bytes desired for a file read or write.
   */
   char *ptrace;
   int  ctr,single_trace_sz,trace_size,slice_ctr,trace_ctr,echo_ctr,nbytes;
   int	inecho,incomp;
   int  blkindex;
   long old_bbytes, offset;
   struct datafilehead file_head;
   struct datablockhead block_head;
   FILE *fid_file, *tmp_file;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Open the two files */
   if ( ( fid_file = fopen ( default_fid, "r" ) ) == 0 )
   {  Vperror ( "flash_convert: fopen (fid file)" );
      ABORT;
   }
   if ( ( tmp_file = fopen ( temp_fid, "w" ) ) == 0 )
   {  Vperror ( "flash_convert: fopen (temporary file)" );
      fclose ( fid_file );
      ABORT;
   }

   /* Obtain file header */
   nbytes = sizeof ( file_head );
   if ( fread ( (char *)&file_head, 1, nbytes, fid_file ) != nbytes )
   {  Vperror ( "flash_convert: fread (file header)" );
      fclose ( fid_file );
      fclose ( tmp_file );
      unlink ( temp_fid );
      ABORT;
   }

   DATAFILEHEADER_CONVERT_NTOH(&file_head);
   /* Check contents before modifying. 		       */
   /* Recall that a value of -1 means "not set yet" */
   if ( *pni == -1 )
   {  if ( ( *pni = file_head.nblocks ) < 1 )
      {  Werrprintf ( "flash_convert: Bad nblocks field in file header" );
         fclose ( fid_file );
         fclose ( tmp_file );
	 unlink ( temp_fid );
	 ABORT;
      }
   }
   else
   {
      if ( file_head.nblocks != *pni )
      {  Werrprintf ( "flash_convert: bad nblocks field in file header" );
         fclose ( fid_file );
         fclose ( tmp_file );
	 unlink ( temp_fid );
	 ABORT;
      }
   }
   if ( file_head.ntraces != nf )
   {  Werrprintf ( "flash_convert: bad ntraces field in file header" );
      fclose ( fid_file );
      fclose ( tmp_file );
      unlink ( temp_fid );
      ABORT;
   }

   /* fprintf(stderr,"flash_convert: ncomp=%g, necho=%g nf=%g pni=%g\n", */
   /*		ncomp, necho, nf, *pni);				 */

   /* Save the size of the input file blocks */
   old_bbytes = file_head.bbytes;

   /* Edit parameters and write file header back out to disk */
   inecho = (int) (necho+0.5);
   incomp = (int) (ncomp+0.5);
   file_head.nblocks = (int)((arraydim * nf / ncomp) + 0.5);
   file_head.ntraces = (int)ncomp;
   single_trace_sz = file_head.tbytes;
   trace_size = (int)ncomp * file_head.tbytes;
   file_head.bbytes  = trace_size + sizeof(block_head);
   
   DATAFILEHEADER_CONVERT_HTON(&file_head);
   if ( fwrite ( (char *)&file_head, 1, nbytes, tmp_file ) != nbytes )
   {  Vperror ( "flash_convert: fwrite (file header)" );
      fclose ( fid_file );
      fclose ( tmp_file );
      unlink ( temp_fid );
      ABORT;
   }

   /* Obtain block header.  It will be repeatedly copied */
   nbytes = sizeof ( block_head );
   if ( fread ( (char *)&block_head, 1, nbytes, fid_file ) != nbytes )
   {  Vperror ( "flash_convert: fread (block header)" );
      fclose ( fid_file );
      fclose ( tmp_file );
      unlink ( temp_fid );
      ABORT;
   }
   blkindex = 0;
   block_head.index = htons(blkindex);

   /* Allocate space to hold a trace of fid data */
   if ( ( ptrace = malloc ( (unsigned)trace_size ) ) == 0 )
   {  Vperror ( "flash_convert: malloc" );
      fclose ( fid_file );
      fclose ( tmp_file );
      unlink ( temp_fid );
      ABORT;
   }

   /* Loop over input slices or echoes */
   for ( slice_ctr = 0; slice_ctr < floor(nf/(ncomp*necho)+0.5); slice_ctr++ )
   {
    for ( echo_ctr = 0 ; echo_ctr < floor(necho+0.5) ; echo_ctr++ )
    {
      /* Display slice or echo number, occasionally */
      if ( (slice_ctr*(inecho) + echo_ctr) % 16 == 0 )
	 disp_index ( slice_ctr );

      /* Loop over input traces */
      for ( trace_ctr = 0 ; trace_ctr < *pni ; trace_ctr++ )
      {
	 /* Write out a block header */
         blkindex++;
         block_head.index = htons(blkindex);
	 nbytes = sizeof ( block_head );
	 if ( fwrite ( (char *)&block_head, 1, nbytes, tmp_file ) != nbytes )
	 {  Vperror ( "flash_convert: fwrite (block header)" );
	    fclose ( fid_file );
	    fclose ( tmp_file );
	    unlink ( temp_fid );
	    ABORT;
	 }

         switch (format) {
	   case 0:  /* compressed phase-encode and compressed multi-slice */
	     /* Compute position in input (flash) file for the current trace */
	     offset = sizeof(file_head) + (trace_ctr)*old_bbytes +
		      sizeof(block_head) + (slice_ctr)*trace_size;
	 
	     /* Position the input file */
	     if ( fseek ( fid_file, offset, 0 ) == -1 )
	     {  Vperror ( "flash_convert: fseek" );
		fclose ( fid_file );
		fclose ( tmp_file );
		unlink ( temp_fid );
	        ABORT;
	     }

	     /* Copy one trace of fid data */
	     if ( fread ( ptrace, 1, trace_size, fid_file ) != trace_size )
	     {  Vperror ( "flash_convert: fread (trace data)" );
		fclose ( fid_file );
		fclose ( tmp_file );
		unlink ( temp_fid );
	        ABORT;
	     }
	     break;

           case 1: /* compressed multi-image and compressed phase-encode */
	     for (ctr=0; ctr<ncomp; ctr++) {
	         /* Compute position in input file for the current single trace */
	         offset = sizeof(file_head) + (trace_ctr)*old_bbytes +
		          sizeof(block_head) +
			  (ctr*nf/ncomp + slice_ctr)*single_trace_sz;
	 
	         /* Position the input file */
	         if ( fseek ( fid_file, offset, 0 ) == -1 )
	         {  Vperror ( "flash_convert: fseek" );
		    fclose ( fid_file );
		    fclose ( tmp_file );
		    unlink ( temp_fid );
	            ABORT;
	         }

	         /* Copy a single trace of fid data */
	         if ( fread ( ptrace + ctr*single_trace_sz, 1,
		     single_trace_sz, fid_file ) != single_trace_sz )
	         {  Vperror ( "flash_convert: fread (trace data)" );
		    fclose ( fid_file );
		    fclose ( tmp_file );
		    unlink ( temp_fid );
	            ABORT;
	         }
	     }
	     break;

           case 2: /* compressed echo-train, multi-slice, and phase-encode */
	     for (ctr=0; ctr<ncomp; ctr++) {
	         /* Compute position in input file for  current single trace */
		 /* fprintf(stderr,					*/
		 /* "slice_ctr:%d echo_ctr:%d trace_ctr:%d ctr%d\n",	*/
		 /*	slice_ctr,echo_ctr,trace_ctr,ctr);		*/
	         offset = sizeof(file_head) + (trace_ctr)*old_bbytes +
		          sizeof(block_head) +
			  slice_ctr*inecho*incomp*single_trace_sz +
			  (ctr*inecho + echo_ctr)*single_trace_sz;
	 
	         /* Position the input file */
	         if ( fseek ( fid_file, offset, 0 ) == -1 )
	         {  Vperror ( "flash_convert: fseek" );
		    fclose ( fid_file );
		    fclose ( tmp_file );
		    unlink ( temp_fid );
	            ABORT;
	         }

	         /* Copy a single trace of fid data */
	         if ( fread ( ptrace + ctr*single_trace_sz, 1,
		     single_trace_sz, fid_file ) != single_trace_sz )
	         {  Vperror ( "flash_convert: fread (trace data)" );
		    fclose ( fid_file );
		    fclose ( tmp_file );
		    unlink ( temp_fid );
	            ABORT;
	         }
	     }
	     break;
	 }  /* end switch */

	 if ( fwrite ( ptrace, 1, trace_size, tmp_file ) != trace_size )
	 {  Vperror ( "flash_convert: fwrite (trace data)" );
	    fclose ( fid_file );
	    fclose ( tmp_file );
	    unlink ( temp_fid );
	    ABORT;
	 }

      }  /* End loop over input traces */

    }  /* End loop over input echo train */

   }  /* End loop over input slices or echoes */

   /* Clear display of slice or echo number */
   disp_index ( 0 );

   /* Close the files */
   if ( fclose ( fid_file ) != 0 )
   {  (void)fclose ( tmp_file );
      Vperror ( "flash_convert: fclose (fid file)" );
      ABORT;
   }
   if ( fclose ( tmp_file ) != 0 )
   {  Vperror ( "flash_convert: fclose (temporary file)" );
      ABORT;
   }

   /* Get rid of the original fid file,       */
   /* and replace it with the temporary file. */
   if ( unlink ( default_fid ) != 0 )
   {  Vperror ( "flash_convert: unlink" );
      ABORT;
   }
   if ( rename ( temp_fid, default_fid ) != 0 )
   {  Vperror ( "flash_convert: rename" );
      ABORT;
   }

   /* Set file name parameter */
   P_setstring ( PROCESSED, "file", default_fid, 0 );
   P_setstring ( CURRENT, "file", default_fid, 0 );

   /* Normal successful return */
   RETURN;

} /* End function flash_convert */

/******************************************************************************/
static int nf_flash_convert (char *default_fid, char *temp_fid,
           double arraydim, int format, double ncomp, double necho,
           double nf, double *pni )
/* 
Purpose:
-------
     Routine nf_flash_convert shall replace a fid file in flash format with
     one in the format of a standard compressed two dimensional image fid.
     This routine also sets the "file" parameter in the CURRENT tree.

Arguments:
---------
default_fid  :  (I  )  The path of the fid file to be edited.
temp_fid     :  (I  )  The path of a parallel scratch file.
arraydim     :  (I  )  (Old) value of the process parameter of the same name.
format       :  (I  )  image compression format; 
			0 = standard or compressed multi-image,
			1 = compressed multi-slice.
			2 = rare-type with pe - echo train.
ncomp        :  (I  )  Number of traces to keep in compressed nf form.
necho	     :  (I  )  Number of echos. (in echo train for format 2)
nf	     :  (I)  Ditto.
pni	     :  (I/O)  Ditto.
*/

{ /* Begin function flash_convert */
   /*
   Local Variables:
   ---------------
   fid_file    :  Stdio file indicator for the (input) fid file.
   tmp_file    :  Stdio file indicator for the (output) temporary file.
   file_head   :  A datafilehead struct for holding file header.
   block_head  :  A datablockhead struct for holding block header.
   ptrace      :  Allocated space to hold one trace of fid data.
   trace_size  :  Number of bytes per trace of fid data.
   slice_ctr   :  Loop control variable for slices in a multi-slice experiment,
		  or echoes in a multi-echo experiment.
   trace_ctr   :  Loop control variable for traces.
   old_bbytes  :  Block size (including block header) of input (flash format)
		  fid file.
   offset      :  Offset into input file, for reading next trace.
   nbytes      :  Number of bytes desired for a file read or write.
   */
   int  array_ctr;
   int  ctr,single_trace_sz,trace_size,slice_ctr,trace_ctr,echo_ctr,nbytes;
   int	inecho,incomp,inni,inarray,blockindex;
   long old_bbytes, offset;
   struct datafilehead *src_file_head,*dest_file_head;
   struct datafilehead src_file_head_data;
   struct datablockhead *src_block_head,*dest_block_head;
   MFILE_ID fid_file, tmp_file;
   uint64_t	newbytelen;
   /*
   Begin Executable Code:
   ---------------------
   */
   inecho = (int) (necho+0.5);
   incomp = (int) (ncomp+0.5);

   if (incomp < 1)
   {
      Werrprintf ( "flashc: Number of traces (%d) must be at least 1", incomp);
      ABORT;
   }    
   /* Open the two files */
   if ( ( fid_file = mOpen(default_fid, (uint64_t) 0, O_RDONLY) ) == 0 )
   {  Vperror ( "flash_convert: mOpen (fid file)" );
      ABORT;
   }
   newbytelen = fid_file->byteLen + ((uint64_t) incomp * (uint64_t)(arraydim+0.5) * 
   						(uint64_t) sizeof(struct datablockhead));
   if ( ( tmp_file = mOpen(temp_fid,newbytelen,O_RDWR | O_CREAT)) == 0)
   {  Vperror ( "flash_convert: mOpen (temporary file)" );
      mClose(fid_file);
      ABORT;
   }

   /* Set src file header */
   src_file_head = &src_file_head_data;
   memcpy(src_file_head,fid_file->mapStrtAddr,sizeof(struct datafilehead));
   DATAFILEHEADER_CONVERT_NTOH(src_file_head);
   /* Set dest file header */
   dest_file_head = (struct datafilehead *)tmp_file->mapStrtAddr;

/*    fprintf(stderr,"nf_flash_convert: oldblocks=%d oldtraces=%d\n", */
/*			src_file_head->nblocks,src_file_head->ntraces); */

   /* Check contents before modifying. 		       */
   /* Recall that a value of -1 means "not set yet" */
   if ( *pni == -1.0 )
   {  
      *pni = 1.0;
      if (( src_file_head->nblocks < 1 ) ||
	  ( src_file_head->nblocks != floor(arraydim+0.5) ))
      {  Werrprintf ( "flash_convert: Bad nblocks field in file header" );
	 mClose(fid_file);
	 mClose(tmp_file);
	 unlink ( temp_fid );
	 ABORT;
      }
   }
   else
   {
      if ( src_file_head->nblocks != floor(arraydim+0.5) )
      {  Werrprintf ( "flash_convert: nblocks %d not equal to arraydim %d",
		src_file_head->nblocks, (int)floor(arraydim+0.5));

	 mClose(fid_file);
	 mClose(tmp_file);
	 unlink ( temp_fid );
	 ABORT;
      }
   }
   inni = floor(*pni+0.5);

   if ( src_file_head->ntraces != nf )
   {  Werrprintf ( "flash_convert: bad ntraces field in file header" );
      mClose(fid_file);
      mClose(tmp_file);
      unlink ( temp_fid );
      ABORT;
   }

   /* copy file header */
   memcpy(dest_file_head,src_file_head,sizeof(struct datafilehead));

   /* fprintf(stderr,"nf_flash_convert: ncomp=%g, necho=%g nf=%g pni=%g\n", */
   /*		ncomp, necho, nf, *pni);				   */
   /* Update vers_id,status in fid header */
   fidversion(dest_file_head,FILEHEAD,-1);

   /* Save the size of the input file blocks */
   old_bbytes = src_file_head->bbytes;

   /* Edit parameters and write file header back out to disk */
   dest_file_head->nblocks = (int)(((arraydim/(*pni)) * ncomp) + 0.5);
   dest_file_head->ntraces = (int)(((nf)/ncomp) * (*pni));
   single_trace_sz = src_file_head->tbytes;
   trace_size = (int)((nf/ncomp)+0.5) * src_file_head->tbytes;
   dest_file_head->bbytes  = (dest_file_head->ntraces*single_trace_sz)
						 + sizeof(struct datablockhead);

   /* Do some consistency checks */
   if ((format == 1) && ((int)(nf/ncomp) <= 1))
   {
	/* not a true multislice */
      Werrprintf("flash_convert_warning: Single slice, conversion not done.");
      mClose(fid_file);
      mClose(tmp_file);
      unlink ( temp_fid );
      ABORT;
   }
   if (format == 2)
   {
      int ok = 1;

      if (inecho < 1)
      {
         ok = 0;
         Werrprintf("flashc: For 'rare', number of echos (%d) must be at least 1.",
            inecho);
      }
      if (ok)
      {
         int tmp, intNf;

         intNf = (int) nf;
         tmp = (int)((nf/(ncomp*inecho))+0.5);
         ok = ((tmp * inecho * incomp) == intNf);
         if (!ok)
            Werrprintf("flashc: For 'rare', slices (%d) * echos (%d) must be a factor of nf (%d).",
               incomp, inecho, intNf);
      }
      if (!ok)
      {
         mClose(fid_file);
         mClose(tmp_file);
         unlink ( temp_fid );
         ABORT;
      }
   }

   /* advance dest data pointer, write file header */
   tmp_file->offsetAddr += sizeof(struct datafilehead);
   tmp_file->newByteLen = sizeof(struct datafilehead);
   
   DATAFILEHEADER_CONVERT_HTON(dest_file_head);

/*   fprintf(stderr,"nf_flash_convert: blocks=%d traces=%d bbytes=%d\n", */
/* 			file_head.nblocks,file_head.ntraces,old_bbytes); */

   /* Advance dest data pointer, Obtain block header */
   fid_file->offsetAddr += sizeof(struct datafilehead);
   src_block_head = (struct datablockhead *)fid_file->offsetAddr;
   nbytes = sizeof ( src_block_head );

   blockindex = 0;

   inarray = floor((arraydim/(*pni))+0.5);
   /* Loop over arrayed data */
   for (array_ctr=0; array_ctr< inarray; array_ctr++)
   {

   /* Loop over input slices or echoes */
   for ( slice_ctr = 0; slice_ctr < floor(ncomp+0.5); slice_ctr++ )
   {
     /* write a block header */
     blockindex++;
     /* copy block and update index */
     dest_block_head = (struct datablockhead *)tmp_file->offsetAddr;
     memcpy(tmp_file->offsetAddr,src_block_head,
					sizeof(struct datablockhead));

     /* Update vers_id,status in fid header */
     /* version = dest_file_head->vers_id & 0x3; */	/* VERSION */
     /* fidversion(dest_block_head,BLOCKHEAD,version); */

     tmp_file->offsetAddr += sizeof(struct datablockhead);
     tmp_file->newByteLen += sizeof(struct datablockhead);
     dest_block_head->index = htons(blockindex);
     dest_block_head->status = dest_file_head->status;

     echo_ctr = 0;
     /*for ( echo_ctr = 0 ; echo_ctr < floor(necho+0.5) ; echo_ctr++ ) */
     /*{ */
      /* Display slice or echo number, occasionally */
      if ( (slice_ctr*(inecho) + echo_ctr) % 16 == 0 )
	 disp_index ( slice_ctr );

      /* Loop over input phase encode elements */
      for ( trace_ctr = 0 ; trace_ctr < *pni ; trace_ctr++ )
      {
         switch (format) {
	   case 0:  /* standard phase-encode and compressed multi-image */
	     /* Compute position in input (flash) file for the current trace */
	     offset = sizeof(struct datafilehead) + 
		      (array_ctr)*old_bbytes + 
		      (trace_ctr*inarray)*old_bbytes +
		      sizeof(struct datablockhead) + (slice_ctr)*trace_size;

	     /* Position the input file */
	     fid_file->offsetAddr = fid_file->mapStrtAddr + offset;

	     /* Copy one trace of fid data */
	     memcpy(tmp_file->offsetAddr,fid_file->offsetAddr,trace_size);
	     tmp_file->offsetAddr += trace_size;
	     tmp_file->newByteLen += trace_size;
	     break;

           case 1: /* compressed multi-slice */
	     for (ctr=0; ctr<(int)((nf/ncomp)+0.5); ctr++) { 
	     /* Compute position in input file for the current single trace */
	         offset = sizeof(struct datafilehead) + 
			(array_ctr)*old_bbytes + 
			(trace_ctr*inarray)*old_bbytes + 
			sizeof(struct datablockhead) +
			(ctr*ncomp + slice_ctr)*(single_trace_sz);
	 
	    	 /* Position the input file */
	     	 fid_file->offsetAddr = fid_file->mapStrtAddr + offset;

	    	 /* Copy one trace of fid data */
		 /* trace_size = single_trace_sz? */
	   	 memcpy(tmp_file->offsetAddr,fid_file->offsetAddr,
							single_trace_sz);
	    	 tmp_file->offsetAddr += single_trace_sz;
	    	 tmp_file->newByteLen += single_trace_sz;
	     }
	     break;

           case 2: /* compressed echo-train, multi-slice, and phase-encode */
	     for (ctr=0; ctr<(int)((nf/(ncomp*inecho))+0.5); ctr++) {
	        /* Compute position in input file for  current single trace */
		/* fprintf(stderr,					    */
		/* "slice_ctr:%d trace_ctr:%d ctr:%d necho:%d ncomp:%d\n",  */
		/* 	slice_ctr,echo_ctr,trace_ctr,ctr,inecho,incomp);    */
	        offset = sizeof(struct datafilehead) +  
			  (array_ctr)*old_bbytes + 
			  (trace_ctr*inarray)*old_bbytes + 
			  sizeof(struct datablockhead) +
			  ctr*inecho*incomp*single_trace_sz +
			  (slice_ctr*inecho)*single_trace_sz;
	 
	        /* Position the input file */
		fid_file->offsetAddr = fid_file->mapStrtAddr + offset;

	        /* Copy a single trace of fid data */
		memcpy(tmp_file->offsetAddr,fid_file->offsetAddr,
						single_trace_sz*inecho);
		tmp_file->offsetAddr += single_trace_sz*inecho;
		tmp_file->newByteLen += single_trace_sz*inecho;
	     }
	     break;
	 }  /* end switch */

      }  /* End loop over input traces */

    /* } */  /* End loop over input echo train */

   }  /* End loop over input slices or echoes */

   }  /* End array loop */

   /* Clear display of slice or echo number */
   disp_index ( 0 );

   /* Close the files */
   mClose(fid_file);
   mClose(tmp_file);

   /* Get rid of the original fid file,       */
   /* and replace it with the temporary file. */
   if ( unlink ( default_fid ) != 0 )
   {  Vperror ( "flash_convert: unlink" );
      ABORT;
   }
   if ( rename ( temp_fid, default_fid ) != 0 )
   {  Vperror ( "flash_convert: rename" );
      ABORT;
   }

   /* Set file name parameter */
   P_setstring ( PROCESSED, "file", default_fid, 0 );
   P_setstring ( CURRENT, "file", default_fid, 0 );

   /* Normal successful return */
   RETURN;

} /* End function flash_convert */

/******************************************************************************/
void Vperror (char *usr_string )

/* 
Purpose:
-------
     Routine Vperror is similar to system routine perror except that output is
made through Vnmr routine Werrprintf.

Arguments:
---------
usr_string  :  (I  )  String to be used in conjunction with system error 
		      message.
*/
{ /* Begin function Vperror */
   /*
   Local Variables:
   ---------------
   ptr      :  Text space allocated for editing error message.
   sys_err  :  One of the canned system error messages, or an alternate message
	       expressing failure.
   failbuf  :  Alternate message expressing failure.
   */
   char *ptr, *sys_err, failbuf[40];
   /*
   Begin Executable Code:
   ---------------------
   */
   /* If errno is within range */
   if ( (sys_err = strerror(errno)) == NULL)
   {  (void)sprintf ( failbuf, "Unknown error number %d", errno );
      sys_err = failbuf;
   }
   /* Allocate space and compose message */
   if ( ( ptr = malloc ( (unsigned)(strlen(usr_string)+strlen(sys_err)+3) ) ) 
      == 0 )
   {  perror ( "Vperror: malloc" );
      exit ( 1 );
   }
   (void)strcpy ( ptr, usr_string );
   (void)strcat ( ptr, ": " );
   (void)strcat ( ptr, sys_err );

   /* Issue the finished message */
   Werrprintf ( ptr );

   /* Deallocate space */
   free ( ptr );

} /* End function Vperror */

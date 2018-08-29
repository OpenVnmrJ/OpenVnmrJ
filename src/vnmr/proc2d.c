/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************************************/
/*							*/
/*  proc2d.c	-	common 2D functions		*/
/*							*/
/********************************************************/

#include "group.h"
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include "data.h"
#include "allocate.h"
#include "disp.h"
#include "displayops.h"
#include "init2d.h"
#include "init_display.h"
#include "init_proc.h"
#include "vnmrsys.h"
#include "process.h"
#include "pvars.h"
#include "wjunk.h"

#define ERROR		-1
#define COMPLETE	0
#define MINDEGREE	0.005
#define FALSE	0
#define TRUE	1
#define SUM	0
#define SQRT	1

#define MODE_BIT(mode, dim)	((mode) << (dim))

#define C_GETPAR(name, value)				\
	if (r = P_getreal(CURRENT, name, value, 1))	\
	{ P_err(r, name, ":"); return ERROR; }

#define getdatatype(status)				\
  	( (status & S_HYPERCOMPLEX) ? HYPERCOMPLEX :	\
    	( (status & S_COMPLEX) ? COMPLEX : REAL ) )

#define nonzerophase(arg1, arg2)			\
	( (fabs(arg1) > MINDEGREE) ||			\
	  (fabs(arg2) > MINDEGREE) )

#define nochange_in_phase(arg1, arg2, arg3, arg4)	\
	( (fabs(arg1 - arg2) < MINDEGREE) &&		\
	  (fabs(arg3 - arg4) < MINDEGREE) )

extern int interuption;
extern void maxfloat(register float  *datapntr, register int npnt, register float  *max);
extern void leveltilt(float *ptr, int offset, double oldlvl, double oldtlt);
extern void long_event();
extern void dodc(float *ptr, int offset, double oldlvl, double oldtlt);
extern int checkS_BCbit(int fileindex);
extern int bph();
extern float getBph0(int trace);
extern float getBph1(int trace);

/*-----------------------------------------------
|						|
|		checkblock()/3			|
|						|
|   This function checks the current phase	|
|   file block to see it the data in that	|
|   block must be re-calculated.		|
|						|
+----------------------------------------------*/
static int checkblock(int trace, int *ok, int file_id)
{
  int		block,
		block1,
		r,
		found,
		phasefile_mode,
                quad2=0,
                quad4=0;
  int mask;


  *ok = TRUE;		/* initialize the OK flag */

/*******************************************
*  If the requested trace does not lie in  *
*  the currently active phasefile block,   *
*  get a new phasefile block.              *
*******************************************/

  if ((trace > c_last) || (trace < c_first))
  {
     block1 = block = trace/specperblock;
     if ((d2flag) && (!revflag))
        block1 += nblocks;

     if (block != c_buffer)
     { /* must get access to proper data block */
        if (c_buffer >= 0)	/* release last used block */
        {
	   if ( (r = D_release(file_id, c_buffer)) )
           {
              D_error(r);
              D_close(file_id);
              return(ERROR);
           }
        }

	if ( (r = D_getbuf(file_id, nblocks, block1, &c_block)) )
        {
           if ( (r = D_allocbuf(file_id, block1, &c_block)) )
	   {
              D_error(r);
              return(ERROR);
           }

           c_block.head->status = 0;
           c_block.head->mode = 0;
        }

        c_buffer = block1;
        c_first = block * specperblock;
        c_last = c_first + specperblock - 1;
     }
  }

/*	COMMENTED OUT
  else
  {
*/

/*******************************************
*  If the requested trace does lie in the  *
*  currently active phasefile block, it    *
*  should be okay.  NOTE:  This has been   *
*  made to be so for 1D data which has     *
*  been interactively phased with the      *
*  mouse.  It does not work yet, however!  *
*******************************************/

/*	COMMENTED OUT
     return(COMPLETE);
  }
*/


/*************************************************
*  Check to see if any data are already present  *
*  in the currently active phasefile block.      *
*************************************************/

  if ((~c_block.head->status) & S_DATA)
  {
     *ok = FALSE;
  } else if(checkS_BCbit(D_PHASFILE)) {
     *ok = TRUE;
  }


/******************
*  Generic tests  *
******************/

  else
  {

/*******************************************
*  Determine "display mode" bit field for  *
*  the requested display.                  *
*******************************************/

     phasefile_mode = get_mode(HORIZ); 
     mask = 0xfff;
     if (d2flag)
     {
        quad2 = FALSE;
        quad4 = (datahead.status & S_HYPERCOMPLEX);
        if (!quad4)
           quad2 = (datahead.status & S_COMPLEX);

        phasefile_mode |= get_mode(VERT);
     }

/**********************************************
*  Compare the bit field for the requested    *
*  "display mode" with that currently stored  *
*  in the phasefile block header.             *
**********************************************/
     if ( (c_block.head->mode&mask) != phasefile_mode ) 
     {
        *ok = FALSE;
     }
     else if ( d2flag && (quad4 || quad2) )
     {
        int	  direction;
        double rp_val,lp_val;

/**************************************************
*  If the 2D data are to be phased along either   *
*  F1 or F2, switch certain parameters depending  *
*  on "revflag."                                  *
**************************************************/

        direction = get_direction(REVDIR);
        if ( (get_phase_mode(direction) || get_phaseangle_mode(direction)) && get_axis_freq(direction) )
        {
           get_phase_pars(direction,&rp_val,&lp_val);
           *ok = nochange_in_phase(c_block.head->rpval, (float)rp_val,
			c_block.head->lpval, (float)lp_val );
        }

        direction = get_direction(NORMDIR);
        if ( quad4 && (*ok) && (get_phase_mode(direction) || get_phaseangle_mode(direction)) &&
	      get_axis_freq(direction) )
        {
           hycmplxhead	*tmpblkhead;

           tmpblkhead = (hycmplxhead *) (c_block.head);
           found = FALSE;
           while (!found)
           {
              if ((~tmpblkhead->status) & MORE_BLOCKS) 
                 return(COMPLETE); 

              tmpblkhead += 1;
              found = (tmpblkhead->status & U_HYPERCOMPLEX);
           }

           get_phase_pars(direction,&rp_val,&lp_val);
           *ok = nochange_in_phase(tmpblkhead->rpval1, (float)rp_val,
			tmpblkhead->lpval1, (float)lp_val);
        }
     }
     else if (!d2flag && (get_phase_mode(HORIZ) || get_phaseangle_mode(HORIZ)))
     {
	if(bph()>0) {
	  *ok = FALSE;
	} else {
          *ok = nochange_in_phase(c_block.head->rpval, (float)rp,
			c_block.head->lpval, (float)lp);
        }
     }
  }

  return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		 checkdata()/1			|
|						|
+----------------------------------------------*/
static int checkdata(dblockhead *header)
{
  int	d_mode;

/*********************************************
*  Insure that there are data in the block.  *
*********************************************/

  if ((~header->status) & S_DATA)
  {
     Werrprintf("No data in file");
     return(ERROR);
  }

/********************************************************
*  If the current phasefile data are not displayed in   *
*  the correct mode along F1, the phasefile data must   *
*  be recalculated.  If the data are not complex along  *
*  f1, this is not possible without completely repro-   *
*  cessing the data.                                    *
********************************************************/

  if (d2flag && (datahead.status & (S_HYPERCOMPLEX|S_COMPLEX)) &&
        !((~datahead.status) & S_SPEC))   /* if not fid data */
  {
     d_mode = get_mode(VERT);
     if ( (d_mode & c_block.head->mode) != d_mode )
     {
        if (!( header->status & (NF_CMPLX|NI_CMPLX|NI2_CMPLX) ))
        {
           Werrprintf("Cannot display data without reprocessing");
           return(ERROR);
        }
     }
/********************************************************
*  If the current phasefile data are not displayed in   *
*  the correct mode along F2, the phasefile data must   *
*  be recalculated.  If the data are not complex along  *
*  f1, this is not possible without completely repro-   *
*  cessing the data.                                    *
********************************************************/
     d_mode = get_mode(HORIZ);
     if ( (d_mode & c_block.head->mode) != d_mode )
     {
        int h_complex;
        int v_complex;

        if (( datahead.status & (S_NI2) ))
        {
           v_complex = ( header->status & NI2_CMPLX );
           h_complex = ( header->status & (NP_CMPLX|NF_CMPLX|NI_CMPLX) );
        }
        else
        {
           v_complex = ( header->status & (NF_CMPLX|NI_CMPLX|NI2_CMPLX) );
           h_complex = ( header->status & NP_CMPLX );
        }
        if (!h_complex && !v_complex)
        {
           Werrprintf("Cannot display data without reprocessing");
           return(ERROR);
        }
     }
  }
  return(COMPLETE);
}


/*-----------------------------------------------
|						|
|	 	  getdata()/0			|
|						|
|   This function reads the 1D or 2D data in 	|
|   form disk, manipulates it appropriately,	|
|   and moves the ReRe component to the memory	|
|   allocated for the phasefile data.		|
|						|
+----------------------------------------------*/
static int getdata()
{
   char		dsplymes[20],	/* display mode label		*/
		dmg1[5];
   int		npf1=0,		/* number of points along F1	*/
		npf2=0,		/* number of points along F2	*/
		i,
		r,
		found,
		f1phase,
		f2phase,
		quad2,
		quad4,
		complex_1D=0,
                norm_av,
                norm_dir,
                norm_phase,
                norm_phaseangle,
                norm_dbm,
                rev_av=0,
                rev_dir=0,
                rev_phase=0,
                rev_phaseangle=0,
		nplinear,
		npblock;
   float	*f1phasevec=NULL,	/* F1 phasing vector		*/
		*f2phasevec=NULL;	/* F2 phasing vector		*/
   double	rp_norm,
		lp_norm;
   double	rp_rev,
		lp_rev;
   dpointers	datablock;
   hycmplxhead	*tmpblkhead;


/********************************************
*  Get pointer to data in specified block.  *
********************************************/

   if ( (r = D_getbuf(D_DATAFILE, nblocks, c_buffer, &datablock)) )
   {
      D_error(r);
      return(ERROR);
   }
   else if (checkdata(datablock.head))
   {
      return(ERROR);
   }

/**********************
*  Setup for 2D data  *
**********************/

   quad2 = quad4 = FALSE;
   f1phase = f2phase = FALSE;

   if (d2flag)
   {
/************************************
*  Set the flags for the number of  *
*  quadrants.                       *
************************************/

     quad4 = (datahead.status & S_HYPERCOMPLEX);
     if (!quad4)
        quad2 = (datahead.status & S_COMPLEX);

/********************************************
*  Precalculate phasing vectors for the F1  *
*  and F2 dimensions if necessary.          *
********************************************/

      nplinear = pointsperspec;
      npblock = specperblock * nblocks;
      if (quad4)
      {
         npblock *= 2;
         nplinear /= 2;
      }

      if (revflag)
      {
         npf1 = nplinear;
         npf2 = npblock;
      }
      else
      {
         npf2 = nplinear;
         npf1 = npblock;
      }

      f1phasevec = NULL;	/* initialize pointer */
      f2phasevec = NULL;	/* initialize pointer */

/**********************************************
*  If a phase-sensitive display is requested  *
*  along F1, precalculate the F1 phasing      *
*  vector.                                    *
**********************************************/

      rev_dir = get_direction(REVDIR);
      get_phase_pars(rev_dir,&rp_rev,&lp_rev);
      rev_phase = get_phase_mode(rev_dir);
      rev_phaseangle = get_phaseangle_mode(rev_dir);
      rev_av = get_av_mode(rev_dir);
      f1phase = ( ((rev_phase && nonzerophase(rp_rev, lp_rev)) || rev_phaseangle) &&
                  (quad2 || quad4) &&
		  get_axis_freq(rev_dir) );

      if (f1phase)
      {
         f1phasevec = (float *) (allocateWithId( (unsigned) (sizeof(float)
	 			    * npf1), "getdata" ));
         if (f1phasevec == NULL)
         {
            Werrprintf("Unable to allocate memory for F1 phase buffer");
            return(ERROR);
         }
         else
         {
            phasefunc(f1phasevec, npf1/2, lp_rev, rp_rev);
         }
      }

/**********************************************
*  If a phase-sensitive display is requested  *
*  along F2, precalculate the F2 phasing      *
*  vector.                                    *
**********************************************/

      norm_dir = get_direction(NORMDIR);
      get_phase_pars(norm_dir,&rp_norm,&lp_norm);
      norm_phase = get_phase_mode(norm_dir);
      norm_phaseangle = get_phaseangle_mode(norm_dir);
      norm_av = get_av_mode(norm_dir);
      norm_dbm = get_dbm_mode(norm_dir); /* may not be active?? */
      f2phase = ( (norm_phase || norm_phaseangle) && quad4 && nonzerophase(rp_norm, lp_norm) );
		/* (quad4 || quad2)??  (quad4) only?? */

      if (f2phase)
      {
         f2phasevec = (float *) (allocateWithId( (unsigned) (sizeof(float)
	 			    * npf2), "getdata" ));
         if (f2phasevec == NULL)
         {
            Werrprintf("Unable to allocate memory for F2 phase buffer");
            if (f1phase)
               releaseAllWithId("getdata");
            return(ERROR);
         }
         else
         {
            phasefunc(f2phasevec, npf2/2, lp_norm, rp_norm);
         }
      }
   }
   else
   {
      complex_1D = (datablock.head->status & NP_CMPLX);
      norm_dir = HORIZ;
      norm_phase = get_phase_mode(norm_dir);
      norm_phaseangle = get_phaseangle_mode(norm_dir);
      norm_av = get_av_mode(norm_dir);
      norm_dbm = get_dbm_mode(norm_dir); /* may not be active 2 */
   }

/*************************************
*  Readjust "npf1" and "npf2" to be  *
*  per block.                        *
*************************************/

   if (d2flag)
   {
      if (revflag)
      {
         npf2 /= nblocks;
      }
      else
      {
         npf1 /= nblocks;
      }
   }

/**********************************
*  Construct display mode label.  *
**********************************/

   strcpy(dsplymes, ""); 	/* initialization of string */

   if ( d2flag && (quad2 || quad4) )
   {
      char charval;
      char tmp[10];

      get_display_label(rev_dir,&charval);
      if (rev_phase)
      {
         sprintf(tmp, "PH%c ",charval);
      }
      else if (rev_av)
      {
         sprintf(tmp, "AV%c ",charval);
      }
      else if (rev_phaseangle)
      {
         sprintf(tmp, "PA%c ",charval);
      }
      else
      {
         sprintf(tmp, "PW%c ",charval);
      }

      strcpy(dsplymes, tmp);
   }

   if ( (d2flag && quad4) || ((!d2flag) && complex_1D) )
   {
      char charval;
      char tmp[10];

      get_display_label(norm_dir,&charval);
      if (charval == '\0')
         charval = ' ';
      if (norm_phase)
      {
         sprintf(tmp, "PH%c",charval);
      }
      else if (norm_av)
      {
         sprintf(tmp, "AV%c",charval);
      }
      else if (norm_phaseangle)
      {
         sprintf(tmp, "PA%c",charval);
      }
      else
      {
         sprintf(tmp, "PW%c",charval);
      }

      strcat(dsplymes, tmp);
   }

   disp_status(dsplymes);

/*************************************************
*  Manipulate data depending upon the number of  *
*  2D data quadrants and the desired mode of     *
*  display.					 *
*************************************************/

   if (d2flag)
   {
      if (quad4)
      {

/***********************************
*  HYPERCOMPLEX 2D spectral data:  *
*  F2 phase-sensitive display      *
***********************************/

         if (norm_phase)
         {
            if (rev_phase)
            {

	/*****************************
	*  phase for ReRe component  *
	*****************************/
               if (f1phase && f2phase)
               {
                  blockphase4(datablock.data, c_block.data, f1phasevec,
	  		f2phasevec, nblocks, c_buffer, npf1/2, npf2/2);
               }
               else if (f1phase)
               {
                  blockphase2(datablock.data, c_block.data, f1phasevec,
			nblocks, c_buffer, npf1/2, npf2/2, HYPERCOMPLEX,
			COMPLEX, TRUE, FALSE);
               }
               else if (f2phase)
               {
                  blockphase2(datablock.data, c_block.data, f2phasevec,
			nblocks, c_buffer, npf1/2, npf2/2, HYPERCOMPLEX,
			REAL, FALSE, FALSE);
               }
               else
               {
                  movmem((char *)datablock.data, (char *)c_block.data,
			(npf1*npf2*sizeof(float))/4, HYPERCOMPLEX,
			4);
               }
            }
            else
            {
               if (f2phase)
               {

	/***************************
	*  rotate ReRe <---> ReIm  *
	*  rotate ImRe <---> ImIm  *
	***************************/
                  if (rev_av)
                  {

	/********************************
	*  S = sqrt(ReRe**2 + ImRe**2)  *
	********************************/
                     blockphsabs4(datablock.data, c_block.data, f2phasevec,
			   nblocks, c_buffer, npf1/2, npf2/2, REAL, FALSE);
                  }
                  else if (rev_phaseangle)
                  {
                     blockphaseangle2(datablock.data, c_block.data, f1phasevec,
                           nblocks, c_buffer, npf1/2, npf2/2, HYPERCOMPLEX,
                           COMPLEX, TRUE, FALSE);
                  }
                  else
                  {

	/**************************
	*  S = ReRe**2 + ImRe**2  *
	**************************/
                     blockphspwr4(datablock.data, c_block.data, f2phasevec,
			   nblocks, c_buffer, npf1/2, npf2/2, REAL, FALSE);
                  }
               }
               else if (rev_av)	/* no F2 phasing required */
               {

	/********************************
	*  S = sqrt(ReRe**2 + ImRe**2)  *
	********************************/
                  absval2(datablock.data, c_block.data, (npf1*npf2)/4,
                        HYPERCOMPLEX, COMPLEX, REAL, FALSE);
               }
/* else if (rev_phaseangle) {} */
               else			/* no F2 phasing required */
               {

	/**************************
	*  S = ReRe**2 + ImRe**2  *
	**************************/
                  pwrval2(datablock.data, c_block.data, (npf1*npf2)/4,
                        HYPERCOMPLEX, COMPLEX, REAL, FALSE);
               }
            }
         }

/***********************************
*  HYPERCOMPLEX 2D spectral data:  *
*  F2 absolute-value display       *
***********************************/

         else if (norm_av)
         {
            if (rev_phase)
            {
               if (f1phase)
               {

	/********************************
	*  rotate ReRe <---> ImRe       *
	*  rotate ReIm <---> ImIm       *
	*  S = sqrt(ReRe**2 + ReIm**2)  *
	********************************/
                  blockphsabs4(datablock.data, c_block.data, f1phasevec,
			   nblocks, c_buffer, npf1/2, npf2/2, COMPLEX, TRUE);
               }
               else
               {

	/********************************
	*  S = sqrt(ReRe**2 + ReIm**2)  *
	********************************/
                  absval2(datablock.data, c_block.data, (npf1*npf2)/4,
			HYPERCOMPLEX, REAL, REAL, FALSE);
               }
            }
            else if (rev_av)
            {

	/****************************************************
	*  S = sqrt(ReRe**2 + ReIm**2 + ImRe**2 + ImIm**2)  *
	****************************************************/
               absval4(datablock.data, c_block.data, npf1*npf2/4);
            }
/* else if (rev_phaseangle) {} */
            else
            {

	/****************************
	*  S1 = ReRe**2 + ImRe**2   *
	*  S2 = ReIm**2 + ImIm**2   *
        *  S = sqrt(S1**2 + S2**2)  *
	****************************/
               blockpwrabs(datablock.data, c_block.data, (npf1*npf2)/4,
				COMPLEX);
            }
         }

/***********************************
*  HYPERCOMPLEX 2D spectral data:  *
*  F2 phaseangle display           *
***********************************/

         else if (norm_phaseangle)
         {
            Werrprintf("Cannot perform hypercomplex phaseangle");
            return(ERROR);
         }

/***********************************
*  HYPERCOMPLEX 2D spectral data:  *
*  F2 power display                *
***********************************/

         else
         {
            if (rev_phase)
            {
               if (f1phase)
               {

	/***************************
	*  rotate ReRe <---> ImRe  *
	*  rotate ReIm <---> ImIm  *
	*  S = ReRe**2 + ReIm**2   *
	***************************/
                  blockphspwr4(datablock.data, c_block.data, f1phasevec,
			   nblocks, c_buffer, npf1/2, npf2/2, COMPLEX, TRUE);
               }
               else
               {

	/**************************
	*  S = ReRe**2 + ReIm**2  *
	**************************/
                  pwrval2(datablock.data, c_block.data, (npf1*npf2)/4,
			   HYPERCOMPLEX, REAL, REAL, FALSE);
               }
            }
            else if (rev_av)
            {

	/****************************
	*  S1 = ReRe**2 + ReIm**2   *
	*  S2 = ImRe**2 + ImIm**2   *
        *  S = sqrt(S1**2 + S2**2)  *
	****************************/
               blockpwrabs(datablock.data, c_block.data, (npf1*npf2)/4,
				REAL);
            }
/* else if (rev_phaseangle) {} */
            else
            {

	/**********************************************
	*  S = ReRe**2 + ReIm**2 + ImRe**2 + ImIm**2  *
	*  THIS IS NOT QUITE RIGHT!                   *
	**********************************************/
               pwrval4(datablock.data, c_block.data, npf1*npf2/4);
            }
         }

/**********************************************
*  Set the F1 phase constants into the block  *
*  header of the phasefile data.              *
**********************************************/

         if (rev_phase || rev_phaseangle)
         {
            c_block.head->lpval = lp_rev;
            c_block.head->rpval = rp_rev;
         }
         else
         {
            c_block.head->lpval = 0.0;
            c_block.head->rpval = 0.0;
         }

/***************************************************
*  Locate the block header for the "hypercomplex"  *
*  information.  Then set the F2 phase constants   *
*  into the block header of the phasefile data.    *
***************************************************/

         i = 0;
         found = FALSE;

         while (!found)
         {
            if ( (~(datablock.head + i)->status) & MORE_BLOCKS )
            {
               Werrprintf("Block headers inconsistent with hypercomplex data");
               if (f1phase || f2phase)
                  releaseAllWithId("getdata");
               return(ERROR);
            }

            i += 1;
            found = ( (datablock.head + i)->status & U_HYPERCOMPLEX );
         }

         tmpblkhead = (hycmplxhead *) (c_block.head + i);
         if (norm_phase)
         {
            tmpblkhead->lpval1 = lp_norm;
            tmpblkhead->rpval1 = rp_norm;
         }
         else
         {
            tmpblkhead->lpval1 = 0.0;
            tmpblkhead->rpval1 = 0.0;
         }
      }
      else if (quad2)
      {

/*******************************
*  COMPLEX 2D spectral data:   *
*  F1 display selection        *
*******************************/

         if (rev_phase)
         {

       /***************************
       *  rotate ReRe <---> ImRe  *
       ***************************/
            if (f1phase)
            {
               blockphase2(datablock.data, c_block.data, f1phasevec,
			nblocks, c_buffer, npf1/2, npf2, COMPLEX, 1,
			TRUE, FALSE);
            }
            else
            {
               movmem((char *)datablock.data, (char *)c_block.data,
			(npf1*npf2*sizeof(float))/2, COMPLEX, 4);
            }
         }
         else if (rev_phaseangle)
         {
            if (f1phase)
            {
               blockphaseangle2(datablock.data, c_block.data, f1phasevec,
			nblocks, c_buffer, npf1/2, npf2, COMPLEX, 1,
			TRUE, FALSE);
            }
            else
            {
               movmem((char *)datablock.data, (char *)c_block.data,
                        (npf1*npf2*sizeof(float))/2, COMPLEX, 4);
            }
         }
         else if (rev_av)
         {

       /********************************
       *  S = sqrt(ReRe**2 + ImRe**2)  *
       ********************************/
            absval2(datablock.data, c_block.data, (npf1*npf2)/2,
			COMPLEX, 1, 1, FALSE);
         }
         else
         {

       /**************************
       *  S = ReRe**2 + ImRe**2  *
       **************************/
            pwrval2(datablock.data, c_block.data, (npf1*npf2)/2,
			COMPLEX, 1, 1, FALSE);
         }

         if (rev_phase || rev_phaseangle)
         {
            c_block.head->lpval = lp_rev;
            c_block.head->rpval = rp_rev;
         }
         else
         {
            c_block.head->lpval = 0.0;
            c_block.head->rpval = 0.0;
         }
      }
      else
      {

/***************************
*  REAL 2D spectral data   *
***************************/

         movmem((char *)datablock.data, (char *)c_block.data,
			npf1*npf2*sizeof(float), REAL, 4);
         c_block.head->lpval = 0.0;
         c_block.head->rpval = 0.0;
      }
   }

/**********************
*  Setup for 1D data  *
**********************/

   else
   {
    if(bph()>0 && complex_1D && (norm_phase || norm_phaseangle)) {
      // don't use lp, rp, because block is individually phased. 
      c_block.head->lpval=getBph1(c_buffer); 
      c_block.head->rpval=getBph0(c_buffer);
      if (norm_phase)
      {
        phase2(datablock.data, c_block.data, fn/2, c_block.head->lpval, c_block.head->rpval);
      }
      else if (norm_phaseangle)
      {
        phaseangle2(datablock.data, c_block.data, fn/2, COMPLEX,
			1, 1, FALSE, c_block.head->lpval, c_block.head->rpval);
      }
      P_setreal(CURRENT,"bph1",c_block.head->lpval,0);
      P_setreal(CURRENT,"bph0",c_block.head->rpval,0);
    } else { 
      c_block.head->lpval = 0.0;
      c_block.head->rpval = 0.0;

      if (complex_1D)
      {
         if (norm_phase)
         {
            phase2(datablock.data, c_block.data, fn/2, lp, rp);
            c_block.head->lpval = lp;
            c_block.head->rpval = rp;
         }
         else if (norm_phaseangle)
         {
            phaseangle2(datablock.data, c_block.data, fn/2, COMPLEX,
			1, 1, FALSE, lp, rp);
            c_block.head->lpval = lp;
            c_block.head->rpval = rp;
         }
         else if (norm_av)
         {
            absval2(datablock.data, c_block.data, fn/2, COMPLEX,
			1, 1, FALSE);
         }
         else if (norm_dbm)
         {
            dbmval2(datablock.data, c_block.data, fn/2, COMPLEX,
			1, 1, FALSE);
         } 
         else
         {
            pwrval2(datablock.data, c_block.data, fn/2, COMPLEX,
			1, 1, FALSE);
         }
      }
      else
      {
         if (datablock.head->status & S_COMPLEX)
         {
            movmem((char *)datablock.data, (char *)c_block.data,
			(fn/2)*sizeof(float), COMPLEX, 4);
         }
         else
         {
            movmem((char *)datablock.data, (char *)c_block.data,
			(fn/2)*sizeof(float), REAL, 4);
         }
      }
    }
   }

/*************************************
*  Set status word for block header  *
*  of phasefile.                     *
*************************************/

   c_block.head->status = (S_DATA|S_SPEC|S_FLOAT);

/*********************************** 
*  Set mode word for block header  * 
*  of phasefile.                   * 
***********************************/

   c_block.head->mode = get_mode(HORIZ);
   if (d2flag)
      c_block.head->mode |= get_mode(VERT);

/***************************************
*  Set additional words in main block  *
*  header of phasefile.                *
***************************************/

   c_block.head->scale = 0;
   c_block.head->ctcount = 0;
   c_block.head->index = (short)c_buffer;
   c_block.head->lvl = 0.0;
   c_block.head->tlt = 0.0;

/************************************************
*  Set status word in previous block header of  *
*  phasefile to indicate the presence of a      *
*  following block header.                      *
************************************************/

   if (d2flag)
   {
      i = 0;
      while ((datablock.head + i)->status & MORE_BLOCKS)
      {
         (c_block.head + i)->status |= MORE_BLOCKS;
         i += 1;
         if ((datablock.head + i)->status & U_HYPERCOMPLEX)
            (c_block.head + i)->status |= U_HYPERCOMPLEX;
      }
   }


/******************************************
*  Release this DATAFILE buffer with the  *
*  data handler routines.                 *
******************************************/

   if ( (r = D_release(D_DATAFILE, c_buffer)) )
   {
      D_error(r);
      if (f1phase || f2phase)
         releaseAllWithId("getdata");
      return(ERROR);
   }

   if (P_copyvar(CURRENT, PROCESSED, "dmg", "dmg"))
   {
      Werrprintf("dmg:  cannot copy from 'current' to 'processed' tree\n");
      if (f1phase || f2phase)
         releaseAllWithId("getdata");
      return(ERROR);
   }

   if (d2flag)
   {
      if (!P_getstring(CURRENT, "dmg1", dmg1, 1, 5))
      {
         if (P_copyvar(CURRENT, PROCESSED, "dmg1", "dmg1"))
         {
            Werrprintf("dmg1: cannot copy from 'current' to 'processed' tree");
            if (f1phase || f2phase) 
               releaseAllWithId("getdata");
            return(ERROR);
         }
      }
   }

   if (f1phase || f2phase)
      releaseAllWithId("getdata");
   if (!Bnmr)
      disp_status("                 ");
   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     gettrace()/2		|
|					|
|   This function gives access to a	|
|   single trace within a 2D data	|
|   set.				|
|					|
+--------------------------------------*/
float *gettrace(int trace, int fpoint)
{
   int	res,
	ok;


   if (interuption)
      return(NULL);
   long_event();

/***********************************************
*  Check to see if the phasefile data need to  *
*  be recalculated from the datafile data.     *
***********************************************/

   if (checkblock(trace, &ok, D_PHASFILE))
      return(NULL);

/**************************************************
*  Get the datafile data and process for display  *
*  according the "dmg" and "dmg1" (if relevant)   *
*  display parameters.                            *
**************************************************/

   if (!ok)
   { /* must calculate from complex or hypercomplex data, includes 1D or 2D */
      if (getdata())
         return(NULL);
      if ( (res = D_markupdated(D_PHASFILE, c_buffer)) )
      {
         D_error(res);
         return(NULL);
      }
   }

   return(c_block.data + (trace - c_first)*(fn/2) + fpoint);
}

/****************************
*  setnormfactor()/2 
*  saves the maximum data point value in the phasefile block header
*  Rather than trying to scale the floating point number,  it is
*  saved directly into the structure member ctcount as a float.
*  Note that ctcount is declared an int.
****************************/
static void setnormfactor(int *ptr, float val)
{
   float *fptr;

   fptr = (float *)ptr;
   *fptr = val;
}
/****************************
*  getnormfactor()/2 
*  return the maximum data point
*  value from the phasefile block header
*  See setnormfactor above.
****************************/
static void getnormfactor(void *ptr, float *val)
{
   float *fptr;

   fptr = (float *)ptr;
   *val = *fptr;
}

/*---------------------------------------
|					|
|	       calc_user()/6		|
|					|
|   This function gives access to a	|
|   single data trace in a 2D data	|
|   set.  It also performs drift cor-	|
|   rection and normalization if	|
|   requested.				|
|					|
+--------------------------------------*/
/* fpoint	first point					*/
/* dcflag	calculate new lvl and tlt			*/
/* normok	calculate normalizing factor if "normflag"	*/
/* newspec	flag for whether phase file has been updated */
float *calc_user(int trace, int fpoint, int dcflag, int normok, int *newspec, int file_id)
{
   int		res,
		ok;
   float	datamax,
		*spec_ptr;


/*****************************************************
*  Check to see if data in phasefile block needs to  *
*  be recalculated from data in datafile block.      *
*****************************************************/

   *newspec = FALSE;
   if (interuption)
      return(NULL);
   if (checkblock(trace, &ok, file_id))
      return(NULL);

   if (!ok)
   { /* must calculate from complex or hypercomplex data */
      if (getdata())
         return(NULL);
      *newspec = TRUE;
   }

   spec_ptr = c_block.data + (trace - c_first)*(fn/2);

/*********************
*  Drift correction  *
*********************/

   if (dcflag)
      dodc(spec_ptr, 1, c_block.head->lvl, c_block.head->tlt);

   if ((fabs(c_block.head->lvl - (float)lvl) > 0.01) ||
        (fabs(c_block.head->tlt - (float)tlt) > 0.01))
   {
      leveltilt(spec_ptr, 1, c_block.head->lvl, c_block.head->tlt);
      c_block.head->lvl = (float)lvl;
      c_block.head->tlt = (float)tlt;
      c_block.head->scale = 0;
      c_block.head->ctcount = 0;
      *newspec = TRUE;
   }

/******************
*  Normalization  *
******************/

   if (normok && normflag)
   {
      if (c_block.head->ctcount == 0)
      {
         maxfloat(spec_ptr, fn/2, &datamax);
         setnormfactor(&(c_block.head->ctcount), datamax);
      }
      getnormfactor(&(c_block.head->ctcount), &datamax);
      if (datamax <= 1e-5)
         datamax = 1e-5;
      normalize = 1.0 / datamax;
   }

/*********************************************
*  If a "new spectrum" has been calculated,  *
*  update the internal buffer in which this  *
*  spectrum is stored.                       *
*********************************************/

   if (*newspec)
   {
      if ( (res = D_markupdated(file_id, c_buffer)) )
      {
         D_error(res);
         return(NULL);
      }
   }

   return(spec_ptr + fpoint);
}

extern float *mspec_getSpec(int trace, int npts, int fpoint);

/*-------------------------------
|				|
|	  calc_spec()/5		|
|				|
+------------------------------*/
float *calc_spec(int trace, int fpoint, int dcflag, int normok, int *newspec)
{
  float *fdfdata = NULL;
  float *data = calc_user(trace, fpoint, dcflag, normok, newspec, D_PHASFILE);
  if(data != NULL) { // do this only if trace in D_PHASFILE is ok. 
     fdfdata = mspec_getSpec(trace, fn/2, fpoint);
  }

  if(fdfdata != NULL) 
     return fdfdata;
  else
     return data;  
}

/*-------------------------------
|				|
|	  rel_spec()/0		|
|				|
+------------------------------*/
int rel_spec()
{
  int res;

  if(c_buffer>=0) /* release last used block */
    if ( (res=D_release(D_PHASFILE,c_buffer)) )
    {
      D_error(res); 
      D_close(D_PHASFILE);  
      ABORT;
    }
  RETURN;
}

int rel_data()
{
  if (D_release(D_DATAFILE,c_buffer))
     ABORT;
  else
     RETURN;
}

float *get_data_buf(int trace)
{
   struct datapointers datablock;

  if (D_getbuf(D_DATAFILE,datahead.nblocks,c_buffer,&datablock))
     return(0);
  else
     return(datablock.data + (trace - c_first)*pointsperspec);
}

void ds_phase_data(int trace, float *in_ptr, float *out_ptr, int fp, int np,
                   double l_phase, double r_phase)
{
  int	dtype, /* phtype,*/ patype;

  dtype = getdatatype(datahead.status);
  if (dtype == HYPERCOMPLEX)
     phase4(in_ptr + (fp*dtype), out_ptr, np, l_phase, r_phase, lp1, rp1,
            fn1/2, trace, revflag);
  else
  {
/*    phtype = get_phase_mode(HORIZ); */
     patype = get_phaseangle_mode(HORIZ);
     if (patype == 1)
        phaseangle2(in_ptr + (fp*dtype), out_ptr, np, COMPLEX, 1, 1, FALSE, l_phase, r_phase);
     else
        phase2(in_ptr + (fp*dtype), out_ptr, np, l_phase, r_phase);
  }
}

/*
 *  Eventually,  the parameter c_buffer will be defined as a static
 *  variable in this module.
 */
int cur_spec()
{
   return(c_buffer);
}

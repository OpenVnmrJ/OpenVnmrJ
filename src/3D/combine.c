/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "constant.h"


/*-----------------------------------------------
|                                       	|
|                combinef3()/8              	|
|                                       	|
|  The coefficients used to construct the 2 t2	|
|  interferograms from the `arraydim` (t1,t2)	|
|  elements are stored in the following order:	|
|						|
|						|
|      [ A(rr1), D(rr1), A(rr2), D(rr2),	|
|	 A(rr3), D(rr3), A(rr4), D(rr4),	|
|	 .............., A(rrN), D(rrN),	|
|	 					|
|	 A(ri1), D(ri1), A(ri2), D(ri2),	|
|	 A(ri3), D(ri3), A(ri4), D(ri4),	|
|	 .............., A(riN), D(riN),	|
|						|
|	 A(ir1), D(ir1), A(ir2), D(ir2),	|
|	 A(ir3), D(ir3), A(ir4), D(ir4),	|
|	 .............., A(irN), D(irN),	|
|						|
|	 A(ii1), D(ii1), A(ii2), D(ii2),	|
|	 A(ii3), D(ii3), A(ii4), D(ii4),	|
|	 .............., A(iiN), D(iiN) ]	|
|						|
|						|
|  The coefficient A(ri1) is multiplied with	|
|  the absorptive (real) part of the FIRST F3	|
|  spectrum in the series containing `arraydim`	|
|  elements.  The result is then added to the 	|
|  RI element of the hypercomplex t2 inter-	|
|  ferogram.  RI is short hand for Real(t1)-	|
|  Imag(t2).					|
|						|
+----------------------------------------------*/
void combinef3(data, combinebuf, coef, nbufpts, strtdif, ncmbpts,
			enddif, arraydim)
int             ncmbpts,	/* number of cx points to combine	*/
		nbufpts,	/* number of cx points in cmb buffer	*/
		strtdif,	/* number of cx points to skip at start	*/
		enddif,		/* number of cx points to skip at end	*/
                arraydim;	/* number of FID's per (t1,t2) element	*/
float           *data,		/* pointer to F3 frequency data		*/
                *combinebuf,	/* pointer to combined F3 data		*/
                *coef;		/* pointer to F3-FT3D coefficients	*/
{
   register int         i,
                        j,
                        k,
                        skip;
   register float       *tmpcombine,
			*tmpcoef,
			*tmpdata,
			rra_coef,
			rrd_coef,
			ria_coef,
			rid_coef;
   extern void		datafill();
 
 
   skip = HYPERCOMPLEX/2;
   tmpcoef = coef;
   datafill(combinebuf, HYPERCOMPLEX*nbufpts, 0.0);
 
   for (i = 0; i < skip; i++)
   {
      tmpdata = data + COMPLEX*strtdif;

      for (j = 0; j < arraydim; j++)
      {
         tmpcombine = combinebuf + i;
         rra_coef = *tmpcoef;
         rrd_coef = *(tmpcoef + 1);
         ria_coef = *(tmpcoef + HYPERCOMPLEX*arraydim);
         rid_coef = *(tmpcoef + HYPERCOMPLEX*arraydim + 1);
         tmpcoef += COMPLEX;
 
         for (k = 0; k < ncmbpts; k++)
         {
            *tmpcombine += rra_coef * (*tmpdata);
            *tmpcombine += rrd_coef * (*(tmpdata + 1));
            tmpcombine += skip;
            *tmpcombine += ria_coef * (*tmpdata++);
            *tmpcombine += rid_coef * (*tmpdata++);
            tmpcombine += skip;
         }

         tmpdata += COMPLEX*(enddif + strtdif);
      }
   }
}


/*-----------------------------------------------
|                                       	|
|                  combinef2()/5             	|
|                                       	|
|  The coefficients used to construct the one	|
|  t1 interferogram are stored in the same	|
|  format as described for ft2d() in VNMR.	|
|                                       	|
+----------------------------------------------*/
void combinef2(data, combinebuf, coef, npts, mode)
int             npts,		/* number of hypercomplex F2 points	*/
		mode;		/* type of F2 combination		*/
float           *data,		/* pointer to F2 frequency data		*/
                *combinebuf,	/* pointer to combined F2 data		*/
                *coef;		/* pointer to F2-FT3D coefficients	*/
{
   register int         i,
			j,
                        skip,
			dskip;
   register float       *tmpcombine,
			*r_tmpdata,
			*i_tmpdata,
                        ra_coef,
                        rd_coef,
			ia_coef,
			id_coef;
   extern void		datafill();
 
 
   datafill(combinebuf, HYPERCOMPLEX*npts, 0.0);
   dskip = HYPERCOMPLEX;
   skip = dskip/2;		/* the two F2 spectra which are to be
				   combined are interlinked in `data`. */


   for (i = 0; i < skip; i++)
   {
      ra_coef = *coef;
      rd_coef = *(coef + 1);
      ia_coef = *(coef + 2*COMPLEX);
      id_coef = *(coef + 2*COMPLEX + 1);

      r_tmpdata = data + i;
      i_tmpdata = r_tmpdata + skip;
      tmpcombine = combinebuf;

      if (mode == HYPERCOMPLEX)
      {
         for (j = 0; j < npts; j++)
         {
            *tmpcombine += ra_coef * (*r_tmpdata);
            (*tmpcombine++) += rd_coef * (*i_tmpdata);
            *tmpcombine -= rd_coef * (*r_tmpdata);
            (*tmpcombine++) += ra_coef * (*i_tmpdata);
            *tmpcombine += ia_coef * (*r_tmpdata);
            (*tmpcombine++) += id_coef * (*i_tmpdata);
            *tmpcombine -= id_coef * (*r_tmpdata);
            (*tmpcombine++) += ia_coef * (*i_tmpdata);

            r_tmpdata += dskip;
            i_tmpdata += dskip;
         }
      }
      else
      {
         for (j = 0; j < npts; j++)
         {
            *tmpcombine += ra_coef * (*r_tmpdata);
            *tmpcombine += rd_coef * (*i_tmpdata);
            tmpcombine += skip;
            *tmpcombine += ia_coef * (*r_tmpdata);
            *tmpcombine += id_coef * (*i_tmpdata);
            tmpcombine += skip;

            r_tmpdata += dskip;
            i_tmpdata += dskip;
         }
      }

      coef += COMPLEX;
   }


   r_tmpdata = data;
   tmpcombine = combinebuf;

   for (i = 0; i < (HYPERCOMPLEX*npts); i++)
      *r_tmpdata++ = *tmpcombine++;
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <math.h>
#include <sys/statvfs.h>

#include "constant.h"
#include "coef3d.h"
#include "lock3D.h"
#include "process.h"
#include "struct3d.h"

#define FT3D
#include "command.h"
#include "data.h"
#include "datac.h"
#undef FT3D

#include "fileio.h"


#define F3_FTNORM			5.0e-7
#define F2_FTNORM			1.0
#define F1_FTNORM			1.0

#define validF2fheader(status)					\
	((~status) & S_SECOND_D) && ((~status) & S_THIRD_D) &&	\
	(status & S_FIRST_D) && (status & S_DATA) &&		\
	(status & S_3D) && ((~status) & PROCSTART)

#define validF1fheader(status)					\
	(status & S_SECOND_D) && ((~status) & S_THIRD_D) &&	\
	(status & S_FIRST_D) && (status & S_DATA) &&		\
	(status & S_3D) && ((~status) & PROCSTART)


static int      doft = 1;
char		*userdir;
int		maxfn,			/* max real Fourier number           */
		maxfn12,		/* max F1-F2 real Fourier number     */
		master_ft3d;		/* Master ft3d flag		     */

extern void	Werrprintf(char *format, ...),
		Wlogprintf(char *format, ...),
		Wlogdateprintf(char *format, ...);

extern void	fiddc(),
		specdc();


/*---------------------------------------
|					|
|	      firstft()/11		|
|					|
+--------------------------------------*/
static int firstft(fid_fd, datafinfo, mempntr, f3coefval, sinetab,
	fidheader, p3Dinfo, pinfo, f3block, datahead, maxwords, xoff, yoff)
char		*mempntr;	/* pointer to memory			*/
int		fid_fd,		/* FID file descriptor			*/
		maxwords;	/* 3D internal data buffer size		*/
float		*f3coefval,	/* pointer to FT(t3) coefficients	*/
		*sinetab,	/* pointer to sine table		*/
		xoff,		/* dc offset for real points 		*/
		yoff;		/* dc offset for imaginary points	*/
filedesc	*datafinfo;	/* pointer to DATA file ID structure	*/
dfilehead	*fidheader;	/* pointer to FID file header		*/
datafileheader	*datahead;	/* pointer to 3D data file header	*/
proc3DInfo	*p3Dinfo;	/* pointer to 3D information structure	*/
comInfo		*pinfo;		/* pointer to command info structure	*/
f3blockpar	*f3block;	/* pointer to F3 block information	*/
{
   char		*scratch;
   int		done = FALSE,
		totalcount,
		lastfid,
		i,
		j,
		k,
		m,
		res,
		fidblock_no,
		nt1fids,
		nt2fids,
		ncf3pts,
		cxstpt,
		ncf3cmbpts,
		ncendpts;
   off_t	stwrite;
   float	*data,
		*svdata,
		*combinebuf;
   extern void	datafill(),
		weight(),
		fft(),
		phaserotate(),
		combinef3(),
		fidrotate();
   extern void  rotate2_center();


   if (pinfo->procf3acq.ival)
   { /* for interactive F3 processing */
      lastfid = datahead->lastfid;
      i = (lastfid + 1) / (p3Dinfo->arraydim * p3Dinfo->f2dim.scdata.np);
      j = ( (lastfid + 1) / p3Dinfo->arraydim ) % p3Dinfo->f2dim.scdata.np;
      totalcount = lastfid + 2;
      printf("\nStarted F3 processing at FID %d\n", totalcount);
   }
   else
   {
      lastfid = -1;
      totalcount = 1;
      i = 0;
      j = 0;
   }
 
   svdata = data = (float *)mempntr;
   combinebuf = data + maxwords;
   scratch = (char *) (combinebuf + 2*maxfn);

   ncf3pts = (p3Dinfo->f3dim.scdata.reginfo.endptzero -
			p3Dinfo->f3dim.scdata.reginfo.stptzero) / COMPLEX;
   cxstpt = p3Dinfo->f3dim.scdata.reginfo.stpt / COMPLEX;
   ncf3cmbpts = (p3Dinfo->f3dim.scdata.reginfo.endpt -
			p3Dinfo->f3dim.scdata.reginfo.stpt) / COMPLEX;
   ncendpts = (p3Dinfo->f3dim.scdata.fn - p3Dinfo->f3dim.scdata.reginfo.endpt)
			/ COMPLEX;

   nt2fids = ( (p3Dinfo->f2dim.scdata.np < p3Dinfo->f2dim.scdata.fn/COMPLEX)
		   ? p3Dinfo->f2dim.scdata.np :
			p3Dinfo->f2dim.scdata.fn/COMPLEX );

   nt1fids = ( (p3Dinfo->f1dim.scdata.np < p3Dinfo->f1dim.scdata.fn/COMPLEX)
		   ? p3Dinfo->f1dim.scdata.np :
			p3Dinfo->f1dim.scdata.fn/COMPLEX );


   while ( (i < nt2fids) && !done )
   {
      fidblock_no = (i*p3Dinfo->f1dim.scdata.np + j) * p3Dinfo->arraydim;

      while ( (j < nt1fids) && !done )
      {
         k = 0;

         while ( (k < p3Dinfo->arraydim) && !done )
         {
            if ( (totalcount & 255) == 0 )
               Wlogdateprintf("FT(t3) of FID number %d:", totalcount);

            if ( res = readFIDdata(fid_fd, data, scratch, fidheader,
			f3block->bytesperfid, p3Dinfo->f3dim.scdata.npadj,
			p3Dinfo->f3dim.scdata.lsfid, &lastfid, fidblock_no,
			f3block->dpflag, pinfo->fidmapfilepath.vset,
			pinfo->procf3acq.ival,&(p3Dinfo->f3dim.scdata.lpval)) )
            {
               if (res == LASTFID)
               {
                  done = TRUE;
               }
               else
               {
                  return(ERROR);
               }
            }
 
            if (!done)
            {
               data += p3Dinfo->f3dim.scdata.fn;
               fidblock_no += 1;
               k += 1;
               totalcount += 1;
            }
         }


         if (!done)
         {
            data = svdata;
            if (doft) for (k = 0; k < p3Dinfo->arraydim; k++)
            {
               if (p3Dinfo->f3dim.scdata.dcflag)
               {
                 register float *pntr;
                 register float xdc, ydc;
                 register int index;

                 pntr = data;
                 xdc = xoff;
                 ydc = yoff;
                 index = p3Dinfo->f3dim.scdata.npadj;
                 while (index--)
                 {
                    *pntr++ -= xdc;
                    *pntr++ -= ydc;
                 }
               }

               fiddc(data, p3Dinfo->f3dim.scdata.npadj,
                        p3Dinfo->f3dim.scdata.lsfid,
                        p3Dinfo->f3dim.scdata.fiddc,
                        COMPLEX);

               fidrotate(data, p3Dinfo->f3dim.scdata.npadj,
			p3Dinfo->f3dim.scdata.phfid,
			p3Dinfo->f3dim.scdata.lsfrq,
			COMPLEX);

               if (p3Dinfo->f3dim.scdata.sspar.zfsflag ||
		   p3Dinfo->f3dim.scdata.sspar.lfsflag)
               {
                  if ( fidss(p3Dinfo->f3dim.scdata.sspar, data, 
				p3Dinfo->f3dim.scdata.np,
				p3Dinfo->f3dim.scdata.lsfid) )
                  {
                     Werrprintf("Time-domain solvent subtraction failed");
                     return(ERROR);
                  }
               }

               if (p3Dinfo->f3dim.parLPdata.sizeLP)
               {
                  for (m = 0; m < p3Dinfo->f3dim.parLPdata.sizeLP; m++)
                  {
                     if ( lpz(totalcount-1, data, (p3Dinfo->f3dim.scdata.np
			    - p3Dinfo->f3dim.scdata.lsfid),
			    *(p3Dinfo->f3dim.parLPdata.parLP + m)) )
                     {
                        Werrprintf("LP analysis failed for F3 dimension");
                        return(ERROR);
                     }
                  }
               }

               datafill(data + COMPLEX*p3Dinfo->f3dim.scdata.npadj,
			p3Dinfo->f3dim.scdata.zfnum,
			0.0);
               weight(p3Dinfo->f3dim.ptdata.wtv, data,
                        p3Dinfo->f3dim.scdata.npadj,
                        p3Dinfo->f3dim.scdata.wtflag,
			COMPLEX);
               fft(data, sinetab, maxfn/COMPLEX,
			p3Dinfo->f3dim.scdata.fn/COMPLEX,
                        p3Dinfo->f3dim.scdata.pwr,
                        p3Dinfo->f3dim.scdata.zflvl,
                        COMPLEX, F3_FTNORM);
               if (fabs(p3Dinfo->f3dim.scdata.lpval) > MINDEGREE)
               {
                 rotate2_center(data, p3Dinfo->f3dim.scdata.fn/COMPLEX,
                        (double)p3Dinfo->f3dim.scdata.lpval*
                        (p3Dinfo->f3dim.scdata.fn/COMPLEX)/
                        (p3Dinfo->f3dim.scdata.fn/COMPLEX-1.0), (double)0.0);
               }
               phaserotate(p3Dinfo->f3dim.ptdata.phs, data,
                        p3Dinfo->f3dim.scdata.fn/COMPLEX,
			p3Dinfo->f3dim.scdata.dsply,
			COMPLEX);
               specdc(data + p3Dinfo->f3dim.scdata.reginfo.stpt,
			(p3Dinfo->f3dim.scdata.reginfo.endpt -
			 p3Dinfo->f3dim.scdata.reginfo.stpt)/COMPLEX,
			p3Dinfo->f3dim.scdata.specdc, COMPLEX);
 
               data += p3Dinfo->f3dim.scdata.fn;
            }
 
            data = svdata;
            combinef3(data, combinebuf, f3coefval, ncf3pts, cxstpt,
			ncf3cmbpts, ncendpts, p3Dinfo->arraydim);

            stwrite = 4*j + 2*i*p3Dinfo->f1dim.scdata.fn;
            stwrite *= (off_t) f3block->hcptspertrace * (off_t) sizeof(float);
            stwrite += datahead->nheadbytes;
        
            if ( f3block_wr(datafinfo, (char *)combinebuf, stwrite,
			f3block->hcptspertrace*HYPERCOMPLEX*sizeof(float)) )
            {
               return(ERROR);
            }
 
            datahead->lastfid = lastfid;
            j += 1;
         }
      }
 
      if (!done)
      {
         i += 1;
         j = 0;
      }
   }

/**********************************************
*  Adjust the number of time-domain points    *
*  in the other 2 dimensions based upon the   *
*  number of actual t3 FID's that were FT'd.  *
*  Set the appropriate status bits in the     *
*  DATA file header.                          *
**********************************************/
 
   if (!pinfo->procf3acq.ival)
   {
      if ( checkt2np(i, p3Dinfo) )
         return(ERROR);
   }

   datahead->Vfilehead.status |= (S_DATA|S_FLOAT|S_HYPERCOMPLEX|
					S_COMPLEX|S_FIRST_D|S_3D);

   for (i = 0; i < datafinfo->ndatafd; i++)
   {
      if ( writeDATAheader( *(datafinfo->dfdlist + i), datahead) )
         return(ERROR);
   }

   if (!pinfo->procf3acq.ival)
      Wlogdateprintf("\nFT of F3 Dimension Completed:");

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	      secondft()/10 		|
|					|
+--------------------------------------*/
static int secondft(dfd, mempntr, f3coef, f2coefval, sinetab, p3Dinfo,
	f3block, datahead, filenum, maxwords)
char		*mempntr;	/* pointer to memory			*/
int		dfd,		/* DATA file descriptor			*/
		filenum,	/* DATA file number - 1			*/
		maxwords;	/* 3D internal data buffer size		*/
float		*f2coefval,	/* pointer to FT(t2) coefficients	*/
		*sinetab;	/* pointer to sine table		*/
subcoef		f3coef;		/* FT(t3) subcoefficient structure	*/
proc3DInfo	*p3Dinfo;	/* pointer to 3D information structure	*/
f3blockpar	*f3block;	/* pointer to F3 block information	*/
datafileheader	*datahead;	/* pointer to DATA3D file header	*/
{
   int		i,
		j,
		k,
		m,
		totalcount = 1,
		totalt2count = 1,
		nt1fids;
   float	*data,
		*svdata,
		*scratch,
		*combinebuf;
   extern void	getcmplx_intfgm(),
		putcmplx_intfgm(),
		datafill(),
		weight(),
		fft(),
		phaserotate(),
		combinef2(),
		datashift(),
		fidrotate(),
		negateimag();


   svdata = data = (float *)mempntr;
   combinebuf = data + maxwords;
   scratch = combinebuf + 2*maxfn;

   nt1fids = ( (p3Dinfo->f1dim.scdata.np < p3Dinfo->f1dim.scdata.fn/COMPLEX)
		   ? p3Dinfo->f1dim.scdata.np :
			p3Dinfo->f1dim.scdata.fn/COMPLEX );

   for (j = 0; j < nt1fids; j++)
   {
      if (j)
      {
         Wlogdateprintf("File %d:  FT(t2) of t2 block %d:", filenum + 1,
			totalcount);
      }
      else
      {
         Wlogdateprintf("\n\nFile %d:  FT(t2) of t2 block %d:", filenum + 1,
			totalcount);
      }

      if ( f21block_io(dfd, data, scratch, j, f3block->hcptspertrace,
			READ, F2_DIMEN, HYPERCOMPLEX, p3Dinfo,
			datahead->nheadbytes) )
      {
         return(ERROR);
      }

      if (doft) for (k = 0; k < f3block->hcptspertrace; k++)
      {
         datashift(data, p3Dinfo->f2dim.scdata.npadj,
			p3Dinfo->f2dim.scdata.lsfid,
			HYPERCOMPLEX);
         fidrotate(data, p3Dinfo->f2dim.scdata.npadj,
                        p3Dinfo->f2dim.scdata.phfid,
                        p3Dinfo->f2dim.scdata.lsfrq,
                        HYPERCOMPLEX);
         fiddc(data, p3Dinfo->f2dim.scdata.npadj,
                        p3Dinfo->f2dim.scdata.lsfid,
                        p3Dinfo->f2dim.scdata.fiddc,
                        HYPERCOMPLEX);

         if (p3Dinfo->f2dim.parLPdata.sizeLP)
         {
            getcmplx_intfgm(data, scratch, p3Dinfo->f2dim.scdata.fn/2,
			COMPLEX);

            for (m = 0; m < p3Dinfo->f2dim.parLPdata.sizeLP; m++)
            {
               if ( lpz(totalt2count-1, scratch, (p3Dinfo->f2dim.scdata.np
                       - p3Dinfo->f2dim.scdata.lsfid),
                       *(p3Dinfo->f2dim.parLPdata.parLP + m)) )
               {
                  Werrprintf("LP analysis failed for F2 dimension");
                  return(ERROR);
               }
            }

            putcmplx_intfgm(data, scratch, p3Dinfo->f2dim.scdata.fn/2,
			COMPLEX);
            getcmplx_intfgm(data + 1, scratch, p3Dinfo->f2dim.scdata.fn/2,
			COMPLEX);

            for (m = 0; m < p3Dinfo->f2dim.parLPdata.sizeLP; m++)
            {
               if ( lpz(totalt2count-1, scratch, (p3Dinfo->f2dim.scdata.np
                       - p3Dinfo->f2dim.scdata.lsfid),
                       *(p3Dinfo->f2dim.parLPdata.parLP + m)) )
               {    
                  Werrprintf("LP analysis failed for F2 dimension");
                  return(ERROR);
               }
            }

            putcmplx_intfgm(data + 1, scratch, p3Dinfo->f2dim.scdata.fn/2,
			COMPLEX);
         }

         datafill(data + HYPERCOMPLEX*p3Dinfo->f2dim.scdata.npadj,
                        (HYPERCOMPLEX/2)*p3Dinfo->f2dim.scdata.zfnum,
                        0.0);
 
         if (f3coef.rr_nzero || f3coef.ri_nzero)
         {
            weight(p3Dinfo->f2dim.ptdata.wtv, data,
                        p3Dinfo->f2dim.scdata.npadj,
			p3Dinfo->f2dim.scdata.wtflag,
                        HYPERCOMPLEX);
            negateimag(data + 2, p3Dinfo->f2dim.scdata.fn/COMPLEX,
			p3Dinfo->f2dim.scdata.ntype,
			HYPERCOMPLEX);
            fft(data, sinetab, maxfn/COMPLEX,
			p3Dinfo->f2dim.scdata.fn/COMPLEX,
			p3Dinfo->f2dim.scdata.pwr,
			p3Dinfo->f2dim.scdata.zflvl,
			HYPERCOMPLEX, F2_FTNORM);
            phaserotate(p3Dinfo->f2dim.ptdata.phs, data,
                        p3Dinfo->f2dim.scdata.fn/COMPLEX,
			p3Dinfo->f2dim.scdata.dsply,
                        HYPERCOMPLEX);
         }

         if (f3coef.ir_nzero || f3coef.ii_nzero) 
         {
            weight(p3Dinfo->f2dim.ptdata.wtv, data + 1,
                        p3Dinfo->f2dim.scdata.npadj,
			p3Dinfo->f2dim.scdata.wtflag,
                        HYPERCOMPLEX);
            negateimag(data + 1, p3Dinfo->f2dim.scdata.fn/COMPLEX,
			p3Dinfo->f2dim.scdata.ntype,
			HYPERCOMPLEX);
            fft(data + 1, sinetab, maxfn/COMPLEX,
			p3Dinfo->f2dim.scdata.fn/COMPLEX,
                        p3Dinfo->f2dim.scdata.pwr,
                        p3Dinfo->f2dim.scdata.zflvl,
                        HYPERCOMPLEX, F2_FTNORM);
            phaserotate(p3Dinfo->f2dim.ptdata.phs, data + 1,
                        p3Dinfo->f2dim.scdata.fn/COMPLEX,
			p3Dinfo->f2dim.scdata.dsply,
			HYPERCOMPLEX);
         }

         specdc(data, p3Dinfo->f2dim.scdata.fn/COMPLEX,
                        p3Dinfo->f2dim.scdata.specdc,
                        HYPERCOMPLEX);
         combinef2(data, combinebuf, f2coefval,
                        (p3Dinfo->f2dim.scdata.fn/COMPLEX),
                        HYPERCOMPLEX);

         data += HYPERCOMPLEX * (p3Dinfo->f2dim.scdata.fn/COMPLEX);
         totalt2count += 1;
      }

      data = svdata;
      if (j == 0)
      {
         datahead->Vfilehead.status |= PROCSTART;
         if ( writeDATAheader(dfd, datahead) )
            return(ERROR);
      }

      if ( f21block_io(dfd, data, scratch, j, f3block->hcptspertrace,
                        WRITE, F2_DIMEN, HYPERCOMPLEX, p3Dinfo,
			datahead->nheadbytes) )
      {
         return(ERROR);
      }

      totalcount += 1;
   }


   datahead->Vfilehead.status |= S_SECOND_D;
   datahead->Vfilehead.status &= (~PROCSTART);
   Wlogdateprintf("\nFile %d:  FT of F2 Dimension Completed:", filenum + 1);
   return(COMPLETE);
}


/*---------------------------------------
|					|
|	      thirdft()/9		|
|					|
+--------------------------------------*/
static int thirdft(dfd, mempntr, sinetab, p3Dinfo, f3block, pinfo,
	datahead, filenum, maxwords)
char		*mempntr;	/* pointer to memory			*/
int		dfd,		/* DATA file descriptor			*/
		filenum,	/* DATA file number - 1			*/
		maxwords;	/* 3D internal data buffer size		*/
float		*sinetab;	/* pointer to sine table		*/
proc3DInfo	*p3Dinfo;	/* pointer to 3D information structure	*/
f3blockpar	*f3block;	/* pointer to F3 block information	*/
comInfo		*pinfo;		/* pointer to command line structure	*/
datafileheader	*datahead;	/* pointer to DATA3D file header	*/
{
   int		i,
		j,
		k,
		m,
		wrdatatype,
		dosecondF1ft,
		totalcount = 1,
		totalt1count = 1;
   float	*data,
		*scratch,
		*svdata;
   extern void	getcmplx_intfgm(),
		putcmplx_intfgm(),
		datafill(),
		weight(),
		fft(),
		specdc(),
		calc3Ddisplay(),
		phaserotate(),
		datashift(),
		fidrotate(),
		getmaxmin(),
		negateimag();


   svdata = data = (float *)mempntr;
   scratch = data + maxwords + 2*maxfn;
   dosecondF1ft = ((p3Dinfo->f2dim.scdata.dsply << 11) &
			(NI2_AVMODE|NI2_PWRMODE))
			|| (p3Dinfo->datatype == HYPERCOMPLEX);
   wrdatatype = ( pinfo->reduce.ival ? REAL : p3Dinfo->datatype );
   

   for (j = 0; j < (p3Dinfo->f2dim.scdata.fn/COMPLEX); j++)
   {
      if (j)
      {
         Wlogdateprintf("File %d:  FT(t1) of t1 block %d:", filenum + 1,
			totalcount);
      }
      else
      {
         Wlogdateprintf("\n\nFile %d:  FT(t1) of t1 block %d:", filenum + 1,
			totalcount);
      }

      if ( f21block_io(dfd, data, scratch, j, f3block->hcptspertrace,
			READ, F1_DIMEN, HYPERCOMPLEX, p3Dinfo,
			datahead->nheadbytes) )
      {
         return(ERROR);
      }

      if (doft) for (k = 0; k < f3block->hcptspertrace; k++)
      {
         datashift(data, p3Dinfo->f1dim.scdata.npadj,
			p3Dinfo->f1dim.scdata.lsfid,
			HYPERCOMPLEX);
         fidrotate(data, p3Dinfo->f1dim.scdata.npadj,
                        p3Dinfo->f1dim.scdata.phfid,
                        p3Dinfo->f1dim.scdata.lsfrq,
                        HYPERCOMPLEX);
         fiddc(data, p3Dinfo->f1dim.scdata.npadj,
                        p3Dinfo->f1dim.scdata.lsfid,
                        p3Dinfo->f1dim.scdata.fiddc,
                        HYPERCOMPLEX);

         if (p3Dinfo->f1dim.parLPdata.sizeLP)
         {
            getcmplx_intfgm(data, scratch, p3Dinfo->f1dim.scdata.fn/2,
			COMPLEX);

            for (m = 0; m < p3Dinfo->f1dim.parLPdata.sizeLP; m++)
            {
               if ( lpz(totalt1count-1, scratch, (p3Dinfo->f1dim.scdata.np
                          - p3Dinfo->f1dim.scdata.lsfid),
                          *(p3Dinfo->f1dim.parLPdata.parLP + m)) )
               {
                  Werrprintf("LP analysis failed for F1 dimension");
                  return(ERROR);
               }
            }

            putcmplx_intfgm(data, scratch, p3Dinfo->f1dim.scdata.fn/2,
			COMPLEX);

            if (dosecondF1ft)
            {
               getcmplx_intfgm(data + 1, scratch, p3Dinfo->f1dim.scdata.fn/2,
			COMPLEX);

               for (m = 0; m < p3Dinfo->f1dim.parLPdata.sizeLP; m++)
               {
                  if ( lpz(totalt1count-1, scratch, (p3Dinfo->f1dim.scdata.np
                             - p3Dinfo->f1dim.scdata.lsfid),
                             *(p3Dinfo->f1dim.parLPdata.parLP + m)) )
                  {
                     Werrprintf("LP analysis failed for F1 dimension");
                     return(ERROR);
                  }
               }

               putcmplx_intfgm(data + 1, scratch, p3Dinfo->f1dim.scdata.fn/2,
			COMPLEX);
            }
         }

         datafill(data + HYPERCOMPLEX*p3Dinfo->f1dim.scdata.npadj,
                        (HYPERCOMPLEX/2)*p3Dinfo->f1dim.scdata.zfnum,
                        0.0);
         weight(p3Dinfo->f1dim.ptdata.wtv, data,
                        p3Dinfo->f1dim.scdata.npadj,
			p3Dinfo->f1dim.scdata.wtflag,
                        HYPERCOMPLEX);
         negateimag(data + 2, p3Dinfo->f1dim.scdata.fn/COMPLEX,
			p3Dinfo->f1dim.scdata.ntype,
			HYPERCOMPLEX);
         fft(data, sinetab, maxfn/COMPLEX,
			p3Dinfo->f1dim.scdata.fn/COMPLEX,
                        p3Dinfo->f1dim.scdata.pwr,
                        p3Dinfo->f1dim.scdata.zflvl,
			HYPERCOMPLEX, F1_FTNORM);
         phaserotate(p3Dinfo->f1dim.ptdata.phs, data,
                        p3Dinfo->f1dim.scdata.fn/COMPLEX,
			p3Dinfo->f1dim.scdata.dsply,
                        HYPERCOMPLEX);
 
         if (dosecondF1ft)
         {
            weight(p3Dinfo->f1dim.ptdata.wtv, data + 1,
                        p3Dinfo->f1dim.scdata.npadj,
			p3Dinfo->f1dim.scdata.wtflag,
                        HYPERCOMPLEX);
            negateimag(data + 1, p3Dinfo->f1dim.scdata.fn/COMPLEX,
			p3Dinfo->f1dim.scdata.ntype,
			HYPERCOMPLEX);
            fft(data + 1, sinetab, maxfn/COMPLEX,
			p3Dinfo->f1dim.scdata.fn/COMPLEX,
                        p3Dinfo->f1dim.scdata.pwr,
                        p3Dinfo->f1dim.scdata.zflvl,
			HYPERCOMPLEX, F1_FTNORM);
            phaserotate(p3Dinfo->f1dim.ptdata.phs, data + 1,
                        p3Dinfo->f1dim.scdata.fn/COMPLEX,
			p3Dinfo->f1dim.scdata.dsply,
                        HYPERCOMPLEX);
         }

         specdc(data, p3Dinfo->f1dim.scdata.fn/COMPLEX,
                        p3Dinfo->f1dim.scdata.specdc,
                        HYPERCOMPLEX);
         calc3Ddisplay(data, p3Dinfo->f1dim.scdata.fn/COMPLEX,
			p3Dinfo->f2dim.scdata.dsply,
			p3Dinfo->f1dim.scdata.dsply);
         getmaxmin(datahead, data, p3Dinfo->f1dim.scdata.fn/COMPLEX,
			wrdatatype, p3Dinfo->datatype);
         data += HYPERCOMPLEX * (p3Dinfo->f1dim.scdata.fn/COMPLEX);
         totalt1count += 1;
      }
 
      data = svdata;
      if (j == 0)
      {
         datahead->Vfilehead.status |= PROCSTART;
         if ( writeDATAheader(dfd, datahead) )
            return(ERROR);
      }

      if ( f21block_io(dfd, data, scratch, j, f3block->hcptspertrace,
			WRITE, F1_DIMEN, wrdatatype, p3Dinfo,
			datahead->nheadbytes) )
      {
         return(ERROR);
      }

      totalcount += 1;
   }

 
/*
 * fn*fn1*fn2 can overflow an integer.  Therefore, divide the factors to avoid
 * overflow
 */
   if ( ftruncate( dfd, (off_t)
             ( ( (p3Dinfo->f1dim.scdata.fn/8) * (p3Dinfo->f2dim.scdata.fn/datahead->ndatafiles) *
		 (p3Dinfo->f3dim.scdata.reginfo.endptzero - p3Dinfo->f3dim.scdata.reginfo.stptzero) *
                  wrdatatype * sizeof(float) ) + datahead->nheadbytes) ) )
   {
      Werrprintf("\nthirdft():  cannot truncate 3D data file");
      return(ERROR);
   }
 
   datahead->Vfilehead.status |= (S_THIRD_D | S_SPEC);
   datahead->Vfilehead.status &= (~PROCSTART);
   if (wrdatatype < HYPERCOMPLEX)
   {
      datahead->Vfilehead.status &= ~(S_HYPERCOMPLEX);
      if (wrdatatype < COMPLEX)
         datahead->Vfilehead.status &= ~(S_COMPLEX);
   }

   datahead->Vfilehead.np /= (HYPERCOMPLEX/wrdatatype);
   datahead->Vfilehead.tbytes /= (HYPERCOMPLEX/wrdatatype);
   datahead->Vfilehead.bbytes /= (HYPERCOMPLEX/wrdatatype);

   Wlogdateprintf("\nFile %d:  FT of F1 Dimension Completed:", filenum + 1);
   return(COMPLETE);
}


/*---------------------------------------
|					|
|	    checkdiskspace()/3		|
|					|
+--------------------------------------*/
int checkdiskspace(info3d, outputdir, procdim)
char		*outputdir;
proc3DInfo	*info3d;
procMode	procdim;
{
   unsigned int req_kbytes = 100;
   unsigned int free_kbytes;

   struct statvfs	freeblocks_buf;

   statvfs(outputdir, &freeblocks_buf);
   if (freeblocks_buf.f_frsize >= 1024) {
       free_kbytes = freeblocks_buf.f_bavail * (freeblocks_buf.f_frsize / 1024);
   } else {
       free_kbytes = freeblocks_buf.f_bavail / (1024 / freeblocks_buf.f_frsize);
   }

   if (procdim.procf3)
   {
/*
 * fn*fn1*fn2 can overflow an integer.  Therefore, divide the factors to avoid
 * overflow
 */
       req_kbytes += (unsigned int)
               (((info3d->f3dim.scdata.reginfo.endptzero - info3d->f3dim.scdata.reginfo.stptzero)
                 * (info3d->f2dim.scdata.fn/8)
                 * (info3d->f1dim.scdata.fn/8) ) / 8);
   }
   else
   {
      return(COMPLETE);
   }

   if (free_kbytes < req_kbytes) {
       fprintf(stderr,"start=%d, end=%d, f1dim=%d, f2dim=%d\n",
               info3d->f3dim.scdata.reginfo.stptzero,
               info3d->f3dim.scdata.reginfo.endptzero,
               info3d->f1dim.scdata.fn,
               info3d->f2dim.scdata.fn);
               
       fprintf(stderr, "Need %u kB in %s, have %u\n",
               req_kbytes, outputdir, free_kbytes);
   }

   return (free_kbytes < req_kbytes);
}

static void
rm_numbered_files(basename)
  char *basename;
{
    char fname[MAXPATHL+1];
    int i;

    for (i=1; 1; i++){
	sprintf(fname,"%s%d", basename, i);
	if (access(fname, W_OK) == -1){
	    break;
	}
	unlink(fname);
    }
}

static void vnmrMsg(comInfo *pinfo, char *msg)
{
   if (pinfo->hostAddr.vset)
   {
      char host[MAXPATHL];
      char port[MAXPATHL];
      char dmsg[MAXPATHL];
      int  ppid;
      int  err;

      if ( (sscanf(pinfo->hostAddr.sval,"%s %s %d",host, port, &ppid) == 3) && !kill(ppid,0) )
      {
         sprintf(dmsg,"write('line3','%s') write('alpha','%s')\n",msg, msg);
         net_write(host, port, dmsg);
      }
      else
      {
         printf(msg);
         printf("\n");
      }
   }
   else
   {
      printf(msg);
      printf("\n");
   }
}

/*---------------------------------------
|					|
|		main()/2		|
|					|
+--------------------------------------*/
int main(int argc, char *argv[])
{
   char				*mempntr,
				*pwdir,
				*username,
				workingdirpath[ MAXPATHL ],
				hostname[MAXPATHL];
   char *fdf_plane = "f1f3";
   int				add_fdf_plane,
       				i,
				done,
       				extr_for_fdf,
				fid_fd,
   				hdr_bytes,
				maxwords,
				ndatafiles,
				nmembytes;
   float			*sinetab;
   dfilehead			*fidheader;
   datafileheader		*datahead;
   coef3D			*coef;
   comInfo			*pinfo;
   proc3DInfo			*p3Dinfo;
   f3blockpar			*f3block;
   filedesc			*datafinfo;
   procMode			procinfo;
   extern char			*getcwd();
   extern float			*create_sinetable();
   extern void			pathadj(),
#ifndef LINUX
				remote_ft3d(),
#endif
				closeDATAfiles(),
				openlogfile(),
				closelogfile(),
				openmasterlogfile(),
				closemasterlogfile();
   extern comInfo		*parseinput();
   extern proc3DInfo		*read3Dinfo();
   extern dfilehead		*readFIDheader();
   extern datafileheader	*readDATAheader(),
				*initDATAheader();
   extern f3blockpar		*setf3blockpar();
   extern filedesc		*openDATAfiles();
   extern coef3D		*readcoefs();


/*************************************************
*  Parse the command line entry.  Read the 3D    *
*  processing information from disk and load     *
*  into the internal data structure.  Check      *
*  these structure elements for consistency.     *
*  Create the FID map if this function has been  *
*  requested.                                    *
*************************************************/

   doft = ( strstr(argv[0], "noft3d") == NULL );
   master_ft3d = !( strcmp(argv[0], "ftr3d") == 0 );
   hdr_bytes = 0;

   if ( (pinfo = parseinput(argc, argv, &procinfo, master_ft3d)) == NULL )
      return ERROR;

   if (master_ft3d)
      openmasterlogfile(pinfo);

   if ( (p3Dinfo = read3Dinfo(pinfo->info3Ddirpath.sval)) == NULL )
   {
      removelock(pinfo->info3Ddirpath.sval);
      return ERROR;
   }

   if ( check3Dinfo(p3Dinfo) )
      return ERROR;

   if ( checkdiskspace(p3Dinfo, pinfo->datadirpath.sval, procinfo) )
   {
      Werrprintf("\nmain():  insufficient disk space available");
      return ERROR;
   }

   if (pinfo->fidmapfilepath.vset)
   {
      if ( createFIDmap() )
         return ERROR;
   }


/***********************************************
*  Read the FT3D processing coefficients from  *
*  disk and load into memory.                  *
***********************************************/

   if ( (coef = readcoefs(pinfo->coeffilepath.sval,
	   p3Dinfo->arraydim)) == NULL )
   {
      return ERROR;
   }


/**************************************************
*  Open the FID file if F3 processing is being    *
*  performed.  Then read in the FID file header.  *
**************************************************/

   if (procinfo.procf3)
   {
      char	fidfilepath[MAXPATHL];


      (void) strcpy(fidfilepath, pinfo->fiddirpath.sval);
      (void) strcat(fidfilepath, "/fid");

      if ( (fid_fd = open(fidfilepath, O_RDONLY, 0440)) < 0 )
      {
         Werrprintf("\nmain():  cannot open FID file %s", fidfilepath);
         return ERROR;
      }

      if ( (fidheader = readFIDheader(fid_fd)) == NULL )
      {
         (void) close(fid_fd);
         return ERROR;
      }
   }


/*******************************************
*  Create the sine table based upon the    *
*  largest FN value for the 3 dimensions.  *
*******************************************/

   if ( (sinetab = create_sinetable(p3Dinfo)) == NULL )
   {
      (void) close(fid_fd);
      return ERROR;
   }


/*******************************************************
*  Calculate the I/O parameters pertaining to reading  *
*  in the t3 time-domain data and writing out the F3   *
*  frequency-domain data.                              *
*******************************************************/

   maxwords = calcmaxwords();

   if (procinfo.procf3)
   {
      if ( (f3block = setf3blockpar(p3Dinfo, pinfo, &maxwords, fidheader))
		== NULL )
      {
         (void) close(fid_fd);
         return ERROR;
      }
   }


/*******************************************
*  Adjust `np` and `lsfid` for all three   *
*  dimensions prior to calculating the F3  *
*  block I/O parameters.                   *
*******************************************/

   if ( setftpar(p3Dinfo) )
   {
      (void) close(fid_fd);
      return ERROR;
   }


/*********************************************
*  Allocate memory for data, scratch space,  *
*  and the combination buffer.               *
*********************************************/

   nmembytes = 2*maxfn + maxwords;
   nmembytes += 2 * ( (maxfn > p3Dinfo->f3dim.scdata.np) ? maxfn :
			p3Dinfo->f3dim.scdata.np );
   nmembytes *= sizeof(float);

   if ( (mempntr = (char *)malloc( (unsigned)nmembytes )) == NULL )
   {
      Werrprintf("\nmain():  cannot allocate memory for 3D FT");
      (void) close(fid_fd);
      return ERROR;
   }

/*******************************************
*  Get user and host names for log files.  *
*******************************************/

   username = getenv("LOGNAME");
   (void) strcpy(hostname, "");
   gethostname(hostname, MAXPATHL-1);


/*********************************************************************
*  Open the DATA file(s).  Read in the DATA file header if the DATA  *
*  file can be opened.  If it cannot be opened, initialize the DATA  *
*  file header.  For F3, one must open all the DATA files.  For      *
*  (F1,F2), one must open the DATA files one at a time.  All of the  *
*  DATA files are closed after the F3 processing.  The function      *
*  "openDATAfiles()" also installs a lock file for each DATA file.   *
*  The function "closeDATAfiles()" removes each lock file as each    *
*  respective DATA file is closed.                                   *
*********************************************************************/

   if (procinfo.procf3)
   {
      float xoff, yoff;

      if ( (datafinfo = openDATAfiles(pinfo, ALL, CHECK))
		== NULL )
      {
         (void) close(fid_fd);
         return ERROR;
      }
      else if (datafinfo->result)
      {
         (void) close(fid_fd);
         closeDATAfiles(datafinfo, pinfo);
         datafinfo = NULL;
         return ERROR;
      }

      if (datafinfo->dataexists)
      { /* read the header from the first DATA file only */
         if ( (datahead = readDATAheader(*(datafinfo->dfdlist), coef) )
		   == NULL )
         {
            (void) close(fid_fd);
            closeDATAfiles(datafinfo, pinfo);
            datafinfo = NULL;
            return ERROR;
         }

         free((char *)f3block);
         f3block = &(datahead->f3blockinfo);
      }
      else
      {
         if ( (datahead = initDATAheader(f3block, coef, p3Dinfo, pinfo))
		   == NULL )
         {
            (void) close(fid_fd);
            closeDATAfiles(datafinfo, pinfo);
            datafinfo = NULL;
            return ERROR; 
         }
      }


/*****************
*  F3 transform  *
*****************/

      openlogfile(pinfo, F3_DIMEN, 1);
      if ( username != NULL )
         Wlogprintf("User = %s", username);

      if ( strcmp(hostname, "") != 0 )
         Wlogprintf("Host = %s\n\n", hostname);
      xoff = yoff = 0;
      if (p3Dinfo->f3dim.scdata.dcflag)
      {
         dblockhead block0head;
         lseek(fid_fd, (off_t) sizeof(struct datafilehead), SEEK_SET);
         if (read(fid_fd, (char *) (&block0head), sizeof(dblockhead)) ==
                sizeof(dblockhead) )
         {
#ifdef LINUX
            floatInt un;

            un.fval = &block0head.lvl;
            xoff = ntohl( *un.ival);
            un.fval = &block0head.tlt;
            yoff = ntohl( *un.ival);
#else
            xoff = block0head.lvl;
            yoff = block0head.tlt;
#endif
         }
         if (ntohl(block0head.ctcount) != 1)
           p3Dinfo->f3dim.scdata.dcflag = 0;
      }

      if ( firstft(fid_fd, datafinfo, mempntr, coef->f3t2.coefval, sinetab,
		fidheader, p3Dinfo, pinfo, f3block, datahead, maxwords, xoff, yoff) )
      {
         (void) close(fid_fd);
         closeDATAfiles(datafinfo, pinfo);
         datafinfo = NULL;
         return ERROR;
      }

      (void) close(fid_fd);
      closeDATAfiles(datafinfo, pinfo);
      closelogfile();
      datafinfo = NULL;

      if ( copyinfofiles(pinfo) )
         return ERROR;
   }


/******************************
*  F2 and/or F1 transform(s)  *
******************************/

   if (procinfo.procf2 || procinfo.procf1)
   {
      done = FALSE;
      i = 0;
      ndatafiles = 1;

      while (!done)
      {

/***************************************
*  Open the first data file READ ONLY  *
*  to find out how many DATA files     *
*  were used.                          *
***************************************/

         if (i == 0)
         {
            if ( (datafinfo = openDATAfiles(pinfo, i, CHECK|READONLY))
			== NULL )
            {
               return ERROR;
            }

            if (datafinfo->result == NOMORE_DFILES)
            {
               Werrprintf("\nmain():  cannot open one of the 3D DATA files");
               return ERROR;
            }

            if ( (datahead = readDATAheader(*(datafinfo->dfdlist + i),
		      coef) ) == NULL )
            {
               closeDATAfiles(datafinfo, pinfo);
               datafinfo = NULL;
               return ERROR;
            }

            closeDATAfiles(datafinfo, pinfo);
            ndatafiles = datahead->ndatafiles;
            pinfo->multifile.ival = ndatafiles;
            datafinfo = NULL;
         }

/*********************************************************************
*  Attempt to open the (i+1)-th DATA file.  If the file does not     *
*  exist, then there are no more data files and processing should    *
*  stop.  If the file exists but is locked by another FT3D process,  *
*  go on to the next DATA file.  If the former DATA file exists and  *
*  is not locked, do the requested processing on it.                 *
*********************************************************************/

         if ( (datafinfo = openDATAfiles(pinfo, i, CHECK))
			== NULL )
         {
            return ERROR;
         }

         switch (datafinfo->result)
         {
            case OK:
            case LOCKED_DFILE:   break;
            case NOMORE_DFILES:	 done = TRUE;
				 break;
            default:		 Werrprintf("\nmain():  internal error");
				 closeDATAfiles(datafinfo, pinfo);
         			 datafinfo = NULL;
				 return ERROR;
         }

         if (done)
            break;

         if (datafinfo->result != LOCKED_DFILE)
         {
#ifndef LINUX
            if (i == 0)
            {
               if (!master_ft3d)
               {
                  Werrprintf("\nmain():  this ft3d should be the master");
                  closeDATAfiles(datafinfo, pinfo);
                  datafinfo = NULL;
                  return ERROR;
               }

               if (pinfo->xrpc.vset)
               {
                  if ( (pwdir = getcwd( &workingdirpath[ 0 ], sizeof( workingdirpath ))) != NULL )
                  {
                     char	autofile[MAXPATHL];

                     pathadj(pinfo->datadirpath.sval, pwdir, autofile);
                     (void) strcat(autofile, "/info/auto");
                     remote_ft3d(ndatafiles, autofile, pinfo->procmode.sval);
                  }
               }
            }
#endif

            if ( (datahead = readDATAheader(*(datafinfo->dfdlist + i),
		      coef) ) == NULL )
            {
               closeDATAfiles(datafinfo, pinfo);
               datafinfo = NULL;
               return ERROR;
            }

            f3block = &(datahead->f3blockinfo);

/*****************************************
*  Do the F2 transform if requested and  *
*  if the header status is appropriate   *
*  for such processing.                  *
*****************************************/

            if ( validF2fheader(datahead->Vfilehead.status) &&
		 procinfo.procf2 )
            {
               if ( checkt2np( (datahead->lastfid + 1) /
			(p3Dinfo->f1dim.scdata.np *
			p3Dinfo->arraydim), p3Dinfo ) )
               {
                  closeDATAfiles(datafinfo, pinfo);
                  datafinfo = NULL;
                  return ERROR;
               }

               openlogfile(pinfo, F2_DIMEN, i+1);
               if ( username != NULL )
                  Wlogprintf("User = %s", username);

               if ( strcmp(hostname, "") != 0 )
                  Wlogprintf("Host = %s", hostname);

               if ( secondft( *(datafinfo->dfdlist + i), mempntr, coef->f3t2,
			   coef->f2t1.coefval, sinetab, p3Dinfo, f3block,
			   datahead, i, maxwords) )
               {
                  closeDATAfiles(datafinfo, pinfo);
                  closelogfile();
                  datafinfo = NULL;
                  return ERROR;
               }

               closelogfile();
            }


/*****************************************
*  Do the F1 transform if requested and  *
*  if the header status is appropriate   *
*  for such processing.                  *
*****************************************/

            if ( validF1fheader(datahead->Vfilehead.status) &&
		 procinfo.procf1 )
            {
               openlogfile(pinfo, F1_DIMEN, i+1);
               if ( username != NULL )
                  Wlogprintf("User = %s", username);

               if ( strcmp(hostname, "") != 0 )
                  Wlogprintf("Host = %s", hostname);

               if ( thirdft( *(datafinfo->dfdlist + i), mempntr, sinetab,
			   p3Dinfo, f3block, pinfo, datahead, i, maxwords) )
               {
                  closeDATAfiles(datafinfo, pinfo);
                  closelogfile();
                  datafinfo = NULL;
                  return ERROR;
               }

               closelogfile();
            }

/***************************************
*  Write out the DATA file header and  *
*  then close the DATA file.           *
***************************************/

            if ( writeDATAheader( *(datafinfo->dfdlist + i), datahead) )
            {
               closeDATAfiles(datafinfo, pinfo);
               datafinfo = NULL;
               return ERROR;
            }
	    hdr_bytes = lseek(*(datafinfo->dfdlist + i), (off_t) 0L, SEEK_CUR);

            closeDATAfiles(datafinfo, pinfo);
            datafinfo = NULL;
         }

         i += 1;
         if (i >= datahead->ndatafiles)
            done = TRUE;
      }
   }


/******************************************
*  If the `-x` option has been selected,  *
*  wait until there are no more locked    *
*  3D data files around.  Then, begin     *
*  the plane extraction.                  *
******************************************/

   extr_for_fdf = FALSE;
   add_fdf_plane = !strstr(pinfo->getplane.sval, fdf_plane);
   if ( master_ft3d
	&& (pinfo->fdfheaderpath.vset
	    && (add_fdf_plane || !pinfo->getplane.vset)))
   {
      extr_for_fdf = TRUE;
      if (add_fdf_plane){
	  if (pinfo->getplane.vset){
	      strcat(pinfo->getplane.sval, ":");
	      strcat(pinfo->getplane.sval, fdf_plane);
	  }else{
	      strcpy(pinfo->getplane.sval, fdf_plane);
	  }
      }
   }
   if ( master_ft3d
	&& (pinfo->getplane.vset || extr_for_fdf)
	&& !pinfo->procf3acq.ival)
   {
      char		syscmd[MAXPATHL],
			datapath[MAXPATHL];
      int		n;


      while ( checkforanylocks(pinfo->datadirpath.sval,
		   datahead->ndatafiles) )
      {
         sleep(10);
      }

      (void) strcpy(datapath, pinfo->datadirpath.sval);
      n = strlen(datapath);
      datapath[n-5] = '\0';

      if (pinfo->curexp.vset)
      {
         (void) sprintf(syscmd,
	      "getplane -e %d -i \"%s\" -o \"%s\" -p %s", pinfo->curexp.ival,
		   datapath, datapath, pinfo->getplane.sval);
      }
      else
      {
         (void) sprintf(syscmd,
	      "getplane -i \"%s\" -p %s", datapath, pinfo->getplane.sval);
      }

      (void) system(syscmd);
   }

   if (master_ft3d && pinfo->fdfheaderpath.vset)
   {
       /* Produce the FDF format output */
       char fname[MAXPATHL+1];
       char hname[MAXPATHL+1];
       char oname[MAXPATHL+1];
       char sname[MAXPATHL+1];
       char command[2*MAXPATHL + 32];
       hdr_bytes = sizeof(dfilehead) + sizeof(dblockhead);
       sprintf(fname,"%.*s/data", MAXPATHL-16, pinfo->datadirpath.sval);
       sprintf(hname,"%.*s", MAXPATHL-16, pinfo->fdfheaderpath.sval);
       sprintf(oname,"%.*s/data.fdf", MAXPATHL-16, pinfo->datadirpath.sval);
       sprintf(sname,"%.*s/../extr/data%s.",
	       MAXPATHL-16, pinfo->datadirpath.sval, fdf_plane);
#ifdef LINUX
       sprintf(command,"fdfgluer -byteswap 1 -offset %d -infiles %s# %s >%s",
	       hdr_bytes, sname, hname, oname);
#else
       sprintf(command,"fdfgluer -offset %d -infiles %s# %s >%s",
	       hdr_bytes, sname, hname, oname);
#endif
       (void)system(command);	/* Create FDF file */
       sprintf(command,"/bin/rm \"%s\"", hname);
       (void)system(command);	/* Remove header file */
       rm_numbered_files(fname);/* Remove old data file(s) */
       if (add_fdf_plane){
	   /* Remove unrequested plane extraction files. */
	   rm_numbered_files(sname);
       }
   }


   if (master_ft3d)
   {
      char	datapath[MAXPATHL];
      FILE	*fd;


      if (!pinfo->procf3acq.ival)
      {
         if (pinfo->curexp.vset)
         { /* mainly for VNMR */
            if (procinfo.procf1)
            {
               vnmrMsg(pinfo,"ft3d done!");
               (void) strcpy(datapath, pinfo->datadirpath.sval);
               (void) strcat(datapath, "/done");
               if ( (fd = fopen(datapath, "w")) != NULL )
               {
                  fprintf(fd, "The 3D FT has finished.\n");
                  fclose(fd);
               }
            }
            else if (procinfo.procf2)
            {
               if (procinfo.procf3)
               {
                  vnmrMsg(pinfo,"F3-F2 transform of ft3d done!");
               }
               else
               {
                  vnmrMsg(pinfo,"F2 transform of ft3d done!");
               }
            }
            else
            {
               vnmrMsg(pinfo,"F3 transform of ft3d done!");
            }
         }
      }

      closemasterlogfile();
   }
/*******************************
*  Cleanup memory allocations  *
*******************************/
   free( (char *)mempntr );
   free( (char *)datahead );
   free( (char *)pinfo );
   free( (char *)p3Dinfo );
   free( (char *)sinetab );
   free( (char *)fidheader );
   free( (char *) (coef->f3t2.coefval) );
   free( (char *)coef );

   return COMPLETE;
}

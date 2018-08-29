/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <sys/file.h>

#define MAXPATHL	128
#include "process.h"
#include "struct3d.h"		/* requires ftpar.h too */

#define MAXDIM		3

extern char	*malloc();


main(argc, argv)
char	*argv[];
int	argc;
{
   int		i,
		j,
		fd;
   FILE		*fdes,
		*fopen();
   proc3DInfo	*infopntr;


/*********************************************
*  Read in 3D information in binary format.  *
*********************************************/

   if ( (fd = open("info/procdat", O_RDONLY, 0440)) < 0 )
   {
      printf("cannot open `info/procdat` file\n");
      return -1;
   }

   if ( (infopntr = (proc3DInfo *) malloc( (unsigned)sizeof(proc3DInfo) ))
                == NULL )
   {
      printf("cannot allocate memory for 3D information\n");
      (void) close(fd);
      return -1;
   }

   if ( read(fd,  (char *) (&(infopntr->vers)), sizeof(int)) != sizeof(int) )
   {
      printf("cannot read `version` parameter\n");
      (void) close(fd);
      return -1;
   }

   if ( infopntr->vers != FT3D_VERSION )
   {
      printf("version clash with 3D information file\n");
      (void) close(fd);
      return -1;
   }

   if ( read(fd, (char *) &(infopntr->arraydim), sizeof(int))
                != sizeof(int) )
   {
      printf("cannot read `arraydim` parameter\n");
      (void) close(fd);
      return -1;
   }

   if ( read(fd, (char *) &(infopntr->datatype), sizeof(int))
                != sizeof(int) )
   {
      printf("cannot read `datatype` parameter\n");
      (void) close(fd);
      return -1;
   }


   for (i = 0; i < MAXDIM; i++)
   {
      int       nbytes;
      dimenInfo	*ptmpdimen;

      switch (i)
      {
         case 0:   ptmpdimen = &(infopntr->f3dim); break;
         case 1:   ptmpdimen = &(infopntr->f1dim); break;
         case 2:   ptmpdimen = &(infopntr->f2dim); break;
         default:  printf("internal error 1\n");
                   (void) close(fd);
                   return -1;
      }

      if ( read(fd, (char *) &(ptmpdimen->scdata), sizeof(sclrpar3D)) !=
                        sizeof(sclrpar3D) )
      {
         printf("cannot read 3D processing information\n");
         (void) close(fd);
         return -1;
      }

      if ( read(fd, (char *) &(ptmpdimen->parLPdata.sizeLP), sizeof(int))
			!= sizeof(int) )
      {
         printf("cannot read size of LP parameters");
         (void) close(fd);
         return -1;
      }

      if (ptmpdimen->parLPdata.sizeLP)
      {  
         lpinfo *lpdata;
 
 
         if ( read(fd, (char *) &(ptmpdimen->parLPdata.membytes),
			sizeof(int)) != sizeof(int) )
         {
            printf("cannot read memory size for LP");
            (void) close(fd);
            return -1;
         }

         if ( (ptmpdimen->parLPdata.parLP = (lpinfo *) malloc(
			(unsigned) (ptmpdimen->parLPdata.sizeLP *
			sizeof(lpinfo)) )) == NULL )
         {
            printf("cannot allocate memory for LP parameters");
            (void) close(fd);
            return -1;
         }
 
         nbytes = sizeof(lpinfo) - LP_LABEL_SIZE;
 
         for (j = 0; j < ptmpdimen->parLPdata.sizeLP; j++)
         {
            lpdata = ptmpdimen->parLPdata.parLP + j;
            if ( read(fd, (char *)lpdata, nbytes) != nbytes )
            {
               printf("cannot read LP information %d", i+1);
               (void) close(fd);
               return -1;
            }
         }
      }

      nbytes = sizeof(float) * (ptmpdimen->scdata.fn / 2);

      if ( (ptmpdimen->ptdata.wtv = (float *) malloc( (unsigned)nbytes ))
                == NULL )
      {
         printf("cannot allocate memory for weighting vector\n");
         (void) close(fd);
         return -1;
      }

      if ( read(fd, (char *) (ptmpdimen->ptdata.wtv), nbytes) != nbytes )
      {
         printf("cannot read weighting vector %d\n", i+1);
         (void) close(fd);
         return -1;
      }

      nbytes = sizeof(float) * ptmpdimen->scdata.fn;

      if ( (ptmpdimen->ptdata.phs = (float *) malloc( (unsigned)nbytes ))
                == NULL )
      {
         printf("cannot allocate memory for phasing vector\n");
         (void) close(fd);
         return -1;
      }

      if ( read(fd, (char *) (ptmpdimen->ptdata.phs), nbytes) != nbytes )
      {
         printf("cannot read phasing vector %d\n", i+1);
         (void) close(fd);
         return -1;
      }
   }

   (void) close(fd);


/**********************************************
*  Write out 3D information in ASCII format.  *
**********************************************/

   if ( (fdes = fopen("procdat.dec", "w")) == NULL )
   {
      printf("cannot open `procdat.dec` file\n");
      return -1;
   }

   fprintf(fdes, "FT3D version number = %1d\n", infopntr->vers);
   fprintf(fdes, "Array elements per (t1,t2) increment = %2d\n",
		infopntr->arraydim);
   fprintf(fdes, "Datatype for 3D data = %1d\n\n\n", infopntr->datatype);

   for (i = 0; i < MAXDIM; i++)
   {
      dimenInfo	tmpdimen;

      switch (i)           
      {
         case 0:   tmpdimen = infopntr->f3dim; break;
         case 1:   tmpdimen = infopntr->f2dim; break;
         case 2:   tmpdimen = infopntr->f1dim; break;
         default:  printf("internal error 2\n");
		   (void) fclose(fdes);
		   return -1;
      }

      fprintf(fdes, "Dimension %1d scalars:\n\n", MAXDIM - i);
      fprintf(fdes, "\t  proc = %d\n", tmpdimen.scdata.proc);
      fprintf(fdes, "\t    np = %d\n", tmpdimen.scdata.np);
      fprintf(fdes, "\t npadj = %d\n", tmpdimen.scdata.npadj);
      fprintf(fdes, "\t    fn = %d\n", tmpdimen.scdata.fn);
      fprintf(fdes, "\t lsfid = %d\n", tmpdimen.scdata.lsfid);
      fprintf(fdes, "\t   pwr = %d\n", tmpdimen.scdata.pwr);
      fprintf(fdes, "\t zflvl = %d\n", tmpdimen.scdata.zflvl);
      fprintf(fdes, "\t zfnum = %d\n", tmpdimen.scdata.zfnum);
      fprintf(fdes, "\t    rp = %f\n", tmpdimen.scdata.rp);
      fprintf(fdes, "\t    lp = %f\n", tmpdimen.scdata.lp);
      fprintf(fdes, "\t phfid = %f\n", tmpdimen.scdata.phfid);
      fprintf(fdes, "\t lsfrq = %f\n", tmpdimen.scdata.lsfrq);
      fprintf(fdes, "\twtflag = %d\n", tmpdimen.scdata.wtflag);
      fprintf(fdes, "\t ntype = %d\n", tmpdimen.scdata.ntype);
      fprintf(fdes, "\t dsply = %d\n\n", tmpdimen.scdata.dsply);

      if (tmpdimen.scdata.fiddc)
         fprintf(fdes, "\tTime-domain DC correction\n");
      if (tmpdimen.scdata.specdc)
         fprintf(fdes, "\tFrequency-domain DC correction\n\n");

      if ( (tmpdimen.scdata.reginfo.stpt != 0) &&
	   (tmpdimen.scdata.reginfo.endpt != tmpdimen.scdata.fn) )
      {
         fprintf(fdes, "\n\tRegion select:  zero start point = %d\n",
			tmpdimen.scdata.reginfo.stptzero);
         fprintf(fdes, "\tRegion select:  start point = %d\n",
			tmpdimen.scdata.reginfo.stpt);
         fprintf(fdes, "\tRegion select:  end point = %d\n",
			tmpdimen.scdata.reginfo.endpt);
         fprintf(fdes, "\tRegion select:  zero end point = %d\n\n",
			tmpdimen.scdata.reginfo.endptzero);
      }

      if (tmpdimen.parLPdata.sizeLP)
      {
         lpinfo	*lpdata;

         for (j = 0; j < tmpdimen.parLPdata.sizeLP; j++)
         {
            lpdata = tmpdimen.parLPdata.parLP + j;
            fprintf(fdes, "   LP parameter set %d:\n\n", j+1);
            fprintf(fdes, "\tLP.status   = %d\n", lpdata->status);
            fprintf(fdes, "\tLP.lpfilt   = %d\n", lpdata->ncfilt);
            fprintf(fdes, "\tLP.lpnupts  = %d\n", lpdata->ncupts);
            fprintf(fdes, "\tLP.strtlp   = %d\n", lpdata->startlppt);
            fprintf(fdes, "\tLP.lpext    = %d\n", lpdata->ncextpt);
            fprintf(fdes, "\tLP.strtext  = %d\n", lpdata->startextpt);
            fprintf(fdes, "\tLP.membytes = %d\n", lpdata->membytes);
            fprintf(fdes, "\tLP.lpprint  = %d\n", lpdata->printout);
            fprintf(fdes, "\tLP.lptrace  = %d\n", lpdata->trace);
            fprintf(fdes, "\tLP.index    = %d\n\n", lpdata->index);
         }
      }

      if (tmpdimen.scdata.sspar.zfsflag || tmpdimen.scdata.sspar.lfsflag)
      {
         fprintf(fdes, "\tSS.zfsflag   = %d\n",
			tmpdimen.scdata.sspar.zfsflag);
         fprintf(fdes, "\tSS.lfsflag   = %d\n",
			tmpdimen.scdata.sspar.lfsflag);
         fprintf(fdes, "\tSS.membytes  = %d\n",
			tmpdimen.scdata.sspar.membytes);
         fprintf(fdes, "\tSS.matsize   = %d\n",
			tmpdimen.scdata.sspar.matsize);
         fprintf(fdes, "\tSS.ntaps     = %d\n",
			tmpdimen.scdata.sspar.ntaps);
         fprintf(fdes, "\tSS.decfactor = %d\n",
			tmpdimen.scdata.sspar.decfactor);
         fprintf(fdes, "\tSS.option    = %d\n\n",
			tmpdimen.scdata.sspar.option);
      }

      fprintf(fdes, "\n");
   }


   fprintf(fdes, "\n\n");

   for (i = 0; i < MAXDIM; i++)
   {
      dimenInfo tmpdimen;

      switch (i)
      {
         case 0:   tmpdimen = infopntr->f3dim; break;
         case 1:   tmpdimen = infopntr->f2dim; break;
         case 2:   tmpdimen = infopntr->f1dim; break;
         default:  printf("internal error 3\n");
                   (void) fclose(fdes);
                   return -1;
      }

      fprintf(fdes, "Dimension %1d vectors:\n\n", MAXDIM - i);
      fprintf(fdes, "\t(Weighting)\n\n");

      for (j = 0; j < tmpdimen.scdata.np; )
      {
         fprintf(fdes, "\t%7.5f", *(tmpdimen.ptdata.wtv + j));
         j += 1;
         fprintf(fdes, "\t%7.5f", *(tmpdimen.ptdata.wtv + j));
         j += 1;
         fprintf(fdes, "\t%7.5f", *(tmpdimen.ptdata.wtv + j));
         j += 1;
         fprintf(fdes, "\t%7.5f\n", *(tmpdimen.ptdata.wtv + j));
         j += 1;
      }

      fprintf(fdes, "\n\t(Phasing)\n\n");

      for (j = 0; j < tmpdimen.scdata.fn; )
      { 
         fprintf(fdes, "\t%7.5f", *(tmpdimen.ptdata.phs + j));
         j += 1;
         fprintf(fdes, "\t%7.5f", *(tmpdimen.ptdata.phs + j));
         j += 1;
         fprintf(fdes, "\t%7.5f", *(tmpdimen.ptdata.phs + j));
         j += 1;
         fprintf(fdes, "\t%7.5f\n", *(tmpdimen.ptdata.phs + j));
         j += 1;
      }

      fprintf(fdes, "\n\n");
   }


   (void) fclose(fdes);
   return 0;
}

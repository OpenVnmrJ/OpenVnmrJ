/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
#include "constant.h"
#include "coef3d.h"
#include "lock3D.h"
#include "process.h"
#include "struct3d.h"

#define FT3D
#include "command.h"
#undef FT3D


extern void	Werrprintf(char *format, ...);
extern int removelock(char *filepath);
extern int createlock(char *filepath, int type);


/*---------------------------------------
|                                       |
|          copyinfofiles()/1            |
|                                       |
+--------------------------------------*/
int copyinfofiles(comInfo *pinfo)
{
   char	newinfodir[MAXPATHL],
	foutpath[MAXPATHL],
	finpath[MAXPATHL],
	syscmd[2*MAXPATHL];


   (void) strcpy(newinfodir, pinfo->datadirpath.sval);
   (void) strcat(newinfodir, "/info");

   if ( mkdir(newinfodir, 0777) )
   {
      if (errno != EEXIST)
      {
         Werrprintf("\ncopyinfofiles():  cannot create directory `%s`",
			newinfodir);
         return(ERROR);
      }
   }

   (void) strcpy(foutpath, newinfodir);
   (void) strcat(foutpath, "/procdat");
   (void) strcpy(finpath, pinfo->info3Ddirpath.sval);
   (void) strcat(finpath, "/procdat");

   sprintf(syscmd, "cp \"%s\" \"%s\"\n", finpath, foutpath);
   (void) system(syscmd);

   (void) strcpy(foutpath, newinfodir);
   (void) strcat(foutpath, "/procpar3d"); 
   (void) strcpy(finpath, pinfo->info3Ddirpath.sval); 
   (void) strcat(finpath, "/procpar3d");
 
   sprintf(syscmd, "cp \"%s\" \"%s\"\n", finpath, foutpath); 
   (void) system(syscmd);

   (void) strcpy(foutpath, newinfodir);
   (void) strcat(foutpath, "/coef");

   sprintf(syscmd, "cp \"%s\" \"%s\"\n", pinfo->coeffilepath.sval, foutpath);
   (void) system(syscmd);

   (void) strcpy(foutpath, newinfodir);
   (void) strcat(foutpath, "/auto");

   sprintf(syscmd, "cp \"%s\" \"%s\"\n", pinfo->autofilepath.sval, foutpath);
   (void) system(syscmd);

   return(COMPLETE);      
}


/*---------------------------------------
|                                       |
|            read3Dinfo()/1             |
|                                       |
+--------------------------------------*/
proc3DInfo *read3Dinfo(infodir)
char    *infodir;
{
   char		filepath[MAXPATHL];
   int          fd,
                i,
		j,
		k;
   proc3DInfo   *infopntr;
   int debug = 0;
   static char *dimname[] = {"f3", "f1", "f2"};


   (void) strcpy(filepath, infodir);
   (void) strcat(filepath, "/procdat");

   if ( createlock(infodir, INFO_LFILE) )
      return(NULL);

   if ( (infopntr = (proc3DInfo *) malloc( (unsigned)sizeof(proc3DInfo) ))
                == NULL )
   {
      Werrprintf("\nread3Dinfo():  cannot allocate memory for 3D information");
      return(NULL);
   }

   fd = open("procdat3d", O_RDONLY);
   if (fd >= 0){
       fprintf(stderr,"FT3D: using 'procdat3d' file in current directory\n");
   }else{
       if ( (fd = open(filepath, O_RDONLY, 0440)) < 0 )
       {
	   Werrprintf("\nread3Dinfo():  cannot open 3D information file %s",
		      filepath);
	   return(NULL);
       }
   }
 
   if ( read(fd,  (char *) (&(infopntr->vers)), sizeof(int)) != sizeof(int) )
   {
      Werrprintf("\nread3Dinfo():  cannot read `version` parameter");
      (void) close(fd);
      return(NULL);
   }
   if (debug){
       fprintf(stderr,"Version=%d\n", infopntr->vers);
   }
 
   if ( infopntr->vers != FT3D_VERSION )
   {
      Werrprintf("\nread3Dinfo():  version clash with 3D information file");
      (void) close(fd);
      return(NULL);
   }
 
   if ( read(fd, (char *) &(infopntr->arraydim), sizeof(int))
		!= sizeof(int) )
   {
      Werrprintf("\nread3Dinfo():  cannot read `arraydim` parameter");
      (void) close(fd);
      return(NULL);
   }
   if (debug){
       fprintf(stderr,"arraydim=%d\n", infopntr->arraydim);
   }

   if ( read(fd, (char *) &(infopntr->datatype), sizeof(int))
		!= sizeof(int) )
   {
      Werrprintf("\nread3Dinfo():  cannot read `datatype` parameter");
      (void) close(fd);
      return(NULL);
   }
   if (debug){
       fprintf(stderr,"datatype=%d\n", infopntr->datatype);
   }

   for (i = 0; i < MAXDIM; i++)
   {
      int       nbytes;
      dimenInfo *tmpdimen;

      if (debug){
	  fprintf(stderr,"%s:\n", dimname[i]);
      }
      switch (i)
      {
         case 0:   tmpdimen = &(infopntr->f3dim); break;
         case 1:   tmpdimen = &(infopntr->f1dim); break;
         case 2:   tmpdimen = &(infopntr->f2dim); break;
         default:  Werrprintf("\nread3Dinfo():  internal error 1");
                   (void) close(fd);
                   return(NULL);
      }
 
      if ( read(fd, (char *) &(tmpdimen->scdata), sizeof(sclrpar3D)) !=
			sizeof(sclrpar3D) )
      {
         Werrprintf("\nread3Dinfo():  cannot read 3D processing information");
         (void) close(fd);
         return(NULL);
      }
      if (debug){
	  fprintf(stderr,"  scdata\n");
	  fprintf(stderr,"    reginfo:\n");
	  fprintf(stderr,"      stptzero=%d, stpt=%d, endpt=%d, endptzero=%d\n",
		  tmpdimen->scdata.reginfo.stptzero,
		  tmpdimen->scdata.reginfo.stpt,
		  tmpdimen->scdata.reginfo.endpt,
		  tmpdimen->scdata.reginfo.endptzero);
	  fprintf(stderr,"    sspar:\n");
          k = tmpdimen->scdata.sspar.sslsfrqsize;
          if (k > SSLSFRQSIZE_MAX) k = SSLSFRQSIZE_MAX;
          if (k < 1) k = 1;
          for (j=0; j<k; j++)
              if (fabs(tmpdimen->scdata.sspar.sslsfrq[j]) > 0.0001)
                  fprintf(stderr,"      sslsfrq[%d]=%g\n", 
                                j, tmpdimen->scdata.sspar.sslsfrq[j]);
	  fprintf(stderr,"      membytes=%d, zfsflag=%d, lfsflag=%d\n",
		  tmpdimen->scdata.sspar.membytes,
		  tmpdimen->scdata.sspar.zfsflag,
		  tmpdimen->scdata.sspar.lfsflag);
	  fprintf(stderr,"      decfactor=%d,  ntaps=%d, matsize=%d\n",
		  tmpdimen->scdata.sspar.decfactor,
		  tmpdimen->scdata.sspar.ntaps,
		  tmpdimen->scdata.sspar.matsize);
	  fprintf(stderr,"    phfid=%g, lsfrq=%g, rp=%g, lp=%g, lpval=%g\n",
		  tmpdimen->scdata.phfid,
		  tmpdimen->scdata.lsfrq,
		  tmpdimen->scdata.rp,
		  tmpdimen->scdata.lp,
		  tmpdimen->scdata.lpval);
	  fprintf(stderr,"    np=%d, npadj=%d, fn=%d, lsfid=%d\n",
		  tmpdimen->scdata.np,
		  tmpdimen->scdata.npadj,
		  tmpdimen->scdata.fn,
		  tmpdimen->scdata.lsfid);
	  fprintf(stderr,"    pwr=%d, zflvl=%d, zfnum=%d, proc=%d\n",
		  tmpdimen->scdata.pwr,
		  tmpdimen->scdata.zflvl,
		  tmpdimen->scdata.zfnum,
		  tmpdimen->scdata.proc);
	  fprintf(stderr,
		  "  ntype=%d,   fiddc=%d, specdc=%d, wtflag=%d, dsply=%d\n",
		  tmpdimen->scdata.ntype,
		  tmpdimen->scdata.fiddc,
		  tmpdimen->scdata.specdc,
		  tmpdimen->scdata.wtflag,
		  tmpdimen->scdata.dsply);
      }

      if ( read(fd, (char *) &(tmpdimen->parLPdata.sizeLP), sizeof(int))
			!= sizeof(int) )
      {
         Werrprintf("\nread3Dinfo():  cannot read size of LP parameters");
         (void) close(fd);
         return(NULL);
      }
      if (debug){
	  fprintf(stderr,"  parLPdata:\n");
	  fprintf(stderr,"    sizeLP=%d\n", tmpdimen->parLPdata.sizeLP);
      }

      if (tmpdimen->parLPdata.sizeLP)
      {
         lpinfo	*lpdata;


         if ( read(fd, (char *) &(tmpdimen->parLPdata.membytes),
			sizeof(int)) != sizeof(int) )
         {
            Werrprintf("\nread3Dinfo():  cannot read memory size for LP");
            (void) close(fd);
            return(NULL);
         }
	 if (debug){
	     fprintf(stderr,"    membytes=%d\n", tmpdimen->parLPdata.membytes);
	 }

         if ( (tmpdimen->parLPdata.parLP = (lpinfo *) malloc( (unsigned)
			(tmpdimen->parLPdata.sizeLP * sizeof(lpinfo)) ))
			== NULL )
         {
            Werrprintf("\nread3Dinfo():  cannot allocate memory for LP");
            (void) close(fd);
            return(NULL);   
         }

         nbytes = sizeof(lpinfo) - LP_LABEL_SIZE;

         for (j = 0; j < tmpdimen->parLPdata.sizeLP; j++)
         {
            lpdata = tmpdimen->parLPdata.parLP + j;
            if ( read(fd, (char *)lpdata, nbytes) != nbytes )
            {
               Werrprintf("\nread3Dinfo():  cannot read LP information %d",
				i+1);
               (void) close(fd);
               return(NULL);
            }
	    if (debug){
		fprintf(stderr,"    parLP[%d]:\n", j);
		fprintf(stderr,"      lstrace=%g\n", lpdata->lstrace);
		// fprintf(stderr,"      lstrace=%g\n, *lppntr=(%g, %g)\n",
		// 	lpdata->lstrace, lpdata->lppntr[0], lpdata->lppntr[1]);
		/* MORE ... */
	    }
         }
      }
 
      nbytes = sizeof(float) * (tmpdimen->scdata.fn / 2);
 
      if ( (tmpdimen->ptdata.wtv = (float *) malloc( (unsigned)nbytes ))
		== NULL )
      {
         Werrprintf("\nread3Dinfo(): insufficient memory for weighting vector");
         (void) close(fd);
         return(NULL);
      }
 
      if ( read(fd, (char *) (tmpdimen->ptdata.wtv), nbytes) != nbytes )
      {
         Werrprintf("\nread3Dinfo():  cannot read weighting vector %d", i+1);
         (void) close(fd);
         return(NULL);
      }

      nbytes = sizeof(float) * tmpdimen->scdata.fn;

      if ( (tmpdimen->ptdata.phs = (float *) malloc( (unsigned)nbytes ))
		== NULL )
      {
         Werrprintf("\nread3Dinfo():  insufficient memory for phasing vector");
         (void) close(fd);
         return(NULL);
      }
 
      if ( read(fd, (char *) (tmpdimen->ptdata.phs), nbytes) != nbytes )
      {
         Werrprintf("\nread3Dinfo():  cannot read phasing vector %d", i+1);
         (void) close(fd);
         return(NULL);
      }
   }

   if ( removelock(infodir) )
   {
      (void) close(fd);
      return(NULL);
   }

   (void) close(fd);
   return(infopntr);
}      
 
 
/*---------------------------------------
|                                       |
|            check3Dinfo()/1            |
|                                       |
+--------------------------------------*/
int check3Dinfo(proc3DInfo *infopntr)
{
   int          i;
   dimenInfo    *dinfo;
 
 
   for (i = 0; i < MAXDIM; i++)
   {
      switch (i)
      {
         case 0:   dinfo = &(infopntr->f3dim); break;
         case 1:   dinfo = &(infopntr->f1dim); break;
         case 2:   dinfo = &(infopntr->f2dim); break;
         default:  break;
      }
 
      if ( (dinfo->scdata.np < MIN_NP) || (dinfo->scdata.np > MAX_NP) )
      {
         Werrprintf("\ncheck3Dinfo():  `np` out of bounds for dimension %d",
                        MAXDIM - i);
         return(ERROR);
      }
      else if ( (dinfo->scdata.fn < MIN_FN) || (dinfo->scdata.fn > MAX_FN) )
      {
         Werrprintf("\ncheck3Dinfo():  `fn` out of bounds for dimension %d",
                        MAXDIM - i);
         return(ERROR);
      }
   }
 
   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|             readcoefs()/2             |
|                                       |
+--------------------------------------*/
coef3D *readcoefs(filepath, arraydim)
char    filepath[MAXPATHL];
int     arraydim;
{
   int          i = 1,
		j,
                ncoefs;
   float        *coef;
   coef3D	*coef3d;
   FILE         *fd,
                *fopen();
 
 
   ncoefs = 8 * (arraydim + 1);
   if ( (coef3d = (coef3D *) malloc( (unsigned) sizeof(coef3D) )) == NULL )
   {
      Werrprintf("\nreadcoefs():  cannot allocate memory for coefficients");
      return(NULL);
   }

   if ( (coef = (float *) malloc( (unsigned) ((ncoefs + 1)*sizeof(float)) ))
	  == NULL )
   {
      Werrprintf("\nreadcoefs():  cannot allocate memory for coefficients");
      return(NULL);
   }

   coef3d->ncoefs = ncoefs;
   coef3d->f3t2.coefval = coef;
   coef3d->f2t1.coefval = coef + COMPLEX*HYPERCOMPLEX*arraydim;
 
   if ( (fd = fopen(filepath, "r")) == NULL )
   {
      Werrprintf("\nreadcoefs():  cannot open COEFFICIENT file %s", filepath);
      return(NULL);
   }
 

   while ( fscanf(fd, "%f", coef++) != EOF )
   {
      if ( i++ > ncoefs )
      {
         Werrprintf("\nreadcoefs():  coefficient mismatch in COEFFICIENT file");
         return(NULL);
      }
   }
 
   (void) fclose(fd);

 
/********************************************
*  Now check to insure which points in the  *
*  hypercomplex t2 and t1 interferograms    *
*  are associated with at least one non-    *
*  zero coefficient.                        *
********************************************/

   coef = coef3d->f3t2.coefval;

   for (i = 0; i < HYPERCOMPLEX; i++)
   {
      int	*tmpflag;

      switch (i)
      {
         case 0:   tmpflag = &(coef3d->f3t2.rr_nzero); break;
         case 1:   tmpflag = &(coef3d->f3t2.ir_nzero); break;
         case 2:   tmpflag = &(coef3d->f3t2.ri_nzero); break;
         case 3:   tmpflag = &(coef3d->f3t2.ii_nzero); break;
      }

      *tmpflag = FALSE;

      for (j = 0; j < (COMPLEX*arraydim); j++)
      {
         if ( fabs(*coef++) > MIN_COEFVAL )
         {
            *tmpflag = TRUE;
            coef += COMPLEX*arraydim - (j + 1);
            j = COMPLEX*arraydim;
         }
      }
   }

   return(coef3d);
}

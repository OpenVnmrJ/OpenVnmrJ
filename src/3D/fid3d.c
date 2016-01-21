/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <strings.h>
#include <sys/file.h>
#include <math.h>
#include <errno.h>


#define MAXPATHL	128
#define MAXSTR		256
#define ERROR		-1
#define COMPLETE	0
#define FALSE		0
#define TRUE		1
#define FISCONV		16384.0
#define FILCONV		65536.0
#define MAXDIM		3
#define COMPLEX		2

#define FID_FILE	64
#define FHEAD		0
#define BHEAD		1
#define S_DATA		0x1
#define S_32		0x4
#define OLD_S_COMPLEX	0x40

#define SELECT		0
#define SET		1
#define INPUT_FILE	10
#define OUTPUT_DIR	11
#define SNGL_PREC	12
#define OVER_WRITE	13


struct _comInfo
{
   char	inputfilepath[MAXPATHL];
   char	outputdirpath[MAXPATHL];
   int	overwrite;
   int	dpflag;
};

typedef struct _comInfo comInfo;

struct _dimInfo
{
   int		np;	/* number of increments or complex points */
   int		*phase;	/* pointer to phase elements		  */
   float	sw;	/* spectral width			  */
   float	w0;	/* carrier frequency (in Hz)		  */
};

typedef struct _dimInfo dimInfo;

struct _spinInfo
{
   int		nspins;
   float	*freq3;
   float	*freq2;
   float	*freq1;
   float	*relax;
};

typedef struct _spinInfo spinInfo;

struct _simInfo
{
   int		arraydim;
   float	*t3data;
   float	*data;
   spinInfo	spinsys;
   dimInfo	t3dim;
   dimInfo	t2dim;
   dimInfo	t1dim;
};

typedef struct _simInfo simInfo;

struct _fileheader
{
   long         nblocks;
   long         ntraces;
   long         np;
   long         ebytes;
   long         tbytes;
   long         bbytes;
   short        vers_id;
   short        status;
   long         nbheaders;
};

typedef struct _fileheader fileheader;

struct _blockheader
{
   short        scale;
   short        status;
   short        index;
   short        mode;
   long         ctcount;
   float        lpval;
   float        rpval;
   float        lvl;
   float        tlt;
};

typedef struct _blockheader blockheader;

extern char	*malloc();
extern int	errno;
extern void	printf();


/*---------------------------------------
|                                       |
|            readselect()/1             |
|                                       |
+--------------------------------------*/
int readselect(argstr)
char    *argstr;
{
   if (*argstr++ == '-')
   {
      switch (*argstr)
      {
         case 'f':   return(OVER_WRITE);
         case 'i':   return(INPUT_FILE);
         case 'o':   return(OUTPUT_DIR);
         case 's':   return(SNGL_PREC);
         default:    printf("readselect():  invalid option\n");
                     return(ERROR);
      }
   }
   else
   {
      printf("readselect():  improper input format\n");
      return(ERROR);
   }
}


/*---------------------------------------
|                                       |
|            setselect()/3              |
|                                       |
+--------------------------------------*/
int setselect(argstr, selection, pinfo)
char    *argstr;
int     selection;
comInfo *pinfo;
{
   switch (selection)
   {
      case OVER_WRITE:  pinfo->overwrite = TRUE;
                        break;
      case INPUT_FILE:  (void) strcpy(pinfo->inputfilepath, argstr);
                        break;
      case OUTPUT_DIR:  (void) strcpy(pinfo->outputdirpath, argstr);
                        break;
      case SNGL_PREC:   pinfo->dpflag = FALSE;
			break;
      default:          break;
   }

   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|            parseinput()/2             |
|                                       |
+--------------------------------------*/
comInfo *parseinput(argc, argv)
char    *argv[];
int     argc;
{
   int          argno = 1,
                mode = SELECT,
                selection;
   comInfo      *pinfo;
                            
                            
   if ( (pinfo = (comInfo *) malloc( (unsigned) sizeof(comInfo) ))
                == NULL )
   {
      printf("parseinput():  cannot allocate memory for command line data\n");
      return(NULL);
   }

   pinfo->overwrite = FALSE;
   pinfo->dpflag = TRUE;
   (void) strcpy(pinfo->inputfilepath, "");
   (void) strcpy(pinfo->outputdirpath, "");

   while (argno < argc)               
   {
      switch (mode)
      {
         case SELECT:   if ( (selection = readselect(argv[argno])) == ERROR )
                           return(NULL);
			if ( (selection != OVER_WRITE) &&
			     (selection != SNGL_PREC) )
			{
			   argno++;
			}
			mode = SET;
                        break;
         case SET:      if ( setselect(argv[argno++], selection, pinfo) )
                           return(NULL);
                        mode = SELECT;
                        break;
         default:       break;
      }
   }

   if (mode == SET)
   {
      printf("parseinput():  improper input format\n");
      return(NULL);
   }

   if ( strcmp(pinfo->inputfilepath, "") == 0 )
   {
      printf("parseinput():  input file path undefined\n");
      return(NULL);
   }

   if ( strcmp(pinfo->outputdirpath, "") == 0 ) 
   { 
      printf("parseinput():  output directory path undefined\n"); 
      return(NULL); 
   }
   else
   {
      (void) strcat(pinfo->outputdirpath, ".fid");
      if (mkdir(pinfo->outputdirpath, 0777))
      {
         if (errno == EEXIST)
         {
            if (!pinfo->overwrite)
            {
               printf("parseinput():  directory `%s` already exists\n",
                           pinfo->outputdirpath);
               return(NULL);
            }
         }
         else
         {
            printf("parseinput():  cannot create directory `%s`\n",
			 pinfo->outputdirpath);
            return(NULL);
         }
      }
   }

   return(pinfo);
}


/*---------------------------------------
|                                       |
|            openfiles()/2              |
|                                       |
+--------------------------------------*/
FILE *openfiles(pinfo, outfd)
int     *outfd;
comInfo *pinfo;
{
   char	outputfilepath[MAXPATHL];
   FILE	*infdes,
	*fopen();


   (void) strcpy(outputfilepath, pinfo->outputdirpath);
   (void) strcat(outputfilepath, "/fid");
   if ( (*outfd = open(outputfilepath, (O_RDWR|O_CREAT|O_TRUNC),
		0666)) < 0 )
   {
      printf("openfiles():  cannot open output file `%s`\n",
		outputfilepath);
      return(NULL);
   }

   if ( (infdes = fopen(pinfo->inputfilepath, "r")) == NULL )
   {
      (void) close(*outfd);
      printf("openfiles():  cannot open input file `%s`\n",
		pinfo->inputfilepath);
      return(NULL);
   }

   return(infdes);
}


/*---------------------------------------
|                                       |
|            readinput()/1              |
|                                       |
|  The order of the information in the	|
|  ASCII file is as follows:		|
|					|
|					|
|	    nspins			|
|	    arraydim			|
|					|
|   	    np/2			|
|	    sw				|
|	    w0				|
|	    *freq(nspins)		|
|					|
|	    ni				|
|	    sw1				|
|	    *phase(arraydim)		|
|	    w0				|
|	    *freq(nspins)		|
|					|
|	    ni2				|
|	    sw2				|
|	    *phase2(arraydim)		|
|	    w0				|
|	    *freq(nspins)		|
|					|
|	    *relax(nspins)		|
|					|
+--------------------------------------*/
simInfo	*readinput(fdes)
FILE	*fdes;
{
   int		i,
		j;
   float	*wspace,
		*freq,
		*relax;
   simInfo	*sinfo;
   dimInfo	*dinfo;


   if ( (sinfo = (simInfo *) malloc( (unsigned)sizeof(simInfo) ))
		== NULL )
   {
      printf("readinput():  insufficient memory for simulation structure\n");
      return(NULL);
   }

   if ( fscanf(fdes, "%d", &(sinfo->spinsys.nspins)) == EOF )
   {
      printf("readinput():  `nspins` is undefined\n");
      return(NULL);
   }

   if ( fscanf(fdes, "%d", &(sinfo->arraydim)) == EOF )
   {
      printf("readinput():  `arraydim` is undefined\n");
      return(NULL);
   }

   if ( (wspace = (float *) malloc( (unsigned) ( (4*sinfo->spinsys.nspins
		+ 2*sinfo->arraydim)*sizeof(float) ) )) == NULL )
   {
      printf("readinput():  insufficient memory for spin system data\n");
      return(NULL);
   }

   sinfo->spinsys.freq3 = wspace;
   sinfo->spinsys.freq2 = sinfo->spinsys.freq3 + sinfo->spinsys.nspins;
   sinfo->spinsys.freq1 = sinfo->spinsys.freq2 + sinfo->spinsys.nspins;
   sinfo->spinsys.relax = sinfo->spinsys.freq1 + sinfo->spinsys.nspins;
   sinfo->t3dim.phase = NULL;
   sinfo->t2dim.phase = (int *)sinfo->spinsys.relax + sinfo->spinsys.nspins;
   sinfo->t1dim.phase = sinfo->t2dim.phase + sinfo->arraydim;


   for (i = 0; i < MAXDIM; i++)
   {
      switch (i)
      {
         case 0:   freq = sinfo->spinsys.freq3;
		   dinfo = &(sinfo->t3dim);
		   break;
         case 1:   freq = sinfo->spinsys.freq2;
		   dinfo = &(sinfo->t2dim);
		   break;
         case 2:   freq = sinfo->spinsys.freq1;
		   dinfo = &(sinfo->t1dim);
		   break;
         default:  break;
      }

      if ( fscanf(fdes, "%d", &(dinfo->np)) == EOF )
      {
         printf("readinput():  `np` for dimension %d is undefined\n",
		   MAXDIM - i);
         return(NULL);
      }

      if ( fscanf(fdes, "%f", &(dinfo->sw)) == EOF )
      {
         printf("readinput():  `sw` for dimension %d is undefined\n",
		   MAXDIM - i);
         return(NULL);
      }

      if (i)
      {
         int	*tmpphase;


         tmpphase = dinfo->phase;

         for (j = 0; j < sinfo->arraydim; j++)
         {
            if ( fscanf(fdes, "%d", tmpphase) == EOF )
            {
               printf("readinput():  `phase` for dimension %d is undefined\n",
			   MAXDIM - i);
               return(NULL);
            }

            if ( (*tmpphase < 1) || (*tmpphase > 3) )
            {
               printf("readinput():  `phase` out of range for dimension %d\n",
			   MAXDIM - i);
               return(NULL);
            }

            tmpphase += 1;
         }
      }

      if ( fscanf(fdes, "%f", &(dinfo->w0)) == EOF ) 
      { 
         printf("readinput():  `w0` for dimension %d is undefined\n",
                   MAXDIM - i);
         return(NULL); 
      }

      for (j = 0; j < sinfo->spinsys.nspins; j++)
      {
         if ( fscanf(fdes, "%f", freq++) == EOF )
         {
            printf("readinput():  `freq's` for dimension %d are undefined\n",
		      MAXDIM - i);
            return(NULL);
         }
      }
   }


   relax = sinfo->spinsys.relax;

   for (i = 0; i < sinfo->spinsys.nspins; i++)
   {
      if ( fscanf(fdes, "%f", relax++) == EOF )
      {
         printf("readinput():  `relax's` for dimension %d are undefined\n",
		   MAXDIM - i);
         return(NULL);
      }
   }

   
   if ( (sinfo->t3data = (float *) malloc( (unsigned)
		( (1 + sinfo->spinsys.nspins)*COMPLEX*sinfo->t3dim.np*
		  sizeof(float) ) )) == NULL )
   {
      printf("readinput():  insufficient memory for data\n");
      return(NULL);
   }

   sinfo->data = sinfo->t3data + (sinfo->spinsys.nspins *
			COMPLEX * sinfo->t3dim.np);
   return(sinfo);
}


/*---------------------------------------
|                                       |
|            writeheader()/3		|
|					|
+--------------------------------------*/
int writeheader(fd, header, type)
char	*header;
int	fd,
	type;
{
   int	nbytes;


   switch (type)
   {
      case FHEAD:  nbytes = sizeof(fileheader);  break;
      case BHEAD:  nbytes = sizeof(blockheader); break;
      default:     break;
   }

   if ( write(fd, header, nbytes) != nbytes )
   {
      printf("writeheader():  error on writing header to output file\n");
      return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	      writefid()/5		|
|					|
+--------------------------------------*/
int writefid(fd, data, nbytes, dpflag, nspins)
char	*data;
int	fd,
	nbytes,
	dpflag,
	nspins;
{
   int		nwords;
   float	*inp,
		conv;
   register int	i;


   inp = (float *)data;

   if (dpflag)
   {
      int	*outp;

      outp = (int *)inp;
      nwords = nbytes / sizeof(int);
      conv = FILCONV / (float)nspins;

      for (i = 0; i < nwords; i++)
         *outp++ = (int) ( (*inp++) * conv );
   }
   else
   {
      short	*outp;

      outp = (short *)inp;
      nwords = nbytes / sizeof(short);
      conv = FISCONV / (float)nspins;

      for (i = 0; i < nwords; i++)
         *outp++ = (short) ( (*inp++) * conv );
   }
 

   if ( write(fd, data, nbytes) != nbytes )
   {
      printf("writefid():  error on writing FID data to output file\n");
      return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|	      writetext()/1		|
|					|
+--------------------------------------*/
int writetext(pinfo)
comInfo	*pinfo;
{
   char	outputfilepath[MAXPATHL],
	textcopy[MAXPATHL];


   (void) strcpy(outputfilepath, pinfo->outputdirpath);
   (void) strcat(outputfilepath, "/text");
   (void) sprintf(textcopy, "cp \"%s\" \"%s\"\n", pinfo->inputfilepath,
		outputfilepath);
   (void) system(textcopy);

   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|	     writeprocpar()/2		|
|					|
+--------------------------------------*/
int writeprocpar(pinfo, sinfo)
comInfo	*pinfo;
simInfo	*sinfo;
{
   char		outputfilepath[MAXPATHL],
		inputfilepath[MAXPATHL],
		parname[MAXSTR],
		*vnmruser;
   int		i,
		c,
		tmp_int,
		tmp_int2,
		stat1 = TRUE,
		stat2 = TRUE;
   float	tmp_float;
   FILE		*rid,
		*wid,
		*fopen();
   extern char	*getenv();


   if ( (vnmruser = getenv("vnmruser")) == NULL )
   {
      printf("writeprocpar():  cannot get `$vnmruser` variable\n");
      return(ERROR);
   }

   (void) strcpy(inputfilepath, vnmruser);
   (void) strcat(inputfilepath, "/parlib/fid3d.par/procpar");
   (void) strcpy(outputfilepath, pinfo->outputdirpath); 
   (void) strcat(outputfilepath, "/procpar");

   if ( (rid = fopen(inputfilepath, "r")) == NULL )
   {
      printf("writeprocpar():  cannot open file `%s`\n", inputfilepath);
      return(ERROR);
   }

   if ( (wid = fopen(outputfilepath, "w")) == NULL )
   {
      printf("writeprocpar():  cannot open file `%s`\n", outputfilepath);
      (void) fclose(rid);
      return(ERROR);
   }


   while (stat2)
   {
      c = getc(rid);
      if (c == EOF)
      {
         stat2 = FALSE;
         break;
      }

      (void) ungetc(c, rid);
      fscanf(rid, "%s", parname);
      (void) fprintf(wid, "%s", parname);
      while ( (c = getc(rid)) != '\n' )
         (void) putc(c, wid);
      (void) putc(c, wid);
 
      if (strcmp(parname, "at") == 0)
      {
         fscanf(rid, "%d %f", &tmp_int, &tmp_float);
         tmp_float = sinfo->t3dim.np / sinfo->t3dim.sw;
         (void) fprintf(wid, "%d %f \n", tmp_int, tmp_float);
         fscanf(rid, "%d", &tmp_int);
         (void) fprintf(wid, "%d \n", tmp_int);
      }
      else if (strcmp(parname, "sw") == 0)
      {
         fscanf(rid, "%d %f", &tmp_int, &tmp_float);
         (void) fprintf(wid, "%d %f \n", tmp_int, sinfo->t3dim.sw);
         fscanf(rid, "%d", &tmp_int);
         (void) fprintf(wid, "%d \n", tmp_int);
      }
      else if (strcmp(parname, "sw1") == 0)
      {
         fscanf(rid, "%d %f", &tmp_int, &tmp_float);
         (void) fprintf(wid, "%d %f \n", tmp_int, sinfo->t2dim.sw);
         fscanf(rid, "%d", &tmp_int);
         (void) fprintf(wid, "%d \n", tmp_int);
      }
      else if (strcmp(parname, "sw2") == 0)
      {
         fscanf(rid, "%d %f", &tmp_int, &tmp_float);
         (void) fprintf(wid, "%d %f \n", tmp_int, sinfo->t1dim.sw);
         fscanf(rid, "%d", &tmp_int);
         (void) fprintf(wid, "%d \n", tmp_int);
      }
      else if (strcmp(parname, "np") == 0)
      {
         fscanf(rid, "%d %d", &tmp_int, &tmp_int2);
         (void) fprintf(wid, "%d %d \n", tmp_int, COMPLEX*sinfo->t3dim.np);
         fscanf(rid, "%d", &tmp_int);
         (void) fprintf(wid, "%d \n", tmp_int);
      }
      else if (strcmp(parname, "ni") == 0)
      {
         fscanf(rid, "%d %d", &tmp_int, &tmp_int2);
         (void) fprintf(wid, "%d %d \n", tmp_int, sinfo->t2dim.np);
         fscanf(rid, "%d", &tmp_int);
         (void) fprintf(wid, "%d \n", tmp_int);
      }
      else if (strcmp(parname, "ni2") == 0)
      {
         fscanf(rid, "%d %d", &tmp_int, &tmp_int2);
         (void) fprintf(wid, "%d %d \n", tmp_int, sinfo->t1dim.np);
         fscanf(rid, "%d", &tmp_int);
         (void) fprintf(wid, "%d \n", tmp_int);
      }
      else if (strcmp(parname, "arraydim") == 0)
      {
         fscanf(rid, "%d %d", &tmp_int, &tmp_int2);
         (void) fprintf(wid, "%d %d \n", tmp_int,
		  sinfo->arraydim*sinfo->t1dim.np*sinfo->t2dim.np);
         fscanf(rid, "%d", &tmp_int);
         (void) fprintf(wid, "%d \n", tmp_int);
      }
      else if (strcmp(parname, "phase") == 0)
      {
         fscanf(rid, "%d", &tmp_int);
         (void) fprintf(wid, "%d", sinfo->arraydim);

         for (i = 0; i < sinfo->arraydim; i++)
            (void) fprintf(wid, " %d", i+1);

         (void) fprintf(wid, " \n");
         while ( (c = getc(rid)) != '\n' );
         fscanf(rid, "%d", &tmp_int);
         (void) fprintf(wid, "%d \n", tmp_int);
      }
      else
      {
         stat1 = TRUE;

         while (stat1)
         {
            c = getc(rid);
            if (c == EOF)
            {
               stat2 = FALSE;
               stat1 = FALSE;
            }
            else
            {
               (void) putc(c, wid);
               if (c == '\n')
                  stat1 = FALSE;
            }
         }

         if (stat2)
            stat1 = TRUE;

         while (stat1)
         {
            c = getc(rid);
            if (c == EOF)
            {
               stat2 = FALSE;
               stat1 = FALSE;
            }
            else
            {
               (void) putc(c, wid);
               if (c == '\n')
                  stat1 = FALSE;
            }
         }
      }
   }

   (void) fclose(rid);
   (void) fclose(wid);

   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|	       calcfid()/4		|
|					|
+--------------------------------------*/
void calcfid(t1inc, t2inc, arrayelem, sinfo)
int	t1inc,
	t2inc,
	arrayelem;
simInfo	*sinfo;
{
   int			i,
			phase1,
			phase2;
   float		w01,
			w02,
			tval1,
			tval2,
			arg1,
			arg2;
   register float	dval,
			t1t2data,
			*freq1,
			*freq2,
			*relax;
   void			combinedata();


   relax = sinfo->spinsys.relax;

   freq1 = sinfo->spinsys.freq1;
   w01 = sinfo->t1dim.w0;
   phase1 = *(sinfo->t1dim.phase + arrayelem);
   tval1 = (float)t1inc / sinfo->t1dim.sw;
   arg1 = 2*M_PI*tval1;

   freq2 = sinfo->spinsys.freq2;
   w02 = sinfo->t2dim.w0;
   phase2 = *(sinfo->t2dim.phase + arrayelem);
   tval2 = (float)t2inc / sinfo->t2dim.sw;
   arg2 = 2*M_PI*tval2;

   for (i = 0; i < sinfo->spinsys.nspins; i++)
   {
      switch (phase1)
      {
         case 1:   dval = cos( arg1*((*freq1) - w01) );
		   break;
         case 2:   dval = sin( arg1*((*freq1) - w01) );
		   break;
         case 3:   if (t1inc % 2)
		   {
		      dval = sin( arg1*((*freq1) - w01) );
		   }
		   else
		   {
		      dval = cos( arg1*((*freq1) - w01) );
		   }
		   dval = ( ((t1inc/2) % 2) ? (-1)*dval : dval );
		   break;
         default:  break;
      }

      t1t2data = dval * exp( (-1)*tval1 / (*relax) );

      switch (phase2)
      {
         case 1:   dval = cos( arg2*((*freq2) - w02) );
		   break;
         case 2:   dval = sin( arg2*((*freq2) - w02) );
		   break;
         case 3:   if (t2inc % 2)
		   {
		      dval = sin( arg2*((*freq2) - w02) );
		   }
		   else
		   {
		      dval = cos( arg2*((*freq2) - w02) );
		   }
		   dval = ( ((t2inc/2) % 2) ? (-1)*dval : dval );
		   break;
         default:  break;
      }

      t1t2data *= ( dval * exp((-1)*tval2 / (*relax)) );
      combinedata(sinfo, t1t2data, i);

      relax += 1;
      freq1 += 1;
      freq2 += 1;
   }
}


/*---------------------------------------
|                                       |
|	     combinedata()/3		|
|					|
+--------------------------------------*/
void combinedata(sinfo, t1t2mult, spin_no)
int	spin_no;
float	t1t2mult;
simInfo	*sinfo;
{
   register int		i;
   register float	*inp,
			*outp;


   inp = sinfo->t3data + spin_no*COMPLEX*sinfo->t3dim.np;
   outp = sinfo->data;

   if (spin_no)
   {
      for (i = 0; i < (COMPLEX*sinfo->t3dim.np); i++)
         (*outp++) += ( t1t2mult * (*inp++) );
   }
   else
   {
      for (i = 0; i < (COMPLEX*sinfo->t3dim.np); i++)
         (*outp++) = ( t1t2mult * (*inp++) );
   }
}


/*---------------------------------------
|                                       |
|	      calct3fids()/1		|
|					|
+--------------------------------------*/
void calct3fids(sinfo)
simInfo	*sinfo;
{
   int			i,
			j;
   float		w0,
			dval,
			sw3,
			arg;
   register float	arg1,
			tval,
			*outp,
			*freq,
			*relax;


   freq = sinfo->spinsys.freq3;
   relax = sinfo->spinsys.relax;
   outp = sinfo->t3data;
   w0 = sinfo->t3dim.w0;
   sw3 = sinfo->t3dim.sw;

   for (i = 0; i < sinfo->spinsys.nspins; i++)
   {
      arg = 2*M_PI*( (*freq++) - w0 );
      dval = (-1.0) / (*relax++);

      for (j = 0; j < sinfo->t3dim.np; j++)
      {
         tval = (float)j / sw3;
         arg1 = tval * arg;
         *outp = cos(arg1);
         *(outp + 1) = sin(arg1);
         arg1 = exp(dval*tval);
         (*outp++) *= arg1;
         (*outp++) *= arg1;
      }
   }
}


/*---------------------------------------
|					|
|		 main()/2		|
|					|
+--------------------------------------*/
main(argc, argv)
char	*argv[];
int	argc;
{
   int		i,
		j,
		k,
		outfd;
   void		calct3fids(),
		calcfid();
   FILE		*infdes,
		*openfiles();
   comInfo	*pinfo,
		*parseinput();
   simInfo	*sinfo,
		*readinput();
   fileheader	fidfilehead;
   blockheader	fidblockhead;


#ifdef DBXTOOL           
   sleep(40);
#endif DBXTOOL

/* Parse command line information */
   if ( (pinfo = parseinput(argc, argv)) == NULL )
      return ERROR;


/* Open ASCII input file and binary output file */
   if ( (infdes = openfiles(pinfo, &outfd)) == NULL )
      return ERROR;


/* Read in the simulation information */
   if ( (sinfo = readinput(infdes)) == NULL )
   {
      (void) fclose(infdes);
      (void) close(outfd);
      return ERROR;
   }

   (void) fclose(infdes);


/* Set file header structure elements and write to disk */
   fidfilehead.vers_id = FID_FILE;
   fidfilehead.nbheaders = 1;

   fidfilehead.status = ( S_DATA|OLD_S_COMPLEX );
   if (pinfo->dpflag)
      fidfilehead.status |= S_32;

   fidfilehead.nblocks = sinfo->arraydim * sinfo->t2dim.np * sinfo->t1dim.np;
   fidfilehead.np = COMPLEX*sinfo->t3dim.np;
   fidfilehead.ntraces = 1;

   fidfilehead.ebytes = ( (fidfilehead.status & S_32) ? 4 : 2 );
   fidfilehead.tbytes = fidfilehead.ebytes * fidfilehead.np;
   fidfilehead.bbytes = fidfilehead.nbheaders*sizeof(blockheader) +
			   fidfilehead.ntraces*fidfilehead.tbytes;

   if ( writeheader(outfd, (char *)(&fidfilehead), FHEAD) )
      return ERROR;


/* Initialize block header structure elements */
   fidblockhead.scale = 0;
   fidblockhead.status = fidfilehead.status;
   fidblockhead.index = 1;
   fidblockhead.mode = 0;
   fidblockhead.ctcount = 1;
   fidblockhead.lpval = 0.0;
   fidblockhead.rpval = 0.0;
   fidblockhead.lvl = 0.0;
   fidblockhead.tlt = 0.0;


/* Calculate the base t3 FID */
   calct3fids(sinfo);


/* Calculate (t1,t2,t3) FID */
   for (i = 0; i < sinfo->t1dim.np; i++)
   {
      for (j = 0; j < sinfo->t2dim.np; j++)
      {
         for (k = 0; k < sinfo->arraydim; k++)
         {
            if ( writeheader(outfd, (char *)(&fidblockhead), BHEAD) )
               return ERROR;

            calcfid(i, j, k, sinfo);
            if ( writefid(outfd, (char *) (sinfo->data),
			(int) (fidfilehead.tbytes), pinfo->dpflag,
			sinfo->spinsys.nspins) )
            {
               return ERROR;
            }

            fidblockhead.index += 1;
         }
      }
   }


/* Clean up */
   free( (char *)(sinfo->spinsys.freq3) );
   free( (char *)(sinfo->t3data) );
   (void) close(outfd);


/* Write out a text and procpar file */
   if ( writetext(pinfo) )
      return ERROR;

   if ( writeprocpar(pinfo, sinfo) )
      return ERROR;


/* End */
   return COMPLETE;
}

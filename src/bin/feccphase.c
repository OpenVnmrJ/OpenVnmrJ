/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
/*----------------------------------------------------------------------*/
/* feccphase.c 								*/
/*									*/
/* args 								*/
/*     filename      mandatory. full path name to data file		*/
/*     -s	     test mode.						*/
/*     -l=val        left_shift	number of points			*/
/*     -n=val	     multiplier for setting noise level			*/
/*     -p 'string'   the directory to use for files			*/
/*     -e=n_cm,delf1,delf2,Gn	experiments values needed		*/
/*		     n_cm = distance in cm between F19 samples		*/
/*		     delf1,delf2 = frequency shifts of samples with 	*/
/*				   applied gradient			*/
/*		     Gn = Field shift produced by 1G/cm gradient at 19F */
/*     -t=time	     finest relevant time				*/
/*     -T 'string'   name of file that contains the interblock times	*/
/*----------------------------------------------------------------------*/
/* This file expects data with the following array characteristics	*/
/* array='tof'	and ni is used to make effective array='ni,tof'		*/
/*	where 	tof=tof1,tof2						*/
/*		ni=1-n					*/
/* The first arrayed acquisition for tof1 and tof2 is a noise acquire.	*/
/*----------------------------------------------------------------------*/

/*****************************************************************/
/*   static char HeadSid[] = "data.h	1.4    9/4/86";  	 */
/************************************/
/*  data.h   -   data file handler  */
/************************************/
/*  The data file handler allows processing of NMR data files    */
/*  The NMR data files have the following structure :            */
/*      filehead blockhead blockdata blockhead blockdata ...     */
/*  All blocks within one file have exactly the same size        */
/*  Two files can be processed at the same time			 */
/*  The first is always the file "data" of the current 		 */
/*  experiment. It is left open and can be used by any module    */
/*  The second one can be used for "acqfil/fid" or other files   */
/*  and is only used within one module.				 */
/*****************************************************************/

#define D_DATAFILE 0	/* curexp/data file index 		 */
#define D_PHASFILE 1	/* curexp/phasefile file index		 */
#define D_USERFILE 2	/* any user file used in one module	 */

/*  The file headers are defined as follows:                     */

/*****************/
struct datafilehead
/*****************/
/* Used at the beginning of each data file (fid's, spectra, 2D)  */
{
  int nblocks;        /* number of blocks in file     */
  int ntraces;        /* number of traces per block   */
  int np;             /* number of elements per trace */
  int ebytes;         /* number of bytes per element  */
  int tbytes;         /* number of bytes per trace    */
  int bbytes;         /* number of bytes per block    */
  short transf;	       /* transposed storage flag      */
  short status;        /* status of whole file         */
  int spare1;         /* reserved for future use      */
};

/******************/
struct datablockhead
/******************/
/* Each file block contains the following header      */
{
  short scale;         /* scaling factor               */
  short status;        /* status of data in block      */
  short index;         /* block index                  */
  short spare3;        /* reserved for future use      */
  int  ctcount;       /* completed transients in fids */
  float lpval;	       /* left phase in phasefile      */
  float rpval;	       /* right phase in phasefile     */
  float lvl;	       /* level drift correction       */
  float tlt;           /* tilt drift correction        */
};

/* The data file handler provides a set of subroutines, which    */
/* allow to buffer several blocks of a file in memory at a time. */
/* The number of blocks buffered in memory is limited by the     */
/* constant D_MAXBYTES in the data file handler.                 */
/* Data file status codes bits					 */

#define S_DATA	1	 /* 0 = no data    1 = data there	 */
#define S_SPEC  2	 /* 0 = fid	   1 = spectrum		 */
#define S_32	4	 /* 0 = 16 bit	   1 = 32 bit		 */
#define S_FLOAT 8	 /* 0 = integer	   1 = floating point	 */
#define S_SECND	16	 /* 0 = first ft   1 = second ft	 */
#define S_ABSVAL 32      /* 0 = not absval 1 = absolute value    */
#define S_COMPLEX 64     /* 0 = not complex 1 = complex		 */
#define S_ACQPAR 128     /* 0 = not acqpar 1 = acq. params	 */

/* Some data file handler routines return pointers to the data   */
/* in the following structure.                                   */

/*****************/
struct datapointers
/*****************/
{
  struct datablockhead *head;  /* pointer to the head of block   */
  float *data;          /* pointer to the data of the block       */
};

/************************************************************************/
/* THE ABOVE DEFINITIONS ARE VARIAN DATA FILES				*/
/* the program lists the data structures in the various data formats	*/
/************************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TRUE 1
#define FALSE 0
#define	MAX_BLKS 200
#define  ecname	"ecph.inp"
#define	b0name	"b0ph.inp"

extern char *strcat();
extern FILE *fopen();

struct datafilehead header0;
struct datablockhead header1;

double offset_time;
int nblks;
int mode,my_ave,res,mres;
int lsfid;
double getnoise_val();
struct exp_vals
{
	double d_cm;
	double delf1;
	double delf2;
	double Gn;
	double fine_time;		/* The time between points in FID */
	double Tval[MAX_BLKS];		/* List of times that FIDs started */
};

main(argc,argv)
int argc;
char *argv[];
{  
   char infile[60],outpath[60],ecpath[60],b0path[60],tname[60],tpath[60],*tpntr;
   double *fbuff[MAX_BLKS],noise_mult;
   double noise_val,temp_val,lsfid_val;
   struct exp_vals eccvals;
   eccvals.fine_time = 1.0;
   offset_time = 0.0;
   lsfid_val = 0.0;
   noise_mult=1.0;
   mode = 1;
   my_ave = 1;
   while (--argc > 0)
   {
     ++argv;  /* get argument */
     tpntr = argv[0];
     if (*tpntr == '-')
     {
       switch (*(tpntr+1))
       {
	 case 's':
			mode = 0;
			break;
	 case 'l':
			mres = sscanf(tpntr,"-l=%lf", &lsfid_val);
			break;
	 case 'n':
			mres = sscanf(tpntr,"-n=%lf", &noise_mult);
			break;
	 case 'p':	/* get pathname for output files */
			--argc;
			++argv;
        		strcpy(outpath,*argv);
			break;
	 case 'e':	
			mres = sscanf(tpntr,"-e=%lf,%lf,%lf,%lf", &eccvals.d_cm,
							&eccvals.delf1,
							&eccvals.delf2,
							&eccvals.Gn);
			break;
	 case 't': 	
			res = sscanf(tpntr,"-t=%lf",&eccvals.fine_time);
			if (res != 1)
			   eccvals.fine_time = 0.01;
                        break;
	 case 'T': 	/* get the filename that contains the coarse time */
			/* values.					  */ 
			--argc;
			++argv;
        		strcpy(tname,*argv);
                        break;
	 case 'a': 	
			res = sscanf(tpntr,"-a=%d",&my_ave);
			if (res != 1)
			   my_ave = 1;
                        break;
	 default:
			printf("eccphase: Invalid Arg = %s\n",tpntr);
			exit(1);
			break;
       }
     }
     else
     {
        strcpy(infile,*argv);
     }
   }
   strcpy(tpath,outpath);
   strcat(tpath,"/");
   strcat(tpath,tname);

   /* set global lsfid */
   lsfid = (int)lsfid_val;

   /* read in delay times and numbers of blocks of data */
   nblks = read_tvals(tpath,&eccvals);

   /* read in data points, calculate instantaneous phase */
   read_in_file(infile,outpath,fbuff,&eccvals,nblks);

   if (mode > 0)
   {
	strcpy(ecpath,outpath);
   	strcat(ecpath,"/");
	strcat(ecpath,ecname);
	strcpy(b0path,outpath);
   	strcat(b0path,"/");
	strcat(b0path,b0name);
	noise_val = getnoise_val(fbuff[0]);
	temp_val = getnoise_val(fbuff[1]);
	if (temp_val > noise_val) noise_val = temp_val;
	noise_val = noise_val*noise_mult;
	printf("Noise value tof2 = %12.6f\n",noise_val);

	/* calculates ec and b0 values and outputs data	*/
	outputdata(ecpath,b0path,fbuff,nblks,&eccvals,&noise_val);
   }
exit(0);
}
  
static int *buff;

/****************************************************************/
/* Routine reads in data points, calculates phase (freq, and 	*/
/* puts values through low pass filter.				*/
/****************************************************************/
read_in_file(name,outfile,fbuff,eccvals,nblks)
char *name,*outfile;
double *fbuff[];
struct exp_vals *eccvals;
int nblks;
{ int file_d_in,tot,bsize,i,npoints;
    if ((file_d_in = open(name,O_RDONLY,0644)) < 0)
     {
     printf("could not open %s\n",name);
     return(1);
    }
    if ((tot = read(file_d_in,&header0,sizeof(header0))) != sizeof(header0))
    { 
       printf("panic %d read\n",tot);  
       exit(1); 
    }
    bsize = header0.bbytes - sizeof(header1);
    buff = (int *) malloc(bsize);
    if (header0.nblocks != nblks*2)
    {
	printf("eccphase: nblks = %d   header0.nblocks %d\n",
			nblks,header0.nblocks);
	exit(1);
    }
    if (header0.nblocks > MAX_BLKS)
    {
	printf("eccphase: ERROR - header0.nblocks %d. Max allowed is %d\n",
	       header0.nblocks, MAX_BLKS);
	exit(1);
    }

    for (i = 0; i < header0.nblocks; i++) 
    {
      if ((tot = read(file_d_in,&header1,sizeof(header1))) != sizeof(header1))
      {   
	 printf("panic %d read\n",tot);  
	 exit(1); 
      }
      if ((tot = read(file_d_in,buff,bsize)) != bsize)
      { 
	printf("panic only %d read of %d requested \n",tot,bsize);
	exit(-2);
      }
      /* Establish number of points after left shifting */
      npoints = header0.np - (lsfid*2);

      if (header0.ntraces > 0)
        fbuff[i] = (double *) malloc(sizeof(double)*npoints*header0.ntraces); 
      else
        fbuff[i] = (double *) malloc(sizeof(double)*npoints); 

      if (my_ave > npoints/2) 
         my_ave = npoints/2;
/*	printf("NBLK = %4d\n",i); */
      phase_calc(header0.ntraces,npoints,header0.ebytes,fbuff[i],
							eccvals->fine_time);
      lowpass(fbuff[i]); 
      if (mode == 0) 
	 print_block(fbuff[i],eccvals->Tval[i],eccvals->fine_time);
    }
    close(file_d_in);   
}


report_file_status()
{
   int temp,tot;
   printf("\n\n");
   printf("file header information\n----------------------------------------\n");
   printf("blocks = %d\n",header0.nblocks);
   printf("traces per block = %d\n",header0.ntraces);
   printf("elements per trace = %d\n",header0.np);
   printf("bytes per element = %d\n",header0.ebytes);
   printf("bytes per trace = %d\n",header0.tbytes);
   printf("bytes per block = %d\n",header0.bbytes);
   /* bbytes = ntraces * tbytes
      bbytes = ntraces * np * ebytes 
      total bytes = nblocks * bbytes + sizeof(header0) + sizeof(header1)
   */
   if (header0.tbytes != header0.np*header0.ebytes) 
      printf("size disagreements tbytes,ebytes,np\n");
   tot = sizeof(header0) + header0.nblocks*header0.bbytes;
   printf("file size totally should be %d\n",tot);
   printf("filehead size = %d\n",sizeof(header0));
   printf("transf flag  = %d  ",(int)header0.transf);
   printf("status flag  = 0x%x  ",(int)header0.status);
   printf("spare flag = 0x%x\n",(int)header0.spare1);
   printf("------------------------------------------------\n");
   printf("-----sanity check-------------------------------\n");
   printf("------------------------------------------------\n");
   temp = header0.status;
   if (!(temp & S_DATA)) printf("no data\n"); 
   else
   {
   printf("data present\n");
   if (!(temp & S_32)) printf("16 bit "); else printf("32 bit ");
   if (!(temp & S_FLOAT )) printf("integer \n"); else printf("float\n");
   if (!(temp & S_SPEC)) printf("fid \n"); 
   else printf("spectrum\n");
   if (!(temp & S_SECND )) printf("first ft \n"); else printf("second ft\n");
   if (!(temp & S_ABSVAL )) printf("phased \n"); else printf("abs val \n");
   if (!(temp & S_COMPLEX )) printf("not complex \n"); else printf("complex\n");
   if (!(temp & S_ACQPAR )) printf("not acq par\n"); else printf("acqpar\n");
   }
   printf("------------------------------------------------\n");
}

report_block_status(i)
int i;
{
   int temp,tot;
   printf("block number %d\n",i);
   printf("------------------------------------------------\n");
   printf("scaling = %d\n",(int)header1.scale);
   printf("ctcount = %d\n",header1.ctcount);
   printf("lpval = %g  rpval = %g\n",header1.lpval,header1.rpval);
   printf("lvl   = %g  tlt   = %g\n",header1.lvl,header1.tlt);
   printf("index  = %d   blockstatus = 0x%x \n",header1.index,header1.status);
}

/****************************************************************/
/* Calculates phase and instantaneous frequency.		*/
/****************************************************************/
phase_calc(ntraces,npoints,prec,fbuff,fine_time)
int ntraces,npoints,prec;
double *fbuff;
double fine_time;
{
    double x,y,p,a,norm,twopi;
    double lastp,*fpntr,*curfbuff,fx;
    int i,j,basex,basey;
    int *ipntr;
    short *spntr; 
    ipntr = buff;
    twopi = 2.0*M_PI;
    spntr = (short *) buff;
    curfbuff = fbuff;
    norm = twopi*fine_time;
    if (norm == 0.0) 
      norm = 1.0;
    else
      norm = 1.0 / norm;		/* fine_time in sec. */
					/* 1000.0/norm if fine_time in msec */

    if (header1.ctcount == 1)
    {
      basex = header1.lvl;
      basey = header1.tlt;
    }
    else
    {
      basex = 0;
      basey = 0;
    }
    for (i = 0; i < ntraces; i++) /* each trace */
    {
      lastp = 0.0;
/*      printf("ntrace=%4d\n",i); */
      /* Account for left shifting of fid */
      if (lsfid > 0)
      {
	for (j = 0; j < lsfid; j++)
	{
	   ipntr++;	/* x	*/
	   ipntr++;	/* y	*/
	}
      }
      for (j = 0; j < npoints/2; j++)  /* each point pair*/
      {
	if (prec == 4)
	{
          x = (double) (*ipntr++ - basex); 
          y = (double) (*ipntr++ - basey); 
	}
	else
	{
          x = (double) (*spntr++ - basex); 
          y = (double) (*spntr++ - basey); 
	}
        p = atan2(x,y);
        a = sqrt(x*x+y*y);
	fx = p - lastp;
	lastp = p;
	
	while (fx > M_PI) 
	   fx -= twopi;
	while (fx < -M_PI) 
	   fx += twopi;

	*curfbuff++ =  fx*norm;
	*curfbuff++ =  a;
/*        printf("%4d  %7.4f  %7.4f\n",j,*(curfbuff-2),*(curfbuff-1));  */
      }
    }
}


lowpass(fbuff)
double *fbuff;
{
  int i,j,k,npoints,ntr;
  register double x0,x1,x2,x3,x4,*fp,*fo,yt;
  ntr = header0.ntraces;
  npoints = header0.np/2 - lsfid;
  fp = fbuff;
  /*
  printf("instanteous freq of fid\n");
  printf("delta degrees\n");
  printf("%4d   %4d\n",ntr,npoints-21);
  */
  for (j = 0; j < ntr; j++)
  {
  fp +=2;  	/* This effectively gets rid of the first 1 point 	*/
		/* which we don't want since there was no frame of  	*/
		/* reference when we calculated its phase difference.	*/
  fo = fp;
  x0 = *fp;
  fp += 2;
  x1 = x0;
  x2 = x0; 
  x3 = *fp;
  fp += 2;
  x4 = *fp;
  fp += 2;
	/*  printf("ntrace=%4d,npoints=%4d\n",j,npoints); */
  for (i = 1; i < npoints; i++)
  {
	/*    printf("%7.4f   |",*fo); */
    yt = (0.1859*(x0+x4)+0.6891*(x1+x3)+x2)/2.750;   /* hamming */
    *fo = yt;
	/*    printf("%4d  %7.4f\n",i,*fo);  */
    fo += 2; 
    x0 = x1;
    x1 = x2;
    x2 = x3;
    x3 = x4;
    if (i < npoints -3)
    {
      x4 = *fp;
      fp += 2;
    }
    /* otherwise x4 is copied */
  }
 }
}

/*----------------------------------------------------------------------*/
/* read_tvals								*/
/*	Reads data set that contains the values for a set of delay 	*/
/*	times for which the experiment was performed.  The first "time"	*/
/*	is 0.0, and corresponds to the noise acquisition.  Returns the	*/
/*	the number of blocks of data that were taken.			*/
/*----------------------------------------------------------------------*/
int read_tvals(infile,eccvals)
char *infile;
struct exp_vals *eccvals;
{
   FILE *fp;
   char line[80];
   int cntr;
   if ((fp = fopen(infile,"r")) == NULL)
   {
       printf("could not open %s\n",infile);
       exit(1);
   }
   cntr = 0;
   while ((fgets(line,80,fp) != NULL) && (cntr < MAX_BLKS) )
   {
        sscanf(line,"%lf",&eccvals->Tval[cntr]);
	cntr++;
   }

   fclose(fp);
   return(cntr);
}


/*----------------------------------------------------------------------*/
/* getnoise_val								*/
/*	Steps through the amplitude values of a block of data and	*/
/*	returns the maximum value.  					*/
/*----------------------------------------------------------------------*/
double getnoise_val(fbuff)
double *fbuff;
{
   double max_val;
   int npoints,i;
   max_val = 0.0;
   npoints = header0.np/2 - lsfid;
   fbuff = fbuff + 1;			/* starts at the amplitude value */
   for (i=1; i<npoints; i++)
   {
	if (max_val < *fbuff)
	   max_val = *fbuff;
	fbuff = fbuff + 2;
   }
   return(max_val);
}

/*----------------------------------------------------------------------*/
/* outputdata								*/
/*	Outputs the first order eddycurrent data (Ec) and the zero	*/
/*	order eddycurrent data (b0).					*/
/*----------------------------------------------------------------------*/
outputdata(ecpath,b0path,fbuff,nblks,eccvals,noise_val)
char *ecpath,*b0path;
double *fbuff[],*noise_val;
struct exp_vals *eccvals;
int nblks;
{
   FILE *fe,*fb;
   double a,G,time,Ec,outtime,nextarray_time;
   double *f1ptr,*f2ptr;
   int f1cntr,f2cntr,npoints,i;

   if ((fe = fopen(ecpath,"a")) == NULL)
   {
       printf("could not open %s\n",ecpath);
       exit(1);
   }
   if ((fb = fopen(b0path,"a")) == NULL)
   {
       printf("could not open %s\n",b0path);
       exit(1);
   }

   /* calculate applied static gradient field in Hz/cm	*/
   G = (eccvals->delf1 - eccvals->delf2)/eccvals->d_cm;

   /* calculate position of sample 1 in cm	*/
   a = eccvals->delf1/G;

   /* write out block header for both files	*/
   fprintf(fe,"NEXT  %12.6f  %12.6f  %12.6f\n",G,eccvals->Gn,eccvals->d_cm);
   fprintf(fb,"NEXT  %12.6f  %12.6f  %12.6f\n",G,eccvals->Gn,eccvals->d_cm);

   for (f1cntr=2; f1cntr<nblks*2; f1cntr=f1cntr+2)
   {
	f2cntr=f1cntr+1;

	npoints = header0.np/2;
	npoints -= lsfid;	/* Allows for points not stored here */
	npoints -= 1;		/* First point had no phase reference */
	npoints -= 4;		/* Two points at each end trashed by filter */

	time = eccvals->Tval[f1cntr/2];
	/* Increment time past left-shifted points and skipped points */
	time += (lsfid + 3) * eccvals->fine_time;
	/* Time reference is halfway between this point and it's phase ref */
	time -= 0.5 * eccvals->fine_time;

	if ((f1cntr/2+1) < nblks)
	   nextarray_time = eccvals->Tval[f1cntr/2+1];
	else
	   nextarray_time = time+(npoints*eccvals->fine_time);

	f1ptr = fbuff[f1cntr] + 2 * 3;	/* Skip first 3 points */
	f2ptr = fbuff[f2cntr] + 2 * 3;
	for (i=0; (i < npoints) & (time < nextarray_time); i++)
	{
	   Ec = *f1ptr - *f2ptr;
	   if ( (*noise_val <= *(f1ptr+1)) || (*noise_val <= *(f2ptr+1)) )
	   {
	      /* output first order and zero order eddy current values */
	      fprintf(fe," %12.7f  %12.6f\n",time,Ec);
	      fprintf(fb," %12.7f  %12.6f\n",time,
		     *f1ptr - (Ec*a/eccvals->d_cm));
	   }

	   f1ptr = f1ptr+2;
	   f2ptr = f2ptr+2;
	   time = time + eccvals->fine_time;
	}
   }
   fclose(fe);
   fclose(fb);
}
	

print_block(fbuff,coarse_time,fine_time)
double *fbuff;
double coarse_time,fine_time;
{
  int i,j,npoints,ntr;
  int cntr;
  register double *fp,offset,*offset_ptr;
  double tt,f_ave;
  ntr = header0.ntraces;
  npoints = header0.np/2 - lsfid;
  fp = fbuff;
  offset = 0.0;
  if (mode == 2)
    printf("NEXT\n");
  for (j = 0; j < ntr; j++)
  {
    cntr = 1;
    f_ave = 0.0;
    printf("NEXT\n");
    for (i = 0; i < npoints; i++)
    {
      if ((i >= 1) && (i<npoints ))
      {
        if (my_ave < 2)
        {
        tt = coarse_time+fine_time*i+offset_time;
        printf("%11.5f  %11.5f\n",tt,*fp);  
        }
        else
        {
          f_ave += *fp;
          if (cntr == my_ave)
          { 
             tt = coarse_time+fine_time*(i - my_ave/2)+offset_time;
             printf("%11.5f  %11.5f\n",tt,f_ave/((double) my_ave));
             cntr = 0;
             f_ave = 0.0;
          }
          cntr++;
        }
      }
      fp += 2; 
    }
  }
}

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
/**************************************************************************/


#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/file.h>

/*----------------------------------------------------------------------*/
/* eccdisp.c 								*/
/*									*/
/* args  <input_filename> <output_filename> (these are manditory)	*/
/*     -f<format> 	format = 'f'(freq), 'p'(%grad), 'g'(Hz/G/cm)	*/
/*				 'c'- output calculated tc's, amp's.	*/
/*				 'i'- output input (act as fifo)	*/
/*				 'd'- output difference between first	*/
/*					and second blocks of data	*/
/*     -t<start>,<last>,<delta>   time between values to be plotted	*/
/*     -b<trace_blocks>	num of blocks of data to keep/display		*/
/*----------------------------------------------------------------------*/
/* Input File Format:							*/
/*									*/
/* Each block of data should have a header line followed by a pair of	*/
/* floating point numbers of each line representing time and freq.	*/
/* G = applied static gradient field					*/
/* Gn = field shift produced by a static gradient of 1G/cm at 19F	*/
/* d  = sample seperation in cm						*/
/* ie.									*/
/*	NEXT  <G>  <Gn>  <d>					*/
/*	<time>	<freq>							*/ 
/*	<time>	<freq>							*/ 
/*	.								*/
/*	.								*/
/*	<time>	<freq>							*/ 
/*	NEXT  <G>  <Gn>  <d>					*/
/*	<time>	<freq>							*/ 
/*	<time>	<freq>							*/ 
/*	.								*/
/*	.								*/
/*----------------------------------------------------------------------*/
/* Output File Format:							*/
/*									*/
/* All output data will be appended to a file that already exists	*/
/* It is put in a format that "expl" (Vnmr graphics pkg.) will handle	*/
/* Each block of data should have 2 header lines followed by a pair of	*/
/* floating point numbers of each line representing time and freq.	*/
/* ie.									*/
/*	NEXT								*/
/*	<time>	<value>							*/ 
/*	<time>	<value>							*/ 
/*	.								*/
/*	.								*/
/*	<time>	<value>							*/ 
/*	NEXT								*/
/*	<time>	<value>							*/ 
/*	<time>	<value>							*/ 
/*	.								*/
/*	.								*/
/*----------------------------------------------------------------------*/

#define	MAX_BLOCKS	6
#define MAX_POINTS	4096*20
#define MAX_TC		64
#define FALSE	0
#define TRUE	1
extern double log();
extern double exp();
int start,stop,ref;
int mode,my_ave,res,mres;
int nblocks,npoints[MAX_BLOCKS],tot_blocks;
double Gn[MAX_BLOCKS];			/* field shift produced by	*/
					/* static gradient of 1G/cm	*/
double G[MAX_BLOCKS];			/* applied static Gradient in	*/
					/* Hz/cm			*/
double distance[MAX_BLOCKS];
 double start_time,last_time,delta_time;
struct datapair {			/* initially this is used as a global */
	double	time;			/* structure			      */
	double	freq;
} eccdata[MAX_BLOCKS][MAX_POINTS];
FILE *fopen();

main(argc,argv)
int argc;
char *argv[];
{  
 char infile[60],outfile[60],*tpntr,format;
 int req_blocks, readinfile,i;

 readinfile = FALSE;
 nblocks = -1;
 format = 'f';				/* default format(same as input) */
 req_blocks = 1;			/* default output blocks */
 while (--argc > 0)
   {
     ++argv;  /* get argument */
     tpntr = argv[0];
     if (*tpntr == '-')
     {
       switch (*(tpntr+1))
       {
	 case 'f':	
			mres = sscanf(tpntr,"-f%c",&format); 
			break;
	 case 'b':	 
			mres = sscanf(tpntr,"-b%d",&req_blocks);
			break;
	 case 't':	 
			mres = sscanf(tpntr,"-t=%lf,%lf,%lf",&start_time,
						&last_time,&delta_time);
			break;
	 default:
			printf
			 ("eccdisp: Error invalid argument request (%c)\n",
				*(tpntr+1));
			exit(1);
			break;
       }
     }
     else
     {
	if (readinfile == FALSE)
	{
           strcpy(infile,*argv);
	   readinfile = TRUE;
	}
	else
	   strcpy(outfile,*argv);
     }
   }
   if (read_in_file(infile) != 0)
	exit(1);
   if (format == 'i')
	output_input(infile,req_blocks);
   else
	output_files(outfile,req_blocks,format);
   exit(0);
}
  
read_in_file(name)
char *name;
{   int file_d_in,tot,bsize,i,ok;
    FILE *fp;
    char field1[20],line[80];

    ok = TRUE;
    if ((fp = fopen(name,"r")) == NULL)
    {
       printf("could not open %s\n",name);
       exit(1);
    }
    while ((fgets(line,80,fp) != NULL) && ok)
    {
	sscanf(line,"%s",field1);
	if (strcmp(field1,"NEXT") == 0)
	{
	   nblocks = nblocks +1;
	   npoints[nblocks] = 0;
	   sscanf(line,"%s %lf %lf %lf",field1,&G[nblocks],&Gn[nblocks],
						&distance[nblocks]);
	}
	else
	{
	   sscanf(line,"%lf %lf",&eccdata[nblocks][npoints[nblocks]].time,
				   &eccdata[nblocks][npoints[nblocks]].freq);
	   npoints[nblocks] = npoints[nblocks] + 1;
	}
	if (npoints[nblocks] > MAX_POINTS)
	{
	   printf("eccdisp: Error, number of points > 8K.\n");
	   ok = FALSE;
	}
	if (nblocks > MAX_BLOCKS)
	{
	   printf("eccdisp: Error, number of data blocks > %d.\n",MAX_BLOCKS);
	   ok = FALSE;
	}
    }
    fclose(fp); 
    if (ok) 
    	return(0);
    else
	return(1);
}

/*------------------------------------------------------------------	*/
/* This routine outputs the last n blocks of original data back to the 	*/
/* input file.  In this way the data is kept as a fifo if the user	*/
/* wants to keep more than the most recently acquired data around for	*/
/* observation.								*/
/*------------------------------------------------------------------	*/
output_input(name,blocks)
char *name;
int blocks;
{
  int start_block,i,j,acc_cntr,tc_cntr,stat;
  FILE *fp;

  stat = 0;
  if ((fp = fopen(name,"w")) == NULL)
  {
       printf("could not open %s for writing\n",name);
       exit(1);
  }
  start_block = nblocks - blocks + 1;	/* detemine 1st block to output */
  if (start_block < 0) start_block = 0;
  
  /* output input file and display file */
  /* program assumes headers in display file are already there */
  for (i=start_block; i<=nblocks; i++)
  {
	/* output infile block headers*/
	fprintf(fp,"NEXT  %12.6f  %12.6f  %12.6f\n",G[i],Gn[i],distance[i]);

	for (j=0; j<npoints[i]; j++)
	{
	   /* output infile data */
	   fprintf(fp," %12.7f  %12.6f\n",eccdata[i][j].time,
				   eccdata[i][j].freq);

	}
  }
  fclose(fp);
}


output_files(outname,blocks,fmt)
char *outname,fmt;
int blocks;
{
  int start_block,i,j,acc_cntr[2],tc_cntr,mod_cntr,stat,flip,flip_cntr;
  FILE *fp, *fo;
  double acc_time[2],acc_data[2],prev_time,tc_prev_data[2],tc_prev_time[2];

  stat = 0;
  if ((fo = fopen(outname,"a")) == NULL)
  {
       printf("could not open %s for output\n",outname);
       exit(1);
  }

  start_block = nblocks - blocks + 1;	/* detemine 1st block to output */
  if (start_block < 0) start_block = 0;
  
  /* output input file and display file */
  /* program assumes headers in display file are already there */
  for (i=start_block; i<=nblocks; i++)
  {

	/* output calculated amplitudes, time constants if requested */
	if ( fmt == 'c' )
	{
	  printf("\nBLOCK = %d\n",i);
	  printf
	   ("   Delay(s)      Freq(hz)       tc(ms)       Amp(hz)     Amp(pct)    n\n");
	}

	/* output display file block headers */
	fprintf(fo,"NEXT\n");

	acc_time[0] = acc_time[1] = 0.0;
	acc_data[0] = acc_data[1] = 0.0;
	tc_prev_data[0] = tc_prev_data[1] = 0.0;
	acc_cntr[0] = acc_cntr[1] = 0;
	tc_cntr = 0;
	mod_cntr = npoints[i]/MAX_TC;
	if (mod_cntr == 0) mod_cntr = 1;
	flip = 0;
	flip_cntr = 0;
	prev_time = tc_prev_time[0] = tc_prev_time[1] = 0.0;
	for (j=0; j<npoints[i]; j++)
	{
	   if (((eccdata[i][j].time - prev_time) >= delta_time) || (j == 0))
	   {
	     if ( (eccdata[i][j].time >= start_time) && 
				(eccdata[i][j].time <= last_time) ) 
	     {
	       switch (fmt)
	       {
		   case 'f':
			/* output ECC values in frequency */
			fprintf(fo," %12.7f  %12.6f\n",eccdata[i][j].time,
						   eccdata[i][j].freq);
			break;
		   case 'p':
			/* output ECC values in percent */
			fprintf(fo," %12.7f  %12.6f\n",eccdata[i][j].time,
				eccdata[i][j].freq*100.0/(G[i]*distance[i]));
			break;
		   case 'g':
			/* output ECC values in Hz/G/cm */
			fprintf(fo," %12.7f  %12.6f\n",eccdata[i][j].time,
				eccdata[i][j].freq*Gn[i]/G[i]);
			break;
		   case 'c':
			/* output ECC values in freq, calculate tc's */
			fprintf(fo," %12.7f  %12.6f\n",eccdata[i][j].time,
						   eccdata[i][j].freq);
			tc_cntr = tc_cntr + 1;
			stat = calc_tc(tc_cntr,mod_cntr,&eccdata[i][j].time,
				&tc_prev_time[flip],&eccdata[i][j].freq,
				&tc_prev_data[flip],i);
			tc_prev_time[flip] = eccdata[i][j].time;
			tc_prev_data[flip] = eccdata[i][j].freq;
			break;
		   default:
			printf
			 ("eccdisp: Error invalid output format request (%c)\n",
				fmt);
			exit(1);
			break;
	        }
	      }
	      acc_time[flip] = 0.0;
	      acc_data[flip] = 0.0;
	      acc_cntr[flip] = 0;
	      /* flip_cntr++; */
	      prev_time = eccdata[i][j].time;
	   }
	}
  }
  fclose(fo);
}

/*----------------------------------------------------------------------*/
/* calc_tc								*/
/* Routine to calculate the time constants and amplitude values		*/
/* for eddy current calculation.					*/
/*----------------------------------------------------------------------*/
calc_tc(cntr,mod_cntr,time,prev_time,data,prev_data,iblk)
int cntr,mod_cntr,iblk;
double *time, *prev_time, *data, *prev_data;
{
 double tc, amp, alpha;
 int negate;

 if (cntr%mod_cntr > 0)
 {
    if ( cntr == (MAX_TC+1))
    {
	printf("Error: Too many points for tc/amplitude calculations;\n");
	printf("       Try increasing the time between display points.\n");
    }
	return(1);
 }

 /* calculate the time constant between points. */
 negate = FALSE;
 if (cntr%mod_cntr == 0)
 {
 	/* calculate the time constant and amplitude between points. */
	if ( (*prev_data < 0.0) && (*data < 0.0) ) 
	{
		tc = (*prev_time - *time)/
			log((*data)/(*prev_data) );
		/* calculate the amplitude */
		amp = *prev_data/exp(-1.0*(*prev_time)/tc);
	}
	else if ( (*prev_data > 0.0) && (*data > 0.0) )
	{
		tc = (*prev_time - *time)/
			(log(*data) - log(*prev_data));
		/* calculate the amplitude */
		amp = *prev_data/exp(-1.0*(*prev_time)/tc);
	}
	else
		negate = TRUE;
 }
 else
	negate = TRUE;

 if ((tc > 10.0) | (tc < -10.0))
	negate = TRUE;
 if ((amp > 99999.0) | (amp < -99999.0))
	negate = TRUE;

 if (negate == TRUE)
	/* output the values */
	printf(" %10.7f   %10.2f           x            x           x\n",*time,
		*data);
 else
 {
	/* calculate alpha which is (1 - amp/G) this is applied 	*/
	/* to both the time constant and the amplitude			*/
	alpha = 1.0 - (amp/(G[iblk]*distance[iblk]));
	/* output the values */
	printf(" %10.7f   %10.2f    %10.2f    %10.2f   %8.2f   %4d\n",*time,
		*data,alpha*tc*1000,amp,
		(amp/alpha)*(100.0/(G[iblk]*distance[iblk])),cntr);
 }
 return(0);
}




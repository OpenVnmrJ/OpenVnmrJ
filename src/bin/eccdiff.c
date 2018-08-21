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
/* eccdiff.c 								*/
/*									*/
/* args <input_file> <input_file2> <output_file> (manditory)		*/
/*     -b<trace_blocks>	num of blocks of data to keep/display		*/
/*									*/
/* Description:								*/
/* 	Program subtracts the value of each data point in the one block	*/
/*	of data in input_file2 from the value of each data point in 	*/
/*	each block of data in input_file.				*/
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

#define	MAX_BLOCKS	4
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

int nblocks2,npoints2[1],tot_blocks2;
double Gn2[1];			/* field shift produced by	*/
					/* static gradient of 1G/cm	*/
double G2[1];			/* applied static Gradient in	*/
					/* Hz/cm			*/
double distance2[1];
struct datapair2 {			/* initially this is used as a global */
	double	time;			/* structure			      */
	double	freq;
} eccdata2[1][MAX_POINTS];
FILE *fopen();

main(argc,argv)
int argc;
char *argv[];
{  
 char infile[60],infile2[60],outfile[60],*tpntr,format;
 int req_blocks, readinfile,readinfile2,i;

 readinfile = FALSE;
 nblocks = -1;
 readinfile2 = FALSE;
 nblocks2 = -1;
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
			 ("eccdiff: Error invalid argument request (%c)\n",
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
	else if (readinfile2 == FALSE)
	{
           strcpy(infile2,*argv);
	   readinfile2 = TRUE;
	}
	else
	   strcpy(outfile,*argv);
     }
   }
   if (read_in_file(infile) != 0)
	exit(1);
   if (read_in_file2(infile2) != 0)
	exit(1);

   output_diff(outfile,req_blocks);
   exit(0);
}
  
/*------------------------------------------------------------------	*/
/* This will always be just one block of data.			        */
/*------------------------------------------------------------------	*/
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
	   printf("eccdiff: Error, number of points > 8K.\n");
	   ok = FALSE;
	}
	if (nblocks > MAX_BLOCKS)
	{
	   printf("eccdiff: Error, number of data blocks > %d.\n",MAX_BLOCKS);
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
/* This will always be just one block of data.			        */
/*------------------------------------------------------------------	*/
read_in_file2(name)
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
	   nblocks2 = nblocks2 +1;
	   npoints2[nblocks2] = 0;
	   sscanf(line,"%s %lf %lf %lf",field1,&G2[nblocks2],&Gn2[nblocks2],
						&distance2[nblocks2]);
	}
	else
	{
	   sscanf(line,"%lf %lf",&eccdata2[nblocks2][npoints2[nblocks2]].time,
				  &eccdata2[nblocks2][npoints2[nblocks2]].freq);
	   npoints2[nblocks2] = npoints2[nblocks2] + 1;
	}
	if (npoints2[nblocks2] > MAX_POINTS)
	{
	   printf("eccdiff: Error-2, number of points > 8K.\n");
	   ok = FALSE;
	}
	if (nblocks2 > 1)
	{
	   printf("eccdiff: Error-2, number of data blocks > 1.\n");
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
/* This routine outputs the difference of the last n blocks of the      */
/* first data with the second data file.b			 	*/
/*------------------------------------------------------------------	*/
output_diff(name,blocks)
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

	if (npoints[i] != npoints2[0])
	{
	   printf("eccdiff: Error, Number of points not equal %d:%d.\n",
				npoints[i],npoints2[0]);
	   printf("         Re-acquire base difference values.\n");
	   exit(1);
	}

	for (j=0; j<npoints[i]; j++)
	{
	   /* output infile data */
	   if (eccdata[i][j].time == eccdata2[0][j].time) 
	   {
	   	fprintf(fp," %12.7f  %12.6f\n",eccdata[i][j].time,
				eccdata[i][j].freq-eccdata2[0][j].freq);
	   }
	   else
	   {
	   	printf("eccdiff: Error, Times not equal %12.7f:%12.7f.\n",
				eccdata[i][j].time, eccdata2[0][j].time);
	   	printf("         Re-acquire base difference values.\n");
	   	exit(1);
	   }
	}
  }
  fclose(fp);
}






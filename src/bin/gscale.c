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
/**********************************************************************
 gscale  -  Determine the extent of the sample

 Details - The size of the sample along the read, phase and slice
           dimensions are calculated. Signals below the threshold are
           ignored. The longest line containing signals above the
           threshold is determined and reported - mm, points.

 Usage - gscale  rootfilename
         Input - file.1 file.param
         Output - file.out  -thresholded output for debugging
**********************************************************************/


#include	<stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include	<math.h>
#include	"util.h"

#define		TWOPI		6.28318531
#define 	PI		3.1415926
#define 	index(s,p,r) (s*psize*rsize+p*rsize+r)

/* I/O string */
char		str[80];

main(argc,argv)
int 	argc;
char	*argv[];
{
    FILE	*paramsfile,*imagefile,*outfile;
    char	fieldname[80];
    int		args;
    int		rsize,psize,ssize;
    float	rfov,pfov,sfov;
    int		totalsize,r,p,s;
    float	*p1,*p2,*out;
    float       v,v2,slength,plength,rlength;
    int         maxpts,pts;
    float	maxmag,avmag,thresh,threshold;
    int   	i,numpoints;
    

    /* 
     * usage 
     */ 

    checkargs(argv[0],argc,"rootfilename");

    /*
     * process arguments
     */

    args = 1;

    /* open the field map phase files, frequency file, and parameter files */
    strcpy(fieldname,argv[args]);
    imagefile = efopen(argv[0],strcat(fieldname,".1"),"r");
    strcpy(fieldname,argv[args]);
    paramsfile = efopen(argv[0],strcat(fieldname,".param"),"r");
    strcpy(fieldname,argv[args]);
    outfile = efopen(argv[0],strcat(fieldname,".out"),"w");

    /* read the field map parameters */

    /* read field map size */
    efgets(str,80,paramsfile);
    sscanf(str,"%d %d %d",&rsize,&psize,&ssize);
    efgets(str,80,paramsfile);
    sscanf(str,"%f %f %f",&rfov,&pfov,&sfov);


    /* calculate array size  */   
    totalsize = rsize*psize*ssize;

    /* allocate data space  */
    p1 = (float *) calloc((unsigned)totalsize,sizeof(float));
    out = (float *) calloc((unsigned)totalsize,sizeof(float));
    /*
     * process
     */

    /* read in */
    fread(p1,sizeof(float),totalsize,imagefile);

    /* find the maximum value */
    maxmag = 0.0;
    for ( i=0 ; i<totalsize ; i++ )
    maxmag = (p1[i]>maxmag) ? p1[i] : maxmag;

    /* average everything that is at least 50% of maximum */
    avmag = 0.0;
    numpoints = 0;
    for ( i=0 ; i<totalsize ; i++ )
    if ( p1[i] > 0.5*maxmag )
    {   
        avmag += p1[i];
        numpoints++;
    }   
    avmag /= numpoints;
  
    /* set the threshold at 20% of this average */
    threshold = 50;
    thresh = threshold*avmag/100;

    /* zero data below the threshold */
    if (threshold > 0)
    {
      for ( i=0 ; i<totalsize ; i++ )
      {
    	if (p1[i]<thresh)
          p1[i] = 0.0;

          out[i] = p1[i];   /* thresholded image */ 
      }
    }
    /* write out the field map */
    fwrite(out,sizeof(float),totalsize,outfile);

    numpoints = 0;
    maxpts = 0;
    for ( s=0 ; s<ssize ; s++ )
    {
      for ( p=0 ; p<psize ; p++ )  
      {
         pts = 0;  /* no of pts > 0 */
	 for ( r=0; r<rsize-1 ; r++ )  
	 {
          /* calc */
          v = p1[index(s,p,r)];
          v2 = p1[index(s,p,r+1)];
          if(v > 0)
          {
            pts++;
            numpoints++;
            if(v2 = 0)
              r = rsize;  /* end reached; done */
          }
        }
        if(pts > maxpts)
          maxpts = pts;
      }
    }
    rlength =  (maxpts-1)*rfov/(rsize-1);
    
    /* phase encode dimension */
    numpoints = 0;
    maxpts = 0;  /* maximum points >0 in line */
    for ( s=0 ; s<ssize ; s++ )
    {
      for ( r=0; r<rsize ; r++ )  
      {
         pts = 0;  /* no of pts > 0 */
         for ( p=0 ; p<psize-1 ; p++ )  
	 {
          /* calc */
          v = p1[index(s,p,r)];
          v2 = p1[index(s,p+1,r)];
          if(v > 0)
          {
            pts++;
            numpoints++;
            if(v2 = 0)
              p = psize;  /* end reached; done */
          }
        }
        if(pts > maxpts)
          maxpts = pts;
      }
    }
    plength =  (maxpts-1)*pfov/(psize-1);

    /* pe-2 dimension */
    numpoints = 0;
    maxpts = 0;  /* maximum points >0 in line */
    for ( p=0 ; p<psize ; p++ )
    {
      for ( r=0; r<rsize ; r++ )  
      {
         pts = 0;  /* no of pts > 0 */
         for ( s=0 ; s<ssize-1 ; s++ )  
	 {
          /* calc */
          v = p1[index(s,p,r)];
          v2 = p1[index(s+1,p,r)];
          if(v > 0)
          {
            pts++;
            numpoints++;
            if(v2 = 0)
              s = ssize;  /* end reached; done */
          }
        }
        if(pts > maxpts)
          maxpts = pts;
      }
    }
    slength =  (maxpts-1)*sfov/(ssize-1);
    printf("Size %4.4f %4.4f %4.4f \n",rlength,plength,slength);

}

/*************************************************************************
		Modification History
		
**************************************************************************/

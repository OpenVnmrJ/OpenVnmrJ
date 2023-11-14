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
/*******************************************************************

TITLE: gsfield
READ AS: Gradient shimming field measurement routine

**********************************************************************
PURPOSE: To estimate the true frequency

DETAILS: The input file B0.A.wf is assumed to be the phase file without
	 wraps. The frequency is estimated for tau=delta using the latter.
	 This avoids the phase unwrapping using software.
	 
         Magnitude file mapname.mag is used for thresholding. This is 
         usually, tau=delta.

USAGE:

INPUT:   Phase file 'mapname'.A.wf, 'mapname'.B.wf, 'mapname.param

OUTPUT:  True (unwrapped) phase file mapname.f

EXAMPLE OF CALL: 
 
LIMITATIONS:

AUTHOR: S. Sukumar

**********************************************************************/
#include	<stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include	<math.h>
#include	"util.h"

#define 	PI              3.14159265358979323846
#define		TWOPI           (2*PI)

/* indexing macro */
#define rmapindex(z,y,x) (z*yres*xres+y*xres+x)

/* I/O string */
char		s[80];
static float puc;

main(argc,argv)
int 	argc;
char	*argv[];
{
    FILE	*magfile, *paramsfile,*ufieldfile,*ufieldfile2;
    FILE	*fieldfile2,*fieldfile;
    char	fieldname[80];
    int		args;
    int		xfieldres,yfieldres,zfieldres;
    int		xres,yres,zres;
    float	xfov,yfov,zfov;
    int		totalfieldsize;
    float	*ufield, *field, *field2, *mag;
    float	freq;
    float	delay,mindelay,threshold, thresh;
    float	maxmag,avmag;
    float	pup,pp,pc;
    int		xi,x,y,z,i,numpoints;
    int		xr,xl,yt,yb,zi,zo,xlimit,pass1;
    int		zpass,ypass,xpass;
    float	p2a,p2c,p2e,p1,p2;
    float	wraps,scale,diffa,prange;
    int		intwraps;

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
    magfile = efopen(argv[0],strcat(fieldname,".mag"),"r");
    strcpy(fieldname,argv[args]);
    ufieldfile = efopen(argv[0],strcat(fieldname,".f"),"w");
    strcpy(fieldname,argv[args]);
    paramsfile = efopen(argv[0],strcat(fieldname,".param"),"r");
    strcpy(fieldname,argv[args]);  
    fieldfile = efopen(argv[0],strcat(fieldname,".A.wf"),"r");  
    strcpy(fieldname,argv[args++]);
    fieldfile2 = efopen(argv[0],strcat(fieldname,".B.wf"),"r");

    /* read the field map parameters */

    /* read field map size */
    efgets(s,80,paramsfile);
    sscanf(s,"%d %d %d",&xfieldres,&yfieldres,&zfieldres);
    efgets(s,80,paramsfile);
    sscanf(s,"%f %f %f",&xfov,&yfov,&zfov);
    /* read phase delay */
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&delay);
    /* read phase delay */
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&threshold);
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&mindelay);
    
    /*
     * calculate array size; x,y,z correspond to ro,pe,ss
     */
    
    totalfieldsize = xfieldres*yfieldres*zfieldres;
	xres = xfieldres;
	yres = yfieldres;
	zres = zfieldres;

    /*
     * allocate data space
     */

    ufield = (float *) calloc((unsigned)totalfieldsize,sizeof(float));
    field = (float *) calloc((unsigned)totalfieldsize,sizeof(float));
    field2 = (float *) calloc((unsigned)totalfieldsize,sizeof(float));

    /* read in phase file */
    fread(field,sizeof(float),totalfieldsize,fieldfile);
    fread(field2,sizeof(float),totalfieldsize,fieldfile2);
    /*
     * process
     */
    /* 
     * apply the magnitude mask
     */ 

    /* load the magnitude image */
    mag = (float *) calloc(totalfieldsize,sizeof(float));
    fread(mag,sizeof(float),totalfieldsize,magfile);

    /* find the maximum value */
    maxmag = 0.0;
    for ( i=0 ; i<totalfieldsize ; i++ )
    maxmag = (mag[i]>maxmag) ? mag[i] : maxmag;

    /* average everything that is at least 50% of maximum */
    avmag = 0.0;
    numpoints = 0;
    for ( i=0 ; i<totalfieldsize ; i++ )
    if ( mag[i] > 0.5*maxmag )
    {   
        avmag += mag[i];
        numpoints++;
    }   
    avmag /= numpoints;
        
    /* set the threshold at 20% of this average */
    thresh = threshold*avmag/100;
 
    /* zero data below the threshold */
    if (threshold > 0)
    {
      for ( i=0 ; i<totalfieldsize ; i++ )
      {
    	if (mag[i]<thresh)
    	{
		field[i] = 0;
		field2[i] = 0;
	}            
      }
    }

    free(mag);

    prange = 1;   /* estimated error limit between calculated phases */
    scale = delay/mindelay;
    for ( i=0 ; i<totalfieldsize ; i++ )
    {
      
        ufield[i] = 0;   /* clear out output buffer */
	p1 = field[i];  /* measured phase in radians for tau=mindelta */
	p2 = field2[i];    /* measured phase for delta */ 
	if (!((p1==0)||(p2==0)))  /* trap for phase=0, mask or threshold limit */      
	{
          if (field2[i] != 0)
    	  {
	        p2e = p1 * scale;  /* phase estimate; assume no p1 wraps */
    	        
	        /* Check for no wrap condition; this avoids sign problem
	         * when snr is low
	         */
	        if ((p2e < 2) && (p2e > -2)) 
	        {
	            p2c = p2;
	            ufield[i] = p2;
	        }
	        else if (p2e > 0)       /* positive swing */
	        {  
	            if (p2 < 0)
		        p2a = p2 + TWOPI;       /* adjusted phase, 0 to 2PI */
		    else
		        p2a = p2;
		        
		    wraps = fabs(p2e)/TWOPI;    /* no of wraps */
		    intwraps = (int)(wraps);      /* no of integer wraps */
		    p2c = (intwraps*TWOPI)+p2a;   /* calculated phase */
		    diffa = fabs(p2e-p2c);	/*  phase error, magnitude */
		    
		    if (diffa < prange) 	/* check if within range */
		        ufield[i] = p2c;
		    else
		    {
		        if (diffa > PI)
		        {
		           p2c = p2c + TWOPI;       /* this is to correct for noise near zero */
		           diffa = fabs(p2e - p2c);
		           if (diffa < prange)
		               ufield[i] = p2c;
		           else
		           {
		               p2c = p2c - TWOPI;  
		               diffa = fabs(p2e - p2c);
		               if (diffa < prange)
		                   ufield[i] = p2c;
		               else
		                   ufield[i] = 0;
		           } 
		        }
		        else
		            ufield[i] = 0;   /* between prange-2PI */		               
		    }
		}
		else		/* negative swing */
	        {  
	            if (p2 > 0)
		        p2a = p2 - TWOPI;       /* phase adjusted, 0 to -2PI */
		    else
		        p2a = p2;
		        
		    wraps = fabs(p2e)/TWOPI;    /* no of wraps */
		    intwraps = (int)(wraps);      /* no of integer wraps */
		    p2c = (-intwraps*TWOPI)+p2a;   /* unwrapped phase */
		    diffa = fabs(p2e-p2c);	/*  phase error, magnitude */
		    
		    if (diffa < prange) 	/* check if within range */
		        ufield[i] = p2c;
		    else			/* check if off by 2PI */
		    {
		        if (diffa > PI)
		        {
		           p2c = p2c + TWOPI;
		           diffa = fabs(p2e - p2c);
		           if (diffa < prange)
		               ufield[i] = p2c;
		           else
		           {
		               p2c = p2c - TWOPI;  /* 2PI already added */
		               diffa = fabs(p2e - p2c);
		               if (diffa < prange)
		                   ufield[i] = p2c;
		               else
		                   ufield[i] = 0;
		           }
		        }
		        else
		            ufield[i] = 0;   /* between prange-2PI */        
		    }
		}		
	  }
	} 
    /***
    if((i > 64*32*16.5)&&(i < 64*32*16.5+64))
      printf("%d: %7.2f %7.2f %7.2f %7.2f \n",i,p1,p2,p2e,ufield[i]);	
    ****/
    }
    
    /****
     	verify validity of phase unwrap (TBD)
    *****/
    
    /* write out the field map */
    fwrite(ufield,sizeof(float),totalfieldsize,ufieldfile);
}



/*****************************************************************
		Modification History
		
030512(ss) mapname.mag used for threshold

******************************************************************/

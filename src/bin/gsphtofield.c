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

TITLE: gsphtofield
READ AS: Gradient shimming field measurement routine; phase to field

**********************************************************************
PURPOSE: To estimate the true frequency from phase (R,I) data

DETAILS: The input file name.{1,A,B}.ri is the FT'ed file. 
	 The frequency is estimated for tau=delta using the latter.
	 This avoids the phase unwrapping using software.
	 Phase angle and then the two field maps corresponding to
	 del, and delref are calculated. Phase wrapping is removed as
	 in gsfield.
	 
         Magnitude file name.1.mag is used for thresholding. This is 
         usually, tau=0, i.e. shortest te data.

USAGE:   gsphtofield mapname 

INPUT:   Phase file mapname.{1,A,B}.ri mapname.gcoil.shimmapid.param

OUTPUT:  True (unwrapped) phase file mapname.f
         mapname.A.wf, mapname.B.wf

EXAMPLE OF CALL: 
 
LIMITATIONS: pss must be equal to 0.

AUTHOR: S. Sukumar

**********************************************************************/
#include	<stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include	<math.h>
#include	"util.h"

#define 	PI              3.14159265358979323846
#define		TWOPI           (2*PI)

/* I/O string */
char		s[80];
static float puc;

main(argc,argv)
int 	argc;
char	*argv[];
{
    FILE	*magfile,*wffile2,*wffile3;
    FILE	*paramsfile,*ufieldfile;
    FILE	*fieldfile3,*fieldfile2,*fieldfile;
    char	fieldname[80];
    int		args;
    int		xfieldres,yfieldres,zfieldres;
    int		xres,yres,zres;
    float	xfov,yfov,zfov;
    int		totalfieldsize;
    float	*ufield, *field, *field2, *field3, *mag, *ph,*ph2,*ph3;
    float	freq;
    float	delay,mindelay,threshold, thresh;
    float	maxmag,avmag;
    int		x,y,z,i,j,numpoints,point;
    float	p2a,p2c,p2e,p1,p2;
    float	wraps,scale,diffa,prange;
    int		intwraps;
    char	orient[20],str[20];
    int		slices;
    float	thk,psi,phi,theta,xoffset,yoffset,zoffset;

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
    ufieldfile = efopen(argv[0],strcat(fieldname,".f"),"w");
    strcpy(fieldname,argv[args]);   
    magfile = efopen(argv[0],strcat(fieldname,".B.mag"),"w");
    strcpy(fieldname,argv[args]);   
    wffile2 = efopen(argv[0],strcat(fieldname,".A.wf"),"w");    
    strcpy(fieldname,argv[args]);
    wffile3 = efopen(argv[0],strcat(fieldname,".B.wf"),"w");    
    strcpy(fieldname,argv[args]);           
    paramsfile = efopen(argv[0],strcat(fieldname,".param"),"r");
    strcpy(fieldname,argv[args]);  
    fieldfile = efopen(argv[0],strcat(fieldname,".1.ri"),"r");  
    strcpy(fieldname,argv[args]);
    fieldfile2 = efopen(argv[0],strcat(fieldname,".A.ri"),"r");
    strcpy(fieldname,argv[args++]);
    fieldfile3 = efopen(argv[0],strcat(fieldname,".B.ri"),"r");
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
    efgets(s,80,paramsfile);
    sscanf(str,"%s %f %f %f",orient,&psi,&phi,&theta);
    efgets(s,80,paramsfile);
    sscanf(s,"%f %f %f",&xoffset,&yoffset,&zoffset); /* r,p,s */
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&thk);
    efgets(s,80,paramsfile);
    sscanf(s,"%d",&slices);       
 
   /*
     * calculate array size; x,y,z correspond to ro,pe,ss
     */
    
    if (zfieldres <= 1)
    	zfieldres = slices;   /* multi-slice data */
    if (!strcmp(orient,"3orthogonal"))
        zfieldres = slices*3;
      
        totalfieldsize = xfieldres*yfieldres*zfieldres;    /* no of complex pts */
	xres = xfieldres;
	yres = yfieldres;
	zres = zfieldres;

    /*
     * allocate data space
     */

    field = (float *) calloc((unsigned)(2*totalfieldsize),sizeof(float));
    field2 = (float *) calloc((unsigned)(2*totalfieldsize),sizeof(float));
    field3 = (float *) calloc((unsigned)(2*totalfieldsize),sizeof(float));
    mag = (float *) calloc(totalfieldsize,sizeof(float));  /* magnitude data */
    ph = (float *) calloc(totalfieldsize,sizeof(float));   /* phase angle */
    ph2 = (float *) calloc(totalfieldsize,sizeof(float));
    ph3 = (float *) calloc(totalfieldsize,sizeof(float));
    /* read in FT'ed phase sensitive image data */
    fread(field,sizeof(float),(2*totalfieldsize),fieldfile);
    fread(field2,sizeof(float),(2*totalfieldsize),fieldfile2);
    fread(field3,sizeof(float),(2*totalfieldsize),fieldfile3); 

    /*
     * process
     */
     i=0;j=0;
     for(z=0; z<zres; z++)
       for(y=0; y<yres; y++)
         for(x=0; x<xres; x++)
         {
           /* phase alternate data */
           /* magnitude for image with longest TE */
	   mag[j] = hypot((double)field3[i+1], (double)field3[i]);
	   
	   if(y==yres/2)
	     printf("x=%d: y=%4.2f: z=%d/n",x,mag[j],z);	     
            
           field[i] *= (1.0-2.0*((y+x)%2));   
           field[i+1] *= (1.0-2.0*((y+x)%2));
           field2[i] *= (1.0-2.0*((y+x)%2));   
           field2[i+1] *= (1.0-2.0*((y+x)%2));
           field3[i] *= (1.0-2.0*((y+x)%2));   
           field3[i+1] *= (1.0-2.0*((y+x)%2));
          
           ph[j] = atan2((double)field[i+1], (double)field[i]);
           ph2[j] = atan2((double)field2[i+1], (double)field2[i]);
           ph3[j] = atan2((double)field3[i+1], (double)field3[i]);         
           
           /* longest te is used for thresholding */
	   j++;        
           i = i+2;
         }

    /* Calculate the magnitude image and apply threshold mask */
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
        
    /* set the threshold at 'threshold'% of this average */
    thresh = threshold*avmag/100;
    /**
printf("th=%f, thresh=%f, avmag=%f, maxmag=%f\n",threshold,thresh,avmag,maxmag);
exit(1);
**/    
    /* zero data below the threshold */
    if (threshold > 0)
    {
      for ( i=0 ; i<totalfieldsize ; i++ )
      {
    	if (mag[i]<thresh)
    	{
		ph[i] = 0;
		ph2[i] = 0;
		ph3[i] = 0;
		mag[i] = 0;
	}            
      }
    } 
    
    /* calc. phase difference, checking overange ph.1 - ph.B (wrapped) */
    /* Note: sign changed for gsft2d routines */
    /* note field[] array now contains wrapped, phase difference */
    for ( point=0 ; point<totalfieldsize ; point++ )
    {
	freq = ph[point]-ph2[point];
	freq = (freq>PI) ? freq-TWOPI : freq;
	freq = (freq<-PI) ? freq+TWOPI : freq;
	field2[point] = -freq;
    }
    /* phasesubtract, checking overange ph.3 - ph.1 */
    for ( point=0 ; point<totalfieldsize ; point++ )
    {
	freq = ph[point]-ph3[point];
	freq = (freq>PI) ? freq-TWOPI : freq;
	freq = (freq<-PI) ? freq+TWOPI : freq;
	field3[point] = -freq;
    }
    /* write out the unwrapped phase change (freq) files */
    fwrite(mag,sizeof(float),totalfieldsize,magfile); 
    fwrite(field2,sizeof(float),totalfieldsize,wffile2); 
    fwrite(field3,sizeof(float),totalfieldsize,wffile3);    
    free(ph);
    free(ph2);
    free(ph3); 
    free(field);
    free(mag);

    ufield = (float *) calloc((unsigned)totalfieldsize,sizeof(float));
    prange = 1;   /* estimated error limit between calculated phases in radians */
    scale = delay/mindelay;
    for (i=0 ; i<totalfieldsize ; i++)
    {
      
        ufield[i] = 0;   /* clear out output buffer */
	p1 = field2[i];  /* measured phase change in radians for tau=mindelta */
	p2 = field3[i];    /* measured phase change for tau=delta */ 
	if (!((p1==0)||(p2==0)))  /* trap for phase=0, mask or threshold limit */      
	{
          if (field3[i] != 0)
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
     } 
    /****
     	verify validity of phase unwrap (TBD)
    *****/
    
    /* write out the field map */
    fwrite(ufield,sizeof(float),totalfieldsize,ufieldfile);
    free(field2);
    free(field3);
    free(ufield);	
}



/*****************************************************************
		Modification History
		
030512(ss) mapname.B.mag used for threshold
17aug05(ss) phase = ph1 - phB; sign changed for gsft2d

******************************************************************/

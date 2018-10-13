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
/*****************************************************************************

 gsft2d - Gradient Autoshim, FT routines for multi-slice data

 Details: - 2DFFT is done on raw binary integer (input) data
            Input: filename.{1,A,B}.bin, filename.param
	    output: filename.{1,A,B}.ri  - FT'ed multislice data

            gaussian filter applied
            

*****************************************************************************/

#include	<stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include	<math.h>
#include	"nrutil.h"
#include	"nr.h"
#include	"util.h"


/* indexing macro */
#define cmapindex(Z,Y,X) (2*((Z)*xres*yres+(Y)*xres+(X)))
#define symapindex(Z,Y,X) (2*((Z)*xres*yres+(yres-1-(Y))*xres+(X)))
#define rmapindex(Z,Y,X) ((Z)*xres*yres+(Y)*xres+(X))

#define PI 3.1415926
#define TWOPI 6.28318531

#define MSLICE 1

/* I/O string */
char		s[80];

main(argc,argv)
int 	argc;
char	*argv[];
{
    FILE	*rawfile,			/* raw data */
		*rawfile2,			/* second echo data */
		*rawfile3,
		*paramsfile,			/* parameters data set */
		*phasefile,*phasefile2,*phasefile3;	/* image phase, floats */
    char	mapname[80];			/* root name for all files */

    int		xres,yres,zres;			/* map dimensions */
    float	xfov,yfov,zfov;
    float	delay,threshold, thresh;
    int		*mapsize;			/* NR vector of dimensions */
    int		totalmapsize;			/* product of dimensions */

    float	*raw,*raw2,*raw3,*buf;		/* raw data */
    float	*fraw,*fraw2,*fraw3;		/* floated raw data */
    float	*ftbuf,*ftbuf2,*ftbuf3;		/* ft buffer */
    int		args;				/* argument cntr */

    int		x,y,z,echo,block,i,ptr;		/* loop counters */

    float	scale;				/* image scaling factor */
    float	z0,freq;			/* baseline phase */
    int		zstep,ystep,xstep,		/* step size for filter array */
    		zs,ys,xs,zi,yi,xi;		/* filter start and index */
    int		gfilter;			/* 1/0 = gauss filter on/off */
    float	maxmag,avmag,numpoints;		/* used for thresholding */
    char	orient[80];
    int		slices,imgsize;
    float	thk,psi,phi,theta,xoffset,yoffset,zoffset;
    float	mindelay;
            
#include        "gauss.h"

    gfilter = 1;	/* gaussian filter flag */

    /* check command string */
    checkargs(argv[0],argc,"rootfilename");

     
    mapsize = ivector(1,2); /* allocate space for mapsize (NR)vector */

     /* process arguments */

    args = 1;

    /* open the field map raw data, reconstructed, and parameter files */
    strcpy(mapname,argv[args]);
    rawfile = efopen(argv[0],strcat(mapname,".1.bin"),"r");
    strcpy(mapname,argv[args]);
    rawfile2 = efopen(argv[0],strcat(mapname,".A.bin"),"r");
    strcpy(mapname,argv[args]);
    rawfile3 = efopen(argv[0],strcat(mapname,".B.bin"),"r");
    strcpy(mapname,argv[args]);    
    paramsfile = efopen(argv[0],strcat(mapname,".param"),"r"); /* parameters */
    strcpy(mapname,argv[args]);
    phasefile = efopen(argv[0],strcat(mapname,".1.ri"),"w");  /* phase image */
    strcpy(mapname,argv[args]);
    phasefile2 = efopen(argv[0],strcat(mapname,".A.ri"),"w");
    strcpy(mapname,argv[args]);
    phasefile3 = efopen(argv[0],strcat(mapname,".B.ri"),"w");
    

     /* read field map size */
     
    efgets(s,80,paramsfile);
    sscanf(s,"%d %d %d",&xres,&yres,&zres);  /* r,p,s */    
    efgets(s,80,paramsfile);
    sscanf(s,"%f %f %f",&xfov,&yfov,&zfov);
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&delay);
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&threshold);
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&mindelay);
    efgets(s,80,paramsfile);
    sscanf(s,"%s %f %f %f",orient,&psi,&phi,&theta);
    efgets(s,80,paramsfile);
    sscanf(s,"%f %f %f",&xoffset,&yoffset,&zoffset); /* r,p,s */
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&thk);
    efgets(s,80,paramsfile);
    sscanf(s,"%d",&slices);    
     /* calculate array size */

    mapsize[2] = xres;  /* read */
    mapsize[1] = yres;  /* pe */
    if(zres <= 1)
      zres = slices; 
    totalmapsize = zres*yres*xres;  /* r,p,s */
    imgsize = xres*yres;
    
    scale = 1.0/sqrt((double)(yres*xres));

    /* allocate space for raw, phase, and magnitude arrays */

    raw = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    raw2 = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    raw3 = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    
    fraw = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    fraw2 = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    fraw3 = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    buf = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    
    ftbuf = vector(0,2*imgsize);
    ftbuf2 = vector(0,2*imgsize);
    ftbuf3 = vector(0,2*imgsize);
    
    /* process echo #1 */
    /* read in binary data */
    if ( fread(raw,sizeof(float), 2*totalmapsize, rawfile) != 2*totalmapsize )
	exitm(strcat(argv[0],": map file read error"));
	
    /* mslice data compressed, convert to standard format */
    if(MSLICE)
    {
      for ( z=0 ; z<zres ; z++ )
	for ( y=0 ; y<yres ; y++ )
	    for ( x=0 ; x<xres ; x++ )
            {
              i = ((y*zres*xres)+(z*xres)+x)*2;
	      fraw[cmapindex(z,y,x)] = raw[i];
	      fraw[cmapindex(z,y,x)+1] = raw[i+1];
	    }     
    }   
	
    /* check file size and apply gaussian filter */
    /* gauss.h contains a 256 point, gaussian array, _step is resolution */ 

    if (yres == 32)
    	ystep = 8;
    else if (yres == 64)
            ystep = 4;
    else if (yres == 128)
        ystep = 2;
    else
        exitm(strcat(argv[0],": Illegal size (phase)"));
    if (xres == 32)
    	xstep = 8;
    else if (xres == 64)
            xstep = 4;
    else if (xres == 128)
        xstep = 2;
    else
        exitm(strcat(argv[0],": Illegal size (read)"));

    zs = 0; ys = 0; xs = 0;		/* starting point of filter array */    	
    
    /* apply gaussian filter and phase alternate and convert to float*/
    if (gfilter) 
    { 
      for ( z=0,zi=zs ; z<zres ; z++, zi=zi+zstep ) 	
	for ( y=0,yi=ys ; y<yres ; y++, yi=yi+ystep  )
	{
	    for ( x=0,xi=xs ; x<xres ; x++, xi=xi+xstep )
	    {
	        /****
	        printf("z=%6.4f, y=%6.4f, x=%6.4f\n",gf[zi],gf[yi],gf[xi]);
	        printf("zi=%d,zs=%d,yi=%d,ys=%d,xi=%d,xs=%d\n",zi,zs,yi,ys,xi,xs);
	        printf("zstep=%d,ystep=%d,xstep=%d\n",zstep,ystep,xstep);
	        ***/
		fraw[cmapindex(z,y,x)] = 
		    (1-2*((y+x)%2))*gf[yi]*gf[xi]*(fraw[cmapindex(z,y,x)]);
		fraw[cmapindex(z,y,x)+1] = 
		    (1-2*((y+x)%2))*gf[yi]*gf[xi]*(fraw[cmapindex(z,y,x)+1]);
	    }
	}
    }
    else 
    {
      /* alternate phase and float */
      for ( z=0 ; z<zres ; z++ )
	for ( y=0 ; y<yres ; y++ )
	    for ( x=0 ; x<xres ; x++ )
	    {
		fraw[cmapindex(z,y,x)] = 
		    (1-2*((y+x)%2))*fraw[cmapindex(z,y,x)];
		fraw[cmapindex(z,y,x)+1] = 
		    (1-2*((y+x)%2))*fraw[cmapindex(z,y,x)+1];
	    }
    }	    

    for ( z=0 ; z<zres ; z++ )
    {
        ptr = z*imgsize*2; /* pointer to next slice */
        for(i=0; i<imgsize*2; i++)
          ftbuf[i] = fraw[ptr+i];  
        fourn(ftbuf-1, mapsize, 2, -1);  /* 2D FFT on each slice*/
        for(i=0; i<imgsize*2; i++)
          fraw[ptr+i] = ftbuf[i];        
        /* dc correction */    
	for ( y=0 ; y<yres ; y++ )
	{
	    for ( x=0 ; x<xres ; x++ )
	    {
		fraw[cmapindex(z,y,x)] *= (1.0-2.0*((y+x)%2))*scale;
		fraw[cmapindex(z,y,x)+1] *= (1.0-2.0*((y+x)%2))*scale;			
	    }
	}
    }
    /* swap pe2 dimension */

       for ( z=0 ; z<zres ; z++ )
	for ( y=0 ; y<yres ; y++ )
	    for ( x=0 ; x<xres ; x++ )
            {
              i = ((z*yres*xres)+((yres-1-y)*xres)+x)*2;
	      buf[cmapindex(z,y,x)] = fraw[i];
	      buf[cmapindex(z,y,x)+1] = fraw[i+1];
	    }    
                  
    /* write out the FT'ed complex data */
    fwrite(buf,sizeof(float),2*totalmapsize,phasefile);

    /* process echo #2 */
    /* read in binary data */
    if ( fread(raw2,sizeof(float), 2*totalmapsize, rawfile2) != 2*totalmapsize )
	exitm(strcat(argv[0],": map file read error"));
	
    /* mslice data compressed, convert to standard format */
    if(MSLICE)
    {
      for ( z=0 ; z<zres ; z++ )
	for ( y=0 ; y<yres ; y++ )
	    for ( x=0 ; x<xres ; x++ )
            {
              i = ((y*zres*xres)+(z*xres)+x)*2;
	      fraw2[cmapindex(z,y,x)] = raw2[i];
	      fraw2[cmapindex(z,y,x)+1] = raw2[i+1];
	    }     
    }   
	
    /* check file size and apply gaussian filter */
    /* gauss.h contains a 256 point, gaussian array, _step is resolution */ 

    if (yres == 32)
    	ystep = 8;
    else if (yres == 64)
            ystep = 4;
    else if (yres == 128)
        ystep = 2;
    else
        exitm(strcat(argv[0],": Illegal size (phase)"));
    if (xres == 32)
    	xstep = 8;
    else if (xres == 64)
            xstep = 4;
    else if (xres == 128)
        xstep = 2;
    else
        exitm(strcat(argv[0],": Illegal size (read)"));

    zs = 0; ys = 0; xs = 0;		/* starting point of filter array */    	
    
    /* apply gaussian filter and phase alternate and convert to float*/
    if (gfilter) 
    { 
      for ( z=0,zi=zs ; z<zres ; z++, zi=zi+zstep ) 	
	for ( y=0,yi=ys ; y<yres ; y++, yi=yi+ystep  )
	{
	    for ( x=0,xi=xs ; x<xres ; x++, xi=xi+xstep )
	    {
	        /****
	        printf("z=%6.4f, y=%6.4f, x=%6.4f\n",gf[zi],gf[yi],gf[xi]);
	        printf("zi=%d,zs=%d,yi=%d,ys=%d,xi=%d,xs=%d\n",zi,zs,yi,ys,xi,xs);
	        printf("zstep=%d,ystep=%d,xstep=%d\n",zstep,ystep,xstep);
	        ***/
		fraw2[cmapindex(z,y,x)] = 
		    (1-2*((y+x)%2))*gf[yi]*gf[xi]*(fraw2[cmapindex(z,y,x)]);
		fraw2[cmapindex(z,y,x)+1] = 
		    (1-2*((y+x)%2))*gf[yi]*gf[xi]*(fraw2[cmapindex(z,y,x)+1]);
	    }
	}
    }
    else 
    {
      /* alternate phase and float */
      for ( z=0 ; z<zres ; z++ )
	for ( y=0 ; y<yres ; y++ )
	    for ( x=0 ; x<xres ; x++ )
	    {
		fraw2[cmapindex(z,y,x)] = 
		    (1-2*((y+x)%2))*fraw2[cmapindex(z,y,x)];
		fraw2[cmapindex(z,y,x)+1] = 
		    (1-2*((y+x)%2))*fraw2[cmapindex(z,y,x)+1];
	    }
    }	    

    for ( z=0 ; z<zres ; z++ )
    {
        ptr = z*imgsize*2; /* pointer to next slice */
        for(i=0; i<imgsize*2; i++)
          ftbuf2[i] = fraw2[ptr+i];  
        fourn(ftbuf2-1, mapsize, 2, -1);  /* 2D FFT on each slice*/
        for(i=0; i<imgsize*2; i++)
          fraw2[ptr+i] = ftbuf2[i];        
        /* dc correction */    
	for ( y=0 ; y<yres ; y++ )
	{
	    for ( x=0 ; x<xres ; x++ )
	    {
		fraw2[cmapindex(z,y,x)] *= (1.0-2.0*((y+x)%2))*scale;
		fraw2[cmapindex(z,y,x)+1] *= (1.0-2.0*((y+x)%2))*scale;			
	    }
	}
    }

       for ( z=0 ; z<zres ; z++ )
	for ( y=0 ; y<yres ; y++ )
	    for ( x=0 ; x<xres ; x++ )
            {
              i = ((z*yres*xres)+((yres-1-y)*xres)+x)*2;
	      buf[cmapindex(z,y,x)] = fraw2[i];
	      buf[cmapindex(z,y,x)+1] = fraw2[i+1];
	    }    
	                     
    /* write out the FT'ed complex data */
    fwrite(buf,sizeof(float),2*totalmapsize,phasefile2);

    /* process echo #3 */
    /* read in binary data */
    if ( fread(raw3,sizeof(float), 2*totalmapsize, rawfile3) != 2*totalmapsize )
	exitm(strcat(argv[0],": map file read error"));
	
    /* mslice data compressed, convert to standard format */
    if(MSLICE)
    {
      for ( z=0 ; z<zres ; z++ )
	for ( y=0 ; y<yres ; y++ )
	    for ( x=0 ; x<xres ; x++ )
            {
              i = ((y*zres*xres)+(z*xres)+x)*2;
	      fraw3[cmapindex(z,y,x)] = raw3[i];
	      fraw3[cmapindex(z,y,x)+1] = raw3[i+1];
	    }     
    }   
	
    /* check file size and apply gaussian filter */
    /* gauss.h contains a 256 point, gaussian array, _step is resolution */ 

    if (yres == 32)
    	ystep = 8;
    else if (yres == 64)
            ystep = 4;
    else if (yres == 128)
        ystep = 2;
    else
        exitm(strcat(argv[0],": Illegal size (phase)"));
    if (xres == 32)
    	xstep = 8;
    else if (xres == 64)
            xstep = 4;
    else if (xres == 128)
        xstep = 2;
    else
        exitm(strcat(argv[0],": Illegal size (read)"));

    zs = 0; ys = 0; xs = 0;		/* starting point of filter array */    	
    
    /* apply gaussian filter and phase alternate and convert to float*/
    if (gfilter) 
    { 
      for ( z=0,zi=zs ; z<zres ; z++, zi=zi+zstep ) 	
	for ( y=0,yi=ys ; y<yres ; y++, yi=yi+ystep  )
	{
	    for ( x=0,xi=xs ; x<xres ; x++, xi=xi+xstep )
	    {
	        /****
	        printf("z=%6.4f, y=%6.4f, x=%6.4f\n",gf[zi],gf[yi],gf[xi]);
	        printf("zi=%d,zs=%d,yi=%d,ys=%d,xi=%d,xs=%d\n",zi,zs,yi,ys,xi,xs);
	        printf("zstep=%d,ystep=%d,xstep=%d\n",zstep,ystep,xstep);
	        ***/
		fraw3[cmapindex(z,y,x)] = 
		    (1-2*((y+x)%2))*gf[yi]*gf[xi]*(fraw3[cmapindex(z,y,x)]);
		fraw3[cmapindex(z,y,x)+1] = 
		    (1-2*((y+x)%2))*gf[yi]*gf[xi]*(fraw3[cmapindex(z,y,x)+1]);
	    }
	}
    }
    else 
    {
      /* alternate phase and float */
      for ( z=0 ; z<zres ; z++ )
	for ( y=0 ; y<yres ; y++ )
	    for ( x=0 ; x<xres ; x++ )
	    {
		fraw3[cmapindex(z,y,x)] = 
		    (1-2*((y+x)%2))*fraw3[cmapindex(z,y,x)];
		fraw3[cmapindex(z,y,x)+1] = 
		    (1-2*((y+x)%2))*fraw3[cmapindex(z,y,x)+1];
	    }
    }	    

    for ( z=0 ; z<zres ; z++ )
    {
        ptr = z*imgsize*2; /* pointer to next slice */
        for(i=0; i<imgsize*2; i++)
          ftbuf3[i] = fraw3[ptr+i];  
        fourn(ftbuf3-1, mapsize, 2, -1);  /* 2D FFT on each slice*/
        for(i=0; i<imgsize*2; i++)
          fraw3[ptr+i] = ftbuf3[i];        
        /* dc correction */    
	for ( y=0 ; y<yres ; y++ )
	{
	    for ( x=0 ; x<xres ; x++ )
	    {
		fraw3[cmapindex(z,y,x)] *= (1.0-2.0*((y+x)%2))*scale;
		fraw3[cmapindex(z,y,x)+1] *= (1.0-2.0*((y+x)%2))*scale;			
	    }
	}
    }
       for ( z=0 ; z<zres ; z++ )
	for ( y=0 ; y<yres ; y++ )
	    for ( x=0 ; x<xres ; x++ )
            {
              i = ((z*yres*xres)+((yres-1-y)*xres)+x)*2;
	      buf[cmapindex(z,y,x)] = fraw3[i];
	      buf[cmapindex(z,y,x)+1] = fraw3[i+1];
	    }    
	                     
    /* write out the FT'ed complex data */
    fwrite(buf,sizeof(float),2*totalmapsize,phasefile3);
        
}

/******************************************************************************
                        Modification History


******************************************************************************/

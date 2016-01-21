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

TITLE: gsreformat
READ AS: Gradient shimming - Reformat fielmap 

**********************************************************************
PURPOSE: The fieldmap and image data is reformatted to the shimmap image

DETAILS: The input fieldmap and image data are remapped onto the shimmap grid
	 i.e. the field of view of input image same as shimmap image.
	 Input images are concatenated multi-slice images. The slice
	 dimension is defined by the slice positions. The slice thickness
	 is not taken into account, ie. slice is represented as a singl
	 point along z dimension.
	 
	 For each fieldmap point the closest shimmap point is "marked".
	 Each shimmap point may be marked by multiple fieldmap points.
	 The closest fieldmap point is chosen by keeping track of the
	 deviation in x, y and z directions. If the field map point is
	 outside the shimmap FOV it is ignored.
	 
	 For multi-slice data, smapsize is set to 1 (or 0) and the FOV
	 along the slice dimension is set to 0.
	 	 	 	          
USAGE:   gsmapmask mapname(.f,.mag,.param)  shimmap(.param) -mapname refers to shimmap

INPUT:   mapname.f,mapname.mag, mapname.param, shimmap.param

OUTPUT:  mapname{.f,.mag}.r1, mapname.mask.r1   - fieldmap or image shim ROI mask
 
LIMITATIONS: 

AUTHOR: S. Sukumar

**********************************************************************/
#include	<stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include	<math.h>
#include	"util.h"

/*
void obl_matrix(float ang1,float ang2,float ang3,
                double *tm11,double *tm12,double *tm13,
                double *tm21,double *tm22,double *tm23,
                double *tm31,double *tm32,double *tm33);                
*/
		
/* indexing calculations */
#define 	refindex(xx,yy,zz) (xx*yrefsize*zrefsize+yy*zrefsize+zz)
#define 	mapindex(s,p,r) (s*pmapsize*rmapsize+p*rmapsize+r)

#define		TWOPI		6.28318531
#define 	PI		3.14159265
#define D2R(x)		x*3.14159/180.0
#define EPS	1.0e-12
    
/* I/O string */
char		str[80];
    
main(argc,argv)
int 	argc;
char	*argv[];
{
    FILE	*fieldfile;			/* input field map, B0.f */
    FILE	*rfieldfile;			/* reformatted field, output */
    FILE	*magfile,*rmagfile;  		/* input and output magnitude file */
    FILE	*refparamsfile,*map_paramsfile; /* shimmap and B0 paramsfile */
    char	refname[80],mapname[80];
    int		args;
    int		xrefsize,yrefsize,zrefsize;
    float	xreffov,yreffov,zreffov;
    float	xrefres,yrefres,zrefres;
    int		rmapsize,pmapsize,smapsize;	/* size of field maps */
    float	rmapfov,pmapfov,smapfov;	/* FOV of field map image,x/y/z */    
    float	roffset,poffset,soffset;	/* image offsets; pro,ppe,ppe2(?) */
    float	mapdelay,threshparam,minmapdelay;  /* delays, threshold */
    char	orient[16];			/* orient*/
    float	psi,phi,theta;			/* Euler angles */
    int		totalmapsize,totalrefsize;	/* size of fieldmap,shimmap */
    float	*fieldin,*rfieldout;		/* buffer for field data */
    float	*rfieldvar,*magin,*rmagout; 	/* buffer */
    int		i,j,x,y,z,zz,yy,xx,r,p,s;
    
    float	rmapoff,pmapoff,smapoff;
    
    float	minrefres,rres,pres,sres;
    int		slices;
    float	thk,pss[32];
    int		multislice,skip;
    double 	m11,m12,m13,m21,m22,m23,m31,m32,m33;
    float	rval,pval,sval,xval,yval,zval;
    float	xpt,ypt,zpt,xfrac,yfrac,zfrac;
    float	maxvar;

    char	prgm[16];
    
    strcpy(prgm,"gsreformat");    
     /* usage */     
    checkargs(argv[0],argc,"rootfilename");
      
     /***** process arguments *****/
    args = 1;
    /* open the field map phase, magnitude and parameter files */
    strcpy(mapname,argv[args]);
    rfieldfile = efopen(argv[0],strcat(mapname,".f.r1"),"w"); /* output => B0.f.r1 */
    strcpy(mapname,argv[args]);
    rmagfile = efopen(argv[0],strcat(mapname,".mag.r1"),"w"); /* output => B0.mag.r1 */    
    strcpy(mapname,argv[args]); 
    fieldfile = efopen(argv[0],strcat(mapname,".f"),"r");
    strcpy(mapname,argv[args]); 
    magfile = efopen(argv[0],strcat(mapname,".mag"),"r");    
    strcpy(mapname,argv[args]);
    map_paramsfile = efopen(argv[0],strcat(mapname,".param"),"r");
    strcpy(mapname,argv[args++]);
     
    strcpy(refname,argv[args]);
    refparamsfile = efopen(argv[0],strcat(refname,".param"),"r");
    
    /* open the fielmap parameter file and read parameters */
    map_paramsfile = efopen(argv[0],strcat(mapname,".param"),"r");    
    efgets(str,80,map_paramsfile);
    sscanf(str,"%d %d %d",&rmapsize,&pmapsize,&smapsize);  /* r,p,s size */
    efgets(str,80,map_paramsfile);  
    sscanf(str,"%f %f %f",&rmapfov,&pmapfov,&smapfov);  /* r,p,s FOV(mm) */   
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f",&mapdelay);     /* phase delay, del */ 
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f",&threshparam);  /* threshold */ 
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f",&minmapdelay);  /* reference phase delay, delref */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%s %f %f %f",orient,&psi,&phi,&theta);  /* orientation */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f %f %f",&rmapoff,&pmapoff,&smapoff);  /* pro,ppe,ppe2 */
    /* read in slice info */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f",&thk);       
    efgets(str,80,map_paramsfile);
    sscanf(str,"%d",&slices);
    
    if(slices > 32) 
    {
      	fprintf(stderr,"\n slices=%d; slices must be <32\n",slices);
	exit(1);
    }
    else if(slices < 3)
    {
        fprintf(stderr,"\n slices=%d; slices must be >2\n",slices);
	exit(1);
    }        
    for ( i=0 ; i<slices; i++ )   
    {
        efgets(str,80,map_paramsfile);   /* read in slice positions */
        sscanf(str,"%f ",&pss[i]);
    }  
     
    multislice=0;
    if ( (smapsize==0) || (smapsize==1) )
    {
        smapsize = slices;  /* size of slice dimension */
        multislice=1;
    }   
    else
    {
      fprintf(stderr,"\n Invalid slice dimension parameters");
      exit(1);
    }
 
    /* read the reference map parameters; sagittal */
    efgets(str,80,refparamsfile);
    sscanf(str,"%d %d %d",&zrefsize,&yrefsize,&xrefsize);  /* r,p,s */
    /* field of view of reference map data */
    efgets(str,80,refparamsfile);
    sscanf(str,"%f %f %f",&zreffov,&yreffov,&xreffov);
                           
    /* calculate final array sizes */
    totalmapsize = rmapsize*pmapsize*smapsize;  /* fieldmap/image file size */
    totalrefsize = zrefsize*yrefsize*xrefsize;  /* shimmap/image file size */
    
    /* allocate data space for data */
    fieldin = (float *) calloc((unsigned)(totalmapsize),sizeof(float)); /* input field */
    magin = (float *) calloc((unsigned)(totalmapsize),sizeof(float));  /* input magnitude */
    /* allocate space for reformatted 3D fieldmap */
    rfieldvar = (float *) calloc((unsigned)(totalrefsize),sizeof(float)); /* buffer */
    rfieldout = (float *) calloc((unsigned)(totalrefsize),sizeof(float));  /* output field */
    rmagout = (float *) calloc((unsigned)(totalrefsize),sizeof(float));  /* output image */
    /* read in B0 map data */
    fread(fieldin,sizeof(float),(totalmapsize),fieldfile);
    fread(magin,sizeof(float),(totalmapsize),magfile);
  
    for(i=0; i<totalrefsize; i++)   /* buffer saves positional variation info */
      rfieldvar[i] = 100.0;       
    
    rres = rmapfov/(rmapsize-1);  /* fieldmap resolution, mm/pt */
    pres = pmapfov/(pmapsize-1);
    if(!multislice)
      sres = smapfov/(smapsize-1);
    
    xrefres = xreffov/(xrefsize-1);  /* shimmap resolution mm/pt */
    yrefres = yreffov/(yrefsize-1);
    zrefres = zreffov/(zrefsize-1);
       
    theta= -theta;  /* correction in directional cosines */    
    obl_matrix(psi,phi,theta, &m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33); /* init constants */
   
    skip=0;             
    /* go through each map point and find x,y,z value */    
    for(s=0; s<smapsize; s++)   /* if multi-slice, no of slices */
      for(p=0; p<pmapsize; p++)
        for(r=0; r<rmapsize; r++)
        { 

          rval = -(rmapfov/2.0) + (r*rres) + rmapoff;  
          pval = -(pmapfov/2.0) + (p*pres) + pmapoff;
          if(multislice)
            sval = pss[s];
          else
            sval = -(smapfov/2.0) + (s*sres) - smapoff;                    
                 
          /*Transform logical gradient levels to magnet frame****/
          xval = rval*m11 + pval*m12 + sval*m13;
          yval = rval*m21 + pval*m22 + sval*m23;
          zval = rval*m31 + pval*m32 + sval*m33;
        
          /* determine the position (points) */
          xpt = (-xval/xrefres) + (xrefsize/2.0);
          ypt = (-yval/yrefres) + (yrefsize/2.0);
          zpt = (-zval/zrefres) + (zrefsize/2.0);
           
          xx = (int)(xpt);
          yy = (int)(ypt);
          zz = (int)(zpt);
          /* find the closest point by checking the fraction */
          if(xfrac >= 0.5)   
          {
            xx = xx+1;
            xfrac = 1-xfrac;
          }
          if(yfrac >= 0.5)
          {
            yy = yy+1;
            yfrac = 1-yfrac;
          }
          if(zfrac >= 0.5)
          {
            zz = zz+1;
            zfrac = 1-zfrac;
          }
        
          /* if map point outside shimmap fov, skip */
          if(xx < 0) skip=1;
          if(xx > xrefsize-1) skip=1;
          if(yy < 0) skip=1;
          if(yy > yrefsize-1) skip=1;
          if(zz < 0) skip=1;
          if(zz > zrefsize-1) skip=1;
     
        /* find the matching point in the shimmap data matrix */
        maxvar = xfrac + yfrac + zfrac;  /* fractional deviation */
        if(!skip)
        {        
          if(rfieldvar[refindex(xx,yy,zz)] > maxvar)
          {
            rfieldvar[refindex(xx,yy,zz)] = maxvar;
            rfieldout[refindex(xx,yy,zz)] = fieldin[mapindex(s,p,r)]; 
            rmagout[refindex(xx,yy,zz)] = magin[mapindex(s,p,r)];
                        
            if(zz==26)
              fprintf(stderr,"\n %s %f",prgm,magin[mapindex(s,p,r)]);
          }
        }
        skip=0;
      }  /* go to next map point */              
      
/**
fprintf(stderr,"\n %s %d: 1=%d 2=%d 3=%d; ",prgm,skip,xx,yy,zz);
fprintf(stderr,"\n 1=%4.2f 2=%4.2f 3=%4.2f; xxx",pp[0],pp[1],pp[2]);    
fprintf(stderr,"\n 1=%d 2=%d 3=%d; xxx",zz,yy,xx);
exit(1);
**/          
                  
    fwrite(rfieldout,sizeof(float),totalrefsize,rfieldfile);  /* write output files */
    fwrite(rmagout,sizeof(float),totalrefsize,rmagfile);
    free(rfieldout);       
    free(fieldin);
    free(rfieldvar);
    free(magin);
    free(rmagout);              	
}

        /*******************************************************
                                obl_matrix()

                Procedure to provide the elements of the
                logical to magnet gradient transform matrix
        ********************************************************/

obl_matrix(ang1,ang2,ang3,tm11,tm12,tm13,tm21,tm22,tm23,tm31,tm32,tm33)
double ang1,ang2,ang3;
double *tm11,*tm12,*tm13,*tm21,*tm22,*tm23,*tm31,*tm32,*tm33;
{
    double D_R;
    double sinang1,cosang1,sinang2,cosang2,sinang3,cosang3;
    double m11,m12,m13,m21,m22,m23,m31,m32,m33;
    double im11,im12,im13,im21,im22,im23,im31,im32,im33;
    double tol = 1.0e-14;

    /* Convert the input to the basic mag_log matrix***************/
    D_R = M_PI / 180;

    cosang1 = cos(D_R*ang1);
    sinang1 = sin(D_R*ang1);
        
    cosang2 = cos(D_R*ang2);
    sinang2 = sin(D_R*ang2);
        
    cosang3 = cos(D_R*ang3);
    sinang3 = sin(D_R*ang3);

    m11 = (sinang2*cosang1 - cosang2*cosang3*sinang1);
    m12 = (-1.0*sinang2*sinang1 - cosang2*cosang3*cosang1);
    m13 = (sinang3*cosang2);

    m21 = (-1.0*cosang2*cosang1 - sinang2*cosang3*sinang1);
    m22 = (cosang2*sinang1 - sinang2*cosang3*cosang1);
    m23 = (sinang3*sinang2);

    m31 = (sinang1*sinang3);
    m32 = (cosang1*sinang3);
    m33 = (cosang3);

    if (fabs(m11) < tol) m11 = 0;
    if (fabs(m12) < tol) m12 = 0;
    if (fabs(m13) < tol) m13 = 0;
    if (fabs(m21) < tol) m21 = 0;
    if (fabs(m22) < tol) m22 = 0;
    if (fabs(m23) < tol) m23 = 0;
    if (fabs(m31) < tol) m31 = 0;
    if (fabs(m32) < tol) m32 = 0;
    if (fabs(m33) < tol) m33 = 0;

    /* Generate the transform matrix for mag_log ******************/

    /*HEAD SUPINE*/
        im11 = m11;       im12 = m12;       im13 = m13;
        im21 = m21;       im22 = m22;       im23 = m23;
        im31 = m31;       im32 = m32;       im33 = m33;

    /*Transpose intermediate matrix and return***********/
    *tm11 = im11;     *tm21 = im12;     *tm31 = im13;
    *tm12 = im21;     *tm22 = im22;     *tm32 = im23;
    *tm13 = im31;     *tm23 = im32;     *tm33 = im33;
}


/*****************************************************************
		Modification History
		


******************************************************************/

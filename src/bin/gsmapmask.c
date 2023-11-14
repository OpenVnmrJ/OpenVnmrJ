/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*******************************************************************

TITLE: gsmapmask
READ AS: Gradient shimming - Generate map mask for shim ROI

**********************************************************************
PURPOSE: The shim ROI mask is generated from the magnitude image

DETAILS: The voxel/slab information is used to generate the shim ROI mask.
	 Regions outside the voxel/slab region is zeroed in the magnitude image. 
	 The input fieldmap data is assumed to be remapped onto the shimmap grid
	 i.e. the field of view of input image same as shimmap image.
	 A mask file, B0.mag.mask.r2 is also generated. The shim ROI region is
	 set to 1 in the mask image.
	 
	 	          
USAGE:   gsmapmask mapname mapname(.param) shimroi(.param)     -roiname refers to shimmap

INPUT:   mapname.f (or mapname.mag), mapname<.param>, shimroi<.param>

OUTPUT:  mapname(.f or.mag).r2, mapname.mask.r2   - fieldmap or image shim ROI mask
 
LIMITATIONS: 

AUTHOR: S. Sukumar

**********************************************************************/
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>
#include	"util.h"

/*
void obl_matrix(float ang1,float ang2,float ang3,
                double *tm11,double *tm12,double *tm13,
                double *tm21,double *tm22,double *tm23,
                double *tm31,double *tm32,double *tm33);                
*/
		
/* indexing calculations */
#define 	refindex(xx,yy,zz) (xx*ymapsize*zmapsize+yy*zmapsize+zz)

#define 	PI              3.14159265358979323846
#define		TWOPI           (2*PI)
#define         D2R(x)          (x*PI)/180.0
#define         EPS             1.0e-12
    
/* I/O string */
char		str[80];
    
main(argc,argv)
int 	argc;
char	*argv[];
{
    FILE	*fieldfile,*rfieldfile,*rmaskfile;
    FILE	*roiparamsfile,*map_paramsfile;
    char	roiname[80],mapname[80],mapname2[80];
    int		args;
    int		zmapsize,ymapsize,xmapsize;	/* size of field maps */
    float	zmapfov,ymapfov,xmapfov;	/* FOV of field map image,x/y/z */    
    float	zmapres,ymapres,xmapres;	/* resolution mm/pt */
    float	roffset,poffset,soffset;	/* image offsets */
    float	pos1,pos2,pos3;			/* voxel position */
    float	vox1,vox2,vox3;			/* voxel dimension */
    float	vphi,vpsi,vtheta;		/* voxel orient*/ 
    float	mapdelay,threshparam,minmapdelay;  /* delays, threshold */
    char	orient[16],vorient[16];		/* orient*/
    char	shimregion[16];
    float	psi,phi,theta;			/* Euler angles */
    int		totalmapsize;
    float	*rfield,*fieldin,*rmask;
    int		i,j,x,y,z,zz,yy,xx,r,p,s; 
    float	rmapoff,pmapoff,smapoff;
    
    float	minrefres,rres,pres,sres;
    int		vox1sz,vox2sz,vox3sz,slices;
    float	thk,pss[32];

    double 	m11,m12,m13,m21,m22,m23,m31,m32,m33;
    double      level1,level2,level3;
    float       xlevel,ylevel,zlevel;
    
     /* usage */     
    checkargs(argv[0],argc,"rootfilename");
      
     /***** process arguments *****/
    args = 1;
    /* open the field map phase files, frequency file, and parameter files */
    strcpy(mapname,argv[args]);
    rfieldfile = efopen(argv[0],strcat(mapname,".r2"),"w"); /* output => B0.mag.r2 */
    strcpy(mapname,argv[args]); 
    rmaskfile = efopen(argv[0],strcat(mapname,".r2.mask"),"w"); /* output => B0.mag.r2.mask */
    strcpy(mapname,argv[args]); 
    fieldfile = efopen(argv[0],mapname,"r");
    strcpy(mapname,argv[args++]);
    
    /* open fieldmap/image parameter file */  
    strcpy(mapname2,argv[args]);
    map_paramsfile = efopen(argv[0],strcat(mapname2,".param"),"r");
    strcpy(mapname2,argv[args++]); 

    /* open the shim ROI parameter files */
    strcpy(roiname,argv[args]);
    roiparamsfile = efopen(argv[0],strcat(roiname,".param"),"r");
    
    /* read field map size */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%d %d %d",&zmapsize,&ymapsize,&xmapsize);
    /* field of view of field map data */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f %f %f",&zmapfov,&ymapfov,&xmapfov);    

    /* read shim roi info */
    efgets(str,80,roiparamsfile);
    sscanf(str,"%s",shimregion);   /* voxel or slab shim ROI */
    efgets(str,80,roiparamsfile);
    sscanf(str,"%s %f %f %f",vorient,&vpsi,&vphi,&vtheta);  
    efgets(str,80,roiparamsfile);
    sscanf(str,"%f %f %f",&pos1,&pos2,&pos3);
    efgets(str,80,roiparamsfile);
    sscanf(str,"%f %f %f",&vox1,&vox2,&vox3);  
      
    /* calculate final array sizes */
    totalmapsize = zmapsize*ymapsize*xmapsize;  /* fieldmap/image file size */
    
    /* allocate data space for input data, B0 map */
    fieldin = (float *) calloc((unsigned)(totalmapsize),sizeof(float));
    /* allocate space for shim-roi mask */
    rfield = (float *) calloc((unsigned)(totalmapsize),sizeof(float));
    rmask = (float *) calloc((unsigned)(totalmapsize),sizeof(float));
    /* read in B0 map data */
    fread(fieldin,sizeof(float),(totalmapsize),fieldfile);

    /* find shimmap resolution */
    minrefres = zmapfov/zmapsize;
    if(minrefres > ymapfov/ymapsize) 
      minrefres = ymapfov/ymapsize;
    if(minrefres > xmapfov/xmapsize) 
      minrefres = xmapfov/xmapsize;
      
    minrefres = minrefres/2.0;    /* double the resolution */  
    
    /* determine voxel dimension size */
    vox1sz = (int)((vox1/minrefres)+0.5);
    vox2sz = (int)((vox2/minrefres)+0.5);
    vox3sz = (int)((vox3/minrefres)+0.5);

    if (((vox1sz < 3) || (vox2sz < 3)) || (vox3sz < 3))
    {
        fprintf(stderr,"\n vox1sz=%d vox2sz=%d vox3sz=%d; voxel size too small",vox1sz,vox2sz,vox3sz);
        exit(1);
    }      
    
    rres = vox1/(vox1sz-1);  /* voxel resolution, mm/pt */
    pres = vox2/(vox2sz-1);
    sres = vox3/(vox3sz-1);
    xmapres = xmapfov/(xmapsize-1);  /* shimmap resolution mm/pt */
    ymapres = ymapfov/(ymapsize-1);
    zmapres = zmapfov/(zmapsize-1);
        
    vtheta= -vtheta;      
    obl_matrix(vpsi,vphi,vtheta, &m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33); /* init constants */
                  
    /* go through each voxel point and find x,y,z value */    
    for(s=0; s<vox3sz; s++)
      for(p=0; p<vox2sz; p++)
        for(r=0; r<vox1sz; r++)
        { 

          level1 = -(vox1/2.0) + (r*rres) + pos1;  
          level2 = -(vox2/2.0) + (p*pres) + pos2;
          level3 = -(vox3/2.0) + (s*sres) - pos3;
                 
          /*Transform logical gradient levels to magnet frame****/
          xlevel = level1*m11 + level2*m12 + level3*m13;
          ylevel = level1*m21 + level2*m22 + level3*m23;
          zlevel = level1*m31 + level2*m32 + level3*m33;
                
     /********check sign for FOV */
          /* shimmap is a sagittal image */
          zz = (int)((zlevel/zmapres)+(zmapsize/2.0));
          yy = (int)((ylevel/ymapres)+(ymapsize/2.0));
          xx = (int)((xlevel/xmapres)+(xmapsize/2.0));
/**
fprintf(stderr,"\n 1=%4.2f 2=%4.2f 3=%4.2f; xxx",pp[0],pp[1],pp[2]);    
fprintf(stderr,"\n 1=%d 2=%d 3=%d; xxx",zz,yy,xx);
exit(1);
**/          
          if(zz < 0) zz = 0;
          if(zz > zmapsize-1) zz = zmapsize-1;
          if(yy < 0) yy = 0;
          if(yy > ymapsize-1) yy = ymapsize-1;
          if(xx < 0) xx = 0;
          if(xx > xmapsize-1) xx = xmapsize-1;
          
          rfield[refindex(xx,yy,zz)] = fieldin[refindex(xx,yy,zz)];  /* shimroi copied */
          rmask[refindex(xx,yy,zz)] = 1.0;   /* shimroi mask */                           
        }
                  
    fwrite(rfield,sizeof(float),totalmapsize,rfieldfile);  /* write output files */
    fwrite(rmask,sizeof(float),totalmapsize,rmaskfile);
    free(rfield);       
    free(fieldin);
    free(rmask);
               	
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

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

TITLE: gsregrid
READ AS: Gradient shimming - regrid slice maps onto 3D map

**********************************************************************
PURPOSE: To remap the slice field maps onto a 3D map for gshimcalc

DETAILS: The input file name.f is the multi-slice field map 
	 The slices can be trans, sag, cor, or 3orthogonal.
	 The input file contains the data from all slices contcatenated.
	 The input images can have any fov or matrix. It is assumed
	 that the images are taken from a gradient echo sequence.
	 The images will then be mapped onto a smaller (e.g. 64x32x32
	 matrix corresponding to the shimmap.f image.
	 Slice orientation must be trans, cor, or sag.
         Currently only orient = 'axial' is supported.
         Usually shimmap.param is a link to calib/shimmap....param file 
         
USAGE:   gsregrid mapname shimmap 

INPUT:   Field file mapname.{f,mag,param} shimmap.param

OUTPUT:  B0sl.3d.f B0sl.3.mag  - 3D images
 
LIMITATIONS: Currently, only orient='3orthogonal' is implemented

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
char		str[80];
static float puc;

int main(int argc, char *argv[])
{
    FILE	*mapparamsfile,*mapfieldfile,*mapmagfile;
    FILE	*reffieldfile,*refparamsfile,*refmagfile;
    float	*field2,*field3,*map,*buf,*mag,*magbuf,*mag3,*mag2;
    char	mapname[80],refname[80];
    int		args;
    int		totalrefsize,totalmapsize,bufsize;
    float	freq,tmp;
    float	mapdelay,minmapdelay,threshold;
    int		a,i,j,k,p,ni,ji,cnt,mcnt;
    int		p1,p2,plane,slc;
    float	jf,nf,sl,fval,mval;
    char	orient[40];
    int		slices,rmapres,pmapres,smapres,rrefres,prefres,srefres;
    int		hrefres,vrefres,arefres;
    float	rmapfov,pmapfov,smapfov,rreffov,preffov,sreffov;
    float       hreffov,vreffov,areffov;
    float	pss[32];
    float	thk,psi,phi,theta,xoffset,yoffset,zoffset;    

/* check arguments */ 
    checkargs(argv[0],argc,
	"slice_fieldmap_name shimmap_name");

    args = 1;
    /* open the field map field, magnitude, and parameter files */
    strcpy(mapname,argv[args]);
    mapfieldfile = efopen(argv[0],strcat(mapname,".f"),"r");
    strcpy(mapname,argv[args]);
    mapmagfile = efopen(argv[0],strcat(mapname,".mag"),"r");    
    strcpy(mapname,argv[args]);
    mapparamsfile = efopen(argv[0],strcat(mapname,".param"),"r");
    strcpy(mapname,argv[args]);
    reffieldfile = efopen(argv[0],strcat(mapname,".3d.f"),"w");
    strcpy(mapname,argv[args]);
    refmagfile = efopen(argv[0],strcat(mapname,".3d.mag"),"w");
    
    /* read the field map parameters */
    /* reffieldfile refers to 3D shim reference map */
    args++;
    /* open the shim reference map  parameter file */   
    strcpy(refname,argv[args]);
    refparamsfile = efopen(argv[0],strcat(refname,".param"),"r");

    /* read field map size */
    efgets(str,80,mapparamsfile);
    sscanf(str,"%d %d %d",&rmapres,&pmapres,&smapres);
    /* field of view of field map data */
    efgets(str,80,mapparamsfile);
    sscanf(str,"%f %f %f",&rmapfov,&pmapfov,&smapfov);
    /* read phase delay */
    efgets(str,80,mapparamsfile);
    sscanf(str,"%f",&mapdelay);
    efgets(str,80,mapparamsfile);
    sscanf(str,"%f",&threshold);
    efgets(str,80,mapparamsfile);
    sscanf(str,"%f",&minmapdelay);
    efgets(str,80,mapparamsfile);
    sscanf(str,"%s %f %f %f",orient,&psi,&phi,&theta);
    efgets(str,80,mapparamsfile);
    sscanf(str,"%f %f %f",&xoffset,&yoffset,&zoffset); /* r,p,s */
    efgets(str,80,mapparamsfile);
    sscanf(str,"%f",&thk);    
    efgets(str,80,mapparamsfile);
    sscanf(str,"%d",&slices);            
    for ( i=0 ; i<slices; i++ )   
    {
        efgets(str,80,mapparamsfile);
        sscanf(str,"%f ",&pss[i]);
    }


    /* read the reference map fov and matrix parameters */
    /* read reference map size */
    efgets(str,80,refparamsfile);
    sscanf(str,"%d %d %d",&rrefres,&prefres,&srefres);
    /* field of view of reference map data */
    efgets(str,80,refparamsfile);
    sscanf(str,"%f %f %f",&rreffov,&preffov,&sreffov);

    if (smapres <= 1)
    {
      smapres = slices;   /* multi-slice data */
      if (!strcmp(orient,"3orthogonal"))
          smapres = slices*3;
      else if ((!strcmp(orient,"trans"))||(!strcmp(orient,"sag"))||(!strcmp(orient,"cor")))
          smapres = slices;
      else
      {
          printf("gsregrid: Illegal orientation, %s, \n",orient);
          exit(1);
      }
    }
    else
    {
      printf("gsregrid: %s: Error, slice dimension >0; ro:pe:ss %dx%dx%d\n",mapname,rmapres,pmapres,smapres);
      exit(1);
    }

    /*
     * calculate final array sizes
     */  
    totalrefsize = rrefres*prefres*srefres;
    totalmapsize = rmapres*pmapres*smapres; 
    bufsize = rmapres*pmapres;     
    /* allocate space for the input map */
    map = (float *) calloc((unsigned)totalmapsize,sizeof(float));
    mag = (float *) calloc((unsigned)totalmapsize,sizeof(float));
    /* read in B0_slice field map and magnitude image */
    fread(map,sizeof(float),totalmapsize,mapfieldfile);
    fread(mag,sizeof(float),totalmapsize,mapmagfile);

    /*
     * allocate data space for 3D output field map
     */
    field3 = (float *) calloc((unsigned)(totalrefsize),sizeof(float));        
    mag3 = (float *) calloc((unsigned)(totalrefsize),sizeof(float));
        
    /* process slice #1 - axial, rmapres*pmapres, Y*X  */
    /* temporary 2D intermediate buffer */
    field2 = (float *) calloc((unsigned)(bufsize),sizeof(float));
    mag2 = (float *) calloc((unsigned)(bufsize),sizeof(float));
    buf = (float *) calloc((unsigned)(bufsize),sizeof(float));
    magbuf = (float *) calloc((unsigned)(bufsize),sizeof(float));

    /*
     * process
     */
   for(slc=0; slc<smapres; slc++)   /* loop through each slice */
   {          
     if (!strcmp(orient,"3orthogonal"))
       plane = slc;         /* 0=trans, 1=cor, 2=sag */
     else if (!strcmp(orient,"trans"))
       plane = 0;
     else if (!strcmp(orient,"cor"))
       plane = 1;
     else if (!strcmp(orient,"sag"))
       plane = 2;       
     else
     {
       printf("gsregrid: Illegal orientation: %s\n",orient);
       exit(1);
     }

     if (plane == 0)
     {
       hrefres = prefres;
       hreffov = preffov;
       vrefres = srefres;
       vreffov = sreffov;
       arefres = rrefres;
       areffov = rreffov;       
     }
     else if (plane == 1)
     {
       hrefres = rrefres;
       hreffov = rreffov;
       vrefres = srefres;
       vreffov = sreffov;
       arefres = prefres;
       areffov = preffov;       
     }       
     else if (plane == 2)
     {
       hrefres = rrefres;
       hreffov = rreffov;
       vrefres = prefres;
       vreffov = preffov;
       arefres = srefres;
       areffov = sreffov;       
       
     }
      
     for(i=0;i<hrefres;i++)
     {
       /*  sl = signed(+/-) fov value for i-th pt. in B0.f */
       sl = (i-hrefres/2)*hreffov/hrefres;   
       /* get the corresponding data point (0 to rmapres-1) in the slice */
       jf = ((rmapres/rmapfov)*sl) + rmapres/2;    /* fractional value */
       ji = (int)(jf+0.5);
       /* calculate the ratio of slice points to ref. map points */
       nf = (rmapres/rmapfov)*hreffov/hrefres;   /* each ref. map pt corresponds to nf slice pts */
       ni = (int)(nf+0.5);           /* no of points in slice image trace to be averaged */
       if (ni == 0)
       {
         printf("gsregrid: Error size ratio must be >= 1 %d/%d\n",rmapres,hrefres);
         exit(1);
       }   
       for(k=0;k<pmapres;k++)
       {
         cnt = 0;
         mval = 0;
         mcnt = 0;
         fval = 0;   /* field sum */
         p = (int)(jf - nf/2.0 + 0.5);  /* starting pt for averaging */
         for (a=0; a<ni; a++)      /* average slice points */
         {
           p = p+a;
           if ((p > 0)&&(p < rmapres) )  /* check boundry limits */
           {
             p2 = (slc*rmapres*pmapres);  /* offset to current slice data */
             p2 += p+(k*rmapres);
             if (map[p2] != 0)
             {
               fval += map[p2];
               cnt++;
             } 
             if (mag[p2] != 0)
             {
               mval += mag[p2];
               mcnt++;
             }                        
           }
         } 
         p1 = i+(k*hrefres);
         if(fval==0.0)
           field2[p1] = 0.0;
         else
	   field2[p1] = fval/cnt;   /* put the averaged data in output buffer */           
         if(mval==0.0)
           mag2[p1] = 0.0;
         else
	   mag2[p1] = mval/mcnt;   /* put the averaged data in output buffer */
       }
    }      
     free(map);
    
     /* for trans, loop through each lpe2 (X) point in axial plane in 3D shim ref. map */
     /* vrefres,vreffov refer to lpe2 (X) dimension in shim ref. map */
     /* in the 2D slice image, ro = y; pe = x */
     for(i=0; i<vrefres; i++)
     {
       sl = (i-vrefres/2)*vreffov/vrefres;  /*  signed(+/-) fov value for i-th pt. */
       /* get the corresponding data point (0 to pmapres-1) in the slice */
       jf = ((pmapres/pmapfov)*sl) + pmapres/2;    /* real value */
       ji = (int)(jf+0.5);
      
       /* calculate the ratio of slice points to ref. map points */
       nf = (pmapres/pmapfov)*hreffov/hrefres;   /* each ref. map pt corresponds to nf slice pts */
       ni = (int)(nf+0.5);      /* no of points in slice image trace to be averaged */
       if (ni == 0)
       {
         printf("gsregrid: size ratio error %d/%d\n",pmapres,vrefres);
         exit(1);
       }     
       for (k=0; k<hrefres; k++)      /* loop through each ro trace in (intermed) slice image */
       {
         cnt = 0;
         fval = 0;   /* field sum */
         mcnt = 0;
         mval =0;
         p = (int)(jf - nf/2.0 + 0.5);  /* starting pt for averaging along vertical dir.*/
         for (a=0; a<ni; a++)      /* average slice points */
         {
           p = p+a;
           if ((p > 0)&&(p < pmapres) )  /* check boundry limits */
           {
             p2 = (p*hrefres)+k;     /* position in intermed slice image */
             if (field2[p2] != 0)
             {
               fval += field2[p2];
               cnt++;
             }
             if (mag2[p2] != 0)
             {
               mval += mag2[p2];
               mcnt++;
             }             
           }
         }
         p1 = k+(i*hrefres);
         if(fval==0.0)
           buf[p1] = 0.0;
         else
	   buf[p1] = fval/cnt;   /* put the averaged data into buffer */
         if(mval==0.0)
           magbuf[p1] = 0.0;
         else
	   magbuf[p1] = mval/mcnt;   /* put the averaged data into buffer */	   
       }         
     }    
     if(plane==0)  
     {  /* trans; Note sign of pss changed */
       k = (int)( (((arefres-1)/areffov)*-pss[slc])+arefres/2.0 ); /* pt. corresponding to slice plane */
       /* k = arefres/2; */  /* assume pss=0, i.e. center image */
        
       for(i=0; i<vrefres; i++)
         for(j=0; j<hrefres; j++)
         {
           field3[k+(j*arefres)+(arefres*hrefres*i)]=buf[j+(i*hrefres)];
           mag3[k+(j*arefres)+(arefres*hrefres*i)]=magbuf[j+(i*hrefres)];
         }

         printf("gsregrid: arefres=%d, areffov=%f, pss[slc]=%f, \n",arefres,areffov,pss[slc]);
         printf("gsregrid: slc=%d  k=%d j=%f \n",slc,k,((arefres-1)/areffov)*pss[slc]);

           
     }
     else if(plane==1)  
     {
       k = arefres/2;    
       for(i=0; i<vrefres; i++)
         for(j=0; j<hrefres; j++)
         {
           field3[j+(k*hrefres)+(arefres*hrefres*i)]=buf[j+(i*hrefres)];  
           mag3[j+(k*hrefres)+(arefres*hrefres*i)]=magbuf[j+(i*hrefres)];
         }               
     }
     else if(plane==2)
     {
       for(i=0; i<vrefres/2; i++)   /* flip image about the pe axis */
         for(j=0; j<hrefres; j++)
         {
           tmp = buf[j+(i*hrefres)];
           buf[j+(hrefres*i)] = buf[j+(((vrefres-1)-i)*hrefres)];
           buf[j+(((vrefres-1)-i)*hrefres)] =tmp;
           tmp = magbuf[j+(i*hrefres)];
           magbuf[j+(hrefres*i)] = magbuf[j+(((vrefres-1)-i)*hrefres)];
           magbuf[j+(((vrefres-1)-i)*hrefres)] =tmp;           
         }
       k = arefres/2;    /* assume pss=0 */           
       for(i=0; i<vrefres; i++)
         for(j=0; j<hrefres; j++)
         {           
           field3[(hrefres*vrefres*k)+j+(i*hrefres)]=buf[j+(i*hrefres)];
           mag3[(hrefres*vrefres*k)+j+(i*hrefres)]=magbuf[j+(i*hrefres)];
         }
     }          
   }  /* slice loop */
     fwrite(field3,sizeof(float),(totalrefsize),reffieldfile);   
     fwrite(mag3,sizeof(float),(totalrefsize),refmagfile);                               
            
     free(field2);
     free(field3);
     free(buf);
     free(mag2);
     free(mag3);
     free(magbuf);
           
}



/*****************************************************************
		Modification History
		

******************************************************************/

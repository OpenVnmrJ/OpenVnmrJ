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

TITLE: gsremap
READ AS: Gradient shimming - remap the B0 field map data

**********************************************************************
PURPOSE: To remap (regrid) B0 field map data to match the shimmap dataset.

DETAILS: The input file name B0.f is B0 field map. The field map
	 is recalculated to match the shimmap dimension and orientation.
	 shimmap.param should point to the current shimmap file in
	 .../gshimdir/calib/shimmap.gcoil.fov.param file. 

USAGE:   gsremap mapname mapname(.param) refname(.param)     -refname refers to shimmap

INPUT:   mapname.f (or mapname.mag), mapname<.param>, shimmap<.param>

OUTPUT:  mapname(.f or.mag).r   -regridded fieldmap or image

EXAMPLE OF CALL: gsremap B0.f B0 shimmap
		 gsremap B0.mag B0 shimmap
 
LIMITATIONS: pro, ppe, ppe2 assumed to be zero. Or else we need to 
	     take into account the sign of the position information.
	     Assume pro=ppe=ppe2=0 for the shimmap data

AUTHOR: S. Sukumar

**********************************************************************/
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>
#include	"util.h"

/* indexing calculations */
#define 	refindex(xx,yy,zz) (xx*yrefres*zrefres+yy*zrefres+zz)
#define	        image(s,p,r) (s*pmapres*rmapres+p*rmapres+r)
#define 	mapindex(x,y,z) (x*ymapres*zmapres+y*zmapres+z)
#define 	map1index(x,y,zz) (x*ymapres*zrefres+y*zrefres+zz)
#define 	map2index(x,yy,zz) (x*yrefres*zrefres+yy*zrefres+zz)

#define 	PI              3.14159265358979323846
#define		TWOPI           (2*PI)

/* I/O string */
char		str[80];

int main(int argc, char *argv[])
{
    FILE	*fieldfile,*rfieldfile;
    FILE	*refparamsfile,*map_paramsfile;
    char	refname[80],mapname[80],mapname2[80];
    int		args;
    int		rmapres,pmapres,smapres;	/* size of field maps */
    float	rmapfov,pmapfov,smapfov;	/* FOV of field map image */
    int		zmapres,ymapres,xmapres;	/* size of field maps,x/y/z */
    float	zmapfov,ymapfov,xmapfov;	/* FOV of field map image,x/y/z */    
    int		zrefres,yrefres,xrefres;	/* size of reference maps */
    float	zreffov,yreffov,xreffov;	/* FOV of field map image */
    float	roffset,poffset,soffset;	/* image offsets */ 
    float	mapdelay,threshparam,minmapdelay;  /* delays, threshold */
    char	orient[15];			/* orientation */
    float	rmapoff,pmapoff,smapoff;        /* pro,ppe,ppe2 */
    float	psi,phi,theta;			/* Euler angles */
    float       thk,ns,pss0;             /* slice parameters */
    float       slim,plim,rlim;          /* size limits */
    int		totalmapsize,totalrefsize;
    float	*rfield,*field,*field1,*field2,*fieldin,*out;
    int		i,j,x,y,z,zz,yy,xx,r,p,s; 
    int		interpolation;
    float	zmaploc,zmaploc2,zmaploc0,zmapinc,zmapend,zmapoff;
    float	ymaploc,ymaploc2,ymaploc0,ymapinc,ymapend,ymapoff;
    float	xmaploc,xmaploc2,xmaploc0,xmapinc,xmapend,xmapoff;	
    float	zrefloc,zrefloc0,zrefinc,zrefoff;
    float	yrefloc,yrefloc0,yrefinc,yrefoff;
    float	xrefloc,xrefloc0,xrefinc,xrefoff;
    float	d1,d2;
    int		done,regrid;

     /* usage */     
    checkargs(argv[0],argc,"rootfilename");
      
     /***** process arguments *****/
    args = 1;
    /* open the field map phase files, frequency file, and parameter files */
    strcpy(mapname,argv[args]);
    rfieldfile = efopen(argv[0],strcat(mapname,".r"),"w"); /* e.g. B0.mag.r */
    strcpy(mapname,argv[args]);  
    fieldfile = efopen(argv[0],mapname,"r");
    strcpy(mapname,argv[args++]);
      
    strcpy(mapname2,argv[args]);
    map_paramsfile = efopen(argv[0],strcat(mapname2,".param"),"r");
    strcpy(mapname2,argv[args++]); 

    /* open the reference map field and parameter files */
    strcpy(refname,argv[args]);
    refparamsfile = efopen(argv[0],strcat(refname,".param"),"r");
    
    /* read field map size */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%d %d %d",&rmapres,&pmapres,&smapres);
    /* field of view of field map data */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f %f %f",&rmapfov,&pmapfov,&smapfov); 
    /* read phase delay */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f",&mapdelay);    
    /* read threshold from B0.param file for setting mask */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f",&threshparam);
    /* read phase delay */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f",&minmapdelay);  
    /* orientation info */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%s %f %f %f",orient,&psi,&phi,&theta);
    /* pro,ppe,ppe2 */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f %f %f",&rmapoff,&pmapoff,&smapoff); 
    /* size limits +/- points along r,p,s directions */
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f %f %f",&rlim,&plim,&slim); 
    /** Note: slice information moved in param file; not used in gsremap  
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f",&thk);
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f",&ns);   
    efgets(str,80,map_paramsfile);
    sscanf(str,"%f",&pss0); 
    ****/ 
          
    /* read the reference map parameters; sagittal */
    /* read reference map size */
    efgets(str,80,refparamsfile);
    sscanf(str,"%d %d %d",&zrefres,&yrefres,&xrefres);
    /* field of view of reference map data */
    efgets(str,80,refparamsfile);
    sscanf(str,"%f %f %f",&zreffov,&yreffov,&xreffov);
      
    /* calculate final array sizes */
    totalrefsize = zrefres*yrefres*xrefres;  /* shimmap reference file size */
    totalmapsize = rmapres*pmapres*smapres;  /* fieldmap file size */
    /* check size-limits */
    if(totalrefsize != totalmapsize) {
      fprintf(stderr,"gslimit: shimmap and fieldmap sizes differ: %d %d\n",totalrefsize,totalmapsize);
      exit(1);
    }
    
    /* allocate data space for input data, B0 map */
    fieldin = (float *) calloc((unsigned)(totalmapsize),sizeof(float));
    /* allocate space for rotated data */
    field = (float *) calloc((unsigned)(totalmapsize),sizeof(float));
    /* read in B0 map data */
    fread(fieldin,sizeof(float),(totalmapsize),fieldfile);

    /* zero outer limits; points outside rlim,plim,slim are zeroed */
    out = (float *) calloc((unsigned)totalmapsize,sizeof(float)); /* zero buffer */
    /* check size limits */
    if(rlim > rmapres/4) {
      fprintf(stderr,"gslimit: Illegal RO size limit: %f\n",rlim);
      exit(1);
    }
    if(plim > pmapres/4) {
      fprintf(stderr,"gslimit: Illegal PE size limit: %f\n",plim);
      exit(1);
    }
    if(slim > smapres/4) {
      fprintf(stderr,"gslimit: Illegal PE#2 size limit: %f\n",slim);
      exit(1);
    }
    for ( s=slim ; s<smapres-slim ; s++ )
      for ( p=plim ; p<pmapres-plim ; p++ )  
	 for ( r=rlim; r<rmapres-rlim ; r++ )  
           out[image(s,p,r)] = fieldin[image(s,p,r)]; 
    /* copy back the data */
    for(i=0; i<totalmapsize; i++)
      fieldin[i] = out[i];  

    /********* Rotate from logical to xyz frame *****************/
    xmapoff=ymapoff=zmapoff=0.0;  /* fov offsets initialized to zero */
    xrefoff=yrefoff=zrefoff=0.0;  /* pro in shimmap must be zero */
      
    if (!strcmp(orient,"sag")) 
    {   
      for(i=0; i<totalmapsize; i++) /* shimmap is sag, so just copy */
        field[i] = fieldin[i];
        
      zmapoff = -rmapoff; /* pro */   
      zmapres=rmapres; ymapres=pmapres; xmapres=smapres;
      zmapfov=rmapfov; ymapfov=pmapfov; xmapfov=smapfov;
      zmapoff = -rmapoff; /* pro */
    }
    else if (!strcmp(orient,"cor"))
    {
      zmapoff = -rmapoff; /* pro */
      zmapres=rmapres; xmapres=pmapres; ymapres=smapres;
      zmapfov=rmapfov; xmapfov=pmapfov; ymapfov=smapfov;
      zmapoff = -rmapoff; /* pro */    
      for(r=0,z=0; r<rmapres; r++,z++)
       /* for(p=0,x=0; p<pmapres; p++,x++) */
          for(p=0,x=pmapres-1; p<pmapres; p++,x--)  /* sign reversed */
          for(s=0,y=0; s<smapres; s++,y++)
            field[mapindex(x,y,z)] = fieldin[image(s,p,r)];          
    }
    else if (!strcmp(orient,"trans"))
    {
      ymapoff = -rmapoff; /* pro */   
      ymapres=rmapres; xmapres=pmapres; zmapres=smapres;
      ymapfov=rmapfov; xmapfov=pmapfov; zmapfov=smapfov;    
      for(r=0,y=0; r<rmapres; r++,y++)
        for(p=0,x=pmapres-1; p<pmapres; p++,x--)    /* sign reversed */
          for(s=0,z=smapres-1; s<smapres; s++,z--)  /* sign reversed */
            field[mapindex(x,y,z)] = fieldin[image(s,p,r)];
    }    
    else if (!strcmp(orient,"sag90")) 
    {     
      ymapoff = -rmapoff;      
      zmapres=pmapres; ymapres=rmapres; xmapres=smapres;
      zmapfov=pmapfov; ymapfov=rmapfov; xmapfov=smapfov;            
      for(r=0,y=0; r<rmapres; r++,y++)
        for(p=0,z=pmapres-1; p<pmapres; p++,z--)  /* sign reversed */
          for(s=0,x=0; s<smapres; s++,x++)
            field[mapindex(x,y,z)] = fieldin[image(s,p,r)];  
    }        
    else if (!strcmp(orient,"cor90"))
    {
      xmapoff = rmapoff;
      xmapres=rmapres; zmapres=pmapres; ymapres=smapres;
      xmapfov=rmapfov; zmapfov=pmapfov; ymapfov=smapfov;    
      for(r=0,x=rmapres-1; r<rmapres; r++,x--)   /* sign reversed */
        for(p=0,z=pmapres-1; p<pmapres; p++,z--)  /* sign reversed */
          for(s=0,y=0; s<smapres; s++,y++)
            field[mapindex(x,y,z)] = fieldin[image(s,p,r)];          
    }  

    else if (!strcmp(orient,"trans90"))
    {
      xmapoff = rmapoff;
      xmapres=rmapres; ymapres=pmapres; zmapres=smapres;
      xmapfov=rmapfov; ymapfov=pmapfov; zmapfov=smapfov;    
      for(r=0,x=rmapres-1; r<rmapres; r++,x--)     /* all signs reversed */
        for(p=0,y=pmapres-1; p<pmapres; p++,y--)
          for(s=0,z=smapres-1; s<smapres; s++,z--)
            field[mapindex(x,y,z)] = fieldin[image(s,p,r)];                      
    }        
    else
    {
      fprintf(stderr,"Illegal orientation: %s\n",orient);
      exit(1);
    }    
    /********* image rotated to xyz frame ***********************/
           
  regrid = 1;
  if(regrid)
  {      

    
    rfield = (float *) calloc((unsigned)(totalrefsize),sizeof(float)); /* output file */
    field1 = (float *) calloc((unsigned)(xmapres*ymapres*zrefres),sizeof(float)); /* temporary file */
    field2 = (float *) calloc((unsigned)(xmapres*yrefres*zrefres),sizeof(float));

    /**** regrid along the read (zz) dimension *****/
    zrefloc0 = -(zreffov/2.0)+zrefoff;  /* position of first data pt in shimmap */
    zrefinc = zreffov/(zrefres-1);    /* shimmap position increment, mm/pt  */ 
    zmaploc0 = -(zmapfov/2.0)+zmapoff;  /* position of first point in B0map */
    zmapinc = zmapfov/(zmapres-1);    /* B0 map position increment, mm/pt  */ 
    zmapend = zmapinc*(zmapres-1) + zmaploc0;  /* position of last pt in B0map */ 
    zrefloc = zrefloc0-zrefinc;      /* init current shimmap position pointer */ 

    for ( zz=0; zz<zrefres; zz++ )    /* for each ref map point */ 
    {
      zrefloc = zrefloc+zrefinc;    /* current position, mm */
      zmaploc = zmaploc0-zmapinc;   /* init B0 map/image position */
      /* find points for interpolation */
      interpolation = -1;  /* -1=outside fov; 1=current value; 2=lin interpolation */
      done=0;

      for( z=0; !done; z++ )  
      {    
        zmaploc = zmaploc+zmapinc;
        zmaploc2 = zmaploc+zmapinc;

        if (z == zmapres-1)   /* last point? */
        {
          if(zmaploc==zrefloc)
          {
            interpolation = 1;
            done=1;
          }
          else
          {
            interpolation = -1;
            done=1;
          }
        }
        else if (zmaploc == zrefloc)
        {
          interpolation = 1;
          done=1;
        }
        /* find the points for interpolation */
        else if( (zrefloc > zmaploc)&&(zrefloc < zmaploc2) )
        {
          interpolation = 2;   /* points found for interpolation */
          done=1;
        } 
      }
      z--; /* reset pointer to current data point in input file */
      /**
      fprintf(stderr,"zz=%d, zrefloc=%4.3f, z=%d, zmaploc=%4.3f, zmaploc2=%4.3f,interp=%d\n",
            zz,zrefloc,z,zmaploc,zmaploc2,interpolation);
      **/
      /* generate new data point; go through all lines */
      for(x=0; x<xmapres; x++)
      {
        for(y=0; y<ymapres; y++)
        {
          if (interpolation == 1)
            field1[map1index(x,y,zz)] = field[mapindex(x,y,z)];
          else if(interpolation == -1)
            field1[map1index(x,y,zz)] = 0;  /* probably outside B0 FOV */
          else if (interpolation == 2)
          {
            /* interpolation between r and r+1 */
            /* quick interpolation, for testing */
            d1 = field[mapindex(x,y,z)];
            z++;
            d2 = field[mapindex(x,y,z)];
            z--;
            if( zrefloc < (zmaploc2-zmapinc*0.5) )
              field1[map1index(x,y,zz)] = d1;
            else
              field1[map1index(x,y,zz)] = d2; 
          }
        }
      }
    }     /* next shimmap point along read dimension */                    

    /**** regrid along the phase#1 (yy) dimension *****/   
    
/***** Bug: check sign for fov parameters if ppe != 0 */
    yrefloc0 = -(yreffov/2.0)+yrefoff;  /* position of first data pt in shimmap */
    yrefinc = yreffov/(yrefres-1);     /* shimmap position increment, mm/pt  */ 
    ymaploc0 = -(ymapfov/2.0)+ymapoff;  /* position of first point in B0map */
    ymapinc = ymapfov/(ymapres-1);     /* B0 map position increment, mm/pt  */ 
    ymapend = ymapinc*(ymapres-1) + ymaploc0;  /* position of last pt in B0map */ 
    yrefloc = yrefloc0-yrefinc;        /* init current shimmap position pointer */   
    for ( yy=0; yy<yrefres; yy++ )     /* for each ref map point */ 
    {
      yrefloc = yrefloc+yrefinc;    /* current position, mm */
      ymaploc = ymaploc0-ymapinc;   /* init B0 map/image position */
      /* find points for interpolation */
      done=0;      
      interpolation = -1;  /* -1=outside fov; 1=current value; 2=lin interpolation */
      for( y=0; !done; y++ )  
      {
        ymaploc = ymaploc+ymapinc;  /* adjacent points selected */
        ymaploc2 = ymaploc+ymapinc;
        if (y == ymapres-1)  /* last point? */
        {
          if(ymaploc==yrefloc)
          {
            interpolation = 1;
            done=1;
          }
          else
          {
            interpolation = -1;
            done=1;
          }
        }
        else if (ymaploc == yrefloc)
        {
          interpolation = 1;
          done=1;
        }
        /* find the points for interpolation */
        else if( (yrefloc > ymaploc)&&(yrefloc < ymaploc2) )
        {
          interpolation = 2;   /* points found for interpolation */
          done=1;
        }
      }
      y--;  /* reset pointer to current data point in input file */
      /**
      fprintf(stderr,"yy=%d, yrefloc=%4.3f, y=%d, ymaploc=%4.3f, ymaploc2=%4.3f,interp=%d\n",
            yy,yrefloc,y,ymaploc,ymaploc2,interpolation);
      **/
                        
      /* generate new data point; go through all lines */
      for(x=0; x<xmapres; x++)
      {
        for(zz=0; zz<zrefres; zz++)  /* already interpolated in ro; note size=zrefres */
        {
          if (interpolation == 1)
            field2[map2index(x,yy,zz)] = field1[map1index(x,y,zz)];
          else if(interpolation == -1)
            field2[map2index(x,yy,zz)] = 0;  /* outside shimmap FOV region */
          else if (interpolation == 2)
          {
            /* interpolation between p and p+1 */
            /* quick interpolation, for testing */
            d1 = field1[map1index(x,y,zz)];
            y++;
            d2 = field1[map1index(x,y,zz)];
            y--;
            if( yrefloc < (ymaploc2-ymapinc*0.5) )
              field2[map2index(x,yy,zz)] = d1;
            else
              field2[map2index(x,yy,zz)] = d2; 
          }
        }
      }
      
    }     /* next shimmap point in pe dimension */     
                 
    /**** regrid along the slice/phase#2 dimension *****/

/*****Bug: sign for fov parameters not checked if ppe2 used */
    xrefloc0 = -(xreffov/2.0)+xrefoff;  /* position of first data pt in shimmap */
    xrefinc = xreffov/(xrefres-1);      /* shimmap position increment, mm/pt  */ 
    xmaploc0 = -(xmapfov/2.0)+xmapoff;  /* position of first point in B0map */
    xmapinc = xmapfov/(xmapres-1);      /* B0 map position increment, mm/pt  */ 
    xmapend = xmapinc*(xmapres-1) + xmaploc0;  /* position of last pt in B0map */ 
    xrefloc = xrefloc0-xrefinc;         /* init current shimmap position */   
    for ( xx=0; xx<xrefres; xx++ )      /* for each ref map point along pe#2 */ 
    {
      xrefloc = xrefloc+xrefinc;    /* current position, mm */
      xmaploc = xmaploc0-xmapinc;   /* init B0 map/image position */
      /* find points for interpolation */
      interpolation = -1;  /* 0=outside fov; 1=current value; 2=two pt interpoln */
      done=0;
      for( x=0; !done; x++ )  
      {
        xmaploc = xmaploc+xmapinc;  /* adjacent points selected */
        xmaploc2 = xmaploc+xmapinc;
        
        if (x == xmapres-1)  /* last point? */
        {
          if(xmaploc==xrefloc)
          {
            interpolation = 1;
            done=1;
          }
          else
          {
            interpolation = -1;
            done=1;
          }
        }
        else if (xmaploc == xrefloc)
        {
          interpolation = 1;
          done=1;
        }
        /* find the points for interpolation */
        else if( (xrefloc > xmaploc)&&(xrefloc < xmaploc2) )
        {
          interpolation = 2;   /* points found for interpolation */
          done=1;
        }
      }         
      x--;    /* reset pointer to current data pt in input file */
      /**
      fprintf(stderr,"xx=%d, xrefloc=%4.3f, x=%d, xmaploc=%4.3f, xmaploc2=%4.3f,interp=%d\n",
            xx,xrefloc,x,xmaploc,xmaploc2,interpolation);
      **/
                  
      /* generate new data point; go through all lines */
      for(yy=0; yy<yrefres; yy++)    /* already interpolated in pe; size is yrefres */
      {
        for(zz=0; zz<zrefres; zz++)  /* already interpolationd in ro; note zrefres */
        {
          if (interpolation == 1)
            rfield[refindex(xx,yy,zz)] = field2[map2index(x,yy,zz)];
          else if (interpolation == -1)
            rfield[refindex(xx,yy,zz)] = 0.0;    /* outside FOV region */
          else if (interpolation == 2)
          {
            /* interpolation between s and s+1 */
            /* quick interpolation, for testing */
            d1 = field2[map2index(x,yy,zz)];
            x++;
            d2 = field2[map2index(x,yy,zz)];
            x--;
            if( xrefloc < (xmaploc2-xmapinc*0.5) )
              rfield[refindex(xx,yy,zz)] = d1;
            else
              rfield[refindex(xx,yy,zz)] = d2; 
          }
        }
      }
    }     /* next shimmap point in pe#2 dimension */         
    fwrite(rfield,sizeof(float),totalrefsize,rfieldfile);
    free(rfield);       
    free(field);
    free(field1);
    free(field2);
  }   /* regrid */                         	
}

/*****************************************************************
		Modification History
		
02jun05(ss) - image rotation function added
01dec07(ss) - size limit option added

******************************************************************/

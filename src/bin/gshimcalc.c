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
#include	<stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include	<math.h>
#include	"nrutil.h"
#include	"nr.h"
#include	"util.h"

#define		TWOPI		(2.0 * 3.14159265358979323846)
#define		ZWIDTH		15		/* minimum z to use z3 */
#define		TOTALSHIMS	50		/* max number of shims in varian shimsets */

/* indexing calculations */
#define 	refindex(s,p,r)	(s*prefres*rrefres+p*rrefres+r)
#define 	mapindex(s,p,r)	(s*pmapres*rmapres+p*rmapres+r)

/* shim names, units, offsets */
char		shimname[TOTALSHIMS][10];	/* names of shims */
char		shimunit[TOTALSHIMS][10];	/* units of shim meas. */
char		shimlist[TOTALSHIMS][10];	/* names of shims, for shim file */
float		shimoffset[TOTALSHIMS];		/* offsets used for ref */
float		correction[TOTALSHIMS];		/* final delta currents */
int		shimactive[TOTALSHIMS]; 	/* 1= shim values are calculated for shimming */
int		shimused[TOTALSHIMS]; 		/* 1= shims are mapped in shimmap.f */
float		shimval[TOTALSHIMS]; 		/* shim values for shim file */
float		shimvalue[TOTALSHIMS];		/* shim values from shim file */

/* I/O string */
char		str[80];

int main(int argc, char *argv[])
{
    /* number of shims in reference set */
    int		numrefshims;		/* no of shims used for mapping in shimmap.f */
    int		totalshims;       	/* tot no of shims available shimset not-including z0 */
    int		shimct;          	/* no of shims listed in shimmap.param */
    
    /* bounds of the shimming region */
    int		cylinder = 1;  			/* default is cylindrical mask */
    int 	ellipse = 0;              	/* default is eliptical mask */
    int		rlower = -1000,
		rupper = 1000,			/* ro shimming bounds */
    		plower = -1000,
		pupper = 1000,			/* y shimming bounds */
    		slower = -1000,
		supper = 1000;			/* z shimming bounds */

    /* center and size of VOI ellipse */
    float	rcenter,pcenter,scenter,
		rlength,plength,slength;

    /* indices, array sizes */
    int		totalmapsize,totalrefsize;	/* size of current/ref maps */
    int		numpoints;			/* number of active points */
    int		numshims;			/* number of active shims */
    int		args;				/* indexes through arg list */
    int		i,j,k;				/* generic indices */
    int		r,p,s;				/* spatial indices */
    int		rr,pp,ss,point,thispoint;	/* spatial positions */
    float 	rl,pl,sl;			/* float spatial location */
    int		shim,thisshim;			/* shim table indices */
    int		rmapstart,rmapstop,
		pmapstart,pmapstop,
		smapstart,smapstop;
    int		voxpoints;
    int		rrefstart,rrefstop,
		prefstart,prefstop,
		srefstart,srefstop;
    int		start;
    int		offset;
    float	max,min,res;	

    /* reference maps */
    char	refname[80];			/* root name for reference */
    FILE	*reffieldfile,*refparamsfile;	/* reference maps, params */
    /** FILE	*shimfile;  **/			/* shim info file */
    /** FILE	*newshimfile;  **/		/* new shim output file info */
    int		rrefres,prefres,srefres;	/* size of reference maps */
    float	rreffov,preffov,sreffov;	/* FOV of field map image */    
    float	*onerefmap;			/* one whole reference map */
    float	**A,**AA;			/* all extracted ref maps */
    float	mapdelay,refdelay;		/* delay (ms) between refs */
    float	minmapdelay,minrefdelay;	/* delay (ms) refs; short */
    /* field map */
    char	mapname[80];			/* root name for field */
    FILE	*mapfieldfile,			/* field map */
		*mapparamsfile,			/* parameters */
		*mapmagfile,			/* magnitude of object */
		*outfieldfile,			/* the residual field */
		*outmaskfile;			/* the magnitude mask used */
    int		rmapres,pmapres,smapres;	/* size of field maps */
    float	rmapfov,pmapfov,smapfov;	/* FOV of field map image */
    float	*map;				/* field map */
    float	*mag;				/* magnitude map */
    float	*mask;				/* extraction mask */
    float	*B;				/* extracted field map */

    /* extraction masks */
    short int	*mapmask,*refmask;

    /* magnitude masking */
    float	maxmag,avmag,threshold,threshparam,threshpcref;

    /* SVD factors */
    float	**V,*W;				

    /* solution */
    float	*X;

    /* residual */
    float	*R;			

    /* analysis */
    float	minbefore = 10000.0 ,
		maxbefore = -10000.0,
		meanbefore = 0.0,
		varbefore = 0.0;
    float	minafter = 10000.0,
		maxafter = -10000.0,
		meanafter = 0.0,
		varafter = 0.0;
    
    /* functions */
    int		atoi();

/* check arguments */

    checkargs(argv[0],argc,
	"[-r l u] [-p l u] [-s l u] [-b | -c | -e] fieldname refname");

    args = 1;
    while ( argv[args][0] == '-' )
	switch (argv[args++][1])
	{
	    case 'r':	/* frequency encode, limits are to be used */
		rlower = atoi(argv[args++]);
		rupper = atoi(argv[args++]);		
		break;

	    case 'p':	/* pe#1 limits are to be used */
		plower = atoi(argv[args++]);
		pupper = atoi(argv[args++]);
		break;

	    case 's':	/* ss (pe#2) limits are to be used */
		slower = atoi(argv[args++]);
		supper = atoi(argv[args++]);
		break;
 	    case 'e':  /* use elliptical mask */
		ellipse=1;
		cylinder=0;
		break;
 	    case 'c':  /* use cylindrical mask */
		ellipse=0;
		cylinder=1;
		break;		
 	    case 'b':  /* use rectangular mask */
		ellipse=0;
		cylinder=0;
		break;
	}
            
    /* open the field map field, magnitude, and parameter files */
    strcpy(mapname,argv[args]);
    mapfieldfile = efopen(argv[0],strcat(mapname,".f"),"r");
    strcpy(mapname,argv[args]);
    mapmagfile = efopen(argv[0],strcat(mapname,".mag"),"r");
    strcpy(mapname,argv[args]);
    mapparamsfile = efopen(argv[0],strcat(mapname,".param"),"r");
    strcpy(mapname,argv[args]);
    outfieldfile = efopen(argv[0],strcat(mapname,".pf"),"w"); /* predicted field */
    strcpy(mapname,argv[args]);
    outmaskfile = efopen(argv[0],strcat(mapname,".mask"),"w");    
    strcpy(mapname,argv[args++]);

   /* read the field map parameters */
    /* open the reference map field and parameter files */
    strcpy(refname,argv[args]);
    reffieldfile = efopen(argv[0],strcat(refname,".f"),"r");
    strcpy(refname,argv[args]);
    refparamsfile = efopen(argv[0],strcat(refname,".param"),"r");
    
    /* open the shim info file  */
    /***
    strcpy(refname,argv[args]);
    shimfile = efopen(argv[0],strcat(refname,".f"),"r");
    strcpy(refname,argv[args]);
    newshimfile = efopen(argv[0],strcat(refname,".shim.out"),"w");
    **/
    strcpy(mapname,argv[args++]);

/**** 
    strcpy(mapname,argv[--args]);
    outfieldfile = efopen(argv[0],strcat(mapname,".cf"),"w");
    strcpy(mapname,argv[args++]);
    outmaskfile = efopen(argv[0],strcat(mapname,".mask"),"w");
***/
    /* read field map size */
    efgets(str,80,mapparamsfile);
    sscanf(str,"%d %d %d",&rmapres,&pmapres,&smapres);
    /* field of view of field map data */
    efgets(str,80,mapparamsfile);
    sscanf(str,"%f %f %f",&rmapfov,&pmapfov,&smapfov);
    /* read phase delay */
    efgets(str,80,mapparamsfile);
    sscanf(str,"%f",&mapdelay);
    
    /* read threshold from B0.param file for setting mask */
    efgets(str,80,mapparamsfile);
    sscanf(str,"%f",&threshparam);
    /* read phase delay */
    efgets(str,80,mapparamsfile);
    sscanf(str,"%f",&minmapdelay);
    
    /* read the reference map parameters */
    /* read reference map size */
    efgets(str,80,refparamsfile);
    sscanf(str,"%d %d %d",&rrefres,&prefres,&srefres);
    /* field of view of reference map data */
    efgets(str,80,refparamsfile);
    sscanf(str,"%f %f %f",&rreffov,&preffov,&sreffov);
    /* read phase delay */
    efgets(str,80,refparamsfile);
    sscanf(str,"%f",&refdelay);
    /* read threshold in percent */
    efgets(str,80,refparamsfile);
    sscanf(str,"%f",&threshpcref);
    /* read phase delay */
    efgets(str,80,refparamsfile);
    sscanf(str,"%f",&minrefdelay);
    /* read number of shims */
    efgets(str,80,refparamsfile);
    sscanf(str,"%d",&shimct); 
   
    totalshims = shimct+1;   /* shimct=no of shims defined in shimmap.param */

    /* read the information for each shim; Note, shimname[0] = z0 */
    /* shimused - 1=used i.e. mapped to generate field map, ref.field; 0=not used */
    /* shimactive - 1=used to generate field map and for shim adjustment calc.
                    0= not used for shim adjustment calc. */
    for ( shim=1 ; shim<totalshims; shim++ )   /* shim[0]==z0 info not in .param file */
    {
	efgets(str,80,refparamsfile);
	sscanf(str,"%s %f %s %d %d",shimname[shim],&shimoffset[shim],
	    shimunit[shim],&shimused[shim],&shimactive[shim]);	    	    
    }
          
    /* read shim information from shimmap.shim; shim_name current_shim_value */
    shimvalue[0] = 0;	/* z0 value */
    shimused[0] = 1;
    /* note totalshims includes z0 */
    /**
    for ( i=1; i<totalshims; i++ )    
    {
	efgets(str,80,shimfile);
	sscanf(str,"%s %f",shimlist[i],&shimvalue[i]);
    }
    **/  
 
    /* init shimvalues for working array */
    shimval[0] = 0;	/* z0 */
    for (i=1; i<totalshims; i++)
    {
    	if (shimused[i] == 1)	/* is this shim mapped in shimmap.f */
    	{

    		shimval[i] = shimvalue[i];
    	}
    }
 
    /* add the z0 shim information, and increment number of shims */
    strcpy(shimname[0],"z0");
    shimoffset[0] = 1000.0/(refdelay*TWOPI);
    shimactive[0] = 1;
    strcpy(shimunit[0],"Hertz");
        	    
    /* 
     * check whether shimming bounds are filled in
     */

    rlower = ( rlower > -999 ) ? rlower : -rrefres/2;
    rupper = ( rupper < 999 ) ? rupper : rrefres/2 - 1;
    plower = ( plower > -999 ) ? plower : -prefres/2;
    pupper = ( pupper < 999 ) ? pupper : prefres/2 - 1;
    slower = ( slower > -999 ) ? slower : -srefres/2;
    supper = ( supper < 999 ) ? supper : srefres/2 - 1;

    /*
     * calculate final array sizes
     */
    
    totalrefsize = rrefres*prefres*srefres;
    totalmapsize = rmapres*pmapres*smapres;

    /* 
     * convert the shim range given into matrix indices
     */

    /* reference fieldmaps */
    rrefstart = rrefres/2 + rlower;
    rrefstop = rrefres/2 + rupper;
    prefstart = prefres/2 + plower;
    prefstop = prefres/2 + pupper;
    srefstart = srefres/2 + slower;
    srefstop = srefres/2 + supper;

    /* current field map */
    rmapstart = rmapres/2 + rlower;
    rmapstop = rmapres/2  + rupper;
    pmapstart = pmapres/2 + plower;
    pmapstop = pmapres/2  + pupper;
    smapstart = smapres/2 + slower;
    smapstop = smapres/2  + supper;

    /* 
     * generate the mask array that tells us which points in the
     * reference map shim files and current map file are to be used 
     * for calculation
     */

    /* current map mask */
    mask = (float *) calloc((unsigned)totalmapsize,sizeof(float));

    /* calculate the center and size of the VOI ellipse */
    rcenter = (rmapstart+rmapstop)/2.0;
    pcenter = (pmapstart+pmapstop)/2.0;
    scenter = (smapstart+smapstop)/2.0;
    rlength = ( rmapstart==rmapstop ) ? 1.0 : (rmapstop-rmapstart)/2.0;
    plength = ( pmapstart==pmapstop ) ? 1.0 : (pmapstop-pmapstart)/2.0;
    slength = ( smapstart==smapstop ) ? 1.0 : (smapstop-smapstart)/2.0;

    /* apply spatial ellipse/rectangle limits */
    for ( s=smapstart ; s<=smapstop ; s++ )  
    {
	sl = (s-scenter)*(s-scenter)/(double)(slength*slength);
	for ( p=pmapstart ; p<=pmapstop ; p++ )  
	{
	    pl = (p-pcenter)*(p-pcenter)/(double)(plength*plength);
	    for ( r=rmapstart ; r<=rmapstop ; r++ )  
	    {
		rl = (r-rcenter)*(r-rcenter)/(double)(rlength*rlength);
		if (cylinder)
		{
			if ((pl + sl) < 0.99) /* note 2nd and 3rd dimension */
			mask[mapindex(s,p,r)] = 1;
		}
		else if ((( rl + pl + sl ) < 0.99) || !ellipse)  
		    mask[mapindex(s,p,r)] = 1;
	    }
	}
    }
    voxpoints = (rmapstop-rmapstart+1)*(pmapstop-pmapstart+1)*(smapstop-smapstart+1);

    /* 
     * apply the magnitude mask 
     */

    /* load the magnitude image */
    mag = (float *) calloc(totalmapsize,sizeof(float));
    fread(mag,sizeof(float),totalmapsize,mapmagfile);

    /* find the maximum value */
    maxmag = 0.0;
    for ( i=0 ; i<totalmapsize ; i++ )
	maxmag = (mag[i]>maxmag) ? mag[i] : maxmag;

    /* average everything that is at least 50% of maximum */
    avmag = 0.0;
    numpoints = 0;
    for ( i=0 ; i<totalmapsize ; i++ )
	if ( mag[i] > 0.5*maxmag )
	{
	    avmag += mag[i];
	    numpoints++;
	}
    avmag /= numpoints;
	    
    /* set the threshold; threshparam read from map.param file */ 
    threshold = threshparam*avmag/100; 

    /* apply to the mask */
    for ( i=0 ; i<totalmapsize ; i++ )
	mask[i] *= (mag[i]>threshold);
    
    /* 
     * set this mask as the mapmask 
     */
    mapmask = (short int *) calloc((unsigned)totalmapsize,sizeof(short int));
    for ( i=0 ; i<totalmapsize ; i++ )
	mapmask[i] = mask[i];
	
     /* apply the same mask to the reference maps */

    refmask = (short int *) calloc((unsigned)totalrefsize,sizeof(short int));

    /* index through the current map, calculating position in reference map */
    for ( s=smapstart ; s<=smapstop ; s++ )  
    {
	ss = s - smapres/2 + srefres/2;
	for ( p=pmapstart ; p<=pmapstop ; p++ )  
	{
	    pp = p - pmapres/2 + prefres/2;
	    for ( r=rmapstart ; r<=rmapstop ; r++ )  
	    {
		rr = r - rmapres/2 + rrefres/2;
		/* set the mask */
		refmask[refindex(ss,pp,rr)] = mapmask[mapindex(s,p,r)];
	    }
	}
    }

    free(mag);

     /* determine the size of the SVD problem  */

    numpoints = 0;
    for ( i=0 ; i<totalmapsize ; i++ )
	numpoints += mapmask[i];


    if(numpoints < 1) {
        printf("ERROR: Points in voxel   - %d\n",numpoints);
	exit(1);
    }
	
    numshims = 0;
    numrefshims = 0;
    for ( shim=0 ; shim<totalshims ; shim++ )   /* no of shims used for calc; includes z0 */
    {
	numshims += shimactive[shim];        	/* shimactive[0]=1 for z0 */
	numrefshims += shimused[shim];		/* shimused[0]=1 for z0 */
    }

    /*
     * read the reference maps
     * 
     * this array is allocated per Numerical Recipes requirement
     * for SVD solution, i.e. with [1..N] indexing
     *
     * two copies of this matrix are required, so we can compute the residual
     */

    /* allocate space for the A matrix */
    A = matrix(1,numpoints,1,numshims);
    AA = matrix(1,numpoints,1,numshims);

    /* fill the first column of the matrix, for the z0 shim */
    for ( point=1 ; point<=numpoints ; point++ ) 
	A[point][1] = 1.0;

    /* allocate space for a single map */
    onerefmap = (float *) calloc((unsigned)totalrefsize,sizeof(float));

    /* loop through the shims, extracting active points */
    thisshim = 2;
    for ( shim=1 ; shim<totalshims ; shim++ ) 
    {
    	if (shimused[shim] == 1)    /* if this shim is mapped in shimmap.f read into array */
    	{
		/* read in reference field */
		fread(onerefmap,sizeof(float),totalrefsize,reffieldfile);
	
		/* extract the points required for calculation */
		if ( shimactive[shim] == 1 )   /* if this shim used for calculation? */
		{
	   	 thispoint = 1;
	   	 for ( point=0 ; point<totalrefsize ; point++ )
			if ( refmask[point] == 1 )
			    A[thispoint++][thisshim] = onerefmap[point];
	   	 thisshim++;
		}
	}
    }
    free(onerefmap);

    /* copy A */
    for ( point=1 ; point<=numpoints ; point++ )
	for ( shim=1 ; shim<=numshims ; shim++ )
	    AA[point][shim] = A[point][shim];

    /*
     * read the current field map
     *
     * this array is allocated per Numerical Recipes requirement
     * for SVD solution, i.e. with [1..N] indexing
     */

    /* allocate space for the B matrix */
    B = vector(1,numpoints);

    /* allocate space for the input map */
    map = (float *) calloc((unsigned)totalmapsize,sizeof(float));

    /* read in B0 field map */
    fread(map,sizeof(float),totalmapsize,mapfieldfile);

    /* extract the points required for calculation */
    thispoint = 1;
    for ( point=0 ; point<totalmapsize ; point++ )
	if ( mapmask[point] == 1 )
	    B[thispoint++] = map[point];

    /*
     * do the calculations
     */

    /* allocate space for the decomposition, residual, and result */
    W = vector(1,numshims);
    V = matrix(1,numshims,1,numshims);
    R = vector(1,numpoints);
    X = vector(1,numshims);

    /* perform the decomposition */
    svdcmp(A,numpoints,numshims,W,V);

    /* solve */
    svbksb(A,W,V,numpoints,numshims,B,X);

    /* compute residual */
    for ( point=1 ; point<=numpoints ; point++ )
    {
	R[point] = B[point];
	for ( shim=1 ; shim<=numshims ; shim++ )
	   R[point] -= AA[point][shim]*X[shim];
    }
    
    free(refmask);

    /*
     * compute the change in shim DAC's
     */

    thisshim = 1;
    for ( shim=0 ; shim<totalshims ; shim++ )
    {
    	if (shimused[shim] == 1)
    	{
		if ( shimactive[shim] == 1 )
	    		correction[shim] = 
				X[thisshim++]*shimoffset[shim]*refdelay/mapdelay;
	}
    }			

    /* 
     * write out the field map, B0.f.out 
     */

    thispoint = 1;
    for ( point=0 ; point<totalmapsize ; point++ )
    {
	if ( mapmask[point] == 1 )
	    map[point] = R[thispoint++];
	else
	    map[point] = 0.0;
    }
    /* write out predicted field map */
    fwrite(map,sizeof(float),totalmapsize,outfieldfile);
    /*
     * write out the magnitude mask
     */
    fwrite(mask,sizeof(float),totalmapsize,outmaskfile);
    /*
     * analyse the current shim map
     */

    for ( point=1 ; point<=numpoints ; point++ )
    {
	meanbefore += B[point];
	maxbefore = ( maxbefore < B[point] ) ? B[point] : maxbefore;
	minbefore = ( minbefore > B[point] ) ? B[point] : minbefore;
    }

    meanbefore /= numpoints;

    for ( point=1 ; point<=numpoints ; point++ )
	varbefore += pow((double)(B[point]-meanbefore),2.0);
    varbefore /= numpoints;

    /* scale to Hertz */
    minbefore *= 1000.0/(mapdelay*TWOPI);
    maxbefore *= 1000.0/(mapdelay*TWOPI);
    meanbefore *= 1000.0/(mapdelay*TWOPI);
    varbefore *= pow((double)(1000.0/(mapdelay*TWOPI)),2.0);

    /* 
     * analyse the output
     */

    for ( point=1 ; point<=numpoints ; point++ )
    {
	meanafter += R[point];
	maxafter = ( maxafter < R[point] ) ? R[point] : maxafter;
	minafter = ( minafter > R[point] ) ? R[point] : minafter;
    }

    meanafter /= numpoints;

    for ( point=1 ; point<=numpoints ; point++ )
	varafter += pow((double)(R[point]-meanafter),2.0);
    varafter /= numpoints;

    /* scale to Hertz */
    minafter *= 1000.0/(mapdelay*TWOPI);
    maxafter *= 1000.0/(mapdelay*TWOPI);
    meanafter *= 1000.0/(mapdelay*TWOPI);
    varafter *= pow((double)(1000.0/(mapdelay*TWOPI)),2.0);

    /* 
     * print out the report
     */

    /* required delta shim changes */
    printf("Shim\t Change\n");
    printf("........................\n");
    for ( shim=0 ; shim<totalshims ; shim++ )
    {        	
    	if ((shimused[shim] == 1) && (shimactive[shim] == 1) )     /* if the shim is mapped, include z0 */
    	{    	
    		j = (int)(-correction[shim]);
		printf("%s\t %6d \n", shimname[shim],j);		
	}
	else
		printf("%s\t %6d \n", shimname[shim],0);
   }
    printf("........................\n");
/****
    for ( shim=1 ; shim<totalshims ; shim++ )   
    {    
    printf("\n");
    printf("... %s %6.0f %s %d %d\n",shimname[shim],shimoffset[shim],shimunit[shim],
                shimused[shim],shimactive[shim]);
    }
    printf("... %d %d\n",numshims,numrefshims);
****/ 
    printf("\n");          
    printf("B0 Field Map:\n");
    printf("Matrix     - %d %d %d\n",rmapres,pmapres,smapres);
    printf("FOV        - %5.2f %5.2f %5.2f\n",rmapfov,pmapfov,smapfov);
    printf("Phasedelay - %-5.2f\n",mapdelay);
    printf("Threshold  - %8.1f x %3.1f %% \n",avmag,threshparam);
    printf("........................\n");
    printf("\n");
    printf("Shim Field Map:\n");
    printf("Matrix     - %d %d %d\n",rrefres,prefres,srefres);
    printf("FOV        - %5.2f %5.2f %5.2f\n",rmapfov,pmapfov,smapfov);    
    printf("Phasedelay - %-5.2f\n",refdelay);
    printf("Number of shims mapped  - %d\n",numrefshims-1);  /* z0 not in list */
    printf("Number of shims defined - %d\n",totalshims-1);  
    printf("........................\n");
    printf("\n");
  
    printf("Shim Voxel Information:\n");
    if (cylinder)
      printf("Voxel shape: cylindrical\n");
    else if (ellipse)
      printf("Voxel shape: elliptical\n");
    else
        printf("Voxel shape: rectangular\n");

    printf ("Voxel dimension (pts)   - r=%d,%d, p=%d,%d, s=%d,%d\n",
        rlower,rupper,plower,pupper,slower,supper);
    printf("Points in voxel(fraction)   - %d/%d\n",numpoints,voxpoints);
    /* map statistics */
    printf("........................\n");
    printf("\n");
    printf("B0 Field Map Statistics:\n");
    res = 100*(pow((double)varbefore,0.5)-pow((double)varafter,0.5))/pow((double)varbefore,0.5);
    printf("RMS deviation  - %6.2f Hz\n",pow((double)varbefore,0.5));
    printf("Peak deviation - %6.2f Hz\n",maxbefore-minbefore);
    printf("........................\n");
    printf("\n");
    printf("Calculated Field Map:\n");   
    printf("RMS deviation  - %6.2f Hz\n",pow((double)varafter,0.5));
    printf("Peak deviation - %6.2f Hz\n",maxafter-minafter);
    printf("Predicted change - %6.2f %%\n",res);
    printf("........................\n");
    printf("\n");
}

/*****************************************************************************
		Modification History
		
030512(ss) minrefdelay,minmapdelay added

******************************************************************************/


/**********************************************************************
                 Modification History
25jul02(ss)
20jun02(ss) Working version
31jul02(ss) read, phase, slice references instead of x, y, z
27aug03(ss) voxpoints points printed

**********************************************************************/

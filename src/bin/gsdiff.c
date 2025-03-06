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
/*********************************************************************

 gsdiff - Difference between two images
 
 Usage: gsdiff rootname
 
 Details: Input - rootname.2,1,param
 	  Output rootname.f
 	  rootname.2 - rootname.1 => rootname.f

*********************************************************************/
 



#include	<stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include	<math.h>
#include	"util.h"

#define 	PI              3.14159265358979323846 
#define		TWOPI		(2*PI)

/* I/O string */
char		s[80];

int main(int argc, char *argv[])
{
    FILE	*paramsfile,*phasefile1,*phasefile2,*fieldfile;
    char	fieldname[80];
    int		args;
    int		xfieldres,yfieldres,zfieldres;
    float	xfov,yfov,zfov;
    int		totalfieldsize;
    float	*p1,*p2,*field;
    float	freq;
    int		point;
#   ifdef SHORTIMAGES
    short int   *fieldi;
#   endif
    float delay;

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
    phasefile1 = efopen(argv[0],strcat(fieldname,".1"),"r");
    strcpy(fieldname,argv[args]);
    phasefile2 = efopen(argv[0],strcat(fieldname,".2"),"r");
    strcpy(fieldname,argv[args]);
    paramsfile = efopen(argv[0],strcat(fieldname,".param"),"r");
    strcpy(fieldname,argv[args++]);
    fieldfile = efopen(argv[0],strcat(fieldname,".f"),"w");

    /* read the field map parameters */

    /* read field map size */
    efgets(s,80,paramsfile);
    sscanf(s,"%d %d %d",&xfieldres,&yfieldres,&zfieldres);
    efgets(s,80,paramsfile);
    sscanf(s,"%f %f %f",&xfov,&yfov,&zfov);
    /* read phase delay */
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&delay);

    /* calculate array size  */
    
    totalfieldsize = xfieldres*yfieldres*zfieldres;

    /* allocate data space  */

    p1 = (float *) calloc((unsigned)totalfieldsize,sizeof(float));
    p2 = (float *) calloc((unsigned)totalfieldsize,sizeof(float));
    field = (float *) calloc((unsigned)totalfieldsize,sizeof(float));

    /*
     * process
     */

    /* read in */
    fread(p1,sizeof(float),totalfieldsize,phasefile1);
    fread(p2,sizeof(float),totalfieldsize,phasefile2);

    /* phasesubtract, checking overange */
    for ( point=0 ; point<totalfieldsize ; point++ )
    {
	if (((p1[point]==0)||(p2[point]==0)))  /* trap for phase=0, mask or threshold limit */
	{
	  field[point] = 0;
	}
	else 
	{          
	    freq = p2[point]-p1[point];
	    field[point] = freq;
	}
    }

    /* write out the field map */
    fwrite(field,sizeof(float),totalfieldsize,fieldfile);
   
}

/*************************************************************************
		Modification History
		
**************************************************************************/

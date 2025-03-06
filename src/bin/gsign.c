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
 gsignscale -  Determine sign and scale of gradient

 Details - The shim field map is used to determine the sign of the 
           gradient. The sign corresponds to the sign of the gradient
           scaling factor in decctool. 4 points, starting from size/4 
           in the center trace are checked.

 Usage - gsign rootfilename
         Input - file.1 file.2 file.param

**********************************************************************/


#include	<stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include	<math.h>
#include	"util.h"

#define 	PI		3.14159265358979323846
#define		TWOPI           (2*PI)
#define 	index(s,p,r) (s*psize*rsize+p*rsize+r)

/* I/O string */
char		str[80];

int main(int argc, char *argv[])
{
    FILE	*paramsfile,*imagefile;
    char	fieldname[80];
    int		args;
    int		rsize,psize,ssize;
    float	rfov,pfov,sfov;
    int		totalsize,r,p,s;
    float	*p1,*p2;
    float   v,v2,v3,v4;
    int		gsign,gsign2,gsign3;
    int     ss,pp,rr;

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

    /*
     * process
     */

    /* read in */
    fread(p1,sizeof(float),totalsize,imagefile);

    /* search read out dimension */
    p = psize/2;    /* select center trace */
    s = ssize/2;
    gsign = 0;  /* gradient field sign */
    for ( r=rsize/4; r<rsize-3 ; r++ )
    {
      /* check 4 datapoints for sign of gradient field */
      v = p1[index(s,p,r)];
      v2 = p1[index(s,p,r+1)];
      v3 = p1[index(s,p,r+2)];
      v4 = p1[index(s,p,r+3)];
      if(v > 0)
      {
        if((v2>0)&&(v3>0)&&(v4>0))
        {
          gsign = 1;   /* positive field gradient */
        }       
      }
      else if(v < 0)
      {     
        if((v2<0)&&(v3<0)&&(v4<0))
        {
          gsign = -1;   /* negative field gradient */
        }
      }
      if(gsign != 0)
        break;
    }

    /* PE dimension */
    r = rsize/2;    /* select center trace */
    s = ssize/2;
    gsign2 = 0;  /* gradient field sign */
    for ( p=psize/4; p<psize-3 ; p++ )  
    {
      /* check 4 datapoints for sign of gradient field */
      ss=s; pp=p; rr=r;
      v = p1[index(ss,pp,rr)];
      ss=s; pp=p+1; rr=r;
      v2 = p1[index(ss,pp,rr)];
      ss=s; pp=p+2; rr=r;
      v3 = p1[index(ss,pp,rr)];
      ss=s; pp=p+3; rr=r;
      v4 = p1[index(ss,pp,rr)];
      if(v > 0)
      {
        if((v2>0)&&(v3>0)&&(v4>0))
        {
          gsign2 = 1;   /* positive field gradient */
        }
      }
      else if(v < 0)
      {     
        if((v2<0)&&(v3<0)&&(v4<0))
        {
          gsign2 = -1;   /* negative field gradient */
        }
      }
      if(gsign2 != 0)
        break;
    }
    /* PE2 dimension */
    r = rsize/2;    /* select center trace */
    p = psize/2;
    gsign3 = 0;  /* gradient field sign */
    for ( s=ssize/4; s<ssize-3 ; s++ )  
    {
      /* check 4 datapoints for sign of gradient field */
      ss=s; pp=p; rr=r;
      v = p1[index(ss,pp,rr)];
      ss=s+1; pp=p; rr=r;
      v2 = p1[index(ss,pp,rr)];
      ss=s+2; pp=p; rr=r;
      v3 = p1[index(ss,pp,rr)];
      ss=s+3; pp=p; rr=r;
      v4 = p1[index(ss,pp,rr)];
      if(v > 0)
      {
        if((v2>0)&&(v3>0)&&(v4>0))
        {
          gsign3 = 1;   /* positive field gradient */
        }
      }
      else if(v < 0)
      {     
        if((v2<0)&&(v3<0)&&(v4<0))
        {
          gsign3 = -1;   /* negative field gradient */
        }
      }
      if(gsign3 != 0)
        break;
    }
    printf("Field %d %d %d \n",gsign,gsign2,gsign3);   
}

/*************************************************************************
		Modification History

20080129 Bug: PE2 dimension indexing wrong

		
**************************************************************************/

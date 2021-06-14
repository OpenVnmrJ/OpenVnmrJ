/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************************************************
                        sis_oblique
        
        SEQD file for oblique gradient elements
*********************************************************************/
        
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "vnmrsys.h"
#include "oopc.h"
#include "acqparms.h"
#include "rfconst.h"
#include "macros.h"
#include "group.h"
#include "abort.h"

#define MINDELAY	0.195e-6	/* minimum delay for the ACB */
#define MAX_GRAD_AMP	32767.0

extern int      dps_flag;
extern double   dps_fval[];
extern double getval();
extern int bgflag;

extern Gpattern *get_gradient_pattern();

static int warningDone = 0;

/*----------------------------------------------*/
/* static info for tracking oblique patterns	*/
/*----------------------------------------------*/
struct obl_list
{
   char pat1[MAXSTR],pat2[MAXSTR],pat3[MAXSTR];
   double lvl1,lvl2,lvl3;
   double ang1,ang2,ang3;
   char pat_x[MAXSTR],pat_y[MAXSTR],pat_z[MAXSTR];
   double xlevel,ylevel,zlevel;
   struct obl_list *obl_link;
};

static struct obl_list *obl_list_head = NULL;
static struct obl_list   *obl_list_search();
static add_obl_shape();
	
static void
print_rotation_matrix(double m[9])
{
    printf("\nRotation matrix:\n");  
    printf("%9f   %9f   %9f\n", m[0], m[1], m[2]);
    printf("%9f   %9f   %9f\n", m[3], m[4], m[5]);
    printf("%9f   %9f   %9f\n", m[6], m[7], m[8]);
}
        
        /*******************************************************
                            gauss_dac()/1

                Returns the value of the G/cm to DAC unit
                conversion constant appropriate for the
                system hardware as a double.
        ********************************************************/

double gauss_dac(gid)
char gid; 
{
    double  G_dac,gradmax;

    int ix, gamp;
    switch (gid)
    {
	case 'x': case 'X':  ix = 0;
                           gradmax = gxmax;
                           break;
	case 'y': case 'Y':  ix = 1;
                           gradmax = gymax;
                           break;
	case 'z': case 'Z':  ix = 2;
                           gradmax = gzmax;
                           break;
	case 'n': case 'N':  break;
	default: text_error("Illegal gradient configuration."); psg_abort(1);
    }

    /* Consistency checks */
    switch (gradtype[ix])
    {
	case 'N': case 'n':
	  if (gradmax != 0.0) {
	    text_error("gauss_dac: Gradient configuration,table mismatch.\n");
	    psg_abort(1);
	  }
	  return(0.0);
	  break;
	default:
	  if (gradmax <= 0.0) {
             text_error("gauss_dac: gradmax <= zero\n");
             psg_abort(1);
	  }
    }

    G_dac = (double)gradstepsz/gradmax;
    return(G_dac);
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

static int
get_least_num_ticks(a,b,c)
int a,b,c;
{
    int ticks;
    ticks = a;
    if (b < a)
    {
	if (b<c) ticks = b;
	else ticks = c;
    }
    else {
	if (c<a) ticks = c;
    }
    return(ticks);
}

print_pattern(gpat,nelems)
Gpattern *gpat;
int nelems;
{
   int i;
   for (i=0; i<nelems; i++)
	{
	printf("gpat[i].amp %f \n", gpat[i].amp);
        }
}

scaleamp(gpat,nelems,scale)
Gpattern *gpat;
int nelems;
double scale;
{
   int i;
   for (i=0; i<nelems; i++)
	{
        gpat[i].amp *= scale;
        }
}

/*
 * Returns the index of the one row of the 3x3 matrix "m" that has a
 *   non-zero entry in column "col".
 * Returns -1 if there are more than one (or none--which is not possible
 *   for rotation matrices).
 */
static int
non_zero_row(col, m)
     int col;
     double m[3][3];
{
    int i;
    int row;
    static double tol = 0.01693; /* = sin(0.97 degrees) */

    for (row=-1, i=0; i<3; i++){
	if (fabs(m[i][col]) > tol){
	    if (row >= 0){
		return -1;
	    }else{
		row = i;
	    }
	}
    }
    return row;
}

/*--------------------------------------------------------------*/
/* rotate_shapes()						*/
/*	Procedure to rotate gradient shapes.			*/
/*	-  Reads in gradient shape files.			*/
/*	-  scales amplitudes in file according to scale factors */
/*	-  Expands or compresses shapes according to elements	*/
/*	   in the shape and number of ticks for each elements.	*/
/*	   Makes sure all shapes are an integral number of ticks*/
/*	   so each point can be rotated.			*/
/*	-  puts amplitudes through rotation matrix		*/
/*	-  Compress each output element.  In other words if the */
/*	   output amplitude is the same as the previous 	*/
/*	   amplitude, increment the number of ticks.		*/
/*	-  Rescale amplitudes to max amplitude value.		*/
/*	-  Output new shapes to userdir/shapelib with argument	*/
/*	   names pat_x,pat_y,pat_z				*/
/*  RETURN: rescale value					*/
/*--------------------------------------------------------------*/
rotate_shapes(pat1,pat2,pat3,scale1,scale2,scale3,ang1,ang2,ang3,
			pat_x,pat_y,pat_z,scale_x,scale_y,scale_z)
char pat1[],pat2[],pat3[];
double scale1,scale2,scale3;
double ang1,ang2,ang3;
char pat_x[],pat_y[],pat_z[];
double *scale_x,*scale_y,*scale_z;
{
    double m[3][3];
    Gpattern *gpat[3];
    Gpattern *gpatx,*gpaty,*gpatz;
    int nelems[3],maxelems,patmaxelems,extraelems;
    int nticks[3],maxticks,patmaxticks;
    int outelemx,outelemy,outelemz;
    double ampx, ampy, ampz, extratime, rescale_val;
    double maxampx, maxampy,maxampz;
    int i,j,ticks,first;
    char msge[MAXSTR];
    int ix[3];			/* Indices for 3 input gradient patterns */
    int boopatin[3]={0,0,0};	/* True if input pattern [i] has Booster cntl*/
    static int bootrans_plus[] = {0, 1, 2, 3};
    static int bootrans_minus[] = {0, 1, 3, 2};	/* Xform ctl bits for neg amp */
    int *bootrans[3];		/* bootrans[0] = xformation matrix for x */
    int booin[3];		/* booin[0] = logical axis # that maps to x */
    int boox, booy, booz;	/* Booster ctrl bits to output */
    int wantBoost;
    char gboost[3];
    int noscale;
    
    /* Special case do not scale pattern */
    noscale = 0;
    if ((scale1 == 1.0) && (scale2 == 1.0) && (scale3 == 1.0)) noscale=1;

    /*Obtain the transformation matrix*****************/
    obl_matrix(ang1,ang2,ang3,
	       &m[0][0],&m[0][1],&m[0][2],
	       &m[1][0],&m[1][1],&m[1][2],
	       &m[2][0],&m[2][1],&m[2][2]);

    /* get gradient patterns */
    gpat[0] = get_gradient_pattern(pat1,&nelems[0],&nticks[0], &boopatin[0]);
    gpat[1] = get_gradient_pattern(pat2,&nelems[1],&nticks[1], &boopatin[1]);
    gpat[2] = get_gradient_pattern(pat3,&nelems[2],&nticks[2], &boopatin[2]);


    /* determine max ticks and max elements */
    maxticks=0;
    maxelems=0;
    for (i=0; i<3; i++)
    {
 	if (nticks[i] > maxticks) 
	{
	   maxticks = nticks[i];
	   patmaxticks = i;
	}
 	if (nelems[i] > maxelems) 
	{
	   maxelems = nelems[i];
	   patmaxelems = i;
	}
    }

    /* make sure ticks are a multiple of maxticks */
    for (i=0; i<3; i++)
    {
	if ((maxticks % nticks[i]) != 0)
	{
	   char tpat[3][MAXSTR];
	   strcpy(tpat[0],pat1);
	   strcpy(tpat[1],pat2);
	   strcpy(tpat[2],pat3);
	   sprintf(msge,"rotate_shapes: %s ticks:%d and %s ticks:%d\n", 
		tpat[i],nticks[i],tpat[patmaxticks],nticks[patmaxticks]);
	   text_error(msge);
	   text_error  ("               do not have integral num of ticks.");
	   psg_abort(1); 
	}
	else {
	   /* Distribute same number of ticks across patterns */
	   int multicks;
	   multicks = maxticks/nticks[i];
	   if (multicks != 1)
	   {
		for (j=0; j<nelems[i]; j++) gpat[i][j].time *= multicks;
	   }
	}
    }

    /* Check that axes with Booster control only rotated in 90 deg increments.
     * Initialize some Booster related variables.
     */
    boox = booy = booz = 0;
    booin[0] = booin[1] = booin[2] = -1;
    bootrans[0] = bootrans[1] = bootrans[2] = bootrans_plus;

    if (P_getstring(GLOBAL,"gboost",gboost, 1, 3) != 0 || *gboost != 'y') {
        booin[0] = 0;
        booin[1] = 1;
        booin[2] = 2;
    } else {
        if (boopatin[0]){
            /* First logical axis has Booster control */
            /* First column in transformation matrix must have 2 zeros */
            if ((i=non_zero_row(0, m)) < 0){
                text_error("rotate_shapes: axis 1, with Booster control, is rotated to oblique angle.\n");
                psg_abort(1);
            }
            booin[i] = 0;       /* Output axis #i comes from logical #0 */
            bootrans[i] = m[i][0] * scale1 >= 0 ? bootrans_plus : bootrans_minus;
        }
        if (boopatin[1]){
            /* Second logical axis has Booster control */
            /* Second column in transformation matrix must have 2 zeros */
            if ((i=non_zero_row(1, m)) < 0){
                text_error("rotate_shapes: axis 2, with Booster control, is rotated to oblique angle.\n");
                psg_abort(1);
            }
            booin[i] = 1;		/* Output axis #i comes from logical #1 */
            bootrans[i] = m[i][1] * scale2 >= 0 ? bootrans_plus : bootrans_minus;
        }
        if (boopatin[2]){
            /* Third logical axis has Booster control */
            /* Third column in transformation matrix must have 2 zeros */
            if ((i=non_zero_row(2, m)) < 0){
                text_error("rotate_shapes: axis 3, with Booster control, is rotated to oblique angle.\n");
                psg_abort(1);
            }
            booin[i] = 2;		/* Output axis #i comes from logical #2 */
            bootrans[i] = m[i][2] * scale3 >= 0 ? bootrans_plus : bootrans_minus;
        }
    }

    /* Rotate Shapes */
    outelemx = 0;
    outelemy = 0;
    outelemz = 0;
    extraelems = 20;	/* May want to make this a parameter or realloc */
			/* pattern sizes if they overflow.		*/
    gpatx = (Gpattern *) malloc((maxelems+extraelems) * sizeof(Gpattern));
    gpaty = (Gpattern *) malloc((maxelems+extraelems) * sizeof(Gpattern));
    gpatz = (Gpattern *) malloc((maxelems+extraelems) * sizeof(Gpattern));
    gpatx[outelemx].amp = 0.0;	/* init first point */
    gpaty[outelemy].amp = 0.0;	/* init first point */
    gpatz[outelemz].amp = 0.0;	/* init first point */
    gpatx[outelemx].time = 0.0;	/* init first point */
    gpaty[outelemy].time = 0.0;	/* init first point */
    gpatz[outelemz].time = 0.0;	/* init first point */
    gpatx[outelemx].ctrl = 0;	/* init first point */
    gpaty[outelemy].ctrl = 0;	/* init first point */
    gpatz[outelemz].ctrl = 0;	/* init first point */
    first = TRUE;
    maxampx=0.0; maxampy=0.0; maxampz=0.0;
    /* NOTE: ix[n] is the input pattern index for the n'th logical pattern */
    ix[0]=0; ix[1]=0; ix[2]=0;

    /* Scale initial amplitudes */
    gpat[0][ix[0]].amp = scale1*gpat[0][ix[0]].amp;
    gpat[1][ix[1]].amp = scale2*gpat[1][ix[1]].amp;
    gpat[2][ix[2]].amp = scale3*gpat[2][ix[2]].amp;
	
    while ((ix[0] < nelems[0]) && (ix[1] < nelems[1]) && (ix[2] < nelems[2]))
    {
	ticks = get_least_num_ticks( (int)gpat[0][ix[0]].time,
				     (int)gpat[1][ix[1]].time,
				     (int)gpat[2][ix[2]].time);

	if (ticks <= 0)
	{
	   sprintf(msge,
	    "rotate_shapes: Error retrieving ticks p1: %d p2: %d p3: %d.\n",
		   gpat[0][ix[0]].time,gpat[1][ix[1]].time,gpat[2][ix[2]].time);
	   text_error(msge);
	   psg_abort(1); 
	}


	/* Transform logical gradient levels to magnet frame****/

	ampx = floor(gpat[0][ix[0]].amp*m[0][0] + gpat[1][ix[1]].amp*m[0][1]
		     + gpat[2][ix[2]].amp*m[0][2] + 0.5);
	ampy = floor(gpat[0][ix[0]].amp*m[1][0] + gpat[1][ix[1]].amp*m[1][1]
		     + gpat[2][ix[2]].amp*m[1][2] + 0.5);
	ampz = floor(gpat[0][ix[0]].amp*m[2][0] + gpat[1][ix[1]].amp*m[2][1]
		     + gpat[2][ix[2]].amp*m[2][2] + 0.5);

	if (booin[0] >= 0){
	    boox = bootrans[0] [gpat [booin[0]] [ix[booin[0]]].ctrl];
	}
	if (booin[1] >= 0){
	    booy = bootrans[1] [gpat [booin[1]] [ix[booin[1]]].ctrl];
	}
	if (booin[2] >= 0){
	    booz = bootrans[2] [gpat [booin[2]] [ix[booin[2]]].ctrl];
	}
	
	/* Check amplitudes */
	if (maxampx < fabs(ampx)) maxampx = fabs(ampx);
	if (maxampy < fabs(ampy)) maxampy = fabs(ampy);
	if (maxampz < fabs(ampz)) maxampz = fabs(ampz);

	if ((maxampx > MAX_GRAD_AMP) || (maxampy > MAX_GRAD_AMP) ||
						(maxampz > MAX_GRAD_AMP))
	{
	   sprintf(msge,
		"rotate_shapes: pattern amp exceeds limit x:%g y:%g z:%g\n",
		ampx,ampy,ampz); 
	   psg_abort(1);
	}

	if (ampx == gpatx[outelemx].amp && boox == gpatx[outelemx].ctrl) {
	   gpatx[outelemx].time += (double) ticks;
	   if (gpatx[outelemx].time > 255.0) {
		extratime = gpatx[outelemx].time - 255.0;
		gpatx[outelemx].time = 255.0;
		outelemx++;
		gpatx[outelemx].time = extratime;
		gpatx[outelemx].amp = ampx;
		gpatx[outelemx].ctrl = boox;
	   } 
	}
	else {
	   if (!first) outelemx++;
	   gpatx[outelemx].time = (double) ticks;
	   gpatx[outelemx].amp = ampx;              
	   gpatx[outelemx].ctrl = boox;
	}

	if (ampy == gpaty[outelemy].amp && booy == gpaty[outelemy].ctrl) {
	   gpaty[outelemy].time += (double) ticks;
	   if (gpaty[outelemy].time > 255.0) {
		extratime = gpaty[outelemy].time - 255.0;
		gpaty[outelemy].time = 255.0;
		outelemy++;
		gpaty[outelemy].time = extratime;
		gpaty[outelemy].amp = ampy;
		gpaty[outelemy].ctrl = booy;
	   }
	}
	else {
	   if (!first) outelemy++;
	   gpaty[outelemy].time = (double) ticks;
	   gpaty[outelemy].amp = ampy;
	   gpaty[outelemy].ctrl = booy;
	}

	if (ampz == gpatz[outelemz].amp && booz == gpatz[outelemz].ctrl) {
	   gpatz[outelemz].time += (double) ticks;
	   if (gpatz[outelemz].time > 255.0) {
		extratime = gpatz[outelemz].time - 255.0;
		gpatz[outelemz].time = 255.0;
		outelemz++;
		gpatz[outelemz].time = extratime;
		gpatz[outelemz].amp = ampz;
		gpatz[outelemz].ctrl = booz;
	   }
	}
	else {
	   if (!first) outelemz++;
	   gpatz[outelemz].time = (double) ticks;
	   gpatz[outelemz].amp = ampz;
	   gpatz[outelemz].ctrl = booz;
	}

	if (first) first = FALSE;

	gpat[0][ix[0]].time -= ticks;
	gpat[1][ix[1]].time -= ticks;
	gpat[2][ix[2]].time -= ticks;

	/* Check for new element */
	if (gpat[0][ix[0]].time == 0)
	{
	   ix[0]++;
	   gpat[0][ix[0]].amp = scale1*gpat[0][ix[0]].amp; /* scale new amp */
	}
	if (gpat[1][ix[1]].time == 0)
        {
	   ix[1]++;
	   gpat[1][ix[1]].amp = scale2*gpat[1][ix[1]].amp; /* scale new amp */
	}
	if (gpat[2][ix[2]].time == 0)
	{
	   ix[2]++;
	   gpat[2][ix[2]].amp = scale3*gpat[2][ix[2]].amp; /* scale new amp */
	}


	if ((outelemx >= (maxelems+extraelems)) || (outelemy >= 
		(maxelems+extraelems)) || (outelemz >= (maxelems+extraelems)))
	{
	   /* for now abort, but we may want to realloc the shapes */
	   sprintf(msge,
	 "rotate_shapes: output elems x:%d y:%d z:%d larger than allocated.\n",
			outelemx,outelemy,outelemz);
	   text_error(msge);
	   psg_abort(1); 
		
	}

    }
    /* Increment the element counters */
    outelemx++;
    outelemy++;
    outelemz++;

    /* free initial shapes */
    for (i=0; i<3; i++) 
	{
	free(gpat[i]);
	}

    /* rescale amplitudes to maxamp */
    if (noscale) {
    	*scale_x = floor(1.0*m[0][0] + 1.0*m[0][1] + 1.0*m[0][2] + 0.5);
	*scale_y = floor(1.0*m[1][0] + 1.0*m[1][1] + 1.0*m[1][2] + 0.5);
	*scale_z = floor(1.0*m[2][0] + 1.0*m[2][1] + 1.0*m[2][2] + 0.5);
    }

    if (maxampx != 0){
        if (noscale==0)
           *scale_x = MAX_GRAD_AMP/maxampx;
	scaleamp(gpatx,outelemx,*scale_x);
    }
    if (maxampy != 0){
        if (noscale==0)
	   *scale_y = MAX_GRAD_AMP/maxampy;
	scaleamp(gpaty,outelemy,*scale_y);
    }
    if (maxampz != 0){
        if (noscale==0)
	   *scale_z = MAX_GRAD_AMP/maxampz;
	scaleamp(gpatz,outelemz,*scale_z);
    }

/*  write transformed gradient shapes to local shapelib   */ 
    if (booin[0] >= 0){
	init_Bpattern(pat_x, gpatx, outelemx);
    }else{
	init_Gpattern(pat_x, gpatx, outelemx);
    }
    if (booin[1] >= 0){
	init_Bpattern(pat_y, gpaty, outelemy);
    }else{
	init_Gpattern(pat_y, gpaty, outelemy);
    }
    if (booin[2] >= 0){
	init_Bpattern(pat_z, gpatz, outelemz);
    }else{
	init_Gpattern(pat_z, gpatz, outelemz);
    }

    /* free transformed gradient shapes */
    free(gpatx);
    free(gpaty);
    free(gpatz);

    return;
}

/****************************************************************/
/* create_rpat_names()					*/
/*	Creates x,y,z pattern names and has to include all	*/
/*	the factors that influence the final shape of the	*/
/*	x,y,z gradient patterns.  These are initial shape,	*/
/*	amplitude, and phase angle.				*/
/*								*/
/*	In order to compress the final name somewhat zero	*/
/*	values for amplitude and angle are ignored.		*/
/****************************************************************/
create_rpat_names(pat_x,pat_y,pat_z,
		pat1,pat2,pat3,lvl1,lvl2,lvl3,ang1,ang2,ang3)
char pat_x[],pat_y[],pat_z[];
char pat1[],pat2[],pat3[];
double lvl1,lvl2,lvl3;
double ang1,ang2,ang3;
{
    char tempstr[MAXSTR];
    strcpy(pat_x,pat1);
    strcat(pat_x,"_");
    if (lvl1 != 0.0)
    {
	sprintf(tempstr,"%-.2f",lvl1);
	strcat(pat_x,tempstr);
    }
    strcat(pat_x,"_");
    if (ang1 != 0.0)
    {
	sprintf(tempstr,"%-.2f",ang1);
	strcat(pat_x,tempstr);
    }
    strcat(pat_x,"_");
    if (strlen(pat_x) > MAXSTR) 
    {
       text_error("create_rpat_names: pat1 name exceeds string limit.");
       psg_abort(1);
    }

    /* concat 2nd pat info */
    strcat(pat_x,pat2);
    strcat(pat_x,"_");
    if (lvl2 != 0.0)
    {
	sprintf(tempstr,"%-.2f",lvl2);
	strcat(pat_x,tempstr);
    }
    strcat(pat_x,"_");
    if (ang2 != 0.0)
    {
	sprintf(tempstr,"%-.2f",ang2);
	strcat(pat_x,tempstr);
    }
    strcat(pat_x,"_");
    if (strlen(pat_x) > MAXSTR) 
    {
       text_error("create_rpat_names: pat1,pat2 names exceed string limit.");
       psg_abort(1);
    }

    /* concat 3rd pat info */
    strcat(pat_x,pat3);
    strcat(pat_x,"_");
    if (lvl3 != 0.0)
    {
	sprintf(tempstr,"%-.2f",lvl3);
	strcat(pat_x,tempstr);
    }
    strcat(pat_x,"_");
    if (ang3 != 0.0)
    {
	sprintf(tempstr,"%-.2f",ang3);
	strcat(pat_x,tempstr);
    }
    strcat(pat_x,"_");
    strcpy(pat_y,pat_x);
    strcpy(pat_z,pat_x);
    
    strcat(pat_x,"x");
    strcat(pat_y,"y");
    strcat(pat_z,"z");

    if (strlen(pat_x) > MAXSTR) 
    {
       text_error("create_rpat_names: pat1,pat2,pat3 names exceed string limit.");
       psg_abort(1);
    }

}

remove_rotated_patterns(pat_x,pat_y,pat_z)
char pat_x[],pat_y[],pat_z[];
{
    char    file_name[MAXSTR];
/*-------------------------------------------------------------
  Assemble absolute path name for rotated gradient pattern files
-------------------------------------------------------------*/
    /* x */
    strcpy (file_name, userdir);
    strcat (file_name, "/shapelib/");
    strcat (file_name, pat_x);
    strcat (file_name, ".GRD");
    unlink(file_name);
    /* y */
    strcpy (file_name, userdir);
    strcat (file_name, "/shapelib/");
    strcat (file_name, pat_y);
    strcat (file_name, ".GRD");
    unlink(file_name);
    /* z */
    strcpy (file_name, userdir);
    strcat (file_name, "/shapelib/");
    strcat (file_name, pat_z);
    strcat (file_name, ".GRD");
    unlink(file_name);
}

static struct obl_list *obl_list_search(pat1,pat2,pat3,
					lvl1,lvl2,lvl3,ang1,ang2,ang3)
   char pat1[],pat2[],pat3[];
   double lvl1,lvl2,lvl3;
   double ang1,ang2,ang3;
{
   struct obl_list *scan;

   scan = obl_list_head;
   
   while ((scan != NULL) &&
          ((scan->lvl1 != lvl1) || (scan->lvl2 != lvl2) ||
	   (scan->lvl3 != lvl3) || (scan->ang1 != ang1) ||
	   (scan->ang2 != ang2)     || (scan->ang3 != ang3) ||
	    strcmp(scan->pat1,pat1) || strcmp(scan->pat2,pat2) ||
	     strcmp(scan->pat3,pat3)) )
   {
     scan = scan->obl_link;
   }
   return((struct obl_list *) scan);
}

static int add_obl_shape(pat1,pat2,pat3,lvl1,lvl2,lvl3,
		ang1,ang2,ang3,pat_x,pat_y,pat_z,xlevel,ylevel,zlevel)
   char pat1[],pat2[],pat3[];
   double lvl1,lvl2,lvl3;
   double ang1,ang2,ang3;
   char pat_x[],pat_y[],pat_z[];
   double xlevel,ylevel,zlevel;
{
   struct obl_list *scan,*prev;

   prev = obl_list_head;
   scan = obl_list_head;
   while (scan != NULL)
   {
     prev = scan;
     scan = scan->obl_link;
   }
   scan = (struct obl_list *) malloc(sizeof(struct obl_list));
   if (scan == NULL)
   {
       text_error("add_obl_shape: Could not create obl list entry.");
       psg_abort(1);
   }
   /* insert info into list element */
   scan->lvl1 = lvl1;
   scan->lvl2 = lvl2;
   scan->lvl3 = lvl3;
   scan->ang1 = ang1;
   scan->ang2 = ang2;
   scan->ang3 = ang3;
   strcpy(scan->pat1,pat1);
   strcpy(scan->pat2,pat2);
   strcpy(scan->pat3,pat3);
   strcpy(scan->pat_x,pat_x);
   strcpy(scan->pat_y,pat_y);
   strcpy(scan->pat_z,pat_z);
   scan->xlevel = xlevel;
   scan->ylevel = ylevel;
   scan->zlevel = zlevel;
   scan->obl_link = NULL;
   if (	obl_list_head == NULL)	
      obl_list_head = scan;
   else
      prev->obl_link = scan;
   return(0);
}

close_obl_list()
{
   struct obl_list *scan,*prev;

   scan = obl_list_head;
   while (scan != NULL)
   {

     if (!bgflag)
	remove_rotated_patterns(scan->pat_x,scan->pat_y,scan->pat_z);     
     prev = scan;
     scan = scan->obl_link;
     free(prev);
     prev = NULL;
   }
}


        /*********************************************************
                          oblique_gradient()

               Procedure to set an oblique static gradient level.
        **********************************************************/

void S_oblique_gradient(double level1,double level2,double level3,
                        double ang1,double ang2,double ang3)
{
    double xlevel,ylevel,zlevel;
    double Gx_dac, Gy_dac, Gz_dac;
    double m11,m12,m13,m21,m22,m23,m31,m32,m33;

    Gx_dac = gauss_dac('x');
    Gy_dac = gauss_dac('y');
    Gz_dac = gauss_dac('z');

    /*Obtain the transformation matrix*****************/
    obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);

    /*Transform logical gradient levels to magnet frame****/
    xlevel = level1*m11 + level2*m12 + level3*m13;
    ylevel = level1*m21 + level2*m22 + level3*m23;
    zlevel = level1*m31 + level2*m32 + level3*m33;

    if (dps_flag)
    {
        dps_fval[0] = Gx_dac*xlevel;
        dps_fval[1] = Gy_dac*ylevel;
        dps_fval[2] = Gz_dac*zlevel;
        return;
    }

    grad_limit_checks(xlevel,ylevel,zlevel,"oblique_gradient");

    if (fabs(Gx_dac*xlevel)>gradstepsz ) {
        text_error("oblique_gradient: gradient x value exceeds DAC limit.");
        psg_abort(1);
    }
    if (fabs(Gy_dac*ylevel)>gradstepsz) {
        text_error("oblique_gradient: gradient y value exceeds DAC limit.");
        psg_abort(1);
    }
    if (fabs(Gz_dac*zlevel)>gradstepsz) {
        text_error("oblique_gradient: gradient z value exceeds DAC limit.");
        psg_abort(1);
    }

    if (bgflag)
       fprintf(stderr,"oblique_gradient: x=%d y=%d z=%d dac units\n",
	 (int)(Gx_dac*xlevel),(int)(Gy_dac*ylevel),(int)(Gz_dac*zlevel));

/**************************  begin for coordinate rotator board   *************/

    if (anygradcrwg)
        {
	gradient('x',(int)(Gx_dac*level1));
        gradient('y',(int)(Gy_dac*level2));
        gradient('z',(int)(Gz_dac*level3));
        }

/**************************  end for coordinate rotator board   *************/

    else 
        {
        /*Set gradients in magnet frame*******************/
        gradient('x',(int)(Gx_dac*xlevel));
        gradient('y',(int)(Gy_dac*ylevel));
        gradient('z',(int)(Gz_dac*zlevel));
        }
}


        /***********************************************************
                        oblique_shapedgradient()

                Procedure to provide an oblique gradient pulse
                modulated using the gradient waveshaping hardware.
        ***********************************************************/

S_oblique_shapedgradient(pat,width,lvl1,lvl2,lvl3,ang1,ang2,ang3,loops,wait)

  char pat[];
double width;
double lvl1,lvl2,lvl3;
double ang1,ang2,ang3;
   int loops,wait;
{
    double xlevel,ylevel,zlevel;
    double Gx_dac, Gy_dac, Gz_dac;
    double m11,m12,m13,m21,m22,m23,m31,m32,m33;
    double offset1,offset2,offset3;

    Gx_dac = gauss_dac('x');
    Gy_dac = gauss_dac('y');
    Gz_dac = gauss_dac('z');

    /*Obtain the transformation matrix*****************/
    obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);

    /*Transform logical gradient levels to magnet frame****/
    xlevel = (lvl1*m11 + lvl2*m12 + lvl3*m13);
    ylevel = (lvl1*m21 + lvl2*m22 + lvl3*m23);
    zlevel = (lvl1*m31 + lvl2*m32 + lvl3*m33);

    if (dps_flag)
    {
        dps_fval[0] = Gx_dac*xlevel;
        dps_fval[1] = Gy_dac*ylevel;
        dps_fval[2] = Gz_dac*zlevel;
        return;
    }

    grad_limit_checks(xlevel,ylevel,zlevel,"oblique_shapedgradient");

    xlevel = Gx_dac*xlevel;
    ylevel = Gy_dac*ylevel;
    zlevel = Gz_dac*zlevel;

    if (fabs(xlevel)>gradstepsz ) {
       text_error("oblique_shapedgradient: gradient x value exceeds DAC limit.");
       psg_abort(1);
    }
    if (fabs(ylevel)>gradstepsz) {
       text_error("oblique_shapedgradient: gradient y value exceeds DAC limit.");
       psg_abort(1);
    }
    if (fabs(zlevel)>gradstepsz) {
       text_error("oblique_shapedgradient: gradient z value exceeds DAC limit.");
       psg_abort(1);
    }

    if (bgflag)
       fprintf(stderr,"oblique_shapedgradient: x=%d y=%d z=%d dac units\n",
	  (int)(xlevel),(int)(ylevel),(int)(zlevel));

/**************************  begin for coordinate rotator board   *************/

    if (anygradcrwg)
        {
    	lvl1 = Gx_dac*lvl1;
    	lvl2 = Gy_dac*lvl2;
    	lvl3 = Gz_dac*lvl3;

        shapedgradient(pat, width, lvl1, 'x', loops, NOWAIT);
        shapedgradient(pat, width, lvl2, 'y', loops, NOWAIT);
        shapedgradient(pat, width, lvl3, 'z', loops, wait);
        }

/**************************  end for coordinate rotator board   *************/

    else
        {
        /*Set gradients in magnet frame*******************/
        shapedgradient(pat, width, xlevel, 'x', loops, NOWAIT);
        shapedgradient(pat, width, ylevel, 'y', loops, NOWAIT);
        shapedgradient(pat, width, zlevel, 'z', loops, wait);
        }

}


    
        /***********************************************************
                        oblique_shaped3gradient()

                Procedure to provide an oblique modulated gradient pulse
                for gradient shapes for three axes.

        ***********************************************************/

S_oblique_shaped3gradient(pat1,pat2,pat3,width,lvl1,lvl2,lvl3,ang1,ang2,ang3,
								loops,wait)
char pat1[],pat2[],pat3[];
double width;
double lvl1,lvl2,lvl3;
double ang1,ang2,ang3;
int loops,wait;
{
    double xlevel,ylevel,zlevel;
    double xlvl,ylvl,zlvl;
    double Gx_dac, Gy_dac, Gz_dac;
    double scale1,scale2,scale3,scale_x,scale_y,scale_z;
    double m11,m12,m13,m21,m22,m23,m31,m32,m33;
    char patx[MAXSTR],paty[MAXSTR],patz[MAXSTR];
    char pat_x[MAXSTR],pat_y[MAXSTR],pat_z[MAXSTR],tmpstr[MAXSTR];
    Gpattern *gpat[3];
    int boopatin[3]={0,0,0};	/* True if input pattern [i] has Booster cntl*/
    int nelems[3];
    int nticks[3];
    struct obl_list  *obl_shape_ptr;



/**************************  begin for coordinate rotator board   *************/

    if (anygradcrwg)
    {

        Gx_dac = gauss_dac('x');
        Gy_dac = gauss_dac('y');
        Gz_dac = gauss_dac('z');
 
        /*Obtain the transformation matrix*****************/
        obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);
     
        /*Transform logical gradient levels to magnet frame****/
        xlevel = (lvl1*m11 + lvl2*m12 + lvl3*m13);
        ylevel = (lvl1*m21 + lvl2*m22 + lvl3*m23);
        zlevel = (lvl1*m31 + lvl2*m32 + lvl3*m33);
 
        if (dps_flag)
        {
            dps_fval[0] = Gx_dac*xlevel;
            dps_fval[1] = Gy_dac*ylevel;
            dps_fval[2] = Gz_dac*zlevel;
            return;
        }
 
        grad_limit_checks(xlevel,ylevel,zlevel,"oblique_shaped3gradient");
 
    	xlevel = Gx_dac*xlevel;
    	ylevel = Gy_dac*ylevel;
    	zlevel = Gz_dac*zlevel;
 
    	if (fabs(xlevel)>gradstepsz ) {
       		text_error("oblique_shaped3gradient: gradient x value exceeds DAC limit.");
       		psg_abort(1);
    	}
    	if (fabs(ylevel)>gradstepsz) {
       		text_error("oblique_shaped3gradient: gradient y value exceeds DAC limit.");
       		psg_abort(1);
    	}
    	if (fabs(zlevel)>gradstepsz) {
       		text_error("oblique_shaped3gradient: gradient z value exceeds DAC limit.");
       		psg_abort(1);
    	}
 
    	if (bgflag)
       		fprintf(stderr,"oblique_shaped3gradient: x=%d y=%d z=%d dac units\n",
          		(int)(xlevel),(int)(ylevel),(int)(zlevel));
 

    	lvl1 = Gx_dac*lvl1;
    	lvl2 = Gy_dac*lvl2;
    	lvl3 = Gz_dac*lvl3;

        if ((int)strlen(pat1) < 1)                             /* NULL pattern name */
        { 
            if ((int)strlen(pat2) < 1)  			/* NULL pattern name */
	    {
                 if ((int)strlen(pat3) < 1)  			/* NULL pattern name */
		 {
                 	text_error("All 3 pattern names are null in pulse sequence \n");
			psg_abort(1);
		 }
	         else 
		 {
			pat1 = pat3;
		 	lvl1 = 0.0;
		 }
	    }   
	    else 
	    {
		pat1 = pat2;
	    	lvl1 = 0.0;
	    }
	}

        if ((int)strlen(pat2) < 1)                             /* NULL pattern name */
        { 
            if ((int)strlen(pat3) < 1)  			/* NULL pattern name */
	    {
                 if ((int)strlen(pat1) < 1)  			/* NULL pattern name */
		 {
                 	text_error("All 3 pattern names are null in pulse sequence \n");
			psg_abort(1);
		 }
	         else 
		 {
			pat2 = pat1;
		 	lvl2 = 0.0;
		 }
	    }   
	    else 
	    {
		pat2 = pat3;
	    	lvl2 = 0.0;
	    }
	}

        if ((int)strlen(pat3) < 1)                             /* NULL pattern name */
        { 
            if ((int)strlen(pat1) < 1)  			/* NULL pattern name */
	    {
                 if ((int)strlen(pat2) < 1)  			/* NULL pattern name */
		 {
                 	text_error("All 3 pattern names are null in pulse sequence \n");
			psg_abort(1);
		 }
	         else 
		 {
			pat3 = pat2;
		 	lvl3 = 0.0;
		 }
	    }   
	    else 
	    {
		pat3 = pat1;
	    	lvl3 = 0.0;
	    }
	}

	shapedgradient(pat1, width, lvl1, 'x', loops, NOWAIT);
        shapedgradient(pat2, width, lvl2, 'y', loops, NOWAIT);
        shapedgradient(pat3, width, lvl3, 'z', loops, wait);

    }

/**************************  end for coordinate rotator board   *************/

    else
    {
    
        /*----------------------------------------------------------*/
        /* Check to see if oblique shape has already been defined.	*/
        /* If it has, call that shape.  Otherwise the routine will	*/
        /* will continue and create and define the requested shape.	*/
        /*----------------------------------------------------------*/
        obl_shape_ptr = obl_list_search(pat1,pat2,pat3,lvl1,lvl2,lvl3,
						ang1,ang2,ang3);

        if (obl_shape_ptr != NULL)
        {

    	    /*Set gradients in magnet frame*******************/
    	    shapedgradient(obl_shape_ptr->pat_x,width,obl_shape_ptr->xlevel,
					 	'x',loops,NOWAIT);
    	    shapedgradient(obl_shape_ptr->pat_y,width,obl_shape_ptr->ylevel,
						'y',loops, NOWAIT);
    	    shapedgradient(obl_shape_ptr->pat_z,width,obl_shape_ptr->zlevel,
						'z',loops,wait);
	    return;
        }

        create_rpat_names(pat_x,pat_y,pat_z,
			pat1,pat2,pat3,lvl1,lvl2,lvl3,ang1,ang2,ang3);

        scale1 = lvl1/gmax;
        scale2 = lvl2/gmax;
        scale3 = lvl3/gmax;
        scale_x = 0.0;
        scale_y = 0.0;
        scale_z = 0.0;

        rotate_shapes(pat1,pat2,pat3,scale1,scale2,scale3,ang1,ang2,ang3,
		  	pat_x,pat_y,pat_z,&scale_x,&scale_y,&scale_z);

        if (scale_x != 0.0) xlevel = gmax/scale_x;
        else xlevel = 0.0;
        if (scale_y != 0.0) ylevel = gmax/scale_y;
        else ylevel = 0.0;
        if (scale_z != 0.0) zlevel = gmax/scale_z;
        else zlevel = 0.0;

        Gx_dac = gauss_dac('x');
        Gy_dac = gauss_dac('y');
        Gz_dac = gauss_dac('z');

        if (dps_flag)
        {
            dps_fval[0] = Gx_dac*xlevel;
            dps_fval[1] = Gy_dac*ylevel;
            dps_fval[2] = Gz_dac*zlevel;
            return;
        }

        grad_limit_checks(xlevel,ylevel,zlevel,"oblique_shapedgradient");

        xlevel = Gx_dac*xlevel;
        ylevel = Gy_dac*ylevel;
        zlevel = Gz_dac*zlevel;

        if (fabs(xlevel)>gradstepsz ) {
           text_error("oblique_shaped3gradient: gradient x value exceeds DAC limit.");
           if (!bgflag)
	       remove_rotated_patterns(pat_x,pat_y,pat_z);
           psg_abort(1);
        }
        if (fabs(ylevel)>gradstepsz) {
           text_error("oblique_shaped3gradient: gradient y value exceeds DAC limit.");
           if (!bgflag)
	       remove_rotated_patterns(pat_x,pat_y,pat_z);
           psg_abort(1);
        }
        if (fabs(zlevel)>gradstepsz) {
           text_error("oblique_shaped3gradient: gradient z value exceeds DAC limit.");
           if (!bgflag)
	       remove_rotated_patterns(pat_x,pat_y,pat_z);
           psg_abort(1);
        }

        if (bgflag)
           fprintf(stderr,"oblique_shaped3gradient: x=%d y=%d z=%d dac units\n",
	      (int)(xlevel),(int)(ylevel),(int)(zlevel));


        /*Set gradients in magnet frame*******************/
        /* get gradient patterns */
        gpat[0] = get_gradient_pattern(pat_x,&nelems[0],&nticks[0], &boopatin[0]);
        gpat[1] = get_gradient_pattern(pat_y,&nelems[1],&nticks[1], &boopatin[1]);
        gpat[2] = get_gradient_pattern(pat_z,&nelems[2],&nticks[2], &boopatin[2]);
    
        shapedgradient(pat_x, width, xlevel, 'x', loops, NOWAIT);
        shapedgradient(pat_y, width, ylevel, 'y', loops, NOWAIT);
        shapedgradient(pat_z, width, zlevel, 'z', loops, wait);

        add_obl_shape(pat1,pat2,pat3,lvl1,lvl2,lvl3,ang1,ang2,ang3,
		  	pat_x,pat_y,pat_z,xlevel,ylevel,zlevel);
    }
}

        /*********************************************************
                             phase_encode_gradient()

             Procedure to set static oblique gradient levels plus
                    one oblique phase encode gradient.

             The phase encode gradient is associated with the
             second axis of the logical frame.
        **********************************************************/

S_phase_encode_gradient(stat1,stat2,stat3,step2,vmult2,lim2,ang1,ang2,ang3)

double  lim2,stat1,stat2,stat3,step2,ang1,ang2,ang3;
codeint vmult2;
{
    double   Gx_dac, Gy_dac, Gz_dac;
    double   m11,m12,m13,m21,m22,m23,m31,m32,m33;
    double   statX,statY,statZ,stepX,stepY,stepZ;
    double   pedacstep,tstlvl2;

    Gx_dac = gauss_dac('x');
    Gy_dac = gauss_dac('y');
    Gz_dac = gauss_dac('z');

    /* Obtain the transformation matrix*****************/
    obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);

    /*Transform logical gradient levels to magnet frame****/
    statX = (stat1*m11 + stat2*m12 + stat3*m13);
    statY = (stat1*m21 + stat2*m22 + stat3*m23);
    statZ = (stat1*m31 + stat2*m32 + stat3*m33);

    stepX = m12*step2;
    stepY = m22*step2;
    stepZ = m32*step2;

    grad_limit_checks(statX+(stepX*lim2),
			statY+(stepY*lim2),
			statZ+(stepZ*lim2),"phase_encode_gradient");

    /* Convert to DAC units *****************/
    statX = Gx_dac*statX;
    statY = Gy_dac*statY;
    statZ = Gz_dac*statZ;

    stepX = Gx_dac*stepX;
    stepY = Gy_dac*stepY;
    stepZ = Gz_dac*stepZ;

    if ( fabs(statX + (stepX*lim2)) > gradstepsz ) {

        text_error("phase_encode_gradient: gradient x value exceeds DAC limit.");
        psg_abort(1);
    }
    if ( fabs(statY + (stepY*lim2)) > gradstepsz ) {

        text_error("phase_encode_gradient: gradient y value exceeds DAC limit.");
        psg_abort(1);
    }
    if ( fabs(statZ + (stepZ*lim2)) > gradstepsz ) {

        text_error("phase_encode_gradient: gradient z value exceeds DAC limit.");
        psg_abort(1);
    }

    if (bgflag)
    {
       fprintf(stderr,"phase_encode_gradient: start x=%d y=%d z=%d dac units\n",
	  (int)(statX),(int)(statY),(int)(statZ));
       fprintf(stderr,"phase_encode_gradient: step x=%d y=%d z=%d dac units\n",
	  (int)(stepX),(int)(stepY),(int)(stepZ));
    }

/************************** begin for coordinate rotator board   *************/

    if (anygradcrwg)
    {
	tstlvl2 = 1.0*stat2;
    	stat1 = Gx_dac*stat1;
    	stat2 = Gy_dac*stat2;
    	stat3 = Gz_dac*stat3;
        pedacstep = Gy_dac*step2;

        if ( fabs(stat2 + (pedacstep*lim2)) > gradstepsz ) {
           text_error("phase_encode__gradient exceeds DAC limit.");
                psg_abort(1);
        }

        if (((tstlvl2 < 0.0) && (stat2 > 0.0)) ||
            ((tstlvl2 > 0.0) && (stat2 < 0.0)))
                        pedacstep = pedacstep*(-1.0);

        vgradient('x',stat1,0.0,vmult2);
        vgradient('y',stat2,pedacstep,vmult2);
        vgradient('z',stat3,0.0,vmult2);
    }

/************************** end for coordinate rotator board   *************/

    else
    {

        /* Set gradients in magnet frame*******************/
        vgradient('x', statX, stepX, vmult2);
        vgradient('y', statY, stepY, vmult2);
        vgradient('z', statZ, stepZ, vmult2);
    }
}



        /*********************************************************
                         phase_encode_shapedgradient()

             Procedure to set static oblique gradient levels plus
                    one oblique phase encode gradient.

             The phase encode gradient is associated with the
             second axis of the logical frame.
        **********************************************************/

S_phase_encode_shapedgradient(pat,width,stat1,stat2,stat3,step2,
        vmult2,lim2,ang1,ang2,ang3,vloops,wait,tag)

char    pat[];
int     tag,wait;
double  lim2,stat1,stat2,stat3,step2,ang1,ang2,ang3,width;
codeint vmult2,vloops;
{
    double   Gx_dac, Gy_dac, Gz_dac;
    double   xlvl,ylvl,zlvl,xstep,ystep,zstep;
    double   m11,m12,m13,m21,m22,m23,m31,m32,m33;
    double   statX,statY,statZ,stepX,stepY,stepZ;
    double   pestep,pedacstep,tstlvl2;
    char     pat1[MAXSTR],pat2[MAXSTR],pat3[MAXSTR],tmpstr[MAXSTR];
    struct   obl_list  *obl_shape_ptr;
    int      loops,xpe,ype,zpe;

    loops = 1;

    Gx_dac = gauss_dac('x');
    Gy_dac = gauss_dac('y');
    Gz_dac = gauss_dac('z');

    /* Obtain the transformation matrix*****************/
    obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);

    /* Transform logical gradient levels to magnet frame****/
    statX = (stat1*m11 + stat2*m12 + stat3*m13);
    statY = (stat1*m21 + stat2*m22 + stat3*m23);
    statZ = (stat1*m31 + stat2*m32 + stat3*m33);

    stepX = m12*step2;
    stepY = m22*step2;
    stepZ = m32*step2;

    grad_limit_checks(statX+(stepX*lim2),
			statY+(stepY*lim2),
			statZ+(stepZ*lim2),"phase_encode_shapedgradient");

    /* Convert to DAC units *****************/
    statX = Gx_dac*statX;
    statY = Gy_dac*statY;
    statZ = Gz_dac*statZ;

    stepX = Gx_dac*stepX;
    stepY = Gy_dac*stepY;
    stepZ = Gz_dac*stepZ;

    if ( fabs(statX + (stepX*lim2)) > gradstepsz ) {

        text_error("phase_encode_shapedgradient: x value exceeds DAC limit.");
        psg_abort(1);
    }
    if ( fabs(statY + (stepY*lim2)) > gradstepsz ) {

        text_error("phase_encode_shapedgradient: y value exceeds DAC limit.");
        psg_abort(1);
    }
    if ( fabs(statZ + (stepZ*lim2)) > gradstepsz ) {

        text_error("phase_encode_shapedgradient: z value exceeds DAC limit.");
        psg_abort(1);
    }


    if (bgflag)
    {
      fprintf(stderr,"phase_encode_shapedgradient: start x=%d y=%d z=%d dac units\n",
	(int)(statX),(int)(statY),(int)(statZ));
      fprintf(stderr,"phase_encode_shapedgradient: step x=%d y=%d z=%d dac units\n",
	(int)(stepX),(int)(stepY),(int)(stepZ));
    }

/************************** begin for coordinate rotator board   *************/
 
    if (anygradcrwg)
    {
        tstlvl2 = 1.0*stat2;
        statX = Gx_dac*stat1;
        statY = Gy_dac*stat2;
        statZ = Gz_dac*stat3;
        pedacstep = Gy_dac*step2;
 
        if (((tstlvl2 < 0.0) && (statY > 0.0)) ||
            ((tstlvl2 > 0.0) && (statY < 0.0)))
                        pedacstep = pedacstep*(-1.0);

    	prepWFGforPE(pat,width,statY,pedacstep,vmult2,'y',vloops,NOWAIT,tag);
        shapedgradient(pat, width, statX, 'x', loops, NOWAIT);
    	doshapedPEgradient(pat,width,statY,pedacstep,vmult2,'y',vloops,NOWAIT,tag);
    	shapedgradient(pat, width, statZ, 'z', loops, wait);
        return;
    }

/************************** end for coordinate rotator board   *************/

    else 
    {

       xpe = 0; ype = 0; zpe = 0;
    
       if (m12 != 0.0)
       {
           xpe = 1;
           pestep = m12*step2;
           pedacstep = Gx_dac*pestep;
           tstlvl2 = m12*stat2; /* used for sign check of phase encode steps */
       }
       if (m22 != 0.0)
       {
           ype = 1;
           pestep = m22*step2;
           pedacstep = Gy_dac*pestep;
           tstlvl2 = m22*stat2; /* used for sign check of phase encode steps */
       }
       if (m32 != 0.0)
       {   
           zpe = 1;
           pestep = m32*step2;
           pedacstep = Gz_dac*pestep;
           tstlvl2 = m32*stat2; /* used for sign check of phase encode steps */
       }

       if ((xpe+ype+zpe) > 1){
           text_error("pe_shapedgradient: Axes not aligned along x,y,z.");
           psg_abort(1);
       }

       if (xpe) 
       {
              if (((tstlvl2 < 0.0) && (statX > 0.0)) ||
               ((tstlvl2 > 0.0) && (statX < 0.0)))
                                   pedacstep = pedacstep*(-1.0);
             
      	      prepWFGforPE(pat,width,statX,pedacstep,vmult2,'x',vloops,NOWAIT,tag);
              doshapedPEgradient(pat,width,statX,pedacstep,vmult2,'x',vloops,NOWAIT,tag);
    	      shapedgradient(pat, width, statY, 'y', loops, NOWAIT);
    	      shapedgradient(pat, width, statZ, 'z', loops, wait);
       }


       else if (ype) 
       {
              if (((tstlvl2 < 0.0) && (statY > 0.0)) ||
               ((tstlvl2 > 0.0) && (statY < 0.0)))
                                   pedacstep = pedacstep*(-1.0);
             
      	      prepWFGforPE(pat,width,statY,pedacstep,vmult2,'y',vloops,NOWAIT,tag);
    	      shapedgradient(pat, width, statX, 'x', loops, NOWAIT);
              doshapedPEgradient(pat,width,statY,pedacstep,vmult2,'y',vloops,NOWAIT,tag);
    	      shapedgradient(pat, width, statZ, 'z', loops, wait);
       }
       else if (zpe) 
       {
              if (((tstlvl2 < 0.0) && (statZ > 0.0)) ||
               ((tstlvl2 > 0.0) && (statZ < 0.0)))
                                   pedacstep = pedacstep*(-1.0);
             
      	      prepWFGforPE(pat,width,statZ,pedacstep,vmult2,'z',vloops,NOWAIT,tag);
    	      shapedgradient(pat, width, statX, 'x', loops, NOWAIT);
    	      shapedgradient(pat, width, statY, 'y', loops, NOWAIT);
              doshapedPEgradient(pat,width,statZ,pedacstep,vmult2,'z',vloops,wait,tag);
       }
    }
}


        /***********************************************************
                        pe_oblique_shaped3gradient()

        Procedure to provide a phase encode oblique 
        modulated gradient pulse for gradient shapes for three axes.
	The gradient shapes can be rotated on to any axis but they always
	have to end up aligned along the x, y,and z axes.
	
        ***********************************************************/

S_pe_oblique_shaped3gradient(pat1,pat2,pat3,width,lvl1,lvl2,lvl3,
			step2,vmult2,lim2,ang1,ang2,ang3,wait,tag)
char pat1[],pat2[],pat3[];
double width;
double lvl1,lvl2,lvl3;
double ang1,ang2,ang3;
double step2,lim2;
int wait,tag;
codeint vmult2;
{
    double xlevel,ylevel,zlevel,pestep,pedacstep,tstlvl2;
    double Gx_dac, Gy_dac, Gz_dac;
    double scale1,scale2,scale3,scale_x,scale_y,scale_z;
    double m11,m12,m13,m21,m22,m23,m31,m32,m33;
    char pat_x[MAXSTR],pat_y[MAXSTR],pat_z[MAXSTR],tmpstr[MAXSTR];
    struct obl_list  *obl_shape_ptr;
    int loops,xpe,ype,zpe;

	loops = 1;

	Gx_dac = gauss_dac('x');
        Gy_dac = gauss_dac('y');
        Gz_dac = gauss_dac('z');

        /*----------------------------------------------------------*/
        /* Obtain the transformation matrix to determine which axis	*/
        /* has the phase encode steps.  Set the pe variables.	*/
        /*----------------------------------------------------------*/
        xpe = 0; ype = 0; zpe = 0;
        obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);

        /*Transform logical gradient levels to magnet frame****/
        xlevel = (lvl1*m11 + lvl2*m12 + lvl3*m13);
        ylevel = (lvl1*m21 + lvl2*m22 + lvl3*m23);
        zlevel = (lvl1*m31 + lvl2*m32 + lvl3*m33);
 
        if (dps_flag)
        {
            dps_fval[0] = Gx_dac*xlevel;
            dps_fval[1] = Gy_dac*ylevel;
            dps_fval[2] = Gz_dac*zlevel;
            return;
        }
 
        grad_limit_checks(xlevel,ylevel,zlevel,"oblique_shaped3gradient");
 
    	xlevel = Gx_dac*xlevel;
    	ylevel = Gy_dac*ylevel;
    	zlevel = Gz_dac*zlevel;
 
    	if (fabs(xlevel)>gradstepsz ) {
       		text_error("pe_oblique_shaped3gradient: gradient x value exceeds DAC limit.");
       		psg_abort(1);
    	}
    	if (fabs(ylevel)>gradstepsz) {
       		text_error("pe_oblique_shaped3gradient: gradient y value exceeds DAC limit.");
       		psg_abort(1);
    	}
    	if (fabs(zlevel)>gradstepsz) {
       		text_error("pe_oblique_shaped3gradient: gradient z value exceeds DAC limit.");
       		psg_abort(1);
    	}
 
    	if (bgflag)
       		fprintf(stderr,"pe_oblique_shaped3gradient: x=%d y=%d z=%d dac units\n",
          		(int)(xlevel),(int)(ylevel),(int)(zlevel));
 


/************************** begin for coordinate rotator board   *************/

    if (anygradcrwg)
    {
    	lvl1 = Gx_dac*lvl1;
    	lvl2 = Gy_dac*lvl2;
    	lvl3 = Gz_dac*lvl3;

        if ((int)strlen(pat1) < 1)                             /* NULL pattern name */
        { 
            if ((int)strlen(pat2) < 1)  			/* NULL pattern name */
	    {
                 if ((int)strlen(pat3) < 1)  			/* NULL pattern name */
		 {
                 	text_error("All 3 pattern names are null in pulse sequence \n");
			psg_abort(1);
		 }
	         else 
		 {
			pat1 = pat3;
		 	lvl1 = 0.0;
		 }
	    }   
	    else 
	    {
		pat1 = pat2;
	    	lvl1 = 0.0;
	    }
	}

        if ((int)strlen(pat2) < 1)                             /* NULL pattern name */
        { 
            if ((int)strlen(pat3) < 1)  			/* NULL pattern name */
	    {
                 if ((int)strlen(pat1) < 1)  			/* NULL pattern name */
		 {
                 	text_error("All 3 pattern names are null in pulse sequence \n");
			psg_abort(1);
		 }
	         else 
		 {
			pat2 = pat1;
		 	lvl2 = 0.0;
		 }
	    }   
	    else 
	    {
		pat2 = pat3;
	    	lvl2 = 0.0;
	    }
	}

        if ((int)strlen(pat3) < 1)                             /* NULL pattern name */
        { 
            if ((int)strlen(pat1) < 1)  			/* NULL pattern name */
	    {
                 if ((int)strlen(pat2) < 1)  			/* NULL pattern name */
		 {
                 	text_error("All 3 pattern names are null in pulse sequence \n");
			psg_abort(1);
		 }
	         else 
		 {
			pat3 = pat2;
		 	lvl3 = 0.0;
		 }
	    }   
	    else 
	    {
		pat3 = pat1;
	    	lvl3 = 0.0;
	    }
	}

        tstlvl2 = 1.0*lvl2;
        pedacstep = Gy_dac*step2;

        if (((tstlvl2 < 0.0) && (lvl2 > 0.0)) ||
            ((tstlvl2 > 0.0) && (lvl2 < 0.0)))
                        pedacstep = pedacstep*(-1.0);

        prepWFGforPE(pat2,width,lvl2,pedacstep,vmult2,'y',one,NOWAIT,tag);
        shapedgradient(pat1, width, lvl1, 'x', loops, NOWAIT);
        doshapedPEgradient(pat2,width,lvl2,pedacstep,vmult2,'y',one,NOWAIT,tag);
        shapedgradient(pat3, width, lvl3, 'z', loops, wait);
        return;

    }

/************************** end for coordinate rotator board   *************/

    else
    {   
        if (m12 != 0.0) 
        {
	    xpe = 1;
	    pestep = m12*step2;
	    pedacstep = Gx_dac*pestep;
	    tstlvl2 = m12*lvl2; /* used for sign check of phase encode steps */
        }
        if (m22 != 0.0)
        {
	    ype = 1;
	    pestep = m22*step2;
	    pedacstep = Gy_dac*pestep;
	    tstlvl2 = m22*lvl2; /* used for sign check of phase encode steps */
        }
        if (m32 != 0.0)
        {
	    zpe = 1;
	    pestep = m32*step2;
	    pedacstep = Gz_dac*pestep;
	    tstlvl2 = m32*lvl2; /* used for sign check of phase encode steps */
        }

        if ((xpe+ype+zpe) > 1){
            text_error("pe_oblique_shaped3gradient: Axes not aligned along x,y,z.");
            psg_abort(1);
        }

        if (dps_flag)
        {
	    if (xpe)
	       dps_fval[3] = 1.0;
	    else if (ype)
	       dps_fval[3] = 2.0;
        }

        /*----------------------------------------------------------*/
        /* Check to see if oblique shape has already been defined.	*/
        /* If it has, call that shape.  Otherwise the routine will	*/
        /* will continue and create and define the requested shape.	*/
        /*----------------------------------------------------------*/
        obl_shape_ptr = obl_list_search(pat1,pat2,pat3,lvl1,lvl2,lvl3,
						ang1,ang2,ang3);
        if (obl_shape_ptr != NULL)
        {
    	    /*Set gradients in magnet frame*******************/
	    if (dps_flag)
            {
               dps_fval[0] = obl_shape_ptr->xlevel;
               dps_fval[1] = obl_shape_ptr->ylevel;
               dps_fval[2] = obl_shape_ptr->zlevel;
	       if (dps_flag > 8)
	          return;
            }

	    if (xpe)
	    {
	       if ( fabs(obl_shape_ptr->xlevel + (pedacstep*lim2)) > gradstepsz ) {
               text_error("pe_oblique_shaped3gradient: x value exceeds DAC limit.");
            	psg_abort(1);
	       }
	       /* sign check of phase encode start if the sign is different	*/
	       /* this means the pattern was inverted and the sign of the 	*/
	       /* step has to be changed.					*/
	       if (((tstlvl2 < 0.0) && (obl_shape_ptr->xlevel > 0.0)) || 
	        ((tstlvl2 > 0.0) && (obl_shape_ptr->xlevel < 0.0))) 
					pedacstep = pedacstep*(-1.0);
	       if (dps_flag)
	       {
                   dps_fval[4] = pedacstep;
	           return;
	       }
    	       shapedvgradient(obl_shape_ptr->pat_x,width,obl_shape_ptr->xlevel,
				pedacstep,vmult2,'x',one,NOWAIT,tag);
    	       shapedgradient(obl_shape_ptr->pat_y,width,obl_shape_ptr->ylevel,
						'y',loops, NOWAIT);
    	       shapedgradient(obl_shape_ptr->pat_z,width,obl_shape_ptr->zlevel,
						'z',loops,wait);
	    }
	    else if (ype)
	    {
	       if ( fabs(obl_shape_ptr->ylevel + (pedacstep*lim2)) > gradstepsz ) {
               text_error("pe_oblique_shaped3gradient: y value exceeds DAC limit.");
        	psg_abort(1);
	       }
	       if (((tstlvl2 < 0.0) && (obl_shape_ptr->ylevel > 0.0)) || 
	        ((tstlvl2 > 0.0) && (obl_shape_ptr->ylevel < 0.0))) 
					pedacstep = pedacstep*(-1.0);
	       if (dps_flag)
	       {
                   dps_fval[4] = pedacstep;
	           return;
	       }
    	       shapedvgradient(obl_shape_ptr->pat_y,width,obl_shape_ptr->ylevel,
				pedacstep,vmult2,'y',one,NOWAIT,tag);
    	       shapedgradient(obl_shape_ptr->pat_x,width,obl_shape_ptr->xlevel,
						'x',loops, NOWAIT);
    	       shapedgradient(obl_shape_ptr->pat_z,width,obl_shape_ptr->zlevel,
						'z',loops,wait);
	    }
	    else
	    {
	       if ( fabs(obl_shape_ptr->zlevel + (pedacstep*lim2)) > gradstepsz ) {
               text_error("pe_oblique_shaped3gradient: z value exceeds DAC limit.");
            	psg_abort(1);
	       }
	       if (((tstlvl2 < 0.0) && (obl_shape_ptr->zlevel > 0.0)) || 
	        ((tstlvl2 > 0.0) && (obl_shape_ptr->zlevel < 0.0))) 
					pedacstep = pedacstep*(-1.0);
	       if (dps_flag)
	       {
                   dps_fval[4] = pedacstep;
	           return;
	       }
    	       shapedvgradient(obl_shape_ptr->pat_z,width,obl_shape_ptr->zlevel,
				pedacstep,vmult2,'z',one,NOWAIT,tag);
    	       shapedgradient(obl_shape_ptr->pat_x,width,obl_shape_ptr->xlevel,
						'x',loops, NOWAIT);
    	       shapedgradient(obl_shape_ptr->pat_y,width,obl_shape_ptr->ylevel,
						'y',loops, wait);
	    }
	    return;
        }
        if (dps_flag > 8)
        {
	    xlevel = (lvl1*m11 + lvl2*m12 + lvl3*m13);
	    ylevel = (lvl1*m21 + lvl2*m22 + lvl3*m23);
	    zlevel = (lvl1*m31 + lvl2*m32 + lvl3*m33);
            dps_fval[0] = Gx_dac*xlevel;
            dps_fval[1] = Gy_dac*ylevel;
            dps_fval[2] = Gz_dac*zlevel;
            return;
        }

        /*----------------------------------------------------------*/
        /* If the shape has not been defined then the routine will  */
        /* will continue and create and define the requested shape. */
        /*----------------------------------------------------------*/

        create_rpat_names(pat_x,pat_y,pat_z,
			pat1,pat2,pat3,lvl1,lvl2,lvl3,ang1,ang2,ang3);

        scale1 = 1.0;
        scale2 = 1.0;
        scale3 = 1.0;
        scale_x = 0.0;
        scale_y = 0.0;
        scale_z = 0.0;

        /* since scale1,scale2,scale3 are 1.0 no scaling will be done 	*/
        /* in rotate_shapes and scale_x, scale_y, scale_z will be zero	*/
        rotate_shapes(pat1,pat2,pat3,scale1,scale2,scale3,ang1,ang2,ang3,
		  	pat_x,pat_y,pat_z,&scale_x,&scale_y,&scale_z);

       /* if (scale_x != 0.0) xlevel = gmax/scale_x;
        * else xlevel = 0.0;
        * if (scale_y != 0.0) ylevel = gmax/scale_y;
        * else ylevel = 0.0;
        * if (scale_z != 0.0) zlevel = gmax/scale_z;
        * else zlevel = 0.0;
        */

        /* Transform starting logical gradient levels to magnet frame****/
        xlevel = (lvl1*m11 + lvl2*m12 + lvl3*m13);
        ylevel = (lvl1*m21 + lvl2*m22 + lvl3*m23);
        zlevel = (lvl1*m31 + lvl2*m32 + lvl3*m33);

        if (xpe) {
	    grad_limit_checks(xlevel+(pestep*lim2),ylevel,zlevel,
					"pe_oblique_shaped3gradient");
        }
        else if (ype) {
	    grad_limit_checks(xlevel,ylevel+(pestep*lim2),zlevel,
					"pe_oblique_shaped3gradient");
        }
        else {
	    grad_limit_checks(xlevel,ylevel,zlevel+(pestep*lim2),
					"pe_oblique_shaped3gradient");
        }

        xlevel = Gx_dac*xlevel;
        ylevel = Gy_dac*ylevel;
        zlevel = Gz_dac*zlevel;

        if (dps_flag)
        {
            dps_fval[0] = xlevel;
            dps_fval[1] = ylevel;
            dps_fval[2] = zlevel;
        }

        if (fabs(xlevel)>gradstepsz ) {
           text_error("oblique_shaped3gradient: gradient x value exceeds DAC limit.");
           if (!bgflag)
	       remove_rotated_patterns(pat_x,pat_y,pat_z);
           psg_abort(1);
        }
        if (fabs(ylevel)>gradstepsz) {
           text_error("oblique_shaped3gradient: gradient y value exceeds DAC limit.");
           if (!bgflag)
	       remove_rotated_patterns(pat_x,pat_y,pat_z);
           psg_abort(1);
        }
        if (fabs(zlevel)>gradstepsz) {
           text_error("oblique_shaped3gradient: gradient z value exceeds DAC limit.");
           if (!bgflag)
	       remove_rotated_patterns(pat_x,pat_y,pat_z);
           psg_abort(1);
        }

        if (bgflag)
           fprintf(stderr,
	      "pe_oblique_shaped3gradient: x=%d y=%d z=%d pe=%d dac units\n",
	      (int)(xlevel),(int)(ylevel),(int)(zlevel),(int)(pedacstep));

        /*Set gradients in magnet frame*******************/
        if (xpe)
        {
        	if ( fabs(xlevel + (pedacstep*lim2)) > gradstepsz ) {
               text_error("pe_oblique_shaped3gradient: x value exceeds DAC limit.");
            	psg_abort(1);
		}
		/* sign check of phase encode start if the sign is different	*/
		/* this means the pattern was inverted and the sign of the 	*/
		/* step has to be changed.					*/
		if (((tstlvl2 < 0.0) && (xlevel > 0.0)) || 
	    	((tstlvl2 > 0.0) && (xlevel < 0.0))) pedacstep = pedacstep*(-1.0);
		if (dps_flag)
		{
                  dps_fval[4] = pedacstep;
	          return;
		}
    		shapedvgradient(pat_x,width,xlevel,pedacstep,vmult2,'x',one,NOWAIT,tag);
    		shapedgradient(pat_y,width,ylevel,'y',loops, NOWAIT);
    		shapedgradient(pat_z,width,zlevel,'z',loops,wait);
    	}
    	else if (ype)
    	{
		if ( fabs(ylevel + (pedacstep*lim2)) > gradstepsz ) {
           	text_error("pe_oblique_shaped3gradient: y value exceeds DAC limit.");
           	psg_abort(1);
		}
		/* sign check of phase encode start if the sign is different	*/
		/* this means the pattern was inverted and the sign of the 	*/
		/* step has to be changed.					*/
		if (((tstlvl2 < 0.0) && (ylevel > 0.0)) || 
		    ((tstlvl2 > 0.0) && (ylevel < 0.0))) pedacstep = pedacstep*(-1.0);
		if (dps_flag)
		{
       		   dps_fval[4] = pedacstep;
		   return;
		}
    		shapedvgradient(pat_y,width,ylevel,pedacstep,vmult2,'y',one,NOWAIT,tag);
    		shapedgradient(pat_x,width,xlevel,'x',loops, NOWAIT);
    		shapedgradient(pat_z,width,zlevel,'z',loops,wait);
    	}
    	else
    	{
		if ( fabs(zlevel + (pedacstep*lim2)) > gradstepsz ) {
       		    text_error("pe_oblique_shaped3gradient: z value exceeds DAC limit.");
       		    psg_abort(1);
		}
		/* sign check of phase encode start if the sign is different	*/
		/* this means the pattern was inverted and the sign of the 	*/
		/* step has to be changed.					*/
		if (((tstlvl2 < 0.0) && (zlevel > 0.0)) || 
		    ((tstlvl2 > 0.0) && (zlevel < 0.0))) pedacstep = pedacstep*(-1.0);
		if (dps_flag)
		{
       	          dps_fval[4] = pedacstep;
	          return;
		}
    		shapedvgradient(pat_z,width,zlevel,pedacstep,vmult2,'z',one,NOWAIT,tag);
    		shapedgradient(pat_x,width,xlevel,'x',loops, NOWAIT);
    		shapedgradient(pat_y,width,ylevel,'y',loops, wait);
    	}

    	add_obl_shape(pat1,pat2,pat3,lvl1,lvl2,lvl3,ang1,ang2,ang3,
		  	pat_x,pat_y,pat_z,xlevel,ylevel,zlevel);
    }
}



        /*********************************************************
                            pe3_oblique_shaped3gradient()

             Procedure to set static oblique gradient levels plus
             three oblique phase encode shapedgradients.
        **********************************************************/

S_pe3_oblique_shaped3gradient(pat1,pat2,pat3, width, stat1, stat2, stat3, 
			step1, step2, step3, vmult1, vmult2, vmult3,
			lim1, lim2, lim3, ang1, ang2, ang3, loops, wait)

char pat1[],pat2[],pat3[];
double  width;
double  lim1,lim2,lim3,stat1,stat2,stat3;
double  step1,step2,step3,ang1,ang2,ang3;
codeint vmult1,vmult2,vmult3;
int     loops,wait;

{
    double  Gx_dac, Gy_dac, Gz_dac;
    double  m11,m12,m13,m21,m22,m23,m31,m32,m33;
    double  step1x,step1y,step1z,step2x,step2y,step2z,step3x,step3y,step3z;
    double  statX,statY,statZ;

    Gx_dac = gauss_dac('x');
    Gy_dac = gauss_dac('y');
    Gz_dac = gauss_dac('z');

    /*Obtain the transformation matrix*****************/
    obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);

    /*Transform logical static gradient levels to magnet frame****/
    statX = (stat1*m11 + stat2*m12 + stat3*m13);
    statY = (stat1*m21 + stat2*m22 + stat3*m23);
    statZ = (stat1*m31 + stat2*m32 + stat3*m33);

    grad_limit_checks(statX+(m11*step1*lim1)+(m12*step2*lim2)+(m13*step3*lim3),
		statY+(m21*step1*lim1)+(m22*step2*lim2)+(m23*step3*lim3),
		statZ+(m31*step1*lim1)+(m32*step2*lim2)+(m33*step3*lim3),
					"pe3_oblique_shaped3gradient");

    /* Convert to DAC units *****************/
    statX = Gx_dac*statX;
    statY = Gy_dac*statY;
    statZ = Gz_dac*statZ;

    step1x = step1*Gx_dac; step1y = step1*Gy_dac; step1z = step1*Gz_dac;
    step2x = step2*Gx_dac; step2y = step2*Gy_dac; step2z = step2*Gz_dac;
    step3x = step3*Gx_dac; step3y = step3*Gy_dac; step3z = step3*Gz_dac;

    /*Check gradient levels against max dac value ****************/
    if ( ( fabs(statX + (m11*step1x*lim1) + (m12*step2x*lim2) +
                        (m13*step3x*lim3)) > gradstepsz ) ) {

       text_error("pe3_oblique_shaped3gradient: gradient x exceeds DAC limit.");
       psg_abort(1);
    }
    if ( ( fabs(statY + (m21*step1y*lim1) + (m22*step2y*lim2) +
                        (m23*step3y*lim3)) > gradstepsz ) ) {

       text_error("pe3_oblique_shaped3gradient: gradient y exceeds DAC limit.");
       psg_abort(1);
    }
    if ( ( fabs(statZ + (m31*step1z*lim1) + (m32*step2z*lim2) +
                        (m33*step3z*lim3)) > gradstepsz ) ) {

       text_error("pe3_oblique_shaped3gradient: gradient z exceeds DAC limit.");
       psg_abort(1);
    }

    if (bgflag)
    {
      fprintf(stderr,"pe3_oblique_shaped3gradient: start x=%d y=%d z=%d dac units\n",
	(int)(statX),(int)(statY),(int)(statZ));
      fprintf(stderr,"pe3_oblique_shaped3gradient: step x=%d y=%d z=%d dac units\n",
	(int)(step1x*m11+step2x*m12+step3x*m13),
	(int)(step1y*m21+step2y*m22+step3y*m23),
	(int)(step1z*m31+step2z*m32+step3z*m33));
    }

/************************** begin for coordinate rotator board   *************/

    if (anygradcrwg)
    {
        statX = Gx_dac*stat1;
        statY = Gy_dac*stat2;
        statZ = Gz_dac*stat3;
 
        if ((int)strlen(pat1) < 1)                             /* NULL pattern name */
        { 
            if ((int)strlen(pat2) < 1)  			/* NULL pattern name */
	    {
                 if ((int)strlen(pat3) < 1)  			/* NULL pattern name */
		 {
                 	text_error("All 3 pattern names are null in pulse sequence \n");
			psg_abort(1);
		 }
	         else 
		 {
			pat1 = pat3;
		 	statX = 0.0;
		 }
	    }   
	    else 
	    {
		pat1 = pat2;
	    	statX = 0.0;
	    }
	}

        if ((int)strlen(pat2) < 1)                             /* NULL pattern name */
        { 
            if ((int)strlen(pat3) < 1)  			/* NULL pattern name */
	    {
                 if ((int)strlen(pat1) < 1)  			/* NULL pattern name */
		 {
                 	text_error("All 3 pattern names are null in pulse sequence \n");
			psg_abort(1);
		 }
	         else 
		 {
			pat2 = pat1;
		 	statY = 0.0;
		 }
	    }   
	    else 
	    {
		pat2 = pat3;
	    	statY = 0.0;
	    }
	}

        if ((int)strlen(pat3) < 1)                             /* NULL pattern name */
        { 
            if ((int)strlen(pat1) < 1)  			/* NULL pattern name */
	    {
                 if ((int)strlen(pat2) < 1)  			/* NULL pattern name */
		 {
                 	text_error("All 3 pattern names are null in pulse sequence \n");
			psg_abort(1);
		 }
	         else 
		 {
			pat3 = pat2;
		 	statZ = 0.0;
		 }
	    }   
	    else 
	    {
		pat3 = pat1;
	    	statZ = 0.0;
	    }
	}

    prepforshapedINCgradient('x',pat1,width,statX,step1x,0.0,0.0,vmult1,vmult2,
		vmult3,loops, NOWAIT);
    prepforshapedINCgradient('y', pat2, width,statY,0.0,step2y,0.0,vmult1,vmult2,
		vmult3,loops, NOWAIT);
    prepforshapedINCgradient('z', pat3, width,statZ,0.0,0.0,step3z,vmult1,vmult2,
		vmult3,loops, NOWAIT);
    doshapedINCgradient('x',pat1,width,statX,step1x,0.0,0.0,vmult1,vmult2,
		vmult3,loops, NOWAIT);
    doshapedINCgradient('y', pat2, width,statY,0.0,step2y,0.0,vmult1,vmult2,
		vmult3,loops, NOWAIT);
    doshapedINCgradient('z', pat3, width,statZ,0.0,0.0,step3z,vmult1,vmult2,
		vmult3,loops, wait);
        
    }
 
/************************** end for coordinate rotator board   *************/

    else
    {
       text_error("pe3_oblique_shaped3gradient: required Gradient Coordinate Rotator Board");
       psg_abort(1);
    }

}


        /*********************************************************
                            phase_encode3_gradient()

             Procedure to set static oblique gradient levels plus
             three oblique phase encode gradients.
        **********************************************************/

S_phase_encode3_gradient(stat1, stat2, stat3, step1, step2, step3,
		       vmult1, vmult2, vmult3, lim1, lim2, lim3,
		       ang1, ang2, ang3)

double  lim1,lim2,lim3,stat1,stat2,stat3;
double  step1,step2,step3,ang1,ang2,ang3;
codeint vmult1,vmult2,vmult3;
{
    double  Gx_dac, Gy_dac, Gz_dac;
    double  m11,m12,m13,m21,m22,m23,m31,m32,m33,m[9];
    double  step1x,step1y,step1z,step2x,step2y,step2z,step3x,step3y,step3z;
    double  statX,statY,statZ;

    Gx_dac = gauss_dac('x');
    Gy_dac = gauss_dac('y');
    Gz_dac = gauss_dac('z');

    /*Obtain the transformation matrix*****************/
    obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);

    /*Transform logical static gradient levels to magnet frame****/
    statX = (stat1*m11 + stat2*m12 + stat3*m13);
    statY = (stat1*m21 + stat2*m22 + stat3*m23);
    statZ = (stat1*m31 + stat2*m32 + stat3*m33);

    grad_limit_checks(statX+(m11*step1*lim1)+(m12*step2*lim2)+(m13*step3*lim3),
		statY+(m21*step1*lim1)+(m22*step2*lim2)+(m23*step3*lim3),
		statZ+(m31*step1*lim1)+(m32*step2*lim2)+(m33*step3*lim3),
						"phase_encode3_gradient");

    /* Convert to DAC units *****************/
    statX = Gx_dac*statX;
    statY = Gy_dac*statY;
    statZ = Gz_dac*statZ;

    step1x = step1*Gx_dac; step1y = step1*Gy_dac; step1z = step1*Gz_dac;
    step2x = step2*Gx_dac; step2y = step2*Gy_dac; step2z = step2*Gz_dac;
    step3x = step3*Gx_dac; step3y = step3*Gy_dac; step3z = step3*Gz_dac;

    /*Check gradient levels against max dac value ****************/
    if ( ( fabs(statX + (m11*step1x*lim1) + (m12*step2x*lim2) +
                        (m13*step3x*lim3)) > gradstepsz ) ) {

       text_error("phase_encode3_gradient: gradient x value exceeds DAC limit.");
       psg_abort(1);
    }
    if ( ( fabs(statY + (m21*step1y*lim1) + (m22*step2y*lim2) +
                        (m23*step3y*lim3)) > gradstepsz ) ) {

       text_error("phase_encode3_gradient: gradient y value exceeds DAC limit.");
       psg_abort(1);
    }
    if ( ( fabs(statZ + (m31*step1z*lim1) + (m32*step2z*lim2) +
                        (m33*step3z*lim3)) > gradstepsz ) ) {

       text_error("phase_encode3_gradient: gradient z value exceeds DAC limit.");
       psg_abort(1);
    }

    if (bgflag)
    {
      fprintf(stderr,"phase_encode3_gradient: start x=%d y=%d z=%d dac units\n",
	(int)(statX),(int)(statY),(int)(statZ));
      fprintf(stderr,"phase_encode3_gradient: step x=%d y=%d z=%d dac units\n",
	(int)(step1x*m11+step2x*m12+step3x*m13),
	(int)(step1y*m21+step2y*m22+step3y*m23),
	(int)(step1z*m31+step2z*m32+step3z*m33));
    }

/************************** begin for coordinate rotator board   *************/

    if (anygradcrwg)
    {
        statX = Gx_dac*stat1;
        statY = Gy_dac*stat2;
        statZ = Gz_dac*stat3;

       incgradient('x',statX,step1x,0.0,0.0,vmult1,vmult2,vmult3);    
       incgradient('y',statY,0.0,step2y,0.0,vmult1,vmult2,vmult3);    
       incgradient('z',statZ,0.0,0.0,step3z,vmult1,vmult2,vmult3);

    }

/************************** end for coordinate rotator board   *************/

    else
    {

    /*Set gradients in magnet frame*******************/
    incgradient('x',statX,step1x*m11,step2x*m12,step3x*m13,vmult1,vmult2,vmult3);    
    incgradient('y',statY,step1y*m21,step2y*m22,step3y*m23,vmult1,vmult2,vmult3);    
    incgradient('z',statZ,step1z*m31,step2z*m32,step3z*m33,vmult1,vmult2,vmult3);
    }
}


        /*********************************************************
                            phase_encode3_shapedgradient()

             Procedure to set static oblique gradient levels plus
             three oblique phase encode shapedgradients.
        **********************************************************/

S_phase_encode3_shapedgradient(pat, width, stat1, stat2, stat3, 
			step1, step2, step3, vmult1, vmult2, vmult3,
			lim1, lim2, lim3, ang1, ang2, ang3, loops, wait)

char    *pat;
double  width;
double  lim1,lim2,lim3,stat1,stat2,stat3;
double  step1,step2,step3,ang1,ang2,ang3;
codeint vmult1,vmult2,vmult3;
int     loops,wait;

{
    double  Gx_dac, Gy_dac, Gz_dac;
    double  m11,m12,m13,m21,m22,m23,m31,m32,m33;
    double  step1x,step1y,step1z,step2x,step2y,step2z,step3x,step3y,step3z;
    double  statX,statY,statZ;

    Gx_dac = gauss_dac('x');
    Gy_dac = gauss_dac('y');
    Gz_dac = gauss_dac('z');

    /*Obtain the transformation matrix*****************/
    obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);

    /*Transform logical static gradient levels to magnet frame****/
    statX = (stat1*m11 + stat2*m12 + stat3*m13);
    statY = (stat1*m21 + stat2*m22 + stat3*m23);
    statZ = (stat1*m31 + stat2*m32 + stat3*m33);

    grad_limit_checks(statX+(m11*step1*lim1)+(m12*step2*lim2)+(m13*step3*lim3),
		statY+(m21*step1*lim1)+(m22*step2*lim2)+(m23*step3*lim3),
		statZ+(m31*step1*lim1)+(m32*step2*lim2)+(m33*step3*lim3),
					"phase_encode3_shapedgradient");

    /* Convert to DAC units *****************/
    statX = Gx_dac*statX;
    statY = Gy_dac*statY;
    statZ = Gz_dac*statZ;

    step1x = step1*Gx_dac; step1y = step1*Gy_dac; step1z = step1*Gz_dac;
    step2x = step2*Gx_dac; step2y = step2*Gy_dac; step2z = step2*Gz_dac;
    step3x = step3*Gx_dac; step3y = step3*Gy_dac; step3z = step3*Gz_dac;

    /*Check gradient levels against max dac value ****************/
    if ( ( fabs(statX + (m11*step1x*lim1) + (m12*step2x*lim2) +
                        (m13*step3x*lim3)) > gradstepsz ) ) {

       text_error("phase_encode3_shapedgradient: gradient x exceeds DAC limit.");
       psg_abort(1);
    }
    if ( ( fabs(statY + (m21*step1y*lim1) + (m22*step2y*lim2) +
                        (m23*step3y*lim3)) > gradstepsz ) ) {

       text_error("phase_encode3_shapedgradient: gradient y exceeds DAC limit.");
       psg_abort(1);
    }
    if ( ( fabs(statZ + (m31*step1z*lim1) + (m32*step2z*lim2) +
                        (m33*step3z*lim3)) > gradstepsz ) ) {

       text_error("phase_encode3_shapedgradient: gradient z exceeds DAC limit.");
       psg_abort(1);
    }

    if (bgflag)
    {
      fprintf(stderr,"phase_encode3_gradient: start x=%d y=%d z=%d dac units\n",
	(int)(statX),(int)(statY),(int)(statZ));
      fprintf(stderr,"phase_encode3_gradient: step x=%d y=%d z=%d dac units\n",
	(int)(step1x*m11+step2x*m12+step3x*m13),
	(int)(step1y*m21+step2y*m22+step3y*m23),
	(int)(step1z*m31+step2z*m32+step3z*m33));
    }

/************************** begin for coordinate rotator board   *************/

    if (anygradcrwg)
    {
        statX = Gx_dac*stat1;
        statY = Gy_dac*stat2;
        statZ = Gz_dac*stat3;
 
    prepforshapedINCgradient('x',pat,width,statX,step1x,0.0,0.0,vmult1,vmult2,
		vmult3,loops, NOWAIT);
    prepforshapedINCgradient('y', pat, width,statY,0.0,step2y,0.0,vmult1,vmult2,
		vmult3,loops, NOWAIT);
    prepforshapedINCgradient('z', pat, width,statZ,0.0,0.0,step3z,vmult1,vmult2,
		vmult3,loops, NOWAIT);
    doshapedINCgradient('x',pat,width,statX,step1x,0.0,0.0,vmult1,vmult2,
		vmult3,loops, NOWAIT);
    doshapedINCgradient('y', pat, width,statY,0.0,step2y,0.0,vmult1,vmult2,
		vmult3,loops, NOWAIT);
    doshapedINCgradient('z', pat, width,statZ,0.0,0.0,step3z,vmult1,vmult2,
		vmult3,loops, wait);
        
    }
 
/************************** end for coordinate rotator board   *************/
 
    else
    {

    /*Set gradients in magnet frame*******************/
    shapedincgradient('x', pat, width,
		statX,step1x*m11,step2x*m12,step3x*m13,vmult1,vmult2,vmult3,
		loops, NOWAIT);
    shapedincgradient('y', pat, width,
		statY,step1y*m21,step2y*m22,step3y*m23,vmult1,vmult2,vmult3,
		loops, NOWAIT);
    shapedincgradient('z', pat, width,
		statZ,step1z*m31,step2z*m32,step3z*m33,vmult1,vmult2,vmult3,
		loops, wait);
    }
}



/****************************************************************************
                            oblique_check()

                Oblique orientations are only permitted using 
                16 bit DACs. Acceptable cases for Euler angle 
                settings are defined as having a gradient 
                transformation matrix with two zero values 
                per row (ie the gradient associated with the
                corresponding row is directed along one axis
                of the magnet frame.)
        
                If the user requests an oblique orientation 
                for the logical frame defined by ang1-ang3 
                AND has only 12 bit DACs then we abort. 

                30th September 1991     
*****************************************************************************/

oblique_check(ang1,ang2,ang3)

double ang1,ang2,ang3;

{ /*BEGIN: oblique_check************************************************/

        double m11,m12,m13;             /*transformation matrix*/
        double m21,m22,m23;             /*transformation matrix*/
        double m31,m32,m33;             /*transformation matrix*/
        double tol;                     /*tolerence for matrix elements*/
           int ac1,ac2,ac3;             /*Active components on each row*/


        /*Set tolerence**************************************
                The tolerence used with the Euler angle
                decoder should match the one used here.
        *****************************************************/
        tol=0.97 * M_PI / 180;	/* ~= sin(0.97 degrees) */

        /*Get transformation matrix**************************/
        obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);

        /*Obtain ac1-ac3*************************************/
        ac1=0;
        if (fabs(m11) > tol) ac1=ac1+1;
        if (fabs(m12) > tol) ac1=ac1+1;
        if (fabs(m13) > tol) ac1=ac1+1;

        ac2=0;
        if (fabs(m21) > tol) ac2=ac2+1;
        if (fabs(m22) > tol) ac2=ac2+1;
        if (fabs(m23) > tol) ac2=ac2+1;

        ac3=0;
        if (fabs(m31) > tol) ac3=ac3+1;
        if (fabs(m32) > tol) ac3=ac3+1;
        if (fabs(m33) > tol) ac3=ac3+1;

        /*Detect Oblique Case*******************************/
        if ((ac1 > 1) || (ac2 > 1) || (ac3 > 1))
        {
                if (gradstepsz < 2049)
                {
                   text_error("oblique_check: Oblique slice or voxel\n");
                   text_error("Not supported with 12 bit DAC hardware\n");
                   psg_abort(1);
                }
        }
                
                
} /*END: oblique_check**************************************************/

/*--------------------------------------------------------------*/
/* grad_limit_checks						*/
/* 					 			*/
/*--------------------------------------------------------------*/
grad_limit_checks(xgrad,ygrad,zgrad,routinename)
double xgrad,ygrad,zgrad;
char *routinename;
{
 double precision_limit;
 char msge[256];

    if (bgflag)
       fprintf(stderr,"%s: xgrad: %7.3f  ygrad: %7.3f  zgrad: %7.3f\n",
				routinename,xgrad,ygrad,zgrad);
    precision_limit = 0.5/MAX_GRAD_AMP;
    xgrad = fabs(xgrad);
    ygrad = fabs(ygrad);
    zgrad = fabs(zgrad);
    if (xgrad > (gxlimit+(gxlimit*precision_limit)))
    {
	sprintf(msge,"%s: X gradient: %7.3f exceeds safety limit: %7.3f.\n",
						routinename,xgrad,gxlimit);
	text_error(msge);
	psg_abort(1);
    }
    if (ygrad > (gylimit+(gylimit*precision_limit)))
    {
	sprintf(msge,"%s: Y gradient: %7.3f exceeds safety limit: %7.3f.\n",
						routinename,ygrad,gylimit);
	text_error(msge);
	psg_abort(1);
    }
    if (zgrad > (gzlimit+(gzlimit*precision_limit)))
    {
	sprintf(msge,"%s: Z gradient: %7.3f exceeds safety limit: %7.3f.\n",
						routinename,zgrad,gzlimit);
	text_error(msge);
	psg_abort(1);
    }

}


/*--------------------------------------------------------------*/
/* S_oblique_gradpulse						*/
/* 					 			*/
/*--------------------------------------------------------------*/
void S_oblique_gradpulse(double level1, double level2, double level3,
                double ang1, double ang2, double ang3, double gdelay)
{
   char graddisableflag[2];

   if (gdelay > MINDELAY)
   {

     if ( P_getstring(GLOBAL,"gradientdisable",&graddisableflag,1,2) == 0 )
     {
       if ((graddisableflag[0] == 'y'))
       {
        oblique_gradient(0.0,0.0,0.0,ang1,ang2,ang3);
        if ((ix == 1) && (! warningDone)) warn_message("no gradients active for gradientdisable = 'y' \n");
        warningDone = 1;
       }
       else
       {
         oblique_gradient(level1,level2,level3,ang1,ang2,ang3);
       }
     }
     else
     {
        oblique_gradient(level1,level2,level3,ang1,ang2,ang3);
     }

     if (dps_flag)
           return;
     delay(gdelay);
     zero_all_gradients();
   }
}


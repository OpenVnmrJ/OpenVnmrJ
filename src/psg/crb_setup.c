/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  crb_setup.c derived from coordtest.c from GB/CP and based on ecc_setup.c

4/28/04:	Greg's version of this file is now required with 1.1D and new compiler

9/3/03:		Greg B. cleaned up file
		this may help with malloc problem conflict with poffset_list

12/12/02:	download 5 numbers to each DSP chip: 
 			1st rotation vector number
 			2nd rotation vector number
 			3rd rotation vector number
 			shim offset value
			gradient amplifier group delay
	pulse sequence element rotate_angle(ang1,ang2,ang3,offsetx,offsety,offsetz,gxdelay,gydelay,gzdelay);
	pulse sequence element init_crb(); also have to include zero amplifier group delay
			
04/04/01:	download 4 numbers to each DSP chip: 
 			1st rotation vector number
 			2nd rotation vector number
 			3rd rotation vector number
 			shim offset value

10/31/00:	download 6 numbers to each DSP chip: 
			number of numbers to be downloaded
			address to download to on DSP chip
			1st rotation vector number
			2nd rotation vector number
			3rd rotation vector number
			shim offset value

2/14/01:	pulse sequence element rotate_angle(ang1,ang2,ang3,offsetx,offsety,offsetz);

		pulse sequence element init_crb(); to send unitary matrix to coordinate rotator board

		dlmattiello
*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "vnmrsys.h"
#include "acodes.h"
#include "acqparms.h"
#include "group.h"
#include "macros.h"

/* These AP defines are also in oopc.h, but other stuff there messes us up */
#ifndef APSELECT
#define APSELECT 0xA000
#define APWRITE 0xB000
#endif

#ifndef TRUE
#define TRUE (0==0)
#define FALSE (!TRUE)
#endif

#define MAX_GRAD_AMP    32767.0

#define	APBOUT	    6
#define APWRT   (0x01000000)

#define HS_LINE_MASK    0x07FFFFFF
#define CW_DATA_FIELD_MASK 0x00FFFFFF
#define DATA_FIELD_SHFT_IN_HSW  5
#define DATA_FIELD_SHFT_IN_LSW  27

/* Stuff about the Coordinate Rotator hardware */
#define REG_0	0xC94
#define REG_Z	0xC95
#define REG_Y	0xC96
#define REG_X	0xC97

#define CROT 0xC94		/* Base AP address of CROT board */
#define CROTWRITE (APWRITE | (CROT & 0xf00))
#define DSPBUFLEN 5		/* Number of words to load into DSP - now 5  */
#define DSPADDR 0x87fff0        /* address of DSP to load values into */
#define NCHIPS 3		/* Number of DSP chips */

#define X1_AXIS 0
#define Y1_AXIS 1
#define Z1_AXIS 2

#define ECCSCALE 0

extern int bgflag;
extern char gradtype[];
extern double getval();
extern double zero_all_gradients();
extern float get_decctool_shimscale();



/* let's avoid the whole mallocing thing, no need since the
   sizes are always static   Greg B.  6/20/03
*/

 
/* generate the static structure for the DSP chip ap instructions */
static unsigned int chipbuf1[DSPBUFLEN + 1];
static unsigned int chipbuf2[DSPBUFLEN + 1];
static unsigned int chipbuf3[DSPBUFLEN + 1];
static unsigned int *chip_bufs[3] = { chipbuf1, chipbuf2, chipbuf3 };

static  codeint head[] = {APBOUT,
                      0,                             /* Reserve for count-1 */
                      (APSELECT | CROT+0),  /* Register 0 */
                      (CROTWRITE | 0x00),   /* Reset FIFOs */
                      (CROTWRITE | 0x40)};  /* Enable FIFOs */


static codeint tail[] = {(APSELECT | CROT+0),  /* Register 0 */
                      (CROTWRITE | 0x47),   /* Interrupt all 3 DSPs */
                      (CROTWRITE | 0x40)};  /* DSPs to normal mode */

#define BSIZE  (DSPBUFLEN * sizeof(int))  /* get_dsp_chip_download_bufsize(); */
#define WSIZE  (BSIZE/sizeof(unsigned int)) /* wsize = bsize / sizeof(unsigned int); */

#define HEADLEN (sizeof(head) / sizeof(codeint))
#define TAILLEN  (sizeof(tail) / sizeof(codeint))
#define OUTLEN  (HEADLEN + NCHIPS + NCHIPS * WSIZE * 4 + TAILLEN)

/* Static array of DSP apbus commands without the malloc overhead */
static codeint apbcmds[OUTLEN + 1]; 
static void obl_matrix(double ang1, double ang2, double ang3, double *matrix);


static void
print_rotation_matrix(double m[9])
{
    printf("%9f   %9f   %9f\n", m[0], m[1], m[2]);
    printf("%9f   %9f   %9f\n", m[3], m[4], m[5]);
    printf("%9f   %9f   %9f\n", m[6], m[7], m[8]);
}

static void
print_dsp_buf(unsigned int *buf, int len, char *axis)
{
    printf("\nTMS chip download words for %s-axis:\n", axis);
    while (len--) {
	printf("0x%08x   \n", *buf);   
	buf++;
    }
}

static void
print_apbout_string(codeint *buf)
{
    int len;

    len = buf[1] + 3;
    printf("\nAPBOUT string:\n");
    while (len--) {
        printf("0x%08x\n", *buf++ & 0xffff);
    }
}

static float
tms2float(unsigned int x)
{
    float e;
    int s;
    float f;

    e = (int)x >> 24;
    s = (x >> 23) & 1;
    f = (float)(x & 0x7fffff) / 0x800000;
    if (e == -128) {
	return 0;
    } else if (s) {
	return -(2 - f) * pow(2.0, e);
    } else {
	return (1 + f) * pow(2.0, e);
    }
}

/*
 * Convert a floating point number in native format to TMS32032
 * floating point single precision (32 bit) format.  Note that the
 * TMS floating point value is returned as an unsigned integer.
 */
static unsigned int
float2tms32(float x)
{
    unsigned int zero = 0x80000000; /* Zero value is special case */
    int nfracbits = 23;		/* Not including hidden bit */
    int signbit = 1 << nfracbits;
    int fracmask = ~((~0)<<nfracbits);
    int iexp;
    int sign;
    int ifrac;
    unsigned int rtn;

    if (x == 0) {
	rtn = zero;
    } else {
	iexp = ilogb(x);	/* Binary exponent if 1 <= |fraction| < 2 */
	ifrac = (int)scalbn(x, nfracbits-iexp); /* Frac part as integer */
	if (x<0 && (ifrac & signbit)) {
	    /* Force top bit of negative fraction to be 0 */
	    ifrac <<= 1;
	    iexp--;
	}
	sign = x<0 ? signbit : 0;
	rtn = (iexp << (nfracbits+1)) | sign | (ifrac & fracmask);
    }
    return rtn;
}

static double gauss_dac(gid)
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
	default: printf("Illegal gradient configuration."); psg_abort(1);
    }

    G_dac = (double)gradstepsz/gradmax;
    return(G_dac);
}
                
rotateHW(ang1,ang2,ang3,matrix)
double ang1,ang2,ang3;
double matrix[9];
{
    /*Obtain the transformation matrix*****************/
    obl_matrix(ang1,ang2,ang3,matrix);

}

/*******************************************************
                     obl_matrix()
        Procedure to provide the elements of the
        logical to magnet gradient transform matrix
********************************************************/

static void obl_matrix(double ang1, double ang2, double ang3, double *matrix)
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
    /*   
    *tm11 = im11;     *tm21 = im12;     *tm31 = im13;
    *tm12 = im21;     *tm22 = im22;     *tm32 = im23;
    *tm13 = im31;     *tm23 = im32;     *tm33 = im33;
    */
    matrix[0] = im11; matrix[1] = im21; matrix[2] = im31;
    matrix[3] = im12; matrix[4] = im22; matrix[5] = im32;
    matrix[6] = im13; matrix[7] = im23; matrix[8] = im33;
 
}
 

static rotate_unity(matrix)
double matrix[9];
{
    /*Define the UNITY transformation matrix*****************/

    matrix[0] = 1.0;
    matrix[1] = 0.0;
    matrix[2] = 0.0;
    matrix[3] = 0.0;
    matrix[4] = 1.0;
    matrix[5] = 0.0;
    matrix[6] = 0.0;
    matrix[7] = 0.0;
    matrix[8] = 1.0;
 
}
 

/*
 * Fills a buffer with stuff to be sent to a TMS DSP chip on the
 * coordinate rotator board with NO header and NO trailer.
 * 
 * Buffer organization:
 * +--------------------------------------------+ 
 * |                               |
 * |  downloaded data (5 words)    		|
 * |                               | 
 * |  3 direction  1 magnet-frame     1 amp   	|
 * |   cosines         offset         delay  	|
 * +--------------------------------------------+  
 *
 * The user supplied "buffer" must have the length in bytes returned
 * by get_dsp_chip_download_bufsize
 */


static int
get_dsp_chip_download_bufsize()
{
    int nsize;

    nsize =  DSPBUFLEN * sizeof(int); 

    return nsize ;  
}


static 
make_dsp_chip_download(unsigned int *buffer,
		       double dir_cosines[3],
		       double lab_offset,
		       double amp_delay)
{
    unsigned int *pdata;
    int i;

    /* Load data */
    pdata = buffer;
    for (i=0; i<3; i++) {
	*pdata++ = float2tms32((float)dir_cosines[i]);
    }
    *pdata++ = float2tms32((float)lab_offset);
    *pdata++ = float2tms32((float)amp_delay);

}

/*
 * Takes three DSP download blocks (one for each chip), and forms an
 * APBOUT string that loads all three chips.
 * Returns a pointer to the APBOUT string.  (The length of the string
 * is encoded into the string.)
 * NB: The caller must free the memory for the string.
 */
static codeint *
make_apbout_from_dspcode(unsigned int *dspcode[], int nwords) /* 3 DSP code lists */
{
    int i, j;
    int outlen;			/* Number of codeints to output */
    int headlen;
    int taillen;

    /* NB: on board: X=reg3, Y=reg2, Z=reg1, ctl=reg0 */
    int regorder[] = {3, 2, 1};	/* First download to first reg # in list */

    /* codeint head[], codeint tail[]  are define as static at the beginning */

    codeint *pc;

   headlen = HEADLEN;
   taillen = TAILLEN;
   outlen = OUTLEN;


    /* Write header */
    pc = apbcmds;
    head[1] = outlen - 3;
    for (i=0; i<headlen; i++) {
	*pc++ = head[i];
    }

    /* Write to the FIFOs. Words go in bytewise, little-endian */
    for (i=0; i<NCHIPS; i++) {
	*pc++ = (APSELECT | CROT + regorder[i]);
	for (j=0; j<nwords; j++) {
	    *pc++ = CROTWRITE | (dspcode[i][j] & 0xff);
	    *pc++ = CROTWRITE | ((dspcode[i][j] >> 8) & 0xff);
	    *pc++ = CROTWRITE | ((dspcode[i][j] >> 16) & 0xff);
	    *pc++ = CROTWRITE | ((dspcode[i][j] >> 24) & 0xff);
	}
    }

    /* Write tail */
    for (i=0; i<taillen; i++) {
	*pc++ = tail[i];
    }

   /* 
    pc = apbcmds;
    for(i=0; i < outlen; i++)
    {
      fprintf(stderr," apb[%d] (0x%lx) = 0x%x (%d)\n",i,pc,*pc,*pc);
      pc++;
    }
    */
    return apbcmds;
}


/*
 * Load an APBOUT string into the Acode stream
 *    copied this from ecc_setup.c dlm 10/24/00
 */
static void
send_apbout_string(codeint *apb)
{
    int i;
    int len;
    codeint *apend;

    if (apb){

        len = apb[1] + 1;       /* Count word is (length - 1) */
        /* i = 0; */

        apend = apb + len + 2;  /* Include 2 hdr words (APBOUT + count) */
        while (apb < apend){
/*           printf("0x%04x\n", (int)*apb & 0xffff);
           printf("0x%08x\n", (int)*apb & 0xffff);
           printf("0x%08x\n", (int)*apb );  
*/
          /* fprintf(stderr," apb[%d] (0x%lx) = 0x%x (%d)\n",i,apb,*apb,*apb); */
           putcode(*apb++);
           /* i++; */
        }
        curfifocount += len;
    }
}


static double *
calcOffsetVals(offseta,offsetb,offsetc,offsets)
double offseta,offsetb,offsetc;
double *offsets;
{
	double dshimset;
	double offset1,offset2,offset3;
        float dtxshimscale,dtyshimscale,dtzshimscale;
        double *offsetvals;

        if ( P_getreal(GLOBAL,"shimset",&dshimset,1) != 0 )
        {
           text_error("PSG: shimset not found.\n");
           psg_abort(1);
        }

	dtxshimscale = get_decctool_shimscale(X1_AXIS);  /* shim scaling factors from  */	
	dtyshimscale = get_decctool_shimscale(Y1_AXIS);	 /* decctool - function itself */ 
	dtzshimscale = get_decctool_shimscale(Z1_AXIS);	 /* is in ecc_setup.c  */

        offsetvals = offsets;

	switch ((int)(dshimset+0.5)) {
          case 8: 
            *offsets++ = offset1 = (double)(offseta * 16.0 * dtxshimscale);  /* factor of 16 for 32k/2k  */
            *offsets++ = offset2 = (double)(offsetb * 16.0 * dtyshimscale);	/* Var14 with Oxford15 shims */
            *offsets = offset3 = (double)(offsetc * 16.0 * dtzshimscale);
	
	    break;
	  default:
            *offsets++ = offset1 = (double)(offseta * 1.0 * dtxshimscale);
            *offsets++ = offset2 = (double)(offsetb * 1.0 * dtyshimscale);
            *offsets = offset3 = (double)(offsetc * 1.0 * dtzshimscale);
	    break;
	}

/*
	printf("offset values after scaling:%8.6f %8.6f	%8.6f \n", offset1,offset2,offset3);
*/
	
	return offsetvals;
}

static double *
calcDelayVals(delaya,delayb,delayc,delays)
double delaya,delayb,delayc;
double *delays;
{
        double dlyvalx,dlyvaly,dlyvalz; 	/* number of 20MHz/50ns clock periods  */
	double maxdelay;
        double *delayvals;
/*
	printf("delay values as doubles:%8.6f	%8.6f	%8.6f \n", delaya,delayb,delayc);
*/
	if ((delaya >= delayb) && (delaya >= delayc)) 
	{
		maxdelay = delaya + 50.0e-9;
		dlyvalx = maxdelay - delaya;
		dlyvaly = maxdelay - delayb;
		dlyvalz = maxdelay - delayc;
	}
	else if ((delayb >= delaya) && (delayb >= delayc)) 
	{
		maxdelay = delayb + 50.0e-9;
		dlyvalx = maxdelay - delaya;
		dlyvaly = maxdelay - delayb;
		dlyvalz = maxdelay - delayc;
	}
	else if ((delayc >= delaya) && (delayc >= delayb))
	{
		maxdelay = delayc + 50.0e-9;
		dlyvalx = maxdelay - delaya;
		dlyvaly = maxdelay - delayb;
		dlyvalz = maxdelay - delayc;
	}
	else 
	{
           text_error("PSG: gradient amplifier group delays can't be calculated.\n");
           psg_abort(1);
        }

        delayvals = delays;

	*delays++ = delayvals[0] = (double)(dlyvalx/50.0e-9);
	*delays++ = delayvals[1] = (double)(dlyvaly/50.0e-9);
	*delays = delayvals[2] = (double)(dlyvalz/50.0e-9);
/*	
	printf("delay values as number of clock periods:  %8.2f	%8.2f	%8.2f \n", delayvals[0],delayvals[1],delayvals[2]);
*/	
	return delayvals;
}



static genApCmds(matrix,offset1,offset2,offset3,delay1,delay2,delay3)
double matrix[9];
double offset1,offset2,offset3;
double delay1,delay2,delay3;
{
    int i;
    int bsize;
    int wsize;

    /*
    bsize = get_dsp_chip_download_bufsize();
    wsize = bsize / sizeof(unsigned int);
    fprintf(stderr,"bsize: %d, BSIZE: %d, wise: %d, WSIZE: %d\n",bsize,BSIZE,wsize,WSIZE);
   */

    /* chip_bufs is a static defined above */
    make_dsp_chip_download(chip_bufs[0],
			   matrix,
			   offset1,
			   delay1);
    make_dsp_chip_download(chip_bufs[1],
			   matrix + 3,
			   offset2,
			   delay2);
    make_dsp_chip_download(chip_bufs[2],
			   matrix + 6,
			   offset3,
			   delay3);
/*   
    if (ix == 1)
	{
    	printf("\nOffset Values :\n");
    	printf("%9f   %9f   %9f\n", offset1, offset2, offset3);
    	printf("\nDelay Values :\n");
    	printf("%9f   %9f   %9f\n", delay1, delay2, delay3);
	print_dsp_buf(chip_bufs[0], wsize, "x");     
	print_dsp_buf(chip_bufs[1], wsize, "y");     
	print_dsp_buf(chip_bufs[2], wsize, "z");     
	}
*/

    make_apbout_from_dspcode(chip_bufs,WSIZE);

    /*print_apbout_string(apb); */   

    /* apbcmds is a static defined above & filled via make_apbout_from_dspcode */
    send_apbout_string(apbcmds);

}

 
/*
 * Put the values directly into the Acode stream.
 * Returns TRUE on success, FALSE on failure 
 */
void rotate_angle(ang1,ang2,ang3,offset1,offset2,offset3,delay1,delay2,delay3)
double ang1,ang2,ang3;
double offset1,offset2,offset3;
double delay1,delay2,delay3;
{
    	double matrix[9];
    	double xoffset,yoffset,zoffset;
    	double xdelay,ydelay,zdelay;
        double *delaysptr;
        double offsets[3],delays[3];;
 
        if ((gradtype[0] != 'R') && (gradtype[0] != 'r'))
           return;

    	rotateHW(ang1,ang2,ang3,matrix);
/*
	printf("offset values as doubles (in rotate_angle):%8.6f %8.6f	%8.6f \n", offset1,offset2,offset3);
*/
	calcOffsetVals(offset1,offset2,offset3,offsets);
	xoffset = offsets[0];
	yoffset = offsets[1];
	zoffset = offsets[2];
/*	
	printf("delay values as doubles (in rotate_angle):%8.6f	%8.6f	%8.6f \n", delay1,delay2,delay3);
*/
	calcDelayVals(delay1,delay2,delay3,delays);
	xdelay = delays[0];
	ydelay = delays[1];
	zdelay = delays[2];
	
	genApCmds(matrix,xoffset,yoffset,zoffset,xdelay,ydelay,zdelay);
	delay(20.0e-6);                        /* 20us is the absolute minimum */
	zero_all_gradients();
	delay(5.0e-6);
	

}

/*
 *  this is the init_crb() routine to download a unity matrix to the coordinate rotator board
 */

int init_crb()
{
 	double matrix[9];
	double init_offset1 = 0.0;
	double init_offset2 = 0.0;
	double init_offset3 = 0.0;
	double init_delay1 = 1.0;
	double init_delay2 = 1.0;
	double init_delay3 = 1.0;
	rotate_unity(matrix);
	genApCmds(matrix,init_offset1,init_offset2,init_offset3,init_delay1,init_delay2,init_delay3);
	delay(20.0e-6);                        /* 20us is the absolute minimum */
	zero_all_gradients();
	delay(5.0e-6);

}
 

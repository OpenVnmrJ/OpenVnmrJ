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

#include <stdio.h>
#include <stdlib.h>

#include "vnmrsys.h"
#include "acqparms2.h"
#include "lc_gem.h"
#include "acodes.h"

#define OK	0	
#define SYSERR	-1	/* compatible with most UNIX error returns */

#define HEADER_ARRAYSIZE 256
#define MAXGLOBALSEG 10
#define MAXFREQACODE 100	/* was 24 on UnityPLUS; needs to
				    be much larger for Mercury	*/
/* For frequency lists ... */
#define	ACQ_XPAN_GSEG	5 


extern int	bb_refgen, cardb, xltype;
extern int	bgflag;		/* print diagnostics if not 0 */
extern int	newacq;		/* Mercury with VxWorks */ 


struct acode_globindx {
	int	gseg;		/* number of global segment to use */
	int	listcnt;
				/* number of acodes for each frequency 	*/
				/* setting.				*/
	int	acodes_forelem[HEADER_ARRAYSIZE];
	int	tblnum[HEADER_ARRAYSIZE];
	} gfreq = { -1, 0};


codeint	*GTABptr[MAXGLOBALSEG];
codeint *GtabStart,*GtabEnd;


/*  Used by the Qtune program ...  */

int
get_maxfreqacode()
{
	return( MAXFREQACODE );
}

/************************************************************************
* init_global_list needs to be called once at the beginning of an
* experiment acquisition.  It initializes the freq tables used  
* in the acquisition.
*************************************************************************/
init_global_list(globaltable)
int globaltable;	
{
	gfreq.gseg = globaltable;
	gfreq.listcnt = 0;

	return( OK );
}


/************************************************************************
* Description:
*       Adapted from the UnityPLUS/INOVA version, with emphasis on the
*	INOVA version
*
*	Element will generate frequencies for whatever the current base
*	frequency is.  Will create the list if the list specified by
*	the list number has not been created before.
*
* Arguments: 
*	list		: pointer to a list of frequency offsets 
*	nvals		: number of values in the list 
*	device		: TODEV, DODEV 
*	list_no		: 0-255
* 							
* Return value: 
*	>= 0, Number of the list it has created. 
*        < 0, No list created.
*************************************************************************/
int Create_freq_list(list, nvals, device, list_no)
double *list;
int	nvals;
int	device;
int	list_no;
{
	char	 msge[128];
	int	 acodes_per_elem, size;
	codeint *outputbuf;

 /* check to make sure a valid global segment is available	*/

	if (gfreq.gseg < 0) {
		text_error("Global segment not available for frequency list.\n");
		psg_abort(1);
	}

	if (list_no < 0) {
		list_no = gfreq.listcnt;
	}

	if (gfreq.listcnt == list_no) {
		size = nvals*MAXFREQACODE*2;	/* size of buffer in bytes */
		outputbuf = (codeint *)malloc(size);
		gfreq.acodes_forelem[list_no] = acodes_per_elem =
		  Create_mem_freq_list(list, nvals, device, outputbuf);

		gfreq.tblnum[list_no] = createglobaltable(nvals, acodes_per_elem*2, 
						      (char *)outputbuf);
		free(outputbuf);
		gfreq.listcnt++;
		return(list_no);
	}
	else {
		if (gfreq.listcnt > list_no) {
			if (bgflag) {
				sprintf(msge,
		   "Warning: List [%d] already created.\n", list_no
			);
			text_error(msge);
			}
		}
		if (gfreq.listcnt < list_no) {
			sprintf(msge, "Error: List [%d] not in order.\n",list_no);
			text_error(msge);
			sprintf(msge, 
		   "Make sure to start at 0, and to have unique list\n"
			); 
			text_error(msge);
			sprintf(msge,"numbers for each list.\n");
			text_error(msge);
			psg_abort(1);
		}
		return(-1);
	}
}

/************************************************************************
 * Description:
 *	Element will generate a list of frequency setting Acodes.
 *
 * Arguments: 
 *	list		: pointer to a list of frequencys
 *	nvals		: number of values in the list 
 *	device		: TODEV, DODEV
 *	outputbuf	: address of list in memory
 * 							
 * Return value: 
 *	Number of Acodes in each list element.
 *************************************************************************/
int
Create_mem_freq_list(list, nvals, device, outputbuf)
double *list;
int	nvals;
int	device;
codeint *outputbuf;
{
	int	 iter;
	int	 acodes_forelem;
	codeint *beginaddr;
	char	 msge[ 128 ];

	GTABptr[gfreq.gseg] = GtabStart = outputbuf;
	GtabEnd = &outputbuf[(nvals*MAXFREQACODE)-1];
	acodes_forelem = 0;

	for (iter = 0; iter < nvals; iter++) {
		int num_acodes_forelem;

		beginaddr = GTABptr[gfreq.gseg];
		GTABptr[gfreq.gseg]++;
		if (cardb)
		  setlbfreq_table( list[ iter ], 0.0 );
		else
		  sethbfreq_table( list[ iter ], 0.0 );

		num_acodes_forelem = (int)((long)GTABptr[gfreq.gseg] - 
				     (long)beginaddr)/2;
		if (acodes_forelem == 0) {
			acodes_forelem = num_acodes_forelem;
		} else {
			if (acodes_forelem != num_acodes_forelem) {
				sprintf(msge,
			   "freq_list: Mismatch in freq acodes %d, is %d\n",
			    acodes_forelem, num_acodes_forelem
				);
				text_error(msge);
				psg_abort(1);
			}
		}
		*beginaddr = acodes_forelem;
	}

	return( acodes_forelem );
}


#define	APB_SELECT	24576	/* A000, if used with minus sign */
#define	APB_WRITE	20480	/* B000, if used with minus sign */
#define	APB_INRWR	28672	/* 9000, if used with minus sign */

#define	RF_ADDR		0xa00	/* apbus address for al rf boards */
#define	LB_XMT_SEL	0x00
#define	HB_XMT_SEL	0x18

#define FACTOR  (double)(0x40000000/10e6)
#define	LOCK_SEL	0x030
#define LOCK_PLLM	0x020
#define	LOCK_PLLA	0x022
#define	LOCK_STR	0x02e
/*------------------------------*/
/*	setoffset table		*/
/*  number=offset*(2^32)/40e6   */
/*------------------------------*/
setoffset_table(offset,address)
double	offset;
int	address;
{

   int bits32, llbyte, hlbyte, lhbyte, hhbyte;

   if (address == LOCK_SEL) {
      text_error( "set offset table cannot be used with the lock system" );
      psg_abort( 1 );
   }
   bits32 = (int) (offset * FACTOR);
   llbyte = ((bits32)&0xFF);
   hlbyte = ((bits32 >>  8)&0xFF);
   lhbyte = ((bits32 >> 16)&0xFF);
   hhbyte = ((bits32 >> 24)&0xFF);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x0A);	/* DDS1 AMC register */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + 0x8F);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x08);	/* DDS1 SMC register */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x0C);	/* DDS1 ARR register */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x00);	/* DDS1 PIRA 0-7 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + llbyte);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x01);	/* DDS1 PIRA 8-15 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + hlbyte);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x02);	/* DDS1 PIRA 16-23 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + lhbyte);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x03);	/* DDS1 PIRA 24-31 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + hhbyte);

   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x04);	/* DDS1 PIRB 0-7 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x05);	/* DDS1 PIRB 8-15 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x06);	/* DDS1 PIRB 16-23 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x07);	/* DDS1 PIRB 24-31 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);

   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x1A);	/* DDS2 AMC */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + 0x2F);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x18);	/* DDS2 SMC */
   /*if (address == LOCK_SEL)
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + 0x08);
   else*/			/* LOCK_SEL is disallowed, see top of program */
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + 0x0A);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x1C);	/* DDS2 ARR register */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x10);	/* DDS2 PIRA 0-7 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + llbyte);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x11);	/* DDS2 PIRA 8-15 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + hlbyte);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x12);	/* DDS2 PIRA 16-23 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + lhbyte);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x13);	/* DDS2 PIRA 24-32 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + hhbyte);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x14);	/* DDS2 PIRB 0-7 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x15);	/* DDS2 PIRB 8-15 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x16);	/* DDS2 PIRB 16-23 */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x17);	/* DDS2 PIRB 24-31 */
   /*if (address == LOCK_SEL)
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + ((int)(lockphase*256.0/360.0)&0xFF));
   else*/			/* LOCK_SEL is disallowed, see top of program */
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);

   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x1E);	/* DDS2 AHC */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);

   putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + address);
   putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + 0x0E);	/* DDS1 AHC register */
   putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
}

#define LB_PLL_SEL	0x08
#define LB_PLL_M1	0x20
#define LB_PLL_A1	0x22
#define LB_PLL_STR1	0x2e
#define LB_PLL_M2	0x30
#define LB_PLL_A2	0x32
#define LB_PLL_STR2	0x3e
/*-------------------------------*/
/*       setlbfreq_table         */
/*-------------------------------*/
setlbfreq_table(freq,off)
double	freq;
double	off;
{
double	fabs();
double	offset;
double	ref_freq;
double	LO_freq, xabs, best_xabs;
int	test_r, test_v, r, v;
   if (bb_refgen)
   {  /* test has shown that 90 < r <= 150		*/
      /*                     98 < v <  221		*/
      /* when  20 < freq < 161 for BB RefGen 200-400 MHz*/
      best_xabs = 1000.0;
      for (test_r=150; test_r>90; test_r--)
         for (test_v=221; test_v>98; test_v--)
         {  LO_freq=360.0 * test_v / test_r - 360.0;
            xabs = fabs(LO_freq-freq-off/1e6-10.65);
            if (xabs < best_xabs)
            {
                best_xabs=xabs;
                r=test_r; v=test_v;
            }
         }
      offset = (360.0*v/r-360.0-freq)*1e6 - off;
   }
   else
   {  /* test has shown that  8 < r <= 16		*/
      /*                     28 < v <=110		*/
      /* when nucleus is 13C/31P for 200, 300, 400 MHz	*/
      best_xabs = 1000.0;
      if (xltype == 200) ref_freq=10.0;
      else               ref_freq=20.0;
      for (test_r=16; test_r>8; test_r--)
         for (test_v=110; test_v>28; test_v--)
         {  LO_freq = ref_freq * test_v / test_r;
            xabs = fabs(LO_freq-freq-off/1e6-10.65);
            if (xabs < best_xabs)
            {  best_xabs = xabs;
               r = test_r;
               v = test_v;
            }
         }
      offset = (ref_freq * v/r - freq)*1e6 - off;
    }
/* printf("LOW: r=%d, v=%d, error=%f, offset=%12.4f sfrq=%g\n",
		r,v,best_xabs,offset,freq); */
   if (bb_refgen)
   {  putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + LB_PLL_M1);
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + ((r/10 - 1)&0xFF) );
      putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + LB_PLL_A1);
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + ((r%10)&0xFF) );
      putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + LB_PLL_STR1);
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
      putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + LB_PLL_M2);
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + ((v/10 - 1)&0xFF) );
      putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + LB_PLL_A2);
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + ((v%10)&0xFF) );
      putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + LB_PLL_STR2);
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
   }
   else
   {  putgtab(gfreq.gseg, 0xAA10);
      putgtab(gfreq.gseg, 0xBA22); /* select M register */
      putgtab(gfreq.gseg, 0x9A00 | (v+127) );	/* (M-1) + 128 for sign bit */
      putgtab(gfreq.gseg, 0xAA10);
      putgtab(gfreq.gseg, 0xBA2e);   /* select R+A register */
      putgtab(gfreq.gseg, 0x9A00 | (r-1)*16 );
      putgtab(gfreq.gseg, 0xAA10);
      putgtab(gfreq.gseg, 0xBA20);
      putgtab(gfreq.gseg, 0x9a00);   /* anything for strobe */

      putgtab(gfreq.gseg, 0xAA12);
      if ((int)freq < (xltype/3) )
         putgtab(gfreq.gseg, 0xba00);	/* select 13C filter */
      else
         putgtab(gfreq.gseg, 0xBA01);	/* select 31P filter */
   }

   setoffset_table(offset,LB_XMT_SEL);
}

#define HB_PLL_SEL	0x08
#define HB_PLL_M1	0x40
#define HB_PLL_A1	0x42
#define HB_PLL_STR	0x4e
/*-------------------------------*/
/*       sethbfreq_table         */
/*-------------------------------*/
sethbfreq_table(freq,off)
double	freq;
double	off;
{
double	fabs();
double	offset;
double	LO_freq, xabs, best_xabs;
int	test_r, test_v, r, v;

/* first lets find the best r and v values	*/
/* test has shown that  8 < r <= 16		*/
/*                     85 < v <=310		*/
/* when nucleus is 19F/1H for 200, 300, 400 MHz	*/
   best_xabs = 1000.0;
   for (test_r=16; test_r>8; test_r--)
      for (test_v=310; test_v>85; test_v--)
      {  LO_freq = 20.0 * test_v / test_r;
         xabs = fabs(LO_freq-freq-off/1e6-10.65);
         if (xabs < best_xabs)
         {  best_xabs = xabs;
            r = test_r;
            v = test_v;
         }
      }

   offset = (20.0*v/r-freq)*1e6-off;
   /*hr=r; hv=v;	Not required for frequency tables */
/* printf("HI: r=%d, v=%d, error=%f, offset=%12.4f, sfrq=%g\n",
			r,v,best_xabs,offset,freq); */
   if (bb_refgen)
   {  putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + HB_PLL_SEL);
      putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + HB_PLL_M1);
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + ((v/10 - 1)&0xFF) );
      putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + HB_PLL_SEL);
      putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + HB_PLL_A1);
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR + (((v%10)+(r-1)*16)&0xFF) );
      putgtab(gfreq.gseg, -APB_SELECT + RF_ADDR + HB_PLL_SEL);
      putgtab(gfreq.gseg, -APB_WRITE  + RF_ADDR + HB_PLL_STR);
      putgtab(gfreq.gseg, -APB_INRWR  + RF_ADDR);
   }
   else
   {  putgtab(gfreq.gseg, 0xAA10);
      putgtab(gfreq.gseg, 0xBA80);
      putgtab(gfreq.gseg, 0x9a00 | (v/10 - 1));
      putgtab(gfreq.gseg, 0xAA10);
      putgtab(gfreq.gseg, 0xBA10);   /* select R+A register */
      putgtab(gfreq.gseg, 0x9A00 | ((r-1)*16 + v%10) );
      putgtab(gfreq.gseg, 0xAA10);
      putgtab(gfreq.gseg, 0xBA50);
      putgtab(gfreq.gseg, 0x9a00);   /* anything for strobe */
   }

   setoffset_table(offset,HB_XMT_SEL);
}


static int	gtab_count = 0;

putgtab( table, word )
int table;
codeint word;
{
	*GTABptr[table]++ = word; 		/* Put word into Codes array */

    /* test for acodes overflowing malloc acode memory */

	if ((long)GTABptr[table] > (long)GtabEnd) {    
		char msge[128];
		sprintf(msge,"Acode overflow, %ld words generated.",
			 (long) (GTABptr[table] - GtabStart));
		text_error(msge);
		psg_abort(0); 
	}

	gtab_count++;
}

/* if newacq == TRUE, then close_global_lost is a NO-OP in Unity PSG.
   Since the Mercury application only allows global lists if newacq == TRUE,
   close_global_list becomes a NO-OP.  It is being kept so the Mercury PSG
   and Unity PSG will have common source code entry points.		*/

close_global_list()
{
	return( OK );
}

/************************************************************************
* Description:
*	Element creates the acodes for indexing into a table of previously
*	created frequency offsets or frequencies.   
*
* Arguments: 
*	list_no		: 0-255 should match will a generated list using
*			  create_offset_list or create_freq_list. 
*	vindex		: realtime variable v1-v14 etc., table T1-T60
*			: used to index to the desired frequency.
* 							
* Return value: 
*	NONE 
*  
*************************************************************************/
void vget_elem(list_no, vindex)
int 	list_no;
codeint	vindex;
{
	char msge[MAXPATHL];

   /* check to make sure a valid global segment is available	*/

	if (gfreq.gseg < 0) {
		text_error("Global segment not available for frequency list.\n");
		psg_abort(1);
		}

   /* test valid apbus range for vindex */

	if ((vindex < v1) || (vindex > v10)) {
		sprintf(msge,"voffset: vindex illegal dynamic %d \n", vindex);
		text_error(msge);
		psg_abort(1); 
	}

   /* test valid list number */

	if (gfreq.acodes_forelem[list_no] == 0) {
		sprintf(msge, "List number: %d is incorrect.\n",list_no);
		text_error(msge);
		psg_abort(1);
	}

   /* sprintf(msge,"voffset: list[%d] gseg: %d  freq_addr: %d vindex: %d\n", */
   /*		list_no,gfreq.gseg,gfreq.acodes_forelem[list_no],vindex); */
   /* text_error(msge); */

	putcode((codeint)GTABINDX);
	putcode((codeint)gfreq.gseg);
	putcode((codeint)gfreq.tblnum[list_no]);
	putcode((codeint)gfreq.acodes_forelem[list_no]);
	putcode((codeint)vindex);
}

/*--------------------------------------------------------------------
| ClearTable()
|       zero out array or structure
|       tableptr - pointer to array or structure
|       tablesize - size of array or structure in Bytes.
|                               Author Greg Brissey 8/9/88
|       taken from device.c, SCCS category psg  11/19/1997
+-------------------------------------------------------------------*/
ClearTable(tableptr, tablesize)
register char  *tableptr;
register long   tablesize;
{
   register long   i;

   for (i = 0L; i < tablesize; i++)
      *tableptr++ = 0;
}

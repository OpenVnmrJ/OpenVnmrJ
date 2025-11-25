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
#ifndef CONFIG_H
#define CONFIG_H

#include "sram.h"

#ifdef LINUX

#include <netinet/in.h>

#define HWCONF_CONVERT(data) \
{ \
   int count; \
   data->valid_struct = ntohs(data->valid_struct); \
   data->size = ntohs(data->size); \
   data->InterpVer = ntohs(data->InterpVer); \
   data->SystemVer = ntohs(data->SystemVer); \
   data->system = ntohs(data->system); \
   data->H1freq = ntohs(data->H1freq); \
   data->sram = ntohs(data->sram); \
   data->STM_present = ntohs(data->STM_present); \
   for (count = 0; count < 6; count++) \
      data->STM_Type[count] = ntohs(data->STM_Type[count]); \
   data->ADCsize = ntohs(data->ADCsize); \
   data->WLadc_present = ntohs(data->WLadc_present); \
   data->spin = ntohs(data->spin); \
   data->VTpresent = ntohs(data->VTpresent); \
   data->PTSes_present = ntohs(data->PTSes_present); \
   data->amprout1_ver = ntohs(data->amprout1_ver); \
   data->amprout2_ver = ntohs(data->amprout2_ver); \
   data->AMT1_conf = ntohs(data->AMT1_conf); \
   data->AMT2_conf = ntohs(data->AMT2_conf); \
   data->AMT3_conf = ntohs(data->AMT3_conf); \
   data->attn1_range = ntohs(data->attn1_range); \
   data->attn2_range = ntohs(data->attn2_range); \
   data->attn1_present = ntohs(data->attn1_present); \
   data->attn2_present = ntohs(data->attn2_present); \
   data->mleg_conf = ntohs(data->mleg_conf); \
   data->solids_conf = ntohs(data->solids_conf); \
   data->probe = ntohs(data->probe); \
   data->user1 = ntohs(data->user1); \
   data->solids_rtn = ntohs(data->solids_rtn); \
   for (count = 0; count < 4; count++) \
      data->rcvr_stat[count] = ntohs(data->rcvr_stat[count]); \
   data->xmtr_present = ntohs(data->xmtr_present); \
   for (count = 0; count < 6; count++) \
      data->xmtr_stat[count] = ntohs(data->xmtr_stat[count]); \
   data->amp_present = ntohs(data->amp_present); \
   data->fifolpsize = ntohs(data->fifolpsize); \
   data->shimtype = ntohs(data->shimtype); \
   for (count = 0; count < 4; count++) \
      data->gradient[count] = ntohs(data->gradient[count]); \
   data->homodec = ntohs(data->homodec); \
   data->dspPromType = ntohs(data->dspPromType); \
   for (count = 0; count < SRAM_SIZE; count++) \
      data->sram_val[count] = ntohs(data->sram_val[count]); \
}
#endif

/* _hw_config is 76 bytes as of 4/14/92 */
/* _hw_config measured at 100 bytes as of 11/20/95 */

struct _hw_config
{
#ifndef ALCYON 
	int	 dspDownLoadAddr;  /* Unity INOVA only */
	int	 dspDownLoadAddrs[6];  /* added for multi-receiver configuration where */
				       /* more than 1 DSP needs software downloaded.  */
#else
	char	*dspDownLoadAddr;  /* Unity INOVA only, alcyon compiler doesn't know void  
				      (assumption is  void* & char* are same size) */
	char	*dspDownLoadAddrs[6];  /* added for multi-receiver configuration where */
				       /* more than 1 DSP needs software downloaded.  */
#endif
				/* VME address for DSP download if DSP board present */
				/* 0xFFFFFFFF is no DSP download is to be done.      */
	short	valid_struct;	/* All systems                       */
				/* non-valid=0, valid=CONFIG_VALID=4 */
	short	size;		/* All systems                               */
				/* size of structure in 512 bytes, so far =1 */
        short   InterpVer;	/* Acode Interpreter Version Number */
        short   SystemVer;	/* System Version Number */
	short	system;		/* All systems        */
				/* 0 = do not bootup  */
				/* 1 = load xrop.out  */
				/* 2 = load xrxrp.out */
				/* 3 = load xrxrh.out */
				/* 4 = load xr.diag   */
				/* 5 = load xrxrh_img.out, autshm_img.out */
				/* 7 = Mercury        */
	short	H1freq;		/* UnityPlus only                        */
				/* Console 1H frequency in MHz           */
				/* 0x00 = unknown H1freq                 */
				/* 0x20 = 200 MHz                        */
				/* 0x30 = 300 MHz                        */
				/* 0x40 = 400 MHz                        */
				/* 0x50 = 500 MHz                        */
				/* 0x60 = 600 MHz                        */
				/* 0xf0 = broadband lock, unknown H1freq */
				/* 0x1ff = no lock cntl, unknown H1freq  */
/*  H1freq is not used...  hardware is not functional.  see static RAM, below */
	short	sram;		/* All systems                        */
				/* not-present=0, present=1=UnityPlus */
	short	STM_present;	/* All systems              */
				/* Number of STM found      */
        short	STM_Type[6];	/* see stmObj.c for types */
	short	ADCsize;	/* All systems                     */
				/* 18, 16, 15, 12 or 0=not present */
				/* INOVA adds 0x500 to show speed=500 kHz */
	short	WLadc_present;	/* All systems              */
				/* not-present=0, present=1 */
	short	spin;		/* Mercury systems              */
				/* not-present=0, present=1 */
	short	VTpresent;	/* All systems              */
				/* not-present=0, present=1 */
	short	PTSes_present;	/* UnityPlus only                           */
				/* bit=0 no PTS present, bit=1 PTS present  */
				/* bit 0 for channel 1, bit 1 for channel 2 */
				/* bit 2 for channel 3, bit 3 for channel 4 */
				/* bit 4 for channel 5, bit 5 for channel 6 */
	short	amprout1_ver;	/* UnityPlus only                        */
				/* version of the first  amp-route board */
	short	amprout2_ver;	/* UnityPlus only                        */
				/* version of the second amp-route board */
	short	AMT1_conf;	/* UnityPlus only                    */
				/* configuration of AMT1             */
				/* 0x0 AMT 3900-11  Lo-Lo band       */
				/* 0x1 AMT 3900-12  Hi-Lo band       */
				/* 0x4 AMT 3900-1S  Lo band          */
				/* 0x8 AMT 3900-1   Lo band          */
				/* 0xb AMT 3900-1S4 HI-Lo band       */
				/* 0xc AMT 3900-15  Hi-Lo band(100W) */
				/* 0xf None\n");                     */
	short	AMT2_conf;	/* UnityPlus only        */
				/* configuration of AMT2 */
	short	AMT3_conf;	/* UnityPlus only        */
				/* configuration of AMT3 */
	short	attn1_range;	/* UnityPlus only                     */
				/* 63.5 or 79 dB attns on first board */
				/* 0x0 = 63.5 dB, 0xf = 79 dB         */
	short	attn2_range;	/* UnityPlus only                     */
				/* 63.5 or 79 dB attns on first board */
	short	attn1_present;	/* UnityPlus only                            */
				/* attns present on first board              */
				/* bit=0 no attn present, bit=1 attn present */
				/* bit 0 for channel 1, bit 1 for channel 2  */
				/* bit 2 for channel 3, bit 3 for channel 4  */
	short	attn2_present;	/* UnityPlus only                            */
				/* attns present on second board             */
				/* bit=0 no attn present, bit=1 attn present */
				/* bit 0 for channel 5, bit 1 for channel 6  */
	short	mleg_conf;	/* UnityPlus only              */
				/* configuration of magnet leg */
				/* 0x0 Standard Unity Plus     */
				/* 0x01 High Dynamic Range preamp  */
				/* 0x10 SIS PIC */
	short	solids_conf;	/* UnityPlus only              */
				/* configuration of solids bay */
	short	probe;		/* UnityPlus only          */
				/* probe type, not defined */
	short	user1;		/* UnityPlus only  */
				/* input from user */
	short	solids_rtn;	/* UnityPlus only */
				/* ?????          */
	short	rcvr_stat[4];	/* UnityPlus only       */
				/* receiver status byte */
	short	xmtr_present;	/* UnityPlus only          */
				/* bit=0 no xmtr present, bit=1 xmtr present */
				/* bit 0 for channel 1, bit 1 for channel 2  */
				/* bit 2 for channel 3, bit 3 for channel 4  */
				/* bit 4 for channel 5, bit 5 for channel 6  */
	short	xmtr_stat[6];	/* UnityPlus only                           */
				/* xmtr status byte                         */
				/* bit 0 = 40 MHz connected if equal 1      */
				/* bit 1 = ?                                */
				/* bit 2 = xmtr connected to xmtr cntl if 0 */
				/* bit 3 = wfg  connected to xmtr cntl if 0 */
	short	amp_present;	/* UnityPlus only   */
				/* no guarantee yet */
	short	fifolpsize;	/* All systems                           */
				/* Looping-FIFO-Size of the Output Board */
				/* possible values are: 63, 1024, 2048   */
	short	shimtype;	/* UnityPlus only                      */
				/* 0x001 = Old Varian shims (13 shims) */
				/* 0x002 = New Varian shims (23 shims) */
				/* 0x003 = New Varian shims (28 shims) */
				/* 0x1ff = Unknown                     */
	short	gradient[4];	/* INOVA only			*/
				/* g[0]=x g[1]=y g[2]=z	g[3]=r	*/
				/* g=0=Unknown			*/
				/* g='l'=Performa I		*/
				/* g='p'=Performa II		*/
	short	homodec;	/* MERCURY			*/
				/* Not Present=0, Present=1	*/
	short	dspPromType;	/* INOVA only			*/
				/* 0 ==> no DSP board at all	*/
				/* 1 ==> DSP board has PROMs not capable of using download */
				/* 2 ==> DSP board PROMs can use downloaded programs */
/*  A contradiction is possible if dspPromType is 0 or 1
    and dspDownLoadAddr is neither NULL nor -1 ... OR
    if dspPromType is 2 and dspDownLoadAddr is NULL or -1
    In all cases dspDownLoadAddr determines whether programs are downloaded from the host */

/*  Note that further fields are ALL expected to come from Hydra SRAM  */

	short	sram_val[ SRAM_SIZE ];


/* These were the previous definitions.  They are now part of sram_val */

/*	short	PTS_value[6];	   UnityPlus only               */
				/* values of PTS or fixed freqs */
				/* from SRAM, only on UnityPlus */
				/* possible values are:         */
				/* 250, 320, 500, 620           */
/*	short	lockfreq[2];	   UnityPlus only                   */
				/* from SRAM, system lockfreq value */
				/* from 0-200 MHz in 1 Hz           */
/*	short	checksum;	   UnityPlus only SRAM checksum       */

};

#define ACQ_CONFIG_ADDR	0x100000	/* where xr.conf creates hw_conf */

#define CONFIG_INVALID	0
#define	CONFIG_VALID	4

#define NOT_PRESENT	0
#define PRESENT		1

#define MLEG_SIS_PIC 0x10	/* Id on magnet leg driver board for SIS PIC */
#define MLEG_LGSIGMODE 0x01	/* Id on magnet leg driver board for U+      */
				/* Preamps with selectable Large-Signal Mode */

#endif

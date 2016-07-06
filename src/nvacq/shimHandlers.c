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

#define STATIC_SHIMSET
#include <vxWorks.h>
#include <semLib.h>
#include <ioLib.h>
#include "logMsgLib.h"
#include "Console_Stat.h"
#include "masterSPI.h"
#define STATIC_SHIMSET_PARM   // if ifdef out the extern int shimSet in following header
#include "serialShims.h"
#include "FFKEYS.h"
#include "errorcodes.h"

#ifndef MAXSHIMS
#define  MAXSHIMS	48
#endif

extern Console_Stat	*pCurrentStatBlock;	/* Acqstat-like status block */
extern int		 shimType;
extern int               XYZgradflag;

static int shimSet=-1;

/* the Host computer software (Vnmr or VnmrJ) translate the
/* name of the shim ('z0','z1f','x2-y2', etc) to a number between 
/* 0 an 47. fOr the SPI shims the needs to be reset to a number
/* between 1 and 40. Missing entrees will return -1.
/* */
/*
/* This first table is for Minoru or TimShimPlus
/* */
static const int
shimNumberToSpi[] = {
	-1,	//  0 = not here
	 1,	//  1 = $ 1 = z0c
	 2,	//  2 = $ 2 = z1f   z1f and z1c are the same
	-1,	//  3 = no here
	 3,	//  4 = $ 4 = z2f   z2f and z2c are the same
	-1,	//  5 = not here
	 4,	//  6 = $ 6 = z3
	 5,	//  7 = $ 7 = z4
	 6,	//  8 = $ 8 = z5
	 7,	//  9 = $ 9 = z6
	 8,	// 10 = $ a = z7
	-1,	// 11 = not here
	25,	// 12 = $ c = zx3             zc3
	26,	// 13 = $ d = zy3             zs3
	27,	// 14 = $ e = z4x
	28,	// 15 = $ f = z4y
	 9,	// 16 = $10 = x
	10,	// 17 = $11 = y
	11,	// 18 = $12 = xz
	14, 	// 19 = $13 = xy              s2?
	13,	// 20 = $14 = x2-y2           c2?
	12,	// 21 = $15 = yz
	19,	// 22 = $16 = x3              c3
	20,	// 23 = $17 = y3              s3
	16,	// 24 = $18 = yz2
	17,	// 25 = $19 = zx2-y2          zc2
	15,	// 26 = $1a = xz2
	18,	// 27 = $1b = zxy             zs2
	21,	// 28 = $1c = z3x
	22,	// 29 = $1d = z3y
	23,	// 30 = $1e = z2(x2-y2)       z2c2
	24,	// 31 = $1f = z2xy            z2s2
	29,	// 32 = $20 = z3x2y2          z3c2
	30,	// 33 = $21 = z3xy            z3s2
	33,	// 34 = $22 = z2x3            z2c3
	34,	// 35 = $23 = z2y3            z2s3
	37,	// 36 = $24 = z3x3            z3c3
	38,	// 37 = $25 = z3y3            z3s3
	31,	// 38 = $26 = z5x
	32,	// 39 = $27 = z5y
	35,	// 40 = $28 = z4x2y2          z4c2
	36,	// 41 = $29 = z4xy            z4s2
	-1,	// 42 = not here
	-1,	// 43 = not here
	39,	// 44 = $2c = x4              c4
	40,	// 45 = $2d = y4              
	-1,	// 46 = not here
	-1,	// 47 = not here
};

const int *qspi_dac_addr = 0L;

/* This table is for 14 channel shims box */
static const int
qspi_dac_table[ MAXSHIMS ] = {
	-1,
	4,
	0,
	1,
	2,
	3,			/* 5 */
	5,
	6,
	7,
	-1,
	-1,			/* 10 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 15 */
	8,
	9,
	10,
	13,
	12,			/* 20 */
	11,
	14,
	15,
	-1,
	-1,			/* 25 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 30 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 35 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 40 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 45 */
	-1,
	-1
};

/* This table is for 14-channel shim box driving imaging shims. */
static const int
imgqspi_dac_table[ MAXSHIMS ] = {
	-1,
	4,
	0,
	-1,
	2,
	-1,			/* 5 */
	5,
	6,
	-1,
	-1,
	-1,			/* 10 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 15 */
	8,
	9,
	10,
	12,
	13,			/* 20 */
	11,
	-1,
	-1,
	15,
	1,			/* 25 */
	14,
	3,
	-1,
	-1,
	-1,			/* 30 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 35 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 40 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 45 */
	-1,
	-1
};

static int shimDelay = 1360;	/* set to minimum for 16-bit SPI */

void setShimTimeDelay(int time)
{
   shimDelay = time;
}

/* shimType is QSPI_SHIMS, 14-channel shim box */
void qspiShimInterface(int *paramvec, int *index, int count, int fifoFlag)
{
int cs, dacNo, dacInPack, readValue;
int token1, token2;
long fWord[10];
   DPRINT1( -5,"qspiShims(): shimDelay = %d \n", shimDelay);
//   DPRINT2( 1,"index=%d, count=%d\n",*index, count);
   if (shimDelay < 1360) shimDelay = 1360;
   while (*index < count)
   { 
      (*index)++;     // skip over the SHIMDAC token
      token1 = paramvec[*index]; (*index)++;
      token2 = paramvec[*index]; (*index)++;
      DPRINT3(2,"token1=%d, token2=%d index=%d\n",token1,token2,*index);
      pCurrentStatBlock->AcqShimValues[token1] = token2;

      dacNo = qspi_dac_addr[ token1 ];
      if (dacNo == -1) continue;
      cs = (dacNo/4) + 3;		// which DAC package
      dacInPack = (dacNo % 4) << 14;	// which DAC in package

      if (token2 > 2047)
         token2 = 4095;
      else if (token2 < -2048)
         token2 = 0;
      else				// this 'else' is not right
         token2 += 2048;		// 0=-5 V, 2048=0V, 4095=+5V

      DPRINT3(2,"cs=%d dacInPack=%x  value = %d\n",cs,dacInPack,token2);
      readValue = (unsigned int)((cs<<24) | dacInPack | token2);
      DPRINT1(2,"outValue=%x\n",readValue);
      if (fifoFlag)
      {
         /* 17 usec/shim, this is hardcoded in psg's	   */
         /* MasterController::loadshim() 		   */
         /* if you change it here, change it there as well */
         /* 1280 is 16 usec, for 16 bits, 1usec to load    */
         fWord[0] = (DURATIONKEY | shimDelay);   
         // use port << 28, but port = 0 here
         fWord[1] = (LATCHKEY    | SPIKEY | readValue);
         writeCntrlFifoBuf(fWord,2); 
      }
      else
         readValue = hsspi(0, readValue );
      DPRINT1(2,"hsspi read: %x\n",readValue);
   }
}


void sendSpiShim(int token1, int token2, int fifoFlag)
{
      int dacNo,cs,dacInPack,tmp,readValue;
      int fWord[2];

      dacNo = shimNumberToSpi[ token1 ];
      if (dacNo == -1) return;
      cs = (dacNo-1)/4;
      switch (dacNo%4)
      {
      case 0:
          dacInPack = 0xdf0000;
          break;
      case 1:
          dacInPack = 0x1f0000;
          break;
      case 2:
          dacInPack = 0x5f0000;
          break;
      case 3:
          dacInPack = 0x9f0000;
          break;
      }
//      DPRINT3( 1,"cs=%d dacInPack=%x  value = %d\n",cs,dacInPack,token2);
      tmp = ( (-token2 & 0xFFFF)- 0x8000) & 0xFFFF;
      readValue = (unsigned int)((cs<<24) | dacInPack | tmp);
      DPRINT1(1,"outValue=%x\n",readValue);
      if (fifoFlag)
      {
         /* 25 usec/shim, this is hardcoded in psg's	   */
         /* MasterController::loadshim() 		   */
         /* if you change it here, change it there as well */
         /* 24 usec for 24 bits, 1 usec to load            */
         fWord[0] = (DURATIONKEY | shimDelay);
         // use (port << 28), but port = 0 here
         fWord[1] = (LATCHKEY    | SPIKEY | readValue);
         writeCntrlFifoBuf(fWord,2); 
      }
      else
         readValue = hsspi(0, readValue );
//      DPRINT1( 1,"hsspi read: %x\n",readValue);
   }

/* TOMY special shims */
double z2_a  =  1.0;
double z2_b1 = -1.0;
double z2_b2 = -1.0;
double z2_b3 =  1.0;
double z3_a =   1.0;
double z3_b =  -1.0;
double zx_a =   1.0;
double zx_b =  -1.0;
double zy_a =   1.0;
double zy_b =  -1.0;

void spi1ShimInterface(int *paramvec, int *index, int count, int fifoFlag)
{
   int token1, token2, ttoken;
   double ftoken;
   // skip paramvec[0], it is SHIMDAC (13) 
   if (shimDelay < 2000) shimDelay=2000;
   while (*index < count)
   { 
      (*index)++;     // skip over the SHIMDAC token
      token1 = paramvec[*index]; (*index)++;
      token2 = paramvec[*index]; (*index)++;
      DPRINT3( -3,"token1=%d, token2=%d index=%d\n",token1,token2,*index);
      pCurrentStatBlock->AcqShimValues[token1] = token2;

      if (XYZgradflag)				// gradient amps present
      {  if ((token1==16) || (token1==17) || (token1==2))// x, y or z1?
         { if (fifoFlag == 0)
           DPRINT2(-3,"send2Grad (1) %d %d\n",token1,token2);
               send2Grad(13,token1,token2);	// let gradient cntrl do it
           return;
         }
      }
      ftoken = (double) token2;
      DPRINT3(-3,"t:ftoken = %f token2 = %d,%x\n",ftoken,token2,token2);
      switch (token1) 
      { 
         case 7: case 8: case 9: case 10: case 28: case 29: break;
         case 4:   //  Z2 (4) drives z5,z6,z7 VERIFIED 
             ttoken = (int) (z2_a*ftoken);
             DPRINT1(-3,"z2 a value %x\n",ttoken);
             sendSpiShim(token1,ttoken,fifoFlag);
             ttoken = (int) (z2_b2*ftoken);
             DPRINT1(-3,"z2 b2 value %x\n",ttoken);
             sendSpiShim(9,ttoken,fifoFlag);
             ttoken = (int) (z2_b1*ftoken);   
             DPRINT1(-3,"z2 b1 value %x\n",ttoken);
             sendSpiShim(8,ttoken,fifoFlag);
             ttoken = (int) (z2_b3*ftoken);
             DPRINT1(-3,"z2 b3 value %x\n",ttoken);
             sendSpiShim(10,ttoken,fifoFlag);
             break;
         case 6: 
             ttoken = (int) (z3_a*ftoken);
             sendSpiShim(token1,ttoken,fifoFlag);
             ttoken = (int) (z3_b*ftoken);
             sendSpiShim(7,ttoken,fifoFlag);
             break; 
         case 18:  
             ttoken = (int) (zx_a*ftoken);
             sendSpiShim(token1,ttoken,fifoFlag);
             ttoken = (int) (zx_b*ftoken);
             sendSpiShim(28,ttoken,fifoFlag);
             break; 
         case 21:  
             ttoken = (int) (zy_a*ftoken);
             sendSpiShim(token1,ttoken,fifoFlag);
             ttoken = (int) (zy_b*ftoken);
             sendSpiShim(29,ttoken,fifoFlag);
             break;
         default: 
             sendSpiShim(token1,token2,fifoFlag);
      }
// weird map!!! 
   }
}

int input_record[MAXSHIMS];

void spi2ShimInterface(int *paramvec, int *index, int count, int fifoFlag)
{
   int token1, token2, m1,m2,val1;
   int ttoken1, ttoken2;
   int priorvalue;
   char hstr[24];
   strcpy(hstr,"bogus");
   if (shimDelay < 2000) shimDelay=2000;
   while (*index < count)
   { 
      (*index)++;     // skip over the SHIMDAC token
      token1 = paramvec[*index]; (*index)++;
      token2 = paramvec[*index]; (*index)++;
      priorvalue = input_record[token1]; // save for clip test
      // DPRINT2(-5,"priorvalue at entry = %d,%d\n",priorvalue,input_record[token1]);
      if (XYZgradflag)				// gradient amps present
      {  if ((token1==16) || (token1==17) || (token1==2))// x, y or z1?
         { if (fifoFlag == 0)
           DPRINT2(-3,"send2Grad (1) %d %d\n",token1,token2);
               send2Grad(13,token1,token2);	// let gradient cntrl do it
           return;
         }
      }
      // token1 is shim coil id and token 2 the value
      input_record[token1] = token2;  // keep the value
      switch (token1) 
      { 
         case 16: case 18:   // pair 1 - X/XZ
             m1 = 16;  m2 = 18;
             strcpy(hstr,"x/xz");
         break; 
         case 17: case 21:  // pair 2 Y/YZ
             m1 = 17; m2 = 21;
             strcpy(hstr,"y/yz");
         break; 
         case 20: case 25:  // pair 3 c2 zc2 
             m1 = 20; m2 = 25;
             strcpy(hstr,"c2/zc2 [x2-y2,zx2-y2]");
         break; 
         case 19: case 27:  // pair 4 s2 zs2 
             m1 = 19; m2 = 27;
             strcpy(hstr,"s2/zs2 [xy,zxy]");
         break; 
         case 22: case 12:  // pair 5 c3 zc3 
             m1 = 22; m2 = 12;
             strcpy(hstr,"c3/zc3 [x3,zx3]");
         break; 
         case 23: case 13:  // pair 6 s3 zs3  
             m1 = 23; m2= 13;
             strcpy(hstr,"s3/zs3 [y3,zy3]");
         break; 
         default:
             m1 = 0;
      }
      /* if (m1 == 0)
      DPRINT2(-5,"non paired shim = %d,  value = %d\n",token1,token2); */
      if (m1 == 0)
      {
          pCurrentStatBlock->AcqShimValues[token1] = token2;
          sendSpiShim(token1,token2,fifoFlag);
      }
      else
      {   // since both are updated no decisions..
          ttoken1 = input_record[m1] + input_record[m2];
          ttoken2 = input_record[m1] - input_record[m2];
          if ((ttoken1 > 32767) || (ttoken1 < -32767) || (ttoken2 > 32767) || (ttoken2 < -32767))
          {
             pCurrentStatBlock->AcqShimValues[token1] = priorvalue;
             input_record[token1] = priorvalue;
             errLogRet(LOGIT,debugInfo,"Shim DAC pair at limit\n");
             //DPRINT2(-3,"index = %d reset to priorvalue = %d\n",token1,priorvalue);
             return;  /* don't set these values*/ 
             /*return(HDWAREERROR+SHIMDACLIM); /* nothing handles this error */ 
          }
          DPRINT5(-2,"\n   Shim pair %s: %d <= %d , %d <= %d",hstr,m1,ttoken1,m2,ttoken2);
          pCurrentStatBlock->AcqShimValues[token1] = token2;
          sendSpiShim(m1,ttoken1,fifoFlag);
          sendSpiShim(m2,ttoken2,fifoFlag);
      }
   }
}
/*********************************************************/
/* interface to spi shims                                */
/* two special variants exist for 18,19 that repurpose   */
/* DAC's						 */
/*********************************************************/

void spiShimInterface(int *paramvec, int *index, int count, int fifoFlag)
{
int i, cs, dacNo, dacInPack, readValue;
long fWord[10];
unsigned int	tmp;
unsigned int    token1, token2;
   // skip paramvec[0], it is SHIMDAC (13) 
   if (shimDelay < 2000) shimDelay=2000;
   while (*index < count)
   { 
      (*index)++;     // skip over the SHIMDAC token
      token1 = paramvec[*index]; (*index)++;
      token2 = paramvec[*index]; (*index)++;
      DPRINT3( 1,"token1=%d, token2=%d index=%d\n",token1,token2,*index);
      pCurrentStatBlock->AcqShimValues[token1] = token2;

      if (XYZgradflag)				// gradient amps present
      {  if ((token1==16) || (token1==17) || (token1==2))// x, y or z1?
         { if (fifoFlag == 0)
//           DPRINT2(-1,"send2Grad %d %d\n",token1,token2);
               send2Grad(13,token1,token2);	// let gradient cntrl do it
           return;
         }
      }

      dacNo = shimNumberToSpi[ token1 ];
      if (dacNo == -1) continue;
      cs = (dacNo-1)/4;
      switch (dacNo%4)
      {
      case 0:
          dacInPack = 0xdf0000;
          break;
      case 1:
          dacInPack = 0x1f0000;
          break;
      case 2:
          dacInPack = 0x5f0000;
          break;
      case 3:
          dacInPack = 0x9f0000;
          break;
      }
//      DPRINT3( 1,"cs=%d dacInPack=%x  value = %d\n",cs,dacInPack,token2);
      tmp = ( (-token2 & 0xFFFF)- 0x8000) & 0xFFFF;
      readValue = (unsigned int)((cs<<24) | dacInPack | tmp);
      DPRINT1(1,"outValue=%x\n",readValue);
      if (fifoFlag)
      {
         /* 25 usec/shim, this is hardcoded in psg's	   */
         /* MasterController::loadshim() 		   */
         /* if you change it here, change it there as well */
         /* 24 usec for 24 bits, 1 usec to load            */
         fWord[0] = (DURATIONKEY | shimDelay);
         // use (port << 28), but port = 0 here
         fWord[1] = (LATCHKEY    | SPIKEY | readValue);
         writeCntrlFifoBuf(fWord,2); 
      }
      else
         readValue = hsspi(0, readValue );
//      DPRINT1( 1,"hsspi read: %x\n",readValue);
   }
}

/*********************************************************/
/* clear all shims to zero, used at bootup to prevent    */
/* random wiggling of CS lines to rail shims  during     */
/* bootup and reset                                      */
/*********************************************************/
clear_all_shims()
{
int i,j;
   if (shimType == NO_SHIMS)
   {
      i = determineShimType();
      establishShimType(i);   // also sets SPI bits
      /*  If it doesn't work, we'll find out in good time... */
   }

   switch (shimType)
   {
      case NO_SHIMS:
            errLogRet( LOGIT, debugInfo,
                     "shim handler cannot establish type of shims\n");
            break;

      case QSPI_SHIMS:	// 14 channel shims (16 total) 
            for (i=3; i< 7; i++)
               for (j=0; j<4; j++)
                  hsspi(0, ((i<24) | (j<14) | 2028) );
            break;

      case RRI_SHIMS:
//            rriShimInterface( paramvec, index, count, fifoFlag);
            break;

      case SPI_SHIMS:	// 40 shims
             for (i=0; i< 10; i++)
                hsspi(0, ((i<<24) | 0x208000) ); // zero 4 in package
            break;

      default:
            errLogRet(LOGIT,debugInfo,"unknown shims set %d\n",shimType);
            return;
    } 
}


/*********************************************************
 establish type of shims present on this system
 AS SPECIFIED BY THE HOST COMPUTER WITH SETHW
 -  OVERRIDES THE SHIM TYPE DETERMINED LOCALLY
*********************************************************/
int
establishShimType(int shimset)
{
    shimSet = shimset;
    DPRINT1(-1,"establishShimType: shimset %d\n",shimset);
    switch (shimset) 
    {
	  case 1:	/* Varian 13 obsolete */
	  case 10:	/* Varian 14 */
//                if (shimType != QSPI_SHIMS)
//                   return(0);
	        shimType = QSPI_SHIMS;
		qspi_dac_addr = qspi_dac_table;
		break;

	  case 2:	/* Oxford 18 obsolete */
	  case 3:	/* Varian 23 */
	  case 4:	/* Varian 28 */
	  case 6:	/* Varian 18 */
	  case 7:	/* Varian 21 */
	  case 9:	/* Varian 40 */
          case 12:      /* Varian 26 */
          case 13:      /* Varian 29 */
          case 14:      /* Varian 35 */
          case 15:      /* Varian 15 */
          case 17:      /* Varian 27 */
          case 20:      /* Agilent 32 */
          case 21:      /* Agilent 24 */
          case 22:      /* Oxford  28 */
          case 24:      /* PC+     27 */
          case 25:      /* PC+     28 */
          case 26:      /* Agilent 32 (28 shims) */
          case 27:      /* Agilent 40 (28 shims) */


		shimType = SPI_SHIMS;
		break;

	  case 5:	/* Ultra 39 */
	  case 16:	/* Ultra 18 */

		shimType = RRI_SHIMS;

		break;

	  case 8:	/* Oxord 15 obsolete */
	        shimType = QSPI_SHIMS;
		qspi_dac_addr = imgqspi_dac_table;
		break;

	  case 11:	/* Whole Body Systems */
		/* shimType is already set */
		break;

          case 18:      /* COMBO */ 
	        shimType = SPI_M_SHIMS;
		break;
          case 19:      /* Thin Shims (51mm) */ 
          case 23:      /* Thin Shims (54mm) */
	        shimType = SPI_THIN_SHIMS;
		break;
	  default:
		shimType = NO_SHIMS;
		break;
	}
    DPRINT1(-1,"establishShimType: shimType %d\n",shimType);
    pCurrentStatBlock->AcqShimSet = (short) shimset;
    if (shimType == QSPI_SHIMS)
       setSPIbits(0, ( SPI_RISING_EDGE | SPI_RESTING_HI) );
    else if (shimType == SPI_SHIMS)
        setSPIbits(0, (SPI_24_BITS | SPI_RISING_EDGE | SPI_RESTING_HI) );
    return( 0 );
}

int getShimSet()
{
   return(shimSet);
}

/*------------------------------------------------------------*/
/* Routine to check which shims are if the system, if any     */
/* It first tries to read the 14-channel shim box, followed   */
/* by the 28/40 channel shim box, and finally the RRI box.    */
/*------------------------------------------------------------*/
determineShimType()
{
   shimSet = readEE14();
   if ((shimSet == 1) || (shimSet == 10))   
   {
       shimType = QSPI_SHIMS;
       qspi_dac_addr = qspi_dac_table;
   }
   else if (shimSet == 8)
   {
       shimType = QSPI_SHIMS;
       qspi_dac_addr = imgqspi_dac_table;
   }
   else
   {
      shimSet = readEE40();
      if (shimSet != -1) 
      {
         shimType = SPI_SHIMS;
      }
      else 
      {  // check for RRI
         if ( initRRIShim()== 0 )
         {
            shimType = RRI_SHIMS;
            shimSet  = initGradTest(); // ULTRA or ULTRA18??
         }
         else
         {  // cannot find any
            shimType = NO_SHIMS;
            DPRINT( 1,"determineShimType: cannot find any\n");
         }
      }
   }
   DPRINT2(1,"determineShimType: shimSet=%d, shimType=%d\n",shimSet,shimType);
   return(shimSet);
}


/* read address a using chipselect cs */
readEE1(int a, int cs)
{
int i,j, myCS;
   i=0; j=0;
   myCS = cs << 24;
   while ( (i != 0xf0) && (j<1000)) 
   {
      i = ( hsspi(0,0x050000 | myCS) & 0xff );
      j++;
   }
   if (j>900) return (-1);
   i = hsspi(0,0x030000 | (a<<8) | myCS);
//   printf("EEread %x\n\n",i);
   return(i & 0xFF);
}

/* read the EEprom from the 14-channel shims */
/* if present should return 10 */
readEE14()
{
int i, j;
   setSPIbits(0,0x280);
   i = readEE1(29,0x2);
   j = readEE1(30,0x2);
   if ( (i == -1) || (j == -1) ) 
      i = -1;
   else
      i = (i-0x30)*10 + (j-0x30);
   printf("readEE14 shows %d\n",i);
   return (i);
}

/* read EEprom from the TimShimPlus shims       */
/* if present should return either 4(=28 shims) */
/* or 9(=40 shims) */
readEE40()
{
int i, j;
   setSPIbits(0,0x380);
   i = readEE1(29,0xa);
   j = readEE1(30,0xa);
   if ( (i == -1) || (j == -1) ) 
      i = -1;
   else
      i = (i-0x30)*10 + (j-0x30);
   printf("readEE40 shows %d\n",i);
   return(i);
}

/* ------------------- */
/* Some test routines  */
/* ------------------- */

/* set read and write chipselect values */
static int promR, promW;

void setPromAddr(int r, int w)
{
    promR = r<<24;
    promW = w<<24;
}

void getPromAddr()
{
    printf("promtR = 0x%x = 0x%x = %d\n",promR, promR>>24, promR>>24);
    printf("promtW = 0x%x = 0x%x = %d\n",promW, promW>>24, promW>>24);
}

/* write x, if x=0xaacc then aa is the address, xx is the value */
writeEE(int a, int x)
{
int i;
   i = hsspi(0,0x050000 | promW);
//   printf("writeEE status %x\n", (i & 0xff) );

   hsspi(0,0x060000 | promW);

   i = hsspi(0,0x050000 | promW);
//   printf("writeEE status %x\n", (i & 0xff) );

   hsspi(0,0x020000 | (a << 8) | x | promW);
   hsspi(0,(promW | 0x040000));	// Reset Write Enable
//   printf("writeEE address=%d byte=0x%x (%c)\n",a,x,x);

}

/* read the value at address a */
int readEE(int a)
{
int i,j;
   i=0; j=0;
   while ((j<1000) && ((i&0xff)!=0xf0) ) {
      i = hsspi(0,0x050000 | promR);
//      printf("ReadEE status=%x\n", (i & 0xff) );
      taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
      j++;
   }

   i = hsspi(0,0x030000 | (a<<8) | promR);
//   printf("readEE address=%x byte=%x (%c)\n\n",a,i,i);
   return(i & 0xFF);
}

/* read address 0-80 in the prom, and display, for EXAMPLE:               */
/* SHIM Single QSPI Standard  ? 10 Board=0190229203A, Schem=0190229500A. */
/*     5    0    5    0    5    0    5    0    5    0    5    0    5    0*/
readEEprom()
{
int i,c;
char str[100];
   for (i=0;i<70; i++)
   {   c = readEE(i);
       str[i]=c;
//       printf("%c",c);
   }
   str[69]=0;
   printf("\n");
   printf("Prom has '%s'\n",str);
}

char prom10[]=
  "SHIM Single QSPI Standard    10 Board=0190229203A, Schem=0190229500A.";
char prom40[]=
  "Low Cost SHIM PS 40 Channel  40 Board=0191318700A, Schem=0191319000A.";
char promRRI[]=
  "SHIM Single QSPI Standard     5 Board=0190229203A, Schem=0190229500A.";
/*     5    0    5    0    5    0    5    0    5    0    5    0    5    0*/
                             
writeEEtext(int address, char *str, int shimCount, int thisShimset)
{
int i=0,j;
    printf("Writing...\n");
    setPromAddr(address,address);
    while (*str != (char)NULL) 
    {
        writeEE(i,*str);
//        hsspi(0,(promR | 0x040000));	// Reset Write Enable
	j = readEE1(i,address);
//        printf("writeEEtext read @%2d=0x%x (%c)\n",i,j,j);
        printf(".");
        taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
        i++; str++;
    }
    writeEE(29,thisShimset/10+'0');
    j = readEE1(29,address);
//    printf("writeEEtext read @29=0x%x (%c)\n",j,j);
    printf(".");
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    writeEE(30,thisShimset%10+'0');
    j = readEE1(30,address);
//    printf("writeEEtext read @%d=0x%x (%c)\n",j,j);
    printf(".");
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    if (shimCount) 
    {
       writeEE(17,shimCount/10+'0');
       j = readEE1(29,address);
//       printf("writeEEtext read @29=0x%x (%c)\n",j,j);
       printf(".");
       taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
       writeEE(18,shimCount%10+'0');
       j = readEE1(30,address);
//       printf("writeEEtext read @%d=0x%x (%c)\n",j,j);
       printf(".");
       taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    }
    printf("\n");
}


writeEEprom()
{
int thisShimset, thisShimtype,ready;    
   printf("1 Program 14-channel prom\n");
   printf("2 Program 15-channel prom\n");
   printf("3 Program 18-channel prom\n");
   printf("4 Program 23-channel prom\n");
   printf("5 Program 26-channel prom\n");
   printf("6 Program 28-channel prom\n");
   printf("7 Program 29-channel prom\n");
   printf("8 Program 35-channel prom\n");
   printf("9 Program 40-channel prom\n");
   printf("r Program RRI prom\n");
   printf("x Program OTHER shimset in prom\n");
   printf("0 to exit\n");
  
   printf("\nEnter selection: ");
   thisShimtype = getchar();
   getchar();
   printf("\n\n");
   switch (thisShimtype) {
   case '1':
        setSPIbits(0,0x280);
        writeEEtext(2,  prom10, 14, 10);
        readEEprom();
        setSPIbits(0,0x200);
	break;	
   case '2':
        setSPIbits(0,0x380);
        writeEEtext(10, prom40, 15, 15);
        readEEprom();
        break;
   case '3':
        setSPIbits(0,0x380);
        writeEEtext(10, prom40, 18, 6);
        readEEprom();
        break;
   case '4':
        setSPIbits(0,0x380);
        writeEEtext(10, prom40, 23, 3);
        readEEprom();
        break;
   case '5':
        setSPIbits(0,0x380);
        writeEEtext(10, prom40, 26, 12);
        readEEprom();
        break;
   case '6':
        setSPIbits(0,0x380);
        writeEEtext(10, prom40, 28, 4);
        readEEprom();
        break;
   case '7':
        setSPIbits(0,0x380);
        writeEEtext(10, prom40, 29, 13);
        readEEprom();
        break;
   case '8':
        setSPIbits(0,0x380);
        writeEEtext(10, prom40, 35, 14);
        readEEprom();
        break;
   case '9':
        setSPIbits(0,0x380);
        writeEEtext(10, prom40, 40, 9);
        readEEprom();
        break;
   case 'r':
        setSPIbits(0,0x280);
        writeEEtext(2, prom10, 0, 5);
        readEEprom();
        setSPIbits(0,0x200);
        break;
   case 'x':
        printf("\n\nEnter shimset to be stored: ");
        scanf("%d\n", &thisShimset);
        setSPIbits(0,0x380);
        writeEEtext(10,  prom40, 0, thisShimset);
        readEEprom();
        break;
   default:
        printf("Huh??\n");
        break;
   }
   getchar();
   
}
  
/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/*                                                                       */
/*                          Code for RRI shims                           */
/*                                                                       */
/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/* dac number to RRI command string */
char  *rriShimTable[] = {
         "Fz0" ,        /* 0 = z0f */
         "Cz0" ,        /* 1 = z0c */
         "Fz" ,         /* 2 = z1f */
         "Cz" ,         /* 3 = z1c */
         "Fz2" ,        /* 4 = z2f */
         "Cz2" ,        /* 5 = z2c */
         "Fz3" ,        /* 6 = z3f */
         "Fz4" ,        /* 7 = z4f */
         "Cz5" ,        /* 8 = z5 */
         "Cz6" ,        /* 9 = z6 */
         "Cz7" ,        /* 10 = z7 */
         "Cz8" ,        /* 11 = z8 */  /* new DAC definition */
         "Czc3" ,       /* 12 = zx3 */
         "Czs3" ,       /* 13 = zy3 */
         "Cz4x" ,       /* 14 = z4x */
         "Cz4y" ,       /* 15 = z4y */
         "Cx" ,         /* 16 = x */
         "Cy" ,         /* 17 = y */
         "Czx" ,        /* 18 = xz */
         "Cs2" ,        /* 19 = xy */
         "Cc2" ,        /* 20 = x2-y2 */
         "Czy" ,        /* 21 = yz */
         "Cc3" ,        /* 22 = x3 */
         "Cs3" ,        /* 23 = y3 */
         "Cz2y" ,       /* 24 = yz2 */
         "Czc2" ,       /* 25 = zx2-y2 */
         "Cz2x" ,       /* 26 = xz2 */
         "Czs2" ,       /* 27 = zxy */
         "Cz3x" ,       /* 28 = z3x */
         "Cz3y" ,       /* 29 = z3y */
         "Cz2c2" ,      /* 30 = z2(x2-y2) */
         "Cz2s2" ,      /* 31 = z2xy */
         "Cz3c2" ,      /* 32 = z3x2-y2 */  /* all new DAC defs RRI */
         "Cz3s2" ,      /* 33 = z3xy */
         "Cz2c3" ,      /* 34 = z2x3 */
         "Cz2s3" ,      /* 35 = z2y3 */
         "Cz3c3" ,      /* 36 = z3x3 */
         "Cz3s3" ,      /* 37 = z3y3 */
         "Cz5x" ,       /* 38 = z5x */
         "Cz5y" ,       /* 39 = z5y */
         "Cz4c2" ,      /* 40 = z4x2-y2 */
         "Cz4s2" ,      /* 41 = z4xy */
         "Cz3" ,        /* 42 = z3c */
         "Cz4" ,        /* 43 = z4c */
         NULL
};

#define CMD	0
#define ECHO	1
#define TIMEOUT 98

/* Two types of RRI shims */
#define ULTRA   5
#define ULTRA18 16

#define MAX_RRI_DAC	sizeof( rriShimTable ) / sizeof( rriShimTable[ 0 ] )

static SEM_ID	pShimMutex = NULL;
static int	shimPort = -1;
static char	RRIcmd = 'S';
static int	shimstat = 0;
static int	gradOK[MAX_RRI_DAC];


static testSerialShims( int port )
{
   if (pShimMutex != NULL)
   {
      semTake(pShimMutex,WAIT_FOREVER); /* protect shim serial comm */
   }
   else
   {
      errLogRet(LOGIT,debugInfo,"testSerialShims: Mutex Pointer NULL\n");
      return(-1);
   }


   clearport( port );
   /* printf("testSerialShims: put B, wait for responce\n"); */
   pputchr(port, 'B');
   if (cmdecho(port, CMD))
   {  /* if command times out, try again */
      pputchr( port, 'B');    /* RRI peripheral responds on the 2nd try */
      /* printf("testSerialShims: again put B, wait for responce\n"); */
      if (cmdecho(port, CMD))    /* even though RRI calls it an error. */
      {
         semGive(pShimMutex);
         return( TIMEOUT );
      }
   }
   // wait a bit after this test, seem it is necessary for proper operation
   // the problem encountered was the initGradTest would fail to get the proper
   // response from the PC command, causing the wrong shimSet to be selected
   // when called immediately after this test (via initRRIShim()) 
   // not having any docs wrt RRI shim box, this is the best that can be done
   // for the time being.    GMB  5/20/2010
   taskDelay(calcSysClkTicks(16));  // 16 msec delay, the minimum, 1 tick

   semGive(pShimMutex);
   return( 0 );
}


/* RRI is connected to Serial Port 0 of the master */
/* via the MIF board (right most)                  */
/*                                                 */
/* initialize the port and test it                 */
int initRRIShim()
{
int ival;
   if (shimPort > 0) close(shimPort);

   shimPort = open("/TyMaster/0",O_RDWR,0);   /* port 0 on the MIF */
   if (shimPort == ERROR) 
   {  DPRINT(-10,"Error opening /TyMaster/0 (shims)\n");
      return(-1);
   }
   setSerialTimeout(shimPort,25);	/* set timeout to 1/4 sec */

   if (pShimMutex == NULL)
   {
      pShimMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                   SEM_DELETE_SAFE);
   }

   ival = testSerialShims( shimPort );
   printf("RRI shims returned %d, shimPort=%d\n", ival, shimPort);
   if ((ival == -1) || (ival == TIMEOUT))
   {
       close( shimPort );
       shimPort = -1;
       return( -1 );
   }
   return( 0 );
}

/*
 * This routine is called only for RRI shims.  It first decides if
 * the old shimmer software or the new mxserver software is present
 * in the RRI. It does this by testing whether or not the PC (prepare
 * coarse) command is present.  It is not present in old software.
 * If the new software is present, we use the PC command to disable
 * DACs which are not present. Finally, if the z7 dac is not present,
 * we assume the ULTRA18 shims are present.
 */
initGradTest()
{
   int index;
   int stat;

   /* Set all dacs to present */
   for (index = 0; index < sizeof(rriShimTable)/sizeof(rriShimTable[0]);
        index++)
   {
      gradOK[index] = 1;
   }
   if (pShimMutex != NULL)
   {
      semTake(pShimMutex,WAIT_FOREVER); /* protect shim serial comm */
   }
   else
   {
      return(ULTRA);
   }
   clearport(shimPort);
   pputchr(shimPort, 'P');
   cmdecho(shimPort, ECHO);
   pputchr(shimPort, 'C');
   stat = cmddone(shimPort, 100);
   if (stat == 200)
   {
/*
 * The PC command is not recognized. This must be old RRI SHIMMER software
 */
      semGive(pShimMutex);
      return(ULTRA);
   }

   pputchr(shimPort, '\r');

/*
 * At this point an error should occur for the MXSERVER RRI software.
 * There are two cases.
 * 1. MxServer with no error handling should not give an error number
 *    but it will give an error message about "Incorrect number of
 *    arguments".
 * 2. MxServer with error handling should give error 201 ""Incorrect number of
 *    argument".
 */

   stat = cmddone(shimPort, 500);

   semGive(pShimMutex);
   if (stat != 201)
   {
      // without the delay in testSerialShims it would always return here. GMB 5/20/2010
      return(ULTRA);
   }
   RRIcmd = 'P';
   for (index = 0; index < sizeof(rriShimTable)/sizeof(rriShimTable[0]);
        index++)
   {
      setRRIShim(index,0,0);
/*
 * A return value of 202 is given by RRI's mxserver if a gradient is
 * not present
 */
      
      if (shimstat == 202)
      {
         DPRINT1(1,"turn off dac %d\n",index);
         gradOK[index] = 0;
      }
      else if (shimstat != 0)
      {
        DPRINT3(1,"dac %d (%s) status= %d\n",
                index,rriShimTable[index],shimstat);
      }
   }
   RRIcmd = 'S';
   /* test if z7 (dac 10) is present */
   return( (gradOK[10] == 0) ? ULTRA18 : ULTRA);
}

int setRRIShim( int dac, int value, int fifoFlag )
{
   char *chrptr;

   if ( ((dac==16) || (dac==17) || (dac==3)) && (shimSet != 5) )// x, y or z1?
   {
      if (fifoFlag == 0)
         send2Grad(13,dac,value);		// let gradient cntrl do it
   }
   else 
   {
      if ( (dac < 0) || (dac >= MAX_RRI_DAC) || (shimPort < 0) )
      return( -1 );
      if (gradOK[dac] == 0)
      {
         DPRINT1(1,"turned off dac %d\n",dac);
         return(0);
      }
      chrptr = rriShimTable[dac];
      if (chrptr == NULL)
         return( -1 );

      if (pShimMutex != NULL)
      {
         semTake(pShimMutex,WAIT_FOREVER); /* protect shim serial comm */
      }
      else
      {
         errLogRet(LOGIT,debugInfo,"setRRIShim: Mutex Pointer NULL\n");
         return(-1);
      }
      clearport(shimPort);
      pputchr(shimPort, RRIcmd);
      if (cmdecho(shimPort, ECHO)) {
         errLogRet(LOGIT,debugInfo,"set RRI shim failed on the S command\n" );
         semGive(pShimMutex);
         return(TIMEOUT);
      }
   
   /*  First character is C(oarse) or F(ine).  RRI echoes it with an EOM (^C).
       CMD for 2nd argument to cmdecho tells cmdecho to expect EOM.
   */
   
      pputchr(shimPort, *chrptr);
      if (cmdecho(shimPort, CMD)) {
         errLogRet( LOGIT, debugInfo,
                    "set RRI shim failed on the %c command\n", *chrptr );
         semGive(pShimMutex);
         return(TIMEOUT);
      }
      chrptr++;
   
      while (*chrptr != '\0') {
         pputchr(shimPort, *chrptr);
         if (cmdecho(shimPort, ECHO)) {
            errLogRet( LOGIT, debugInfo,
                     "set RRI shim failed on the %c command\n", *chrptr );
            semGive(pShimMutex);
            return(TIMEOUT);
         }
         chrptr++;
      }
   
      pputchr(shimPort, ' ');
      if (cmdecho(shimPort,ECHO)) {
         errLogRet( LOGIT, debugInfo, "set RRI shim failed on the ' '\n" );
         semGive(pShimMutex);
         return(TIMEOUT);
      }
   
      if (echoval(shimPort, value )) {
         errLogRet(LOGIT,debugInfo,"set RRI shim failed setting DAC value\n" );
         semGive(pShimMutex);
         return(TIMEOUT);
      }
      pputchr(shimPort, '\r');             /* NOT <L/F> !!! */
   
      shimstat = cmddone(shimPort, 500);
      semGive(pShimMutex);
   }
   return( 0 );
}


/*  This program attempts to reset the RRI or other serial shims when the
/*  shims are not responding becasue the Shim CPU and the console/MSR CPU
/*  are out of phase in the command sequence.   */
int resetSerialShims()
{
        int     stat;

        pputchr(shimPort, '\n');
        stat = cmddone(shimPort, 500);
        DPRINT1( -1, "cmddone returned %d in reset serial shims\n", stat );

        pputchr(shimPort, '\r');
        stat = cmddone(shimPort, 500);
        DPRINT1( -1, "cmddone returned %d in reset serial shims\n", stat );
}

int readSerialShimType()
{
char    typeSerialShim[ 12 ];
int     ival, shimset;

   memset( &typeSerialShim[ 0 ], 0, sizeof( typeSerialShim ) );

   if (shimPort < 0)
   {
      ival = initRRIShim();
      if (ival < 0 || ival == TIMEOUT)
         return( -1 );
   }

   if (pShimMutex != NULL)
   {
      semTake(pShimMutex,WAIT_FOREVER); /* protect shim serial comm */
   }
   else
   {
      errLogRet(LOGIT,debugInfo,"readSerialShimType: Mutex Pointer NULL\n");
      return(-1);
   }
   shimset = 0;
   clearport( shimPort );
   pputchr( shimPort, 'I');
   if (cmdecho( shimPort, CMD))
   {
      /* No resposnse--must be different type serial shim */
      /* shimset = readOmtSerialShimType(); /* Try OMT shims */
   }
   else
   {
      /* Responded to "I" cmd */
      ival = readreply( shimPort, 300,
                   &typeSerialShim[ 0 ], sizeof( typeSerialShim ) );
      if (ival == 0)
      {
         shimset = atoi( &typeSerialShim[ 0 ] );
      }
      else
         shimset = -1;
   }

   semGive(pShimMutex);
   return( shimset );
}

void rriShimInterface(int *paramvec, int *index, int count, int fifoFlag)
{ 
unsigned int    token1, token2;
   DPRINT1(-1,"rriShimInterface: shimDelay=%d\n",shimDelay);
   if (shimDelay < 24000000) shimDelay=240000;
   while (*index < count)
   { 
      (*index)++;     // skip over the SHIMDAC token
      token1 = paramvec[*index]; (*index)++;
      token2 = paramvec[*index]; (*index)++;
      DPRINT3(1,"token1=%d, token2=%d index=%d\n",token1,token2,*index);
      pCurrentStatBlock->AcqShimValues[token1] = token2;

      setRRIShim(token1, token2, fifoFlag);
   }
}
/*---------------------------------------------------------------*/
/*       shimSHow()                                              */
/*---------------------------------------------------------------*/

shimShow()
{
   printf("shimType=%d (0-NONE 1-QSPI, 3-RRI, 9-SPI)\n",shimType);
   printf("shimSet=%d\n",shimSet);
   printf("XYZgradflag=%d\n",XYZgradflag);
}


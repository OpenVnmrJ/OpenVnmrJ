/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

//=========================================================================
// FILE: DDR_Common.c
//=========================================================================

#include "DDR_Common.h"

//######################  OutPut FIFO ###################################

//=========================================================================
// clearCumDur(): clear cumulative duration counter
//=========================================================================
void clearCumDur()
{
	BIT_OFF(DUR_CLR,DUR_CLR_SET);
	BIT_ON(DUR_CLR,DUR_CLR_SET);
	BIT_OFF(DUR_CLR,DUR_CLR_SET);
}

//=========================================================================
// setSoftwareGates(): set software gates register
//=========================================================================
void setSoftwareGates(int g)
{
	SWGATES=g;
}

//=========================================================================
// getSoftwareGates(): return software gates register
//=========================================================================
int getSoftwareGates()
{
	return SWGATES;
}

//=========================================================================
// setFifoOutputs(): select FIFO outputs
//=========================================================================
void selectFifoOutputs()
{
	OTF_OUT_SEL=OTF_FIFO_OUT;
}

//=========================================================================
// selectSwOutputs(): select Software outputs
//=========================================================================
void selectSwOutputs()
{
	OTF_OUT_SEL=OTF_SOFT_OUT;
}

//=========================================================================
// resetOTF(): reset output FIFO
//=========================================================================
void resetOTF()
{
	OTF_CTL=0;
	OTF_CTL=OTF_RESET; // rising edge resets fifo
	OTF_CTL=0;
	selectFifoOutputs();
	setSoftwareGates(0);
	clearCumDur();
}

//=========================================================================
// softStartOTF(): start output fifo and timer
//=========================================================================
void softStartOTF()
{
	OTF_CTL=0;
	OTF_CTL=OTF_SOFT_START;
}

//=========================================================================
// syncStartOTF(): start output fifo and timer (use sync line)
//=========================================================================
void syncStartOTF()
{
	OTF_CTL=0;
	OTF_CTL=OTF_SYNC_START;
}


//=========================================================================
// stopOTF(): stop output fifo and timer
//=========================================================================
void stopOTF()
{
	OTF_CTL=0;
}

//######################  ADC data FIFO ###################################

//=========================================================================
// setCollectMode(m): set data collection mode
// m=0:  all data collection controlled from software (setCollect)
// m=1:  data collection start using FIFO ADC bit
// m=2:  data collection stop using FIFO ADC bit
// m=3:  data collection start & stop using FIFO ADC bit
//=========================================================================
void setCollectMode(int m)
{
    BIT_OFF(ADF_ARM,(ADC_START|ADC_STOP));
    BIT_ON(ADF_ARM,m&(ADC_START|ADC_STOP));
}
//=========================================================================
// getCollectMode(): return data collection mode
//=========================================================================
int getCollectMode()
{
	return ADF_ARM&(ADC_START|ADC_STOP);
}

//=========================================================================
// setCollect(m): set data collection bit
// m=0:  stop data collection
// m=1:  start data collection
//=========================================================================
void setCollect(int m)
{
     if(m==0)
         BIT_OFF(ADF_ARM,ADC_COLLECT);
     else
         BIT_ON(ADF_ARM,ADC_COLLECT);
}

//=========================================================================
// getADFcount(): return current number of points in the data fifo
//=========================================================================
int getADFcount()
{
	return ADF_CNT;
}

//=========================================================================
// setTripPoint(m): set data fifo trip point
//=========================================================================
void setTripPoint(int m)
{
    m&=TRIP_BITS;

    if(ADF_CTL&CLR_ADF)
        m|=CLR_ADF;
    ADF_CTL=m;     
 }

//=========================================================================
// getTripPoint(): return data fifo trip point
//=========================================================================
int getTripPoint()
{
	return ADF_CTL&TRIP_BITS;
}

//=========================================================================
// setADCDebug(m): set/clear ADC debug mode bit 
//=========================================================================
void setADCDebug(int m)
{
    if(m)
		BIT_ON(ADF_ARM,ADC_DEBUG);
	else
		BIT_OFF(ADF_ARM,ADC_DEBUG);	
}

//=========================================================================
// getADCDebug(): return ADC debug mode bit 
//=========================================================================
int getADCDebug()
{
     return ADF_ARM&ADC_DEBUG?1:0;
}

//=========================================================================
// clearADCFIFO(m): clear and disable data fifo 
//=========================================================================
void clearADCFIFO()
{
	ADF_CTL=ADF_CTL|CLR_ADF;
}

//=========================================================================
// enableADCFIFO(m): enable data fifo 
//=========================================================================
void enableADCFIFO()
{
	ADF_CTL=ADF_CTL&TRIP_BITS;
}

//=========================================================================
// setADFread(): set ADF read pointer
//=========================================================================
void setADFread(int i)
{
	ADF_RPTR=i;
}

//=========================================================================
// getADFread(): return ADF read pointer
//=========================================================================
int getADFread()
{
	return ADF_RPTR;
}

//=========================================================================
// getClockPhase: get 20 MHz clock phase
//=========================================================================
int getClockPhase()
{ 
    int phs=0;
#ifdef DDR_AD6634SyncPhase
    phs=get_field(DDR,ad6634_sync_phase);
#endif       
	return phs&0x3;
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *
 */ 
//=========================================================================
// FILE: DDR_AD6634.c
//=========================================================================

#include "DDR_Common.h"
#define DTOR    0.017453293    // 2*PI/360
//######################  AD6634 ##########################################

static double sampling_rate=0;
static int filter_option=AD_FILTER1;

//#define PHASE_CONTINUOUS // phase continuous freq hops on pinsync
// NCO Control register bits (0x88)
enum {
    DITHER_PHASE  = 0x02,
    DITHER_AMPL   = 0x04,
    CLR_ONHOP     = 0x08,  // clear NCO phase on pin sync
    CLK_ON_IEN    = 0x10   // use IEN to gate input clock        
};

static void set_io_ports();
static void set_chnl_map(int chnl);

//=========================================================================
// resetAD6634: reset the AD6634
//=========================================================================
void resetAD6634()
{
    AD6634_RST=0;
    AD6634_RST=1;
    filter_option=AD_FILTER1;
}

//=========================================================================
// set_option: set AD6634 option
//=========================================================================
void set_filter_option(int i)
{
    filter_option=i&AD_FILTER;
}

//=========================================================================
// set_option: get AD6634 option
//=========================================================================
int get_filter_option()
{
    return filter_option;
}

//=========================================================================
// softStartAD6634: start AD6634 data collection (software start)
//=========================================================================
void softStartAD6634()
{
    WMICRO(PSYNC,0x0F);			// ch 0..3 ON ready for start
	WMICRO(SSYNC,0x1F);			// PIN_SYNC Hop_En + Start_EN + Sync_EN A
}

//=========================================================================
// pinStartAD6634: start AD6634 data collection (use fifo signals)
//=========================================================================
void pinStartAD6634()
{
	WMICRO(SSYNC,0x0F);			// PIN_SYNC Hop_En + Start_EN + Sync_EN A
    WMICRO(PSYNC,0x3F);			// ch 0..3 ON ready for start
}

//=========================================================================
// pinStartAD6634: start AD6634 data collection (use fifo signals)
//=========================================================================
void clearStartAD6634()
{
	WMICRO(SSYNC,0x00);
    WMICRO(PSYNC,0x00);
}

//=========================================================================
// stopAD6634: stop AD6634 data collection
//=========================================================================
void stopAD6634()
{
	WMICRO(SLEEP,0x0F);			// sleep	
}
//=========================================================================
// select_chnl: set AD6634 target channel
//=========================================================================
void select_chnl(int ch)
{
    int acr=0x50|(0x3&ch);
	WMICRO(ACR,acr);			// not auto-incr +not broadcast	
}


//=========================================================================
// get_AD6634_reg: return AD6634 register value (last selected channel)
//=========================================================================
int get_AD6634_reg(int adrs, int bytes)
{
    int val=0;
	WMICRO(CAR,adrs);         	// set address reg
	val=RMICRO(DR0);			// add lsb
	if(bytes>1){
		val+=(RMICRO(DR1))<<8;	// add 2nd byte
		if(bytes>2){
			WMICRO(CAR,adrs+1);   // 2nd word reg
			val+=RMICRO(DR0)<<16; // add 3rd byte
			if(bytes>3)
				val+=RMICRO(DR1)<<24; // add 4th byte
		}
	}
	return val;	    
}

//=========================================================================
// set_AD6634_reg: set AD6634 register (last selected channel)
//=========================================================================
void set_AD6634_reg(int adrs, int bytes, int val)
{
	WMICRO(CAR,adrs);         	// select lw word
	if(bytes>1)
	    WMICRO(DR1,(val>>8)&0xff);
	WMICRO(DR0,val&0xff);	
	if(bytes>2){                // select ms word
		WMICRO(CAR,adrs+1);
		if(bytes>3)
			WMICRO(DR1,(val>>24)&0xff);
		WMICRO(DR0,(val>>16)&0xff);
	}
}

//=========================================================================
// get_AD6634_reg: return AD6634 io port register value
//=========================================================================
int get_AD6634_io_reg(int adrs, int bytes)
{
	int val;
	WMICRO(SLEEP,0x2F);         // access to I/O Ctrl Reg + sleep
	val=get_AD6634_reg(adrs,bytes);
	WMICRO(SLEEP,0x0F);         // return to channel cntrl + sleep
	return val;
}

//=========================================================================
// set_AD6634_reg: set AD6634 io port register value
//=========================================================================
void set_AD6634_io_reg(int adrs, int bytes, int val)
{
	WMICRO(SLEEP,0x2F);         // access to I/O Ctrl Reg + sleep
	set_AD6634_reg(adrs,bytes,val);
	WMICRO(SLEEP,0x0F);         // return to channel cntrl + sleep
}

//=========================================================================
// set_dither: set or clear FPGA DDR-A dither enable
//=========================================================================
void set_dither(int i)
{
    if(i){
    	set_field(DDR,AD6634_DIT_OFF,1);
    }
    else{
     	set_field(DDR,AD6634_DIT_OFF,0);
   }
}

//=========================================================================
// get_dither: return FPGA dither enabled status
//=========================================================================
int get_dither()
{
    int doff=0;
    doff=get_field(DDR,AD6634_DIT_OFF);
    return doff?0:1;
}

//=========================================================================
// set_NCO_dither: set AD6634 NCO dither option
// 0: no dither
// 1: phase dither
// 2: ampl dither
// 3: phase and ampl dither
//=========================================================================
void set_NCO_dither(int i)
{
    int nco=get_AD6634_reg(0x88,2);
    BIT_OFF(nco,DITHER_PHASE|DITHER_AMPL);
	
	switch(i){
	case 0:
	   break;
	case 1:
	   BIT_ON(nco,DITHER_PHASE);
	   break;
	case 2:
	   BIT_ON(nco,DITHER_AMPL);
	   break;
	case 3:
	   BIT_ON(nco,DITHER_PHASE|DITHER_AMPL);
	   break;
	}
	WMICRO(ACR,0x40);			// all channels
	set_AD6634_reg(0x88, 2, nco);
}

//=========================================================================
// get_NCO_dither: return AD6634 NCO dither option
//=========================================================================
int get_NCO_dither()
{
    int nco=get_AD6634_reg(0x88,2);
	return (nco&(DITHER_PHASE|DITHER_AMPL))>>1;
}

//=========================================================================
// set_NCO_phase: set AD6634 NCO phase
//=========================================================================
void set_NCO_phase(double p)
{ 
    unsigned int nco=0x10000*(1.0-p/DTOR);
	WMICRO(ACR,0x40);			// all channels
	set_AD6634_reg(0x87, 2, nco);
}

//=========================================================================
// get_NCO_phase: return AD6634 NCO phase
//=========================================================================
double get_NCO_phase()
{
    unsigned int nco=get_AD6634_reg(0x87,2);
    double p=DTOR*(1.0-(double)nco/0x10000);
	return p;
}

//=========================================================================
// set_NCO_freq: set AD6634 NCO frequency (all channels)
//=========================================================================
void set_NCO_freq(double val)
{
    int  freq=0x40000000+(int)(val*53.6870912);       // 20 MHz
    int b1,b2,b3,b4;
    
    b4=(freq>>24)&0xff;
    b3=(freq>>16)&0xff;
    b2=(freq>>8)&0xff;
    b1=(freq)&0xff;
    
	WMICRO(ACR,0x40);		// all channels     

	WMICRO(CAR,0x85);		// NCO Freq. reg 0
	WMICRO(DR1,b2);			// MSB of  NCO
	WMICRO(DR0,b1);			// ...

	WMICRO(CAR,0x86);		// NCO Freq. reg 0
	WMICRO(DR1,b4);			// LSB of 20 MHz NCO
	WMICRO(DR0,b3);			// ...
}

//=========================================================================
// get_NCO_freq: return AD6634 NCO freq
//=========================================================================
double get_NCO_freq()
{
    unsigned int nco=get_AD6634_reg(0x87,2);
    double p=DTOR*(1.0-(double)nco/0x10000);
	return p;
}
//=========================================================================
// set_IEN_mode: set AD6634 NCO IEN mode
// 0: insert zeros when IEN=0 (default)
// 1: disable input clock when IEN=0 (explicit sampling mode)
//=========================================================================
void set_IEN_mode(int i)
{
    int nco=get_AD6634_reg(0x88,2);
    BIT_OFF(nco,CLK_ON_IEN);
	
	switch(i){
	case 0:
	   break;
	case 1:
	   BIT_ON(nco,CLK_ON_IEN);
	   break;
	}
	WMICRO(ACR,0x40);			// all channels
	set_AD6634_reg(0x88, 2, nco);
}

//=========================================================================
// get_IEN_mode: return AD6634 NCO IEN mode
//=========================================================================
int get_IEN_mode()
{
    int nco=get_AD6634_reg(0x88,2);
	return (nco&(CLK_ON_IEN))>>4;
}

//=========================================================================
// set_NCO_clear: set NCO CLR on HOP mode
// 1: reset NCO accumultor on pinsync (phase non-continuous)
// 0: don't reset NCP accumultor on pinsync (phase continuous)
//=========================================================================
void set_NCO_clear(int i)
{
    int nco=get_AD6634_reg(0x88,2);
    BIT_OFF(nco,CLR_ONHOP);
	
	switch(i){
	case 0:
	   break;
	case 1:
	   BIT_ON(nco,CLR_ONHOP);
	   break;
	}
	WMICRO(ACR,0x40);			// all channels
	set_AD6634_reg(0x88, 2, nco);
}

//=========================================================================
// get_NCO_clear: return NCO CLR on HOP mode
//=========================================================================
int get_NCO_clear()
{
    int nco=get_AD6634_reg(0x88,2);
	return (nco&(CLR_ONHOP))>>3;
}
//=========================================================================
// get_AD6634_freq: return AD6634 NCO frequency (last selected channel)
//=========================================================================
int get_AD6634_freq()
{
	return get_AD6634_reg(0x85,4);
}

//=========================================================================
// set_AD6634_freq: set AD6634 frequency (all channels)
//=========================================================================
void set_AD6634_freq(int val)
{
	WMICRO(ACR,0x40);			// all channels
	set_AD6634_reg(0x85,4,val);
}

//=========================================================================
// set_input_attn: set CIC5 data scale (4=1/16,5=1/32)
//=========================================================================
void set_input_attn(int val)
{
	WMICRO(ACR,0x40);			// all channels
	set_AD6634_reg(0x95,1,val);
}

//=========================================================================
// get_input_attn:  get CIC5 data scale (4=1/16,5=1/32)
//=========================================================================
int get_input_attn()
{
	return get_AD6634_reg(0x95,1);
}

//=========================================================================
// set_output_scale: set output scale (1=*2,2=*4,3=*8) 
//=========================================================================
void set_output_scale(int val)
{
	WMICRO(ACR,0x40);			// all channels
	set_AD6634_reg(0xA4,2,val);
}

//=========================================================================
// get_output_scale:  get output scale
//=========================================================================
int get_output_scale()
{
	return get_AD6634_reg(0xA4,2);
}

//=========================================================================
// get_RCF_phase: return AD6634 phase (last selected channel)
//=========================================================================
int get_RCF_phase()
{
	return get_AD6634_reg(0xA1,1);
}

//=========================================================================
// set_RCF_phase: set AD6634 RCF phase (all channels)
//=========================================================================
void set_RCF_phase(int rcfdec)
{
    int i,step=rcfdec/4;
    for(i=0;i<4;i++){
    	select_chnl(i);
    	set_AD6634_reg(0xA1,1,i*step);
    }
}

//=========================================================================
// load_coeffs: load filter coefficient table (all channels)
//=========================================================================
void load_coeffs(int *table, int size)
{
    int i,val;
    unsigned char byte1,byte2,byte3;
 
 	WMICRO(SLEEP,0x0F);			// channel memory map + sleep	
	WMICRO(ACR,0xC0);			// auto-incr + broadcast + all channels
	WMICRO(CAR,0x00);			// point to coeff memory
	for(i=0;i<size;i++){
	    val=table[i];
	    byte3=(val>>16)&0x0f;
	    WMICRO(DR2,byte3);
	    byte2=(val>>8)&0xff;
	    WMICRO(DR1,byte2);
	    byte1=val&0xff;
	    WMICRO(DR0,byte1);
	}
	WMICRO(ACR,0x40);			// no auto-incr + broadcast + all channels
}

//=========================================================================
// get_coeffs: return in array loaded filter coefficient table
//=========================================================================
int get_coeffs(int *table)
{
	int i,val,size;
    unsigned char byte1,byte2,byte3;
  	
	WMICRO(SLEEP,0x00);			// channel 0 memory map
 	WMICRO(CAR,0xA2);			// RCF Taps
	size=RMICRO(DR0)+1;
	WMICRO(ACR,0xC0);			// auto-incr + broadcast + all channels
	WMICRO(CAR,0x00);			// point to coeff memory
	for(i=0;i<size;i++){
	    val=0;
	    byte1=RMICRO(DR0);
	    byte1&=0xff;
	    byte2=RMICRO(DR1);
	    byte2&=0xff;
	    byte3=RMICRO(DR2);
	    byte3&=0x0f;
	    val=(byte1)|(byte2<<8)|(byte3<<16);
	    if(byte3&0x08)
	        val|=0xfff00000;
	    table[i]=val;
	}
	WMICRO(ACR,0x40);			// no auto-incr + broadcast + all channels
 	WMICRO(SLEEP,0x0F);			// channel memory map + sleep
 	return size;
}

//=========================================================================
// set10M: set 10 MHz data collection rate
//=========================================================================
void set10M()
{
	static int taps[]={ 
		-377,	-494,	516,	2354,	1968,	-2750,	-7518,	-3437,
		10370,	18299,	2022,	-31102,	-40814,	10597,	110376,	192133,
		192133,	110376,	10597,	-40814,	-31102,	2022,	18299,	10370,
		-3437,	-7518,	-2750,	1968,	2354,	516,	-494,	-377
    };
    int ntaps=sizeof(taps)/sizeof(int);
    sampling_rate=10;
    
	WMICRO(SLEEP,0x0F);			// sleep	
	WMICRO(ACR,0x40);			// all channels

	WMICRO(CAR,0x90);			// MrCIC2 = 1
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x91);			// LrCIC2 = 1
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x94);			// CIC5 Decim M 
	WMICRO(DR0,0x03);			// MCIC5 = 4

	WMICRO(CAR,0xA0);			// RCF Decim M 
	WMICRO(DR0,0x07);			// MRCF = 8
	
	WMICRO(CAR,0xA2);			// RCF Taps = 32 
	WMICRO(DR0,ntaps-1);	    //  ...
   
    load_coeffs(taps,ntaps);    
    set_RCF_phase(8);  
}

//=========================================================================
// set5M: set 5 MHz data collection rate
//=========================================================================
void set5M()
{
	static int taps32[] = {
	-499, -737, 326, 2569, 2526, -2508, -8221, -4573,
	10289, 19957, 3816, -31776, -44346, 7326, 111086, 196909,
	196909, 111086, 7326, -44346, -31776, 3816, 19957, 10289,
    -4573, -8221, -2508, 2526, 2569, 326, -737, -499,
    };
	static int taps64[]={ 
	-4,-102,-130,-50,231,438,242,-443,
	-1047,-740,681,2096,1770,-824,-3722,-3656,
	646,6065,6868,247,-9301,-12209,-2596,13811,
	21520,8248,-20775,-41089,-25045,34886,114741,171388,
	171388,114741,34886,-25045,-41089,-20775,8248,21520,
	13811,-2596,-12209,-9301,247,6868,6065,646,
	-3656,-3722,-824,1770,2096,681,-740,-1047,
	-443,242,438,231,-50,-130,-102,-4
    };
    int ntaps;
    int *taps;
    switch(filter_option){
    case AD_FILTER1:
        ntaps=sizeof(taps32)/sizeof(int);
        taps=taps32;
        break;
    default:
    case AD_FILTER2:
        ntaps=sizeof(taps64)/sizeof(int);
        taps=taps64;
        break;
    }

    sampling_rate=5;
    
	WMICRO(SLEEP,0x0F);			// sleep	
	WMICRO(ACR,0x40);			// all channels

	WMICRO(CAR,0x90);			// MrCIC2 = 1
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x91);			// LrCIC2 = 1
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x94);			// CIC5 Decim M 
	WMICRO(DR0,0x03);			// MCIC5 = 4

	WMICRO(CAR,0xA0);			// RCF Decim M 
	WMICRO(DR0,0x0f);			// MRCF = 16
	
	WMICRO(CAR,0xA2);			// RCF Taps = 64 
	WMICRO(DR0,ntaps-1);	    //  ...
   
    load_coeffs(taps,ntaps);
    
    set_RCF_phase(16);
}

//=========================================================================
// set2500K: set 2.5 MHz data collection rate
//=========================================================================
void set2500K()
{
	static int taps32[] = {
	-186, -633, -1047, -765, 1042, 4022, 6169, 4162,
	-3855, -15424, -22614, -15289, 12581, 57029, 103564, 133387,
	133387, 103564, 57029, 12581, -15289, -22614, -15424, -3855,
	4162, 6169, 4022, 1042, -765, -1047, -633, -186,
	};
	static int taps64[] = {
	19, 22, 18, -34, -123, -223, -248, -110,
	237, 706, 1070, 1008, 284, -1058, -2557, -3409,
	-2780, -295, 3541, 7233, 8706, 6198, -687, -10241,
	-18693, -21227, -13734, 5334, 33794, 65859, 93686, 109847,
	109847, 93686, 65859, 33794, 5334, -13734, -21227, -18693,
	-10241, -687, 6198, 8706, 7233, 3541, -295, -2780,
	-3409, -2557, -1058, 284, 1008, 1070, 706, 237,
	-110, -248, -223, -123, -34, 18, 22, 19,
	};
    int ntaps;
    int *taps;
    switch(filter_option){
    case AD_FILTER1:
        ntaps=sizeof(taps32)/sizeof(int);
        taps=taps32;
        break;
    case AD_FILTER2:
    default:
        ntaps=sizeof(taps64)/sizeof(int);
        taps=taps64;
        break;
    }
    
	WMICRO(SLEEP,0x0F);			// sleep	
	WMICRO(ACR,0x40);			// all channels

	WMICRO(CAR,0x90);			// MrCIC2 = 1
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x91);			// LrCIC2 = 1
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x94);			// CIC5 Decim M 
	WMICRO(DR0,0x03);			// MCIC5 = 4

	WMICRO(CAR,0xA0);			// RCF Decim M 
	WMICRO(DR0,31);			    // MRCF
	
	WMICRO(CAR,0xA2);			// RCF Taps = 128 
	WMICRO(DR0,ntaps-1);	    //  ...
   
    load_coeffs(taps,ntaps);   
    set_RCF_phase(32);
    sampling_rate=2.5;
}

//=========================================================================
// samplingRate: set ADC fifo sampling rate (Hz)
//=========================================================================
void setSamplingRate(double sa)
{
    if(sa>=10.0)
    	set10M();
    else if(sa>=5.0)
        set5M();
    else if(sa>=2.5)
        set2500K(); 
}

//=========================================================================
// getSamplingRate: return current ADC fifo sampling rate (Hz)
//=========================================================================
double getSamplingRate()
{
	return sampling_rate;   
}

//=========================================================================
// initAD6634: initialize AD6634 registers and memory
//=========================================================================
void initAD6634()
{   
    int i;
    resetAD6634();
    set_io_ports();
    for(i=0;i<4;i++)
        set_chnl_map(i);
    setSamplingRate(5.0);
}

//=========================================================================
// set_io_ports: configure AD6634 I/O registers
//=========================================================================
static void set_io_ports()
{
	// set input port()
	WMICRO(ACR,0x00);			// not auto-inc + not broadcast
	WMICRO(SLEEP,0x2F);			// access to I/O Ctrl Regc + sleep
	
	WMICRO(CAR,0x00);			// Lower Threshold A (not used) 
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x01);			// Upper Threshold A (not used) 
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x02);			// Dwell Time A (not used) 
	WMICRO(DR2,0x00);			// ...
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x03);			// gain range A Ctrl 
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x04);			// Lower Threshold B (not used) 
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x05);			// Upper Threshold B (not used) 
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x06);			// Dwell Time B (not used) 
	WMICRO(DR2,0x00);			// ...
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x07);			// gain range B Ctrl (not used)
	WMICRO(DR0,0x00);			// ...

	// set output port()
	
	WMICRO(CAR,0x08);			// all 4 chnls interleaving+bypass HB
	WMICRO(DR0,0x0F);			// ...
	
	WMICRO(CAR,0x09);			// port B disabled + bypss HB 
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x0A);			// AGC A Ctrl
	WMICRO(DR0,0x01);			// bypass AGC
	 
	WMICRO(CAR,0x0B);			// AGC A hold-off (not used)
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x0C);			// AGC A desired lvl (not used)
	WMICRO(DR0,0x01);			// ...
	
	WMICRO(CAR,0x0D);			// AGC A signal gain (not used) 
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x0E);			// AGC A loop gain (not used) 
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x0F);			// AGC A pole loc (not used) 
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x10);			// AGC A avrg. sample (not used) 
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x11);			// AGC A update dec. (not used) 
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x12);			// AGC B Ctrl (not used)
	WMICRO(DR0,0x01);			// ...
	 
	WMICRO(CAR,0x13);			// AGC B hold-off (not used)
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x14);			// AGC B desired lvl (not used)
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x15);			// AGC B signal gain (not used) 
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x16);			// AGC B loop gain (not used) 
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x17);			// AGC B pole loc (not used) 
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x18);			// AGC B avrg. sample (not used) 
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0x19);			// AGC B update dec. (not used) 
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x1A);			// parallel A cntrl 
	WMICRO(DR0,0x03);			// 16 bit interleaved

	WMICRO(CAR,0x1B);			// link A control 
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x1C);			//  parallel B cntrl (not used) 
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x1D);			// parallel B cntrl (not used)
	WMICRO(DR0,0x0);			// ...

	WMICRO(CAR,0x1E);			// PCLK slave
	WMICRO(DR0,0x02);			// ...

	WMICRO(SLEEP,0x0F);			// select channel memory map
}

//=========================================================================
// set_chnl_map: set channel memory map
//=========================================================================
static void set_chnl_map(int chnl)
{
	 WMICRO(SLEEP,0x0F);			// channel memory map + sleep
	
	// set channel select register

	select_chnl(chnl);
	
	WMICRO(CAR,0x81);			// Soft Sync: Hop sync:off + start sync: on
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x82);			// Pin Sync: First sync only:off Hop-Ena:on
	WMICRO(DR0,0x03);			// + Start-Ena:on + Hop-Ena:on

	WMICRO(CAR,0x83);		    // Start hold-off control
	WMICRO(DR1,0x00);			// NOTE: anything but "1" fails to sigave
	WMICRO(DR0,0x01);		    // all start holdoffs the same
  
	WMICRO(CAR,0x84);			// NCO freq. hold-off control
	WMICRO(DR1,0x00);			// NOTE: anything but "1" fails to sigave
	WMICRO(DR0,0x01);		    // all hop holdoffs the same

	WMICRO(CAR,0x85);			// NCO Freq. reg 0
	WMICRO(DR1,0x00);			// LSB of 20 MHz NCO
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x86);			// NCO Freq. reg 0
	WMICRO(DR1,0x40);			// MSB of 20 MHz NCO
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x87);			// NCO phase offset
	WMICRO(DR1,0x00);		    // ...    
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x88);			// NCO control register
	WMICRO(DR1,0x00);			// sync phase-continuous (reset NCO)
#ifdef PHASE_CONTINUOUS
	WMICRO(DR0,DITHER_PHASE);			// phase dither - phase continuous
#else
	WMICRO(DR0,CLR_ONHOP|DITHER_PHASE); // phase dither  only
#endif
	// CAR 0x90			MrCIC2 = 1  (set in set5M etc.)
	// CAR 0x91			LrCIC2 = 1  (set in set5M etc.)

	WMICRO(CAR,0x92);			// CIC2 Scale
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0x93);			// Reserved (must be written low)
	WMICRO(DR0,0x00);			// ...

	// CAR 0x94			CIC5 Decim M (set in set5M etc.)

	WMICRO(CAR,0x95);			// CIC5 Scale
	WMICRO(DR0,0x04);			// ...

	WMICRO(CAR,0x96);			// Reserved (must be written low)
	WMICRO(DR0,0x00);			// ...

	// CAR 0xA0   		RCF Decim (set in set5M etc.)
	
	WMICRO(CAR,0xA1);			// RCF Decim Phase 
	WMICRO(DR0,0x00);			// PRCF = 0 for channel 0

	// CAR 0xA2   		RCF number of taps (set in set5M etc.)
	
	WMICRO(CAR,0xA3);			// RCF Coeff. Offset 
	WMICRO(DR0,0x00);			// CORCF=0

	WMICRO(CAR,0xA4);			// point to RCF control reg
	WMICRO(DR1,0x00);			// set ram bank 0 fixed pt + scale
	//WMICRO(DR0,0x01);			//  1= +- 8192
	WMICRO(DR0,0x03);			//  3= +- 32k
	
	WMICRO(CAR,0xA5);			// BIST I 
	WMICRO(DR1,0x00);			// (not used for normal mode)
	WMICRO(DR0,0x00);			// ...
	
	WMICRO(CAR,0xA6);			// BIST Q 
	WMICRO(DR1,0x00);			// (not used for normal mode)
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0xA7);			// number of BIST outputs
	WMICRO(DR2,0x00);			// (not used for normal mode)
	WMICRO(DR1,0x00);			// ...
	WMICRO(DR0,0x00);			// ...

	WMICRO(CAR,0xA8);			// RAM BIST Control 
	WMICRO(DR0,0x00);			// (not used for normal mode)

	WMICRO(CAR,0xA9);			// Output Control 
	WMICRO(DR1,0x01);			// map RCF data to BIST reg
	WMICRO(DR0,0x20);			// ... 16 bit fixed point output
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
// Synthesizer Class...
// notes for those who cherish the odd
// pts 320 subtract 10.0 MHz always.
// pts 160 convert top 2 bcd digits to binary.
//
#ifdef __cplusplus
#include <iostream>
#endif /* __cplusplus */

#include <string.h>
#include <stdio.h>
#include "ACode32.h"     /* for union64 */
#include "Synthesizer.h"

#ifdef VXWORKS
#include <string.h>
#include <stdioLib.h>
#include <stdlib.h>
#include <vxWorks.h>
 
#include "logMsgLib.h"
#include "fifoFuncs.h"
#include "FFKEYS.h"
#include "rf.h"
#include "rf_fifo.h"
#include "nvhardware.h"

#else
#ifdef STANDALONE
//#define set_field(fpga, reg, v) printf("set_field(%s, %s, %x)\n", #fpga, #reg, v)
  #define set_field(fpga, reg, v)
  #define taskDelay(x)
#endif 

#endif /* VXWORKS */

#ifdef STANDALONE
int bgflag = 0;
#else
extern int bgflag;
#endif

#ifndef __cplusplus
static SynthesizerType *type = NULL;
#endif

/* Tuning words and switch settings are calculeted for the VFS and DJ
   synthesizers according to parameters set for various frequency
   bands. The bands and the parameters associated with them are stored
   in a breakpoint table. The tables are defined below.

   There are functions for computing the RF switch settings and the
   tuning words for both DDS chips in the synthesizer. These functions
   are pased a frequency and a breakpoint table, and they return the
   appropriate switch setting or tuning word. These functions have the
   _base suffix. The API exposes two functions for each _base
   function. These select the appropriate breakpoint table for the
   type of synthesizer, VFS or DJ. The VFS functions have the dds_
   prefix and the DJ functions use the dds2_ prefix.
 */

struct breakpoint 
{
  double   freq;	/* Breakpoint applies to frequency less than or equal to this frequency */
  double   freq_adjust; /* Adjust frequency before scaling */
  double   freq_scale;	/* Scale frequency by this amount during folding. Usually 1 or -1. */
  double   freq_offset;	/* Add this amount to the scaled frequency. */
  int      ftw2_adjust;	/* After computing TW2, add this to the TW2 value. */
  unsigned rfswitch;
};

static struct breakpoint var400_breakpoints[] =
{
  {1e10, -(135.0+270.0), 0x100000000000LL/(16*80000000.0), 0.0, 0x101},
};

static struct breakpoint pts320_breakpoints[] =
{
  {1e10, -10.0, 10000000.0L, +0.5, 0x101},
};

static struct breakpoint pts160_breakpoints[] =
{
  {1e10, +10.0, 10000000.0L, +0.5, 0x101},
};

static struct breakpoint pts_breakpoints[] =
{
  {1e10, 0, 10000000.0L, +0.5, 0x101},
};

/* Breakpoints for the VFS synthesizer. */
static struct breakpoint dds_breakpoints[] = 
{
  {325e6,   0,  1.0, 0.0,   0, 0x00aaaaaa},
  {1e10,    0, -1.0, 720e6, 0, 0x03555555},
};

/* Breakpoints for the DJ synthesizer Note: These values do not work
   above 500 MHz. */
static struct breakpoint vnmrj_dds2_breakpoints[] = 
{
  {310e6,   0,  1.0,    0e6,      0, 0x069595},
  {400e6,   0, -1.0,  480e6,      0, 0x156aa6},
  {420.2e6, 0, -1.0,  720e6,      0, 0x19a95a},
  {500e6,   0, -1.0,  720e6, -0x100, 0x19a95a},
  {610e6,   0, -1.0,  720e6,      0, 0x19a95a},
  {780e6,   0,  1.0,  480e6,      0, 0x19a9a6},
  {1e10,    0,  1.0,  720e6,      0, 0x16665a},
};
  
/* Eric's preferred breakpoints for DJ.  Tested to work up to 800MHz. */
static struct breakpoint amrs_breakpoints[] = 
{
  {310e6,   0,  1.0,    0e6,      0, 0x069595},
  {400e6,   0, -1.0,  480e6,      0, 0x156aa6},
  {420.2e6, 0, -1.0,  720e6,      0, 0x19a95a},
  {500e6,   0, -1.0,  720e6, -0x100, 0x19a95a},
  {600e6,   0, -1.0,  720e6,      0, 0x19a95a},
  {1e10,    0,  1.0, -480e6,      0, 0x1a65a6},
};
  
#ifdef USE_NEW_DJ_BREAKPOINTS
static struct breakpoint *dds2_breakpoints = amrs_breakpoints;
#else
static struct breakpoint *dds2_breakpoints = vnmrj_dds2_breakpoints;
#endif

/* rfswitch_base - return the rfswitch value for the given frequency
   from the breakpoint table. */
static unsigned rfswitch_base(double freq, struct breakpoint *bp)
{
  for (;freq > bp->freq; bp++);
  return bp->rfswitch;
}

unsigned dds2_switch(double freq)
{
  return rfswitch_base(freq,dds2_breakpoints);
}

unsigned dds_switch(double freq)
{
  return rfswitch_base(freq,dds_breakpoints);
}

/* fold_freq_base - maps the given frequency onto a new frequency
   according to the values specified in the passed breakpoint
   table entry. */
static double fold_freq_base(double freq, struct breakpoint *bp)
{
  for (;freq > bp->freq; bp++);
  return (freq + bp->freq_adjust) * bp->freq_scale + bp->freq_offset;
}

static double fixed_to_fc(double freq, double fixed)
{
  return 65536.0 * freq / fixed;
}

/* ftw2_base - computes tuning word 2. */
static int ftw2_base(double freq, struct breakpoint *bp)
{
  int count;
  for (;freq > bp->freq; bp++);

  freq = fold_freq_base(freq,bp);
  int fixed = ((int)(fixed_to_fc(freq, 782.5e6))) & 0xff00;
  double fc = fixed_to_fc(freq,fixed);

  for (count = 16; fc>819e6 && count!=0; count--) {
    fixed += 16;
    fc = fixed_to_fc(freq,fixed);
  }

  return count ? fixed + bp->ftw2_adjust : -1;
}

int dds_ftw2(double freq)
{
  return ftw2_base(freq,dds_breakpoints);
}

int dds2_ftw2(double freq)
{
  return ftw2_base(freq,dds2_breakpoints);
}

/* ftw1_base - computes tuning word 1. */
static int ftw1_base(double freq, struct breakpoint *bp)
{
  int ftw2 = ftw2_base(freq,bp);
  if (ftw2 == -1)
    return -1;
  return (int)(((((fold_freq_base(freq,bp)*65536.0/((double) ftw2))/3.0)-240e6)/240e6)*4294967296.0L);
}

int dds_ftw1(double freq)
{
  return ftw1_base(freq,dds_breakpoints);
}

int dds2_ftw1(double freq)
{
  return ftw1_base(freq,dds2_breakpoints);
}

/* format_packet - formats a packet for transmission to the VFS/DJ programmer. */
int format_dds(int *buffer, 
	       unsigned long ftw1, unsigned long ftw2, unsigned long rfswitch)
{
  int i, nbytes = 0;
  buffer[++nbytes] = 0x01;
  buffer[++nbytes] = 0x00;
  buffer[++nbytes] = 0x00;
  buffer[++nbytes] = ftw1 & 0xff;
  buffer[++nbytes] = (ftw1>>8) & 0xff;
  buffer[++nbytes] = (ftw1>>16) & 0xff;
  buffer[++nbytes] = (ftw1>>24) & 0xff;
  buffer[++nbytes] = 0x00;
  buffer[++nbytes] = 0x00;
  buffer[++nbytes] = ftw2 & 0xff;
  buffer[++nbytes] = (ftw2 >> 8) & 0xff;
  buffer[++nbytes] = rfswitch & 0xff;
  buffer[++nbytes] = (rfswitch >> 8) & 0xff;
  buffer[++nbytes] = (rfswitch >> 16) & 0xff;
  buffer[++nbytes] = (rfswitch >> 24) & 0xff;
  buffer[++nbytes] = 0x00;
  buffer[++nbytes] = 0x00;
  buffer[++nbytes] = 0x1ff; // termination character
  buffer[0] = nbytes++; // 0x52;
  //if (bgflag) {
    printf("Synthesizer: tw =");
    for (i=0; i<nbytes; i++)
      printf(" %02x", buffer[i]);
    printf("\n");
    //}
  return nbytes;
}

int format_var400(int *buffer, unsigned long long ftw, int rfswitch)
{
  int nbytes = 0;
  buffer[++nbytes] = 0x01;
  buffer[++nbytes] = ftw & 0xff;
  buffer[++nbytes] = (ftw >> 8) & 0xff;
  buffer[++nbytes] = (ftw >> 16) & 0xff;
  buffer[++nbytes] = (ftw >> 24) & 0xff;
  buffer[++nbytes] = (ftw >> 32) & 0xff;
  buffer[++nbytes] = (ftw >> 40) & 0xff;
  buffer[++nbytes] = rfswitch;
  buffer[0] = ++nbytes;
  return nbytes;
}

static unsigned long long toBCD(double f)
{
  unsigned long long binary = (unsigned long long) f;
  unsigned long long bcd = 0LL;
  int i;
  for (i=0; i<16; i++, binary /= 10)
     bcd |= (binary % 10) << (i*4);

  return bcd;
}

static unsigned long long var400_ftw(double freq)
{
  struct breakpoint *bp = var400_breakpoints;
  freq = freq + bp->freq_adjust * bp->freq_scale;
  return (unsigned long long) freq;
}

static int format_pts(int* buffer, unsigned long long ftw, unsigned char rfswitch)
{
  union64 tw;
  tw.ll = ftw;
  int nbytes = 0;
  buffer[++nbytes] = tw.c8.char7 & 0xff;
  buffer[++nbytes] = tw.c8.char6 & 0xff;
  buffer[++nbytes] = tw.c8.char5 & 0xff;
  buffer[++nbytes] = tw.c8.char4 & 0xff;
  buffer[++nbytes] = tw.c8.char3 & 0xff;
  buffer[++nbytes] = rfswitch;
  buffer[0] = ++nbytes;
  return nbytes;
}

int _encodeFreq(SynthesizerType *type, double freq, int *array)
{
  freq *= 1e6L;
  if (strncmp(type->type,"VAR2DDS",7)==0 
      || strncmp(type->type,"VARDDS",6)==0
      || strncmp(type->type,"AMRS",4)==0 )
 {
    unsigned rfswitch = rfswitch_base(freq,(struct breakpoint*)type->bp);//dds2_switch(freq);
    int ftw1 = ftw1_base(freq,(struct breakpoint*)type->bp);
    int ftw2 = ftw2_base(freq,(struct breakpoint*)type->bp);
    return format_dds(array, ftw1, ftw2, rfswitch);
  } else if (strncmp(type->type,"VAR400",6) == 0) {
    unsigned rfswitch = var400_breakpoints[0].rfswitch;
    unsigned long long ftw1 = var400_ftw(freq);
    return format_var400(array, ftw1, rfswitch);
  } 
  // it's a PTS 160/320/800/1000
  struct breakpoint *bp = pts_breakpoints;
  unsigned char rfswitch = bp->rfswitch;
  union64 tw;
  if (strncmp(type->type,"PTS320",6) == 0)
    freq -= 10.0;
  freq = (freq + bp->freq_adjust) * bp->freq_scale + bp->freq_offset;
  tw.ll = toBCD(freq);
  if (strncmp(type->type,"PTS160",6) == 0) {
    tw.c8.char3 += 10;
    tw.c8.char3 &= 0xf;
  }
  return format_pts(array, tw.ll, rfswitch);
}

// this data seems to be mostly for documentation purposes
SynthesizerType _SynTypes[] = {
  { 600,  "VARDDS", 1000.0, 380.0, 0.1,   dds_breakpoints},   // default
  { 456,    "AMRS", 1000.0,   1.0, 0.001, amrs_breakpoints},  // confirm this!
  {1000, "PTS1000", 1000.0,   1.0, 0.1,   pts_breakpoints},
  { 620,  "PTS620",  620.0,   1.0, 0.1,   pts_breakpoints},
  { 500,  "PTS500",  520.0,   1.0, 0.1,   pts_breakpoints},
  { 320,  "PTS320",  320.0,   1.0, 0.1,   pts320_breakpoints},
  { 160,  "PTS160",  160.0,   1.0, 0.1,   pts160_breakpoints},
  { 400,  "VAR400",  420.0, 380.0, 0.1,   var400_breakpoints},
  { 800, "VAR2DDS", 1000.0,   1.0, 0.001, amrs_breakpoints},
};

SynthesizerType* synthType(int ptsval)
{
  // system uses PTS val (number); a string would be better
  int i=0, count = sizeof(_SynTypes)/sizeof(SynthesizerType);
  const char *synT = NULL;
  SynthesizerType *type = _SynTypes;
  for (i=0; i<count; i++)
    if (_SynTypes[i].ptsval == ptsval) {
      type = _SynTypes + i;
      synT = _SynTypes[i].type;
      //if (bgflag) 
	printf("Synthesizer: type %s\n", synT);
      return type;
    }
  printf("Synthesizer: unknown PTS type %d\n", ptsval);
  return NULL;
}

int encFreq(int ptsval, int freq)
{
  int buf[25];
  int i;
  char *buffer;
  SynthesizerType *type;
  type = synthType(ptsval);
  buffer = (char*) buf;
  int nb = _encodeFreq(type, freq, buf);
  return nb;
}

//int prtFlag(int flag){}

// To set frequency on an AMRS to 480MHz: setFreq(456,480000000)
int setFreq(int ptsval, double freq)
{
  int buffer[25];
  int i;
  SynthesizerType *type;
  type = synthType(ptsval);
  int nb = _encodeFreq(type, freq, buffer);

  set_field(RF,fifo_output_select,0);
  for (i=0; i<nb; i++) {
    set_field(RF,sw_aux,(0 << 8 ) | buffer[i]);
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    set_field(RF,sw_aux_strobe,1);
    set_field(RF,sw_aux_strobe,0);
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    printf("  %d: %02x ", i+1, buffer[i]);
  }
  printf("\n");
  //set_field(RF,sw_aux,(1 << 8 ) | 0xff);
  //taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
  set_field(RF,sw_aux_strobe,1);
  set_field(RF,sw_aux_strobe,0);
  taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
  return nb;
}

#ifdef STANDALONE
main(int argc, char **argv)
{
  int type = 0;
  double freq = 0;

  if (argc < 3 ) {
    fprintf(stderr, "usage: %s pts-type freq\n\n", argv[0]);
    return 1;
  }
  type = atoi(argv[1]);
  sscanf(argv[2], "%lf", &freq);
  setFreq(type, freq);
}

#endif

#ifdef __cplusplus
int Synthesizer::encodeFreq(double freq, int *array)
{
  return _encodeFreq(type, freq, array);
}

Synthesizer::Synthesizer(int ptsval)
  : type(NULL)
{
  // system uses PTS val (number); a string would be better
  int count = sizeof(_SynTypes)/sizeof(Synthesizer::SynthesizerType);
  type = _SynTypes;
  for (int i=0; i<count; i++)
    if (_SynTypes[i].ptsval == ptsval) {
      type = _SynTypes + i;
      synT = _SynTypes[i].type;
      //if (bgflag) 
	printf("Synthesizer: type %s\n", synT);
    }
}
#endif /* __cplusplus */

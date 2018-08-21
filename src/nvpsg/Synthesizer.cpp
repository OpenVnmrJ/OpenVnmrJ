/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
// notes for those who cherish the odd
// pts 320 subtract 10.0 MHz always.
// pts 160 convert top 2 bcd digits to binary.
//
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "cpsg.h"
#include "ACode32.h"
#include "Synthesizer.h"
#include "aptable.h"

#define DBG if (bgflag) printf

extern int bgflag; 
extern int dps_flag;
extern char systemdir[];

#ifdef STANDALONE
int bgflag = 0;
#else
extern int bgflag; // set when 'debug' passed to VnmrJ "go", i.e. go('debug')
#endif

extern int tuneflag;

// Define this to control the frequency from host-located tables
// (requires talk2simon.c and amrs.c to be compiled and linked in).
#undef VFSCORRECT
// Define this to control the frequency from the controller
#define DO_ADVISE_FREQ
#if defined(VFSCORRECT) && defined(DO_ADVISE_FREQ)
  #error VFSCORRECT and DO_ADVISE_FREQ are mutually exclusive!
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
  {1e10, -(135.0+270.0), 0x100000000000LL/(16*80000000.0), 0, 0, 0x101},
};

static struct breakpoint pts320_breakpoints[] =
{
  {1e10, -10.0, 10000000.0L, +0.5, 0, 0x101},
};

static struct breakpoint pts160_breakpoints[] =
{
  {1e10, +10.0, 10000000.0L, +0.5, 0, 0x101},
};

static struct breakpoint pts_breakpoints[] =
{
  {1e10,     0, 10000000.0L, +0.5, 0, 0x101},
};

/* Breakpoints for the VFS synthesizer. */
static struct breakpoint dds_breakpoints[] = 
{
  {325e6,    0,  1.0, 0.0,   0, 0x00aaaaaa},
  {1e10,     0, -1.0, 720e6, 0, 0x03555555},
};

#if 0
/* Breakpoints for the DJ synthesizer Note: These values do not work
   above 500 MHz, here for documentation purposes only */
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
#endif

/* Eric's preferred breakpoints for DJ.  Tested to work up to 800MHz. */
static struct breakpoint amrs_breakpoints[] = 
{
  {310e6,   0,  1.0,    0e6,      0, 0x069595},
  {400e6,   0, -1.0,  480e6,      0, 0x156aa6},
  {420.2e6, 0, -1.0,  720e6,      0, 0x19a95a},
  {500e6,   0, -1.0,  720e6, -0x100, 0x19a95a},
  {600e6,   0, -1.0,  720e6,      0, 0x19a95a},
  {1e10,    0,  1.0, -480e6,      0, 0x1a65a6},
  {0,0,0,0,0,0}
};

/* rfswitch_base - return the rfswitch value for the given frequency
   from the breakpoint table.  Beware of round-off errors:
   310MHz is represented as 41b27a3980000000, but:
   310MHz Starting at 150 incrementing 1e6: 41b27a3980000001
   310MHz Starting at 82 incrementing 1e6:  41b27a3980000000
 */
static unsigned rfswitch_base(double freq, struct breakpoint *bp)
{
  return bp->rfswitch;
}

/* fold_freq_base - maps the given frequency onto a new frequency
   according to the values specified in the passed breakpoint
   table entry. */

#define AS_(x,TYPE) (*(TYPE*)((void*)(&x))) /* get rid of format warnings */
#define AS_ULL(x) AS_(x,unsigned long long)
#define AS_UL(x) AS_(x,unsigned long)

//FLT2UL(freq_index), DBL2UL(bp->freq));
static double fold_freq_base(double freq, struct breakpoint *bp)
{
  double folded = (freq + bp->freq_adjust) * bp->freq_scale + bp->freq_offset;
  DBG("  fold(%lf) = %lf (%llx, %llx)\n", freq, folded, AS_ULL(freq), AS_ULL(bp->freq));
  return folded;
}

static double fixed_to_fc(double freq, double fixed)
{
  return 65536.0 * freq / fixed;
}

/* ftw2_base - computes tuning word 2. */
static int ftw2_base(double freq, struct breakpoint *bp)
{
  int count;
  double clk_freq_low = 782.5e6, folded_freq;

  folded_freq = fold_freq_base(freq,bp);
  if (tuneflag)
    return ((int)(fixed_to_fc(folded_freq, clk_freq_low))) & 0xfff0;

  int fixed = ((int)(fixed_to_fc(folded_freq, clk_freq_low))) & 0xff00;
  double fc = fixed_to_fc(folded_freq,fixed);

  for (count = 16; fc>819e6 && count!=0; count--) {
    fixed += 16;
    fc = fixed_to_fc(folded_freq,fixed);
  }

  return count ? fixed + bp->ftw2_adjust : -1;
}

/* ftw1_base - computes tuning word 1. */
static int ftw1_base(double freq, struct breakpoint *bp, int ftw2)
{
  if (ftw2 == -1)
    return -1;
  return (int)(((((fold_freq_base(freq,bp)*65536.0/((double) ftw2))/3.0)-240e6)/240e6)*4294967296.0L);
}

/* format_packet - formats a packet for transmission to the VFS/DJ programmer. */
int Synthesizer::format_dds(int *buffer, 
	       unsigned long ftw1, unsigned long ftw2, unsigned long rfswitch,
	       int atten, unsigned short mhz)
{
  int nbytes = 0;
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
#if defined(VFSCORRECT) || defined(DO_ADVISE_FREQ)
  buffer[++nbytes] = atten & 0x3f;
  buffer[++nbytes] = mhz & 0xff;      
  buffer[++nbytes] = (mhz>>8) & 0xff; // MHz, w. 40MHz offset
#else
  buffer[++nbytes] = (rfswitch >> 24) & 0xff;
  buffer[++nbytes] = 0x00;
  buffer[++nbytes] = 0x00;
#endif
  buffer[++nbytes] = 0x1ff; // termination character
  buffer[0] = nbytes++; // 0x52;
  if (bgflag) {
    printf("Synthesizer(%d MHz): tw =", mhz);
    for (int i=0; i<nbytes; i++)
      printf(" %02x", buffer[i]);
    printf("\n");
  }
  return nbytes;
}

int Synthesizer::format_var400(int *buffer, unsigned long long ftw, int rfswitch)
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
  buffer[0] = 0x07; nbytes++; // the actual number of words is 8!?
  return nbytes;
}

static unsigned long long toBCD(double f)
{
  unsigned long long binary = (unsigned long long) f;
  unsigned long long bcd = 0LL;

  for (int i=0; i<16; i++, binary /= 10)
     bcd |= (binary % 10) << (i*4);

  return bcd;
}

static unsigned long long var400_ftw(double freq)
{
  struct breakpoint *bp = var400_breakpoints;
  freq = 1e6 * (freq + bp->freq_adjust) * bp->freq_scale;
  return (unsigned long long) freq;
}

static int format_pts(int* buffer, unsigned long long ftw, unsigned int rfswitch)
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
  buffer[0] = nbytes++;
  return nbytes;
}

int Synthesizer::encodeFreq(double freq_mhz, int *array)
{
  double freq = freq_mhz * 1e6L;
  if (strncmp(type->type,"VAR2DDS",7)==0 
      || strncmp(type->type,"VARDDS",6)==0
      || strncmp(type->type,"AMRS",4)==0)
  {
    int i, j;
    breakpoint *bp;
    unsigned short mhz = (unsigned short) round(freq_mhz);
    float freq_index = (float) freq;  // float to match lack of precision of forth interp
                                      // with which initial calibration was performed...
    DBG("encodeFreq(%lf MHz): %lf Hz\n", freq_mhz, freq);
    for (i=0, bp=(breakpoint*) type->bp; freq_index > (float)bp->freq; bp++, i++)
      DBG("..bp[%d]: %lf > %lf (%llx, %llx)\n", i, freq, bp->freq, AS_ULL(freq), AS_ULL(bp->freq));
    DBG("%f: bp[%d].freq=%f (%lx, %lx)\n", freq_index, i, (float)bp->freq, AS_UL(freq_index), AS_UL(bp->freq));

    unsigned rfswitch = rfswitch_base(freq,bp);
    int ftw2 = ftw2_base(freq, bp);
    int ftw1 = ftw1_base(freq, bp, ftw2);
    int atten = 0x00; // set in the controller itself

    // round away from the breakpoints
    for (j=0, bp=(breakpoint*) type->bp; bp->freq != 0; bp++, j++) {
      float delta = fabsf( (float) (bp->freq - freq) );
      if (delta < 1e6 && freq_index > (float)bp->freq) {
	mhz = (unsigned short) (freq_mhz + 1);
	DBG("%d: %f is close to %f! delta = %f, mhz=%d\n", j, freq, bp->freq, delta, (int)mhz);
	break;
      } else if (delta < 1e6 && freq_index <= (float) bp->freq) {
	mhz = (freq_mhz * 1e6 == (int) bp->freq) ? (unsigned short) (freq_mhz - 1) : (unsigned short) freq_mhz;
	DBG("%d: %f is close to %f! delta = %f, mhz=%d\n", j, freq, bp->freq, delta, (int)mhz);
	break;
      } else {
	DBG("%f is not close to %f - delta = %f, mhz=%d\n", freq, bp->freq, delta, (int)mhz);
      }
    }

    return format_dds(array, ftw1, ftw2, rfswitch, atten, mhz);
  } else if (strncmp(type->type,"VAR400",6) == 0) {
    unsigned rfswitch = var400_breakpoints[0].rfswitch;
    unsigned long long ftw1 = var400_ftw(freq_mhz);
    return format_var400(array, ftw1, rfswitch);
  } 
  // it's a PTS 160/320/800/1000
  struct breakpoint *bp = (breakpoint*) type->bp;
  unsigned int rfswitch = bp->rfswitch;
  union64 tw;
  if (strncmp(type->type,"PTS320",6) == 0)
    freq_mhz -= 10.0;
  freq = freq_mhz * bp->freq_scale + bp->freq_offset;
  tw.ll = ~toBCD(freq);

  if ((strncmp(synT,"PTS160",6) == 0) && (tw.c8.char3 & 0x10)) {
    tw.c8.char3 += 10;
    tw.c8.char3 &= 0xf;
  }
  return format_pts(array, tw.ll, rfswitch);
}

int Synthesizer::adviseFreq()
{
  return strcmp(synT, "AMRS") == 0;
}

// this data seems to be mostly for documentation purposes
Synthesizer::SynthesizerType _SynTypes[] = {
  { 600,  "VARDDS", 1000.0, 380.0, 0.1,   dds_breakpoints},   // default
  { 456,    "AMRS", 1000.0,   1.0, 0.001, amrs_breakpoints},
  {1000, "PTS1000", 1000.0,   1.0, 0.1,   pts_breakpoints},
  { 620,  "PTS620",  620.0,   1.0, 0.1,   pts_breakpoints},
  { 500,  "PTS500",  520.0,   1.0, 0.1,   pts_breakpoints},
  { 320,  "PTS320",  320.0,   1.0, 0.1,   pts320_breakpoints},
  { 160,  "PTS160",  160.0,   1.0, 0.1,   pts160_breakpoints},
  { 400,  "VAR400",  420.0, 380.0, 0.1,   var400_breakpoints},
  { 800, "VAR2DDS", 1000.0,   1.0, 0.001,  amrs_breakpoints},
};

static int Find(int ptsval)
{
  int count = sizeof(_SynTypes)/sizeof(Synthesizer::SynthesizerType);
  for (int i=0; i<count; i++)
    if (_SynTypes[i].ptsval == ptsval)
      return i;
  return -1;
}

Synthesizer::Synthesizer(const char* name, int ptsval)
  : name(name)
  , type(NULL)
{
  // system uses PTS val (number); a string would be better
  int i = Find(ptsval);
  if (i < 0) {
    if (dps_flag)   {
      printf("Invalid value %d for ptsval\n", ptsval);
      return;
    } else {
      abort_message("Invalid value %d for ptsval. Abort!\n",ptsval);
    }
  }

  type = _SynTypes + i;
  synT = _SynTypes[i].type;
  
  DBG("Synthesizer: type %s (ptsval %d)\n", synT, ptsval);
  #ifdef VFSCORRECT
	amrs_atten_enable();
	amrs_tbl_sz = loadAttenTbl();
  #endif
}

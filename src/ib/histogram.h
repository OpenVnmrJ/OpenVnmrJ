/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _HISTOGRAM_H
#define _HISTOGRAM_H
/************************************************************************
*									
*  @(#)histogram.h 18.1 03/21/08 (c)1992 SISCO 
*
*************************************************************************
*									
*  Chris Price
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/
// #include <stream.h>
//#include "stringedit.h"
#include "macrolist_ib.h"


class Histogram
{
  public:
    int nbins;		// Number of bins
    int *counts;	// Number of pixels in each bin
    float bottom;	// Intensity at bottom of first bin
    float top;		// Intensity at top of last bin

    Histogram(){counts = 0;};
    ~Histogram(){
	if (counts){
	    delete [] counts;
	}
    }
};

const int STATS_MAXSTR = 128;

class Stats
{
  public:
    char fname[STATS_MAXSTR+1];	// Name of image data file stats came from
    char vol_label[STATS_MAXSTR+1];	// Type of volume calculation
    Histogram *histogram;
    int framenum;
    double min;
    double max;
    double median;
    double mean;
    double sdv;
    double area;
    double npixels;	// May deal with fractional pixels in future
    double volume;
    double thickness;	// Thickness of data slab in cm.
    double z_location;	// Z-coord of center of slab in user coord frame
    Imginfo *imginfo;	// The image this stuff came from

    Stats(){histogram = 0; strcpy(vol_label, "Volume");};
    ~Stats(){
	if (histogram){
	    delete histogram;
	}
    }
};

Declare_ListClass(Stats);

class StatsIterator {
public:
  StatsIterator(StatsList* r) : rtl(r) {
    if (rtl)
      next = rtl->First();
    else
      next = 0;
  }
  
  ~StatsIterator() {};
  
  int NotEmpty() { return (next ? TRUE : FALSE) ; }
  int  IsEmpty() { return (next ? FALSE : TRUE) ; }
  
  StatsIterator& GotoLast()  {
    if (rtl)
      next = rtl->Last();
    return *this;
  }
  
  StatsIterator& GotoFirst() {
    if (rtl)
      next = rtl->First();
    return *this;
  }
  
  Stats *operator++() {
    StatsLink* curr;
    curr = next;
    if (next)
      next = next->Next();
    if (curr)
      return curr->Item();
    else
      return 0;
  }
  
  Stats *operator--() {
    StatsLink* curr;
    curr = next;
    if (next)
      next = next->Prev();
    if (curr)
      return curr->Item();
    else
      return 0;
  }
  
private:
  
  StatsLink* next;
  StatsList* rtl;
  
};

#endif _HISTOGRAM_H

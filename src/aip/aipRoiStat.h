/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPROISTAT_H
#define AIPROISTAT_H

#include <vector>

#include "aipHistogram.h"
#include "aipImgInfo.h"

const int STATS_MAXSTR = 511;

class RoiData
{
public:
    char fname[STATS_MAXSTR+1];	// Name of image data file stats came from
    char vol_label[16];         // Type of volume calculation
    bool clipped;
    Histogram *histogram;
    int frameNumber;
    int roiNumber;
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
    spImgInfo_t imgInfo;	// The image this stuff came from

    RoiData(){histogram = 0; strcpy(vol_label, "Volume");};
    ~RoiData(){
	if (histogram){
	    delete histogram;
	}
    }
};

typedef std::vector<RoiData *> RoiDataList;

class RoiStat
{
public:
    RoiStat();
    ~RoiStat();

    static RoiStat *get();
    static void segmentRois(int type);
    static void segmentImages();

    static int aipStatUpdate(int argc, char **argv, int retc, char **retv);
    static int aipSegment(int argc, char **argv, int retc, char **retv);
    static int aipStatPrint(int argc, char **argv, int retc, char **retv);

    void calculate(bool movingFlag = false);
    void calcStats(RoiDataList& statlist, RoiData& tot_stat);
    void show(RoiDataList statlist, RoiData tot_stat);

    void clearGraph();
    void drawHistogram(int *hist, int nbins, double min, double max);
    void drawScatterplot(double *x, double *y, string xLabel, string yLabel,
			 int nvalues);

    int buckets;
    void writeStats(const char *fname, const char *mode, int header);

private:
    static RoiStat *roiStat;	// Support one instance of me
    static const string baseFilename;

    int range_type;

    double fboundary1, fboundary2;
    bool boundary1_active, boundary2_active;
    bool boundary1_defined, boundary2_defined;
    bool histogram_showing;
    bool updt_flag;

    //bool zPosnLess(const RoiData *stat1, const RoiData *stat2);
};

#endif /* AIPROISTAT_H */

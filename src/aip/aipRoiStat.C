/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/param.h>

#include <cmath>
#include <algorithm>
using std::stable_sort;
#include <string>
using std::string;
#include <set>
using std::set;

#include "aipUtils.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipRoi.h"
#include "aipRoiManager.h"
#include "aipRoiStat.h"
#include "aipCommands.h"
#include "aipVnmrFuncs.h"
using namespace aip;

RoiStat *RoiStat::roiStat = NULL;
const string RoiStat::baseFilename = "/tmp/VjStatGraphFile";

#ifdef __INTERIX
 extern "C" {
       void unixPathToWin(char *path,char *buff,int maxlength);
 }
#endif

RoiStat::RoiStat()
{
    buckets = (int)getReal("aipStatNumBins", 100);
    range_type = (int)getReal("aipStatHistRangeType", 0);
}

RoiStat::~RoiStat()
{
}

/* STATIC */
RoiStat *
RoiStat::get()
{
    if (!roiStat) {
	deleteOldFiles(baseFilename);
	roiStat = new RoiStat();
    }
    return roiStat;
}

/* STATIC VNMRCOMMAND */
int
RoiStat::aipStatUpdate(int argc, char **argv, int retc, char **retv)
{
    get()->calculate();
    return proc_complete;
}

/* STATIC VNMRCOMMAND */
int
RoiStat::aipSegment(int argc, char **argv, int retc, char **retv)
{
    int segmentType = 'i';
    if (argc >= 2) {
        segmentType = *argv[1];
    }

    switch (segmentType) {
      case 'i':
	segmentImages();
        break;
      default:
	segmentRois(segmentType);
        break;
    }
    return proc_complete;
}

/* STATIC VNMRCOMMAND */
int
RoiStat::aipStatPrint(int argc, char **argv, int retc, char **retv)
{
    argc--;
    argv++;
    
    RoiStat *rs = get();
    switch (argc) {
    case 0:
        return proc_error; // For now, anyway
        //win_stat_print();
        break;
    case 1:
        rs->writeStats(argv[0], "w", false);
        break;
    case 2:
        // If not specified otherwise, set header to true
        if (strlen(argv[1]) == 1&& (*argv[1] == 'w'|| *argv[1] == 'a')) {
            rs->writeStats(argv[0], argv[1], true);
        } else {
            return proc_error;
        }
        break;
    case 3:
        // Third arg is to turn off header info.  If there is a 3rd arg
        // at all, no header.
        if (strlen(argv[1]) == 1&& (*argv[1] == 'w'|| *argv[1] == 'a')) {
            rs->writeStats(argv[0], argv[1], false);
        } else {
            return proc_error;
        }
        break;
    default:
        return proc_error;
        break;
    }
    return proc_complete;
}

/*
 * Comparison function for qsort done in calculate().
 */
static bool
zPosnLess(const RoiData *stat1, const RoiData *stat2)
{
    return stat1->z_location < stat2->z_location;
}

/*
 * Comparison function for sort done in writeStats().
 */
static bool
alphaByFilepath(const RoiData *stat1, const RoiData *stat2)
{
    return strcmp(stat1->fname, stat2->fname) < 0;
}

/*
 * Calculate statistics within selected ROIs.
 */
void
RoiStat::calculate(bool movingFlag)
{
    const int buflen = 100;

    if (movingFlag && getReal("aipStatUpdateOnMove", 0) == 0) {
	return;			// Don't update while still dragging ROI
    }

    RoiDataList statlist;
    RoiData tot_stat;		// Place to gather summary stats
    calcStats(statlist, tot_stat);

    int npixels = (int) tot_stat.npixels;
    if (npixels < 0) {
        npixels = 0;            // Sanity check
    }
    int nslices = statlist.size();
    if (nslices < 0) {
        nslices = 0;            // Sanity check
    }
    char buf[buflen];

    static char minStr[buflen] = "---";
    if (nslices && npixels) {
        sprintf(buf, "Min: %.4g", tot_stat.min);
    } else {
        sprintf(buf, "Min:");
    }
    if (strcmp(buf, minStr)) {
	strcpy(minStr, buf);
	setString("aipStatMinMsg", buf, true);
    }
    static char maxStr[buflen] = "---";
    if (nslices && npixels) {
        sprintf(buf, "Max: %.4g", tot_stat.max);
    } else {
        sprintf(buf, "Max:");
    }
    if (strcmp(buf, maxStr)) {
	strcpy(maxStr, buf);
	setString("aipStatMaxMsg", buf, true);
    }
    static char areaStr[buflen] = "---";
    if (nslices && npixels) {
        sprintf(buf, "Area: %.4g sq cm", tot_stat.area);
    } else {
        sprintf(buf, "Area:");
    }
    if (strcmp(buf, areaStr)) {
	strcpy(areaStr, buf);
	setString("aipStatAreaMsg", buf, true);
    }
    if (npixels > 0){
	tot_stat.mean /= npixels;
	tot_stat.median /= npixels;
    }
    static char meanStr[buflen] = "---";
    if (nslices && npixels) {
        sprintf(buf, "Mean: %.4g", tot_stat.mean);
    } else {
        sprintf(buf, "Mean:");
    }
    if (strcmp(buf, meanStr)) {
	strcpy(meanStr, buf);
	setString("aipStatMeanMsg", buf, true);
    }
    static char medianStr[buflen] = "---";
    if (nslices == 1 && npixels > 0) {
	sprintf(buf, "Median: %.4g", tot_stat.median);
    } else if (nslices == 0 || npixels == 0) {
        sprintf(buf, "Median:");
    } else {
	sprintf(buf, "Median: n/a");
    }
    if (strcmp(buf, medianStr)) {
	strcpy(medianStr, buf);
	setString("aipStatMedianMsg", buf, true);
    }

    static char sdvStr[buflen] = "---";
    if (nslices && npixels) {
        sprintf(buf, "Std dev: %.4g", tot_stat.sdv);
    } else {
        sprintf(buf, "Std dev:");
    }
    if (strcmp(buf, sdvStr)) {
	strcpy(sdvStr, buf);
	setString("aipStatSdvMsg", buf, true);
    }

    static char volStr[buflen] = "---";
    if (nslices && npixels) {
        sprintf(buf, "%s: %.4g", tot_stat.vol_label, tot_stat.volume);
    } else {
        sprintf(buf, "Volume:");
    }
    if (strcmp(buf, volStr)) {
	strcpy(volStr, buf);
	setString("aipStatVolumeMsg", buf, true);
    }

    static char clipStr[buflen] = "---";
    if (nslices && npixels == 0) {
	strcpy(buf,"No Pixels in Data Limits");
    } else if (nslices && tot_stat.clipped) {
	strcpy(buf,"Stats Reflect Data Limits");
    } else {
	strcpy(buf," ");
    }
    if (strcmp(buf, clipStr)) {
	strcpy(clipStr, buf);
	setString("aipStatClipped", buf, true);
    }

    // Update the graph
    show(statlist, tot_stat);

    // Release statistics
    RoiDataList::iterator itr;
    for (itr=statlist.begin(); itr != statlist.end(); ) {
	delete *itr++;
    }
    statlist.clear();
}

void
RoiStat::calcStats(RoiDataList& statlist, RoiData& tot_stat)
{
    int i;
    double last_z_location = 0;
    char last_name[1025];
    spImgInfo_t iptr;	// image information pointer
    double min, max;	// minimum and maximum pixel values

    tot_stat.npixels = 0;
    RoiManager *roim = RoiManager::get();
    if (roim->numberSelected() < 1) {
	return;			// No ROIs to look at
    }

    // Loop over all selected ROIs
    Roi *roi;
    int diff_names = false;
    int diff_slices = false;
    int nslices = 0;
    *last_name = 0;
    range_type = (int)getReal("aipStatHistRangeType", 0);
    boundary1_defined = isActive("aipStatCursMin");
    boundary2_defined = isActive("aipStatCursMax");
    fboundary1 = getReal("aipStatCursMin", 0);
    fboundary2 = getReal("aipStatCursMax", 1);
    double userMin = getReal("aipStatHistMin", 0);
    double userMax = getReal("aipStatHistMax", 1);
    buckets = (int)getReal("aipStatNumBins", 100);
    if(buckets <= 0) {
	buckets = 100;
    	setReal("aipStatNumBins", buckets, false);
    }
    tot_stat.clipped = false;
    for (roi=roim->getFirstSelected(); roi; roi=roim->getNextSelected()) {
	if(!roi->pOwnerFrame) continue;
	iptr = roi->pOwnerFrame->getFirstImage();
	spDataInfo_t di = iptr->getDataInfo();

	if (range_type == 0) {
	    // Set min and max to range in ROI
	    roi->getMinMax(min, max);
	} else if (range_type == 1) {
	    // Set min and max to values for this data file
	    di->getMinMax(min, max);
	} else if (range_type == 2) {
	    // Set the segmentation limits, if we have them
	    // --otherwise Image limits
	    di->getMinMax(min, max);
	    if (boundary1_defined) {
		min = fboundary1;
		tot_stat.clipped = true;
	    }
	    if (boundary2_defined) {
		max = fboundary2;
		tot_stat.clipped = true;
	    }
	} else if (range_type == 3) {
	    // Set min and max to user specfied values
	    min = userMin;
	    max = userMax;
	    tot_stat.clipped = true;
	} else {
	    fprintf(stderr,"RoiStat::calcStats(): invalid range_type");
	    return;
	}

	RoiData *stats = new RoiData; // Gets held in "statlist"
	if (! roi->histostats(buckets, min, max, stats) ) {
	    delete stats;
	} else {
	    if (tot_stat.npixels == 0){
		// Init summary statistics first time through
		if (stats->npixels){
		    tot_stat.min = stats->min;
		    tot_stat.max = stats->max;
		    tot_stat.median = stats->median * stats->npixels;
		    tot_stat.mean = stats->mean * stats->npixels;
		    tot_stat.sdv = stats->sdv * stats->npixels;
		    tot_stat.area = stats->area;
		    tot_stat.volume = stats->volume;
		    tot_stat.npixels = stats->npixels;
		}
	    } else {
		// Accumulate summary statistics
		if (stats->npixels) {
		    if (stats->max > tot_stat.max) {
			tot_stat.max = stats->max;
		    }
		    if (stats->min < tot_stat.min) {
			tot_stat.min = stats->min;
		    }
		    tot_stat.median += stats->median * stats->npixels;
		    tot_stat.mean += stats->mean * stats->npixels;
		    tot_stat.sdv += stats->sdv * stats->npixels;
		    tot_stat.area += stats->area;
		    tot_stat.volume += stats->volume;
		    tot_stat.npixels += stats->npixels;
		}
		if (stats->z_location != last_z_location) {
		    diff_slices = true;
		}
	    }
	    last_z_location = stats->z_location;

	    if (di->getFilepath() == NULL){
		strcpy(stats->fname,"<no-name>");
	    }else{
		// TODO: accept unlimited length image file paths
		strncpy(stats->fname, di->getFilepath(), sizeof(stats->fname));
		//strcpy(stats->fname, com_clip_len_front(cp, STATS_MAXSTR));
	    }
	    if (*last_name){
		if (strcmp(stats->fname, last_name)){
		    diff_names = true;
		}
	    }
	    strcpy(last_name, stats->fname);

	    // Set the imginfo pointer
	    stats->imgInfo = iptr;
	    statlist.push_back(stats);
	    nslices++;
	}
    }
    
    // Now that we have the mean--recalculate the overall SDV
    double dx;
    double sd;
    tot_stat.sdv = 0;
    RoiDataList::iterator itr;
    for (itr=statlist.begin(); itr != statlist.end(); ++itr) {
	dx = ((*itr)->mean - tot_stat.mean / tot_stat.npixels);
	sd = (*itr)->sdv;
	tot_stat.sdv += (*itr)->npixels * (sd * sd + dx * dx);
    }
    if (tot_stat.npixels > 0){
	tot_stat.sdv = sqrt(tot_stat.sdv / tot_stat.npixels);
    }

    if (diff_names) {
	strcpy(tot_stat.fname, "");
    } else {
	strcpy(tot_stat.fname, last_name);
    }

    // If more than one ROI, cannot calculate overall median from summary stats

    // If slices have diff z-positions, recalculate volume, based on
    // interpolation of area in between slabs.
    if (!diff_slices) {
	sprintf(tot_stat.vol_label, "Slab Volume");
    } else {
	// Sort by z position of slices
	stable_sort(statlist.begin(), statlist.end(), zPosnLess);

	// Now calculate the volume
	double prev_a = statlist[0]->area;
	double prev_v = statlist[0]->volume;
	double prev_z = statlist[0]->z_location;
	double this_a;
	double this_v;
	double this_z;
	// Sum up all the volume in the first slab
	for (i=1; i<nslices && statlist[i]->z_location == prev_z; i++){
	    prev_a += statlist[i]->area;
	    prev_v += statlist[i]->volume;
	}
	// "i" now points to slab with different z-position
	tot_stat.volume = prev_v;
	// Now look at the rest of the slabs
	while (i<nslices){
	    this_a = statlist[i]->area;
	    this_v = statlist[i]->volume;
	    this_z = statlist[i]->z_location;
	    // Sum all the volumes in this slab
	    for (++i ; i<nslices && statlist[i]->z_location == this_z; i++){
		this_a += statlist[i]->area;
		this_v += statlist[i]->volume;
	    }
	    // "i" now points to slab with different z-position
	    tot_stat.volume += ( (this_z - prev_z) * (this_a + prev_a) / 2
				- prev_v / 2 + this_v / 2);
	    prev_a = this_a;
	    prev_v = this_v;
	    prev_z = this_z;
	}
	sprintf(tot_stat.vol_label, "3D Volume");
    }
}

void
RoiStat::show(RoiDataList statlist, RoiData tot_stat)
{
    RoiData *stats;
    int nstats = statlist.size();

    // Show the right buttons
    setReal("aipStatNumRois", nstats, false);

    if (nstats == 0 || tot_stat.npixels == 0) {
        clearGraph();
    } else if (nstats == 1) {
	// If only one ROI, plot the histogram
	stats = statlist.front();
	if (stats->npixels) {
	    drawHistogram(stats->histogram->counts,
			  stats->histogram->nbins,
			  stats->histogram->bottom,
			  stats->histogram->top);
	} else {
	    fprintf(stderr,"No pixels within histogram intensity range\n");
	}
    }else if (nstats > 1){
	int ordinate = (int)getReal("aipStatOrdinate", 0);
	int abscissa = (int)getReal("aipStatAbscissa", 0);

	// Gather data for all ROIs, and draw a graph
	string user_var = getString("aipStatUserVar", "");
	bool data_ok = true;
	double *xdata = new double[nstats];
	double *ydata = new double[nstats];
	double *stepwidth = new double[nstats];
	RoiDataList::iterator itr;
	int i;
	string xLabel;		// NB: these get set redundantly nstats times
	string yLabel;
	for (i=0, itr=statlist.begin(); itr != statlist.end(); itr++, i++) {
	    stats = *itr;
	    switch (abscissa){
	      case 0:	// Gframe #
		xdata[i] = stats->frameNumber;
		xLabel = "Frame Number";
		break;
	      case 1:	// ROI #
		xdata[i] = stats->roiNumber;
		xLabel = "ROI Number";
		break;
	      case 2:	// z_location
		xdata[i] = stats->z_location;
		stepwidth[i] = stats->thickness;
		xLabel = "Slice Location";
		break;
	      case 3:	// User variable
		data_ok = stats->imgInfo->getDataInfo()->st->GetValue(user_var.c_str(), xdata[i]);
		xLabel = user_var;
		break;
	      case 4:	// Area
		xdata[i] = stats->area;
		xLabel = "ROI Area";
		break;
	      case 5:	// Volume
		xdata[i] = stats->area * stats->thickness;
		xLabel = "ROI Volume";
		break;
	      case 6:	// Integrated Intensity
		xdata[i] = stats->mean * stats->npixels;
		xLabel = "Total ROI Intensity";
		break;
	      case 7:	// Mean Intensity
		xdata[i] = stats->mean;
		xLabel = "Mean ROI Intensity";
		break;
	      case 8:	// Median Intensity
		xdata[i] = stats->median;
		xLabel = "Median ROI Intensity";
		break;
	      case 9:	// Minimum Intensity
		xdata[i] = stats->min;
		xLabel = "Minimum ROI Intensity";
		break;
	      case 10:	// Maximum Intensity
		xdata[i] = stats->max;
		xLabel = "Maximum ROI Intensity";
		break;
	      case 11:	// Standard Deviation of Intensity
		xdata[i] = stats->sdv;
		xLabel = "SDV of ROI Intensity";
		break;
	    }
	    switch (ordinate){
	      case 0:	// Area
		ydata[i] = stats->area;
		yLabel = "ROI Area";
		break;
	      case 1:	// Volume
		ydata[i] = stats->area * stats->thickness;
		yLabel = "ROI Volume";
		break;
	      case 2:	// Integrated Intensity
		ydata[i] = stats->mean * stats->npixels;
		yLabel = "Total ROI Intensity";
		break;
	      case 3:	// Mean Intensity
		ydata[i] = stats->mean;
		yLabel = "Mean ROI Intensity";
		break;
	      case 4:	// Median Intensity
		ydata[i] = stats->median;
		yLabel = "Median ROI Intensity";
		break;
	      case 5:	// Minimum Intensity
		ydata[i] = stats->min;
		yLabel = "Minimum ROI Intensity";
		break;
	      case 6:	// Maximum Intensity
		ydata[i] = stats->max;
		yLabel = "Maximum ROI Intensity";
		break;
	      case 7:	// Standard Deviation of Intensity
		ydata[i] = stats->sdv;
		yLabel = "SDV of ROI Intensity";
		break;
	    }
	}

	if (data_ok){
	    // Plot it out
	    drawScatterplot(xdata, ydata, xLabel, yLabel, nstats);
	    
	    // Print out the summary statistics /* TODO */
	    //win_stat_update_panel(&tot_stat, nstats);
	} else if (user_var.find_first_not_of(" \t") != string::npos) {
	    // We didn't find "user_var", and it's not all white space.
	    char tbuf[1024];
	    sprintf(tbuf,"User parameter \"%s\" not found.\n", user_var.c_str());
	    fprintf(stderr, tbuf);
	}

	//XFlush(Gframe::gdev->xdpy);
	delete [] xdata;
	delete [] ydata;
	delete [] stepwidth;
    }
}

void
RoiStat::clearGraph()
{
    // Open file
    char fname[128];
    sprintf(fname,"%s%d", baseFilename.c_str(), getpid());
    FILE *fd = fopen(fname, "w");
    if (!fd) {
	fprintf(stderr,"Cannot open file for histogram: %s\n", fname);
	return;
    }

    // Write out header
    fprintf(fd,"Histogram %d\n", 1);
    fprintf(fd,"YMin 0\n");
    fprintf(fd,"XLabel Intensity\n");
    fprintf(fd,"YLabel Number of pixels\n");
    fprintf(fd,"NPoints %d\n", 0);
    fprintf(fd,"Data\n");

    fclose(fd);

    // Set parameter pointing to file
#ifdef __INTERIX
    char winPath[MAXPATH];
    unixPathToWin(fname,winPath,MAXPATH);
    setString("aipStatGraphFile", winPath, true);
#else
    setString("aipStatGraphFile", fname, true);// Send notification of change
#endif
    setReal("aipStatCursEnable", 0, true); // Turn off cursors
}

/*
 * Writes out a Histogram file that looks sort of like:
 *  Histogram 100
 *  Start 0.0
 *  End 0.005
 *  XLabel Intensity
 *  YLabel Number of pixels
 *  Data
 *  0.00123
 *  0.004567
 *  ...
 */
void
RoiStat::drawHistogram(int *hist, // Array of histogram values
		       int nbins, // # of bins in histogram
		       double min, // Intensity at bottom of first bin
		       double max) // Intensity at top of last bin
{
    static int index = 0;

    // Open file
    char fname[128];
    sprintf(fname,"%s%d.%d", baseFilename.c_str(), getpid(), index);
    ++index;
    index %= 2;
    FILE *fd = fopen(fname, "w");
    if (!fd) {
	fprintf(stderr,"Cannot open file for histogram: %s\n", fname);
	return;
    }

    double ymax = hist[0];
    for (int i=1; i<nbins; i++) {
        if (ymax < hist[i]) {
            ymax = hist[i];
        }
    }

    // Write out header
    fprintf(fd,"Histogram %d\n", nbins);
    fprintf(fd,"XMin %g\n", min);
    fprintf(fd,"XMax %g\n", max);
    fprintf(fd,"YMin 0\n");
    fprintf(fd,"YMax %g\n", ymax);
    fprintf(fd,"XLabel Intensity\n");
    fprintf(fd,"YLabel Number of pixels\n");
    fprintf(fd,"NPoints %d\n", nbins);
    fprintf(fd,"Data\n");

    // Write out the data
    for (int i=0; i<nbins; i++) {
	fprintf(fd,"%d\n", hist[i]);
    }

    fclose(fd);

    // Set parameter pointing to file
#ifdef __INTERIX
    char winPath[MAXPATH];
    unixPathToWin(fname,winPath,MAXPATH); 
    setString("aipStatGraphFile", winPath, true);
#else
    setString("aipStatGraphFile", fname, true);// Send notification of change
#endif
    setReal("aipStatCursEnable", 1, true); // OK to show cursors
}

void
RoiStat::drawScatterplot(double *x, // Abscissas to graph
			 double *y, // The values to graph
			 string xLabel,
			 string yLabel,
			 int nvalues) // The number of values
{
    // Open file
    char fname[128];
    sprintf(fname,"%s%d", baseFilename.c_str(), getpid());
    FILE *fd = fopen(fname, "w");
    if (!fd) {
	fprintf(stderr,"Cannot open file for scatterplot: %s\n", fname);
	return;
    }
    double xmin = x[0];
    double xmax = x[0];
    double ymin = y[0];
    double ymax = y[0];
    for (int i=1; i<nvalues; i++) {
        if (xmin > x[i]) {
            xmin = x[i];
        } else if (xmax < x[i]) {
            xmax = x[i];
        }
        if (ymin > y[i]) {
            ymin = y[i];
        } else if (ymax < y[i]) {
            ymax = y[i];
        }
    }

    // Write out header
    fprintf(fd,"XLabel %s\n", xLabel.c_str());
    fprintf(fd,"YLabel %s\n", yLabel.c_str());
    fprintf(fd,"XMin %g\n", xmin);
    fprintf(fd,"XMax %g\n", xmax);
    fprintf(fd,"YMin %g\n", ymin);
    fprintf(fd,"YMax %g\n", ymax);
    fprintf(fd,"NPoints %d\n", nvalues);
    fprintf(fd,"Data\n");

    // Write out the data
    for (int i=0; i<nvalues; i++) {
	fprintf(fd,"%g %g\n", x[i], y[i]);
    }

    fclose(fd);

    // Set parameter pointing to file
#ifdef __INTERIX
    char winPath[MAXPATH];
    unixPathToWin(fname,winPath,MAXPATH);
    setString("aipStatGraphFile", winPath, true);
#else
    setString("aipStatGraphFile", fname, true);// Send notification of change
#endif
    setReal("aipStatCursEnable", 0, true); // Turn off cursors
}

void
RoiStat::segmentRois(int type)
{
    double minData = getReal("aipStatCursMin",0);
    double maxData = getReal("aipStatCursMax",0);
    bool minFlag = isActive("aipStatCursMin");
    bool maxFlag = isActive("aipStatCursMax");

    bool clear = (type == 'r'); // Clear the image outside the ROI

    // Make a list of gframes w/ selected ROIs
    Roi *roi;
    RoiManager *roim = RoiManager::get();
    set<Gframe *> gfSet;
    for (roi=roim->getFirstSelected(); roi; roi=roim->getNextSelected()) {
	gfSet.insert(roi->pOwnerFrame);
    }
    // Segment each of those gframes
    set<Gframe *>::iterator itr;
    for (itr=gfSet.begin(); itr != gfSet.end(); itr++) {
	(*itr)->segmentSelectedRois(minFlag, minData, maxFlag, maxData, clear);
    }

}

void
RoiStat::segmentImages()
{
    double minData = getReal("aipStatCursMin",0);
    double maxData = getReal("aipStatCursMax",0);
    bool minFlag = isActive("aipStatCursMin");
    bool maxFlag = isActive("aipStatCursMax");

    // Segment each selected gframe
    spGframe_t gf;
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    for (gf=gfm->getFirstSelectedFrame(gfi);
	 gf != nullFrame;
	 gf=gfm->getNextSelectedFrame(gfi))
    {
	gf->segmentImage(minFlag, minData, maxFlag, maxData);
    }
}

/************************************************************************
*									*
*  Write statistics to a file.
*									*/
void
RoiStat::writeStats(const char *fname, const char *mode, int header)
{
    const char title[] = " ROI Statistics:";

    // Open output file
    FILE *fd = fopen(fname, mode);
    if (!fd) {
	fprintf(stderr,"Cannot open file \"%s\" for writing.", fname);
	return;
    }

    RoiDataList statlist;
    RoiData tot_stat;		// Place to gather summary stats
    calcStats(statlist, tot_stat);
    stable_sort(statlist.begin(), statlist.end(), alphaByFilepath);

    RoiDataList::iterator itr;
    RoiData *stats;
    int namelen = strlen(title) + 5; // Length of the longest file name
    int nstats = 0;
    for (itr = statlist.begin(); itr != statlist.end(); ++nstats, ++itr) {
        int len;
        stats = *itr;
        if ((len=strlen(stats->fname)) > namelen) {
            namelen = len;
        }
    }
    if (nstats > 0) {
        // Header line if requested   
        if(header) {
            fprintf(fd,"%s%*s  Pixels ", title, namelen-strlen(title), "Name");
            fprintf(fd,
                    "Area       Min        Max        Median     Mean       Sdv\n");
        }

        // Loop over all Stats in the list
        for (itr = statlist.begin(); itr != statlist.end(); ++itr) {
            stats = *itr;
            if (stats->npixels) {
                if(header) {
                    fprintf(fd,
                        "%*s %7.0f %-11.4g%-11.4g%-11.4g%-11.4g%-11.4g%-11.4g\n",
                        namelen, stats->fname, stats->npixels, stats->area,
                        stats->min, stats->max, stats->median,
                        stats->mean, stats->sdv);
                }
                else {
                    // Do not put filename, just rows of numbers
                    fprintf(fd,
                        "%7.0f   %-11.4g%-11.4g%-11.4g%-11.4g%-11.4g%-11.4g\n",
                        stats->npixels, stats->area,
                        stats->min, stats->max, stats->median,
                        stats->mean, stats->sdv);
                }
            } 
            else {
                if(header) {
                    fprintf(fd,"%*s %7.0f %-11.4g\n",
                            namelen, stats->fname, stats->npixels, stats->area);
                }
                else {
                    // Do not put filename, just rows of numbers
                    fprintf(fd,"%7.0f %-11.4g\n",
                            stats->npixels, stats->area);

                }
            }
        }

        // Summary stat line
        if (statlist.size() > 1 && header){
            fprintf(fd,"Total: Area=%.4g  %s=%.4g",
                    tot_stat.area, tot_stat.vol_label, tot_stat.volume);
            if (tot_stat.npixels){
                fprintf(fd,", Min=%.4g, Max=%.4g, Mean=%.4g, Sdv=%.4g\n",
                        tot_stat.min, tot_stat.max, tot_stat.mean,
                        tot_stat.sdv);
            }else{
                fprintf(fd,"\n");
            }
        }
    }

    // Release statistics
    for (itr=statlist.begin(); itr != statlist.end(); ) {
	delete *itr++;
    }
    statlist.clear();

    fclose(fd);
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <iostream>
#include <algorithm>
using std::swap;
#include <unistd.h>

#include "aipVnmrFuncs.h"
#include "aipStderr.h"
#include "aipVsInfo.h"
#include "aipImgInfo.h"
#include "aipGraphics.h"	// for GRAYSCALE_COLORS
#include "aipGraphicsWin.h"	// for ORIENT_BOTTOM / ORIENT_TOP
#include "aipInterpolation.h"	// for INTERP_REPLICATION

spImgInfo_t nullImg = spImgInfo_t(NULL);

ImgInfo::ImgInfo(spDataInfo_t di, bool overlay) {
    //fprintf(stderr,"ImgInfo(%s)\n", di->getFilepath());/*CMP-*/
    aipDprint(DEBUGBIT_0,"ImgInfo()\n");
    dataInfo = di;
    datastx = datasty = 0;
    colormapID = 4;
    transparency = 0;
    //int width, height, depth;
    //di->getSpatialDimensions(width, height, depth);
    datawd = di->getFast();
    dataht = di->getMedium();
    setColormap();
    int mode = VsInfo::getVsMode();
    switch (mode) {
    case VS_HEADER:
        {
            string cmd = di->getString("vsFunction", "");
            if (cmd.length() == 0) {
                autoVscale();
            } else {
                setVsInfo(cmd);
            }
        }
        break;
    case VS_UNIFORM:
        {
            string cmd = VsInfo::getDefaultVsCommand();
            if (cmd.length() == 0) {
                autoVscale();
            } else {
                setVsInfo(cmd);
            }
        }
        break;
    case VS_GROUP:
    case VS_OPERATE:
        // Real VS will be set later (before displaying)
        // ... see GframeManager::loadImage()
        if(overlay) autoVscale();
        else setVsInfo("");
        break;
    case VS_INDIVIDUAL:
    case VS_DISPLAYED:
    case VS_SELECTEDFRAMES:
    case VS_NONE:
    default:
        autoVscale();
        break;
    }
}

ImgInfo::~ImgInfo() {
    aipDprint(DEBUGBIT_0,"~ImgInfo()\n");
}

// get colormap fullpath (corresponding to .fdf file)
string ImgInfo::getImageCmapPath() {
   if (!dataInfo.get()) {
        return "";
   }
   string fname = dataInfo->getKey();
   fname = fname.substr(0,fname.find_last_of("."))+".cmap";
   fname.replace(fname.find_last_of(" "), 1,"/");
   return fname;
}

// get colormap fullpath for the group (corresponding to .img directory) 
string ImgInfo::getGroupCmapPath() {
   if (!dataInfo.get()) {
        return "";
   }
   string fname = dataInfo->getKey();
   fname = fname.substr(0,fname.find_first_of(" ")) + "/image.cmap";
   return fname;
}

// get appdir colormap
string ImgInfo::getColormapPath(string fname) {
        char str[MAXSTR];
        char path[MAXSTR];
        sprintf(str,"templates/colormap/%s",fname.c_str());
        if(appdirFind("image.cmap", str, path, "", R_OK))
           return string(path);
        else
           return string(fname);
}

// set colormap path and id
// called by aipSetColormap (from color map editor)
void ImgInfo::setColormap(string fname) {
   colormapPath = getColormapPath(fname);
   char path[MAXSTR];
   strcpy(path,colormapPath.c_str());
   setColormapID(open_color_palette(path));
}

// set colormap path and id
// called by constructor 
void ImgInfo::setColormap() {

   string fname = getImageCmapPath();
   if(fname.size() < 1) return; // no image

   if (access(fname.c_str(), R_OK) != 0) {
     fname = getGroupCmapPath();
     if (access(fname.c_str(), R_OK) != 0) {
        fname = getString("colormapName","default");
        colormapPath = getColormapPath(fname);
     } else colormapPath=string(fname);
   } else colormapPath=string(fname);

   char name[MAXSTR];
   strcpy(name,fname.c_str());
   setColormapID(open_color_palette(name));
}

void ImgInfo::setColormapID(int n) {
    if (n >= 0 && n != colormapID) {
        colormapID = n;
        if (vsInfo != nullVs)
            vsInfo->setColormapID(n);
    }
}

void ImgInfo::setTransparency(int value) {
  if(value < 0 || value > 100) value=0;
  transparency = value;
}

void
ImgInfo::autoVscale() {
    if (!dataInfo.get()) {
	return;
    }
    double min, max;
    dataInfo->getMinMax(min, max);
    if (max == min){
	vsInfo = spVsInfo_t(new VsInfo(min-1, min+1, colormapID));
	return;
    }

    bool phaseImage = (dataInfo->getString("type","absval") != "absval");

    const int nbins = 1000;
    int *histogram = new int[nbins];
    for (int i=0; i<nbins; i++){
	histogram[i] = 0;
    }

    int npts = dataInfo->getFast() * dataInfo->getMedium();
    float *data = dataInfo->getData();
    float *end = data + npts;
    // Get the scale factor for converting histogram index to intensity
    float scale  = (nbins - 1) / (max - min);
    float *p;
    int i;
    for (p=data; p<end; p++) {
	i = (int)((*p - min) * scale);
	histogram[i]++;
    }

    int n;
    double percent = getReal("aipVsTailPercentile", 0.1);
    // This is percent, we need a fraction
    percent = percent/100.0;
    // Number of data points to ignore in the tails
    int percentile = (int) (percent * (double) npts);
    if(percentile < 1)
        percentile = 1;
    
    // Determine length of bottom tail
    for (i=n=0; n<percentile && i<nbins; n += histogram[i++]);
    double minVs = min + (i-1) / scale;
    // Determine length of top tail
    for (i=nbins, n=0; n<percentile && i>0; n += histogram[--i]);
    double maxVs = min + i / scale;
    // Clean up bottom limit, if appropriate
    if (minVs > 0 && minVs / maxVs < 0.05){
        minVs = 0;
    }
    // If data is pos/neg (phase images), then put 0 in the middle and
    // use the max excursion from 0 for the max and min
    if(minVs < 0.0 && phaseImage) {
        if(-minVs > maxVs)
            maxVs = -minVs;
        else
            minVs = -maxVs;
    } else if(minVs < 0.0) {
	minVs = 0.0;
    }

    delete[] histogram;
    vsInfo = spVsInfo_t(new VsInfo(minVs, maxVs, colormapID));
    setString("aipVsFunction", getVsCommand(), true);
    //setReal("aipVsDataMax", maxVs, true);
    //setReal("aipVsDataMin", minVs, true);
}   

bool
ImgInfo::getLabFrameImageExtents(double& x1, double& y1, // Posn of first pt
				 double& x2, double& y2) // Posn of last pt
{
    double span[3];
    dataInfo->getSpatialSpan(span);
    double x0 = dataInfo->getLocation(0) - span[0] / 2;
    double y0 = dataInfo->getLocation(1) - span[1] / 2;
    double xscale = span[0] / dataInfo->getFast();
    double yscale = span[1] / dataInfo->getMedium();
    x1 = x0 + datastx * xscale;
    y1 = y0 + datasty * yscale;
    x2 = x1 + datawd * xscale;
    y2 = y1 + dataht * yscale;
    return true;
}

spDataInfo_t
ImgInfo::getDataInfo()
{
    return dataInfo;
}

float *
ImgInfo::getData()
{
    return dataInfo->getData();
}

void
ImgInfo::setVsMin(double min)
{
    if (vsInfo != nullVs) {
        vsInfo->minData = min;
    }
}

void
ImgInfo::setVsMax(double max)
{
    if (vsInfo != nullVs) {
        vsInfo->maxData = max;
    }
}

double
ImgInfo::getVsMin()
{
    return (vsInfo == nullVs) ? 0 : vsInfo->minData;
}

double
ImgInfo::getVsMax()
{
    return  (vsInfo == nullVs) ? 0.01 : vsInfo->maxData;
}

string
ImgInfo::getVsCommand()
{
    return  (vsInfo == nullVs) ? "" : vsInfo->getCommand();
}

double
ImgInfo::getDatastx() {
    return datastx;
}

double
ImgInfo::getDatasty() {
    return datasty;
}

double
ImgInfo::getDatawd() {
    return datawd;
}

double
ImgInfo::getDataht() {
    return dataht;
}

spVsInfo_t
ImgInfo::getVsInfo() {
    return vsInfo;
}

void
ImgInfo::setVsInfo(spVsInfo_t vsi)
{
    if (vsi == nullVs || vsi->getCommand().length() == 0) {
        setVsInfo("");
    } else {
        vsInfo = spVsInfo_t(new VsInfo(vsi));
    }
}

void
ImgInfo::setVsInfo(string cmd)
{
    vsInfo = spVsInfo_t(new VsInfo(cmd, colormapID)); // TODO: error checking
}

bool
ImgInfo::saveVsInHeader()
{
    spDataInfo_t di = getDataInfo();
    spVsInfo_t vsi = getVsInfo();
    //di->setDouble("vsDataMin", vsi->minData);
    //di->setDouble("vsDataMax", vsi->maxData);
    di->setString("vsFunction", getVsCommand());
    return true;
}

/*
 * Return min and max data values in a rectangle.
 */
void
ImgInfo::getMinmax(double x1, double y1, // upper-left corner
		   double x2, double y2, // bottom-right corner
		   double &min, double &max)
{
    // NB: We assume some part of the rect intersects the data
    spDataInfo_t di = getDataInfo();
    if (x1 > x2) {
	swap(x1, x2);
    }
    if (y1 > y2) {
	swap(y1, y2);
    }
    if (x1 < 0) {
	x1 = 0;
    }
    if (y1 < 0) {
	y1 = 0;
    }
    double dwidth = di->getFast();
    double dheight = di->getMedium();
    if (x2 >= dwidth) {
	x2 = dwidth - 1;
    }
    if (y2 >= dheight) {
	y2 = dheight - 1;
    }
    int ix = (int)x1;
    int fx = (int)x2;
    int iy = (int)y1;
    int fy = (int)y2;
    int nx = fx - ix + 1;
    float *data = di->getData();
    int idatawd = (int)dwidth;
    min = max = data[idatawd * iy + ix];	// Init to first point
    float *p;
    for (int y = iy; y <= fy; y++) {
	p = data + idatawd * y + ix;
	for (int i = 0; i < nx; i++, p++) {
	    if (max < *p) {
		max = *p;
	    } else if (min > *p) {
		min = *p;
	    }
	}
    }
}

/*
 * Get min and max data values in image.
 */
void
ImgInfo::getMinmax(double &min, double &max)
{
    getMinmax(0, 0, (int)datawd - 1, (int)dataht - 1, min, max);
}

void
ImgInfo::rot90_header()
{
    spDataInfo_t di = getDataInfo();
    
    // Update currently displayed region (from zooming)
     int i = (int) datastx;
     datastx = datasty;
     datasty = di->getFast() - i - datawd;
     i = (int) datawd;
     datawd = dataht;
     dataht = i;

     di->rot90_header();
}

void
ImgInfo::flip_header()
{
    spDataInfo_t di = getDataInfo();
    
    // Update currently displayed region (from zooming)
    datastx = di->getFast() - datastx - datawd;

    di->flip_header();
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPIMGINFO_H
#define AIPIMGINFO_H

#include "sharedPtr.h"

#include "aipDataInfo.h"
#include "aipVsInfo.h"

class ImgInfo
{
public:

    ImgInfo(spDataInfo_t, bool overlay=false);
    ~ImgInfo();

    bool getLabFrameImageExtents(double& x1, double& y1,
				 double& x2, double& y2);
    spDataInfo_t getDataInfo();
    float *getData();/*DEPRECATED*/
    double getDatastx();	// Location on data of first displayed pixel
    double getDatasty();
    double getDatawd();		// Width of displayed data in data points
    double getDataht();
    void getMinmax(double x1, double y1, // upper-left corner
		   double x2, double y2, // bottom-right corner
		   double &min, double &max);
    void getMinmax(double &min, double &max);

    void setVsMin(double min);
    void setVsMax(double max);
    double getVsMin();
    double getVsMax();
    string getVsCommand();
    spVsInfo_t getVsInfo();
    void setVsInfo(spVsInfo_t vsi);
    void setVsInfo(string cmd);
    bool saveVsInHeader();
    bool getVsFromHeader();
    void autoVscale();

    void rot90_header();
    void flip_header();

    /*
     * The area of the data space that is mapped onto the screen.
     * These are not integers because we can show fractions of a data
     * point at the edges of the image.  This can be necessary to
     * properly register overlaid images and is also useful when
     * displaying at a very large scale (many pixels per data point),
     * particularly if interpolation is used, which increases the
     * apparent resolution.
     *
     * The upper left corner of the first data point has coordinates
     * (0,0).  Its lower right corner is at (1,1).  For 1-d data,
     * datasty = 0, and dataht = 1.  A particular data coordinate,
     * (datax, datay) is displayed at:
     *   x = (int)(0.5 + pixstx + (datax - datastx) * (pixwd - 1) / datawd)
     *   y = (int)(0.5 + pixsty + (datay - datasty) * (pixht - 1) / dataht)
     * This looks right because:
     *   datax=datastx ==> x = (int)(pixstx + 0.5)
     *   datax=datastx+datawd ==> x = (int)(pixstx + pixwd - 0.5)
     * so the start of the data window is exactly on the first pixel,
     * and the end of the data window is on the last pixel.
     */
    double datastx, datasty;	// Coordinates on data of upper-left corner
    double datawd, dataht;	// Width and height of image in data points

    void setColormap(string name);
    void setColormapID(int value);
    int getColormapID() {return colormapID;}
    string getColormapPath() {return colormapPath;}
    string getImageCmapPath();
    string getGroupCmapPath();
    string getColormapPath(string fname);

    void setTransparency(int value);
    int getTransparency() {return transparency;}

private:
    spDataInfo_t dataInfo;
    
    // Vertical scaling:
    spVsInfo_t vsInfo;
    int colormapID;
    int transparency;
    string colormapPath;
    void setColormap();

};

typedef boost::shared_ptr<ImgInfo> spImgInfo_t;
extern spImgInfo_t nullImg;

#endif /* AIPIMGINFO_H */

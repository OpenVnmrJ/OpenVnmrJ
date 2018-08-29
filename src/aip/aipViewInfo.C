/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <sys/time.h>

#include <stdio.h>
#include <cmath>
using std::sqrt;
using std::fabs;

#include "aipVnmrFuncs.h"
#include "aipStderr.h"
#include "sharedPtr.h"
#include "aipGraphicsWin.h"
#include "aipImgInfo.h"
#include "aipInterface.h"
#include "aipInterpolation.h"
#include "aipWinRotation.h"
#include "aipViewInfo.h"
#include "aipVolData.h"

int ViewInfo::vsRadius = 4;
spViewInfo_t nullView = spViewInfo_t(NULL);

ViewInfo::ViewInfo(spImgInfo_t img)
{
    aipDprint(DEBUGBIT_0,"ViewInfo()\n");
    imgInfo = img;
    rotation = WinRotation::calcRotation(img->getDataInfo());
    rebinnedData.data = NULL;
    rebinnedData.width = 0;
    rebinnedData.height = 0;
    histogram = NULL;
    cm_off[0]=0;
    cm_off[1]=0;
    cm_off[2]=0;
    pixstx_off=0;
    pixsty_off=0;
}

ViewInfo::~ViewInfo()
{
    aipDprint(DEBUGBIT_0,"~ViewInfo()\n");
    if (rebinnedData.data) delete[] rebinnedData.data;
    rebinnedData.data = NULL;
    if (histogram) delete histogram;
    histogram = NULL;
}

void
ViewInfo::print(double scale)
{
    spDataInfo_t dataInfo = imgInfo->getDataInfo();
    spVsInfo_t vsOld = imgInfo->getVsInfo();
    spVsInfo_t vsNew = spVsInfo_t(new VsInfo(vsOld, 256)); // Colormap 256 long
    if(vsNew == (spVsInfo_t)NULL) return;

    aipPrintImage(dataInfo->getData(),
		  dataInfo->getFast(),
		  dataInfo->getMedium(),
		  (int)imgInfo->getDatastx(),//TODO: doubleize
		  (int)imgInfo->getDatasty(),//TODO: doubleize
		  (int)imgInfo->getDatawd(),//TODO: doubleize
		  (int)imgInfo->getDataht(),//TODO: doubleize
		  (int)(pixwd * scale),
		  (int)(pixht * scale),
		  vsNew,
		  INTERP_REPLICATION);
}

XID_t
ViewInfo::draw()
{
    struct timeval tv1, tv2, tv3;
    if (isDebugBit(DEBUGBIT_5)) {
	gettimeofday(&tv1, 0);
    }

    // Rebin data to fit the display space
    if (!updateViewData()) {
	return (XID_t) 0;		// Failed
    }

    bool isVsValid = imgInfo->getVsCommand().length() != 0;
    if (!isVsValid) {
        return (XID_t) 0;
    }

    if (isDebugBit(DEBUGBIT_5)) {
	gettimeofday(&tv2, 0);
    }

    // Put the image on the screen
    XID_t rtn = (XID_t)aipDisplayImage(rebinnedData.data,
				       pixstx_off, pixsty_off,
				       pixwd, pixht,
				       GRAYSCALE_COLORS/*CMSINDEX*/,
				       imgInfo->getVsInfo(),
				       imgInfo->getColormapID(),
				       imgInfo->getTransparency(),
				       true);

    // TODO: Put image in canvas backup pixmap?

    if (isDebugBit(DEBUGBIT_5)) {
	gettimeofday(&tv3, 0);
	double dt;
	dt = tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec) * 1e-6;
	fprintf(stderr,"UpdateView: %g, ", dt*1e3);
	dt = tv3.tv_sec - tv2.tv_sec + (tv3.tv_usec - tv2.tv_usec) * 1e-6;
	fprintf(stderr,"Draw: %g, ", dt*1e3);
	dt = tv3.tv_sec - tv1.tv_sec + (tv3.tv_usec - tv1.tv_usec) * 1e-6;
	fprintf(stderr,"Total time: %g ms\n", dt*1e3);
    }

    return rtn;
}

rebin_t
ViewInfo::chooseRebinMethod(double numInputBins, int numOutputBins)
{
    double expansion = numOutputBins / numInputBins;
    if (expansion < 2) {
	return &boxcarRebin;
    }
    int quality = (int)getReal("aipInterpolationQuality", 1);
    if (expansion < 2) {
        quality = 0;
    }
    /*if (quality > 1 && numInputBins < 3) {
        quality = 1;
    }
    if (quality == 1 && numInputBins < 3) {
        quality = 0;
    }*/
    if (quality == 0) {
	return &boxcarRebin;
    } else if (quality == 1) {
	return &linearExpand;
    } else {
	return &biParabolicExpand;
    }
}

void
ViewInfo::freeViewData()
{
	if(rebinnedData.data != NULL) delete[] rebinnedData.data;
	rebinnedData.data = NULL;
	rebinnedData.width = 0;
        rebinnedData.height = 0;
}

bool
ViewInfo::updateViewData()
{
    rebin_t rebin;

    if (!viewDataOOD()) {
	return true;		// It's already OK
    }

    // Info about the full data array
    spDataInfo_t di = imgInfo->getDataInfo();
    float *data = di->getData();
    
    if(data==0)
    	return false;
    
    int dataWidth = di->getFast();
    int dataHeight = di->getMedium();
    bool phaseImage = (di->getString("type","absval") != "absval");

    // Info about the part of the data array we are imaging
    double dx = rebinnedData.datastx = imgInfo->datastx;
    double dy = rebinnedData.datasty = imgInfo->datasty;
    double dwd = rebinnedData.datawd = imgInfo->datawd;
    double dht = rebinnedData.dataht = imgInfo->dataht;
    rebinnedData.interpolation = (int)getReal("aipInterpolationQuality", 1);

    // The data rebinning occurs in two steps; first in
    // the data X direction, and then in the data Y direction.  Any
    // transposing, and flipping is done in the second step.

    // Calc how many rows to usefully keep for second stage
    int idy;			// Initial data row
    int fdy;			// Final data row
    int ndy;			// Number of data rows to use
    idy = dy < 2 ? 0 : (dy >= dataHeight ? dataHeight - 3 : (int)dy - 2);
    fdy = (int)(dy + dht);
    fdy = fdy < 2 ? 0 : (fdy >= dataHeight - 3 ? dataHeight - 1 : fdy + 2);
    ndy = fdy - idy + 1;

    int outlen = rotation & 4 ? pixht : pixwd;
    float *ibuf = new float[ndy * outlen]; // For intermediate results

    // Choose interpolation method for data rows
    rebin = chooseRebinMethod(dwd, outlen);

    // Rebin data rows to pix rows or columns
    float *indata = data + idy * dataWidth;
    float *outdata = ibuf;
    for (int i=0; i<ndy; ++i, indata += dataWidth, outdata += outlen) {
	(*rebin)(indata, dataWidth, 1, dx, dwd, outdata, outlen, 1);
    }

    // Now rebin the other dimension
    // The rebinnedData.data array holds the smoothed/rebinned/rotated data.
    // It has the same dimensions as the image pixel data (pixwd by pixht),
    // but holds the full precision data.
    if (rebinnedData.data == NULL
        || rebinnedData.width * rebinnedData.height != pixwd * pixht)
    {
	if(rebinnedData.data != NULL) delete[] rebinnedData.data;
	rebinnedData.data = new float[pixwd * pixht];
	rebinnedData.width = pixwd;
        rebinnedData.height = pixht;
    }
    if (rebinnedData.data == NULL) {
        fprintf(stderr,
                "ViewInfo::updateViewData(): out of memory getting %d bytes\n",
                pixwd * pixht * sizeof(float));
	delete[] ibuf; 
        return false;
    }

    // Rebin data columns to pix columns or rows
    indata = ibuf;
    int inwd = outlen;		// Read down columns of intermediate buf
    int outinc;
    int outinc2;
    outdata = rebinnedData.data;
    if (rotation & 4) {
	outlen = pixwd;
	outinc = 1;
	outinc2 = pixwd;
	if (rotation & 2) {
	    outdata += (pixht - 1) * pixwd; // Beginning of last row
	    outinc2 = -outinc2;
	}
	if (rotation & 1) {
	    outdata += pixwd - 1; // End of row
	    outinc = -outinc;
	}
    } else {
	outlen = pixht;
	outinc = pixwd;
	outinc2 = 1;
	if (rotation & 2) {
	    outdata += (pixht - 1) * pixwd; // Beginning of last row
	    outinc = -outinc;
	}
	if (rotation & 1) {
	    outdata += pixwd - 1; // End of row
	    outinc2 = -outinc2;
	}
    }
    rebinnedData.rotation = rotation;

    // Choose interpolation method for data columns
    rebin = chooseRebinMethod(dht, outlen);

    for (int i=0; i<inwd; ++i, indata += 1, outdata += outinc2) {
	(*rebin)(indata, ndy, inwd, dy-idy, dht, outdata, outlen, outinc);
    }

    if (histogram) delete histogram;
    histogram = new Histogram(100, rebinnedData.data,
                              rebinnedData.width * rebinnedData.height, phaseImage);

    delete[] ibuf;
    return true;
}

void
ViewInfo::setViewDataOOD(bool flag)
{
    if (flag) {
        if (rebinnedData.data != NULL) {
            delete[] rebinnedData.data;
            rebinnedData.data = NULL;
        }
    }
}

bool
ViewInfo::viewDataOOD()
{
    if (rebinnedData.data == NULL ||
        rebinnedData.datastx != imgInfo->datastx ||
        rebinnedData.datasty != imgInfo->datasty ||
        rebinnedData.datawd != imgInfo->datawd ||
        rebinnedData.dataht != imgInfo->dataht ||
        rebinnedData.dataht != imgInfo->dataht ||
        rebinnedData.width != pixwd ||
        rebinnedData.height != pixht ||
        rebinnedData.rotation != rotation ||
        rebinnedData.interpolation != (int)getReal("aipInterpolationQuality", 1)
        )
    {
        return true;
    } else {
        return false;
    }
}

int
ViewInfo::getRotation()
{
    return rotation;
}

void
ViewInfo::setRotation(int newRotation)
{
    rotation = newRotation;
}

bool
ViewInfo::pointIsOnImage(int x, int y)
{
    return (pixstx <= x && x < pixstx + pixwd &&
	    pixsty <= y && y < pixsty + pixht);
}

void
ViewInfo::setVs(int pixX, int pixY, bool doMax)
{
    double x1, y1, x2, y2;
    pixToData(pixX-vsRadius, pixY-vsRadius, x1, y1);
    pixToData(pixX+vsRadius, pixY+vsRadius, x2, y2);
    double min, max;
    imgInfo->getMinmax(x1, y1, x2, y2, min, max);
    if (doMax) {
	imgInfo->setVsMax(max);
    } else {
	imgInfo->setVsMin(min);
    }
    setReal("aipVsDataMax", imgInfo->getVsMax(), true);
    setReal("aipVsDataMin", imgInfo->getVsMin(), true);
    setString("aipVsFunction", imgInfo->getVsCommand(), true);
}

void
ViewInfo::pixToData(double pixX, double pixY, double& dataX, double& dataY)
{
    /*
     * ( dx )   ( m00  m01  m02 )   ( px )
     * ( dy ) = ( m10  m11  m12 ) * ( py )
     * ( 1  )   (  0    0    1  )   ( 1  )
     *
     * where: m == p2d
     */
    dataX = pixX * p2d[0][0] + pixY * p2d[0][1] + p2d[0][2];
    dataY = pixX * p2d[1][0] + pixY * p2d[1][1] + p2d[1][2];
}

void
ViewInfo::dataToPix(double dataX, double dataY, double& pixX, double& pixY)
{
    /*
     * ( px )   ( m00  m01  m02 )   ( dx )
     * ( py ) = ( m10  m11  m12 ) * ( dy )
     * ( 1  )   (  0    0    1  )   ( 1  )
     *
     * where: m == d2p
     */
    pixX = dataX * d2p[0][0] + dataY * d2p[0][1] + d2p[0][2];
    pixY = dataX * d2p[1][0] + dataY * d2p[1][1] + d2p[1][2];

}

void
ViewInfo::updateScaleFactors()
{
    // d2p is a 2D affine transformation
    // We assume no "oblique" rotations for now
    // Orientation is given by the "rotation" flag: 0 <= rotation <= 7
    // The rotation flag's bits have the following meanings:
    //  Bit 0: Rows reversed (drawn right-to-left)
    //  Bit 1: Columns reversed (drawn bottom-to-top)
    //  Bit 2: Transpose x/y
    double pwd = pixwd - 1;
    double pht = pixht - 1;
    double dwd = imgInfo->getDatawd();
    double dht = imgInfo->getDataht();
    if (rotation & 4) {
	// Transpose
	d2p[0][0] = d2p[1][1] = 0;
	d2p[0][1] = rotation & 1 ? -pwd/dht : pwd/dht;
	d2p[1][0] = rotation & 2 ? -pht/dwd : pht/dwd;
    } else {
	d2p[0][1] = d2p[1][0] = 0;
	d2p[0][0] = rotation & 1 ? -pwd/dwd : pwd/dwd;
	d2p[1][1] = rotation & 2 ? -pht/dht : pht/dht;
    }
    double px0 = rotation & 1 ? pixstx + pixwd - 1 : pixstx;
    double py0 = rotation & 2 ? pixsty + pixht - 1 : pixsty;
    double dx0 = imgInfo->getDatastx();
    double dy0 = imgInfo->getDatasty();
    d2p[0][2] = px0 - d2p[0][0] * dx0 - d2p[0][1] * dy0 + 0.5;
    d2p[1][2] = py0 - d2p[1][0] * dx0 - d2p[1][1] * dy0 + 0.5;
    d2p[2][0] = 0;
    d2p[2][1] = 0;
    d2p[2][2] = 1;

    // Inverse calculation for "p2d" is good for any affine transformation.
    // NB: "det" is guaranteed non-zero for non-zero size image.
    double det = d2p[0][0] * d2p[1][1] - d2p[0][1] * d2p[1][0];
    p2d[0][0] = d2p[1][1] / det;
    p2d[0][1] = -d2p[0][1] / det;
    p2d[0][2] = (d2p[0][1] * d2p[1][2] - d2p[0][2] * d2p[1][1]) / det;
    p2d[1][0] = -d2p[1][0] / det;
    p2d[1][1] = d2p[0][0] / det;
    p2d[1][2] = (d2p[0][2] * d2p[1][0] - d2p[0][0] * d2p[1][2]) / det;
    p2d[2][0] = 0;
    p2d[2][1] = 0;
    p2d[2][2] = 1;

    // make p2m and m2p
    int i, j, k;

    spDataInfo_t di = imgInfo->getDataInfo();

    // Matrix multiply to get pixel-to-magnet conversion
    double (*d2m)[3][3] = &(di->d2m); 

    // Make 3D versions of above matrices
    double pd[4][4];
    pd[0][0] = p2d[0][0];
    pd[0][1] = p2d[0][1];
    pd[0][2] = 0;
    pd[0][3] = p2d[0][2];
    pd[1][0] = p2d[1][0];
    pd[1][1] = p2d[1][1];
    pd[1][2] = 0;
    pd[1][3] = p2d[1][2];
    pd[2][0] = pd[2][1] = 0;
    if(pixelsPerCm == 0) pixelsPerCm = 1;
    if (pd[0][0] * pd[1][1]- pd[0][1] * pd[1][0]< 0) {
        pd[2][2] = -1 / pixelsPerCm; // Preserve right-handed coordinate system
    } else {
        pd[2][2] = 1 / pixelsPerCm;
    }
    pd[2][3] = 0;
    pd[3][0] = pd[3][1] = pd[3][2] = 0;
    pd[3][3] = 1;

    double orientation[9];
    if (di->getOrientation(orientation) != 9) {
        return;
    }
    double dcos[3][3];
    for (i=0, j=0; j<3; j++) {
        for (k=0; k<3; k++, i++) {
            dcos[k][j] = orientation[i]; // Load TRANSPOSE of rotation matrix
        }
    }
    double dm[4][4];
    dm[0][0] = (*d2m)[0][0];
    dm[0][1] = (*d2m)[0][1];
    dm[0][2] = dcos[0][2];
    dm[0][3] = (*d2m)[0][2];
    dm[1][0] = (*d2m)[1][0];
    dm[1][1] = (*d2m)[1][1];
    dm[1][2] = dcos[1][2];
    dm[1][3] = (*d2m)[1][2];
    dm[2][0] = (*d2m)[2][0];
    dm[2][1] = (*d2m)[2][1];
    dm[2][2] = dcos[2][2];
    dm[2][3] = (*d2m)[2][2];
    dm[3][0] = dm[3][1] = dm[3][2] = 0;
    dm[3][3] = 1;

    for (i=0; i<4; i++) {
        for (j=0; j<4; j++) {
            p2m[i][j] = 0;
            for (k=0; k<4; k++) {
                p2m[i][j] += dm[i][k] * pd[k][j];
            }
        }
    }

    // TESTS
    /* Print out p2m */
    if (isDebugBit(DEBUGBIT_3)) {
        char *pc;
        int i, j;
        if (!di->st->GetValue("file", pc) &&!di->st->GetValue("filename", pc)) {
            pc = (char *)"Mystery file";
        }
        fprintf(stderr,"p2m matrix of %s:\n", pc);
        for (i=0; i<4; i++) {
            fprintf(stderr,"  {");
            for (j=0; j<3; j++) {
                fprintf(stderr,"%6.3f, ", p2m[i][j]);
            }
            fprintf(stderr,"%6.3f}\n", p2m[i][j]);
        }
    }

    // Now invert p2m
    double s[3]; // scale factors for x, y, z
    for (i=0; i<3; i++) {
        s[i] = (p2m[0][i] * p2m[0][i]+p2m[1][i] * p2m[1][i]+p2m[2][i]
                * p2m[2][i]);
    }
    for (i=0; i<3; i++) {
        for (j=0; j<3; j++) {
            m2p[i][j] = p2m[j][i] / s[i];
        }
    }
    for (i=0; i<3; i++) {
        m2p[i][3] = 0;
        for (j=0; j<3; j++) {
            m2p[i][3] -= m2p[i][j] * p2m[j][3];
        }
    }
    m2p[3][0] = m2p[3][1] = m2p[3][2] = 0;
    m2p[3][3] = 1;

    // TESTS
    /*
     double ck4[4][4];
     for (i=0; i<4; i++) {
     for (j=0; j<4; j++) {
     ck4[i][j] = 0;
     for (k=0; k<4; k++) {
     ck4[i][j] += m2p[i][k] * p2m[k][j];
     }
     fprintf(stderr,"%17.10g", ck4[i][j]);
     if (j==3) {
     fprintf(stderr,"\n");
     } else {
     fprintf(stderr,"  ");
     }
     }
     }
     */

    // make m2u and u2m

    double c[3];
    if(di->getLocation(c) != 3) return;

    // reverse x
    orientation[0]=-orientation[0];
    orientation[1]=-orientation[1];
    orientation[2]=-orientation[2];

    for(j=0; j<3; j++) {
            m2u[j][3] = -c[j];
            for(k=0; k<3; k++) {
                m2u[j][k] = orientation[j*3+k];
                u2m[k][j] = orientation[j*3+k];
            }
    }
    
    for(i=0; i<3; i++) {
        u2m[i][3] = 0.0;
        for(j=0; j<3; j++) {
            u2m[i][3] += c[j]*orientation[j*3+i];
        }
    }
    
    // reverse x back
    orientation[0]=-orientation[0];
    orientation[1]=-orientation[1];
    orientation[2]=-orientation[2];

    return;
}

void ViewInfo::pixToMagnet(double px, double py, double pz, double& mx, double& my, double& mz) {
  
    int i, j;
    double p[3];
    double m[3];
    p[0] = px;
    p[1] = py;
    p[2] = pz;

    for (i=0; i<3; i++) {
        m[i] = p2m[i][3];
        for (j=0; j<3; j++) {
            m[i] += p2m[i][j] * p[j];
        }
    }
    mx = m[0];
    my = m[1];
    mz = m[2];
}

void ViewInfo::magnetToPix(double mx, double my, double mz, double &px, double &py, double &pz) {

    int i, j;
    double p[3];
    double m[3];
    m[0] = mx;
    m[1] = my;
    m[2] = mz;

    for (i=0; i<3; i++) {
        p[i] = m2p[i][3];
        for (j=0; j<3; j++) {
            p[i] += m2p[i][j] * m[j];
        }
    }
    
    px = p[0];
    py = p[1];
    pz = p[2];
}

void ViewInfo::pixToUser(double px, double py, double pz, double& ux, double& uy, double& uz) {
  
    int i, j;
    double p[3];
    double m[3];
    p[0] = px;
    p[1] = py;
    p[2] = pz;

    for (i=0; i<3; i++) {
        m[i] = p2m[i][3];
        for (j=0; j<3; j++) {
            m[i] += p2m[i][j] * p[j];
        }
    }

    for (i=0; i<3; i++) {
        p[i] = 0;
        for (j=0; j<3; j++) {
            p[i] += m2u[i][j] * m[j];
        }
    }
    
    ux = p[0];
    uy = p[1];
    uz = p[2];
}

void ViewInfo::userToPix(double ux, double uy, double uz, double &px, double &py, double &pz) {

    int i, j;
    double p[3];
    double u[3];
    double m[3];
    u[0] = ux;
    u[1] = uy;
    u[2] = uz;

    for (i=0; i<3; i++) {
        m[i] = 0;
        for (j=0; j<3; j++) {
            m[i] += u2m[i][j] * u[j];
        }
    }

    for (i=0; i<3; i++) {
        p[i] = m2p[i][3];
        for (j=0; j<3; j++) {
            p[i] += m2p[i][j] * m[j];
        }
    }
    
    px = p[0];
    py = p[1];
    pz = p[2];

}

// Clip a point's coordinates to keep it inside the image data
void
ViewInfo::keepPointInData(double& x, double& y)
{
    double dataX, dataY;
    //double pixX, pixY;

	    pixToData(x, y, dataX, dataY);
    spDataInfo_t di = imgInfo->getDataInfo();
    if (dataX <= 0) {
	dataX = 1e-3;
    } else if (dataX >= di->getFast()) {
	dataX = di->getFast() - 1e-3;
    }
    if (dataY <= 0) {
	dataY = 1e-3;
    } else if (dataY >= di->getMedium()) {
	dataY = di->getMedium() - 1e-3;
    }
    dataToPix(dataX, dataY, x, y);
    //x = (int)(pixX + 0.5);
    //y = (int)(pixY + 0.5);
}

bool ViewInfo::isOblique() {
    int i;
    double d = 0.0;
    for (i=0; i<3; i++) {
	d += (fabs(m2p[0][i]*m2p[1][i]) + fabs(m2p[0][i]*m2p[2][i]) + fabs(m2p[1][i]*m2p[2][i]));
    }
    if(fabs(d) < 1.0e-4) return false;
    else return true;
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* %Z%%M% %I% %G% Copyright (c) 1991-1996 Varian Assoc.,Inc. All Rights Reserved
 */

#ifndef CGLVIEW_H_
#define CGLVIEW_H_

#include "Volume.h"

class CGLView
{
    double P[16];
    double M[16];
    double R[16];
    int V[4];
    double width,height;
    bool invalid;
    bool reset;
    bool resized;

    double xoffset, yoffset, zoffset;
    double sx,sy,sz;
    double xcenter;
    double aspect;
    double rotx, roty, rotz,last_rotx,last_roty,last_rotz;
    int projection;
    int sliceplane;
    int np,traces,slices;
    int slice,maxslice,numslices;
    int data_type;
    double ymin,ymax;
    int options;
    Point3D eye_vector;
    Point3D slice_vector;

protected:

    void setEyeVector();
    void getModelViewMatrix();
    void getProjectionMatrix();
    void getViewport();

public:
    double delx,dely;
    double ascale,xscale,yscale,zscale,dscale;
    double tilt, twist;
    Point3D yaxis,xaxis,vrot;
    Volume  *volume;

    CGLView(Volume *vol);
    void setView();
    void init();
	void setOptions(int opts);
	void setDataPars(int n, int t, int s, int d);
	void setDataScale(double mn, double mx);
	void setScale(double a,double x, double y, double z);
	void setOffset(double x, double y, double z);
	void setSpan(double x, double y, double z);
	void setRotation3D(double x, double y, double z);
	void setRotation2D(double x, double y);
	void setSlant(double x,double y);
	int maxSlice() { return maxslice;}
	Point3D eyeMinZ();
	Point3D eyeMaxZ();
	double planeDist(Point3D pt, Point3D plane);
	int boundsPts(Point3D plane, Point3D *returnPts);
    int slicePts(Point3D plane, Point3D *returnPts);
	void MatrixMulVector(double *m ,double *v, double *c);
	double getXProjection(double x, double y, double z);
	double getYProjection(double x, double y, double z);
	double getZProjection(double x, double y, double z);
	double getDscale() { return dscale;}
	Point3D project(Point3D p);
	Point3D unproject(Point3D p);
	Point3D project(double x, double y, double z);
	Point3D unproject(double x, double y, double z);
    Point3D screenToRotation(double x, double y, double z);
	Point3D eyeVector();
	bool sliceIsFrontFacing();
	bool sliceIsEyeplane();
    void setReset();
    void setResized();
    void setInvalid();

    int sliceSize();
    double sliceDepth(Point3D p);
    Point3D sliceMaxPoint();
    Point3D sliceMinPoint();
    Point3D eyeMaxPoint();
    Point3D eyeMinPoint();
    void pickMatrix(int x, int y,int pts);
    void setSlice(int slc, int max, int num);
    void setSliceVector(double x, double y, double z);
    void pushRotationMatrix();
    Point3D matrixMultiply(Point3D v);
    Point3D testBounds(Point3D start, Point3D end);

};

#endif

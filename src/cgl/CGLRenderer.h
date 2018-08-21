/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
#ifndef CGLRENDERER_H_
#define CGLRENDERER_H_

#include <math.h>

#include "CGLView.h"
#include "CGLTexMgr.h"
#include "CGLDataMgr.h"
class CGLRenderer : public CGLDataMgr
{
protected:
    double aspect;
    double height,width;
    int show;
    int mode;
    double intensity;
    double contrast;
    double bias;
    double threshold;
    double limit;
    double transparency;
    double contours;
    double alphascale;
    bool phasing;
    bool revslices;
    bool newtexture;
    int next_trc;
    int max_trc;

    int nclipped;
    int ntotal;
    double P[16];

    float *colors;
    float stdcols[NUMSTDCOLS*4];
    float abscols[NUMABSCOLS*4];
    float phscols[NUMPHSCOLS*4];
    float ctrcols[NUMCTRCOLS*4];
    float grays[NUMGRAYS*4];

    double yrange;
    double ybase;
    bool hide;
    bool usebase;
    bool select_mode;

    int maxslice;
    int maxtrace;
    int numslices;
    int numtraces;
    bool csplineInvalid;

    CGLTexMgr *texmgr;
    CGLTex1DMgr *tex1D;
    CGLTex2DMgr *tex2D;
    CGLTex3DMgr *tex3D;

    Volume *volume;
    CGLView *view;

    int maxslices;

    int num2Dtexs;
    int size2Dtexs;

    bool newTexMap;
    bool newTexMode;
    bool newLayout;
    bool newShader;
    bool useLighting;

    float cspline[NUMCOLORS*4];
    int last_trc;

    int ncolors;
    int palette;
    int shader;
    Point3D svect;
	void draw1D(int slc, int trc, int options);
	void draw2D(int s, int options);
	float *getStdColor(int i);
	void setStdColor(int i);
	void setColor(float *array, int i);
	int setBits(int w, int m, int b) {
        int r=w&(~m);
        return r|b;
    }
	void set1DTexture(int);
	void renderVertex(Point3D p, int options);
	void renderVertex(int k, int j, int i, int options);
    void makeCSpline(int options);
    bool selecting();

	int setBit(int w, int b) {
        return w|b;
    }
	int clrBit(int w, int b) {
        return w&(~b);
    }
    bool bitIsSet(int i, int b){
        if((i&b)==0)
            return false;
        return true;
    }

public:
	CGLRenderer();
	~CGLRenderer();

	void setDataScale(double mn, double mx);
    void setDataPars(int n, int t, int s, int dtype);
    void setDataPtr(float *data);
    void setDataMap(char *path);
    float *getDataPtr() { return vertexData;}
    float *getDataInfo(int *size);
    void init(int s);
	void resize(int w, int h);
	void setPhase(double r, double l);
	void setColorArray(int id, float* data, int n);
	void setOptions(int indx, int s);
	void render(int f);
	void setScale(double a,double x, double y, double z);
	void setSpan(double x, double y, double z);
    void setOffset(double x, double y, double z);
    void setRotation3D(double x, double y, double z);
    void setRotation2D(double x, double y);
    void setObjectRotation(double x, double y, double z);
    void setTrace(int i,int max, int num);
    void setSlice(int i,int max, int num);
    void setStep(int i);
    void setSlant(double x,double y);
    void setIntensity(double x);
    void setBias(double x);
    void setContrast(double x);
    void setThreshold(double x);
    void setContours(double x);
    void setLimit(double x);
    void setTransparency(double x);
    void setAlphaScale(double x);
    void setSliceVector(double x, double y, double z,double w);
    void render2DPoint(int pt, int trc, int dtype);
};


#endif /*CGLRENDERER_H_*/

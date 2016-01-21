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

#ifndef CGLTEXMGR_H_
#define CGLTEXMGR_H_

#include "CGLDef.h"
#include "CGLView.h"
#include "CGLDataMgr.h"
#include "CGLProgram.h"

class CGLTexMgr
{
protected:
    double threshold,limit,intensity,contrast,bias;
    double contours,transparency,alphascale;
    float *colormap;
    int color_mode;
    float bgcol[4];
    float gridcol[4];
    int palette;
    float ymin,ymax;
    int ncolors;
    int contrastmap_length;
    CGLView *view;
    CGLDataMgr *data;
    GLuint textureIds[3];
    int slice;
    float scale;
    bool use_shaders;
    bool colormap_invalid;
    bool functionmap_invalid;
    int shader_type;
    
    float params[4];
    float params2[4];
    float functionmap[CVSIZE*2];
    
    bool clip_low,clip_high;
 
    CGLProgram *shader;
    bool use_shader;

    void makeFunctionMap();
    void setVertexData();
	int texColor(double v,int options);
	void makeColorMapTexture(int options,int flags);
	void makeFunctionMapTexture(int options,int flags);
public: 
	CGLTexMgr(CGLView *,CGLDataMgr *);
    
	void setUseShader(bool b);

    double getColorValue(double s);
    double getAlphaValue(double s);
    
	virtual ~CGLTexMgr();
    virtual void setShaderType(int s) {shader_type=s;}
    virtual void setColorTable(float *t, int size, int mode);
    virtual void setBgColor(float *t){
    	for(int i=0;i<4;i++)
    		bgcol[i]=t[i];
    }
    virtual void setGridColor(float *t){
    	for(int i=0;i<4;i++)
    		gridcol[i]=t[i];
    }
    virtual void setContrast(double r);

    virtual void setThreshold(double r);
    virtual void setIntensity(double r);
    virtual void setBias(double r);
    virtual void setLimit(double r);
    virtual void setTransparency(double r);
    virtual void setContours(double r);
    virtual void setAlphaScale(double r);

    virtual void setDataScale(float mn, float mx);
    virtual void invalidate();
    virtual void drawSlice(int slc, int options);
    virtual void init(int opts,int flgs);
    virtual void finish();
    virtual void free();
    virtual void beginShader(int slc, int options){}
    virtual void endShader(){}
};

class CGLTex1DMgr : public CGLTexMgr
{
private:
public:
	CGLTex1DMgr(CGLView *v,CGLDataMgr *d);
	~CGLTex1DMgr();
    void init(int opts,int flgs);
    virtual void beginShader(int slc, int options);
    virtual void endShader();
    void finish();
};

class CGLTex2DMgr : public CGLTexMgr
{
private:
    GLubyte **tex2DByteBuffer;
    GLint   **tex2DIntBuffer;
    int buffer_length;
    GLuint *sliceIds;
    
    int num2Dtexs;
    int size2Dtexs;
    Point3D min;
    Point3D max;
    double delta;       
    double vertexValue(int k, int j, int i, int options);
    void make2DTexture(int options,int flags);
    void allocate();
public:
	CGLTex2DMgr(CGLView *v,CGLDataMgr *d);
	~CGLTex2DMgr();
    void invalidate();
    void drawSlice(int slc, int options);
    void init(int opts,int flgs);
    void finish();
    void free();    	
};

class CGLTex3DMgr : public CGLTexMgr
{
private:
	GLubyte *tex3DByteBuffer;
	GLint   *tex3DIntBuffer;
	
    void make3DTexture(int options,int flags);
	Point3D min;
    Point3D max;
    double delta;		
public:
	CGLTex3DMgr(CGLView *v,CGLDataMgr *d);
   ~CGLTex3DMgr();
    void invalidate();
    void drawSlice(int slc, int options);
    void init(int opts,int flgs);
    void finish();
    void free();
};

#endif // CGLTEXMGR_H_

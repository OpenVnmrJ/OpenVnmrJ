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

#include <math.h>
#include "CGLTexMgr.h" // C++ class header file
#include <fcntl.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/mman.h>
#include <iostream>

#define DEBUG_MEM
//-------------------------------------------------------------
// CGLTexMgr::CGLTexMgr()	constructor
//-------------------------------------------------------------
CGLTexMgr::CGLTexMgr(CGLView *v,CGLDataMgr *d){
    threshold=bias=0;
    limit=1;
	intensity=0;
	contrast=0.0;
    threshold=-1;
    alphascale=0;
	colormap=NULL;
	color_mode=0;
    palette=0;
    ncolors=0;
    view=v;
    slice=0;
    textureIds[0]=textureIds[1]=textureIds[2]=0;
    shader_type=NONE;
    colormap_invalid=true;
    functionmap_invalid=true;
    scale=1;
    ymax=1;
    ymin=0;
    data=d;
	shader=NULL;
}


//-------------------------------------------------------------
// CGLTexMgr::setUseShader()  set shader mode
//-------------------------------------------------------------
void CGLTexMgr::setUseShader(bool b){
	if(!b && shader !=NULL){
		shader->unbind();
		DELETE(shader);
		shader=NULL;
	}
	use_shader=b;
}

//-------------------------------------------------------------
// CGLTexMgr::setMapData()  set mmap data source
//-------------------------------------------------------------
//void CGLTexMgr::setMapData(float *mdat){
//	mapData=mdat;
//}
//-------------------------------------------------------------
// CGLTexMgr::CGLTexMgr() destructor
//-------------------------------------------------------------
CGLTexMgr::~CGLTexMgr(){
	//if(textureIds[0])
	//	glDeleteTextures(3, textureIds); 
}

void CGLTexMgr::invalidate() {}
void CGLTexMgr::finish(){}
void CGLTexMgr::free(){}
void CGLTexMgr::drawSlice(int slc, int options) {
	slice=slc;
}

//-------------------------------------------------------------
// CGLTexMgr::init() initialize
//-------------------------------------------------------------
void CGLTexMgr::init(int options,int flags) {
    if((flags&NEWTEXMAP)>0)
        functionmap_invalid = true;
    clip_high=((options & CLIPHIGH)!=0)?true:false;
    clip_low=((options & CLIPLOW)!=0)?true:false;
	if(textureIds[0]==0)
        glGenTextures(3, textureIds);  
}

//-------------------------------------------------------------
// CGLTexMgr::setDataScale() set data dimensions
//-------------------------------------------------------------
void CGLTexMgr::setDataScale(float mn, float mx){
	if(ymax!=mx)
		invalidate();
    ymin=mn;
    ymax=mx;
    scale=1.0f/(ymax-ymin);
 }

//-------------------------------------------------------------
// CGLTexMgr::setColorTable() set colormap vector
//-------------------------------------------------------------
void CGLTexMgr::setColorTable(float *t, int size, int mode){
    int oldmode=palette;
    palette=mode&PALETTE;
    colormap=t;
    ncolors=size;
    if(oldmode!=palette || mode&NEWCTABLE)
        colormap_invalid=true;
    color_mode=mode&COLORMODE;
}

//-------------------------------------------------------------
// CGLTexMgr::setIntensity() set intensity scaling factor 
//-------------------------------------------------------------
void CGLTexMgr::setIntensity(double r){
    intensity=r;
}

//-------------------------------------------------------------
// CGLTexMgr::setBias() set color bias
//-------------------------------------------------------------
void CGLTexMgr::setBias(double r){
    bias=r;
}

//-------------------------------------------------------------
// CGLTexMgr::setContours() set contour interval
//-------------------------------------------------------------
void CGLTexMgr::setContours(double r){
    contours=r;
}

//-------------------------------------------------------------
// CGLTexMgr::setThreshold() set alpha threshold 
//-------------------------------------------------------------
void CGLTexMgr::setThreshold(double r){
    if(r!=threshold)
        functionmap_invalid=true;
    threshold=r;
}

//-------------------------------------------------------------
// CGLTexMgr::setLimit() set alpha max 
//-------------------------------------------------------------
void CGLTexMgr::setLimit(double r){
    if(r!=limit)
        functionmap_invalid=true;
    limit=r;
}

//-------------------------------------------------------------
// CGLTexMgr::setTransparency() set transparency scaling 
//-------------------------------------------------------------
void CGLTexMgr::setTransparency(double r){
    if(r!=transparency)
        functionmap_invalid=true;
    transparency=r;
}

//-------------------------------------------------------------
// CGLTexMgr::setAlphaScale() set alpha scale 
//-------------------------------------------------------------
void CGLTexMgr::setAlphaScale(double r){
    if(r!=alphascale)
        functionmap_invalid=true;
    alphascale=r;
}

//-------------------------------------------------------------
// CGLTexMgr::setContrast() set contrast 
//-------------------------------------------------------------
void CGLTexMgr::setContrast(double r){
    if(r!=contrast)
        functionmap_invalid=true;   
    contrast=r;
}

//-------------------------------------------------------------
// CGLTexMgr::getColorValue() return function map color value
//-------------------------------------------------------------
double CGLTexMgr::getColorValue(double s){
    makeFunctionMap();
    int n=(int)(fabs(s)*(CVSIZE-1));
    if(n>=CVSIZE)
        n=n%CVSIZE;
     return (double)functionmap[n*2];
}

//-------------------------------------------------------------
// CGLTexMgr::getAlphaValue() return function map alpha value
//-------------------------------------------------------------
double CGLTexMgr::getAlphaValue(double s){
    makeFunctionMap();
    int n=(int)(fabs(s)*(CVSIZE-1));
    //if(n>=CVSIZE)
        n=n%CVSIZE;
    return (double)functionmap[n*2+1];
}

//-------------------------------------------------------------
// CGLTexMgr::makeFunctionMap() make function map
//-------------------------------------------------------------
void CGLTexMgr::makeFunctionMap() {
    if(!functionmap_invalid)
        return;
    double s = 0;
    double B = 6 * contrast;
    double cstep = 1.0 / (CVSIZE - 1);
    for(int i = 0; i < CVSIZE; i++) {
        double val = 1.0 / (1 + exp(-((s - 0.5) * B)));
        functionmap[2*i]=(float)val;
        if(val < threshold || val>limit){
            if(val < threshold && clip_low)
                val = 0;
            if(val>limit && clip_high)
                val=0;
        }
        if(val<=0)
            val=0;
        else {
            double f = val * transparency;
            val = 1.0 - pow(1.0 - f, alphascale);
        }
        functionmap[2*i+1]=(float)val;
        s += cstep;
    }
    functionmap_invalid = false;
}

//-------------------------------------------------------------
// CGLTexMgr::makeColorMapTexture() make 1D colormap texture
//-------------------------------------------------------------
void CGLTexMgr::makeColorMapTexture(int options,int flags){
    int wrp=((options & CLAMP)>0)?GL_CLAMP_TO_EDGE:GL_MIRRORED_REPEAT;
    int intrp=((options & BLEND)>0)?GL_LINEAR:GL_NEAREST;
    glBindTexture(GL_TEXTURE_1D, textureIds[1]);
     
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, wrp);              
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, intrp);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, intrp);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, ncolors, 
            0, GL_RGBA, GL_FLOAT, colormap);
}
    
//-------------------------------------------------------------
// CGLTexMgr::makeFunctionMapTexture() make 1D contrast LUT texture
//-------------------------------------------------------------
void CGLTexMgr::makeFunctionMapTexture(int options,int flags){
    makeFunctionMap();
    int wrp=((options & CLAMP)>0)?GL_CLAMP_TO_EDGE:GL_MIRRORED_REPEAT;
    int intrp=GL_LINEAR;
    glBindTexture(GL_TEXTURE_1D, textureIds[2]);
     
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, wrp);              
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, intrp);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, intrp);

    glTexImage1D(GL_TEXTURE_1D, 0, 2, CVSIZE, 
            0, GL_LUMINANCE_ALPHA, GL_FLOAT, functionmap);
}

//-------------------------------------------------------------
// CGLTexMgr::texColor() return color mapped into an int value
//-------------------------------------------------------------
int CGLTexMgr::texColor(double v,int options){
    double s=intensity*v;
    int h,indx;
    long value=0;
    int r=0,b=0,g=0,a=255;
    double cval,aval;

    if((options & XPARANCY) > 0){
        aval=getAlphaValue(s);
        a=(int)(255*aval)&0xff;
    }  
    cval=getColorValue(s);
    h = (int)((ncolors - 1) * cval);
    indx = (h % ncolors) * 4;
    r=(int)(255.0*colormap[indx])&0xff;
    g=(int)(255.0*colormap[indx+1])&0xff;
    b=(int)(255.0*colormap[indx+2])&0xff;
    value=(a<<24)|(b<<16)|(g<<8)|(r);
    return (int)value;
}

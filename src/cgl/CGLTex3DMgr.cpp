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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

#include "CGLTexMgr.h" // C++ class header file

//#define DEBUG_TEXS
//#define DEBUG_MEM
//#define DEBUG_TIME

//-------------------------------------------------------------
// CGLTex3DMgr::CGLTex3DMgr()	constructor
//-------------------------------------------------------------
CGLTex3DMgr::CGLTex3DMgr(CGLView *v,CGLDataMgr *d) : CGLTexMgr(v,d)
{
	tex3DByteBuffer=NULL;
	tex3DIntBuffer=NULL;
	shader=NULL;
#ifdef DEBUG_MEM
    std::cout << "CGLTex3DMgr::CGLTex3DMgr()" << std::endl;
#endif
}

//-------------------------------------------------------------
// CGLTex3DMgr::~CGLTex3DMgr()	destructor
//-------------------------------------------------------------
CGLTex3DMgr::~CGLTex3DMgr(){
#ifdef DEBUG_MEM
    std::cout << "CGLTex3DMgr::~CGLTex3DMgr()" << std::endl;
#endif
	free();
}

//-------------------------------------------------------------
// CGLTex3DMgr::free()	free resources
//-------------------------------------------------------------
void CGLTex3DMgr::free(){
    invalidate();
#ifdef DEBUG_MEM
    std::cout << "CGLTex3DMgr::free(shader)"<<std::endl;
#endif
    //DELETE(shader);
}

//-------------------------------------------------------------
// CGLTex3DMgr::invalidate()	invalidate texture
//-------------------------------------------------------------
void CGLTex3DMgr::invalidate(){
#ifdef DEBUG_MEM
    std::cout << "CGLTex3DMgr::invalidate()"<<std::endl;
#endif
	FREE(tex3DByteBuffer);
	FREE(tex3DIntBuffer);
}

//-------------------------------------------------------------
// CGLTex3DMgr::finish() render call cleanup
//-------------------------------------------------------------
void CGLTex3DMgr::finish(){
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, 0);
    if(shader != NULL){
        shader->unbind();
        shader->disable();
    }
}

//-------------------------------------------------------------
// CGLTex3DMgr::init()	initialize
//-------------------------------------------------------------
void CGLTex3DMgr::init(int options,int flags){
	CGLTexMgr::init(options,flags);

    min=view->eyeMinPoint();
    max=view->eyeMaxPoint();
    delta=1.0/(view->maxSlice()+1);

    glEnable(GL_TEXTURE_3D);
    glEnable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glColor4f(1, 1, 1, 1);

    if(color_mode==HISTOGRAM){
        if(shader !=NULL){
            delete shader;
            invalidate();
        }
        shader=NULL;
    }
    else if(shader == NULL || (flags & NEWLAYOUT) > 0 || (flags & NEWSHADER) > 0) {
        if(shader!=NULL){
            delete shader;
            invalidate();
        }
        shader=getShader(shader_type);
        if(shader != NULL){
        	switch(options & SHADERTYPE){
            case VOLSHADER:
                shader->loadShader(VOL3D);
                break;
            case MIPSHADER:
                shader->loadShader(MIP3D);
                break;
        	}
        }
        colormap_invalid=true;
        functionmap_invalid = true;
    }

    if(shader != NULL) {
        if((options & XPARANCY)>0)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);

        shader->bind();
        params[1] = (float)intensity;

        if(functionmap_invalid || (flags & (NEWTEXMODE|NEWLAYOUT)) > 0){
            functionmap_invalid = true;
            glActiveTexture(GL_TEXTURE2);
            makeFunctionMapTexture(options,flags);
            shader->setTexture("functionmap", 2);
        }
        if(colormap_invalid || (flags & (NEWTEXMODE|NEWLAYOUT)) > 0){
            glActiveTexture(GL_TEXTURE1);
            makeColorMapTexture(options,flags);
            shader->setTexture("colormap", 1);
        }
        if(tex3DByteBuffer==NULL
                || (flags & (NEWTEXGEOM|NEWTEXMODE|NEWLAYOUT))>0){
            make3DTexture(options,flags);
            shader->setTexture("voltex", 0);
        }
        shader->unbind();
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_1D, textureIds[2]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, textureIds[1]);
    } else if(tex3DIntBuffer == NULL || (flags > 0)) {
        glEnable(GL_BLEND);
         make3DTexture(options, flags);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, textureIds[0]);
}

//-------------------------------------------------------------
// CGLTex3DMgr::make3DTexture()	set texture
//-------------------------------------------------------------
void CGLTex3DMgr::make3DTexture(int options,int flags){
    int intrp=((options & BLEND)>0)?GL_LINEAR:GL_NEAREST;
    int wrp=GL_CLAMP_TO_EDGE;
    int i,j,index=0,k;
    double start=clock();
    if(!data->dataValid()){
        std::cout << "CGLTex3DMgr::make3DTexture() ERROR data invalid"<<std::endl;
    	return;
    }


#ifdef DEBUG_MEM
    std::cout << "CGLTex3DMgr::make3DTexture("<<flags<<")"<<std::endl;
#endif

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glBindTexture(GL_TEXTURE_3D, textureIds[0]);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrp);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrp);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrp);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, intrp);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, intrp);

    float v;
    if(shader != NULL){
        MALLOC(data->data_size,GLubyte,tex3DByteBuffer);
        for(k=0;k<data->slices;k++){
            for(j=0;j<data->traces;j++){
                for(i=0;i<data->np;i++){
                	 v=(float)data->vertexValue(index);
                     v=v*255*scale;
                     v=v>255?255:v;
                     tex3DByteBuffer[index++]=(GLubyte)v;
                }
            }
        }
        glTexImage3D(GL_TEXTURE_3D, 0, GL_ALPHA8,data->np,data->traces,data->slices,
                0, GL_ALPHA, GL_UNSIGNED_BYTE, tex3DByteBuffer);
    } else{
        MALLOC(data->data_size,int,tex3DIntBuffer);
        for(k=0;k<data->slices;k++){
            for(j=0;j<data->traces;j++){
                for(i=0;i<data->np;i++){
                    switch(options & SLICEPLANE){
                    case X:
                        slice=i;
                        break;
                    case Y:
                        slice=j;
                        break;
                    default:
                    case Z:
                        slice=k;
                        break;
                    }
                    v=(float)data->vertexValue(index);
                    int c=texColor(v*scale,options);
                    tex3DIntBuffer[index++]=c;
                }
            }
        }
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, data->np,data->traces,data->slices,
                0, GL_RGBA, GL_UNSIGNED_BYTE, tex3DIntBuffer);
    }
#ifdef DEBUG_TIME
    double msecs=1000.0*((double)(clock() - start)/CLOCKS_PER_SEC);
    std::cout << "CGLTex3DMgr::make3DTexture("<<flags<<") time:"<<msecs<<" ms"<<std::endl;
#endif

}

//-------------------------------------------------------------
// CGLTex3DMgr::drawSlice()	render slice using slice texture
//-------------------------------------------------------------
void CGLTex3DMgr::drawSlice(int slc, int options){
    Point3D spts[12];
    int numpts;
    float color=-1;
    if(shader != NULL) {
        shader->enable();
        shader->bind();
        int n=view->maxSlice();
        switch(options & COLTYPE) {
        case COLONE:
            color=0.5f;
            break;
        case COLIDX:
            color = ((float)(slc))/n;
            if((options & CLAMP) == 0)
               color*=intensity;
            break;
        }
        params[0]=(float)color;
        shader->setFloatVector("params", params);
    }
    double s = delta * slc;
    Point3D zplane = min.plane(max, s); // farthest first
    numpts=view->slicePts(zplane,spts);

    Point3D ps=view->volume->scale.invert();
    glBegin(GL_TRIANGLE_FAN);
    for(int i = 0; i < numpts; i++) {
    	Point3D pt=spts[i].mul(ps);
    	glTexCoord3d(pt.x, pt.y + 0.5, pt.z);
        glVertex3d(spts[i].x, spts[i].y, spts[i].z);
    }
    glEnd();
    if(shader != NULL) {
        shader->unbind();
        shader->disable();
    }
}

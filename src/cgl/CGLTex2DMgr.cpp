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
//
// shader inputs
// ------------------------------------
// voltex       volume data      2D-texture
// colormap     color palette    1D-texture
// functionmap  contrast LUT     1D-texture
// params[0].y  intensity
// params[0].x  color index

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "CGLTexMgr.h" // C++ class header file

static  float stbl[4][2]={0,0,0,1,1,1,1,0};

//#define DEBUG_MEM
//-------------------------------------------------------------
// CGLTex2DMgr::CGLTex2DMgr()	constructor
//-------------------------------------------------------------
CGLTex2DMgr::CGLTex2DMgr(CGLView *v,CGLDataMgr *d) : CGLTexMgr(v,d)
{
	tex2DByteBuffer=NULL;
	tex2DIntBuffer=NULL;
	sliceIds=NULL;
	buffer_length=0;
	shader=NULL;
	num2Dtexs=0;
	size2Dtexs=0;
	delta=0;
#ifdef DEBUG_MEM
    std::cout << "CGLTex2DMgr::CGLTex2DMgr()" << std::endl;
#endif
}

//-------------------------------------------------------------
// CGLTex2DMgr::~CGLTex2DMgr()	destructor
//-------------------------------------------------------------
CGLTex2DMgr::~CGLTex2DMgr(){
#ifdef DEBUG_NATIVE
    std::cout << "CGLTex2DMgr::~CGLTex2DMgr()" << std::endl;
#endif
	if(sliceIds)
		glDeleteTextures(MAXSLICES, sliceIds);
	FREE(sliceIds);
	free();
}

//-------------------------------------------------------------
// CGLTex2DMgr::free()	free resources
//-------------------------------------------------------------
void  CGLTex2DMgr::free(){
    invalidate();
#ifdef DEBUG_MEM
    std::cout << "CGLTex2DMgr::free(shader)"<<std::endl;
#endif
	//DELETE(shader);
}

//-------------------------------------------------------------
// CGLTex2DMgr::invalidate()	invalidate texture
//-------------------------------------------------------------
void CGLTex2DMgr::invalidate(){
	int i;
#ifdef DEBUG_MEM
    std::cout << "CGLTex2DMgr::invalidate()"<<std::endl;
#endif
	if(tex2DByteBuffer!=NULL){
		for(int i=0;i<buffer_length;i++){
	    	FREE(tex2DByteBuffer[i]);
		}
		FREE(tex2DByteBuffer);
	}
	if(tex2DIntBuffer!=NULL){
		for(int i=0;i<buffer_length;i++){
	    	FREE(tex2DIntBuffer[i]);
		}
		FREE(tex2DIntBuffer);
	}
}

//-------------------------------------------------------------
// CGLTex2DMgr::finish() render call cleanup
//-------------------------------------------------------------
void CGLTex2DMgr::finish(){
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    //glBindTexture(GL_TEXTURE_2D, 0);
    if(shader != NULL){
        shader->unbind();
        shader->disable();
    }
}

//-------------------------------------------------------------
// CGLTex2DMgr::init()	initialize
//-------------------------------------------------------------
void CGLTex2DMgr::init(int options,int flags){
	CGLTexMgr::init(options,flags);
    int n=num2Dtexs;

    min=view->sliceMinPoint();
    max=view->sliceMaxPoint();

    num2Dtexs=view->maxSlice()+1;
    delta=1.0/num2Dtexs;
    size2Dtexs=view->sliceSize();

    if(n!=num2Dtexs)
        invalidate();

    if(sliceIds==NULL){
        MALLOC(MAXSLICES,GLuint,sliceIds);
        glGenTextures(MAXSLICES, sliceIds);
    }
    if(color_mode==HISTOGRAM){
        if(shader !=NULL){
            DELETE(shader);
            invalidate();
        }
    }
    else if(shader == NULL || (flags & NEWLAYOUT) > 0 || (flags & NEWSHADER) > 0) {
        if(shader!=NULL){
            delete shader;
            invalidate();
        }
        shader=getShader(shader_type);
        if(shader != NULL){
        	switch(options & SHADERTYPE){
        	default:
        	case VOLSHADER:
                shader->loadShader(VOL2D);
        		break;
        	case MIPSHADER:
         	    shader->loadShader(MIP2D);
        		break;
        	}
        }
        colormap_invalid=true;
        functionmap_invalid = true;
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_3D);
    glColor4f(1, 1, 1, 1);

    if(shader !=NULL){
        if((options & XPARANCY)>0)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);

        shader->bind();
        params[1] = (float)intensity;

        if(functionmap_invalid || (flags & (NEWTEXGEOM|NEWTEXMODE|NEWLAYOUT)) > 0){
            functionmap_invalid=true;
            makeFunctionMapTexture(options,flags);
            shader->setTexture("functionmap", 2);
        }
        if(colormap_invalid || (flags & (NEWTEXGEOM|NEWTEXMODE|NEWLAYOUT)) > 0){
            makeColorMapTexture(options,flags);
            shader->setTexture("colormap", 1);
        }
        if(tex2DByteBuffer==NULL || (flags & (NEWTEXGEOM|NEWTEXMODE|NEWLAYOUT))>0){
            shader->setTexture("voltex", 0);
            invalidate();
            allocate();
        }
        shader->unbind();
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_1D, textureIds[2]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, textureIds[1]);
     }
    else {
        if(flags> 0){
            invalidate();
		    allocate();
        }
        glEnable(GL_BLEND);
    }
     glActiveTexture(GL_TEXTURE0);
}

//-------------------------------------------------------------
// CGLTex2DMgr::allocate()	allocate texture memory
//-------------------------------------------------------------
void CGLTex2DMgr::allocate(){
	int i;
#ifdef DEBUG_MEM
    std::cout << "CGLTex2DMgr::allocate(texmem)"<<std::endl;
#endif
    if(shader != NULL){
    	FREE(tex2DByteBuffer);
    	buffer_length=num2Dtexs;
    	MALLOC(num2Dtexs,GLubyte*,tex2DByteBuffer);
    	for(i=0;i<num2Dtexs;i++)
    		tex2DByteBuffer[i]=NULL;
    }
    else{
    	FREE(tex2DIntBuffer);
    	buffer_length=num2Dtexs;
    	MALLOC(num2Dtexs,int*,tex2DIntBuffer);
    	for(i=0;i<num2Dtexs;i++)
    		tex2DIntBuffer[i]=NULL;
    }
}

//-------------------------------------------------------------
// CGLTex2DMgr::vertexValue(k,j,i,..)	return value at vertex
//-------------------------------------------------------------
double CGLTex2DMgr::vertexValue(int k, int j, int i, int options){
    int adrs=0;
    switch(options & SLICEPLANE){
    case X: adrs=data->np*j+data->np*data->traces*i+k;break;
    case Y: adrs=data->np*k+data->np*data->traces*j+i;break;
    case Z: adrs=data->np*j+data->np*data->traces*k+i;break;
    }
    return  data->vertexValue(adrs);
}

//-------------------------------------------------------------
// CGLTex2DMgr::make2DTexture()	set texture for 1 slice
//-------------------------------------------------------------
void CGLTex2DMgr::make2DTexture(int k, int options){
    int nj=0,ni=0;
    int i,j,index=0;
    float v;
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    switch(options & SLICEPLANE){
    case X: nj=data->traces; ni=data->slices; break;
    case Y: nj=data->slices; ni=data->np; break;
    case Z: nj=data->traces; ni=data->np; break;
    }
    int intrp=((options & BLEND)>0)?GL_LINEAR:GL_NEAREST;
    int wrp=GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, intrp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, intrp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrp);

    if(shader != NULL){
        int maxvalue=255;
        MALLOC(size2Dtexs,GLubyte,tex2DByteBuffer[k]);
        for(j=0;j<nj;j++){
            for(i=0;i<ni;i++){
                 v=(float)vertexValue(k,j,i,options);
                 v=v*maxvalue*scale;
                 v=v>maxvalue?maxvalue:v;
                 tex2DByteBuffer[k][index++]=(GLubyte)v;
            }
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8 ,ni,nj,
                0, GL_ALPHA, GL_UNSIGNED_BYTE, tex2DByteBuffer[k]);
    }
    else{
        int c;
        MALLOC(size2Dtexs,int,tex2DIntBuffer[k]);
        for(j=0;j<nj;j++){
            for(i=0;i<ni;i++){
                slice=k;
                v=(float)vertexValue(k,j,i,options);
                c=texColor(v,options);
                tex2DIntBuffer[k][index++]=c;
            }
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ni,nj,
                0, GL_RGBA, GL_UNSIGNED_BYTE, tex2DIntBuffer[k]);
    }
}

//-------------------------------------------------------------
// CGLTex2DMgr::drawSlice()	render slice using slice texture
//-------------------------------------------------------------
void CGLTex2DMgr::drawSlice(int slc, int options){
    float color=-1;
    Point3D spts[4];

    if(!data->dataValid()){
        std::cout << "CGLTex2DMgr::drawSlice() ERROR data invalid"<<std::endl;
    	return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sliceIds[slc]);

    if(shader != NULL){
        if(tex2DByteBuffer[slc]==NULL)
            make2DTexture(slc, options);
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
    else if (tex2DIntBuffer[slc]==NULL)
        make2DTexture(slc, options);

    double s = delta * slc;
    Point3D plane = min.plane(max, s);
    int numpts=view->boundsPts(plane,spts);
    glPushMatrix();
    // X is inverted (top.bottom). try to figure out why later
    if((options & SLICEPLANE)==X){
    	Point3D pc=view->volume->center;
    	glTranslated(pc.x, pc.y, pc.z);
    	glRotated(180, 1, 0, 0);
    	glTranslated(-pc.x, -pc.y, -pc.z);
    }

    glBegin(GL_TRIANGLE_FAN);
	for(int i = 0; i < numpts; i++) {
		glTexCoord2d(stbl[i][0], stbl[i][1]);
		glVertex3d(spts[i].x, spts[i].y, spts[i].z);
	}
    glEnd();
    glPopMatrix();
    if(shader !=NULL){
        shader->unbind();
        shader->disable();
    }
}


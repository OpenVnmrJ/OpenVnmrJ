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
#include <iostream>

#include "CGLTexMgr.h" // C++ class header file


//-------------------------------------------------------------
// CGLTex1DMgr::CGLTex1DMgr()	constructor
//-------------------------------------------------------------
CGLTex1DMgr::CGLTex1DMgr(CGLView *v,CGLDataMgr *d) : CGLTexMgr(v,d)
{
#ifdef DEBUG_NATIVE    
    std::cout << "CGLTex1DMgr::CGLTex1DMgr()" << std::endl;
#endif
    shader=NULL;
    use_shader=false;
}

//-------------------------------------------------------------
// CGLTex1DMgr::~CGLTex1DMgr()	destructor
//-------------------------------------------------------------
CGLTex1DMgr::~CGLTex1DMgr(){
#ifdef DEBUG_NATIVE    
    std::cout << "CGLTex1DMgr::~CGLTex1DMgr()" << std::endl;
#endif
}


//-------------------------------------------------------------
// CGLTex1DMgr::init()	initialize
//-------------------------------------------------------------
void CGLTex1DMgr::init(int options,int flags){
	CGLTexMgr::init(options,flags);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_3D);
	if (functionmap_invalid
			|| (flags & (NEWTEXGEOM | NEWTEXMODE | NEWLAYOUT)) > 0) {
		functionmap_invalid = true;
		makeFunctionMapTexture(options, flags);
	}
	if (colormap_invalid || (flags & (NEWTEXGEOM | NEWTEXMODE | NEWLAYOUT)) > 0) {
		makeColorMapTexture(options, flags);
	}

	if (use_shader) {
		if (shader == NULL || (flags & NEWLAYOUT) > 0
				|| (flags & NEWSHADER) > 0) {
			if (shader != NULL)
				DELETE(shader);
			shader = getShader(shader_type);
			if (shader != NULL){
				if((flags & USELIGHTING)>0)
					shader->loadShader(LGT1D);
				else
					shader->loadShader(HT1D);
			}
		}
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, textureIds[2]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, textureIds[1]);
	} else {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_1D, textureIds[1]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_DECAL);
	}
}
//-------------------------------------------------------------
// CGLTex1DMgr::beginShader()	begin shader pass
//-------------------------------------------------------------
void CGLTex1DMgr::beginShader(int slc, int options){
	if(shader==0 || data->projection !=TWOD)
		return;

	shader->enable();
	shader->bind();

	float color=-1;
	int cmode=0;
	int n = view->maxSlice();
	switch(options & COLTYPE) {
	case COLONE:
		cmode=1;
		break;
	case COLIDX:
		color = ((float)(slc)) / n;
		if((options & CLAMP) == 0)
			color *= intensity;
		break;
	}
    glPushAttrib(GL_ENABLE_BIT);

    bool xparancy=(options & XPARANCY) > 0;
    bool reversed=(options & REVERSED) > 0;
    bool draw_contours=(options & CONTOURS)>0;
    bool draw_grid=(options & GRID)>0;

    params[1] = xparancy ?1.0f:-1.0f;
    params[0] = (float)color;

    if(draw_contours){
    	float mingap=0.01f;
        float t=(float)(contours);
        t=t<mingap?mingap:t;
    	params[2]=(float)(20*t*sqrt(view->getDscale()));
    }
    else
    	params[2]=0.0f;

    if(reversed||!draw_contours)
    	params[2]=-params[2];

    if(draw_grid)
    	params[3]=20.0f; // grid spacing
    else
    	params[3]=0.0f;

    glDisable(GL_LIGHTING);
    glNormal3d(0,0,0);

	shader->setFloatVector("params", params,0);
    shader->setFloatVector("bcol", bgcol,1);
    if(shader->type()==GLSL){
		shader->setFloatVector("gcol", gridcol);
		shader->setIntValue("cmode", cmode);
		if((options & LIGHTING) >0){
			float lpars[]={0.2f,0.9f,0.8f,4.0f}; // ambient,diffuse,specular,shine
			float lpos[]={1.0f,-1.0f,0.0f,1.0f}; // light position
			shader->setFloatVector("lpars", lpars);
			shader->setFloatVector("lpos", lpos);
		}
    }
	shader->setTexture("functionmap", 1);
	shader->setTexture("colormap", 0);
	glEnable(GL_TEXTURE_1D);
	glEnable(GL_BLEND);
}

//-------------------------------------------------------------
// CGLTex1DMgr::endShader()	end shader pass
//-------------------------------------------------------------
void CGLTex1DMgr::endShader(){
	if(shader==0|| data->projection !=TWOD)
		return;
	shader->unbind();
	shader->disable();
    glPopAttrib();

}

//-------------------------------------------------------------
// CGLTex1DMgr::finish() render call cleanup
//-------------------------------------------------------------
void CGLTex1DMgr::finish(){
		glDisable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, 0);
	if(shader != 0){
		shader->unbind();
		shader->disable();
	}
}

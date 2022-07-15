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
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include "CGLProgram.h"

#define MAXSYMBOLS 100

// volumeFShader2D[3D]
// ARB_fragment_program
// inputs
// ------------------------------------
// texture[0]   volume data    2D[3D]-texture
// texture[1]   color table    1D-texture
// texture[2].r color LUT      1D-texture
// texture[2].a alpha LUT      1D-texture
// params[0].y  intensity   
// params[0].x  color index   

static char *htFShader1D=""
	"!!ARBfp1.0\n"
	 "PARAM params = program.env[0];\n"
	 "PARAM bcol = program.env[1];\n"
	 "TEMP  temp, fmap, cmap, color;\n"
	 "TEX   fmap, fragment.texcoord [0], texture [1], 1D;\n"
	 "SUB   temp.x, params.x,0.5;\n"
	 "CMP   cmap.x, temp.x, fmap.x, params.x;\n"
	 "CMP   cmap.y, params.y, 1.0, fmap.y;\n"
	 "TEX   color, cmap.x, texture [0], 1D;\n"
	 // contours
	 "ABS   temp.z, params.z;\n"       // contour frequency
	 "MUL   temp.x, fmap.x, temp.z;\n" // should use vertex ht
	 "FLR   temp.y, temp.x;\n"
	 "SUB   temp.y, temp.x,temp.y;\n"
	 "ABS   temp.y, temp.y;\n"
	 "SUB   temp.y, temp.y, 0.25;\n" // contours 1/4 of band
	 "SUB   temp.z, 0.0, temp.z;\n"  // if <0 contours enabled
	 "MUL   temp.y, temp.y,params.z;\n" // reverse sign if <0
	 "CMP   temp.y, temp.z, temp.y, -1.0;\n"
	 "CMP   color, temp.y, color, bcol;\n"
	 "MOV   color.a, 1.0;\n"

	 "CMP   temp.x, params.y, 1.0, fmap.y;\n"
	 "MOV   color.a, temp.x;\n"
	 "MOV   result.color, color;\n"
	 "END\n";

static char *volumeFShader2D=""
	"!!ARBfp1.0\n"
	"PARAM params = program.env[0];\n"
	"TEMP  vol, temp, cmap, color;\n"
	"TEX   vol, fragment.texcoord [0], texture [0], 2D;\n"
    "MUL   temp.x, vol.a, params.y;\n"
	"CMP   temp.y, params.x, temp.x, params.x;\n"
	"TEX   cmap, temp.y, texture [2], 1D;\n"
	"TEX   color, cmap.r, texture [1], 1D;\n"
	"MOV   color.a, cmap.a;\n"
	"MOV   result.color, color;\n"
	"END\n";
static char *volumeFShader3D=""
	"!!ARBfp1.0\n"
	"PARAM params = program.env[0];\n"
	"TEMP  vol, temp, cmap, color;\n"
	"TEX   vol, fragment.texcoord [0], texture [0], 3D;\n"
    "MUL   temp.x, vol.a, params.y;\n"
	"CMP   temp.y, params.x, temp.x, params.x;\n"
	"TEX   cmap, temp.y, texture [2], 1D;\n"
	"TEX   color, cmap.r, texture [1], 1D;\n"
	"MOV   color.a, cmap.a;\n"
	"MOV   result.color, color;\n"
	"END\n";

static char *mipFShader2D=""
	"!!ARBfp1.0\n"
	"PARAM params = program.env[0];\n"
	"TEMP  vol, temp, cmap, color;\n"
	"TEX   vol, fragment.texcoord [0], texture [0], 2D;\n"
    "MUL   temp.x, vol.a, params.y;\n"
	"CMP   temp.y, params.x, temp.x, params.x;\n"
	"TEX   cmap, temp.y, texture [2], 1D;\n"
	"TEX   color, cmap.r, texture [1], 1D;\n"
	"SUB   temp.z,1.0,cmap.a;\n"
	"SUB   temp.y,0.0,cmap.a;\n"
	"CMP   result.depth, temp.y, temp.z, 2.0;\n"
	"CMP   result.color, temp.y,color, fragment.color;\n"
	"END\n";

static char *mipFShader3D=""
	"!!ARBfp1.0\n"
	"PARAM params = program.env[0];\n"
	"TEMP  vol, temp, cmap, color;\n"
	"TEX   vol, fragment.texcoord [0], texture [0], 3D;\n"
    "MUL   temp.x, vol.a, params.y;\n"
	"CMP   temp.y, params.x, temp.x, params.x;\n"
	"TEX   cmap, temp.y, texture [2], 1D;\n"
	"TEX   color, cmap.r, texture [1], 1D;\n"
	"SUB   temp.z,1.0,cmap.a;\n"
	"SUB   temp.y,0.0,cmap.a;\n"
	"CMP   result.depth, temp.y, temp.z, 2.0;\n"
	"CMP   result.color, temp.y,color, fragment.color;\n"
	"END\n";

//-------------------------------------------------------------
// ARBProgram::ARBProgram()	constructor
//-------------------------------------------------------------
ARBProgram::ARBProgram(){
    symbols=NULL;
    sid=0;
}

//-------------------------------------------------------------
// ARBProgram::~ARBProgram()	destructor
//-------------------------------------------------------------      
ARBProgram::~ARBProgram(){
    if(symbols !=NULL){
        for(int i=0;i<sid;i++){
            FREE(symbols[i]);
        }
        FREE(symbols);
    }
}

//-------------------------------------------------------------
// ARBProgram::initSymbolTable()    initialize symbol lut
//-------------------------------------------------------------      
void ARBProgram::initSymbolTable(){
    MALLOC(MAXSYMBOLS,char*,symbols);
    if(symbols != NULL){
        for(int i=0;i<MAXSYMBOLS;i++)
            symbols[i]=0;
    }
    sid=0;
}

//-------------------------------------------------------------
// ARBProgram::getParameter()    set symbol 
//-------------------------------------------------------------      
int ARBProgram::getParameter(char *s){
    int i=0;
    if(!symbols)
        initSymbolTable();
    if(!symbols)
        return -1;
    for(i=0;i<sid;i++){
        if(strcmp(s,symbols[i])==0)
            return i;
    }
    MALLOC(strlen(s),char,symbols[sid]);
    if(symbols[sid]){
        strcpy(symbols[sid],s);
        sid++;
        return sid;
    }
    return -1;
}

//-------------------------------------------------------------
// ARBProgram::isSupported() return true if shader is supported
//-------------------------------------------------------------
bool ARBProgram::isSupported(){
   if (isExtensionSupported ("GL_ARB_fragment_program") &&
       isExtensionSupported ("GL_ARB_vertex_program")
    )
        return true;    
    return false; 
}
//-------------------------------------------------------------
// ARBProgram::bind() bind program
//-------------------------------------------------------------    
void ARBProgram::bind() {
    vshader->bind();
    fshader->bind();
}

//-------------------------------------------------------------
// ARBProgram::unbind() unbind program
//-------------------------------------------------------------    
void ARBProgram::unbind() {
    vshader->unbind();
    fshader->unbind();
} 

//-------------------------------------------------------------
// ARBProgram::enable() enable program
//-------------------------------------------------------------    
void ARBProgram::enable() {
    vshader->enable();
    fshader->enable();   
} 

//-------------------------------------------------------------
// ARBProgram::disable() disable program
//-------------------------------------------------------------    
void ARBProgram::disable() {
    vshader->disable();
    fshader->disable();   
}

//-------------------------------------------------------------
// GLSLProgram::loadShader() load shader
//-------------------------------------------------------------    
void ARBProgram::loadShader(int mode) {
    if ((mode & S2D)>0) {
        if ((mode & SMIP)>0)
            loadFragmentShader(mipFShader2D);
        else
            loadFragmentShader(volumeFShader2D);
    } else if((mode & S3D)>0){
        if ((mode & SMIP)>0)
            loadFragmentShader(mipFShader3D);
        else
            loadFragmentShader(volumeFShader3D);
    }
    else
    	loadFragmentShader(htFShader1D);
}   

//-------------------------------------------------------------
// ARBProgram::loadShader() load shader
//-------------------------------------------------------------    
bool ARBProgram::loadShader(char *source, CGLShader *shader){
    shader->setActive();
    shader->bind();
    glProgramStringARB (shader->getProgramType(), GL_PROGRAM_FORMAT_ASCII_ARB,
            strlen(source), source);
    
    if (glGetError () == GL_INVALID_OPERATION ){
        //String errorString = gl.glGetString ( GL.GL_PROGRAM_ERROR_STRING_ARB );
       // System.out.println("GPU shader error >> " + errorString);
        return false;
    }
    return true;
}

//-------------------------------------------------------------
// ARBProgram::setFloatVector() set shader parameter
//------------------------------------------------------------- 
bool ARBProgram::setFloatVector (char *name, float *d){
    int pid=getParameter(name);
    if(pid<0)
        return false;
    return setFloatVector(name,d,pid);
}

//-------------------------------------------------------------
// ARBProgram::setFloatVector() set shader parameter
//-------------------------------------------------------------
bool ARBProgram::setFloatVector (char *name, float *d, int pid){
    if(vshader->isActive())
        glProgramEnvParameter4fARB(vshader->getProgramType(),pid,d[0],d[1],d[2],d[3]);
    if(fshader->isActive())
        glProgramEnvParameter4fARB(fshader->getProgramType(),pid,d[0],d[1],d[2],d[3]);
    return true;
}

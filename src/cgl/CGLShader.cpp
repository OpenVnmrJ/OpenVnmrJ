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
#include "CGLShader.h"

//#define DEBUG_NATIVE

//-------------------------------------------------------------
// CGLShader::CGLShader()	constructor
//-------------------------------------------------------------    
CGLShader::CGLShader(int s, int p){
    shader_type=s;
    program_type=p;
    shaderObj=0;
    active=false;
    initProperties();
    id=0;
}

//-------------------------------------------------------------
// CGLShader::~CGLShader()	destructor
//-------------------------------------------------------------    
CGLShader::~CGLShader(){
#ifdef DEBUG_NATIVE 	   
    std::cout << name() << "::~" <<name()<< std::endl;
#endif
	setInactive();
    shaderObj=0;
}
//-------------------------------------------------------------
// CGLShader::init()	initialize variables
//-------------------------------------------------------------    
void CGLShader::initProperties(){
    maxAluInstructions=getProgramVariable(GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB);
    maxTexInstructions=getProgramVariable(GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB);
    maxTexIndirections=getProgramVariable(GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB);
    maxInstructions=getProgramVariable(GL_MAX_PROGRAM_INSTRUCTIONS_ARB);
    maxLocalParams=getProgramVariable(GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB);
    maxEnvParams=getProgramVariable(GL_MAX_PROGRAM_ENV_PARAMETERS_ARB);
    maxTemporaries=getProgramVariable(GL_MAX_PROGRAM_TEMPORARIES_ARB);
    maxProgramAttribs=getProgramVariable(GL_MAX_PROGRAM_ATTRIBS_ARB);
}

//-------------------------------------------------------------
// CGLShader::showGLInfo()	show openGL properties
//-------------------------------------------------------------    
void CGLShader::showGLInfo(){
    std::cout << name()<<" attributes:" << std::endl;
    std::cout << " Max Program Instructions: "<< maxInstructions << std::endl;
    std::cout << " Max ALU Instructions: "<< maxAluInstructions << std::endl;
    std::cout << " Max Texture Instructions: "<< maxTexInstructions << std::endl;
    std::cout << " Max Texture Indirections: "<< maxTexIndirections << std::endl;
    std::cout << " Max Local Parameters: "<< maxLocalParams << std::endl;
    std::cout << " Max Environment Parameters: "<< maxEnvParams << std::endl;
    std::cout << " Max Temporaries: "<< maxTemporaries << std::endl;
    std::cout << " Max Program Attributes: "<< maxProgramAttribs << std::endl;
}

//-------------------------------------------------------------
// CGLShader::getProgramVariable()	return openGL properties
//-------------------------------------------------------------    
GLint CGLShader::getProgramVariable(GLint attr){
    glGetProgramivARB ( program_type, attr, (GLint*)Id);
    return Id[0];
}

//############# CGLVertexShader ##############################
//-------------------------------------------------------------
// CGLVertexShader::CGLVertexShader()	constructor
//-------------------------------------------------------------    
CGLVertexShader::CGLVertexShader() : CGLShader(GL_VERTEX_SHADER_ARB,GL_VERTEX_PROGRAM_ARB) 
{
#ifdef DEBUG_NATIVE 	   
    std::cout << name() << "::" <<name()<< std::endl;
#endif
}
//############# CGLFragmentShader ##############################
//-------------------------------------------------------------
// CGLFragmentShader::CGLFragmentShader()	constructor
//-------------------------------------------------------------    
CGLFragmentShader::CGLFragmentShader() : CGLShader(GL_FRAGMENT_SHADER_ARB,GL_FRAGMENT_PROGRAM_ARB) 
{
#ifdef DEBUG_NATIVE 	   
    std::cout << name() << "::" <<name()<< std::endl;
#endif
}


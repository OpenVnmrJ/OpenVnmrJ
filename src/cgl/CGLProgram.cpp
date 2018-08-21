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
#include "CGLProgram.h"

//-------------------------------------------------------------
// CGLProgram::CGLProgram()	constructor
//-------------------------------------------------------------
CGLProgram::CGLProgram(){
    program=0;
    fshader=new CGLFragmentShader();
    vshader=new CGLVertexShader();   
}

//-------------------------------------------------------------
// CGLProgram::~CGLProgram()	destructor
//-------------------------------------------------------------      
CGLProgram::~CGLProgram(){
    DELETE(fshader);
    DELETE(vshader);   
}

//-------------------------------------------------------------
// CGLProgram::showGLInfo()	show openGL properties
//-------------------------------------------------------------    
    void CGLProgram::showGLInfo(){
    int intbuffer[1];       
    std::cout << "Shader program type: " << name()<< std::endl;
    glGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB,intbuffer);
    std::cout << " Max Vertex Uniform Componants: " << intbuffer[0] << std::endl;
    glGetIntegerv( GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB,intbuffer);
    std::cout << " Max Fragment Uniform Componants: " << intbuffer[0] << std::endl;
    glGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB,intbuffer);
    std::cout << " Max Vertex Attributes: " << intbuffer[0] << std::endl;
    glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB,intbuffer);
    std::cout << " Max Fragment Texture Units: " << intbuffer[0] << std::endl;
    glGetIntegerv( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB,intbuffer);
    std::cout << " Max Vertex Texture Units: " << intbuffer[0] << std::endl;
    glGetIntegerv( GL_MAX_VARYING_FLOATS_ARB,intbuffer);
    std::cout << " Max Varying Floats: " << intbuffer[0] << std::endl;
    glGetIntegerv( GL_MAX_TEXTURE_COORDS_ARB,intbuffer);
    std::cout << " Max Texture Coords: " << intbuffer[0] << std::endl;
    vshader->showGLInfo();
    fshader->showGLInfo();       
}

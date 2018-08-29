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

//#define DEBUG_GLSL

// ------------------------------------
// GLSL vertex shader
// ------------------------------------
static char *VShader="\n"
	"varying vec3 P;\n"
	"varying vec4 EyeDirection;\n"
	"varying vec4 Normal;\n"
	"varying vec4 Color;\n"
	"void main (void)\n"
	"{\n"
	"    gl_Position = ftransform();\n"
	"    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
	"#ifdef LIGHTING\n"
	"    Normal.xyz = gl_NormalMatrix * gl_Normal;\n"
	"    EyeDirection=(gl_ModelViewMatrix * gl_Vertex);\n"
	"#endif\n"
	"    Color=gl_Color;\n"
	"    P = gl_Vertex.xyz;\n"
	"}\n"
	;

//
// GLSL object shader
// inputs
// ------------------------------------
// voltex       volume data      2D[3D]-texture
// colormap     color table      1D-texture
// functionmap  contrast LUT     1D-texture
// params[0].x  color index
// params[0].y  intensity

static char *FShader1D=""
	"varying vec3 P;\n"
	"varying vec4 Color;\n"
	"uniform vec4 params;\n"
	"uniform vec4 gcol;\n"
	"uniform vec4 bcol;\n"
	"uniform int cmode;\n"
	"uniform sampler1D functionmap;\n"
	"uniform sampler1D colormap;\n"
	"#define grid params.w\n"
	"vec4 addGrid(vec4 color,vec4 col){ \n"
	"    vec2 f  = abs(fract (P.xz * grid)-0.5);\n"
	"    vec2 df = fwidth(P.xz * grid);\n"
	"    vec2 g2 = smoothstep(-df,df , f);\n"
	"    float g = g2.x * g2.y; \n"
	"    g *=1.0-g; \n"
	"    g = min(2.0*g,1.0); \n"
	"    col.rgb=mix(color.rgb,col.rgb,g);\n"
	"    return vec4(col.rgb,color.a);\n"
	"}\n"
	"#define spacing params.z\n"
	"vec4 addContours(vec4 color, vec4 col){ \n"
	"    if(spacing==0.0) return color;\n"
	//"    float y = P.y;\n"
	"    float y =sqrt(abs(P.y));\n"
	"    float f  = abs(fract (y * abs(spacing))-0.5);\n"
	"    float dy = fwidth(y * spacing);\n"
	"    float g = smoothstep(-dy,dy , f);\n"
	"    g *=1.0-g; \n"
	"    g = min(3.0*g,1.0); \n"
   // "	 g *= 1-smoothstep(0.2,0.25 , y);\n"
   // "	 g *= smoothstep(0.05,0.07 , abs(y));\n"
	"    col=mix(color,col,g);\n"
	"    return vec4(col.rgb,g);\n"
	"}\n"
	"#ifdef LIGHTING\n"
	"varying vec4 EyeDirection;\n"
	"varying vec4 Normal;\n"
	"uniform vec4 lpars;\n"
	"uniform vec4 lpos;\n"
	"vec4 addLighting(vec4 color){ \n"
	"    vec3 normal = normalize(Normal.xyz);\n"
	"    vec3 eye = normalize(EyeDirection.xyz);\n"
	"    float ldn = dot(normalize(lpos.xyz),normal);\n"
	"    float ambient = lpars.x;\n"
	"    float diffuse = lpars.y*max(ldn,0.0);\n"
	"    vec3 H = normalize((lpos.xyz + eye)*0.5);\n"
	"    float sdp = max(0.0, dot(normal,H));\n"
	"    float specular = lpars.z*pow(sdp, lpars.w);\n"
	"    vec3 col= color.rgb*(ambient+diffuse)+specular;\n"
	"    return vec4(col,color.a);\n"
	"}\n"
	"#endif\n"
	"void main (void)\n"
	"{\n"
	"    float f = gl_TexCoord[0].x;\n"
	"    vec4 fmap = texture1D(functionmap,f);\n"
	"    float b = (params.x>=0.0)?params.x:fmap.r;\n"
	"    float a = (params.y>0.5)?fmap.y:1.0;\n"
	"    vec4 tcol;\n"
	"    if(cmode<1)\n"
	"		 tcol=texture1D(colormap, b);\n"
	"    else\n"
	" 		 tcol=Color;\n"
	"    vec4 bg = params.z>=0.0?tcol:bcol;\n"
	"    vec4 fg = params.z>=0.0?bcol:tcol;\n"
	"    vec4 color=bg;\n"
	"#ifdef LIGHTING\n"
	"    color= addLighting(bg);\n"
	"#endif\n"
	"    color=addContours(color,fg);\n"
	"    color=addGrid(color,gcol);\n"
	"    gl_FragColor = color;\n"
	"    gl_FragColor.a = a;\n"
	"}\n"

    ;

static char *volumeFShader2D=""
	"uniform vec4 params;\n"
	"uniform sampler1D functionmap;\n"
	"uniform sampler1D colormap;\n"
	"uniform sampler2D voltex;\n"
	"void main (void)\n"
	"{\n"
	"    vec4 v = texture2D(voltex, gl_TexCoord[0].xy);\n"
	"    vec4 fmap = texture1D( functionmap, v.a*params.y);\n"
	"    float b = (params.x>=0.0)?params.x:fmap.r;\n"
	"    gl_FragColor = texture1D( colormap, b);\n"
	"    gl_FragColor.a = fmap.a;\n"
	"}\n";

static char *volumeFShader3D=""
	"uniform vec4 params;\n"
	"uniform sampler1D functionmap;\n"
	"uniform sampler1D colormap;\n"
	"uniform sampler3D voltex;\n"
	"void main (void)\n"
	"{\n"
	"    vec4 v = texture3D( voltex, gl_TexCoord[0].xyz );\n"
	"    vec4 fmap = texture1D( functionmap, v.a*params.y);\n"
	"    float b = (params.x>=0.0)?params.x:fmap.r;\n"
	"    gl_FragColor = texture1D( colormap, b);\n"
	"    gl_FragColor.a = fmap.a;\n"
	"}\n";
static char *mipFShader2D=""
	"uniform vec4 params;\n"
	"uniform sampler1D functionmap;\n"
	"uniform sampler1D colormap;\n"
	"uniform sampler2D voltex;\n"
	"void main (void)\n"
	"{\n"
    "    vec4 v = texture2D(voltex, gl_TexCoord[0].xy);\n"
    "    vec4 fmap = texture1D( functionmap, v.a*params.y);\n"
    "    float b = (params.x>=0.0)?params.x:fmap.r;\n"
    "    if(fmap.a>0.0){\n"
    "        gl_FragColor = texture1D( colormap, b);\n"
    "        gl_FragDepth = 1.0-fmap.a;\n"
    "     }\n"
    "     else {\n"
    "        gl_FragDepth = 2.0;\n"
    "     }\n"
	"}\n";

static char *mipFShader3D=""
	"uniform vec4 params;\n"
	"uniform sampler1D functionmap;\n"
	"uniform sampler1D colormap;\n"
	"uniform sampler3D voltex;\n"
	"void main (void)\n"
	"{\n"
    "    vec4 v = texture3D( voltex, gl_TexCoord[0].xyz );\n"
    "    vec4 fmap = texture1D( functionmap, v.a*params.y);\n"
    "    float b = (params.x>=0.0)?params.x:fmap.r;\n"
    "    if(fmap.a>0.0){\n"
    "        gl_FragColor = texture1D( colormap, b);\n"
    "        gl_FragDepth = 1.0-fmap.a;\n"
    "     }\n"
    "     else {\n"
    "        gl_FragDepth = 2.0;\n"
    "     }\n"
	"}\n";

//-------------------------------------------------------------
// GLSLProgram::GLSLProgram()   constructor
//-------------------------------------------------------------
GLSLProgram::GLSLProgram() : CGLProgram()
{
#ifdef DEBUG_GLSL
    std::cout << name() << "::" <<name()<<std::endl;
#endif
    program = glCreateProgramObjectARB();
}

//-------------------------------------------------------------
// GLSLProgram::GLSLProgram()   constructor
//-------------------------------------------------------------
GLSLProgram::~GLSLProgram()
{
#ifdef DEBUG_GLSL
    std::cout << name() << "::~" <<name()<<std::endl;
#endif
}
//-------------------------------------------------------------
// GLSLProgram::checkLogInfo()	check for shader errors
//-------------------------------------------------------------
void GLSLProgram::checkLogInfo(int obj) {
    int iVal[1];
    glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB,
            iVal);
    int length = iVal[0];
    if(length <= 1) { // no error
        return;
    }
    char *infoLog=NULL;
    MALLOC(length,char,infoLog);

    glGetInfoLogARB(obj, length, iVal, infoLog);
    std::cout << "GLSL Validation >> "<<infoLog <<std::endl;
    FREE(infoLog);
}

//-------------------------------------------------------------
// GLSLProgram::getAttribLocation() get shader attribute location
//-------------------------------------------------------------
int GLSLProgram::getAttribLocation(const char * name) {
    return (glGetAttribLocationARB(program, name));
}

//-------------------------------------------------------------
// GLSLProgram::getAttribLocation() get shader uniform location
//-------------------------------------------------------------
int GLSLProgram::getUniformLocation(const char * name) {
    int loc=0;
    loc=glGetUniformLocationARB(program, name);
    if ( loc < 0 )
        std::cerr<< " WARNING - symbol not found in shader program: " << name << std::endl;
    return loc;
}

//-------------------------------------------------------------
// GLSLProgram::setUniformFloat() set uniform float value
//-------------------------------------------------------------
bool GLSLProgram::setUniformFloat (const char * name, float value ){
    int loc = getUniformLocation(name );
    if ( loc < 0 )
        return false;
    glUniform1fARB ( loc, value );
    return true;
}

//-------------------------------------------------------------
// GLSLProgram::setUniformFloat() set uniform int value
//-------------------------------------------------------------
bool GLSLProgram::setUniformInt (const char * name, int value ){
    int loc = getUniformLocation(name );
    if ( loc < 0 )
        return false;
    glUniform1iARB ( loc, value );
    return true;
}

//-------------------------------------------------------------
// GLSLProgram::setUniformfVector() set uniform float vector
//-------------------------------------------------------------
bool GLSLProgram::setUniformfVector(char *name, float *value, int length){
    int loc = getUniformLocation(name );
    if ( loc < 0 )
        return false;
    switch(length){
    case 1:
        glUniform1fARB ( loc, value[0] );
        return true;
    case 2:
        glUniform2fARB ( loc, value[0],value[1] );
        return true;
    case 3:
        glUniform3fARB ( loc, value[0],value[1],value[2] );
        return true;
    case 4:
        glUniform4fARB ( loc, value[0],value[1],value[2],value[3]);
        return true;
    }
    return false;
 }

//-------------------------------------------------------------
// GLSLProgram::setUniformiVector() set uniform int vector
//-------------------------------------------------------------
bool GLSLProgram::setUniformiVector(char *name, int *value, int length){
    int loc = getUniformLocation(name );
    if ( loc < 0 )
        return false;
    switch(length){
    case 1:
        glUniform1iARB ( loc, value[0] );
        return true;
    case 2:
        glUniform2iARB ( loc, value[0],value[1] );
        return true;
    case 3:
        glUniform3iARB ( loc, value[0],value[1],value[3] );
        return true;
    case 4:
        glUniform4iARB ( loc, value[0],value[1],value[3],value[4] );
        return true;
    }
    return false;
}

//-------------------------------------------------------------
// GLSLProgram::setIntVector() set 4 element int vector
//-------------------------------------------------------------
bool GLSLProgram::setIntVector (char *name, int *value){
    int loc = getUniformLocation(name);
    if ( loc < 0 )
        return false;
    glUniform4iARB (loc, value[0],value[1],value[2],value[3]);
    return true;
}


//############# virtual functions ##############################
//-------------------------------------------------------------
// GLSLProgram::isSupported() return true if shader is supported
//-------------------------------------------------------------
bool GLSLProgram::isSupported(){
   if (isExtensionSupported ("GL_ARB_shading_language_100") &&
       isExtensionSupported ("GL_ARB_shader_objects") &&
       isExtensionSupported ("GL_ARB_vertex_shader") &&
       isExtensionSupported ("GL_ARB_fragment_shader")
    )
        return true;
    return false;
}

//-------------------------------------------------------------
// GLSLProgram::linkShaderProgram() link program
//-------------------------------------------------------------
void GLSLProgram::linkShaderProgram(){
    glLinkProgramARB(program);
    glValidateProgramARB(program);
    checkLogInfo(program);
#ifdef DEBUG_GLSL
    std::cout << name()<<" linkShaderProgram()"<<std::endl;
#endif
}

//-------------------------------------------------------------
// GLSLProgram::setTexture() set texture
//-------------------------------------------------------------
bool GLSLProgram::setTexture(char *tname, int texUnit) {
    int loc=glGetUniformLocationARB(program,tname);
    if(loc==-1){
        std::cout << "ERROR setTexture(" << tname << "," << texUnit<< ")" << std::endl;
        return false;
    }
    glUniform1iARB(loc, texUnit);
#ifdef DEBUG_GLSL
    std::cout << name()<<" setTexture(" << tname << "," << texUnit<< ")"<<std::endl;
#endif
    return true;
}

//-------------------------------------------------------------
// GLSLProgram::bind() bind program
//-------------------------------------------------------------
void GLSLProgram::bind() {
    glUseProgramObjectARB(program);
}

//-------------------------------------------------------------
// GLSLProgram::unbind() unbind program
//-------------------------------------------------------------
void GLSLProgram::unbind() {
    glUseProgramObjectARB(0);
}

//-------------------------------------------------------------
// GLSLProgram::enable() enable program
//-------------------------------------------------------------
void GLSLProgram::enable() {
}

//-------------------------------------------------------------
// GLSLProgram::disable() disable program
//-------------------------------------------------------------
void GLSLProgram::disable() {
}

//-------------------------------------------------------------
// GLSLProgram::loadShader() load shader from string
//-------------------------------------------------------------
bool GLSLProgram::loadShader(char *source, CGLShader *shader){
    int length =strlen(source);
    const char *src=source;
    int obj=shader->getShaderObject();
    if(obj!=0){
        glDetachObjectARB(program, obj);
        glDeleteObjectARB(obj);
        //glDeleteShader(obj);
    }
    shader->setActive();
    obj=shader->createShaderObject();
    glShaderSourceARB(obj, 1, &src, &length);
    glCompileShaderARB(obj);
    checkLogInfo(obj);
    glAttachObjectARB(program, obj);
    return true;
}

//-------------------------------------------------------------
// GLSLProgram::loadShader() load shader
//-------------------------------------------------------------
void GLSLProgram::loadShader(int mode){
    if((mode & S2D)>0){
    	loadVertexShader(VShader);
        if((mode & SMIP)>0)
            loadFragmentShader(mipFShader2D);
        else
            loadFragmentShader(volumeFShader2D);
    }
    else if((mode & S3D)>0){
    	loadVertexShader(VShader);
        if((mode & SMIP)>0)
            loadFragmentShader(mipFShader3D);
        else
            loadFragmentShader(volumeFShader3D);
     }
    else{ // 1D
        if((mode & SLGT)>0){
        	char tmp[2000];
        	sprintf(tmp,"#define LIGHTING\n%s",VShader);
        	loadVertexShader(tmp);
        	sprintf(tmp,"#define LIGHTING\n%s",FShader1D);
        	loadFragmentShader(tmp);
        }
        else{
        	loadVertexShader(VShader);
    	    loadFragmentShader(FShader1D);
        }
    }
    linkShaderProgram();
}

//-------------------------------------------------------------
// GLSLProgram::setFloatVector() load simple volume shader
//-------------------------------------------------------------
bool GLSLProgram::setFloatVector (char *name, float *value){
    int loc = getUniformLocation(name);
    if ( loc < 0 )
        return false;
    glUniform4fARB (loc, value[0],value[1],value[2],value[3]);
    return true;
}

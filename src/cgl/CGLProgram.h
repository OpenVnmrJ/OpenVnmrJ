/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* %Z%%M% %I% %G% Copyright (c) 1991-1996 Varian Assoc.,Inc. All Rights Reserved
 */

#ifndef CGLPROGRAM_H_
#define CGLPROGRAM_H_

#include "CGLShader.h"

enum {
	S1D = 0x00,
    S2D = 0x01,
    S3D = 0x02,
    SVOL = 0x04,
    SMIP = 0x08,
    SLGT = 0x10,
    VOL3D=SVOL|S3D, 
    MIP3D=SMIP|S3D, 
    VOL2D=SVOL|S2D, 
    MIP2D=SMIP|S2D,
    HT1D=S1D,
    LGT1D=S1D|SLGT
};

class CGLProgram
{
protected:
    int program;
    virtual bool checkError(int obj) { return true; }    
    virtual bool loadShader(char *source, CGLShader *shader) { return false;}
    bool loadVertexShader(char * source) {return loadShader(source,vshader);}  
    bool loadFragmentShader(char * source) {return loadShader(source,fshader);}
    
    CGLShader *fshader;
    CGLShader *vshader;
public:
    CGLProgram();       
    ~CGLProgram();
      
    void showGLInfo();
    
    virtual const char *name() {return "generic";}
    virtual bool setTexture(char * name, int texUnit) {return false;}
    virtual bool isSupported(){ return false;}
    virtual void enable() { }
    virtual void disable() { } 
    virtual void bind() {}
    virtual void unbind() {}
    virtual bool setFloatVector (char *name, float *value) { return false;}
    virtual bool setFloatVector (char *name, float *value,int id) { return false;}
    virtual bool setIntVector (char *name, int *value ) { return false;}
    virtual bool setIntValue (char *name, int value ) { return false;}
    virtual void loadShader(int m) { }
    virtual int type() { return NONE;}
 };

class ARBProgram : public CGLProgram
{
    protected:  
    bool loadShader(char *source, CGLShader *shader);
    char **symbols;
    int sid;
    void initSymbolTable();
    int getParameter(char *);
public:  
    ARBProgram();
    ~ARBProgram();
    const char *name() { return "ARB_program";}
    int type() { return ARB;}

    bool isSupported();
    void enable();
    void disable(); 
    void bind();
    void unbind();   
    bool setFloatVector (char *name, float *value);
    bool setFloatVector (char *name, float *value,int id);
    void loadShader(int m);
 };

class GLSLProgram : public CGLProgram
{ 
    void linkShaderProgram();     
    int getUniformLocation(const char *name);   
    bool setUniformFloat (const char *name, float value );
    bool setUniformInt (const char *name, int value );
    bool setUniformfVector (char *name, float *value, int length);
    bool setUniformiVector (char * name, int *value, int length);
    int getAttribLocation(const char * name); 
    void checkLogInfo(int obj);
    bool loadShader(char *source, CGLShader *shader);
public:  
       
    GLSLProgram();
    ~GLSLProgram();
    const char *name() { return "GLSL";}
    int type() { return GLSL;}
    bool setTexture(char * name, int texUnit);
    bool isSupported();
    void enable();
    void disable(); 
    void bind();
    void unbind();   
    bool setFloatVector (char *name, float *value);
    bool setFloatVector (char *name, float *value, int id){
    	return setFloatVector(name,value);
    }
    bool setIntVector (char *name, int *value );

    bool setIntValue (char *name, int value ){
    	return setUniformInt(name,value);
    }
    void loadShader(int m);
    };
#endif

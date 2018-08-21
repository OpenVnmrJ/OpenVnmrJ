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

#ifndef CGLSHADER_H_
#define CGLSHADER_H_

#include "CGLDef.h"
#include "GLUtil.h"

class CGLShader
{
protected:
    int maxAluInstructions;
    int maxTexInstructions;
    int maxTexIndirections;
    int maxInstructions;
    int maxLocalParams;
    int maxEnvParams;
    int maxTemporaries;
    int maxProgramAttribs;

    int program_type;
    int shader_type;
    bool active;

    int shaderObj;
    int id;

    GLuint Id[1];

public:
    CGLShader(int s, int p);
    ~CGLShader();

    virtual void initProperties();
    virtual void showGLInfo();

    GLint getProgramVariable(GLint attr);

    int createShaderObject(){
        shaderObj=glCreateShaderObjectARB(shader_type);
        return shaderObj;
    }

    void setActive(){
        if(!active){
            glGenProgramsARB (1, Id);
            id=Id[0];
        }
        active=true;
    }
    void setInactive(){
        id=0;
        if(active)
            glDeleteProgramsARB(1, Id);
        active=false;
    }
    bool isActive(){
        return active;
    }

    int getProgramType () {
        return program_type;
    }

    int getShaderObject(){
        return shaderObj;
    }
    void bind(){
        if(active)
            glBindProgramARB (program_type, Id[0] );
    }
    void unbind(){
        if(active)
            glBindProgramARB (program_type, 0);
    }
    void enable(){
        if(active)
            glEnable(program_type);
    }
    void disable(){
        glDisable(program_type);
    }

    // required overrides

    virtual bool isSupported (){
        return false;
    }
    virtual const char *name() {
        return "generic";
    }

};

class CGLVertexShader : public CGLShader
{
public:
    CGLVertexShader();
    bool isSupported (){
        return isExtensionSupported ("GL_ARB_vertex_program");
    }
};

class CGLFragmentShader : public CGLShader
{
public:
    CGLFragmentShader();

    bool isSupported (){
        return isExtensionSupported ("GL_ARB_fragment_program");
    }
};
#endif

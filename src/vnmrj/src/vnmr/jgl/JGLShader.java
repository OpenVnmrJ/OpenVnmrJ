/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

import javax.media.opengl.*;
import javax.media.opengl.glu.*;

import vnmr.util.DebugOutput;

public class JGLShader
{
    protected boolean active=false;
    public int maxAluInstructions;
    public int maxTexInstructions;
    public int maxTexIndirections;
    public int maxInstructions;
    public int maxLocalParams;
    public int maxEnvParams;
    public int maxTemporaries;
    public int maxProgramAttribs;
    
    protected String errorString="";
    protected int program_type=0;
    protected int shader_type=0;
    
    int shaderObj=0;
    int id=0;

    protected GL2 gl=null;
    
    int Id[]=new int[1];
    
    JGLShader(int s, int p){
        Id[0]=0;
        program_type=p;
        shader_type=s;
        gl=GLU.getCurrentGL().getGL2();
        if(DebugOutput.isSetFor("gpu"))
            System.out.println(name()+" create("+id+")");
        initProperties();
     }
    public void destroy(){
        setInactive();
        if(DebugOutput.isSetFor("gpu"))
            System.out.println(name()+" destroy("+id+")");
        if(shaderObj!=0)
            gl.glDeleteObjectARB(shaderObj);
        shaderObj=0;
    }
    private void initProperties(){
        maxAluInstructions=getProgramVariable(GL2.GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB);
        maxTexInstructions=getProgramVariable(GL2.GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB);
        maxTexIndirections=getProgramVariable(GL2.GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB);
        maxInstructions=getProgramVariable(GL2.GL_MAX_PROGRAM_INSTRUCTIONS_ARB);
        maxLocalParams=getProgramVariable(GL2.GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB);
        maxEnvParams=getProgramVariable(GL2.GL_MAX_PROGRAM_ENV_PARAMETERS_ARB);
        maxTemporaries=getProgramVariable(GL2.GL_MAX_PROGRAM_TEMPORARIES_ARB);
        maxProgramAttribs=getProgramVariable(GL2.GL_MAX_PROGRAM_ATTRIBS_ARB);
    }

    public void showGLInfo(){
        System.out.println(name()+ " Shader attributes:");
        System.out.println(" Max Program Instructions: "+maxInstructions);
        System.out.println(" Max ALU Instructions: "+maxAluInstructions);
        System.out.println(" Max Texture Instructions: "+maxTexInstructions);
        System.out.println(" Max Texture Indirections: "+maxTexIndirections);
        System.out.println(" Max Local Parameters: "+maxLocalParams);
        System.out.println(" Max Environment Parameters: "+maxEnvParams);
        System.out.println(" Max Temporaries: "+maxTemporaries);
        System.out.println(" Max Program Attributes: "+maxProgramAttribs);
    }
    public int getProgramVariable(int attr){
        gl.glGetProgramivARB ( program_type, attr, Id,0);
        return Id[0];
    }
    public int getProgramType () {
        return program_type;
    }
    public int getShaderType () {
        return shader_type;
    }
    
    String  getErrorString () {
        return errorString;
    }

    public int getShaderObject(){
        return shaderObj;
    }   

    public int createShaderObject(){
        shaderObj=gl.glCreateShaderObjectARB(shader_type);
        return shaderObj;
    }   

    public void setActive(){
        if(!active){
            gl.glGenProgramsARB (1, Id,0);
            id=Id[0];
        }
        active=true;
    }
    public void setInactive(){
        id=0;
        if(active && Id[0] !=0)
            gl.glDeleteProgramsARB(1, Id,0 );
        Id[0]=0;
        active=false;
    } 
    public boolean isActive(){
        return active;
    }
    public void bind(){
        if(active){
            gl.glBindProgramARB (program_type, Id[0] );
            if(DebugOutput.isSetFor("gpu"))
                System.out.println(name()+" bind("+id+")");
        }
    }   
    public void unbind(){
        if(active){
            gl.glBindProgramARB (program_type, 0);
            if(DebugOutput.isSetFor("gpu"))
                System.out.println(name()+" unbind("+id+")");
        }
    }   
    public void enable(){
        if(active){
            gl.glEnable(program_type);
            if(DebugOutput.isSetFor("gpu"))
                System.out.println(name()+" enable("+id+")");
        }
    }
    public void disable(){
        gl.glDisable(program_type);
        if(DebugOutput.isSetFor("gpu"))
            System.out.println(name()+" disable("+id+")");
    }
    
    boolean load (String fileName ){
        return false;
    }

    // required overrides

    public void setProgramType() {
        program_type=0;
    }
    public void setShaderType() {
        shader_type=0;
    }
    String  name () {
        return "";
    }
    public  boolean isSupported (){
        return false;
    }
}

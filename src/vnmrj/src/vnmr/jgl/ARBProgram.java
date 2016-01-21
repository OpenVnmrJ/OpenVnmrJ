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
import java.util.Hashtable;

public class ARBProgram extends JGLShaderProgram
{ 
    Hashtable<String, Integer> symbols=new Hashtable<String, Integer>();
    int sid=0;
 
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
    
    // Notes: The current ARB shader does not support the
    //        following glsl supported effects
    //        1. anti-aliased constant-width contours 
    //           - ARB contours are variable width and
    //             are based on fmap value not vertex ht
    //           - no "fwidth" ARB operator so may not be possible 
    //        2. lighting effects 
    //           - could probably be supported in the future
    //        3. 2d grid 
    //           - glsl uses fwidth so same problem as for contours

    static public String htFShader1D=new String(""
            +"!!ARBfp1.0\n"
            +"PARAM params = program.env[0];\n"
            +"PARAM bcol = program.env[1];\n"
            +"TEMP  temp, fmap, cmap, color;\n"
            +"TEX   fmap, fragment.texcoord [0], texture [1], 1D;\n"
            +"SUB   temp.x, params.x,0.5;\n"
            +"CMP   cmap.x, temp.x, fmap.x, params.x;\n"
            +"CMP   cmap.y, params.y, 1.0, fmap.y;\n"
            +"TEX   color, cmap.x, texture [0], 1D;\n"           
            // contours
            +"ABS   temp.z, params.z;\n"       // contour frequency
            +"MUL   temp.x, fmap.x, temp.z;\n" // should use vertex ht
            +"FLR   temp.y, temp.x;\n"
            +"SUB   temp.y, temp.x,temp.y;\n"
            +"ABS   temp.y, temp.y;\n"            
            +"SUB   temp.y, temp.y, 0.25;\n" // contours 1/4 of band
            +"SUB   temp.z, 0.0, temp.z;\n"  // if <0 contours enabled
            +"MUL   temp.y, temp.y,params.z;\n" // reverse sign if <0
            +"CMP   temp.y, temp.z, temp.y, -1.0;\n"
            +"CMP   color, temp.y, color, bcol;\n"
 
            +"CMP   temp.x, params.y, 1.0, fmap.y;\n"
            +"MOV   color.a, temp.x;\n"
            +"MOV   result.color, color;\n"
            +"END\n"
             );

    static String volumeFShader2D=new String(""
            +"!!ARBfp1.0\n"
            +"PARAM params = program.env[0];\n"
            +"TEMP  vol, temp, cmap, color;\n"
            +"TEX   vol, fragment.texcoord [0], texture [0], 2D;\n"
            +"MUL   temp.x, vol.a, params.y;\n"
            +"CMP   temp.y, params.x, temp.x, params.x;\n"
            +"TEX   cmap, temp.y, texture [2], 1D;\n"
            +"TEX   color, cmap.r, texture [1], 1D;\n"
            +"MOV   color.a, cmap.a;\n"
            +"MOV   result.color, color;\n"
            +"END\n"
             );
     static String volumeFShader3D=new String(""
            +"!!ARBfp1.0\n"
            +"PARAM params = program.env[0];\n"
            +"TEMP  vol, temp, cmap, color;\n"
            +"TEX   vol, fragment.texcoord [0], texture [0], 3D;\n"
            +"MUL   temp.x, vol.a, params.y;\n"
            +"CMP   temp.y, params.x, temp.x, params.x;\n"
            +"TEX   cmap, temp.y, texture [2], 1D;\n"
            +"TEX   color, cmap.r, texture [1], 1D;\n"
            +"MOV   color.a, cmap.a;\n"
            +"MOV   result.color, color;\n"
            +"END\n"
             );

     static String mipFShader2D=new String(""
             +"!!ARBfp1.0\n"
             +"PARAM params = program.env[0];\n"
             +"TEMP  vol, temp, cmap, color;\n"
             +"TEX   vol, fragment.texcoord [0], texture [0], 2D;\n"
             +"MUL   temp.x, vol.a, params.y;\n"
             +"CMP   temp.y, params.x, temp.x, params.x;\n"
             +"TEX   cmap, temp.y, texture [2], 1D;\n"
             +"TEX   color, cmap.r, texture [1], 1D;\n"
             +"SUB   temp.z,1.0,cmap.a;\n"
             +"SUB   temp.y,0.0,cmap.a;\n"
             +"CMP   result.depth, temp.y, temp.z, 2.0;\n"
             +"CMP   result.color, temp.y,color, fragment.color;\n"
             +"END\n"
              );
     static String mipFShader3D=new String(""
             +"!!ARBfp1.0\n"
             +"PARAM params = program.env[0];\n"
             +"TEMP  vol, temp, cmap, color;\n"
             +"TEX   vol, fragment.texcoord [0], texture [0], 3D;\n"
             +"MUL   temp.x, vol.a, params.y;\n"
             +"CMP   temp.y, params.x, temp.x, params.x;\n"
             +"TEX   cmap, temp.y, texture [2], 1D;\n"
             +"TEX   color, cmap.r, texture [1], 1D;\n"
             +"SUB   temp.z,1.0,cmap.a;\n"
             +"SUB   temp.y,0.0,cmap.a;\n"
             +"CMP   result.depth, temp.y, temp.z, 2.0;\n"
             +"CMP   result.color, temp.y,color, fragment.color;\n"
             +"END\n"
             );

    protected boolean loadShader(String source, JGLShader shader){
        shader.setActive();
        shader.bind();
        gl.glProgramStringARB (shader.getProgramType(), GL2.GL_PROGRAM_FORMAT_ASCII_ARB,
                source.length(), source);
        
        if ( gl.glGetError () == GL2.GL_INVALID_OPERATION ){
            //IntBuffer errCode=IntBuffer.allocate(1);
            //gl.glGetIntegerv ( GL.GL_PROGRAM_ERROR_POSITION_ARB, errCode );
            String errorString = gl.glGetString ( GL2.GL_PROGRAM_ERROR_STRING_ARB );
            System.out.println("GPU shader error >> " + errorString);
            return false;
        }
        return true;
    }

    ARBProgram(){
        super();
    }
    public static String name(){
        return "ARB_program";  
    }
       
    public static boolean isSupported() {
        if(GLUtil.isExtensionSupported("GL_ARB_fragment_program")
                && GLUtil.isExtensionSupported("GL_ARB_vertex_program"))
            return true;
        return false;
    }
    
    public void bind() {
        vshader.bind();
        fshader.bind();
    }

    public void unbind () {
        vshader.unbind();
        fshader.unbind();
    }

    public void enable() {
        vshader.enable();
        fshader.enable();
    }

    public void disable() {
        vshader.disable();
        fshader.disable();
    } 

    public boolean setFloatVector (String name, float value[]){
        Integer pid;
        if(symbols.containsKey(name))
            pid=(Integer)symbols.get(name);
        else{
            pid=sid++;
            symbols.put(name,pid);
        }
        float d[]=new float[4];
        int n=value.length>4?4:value.length;
        for(int i=0;i<n;i++){
            if(i<value.length)
                d[i]=value[i];
            else
                d[i]=0.0f;               
        }        
        if(vshader.isActive())
            gl.glProgramEnvParameter4fARB(vshader.getProgramType(),pid,d[0],d[1],d[2],d[3]);
        if(fshader.isActive())
            gl.glProgramEnvParameter4fARB(fshader.getProgramType(),pid,d[0],d[1],d[2],d[3]);
        return true;
    }
    
    public void loadShader(int mode){
        if((mode & S2D)>0){
            if((mode & SMIP)>0)
                loadFragmentShader(mipFShader2D);
            else
                loadFragmentShader(volumeFShader2D);
        }
        else if((mode & S3D)>0){
            if((mode & SMIP)>0)
                loadFragmentShader(mipFShader3D);
            else
                loadFragmentShader(volumeFShader3D);            
        }
        else
        	loadFragmentShader(htFShader1D);
    }   

}

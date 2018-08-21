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

import java.io.*;

import java.nio.IntBuffer;
import java.nio.ByteBuffer;
import java.util.Hashtable;
import java.util.StringTokenizer;

public class JGLShaderProgram
{
    protected GL2 gl=null;
    protected int program=0;
    
    protected JGLShader fshader=null;
    protected JGLShader vshader=null;
    
    public static final int S1D  = 0x00;
    public static final int S2D  = 0x01;
    public static final int S3D  = 0x02;
    public static final int SVOL = 0x04;
    public static final int SMIP = 0x08;
    public static final int SLGT = 0x10;
    
    public static final int VOL3D=SVOL|S3D;
    public static final int MIP3D=SMIP|S3D;
    public static final int VOL2D=SVOL|S2D;
    public static final int MIP2D=SMIP|S2D;
    public static final int HT1D=S1D;
    public static final int LGT1D=S1D|SLGT;
        
    JGLShaderProgram(){
        gl=GLU.getCurrentGL().getGL2();
        fshader=new JGLFragmentShader();
        vshader=new JGLVertexShader();
    }

    public void showGLInfo(){
        if(gl==null)
            return;
        IntBuffer intbuffer=IntBuffer.allocate(1);       
        System.out.println("Shader program type: "+name());
        gl.glGetIntegerv( GL2.GL_MAX_VERTEX_UNIFORM_COMPONENTS,intbuffer);
        System.out.println(" Max Vertex Uniform Componants: "+intbuffer.get(0));
        gl.glGetIntegerv( GL2.GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,intbuffer);
        System.out.println(" Max Fragment Uniform Componants: "+intbuffer.get(0));
        gl.glGetIntegerv( GL2.GL_MAX_VERTEX_ATTRIBS_ARB,intbuffer);
        System.out.println(" Max Vertex Attributes: "+intbuffer.get(0));
        gl.glGetIntegerv( GL2.GL_MAX_TEXTURE_IMAGE_UNITS_ARB,intbuffer);
        System.out.println(" Max Fragment Texture Units: "+intbuffer.get(0));
        gl.glGetIntegerv( GL2.GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,intbuffer);
        System.out.println(" Max Vertex Texture Units: "+intbuffer.get(0));
        gl.glGetIntegerv( GL2.GL_MAX_VARYING_FLOATS,intbuffer);
        System.out.println(" Max Varying Floats: "+intbuffer.get(0));
        gl.glGetIntegerv( GL2.GL_MAX_TEXTURE_COORDS_ARB,intbuffer);
        System.out.println(" Max Texture Coords: "+intbuffer.get(0));
        vshader.showGLInfo();
        fshader.showGLInfo();       
    }

    public static String ReadShaderSourceFile(String filename){
        String res = "";
        try{
            BufferedReader br = new BufferedReader(new FileReader(filename));
            String line;
            while ((line=br.readLine()) != null) {
                res += line + "\n";
            }
        } catch (IOException e){
            System.err.println(e);
        }
        return res;
    }

    public boolean loadVertexShaderFile(String vertexFileName) {
        String shaderSource = ReadShaderSourceFile(vertexFileName);
        if(shaderSource==null)
            return false;        
        return loadVertexShader(shaderSource);
    }
    public boolean loadFragmentShaderFile(String fragmentFileName) {
        String shaderSource = ReadShaderSourceFile(fragmentFileName);
        if(shaderSource==null)
            return false;        
        return loadFragmentShader(shaderSource);
    }

    // the following functions should be overridden in extended classes

    protected boolean loadShader(String source, JGLShader shader){
        return false;
    }

    protected boolean checkError(int obj) {
        return true;
    }  

    public void destroy(){
        vshader.destroy();        
        fshader.destroy();        
        gl=null;
    }

    public static String name(){
        return "generic";  
    }

    public static boolean isSupported(){
        return false;
    }

    public void enable() {
    }

    public void disable() {
    } 

    public void bind() {
    }

    public void unbind() {
    } 
    
    public boolean loadVertexShader(String source) {
        return loadShader(source,vshader);
    }
    public boolean loadFragmentShader(String source) {
        return loadShader(source,fshader);
    }
     
    public boolean setFloatValue (String name, float value ){
        return false;
    }

    public boolean setIntValue (String name, int value ){
        return false;
    }

    public boolean setFloatVector (String name, float value[] ){
        return false;
    }
    public boolean setIntVector (String name, int value[] ){
        return false;
    }
    public boolean setTexture(String name, int texUnit) {
        return false;
    }
    public void loadShader (int m){
    }   
}

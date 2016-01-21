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

public class GLSLProgram extends JGLShaderProgram
{ 
    int program_obj;
    
    static public String Defs="";
    
    // ------------------------------------
    // GLSL vertex shader
    // ------------------------------------

    static public String VShader=new String(""
    +"varying vec3 P;\n"
    +"varying vec4 EyeDirection;\n"
    +"varying vec4 Normal;\n"
    +"varying vec4 Color;\n"
    +"void main (void)\n"
    +"{\n"
    +"    gl_Position = ftransform();\n"
    +"    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    +"#ifdef LIGHTING\n"
    +"    Normal.xyz = gl_NormalMatrix * gl_Normal;\n"
    +"    EyeDirection=(gl_ModelViewMatrix * gl_Vertex);\n"
    +"#endif\n"
    +"    Color=gl_Color;\n"
    +"    P = gl_Vertex.xyz;\n"
    +"}\n"
    );

    // 
    // GLSL fragment shader
    // ------------------------------------
    // inputs
    // ------------------------------------
    // [voltex]     volume data      2D[3D]-texture
    // colormap     color palette    1D-texture
    // functionmap  contrast LUT     1D-texture
    // params[0].x  color index
    // params[0].y  intensity
    // params[0].z  threshold

    static public String FShader1D=new String(""
    +"varying vec3 P;\n"
    +"varying vec4 Color;\n"
    +"uniform vec4 params;\n"
    +"uniform vec4 gcol;\n"
    +"uniform vec4 bcol;\n"
    +"uniform int cmode;\n"
    +"uniform sampler1D functionmap;\n"
    +"uniform sampler1D colormap;\n"
    +"#define grid params.w\n"
    +"vec4 addGrid(vec4 color,vec4 col){ \n"
    +"    vec2 f  = abs(fract (P.xz * grid)-0.5);\n"
    +"    vec2 df = fwidth(P.xz * grid);\n"
    +"    vec2 g2 = smoothstep(-df,df , f);\n"
    +"    float g = g2.x * g2.y; \n"
    +"    g *=1.0-g; \n"
    +"    g = min(2.0*g,1.0); \n"
    +"    vec3 c=mix(color.rgb,col.rgb,g);\n"
    +"    return vec4(c,color.a);\n"
    +"}\n"
    +"#define spacing params.z\n"
    +"vec4 addContours(vec4 color, vec4 col){ \n"
    //+"    if(spacing==0.0) return color;\n"
    //+"    float y = P.y;\n"
   +"    float y = pow(abs(P.y),0.5);\n"
     +"    float f  = abs(fract (y * abs(spacing))-0.5);\n"
    +"    float dy = fwidth(y * spacing);\n"
    +"    float g = smoothstep(-dy,dy , f);\n"
    +"    g *=1.0-g; \n"
    +"    g = min(3.0*g,1.0); \n"
    //+"    g *= 1.0-smoothstep(0.2,0.22,abs(y)); \n"
    +"    vec3 c=mix(color.rgb,col.rgb,g);\n"
    +"    return vec4(c,g);\n"
    +"}\n"
    +"#ifdef LIGHTING\n"
    +"varying vec4 EyeDirection;\n"
    +"varying vec4 Normal;\n"
    +"uniform vec4 lpars;\n"
    +"uniform vec4 lpos;\n"    
    +"vec4 addLighting(vec4 color){ \n"
    +"    vec3 normal = normalize(Normal.xyz);\n"
    +"    vec3 eye = normalize(EyeDirection.xyz);\n"
    +"    float ldn = dot(normalize(lpos.xyz),normal);\n"
    +"    float ambient = lpars.x;\n"
    +"    float diffuse = lpars.y*max(ldn,0.0);\n"
    +"    vec3 H = normalize((lpos.xyz + eye)*0.5);\n"
    +"    float sdp = max(0.0, dot(normal,H));\n"
    +"    float specular = lpars.z*pow(sdp, lpars.w);\n"
    +"    vec3 col= color.rgb*(ambient+diffuse)+specular;\n"
    +"    return vec4(col,color.a);\n"   
    +"}\n"
    +"#endif\n"
    +"void main (void)\n"
    +"{\n"
    +"    float f = gl_TexCoord[0].x;\n"
    +"    vec4 fmap = texture1D(functionmap,f);\n"
    +"    float b = (params.x>=0.0)?params.x:fmap.r;\n"
    +"    float a = (params.y>0.5)?fmap.y:1.0;\n"
    +"    vec4 tcol;\n"
    +"    if(cmode<1)\n"
    +"		  tcol=texture1D(colormap, b);\n"
    +"    else\n"
    +" 		  tcol=Color;\n"
    +"    vec4 bg = params.z>=0.0?tcol:bcol;\n"
    +"    vec4 fg = params.z>=0.0?bcol:tcol;\n"
    +"    vec4 color=bg;\n"    
    +"#ifdef LIGHTING\n"
    +"    color= addLighting(bg);\n"    
    +"#endif\n"
    +"    float alpha=color.a;\n"   
    +"    color=addContours(color,fg);\n"   
    +"    color=addGrid(color,gcol);\n"
    +"    gl_FragColor = color;\n"
    +"    gl_FragColor.a = a*alpha;\n"    
    +"}\n"
    );

    
    // 3D Fragment Shaders
    
    static public String volumeFShader2D=new String(""
    +"uniform vec4 params;\n"
    +"uniform sampler1D functionmap;\n"
    +"uniform sampler1D colormap;\n"
    +"uniform sampler2D voltex;\n"
    +"void main (void)\n"
    +"{\n"
    +"    vec4 v = texture2D(voltex, gl_TexCoord[0].xy);\n"
    +"    vec4 fmap = texture1D( functionmap, v.a*params.y);\n"
    +"    float b = (params.x>=0.0)?params.x:fmap.r;\n"
    +"    gl_FragColor = texture1D( colormap, b);\n"
    +"    gl_FragColor.a = fmap.a;\n"
    +"}\n"
     );
    static public String volumeFShader3D=new String(""
    +"uniform vec4 params;\n"
    +"uniform sampler1D functionmap;\n"
    +"uniform sampler1D colormap;\n"
    +"uniform sampler3D voltex;\n"
    +"void main (void)\n"
    +"{\n"
    +"    vec4 v = texture3D( voltex, gl_TexCoord[0].xyz );\n"
    +"    vec4 fmap = texture1D( functionmap, v.a*params.y);\n"
    +"    float b = (params.x>=0.0)?params.x:fmap.r;\n"
    +"    gl_FragColor = texture1D( colormap, b);\n"
    +"    gl_FragColor.a = fmap.a;\n"
    +"}\n"
     );

    static public String mipFShader2D=new String(""
    +"uniform vec4 params;\n"
    +"uniform sampler1D functionmap;\n"
    +"uniform sampler1D colormap;\n"
    +"uniform sampler2D voltex;\n"
    +"void main (void)\n"
    +"{\n"
    +"    vec4 v = texture2D(voltex, gl_TexCoord[0].xy);\n"
    +"    vec4 fmap = texture1D( functionmap, v.a*params.y);\n"
    +"    float b = (params.x>=0.0)?params.x:fmap.r;\n"
    +"    if(fmap.a>0.0){\n"
    +"        gl_FragColor = texture1D( colormap, b);\n"
    +"        gl_FragDepth = 1.0-fmap.a;\n"
    +     "}\n"
    +"     else {\n"
    +"        gl_FragDepth = 2.0;\n"
    +     "}\n"
    +"}\n"
     );
    static public String mipFShader3D=new String(""
    +"uniform vec4 params;\n"
    +"uniform sampler1D functionmap;\n"
    +"uniform sampler1D colormap;\n"
    +"uniform sampler3D voltex;\n"
    +"void main (void)\n"
    +"{\n"
    +"    vec4 v = texture3D( voltex, gl_TexCoord[0].xyz );\n"
    +"    vec4 fmap = texture1D( functionmap, v.a*params.y);\n"
    +"    float b = (params.x>=0.0)?params.x:fmap.r;\n"
    +"    if(fmap.a>0.0){\n"
    +"        gl_FragColor = texture1D( colormap, b);\n"
    +"        gl_FragDepth = 1.0-fmap.a;\n"
    +     "}\n"
    +"     else {\n"
    +"        gl_FragDepth = 2.0;\n"
    +     "}\n"
    +"}\n"
     );

    GLSLProgram(){
        super();
        program = gl.glCreateProgramObjectARB();
    }

    private int getUniformLocation(String name) {
        int loc=0;
        loc=gl.glGetUniformLocationARB(program, name);
        //if ( loc < 0 )
       //     System.err.println(name()
        //            +" WARNING - symbol not found in shader program: "+name);
        return loc;
    }
     
    private boolean setUniformFloat (String name, float value ){
        int loc = getUniformLocation(name );
        if ( loc < 0 )
            return false;
        gl.glUniform1fARB ( loc, value );
        return true;
    }

    private boolean setUniformInt (String name, int value ){
        int loc = getUniformLocation(name );
        if ( loc < 0 )
            return false;
        gl.glUniform1iARB ( loc, value );
        return true;
    }

    private boolean setUniformfVector (String name, float value[] ){
        int loc = getUniformLocation(name );
        if ( loc < 0 )
            return false;
        switch(value.length){
        case 1:
            gl.glUniform1fARB ( loc, value[0] );
            return true;
        case 2:
            gl.glUniform2fARB ( loc, value[0],value[1] );
            return true;
        case 3:
            gl.glUniform3fARB ( loc, value[0],value[1],value[2] );
            return true;
        case 4:
            gl.glUniform4fARB ( loc, value[0],value[1],value[2],value[3] );
            return true;
        }
        return false;
    }

    private boolean setUniformiVector (String name, int value[] ){
        int loc = getUniformLocation(name );
        if ( loc < 0 )
            return false;
        switch(value.length){
        case 1:
            gl.glUniform1iARB ( loc, value[0] );
            return true;
        case 2:
            gl.glUniform2iARB ( loc, value[0],value[1] );
            return true;
        case 3:
            gl.glUniform3iARB ( loc, value[0],value[1],value[2] );
            return true;
        case 4:
            gl.glUniform4iARB ( loc, value[0],value[1],value[2],value[3] );
            return true;
       }
        return false;
    }
    private void linkShaderProgram(){
        gl.glLinkProgramARB(program);
        gl.glValidateProgramARB(program);
        checkError(program);
        if(DebugOutput.isSetFor("glsl"))
            System.out.println(name()+" linkShaderProgram()");
     }
   
    //  functions overridden from base class

    protected boolean loadShader(String source, JGLShader shader){
        int obj=shader.getShaderObject();
        if(obj!=0){
            gl.glDetachObjectARB(program, obj);
            gl.glDeleteShader(obj);
        }
        shader.setActive();
        obj=shader.createShaderObject();
        gl.glShaderSourceARB(obj, 1, new String[] { source }, (int[])null,
                0);
        gl.glCompileShaderARB(obj);
        if(checkError(obj))
            return false;
        gl.glAttachObjectARB(program, obj);
        if(DebugOutput.isSetFor("glsl"))
            System.out.println(name()+" "+shader.name());
        return true;        
    }
  
    protected boolean checkError(int obj) {
        IntBuffer iVal = IntBuffer.allocate(1);
        gl.glGetObjectParameterivARB(obj, GL2.GL_OBJECT_INFO_LOG_LENGTH_ARB,
                iVal);

        int length = iVal.get();
        if(length <= 1) {
            return false;
        }
        ByteBuffer infoLog = ByteBuffer.allocate(length);
        iVal.flip();
        gl.glGetInfoLogARB(obj, length, iVal, infoLog);
        byte[] infoBytes = new byte[length];
        infoLog.get(infoBytes);
        System.out.println("GPU shader error >> " + new String(infoBytes));
        return true;
    }  

    public static String name(){
        return "GLSL";  
    }

    public static boolean isSupported() {
        if(GLUtil.isExtensionSupported("GL_ARB_shading_language_100")
                && GLUtil.isExtensionSupported("GL_ARB_shader_objects")
                && GLUtil.isExtensionSupported("GL_ARB_vertex_shader")
                && GLUtil.isExtensionSupported("GL_ARB_fragment_shader"))
            return true;
        return false;
    }

    public void destroy(){
        GL2 gl=GLU.getCurrentGL().getGL2();
        if(DebugOutput.isSetFor("gpu"))
            System.out.println(name()+" destroy("+program+")");
        if(program>0){
            int obj;
            obj=vshader.getShaderObject();
            if(obj>0)
                gl.glDetachObjectARB(program, obj);
            obj=fshader.getShaderObject();
            if(obj>0)
                gl.glDetachObjectARB(program, obj);
            gl.glDeleteObjectARB(program);
        }
        program=0;
        super.destroy();
    }

    public void bind() {
        if(program !=0)
        gl.glUseProgramObjectARB(program);
    }

    public void unbind() {
        gl.glUseProgramObjectARB(0);
    } 

    public boolean setFloatValue (String name, float value ){
        return setUniformFloat(name,value);    
    }

    public boolean setIntValue (String name, int value ){
        return setUniformInt(name,value);
    }

    public boolean setFloatVector (String name, float value[] ){
        return setUniformfVector(name,value);
    }

    public boolean setIntVector (String name, int value[] ){
        return setUniformiVector(name,value);
    }
    public boolean setTexture(String name, int texUnit) {
        int loc=gl.glGetUniformLocation(program,name);
        if(loc==-1){
            System.out.println("ERROR setTexture("+name+","+texUnit+")");
            return false;
        }
        gl.glUniform1iARB(loc, texUnit);
        if(DebugOutput.isSetFor("glsl"))
            System.out.println(name()+" setTexture("+name+","+texUnit+")");
        return true;
    }

    public void loadShader(int mode){
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
            	Defs="#define LIGHTING\n";
            	//loadVertexShader(lgtVShader);
            	//loadFragmentShader(lgtFShader1D);
            }
            else{
            	Defs="";
            }
            String vs=Defs+VShader;
            String fs=Defs+FShader1D;
            loadVertexShader(vs);
        	loadFragmentShader(fs);
        }
        linkShaderProgram();  
    }   
}

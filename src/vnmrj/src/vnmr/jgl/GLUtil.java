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
import javax.media.opengl.glu.GLU;

import vnmr.util.DebugOutput;

import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.geom.AffineTransform;
import java.awt.image.BufferedImage;
import java.nio.IntBuffer;
import java.util.Hashtable;
import java.util.StringTokenizer;


public class GLUtil  implements JGLDef{
    static protected Hashtable glExtensions=null;
    
    static int shader=NONE;

    private GLUtil() { }
    public static BufferedImage scaleImage(BufferedImage img, float xScale, float yScale) {
        BufferedImage scaled = new BufferedImage((int) (img.getWidth() * xScale),
                                                 (int) (img.getHeight() * yScale),
                                                 BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = scaled.createGraphics();
        g.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
        g.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BICUBIC);
        g.drawRenderedImage(img, AffineTransform.getScaleInstance(xScale, yScale));
        return scaled;
    }

    public static double calcCValue(double s, double contrast){
        double B=6*contrast;
        return 1.0/(1+Math.exp(-((s-0.5)*B)));
    }
    /** Build a value lookup table for image contrast enhancement.
     * 
     *  Uses an "s-curve" or "logistic" function to distort the value
     *  passed to glTexCoord1d (texture1D based colors) or the
     *  index value used in glColor4f.
     *  
     *  s = 1/(1-exp(-B*(0.5-s)))
     *  
     *  with: B=6*contrast
     */
   public static void makeCVector(double r,double contrast,double cvector[]){
        double s=0;
        double cstep=1.0/(cvector.length-1);
        for(int i=0;i<cvector.length;i++){
            cvector[i]=calcCValue(s,contrast);
            s+=cstep;
        }
    }
   public static void init(){
       GL gl=GLU.getCurrentGL();
       if(glExtensions==null){
           glExtensions=new Hashtable();
           String gstr=gl.glGetString(GL.GL_EXTENSIONS);
           StringTokenizer st = new StringTokenizer(gstr," ");        
           while(st.hasMoreElements()){
               String tok=st.nextToken();
               glExtensions.put(tok, tok);
               if(DebugOutput.isSetFor("glext"))
                   System.out.println(tok);
           }
       }
       shader=NONE;
       String option = System.getProperty("shader");
       if (option == null || option.length()==0 || option.equals("max")){
           if(GLSLProgram.isSupported())
               shader=GLSL;
           else if(ARBProgram.isSupported())
               shader=ARB;
       }
       else if(option.equals("arb") && ARBProgram.isSupported())
           shader=ARB;
       else if(option.equals("glsl") && GLSLProgram.isSupported())
           shader=GLSL;
   }
   public static boolean isExtensionSupported (String ext){
       if(glExtensions.containsKey(ext))
           return true;
       return false;
   }

   public static int getShaderType(){
       return shader;
   }

   public static String getShaderString(int type){
       switch(type){
       default:
       case NONE:
           return "None";
       case ARB:
           return ARBProgram.name();
       case GLSL:
           return GLSLProgram.name();
       }
   }
   public static JGLShaderProgram getShader(int type){
       switch(type){
       default:
       case NONE:
           return null;
       case ARB:
           return new ARBProgram();
       case GLSL:
           return new GLSLProgram();
       }
   }
   public static String getGLExtension (String key){
       return (String)glExtensions.get(key);
   }

   public static void showExtensions(){
       
   }
}

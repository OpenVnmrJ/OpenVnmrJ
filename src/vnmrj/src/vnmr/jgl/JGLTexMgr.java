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
import vnmr.util.Messages;

//import com.sun.opengl.util.BufferUtil;
import com.jogamp.common.nio.Buffers;

import java.io.RandomAccessFile;
import java.nio.*;
import java.nio.channels.FileChannel;

public class JGLTexMgr implements JGLDef 
{
    //  methods and parameters
    protected  double transparency=0;
    protected  double limit=1;
    protected  double threshold=0;
    protected  double intensity=0;
    protected  double bias=0;
    protected  double contrast=0;
    protected  double contours=0.01;
    protected  float colors[]=null;
    protected  FColor bgcol=FColor.WHITE;
    protected  FColor gridcol=FColor.BLACK;
    protected  int palette=0;
    protected  int color_mode=0;
    protected  double ymin=0,ymax=0;
    protected  int slice=0;
    protected  GL2 gl;
    protected int ncolors=0;
    protected JGLView view=null;
    protected double scale=0;
    protected int textureIds[];
    protected FloatBuffer colormap=null;
    protected FloatBuffer functionmap=null;
    protected boolean colormap_invalid=true;
    protected boolean functionmap_invalid=true;
    protected boolean texture_invalid=true;
    protected double alphascale=-1;
    protected float params[]=new float[4];
    protected float params2[]=new float[4];
    protected int shader_type=NONE;
    protected boolean clip_low=true;
    protected boolean clip_high=true;    
    
	public boolean use_shader=false;
    public JGLShaderProgram shader=null;
    
    protected JGLDataMgr data=null;

    public JGLTexMgr(JGLView v,JGLDataMgr d){
        textureIds=new int[4];
        textureIds[0]=textureIds[1]=textureIds[2]=textureIds[3]=0;
        colormap=FloatBuffer.allocate(NUMCOLORS*4);
        //functionmap=BufferUtil.newFloatBuffer(2*CVSIZE);
        functionmap=Buffers.newDirectFloatBuffer(2*CVSIZE);
        view=v;
        data=d;
    }
    public void setUseShader(boolean b){
        if(!b && shader !=null){
            shader.unbind();
            shader.destroy();
            shader=null;
        }
    	use_shader=b;
    }


    public void setShaderType(int s){
        shader_type=s;
    }

    public void setTransparency(double r){
        if(r!=transparency)
            functionmap_invalid=true;
        transparency=r;
    }

    public void setThreshold(double r){
        if(r!=threshold)
            functionmap_invalid=true;
        threshold=r<0.01?0.01:r;
    }

    public void setContours(double r){
        contours=r;
    }

    public void setLimit(double r){
        if(r!=limit)
            functionmap_invalid=true;
        limit=r;
    }

    public  void setIntensity(double r){
        intensity=r;
    }

    public  void setBias(double r){
        bias=r;
    }

    public  void setAlphaScale(double r){
        if(r!=alphascale)
            functionmap_invalid=true;
        alphascale=r;
    }

    public void setDataScale(double mn, double mx){
    	if(ymax!=mx)
    		invalidate();
        ymin=mn;
        ymax=mx;
        scale=1.0/(ymax-ymin);
    }

    public void setBgColor(float t[]){
    	bgcol=new FColor(t);
    }
    public void setGridColor(float t[]){
    	gridcol=new FColor(t);
    }
    public void setColorTable(float t[], int mode){
        int oldmode=palette;
        palette=mode&PALETTE;
        ncolors=t.length/4;
        if(oldmode!=palette || (mode&NEWCTABLE)>0){
            colormap.clear();
            colormap.put(t);
            colormap.flip();
            colormap_invalid=true;
        }
        colors=colormap.array();
        color_mode=mode&COLORMODE;
    }
    public void setContrast(double r){
        if(r!=contrast)
            functionmap_invalid=true;
        contrast=r;
    }

    // extended class override functions
    
    public void invalidate(){}
    public void free(){
        colormap.clear();
        functionmap.clear();
    }
    public void beginShader(int slc, int options){
    }
    public void endShader(){
    }

    public void drawSlice(int slc, int options){
        slice=slc;
    }
    public void init(int options,int flags){
        if((flags&NEWTEXMAP)>0)
            functionmap_invalid = true;
        clip_high=((options & CLIPHIGH)!=0)?true:false;
        clip_low=((options & CLIPLOW)!=0)?true:false;
        gl=GLU.getCurrentGL().getGL2();
        if(textureIds[0]==0)
            gl.glGenTextures(4, textureIds,0);  
    }

    public void finish(){
    }

    // common utilities

    private void makeFunctionMap() {
        if(!functionmap_invalid)
            return;
        double s = 0;
        double B = 6 * contrast;
        double cstep = 1.0 / (CVSIZE - 1);
        functionmap.clear();
        double val=0;
        for(int i = 0; i < CVSIZE; i++) {           
            val = 1.0 / (1 + Math.exp(-((s - 0.5) * B)));
            functionmap.put((float)val); 
            if(val < threshold || val>limit){
                if(val < threshold && clip_low)
                    val = 0;
                if(val>limit && clip_high)
                     val=0;
            }
            if(val<=0)
                val=0;
            else {
                double f = val * transparency;
                val = 1.0 - Math.pow(1.0 - f, alphascale);
            }
            functionmap.put((float)val);
            
            s += cstep;
        }
        functionmap.rewind();
        functionmap_invalid = false;
    }

    public double getColorValue(double s){
        makeFunctionMap();
        int n=(int)(Math.abs(s)*(CVSIZE-1));
        if(n>=CVSIZE)
            n=n%CVSIZE;
        return functionmap.get(n*2);
    }
    public double getAlphaValue(double s){
        makeFunctionMap();
        int n=(int)(Math.abs(s)*(CVSIZE-1));
        //if(n>=CVSIZE)
            n=n%CVSIZE;
        return functionmap.get(n*2+1);
    }
    

    protected void makeColorMapTexture(int options,int flags){
        int wrp=((options & CLAMP)>0)?GL2.GL_CLAMP_TO_EDGE:GL2.GL_MIRRORED_REPEAT;
        int intrp=((options & BLEND)>0)?GL2.GL_LINEAR:GL2.GL_NEAREST;
        gl.glBindTexture(GL2.GL_TEXTURE_1D, textureIds[1]);
         
        gl.glTexParameteri(GL2.GL_TEXTURE_1D, GL2.GL_TEXTURE_WRAP_S, wrp);              
        gl.glTexParameteri(GL2.GL_TEXTURE_1D, GL2.GL_TEXTURE_MAG_FILTER, intrp);
        gl.glTexParameteri(GL2.GL_TEXTURE_1D, GL2.GL_TEXTURE_MIN_FILTER, intrp);
        gl.glTexImage1D(GL2.GL_TEXTURE_1D, 0, GL2.GL_RGBA, ncolors, 0, GL2.GL_RGBA, GL2.GL_FLOAT, colormap);
        colormap_invalid=false;
    }
    
    protected void makeFunctionMapTexture(int options,int flags){
        makeFunctionMap();
        int wrp=((options & CLAMP)>0)?GL2.GL_CLAMP_TO_EDGE:GL2.GL_MIRRORED_REPEAT;
        int intrp=GL2.GL_LINEAR;
        gl.glBindTexture(GL2.GL_TEXTURE_1D, textureIds[2]);
         
        gl.glTexParameteri(GL2.GL_TEXTURE_1D, GL2.GL_TEXTURE_WRAP_S, wrp);              
        gl.glTexParameteri(GL2.GL_TEXTURE_1D, GL2.GL_TEXTURE_MAG_FILTER, intrp);
        gl.glTexParameteri(GL2.GL_TEXTURE_1D, GL2.GL_TEXTURE_MIN_FILTER, intrp);
        gl.glTexImage1D(GL2.GL_TEXTURE_1D, 0, GL2.GL_LUMINANCE_ALPHA16F_ARB, CVSIZE, 0, GL2.GL_LUMINANCE_ALPHA, GL2.GL_FLOAT, functionmap);
    }
    
    protected int texColor(double v,int options){
        double s=intensity*v;
        int h,indx;
        long value=0;
        int r=0,b=0,g=0,a=255;
        double cval,aval;

        if((options & XPARANCY) > 0){
            aval=getAlphaValue(s);
            a=(int)(255*aval)&0xff;
        }  
        cval=getColorValue(s);
        h = (int)((ncolors - 1) * cval);
        indx = (h % ncolors) * 4;
        r=(int)(255.0*colors[indx])&0xff;
        g=(int)(255.0*colors[indx+1])&0xff;
        b=(int)(255.0*colors[indx+2])&0xff;
        value=(a<<24)|(b<<16)|(g<<8)|(r);
        return (int)value;
    }
}

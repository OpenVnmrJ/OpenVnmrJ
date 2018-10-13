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

import java.io.RandomAccessFile;
import java.nio.*;
import java.nio.channels.FileChannel;

// JGLTexture2D
// 
// shader inputs
// ------------------------------------
// voltex       volume data      2D-texture
// colormap     color palette    1D-texture
// functionmap  contrast LUT     1D-texture
// params[0].y  intensity
// params[0].x  color index

public class JGLTex2DMgr extends JGLTexMgr implements JGLDef
{
    static final int MAXSLICES=1024;
    private int sliceIds[]=null; 
    private ByteBuffer tex2DByteBuffer[]=null;
    private IntBuffer tex2DIntBuffer[]=null;
    Point3D min=null;
    Point3D max=null;
    double delta=0;
    
    private static float stbl[][]=null;
    
    int num2Dtexs=0;
    int size2Dtexs=0;

    public JGLTex2DMgr(JGLView v,JGLDataMgr d){
        super(v,d);
        if(stbl==null){
            stbl=new float[4][2];
            stbl[0][0]=0; stbl[0][1]=0;
            stbl[1][0]=0; stbl[1][1]=1;
            stbl[2][0]=1; stbl[2][1]=1;
            stbl[3][0]=1; stbl[3][1]=0;
        }
    }

    public void finish(){
        gl.glDisable(GL2.GL_TEXTURE_1D);
        gl.glDisable(GL2.GL_TEXTURE_2D);
        if(shader !=null)
            shader.unbind();
    }

    public void free(){
        super.free();
        invalidate();
        shader=null;
    }

    public void init(int options,int flags){
        super.init(options,flags);
        int n=num2Dtexs;
        
        min=view.sliceMinPoint();
        max=view.sliceMaxPoint();
        
        num2Dtexs=view.maxSlice()+1;
        delta=1.0/num2Dtexs;
        
        switch(options & SLICEPLANE){
        case X:
            size2Dtexs=data.slices*data.traces;
        case Y:
            size2Dtexs=data.slices*data.np;
        default:
        case Z:
            size2Dtexs=data.traces*data.np;
        }

        if(n!=num2Dtexs)
            invalidate();

        if(sliceIds==null){
            sliceIds=new int[MAXSLICES]; 
            gl.glGenTextures(MAXSLICES, sliceIds,0);  
        }
        if(color_mode==HISTOGRAM){
            if(shader !=null){
                shader.destroy();
                invalidate();
            }
            shader=null;
        }
        else if(shader == null || (flags & NEWLAYOUT) > 0 || (flags & NEWSHADER) > 0) {
            //if(shader != null)
            //    shader.destroy();
            shader = GLUtil.getShader(shader_type);
            if(shader != null){
            	switch(options & SHADERTYPE){
            	default:
            	case VOLSHADER:
                    shader.loadShader(JGLShaderProgram.VOL2D);
            		break;
            	case MIPSHADER:
                    shader.loadShader(JGLShaderProgram.MIP2D);
            		break;
            	}
            }
            colormap_invalid=true;
            functionmap_invalid = true;
        }
        gl.glEnable(GL2.GL_TEXTURE_2D);
        gl.glEnable(GL2.GL_TEXTURE_1D);
        gl.glDisable(GL2.GL_TEXTURE_3D);
        gl.glColor4f(1, 1, 1, 1);

        if(shader !=null){
            shader.bind();
            if((options & XPARANCY)>0)
                gl.glEnable(GL2.GL_BLEND);
            else
                gl.glDisable(GL2.GL_BLEND);
            params[1] = (float)intensity;
            
            if(functionmap_invalid || (flags & (NEWTEXGEOM|NEWTEXMODE|NEWLAYOUT)) > 0){
                functionmap_invalid=true;
                makeFunctionMapTexture(options,flags);
                shader.setTexture("functionmap", 2);
            }
            if(colormap_invalid || (flags & (NEWTEXGEOM|NEWTEXMODE|NEWLAYOUT)) > 0){
                makeColorMapTexture(options,flags);
                shader.setTexture("colormap", 1);
            }
            if(tex2DByteBuffer==null || (flags & (NEWTEXGEOM|NEWTEXMODE|NEWLAYOUT))>0){
                shader.setTexture("voltex", 0);
                invalidate();
                tex2DByteBuffer =new ByteBuffer[num2Dtexs];
            }
            shader.unbind();
            gl.glActiveTexture(GL2.GL_TEXTURE2);
            gl.glBindTexture(GL2.GL_TEXTURE_1D, textureIds[2]);
            gl.glActiveTexture(GL2.GL_TEXTURE1);
            gl.glBindTexture(GL2.GL_TEXTURE_1D, textureIds[1]);
        }
        else {
            if(flags> 0)
                invalidate();
            if(tex2DIntBuffer==null)
                tex2DIntBuffer =new IntBuffer[num2Dtexs];
            gl.glEnable(GL2.GL_BLEND);
        }
        gl.glActiveTexture(GL2.GL_TEXTURE0);
        if(data.mapData !=null)
        	data.mapData.position(0);        	
    }

    public void invalidate(){
        int i;
        if(tex2DIntBuffer !=null){
            for(i=0;i<tex2DIntBuffer.length;i++)
                tex2DIntBuffer[i]=null;
        }
        if(tex2DByteBuffer !=null){
            for(i=0;i<tex2DByteBuffer.length;i++)
                tex2DByteBuffer[i]=null;
        }
        tex2DIntBuffer=null;
        tex2DByteBuffer=null;
    }

    double vertexValue(int k, int j, int i, int options){
        int adrs=0;
        switch(options & SLICEPLANE){
        case X: adrs=data.np*j+data.np*data.traces*i+k;break;
        case Y: adrs=data.np*k+data.np*data.traces*j+i;break;
        default:
        case Z: adrs=data.np*j+data.np*data.traces*k+i;break;
        }
        return data.vertexValue(adrs);
     }
    
    private void make2DTexture(int k, int options){
        int nj=0,ni=0;
        int i,j,index=0;
        float v;

        gl.glTexEnvi(GL2.GL_TEXTURE_ENV, GL2.GL_TEXTURE_ENV_MODE, GL2.GL_MODULATE);
        
        switch(options & SLICEPLANE){
        case X: nj=data.traces; ni=data.slices; break;
        case Y: nj=data.slices; ni=data.np; break;
        default:
        case Z: nj=data.traces; ni=data.np; break;
        }
        int intrp=((options & BLEND)>0)?GL2.GL_LINEAR:GL2.GL_NEAREST;
        int wrp=GL2.GL_CLAMP_TO_EDGE;
        gl.glTexParameteri(GL2.GL_TEXTURE_2D, GL2.GL_TEXTURE_MAG_FILTER, intrp);
        gl.glTexParameteri(GL2.GL_TEXTURE_2D, GL2.GL_TEXTURE_MIN_FILTER, intrp);
        gl.glTexParameteri(GL2.GL_TEXTURE_2D, GL2.GL_TEXTURE_WRAP_S, wrp);
        gl.glTexParameteri(GL2.GL_TEXTURE_2D, GL2.GL_TEXTURE_WRAP_T, wrp);
       
        if(shader !=null){
            int maxvalue=255;
            tex2DByteBuffer[k]=ByteBuffer.allocate(size2Dtexs);
            for(j=0;j<nj;j++){
                for(i=0;i<ni;i++){
                     v=(float)vertexValue(k,j,i,options);
                     v=(float)(v*maxvalue*scale);
                     v=v>maxvalue?maxvalue:v;
                     tex2DByteBuffer[k].put(index++,(byte)v);
                }                
            }
            gl.glTexImage2D(GL2.GL_TEXTURE_2D, 0, GL2.GL_ALPHA8, ni, nj, 
                    0, GL2.GL_ALPHA, GL2.GL_UNSIGNED_BYTE, tex2DByteBuffer[k]);
        }
        else{
            int c;
            tex2DIntBuffer[k]=IntBuffer.allocate(size2Dtexs);
            for(j=0;j<nj;j++){
                for(i=0;i<ni;i++){
                    //data.slice=k;
                    v=(float)vertexValue(k,j,i,options);
                    c=texColor(v,options);
                    tex2DIntBuffer[k].put(index++, c);
                }
            }
            gl.glTexImage2D(GL2.GL_TEXTURE_2D, 0, GL2.GL_RGBA, ni,nj, 
                    0, GL2.GL_RGBA, GL2.GL_UNSIGNED_BYTE, tex2DIntBuffer[k]);           
        }       
    }

    public void drawSlice(int slc, int options) {
        float color = -1;
        Point3D plane;
        Point3D[] spts;
        gl.glActiveTexture(GL2.GL_TEXTURE0);
        gl.glBindTexture(GL2.GL_TEXTURE_2D, sliceIds[slc]);

        if(shader != null) {
            if(tex2DByteBuffer[slc] == null)
                make2DTexture(slc, options);
            shader.enable();
            shader.bind();
            int n = view.maxSlice();
            switch(options & COLTYPE) {
            case COLONE:
                color = 0.5f;
                break;
            case COLIDX:
                color = ((float)(slc)) / n;
                if((options & CLAMP) == 0)
                    color *= intensity;
                break;
            }
            params[0] = (float)color;
            shader.setFloatVector("params", params);
        } else if(tex2DIntBuffer[slc] == null)
            make2DTexture(slc, options);

        double s = delta * slc;
        
        plane = min.plane(max, s);
        spts = view.boundsPts(plane);
        gl.glPushMatrix();
        // X is inverted (top.bottom). try to figure out why later
        if((options & SLICEPLANE)==X){
        	Point3D pc=view.volume.center();
        	gl.glTranslated(pc.x, pc.y, pc.z);
        	gl.glRotated(180, 1, 0, 0);
        	gl.glTranslated(-pc.x, -pc.y, -pc.z);
        }
       
        gl.glBegin(GL2.GL_TRIANGLE_FAN);
        for(int i = 0; i < spts.length; i++) {
            gl.glTexCoord2d(stbl[i][0], stbl[i][1]);
            gl.glVertex3d(spts[i].x, spts[i].y, spts[i].z);
        }
        gl.glEnd();
        gl.glPopMatrix();

        if(shader != null) {
            shader.unbind();
            shader.disable();
        }
    }
}

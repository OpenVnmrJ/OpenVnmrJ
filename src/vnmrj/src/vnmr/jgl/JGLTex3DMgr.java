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

import vnmr.util.DebugOutput;
import vnmr.util.Messages;
import java.nio.*;

public class JGLTex3DMgr extends JGLTexMgr implements JGLDef
{
    private ByteBuffer tex3DByteBuffer=null;
    private IntBuffer tex3DIntBuffer=null;
    
    Point3D min=null;
    Point3D max=null;
    double delta=0;
    
    public JGLTex3DMgr(JGLView v,JGLDataMgr d){
        super(v,d);
    }
    public  void setDataScale(float mn, float mx){
        double old_scale=scale;
        super.setDataScale(mn, mx);
        if(scale!=old_scale){
        	invalidate();
        }
    }
    
     public void init(int options, int flags) {
        super.init(options, flags);
        if(color_mode==HISTOGRAM){
            if(shader !=null){
                shader.destroy();
                invalidate();
            }
            shader=null;
        }
        else if(shader == null || (flags & NEWLAYOUT) > 0 || (flags & NEWSHADER) > 0) {
            if(shader != null){
               // shader.destroy();
            }
            invalidate();
            shader = GLUtil.getShader(shader_type);
            if(shader != null){
            	switch(options & SHADERTYPE){
            	default:
            	case VOLSHADER:
                    shader.loadShader(JGLShaderProgram.VOL3D);
            		break;
            	case MIPSHADER:
            	    shader.loadShader(JGLShaderProgram.MIP3D);
            		break;
            	}
            }
            colormap_invalid=true;
            functionmap_invalid = true;
        }
        
        min = view.eyeMinPoint();
        max = view.eyeMaxPoint();
        delta = 1.0 / (view.maxSlice() + 1);
        gl.glEnable(GL2.GL_TEXTURE_3D);
        gl.glEnable(GL2.GL_TEXTURE_1D);
        gl.glDisable(GL2.GL_TEXTURE_2D);
        
        gl.glColor4f(1, 1, 1, 1);

        if(shader != null) {
            if((options & XPARANCY) > 0)
                gl.glEnable(GL2.GL_BLEND);
            else
                gl.glDisable(GL2.GL_BLEND);
            shader.bind();
            params[1] = (float)intensity;
            if(functionmap_invalid
                    || (flags & (NEWTEXGEOM | NEWTEXMODE | NEWLAYOUT)) > 0) {
                functionmap_invalid = true;
                gl.glActiveTexture(GL2.GL_TEXTURE2);
                makeFunctionMapTexture(options, flags);
                shader.setTexture("functionmap", 2);
            }

            if(colormap_invalid
                    || (flags & (NEWTEXGEOM | NEWTEXMODE | NEWLAYOUT)) > 0) {
                gl.glActiveTexture(GL2.GL_TEXTURE1);
                makeColorMapTexture(options, flags);
                shader.setTexture("colormap", 1);
            }
            if(texture_invalid
                    || (flags & (NEWTEXGEOM | NEWTEXMODE | NEWLAYOUT)) > 0) {
                make3DTexture(options, flags);
                shader.setTexture("voltex", 0);
            }
            shader.unbind();
            gl.glActiveTexture(GL2.GL_TEXTURE2);
            gl.glBindTexture(GL2.GL_TEXTURE_1D, textureIds[2]);
            gl.glActiveTexture(GL2.GL_TEXTURE1);
            gl.glBindTexture(GL2.GL_TEXTURE_1D, textureIds[1]);
        } else if(tex3DIntBuffer == null || (flags > 0)) {
            gl.glEnable(GL2.GL_BLEND);
            make3DTexture(options, flags);
        }
        data.resetDataPosition();
        gl.glActiveTexture(GL2.GL_TEXTURE0);
        gl.glBindTexture(GL2.GL_TEXTURE_3D, textureIds[0]);
    }

    public void finish(){
        gl.glDisable(GL2.GL_TEXTURE_1D);
        gl.glDisable(GL2.GL_TEXTURE_3D);
        if(shader != null){
            shader.unbind();
            shader.disable();
        }
        gl.glBindTexture(GL2.GL_TEXTURE_3D, 0);
    }

    public void free(){
        super.free();
        invalidate();
        shader=null;
    }

    public void invalidate(){
       if(tex3DByteBuffer!=null)
            tex3DByteBuffer.clear();
       if(tex3DIntBuffer!=null)
    	   tex3DIntBuffer.clear();
       tex3DByteBuffer=null;
       tex3DIntBuffer=null;
       texture_invalid=true;
    }

    private void make3DTexture(int options, int flags) {
    	long start=System.nanoTime();
        int intrp = ((options & BLEND) > 0) ? GL2.GL_LINEAR : GL2.GL_NEAREST;
        int wrp = GL2.GL_CLAMP_TO_EDGE;
        int i, j, index = 0, k;
        
        gl.glTexEnvi(GL2.GL_TEXTURE_ENV, GL2.GL_TEXTURE_ENV_MODE, GL2.GL_MODULATE);
        gl.glBindTexture(GL2.GL_TEXTURE_3D, textureIds[0]);

        gl.glTexParameteri(GL2.GL_TEXTURE_3D, GL2.GL_TEXTURE_WRAP_S, wrp);
        gl.glTexParameteri(GL2.GL_TEXTURE_3D, GL2.GL_TEXTURE_WRAP_T, wrp);
        gl.glTexParameteri(GL2.GL_TEXTURE_3D, GL2.GL_TEXTURE_WRAP_R, wrp);
        gl.glTexParameteri(GL2.GL_TEXTURE_3D, GL2.GL_TEXTURE_MAG_FILTER, intrp);
        gl.glTexParameteri(GL2.GL_TEXTURE_3D, GL2.GL_TEXTURE_MIN_FILTER, intrp);

        float v;
        if(data.mapData !=null)
            data.mapData.position(0);
        if(shader != null) {
            tex3DByteBuffer = ByteBuffer.allocate(data.data_size);
 	            for(k = 0; k < data.slices; k++) {
	                for(j = 0; j < data.traces; j++) {
	                    for(i = 0; i < data.np; i++) {
	                    	v=(data.vertexData==null)?data.mapData.getFloat():data.vertexData[index];
	                        v = v * 255 *(float)scale;
	                        v = v > 255 ? 255 : v;
	                        tex3DByteBuffer.put(index++, (byte)v);
	                    }
	                }
	            }
                gl.glTexImage3D(GL2.GL_TEXTURE_3D, 0, GL2.GL_ALPHA8, data.np, data.traces, data.slices, 0,
                    GL2.GL_ALPHA, GL2.GL_UNSIGNED_BYTE, tex3DByteBuffer);        	
        } else {
            tex3DIntBuffer = IntBuffer.allocate(data.data_size);
            for(k = 0; k < data.slices; k++) {
                for(j = 0; j < data.traces; j++) {
                    for(i = 0; i < data.np; i++) {
                        switch(options & SLICEPLANE) {
                        case X:
                            slice = i;
                            break;
                        case Y:
                            slice = j;
                            break;
                        default:
                        case Z:
                            slice = k;
                            break;
                        }
                        //v=(vertexData==null)?mapData.getFloat():vertexData[index];
                        v=data.dataValue(index);
                        int c = texColor(v*scale, options);
                        tex3DIntBuffer.put(index++, c);
                    }
                }
            }
            gl.glTexImage3D(GL2.GL_TEXTURE_3D, 0, GL2.GL_RGBA, data.np, data.traces,
            		data.slices, 0, GL2.GL_RGBA, GL2.GL_UNSIGNED_BYTE, tex3DIntBuffer);
        }
        texture_invalid=false;
        if(DebugOutput.isSetFor("gltime")){
        	long time=System.nanoTime()-start;
        	Messages.postDebug("JGLTex3DMgr.make3DTexture: "+time/1000000.0+" ms");
        }
    }

    public void drawSlice(int slc, int options) {
        float color = -1;
        Point3D plane;
        Point3D[] spts;
        if(shader != null) {
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
        }
        double s = delta * slc;
        plane = min.plane(max, s);
        spts = view.slicePts(plane);
        
        Point3D ps=view.volume.scale.invert();
        gl.glBegin(GL2.GL_TRIANGLE_FAN);
        for(int i = 0; i < spts.length; i++) {
        	Point3D pt=spts[i].mul(ps);
            gl.glTexCoord3d(pt.x, pt.y + 0.5, pt.z);
            gl.glVertex3d(spts[i].x, spts[i].y, spts[i].z);
        }
        gl.glEnd();
        if(shader != null) {
            shader.unbind();
            shader.disable();
        }
    }
}

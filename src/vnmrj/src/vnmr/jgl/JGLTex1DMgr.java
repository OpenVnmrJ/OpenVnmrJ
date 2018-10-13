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

import java.nio.*;

public class JGLTex1DMgr extends JGLTexMgr implements JGLDef
{
    public JGLTex1DMgr(JGLView v,JGLDataMgr d){
        super(v,d);
    }

	public void init(int options, int flags) {
		super.init(options, flags);
        clip_high=false;
        clip_low=false;

		gl.glDisable(GL2.GL_TEXTURE_2D);
		gl.glDisable(GL2.GL_TEXTURE_3D);
		if (functionmap_invalid
				|| (flags & (NEWTEXGEOM | NEWTEXMODE | NEWLAYOUT)) > 0) {
			functionmap_invalid = true;
			makeFunctionMapTexture(options, flags);
		}
		if (colormap_invalid || (flags & (NEWTEXGEOM | NEWTEXMODE | NEWLAYOUT)) > 0) {
			makeColorMapTexture(options, flags);
		}

		if (use_shader) {
			if (shader == null || (flags & NEWLAYOUT) > 0
					|| (flags & NEWSHADER) > 0) {
				//if (shader != null)
				//	shader.destroy();
				shader = GLUtil.getShader(shader_type);
				if (shader != null){
					if((flags & USELIGHTING)>0)
						shader.loadShader(JGLShaderProgram.LGT1D);
					else
						shader.loadShader(JGLShaderProgram.HT1D);
				}
			}
	        gl.glActiveTexture(GL2.GL_TEXTURE1);
	        gl.glBindTexture(GL2.GL_TEXTURE_1D, textureIds[2]);
	        gl.glActiveTexture(GL2.GL_TEXTURE0);
	        gl.glBindTexture(GL2.GL_TEXTURE_1D, textureIds[1]);
		} else {
			gl.glActiveTexture(GL2.GL_TEXTURE0);
			gl.glBindTexture(GL2.GL_TEXTURE_1D, textureIds[1]);
			gl.glTexEnvi(GL2.GL_TEXTURE_ENV, GL2.GL_TEXTURE_ENV_MODE,GL2.GL_DECAL);
		}
	}

    public FColor getColor(double s){
        int h = (int)((ncolors - 1) * getColorValue(s));
        int indx = (h % ncolors) * 4;
        float color[] = new float[4];
        color[0]=colors[indx];
        color[1]=colors[indx + 1];
        color[2]=colors[indx + 2];
        color[3]=(float)getAlphaValue(s);
        return new FColor(color);
     }

    public void beginShader(int slc, int options){
    	if(shader==null || data.projection !=TWOD)
    		return;

        shader.enable();
        shader.bind();
        
    	float color=-1;
    	int n = view.maxSlice();
    	int cmode=0;
        switch(options & COLTYPE) {
        case COLONE:
            cmode=1;
            break;
        case COLIDX:
            color = ((float)(slc)) / n;
            if((options & CLAMP) == 0)
                color *= intensity;
            break;
        }
        gl.glPushAttrib(GL2.GL_ENABLE_BIT);
        boolean xparancy=(options & XPARANCY) > 0;
        boolean reversed=(options & REVERSED) > 0;
        boolean draw_contours=(options & CONTOURS)>0;
        boolean draw_grid=(options & GRID)>0;
       	
        params[1] = xparancy ?1.0f:-1.0f;
        params[0] = (float)color;

        if(draw_contours){
        	float mingap=0.01f;
            float t=(float)(contours);
            t=t<mingap?mingap:t;
        	params[2]=(float)(20*t*Math.sqrt(view.dscale));
        }
        else
        	params[2]=0.0f;
        
        if(reversed||!draw_contours)
        	params[2]=-params[2];
        
        if(draw_grid)
        	params[3]=20.0f; // grid spacing
        else
        	params[3]=0.0f;
        	 
        gl.glDisable(GL2.GL_LIGHTING);
        gl.glNormal3d(0,0,0);       
        
        shader.setFloatVector("params", params);
        shader.setFloatVector("bcol", bgcol.rgba);
        shader.setFloatVector("gcol", gridcol.rgba);
        shader.setIntValue("cmode", cmode);
        if((options & LIGHTING) >0){
	        float lpars[]={0.15f,1.0f,0.9f,5.0f}; // ambient,diffuse,specular,shine
	        float lpos[]={10.0f,-10.0f,0.0f,1.0f}; // light position
	        shader.setFloatVector("lpars", lpars);
	        shader.setFloatVector("lpos", lpos);
        }
        shader.setTexture("functionmap", 1);
        shader.setTexture("colormap", 0);
    	gl.glEnable(GL2.GL_TEXTURE_1D);
        gl.glEnable(GL2.GL_BLEND);
    }
    public void endShader(){
    	if(shader==null)
    		return;
        shader.unbind();
        shader.disable();
        gl.glPopAttrib();
    }

    public void finish(){
        if(shader != null){
        	gl.glDisable(GL2.GL_TEXTURE_1D);
        	gl.glBindTexture(GL2.GL_TEXTURE_1D, 0);
            shader.unbind();
            shader.disable();
        }

    }
}

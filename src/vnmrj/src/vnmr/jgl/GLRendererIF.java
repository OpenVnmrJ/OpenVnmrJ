/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

import javax.media.opengl.GL;

public interface  GLRendererIF {
    public void destroy();
    public void init(int flags);
    public void render(int flags);
    public void releaseDataPtr(float[] data);
    public void setDataPars(int dl, int dl2, int dl3, int type);
    public void setDataPtr(float[] data);
    public void setDataMap(String path);
    public void setDataScale(double mn, double mx);
    public void setOptions(int i, int s);
    public void setPhase(double r, double l);
    public void setTransparency(double r);
    public void setThreshold(double r);
    public void setContours(double r);
    public void setLimit(double r);
    public void setIntensity(double r);
    public void setBias(double r);
    public void setAlphaScale(double r);
    public void setContrast(double r);
    public void resize(int w, int h);
    public void setColorArray(int id, float[] data);
    public void setScale(double a,double x, double y, double z);
    public void setSpan(double a,double x, double y);
    public void setOffset(double x, double y, double z);
    public void setRotation3D(double x, double y, double z);
    public void setRotation2D(double x, double y);
    public void setObjectRotation(double x, double y, double z);
    public void setSliceVector(double x, double y, double z, double w);
    public void setSlant(double x,double y);
    public void setTrace(int i,int max, int num);
    public void setSlice(int i,int max, int num);
    public void setStep(int i);
    public void render2DPoint(int pt, int trc, int dtype);
};


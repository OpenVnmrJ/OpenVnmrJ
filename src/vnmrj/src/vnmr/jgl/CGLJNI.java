/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

/**
 * OpenGL using Java swing and Java JNI
 */

public class CGLJNI implements GLRendererIF{
    public long native_obj=0;  // storage for *C3DRenderer 
    static public boolean libary_loaded=false;
    private native long CCcreate();
    private native void CCdestroy(long cobj);
    private native void CCinit(int f);
    private native void CCrender(int f);
    private native void CCsetOptions(int indx, int i);
    private native void CCresize(int w,int h);
    private native void CCsetPhase(double r, double l);
    private native void CCsetDataScale(double mn, double mx);
    private native void CCsetDataPars(int n, int t, int s, int dtype);
    private native void CCsetDataMap(String mapfile);
    private native void CCsetDataPtr(float[] data);
    private native void CCreleaseDataPtr(float d[]);
    private native void CCsetColorArray(int id, float[] data, int n);
    private native void CCsetScale(double a, double x, double y, double z);
    private native void CCsetSpan(double x, double y, double z);
    private native void CCsetOffset(double x, double y, double z);
    private native void CCsetRotation3D(double x, double y, double z);
    private native void CCsetRotation2D(double x, double y);
    private native void CCsetObjectRotation(double x, double y, double z);
    private native void CCsetTrace(int i,int max, int num);
    private native void CCsetSlice(int i, int max, int num);
    private native void CCsetStep(int i);
    private native void CCsetSlant(double x,double y);
    private native void CCsetIntensity(double x);
    private native void CCsetBias(double x);
    private native void CCsetContrast(double x);
    private native void CCsetThreshold(double x);
    private native void CCsetContours(double x);
    private native void CCsetLimit(double x);
    private native void CCsetTransparency(double x);
    private native void CCsetAlphaScale(double x);
    private native void CCsetSliceVector(double x, double y, double z, double w);
    private native void CCrender2DPoint(int pt, int trc, int dtype);
    
    public CGLJNI(){
        if(!libary_loaded){
            try {
                System.loadLibrary("cgl");
                 libary_loaded=true;
            }
            catch (Exception e){
                return;
            }
        }
        native_obj=CCcreate();       
    }

    public boolean libraryLoaded(){
        return libary_loaded;
    }

    public void destroy() {
        CCdestroy(native_obj);
    }
    public void init(int f) {
        CCinit(f);
    }   
    public void setOptions(int indx, int i) {
        CCsetOptions(indx, i);
    }   
    public void render(int f) {
        CCrender(f);
    }   
    public void resize(int w, int h) {
        CCresize(w,h);
    }
    public void setPhase(double r, double l) {
        CCsetPhase(r,l);
    }
    public void setDataMap(String path){
        CCsetDataMap(path);
    }
    public void setDataPtr(float[] data){
        CCsetDataPtr(data);
    }
    public void setDataPars(int n, int t, int s, int dtype){
        CCsetDataPars(n,t,s,dtype);
    }
    public void releaseDataPtr(float[] data){
        CCreleaseDataPtr(data);
    }
    public void setColorArray(int id, float[] data){
        CCsetColorArray(id, data, data.length/4);
    }
    public void setDataScale(double mn, double mx){
        CCsetDataScale(mn,mx);
    }
    public void setScale(double a,double x, double y, double z){
        CCsetScale(a,x,y,z);
    }
    public void setSpan(double x, double y, double z){
        CCsetSpan(x,y,z);
    }
    public void setOffset(double x, double y, double z){
        CCsetOffset(x,y,z);      
    }
    public void setRotation3D(double x, double y, double z){
        CCsetRotation3D(x,y,z);              
    }
    public void setRotation2D(double x, double y){
        CCsetRotation2D(x,y);              
    }
    public void setObjectRotation(double x, double y, double z){
        CCsetObjectRotation(x,y,z);              
    }
    public void setTrace(int i,int max, int num){
        CCsetTrace(i,max,num);
    }
    public void setSlice(int i, int max, int num){
        CCsetSlice(i,max,num);        
    }
    public void setStep(int i){
        CCsetStep(i);        
    }
    public void setSlant(double x,double y){
        CCsetSlant(x,y);
    }
    public void setIntensity(double x){
        CCsetIntensity(x);
    }
    public void setBias(double x){
        CCsetBias(x);
    }
    public void setContrast(double x){
        CCsetContrast(x);
    }
    public void setThreshold(double x){
        CCsetThreshold(x);
    }
    public void setContours(double x){
        CCsetContours(x);
    }
    public void setLimit(double x){
        CCsetLimit(x);
    }
    public void setTransparency(double x){
        CCsetTransparency(x);
    }
    public void setAlphaScale(double x){
        CCsetAlphaScale(x);
    }
    public void setSliceVector(double x, double y, double z, double w){
        CCsetSliceVector(x,y,z,w);
    }
    public void render2DPoint(int pt, int trc, int dtype){
        CCrender2DPoint(pt,trc,dtype);
    }
}


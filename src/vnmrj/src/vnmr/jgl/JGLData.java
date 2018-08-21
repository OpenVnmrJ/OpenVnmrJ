/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

public class JGLData implements JGLDef {
    public int np=0;
    public int trace=0;
    public int traces=0;
    public int slice=0;
    public int slices=1;
    public int data_size=0;
    public int dim=0;
    public int type=FID;
    public int data_type=0;
    public int step=1;
	public int ws = 1;   // word size
   
    public double rp=0;
    public double lp=0;
    public double ymin=0,ymax=0;
    public double dmin=0,dmax=0;
    public double scale=1.0;
    public float mn=0,mx=1,mean=0.0f,stdev=0.0f;
    public float sx=1,sy=1,sz=1;
    
    public boolean complex=false;
    public boolean absval = false;

    public boolean image=false;
    public boolean autoscale=true;
    public boolean havedata=true;
    public boolean newdata=true;
    public boolean mmapped=false;
    public String volmapfile=null;

    static public final int NEWDTYPE=0x00000010;
    static public final int NEWREV=0x00000020;
    static public final int DFLAGS=NEWDTYPE|NEWREV;
    static public final int UPDATESHOW=1;
    static public final int UPDATEDTYPE=2;
    static public final int XFER=8;
    static public final int XFERCLR=4;
    static public final int XFEREND=3;
    static public final int XFERBGN=2;
    static public final int XFERERROR=1;
    static public final int XFERREAD=0;
    static public final int XFERMODE=0x07;

    public float[] vertexData = null;
    JGLData(){}
    JGLData(JGLData d){
        np=d.np;
        trace=d.trace;
        traces=d.traces;
        slice=d.slice;
        slices=d.slices;
        data_size=d.data_size;
        dim=d.dim;
        type=d.type;
        step=d.step;
        rp=d.rp;
        lp=d.lp;
        ymin=dmax=d.ymin;
        ymax=dmax=d.ymax;
        mn=d.mn;mx=d.mx;mean=d.mean;stdev=d.stdev;
        sx=d.sx;sy=d.sy;sz=d.sz;       
        complex=d.complex;
        image=d.image;
        autoscale=d.autoscale;
        havedata=d.havedata;
        newdata=d.newdata;
        mmapped=d.mmapped;
        vertexData=d.vertexData;
        data_type=dim|type;
        if(d.volmapfile!=null)
            volmapfile=new String(d.volmapfile);
    }
}

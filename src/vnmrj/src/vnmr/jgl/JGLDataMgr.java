/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.jgl;

import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;

import vnmr.util.Messages;

public class JGLDataMgr extends JGLData {
	protected ByteBuffer mapData=null;  // only needed in java data model
	protected String mapfile=null;
    public boolean newDataGeometry=true;
    double pa = 1, pb = 0,cosd = 1, sind = 0;
    boolean allpoints=false;
    
    double lastval=0;
    double sumphs=0;
        
    boolean real=true;
    boolean imag=false;
    
    int projection=0;
    int sliceplane=Z;

	public JGLDataMgr() {
	}

	public JGLDataMgr(JGLData d) {
		super(d);
	}
    public void setDataMap(String path){
        mapfile=path;
    }
    public void setDataPtr(float[] data){
        vertexData=data;
    }
    
    public void releaseDataPtr(float[] data){}
    public void resetDataPosition(){
        if(mapData !=null)
        	mapData.position(0);    	
    }
 
    public void setAbsValue(int newshow){
	    if(((newshow & ABS) == ABS) || !complex)
	    	absval = true;
	    else
	    	absval = false;
    }

    public void setDataPars(int n, int t, int s, int dtype){
        if((np!=n || traces !=t || slices !=s|| data_type !=dtype))
            newDataGeometry=true;
        mapData=null;
        np=n;
        traces=t;
        slices=s;
        data_type=dtype;
        dim=data_type&DIM;
        complex=((dtype&COMPLEX)>0)?true:false;
        ws = complex ? 2 : 1;
        data_size=ws*np*traces*slices;
    }

    /**
     * create a physical or memory-mapped data source
     */
    public void setDataSource(){
        if(vertexData==null && mapData==null){
        	try {
            	RandomAccessFile file=new RandomAccessFile(mapfile,"r");
            	FileChannel rwCh = file.getChannel();
                long fileSize = rwCh.size();
                mapData = rwCh.map(FileChannel.MapMode.READ_ONLY,0, fileSize);
                rwCh.close();
                mapData.order(ByteOrder.nativeOrder());
                mapData.position(0);
        	}
            catch (Exception e) {
                System.out.println(e);
                Messages.writeStackTrace(e);
            }
    	}
    }
    /**
     * return a value from a fixed or memory mapped data array
     * @param adrs
     * @return
     */
    public double vertexValue(int adrs){
    	return ((vertexData==null)?mapData.getFloat(adrs*4):vertexData[adrs]);
    }
    /**
     * return a value from a fixed or memory mapped data array
     * @param adrs
     * @return
     */
    public float dataValue(int index){
    	return (float)((vertexData==null)?mapData.getFloat():vertexData[index]);
    }
    
    /**
     * Calculate normalized coordinates for a set of input indexes
     * @param k  slice index
     * @param j  trace index
     * @param i  point index
     * @return a point containing normalized coordinates
     */
    public Point3D getPoint(int k, int j, int i){
        int nz=0,ny=0,nx=0,ni=0,nj=0;
        double x=0,y=0,z=0;
    	// swizzle input indexes to extract the appropriate sliceplane
        switch(sliceplane){
	        case X: nz=i; ny=j; nx=k; ni=slices; nj=traces;break;
	        case Y: nz=j; ny=k; nx=i; ni=np; nj=slices; break;
	        default: // 1D 2D
	        case Z: nz=k; ny=j; nx=i; ni=np; nj=traces; break;
        }
        switch(projection){
        case ONETRACE:
            x=((double)i)/ni;
            break;
        case OBLIQUE:
            x=((double)i)/ni;
            z=((double)j)/nj;
            break;
        case TWOD:
            x=((double)i)/ni;
            z=((double)j)/nj;
            break;
        case SLICES: 
        case THREED:
            x=((double)nx)/np;
            z=((double)nz)/slices;
            y=((double)ny)/traces-0.5;
        }
        return new Point3D(x,y,z);
    }
    /**
     * return the phased amplitude of a data point at the specified coordinates
     * @param k slice index
     * @param j trace index
     * @param i point index
     * @param options
     * @return data value at input indexes
     */
     public double vertexValue(int k, int j, int i){
         int ni=0,nj=0,nk=0,maxi=0,maxj=0,maxk=np-1;
     	// swizzle input indexes to extract the appropriate sliceplane
         switch(sliceplane){
 	        case X: ni=i; nj=j; nk=k; maxi=slices-1; maxj=traces-1;break;
 	        case Y: ni=j; nj=k; nk=i; maxi=np-1; maxj=slices-1; break;
 	        default: // 1D 2D
 	        case Z: ni=k; nj=j; nk=i; maxi=np-1; maxj=traces-1; break;
        }
        ni=ni>maxi?maxi:ni;
        nj=nj>maxj?maxj:nj;
        nk=nk>maxk?maxk:nk;
		int adrs = 0;
		adrs = traces * np * ni + ws * nj * np + nk;
		if (adrs >= data_size || adrs < 0)
			return 0;
		double rvalue = vertexValue(adrs);
		if (complex) {
			double ivalue = vertexValue(adrs + np);
			if (absval){
				if(real)
					return Math.sqrt(rvalue*rvalue+ivalue*ivalue);
				else{					 
					int dtype = data_type & DTYPE;	
					if(dtype == FID) {
						double p=Math.atan2(rvalue, ivalue)/Math.PI;						
						if (p-lastval >= 1.0)
							sumphs -= 2.0;
						if (p-lastval < -1.0)
							sumphs += 2.0;						
						lastval=p;
						return p+sumphs;
					}
					else
						return 0.0;
				}				
				//return pa * Math.abs(rvalue) + pb * Math.abs(ivalue);
			}
			else
				return pa * rvalue + pb * ivalue;
		} else
			return rvalue;
    }

     public double get2DValue(int pt, int trc, int dtype){
     	if(dtype==0)
     		setReal();
     	else
     		setImag();
     	initPhaseCoeffs(pt);
     	return vertexValue(0, trc, pt);
     }
    
    
     /**
      *  Initialize phase coefficients
      * @param start     start point   
      * @param options   data mode (real/imaginary fid/spectrum)
      */
     protected void initPhaseCoeffs(int start){
         int dtype = data_type & DTYPE;

         if(complex && !absval) {
             double DTOR = Math.PI / 180;
             double phi = (real ? 0.0 : -90.0) + rp;
             if(dtype == SPECTRUM) {
                 phi += lp * (1 - ((double)start)/np);
                 double lpval = -DTOR * lp / ((double)(np - 1));
                 cosd = Math.cos(lpval); // delta cos 
                 sind = Math.sin(lpval); // delta sin 
             }
             phi *= DTOR;
             pa = Math.cos(phi);
             pb = Math.sin(phi);
         } else {
             if(real) {
                 pa = 0.5;
                 pb = 0.5;
             } else { // just draw baseline for imaginary dchnl
                 pa = 0.0;
                 pb = 0.0;
             }
         }
    }

     /**
      * Increment phase coefficients
      */
     protected void incrPhaseCoeffs(){
         if(complex && !absval){
             double anew = pa * cosd - pb * sind;
             pb = pb * cosd + pa * sind;
             pa = anew;
         }   	
     }
     protected void setReal(){
    	 real=true;
    	 imag=false;
    	 sumphs=lastval=0;
     }
     protected void setImag(){
    	 imag=true;
    	 real=false;
    	 sumphs=lastval=0;

     }

     protected void reset(){
    	 sumphs=lastval=0;
    	 setReal();
     }
}

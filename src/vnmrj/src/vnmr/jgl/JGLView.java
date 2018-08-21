/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

import javax.media.opengl.glu.*;
import javax.media.opengl.*;

public class JGLView implements JGLDef
{
    static public double RPD=2.0*Math.PI/360.0;

    double P[]=new double[16];
    double R[]=new double[16];
    double M[]=new double[16];
    int V[]=new int[4];
    double width,height;
    boolean invalid=true;
    boolean reset=true;
    boolean resized=true;
    double xoffset=0;
    double yoffset=0;
    double zoffset=0;
    double ascale=1;
    double dscale=1;
    double xscale=1;
    double yscale=1;
    double zscale=1;
    double xcenter=0;
    double aspect=1;
    double rotx =0, roty = 0.0, rotz = 0.0;
    double tilt = 0.0, twist = 0.0;
    double delx=0.2,dely=1.0;
    
    int projection=0;
    int sliceplane=0;
    int np=0,traces=0,slices=0;
    double ymin=0,ymax=0;
    int options=0;
    Point3D eye_vector=null;
    Point3D slice_vector=new Point3D();
    int maxslice=0;
    int numslices=0;
    int slice=0;
    int maxtrace=0;
    int data_type=0;
    int matrix_mode=GL2.GL_MODELVIEW;
    public Point3D yaxis=new Point3D(0,0.5,0);
    public Point3D xaxis=new Point3D(0.5,0,0);
    public Point3D vrot=new Point3D();
    public Volume  volume;
    GLU glu=new GLU();
    private GL2 gl=null;
    
    public JGLView(Volume vol) {
        volume = vol;
    }

    public String spString(int sp){
        if(sp==Z)
            return "Z";
        if(sp==X)
            return "X";
        if(sp==Y)
            return "Y";
        return "??";
    }

    public void setReset(){
        yaxis=new Point3D(0,0.5,0);
        xaxis=new Point3D(0.5,0,0);
        reset=true;
        invalid=true;
    }
    public void setInvalid(){
        invalid=true;
    }

    public boolean isInvalid(){
        return invalid;
    }

    public void setResized(){
        resized=true;
        invalid=true;
        //getViewport();
    }

    public void init(){
        gl=GLU.getCurrentGL().getGL2();
        gl.glMatrixMode(GL2.GL_MODELVIEW);
        gl.glLoadIdentity();
        getModelViewMatrix();
    }
    public void setOptions(int opts) {
        options=opts;
        //xcenter = ((options & DTYPE)==FID) ? 0.0 : 0.5;
        projection=options &PROJECTION;
        sliceplane=options &SLICEPLANE;
    }

    /** Set data bounds. 
     */
    public void setDataPars(int n, int t, int s, int d){
        np=n;
        traces=t;
        slices=s;
        data_type=d;
    }

    public int maxSlice(){
        return maxslice;
    }

    /** Set data scale multipliers. 
     */
    public void setDataScale(double mn, double mx){
        ymin=mn;
        ymax=mx;
    }

    /** Set axis scale multipliers. 
     */
    public void setScale(double a,double x, double y, double z){
        ascale=a;
        xscale=x;
        yscale=y;
        zscale=z;
    }

    /** Set view scale multipliers. 
     */
    public void setSpan(double sx,double sy, double sz){
    	Point3D ds=new Point3D(sx,sy,sz);
    	volume.setScale(ds);
    }
/** Set linear view offsets. 
     */
    public void setOffset(double x, double y, double z){
        xoffset=x;
        yoffset=y;
        zoffset=z;
    }
    /** Set rotation angles for 3D projections. 
     */
    public void setRotation3D(double x, double y, double z){
        rotx=x;
        roty=y;
        rotz=z;
    }
    /** Set rotation angles for 2D projections. 
     */
    public void setRotation2D(double x, double y){
    	tilt=x;
    	twist=y;
    }
    /** Set slant for oblique projection. 
     */
    public void setSlant(double x,double y){
        delx=x;
        dely=y;
    }
    /** Set eye vector for current view 
     */
    private  void setEyeVector() {
        Point3D pt1=volume.center();
        Point3D pt2=eyeMinZ();
        eye_vector=pt2.sub(pt1);
    }

    /** Return maximum z value of bounds box in screen space. 
     */
    public Point3D eyeMinZ() {
        Point3D p=new Point3D(width/2,height/2,0);
        p=unproject(p);
        return volume.minPoint(p);
    }

    /** return eye vector for current view 
    */
    public Point3D eyeVector(){
        return eye_vector;
    }

    public void setSlice(int s, int max, int num) {
        slice=s;
        maxslice=max;
        numslices=num;
    }
    public void setSliceVector(double x, double y, double z) {
        slice_vector=new Point3D(x,y,z);
    }
    /** return true if face is frontfacing 
      */
    public  boolean faceIsFrontFacing(int face) {
        Point3D normal=volume.faceNormal(face);
        double dp=eye_vector.dot(normal);
        if(dp>=0)
            return false;
        else
            return true;
    }

    /** return true if slice plane is frontfacing 
     */
    public  boolean sliceIsFrontFacing() {
        double dp=eye_vector.dot(slice_vector);
        boolean frontfacing=(dp<0);
        return frontfacing;
    }

    /** return true if slice plane is eyeplane 
     */
    public  boolean sliceIsEyeplane() {
        if(eye_vector.equals(slice_vector))
            return true;
        else
            return false;
    }
    private double planeDist(Point3D pt, Point3D plane) {
        return pt.x * plane.x + pt.y * plane.y + pt.z * plane.z + plane.w;
    }

    /** Create an orthographic view based on projection type
     *  ONETRACE:  1D spectum or FID trace
     *  OBLIQUE:   stacked plot
     *  TWOD:      overhead view (2D data sets only)
     *  SLICES:    rotational view (2D or 3D data sets) 
     *  THREED:    rotational view (3D data sets)  
     */
    public void setView(){
        getViewport();
        gl.glMatrixMode(GL2.GL_PROJECTION);
        gl.glLoadIdentity();
        double rx, ry;
        
        switch(projection) {
        case ONETRACE:
            dscale=Math.abs(ascale/(ymax-ymin));
            gl.glOrtho(0.0, 1.0, 0.0, aspect, 0.0, -1.0);
            gl.glTranslated(-xoffset/xscale, yoffset*aspect,  0.0);
            gl.glScaled(1.0/xscale, ascale/(ymax - ymin), 1.0);
            getProjectionMatrix();
            break;
        case OBLIQUE:
            double fdx = 1 - Math.abs(delx);
            double xoff=(delx >= 0)?0:Math.abs(delx);           
            double yoff=aspect/(traces - 1);
            double xview = -fdx * xoffset/xscale;
            
            double dyz=0.9*(1-yoff/aspect)*dely;
            yoff=(yoff-0.5)*dely+0.5;
            
            double dzx=delx;
            double mat[]=new double[16];
            mat[0]=2;    mat[4]=0;          mat[8]=2*dzx;   mat[12]=-1;
            mat[1]=0;    mat[5]=2/aspect;   mat[9]=2*dyz;   mat[13]=-1;
            mat[2]=0;    mat[6]=0;          mat[10]=2;      mat[14]=-1;
            mat[3]=0;    mat[7]=0;          mat[11]=0;      mat[15]=1;
            
            dscale=Math.abs(ascale/(ymax-ymin));
            gl.glLoadMatrixd(mat,0);
            gl.glTranslated(xview+xoff, yoff+yoffset,  0.0);
            gl.glScaled(fdx/xscale, dscale, 1.0);
            getProjectionMatrix();
            break;
        case TWOD:
        {
        	double ca=Math.cos(tilt*RPD);
        	double ys=0.5*Math.abs(ascale/(ymax-ymin)); 
        	double zs=2.0+ys;
         	dscale=ys;
            gl.glOrtho(-0.5, 0.5, aspect/2,-aspect/2, -zs, zs);
            gl.glTranslated( -(xoffset+0.5)/yscale, 0.0, 0);
            gl.glRotated(90, 1, 0, 0);
            gl.glTranslated(0, 0, zoffset/yscale-0.5*aspect*ca);
            gl.glScaled(1/yscale, 1, 1/yscale);
            gl.glRotated(tilt, 1, 0, 0);
            gl.glTranslated(0.5, 0, 0.5);
            gl.glRotated(twist, 0, 1, 0);
            gl.glScaled(1, ys, 1);
            gl.glTranslated(-0.5, 0, -0.5);
            getProjectionMatrix();
        }
            break;
        case SLICES:
        case THREED:
        	double ca=Math.cos(rotx*RPD);
         	gl.glOrtho(-0.5, 0.5, -0.5*aspect, 0.5*aspect, 1, -1);
            gl.glTranslated( -0.5/zscale, 0.0, 0);
            gl.glRotated(90, 1, 0, 0);
            gl.glTranslated(0, 0, -0.5*aspect*ca);
            gl.glScaled(1/zscale, 1/zscale, 1/zscale);
            gl.glRotated(rotx, 1, 0, 0);
            gl.glTranslated(0.5, 0, 0.5);
            gl.glRotated(roty, 0, -1, 0);
            gl.glTranslated(-0.5, 0, -0.5);
            getProjectionMatrix();
            break;
            
        }
        setEyeVector();
        gl.glMatrixMode(GL2.GL_MODELVIEW);
        gl.glLoadIdentity();
    }
    private void getModelViewMatrix(){
        gl.glGetDoublev(GL2.GL_MODELVIEW_MATRIX,M,0);
    }
    private void getProjectionMatrix(){
        gl.glGetDoublev(GL2.GL_PROJECTION_MATRIX,P,0);
    }
 
    public void pickMatrix(int x, int y,int pts){
        gl.glLoadIdentity();
        gl.glGetIntegerv(GL2.GL_VIEWPORT,V,0);
        double dx=(double)x;
        double dy=(double)(V[3]-y);
        double dp=(double)pts;
        glu.gluPickMatrix(dx, dy, dp,dp, V,0);
        gl.glMultMatrixd(P,0);
    }

    public void getViewport(){
        gl=GLU.getCurrentGL().getGL2();
        gl.glGetIntegerv(GL2.GL_VIEWPORT,V,0);
        width=V[2];
        height=V[3];
        aspect = height/width;
    }
   
    /** Utility to multiply a 4x4 matrix and a 4x1 vector 
     */
     public void MatrixMulVector(double m[],double v[], double c[]){
        c[0]=m[0]*v[0]+m[4]*v[1]+m[8]*v[2] +m[12];
        c[1]=m[1]*v[0]+m[5]*v[1]+m[9]*v[2] +m[13];
        c[2]=m[2]*v[0]+m[6]*v[1]+m[10]*v[2]+m[14];
        c[3]=m[3]*v[0]+m[7]*v[1]+m[11]*v[2]+m[15];
    }

    /** Return X component of projection matrix 
     */
    public double getXProjection(double x, double y, double z){        
        double wx=P[0]*x+P[4]*y+P[8]*z+P[12];
        double winx=0.5*(wx+1);
        return winx;
    }

    /** Return Y component of projection matrix 
     */
    public double getYProjection(double x, double y, double z){        
        double wy=P[1]*x+P[5]*y+P[9]*z+P[13];
        double winy=0.5*(wy+1);
        return winy;
    }

    /** Return Z component of projection matrix 
     */
    public double getZProjection(double x, double y, double z){        
        double wz=P[2]*x+P[6]*y+P[10]*z+P[14];
        double winz=0.5*(wz+1);
        return winz;
    }

    /** Convert a point in object space to a point in screen space 
     */
    public Point3D project(Point3D p){
        return project(p.x,p.y,p.z);        
    }
 
    /** Return a point in screen space from a set of coordinates in object space 
     */
    public Point3D project(double x, double y, double z){ 
        double mat[]=new double[3];
        glu.gluProject(x, y, z, M,0,P,0,V,0,mat,0);      
        return new Point3D(mat[0],mat[1],mat[2]);
    }

    /** Convert a point in screen space to a point in object space 
     */
    public Point3D unproject(Point3D p){
        return unproject(p.x,p.y,p.z);        
    }
 
    /** Return a point in object space from a set of coordinates in screen space 
     */
    public Point3D screenToRotation(double x, double y, double z){
        double mat[]=new double[3];
        glu.gluUnProject(x,y,z, M,0,R,0,V,0,mat,0);
        return new Point3D(mat[0],mat[1],mat[2]);        
    }

    /** Return a point in object space from a set of coordinates in screen space 
     */
    public Point3D unproject(double x, double y, double z){ 
         double mat[]=new double[3];
         glu.gluUnProject(x, y, z, M,0,P,0,V,0,mat,0); 
         return new Point3D(mat[0],mat[1],mat[2]);
     }
    /** Return a point in object space from a point obtained 
     *  via GL_SELECT where: 
     *    x,w:  screen coordinates 
     *    z:    depth of hit point in screen space   
     */
    public Point3D pickPoint(double x, double y, double z){ 
         double mat[]=new double[3];
         glu.gluUnProject(x, height-y, z, M,0,P,0,V,0,mat,0); 
         return new Point3D(mat[0],mat[1],mat[2]);
    }
    /** Return the slice normal vector
     */
    public Point3D sliceVector(){
        return slice_vector;
    }
    /** Return the current slice plane
     */
    public Point3D slicePlane(){
        Point3D min=sliceMinPoint();
        Point3D max=sliceMaxPoint();
        double s=((double)slice)/maxslice;
        return min.plane(max,s);
    }
    /** Return the current width plane
     */
    public Point3D widthPlane(){
        int start=0;
        if((options & REVDIR)==0){
            start=slice+numslices;
            start=start<maxslice?start:maxslice;
        }
        else{
            start=slice-numslices;
            start=start<0?0:start;
        }
        Point3D min=sliceMinPoint();
        Point3D max=sliceMaxPoint();
        double s=((double)start)/maxslice;
        return min.plane(max,s);
    }
    /** Return starting point along the eye vector 
      */
    public Point3D eyeMinPoint(){
        return volume.center().sub(eye_vector).scale(0.9999);
    }

    /** Return end point along the eye vector 
     */
   public Point3D eyeMaxPoint(){
       return eye_vector.add(volume.center()).scale(0.9999);
   }

   /** Return starting point along the slice vector 
    */
   public Point3D sliceMinPoint(){
      return volume.center().sub(slice_vector).scale(0.9999);
   }

   /** Return end point along the slice vector 
   */
    public Point3D sliceMaxPoint() {
        return slice_vector.add(volume.center()).scale(0.9999);
    }

    /** Return the fractional distance of a point in a plane 
     *  along the slice vector. 0:first slice, 1:last slice
     */
    public double sliceDepth(Point3D p){ 
        Point3D min=sliceMinPoint();
        Point3D max=sliceMaxPoint();
        Point3D z1plane=max.plane(min, 0);
        double d=planeDist(min,z1plane);
        double s=planeDist(p,z1plane)/d;
        s=s<0?0:s;
        s=s>1?1:s;
        return s;
    }
    /** Return a variable length array of points, sorted in clockwize order,
     *  that consist of all intersections of the line segments of the bounds 
     *  box with a given plane in object space.
     */
    public Point3D[] boundsPts(Point3D plane) {
        return volume.boundsPts(plane);
    }

    /** Obtain a variable length array of points by intersecting
     *  a plane with the volume bounds box and the current sliceplane. 
     */
    public Point3D[] slicePts(Point3D plane) {
    	
    	Point3D[] spts;
    	// clip rotated slice planes to volume box by rotating
    	// slice_vector before calls to slicePlane & widthPlane
    	Point3D sv=slice_vector;
    	pushRotationMatrix();
    	slice_vector=matrixMultiply(slice_vector);
    	popMatrix();
    	
    	Point3D ps=slicePlane();
    	Point3D pw=widthPlane();

    	slice_vector=sv;
    	 
        if((options & REVDIR)>0)
        	spts=volume.slicePts(plane,ps,pw);
        else
        	spts=volume.slicePts(plane,pw,ps);

        return spts;
    }

    public void pushProjectionMatrix(){
        gl.glMatrixMode(GL2.GL_PROJECTION);
        gl.glPushMatrix();
    }
    public void pushModelMatrix(){
        gl.glMatrixMode(GL2.GL_MODELVIEW);
        gl.glPushMatrix();
    }
    public void popMatrix(){
        gl.glPopMatrix();
    }
    
    public Point3D matrixMultiply(Point3D v){
        Point3D p=new Point3D();
        double D[]=new double[16];
        gl.glGetDoublev(GL2.GL_MODELVIEW_MATRIX,D,0);
        p.x=v.x*D[0]+v.y*D[4]+v.z*D[8]+D[12];
        p.y=v.x*D[1]+v.y*D[5]+v.z*D[9]+D[13];
        p.z=v.x*D[2]+v.y*D[6]+v.z*D[10]+D[14];
        return p;
    }
    
    public void pushRotationMatrix(){
	    pushModelMatrix();
	    if(projection == THREED){
	        gl.glRotated(vrot.x, 0, 1, 0);
	        gl.glRotated(vrot.y, 1, 0, 0);
	        gl.glRotated(vrot.z, 0, 0, 1);
	    }
    }
    /**
     * Return the intersection of a trace with the view clip planes
     * @param start Point defining starting point of a trace
     * @param end   Point defining ending point of a trace
     * @return      Point defining of a trace with clipped ends removed
     */
    public Point3D testBounds(Point3D start, Point3D end){
        double gy1=0,gy2=0,gx1=0,gx2=0;

        boolean clipped=false;
        
        Point3D P=new Point3D();
        
        gy1=getYProjection(start.x,start.y,start.z);
        gy2=getYProjection(end.x,end.y,end.z);
        gx1=getXProjection(start.x,start.y,start.z);
        gx2=getXProjection(end.x,end.y,end.z);
                  
        if(gy2<gy1){
            gy1=1-gy1;
            gy2=1-gy2;                   
        }
        if(gx2<gx1){
            gx1=1-gx1;
            gx2=1-gx2;
        }
        if(gy1>1 || gy2<0 )
            clipped=true;
        else if(gx1>1 || gx2<0){
            clipped=true;
        }
        else{ // not clipped          
            double dx=Math.abs(gx2-gx1);
            double dy=Math.abs(gy2-gy1);
            double fl=1.0/dx;
            double fs=gx1/dx;
            double fe=gx2/dx;
            
            fs=fs<0?-fs:0;
            fe=fs+fl;
            fe=fe>1?1.0:fe;  
            P.x=fs<0?0:fs;
            P.y=fe>1.0?1.0:fe;
            P.z=dy;
        }       
        P.w=clipped?1.0:0.0;
        return P;
    }
}

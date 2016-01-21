/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
*/

#include <math.h>
#include "CGLDef.h"
#include "Point3D.h"
#include "Volume.h"
#include "CGLView.h"
#include <iostream>
//-------------------------------------------------------------
// constructor
//-------------------------------------------------------------
CGLView::CGLView(Volume *vol) {
    volume = vol;
    invalid=true;
    reset=true;
    sx=sy=sz=1;
    ymax=1;
    ymin=0;
}

//-------------------------------------------------------------
// init view
//-------------------------------------------------------------
void CGLView::init(){
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    getModelViewMatrix();
}

//-------------------------------------------------------------
// reset view
//-------------------------------------------------------------
void CGLView::setReset(){
    yaxis=Point3D(0,0.5,0);
    xaxis=Point3D(0.5,0,0);
    reset=true;
    invalid=true;
}

//-------------------------------------------------------------
// invalidate view
//-------------------------------------------------------------
void CGLView::setInvalid(){
    invalid=true;
}

//-------------------------------------------------------------
// invalidate view
//-------------------------------------------------------------
void CGLView::setResized(){
    resized=true;
    getViewport();
}

//-------------------------------------------------------------
// capture current view.
//-------------------------------------------------------------
void CGLView::setOptions(int opts) {
    options=opts;
    //xcenter = ((options & DTYPE)==FID) ? 0.0 : 0.5;
    projection=options & PROJECTION;
}

//-------------------------------------------------------------
// Set data parameters.
//-------------------------------------------------------------
void CGLView::setDataPars(int n, int t, int s, int d){
    np=n;
    traces=t;
    slices=s;
    data_type=d;
}

//-------------------------------------------------------------
// Set axis scale multipliers.
//-------------------------------------------------------------
void CGLView::setDataScale(double mn, double mx){
    ymin=mn;
    ymax=mx;
}

//-------------------------------------------------------------
// Set axis scale multipliers.
//-------------------------------------------------------------
void CGLView::setScale(double a,double x, double y, double z){
    ascale=a;
    xscale=x;
    yscale=y;
    zscale=z;
}

//-------------------------------------------------------------
// Set linear view offsets.
//-------------------------------------------------------------
void CGLView::setOffset(double x, double y, double z){
    xoffset=x;
    yoffset=y;
    zoffset=z;
}

//-------------------------------------------------------------
// Set data scale multipliers.
//-------------------------------------------------------------
void CGLView::setSpan(double x, double y, double z){
    sx=x;
    sy=y;
    sz=z;
	Point3D dscale(sx,sy,sz);
	volume->setScale(dscale);
}

//-------------------------------------------------------------
// Set rotation angles for 2D projections.
//-------------------------------------------------------------
void CGLView::setRotation2D(double x, double y){
	tilt=x;
	twist=y;
}

//-------------------------------------------------------------
// Set rotation angles for 3D projections.
//-------------------------------------------------------------
void CGLView::setRotation3D(double x, double y, double z){
    last_rotx=rotx;
    last_roty=roty;
    rotx=x;
    roty=y;
    rotz=z;
}

//-------------------------------------------------------------
// Set slant for oblique projection.
//-------------------------------------------------------------
void CGLView::setSlant(double x,double y){
    delx=x;
    dely=y;
}

//-------------------------------------------------------------
// Return minimum z value of bounds box in screen space.
//-------------------------------------------------------------
Point3D CGLView::eyeMinZ() {
    Point3D p(width/2,height/2,0);
    p=unproject(p);
    return volume->minPoint(p);
}

//-------------------------------------------------------------
// Set eye vector for current view
//-------------------------------------------------------------
void CGLView::setEyeVector() {
    Point3D pt1=volume->center;
    Point3D pt2=eyeMinZ();
    eye_vector=pt2.sub(pt1);
}

//-------------------------------------------------------------
// return eye vector for current view
//-------------------------------------------------------------
Point3D CGLView::eyeVector(){
    return eye_vector;
}

//-------------------------------------------------------------
// Set slice for current view
//-------------------------------------------------------------
void CGLView::setSlice(int s, int max, int num) {
    slice=s;
    maxslice=max;
    numslices=num;
}

//-------------------------------------------------------------
// Set slice vector for current view
//-------------------------------------------------------------
void CGLView::setSliceVector(double x, double y, double z) {
    Point3D p(x,y,z);
    slice_vector=p;
}

//-------------------------------------------------------------
// return true if slice plane is frontfacing
//-------------------------------------------------------------
bool CGLView::sliceIsFrontFacing() {
    double dp=eye_vector.dot(slice_vector);
    if(dp>=0)
        return false;
    else
        return true;
}

//-------------------------------------------------------------
// return true if slice plane is eyeplane
//-------------------------------------------------------------
bool CGLView::sliceIsEyeplane() {
    if(eye_vector==slice_vector)
        return true;
    else
        return false;
}

//-------------------------------------------------------------
// Return the distance between a point and a plane.
//-------------------------------------------------------------
double CGLView::planeDist(Point3D pt, Point3D plane) {
    return pt.x * plane.x + pt.y * plane.y + pt.z * plane.z + plane.w;
}

//-------------------------------------------------------------
// Create an orthographic view based on projection type
// ONETRACE:  1D spectum or FID trace
// OBLIQUE:   stacked plot
// TWOD:      overhead view (2D data sets only)
// SLICES:    rotational view (2D or 3D data sets)
// THREED:    rotational view (3D data sets)
//-------------------------------------------------------------
void CGLView::setView(){
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    Point3D pc;
    double rx, ry, rz;
    switch(projection) {
    case ONETRACE:
        dscale=fabs(ascale/(ymax-ymin));
        glOrtho(0.0, 1.0, 0.0, aspect, 0.0, -1.0);
        glTranslated(-xoffset/xscale, yoffset*aspect,  0.0);
        glScaled(1/xscale, ascale/(ymax - ymin), 1.0);
        glGetDoublev(GL_PROJECTION_MATRIX,P);
        break;
    case OBLIQUE:
    {
        double fdx = 1 - fabs(delx);
        double xoff=(delx >= 0)?0:fabs(delx);
        double yoff=aspect/(traces - 1);
        double xview = -fdx * xoffset/xscale;

        double dyz=0.9*(1-yoff/aspect)*dely;
        yoff=(yoff-0.5)*dely+0.5;

        double dzx=delx;
        double C[16];
        C[0]=2;    C[4]=0;          C[8]=2*dzx;   C[12]=-1;
        C[1]=0;    C[5]=2/aspect;   C[9]=2*dyz;   C[13]=-1;
        C[2]=0;    C[6]=0;          C[10]=2;      C[14]=-1;
        C[3]=0;    C[7]=0;          C[11]=0;      C[15]=1;

        dscale=fabs(ascale/(ymax-ymin));
        glLoadMatrixd(C);
        glTranslated(xview+xoff, yoff+yoffset,  0.0);
        glScaled(fdx/xscale, dscale, 1.0);
        glGetDoublev(GL_PROJECTION_MATRIX,P);
        }
        break;
    case TWOD:
       {
       	double ca=cos(rotx*RPD);
       	double ys=0.5*fabs(ascale/(ymax-ymin));
       	double zs=2+ys;
       	dscale=ys;
	    glOrtho(-0.5, 0.5, aspect/2,-aspect/2, -zs, zs);
	    glTranslated( -(xoffset+0.5)/yscale, 0.0, 0);
	    glRotated(90, 1, 0, 0);
	    glTranslated(0, 0, zoffset/yscale-0.5*aspect*ca);
	    glScaled(1/yscale, 1, 1/yscale);
	    glRotated(tilt, 1, 0, 0);
	    glTranslated(0.5, 0, 0.5);
	    glRotated(twist, 0, 1, 0);
	    glScaled(1, ys, 1);
	    glTranslated(-0.5, 0, -0.5);
        glGetDoublev(GL_PROJECTION_MATRIX,P);
        }
        break;
    case SLICES:
    case THREED:
    	pc=volume->center;
        glOrtho(-0.5, 0.5, -0.5*aspect, 0.5*aspect, 1, -1);
        if(reset){
            glRotated(roty, yaxis.x, yaxis.y, yaxis.z);
            glRotated(rotx, xaxis.x, xaxis.y, xaxis.z);
            invalid=true;
        }
        else if( (options & FIXAXIS)!=0){
            ry=roty-last_roty;
            rx=rotx-last_rotx;
            glLoadMatrixd(R);
            glRotated(ry, yaxis.x, yaxis.y, yaxis.z);
            glRotated(rx, xaxis.x, xaxis.y, xaxis.z);
        }
        else{
            ry=roty-last_roty;
            rx=rotx-last_rotx;
            glLoadMatrixd(R);
            yaxis=screenToRotation(width/2,height,0.5);
            glRotated(ry, yaxis.x, yaxis.y, yaxis.z);
            glGetDoublev(GL_PROJECTION_MATRIX,R);
            xaxis=screenToRotation(width,height/2,0.5);
            glRotated(rx, xaxis.x, xaxis.y, xaxis.z);
        }
        glGetDoublev(GL_PROJECTION_MATRIX,R);
        glScaled(1/zscale, 1/zscale, 1/zscale);
        glTranslated(-pc.x,-pc.y,-pc.z);
        glGetDoublev(GL_PROJECTION_MATRIX,P);
        setEyeVector();
        invalid=false;
        reset=false;
        resized=false;
        break;
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

//-------------------------------------------------------------
// Utility to multiply a 4x4 matrix and a 4x1 vector
//-------------------------------------------------------------
void CGLView::MatrixMulVector(double *m ,double *v, double *c){
    c[0]=m[0]*v[0]+m[4]*v[1]+m[8]*v[2] +m[12];
    c[1]=m[1]*v[0]+m[5]*v[1]+m[9]*v[2] +m[13];
    c[2]=m[2]*v[0]+m[6]*v[1]+m[10]*v[2]+m[14];
    c[3]=m[3]*v[0]+m[7]*v[1]+m[11]*v[2]+m[15];
}

//-------------------------------------------------------------
// Return X component of projection matrix
//-------------------------------------------------------------
double CGLView::getXProjection(double x, double y, double z){
    double wx=P[0]*x+P[4]*y+P[8]*z+P[12];
    double winx=0.5*(wx+1);
    return winx;
}

//-------------------------------------------------------------
// Return Y component of projection matrix
//-------------------------------------------------------------
double CGLView::getYProjection(double x, double y, double z){
    double wy=P[1]*x+P[5]*y+P[9]*z+P[13];
    double winy=0.5*(wy+1);
    return winy;
}

//-------------------------------------------------------------
// Return Z component of projection matrix
//-------------------------------------------------------------
double CGLView::getZProjection(double x, double y, double z){
    double wz=P[2]*x+P[6]*y+P[10]*z+P[14];
    double winz=0.5*(wz+1);
    return winz;
}

//-------------------------------------------------------------
// Convert a point in object space to a point in screen space
//-------------------------------------------------------------
Point3D CGLView::project(Point3D p){
    return project(p.x,p.y,p.z);
}

//-------------------------------------------------------------
// Convert a point in screen space to a point in object space
//-------------------------------------------------------------
Point3D CGLView::unproject(Point3D p){
    return unproject(p.x,p.y,p.z);
}

//-------------------------------------------------------------
// Return a point in screen space from a set of coordinates in
// object space
//-------------------------------------------------------------
Point3D CGLView::project(double x, double y, double z){
    double wx,wy,wz;
    gluProject(x, y, z, M,P,V,&wx,&wy,&wz);
    return Point3D(wx/width,wy/height,wz);
}

//-------------------------------------------------------------
// Return a point in object space from a set of coordinates in
// screen space
//-------------------------------------------------------------
Point3D CGLView::unproject(double wx, double wy, double wz){
     double x,y,z;
     gluUnProject(wx, wy, wz, M,P,V,&x,&y,&z);
     return Point3D(x,y,z);
 }

//-------------------------------------------------------------
// Return a point in object space from a set of coordinates in
// screen space using rotation matrix only
//-------------------------------------------------------------
Point3D CGLView::screenToRotation(double w, double h, double d){
    double x,y,z;
    gluUnProject(w,h,d, M,R,V,&x,&y,&z);
    return Point3D(x,y,z);
}

//-------------------------------------------------------------
// Return starting point along the slice vector
//-------------------------------------------------------------
Point3D CGLView::sliceMinPoint(){
    return volume->center.sub(slice_vector).scale(0.9999);
}

//-------------------------------------------------------------
// Return end point along the slice vector
//-------------------------------------------------------------
Point3D CGLView::sliceMaxPoint(){
    return slice_vector.add(volume->center).scale(0.9999);
}

//-------------------------------------------------------------
// Return starting point along the eye vector
//-------------------------------------------------------------
Point3D CGLView::eyeMinPoint(){
    return volume->center.sub(eye_vector).scale(0.9999);
}

//-------------------------------------------------------------
// Return end point along the eyw vector
//-------------------------------------------------------------
Point3D CGLView::eyeMaxPoint(){
    return eye_vector.add(volume->center).scale(0.9999);
}

//-------------------------------------------------------------
// Return the fractional distance of a point in a plane
// along the slice vector. 0:first slice, 1:last slice
//-------------------------------------------------------------
double CGLView::sliceDepth(Point3D p){
    Point3D min=sliceMinPoint();
    Point3D max=sliceMaxPoint();
    Point3D z1plane=max.plane(min, 0);
    double d=planeDist(min,z1plane);
    double s=planeDist(p,z1plane)/d;
    s=s<0?0:s;
    s=s>1?1:s;
    return s;
}
//-------------------------------------------------------------
//  return the number of points in a slice
//-------------------------------------------------------------
int CGLView::sliceSize() {
    switch(sliceplane){
    case X:
        return slices*traces;
    case Y:
        return slices*np;
    default:
    case Z:
        return traces*np;
    }
}

//-------------------------------------------------------------
//  return slice plane boundary points
//-------------------------------------------------------------
int CGLView::boundsPts(Point3D plane, Point3D *returnPts){
    return volume->boundsPts(plane,returnPts);
}

//-------------------------------------------------------------
//  return slice plane boundary points
//-------------------------------------------------------------
int CGLView::slicePts(Point3D plane, Point3D *returnPts){
	// clip rotated slice planes to volume box by rotating
	// slice_vector before calls to slicePlane & widthPlane
	Point3D sv=slice_vector;
    Point3D splane;
    Point3D wplane;
    int start=0;
	pushRotationMatrix();
	slice_vector=matrixMultiply(slice_vector);
	glPopMatrix();

    Point3D min=sliceMinPoint();
    Point3D max=sliceMaxPoint();

    slice_vector=sv;

    if((options & REVDIR)==0){
        start=slice+numslices;
        start=start<maxslice?start:maxslice;
    }
    else{
        start=slice-numslices;
        start=start<0?0:start;
    }

    wplane=min.plane(max,((double)start)/maxslice);
    splane=min.plane(max,((double)slice)/maxslice);

    if((options & REVDIR)>0)
        return volume->slicePts(plane,splane,wplane,returnPts);
    else
        return volume->slicePts(plane,wplane,splane,returnPts);
}
//-------------------------------------------------------------
// Return the intersection of a trace with the view clip planes
//  start Point defining starting point of a trace
//  end   Point defining ending pointof a trace
//  return      Point defining of a trace with clipped ends removed
//-------------------------------------------------------------
Point3D CGLView::testBounds(Point3D start, Point3D end){
	double gy1=0,gy2=0,gx1=0,gx2=0;

	bool clipped=false;

	Point3D P;

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
		double dx=fabs(gx2-gx1);
		double dy=fabs(gy2-gy1);
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


void CGLView::pushRotationMatrix(){
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
	if(projection == THREED){
		glRotated(vrot.x, 0, 1, 0);
		glRotated(vrot.y, 1, 0, 0);
		glRotated(vrot.z, 0, 0, 1);
	}
}

Point3D CGLView::matrixMultiply(Point3D v){
	Point3D p;
	double D[16];
	glGetDoublev(GL_MODELVIEW_MATRIX,D);
	p.x=v.x*D[0]+v.y*D[4]+v.z*D[8]+D[12];
	p.y=v.x*D[1]+v.y*D[5]+v.z*D[9]+D[13];
	p.z=v.x*D[2]+v.y*D[6]+v.z*D[10]+D[14];
	return p;
}


// private functions

void CGLView::getModelViewMatrix(){
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
}
void CGLView::getProjectionMatrix(){
    glGetDoublev(GL_PROJECTION_MATRIX,P);
}
void CGLView::getViewport(){
    glGetIntegerv(GL_VIEWPORT,V);
    width=V[2];
    height=V[3];
    aspect = height/width;
}



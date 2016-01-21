/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * CGLDataMgr.cpp
 *
 *  Created on: Jan 22, 2010
 *      Author: deans
 */

#include "CGLDataMgr.h"

//-------------------------------------------------------------
// CGLDataMgr::CGLDataMgr()	constructor
//-------------------------------------------------------------
CGLDataMgr::CGLDataMgr(){
	vertexData=0;
	mapData=0;
	np=0;
	trace=0;
	slice=0;
	data_type=0;
	complex=false;
	absval=false;
	rp=lp=0;
	traces=1;
	slices=1;
	step=1;
	newDataGeometry=true;
	ymax=1.0;
	ymin=0.0;
	data_size=0;
	mapfile[0]=0;
    projection=0;
    sliceplane=Z;
    allpoints=false;
    pa = 1;pb = 0;cosd = 1;sind = 0;
    dim=0;
    ws=0;
    setReal();
}

/**
 * Calculate normalized coordinates for a set of input indexes
 * @param k  slice index
 * @param j  trace index
 * @param i  point index
 * @return a point containing normalized coordinates
 */
Point3D CGLDataMgr::getPoint(int k, int j, int i){
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
	return Point3D(x,y,z);
}

/**
 * return the phased amplitude of a data point at the specified coordinates
 * @param k slice index
 * @param j trace index
 * @param i point index
 * @param options
 * @return data value at input indexes
 */
 double CGLDataMgr::vertexValue(int k, int j, int i){
	 int ni=0,nj=0,nk=0,maxi=0,maxj=0;
	// swizzle input indexes to extract the appropriate sliceplane
	 switch(sliceplane){
		case X: ni=i; nj=j; nk=k; maxi=slices-1; maxj=traces-1;break;
		case Y: ni=j; nj=k; nk=i; maxi=np-1; maxj=slices-1; break;
		default: // 1D 2D
		case Z: ni=k; nj=j; nk=i; maxi=np-1; maxj=traces-1; break;
	}
	ni=ni>maxi?maxi:ni;
	nj=nj>maxj?maxj:nj;
	int adrs = 0;
	adrs = traces * np * ni + ws * nj * np + nk;
	if (adrs >= data_size || adrs < 0)
		return 0;
	double rvalue = vertexValue(adrs);
	if (complex) {
		double ivalue = vertexValue(adrs + np);
		if (absval)
			return pa * fabs(rvalue) + pb * fabs(ivalue);
		else
			return pa * rvalue + pb * ivalue;
	} else
		return rvalue;
}

/**
 *  Initialize phase coefficients
 * @param start     start point
 * @param options   data mode (real/imaginary fid/spectrum)
*/
void CGLDataMgr::initPhaseCoeffs(int start) {
	int dtype = data_type & DTYPE;

	if (complex && !absval) {
		double DTOR = PI / 180;
		double phi = (real ? 0.0 : -90.0) + rp;
		if (dtype == SPECTRUM) {
			phi += lp * (1 - ((double) start) / np);
			double lpval = -DTOR * lp / ((double) (np - 1));
			cosd = cos(lpval); // delta cos
			sind = sin(lpval); // delta sin
		}
		phi *= DTOR;
		pa = cos(phi);
		pb = sin(phi);
	} else {
		if (real) {
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
void CGLDataMgr::incrPhaseCoeffs() {
	if (complex && !absval) {
		double anew = pa * cosd - pb * sind;
		pb = pb * cosd + pa * sind;
		pa = anew;
	}
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include <limits.h>
#include <float.h>
#include "AspUtil.h"
#include "AspCell.h"
#include "AspCursor.h"
#include "AspDis1D.h"
#include "graphics.h"

extern "C" {
void scale2d(int drawbox, int yoffset, int drawscale, int dcolor);
void set_vscale(int off, double vscale);
void set_vscaleMode(int mode);
void setAspMode(int mode);
}

spAspCell_t nullAspCell = spAspCell_t(NULL);

AspCell::AspCell(double px, double py, double pw, double ph, double vx, double vy, double vw, double vh) {

   pstx=px;
   psty=py;
   pwd=pw;
   pht=ph;
   pwd = (pwd > 0) ? pwd:100.0;
   pht = (pht > 0) ? pht:100.0;

   vstx=vx;
   vsty=vy;
   vwd=vw;
   vht=vh;
   vwd = (vwd != 0) ? vwd:1.0;
   vht = (vht != 0) ? vht:1.0;

   xname="";
   yname="";
   xlabel="";
   ylabel="";

   dataInfo = nullAspData;
}

AspCell::AspCell(double px, double py, double pw, double ph) {
   pstx=px;
   psty=py;
   pwd=pw;
   pht=ph;
   pwd = (pwd > 0) ? pwd:100.0;
   pht = (pht > 0) ? pht:100.0;
   vstx=pstx;
   vsty=psty;
   vwd=pwd;
   vht=pht;
   xname="";
   yname="";
   xlabel="";
   ylabel="";

   dataInfo = nullAspData;
}

AspCell::~AspCell() {
}

// Note, mm starts from lower left corner
// pix starts from upper left corner
double AspCell::pix2mm(int dim, double pix) {
   double mmwd = getReal("wc",100.0);
   double mmht = getReal("wc2",100.0);
   double pix2mmX = 1;
   if(mmwd > 0 && pwd > 0) pix2mmX = mmwd/pwd;
   double pix2mmY = 1;
   if(mmht > 0 && pht > 0) pix2mmY = mmht/pht;

   if(dim == VERT) return mmht - (pix) * pix2mmY;
   else return (pix) * pix2mmX;
}

double AspCell::mm2pix(int dim, double mm) {
   double mmwd = getReal("wc",100.0);
   double mmht = getReal("wc2",100.0);
   double mm2pixX = 1;
   if(mmwd > 0 && pwd > 0) mm2pixX = pwd/mmwd;
   double mm2pixY = 1;
   if(mmht > 0 && pht > 0) mm2pixY = pht/mmht;

   if(dim == VERT) return (mmht-mm) * mm2pixY;
   else return (mm) * mm2pixX;
}

double AspCell::pix2val(int dim, double pix, bool mm) {
 if(mm) {
   return pix2mm(dim,pix);
 } else {
   if(dim == VERT && vht < 0) return (pix-psty) * (vht/pht) + vsty - vht;
   else if(dim == VERT) return vsty - (psty+pht-pix) * (vht/pht);
   else return vstx - (pix-pstx) * (vwd/pwd);
 }
}

double AspCell::val2pix(int dim, double val, bool mm) {
//Winfoprintf("##-convert %d %f %f %f %f %f %f",dim,psty,val,vsty,vht,pht,vht);
 if(mm) {
   return mm2pix(dim,val);
 } else {
   if(dim == VERT && vht < 0) return psty + (val-vsty+vht) * (pht / vht);
   else if(dim == VERT) return psty + pht - (vsty-val) *(pht / vht);
   else return pstx + (vstx-val) *(pwd / vwd);
 }
}

double AspCell::offsetval2pix(int dim, double val, bool mm, double off) {
 if(mm) {
   return mm2pix(dim,val);
 } else {
   if(dim == VERT && vht < 0)
   {
      return psty + (val-vsty-off+vht) * (pht / vht);
   }
   else if(dim == VERT)
   {
      return psty + pht - (vsty-val) *(pht / vht);
   }
   else
   {
      return pstx + (vstx-val) *(pwd / vwd);
   }
 }
}

bool AspCell::select(int x, int y) {
   selected=false;
   double xmax=pstx+pwd;
   double ymax=psty+pht;
   if (showPlotBx == 0)
      ymax = mnumypnts;
   if(x>=pstx && x<=xmax && y>=psty && y<=ymax) {
      selected=true;
   }
   return selected;
}

void AspCell::getPixCell(double &px, double &py, double &pw, double &ph)
{
   px=pstx;
   py=psty;
   pw=pwd;
   ph=pht;
}

void AspCell::getValCell(double &vx, double &vy, double &vw, double &vh)
{
   vx=vstx;
   vy=vsty;
   vw=vwd;
   vh=vht;
}

void AspCell::setAxisNames(string xnm, string ynm)
{
   xname=xnm;
   yname=ynm;
}

void AspCell::setAxisLabels(string xlb, string ylb)
{
   xlabel=xlb;
   ylabel=ylb;
}

double AspCell::getCali(int dim) {
   double d=1.0;
   if(dim == VERT) {
	P_getreal(CURRENT, "wc2", &d, 1);
   	if(d<1.0) d=100.0;
   	return pht/d; 
   } else {
	P_getreal(CURRENT, "wc", &d, 1);
   	if(d<1.0) d=100.0;
   	return pwd/d; 
   }
}

void AspCell::showAxis(int flag, int mode) { // only do AX_SOUTH and AX_WEST for now
   if(flag == 0) return;
   if(dataInfo == nullAspData) return;

   setAspMode(mode);
	double cali = getCali(VERT);
	dfpnt = (int)(pstx);
	dnpnt = (int)pwd;
	dfpnt2 = mnumypnts - (int)(psty+pht);
	dnpnt2 = (int)pht;
	
	if(flag & AX_WEST) { // if AX_WEST is on, AX_SOUTH is implied
	  double vpos = cali*(dataInfo->getVpos());
	  double vscale = cali*(dataInfo->getVscale());
	  set_vscale((int)(dfpnt2+vpos), vscale);
	  set_vscaleMode(1);
	} else set_vscaleMode(0);

	if((flag & AX_SOUTH) || (flag & AX_WEST)) {
  	  set_background_region((int)pstx-1,(int)psty,(int)pwd,(int)pht,BOX_BACK,100);
	  AspUtil::drawBox(pstx-1,psty,pwd,pht,SCALE_COLOR);
	  scale2d(0,0,1,SCALE_COLOR); // this only draw the axis
					// so the box has to be drawn and fill separately
        } else if(flag & AX_BOX) {
  	  set_background_region((int)pstx-1,(int)psty,(int)pwd,(int)pht,BOX_BACK,100);
	  AspUtil::drawBox(pstx-1,psty,pwd,pht,SCALE_COLOR);
	}
   setAspMode(0); // NONE_ASP_MODE for dscale
}

// similar to ybars: decimate/convert array data to pixels in voxel (vx,vy,vw,vh).
// then call GraphicsWin::drawPolyline.
void AspCell::drawPolyline(float *data, int npts, int step, double vx, double vy, double vw, double vh, int color, double vScale, double yoff) {

   if(!data) return;
   if(step < 1) step = 1;
   if(!data || npts < 2*step) return;

   bool ybars=true; // set to false to draw polyline
   if(ybars) {
	AspUtil::drawYbars(data,npts,step,vx,vy,vw,vh,color,vScale,yoff,0);
	return;
   }

   float miny, maxy;

   int n, ind, np = npts/step;
   double dx = (double)(vw) / (double)(np-1);
   Dpoint_t *p;
   if(np <= 2*vw) { // data points fewer than vox width vw
     n = np;
     p = new Dpoint_t[np];
     for(int i=0; i<np; i++) {
        ind = i*step;
        p[i].x= vx + i*dx;
        p[i].y= vy + vh - yoff - vScale * data[ind];
     }
   } else if(vw < 2.0) { // make ybars if data points are more than vw 
     n=2;
     p = new Dpoint_t[2*n+4]; // 2*n should be enough, but add 4 to be safe.
     miny = FLT_MAX;
     maxy = -0.1*FLT_MAX;
     double yp = vy + vh - yoff - 1;
     for(int i=0; i<np; i++) {
        ind = i*step;
        if(data[ind] > maxy) maxy = data[ind];
        if(data[ind] < miny) miny = data[ind];
     }
     p[0].x = vx;
     p[0].y = yp - miny * vScale;
     p[1].x = vx;
     p[1].y = yp - maxy * vScale;
   } else { // make ybars if data points are more than vw 
   // TODO: optimize it!!
/*
     n = (int)vw-1;
     p = new Dpoint_t[2*n+4]; // 2*n should be enough, but add 4 to be safe.
     miny = FLT_MAX;
     maxy = -0.1*FLT_MAX;
     double xval;
     int imin,imax;
     int k = 0, j = 0;
     double yp = vy + vh - yoff - 1;
     for(int i=0; i<np; i++) {
        ind = i*step;
        if(data[ind] > maxy) {imax=i; maxy = data[ind];}
        if(data[ind] < miny) {imin=i; miny = data[ind];}
	xval = i*dx;
        if(k == (int)xval) { // dx is a double smaller than 1.0 
	   if(imax > imin) {
             p[j].x = vx + xval;
             p[j].y = yp - miny * vScale;
             p[j+1].x = vx + xval;
             p[j+1].y = yp - maxy * vScale;
	   } else {
             p[j+1].x = vx + xval;
             p[j+1].y = yp - miny * vScale;
             p[j].x = vx + xval;
             p[j].y = yp - maxy * vScale;
	   }
           j += 2;
           miny = FLT_MAX;
           maxy = -0.1*FLT_MAX;
           k++;
        }
     }
     n = j;
*/
     n = (int)vw-1;
     p = new Dpoint_t[2*n+4]; // 2*n should be enough, but add 4 to be safe.
     Dpoint_t *ptr1 = p;
     Dpoint_t *ptr2 = ptr1 + 1;
     float f = vw / (double)(np);
     double d = 0.0;
     double yp = vy + vh - yoff - 1;
     //double v = yp - (int) (vScale * (*data));
     double v = yp - (vScale * (*data));
     double v1;
     ptr1->x = ptr2->x = vx; 
     ptr1->y = ptr2->y = v;
     int i;
     int cnt=1;
     for(i=0; i<np; i++) {
	//v = yp - (int) (vScale * (*data));
	v = yp - (vScale * (*data));
	data += step;
	if(v>(ptr2->y)) ptr2->y = v; 
	if(v<(ptr1->y)) ptr1->y = v; 
	d += f;
	if((d>=1.0) && (cnt < vw)) {
	   d -= 1.0;
	   ptr1 += 2; ptr2 = ptr1 + 1;
	   ptr1->x = ptr2->x = vx + cnt;
	   //v1 = yp - (int) (vScale * (*data));
	   v1 = yp - (vScale * (*data));
	   if(v1 > v) {
		ptr1->y = v + 1; ptr2->y = v1;
	   } else {
		if(v1 < v) {
		   ptr1->y = v1; ptr2->y = v-1;
		} else {
		   ptr1->y = v1; ptr2->y = v1;
		}
	   }
	   cnt++;
	}
     }
     n = 2*cnt;	 
   }

   GraphicsWin::drawPolyline(p, n, color);

   delete[] p;
}


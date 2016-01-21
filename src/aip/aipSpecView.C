/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <limits.h>
#include <float.h>
#include "aipSpecView.h"
#include "aipSpecData.h"

extern "C" {
#include "iplan.h"
}

// index = (iv3-1)*nv2*nv + (iv2-1)*nv + (iv-1)
// the are npts+1 points for the polygon. the polygon[npts] is the same as polygon[0].
SpecView::SpecView(graphInfo_t grafInfo) {
   gInfo.index = grafInfo.index;
   gInfo.npts = grafInfo.npts;
   for(int i=0; i<gInfo.npts+1 && i<12; i++) {
	gInfo.polygon[i].x=grafInfo.polygon[i].x;
	gInfo.polygon[i].y=grafInfo.polygon[i].y;
   }
   index = getIndex(gInfo);
   setViewFrame();
   selected = false;
   show=true;
}

SpecView::~SpecView() {
}

// index starts from 1 for display purpose
int SpecView::getIndex(graphInfo_t g) {
   return g.index + 1;
}

// make view frame only if gInfo is rectanglar.
// test all 4 points have the same distance to the center.
void SpecView::setViewFrame() {
   voxx = voxy = voxw = voxh = 0.0;
   if(gInfo.npts != 4) return;
     
   Dpoint_t center;
   center.x=0.0;
   center.y=0.0;
   for(int i=0; i<4; i++) {
      center.x += gInfo.polygon[i].x;
      center.y += gInfo.polygon[i].y; 
   }
   center.x /= 4;
   center.y /= 4;

// uncomment this if only allow straight angle rectangle
/* 
   double x, y;
   double d1, d2;
   x = gInfo.polygon[0].x - center.x;
   y = gInfo.polygon[0].y - center.y;
   d1 = (x*x + y*y);

   for(int i=1; i<4; i++) {
     x = gInfo.polygon[i].x - center.x;
     y = gInfo.polygon[i].y - center.y;
     d2 = (x*x + y*y);
     if((int)d1 != (int)d2) return;
   }
*/

   Dpoint_t p1, p2, p3;
   p1.x=p1.y=(double)INT_MAX;
   p2.x=p2.y=0.0;
   for(int i=0; i<4; i++) {
      p3.x=gInfo.polygon[i].x;
      p3.y=gInfo.polygon[i].y;
      if(p3.x <= p1.x) p1.x = p3.x;
      if(p3.y <= p1.y) p1.y = p3.y;
      if(p3.x >= p2.x) p2.x = p3.x;
      if(p3.y >= p2.y) p2.y = p3.y;
   }
   voxx = p1.x; 
   voxy = p1.y; 
   voxw = (p2.x - p1.x); 
   voxh = (p2.y - p1.y); 
//Winfoprintf("make view %d %f %f %f %f",index, voxx,voxy,voxw,voxh);
}

void SpecView::drawGrid(int fx, int fy, int fw, int fh, int color) {
   if(!canDrawGrid(fx,fy,fw,fh)) return;
     GraphicsWin::drawPolyline(gInfo.polygon, gInfo.npts+1, color);    
}

void SpecView::drawNumber(int fx, int fy, int fw, int fh, int color) {
   if(!canDrawGrid(fx,fy,fw,fh)) return;
    char str[20];
    int ascent;
    int descent;
    int cwd;
    int cht;

    sprintf(str,"%d",index);
    // Get width and height of a character (cwd, cht)
    GraphicsWin::getTextExtents(str, 12, &ascent, &descent, &cwd);
    cht = ascent + descent;
    if(voxh < cht+1 || voxw < cwd+1) return;

    // upper left
    //GraphicsWin::drawString(str, voxx+1, voxy+cht+1, color, 0,0,0);
    // upper right
    GraphicsWin::drawString(str, voxx+voxw-cwd-1, voxy+cht+1, color, 0,0,0);
}

// if part or whole vox is inside frame 
bool SpecView::canDrawGrid(int fx, int fy, int fw, int fh) {
   if(!show || gInfo.npts < 1) return false;
   int x, y;
   int nxmax=0,nxmin=0,nymax=0,nymin=0;
   int xmin=fx, xmax=fx+fw, ymin=fy, ymax=fy+fh;
   // return true if one of polygon points is in the frame
   for(int i=0; i<gInfo.npts; i++) {
     x = (int)gInfo.polygon[i].x;
     y = (int)gInfo.polygon[i].y;
     if(x >= xmin && y >= ymin && x <= xmax && y < ymax) return true;
     if(x < xmin) nxmin++; 
     if(x > xmax) nxmax++;
     if(y < ymin) nymin++;
     if(y > ymax) nymax++;
   } 

   // nxmin is number of points whose x is smaller than xmin 
   // etc..
   if(nxmin == gInfo.npts || nxmax == gInfo.npts ||
	nymin == gInfo.npts || nymax == gInfo.npts) return false;
   else return true;
}

// part or whole vox is inside frame and is rectanglar 
bool SpecView::canDrawSpec(int fx, int fy, int fw, int fh) {
   if(!show || voxw == 0 || voxh == 0) return false;
   return canDrawGrid(fx,fy,fw,fh); 
}

// draw spec only if rectanglar frame is defined.
void SpecView::drawSpec(float *data, int npts, int step, double vscale, double yoff, int fx, int fy, int fw, int fh, int color) {
   if(!data || npts < 2) return;
   if(!canDrawSpec(fx,fy,fw,fh)) return;

   drawPolyline(data, npts, step, voxx, voxy, voxw, voxh, color, false, vscale, yoff);
}

// similar to ybars: decimate/convert array data to pixels in voxel (vx,vy,vw,vh).
// then call GraphicsWin::drawPolyline.
void SpecView::drawPolyline(float *data, int npts, int step, double vx, double vy, double vw, double vh, int color, bool autoVS, double vScale, double yoff) {
   if(step < 1) step = 1;
   if(!data || npts < 2*step) return;

   float miny, maxy;
   double dy;
   if(!autoVS && vScale > 0 ) {
     dy = (double)vh * vScale;
   } else {
     SpecData::getDataMin(data, npts, &miny);
     SpecData::getDataMax(data, npts, &maxy);

     // y scale (scale data magnitude to vox height vh)
     if(maxy > miny) dy = (double)vh / (double)(maxy-miny);  
     else dy = 1.0;
   }

   int n, ind, np = npts/step;
   double dx = (double)(vw) / (double)(np-1);  
   Dpoint_t *p;
   if(np <= vw) { // data points fewer than vox width vx
     n = np;
     p = new Dpoint_t[np];
     for(int i=0; i<np; i++) {
 	ind = i*step;
	p[i].x= vx + i*dx;
	p[i].y= vy + vh - dy * data[ind];
     }
   } else if(vw < 2.0) { // make ybars if data points are more than vx 
     n=2; 
     p = new Dpoint_t[2*n+4]; // 2*n should be enough, but add 4 to be safe.
     miny = FLT_MAX;
     maxy = -0.1*FLT_MAX;
     int yp = (int)(vy + ((1.0 - yoff)*(float)vh)) - 1;
     for(int i=0; i<np; i++) {
 	ind = i*step;
        if(data[ind] > maxy) maxy = data[ind];  
        if(data[ind] < miny) miny = data[ind];  
     }
     p[0].x = vx;
     p[0].y = yp - miny * dy;
     p[1].x = vx;
     p[1].y = yp - maxy * dy;
   } else { // make ybars if data points are more than vx 
     n = (int)vw-1; 
     p = new Dpoint_t[2*n+4]; // 2*n should be enough, but add 4 to be safe.
     miny = FLT_MAX;
     maxy = -0.1*FLT_MAX;
     int k = 0, j = 0;
     int yp = (int)(vy + ((1.0 - yoff)*(float)vh)) - 1;
     for(int i=0; i<np; i++) {
 	ind = i*step;
        if(data[ind] > maxy) maxy = data[ind];  
        if(data[ind] < miny) miny = data[ind];  
        if(k == (int)(i * dx)) { // dx is a double smaller than 1.0 
	   if(miny == maxy) {
	     p[j].x = vx + k;
	     p[j].y = yp - miny * dy;
	     j++;
	   } else {
	     p[j].x = vx + k;
	     p[j].y = yp - miny * dy;
	     p[j+1].x = vx + k;
	     p[j+1].y = yp - maxy * dy;
	     j += 2;
	   }
           miny = FLT_MAX;
           maxy = -0.1*FLT_MAX;
	   k++;
	}
     }
     n = j;
   }

   GraphicsWin::drawPolyline(p, n, color);    

   delete[] p;
}

// call iplan function
bool SpecView::isSelected(int x, int y) {
    if(gInfo.npts < 1) return false;
    float2 p, poly[12];
    p[0]=x;
    p[1]=y;
    for(int i=0; i<gInfo.npts; i++) {
       poly[i][0]=gInfo.polygon[i].x;
       poly[i][1]=gInfo.polygon[i].y;
    }
    float r= containedInConvexPolygon(p, poly,gInfo.npts);
    if(fabs(r-2*pi) < 1.0e-5) return true;
    else return false;
}


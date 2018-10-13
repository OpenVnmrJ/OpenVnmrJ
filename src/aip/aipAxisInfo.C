/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include <algorithm>
using std::swap;
#include <cmath>
using std::sqrt;
using std::fabs;
using namespace std;

#include "graphics.h"
#include "aipVnmrFuncs.h"
#include "aipStderr.h"
#include "sharedPtr.h"
#include "aipAxisInfo.h"
#include "aipGraphicsWin.h"
#include "aipImgInfo.h"
#include "aipViewInfo.h"
#include "aipGframeManager.h"
#include "aipDataManager.h"
#include "aipInterface.h"
#include "aipMouse.h"
#include "group.h"

int AxisInfo::isDisplayed = 0;
int AxisInfo::axisColor = SCALE_COLOR;
int AxisInfo::labelColor = CURSOR_COLOR;
int AxisInfo::tickSize = 5;
int AxisInfo::minPixSize = 100;
int AxisInfo::minPixPerCm = 10;
string AxisInfo::units = "cm";

// the following maybe set by the user
// horizSize, vertSize are fraction of wd and ht
double AxisInfo::horizSize = 1.0;
double AxisInfo::vertSize = 1.0;
// westX, westY etc... are offset of the four possible axes
double AxisInfo::westX = 0.0; 
double AxisInfo::westY = 0.0;
double AxisInfo::eastX = 0.0;
double AxisInfo::eastY = 0.0;
double AxisInfo::southX = 0.0;
double AxisInfo::southY = 0.0;
double AxisInfo::northX = 0.0;
double AxisInfo::northY = 0.0;
int AxisInfo::axisSize = 46;
// true if ticks are inward
bool AxisInfo::inflg = true; 
bool AxisInfo::crosshair = true; 
bool AxisInfo::magnetFrame = false; 
bool AxisInfo::showAxis = false;
int AxisInfo::pixstx=0;
int AxisInfo::pixsty=0;
int AxisInfo::pixwd=0;
int AxisInfo::pixht=0;


bool AxisInfo::canShowAxis(spGframe_t gf) {
   if(gf == nullFrame) return false;

   // check size
   int minSize = minPixSize;
   if(pixwd < minSize || pixht < minSize) return false;
   minSize = minPixPerCm;
   if(gf->pixelsPerCm < minSize) return false;

   return true;
}

bool AxisInfo::canShowLabel(spGframe_t gf, int x, int y) {
   if(gf == nullFrame) return false;

   // check size (use half of size limit)
   int minSize = minPixSize/2;
   if(pixwd < minSize || pixht < minSize) return false;
   minSize = minPixPerCm/2;
   if(gf->pixelsPerCm < minSize) return false;

   // check whether is too close to border
   // cursor need to be at least 10 pix away from the border
   // this will allow cursor label be erased when mouse exits the frame.
   int xmin = pixstx + 1;
   int xmax = pixstx + pixwd - 1;
   int ymin = pixsty + 1;
   int ymax = pixsty + pixht - 1;
   if(x < xmin || x > xmax || y < ymin || y > ymax) return false;
 
   return true;
}

int AxisInfo::getPositionString(spGframe_t gf, int x, int y, char *str) {
   if(getReal("aipShowPosition",1) < 1 && getReal("aipShowROIPos",0) < 1) return 0;
   if(!canShowLabel(gf, x, y)) return 0;

   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return 0;

   units = getString("aipUnits","cm");

    if(units == "pixel") {
       sprintf(str,"(%d,%d)",x,y);
    } else if(units == "pix") {
       double dx,dy;
       //pixToData(view, (double)x, (double)y, dx,dy);
       view->pixToData((double)x, (double)y, dx,dy);
       sprintf(str,"(%d,%d)",(int)dx,(int)dy);
    } else {
       double u[3];
/*
       pixToLogical(view,(double)x, (double)y, 0.0,u[0],u[1],u[2]);
       if(units == "mm") sprintf(str,"(%.1f,%.1f)",u[0]*10,u[1]*10);
       else sprintf(str,"(%.2f,%.2f)",u[0],u[1]);
*/
       if(magnetFrame) {
         view->pixToMagnet((double)x, (double)y, 0.0,u[0],u[1],u[2]);
         if(units == "mm") sprintf(str,"(%.1f,%.1f,%.1f)",u[0]*10,u[1]*10,u[2]*10);
         else sprintf(str,"(%.2f,%.2f,%.2f)",u[0],u[1],u[2]);
       } else {
         view->pixToUser((double)x, (double)y, 0.0,u[0],u[1],u[2]);
         if(units == "mm") sprintf(str,"(%.1f,%.1f)",u[0]*10,u[1]*10);
         else sprintf(str,"(%.2f,%.2f)",u[0],u[1]);
       }
    } 
    return 1;
}

void AxisInfo::showIntensity(spGframe_t gf, int x, int y, bool updateSlaves, double mag) {

   spViewInfo_t view = gf->getSelView();
   if(view == nullView) return;

   int i,j;
   if(mag == 0.0) gf->getPositionInfo(view,x,y,i,j,mag);

   char str[64];
   sprintf(str,"%d",(int)mag);

   bool showLabel = true;
   int xmin = gf->pixstx;
   int xmax = gf->pixstx + gf->pixwd - 3;
   int ymin = gf->pixsty;
   int ymax = gf->pixsty + gf->pixht - 3;

   if(x < xmin || x > xmax || y < ymin || y > ymax) return;

/*
   if(crosshair && getReal("aipMode",1) == 3 && updateSlaves) {
     GraphicsWin::drawLine(x,ymin,x,ymax, labelColor);
     GraphicsWin::drawLine(xmin,y,xmax,y, labelColor);
   } else {
     GraphicsWin::drawLine(x-3,y-3,x+3,y+3, labelColor);
     GraphicsWin::drawLine(x+3,y-3,x-3,y+3, labelColor);
   }
*/

    // Get width and height of a character (cwd, cht)
    int ascent;
    int descent;
    int cwd;  // string width
    int cht;  // string height
    GraphicsWin::getTextExtents(str, 12, &ascent, &descent, &cwd);
    cht = ascent + descent;
    cwd = (int)(cwd*12.0/14.0); // getTextExtents calls a function that assume font size 14

    int px = x+8;
    if(px <= xmin) showLabel=false;
    if(px+cwd+1 >= xmax) px -= cwd;
    if(px <= xmin) showLabel=false;

    int py = y;
    if(py <= ymin+cht) py = y+cht;
    if(py >=ymax) showLabel=false; 
 
    if(showLabel)
    GraphicsWin::drawString(str, px, py, labelColor, 0,0,0);

    if(updateSlaves && getReal("aipRoiBind",0) > 0) {

       double ux,uy,uz;
       pixToMagnet(view,(double)x, (double)y, 0.0,ux,uy,uz);

       spGframe_t frame;
       GframeManager *gfm = GframeManager::get();
       GframeList::iterator gfi;
       for (frame=gfm->getFirstFrame(gfi); frame != nullFrame; frame
            =gfm->getNextFrame(gfi)) {
	   if(frame == gf || !gfm->isFrameDisplayed(frame)) continue;
   	   view = frame->getFirstView();
   	   if(view == nullView) return;
	      
	   double dx,dy,dz;
	   magnetToPix(view,ux,uy,uz,dx,dy,dz);
	   showIntensity(frame,(int)dx, (int)dy, false);
       }

    }
}

void AxisInfo::showPosition(spGframe_t gf, int x, int y, bool showLabel, bool updateSlaves) {

   initAxis(gf);

   char str[MAXSTR];
   if(!getPositionString(gf, x, y, str) ) return;
   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return;
   int xmin = gf->pixstx;
   int xmax = gf->pixstx + gf->pixwd - 3;
   int ymin = gf->pixsty;
   int ymax = gf->pixsty + gf->pixht - 3;

   if(x < xmin || x > xmax || y < ymin || y > ymax) return;

   if(crosshair && getReal("aipMode",1) == 3 && updateSlaves) {
     GraphicsWin::drawLine(x,ymin,x,ymax, labelColor);
     GraphicsWin::drawLine(xmin,y,xmax,y, labelColor);
   } else {
     GraphicsWin::drawLine(x-3,y-3,x+3,y+3, labelColor);
     GraphicsWin::drawLine(x+3,y-3,x-3,y+3, labelColor);
   }

    // Get width and height of a character (cwd, cht)
    int ascent;
    int descent;
    int cwd;  // string width
    int cht;  // string height
    GraphicsWin::getTextExtents(str, 12, &ascent, &descent, &cwd);
    cht = ascent + descent;
    cwd = (int)(cwd*12.0/14.0); // getTextExtents calls a function that assume font size 14

    int px = x+8;
    if(px <= xmin) showLabel=false;
    if(px+cwd+1 >= xmax) px -= cwd;
    if(px <= xmin) showLabel=false;

    int py = y;
    if(py <= ymin+cht) py = y+cht;
    if(py >=ymax) showLabel=false; 
 
    if(showLabel)
    GraphicsWin::drawString(str, px, py, labelColor, 0,0,0);

    // show position only if mouse in createLine or createPoint state.
    //if((Mouse::creatingLine() || Mouse::creatingPoint()) && updateSlaves && getReal("aipRoiBind",0) > 0) {
    if(updateSlaves && getReal("aipRoiBind",0) > 0) {

       double ux,uy,uz;
       pixToMagnet(view,(double)x, (double)y, 0.0,ux,uy,uz);

       spGframe_t frame;
       GframeManager *gfm = GframeManager::get();
       GframeList::iterator gfi;
       for (frame=gfm->getFirstFrame(gfi); frame != nullFrame; frame
            =gfm->getNextFrame(gfi)) {
	   if(frame == gf || !gfm->isFrameDisplayed(frame)) continue;
   	   view = frame->getFirstView();
   	   if(view == nullView) return;
	      
	   double dx,dy,dz;
	   magnetToPix(view,ux,uy,uz,dx,dy,dz);
	   showPosition(frame,(int)dx, (int)dy, false, false);
       }

    }
}

int AxisInfo::getDistanceString(spGframe_t gf, int x1, int y1, int x2, int y2, char *str, double dis) {

   if(getReal("aipShowPosition",1) < 1 && getReal("aipShowROIPos",0) < 1) return 0;
   if(!canShowLabel(gf, x1, y1)) return 0;
   if(!canShowLabel(gf, x2, y2)) return 0;

   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return 0;

   units = getString("aipUnits","cm");

    if(units == "pixel") {
       dis = sqrt((double)((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)));
       sprintf(str,"%d",(int)dis);
    } else if(units == "pix") {
       double dx1,dy1,dx2,dy2;
       if(dis<=0.0) {
         pixToData(view, (double)x1, (double)y1, dx1,dy1);
         pixToData(view, (double)x2, (double)y2, dx2,dy2);
         dis = sqrt((double)((dx1-dx2)*(dx1-dx2)+(dy1-dy2)*(dy1-dy2)));
       }
       sprintf(str,"%d",(int)dis);
    } else {
       double ux1,uy1,uz1,ux2,uy2,uz2;
       if(dis<=0.0) {
         pixToUser(view,(double)x1, (double)y1, 0.0,ux1,uy1,uz1);
         pixToUser(view,(double)x2, (double)y2, 0.0,ux2,uy2,uz2);
         dis = sqrt((ux1-ux2)*(ux1-ux2)+(uy1-uy2)*(uy1-uy2));
       }
       if(units == "mm") {
         sprintf(str,"%.1f",dis*10);
       } else {
         sprintf(str,"%.2f",dis);
       }
    } 

    return 1;
}

void AxisInfo::showDistance(spGframe_t gf, int x1, int y1, int x2, int y2, double dis) {

    initAxis(gf);

    char str[MAXSTR];
    if(getDistanceString(gf,x1,y1,x2,y2,str,dis) == 0) return;

    int ascent;
    int descent;
    int cwd;
    int cht;

    // Get width and height of a character (cwd, cht)
    GraphicsWin::getTextExtents(str, 12, &ascent, &descent, &cwd);
    cht = ascent + descent;
    cwd = (int)(cwd*12.0/14.0);

    int xmin = gf->pixstx;
    int xmax = gf->pixstx + gf->pixwd;
    int ymin = gf->pixsty;
    int ymax = gf->pixsty + gf->pixht;

    int px = (int)(0.5*(x1+x2));
    if(px <= xmin) return;
    if(px+cwd+1 >= xmax) px -= cwd;
    if(px <= xmin) return;

    int py = (int)(0.5*(y1+y2));
    if(py >=ymax || py-cht <= ymin) return;

    GraphicsWin::drawString(str, px, py, labelColor, 0,0,0);
}

// called first 
void AxisInfo::initAxis(spGframe_t gf) {

   showAxis = false;
   if(getReal("aipAxis",INDEX_SHOW,0) > 0) showAxis = true;

   isDisplayed = 0;
   if(getReal("aipAxis",INDEX_WEST,0) > 0) isDisplayed |= AX_WEST;
   if(getReal("aipAxis",INDEX_EAST,0) > 0) isDisplayed |= AX_EAST;
   if(getReal("aipAxis",INDEX_SOUTH,0) > 0) isDisplayed |= AX_SOUTH;
   if(getReal("aipAxis",INDEX_NORTH,0) > 0) isDisplayed |= AX_NORTH;

   if(getString("aipAxisRef","logical") == "magnet") magnetFrame=true;
   else magnetFrame= false;

   // may set inflg here by a parameter
   if(getReal("aipAxis",INDEX_TICK,0) > 0) inflg = false; 
   else inflg = true; 

   int fov = false;
   // use image size or frame size
   if(getReal("aipAxis",INDEX_FOV,1) > 0) fov = true; 

   if(getReal("aipAxis",INDEX_CROSS,1) > 0 && getReal("aipAxis",INDEX_SHOW,0) > 0 && 
      isDisplayed == (AX_WEST | AX_EAST | AX_SOUTH | AX_NORTH) ) crosshair=true;
   else crosshair=false;

   spViewInfo_t view = gf->getFirstView();
   if(fov && view != nullView) {
      pixstx = view->pixstx; 
      pixsty = view->pixsty; 
      pixwd = view->pixwd; 
      pixht = view->pixht; 
   } else {
      pixstx = gf->pixstx+2; 
      pixsty = gf->pixsty+2; 
      pixwd = gf->pixwd-2; 
      pixht = gf->pixht-2; 
   }
}

void AxisInfo::displayAxis(spGframe_t gf) {

   initAxis(gf);

   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return;

   if(magnetFrame && view->isOblique()) return;

   // draw center lines
   drawCenterLines(gf);

   // draw axes
   if(!showAxis) return;
   if(!canShowAxis(gf)) return;

   if(isDisplayed == 0) return;

   // set offset and size
   horizSize = getReal("aipAxisGeom",INDEX_WIDTH,0.6);
   vertSize = getReal("aipAxisGeom",INDEX_HEIGHT,0.6);
   westX = getReal("aipAxisGeom",INDEX_WX,0.0);
   westY = getReal("aipAxisGeom",INDEX_WY,0.0);
   eastX = getReal("aipAxisGeom",INDEX_EX,0.0);
   eastY = getReal("aipAxisGeom",INDEX_EY,0.0);
   southX = getReal("aipAxisGeom",INDEX_SX,0.0);
   southY = getReal("aipAxisGeom",INDEX_SY,0.0);
   northX = getReal("aipAxisGeom",INDEX_NX,0.0);
   northY = getReal("aipAxisGeom",INDEX_NY,0.0);

   if(isDisplayed & AX_WEST) displayAxis_west(gf, inflg);
   if(isDisplayed & AX_EAST) displayAxis_east(gf, inflg);
   if(isDisplayed & AX_SOUTH) displayAxis_south(gf, inflg);
   if(isDisplayed & AX_NORTH) displayAxis_north(gf, inflg);
}

// westX, westY, etc... are offset of the axis (in % of wd or ht)
// default is zero
void AxisInfo::displayAxis_west(spGframe_t gf, bool inflg) {
   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return;

   int xoff;
   if(inflg) xoff = (int)(pixstx + pixwd * westX); 
   else xoff = (int)(pixstx + axisSize + pixwd * westX); 
   int yoff = (int)(pixsty + 0.5*pixht + pixht * westY);
   int minpixy = (int)(yoff - pixht * vertSize/2); 
   int maxpixy = (int)(yoff + pixht * vertSize/2 - 3); 

   if(!inflg && vertSize == 1.0 && (isDisplayed & AX_NORTH)) minpixy += axisSize;
   if(!inflg && vertSize == 1.0 && (isDisplayed & AX_SOUTH)) maxpixy -= axisSize;

   int tickpix;
   if(inflg) tickpix = 10; 
   else tickpix = -10;
   drawVertAxis(gf, xoff, yoff, minpixy, maxpixy, tickpix); 
}

void AxisInfo::displayAxis_east(spGframe_t gf, bool inflg) {
   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return;

   int xoff;
   if(inflg) xoff = (int)(pixstx + pixwd + pixwd * eastX - 3); 
   else xoff = (int)(pixstx + pixwd + pixwd * eastX - axisSize);
   int yoff = (int)(pixsty + 0.5*pixht + pixht * eastY);
   int minpixy = (int)(yoff - pixht * vertSize/2); 
   int maxpixy = (int)(yoff + pixht * vertSize/2 - 3); 

   if(!inflg && vertSize == 1.0 && (isDisplayed & AX_NORTH)) minpixy += axisSize;
   if(!inflg && vertSize == 1.0 && (isDisplayed & AX_SOUTH)) maxpixy -= axisSize;

   int tickpix;
   if(inflg) tickpix = -10; 
   else tickpix = 10;
   drawVertAxis(gf, xoff, yoff, minpixy, maxpixy, tickpix); 
}

void AxisInfo::displayAxis_south(spGframe_t gf, bool inflg) {
   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return;

   int xoff = (int)(pixstx + 0.5*pixwd + pixwd * southX);
   int yoff;
   if(inflg) yoff = (int)(pixsty + pixht + pixht * southY - 3); 
   else yoff = (int)(pixsty + pixht + pixht * southY - axisSize); 
   int minpixx = (int)(xoff - pixwd * horizSize/2); 
   int maxpixx = (int)(xoff + pixwd * horizSize/2 - 3); 
   
   if(!inflg && horizSize == 1.0 && (isDisplayed & AX_WEST)) minpixx += axisSize;
   if(!inflg && horizSize == 1.0 && (isDisplayed & AX_EAST)) maxpixx -= axisSize;

   int tickpix;
   if(inflg) tickpix = -10; 
   else tickpix = 10;
   drawHorizAxis(gf, xoff, yoff, minpixx, maxpixx, tickpix); 
}

void AxisInfo::displayAxis_north(spGframe_t gf, bool inflg) {
   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return;

   int xoff = (int)(pixstx + 0.5*pixwd + pixwd * northX);
   int yoff;
   if(inflg) yoff = (int)(pixsty + pixht * northY);
   else yoff = (int)(pixsty + axisSize + pixht * northY);
   int minpixx = (int)(xoff - pixwd * horizSize/2); 
   int maxpixx = (int)(xoff + pixwd * horizSize/2 - 3); 

   if(!inflg && horizSize == 1.0 && (isDisplayed & AX_WEST)) minpixx += axisSize;
   if(!inflg && horizSize == 1.0 && (isDisplayed & AX_EAST)) maxpixx -= axisSize;

   int tickpix;
   if(inflg) tickpix = 10; 
   else tickpix = -10;
   drawHorizAxis(gf, xoff, yoff, minpixx, maxpixx, tickpix); 

/*
   GraphicsWin::drawLine(minpixx,yoff,maxpixx, yoff, labelColor);
   units = getString("aipUnits","cm");
*/
}

// determine span of big ticks (bigTickMag)
// determine # of small ticks (nticks = # of small ticks + 1);
// determine step size of small ticks (inc)
// determine firstPoint by adjusting dmin according to tick span 
// return values bigTickMag and firstPoint are in data space. 
// The caller is responsible to convert data point to pixel, vice versa.
// formula to determine whether a tick at data value dValue is a big tick or not: 
// if((int)(fabs(dValue*10.0)/bigTickMag+0.5) % nticks == 0) { 
//	// big tick
// } else {
//	// small tick
// }
// see drawVertAxis method for example
int AxisInfo::calcAxis(double dmin, double dmax, int pmin, int pmax,             
        double &firstPoint, double &inc, double &bigTickMag, int &nticks) {

      double dSpan = dmax-dmin;  // data span
      if(dSpan == 0) return 0;

      double pixPerPoint = (pmax-pmin)/dSpan; 
      if(pixPerPoint <= 0) return 0;

      int maxTicks = (pmax-pmin)/10; // smallest space between ticks is 10 pixels

      // determine order of magnitude
      double mag = pow(10.0,(int)(log10(dSpan)));
      if(mag == 0) return 0;

      // determine magnitude  of big tick (can only be 0.1, 0.2. 0.5 of magnitude order)
      dSpan /= mag; // now dSpan is # of big ticks
      if(dSpan < 2) bigTickMag = mag/10;
      else if(dSpan < 4) bigTickMag = mag/5;
      else if(dSpan < 6) bigTickMag = mag/2;
      else bigTickMag = mag;
      
      // determine nticks, inc and firstPoint 
      // a big tick can only be divided by 10, 5, or 2 
      if(maxTicks > 100) { // divide tick into 10 small ticks 
        nticks = 10;
	inc = bigTickMag/10.0;
        // round up dmin so dmin*bigTickMag is an integer.
        firstPoint = (double)((int)(dmin*10/bigTickMag) - ((int)(10*dmin/bigTickMag) % 2))*bigTickMag/10.0;

      } else if(maxTicks > 20) { // divide tick into 5 small ticks
        nticks = 5;
	inc = bigTickMag/5.0;
        // round up dmin so dmin*bigTickMag can be divided by 2
        firstPoint = (double)((int)(dmin*10/bigTickMag) - ((int)(10*dmin/bigTickMag) % 2))*bigTickMag/10.0;

      } else { // divide tick into 2 ticks
        bigTickMag *= 2.0;
        nticks = 5;
	inc = bigTickMag/5.0;
        // round up dmin so dmin*bigTickMag is an integer.
        firstPoint = (double)((int)(dmin*10/bigTickMag) - ((int)(10*dmin/bigTickMag) % 2))*bigTickMag/10.0;
      }

      return 1;
}

void AxisInfo::drawVertAxis(spGframe_t gf, int xoff, int yoff, int minpixy, int maxpixy, int tickpix) {
   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return;

   int unitPos = maxpixy + 16;

   int miny = minpixy;
   int maxy = maxpixy;
   int pmin = minpixy+abs(tickpix)+10;
   int pmax = maxpixy-abs(tickpix)-10;
   int horizAxis = (isDisplayed & AX_SOUTH) || (isDisplayed & AX_NORTH);

   int tickpix2 = tickpix/2; // small tick
   int direction;
   if(tickpix > 0) direction = AX_EAST;
   else direction = AX_WEST;

   units = getString("aipUnits","cm");

   char str[MAXSTR];
   if(units == "pixel") {
      int smallTickSpan = 10; // 10 pixels
      int bigTickSpan = 50; 

      minpixy = minpixy - pixsty;
      maxpixy = maxpixy - pixsty;

      // adjust minpixy (first point) to start from a tick. 
      minpixy = ((int)(minpixy/smallTickSpan + 0.5))*smallTickSpan;

      // draw ticks, start from minpixy 
      int y = minpixy;
      while(y <= maxpixy) {
	if((y % bigTickSpan) == 0) { //big tick
           GraphicsWin::drawLine(xoff+tickpix,y+pixsty,xoff,y+pixsty, axisColor);
           if(!horizAxis || !inflg || (y>pmin && y<pmax)) {
	     sprintf(str,"%d", y);
	     drawString(gf, str, xoff+tickpix, y+pixsty, axisColor, direction); 
	   }
	} else { // small tick
           GraphicsWin::drawLine(xoff+tickpix2,y+pixsty,xoff,y+pixsty, axisColor);
	}
	y += smallTickSpan;
      }
      maxpixy = y - smallTickSpan;

      minpixy += pixsty;
      maxpixy += pixsty;

      // draw units
      if(inflg) unitPos = maxpixy + 16;
      drawString(gf, "[pixel]", xoff+2, unitPos, axisColor, direction); 

   } else if(units == "pix") {

      double dx,dy,px,py,dymin,dymax;
      px = (double)xoff; // px does not change

      pixToData(view, px, (double)minpixy, dx,dymin);
      pixToData(view, px, (double)maxpixy, dx,dymax);
      
      bool invertY=false;
      if(dymin > dymax) {
        invertY=true;
	double d=dymin;
	dymin=dymax;
	dymax=d;
      }

      double first, inc, mag;
      int nticks;
      if(!calcAxis(dymin, dymax, minpixy, maxpixy, first, inc, mag, nticks))
	return;

      // draw ticks, start from first point
      if(!inflg) dy = first + inc;
      else dy = first;
      dataToPix(view,dx,dy,px,py);
      if(invertY) {
        maxpixy = (int)py;
      } else {
        minpixy = (int)py;
      }
      while(dy <= dymax) {
        if((int)(10.0*fabs(dy)/mag + 0.5) % nticks == 0) { // big tick
           GraphicsWin::drawLine(xoff+tickpix,(int)py,xoff,(int)py, axisColor);
           if(!horizAxis || !inflg || (py>pmin && py<pmax)) {
	     if(mag < 1) sprintf(str,"%.1f",dy);
	     else sprintf(str,"%d",(int)dy);
	     drawString(gf, str, xoff+tickpix, (int)py, axisColor, direction); 
	   }
	} else if(inc>=1) {
           GraphicsWin::drawLine(xoff+tickpix2,(int)py,xoff,(int)py, axisColor);
        }
	dy += inc;
        dataToPix(view,dx,dy,px,py);
      }
      dy = dy - inc;
      dataToPix(view,dx,dy,px,py);
      if(invertY) {
        minpixy = (int)py;
      } else {
        maxpixy = (int)py;
      }

      // draw units
      if(inflg) unitPos = maxpixy + 16;
      drawString(gf, "[pix]", xoff+2, unitPos, axisColor, direction); 

   } else if(units == "cm" || units == "mm") {

      int ind = 1;

      double d[3],px,py,pz,dymin,dymax;
      px = (double)xoff; // px does not change

      pixToLogical(view,px, (double)minpixy, 0.0, d[0],d[1],d[2]);
      dymin = d[ind];
      pixToLogical(view,px, (double)maxpixy, 0.0, d[0],d[1],d[2]);
      dymax = d[ind];

      bool invertY=false;
      if(dymin > dymax) {
        invertY=true;
	double tmp=dymin;
	dymin=dymax;
	dymax=tmp;
      }

      double first, inc, mag;
      int nticks;
      if(!calcAxis(dymin, dymax, minpixy, maxpixy, first, inc, mag, nticks))
	return;

      // draw ticks, start from first point
      d[ind] = first;
      logicalToPix(view,d[0],d[1],d[2],px,py,pz);
      if(invertY) {
        maxpixy = (int)py;
      } else {
        minpixy = (int)py;
      }
      while(d[ind] <= dymax) {
        // note, use fabs for modulus operation
	if(fabs(d[ind]) < 1.0e-4) d[ind] = 0.0;
        if((int)(fabs(d[ind]*10.0)/mag+0.5) % nticks == 0) { // big tick
           GraphicsWin::drawLine(xoff+tickpix,(int)py,xoff,(int)py, axisColor);
           if(!horizAxis || !inflg || (py>pmin && py<pmax)) {
	     if(units=="cm") {
	       if(mag <= 0.01) sprintf(str,"%.2f",d[ind]);
	       else if(inc <= 0.1) sprintf(str,"%.1f",d[ind]);
	       else sprintf(str,"%.0f",d[ind]);
	     } else if(units=="mm") {
	       if(mag <= 0.01) sprintf(str,"%.1f",d[ind]*10.0);
	       else sprintf(str,"%.0f",(d[ind]*10.0));
	     }
	     drawString(gf, str, xoff+tickpix, (int)py, axisColor, direction); 
	   }
	} else {
           GraphicsWin::drawLine(xoff+tickpix2,(int)py,xoff,(int)py, axisColor);
        }
	d[ind] += inc;
        logicalToPix(view,d[0],d[1],d[2],px,py,pz);
      }
      d[ind] = d[ind] - inc;
      logicalToPix(view,d[0],d[1],d[2],px,py,pz);
      if(invertY) {
        minpixy = (int)py;
      } else {
        maxpixy = (int)py;
      }

      // draw units
      if(inflg) unitPos = maxpixy + 16;
      sprintf(str,"[%s]",units.c_str());
      drawString(gf, str, xoff+2, unitPos, axisColor, direction); 
   } 
 
   // draw axis
   if(vertSize == 1.0 && inflg) 
     GraphicsWin::drawLine(xoff,pixsty,xoff,pixsty+pixht - 3, axisColor);
   else if(vertSize == 1.0)
     GraphicsWin::drawLine(xoff,miny+1,xoff,maxy+3, axisColor);
   else
     GraphicsWin::drawLine(xoff,minpixy,xoff,maxpixy, axisColor);
}

void AxisInfo::drawString(spGframe_t gf, char *str, int x, int y, int color, int direction) {
   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return;
    int ascent;
    int descent;
    int cwd;
    int cht;

    // Get width and height of a character (cwd, cht)
    GraphicsWin::getTextExtents(str, 12, &ascent, &descent, &cwd);
    cht = ascent + descent;
    cwd = (int)(cwd*12.0/14.0);

    int xmin = pixstx;
    int xmax = pixstx + pixwd;
    int ymin = pixsty;
    int ymax = pixsty + pixht;

    if(direction & AX_WEST) {
      x -= (cwd+2);
      y += (int)(0.5*cht);
    } else if(direction & AX_SOUTH) {
      x -= (int)(0.5*cwd);
      y += (int)(0.5*cht);
    } else if(direction & AX_NORTH) {
      x -= (int)(0.5*cwd);
      y -= (int)(0.5*cht);
    } else { // east
      x += 2;
      y += (int)(0.5*cht);
    }

    if(x <= xmin || x+cwd+1 >= xmax) return;
    if(y >=ymax || y-cht <= ymin) return;

    GraphicsWin::drawString(str, x, y, color, 0,0,0);
}

void AxisInfo::drawHorizAxis(spGframe_t gf, int xoff, int yoff, int minpixx, int maxpixx, int tickpix) {
   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return;

   int tickpix2 = tickpix/2; // small tick
   int direction;
   if(tickpix > 0) direction = AX_SOUTH;
   else direction = AX_NORTH;

   int minx = minpixx;
   int maxx = maxpixx;

   int pmin = minpixx+abs(tickpix)+10;
   int pmax = maxpixx-abs(tickpix)-10;
   int vertAxis = (isDisplayed & AX_EAST) || (isDisplayed & AX_WEST);

   units = getString("aipUnits","cm");

   char str[MAXSTR];
   if(units == "pixel") {
      int smallTickSpan = 10; // 10 pixels
      int bigTickSpan = 50; 

      minpixx = minpixx - pixstx;
      maxpixx = maxpixx - pixstx;

      // adjust minpixx (first point) to start from a tick. 
      minpixx = ((int)(minpixx/smallTickSpan + 0.5))*smallTickSpan;

      // draw ticks, start from minpixx 
      int x = minpixx;
      while(x <= maxpixx) {
	if((x % bigTickSpan) == 0) { //big tick
           GraphicsWin::drawLine(x+pixstx, yoff+tickpix, x+pixstx, yoff, axisColor);
           if(!vertAxis || !inflg || (x>pmin && x<pmax)) {
	    sprintf(str,"%d", x);
	    drawString(gf, str, x+pixstx, yoff+tickpix+10, axisColor, direction); 
	   }
	} else { // small tick
           GraphicsWin::drawLine(x+pixstx, yoff, x+pixstx, yoff+tickpix2, axisColor);
	}
	x += smallTickSpan;
      }
      maxpixx = x - smallTickSpan;

      minpixx += pixstx;
      maxpixx += pixstx;

      // draw units
      //drawString(gf, "[pixel]", maxpixx+20, yoff+8, axisColor, direction); 

   } else if(units == "pix") {

      double dx,dy,px,py,dxmin,dxmax;
      py = (double)yoff; // py does not change

      pixToData(view, (double)minpixx, py, dxmin,dy);
      pixToData(view, (double)maxpixx, py, dxmax,dy);

      bool invertX = false;
      if(dxmin > dxmax) {
	invertX=true;
	double d=dxmin;
	dxmin=dxmax;
	dxmax=d;
      }

      double first, inc, mag;
      int nticks;
      if(!calcAxis(dxmin, dxmax, minpixx, maxpixx, first, inc, mag, nticks))
	return;

      // draw ticks, start from first point
      if(!inflg) dx = first + inc;
      else dx = first;
      dataToPix(view,dx,dy,px,py);
      if(invertX) {
        maxpixx = (int)px;
      } else {
        minpixx = (int)px;
      }
      while(dx <= dxmax) {
        if((int)(10.0*fabs(dx)/mag + 0.5) % nticks == 0) { // big tick
           GraphicsWin::drawLine((int)px,yoff+tickpix,(int)px,yoff, axisColor);
           if(!vertAxis || !inflg || (px>pmin && px<pmax)) {
	     if(mag < 1) sprintf(str,"%.1f",dx);
	     else sprintf(str,"%d",(int)dx);
	     drawString(gf, str, (int)px, yoff+tickpix+10, axisColor, direction); 
	   }
	} else if(inc>=1) {
           GraphicsWin::drawLine((int)px,yoff+tickpix2,(int)px,yoff, axisColor);
        }
	dx += inc;
        dataToPix(view,dx,dy,px,py);
      }
      dx = dx - inc;
      dataToPix(view,dx,dy,px,py);
      if(invertX) {
        minpixx = (int)px;
      } else {
        maxpixx = (int)px;
      }

      // draw units
      //drawString(gf, "[pix]", maxpixx+20, yoff+8, axisColor, direction); 

   } else if(units == "cm" || units == "mm") {

      int ind = 0;

      double d[3],px,py,pz,dxmin,dxmax;
      py = (double)yoff; // py does not change

      pixToLogical(view,(double)minpixx, py, 0.0, d[0],d[1],d[2]);
      dxmin=d[ind];
      pixToLogical(view,(double)maxpixx, py, 0.0, d[0],d[1],d[2]);
      dxmax=d[ind];

      bool invertX=false;
      if(dxmin > dxmax) {
        invertX=true;
	double tmp=dxmin;
	dxmin=dxmax;
	dxmax=tmp;
      }

      double first, inc, mag;
      int nticks;
      if(!calcAxis(dxmin, dxmax, minpixx, maxpixx, first, inc, mag, nticks))
	return;

      // draw ticks, start from first point
      d[ind] = first;
      logicalToPix(view,d[0],d[1],d[2],px,py,pz);
      if(invertX) {
        maxpixx = (int)px;  // first point is maxpixx 
      } else {
        minpixx = (int)px;  // first point is minpixx 
      }
      while(d[ind] <= dxmax) {
        // note, use fabs for modulus operation
	if(fabs(d[ind]) < 1.0e-4) d[ind] = 0.0;
        if((int)(fabs(d[ind]*10.0)/mag+0.5) % nticks == 0) { // big tick
           GraphicsWin::drawLine((int)px, yoff+tickpix,(int)px,yoff, axisColor);
           if(!vertAxis || !inflg || (px>pmin && px<pmax)) {
	     if(units=="cm") {
	       if(mag <= 0.01) sprintf(str,"%.2f",d[ind]);
	       else if(inc <= 0.1) sprintf(str,"%.1f",d[ind]);
	       else sprintf(str,"%.0f",d[ind]);
	     } else if(units=="mm") {
	       if(mag <= 0.01) sprintf(str,"%.1f",d[ind]*10.0);
	       else sprintf(str,"%.0f",(d[ind]*10.0));
	     }
	     drawString(gf, str, (int)px, yoff+tickpix+10, axisColor, direction); 
	   } 
	} else {
           GraphicsWin::drawLine((int)px, yoff+tickpix2,(int)px, yoff, axisColor);
        }
	d[ind] += inc;
        logicalToPix(view,d[0],d[1],d[2],px,py,pz);
      }
      d[ind] = d[ind] - inc;
      logicalToPix(view,d[0],d[1],d[2],px,py,pz);
      if(invertX) {
        minpixx = (int)px;
      } else {
        maxpixx = (int)px;
      }

      // draw units
/*
      sprintf(str,"[%s]",units.c_str());
      drawString(gf, str, maxpixx+20, yoff+8, axisColor, direction); 
*/
   } 
 
   // draw axis
   if(horizSize == 1.0 && inflg) 
     GraphicsWin::drawLine(pixstx, yoff,pixstx+pixwd - 3, yoff, axisColor);
   else if(vertSize == 1.0)
     GraphicsWin::drawLine(minx+1,yoff,maxx+3, yoff, axisColor);
   else
     GraphicsWin::drawLine(minpixx,yoff,maxpixx, yoff, axisColor);
}

void AxisInfo::drawCenterLines(spGframe_t gf) {
   int flag = getReal("aipShowCenter",0);
   if(flag <= 0) return;
 
   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return;

   int color = axisColor;
   double px,py,pz;
   double ux=0.0,uy=0.0,uz=0.0;
   if(flag > 1) { 
     spImgInfo_t img = view->imgInfo;
     if(img != nullImg) {
       ux -= img->getDataInfo()->getLocation(0);
       uy += img->getDataInfo()->getLocation(1);
       uz += img->getDataInfo()->getLocation(2);
     }
     color = YELLOW; 
   }
   view->userToPix(ux,uy,uz,px,py,pz);

   GraphicsWin::drawLine((int)px, pixsty,(int)px, pixsty + pixht - 3, color);
   GraphicsWin::drawLine(pixstx,(int)py, pixstx + pixwd - 3, (int)py, color);
}

void AxisInfo::pixToLogical(spViewInfo_t view, double px, double py, double pz, double &mx, double &my, double &mz) {

   if(!magnetFrame) pixToUser(view,px,py,pz,mx,my,mz);
   else {
     double m[3];
     view->pixToMagnet(px,py,pz,m[0],m[1],m[2]);
     mx=m[mapIndex(view,0)];
     my=m[mapIndex(view,1)];
     mz=m[mapIndex(view,2)];
   }
}

void AxisInfo::logicalToPix(spViewInfo_t view, double mx, double my, double mz, double &px, double &py, double &pz) {

   if(!magnetFrame) userToPix(view,mx,my,mz,px,py,pz); 
   else {
     double m[3];
     m[mapIndex(view,0)]=mx;
     m[mapIndex(view,1)]=my;
     m[mapIndex(view,2)]=mz;
     view->magnetToPix(m[0],m[1],m[2],px,py,pz);
   }
}

// Note, pixToMagnet calls pixToUser if aipAxisRef<>'magnet'
void AxisInfo::pixToMagnet(spViewInfo_t view, double px, double py, double pz, double &mx, double &my, double &mz) {
   if(view == nullView) return;
   view->pixToMagnet(px,py,pz,mx,my,mz);
}

// Note, magnetToPix calls userToPix if aipAxisRef<>'magnet'
void AxisInfo::magnetToPix(spViewInfo_t view, double mx, double my, double mz, double &px, double &py, double &pz) {
   if(view == nullView) return;
   view->magnetToPix(mx,my,mz,px,py,pz);
}

void AxisInfo::pixToUser(spViewInfo_t view, double px, double py, double pz, double &ux, double &uy, double &uz) {
   if(view == nullView) return;

   if (view->getRotation() & 4) { // x, y swapped and x inverted 
      view->pixToUser(px,py,pz,uy,ux,uz);
//      ux=-ux;
   } else {
      view->pixToUser(px,py,pz,ux,uy,uz);
   }
/*
   float p[3];
   p[0] = (float)px;
   p[1] = (float)py;
   p[2] = (float)pz;
   ptou(gf->id, p);
   ux = (double)p[0];
   uy = (double)p[1];
   uz = (double)p[2];
*/
}

void AxisInfo::userToPix(spViewInfo_t view, double ux, double uy, double uz, double &px, double &py, double &pz) {
   if(view == nullView) return;

   if (view->getRotation() & 4) { // x, y swapped and x inverted 
//      ux=-ux;
      view->userToPix(uy,ux,uz,px,py,pz);
   } else {
      view->userToPix(ux,uy,uz,px,py,pz);
   }
/*
   float p[3];
   p[0] = (float)ux;
   p[1] = (float)uy;
   p[2] = (float)uz;
   utop(gf->id, p);
   px = (double)p[0];
   py = (double)p[1];
   pz = (double)p[2];
*/
}

void AxisInfo::pixToData(spViewInfo_t view, double px, double py, double &dx, double &dy) {
   if(view == nullView) return;

   if (view->getRotation() & 4) { 
      view->pixToData(px,py,dy,dx);
   } else {
      view->pixToData(px,py,dx,dy);
   }
}

void AxisInfo::dataToPix(spViewInfo_t view, double dx, double dy, double &px, double &py) {
   if(view == nullView) return;

   if (view->getRotation() & 4) { 
      view->dataToPix(dy,dx,px,py);
   } else {
      view->dataToPix(dx,dy,px,py);
   }
}

int AxisInfo::mapIndex(spViewInfo_t view, int i) {

   if(i<0 || i>2) return i;
   if(view == nullView) return i;

   double d=0.0;
   int ind=i;
   for(int j=0;j<3;j++) {
     if(fabs(view->p2m[j][i]) > d) {
	d=fabs(view->p2m[j][i]);
	ind = j;
     }
   }
   return ind;
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include "AspRegion.h"
#include "AspUtil.h"

spAspRegion_t nullAspRegion = spAspRegion_t(NULL);

// this is related to toString
// str: 
AspRegion::AspRegion(char words[MAXWORDNUM][MAXSTR], int nw) {
   init();
   npts=1;
   if(sCoord) delete[] sCoord;
   if(pCoord) delete[] pCoord;
   sCoord = new Dpoint_t[npts];
   pCoord = new Dpoint_t[npts];

   if(nw < 3) return;
   index=atoi(words[0]) - 1;
   sCoord[0].x=atof(words[1]);
   sCoord[0].y=atof(words[2]);
   pCoord[0].x=pCoord[0].y==0;
   if(nw>3) dataID=string(words[3]);
   if(nw>4) color=atoi(words[4]); 
}

// x1 is left
AspRegion::AspRegion(int ind, double xval, double yval) {
    init();
    npts=1;
    if(sCoord) delete[] sCoord;
    if(pCoord) delete[] pCoord;
    sCoord = new Dpoint_t[npts];
    pCoord = new Dpoint_t[npts];

    sCoord[0].x=xval;
    sCoord[0].y=yval;
    pCoord[0].x=pCoord[0].y=0;
    index = ind;
}

void AspRegion::init() {
    AspAnno::init();
    created_type = ANNO_BASEPOINT;
    npts=1;
    dataID="?";
}

AspRegion::~AspRegion() {
}

string AspRegion::toString() {
   char str[MAXSTR];
   sprintf(str,"%d %f %f %s %d",
        index+1,sCoord[0].x,sCoord[0].y,dataID.c_str(),color);
   return string(str);
}

void AspRegion::display(spAspCell_t cell, spAspTrace_t trace, spAspDataInfo_t dataInfo) {
	set_transparency_level(transparency);
	int roiColor,thick;
	setRoiColor(roiColor,thick);
/* 
// this replace sCoord[0].y with trace data 
        float *traceData = trace->getFirstDataPtr();
        if(!traceData) return;

	int maxpts = trace->getTotalpts();
	if(maxpts<1) return;

	int p1 = trace->val2dpt(sCoord[0].x);
	sCoord[0].y = (*(traceData+p1));
*/
	double cali = cell->getCali(VERT);
  	double yoff = (trace->vp)*cali*(dataInfo->getVoff());
	pCoord[0].x=cell->val2pix(HORIZ,sCoord[0].x);
	pCoord[0].y=cell->val2pix(VERT,sCoord[0].y) - yoff;
//Winfoprintf("region display %d %d %f %f %f %f",p1,maxpts,sCoord[0].x,sCoord[0].y,pCoord[0].x,pCoord[0].y);
	if(selected == HANDLE_SELECTED)
	   AspUtil::drawMark((int)pCoord[0].x,(int)pCoord[0].y,ACTIVE_COLOR,thick);
	else
	   AspUtil::drawMark((int)pCoord[0].x,(int)pCoord[0].y,roiColor,thick);
	return;

	set_spectrum_width(1);
        set_line_width(1);
	set_transparency_level(0);
}

int AspRegion::select(spAspCell_t cell, int x, int y) {
   selected = selectHandle(x,y);
   return selected;
}

void AspRegion::modify(spAspCell_t cell, int x, int y, int prevX, int prevY) {

// for now, modify both points
   if(selectedHandle) { 
            pCoord[0].x += (x-prevX);
            pCoord[0].y += (y-prevY);
            sCoord[0].x=cell->pix2val(HORIZ,pCoord[0].x);
            sCoord[0].y=cell->pix2val(VERT,pCoord[0].y);
   }
}

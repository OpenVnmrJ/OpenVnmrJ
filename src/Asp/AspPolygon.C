/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspPolygon.h"
#include "AspUtil.h"

AspPolygon::AspPolygon(char words[MAXWORDNUM][MAXSTR], int nw) : AspAnno(words,nw) {
}

AspPolygon::AspPolygon(spAspCell_t cell, int x, int y) : AspAnno() {
  create(cell,x,y);
}

void AspPolygon::create(spAspCell_t cell, int x, int y) {
   npts=2;
   if(sCoord) delete[] sCoord;
   if(pCoord) delete[] pCoord;
   sCoord = new Dpoint_t[npts];
   pCoord = new Dpoint_t[npts];
   pCoord[0].x=pCoord[1].x = x;
   pCoord[0].y=pCoord[1].y = y;
   sCoord[0].x=sCoord[1].x=cell->pix2val(HORIZ,x,mmbind);
   sCoord[0].y=sCoord[1].y=cell->pix2val(VERT,y,mmbind);
   disFlag = ANN_SHOW_ROI;
   created_type = ANNO_POLYGON;
}

void AspPolygon::display(spAspCell_t cell, spAspDataInfo_t dataInfo) {

   set_transparency_level(transparency);

   int labelColor,roiColor,thick;
   setRoiColor(roiColor,thick);

   for(int i=0; i<npts; i++) {
      pCoord[i].x=cell->val2pix(HORIZ,sCoord[i].x,mmbind);
      pCoord[i].y=cell->val2pix(VERT,sCoord[i].y,mmbind);
   }

   labelX=labelY=labelW=labelH=0;
   if(disFlag & ANN_SHOW_ROI) {
     if(created_type == ANNO_POLYGON && npts==1) {
	int x = (int)pCoord[0].x;
	int y = (int)pCoord[0].y;
	AspUtil::drawMark(x,y,MARKSIZE,MARKSIZE,roiColor,thick);
     } else if(created_type == ANNO_POLYGON && npts==2) {
	AspUtil::drawLine(pCoord[0],pCoord[1],roiColor);
     } else if(created_type == ANNO_POLYGON) {
	Dpoint_t poly[npts+1];
	for(int i=0; i<npts; i++) {
	  poly[i].x=pCoord[i].x;
	  poly[i].y=pCoord[i].y;
	}
	poly[npts].x=pCoord[0].x;
	poly[npts].y=pCoord[0].y;
	if(fillRoi) {
	   Gpoint_t poly2[npts+1];
	   for(int i=0; i<npts; i++) {
	     poly2[i].x=(int)pCoord[i].x;
	     poly2[i].y=(int)pCoord[i].y;
	   }
	   GraphicsWin::fillPolygon(poly2, npts, roiColor);
	} else {
	   GraphicsWin::drawPolyline(poly, npts+1, roiColor);
	}
     } else {
	GraphicsWin::drawPolyline(pCoord, npts, roiColor);
     }

     int i = selectedHandle-1;
     if(i>=0 && i < npts)
        AspUtil::drawMark((int)pCoord[i].x,(int)pCoord[i].y,ACTIVE_COLOR,thick);
   }

   string labelStr="";
   if(disFlag & ANN_SHOW_LABEL) {
     getLabel(dataInfo,labelStr,labelW,labelH);
     if(labelStr == "") labelStr="?";

     labelX = (int)(cell->val2pix(HORIZ,0.5*(sCoord[0].x+sCoord[1].x),mmbind)+labelLoc.x);
     labelY = (int)(cell->val2pix(VERT,0.5*(sCoord[0].y+sCoord[1].y),mmbind)+labelLoc.y);

     setFont(labelColor);
     AspUtil::drawString((char *)labelStr.c_str(), labelX,labelY, labelColor, "", rotate);

     labelY -= labelH;
   }
   set_transparency_level(0);
}

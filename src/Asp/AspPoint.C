/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include "AspPoint.h"
#include "AspUtil.h"

AspPoint::AspPoint(char words[MAXWORDNUM][MAXSTR], int nw) : AspAnno(words,nw) {
}

AspPoint::AspPoint(spAspCell_t cell, int x, int y, bool trCase) : AspAnno() {
  doTraces = trCase;
  create(cell,x,y);
}

void AspPoint::create(spAspCell_t cell, int x, int y) {
   npts=1;
   if(sCoord) delete[] sCoord;
   if(pCoord) delete[] pCoord;
   sCoord = new Dpoint_t[npts];
   pCoord = new Dpoint_t[npts];
   pCoord[0].x = x;
   pCoord[0].y = y;
   sCoord[0].x = cell->pix2val(HORIZ,x,mmbind); 
   if (! doTraces )
      sCoord[0].y = cell->pix2val(VERT,y,mmbindY); 
   else
   {
      int trace;
      selectTraceNum( sCoord[0].x, y, &trace);
      sCoord[0].y=trace;
   }
   disFlag = ANN_SHOW_ROI | ANN_SHOW_LABEL | ANN_SHOW_LINK;
   created_type = ANNO_POINT;
}

void AspPoint::display(spAspCell_t cell, spAspDataInfo_t dataInfo) {
   
   double val;
   double off;
   int dispTrace = 0;

   if ( doTraces )
   {
      if ( (dataInfo->dsDisp >= 1) && (dataInfo->dsDisp != sCoord[0].y) )
         return;
      if ( ! dataInfo->dsDisp ) // dss mode
      {
         if ( (dispTrace = traceShown((int)sCoord[0].y)) == 0 )
            return;
      }
      else if (dataInfo->dsDisp < 0) // mspec display
      {
         if ( (dispTrace = mspecShown((int)sCoord[0].y)) == 0 )
            return;
      }
      else  // plain ds display
      {
         dispTrace = 1;
      }
      
   }
   set_transparency_level(transparency);

   int labelColor,roiColor,thick;
   setRoiColor(roiColor,thick);
   if(selected == HANDLE_SELECTED) roiColor = ACTIVE_COLOR;

   int roiX,roiY,roiW,roiH;
   roiX=roiY=roiW=roiH=0;
   pCoord[0].x=cell->val2pix(HORIZ,sCoord[0].x,mmbind);
   if (doTraces)
   {
     phasefileVal((int) sCoord[0].y-1, sCoord[0].x, dispTrace,  &val, &off);
     pCoord[0].y = cell->offsetval2pix(VERT,val,mmbindY, -off);
   }
   else
     pCoord[0].y=cell->val2pix(VERT,sCoord[0].y,mmbindY);
   roiX = (int)pCoord[0].x;
   roiY = (int)pCoord[0].y;
   roiW = MARKSIZE;
   roiH = MARKSIZE;
   if(disFlag & ANN_SHOW_ROI) {
	AspUtil::drawMark(roiX,roiY,roiW,roiH,roiColor,thick);
   }
   
   labelX=labelY=labelW=labelH=0;
   string labelStr="";
   if((disFlag & ANN_SHOW_LABEL)) {
     getLabel(dataInfo,labelStr,labelW,labelH);
     labelX = (int)(cell->val2pix(HORIZ,sCoord[0].x,mmbind)+labelLoc.x) - labelW/2;
     if (doTraces)
       labelY = (int) (cell->offsetval2pix(VERT,val,mmbindY, -off) + labelLoc.y);
     else
       labelY = (int)(cell->val2pix(VERT,sCoord[0].y,mmbindY)+labelLoc.y);

     if((disFlag & ANN_SHOW_LINK)) { 

       // draw connector
       int space=4;
       int xsize=8;
       int ysize=4;       
       int x = labelX + labelW/2;
       int y = labelY;
       int markX = roiX;
       int markY = roiY;
       if((abs(markX-x) >= xsize || abs(markY-y) >= ysize)) {
        Dpoint_t p1,p2;
        p1.x=x;
        p2.x=markX;
        if(markY > y) {
	   p2.y=markY - space;
           p1.y=y;
        } else {
	   p2.y=markY + space;
           p1.y=y - labelH;
	}
        AspUtil::drawArrow(p1,p2,linkColor,false,false,xsize,ysize);
       }

     }

     setFont(labelColor);
     AspUtil::drawString((char *)labelStr.c_str(), labelX,labelY, labelColor, "", rotate);

     labelY -= labelH;

   }
   set_transparency_level(0);
}

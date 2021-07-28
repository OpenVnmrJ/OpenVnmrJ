/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */


#include <math.h>
#include "AspLine.h"
#include "AspUtil.h"

AspLine::AspLine(char words[MAXWORDNUM][MAXSTR], int nw) : AspAnno(words,nw) {
}

AspLine::AspLine(spAspCell_t cell, int x, int y, bool trCase) : AspAnno() {
  doTraces = trCase;
  create(cell,x,y);
}

void AspLine::create(spAspCell_t cell, int x, int y) {
   npts=2;
   if(sCoord) delete[] sCoord;
   if(pCoord) delete[] pCoord;
   sCoord = new Dpoint_t[npts];
   pCoord = new Dpoint_t[npts];
   pCoord[0].x=pCoord[1].x = x;
   pCoord[0].y=pCoord[1].y = y;
   sCoord[0].x=sCoord[1].x=cell->pix2val(HORIZ,x,mmbind);
   if ( ! doTraces )
      sCoord[0].y=sCoord[1].y=cell->pix2val(VERT,y,mmbindY);
   else
   {
      int trace;
      selectTraceNum( sCoord[0].x, y, &trace);
      sCoord[0].y= sCoord[1].y = trace;
   }
   disFlag = ANN_SHOW_ROI;
   created_type = ANNO_LINE;
}

void AspLine::display(spAspCell_t cell, spAspDataInfo_t dataInfo) {
   
   double val;
   double off;
   int dispTrace0 = 0;
   int dispTrace1 = 0;

   if ( doTraces )
   {
      if ( (dataInfo->dsDisp >= 1) && (dataInfo->dsDisp != sCoord[0].y) )
         return;
      if ( ! dataInfo->dsDisp ) // dss mode
      {
         if ( (dispTrace0 = traceShown((int)sCoord[0].y)) == 0 )
            return;
         if ( (dispTrace1 = traceShown((int)sCoord[1].y)) == 0 )
            return;
      }
      else if (dataInfo->dsDisp < 0) // mspec display
      {
         if ( (dispTrace0 = mspecShown((int)sCoord[0].y)) == 0 )
            return;
         if ( (dispTrace1 = mspecShown((int)sCoord[1].y)) == 0 )
            return;
      }
      else  // plain ds display
      {
         dispTrace0 = 1;
         dispTrace1 = 1;
      }

   }

   set_transparency_level(transparency);

   int labelColor,roiColor,thick;
   setRoiColor(roiColor,thick);

   pCoord[0].x=cell->val2pix(HORIZ,sCoord[0].x,mmbind);
   
   if (doTraces)
   {
     phasefileVal((int) sCoord[0].y-1, sCoord[0].x, dispTrace0, &val, &off);
     pCoord[0].y = cell->offsetval2pix(VERT,val,mmbindY, -off);
   }
   else
      pCoord[0].y=cell->val2pix(VERT,sCoord[0].y,mmbindY);
   pCoord[1].x=cell->val2pix(HORIZ,sCoord[1].x,mmbind);
   if (doTraces)
   {
     phasefileVal((int) sCoord[1].y-1, sCoord[1].x, dispTrace1, &val, &off);
     pCoord[1].y = cell->offsetval2pix(VERT,val,mmbindY, -off);
   }
   else
   pCoord[1].y=cell->val2pix(VERT,sCoord[1].y,mmbindY);

   if(disFlag & ANN_SHOW_ROI) {
     if(created_type == ANNO_ARROW)
	if(arrows==2)
          AspUtil::drawArrow(pCoord[0],pCoord[1],roiColor,true,true,8,4,thick);
	else {
          //AspUtil::drawArrow(pCoord[0],pCoord[1],roiColor,true,false,8,4,thick);
	  // pass 0 for thickness to use the thickness set by setRoiColor
	  draw_arrow((int)pCoord[0].x,(int)pCoord[0].y,(int)pCoord[1].x,(int)pCoord[1].y,0,roiColor);
	}
     else
       AspUtil::drawLine(pCoord[0],pCoord[1],roiColor);

     int i = selectedHandle-1;
     if(i>=0 && i < npts) 
	AspUtil::drawMark((int)pCoord[i].x,(int)pCoord[i].y,ACTIVE_COLOR,thick);
   }

   string labelStr="";
   if(disFlag & ANN_SHOW_LABEL) {
     getLabel(dataInfo,labelStr,labelW,labelH);
     if(labelStr == "") return;

     labelX = (int)(cell->val2pix(HORIZ,0.5*(sCoord[0].x+sCoord[1].x),mmbind)+labelLoc.x);
     labelY = (int)(cell->val2pix(VERT,0.5*(sCoord[0].y+sCoord[1].y),mmbindY)+labelLoc.y);

     setFont(labelColor);
     AspUtil::drawString((char *)labelStr.c_str(), labelX,labelY, labelColor, "", rotate);

     labelY -= labelH;
   }
   set_transparency_level(0);
}

int AspLine::select(int x, int y) {

  selected = selectHandle(x,y);

  if(!selected && (disFlag & ANN_SHOW_ROI)) {
   if(AspUtil::selectLine(x,y,(int)pCoord[0].x,(int)pCoord[0].y,
	(int)pCoord[1].x,(int)pCoord[1].y,true) == LINE1) selected=ROI_SELECTED;
  }

  if(!selected) selected = selectLabel(x,y);

   return selected;

}


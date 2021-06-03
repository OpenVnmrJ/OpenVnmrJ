/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspBox.h"
#include "AspUtil.h"

AspBox::AspBox(char words[MAXWORDNUM][MAXSTR], int nw) : AspAnno(words,nw) {
}

AspBox::AspBox(spAspCell_t cell, int x, int y) : AspAnno() {
  create(cell,x,y);
}

void AspBox::create(spAspCell_t cell, int x, int y) {
   npts=2;
   if(sCoord) delete sCoord;
   if(pCoord) delete pCoord;
   sCoord = new Dpoint_t[npts];
   pCoord = new Dpoint_t[npts];
   pCoord[0].x=pCoord[1].x = x;
   pCoord[0].y=pCoord[1].y = y;
   sCoord[0].x=sCoord[1].x=cell->pix2val(HORIZ,x,mmbind);
   sCoord[0].y=sCoord[1].y=cell->pix2val(VERT,y,mmbindY);
   disFlag = ANN_SHOW_ROI;
   created_type = ANNO_BOX;
}

void AspBox::display(spAspCell_t cell, spAspDataInfo_t dataInfo) {

   set_transparency_level(transparency);

   int labelColor,roiColor,thick;
   setRoiColor(roiColor,thick);

   pCoord[0].x=cell->val2pix(HORIZ,sCoord[0].x,mmbind);
   pCoord[0].y=cell->val2pix(VERT,sCoord[0].y,mmbindY);
   pCoord[1].x=cell->val2pix(HORIZ,sCoord[1].x,mmbind);
   pCoord[1].y=cell->val2pix(VERT,sCoord[1].y,mmbindY);

   int roiX,roiY,roiW,roiH;
   labelX=labelY=labelW=labelH=0;
   roiX=roiY=roiW=roiH=0;
   if(disFlag & ANN_SHOW_ROI) {
     roiX=(int)pCoord[0].x;
     roiY=(int)pCoord[0].y;
     roiW=(int)pCoord[1].x-roiX;
     roiH=(int)pCoord[1].y-roiY;
     if(created_type == ANNO_OVAL)
	draw_oval(roiX,roiY,roiX+roiW,roiY+roiH,0,roiColor,fillRoi);
     else if(roundBox)
	draw_round_rect(roiX,roiY,roiX+roiW,roiY+roiH,0,roiColor,fillRoi);
     else
	draw_rect(roiX,roiY,roiX+roiW,roiY+roiH,0,roiColor,fillRoi);
     if(selected == HANDLE_SELECTED) {
         AspUtil::drawHandle(selectedHandle,roiX,roiY,roiW,roiH,ACTIVE_COLOR,thick);
     } else if(selected == ROI_SELECTED) {
         AspUtil::drawHandle(HANDLE1,roiX,roiY,roiW,roiH,ACTIVE_COLOR,thick);
         AspUtil::drawHandle(HANDLE2,roiX,roiY,roiW,roiH,ACTIVE_COLOR,thick);
         AspUtil::drawHandle(HANDLE3,roiX,roiY,roiW,roiH,ACTIVE_COLOR,thick);
         AspUtil::drawHandle(HANDLE4,roiX,roiY,roiW,roiH,ACTIVE_COLOR,thick);
     }
   }

   string labelStr="";
   if(disFlag & ANN_SHOW_LABEL) {
     getLabel(dataInfo,labelStr,labelW,labelH);
     if(labelStr == "") labelStr="?";

     labelX = (int)(cell->val2pix(HORIZ,0.5*(sCoord[0].x+sCoord[1].x),mmbind)+labelLoc.x);
     labelY = (int)(cell->val2pix(VERT,0.5*(sCoord[0].y+sCoord[1].y),mmbindY)+labelLoc.y);

     setFont(labelColor);
     AspUtil::drawString((char *)labelStr.c_str(), labelX,labelY, labelColor, "", rotate);

     labelY -= labelH;
   }
   set_transparency_level(0);
}

int AspBox::select(int x, int y) {

   selected=selectHandle(x,y);
   if(!selected) {
	int x0=(int)pCoord[0].x;
	int y0=(int)pCoord[0].y;
	int x1=(int)pCoord[1].x;
	int y1=(int)pCoord[1].y;
        int sel = AspUtil::select(x,y,x0,y0,x1-x0,y1-y0,2,true);
	if(sel>=HANDLE1 && sel <=HANDLE4) {
          selectedHandle = sel;
          selected=HANDLE_SELECTED;
	}

	if(!selected) selected = selectLabel(x,y);
	if(!selected) { // select box or circle by selecting one of the boarder lines

           selectedHandle = 0;
           if(AspUtil::selectLine(x,y,x0,y0,x1,y0) == LINE1) {
                selected=ROI_SELECTED;
                return selected;
	   } else if(AspUtil::selectLine(x,y,x0,y0,x0,y1) == LINE1) {
                selected=ROI_SELECTED;
                return selected;
	   } else if(AspUtil::selectLine(x,y,x1,y0,x1,y1) == LINE1) {
                selected=ROI_SELECTED;
                return selected;
	   } else if(AspUtil::selectLine(x,y,x0,y1,x1,y1) == LINE1) {
                selected=ROI_SELECTED;
                return selected;
	   }
	}

   } else {
	if(selectedHandle==1) selectedHandle=HANDLE1;
	else if(selectedHandle==2) selectedHandle=HANDLE3;
   }
   return selected;
}

void AspBox::modify(spAspCell_t cell, int x, int y, int prevX, int prevY) {

   if(npts<1) return;
   if(selected == HANDLE_SELECTED) {
        if(selectedHandle == HANDLE1) {
          pCoord[0].x=x;
          pCoord[0].y=y;
          sCoord[0].x=cell->pix2val(HORIZ,x,mmbind);
          sCoord[0].y=cell->pix2val(VERT,y,mmbindY);
        } else if(selectedHandle == HANDLE2) {
          pCoord[1].x=x;
          pCoord[0].y=y;
          sCoord[1].x=cell->pix2val(HORIZ,x,mmbind);
          sCoord[0].y=cell->pix2val(VERT,y,mmbindY);
        } else if(selectedHandle == HANDLE3) {
          pCoord[1].x=x;
          pCoord[1].y=y;
          sCoord[1].x=cell->pix2val(HORIZ,x,mmbind);
          sCoord[1].y=cell->pix2val(VERT,y,mmbindY);
        } else if(selectedHandle == HANDLE4) {
          pCoord[0].x=x;
          pCoord[1].y=y;
          sCoord[0].x=cell->pix2val(HORIZ,x,mmbind);
          sCoord[1].y=cell->pix2val(VERT,y,mmbindY);
	}
	return;
   } 
   double cx=0;
   double cy=0;
   for(int i=0; i<npts; i++) {
       cx += pCoord[i].x;
       cy += pCoord[i].y;
   }
   cx /= npts;
   cy /= npts;
   if(selected == ROI_SELECTED) {
	cx = x-cx;
	cy = y-cy;
        for(int i=0; i<npts; i++) {
            pCoord[i].x += cx;
            pCoord[i].y += cy;
            sCoord[i].x=cell->pix2val(HORIZ,pCoord[i].x,mmbind);
            sCoord[i].y=cell->pix2val(VERT,pCoord[i].y,mmbindY);
        }
   } else if(selected == LABEL_SELECTED) {
     labelLoc.x += (x-prevX);
     labelLoc.y += (y-prevY);
   }
}

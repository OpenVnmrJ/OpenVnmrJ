/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include "AspBar.h"
#include "AspUtil.h"

AspBar::AspBar(char words[MAXWORDNUM][MAXSTR], int nw) : AspLine(words,nw) {
}

AspBar::AspBar(spAspCell_t cell, int x, int y) : AspLine(cell,x,y) {
   disFlag = ANN_SHOW_ROI | ANN_SHOW_LABEL;
   created_type = ANNO_XBAR;
   rotate=1;
   label="%hz%";
}

void AspBar::getLabel(spAspDataInfo_t dataInfo, string &lbl, int &cwd, int &cht) {

   string thisLabel=AspUtil::subParamInStr(label);

   double value, scale;
   if(created_type == ANNO_YBAR) {
      value= fabs(sCoord[1].y - sCoord[0].y);
      if(dataInfo->vaxis.name == "amp") scale = 1.0;
      else scale = dataInfo->vaxis.scale;
   } else {
      value= fabs(sCoord[1].x - sCoord[0].x);
      if(dataInfo->haxis.name == "amp") scale = 1.0;
      else scale = dataInfo->haxis.scale;
   }
   char str[MAXSTR],tmpStr[MAXSTR];
   sprintf(str,"%.4f",value);
   if(thisLabel == "?") {
        lbl = string(str);
   } else {
      size_t p1=string::npos;
      size_t p2=string::npos;
      p1=thisLabel.find("%",0);
      while(p1 != string::npos) {
       p2=thisLabel.find("%",p1+1);
       if(p2 != string::npos) {
         strcpy(tmpStr,thisLabel.substr(p1+1,p2-p1-1).c_str());
         bool hz = (strcasecmp(tmpStr,"hz") == 0);
	 if(hz) sprintf(str,"%.4f",value*scale); 
         thisLabel = thisLabel.substr(0,p1) + string(str) + thisLabel.substr(p2+1);
	 p1=thisLabel.find("%",p2+1);
       } else p1 = string::npos;
      } 
      lbl = string(thisLabel);
   }

   getStringSize(lbl,cwd,cht);

   return;
}

void AspBar::display(spAspCell_t cell, spAspDataInfo_t dataInfo) {
   
   set_transparency_level(transparency);

   int labelColor,roiColor,thick;
   setRoiColor(roiColor,thick);

   if(created_type == ANNO_YBAR) {
     sCoord[1].x=sCoord[0].x;
     pCoord[0].x=pCoord[1].x=cell->val2pix(HORIZ,sCoord[0].x,mmbind);
     pCoord[0].y=cell->val2pix(VERT,sCoord[0].y,mmbind);
     pCoord[1].y=cell->val2pix(VERT,sCoord[1].y,mmbind);
   } else {
     sCoord[1].y=sCoord[0].y;
     pCoord[0].y=pCoord[1].y=cell->val2pix(VERT,sCoord[0].y,mmbind);
     pCoord[0].x=cell->val2pix(HORIZ,sCoord[0].x,mmbind);
     pCoord[1].x=cell->val2pix(HORIZ,sCoord[1].x,mmbind);
   }

   labelX=labelY=labelW=labelH=0;
   if(disFlag & ANN_SHOW_ROI) {
     AspUtil::drawLine(pCoord[0],pCoord[1],roiColor);
     int i = selectedHandle - 1;
     if(i >=0 && i<npts) {
        AspUtil::drawMark((int)pCoord[i].x,(int)pCoord[i].y,ACTIVE_COLOR,thick);
     }
   }

   string labelStr="";
   if((disFlag & ANN_SHOW_LABEL)) {
     getLabel(dataInfo,labelStr,labelW,labelH);
     if(labelStr == "") labelStr="?";

     if(created_type == ANNO_YBAR) {
       if(labelLoc.x==0 && labelLoc.y ==0) {
	  labelLoc.x = -0.5*labelW;
	  labelLoc.y = 0.5*labelH;
       }
     }
     labelX = (int)(cell->val2pix(HORIZ,0.5*(sCoord[0].x+sCoord[1].x),mmbind) + labelLoc.x) - labelW/2;
     labelY = (int)(cell->val2pix(VERT,0.5*(sCoord[0].y+sCoord[1].y),mmbind) + labelLoc.y);


     // draw connector
     if(created_type == ANNO_YBAR) {
       Dpoint_t p1,p2;
       p1.x=pCoord[0].x;
       p1.y=pCoord[0].y;
       p2.x=p1.x+8;
       p2.y=p1.y;
       AspUtil::drawLine(p1,p2,roiColor);
       p1.x=pCoord[0].x;
       p1.y=pCoord[1].y;
       p2.x=p1.x+8;
       p2.y=p1.y;
       AspUtil::drawLine(p1,p2,roiColor);
     } else {
       Dpoint_t p1,p2;
       p1.x=pCoord[0].x;
       p1.y=pCoord[0].y;
       p2.x=p1.x;
       p2.y=p1.y+8;
       AspUtil::drawLine(p1,p2,roiColor);
       p1.x=pCoord[1].x;
       p1.y=pCoord[0].y;
       p2.x=p1.x;
       p2.y=p1.y+8;
       AspUtil::drawLine(p1,p2,roiColor);
     }

     setFont(labelColor);
     AspUtil::drawString((char *)labelStr.c_str(), labelX,labelY, labelColor, "", rotate);

     labelY -= labelH;
   }
   set_transparency_level(0);
}

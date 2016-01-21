/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include "AspInteg.h"
#include "AspUtil.h"

spAspInteg_t nullAspInteg = spAspInteg_t(NULL);

// this is related to toString
// str: 
AspInteg::AspInteg(char words[MAXWORDNUM][MAXSTR], int nw) {
   init();
   npts=2;
   if(sCoord) delete[] sCoord;
   if(pCoord) delete[] pCoord;
   sCoord = new Dpoint_t[npts];
   pCoord = new Dpoint_t[npts];

   if(nw < 5) return;
   index=atoi(words[0]) - 1;
   sCoord[0].x=atof(words[1]);
   sCoord[1].x=atof(words[2]);
   sCoord[0].y=sCoord[1].y=0.0;
   pCoord[0].x=pCoord[0].y=pCoord[1].x=pCoord[1].y=0;
   absValue=atof(words[3]);
   normValue=atof(words[4]);
   if(nw>5) dataID=string(words[5]);
   int count = 6;
   if(nw>count) {
        int ln = strlen(words[count]);
        if(words[count][0]=='|' && words[count][ln-1]=='|') {
           string str = string(words[count]); count++;
           label=str.substr(1,str.find_last_of("|")-1);
        } else if(words[count][0]=='|') {
           string str = string(words[count]); count++;
           bool endW = false;
           while(!endW && nw>count) {
                endW = (strstr(words[count],"|") != NULL);
                str += " ";
                str += string(words[count]); count++;
           }
	   if(endW) label=str.substr(1,str.find_last_of("|")-1);
	   else label=str.substr(1,str.find_last_of("|"));
        } else {
           label=string(words[count]); count++; 
        }
   }
   if(nw>count) {labelLoc.x=atof(words[count]); count++; }
   if(nw>count) {labelLoc.y=atof(words[count]); count++; }
   if(nw>count) {color=atoi(words[count]); count++; }
   if(nw>count) {fontColor=string(words[count]); count++; }
   if(nw>count) {fontSize=atoi(words[count]); count++; }
   if(nw>count) {fontName=string(words[count]); count++; }
   if(nw>count) {fontStyle=string(words[count]); count++; }
   if(nw>count) {rotate=atoi(words[count]); count++; }
   if(nw>count) {disFlag=atoi(words[count]); count++; }
}

// x1 is left
AspInteg::AspInteg(int ind, double freq1, double freq2, double amp) {
    init();
    npts=2;
    if(sCoord) delete[] sCoord;
    if(pCoord) delete[] pCoord;
    sCoord = new Dpoint_t[npts];
    pCoord = new Dpoint_t[npts];

    if(freq1>freq2) {
	sCoord[0].x=freq1;
	sCoord[1].x=freq2;
    } else {
	sCoord[0].x=freq2;
	sCoord[1].x=freq1;
    }
   sCoord[0].y=sCoord[1].y=0.0;
   pCoord[0].x=pCoord[0].y=pCoord[1].x=pCoord[1].y=0;
    absValue=amp;
    index = ind;
}

void AspInteg::init() {
    AspAnno::init();
    created_type = ANNO_INTEG;
    npts=2;
    dataID="?";
    absValue=0.0;
    normValue=0.0;
    m_data = NULL;
    label="?";
}

AspInteg::~AspInteg() {
    if(m_data) delete[] m_data;
}

string AspInteg::toString() {
   char str[MAXSTR];
   sprintf(str,"%d %f %f %f %f %s %s %f %f %d %s %d %s %s %d %d",
        index+1,sCoord[0].x,sCoord[1].x,absValue,normValue,dataID.c_str(),label.c_str(),
        labelLoc.x, labelLoc.y,
        color,fontColor.c_str(),fontSize,fontName.c_str(),fontStyle.c_str(),rotate,disFlag);
   return string(str);
}

void AspInteg::getLabel(spAspDataInfo_t dataInfo, string &lbl, int &cwd, int &cht) { 

   string thisLabel=AspUtil::subParamInStr(label);

   lbl = string(thisLabel); 

   int ascent, descent;
   GraphicsWin::getTextExtents(lbl.c_str(), 14, &ascent, &descent, &cwd);

   if(rotate) {
     cht = cwd;
     cwd = ascent + descent;
   } else {
     cht = ascent + descent;
   }

   return;
}

void AspInteg::getValue(spAspDataInfo_t dataInfo, string &lbl, int &cwd, int &cht) { 
   char str[MAXSTR],tmpStr[MAXSTR];
   sprintf(str,"%.2f",absValue);
   if (P_getstring(GLOBAL, "integProperty", tmpStr, 1, sizeof(tmpStr))) strcpy(tmpStr,"%norm%"); 
   string value = string(tmpStr);
   if(value.length() < 1 || value == "-" || value == "?") {
	lbl = string(str);
   } else {
      size_t p1=string::npos;
      size_t p2=string::npos;
      p1=value.find("%",0);
      if(p1 != string::npos) p2=value.find("%",p1+1);
      if(p2 != string::npos) {
         strcpy(tmpStr,value.substr(p1+1,p2-p1-1).c_str());
         bool norm = (strcasecmp(tmpStr,"norm") == 0);
         if(norm) {
                sprintf(str,"%.2f",normValue);
         }
	 lbl = value.substr(0,p1) + string(str) + value.substr(p2+1);
      } else lbl = string(value); 
   }

   int ascent, descent;
   GraphicsWin::getTextExtents(lbl.c_str(), 14, &ascent, &descent, &cwd);

   cht = ascent + descent;

   return;
}

void AspInteg::display(spAspCell_t cell, spAspTrace_t trace, spAspDataInfo_t dataInfo, 
	int integFlag, double is, double off) {
	set_transparency_level(transparency);

	disFlag = integFlag;

        float *traceData = trace->getFirstDataPtr();
        if(!traceData) return;

	int maxpts = trace->getTotalpts();
	if(maxpts<1) return;

	absValue = trace->getInteg(sCoord[0].x,sCoord[1].x);

   double x1=pCoord[0].x=cell->val2pix(HORIZ,sCoord[0].x);
   double x2=pCoord[1].x=cell->val2pix(HORIZ,sCoord[1].x);

	// get integ data
	if(m_data) delete[] m_data;

        int p1 = trace->val2dpt(sCoord[0].x);                           
        int p2 = trace->val2dpt(sCoord[1].x);                
	m_datapts = p2-p1+1;
        m_data = new float[m_datapts];
        float sum = 0;
        int k=0;
        for(int i=p1;i<=p2;i++) {
          //sum += (*(traceData+i) - *(traceData+p1));
          sum += (*(traceData+i));
          (*(m_data+k)) = sum;
          k++;
        }

	double vcali = cell->getCali(VERT);

	m_scale = is/(double)maxpts;
	m_yoff = off*vcali;

	double vscale = dataInfo->getVscale();
	if(vscale == 0) vscale = 1.0;
	double scale = m_scale*vscale*vcali;

        // add vp
        double yoff = m_yoff + (dataInfo->getVpos())*vcali;

   double y1=pCoord[0].y=cell->val2pix(VERT,(*(m_data))*m_scale) - m_yoff;
   double y2=pCoord[1].y=cell->val2pix(VERT,(*(m_data+m_datapts-1))*m_scale) - m_yoff;
//AspUtil::drawMark((int)x1,(int)y1,ACTIVE_COLOR);
//AspUtil::drawMark((int)x2,(int)y2,ACTIVE_COLOR);

	// check x1, x2 boundary 
	double px,pw,py,ph,px2;
	cell->getPixCell(px,py,pw,ph);
	px2=px+pw;
	if(x1 < px && x2 < px) return;
	if(x1 > px2 && x2 > px2) return;
	if(x1 < px) x1=px;
	if(x2 > px2) x2=px2;
	 

        int roiColor;
 	if(selected == ROI_SELECTED) roiColor=ACTIVE_COLOR;
        else roiColor = INT_COLOR;
        
	char thickName[64];
        int thickness; // used by functions
        int thick=0; // use thickness set by functions
        string value; 
	if(integFlag & SHOW_INTEG) { 
	  getOptName(INTEG_LINE,thickName);
	  //AspUtil::getDisplayOption(string(thickName)+"Thickness",value); 
	  //thickness = atoi(value.c_str());
	  //set_spectrum_width(thickness);
          //set_line_width(thickness);
	  set_spectrum_thickness(thickName,thickName,1.0);
	  set_line_thickness(thickName);
	  cell->drawPolyline(m_data,m_datapts,1,x1,py,x2-x1,ph,roiColor,scale,yoff);
	  int i = selectedHandle-1;
     	  if(i>=0 && i < npts) {
        	AspUtil::drawMark((int)pCoord[i].x,(int)pCoord[i].y,ACTIVE_COLOR,thick);
	  }
	}
        labelX=labelY=labelW=labelH=0;
	if((integFlag & SHOW_LABEL)) {
	  if(integFlag & SHOW_VERT_LABEL) rotate = 1;
	  else rotate=0;
	  string labelStr;
	  getLabel(dataInfo,labelStr,labelW,labelH);
	  labelX=(int)(labelLoc.x+(x1+x2)/2) - labelW/2;
	  labelY=(int)(labelLoc.y+(y1+y2)/2) - labelH;
	  if(labelY<(py+labelH)) labelY = (int)py+labelH;
	  char fontName[64];
	  if(LABEL_SELECTED) {
	     int labelColor;		
	     setFont(labelColor);
     	     AspUtil::drawString((char *)labelStr.c_str(), labelX,labelY, labelColor, "", rotate);
	  } else {
	     getOptName(INTEG_LABEL,fontName); 
	     AspUtil::drawString((char *)labelStr.c_str(), labelX, labelY, -1, fontName,rotate);
	  }
	  labelY -= labelH;
	}
 	if(selected == ROI_SELECTED) roiColor=ACTIVE_COLOR;
	else roiColor = INTEG_MARK_COLOR;

	getOptName(INTEG_MARK,thickName);
	AspUtil::getDisplayOption(string(thickName)+"Thickness",value); 
	thickness = atoi(value.c_str());
	if(thickness < 1) thickness=1;
	//set_spectrum_width(thickness);
        //set_line_width(thickness);
	set_spectrum_thickness(thickName,thickName,1.0);
	set_line_thickness(thickName);

	if((integFlag & SHOW_VALUE) && (integFlag & SHOW_VERT_VALUE)) {
	  string labelStr;
          int cwd, cht, ascent, descent;
	  getValue(dataInfo,labelStr,cht,cwd);
	  // overwrite cht, cwd with fixed length
          char str[MAXSTR];
          sprintf(str,"%.2f",100.00);
          GraphicsWin::getTextExtents(str, 14, &ascent, &descent, &cht);
          cwd = ascent + descent;
	  double y = py + ph - cht - 5*thickness;  
	  Dpoint_t p1,p2;
	  p1.x=x1;
	  p2.x=x2;
	  p1.y=p2.y=y;
	  AspUtil::drawLine(p1,p2,roiColor,thick);
	  p1.x=p2.x=x1;
	  p2.y=y-5*thickness;
	  AspUtil::drawLine(p1,p2,roiColor,thick);
	  p1.x=p2.x=x2;
	  p2.y=y-5*thickness;
	  AspUtil::drawLine(p1,p2,roiColor,thick);
	  p1.x=p2.x=0.5*(x1+x2);
	  p2.y=y+5*thickness;
	  AspUtil::drawLine(p1,p2,roiColor,thick);
	  char fontName[64];
	  getOptName(INTEG_NUM,fontName); 
	  y = py + ph - cwd/2;
	  AspUtil::drawString((char *)labelStr.c_str(), (int)p1.x-cwd/2, (int)y, -1, fontName,1);
	} else if(integFlag & SHOW_VALUE) {
	  string labelStr;
   	  int cwd, cht;
	  getValue(dataInfo,labelStr,cwd,cht);
	  double y = py + ph - cht - 15*thickness;  
	  Dpoint_t p1,p2;
	  p1.x=x1;
	  p2.x=x2;
	  p1.y=p2.y=y;
	  AspUtil::drawLine(p1,p2,roiColor,thick);
	  p1.x=p2.x=x1;
	  p2.y=y-5*thickness;
	  AspUtil::drawLine(p1,p2,roiColor,thick);
	  p1.x=p2.x=x2;
	  p2.y=y-5*thickness;
	  AspUtil::drawLine(p1,p2,roiColor,thick);
	  p1.x=p2.x=0.5*(x1+x2);
	  p2.y=y+5*thickness;
	  AspUtil::drawLine(p1,p2,roiColor,thick);
	  char fontName[64];
	  getOptName(INTEG_NUM,fontName); 
	  y = py + ph - cht/2;
	  AspUtil::drawString((char *)labelStr.c_str(), (int)p1.x-cwd/2, (int)y, -1, fontName,0);
	}
	getOptName(SPEC_LINE_MIN,thickName);
	set_spectrum_width(1);
        set_line_width(1);
	set_transparency_level(0);
}

int AspInteg::select(spAspCell_t cell, int x, int y) {
   selected = selectHandle(x,y);
   if(!selected) selected = selectLabel(x,y);

   if(!selected && x>pCoord[0].x && x<pCoord[1].x) { // select integral line
     double d = (double)(pCoord[0].x - x)/(double) (pCoord[0].x-pCoord[1].x);
     int pos = (int) (m_datapts * d) - 1;
     if(pos>=0 && pos<m_datapts) {
	int pixy = (int)(cell->val2pix(VERT,(*(m_data+pos))*m_scale) - m_yoff);
	if(abs(pixy-y) < MARKSIZE) {
	   selectedHandle=0;
	   selected=ROI_SELECTED;
	} 
     }
   }
   return selected;
}

// only modify io ans is
void AspInteg::modifyVert(spAspCell_t cell, int x, int y, int prevX, int prevY) {
   if(selectedHandle == 2) { // upfield, right, modify is
	double is = AspUtil::getReal("is",100.0);
	is += (prevY-y)*(0.01*is);
	AspUtil::setReal("is",is, false);	
   } else if(selectedHandle == 1) { // modify both io and is
	double io = AspUtil::getReal("io",0);
	io += (prevY-y)/cell->getCali(VERT);
	AspUtil::setReal("io",io, false);	
	double is = AspUtil::getReal("is",100.0);
	is -= (prevY-y)*(0.01*is);
	AspUtil::setReal("is",is, false);	
   } else if(selected == ROI_SELECTED) { // change io
	double io = AspUtil::getReal("io",0);
	io += (prevY-y)/cell->getCali(VERT);
	AspUtil::setReal("io",io, false);	
   }
}

void AspInteg::modify(spAspCell_t cell, int x, int y, int prevX, int prevY) {

   if(selectedHandle == 2) { // upfield, right, modify is
            pCoord[1].x += (x-prevX);
            sCoord[1].x=cell->pix2val(HORIZ,pCoord[1].x);
        double is = AspUtil::getReal("is",100.0);
        is += (prevY-y)*(0.01*is);
        AspUtil::setReal("is",is, false);       
   } else if(selectedHandle == 1) { // modify both io and is
            pCoord[0].x += (x-prevX);
            sCoord[0].x=cell->pix2val(HORIZ,pCoord[0].x);
        double io = AspUtil::getReal("io",0);
        io += (prevY-y)/cell->getCali(VERT);
        AspUtil::setReal("io",io, false);
        double is = AspUtil::getReal("is",100.0);
        is -= (prevY-y)*(0.01*is);
        AspUtil::setReal("is",is, false);       
   } else if(selected == ROI_SELECTED) { // change io
        double io = AspUtil::getReal("io",0);   
        io += (prevY-y)/cell->getCali(VERT);
        AspUtil::setReal("io",io, false);

        for(int i=0; i<npts; i++) {
            pCoord[i].x += (x-prevX);
            sCoord[i].x=cell->pix2val(HORIZ,pCoord[i].x);
        }
  } else if(selected == LABEL_SELECTED) {
     labelLoc.x += (x - prevX);
     labelLoc.y += (y - prevY);
  }
}

string AspInteg::getKey() {
   char str[MAXSTR];
   if(dataID=="")
     sprintf(str,":%.4g:%.4g",sCoord[0].x,sCoord[1].x);
   else
     sprintf(str,"%s:%.4g:%.4g",dataID.c_str(),sCoord[0].x,sCoord[1].x);

   return string(str);
}

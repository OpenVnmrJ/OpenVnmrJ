/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include "AspPeak.h"
#include "AspUtil.h"

spAspPeak_t nullAspPeak = spAspPeak_t(NULL);

void AspPeak::initPeak() {
    index=0;
    cursor=nullAspCursor;
    rank=0;
    label="-";
    labelLoc.x=0.0;
    labelLoc.y=0.0;
    autoLoc.x=0.0;
    autoLoc.y=0.0;
    height=0.0;
    integral=0.0;
    amplitude=0.0;
    selected=0;
    labelStr="";
    markX=markY=0;
    markW=markH=MARKSIZE;
    labelX=labelY=labelW=labelH=0;
    dataID="";
}

AspPeak::AspPeak(spAspCursor_t c) {
    initPeak();
    cursor=c;
    rank=cursor->rank;
}

AspPeak::AspPeak(int ind, spAspCursor_t c) {
    initPeak();
    index=ind;
    cursor=c;
    rank=cursor->rank;
}

// this is related to toString
// str: 
AspPeak::AspPeak(char words[MAXWORDNUM][MAXSTR], int nw) {
   initPeak();

   if(nw < 2) return;
   index=atoi(words[0]) - 1;
   rank=atoi(words[1]);
   int count=0;
   if(rank==1 && nw > 4) {
	cursor= spAspCursor_t(new AspCursor(atof(words[4]),words[2]));	
	cursor->resonances[0].assignedName=words[3];
	count=4;
   } else if(rank==2 && nw > 7) {
	cursor= spAspCursor_t(new AspCursor(atof(words[6]),atof(words[7]),words[2],words[3]));	
	cursor->resonances[0].assignedName=words[4];
	cursor->resonances[1].assignedName=words[5];
	count=7;
   }
   ++count;
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
   if(nw>count) labelLoc.x=atof(words[count]);
   ++count;
   if(nw>count) labelLoc.y=atof(words[count]);
   ++count;
   if(nw>count) height=atof(words[count]);
   ++count;
   if(nw>count) integral=atof(words[count]);
   ++count;
   if(nw>count) amplitude=atof(words[count]);
   ++count;
   if(nw>count) dataID=string(words[count]);
   return;

}

AspPeak::~AspPeak() {
}

string AspPeak::toString() {
   char str[MAXSTR];
   if(dataID=="") {
     sprintf(str, "%d %s %s %f %f %f %f %f",index+1,cursor->toString().c_str(),
	label.c_str(),labelLoc.x, labelLoc.y,height,integral,amplitude);
   } else {
     sprintf(str, "%d %s %s %f %f %f %f %f %s",index+1,cursor->toString().c_str(),
	label.c_str(),labelLoc.x, labelLoc.y,height,integral,amplitude,dataID.c_str());
   }
   return string(str);
}

void AspPeak::display(spAspCell_t cell, int xoff, int yoff, spAspDataInfo_t dataInfo, int peakFlag) {
   if(rank == 1) display1d(cell,xoff,yoff,dataInfo,peakFlag);
   else if(rank == 2) display2d(cell,xoff,yoff,dataInfo,peakFlag);
}

void AspPeak::getLabel(spAspDataInfo_t dataInfo, string &lbl, int &cwd, int &cht) { 

   string thisLabel=AspUtil::subParamInStr(label);

   char str[MAXSTR],tmpStr[MAXSTR];
   double freq = cursor->resonances[0].freq;
   sprintf(str,"%.4f",freq);
   if(thisLabel.length()<1 || thisLabel == "?" || thisLabel == "-") {
	if (P_getstring(GLOBAL, "peakProperty", tmpStr, 1, sizeof(tmpStr))) strcpy(tmpStr,"%ppm%");
   	thisLabel = string(tmpStr);
   }
   if(thisLabel.length()<1 || thisLabel == "?" || thisLabel == "-") {
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
         bool am = (strcasecmp(tmpStr,"amp") == 0);
         if(hz) { // 1D
                sprintf(str,"%.2f",dataInfo->haxis.scale*freq);
         } else if(am) {
                sprintf(str,"%.4f",height);
         }
         thisLabel = thisLabel.substr(0,p1) + string(str) + thisLabel.substr(p2+1);
	 p1=thisLabel.find("%",p2+1);
	} else p1 = string::npos;
      }
      lbl = string(thisLabel);
   }

   int ascent, descent;
   GraphicsWin::getTextExtents(str, 14, &ascent, &descent, &cwd);
   cht = ascent + descent;

   return;
}

void AspPeak::display1d(spAspCell_t cell, int xoff, int yoff, spAspDataInfo_t dataInfo, int peakFlag) {

   char fontName[64];
   int markcolor,linkColor;
   if(selected == PEAK_SELECTED) markcolor = ACTIVE_COLOR; 
   else markcolor = PEAK_MARK_COLOR;
   linkColor = PEAK_MARK_COLOR;
   if(selected == LABEL_SELECTED) {
        getOptName(PEAK_LABEL,fontName);
        markcolor = ACTIVE_COLOR;
	linkColor = ACTIVE_COLOR;
   } else {
        getOptName(PEAK_NUM,fontName);
   }

   if(dataInfo->haxis.name != cursor->resonances[0].name) return;

   double px,py,pw,ph;
   cell->getPixCell(px,py,pw,ph);

   int size1 = (int)(ph/40.0); // for drawing lines connecting peaks and labels.
   int size2 = 2*size1; 
   int size4 = 4*size1;
   
   double freq = cursor->resonances[0].freq;
   markX = (int) cell->val2pix(HORIZ,freq) - xoff;
   markY = (int) cell->val2pix(VERT,height) - yoff;

   if(markX < px || markX>(px+pw)) return;

   if(selected == PEAK_SELECTED || selected == LABEL_SELECTED || (peakFlag & PEAK_MARKING)) 
	AspUtil::drawMark(markX,markY,markW,markH,markcolor); 

   labelStr = "";
   labelX=labelY=labelW=labelH=0;

   if(!((peakFlag & PEAK_NAME) || (peakFlag & PEAK_VALUE))) return;

   // PEAK_MARKING PEAK_TOP PEAK_VERT PEAK_HORIZ PEAK_AUTO PEAK_NAME PEAK_VALUE PEAK_NOLINK

   if((peakFlag & PEAK_VERT))
      getLabel(dataInfo,labelStr,labelH,labelW);
   else 
      getLabel(dataInfo,labelStr,labelW,labelH);

   int x=markX;
   int y=markY;
   
/* this will allow independently set top and vert/horiz
   if((peakFlag & PEAK_VERT)) {
	x += (int)(autoLoc.x+labelLoc.x);
	y += (int)(autoLoc.y+labelLoc.y);
   } else {
	// labelLoc is in pixel relative to peak mark 
	x += (int)labelLoc.x;
	y += (int) labelLoc.y;
   }
   if((peakFlag & PEAK_TOP) ) {
        y = (int) py + labelH + (int) labelLoc.y;
   } else if((peakFlag & PEAK_VERT)) y -= (size1+size2+size4);
   else y -= labelH;
*/
// the following force vert is always top and horiz is not
   if((peakFlag & PEAK_VERT)) {
	x += (int)(autoLoc.x+labelLoc.x);
        y = (int) py + labelH + (int) labelLoc.y;
   } else {
	// labelLoc is in pixel relative to peak mark 
	x += (int)labelLoc.x;
	y += (int) labelLoc.y;
	y -= labelH;
   }

   if(peakFlag & PEAK_VERT) { 
     //if((x-labelW/2) < px || (x+labelW/2)>(px+pw)) return;

     if(y < ((int) py + labelH)) y = (int) py + labelH;
     if(!(peakFlag & PEAK_NOLINK) && (peakFlag & PEAK_SHORT)) { 
	// draw link
        int x1,x2,y1,y2;
	if(markX==x) {
	   x1=x; 
	   x2=x; 
	   y1=y; 
	   y2=y+size1+size1+size4; 
	   GraphicsWin::drawLine(x1,y1,x2,y2, linkColor);
	} else {
	   x1=x;
           x2=x;
           y1=y;
           y2=y1+size1;
           GraphicsWin::drawLine(x1,y1,x2,y2, linkColor);
	   x1=x;
           x2=markX;
           y1=y2;
           y2=y1+size1;
           GraphicsWin::drawLine(x1,y1,x2,y2, linkColor);
	   x1=markX;
           x2=markX;
           y1=y2;
           y2=y1+size4;
           GraphicsWin::drawLine(x1,y1,x2,y2, linkColor);
	}
     } else if(!(peakFlag & PEAK_NOLINK)) { // draw link
        int x1,x2,y1,y2;
	double slope = ph/pw;
        int ymax = (int)(ph);
	if(markX==x) {
	   x1=x; 
	   x2=x; 
	   y1=y; 
	   y2=markY-size2; 
	   if(y2>y1)
	   GraphicsWin::drawLine(x1,y1,x2,y2, linkColor);
	} else {
	   x1=x;
           x2=markX;
           y1=y+size1;
           y2=y1+(int)(slope*fabs(autoLoc.x+labelLoc.x));
	   if(y2>ymax || y2>(markY-size2) ) {
        	y = (int) py + labelH + (int) labelLoc.y;
		y1=y+size1;
		y2=y1+(int)(slope*fabs(autoLoc.x+labelLoc.x));
	   }
           GraphicsWin::drawLine(x1,y1,x2,y2, linkColor);
	   x1=markX;
           x2=markX;
           y1=y2;
           y2=markY-size2;
           if(y2>y1 && y2<ymax)
           GraphicsWin::drawLine(x1,y1,x2,y2, linkColor);
	   x1=x;
           x2=x;
           y1=y;
           y2=y1+size1;
           GraphicsWin::drawLine(x1,y1,x2,y2, linkColor);
	}
     }
     x -= (labelW/2);
     AspUtil::drawString((char *)labelStr.c_str(), x, y, -1, fontName,1);
   } else { // horizontal display with triangle connector
     //if((x-labelW/2) < px || (x+labelW/2)>(px+pw)) return;
     int space=4;
     int xsize=8;
     int ysize=4; 
     if(!(peakFlag & PEAK_NOLINK) && (abs(markX-x) >= xsize || abs(markY-y) >= ysize)) { 
	Dpoint_t p1,p2;
	if(markX > (x+labelW/2)) {
           p1.x=x+labelW/2;
        } else if(markX < (x-labelW/2)){
           p1.x=x-labelW/2;
        } else p1.x=x;
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
     x -= (labelW/2);
     AspUtil::drawString((char*)labelStr.c_str(), x, y, -1, fontName);
   }

   labelX=x;
   labelY=y-labelH;
   markX-=(markW/2);
   markY-=(markH/2);

//debug
//AspUtil::drawBox(labelX,labelY,labelW,labelH,2);
//AspUtil::drawBox(markX,markY,markW,markH,2);
}

void AspPeak::display2d(spAspCell_t cell, int xoff, int yoff, spAspDataInfo_t dataInfo, int peakFlag) {

   char fontName[64];
//   int markcolor;
   if(selected == LABEL_SELECTED) {
        getOptName(PEAK_LABEL,fontName);
   } else {
        getOptName(PEAK_NUM,fontName);
   }
//   if(selected == PEAK_SELECTED) markcolor = ACTIVE_COLOR; 
//   else markcolor = PEAK_NUM_COLOR;

   if(dataInfo->haxis.name == cursor->resonances[0].name
		&& dataInfo->vaxis.name == cursor->resonances[1].name) {
	double freq1 = cursor->resonances[0].freq;
	double freq2 = cursor->resonances[1].freq;
        char str[MAXSTR];
	sprintf(str,"%.3f,%.3f",freq1,freq2);
	int x = (int) cell->val2pix(HORIZ,freq1);
	int y = (int) cell->val2pix(VERT,freq2);
        
	AspUtil::drawString(str, x, y, -1, fontName);
   } else if(dataInfo->haxis.name == cursor->resonances[1].name
		&& dataInfo->vaxis.name == cursor->resonances[0].name) {
	double freq1 = cursor->resonances[1].freq;
	double freq2 = cursor->resonances[0].freq;
        char str[MAXSTR];
	sprintf(str,"%.3f,%.3f",freq1,freq2);
	int x = (int) cell->val2pix(HORIZ,freq1);
	int y = (int) cell->val2pix(VERT,freq2);
        
	AspUtil::drawString(str, x, y, -1, fontName);
   }
}

int AspPeak::select(int x, int y) {
   if(AspUtil::select(x,y,markX,markY,markW,markH,2) != NOSELECT) 
	selected=PEAK_SELECTED;
   else if(AspUtil::select(x,y,labelX,labelY,labelW,labelH,2) != NOSELECT) 
	selected=LABEL_SELECTED;
   else selected=0;

//Winfoprintf("x,y,markX,markY,markW,markH,labelX,labelY,labelW,labelH %d %d %d %d %d %d %d %d %d %d %d",x,y,markX,markY,markW,markH,labelX,labelY,labelW,labelH,selected);
   return selected;     
}

void AspPeak::modify(spAspCell_t cell, int x, int y) {

  double px,py,pw,ph;
  cell->getPixCell(px,py,pw,ph);
  if(selected == PEAK_SELECTED) {
      int size=2;
      if((x-size)<px) x= (int)px+size;
      if((x+size)>(px+pw)) x=(int)(px+pw-size);
      if((y-size)<py) y=(int)py+size;
      if((y+size)>(py+ph)) y=(int)(py+ph-size);
      double vx = cell->pix2val(HORIZ,(double)x);
      double vy = cell->pix2val(VERT,(double)y);

      if(rank==1) {
         cursor->resonances[0].freq=vx;
	 height = vy;
      } else if(rank==2) {
         cursor->resonances[0].freq=vx;
         cursor->resonances[1].freq=vy;
      }
  } else if(selected == LABEL_SELECTED) {
      int w2 = labelW/2;
      int h2 = labelH/2;
      if((x-w2)<px) x=(int)px+w2;
      if((x+w2)>(px+pw)) x=(int)(px+pw-w2);
      if((y-h2)<py) y=(int)py+h2;
      if((y+h2)>(py+ph)) y=(int)(py+ph-h2);
      int x0 = labelX+w2;
      int y0 = labelY+h2;
      labelLoc.x += (x-x0);
      labelLoc.y += (y-y0);
  }
}

void AspPeak::setXval(double freq) {
    cursor->resonances[0].freq = freq;
} 

double AspPeak::getXval() {
  return cursor->resonances[0].freq; 
}

double AspPeak::getYval() {
  if(rank==1) return height;
  else if(rank==2)return cursor->resonances[1].freq; 
  else return 0;
}

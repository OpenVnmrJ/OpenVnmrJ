/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <float.h>
#include <math.h>
#include "AspAnno.h"

void AspAnno::init() {
    index=0;
    selected = 0;
    selectedHandle= 0;
    mmbind = false; // true if ROI binds to mm position 
    mmbindY = false; // true if ROI binds to vertical mm position 

    // ROI position
    created_type=ANNO_NONE;
    npts=0;
    sCoord = NULL;
    pCoord = NULL;

    // label position
    label="%val%";
    labelLoc.x=0.0; // lower-left corner? in spectral coordinates 
    labelLoc.y=0.0;
    labelX=labelY=labelW=labelH=0; // upper-left corner in pixel coordinates 

    // default line color, thick ness, label font
    color=ANNO_LINE_COLOR; 	// color ID
    thicknessName="AnnoLine"; 	// thickness keyword in DisplayOptions
    labelFontName = "GraphLabel"; 	// font keyword in DisplayOptions
    linkColor = PEAK_MARK_COLOR;


    // customer line color, thick ness and label font
    fontSize = 0;
    fontName="-";
    fontStyle="-";
    fontColor="-";
    lineColor="-";
    lineThickness=0;

    // display options 
    disFlag=ANN_SHOW_ROI | ANN_SHOW_LABEL;
    rotate=0;
    roundBox=true;
    fillRoi=false;
    transparency=0.0;
    arrows=1;
    amp=0.0;
    vol=0.0;
}

AspAnno::AspAnno() {
  init();
}

AspAnno::AspAnno(char words[MAXWORDNUM][MAXSTR], int nw) {
   init();
   if(nw < 2) return;
   index=atoi(words[0]) - 1;
   created_type = getType(words[1]);
   int count=2;
   // set npts
   switch (created_type) {
    	case ANNO_LINE:
    	case ANNO_ARROW:
    	case ANNO_XBAR:
    	case ANNO_YBAR:
    	case ANNO_INTEG:
    	case ANNO_BAND:
    	case ANNO_BOX:
    	case ANNO_OVAL:
	   npts=2;
	   break;
    	case ANNO_POLYGON:
    	case ANNO_POLYLINE:
	   if(nw>count) {
		npts=atoi(words[count]); count++;
	   } else npts=3; 
	   break;
	case ANNO_POINT:
    	case ANNO_TEXT:
    	case ANNO_PEAK:
	default:
	   npts=1;
	   break;
   } 
   switch (created_type) {
	case ANNO_POINT:
    	case ANNO_PEAK:
	   disFlag = ANN_SHOW_ROI | ANN_SHOW_LABEL | ANN_SHOW_LINK;
	   break;
    	case ANNO_TEXT:
	   disFlag = ANN_SHOW_LABEL;
	   break;
    	case ANNO_XBAR:
    	case ANNO_YBAR:
	   disFlag = ANN_SHOW_ROI | ANN_SHOW_LABEL;
	   break;
    	case ANNO_BOX:
	   disFlag = ANN_SHOW_ROI;
	   roundBox=false;
	   break;
    	case ANNO_LINE:
    	case ANNO_ARROW:
    	case ANNO_INTEG:
    	case ANNO_BAND:
    	case ANNO_OVAL:
    	case ANNO_POLYGON:
    	case ANNO_POLYLINE:
	default:
	   disFlag = ANN_SHOW_ROI;
	   break;
   } 

   if(sCoord) delete[] sCoord;
   if(pCoord) delete[] pCoord;
   sCoord = new Dpoint_t[npts];
   pCoord = new Dpoint_t[npts];

   if(nw < count+2*npts) {
      for(int i=0;i<npts;i++) {
        sCoord[i].x=0.0;
        sCoord[i].y=0.0;
      }
      return;
   }

   // fill sCoord
   if(created_type == ANNO_BAND || created_type == ANNO_INTEG) {
	   for(int i=0;i<npts;i++) {
		sCoord[i].x=atof(words[count]); count++;	
		sCoord[i].y=0.0; // not used
	   }
   } else {
	   for(int i=0;i<npts;i++) {
		sCoord[i].x=atof(words[count]); count++;	
		sCoord[i].y=atof(words[count]); count++;	
	   }
   }

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
   if(nw>count) { labelLoc.x=atof(words[count]); count++; }
   if(nw>count) { labelLoc.y=atof(words[count]); count++; }
   if(nw>count) { disFlag=atoi(words[count]); count++; }
   if(nw>count) { int val=atoi(words[count]);
                  if (val == 1)
                  {
                      mmbind = mmbindY = true;
                  }
                  else
                  {
                      mmbind = false;
                      mmbindY = (val == 2) ? true : false;
                  }
                  count++;
                }
   if(nw>count) { rotate=atoi(words[count]); count++; }
   // customer line color, thickness, label font
   if(nw>count) { lineThickness=atoi(words[count]); count++; }
   if(nw>count) { lineColor=string(words[count]); count++; }
   if(nw>count) { fontSize=atoi(words[count]); count++; }
   if(nw>count) { fontColor=string(words[count]); count++; }
   if(nw>count) { fontName=string(words[count]); count++; }
   if(nw>count) { fontStyle=string(words[count]); count++; }
   if(nw>count) { transparency=atof(words[count]); count++; }

   if(nw>count && created_type == ANNO_BOX) { // for Box only 
	if(atoi(words[count]) > 0) roundBox=true;
	else roundBox=false;
	count++; 
   }
   if(nw>count && ((created_type == ANNO_BOX) ||
                   (created_type == ANNO_POLYGON) ||
                   (created_type == ANNO_OVAL)  )) { // Box, polygon  and Oval
	if(atoi(words[count]) > 0) fillRoi=true;
	else fillRoi=false;
	count++; 
   }
   if(nw>count && created_type == ANNO_ARROW) { // for ARROW 
	arrows=atoi(words[count]); 
	count++;
   }
   if(nw>count && created_type == ANNO_POINT) {  
	amp=atof(words[count]); 
	count++;
   }
   if(nw>count && created_type == ANNO_POINT) {  
	vol=atof(words[count]); 
	count++;
   }
}

AspAnno::~AspAnno() {
  if(sCoord) delete[] sCoord;
  if(pCoord) delete[] pCoord;
}

string AspAnno::toString() {
   string coordStr = "";
   char str[MAXSTR]; 
   int mmbindVal;

   if(created_type == ANNO_BAND || created_type == ANNO_INTEG) {
	   for(int i=0;i<npts;i++) {
		sprintf(str,"%.4g ",sCoord[i].x);
		coordStr += string(str);
	   }
   } else {
	   if(created_type == ANNO_POLYGON || created_type == ANNO_POLYLINE) {
		sprintf(str,"%d ",npts);
		coordStr += string(str);
	   }
	   for(int i=0;i<npts;i++) {
		sprintf(str,"%.4g ",sCoord[i].x);
		coordStr += string(str);
		sprintf(str,"%.4g ",sCoord[i].y);
		coordStr += string(str);
	   }
   }
   mmbindVal = (mmbind && mmbindY) ? 1 : (mmbindY) ? 2 : 0;
   if(created_type == ANNO_POINT)
     sprintf(str,"%d %s %s |%s| %.3g %.3g %d %d %d %d %s %d %s %s %s %.2f %f %f",
        index+1,getName(created_type).c_str(),coordStr.c_str(),label.c_str(), 
	labelLoc.x, labelLoc.y,disFlag,mmbindVal,rotate,
        lineThickness, lineColor.c_str(),fontSize,fontColor.c_str(),
	fontName.c_str(),fontStyle.c_str(),transparency,amp,vol);
   else if(created_type == ANNO_BOX)
     sprintf(str,"%d %s %s |%s| %.3g %.3g %d %d %d %d %s %d %s %s %s %.2f %d %d",
        index+1,getName(created_type).c_str(),coordStr.c_str(),label.c_str(), 
	labelLoc.x, labelLoc.y,disFlag,mmbindVal,rotate,
        lineThickness, lineColor.c_str(),fontSize,fontColor.c_str(),
	fontName.c_str(),fontStyle.c_str(),transparency,(int)roundBox,(int)fillRoi);
   else if ((created_type == ANNO_POLYGON) || (created_type == ANNO_OVAL))
     sprintf(str,"%d %s %s |%s| %.3g %.3g %d %d %d %d %s %d %s %s %s %.2f %d",
        index+1,getName(created_type).c_str(),coordStr.c_str(),label.c_str(), 
	labelLoc.x, labelLoc.y,disFlag,mmbindVal,rotate,
        lineThickness, lineColor.c_str(),fontSize,fontColor.c_str(),
	fontName.c_str(),fontStyle.c_str(),transparency,(int)fillRoi);
   else if(created_type == ANNO_ARROW)
     sprintf(str,"%d %s %s |%s| %.3g %.3g %d %d %d %d %s %d %s %s %s %.2f %d",
        index+1,getName(created_type).c_str(),coordStr.c_str(),label.c_str(), 
	labelLoc.x, labelLoc.y,disFlag,mmbindVal,rotate,
        lineThickness, lineColor.c_str(),fontSize,fontColor.c_str(),
	fontName.c_str(),fontStyle.c_str(),transparency,arrows);
   else 
     sprintf(str,"%d %s %s |%s| %.3g %.3g %d %d %d %d %s %d %s %s %s %.2f",
        index+1,getName(created_type).c_str(),coordStr.c_str(),label.c_str(), 
	labelLoc.x, labelLoc.y,disFlag,mmbindVal,rotate,
        lineThickness, lineColor.c_str(),fontSize,fontColor.c_str(),
	fontName.c_str(),fontStyle.c_str(),transparency);
   return string(str); 
}

string AspAnno::getName(AnnoType_t type) {
   string str=string("NONE");
   switch (type) {
	case ANNO_POINT: str=string("POINT"); break; 
	case ANNO_LINE: str=string("LINE"); break; 
	case ANNO_ARROW: str=string("ARROW"); break; 
	case ANNO_BOX: str=string("BOX"); break; 
	case ANNO_OVAL: str=string("OVAL"); break; 
	case ANNO_TEXT: str=string("TEXT"); break; 
	case ANNO_POLYGON: str=string("POLYGON"); break; 
	case ANNO_POLYLINE: str=string("POLYLINE"); break; 
	case ANNO_XBAR: str=string("XBAR"); break; 
	case ANNO_YBAR: str=string("YBAR"); break; 
	case ANNO_PEAK: str=string("PEAK"); break; 
	case ANNO_INTEG: str=string("INTEG"); break; 
	case ANNO_BAND: str=string("BAND"); break; 
	default: break;
   }
   return str;
}

AnnoType_t AspAnno::getType(char *str) {
   if(strcasecmp(str,"POINT")==0) return ANNO_POINT;
   else if(strcasecmp(str,"LINE")==0) return ANNO_LINE;
   else if(strcasecmp(str,"ARROW")==0) return ANNO_ARROW;
   else if(strcasecmp(str,"BOX")==0) return ANNO_BOX;
   else if(strcasecmp(str,"OVAL")==0) return ANNO_OVAL;
   else if(strcasecmp(str,"TEXT")==0) return ANNO_TEXT;
   else if(strcasecmp(str,"POLYGON")==0) return ANNO_POLYGON;
   else if(strcasecmp(str,"POLYLINE")==0) return ANNO_POLYLINE;
   else if(strcasecmp(str,"XBAR")==0) return ANNO_XBAR;
   else if(strcasecmp(str,"YBAR")==0) return ANNO_YBAR;
   else if(strcasecmp(str,"PEAK")==0) return ANNO_PEAK;
   else if(strcasecmp(str,"INTEG")==0) return ANNO_INTEG;
   else if(strcasecmp(str,"BAND")==0) return ANNO_BAND;
   else return ANNO_NONE;
}

// this will be overwriten by each anno type.
// it will fill pCoord, labelX,labelY,labelW,labelH etc...
void AspAnno::display(spAspCell_t cell, spAspDataInfo_t dataInfo) {
}

int AspAnno::select(int x, int y) {
  selected = selectHandle(x,y);
  if(!selected) selected = selectLabel(x,y);
  if(!selected && npts>1) { // select object by selecting one of the line.
	int px,py;
	for(int i=0; i<npts; i++) {
	   if(i==0) {
		px=(int)pCoord[npts-1].x;
		py=(int)pCoord[npts-1].y;
	   } else {
		px=(int)pCoord[i-1].x;
		py=(int)pCoord[i-1].y;
	   }
	   if(AspUtil::selectLine(x,y,px,py,(int)pCoord[i].x, (int)pCoord[i].y) == LINE1) {
		selected=ROI_SELECTED;
		break;
	   }
        }
  }
  return selected;
}

int AspAnno::selectHandle(int x, int y) {

  if(created_type == ANNO_BAND) y=0; // not used;

  selected=0;
  selectedHandle=0;
  // when anno first created, all points are on top of each other.
  // force to select the second handle to modify 
  int n1=npts-1;
  int n2=npts-2;
  if(npts > 1 && pCoord[n2].x==pCoord[n1].x && pCoord[n2].y==pCoord[n1].y) {
      if(fabs(pCoord[n2].x-x)+fabs(pCoord[n2].y-y) < MARKSIZE) {
	selectedHandle=npts;
	selected=HANDLE_SELECTED;
        return selected;
      }
  }
  
  if(disFlag & ANN_SHOW_ROI) {
     for(int i=0; i<npts; i++) {
        if(fabs(pCoord[i].x-x)+fabs(pCoord[i].y-y) < MARKSIZE) {
	   selectedHandle = i+1;
	   selected=HANDLE_SELECTED;
           return selected;
	}
     }
  } 
  return selected;
}

int AspAnno::selectLabel(int x, int y) {
  selected=0;
  if(disFlag & ANN_SHOW_LABEL) {
   if(AspUtil::select(x,y,labelX,labelY,labelW,labelH,2) != NOSELECT) selected=LABEL_SELECTED;
  }

//Winfoprintf("x,y,roiX,roiY,roiW,roiH,labelX,labelY,labelW,labelH %d %d %d %d %d %d %d %d %d %d %d %d",x,y,roiX,roiY,roiW,roiH,labelX,labelY,labelW,labelH,selected,selectedHandle);
   
//AspUtil::drawBox(roiX,roiY,roiW,roiH,YELLOW);
//AspUtil::drawBox(labelX,labelY,labelW,labelH,YELLOW);
   return selected;
}

void AspAnno::modify(spAspCell_t cell, int x, int y, int prevX, int prevY) {

   if(npts<1) return;
   if(selected == HANDLE_SELECTED) {
        int i=selectedHandle-1;
	if(i>=0 && i<npts) { 
          pCoord[i].x=x;
          pCoord[i].y=y;
          sCoord[i].x=cell->pix2val(HORIZ,x,mmbind);
          sCoord[i].y=cell->pix2val(VERT,y,mmbindY);
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

void AspAnno::getLabel(spAspDataInfo_t dataInfo, string &lbl, int &cwd, int &cht) {

   string thisLabel=AspUtil::subParamInStr(label);

   bool two = (dataInfo->vaxis.name != "amp");
   char str[MAXSTR],tmpStr[MAXSTR];
   if(two) sprintf(str,"%.2f,%.2f",sCoord[0].x, sCoord[0].y);
   else sprintf(str,"%.3f",sCoord[0].x);
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
	 bool am = (strcasecmp(tmpStr,"amp") == 0);
	 bool vo = (strcasecmp(tmpStr,"vol") == 0);
	 bool ht = (strcasecmp(tmpStr,"ht") == 0);
	 if(hz && two) { // 2D
		sprintf(str,"%.2f,%.2f",dataInfo->haxis.scale*sCoord[0].x,
			dataInfo->vaxis.scale*sCoord[0].y);
	 } else if(hz) { // 1D
		sprintf(str,"%.2f",dataInfo->haxis.scale*sCoord[0].x);
	 } else if(am) {
                sprintf(str,"%.4f",amp);
	 } else if(vo) { 
                sprintf(str,"%.4f",vol);
	 } else if(ht) {
                sprintf(str,"%.4f",sCoord[0].y);
	 }
         thisLabel = thisLabel.substr(0,p1) + string(str) + thisLabel.substr(p2+1);
	 p1=thisLabel.find("%",p2+1);
	} else p1 = string::npos;
      }  
      lbl = string(thisLabel);
   }

   getStringSize(lbl,cwd,cht);

   return;
}

void AspAnno::resetProperties() {
    fontSize = 0;
    fontName="-";
    fontStyle="-";
    fontColor="-";
    lineColor="-";
    lineThickness=0;

    rotate=0;
    roundBox=true;
    fillRoi=false;
    transparency=0.0;
}

void AspAnno::setRoiColor(int &roiColor, int &thick) {
   // set line color
   if(selected == ROI_SELECTED) {
        roiColor = ACTIVE_COLOR;
   } else if(lineColor.length()>1) {
        roiColor = set_anno_color((char *)lineColor.c_str());
   } else roiColor = color;

   // set thickness
   if(lineThickness>0) {
	thick = lineThickness;
   } else {
	string value="1";
	AspUtil::getDisplayOption(thicknessName+"Thickness",value); 
	thick = atoi(value.c_str());
   }
   set_spectrum_width(thick);
   set_line_width(thick);
}

void AspAnno::setFont(int &labelColor) {

   if(fontSize > 0 || fontName.length()>1 || fontStyle.length()>1 || fontColor.length()>1) {
	int size;
	string name, style, clr;
	if(fontSize>0) size=fontSize;
	else {
	  string value;
	  AspUtil::getDisplayOption(labelFontName+"Size",value);
	  size = atoi(value.c_str());
	}
	if(fontName.length()>1) name=fontName;
	else AspUtil::getDisplayOption(labelFontName+"Font",name);
	if(fontStyle.length()>1) style=fontStyle;
	else AspUtil::getDisplayOption(labelFontName+"Style",style);
	if(fontColor.length()>1) clr=fontColor;
	else AspUtil::getDisplayOption(labelFontName+"Color",clr);
        set_anno_font((char *)name.c_str(), (char *)style.c_str(), (char *)clr.c_str(), size);

   } else {
        set_graphics_font((char *)labelFontName.c_str());
   }

   if(selected == LABEL_SELECTED) {
        labelColor = ACTIVE_COLOR;
   } else {
        labelColor = -1;
   }
}

void AspAnno::getStringSize(string str, int &wd, int &ht) {

   int ascent, descent, cwd;
   int charSize = fontSize;
   if(charSize < 1) {
      string value;
      AspUtil::getDisplayOption(labelFontName+"Size",value); 
      charSize = atoi(value.c_str());
   }

   GraphicsWin::getTextExtents(str.c_str(), charSize, &ascent, &descent, &cwd);

   if(rotate) {
     ht = cwd;
     wd = ascent + descent;
   } else {
     wd = cwd;
     ht = ascent + descent;
   }

   return;
}

void AspAnno::setProperty(char *name, char *value, spAspCell_t cell) {
//Winfoprintf("setProperty %s %s",name,value);
   if(strcasecmp(name,"label") == 0) label = string(value);
   else if(strcasecmp(name,"lineColor") == 0) lineColor = string(value);
   else if(strcasecmp(name,"lineThickness") == 0) lineThickness = atoi(value);
   else if(strcasecmp(name,"fontColor") == 0) fontColor = string(value);
   else if(strcasecmp(name,"fontSize") == 0) fontSize = atoi(value);
   else if(strcasecmp(name,"fontName") == 0) fontName = string(value);
   else if(strcasecmp(name,"fontStyle") == 0) fontStyle = string(value);
   else if(strcasecmp(name,"vert") == 0) {
	if(atoi(value) > 0) rotate = 1;
	else rotate = 0;
   }
   else if(strcasecmp(name,"roundBox") == 0) {
	if(atoi(value) > 0) roundBox=true;
	else roundBox=false;
   }
   else if(strcasecmp(name,"fill") == 0) {
	if(atoi(value) > 0) fillRoi=true;
	else fillRoi=false;
   }
   else if(strcasecmp(name,"transparency") == 0) transparency = atof(value)/100.0;
   else if(strcasecmp(name,"arrows") == 0) arrows = atoi(value);
   else if(strcasecmp(name,"amp") == 0) amp = atof(value);
   else if(strcasecmp(name,"vol") == 0) vol = atof(value);
   else if(cell != nullAspCell && strcasecmp(name,"mm") == 0) {
       int mmbindVal = atoi(value);
       if (mmbindVal == 1)
       {
           mmbind = mmbindY = true;
       }
       else
       {
           mmbind = false;
           mmbindY = (mmbindVal == 2) ? true : false;
       }
      for(int i=0; i<npts; i++) {
	     sCoord[i].x = cell->pix2val(HORIZ,pCoord[i].x,mmbind);
	     sCoord[i].y = cell->pix2val(VERT,pCoord[i].y,mmbindY);
      }
   }
   else if(strcasecmp(name,"showRoi") == 0 && atoi(value) > 0) disFlag |= ANN_SHOW_ROI;
   else if(strcasecmp(name,"showRoi") == 0) disFlag &= ~ANN_SHOW_ROI;
   else if(strcasecmp(name,"showLabel") == 0 && atoi(value) > 0) disFlag |= ANN_SHOW_LABEL;
   else if(strcasecmp(name,"showLabel") == 0) disFlag &= ~ANN_SHOW_LABEL;
   else if(strcasecmp(name,"showLink") == 0 && atoi(value) > 0) disFlag |= ANN_SHOW_LINK;
   else if(strcasecmp(name,"showLink") == 0) disFlag &= ~ANN_SHOW_LINK;
   else if(strcasecmp(name,"x") == 0) setX(cell, atof(value));
   else if(strcasecmp(name,"y") == 0) setY(cell, atof(value));
   else if(strcasecmp(name,"w") == 0) setW(cell, atof(value));
   else if(strcasecmp(name,"h") == 0) setH(cell, atof(value));
   else if(strstr(name,"x") == name) {
	int i = atoi(++name) - 1;
	if(i<npts) {
	   sCoord[i].x=atof(value);
           pCoord[i].x=cell->val2pix(HORIZ,sCoord[i].x,mmbind);
        }
   } else if(strstr(name,"y") == name) {
	int i = atoi(++name) - 1;
	if(i<npts) {
	   sCoord[i].y=atof(value);
           pCoord[i].y=cell->val2pix(VERT,sCoord[i].y,mmbindY);
        }
   }
}

string AspAnno::getProperty(char *name) {
   char str[64];
//Winfoprintf("getProperty %s",name);
   if(strcasecmp(name,"type") == 0) {
        return getName(created_type);
   } else if(strcasecmp(name,"label") == 0) {
	return label;
   } else if(strcasecmp(name,"lineColor") == 0) {
	if(lineColor.length()>1) return lineColor;
	else {
	  string value="";
	  AspUtil::getDisplayOption(string("graphics320")+"Color",value);
	  return value;
	}
   } else if(strcasecmp(name,"lineThickness") == 0) {
	if(lineThickness>0) {
	  sprintf(str,"%d",lineThickness);	
	  return string(str);
	} else {
	  string value="1";
	  AspUtil::getDisplayOption(thicknessName+"Thickness",value); 
	  return value;
	}
   } else if(strcasecmp(name,"fontColor") == 0) {
	if(fontColor.length()>1) return fontColor;
	else {
	  string value="";
	  AspUtil::getDisplayOption(labelFontName+"Color",value);
	  return value;
	}
   } else if(strcasecmp(name,"fontSize") == 0) {
	if(fontSize>0) {
	  sprintf(str,"%d",fontSize);	
	  return string(str);
	} else {
	  string value="12";
	  AspUtil::getDisplayOption(labelFontName+"Size",value);
	  return value;
	}
   } else if(strcasecmp(name,"fontName") == 0) {
	if(fontName.length()>1) return fontName;
	else {
	  string value="";
	  AspUtil::getDisplayOption(labelFontName+"Font",value);
	  return value;
	}
   } else if(strcasecmp(name,"fontStyle") == 0) {
	if(fontStyle.length()>1) return fontStyle;
	else {
	  string value="";
	  AspUtil::getDisplayOption(labelFontName+"Style",value);
	  return value;
	}
   } else if(strcasecmp(name,"vert") == 0) {
	if(rotate) sprintf(str,"1");	
	else sprintf(str,"0");
	return string(str);
   } else if(strcasecmp(name,"roundBox") == 0) {
	if(roundBox) sprintf(str,"1");	
	else sprintf(str,"0");
	return string(str);
   } else if(strcasecmp(name,"mm") == 0) {
	if(mmbind && mmbindY) sprintf(str,"1");	
	else if (mmbindY) sprintf(str,"2");
	else sprintf(str,"0");
	return string(str);
   } else if(strcasecmp(name,"fill") == 0) {
	if(fillRoi) sprintf(str,"1");	
	else sprintf(str,"0");
	return string(str);
   } else if(strcasecmp(name,"transparency") == 0) {
	sprintf(str,"%f",100.0*transparency);
	return string(str);
   } else if(strcasecmp(name,"arrows") == 0) {
	sprintf(str,"%d",arrows);
	return string(str);
   } else if(strcasecmp(name,"amp") == 0) {
	sprintf(str,"%f",amp);
	return string(str);
   } else if(strcasecmp(name,"vol") == 0) {
	sprintf(str,"%f",vol);
	return string(str);
   } else if(strcasecmp(name,"showRoi") == 0) {
	if(disFlag & ANN_SHOW_ROI) sprintf(str,"1");	
	else sprintf(str,"0");
	return string(str);
   } else if(strcasecmp(name,"showLabel") == 0) {
	if(disFlag & ANN_SHOW_LABEL) sprintf(str,"1");	
	else sprintf(str,"0");
	return string(str);
   } else if(strcasecmp(name,"showLink") == 0) {
	if(disFlag & ANN_SHOW_LINK) sprintf(str,"1");	
	else sprintf(str,"0");
	return string(str);
   } else if(strcasecmp(name,"x") == 0) { 
	sprintf(str,"%f",getX());
	return string(str);
   } else if(strcasecmp(name,"y") == 0) { 
	sprintf(str,"%f",getY());
	return string(str);
   } else if(strcasecmp(name,"w") == 0) { 
	sprintf(str,"%f",getW());
	return string(str);
   } else if(strcasecmp(name,"h") == 0) { 
	sprintf(str,"%f",getH());
	return string(str);
   } else if(strstr(name,"x") == name) { // xN where N is 1 to npts
	int i = atoi(++name) - 1;
	if(i<npts) {
	   sprintf(str,"%f",sCoord[i].x);
	   return string(str);
	} else return string("0");
   } else if(strstr(name,"y") == name) { // yN
	int i = atoi(++name) - 1;
	if(i<npts) {
	   sprintf(str,"%f",sCoord[i].y);
	   return string(str);
	} else return string("0");
   } else return string("");
}

void AspAnno::addPoint(spAspCell_t cell, int x, int y) {
  if(created_type == ANNO_POLYGON || created_type == ANNO_POLYLINE) {
	Dpoint_t stmp[npts],ptmp[npts];
	for(int i=0; i<npts; i++) {
	   stmp[i].x=sCoord[i].x;
	   stmp[i].y=sCoord[i].y;
	   ptmp[i].x=pCoord[i].x;
	   ptmp[i].y=pCoord[i].y;
	}	

        if(sCoord) delete[] sCoord;
	if(pCoord) delete[] pCoord;
 	int n = npts;
	npts++;
	sCoord = new Dpoint_t[npts];
   	pCoord = new Dpoint_t[npts];
	for(int i=0; i<n; i++) {
	   sCoord[i].x=stmp[i].x;
	   sCoord[i].y=stmp[i].y;
	   pCoord[i].x=ptmp[i].x;
	   pCoord[i].y=ptmp[i].y;
	}	
	pCoord[n].x=x;
	pCoord[n].y=y;
	sCoord[n].x=cell->pix2val(HORIZ,x,mmbind);
	sCoord[n].y=cell->pix2val(VERT,y,mmbindY);;
	selected=HANDLE_SELECTED;
	selectedHandle=npts;
  }
}

void AspAnno::insertPoint(spAspCell_t cell, int x, int y) {
  if(created_type == ANNO_POLYGON || created_type == ANNO_POLYLINE) {
	Dpoint_t stmp[npts],ptmp[npts];
	int ind=-1;
	for(int i=0; i<npts; i++) {
	   stmp[i].x=sCoord[i].x;
	   stmp[i].y=sCoord[i].y;
	   ptmp[i].x=pCoord[i].x;
	   ptmp[i].y=pCoord[i].y;
	   if(ind < 0) {
	      if(i==(npts-1)) {
		if(created_type == ANNO_POLYGON) {
	          if(AspUtil::selectLine(x,y,(int)pCoord[i].x,(int)pCoord[i].y,
			(int)pCoord[0].x,(int)pCoord[0].y) == LINE1) ind=0;
		}
	      } else {
	        if(AspUtil::selectLine(x,y,(int)pCoord[i].x,(int)pCoord[i].y,
			(int)pCoord[i+1].x,(int)pCoord[i+1].y) == LINE1) ind=i+1;
	      }
	   }
	}	

	if(ind<0) return;

        if(sCoord) delete[] sCoord;
	if(pCoord) delete[] pCoord;
 	int n = npts;
	npts++;
	sCoord = new Dpoint_t[npts];
   	pCoord = new Dpoint_t[npts];
	int k=0;
	for(int i=0; i<n; i++) {
	   if(i==ind) {
		pCoord[k].x=x;
		pCoord[k].y=y;
		sCoord[k].x=cell->pix2val(HORIZ,x,mmbind);
		sCoord[k].y=cell->pix2val(VERT,y,mmbindY);;
		k++;
	   }
	   sCoord[k].x=stmp[i].x;
	   sCoord[k].y=stmp[i].y;
	   pCoord[k].x=ptmp[i].x;
	   pCoord[k].y=ptmp[i].y;
	   k++;
	}	
	selected=HANDLE_SELECTED;
	selectedHandle=ind+1;
  }
}
void AspAnno::deletePoint(spAspCell_t cell, int x, int y) {
  if(created_type == ANNO_POLYGON || created_type == ANNO_POLYLINE) {
  	int ind=selectedHandle-1;
  	if(ind<0 || ind >= npts) return;

	Dpoint_t stmp[npts],ptmp[npts];
	for(int i=0; i<npts; i++) {
	   stmp[i].x=sCoord[i].x;
	   stmp[i].y=sCoord[i].y;
	   ptmp[i].x=pCoord[i].x;
	   ptmp[i].y=pCoord[i].y;
	}	

        if(sCoord) delete[] sCoord;
	if(pCoord) delete[] pCoord;
 	int n = npts;
	npts--;
	sCoord = new Dpoint_t[npts];
   	pCoord = new Dpoint_t[npts];
        int k=0;
	for(int i=0; i<n; i++) {
	   if(i == ind) continue;	   
	   sCoord[k].x=stmp[i].x;
	   sCoord[k].y=stmp[i].y;
	   pCoord[k].x=ptmp[i].x;
	   pCoord[k].y=ptmp[i].y;
	   k++;
	}	
	selected=0;
	selectedHandle=0;
  }

}

// x,y define location of anno, are center of sCoords
// w,y define size of anno, are total width and height of sCoords
void AspAnno::setX(spAspCell_t cell, double x) {
   if(npts<1) return;
   if(npts<2) {
        sCoord[0].x=x;
        pCoord[0].x=cell->val2pix(HORIZ,x,mmbind);
	return;
   }

   double cx=getX();
   cx = x-cx;
   for(int i=0; i<npts; i++) {
      sCoord[i].x += cx;
      pCoord[i].x=cell->val2pix(HORIZ,sCoord[i].x,mmbind);
   }
}

void AspAnno::setY(spAspCell_t cell, double y) {
   if(npts<1) return;
   if(npts<2) {
        sCoord[0].y=y;
        pCoord[0].y=cell->val2pix(VERT,y,mmbindY);
	return;
   }

   double cy=getY();
   cy = y-cy;
   for(int i=0; i<npts; i++) {
      sCoord[i].y += cy;
      pCoord[i].y=cell->val2pix(VERT,sCoord[i].y,mmbindY);
   }
}

void AspAnno::setW(spAspCell_t cell, double w) {
   if(npts<2 || w == 0) return;
   double wd = getW();
   if(wd == 0) return;
   double cx = getX();
   for(int i=0; i<npts; i++) {
      sCoord[i].x= (sCoord[i].x-cx)*w/wd + cx;
   }
}

void AspAnno::setH(spAspCell_t cell, double h) {
   if(npts<2 || h == 0) return;
   double ht = getH();
   if(ht == 0) return;
   double cy = getY();
   for(int i=0; i<npts; i++) {
      sCoord[i].y= (sCoord[i].y-cy)*h/ht + cy;
   }
}

double AspAnno::getX() {
   if(npts<1) return 0.0;
   if(npts<2) return sCoord[0].x;
   double cx=0;
   for(int i=0; i<npts; i++) {
       cx += sCoord[i].x;
   }
   cx /= npts;
   return cx;
}
double AspAnno::getY() {
   if(npts<1) return 0.0;
   if(npts<2) return sCoord[0].y;
   double cy=0;
   for(int i=0; i<npts; i++) {
       cy += sCoord[i].y;
   }
   cy /= npts;
   return cy;
}
double AspAnno::getW() {
   if(npts<2) return 0.0;
   double xmax=-0.1*FLT_MAX;
   double xmin=FLT_MAX;
   for(int i=0; i<npts; i++) {
	if(sCoord[i].x < xmin) xmin=sCoord[i].x;
	if(sCoord[i].x > xmax) xmax=sCoord[i].x;
   }
   return (xmax-xmin);
}
double AspAnno::getH() {
   if(npts<2) return 0.0;
   double ymax=-0.1*FLT_MAX;
   double ymin=FLT_MAX;
   for(int i=0; i<npts; i++) {
	if(sCoord[i].y < ymin) ymin=sCoord[i].y;
	if(sCoord[i].y > ymax) ymax=sCoord[i].y;
   }
   return (ymax-ymin);
}

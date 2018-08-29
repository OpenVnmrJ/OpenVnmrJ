/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */


#include <math.h>

#include "AspUtil.h"
#include "AspRoi.h"
#include "AspFrameMgr.h"
#include "AspCellList.h"

spAspRoi_t nullAspRoi = spAspRoi_t(NULL);

// assume c1 is downfield cursor (bigger ppm) and 
// c2 is upfield cursor (smaller ppm, i.e.,cr-delta).
AspRoi::AspRoi(spAspCursor_t c1, spAspCursor_t c2, int i) {
    id=i;
    label="";
    cursor1=c1;
    cursor2=c2;
    color=(int)getReal("aspPref",1,-1);
    opaque=(int)getReal("aspPref",2,-1);
    height=(int)getReal("aspPref",3,-1);
    selected=false;
    mouseOver=NOSELECT;
    show=true;
}

AspRoi::~AspRoi() {
}

void AspRoi::setColor(char *name) {
   if(isdigit(name[0])) color=atoi(name);
   else if(!colorindex(name, &color)) color=-1;
}

int AspRoi::getColor() {
  if(color<0) return ANNO_LINE_COLOR;
  else return color;
}

int AspRoi::getRank() {
   return (cursor1->rank > cursor2->rank) ? cursor2->rank : cursor1->rank; 
}

bool AspRoi::select(spAspCell_t cell, int x, int y, bool handle)
{
    if(cell == nullAspCell) return false;

        int oldmouseOver = mouseOver;
        mouseOver = NOSELECT;

        selectHandle(cell,x,y, handle);

        if(oldmouseOver != mouseOver) mouseOverChanged=true;
        else mouseOverChanged=false;
        if(mouseOver != NOSELECT) return true;
	else return false;
}

// handles start from upper left corner and clockwise
void AspRoi::selectHandle(spAspCell_t cell, int x, int y, bool handle)
{
    if(cell == nullAspCell) return;

        int h=HANDLE_SIZE;
        double px,py,pw,ph;

	// return if mouse near cell border.
        cell->getPixCell(px,py,pw,ph);
	if(x<px+h || x>(px+pw-h) || y<py+h || y>(py+ph-h)) return;

        if(!getRoiBox(cell,px,py,pw,ph)) return;

        int rank = getRank();
        mouseOver = AspUtil::select(x,y,px,py,pw,ph,rank, handle);
//Winfoprintf("selectRoiHandle %d %d %d %d %d %d",mouseOver,x,y,px,py,xmax,ymax);

}

void AspRoi::modify(spAspCell_t cell, int x, int y, int prevx, int prevy)
{
    if(cell == nullAspCell) return;

    double vx = cell->pix2val(HORIZ,x); 
    double vy = cell->pix2val(VERT,y); 
    double vxPrev = cell->pix2val(HORIZ,prevx);
    double vyPrev = cell->pix2val(VERT,prevy);

    mouseOverChanged=true;
    modifyRoi(vx,vy,vxPrev,vyPrev);

}

// cursor1 is lower left, cursor2 is upper right
bool AspRoi::getRoiBox(spAspCell_t cell,double &px, double &py, double &pw, double &ph) {

    if(cell == nullAspCell) return false;

  double vx,vw,vy,vh;
  double pstx, psty, pwd, pht;
  double vstx, vsty, vwd, vht;
  cell->getPixCell(pstx, psty, pwd, pht);
  cell->getValCell(vstx, vsty, vwd, vht);

  // before asign vx,vw,vy,vh, make the following swapping if needed.
  Dpoint_t c1,c2;
  int rank = getRank();
  if(rank==2) {
    
    string xname = cell->getXname();
    string yname = cell->getYname();

    // swap resonances[0] and resonances[1] to match xname, yname if needed
    if(cursor1->resonances[0].name != xname) { // resonances[0] has to be yname
	if(cursor1->resonances[0].name == yname && cursor1->resonances[1].name == xname) { 
	     // swap resonances[0] resonances[1]
	   double tmp=cursor1->resonances[0].freq;
	   string tmpstr=cursor1->resonances[0].name;
	   cursor1->resonances[0].freq=cursor1->resonances[1].freq;
	   cursor1->resonances[0].name=cursor1->resonances[1].name; 
	   cursor1->resonances[1].freq=tmp;
	   cursor1->resonances[1].name=tmpstr;
	   cursor2->resonances[0].freq=cursor1->resonances[0].freq;
	   cursor2->resonances[0].name=cursor1->resonances[0].name; 
	   cursor2->resonances[1].freq=cursor1->resonances[1].freq;
	   cursor2->resonances[1].name=cursor1->resonances[1].name; 
	}
    } 

    if(cursor1->resonances[0].name != xname || cursor1->resonances[1].name != yname) {
/*
	Winfoprintf("Mismatched nucleus names: %s to %s %s to %s.",
		cursor1->resonances[0].name.c_str(), xname.c_str(),
		cursor1->resonances[1].name.c_str(), yname.c_str());
*/
	return false; 
    }

    // swap cursor1 cursor1 so cursor1 is lower left, and cursor2 is upper right
    c1.x = cell->val2pix(HORIZ, cursor1->resonances[0].freq);
    c1.y = cell->val2pix(VERT, cursor1->resonances[1].freq);
    c2.x = cell->val2pix(HORIZ, cursor2->resonances[0].freq);
    c2.y = cell->val2pix(VERT, cursor2->resonances[1].freq);
    if(c1.x>c2.x) {
        double tmp=cursor1->resonances[0].freq;
        cursor1->resonances[0].freq=cursor2->resonances[0].freq;
        cursor2->resonances[0].freq=tmp;
    }
    if(c2.y>c1.y) {
        double tmp=cursor1->resonances[1].freq;
        cursor1->resonances[1].freq=cursor2->resonances[1].freq;
        cursor2->resonances[1].freq=tmp;
    }
    vx=cursor1->resonances[0].freq;
    vw=cursor1->resonances[0].freq-cursor2->resonances[0].freq;
    vy=cursor2->resonances[1].freq;
    vh=cursor1->resonances[1].freq-cursor2->resonances[1].freq;
  } else if(rank == 1) {

    string xname = cell->getXname();
    if(cursor1->resonances[0].name != xname) {
/*
	Winfoprintf("Mismatched nucleus names: %s %s.",
		cursor1->resonances[0].name.c_str(),xname.c_str());
*/
	return false; 
    }

    c1.x = cell->val2pix(HORIZ, cursor1->resonances[0].freq);
    c2.x = cell->val2pix(HORIZ, cursor2->resonances[0].freq);
    if(c1.x>c2.x) {
        // swap cursor1 cursor1 so cursor1 is left, and cursor2 is right
        double tmp=cursor1->resonances[0].freq;
        cursor1->resonances[0].freq=cursor2->resonances[0].freq;
        cursor2->resonances[0].freq=tmp;
    }
    vx=cursor1->resonances[0].freq;
    vw=cursor1->resonances[0].freq-cursor2->resonances[0].freq;
    double percentH;
    if(height>0) percentH = (double)height/100.0;
    else {
	percentH = getReal("aspPref",3,70);
	if(percentH>0) percentH/=100.0;
	else percentH=0.7;
    }
    if(vht<0)
    vy=vsty-percentH*vht;
    else
    vy=vsty-(1.0-percentH)*vht;
    vh=percentH*vht;

  } else return false;

// note, vy is lower left, py is upper left 
  double vp;
  px = cell->val2pix(HORIZ, vx);
  py = cell->val2pix(VERT, vy);
  if(rank==1) {
     P_getreal(CURRENT,"vp",&vp,1);
     py -= cell->getCali(VERT)*vp;
  }
  pw = fabs(vw*pwd/vwd);
  ph = fabs(vh*pht/vht);
//Winfoprintf("roi vx,vy,vw,vh,px,py,pw,ph %f %f %f %f %f %f %f %f",vx,vy,vw,vh,px,py,pw,ph); 

  return true;

}

// aspPref[1] color id
// aspPref[2] transparent to opaque, 0-100% opaque, default 10
// aspPref[3] 1D Roi height, 0-100% of wc2, default 70
void AspRoi::draw(spAspCell_t cell) {

    if(cell == nullAspCell) return;

    double px,py,pw,ph;
    if(!getRoiBox(cell,px,py,pw,ph)) return;

    int op;
    if(opaque<0) op = (int)getReal("aspPref",2,20);
    else op = opaque;
    int color = getColor();
    if(opaque > 0) 
      set_background_region((int)px, (int)py, (int)pw, (int)ph, color, op);
/*
    if(selected) {
      AspUtil::drawBox(px,py,pw,ph,SELECT_COLOR);
    } else {
*/
      AspUtil::drawBox(px,py,pw,ph,color);
 //   }

    // highlight selection
    if(mouseOver==BOXSELECT) {
      AspUtil::drawBox(px,py,pw,ph,ACTIVE_COLOR);
    } else if(mouseOver>=LINE1 && mouseOver<=LINE4) {
      AspUtil::drawBorderLine(mouseOver,px,py,pw,ph,ACTIVE_COLOR);
    } else if(mouseOver>=HANDLE1 && mouseOver<=HANDLE4) {
      AspUtil::drawHandle(mouseOver,px,py,pw,ph,ACTIVE_COLOR);
    }
}

void AspRoi::modifyRoi(double vx, double vy, double vxPrev, double vyPrev) 
{
   int rank = getRank();
   double dx = (vx-vxPrev);
   double dy = (vy-vyPrev);
   if(rank == 2) { // modify box 
      if(mouseOver == BOXSELECT) { // move 
	   cursor1->resonances[0].freq += dx;
	   cursor2->resonances[0].freq += dx;
	   cursor1->resonances[1].freq += dy;
	   cursor2->resonances[1].freq += dy;
	
      } else if(mouseOver == LINE1) { // resize 
     	   cursor1->resonances[0].freq += dx;
      } else if(mouseOver == LINE2) { 
     	   cursor2->resonances[0].freq += dx;
      } else if(mouseOver == LINE3) {
     	   cursor2->resonances[1].freq += dy;
      } else if(mouseOver == LINE4) { 
     	   cursor1->resonances[1].freq += dy;
      } else if(mouseOver == HANDLE1) { 
     	   cursor1->resonances[0].freq += dx;
     	   cursor2->resonances[1].freq += dy;
      } else if(mouseOver == HANDLE2) { 
     	   cursor2->resonances[0].freq += dx;
     	   cursor2->resonances[1].freq += dy;
      } else if(mouseOver == HANDLE3) { 
     	   cursor2->resonances[0].freq += dx;
     	   cursor1->resonances[1].freq += dy;
      } else if(mouseOver == HANDLE4) { 
     	   cursor1->resonances[0].freq += dx;
     	   cursor1->resonances[1].freq += dy;
      }
   } else if(rank == 1) { // modify band
      if(mouseOver == BOXSELECT) { // move 
	   cursor1->resonances[0].freq += dx;
	   cursor2->resonances[0].freq += dx;
      } else if(mouseOver == LINE1) {
	   cursor1->resonances[0].freq += dx;	
      } else if(mouseOver == LINE2) {
	   cursor2->resonances[0].freq += dx;	
      } 
   }
}

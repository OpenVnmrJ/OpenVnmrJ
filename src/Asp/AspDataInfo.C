/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <cmath>

using namespace std;

#include <limits.h>
#include <float.h>

#include "AspDataInfo.h"
#include "aipSpecDataMgr.h"
#include "AspMouse.h"

extern "C" {
#include "data.h"
#include "init2d.h"
#include "init_display.h"
}

spAspDataInfo_t nullAspData = spAspDataInfo_t(NULL);

AspDataInfo::AspDataInfo() {
    initDataInfo();
}

AspDataInfo::~AspDataInfo() {
}

void AspDataInfo::initDataInfo() {
	rank=0;
        hasData = false;
 	haxis.name="?";
	haxis.label="?";
	haxis.dunits="?";
	haxis.units="?";
	haxis.scale=1.0;
	haxis.maxwidth=0.0;
	haxis.minfirst=0.0;
	haxis.width=0.0;
	haxis.start=0.0;
	haxis.rev=0;
	haxis.npts=0;
	haxis.orient=0;
	haxis.ydataMax=-1.0;
	haxis.ydataMin=0.0;

 	vaxis.name="?";
	vaxis.label="?";
	vaxis.dunits="?";
	vaxis.units="?";
	vaxis.scale=1.0;
	vaxis.maxwidth=0.0;
	vaxis.minfirst=0.0;
	vaxis.width=0.0;
	vaxis.start=0.0;
	vaxis.rev=0;
	vaxis.npts=0;
	vaxis.orient=0;
	vaxis.ydataMax=-1.0;
	vaxis.ydataMin=0.0;
}

void AspDataInfo::updateDataInfo() {

	char str[20];
        Wgetgraphicsdisplay(str, 20);

        if (str[0] == '\0')
           return;

        int procdim = (int) AspUtil::getReal("procdim",1);
        if(procdim > 0) rank = procdim;
	else rank=1;

	hasData = false;
	if(rank > 1) {
	   if(!select_init(1,1,0,1,1,1,0,0)) {
		hasData = true;
		if(strstr(str,"dconi") == str) {
		   initAxis(HORIZ, &haxis);	
                   initAxis(VERT, &vaxis);
		} else {
           	   initAxis(HORIZ, &haxis);
           	   init1D_vaxis();
		}
	   } else {
		Winfoprintf("2D spectral FOV not defined."); 
	   }
	   return;
	}

	if(strstr(str,"ds") == str) { 
	   hasData = true;
           initAxis(HORIZ, &haxis);
           init1D_vaxis();
	} else if(strstr(str,"df") == str) {
	   hasData = true;
           initAxis(HORIZ, &haxis);
           init1D_vaxis();
        } else if(select_init(1,1,0,0,0,1,0,0) == 0) {
   	   initAxis(HORIZ, &haxis);
           init1D_vaxis();

	   if(vaxis.npts > 0) hasData = true;
/*
	}  else {
	   Winfoprintf("1D spectral FOV not defined."); 
*/
	}
}

int AspDataInfo::getNtraces(string nucleus) {
    int n=SpecDataMgr::get()->getNtraces(nucleus);
    if(n<1) n = nblocks * specperblock;
    return n;
}

void AspDataInfo::getYminmax(string nucleus, double &ymin, double &ymax) {
  int n=SpecDataMgr::get()->getYminmax(nucleus,ymin,ymax); 
  if(n<1) {
     int npts = getNtraces(nucleus);
     if(npts<1) return;

     float maxy=-0.1*FLT_MAX, miny=FLT_MAX;
     float y, *data;
     int i;
     for(i=0;i<npts;i++) {


        data = SpecDataMgr::get()->getTrace("SPEC", i, 1.0, haxis.npts);
        if(!data) continue;

// should be getDataMinMax
        SpecData::getDataMin(data, haxis.npts, &y);
        if(y < miny) miny=y;

        SpecData::getDataMax(data, haxis.npts, &y);
        if(y > maxy) maxy=y;
     }
     ymax = maxy;
     ymin = miny;
  }
//Winfoprintf("ymin ymax %f %f",ymin,ymax);
}

void AspDataInfo::resetYminmax() {
   vaxis.ydataMin=0.0;
   vaxis.ydataMax=-1.0;
}

void AspDataInfo::init1D_vaxis() {

   vaxis.orient=VERT;
   vaxis.name = string("amp");	
   vaxis.label = string("Intensity");
   vaxis.dunits=string("none");
   vaxis.units=string("none");
   vaxis.rev=1; // reversed if axis is left->right or down->up
   vaxis.npts=getNtraces(haxis.name);
   string aig = AspUtil::getString("aig","ai");
   if(aig == "nm") {
      if ( (vaxis.ydataMin == 0.0) && (vaxis.ydataMax < vaxis.ydataMin) )
      {
         double ymin=0, ymax=0;
         getYminmax(haxis.name, ymin, ymax);
         vaxis.minfirst = ymin;
         vaxis.maxwidth = ymax - ymin;
         vaxis.ydataMin=ymin;
         vaxis.ydataMax=ymax;
      }
      else
      {
         vaxis.minfirst = vaxis.ydataMin;
         vaxis.maxwidth = vaxis.ydataMax - vaxis.ydataMin;
      }
   } else {
      vaxis.maxwidth = 0.0; 
      vaxis.minfirst = 0.0;
      vaxis.ydataMin=0.0;
      vaxis.ydataMax=-1.0;
   }

   vaxis.scale = getVscale();

   vaxis.width= AspUtil::getReal("wc2",100)/vaxis.scale;
   vaxis.start= -getVpos()/vaxis.scale;

   if(vaxis.rev) {
     vaxis.maxwidth = -vaxis.maxwidth;
     vaxis.width = -vaxis.width;
   }
   //dprint(&vaxis);
}

void AspDataInfo::dprint(aspAxisInfo_t *axis) {
Winfoprintf("dim,npts,name,label,unit,start,width,minfirst,maxwidth,scale,rev %d %d %s %s %s %s %f %f %f %f %f %d",axis->orient,axis->npts,axis->name.c_str(),axis->label.c_str(),axis->dunits.c_str(),axis->units.c_str(),axis->start,axis->width,axis->minfirst,axis->maxwidth,axis->scale,axis->rev);

}

// based on select_init in init2d.c 
void AspDataInfo::initAxis(int dim, aspAxisInfo_t *axis) {
	char str[16];

	get_nuc_name(dim, str, 7);
	axis->name = string(str);	

	get_display_label(dim, str);
	axis->label = string(str);

	axis->dunits=string("ppm");

	get_axis_label(dim, str);
	axis->units=string("ppm");

	get_sw_par(dim, &(axis->maxwidth));

	get_rflrfp(dim, &(axis->minfirst));
	
	// get sp,wp,reffrq,revflag
	// revflag true for FID x-axis and vertical axis of 1D display 
	get_scale_pars(dim, &(axis->start), &(axis->width), &(axis->scale), &(axis->rev));

	// convert sp,wp to cr,delta. 
	// rev=0 for freq axis, rev=1 for fid or intensity
        if(axis->rev) { // width is negative, since cursor2=cursor1-delta 
	   axis->maxwidth = -axis->maxwidth;
	   axis->width = -axis->width;
	} else { // reverse start point
	   axis->start += axis->width;
	   axis->minfirst = -axis->minfirst; // this is the first point on the right.
        }

// note, axis->scale from get_scale_pars depends on axis unit.
// since roi is defined in ppm, axis->scale should be reffrq
	getReffrq(dim, &(axis->scale));

	// scale is reffrq (ppm to Hz)
	if(axis->scale == 0) axis->scale=1.0;
 	axis->minfirst /= axis->scale;
 	axis->maxwidth /= axis->scale;
 	axis->start /= axis->scale;
 	axis->width /= axis->scale;

	if(dim == VERT) axis->npts = fn1/2;
	else axis->npts = fn/2;
 
	axis->orient=dim;

	//dprint(axis);
}

double AspDataInfo::getVpos() {
    double vpos;
    if (P_getreal(CURRENT, "vp", &vpos, 1)) vpos=0.0;
    vpos += 2;
    return vpos;
}

double AspDataInfo::getVoff() {
  double voff;
  if (P_getreal(CURRENT, "vo", &voff, 1)) voff=0.0;
  return voff;
}

double AspDataInfo::getHoff() {
  double hoff;
  if (P_getreal(CURRENT, "ho", &hoff, 1)) hoff=0.0;
  return hoff;
}

double AspDataInfo::getVscale() {
  double vscale;
  if (P_getreal(CURRENT, "vs", &vscale, 1)) vscale=1.0;
  char aig[16];
  if (P_getstring(CURRENT, "aig", aig, 1, sizeof(aig))) strcpy(aig,"ai");

  if(strcmp(aig,"nm")==0 && vaxis.maxwidth != 0) {
       vscale=vscale/fabs(vaxis.maxwidth);  // none to mm
  }

  return vscale;
}

string AspDataInfo::getFidPath() {

  	char fidfile[MAXSTR2];
  	if(P_getstring(CURRENT, "file", fidfile, 1, MAXSTR2)) strcpy(fidfile,"");
        return string(fidfile);
}


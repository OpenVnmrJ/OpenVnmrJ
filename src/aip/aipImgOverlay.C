/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "aipVnmrFuncs.h"
#include "aipImgInfo.h"
#include "aipViewInfo.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipDataManager.h"
#include "aipInterface.h"
#include "aipRQgroup.h"
#include "aipImgOverlay.h"
#include "aipWriteData.h"

using namespace aip;

extern "C" {
#include "iplan.h"
#include "iplan_graphics.h"
int P_save(int tree, const char *filename);
int P_read(int tree, const char *filename);
}

// two image planes has the same orient and same z position (pss)
// but x, y position (pro, ppe) may differ.
bool ImgOverlay::isCoPlane(spDataInfo_t di1, spDataInfo_t di2) {
   if(di1 == (spDataInfo_t)NULL || di2 == (spDataInfo_t)NULL) return false; 
   DDLSymbolTable *st1 = di1->st;
   DDLSymbolTable *st2 = di2->st;

   if(!st1 || !st2) return false;

   double d1,d2;
   double eps = 1e-4;
   for(int i=0; i<9; i++) {
     st1->GetValue("orientation", d1, i);
     st2->GetValue("orientation", d2, i);
     if(fabs(d1-d2) > eps) return false;
   }
   eps = getReal("aipOverlayThk",1, 2.0)*0.1;
   st1->GetValue("location",d1,2);
   st2->GetValue("location",d2,2);
   if(fabs(d1-d2) > eps) return false;
   else return true;
}

// two image planes has the same orient.
bool ImgOverlay::isParallel(spDataInfo_t di1, spDataInfo_t di2) {
   if(di1 == (spDataInfo_t)NULL || di2 == (spDataInfo_t)NULL) return false; 
   DDLSymbolTable *st1 = di1->st;
   DDLSymbolTable *st2 = di2->st;

   if(!st1 || !st2) return false;

   double d1,d2;
   double eps = 1e-4;
   for(int i=0; i<9; i++) {
     st1->GetValue("orientation", d1, i);
     st2->GetValue("orientation", d2, i);
     if(fabs(d1-d2) > eps) return false;
   }
   return true;
}

// called by aipOverlayGroup command to get overlay image key(s) for gf. 
list<string> ImgOverlay::getOverlayImages(spGframe_t gf, RQgroup *group) {
   list<string> keys;
 
   ChildList *il = group->getChildren();
   list<RQnode>::iterator iitr;
   for (iitr=il->begin(); iitr != il->end(); ++iitr) {
      string key = iitr->getKey();
      spDataInfo_t od = DataManager::get()->getDataInfoByKey(key,true);
      if(od == (spDataInfo_t)NULL) continue;

      // getCoPlane return a valid key if od image co-plane with base image.
      key = getCoPlane(gf, od);
      if(key.length()>0) keys.push_back(key);
   }
   return keys;
}

string ImgOverlay::getCoPlane(spGframe_t gf, spDataInfo_t overlayData) {

   spViewInfo_t view = gf->getFirstView();
   if(view  == nullView) return "";
   spImgInfo_t img = view->imgInfo;
   if(img == nullImg) return "";

   spDataInfo_t baseData = img->getDataInfo();

    if(baseData == (spDataInfo_t)NULL || overlayData == (spDataInfo_t)NULL)
	return string(""); 

    // overlayData may have rank 2 or 3.
    if(overlayData->getRank() == 2 && isCoPlane(baseData, overlayData)) {
        return overlayData->getKey();
    } else if(overlayData->getRank() == 2) {
	return string("");
    }

    if(overlayData->getRank() != 3) return string("");

    // if gets here, overlayData has rank 3.
    return extractPlane(gf, overlayData);
}

// calc centers for intersecting voxels
// center is in pixel, use baseImage's pixToData to convert to data pix.
// scale data pix to resolution of overlay image.
int ImgOverlay::rebinOverlayData(spGframe_t gf, spDataInfo_t overlayData, int nfast, int nmedium, double scalex, double scaley, float *buf, bool drawIntersect) {
   if(overlayData == (spDataInfo_t)NULL) return 0;

   spViewInfo_t fview = gf->getFirstView();
   if(fview  == nullView) return 0;

   float *data = (float *)overlayData->getData();
   if(!data) return 0;

   iplan_view *view = (iplan_view *)malloc(sizeof(iplan_view));
   iplan_stack *stack = (iplan_stack *)malloc(sizeof(iplan_stack));
   getIBview(view, gf->id);

   string dpath = overlayData->getFilepath();
   dpath = dpath.substr(0,dpath.find_last_of("/"))+"/procpar";
   string tpath = getCurexpdir()+"/curpar";

   P_save(CURRENT,(char *)tpath.c_str());
   P_read(CURRENT,(char *)dpath.c_str());
   getStack(stack, VOLUME);
   P_read(CURRENT,(char *)tpath.c_str());
   
   float orig[3],voxsize[3],p1[3]; 

   int ny = overlayData->getFast();
   int nx = overlayData->getMedium();
   int nz = overlayData->getSlow();

   voxsize[0] = (stack->lpe)/nx;
   voxsize[1] = (stack->lro)/ny;
   voxsize[2] = (stack->lpe2)/nz;

   orig[0] = - 0.5*(stack->lpe+voxsize[0]);
   orig[1] = - 0.5*(stack->lro+voxsize[1]);
   orig[2] = - 0.5*(stack->lpe2+voxsize[2]);

   //float viewThk = 0.5*(view->slice.thk)*(view->pixelsPerCm);
   float viewThk = getReal("aipOverlayThk",2, 1.5)*0.5*voxsize[2]*(view->pixelsPerCm);

   int ix,iy;
   double dx,dy;
   int i,j,k, count=0;
   for(i=0; i<nfast*nmedium; i++) buf[i] = (float)0.0;

   int maxx=0, maxy=0, minx=nfast, miny=nmedium;
   for(k=0;k<nz;k++)
     for(j=0;j<ny;j++)
        for(i=0;i<nx;i++) {
           // p1 is the upper left corner of a 3d voxel 
           // p2 is the diagnal corner of p1
           // Note, p1[0] and p2[0] are swapped
           p1[0] = orig[0] + i*voxsize[0];
           p1[1] = orig[1] + j*voxsize[1];
           p1[2] = orig[2] + k*(voxsize[2]);
           // transform p1, p2 to pixel space (base image space)
           transform(stack->u2m, p1);
           transform(view->m2p, p1);
           if(!(p1[2]>viewThk) && !(p1[2]< -viewThk)) {
              // vox intersects with base image 
		
		fview->pixToData(p1[0],aip_mnumypnts - p1[1] - 1,dx,dy);
		ix = (int)(dx*scalex);
		iy = (int)(dy*scaley);
		if(ix >=0 && ix < nfast && iy >= 0 && iy < nmedium) {
		  int ind = csi_getInd(ny-1-j,i,k,ny,nx,nz);
		  if((data+ind))
		  buf[ix+iy*nfast] = data[ind];
		 // buf[ix+iy*nfast] = *(data+ind);
                 if(drawIntersect) {
                   Dpoint_t polygon[2];
                   polygon[0].x = polygon[1].x = p1[0];
                   polygon[0].y = polygon[1].y = aip_mnumypnts - p1[1] - 1;
                   GraphicsWin::drawPolyline(polygon, 2, 4);
                 }
		 if(ix<minx) minx=ix;
		 if(iy<miny) miny=iy;
		 if(ix>maxx) maxx=ix;
		 if(iy>maxy) maxy=iy;
		 count++;
		}
           }
        }

//Winfoprintf("###%d",count);
    free(view);
    free(stack);

    // smooth data
    // if a data point is missing, use average of its neighboring points
    int ind,ind2,n;
    float f;
    for(j=1; j<maxy-1; j++) {
      for(i=1; i<maxx-1; i++) {
	ind=i+j*nfast;
	if(buf[ind] == 0.0) {
	   n=0;
	   f=0.0;
	   for(iy=j-1; iy<j+1; iy++) {
	     for(ix=i-1; ix<i+1; ix++) { 
		ind2=ix+iy*nfast;	
		if(buf[ind2] != 0.0) {
		    f+=buf[ind2];
		    n++;
		}
	     }
	   }
	   if(n>1) buf[ind]=f/n;
	}
      }
    }
	
    return count;
}

string ImgOverlay::extractPlane(spGframe_t gf, spDataInfo_t overlayData) {

   if(overlayData == (spDataInfo_t)NULL) return "";
   spViewInfo_t view = gf->getFirstView();
   if(view  == nullView) return "";

   // get baseData
   spImgInfo_t img = view->imgInfo;
   if(img == nullImg) return "";
   spDataInfo_t baseData = img->getDataInfo();

   // determin size for overlay plane
   int bfast = baseData->getFast();
   int bmedium = baseData->getMedium();
   int ofast = overlayData->getFast();
   int omedium = overlayData->getMedium();
   double blpe,olpe,blro,olro;
   DDLSymbolTable *bst = baseData->st;
   DDLSymbolTable *ost = overlayData->st;

   if(!bst || !ost) return "";
   bst->GetValue("roi",blpe, 0);
   bst->GetValue("roi",blro, 1);
   ost->GetValue("roi",olpe, 0);
   ost->GetValue("roi",olro, 1);

   int nfast = (int)(blpe*ofast/olpe);
   int nmedium = (int)(blro*omedium/olro);
   double scalex = (blpe*ofast)/(olpe*bfast);
   double scaley = (blro*omedium)/(olro*bmedium);

   // new data struct for extracted plane 
   // The only difference from baseData is ds2d->data, and data size nfast, nmedium. 
   dataStruct_t *ds2d = new dataStruct_t;
    if(!ds2d) {
	return "";
    }
    memcpy(ds2d, baseData->dataStruct, sizeof(*ds2d));

    ds2d->data = NULL;
    ds2d->auxparms = NULL;

   float *buf = new float[nfast*nmedium];
   if(buf == NULL)
   {
            delete ds2d;
            Winfoprintf("VolData: allocate memory returned NULL pointer.");
            return "";
   }
   int np = rebinOverlayData(gf, overlayData, nfast,nmedium, scalex, scaley, buf,false);
   if(np<1) {
	delete[] buf;
            delete ds2d;
	return "";
   }
   DDLSymbolTable *st = (DDLSymbolTable *)baseData->st->CloneList(false);
   DDLSymbolTable *st2 = new DDLSymbolTable();
   // New 2D datainfo
   spDataInfo_t datainfo = spDataInfo_t(new DataInfo(ds2d, st, st2)); 
   if(datainfo == (spDataInfo_t)NULL)
   {
            delete ds2d;
            delete st2;
            Winfoprintf("VolData: allocate memory returned NULL pointer.");
            return "";
   }

   datainfo->st->SetData((float *)buf, sizeof(float) * nfast * nmedium);
   delete[] buf;

   // update header
   char floatStr[16];
   strcpy(floatStr, "float");
   datainfo->st->SetValue("rank", 2);
   datainfo->st->SetValue("bits", 32);
   datainfo->st->SetValue("storage", floatStr);
   datainfo->st->SetValue("matrix", nfast, 0);
   datainfo->st->SetValue("matrix", nmedium, 1);

   string newpath = overlayData->getFilepath()+string("_")+baseData->getShortName();

   datainfo->st->SetValue("filename", newpath.c_str());

   DataManager *dm = DataManager::get();
   string key = dm->loadFile(newpath.c_str(), datainfo->st, datainfo->st2);
//Winfoprintf("-----key %s",key.c_str());

   //ReviewQueue::get()->addImagePlane(dataInfo, key);

   datainfo->st = NULL;

   return key;
}

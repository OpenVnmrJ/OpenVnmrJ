/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <list>

#include <unistd.h>
#include "aipSpecViewMgr.h"
#include "aipCommands.h"
#include "aipGframeManager.h"
#include "aipVnmrFuncs.h"
#include "aipGraphicsWin.h"
#include "aipSpecDataMgr.h"
#include "aipMouse.h"
#include "aipAxisInfo.h"

#define LL 0
#define LI 1

extern "C" {
#include "iplan.h"
#include "iplan_graphics.h"
void aip_writeCSIMap(char *name, float *data, int mapInd, int rows, int cols, double x, double y, double w, double h);
void getCursorFreq(double *freq, double *freq1, double *freq2, double *sw);
void getCursorFreq2(double *freq, double *freq1, double *freq2, double *sw);
void getDefaultMapPath(char *path);
void calc_li_map(char *path, double freq, double freq1, double freq2);
void calc_ll_map( char *path, int nearest, double freq);
double freq_to_csPPM(double freq);
}

using namespace aip;

SpecViewMgr *SpecViewMgr::specViewMgr=NULL;
bool SpecViewMgr::hide=false;

SpecViewMgr::SpecViewMgr() {
   currentView=-1;
}

SpecViewMgr *SpecViewMgr::get() {
    if(!specViewMgr) specViewMgr = new SpecViewMgr();
    return specViewMgr;
}

/* STATIC VNMRCOMMAND */
int SpecViewMgr::aipMakeMaps(int argc, char *argv[], int retc, char *retv[]) {
  int flag = LI;
  if(argc > 1 && strcasecmp(argv[1],"ll")==0) flag = LL;
  string name = "";
  if(argc > 2) name=string(argv[2]);
  string key = "spec";
  if(argc > 3) key=string(argv[3]);

  if(argc > 4 && strstr(argv[4],"ratio") != NULL) {
      return SpecViewMgr::get()->calcRatioMap(argv[4],flag,key,name);
  }
  
  // get cursor frequency, integral region, and sw
  double freq, freq1, freq2, sw;
  getCursorFreq(&freq, &freq1, &freq2, &sw);

  // determine path and name
  char path[MAXSTR], mappath[MAXSTR];
  sprintf(path,"%s",name.c_str());
  if(strlen(path) < 1) { 
     if(flag==LL) sprintf(path,"mmap_ll_%.2f",freq_to_csPPM(freq));
     else sprintf(path,"mmap_li_%.2f",freq_to_csPPM(freq));
  } else if(path[strlen(path)-1]=='_') {
     char tmp[MAXSTR]; 
     sprintf(tmp,"%s%.2f",path,freq_to_csPPM(freq));
     strcpy(path,tmp);
  }
  if(path[0] != '/') {
     char dir[MAXSTR];
     getDefaultMapPath(dir);
     sprintf(mappath,"%s/%s",dir,path);
  } else {
     sprintf(mappath,"%s",path);
  }

  GframeManager *gfm = GframeManager::get();
  GframeList::iterator gfi;
  spGframe_t gf = gfm->getFirstSelectedFrame(gfi);
  if(gf == nullFrame) gf = gfm->getFirstFrame(gfi);

  if(!(SpecViewMgr::get()->makeMaps(gf,mappath,key,flag,freq, freq1, freq2, sw))) {
     if(flag == LL) {
       calc_ll_map(mappath, 0, freq);
     } else {
       calc_li_map(mappath, freq, freq1, freq2);
     }
  }

  return proc_complete;
}

// This command displays CSI grid (based on current parameters) and spectra (if loaded).
// command to load spectra is aipLoadSpec(fullpath, key).
//
// Usage: aipShowCSIData<(key1<,key2,...>)>
// Default: if no argument, show grid and spectra.
// keyList can be data (spectra) keyList defined by aipLoadSpec(fullpath, key) command.
// e.g., aipLoadSpec(fullpath, 'spec'). 
// or reserved keyList:
// 'FID' - show fid data.
// 'SPEC' - show phasefile spectra.
// 'frame:all' - show data in all frames.
// 'frame:sel' - show data in selected frame(s).
// 'frame:i' - show data in frame i, i starts from 1 not 0.
// 'none' - turn off csi display in all frames
// 'hide' -  hide csi display in all frames
// 'hide:i' - hide csi display in frame i
// 'hide:sel' - hide csi display in frame i
// 'grid' - show grid (intersection on base image). 
// '3dgrid' - show 3D grid 
// 'num' - show voxel index (index=(iz-1)*ny*nx + (iy-1)*nx + (ix-1))
//
// Examples: 	
// aipShowCSIData('allFrames') - show grid and spectra in all frames 
// aipShowCSIData('spec') - show spectra without grid in selected frame(s)
// aipShowCSIData('3dgrid') - show 3D grid in selected frame(s) 

// if argv[0] is "aipShowCSIData", set SpecViewList's layoutType to CSILAYOUT
// otherwise  set SpecViewList's layoutType to HORIZLAYOUT, VERTLAYOUT, STACKLAYOUT or GRIDLAYOUT

int SpecViewMgr::aipShowSpec(int argc, char *argv[], int retc, char *retv[])
{
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeList::iterator gfi;
    std::list<string> keyList;
    bool selFrame = true;
    bool selSpec = false;
    int frameNum = 0;
    int layout;
    int sliceInd = 1;
    int rows=0,cols=0;

    if(retc > 0) { // return whether CSI overlay is displayed
       retv[0]=realString((double)0);
       for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
          if(gf->hasSpec() && !hide) {
            retv[0]=realString((double)1);
            break;
          } else if(gf->hasSpec()) {
            retv[0]=realString((double)2);
            break;
          }
       }
       if(retc > 1) {
	  retv[1]=realString((double)is_aip_window_opened()); 
       }
       if(retc > 2) { // return # of spec data set and they keys.
	  SpecDataMap *smap = SpecDataMgr::get()->getSpecDataMap();
          retv[2]=realString((double)smap->size());
          if(retc > 3) {
	     int i = 3;
	     SpecDataMap::iterator pd;
             for (pd=smap->begin(); pd != smap->end(); ++pd) {
		if(i<retc) retv[i] = newString(pd->first.c_str());
		i++;
	     }
        }
      }

       return proc_complete;
    }

    if(gfm->getNumberOfFrames() < 1) gfm->splitWindow(1,1);

    hide=false;
    if(strcmp(argv[0],"aipShowCSIData") == 0) layout = CSILAYOUT;
    else layout = GRIDLAYOUT;

    // show grid and first available data if no argument or the only argument is 'allFrames'
    if(argc < 2 || (argc == 2 && strcasecmp(argv[argc-1],"frame:all") == 0)) { 
        SpecDataMap *dmap = SpecDataMgr::get()->getSpecDataMap();
	keyList.push_back(string("grid"));
	if(dmap->size() > 0) {
	   keyList.push_back(dmap->begin()->first); 
	} else {
	   keyList.push_back("SPEC"); 
	}
    }

    // if arguments are given
    while(argc > 1) { 
  	if(strcasecmp(argv[argc-1],"horiz") == 0) layout = HORIZLAYOUT;
  	else if(strcasecmp(argv[argc-1],"vert") == 0) layout = VERTLAYOUT;
  	else if(strcasecmp(argv[argc-1],"stack") == 0) layout = STACKLAYOUT;
        else if(strstr(argv[argc-1],"slice:") == argv[argc-1] && strlen(argv[argc-1])>6) {
	   char *ptr = argv[argc-1]+6;
	   if(isdigit(*ptr)) sliceInd = atoi(ptr);
        } else if(strstr(argv[argc-1],"rows:") == argv[argc-1] && strlen(argv[argc-1])>5) {
	   char *ptr = argv[argc-1]+5;
	   if(isdigit(*ptr)) rows = atoi(ptr);
        } else if(strstr(argv[argc-1],"cols:") == argv[argc-1] && strlen(argv[argc-1])>5) {
	   char *ptr = argv[argc-1]+5;
	   if(isdigit(*ptr)) cols = atoi(ptr);
        } else if(strstr(argv[argc-1],"frame:") == argv[argc-1] && strlen(argv[argc-1])>6) {
	   char *ptr = argv[argc-1]+6;
           if(strcasecmp(ptr,"all") == 0) selFrame = false;
           else if(strcasecmp(ptr,"sel") == 0) selFrame = true;
	   else if(isdigit(*ptr)) frameNum = atoi(ptr);
        } else if(strcasecmp(argv[argc-1],"sel") == 0) selSpec = true;
	else if(strcasecmp(argv[argc-1],"none") == 0) {
           for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
	      if(gf->hasSpec()) {
                 gf->deleteSpecList();
	         gf->draw();
	      }
           }
           return proc_complete;
	} else if(strcasecmp(argv[argc-1],"hide") == 0) {
	   hide=true;
           SpecViewMgr::get()->selectedViews.clear();
           for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
	      if(gf->hasSpec()) {
                 gf->deleteSpecList();
	         gf->draw();
	      }
           }
  
        } else if(strstr(argv[argc-1],"hide") == argv[argc-1]) {
	   hide=true;
           SpecViewMgr::get()->selectedViews.clear();
           if(strlen(argv[argc-1])>3 && isdigit(argv[argc-1][4])) {
             char *ptr = argv[argc-1]+4;
             int n = atoi(ptr);
             int i = 0;
       	     for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
                 if(++i == n && gf->hasSpec()) {
                   gf->deleteSpecList();
	           gf->draw();
		 }
	     }
           } else {
	     for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
            	=gfm->getNextSelectedFrame(gfi)) {
	        if(gf->hasSpec()) {
                   gf->deleteSpecList();
	           gf->draw();
	        }
	     }
           }
        }
	else if(strlen(argv[argc-1]) > 0) 
	   keyList.push_front(string(argv[argc-1]));
        argc--;
    }

    // update and display with new keyList and selFrame (static members). 
    if(selFrame and frameNum > 0) {
       int i=0;
       for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
          if(++i == frameNum) {
	     gf->setSpecKeys(keyList);
	     gf->getSpecList()->setSliceInd(sliceInd);
	     gf->getSpecList()->setRows(rows);
	     gf->getSpecList()->setCols(cols);
          }
       }
    } else if(selFrame) {
       int i=0;
       for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextSelectedFrame(gfi)) {
          gf->setSpecKeys(keyList);
	  gf->getSpecList()->setSliceInd(sliceInd);
	  gf->getSpecList()->setRows(rows);
	  gf->getSpecList()->setCols(cols);
	  i++;
       }
       if(i==0) { // no frame is selected. use first frame.
	  gf=gfm->getFirstFrame(gfi);
	  if(gf != nullFrame) {
	     gf->setSpecKeys(keyList);
	     gf->getSpecList()->setSliceInd(sliceInd);
	     gf->getSpecList()->setRows(rows);
	     gf->getSpecList()->setCols(cols);
          }
       }
    } else {
       for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
          gf->setSpecKeys(keyList);
	  gf->getSpecList()->setSliceInd(sliceInd);
	  gf->getSpecList()->setRows(rows);
	  gf->getSpecList()->setCols(cols);
       }
    }

    if(layout == CSILAYOUT)
      SpecViewMgr::get()->updateCSIGrid(true);
    else
      SpecViewMgr::get()->updateArrayLayout(true, selSpec, layout);

    return proc_complete;
}

// create graphInfo_t for voxels that intersect with base image in given frame.
// gInfo is allocated by caller for all voxels (n=nx*ny*nz)
// but only those intersect with base image will have graphInfo_t.
// return value is number of graphInfo_t's.
// index of graphInfo_t is based on all voxels.
// gInfo is scaled according to the base image.
int SpecViewMgr::getCSIGrafAreas(graphInfo_t *gInfo, int n, int frameID, bool drawIntersect) {
  float3 orig, voxsize, p1, p2;
  float3 box[8];
  float2 points[12];
  int nx=1,ny=1,nz=1,ns=1;
  float gap, viewThk;
  int i, j,k, m, np, count = 0;
  int firstSlice, lastSlice;
  iplan_view *view = (iplan_view *)malloc(sizeof(iplan_view));
  iplan_stack *stack = (iplan_stack *)malloc(sizeof(iplan_stack));
  getIBview(view, frameID);
  if(!(view->hasFrame)) {
        Winfoprintf("getCSIGrafAreas: frame %d is not defined.",frameID);
    	free(view);
    	free(stack);
        return 0;
  }

  spGframe_t gf = GframeManager::get()->getFrameByID(frameID);
  getCSIDims(&nx, &ny, &nz, &ns, gf);
  if(nx==1 && ny==1 && nz==1 && ns==1) {
     getStack(stack,VOXEL);
     voxsize[0] = (stack->lpe);
     voxsize[1] = (stack->lro);
     voxsize[2] = (stack->lpe2);
     firstSlice = 1;
     lastSlice = 1;
     gap=0;
  } else if(nz>1) { // ns must be 1
     getStack(stack,CSI3D);
     voxsize[0] = (stack->lpe)/nx;
     voxsize[1] = (stack->lro)/ny;
     voxsize[2] = (stack->lpe2)/nz;
     firstSlice = 1;
     lastSlice = nz;
     gap=0;
  } else {
     getStack(stack,CSI2D);
     voxsize[0] = (stack->lpe)/nx;
     voxsize[1] = (stack->lro)/ny;
     voxsize[2] = (stack->thk);
     firstSlice = 1;
     lastSlice = ns;
     gap = stack->gap;
  }

  // upper right corner of an axial plane
  orig[0] = - 0.5*(stack->lpe);
  orig[1] = - 0.5*(stack->lro);
  orig[2] = - 0.5*(lastSlice*voxsize[2] +(lastSlice-1)*gap);

  // vox will be displayed only if it intersects with base image (view).
  // there is a thckness of view 
  // (currently it is a defaultThk because thk is not saved in fdf header).
  viewThk = 0.5*(view->slice.thk)*(view->pixelsPerCm);

  // note, firstSlice=1, lastSlice=nz*ns.
  for(k=firstSlice-1;k<lastSlice;k++)
     for(j=0;j<ny;j++)
        for(i=0;i<nx;i++) {
	   // p1 is the upper left corner of a 3d voxel 
	   // p2 is the diagnal corner of p1
	   // Note, p1[0] and p2[0] are swapped
           p1[0] = orig[0] + (i+1)*voxsize[0];
           p1[1] = orig[1] + j*voxsize[1];
           p1[2] = orig[2] + k*(voxsize[2]+gap);
           p2[0] = p1[0]-voxsize[0];
           p2[1] = p1[1]+voxsize[1];
           p2[2] = p1[2]+voxsize[2];
	   // transform p1, p2 to pixel space (base image space)
           transform(stack->u2m, p1);
           transform(stack->u2m, p2);
           transform(view->m2p, p1);
           transform(view->m2p, p2);
           if(!(p1[2]>viewThk && p2[2]>viewThk) && 
	      !(p1[2]< -viewThk && p2[2]< -viewThk)) {
	      // vox intersects with base image 
	      // (z of p1 and p2 are on the opposite of the base image) 
	      // Now transform all 8 corners of the voxel to magnet frame.
                p1[0] = orig[0] + (i+1)*voxsize[0];
                p1[1] = orig[1] + j*voxsize[1];
                p1[2] = orig[2] + k*(voxsize[2]+gap);
                p2[0] = p1[0]-voxsize[0];
                p2[1] = p1[1]+voxsize[1];
                p2[2] = p1[2]+voxsize[2];
                box[0][0] = p1[0];
                box[0][1] = p2[1];
                box[0][2] = p2[2];
                transform(stack->u2m, box[0]);

                box[1][0] = p2[0];
                box[1][1] = p2[1];
                box[1][2] = p2[2];
                transform(stack->u2m, box[1]);

                box[2][0] = p2[0];
                box[2][1] = p1[1];
                box[2][2] = p2[2];
                transform(stack->u2m, box[2]);

                box[3][0] = p1[0];
                box[3][1] = p1[1];
                box[3][2] = p2[2];
                transform(stack->u2m, box[3]);

                box[4][0] = p1[0];
                box[4][1] = p2[1];
                box[4][2] = p1[2];
                transform(stack->u2m, box[4]);

                box[5][0] = p2[0];
                box[5][1] = p2[1];
                box[5][2] = p1[2];
                transform(stack->u2m, box[5]);

                box[6][0] = p2[0];
                box[6][1] = p1[1];
                box[6][2] = p1[2];
                transform(stack->u2m, box[6]);

                box[7][0] = p1[0];
                box[7][1] = p1[1];
                box[7][2] = p1[2];
                transform(stack->u2m, box[7]);

                np = calcBoxIntersection(view, box, points);
		if(np>0) { 
                 if(gInfo != NULL && count < n) {
                   // Note, nx and ny are swapped  
                   gInfo[count].index=csi_getInd(j,i,k,ny,nx,nz*ns);
                   gInfo[count].npts=np;
                   if(np<12) {
                      gInfo[count].polygon[np].x = points[0][0];
                      gInfo[count].polygon[np].y = aip_mnumypnts - points[0][1] - 1;
                   }
                   for(m=0; m<np && m<12; m++) {
                      gInfo[count].polygon[m].x=points[m][0];
                      gInfo[count].polygon[m].y=aip_mnumypnts - points[m][1] - 1;
                   }
                 }
                 if(drawIntersect) {
  		   Dpoint_t polygon[12];
                   if(np<12) {
                      polygon[np].x = points[0][0];
                      polygon[np].y = aip_mnumypnts - points[0][1] - 1;
                   }
                   for(m=0; m<np && m<12; m++) {
                      polygon[m].x=points[m][0];
                      polygon[m].y=aip_mnumypnts - points[m][1] - 1;
                   }
                   GraphicsWin::drawPolyline(polygon, np+1, 4);
                 }
                 count++;
		}
           }
        }

    free(view);
    free(stack);
    return count;
}

// this function also check whether current params are consistent with fdf header
void SpecViewMgr::getCSIDims(int *nx, int *ny, int *nz, int *ns, spGframe_t gf) {
  csi_getCSIDims(nx,ny,nz,ns); // from CURRENT parameters 

/* comment out, so current parameters are used.
  // try to get from spec header in gf
  if(gf == nullFrame) return;
  string key = gf->getSpecList()->getFirstKey();
  if(key=="") return;
  spSpecData_t sd = SpecDataMgr::get()->getDataByKey(key);
  if(sd == (spSpecData_t)NULL) return;
  specStruct_t *ss = sd->getSpecStruct();
  if(ss == NULL || ss->data == NULL || ss->rank <  2) return;
  if(strcmp(ss->dataType,"spectrum") != 0) return;

  int value;
  if(sd->st->GetValue("slices",value) && value > 0 && value != *ns) {
     *ns=value;
     Winfoprintf("Warning: current ns value %d does not match fdf value %d",
	*ns, value);
  }

  // note, start from ss->matrix[1], because ss->matrix[0] is "spectral dimension"
  if(ss->rank > 1 && ss->matrix[1] > 0 && ss->matrix[1] != *nx) {
     *nx=ss->matrix[1];
     Winfoprintf("Warning: first dimension %d does not match parameter value %d",
	*nx, ss->matrix[1]);
  }
  if(ss->rank > 2 && ss->matrix[2] > 0 && ss->matrix[2] != *ny) {
     *ny=ss->matrix[2];
     Winfoprintf("Warning: second dimension %d does not match parameter value %d",
	*ny, ss->matrix[2]);
  }
  if(ss->rank > 3 && ss->matrix[3] > 0 && ss->matrix[3] != ((*nz)*(*ns))) {
     // Note, ss->matrix[3] should be nz*ns.
     *nz=ss->matrix[3];
     if((*ns)<1 || (*ns)>(*nz)) *ns=1;
     (*nz) /= (*ns);
     Winfoprintf("Warning: third dimension %d does not match parameter value %d",
	*nz, ss->matrix[3]);
  }
*/
  
}

// this function determine the number of grafs to be allocated.
void SpecViewMgr::getArrayDim(int *n, spGframe_t gf) {
  *n = getNtraces();

  if(gf == nullFrame) return;

  // get user specified rows and cols
  int rows = gf->getSpecList()->getRows();
  int cols = gf->getSpecList()->getCols();
  if(rows> 0 && cols > 0) {
     *n=rows*cols;
     return;
  }

  string key = gf->getSpecList()->getFirstKey();
  if(key=="") return;
  spSpecData_t sd = SpecDataMgr::get()->getDataByKey(key);
  if(sd == (spSpecData_t)NULL) return;
  specStruct_t *ss = sd->getSpecStruct();
  if(ss == NULL || ss->data == NULL || ss->rank <  2) return;
  if(strcmp(ss->dataType,"spectrum") != 0) return;

  int nt = sd->getNumTraces();
  if((*n) != nt) {
     *n=nt;
     Winfoprintf("Warning: current ntraces value %d does not match fdf value %d",
	*n, nt);
  }
}

// called by gf->draw()
void SpecViewMgr::updateCSIGrid(SpecViewList *specList, int frameID) {
    if(!specList) return;
    int n, nx, ny, nz, ns;
    spGframe_t gf = GframeManager::get()->getFrameByID(frameID);
    getCSIDims(&nx, &ny, &nz, &ns, gf);
    n = (nx*ny*nz*ns);

    graphInfo_t *gInfo = new graphInfo_t[n];

    int np = SpecViewMgr::get()->getCSIGrafAreas(gInfo, n, frameID, false); 

    specList->setSpecViews(gInfo, np); 
    specList->selectSpecView(selectedViews);
    setShowRoi(gf);
    
    delete[] gInfo;
}

// called by aipShowCSIData and whenever frame layout is changed or frames are resized.
void SpecViewMgr::updateCSIGrid(bool draw) {

    int n, nx, ny, nz, ns, np;

    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeList::iterator gfi;
    for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
             getCSIDims(&nx, &ny, &nz, &ns, gf);
             n = (nx*ny*nz*ns);
             graphInfo_t *gInfo = new graphInfo_t[n];
             if(gf->getFirstView() != nullView) {
                np = SpecViewMgr::get()->getCSIGrafAreas(gInfo, n, gf->id, false); 
             } else { 
                np = SpecViewMgr::get()->getGrafAreas(gInfo, n, gf->id, false, CSILAYOUT); 
             }
             SpecViewList *specList = gf->getSpecList();
	     specList->setSpecViews(gInfo, np); 
	     specList->setSelSpec(false); 
	     specList->setLayoutType(CSILAYOUT); 
             setShowRoi(gf);
             if(draw) gf->draw();
             delete[] gInfo;
    }
}

// called by Gframe when voxel (ind) is selected.
// this function determines selectedViews (a list of indexes), and
// updates all frames
void SpecViewMgr::selectSpecView(SpecViewList *specList, int ind, int mask) {

   bool isRoi=false;

   if(ind < 0) { 
       selectedViews.clear();
   } else {
      switch (mask) {
        case b1+ctrl+shift+down:
        case b1+shift+down:
/*
           // select ROI
	  isRoi=true;
          if(currentView < 0) {
            selectedViews.clear();
            selectedViews.push_back(ind);
            currentView = ind;
	  } else {
            if(ind > currentView) selectedViews.push_back(ind);
	    else selectedViews.push_front(ind);
	    currentView = -1;
          }
          break;
*/
        case b1+ctrl+down:
           // select multiple views
           selectedViews.push_back(ind);
           currentView = -1;
           break;
        case b1+down:
           // select single view
           selectedViews.clear();
           selectedViews.push_back(ind);
           currentView = -1;
           break;
      }
   }
   
   GframeManager *gfm = GframeManager::get();
   spGframe_t gf;
   GframeList::iterator gfi;
   for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
          SpecViewList *sList = gf->getSpecList();
          if(gf->hasSpec()) {
             sList->selectSpecView(selectedViews);
	     gf->draw();
	  } 
   }

   if(selectedViews.size() < 1) return;
   int found = appdirFind("csiSel", "maclib", NULL, "", R_OK);
   if(!found) return;

   string key = specList->getFirstKey();
   
   // pass selection to csiSel (macro).
   char cmd[MAXSTR];
   char str[12];
   if(isRoi && currentView >= 0) {
     sprintf(cmd,"csiSel('%s roi',%d)\n", key.c_str(), currentView);
     execString(cmd); 
   } else if(isRoi) {
     sprintf(cmd,"csiSel('%s roi',", key.c_str());
     std::list<int>::iterator ip;
     for (ip = selectedViews.begin(); ip != selectedViews.end(); ++ip) {
	sprintf(str,"%d ",*ip);
        strcat(cmd,str);
     }
     strcat(cmd,"')\n");
     execString(cmd);
   } else if(selectedViews.size()==1) {
     std::list<int>::iterator ip = selectedViews.begin();
     sprintf(cmd,"csiSel('%s vox',%d)\n",key.c_str(), *ip);
     execString(cmd); 
   } else {
     sprintf(cmd,"csiSel('%s vox',%d,'",key.c_str(), selectedViews.size());
     std::list<int>::iterator ip;
     for (ip = selectedViews.begin(); ip != selectedViews.end(); ++ip) {
	sprintf(str,"%d ",*ip);
        strcat(cmd,str);
     }
     strcat(cmd,"')\n");
     execString(cmd);
   }

}

// called by gf->draw()
void SpecViewMgr::updateArrayLayout(SpecViewList *specList, int frameID) {
    if(!specList) return;
    bool selSpec = specList->getSelSpec();
    int layout = specList->getLayoutType();
    int n;
    spGframe_t gf = GframeManager::get()->getFrameByID(frameID);
    getArrayDim(&n, gf);
    
    graphInfo_t *gInfo = new graphInfo_t[n];

    int np = SpecViewMgr::get()->getGrafAreas(gInfo, n, frameID, selSpec, layout); 
    specList->setSpecViews(gInfo, np); 
    specList->selectSpecView(selectedViews);
    
    delete[] gInfo;
}

// called by aipShowCSIData and whenever frame layout is changed or frames are resized.
void SpecViewMgr::updateArrayLayout(bool draw, bool selSpec, int layout) {

    int n, np;

    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeList::iterator gfi;
    for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
          //if(gf->hasSpec()) {
    	     getArrayDim(&n, gf);
    	     graphInfo_t *gInfo = new graphInfo_t[n];
             np = SpecViewMgr::get()->getGrafAreas(gInfo, n, gf->id, selSpec, layout); 
             SpecViewList *specList = gf->getSpecList();
	     specList->setSpecViews(gInfo, np); 
	     specList->setSelSpec(selSpec); 
	     specList->setLayoutType(layout); 
             if(draw) gf->draw();
    	     delete[] gInfo;
          //}
    }
}

// layout without base image
// start from upper left corner.
// Note!! n gInfo (graf areas) are allocated by the caller. 
// This function cannot layout more than n graf areas. 
int SpecViewMgr::getGrafAreas(graphInfo_t *gInfo, int n, int frameID, bool selSpec, int layout) {
   int np;
   int *ind;
   double d;
   int fx=0;
   int fy=0;
   int fw=getWinWidth();
   int fh=getWinHeight();
   double zoomFactor = 1;
   int zoomSpecID = 0;
   int sliceInd=1;
   int specRows=0;
   int specCols=0;
   spGframe_t gf = GframeManager::get()->getFrameByID(frameID);
   if(gf != nullFrame) {
	fx=gf->minX(); 
	fy=gf->minY(); 
	fw=gf->maxX() - gf->minX() + 1;
	fh=gf->maxY() - gf->minY() + 1;
        zoomFactor=gf->zoomFactor;
	zoomSpecID=gf->zoomSpecID;
        sliceInd = gf->getSpecList()->getSliceInd();
        specRows = gf->getSpecList()->getRows();
        specCols = gf->getSpecList()->getCols();
   } 
   if(!fw || !fh) {
        Winfoprintf("getCSIGrafAreas: frame %d is not defined.",frameID);
        return 0;
   }

   if(gInfo == NULL || n < 1) return 0;
   int rows=1, cols=1;
   double ho=0, vo=0, hs=2, vs=2;
   
   // layout spec view and
   // get spec indexes (do this so "selSpec" can have any user selected traces).
   // index starts from zero
   if(layout == CSILAYOUT) {
      int nx,ny,nz,ns; 
      getCSIDims(&nx, &ny, &nz, &ns, gf);
      np=nx*ny;
      rows=ny;
      cols=nx;
      ind = new int[np];
      getVoxInds(np, nx, ny, nz, ns, sliceInd, ind); 

      // adjust fw,fh to aspect ratio defined by current lpe, lpe2
      // (only show "xy" plan when there is no base image)
      planParams *tag = getCurrPlanParams(CSI2D);
      if(tag->dim1.value > 0 && tag->dim2.value > 0) {
	double daspect = tag->dim1.value / tag->dim2.value;
	double faspect = (double)fw/(double)fh;
        if(daspect <= 0) daspect=1.0;
        if(daspect >= faspect) {
	   fh=(int)((double)fw/daspect);
	} else {
	   fw=(int)((double)fh*daspect);
	}
      } 

   } else {
      
      // n is # of allocated grafs
      // np is # of grid cells to be laid out. np has to be smaller than n.
      if(selSpec) np = P_getsize(CURRENT,"dsSelect",NULL); 
      else np = n;
      if(np < 1) np = 1; 
      if(np > n) np = n;
      
      // determine rows and cols
      if(layout == GRIDLAYOUT && specRows > 0 && specCols > 0) {
        rows = specRows;
        cols = specCols;
        if(rows*cols > n) {
	  rows = 1;
          cols = np;
        }
      } else if(layout == GRIDLAYOUT) { // try to layout squarish grid  
        double aspect = (double)fh/(double)fw;
        int rs = (int)(sqrt(np*aspect) + 0.5);
        for(int i=rs; i>1; i--) {
	  if(np % i == 0) { // zero remainder
	    rows = i; 
	    cols =  np/i;
	    break;
	  }
	}
	if(np != rows*cols) { // non-zero remainder
          rows = rs;
          cols = (int) ((float)np/(float)rows + 0.5);
          if(cols < 1) cols = 1;
	}
        if(rows*cols > n) {
	  rows = 1;
          cols = np;
        }
      } else if(layout == HORIZLAYOUT) {
        rows = 1;
        cols = np;
      } else { // VERTLAYOUT or STACKLAYOUT
        rows = np;
        cols = 1;
      }

      np = rows*cols;

      // note, it is ok ind[i] is larger than available trace: the grid cell will be empty.
      ind = new int[np];
      for(int i=0; i<np; i++) ind[i] = i; 
      
      if(selSpec) { // overwrite ind with dsSelect 
        int nsel = P_getsize(CURRENT,"dsSelect",NULL);
	for(int i=0; i<nsel; i++) {
           if(!(P_getreal(CURRENT,"dsSelect", &d, i+1))) ind[i] =(int)d - 1;
           else break;
	}
      }
   }
      
   hs = ho = ((double)fw/(double)cols);
   vs = vo = ((double)fh/(double)rows);

   // round off so grid is uniform.
   if(hs>1) hs=(double)floor(hs+0.5);
   if(ho>1) ho=(double)floor(ho+0.5);
   if(vs>1) vs=(double)floor(vs+0.5);
   if(vo>1) vo=(double)floor(vo+0.5);
   

//Winfoprintf("###array %d %d %d %d %d %d %d %d %d",n, np, layout,rows,cols,ho,vo,fx,fy);
   double h0, v0; 
   int i = 0;
   v0 = fy+1; // from top down
   for(int j=0; j<rows; j++) {
    v0 += vo; 
    h0 = fx+1; // from left to right
    for(int k=0; k<cols; k++) {
      h0 += ho; 
      i = j*cols + k;
      if(i >= np) break;

      gInfo[i].index=ind[i];
      gInfo[i].npts=4;
      gInfo[i].polygon[0].x = gInfo[i].polygon[3].x = gInfo[i].polygon[4].x = h0;
      gInfo[i].polygon[0].y = gInfo[i].polygon[1].y = gInfo[i].polygon[4].y = v0;
      gInfo[i].polygon[1].x = gInfo[i].polygon[2].x = h0 - hs;
      gInfo[i].polygon[2].y = gInfo[i].polygon[3].y = v0 - vs;
    }
   }

   // scale and offset for zoom/pan
   // Note, zoomSpecID is gInfo[i].index+1
   int zoomVox = -1;
   if(zoomSpecID && zoomFactor != 1) {
     for(int i=0; i<np; i++) {
	if(gInfo[i].index == (zoomSpecID-1)) {
           zoomVox = i;
	   break; 
	}
     } 
   }

   if(zoomVox != -1) { 
     double cx=0, cy=0; // center of zoomSpec
     for(int j=0; j<4; j++) {
	cx += gInfo[zoomVox].polygon[j].x;
	cy += gInfo[zoomVox].polygon[j].y;
     }
     cx = (cx/4.0);
     cy = (cy/4.0);
     int xoff=(int)(fx+0.5*fw);
     int yoff=(int)(fy+0.5*fh);
     for(int i=0; i<np; i++) {
        for(int j=0; j<5; j++) {
	    gInfo[i].polygon[j].x = (gInfo[i].polygon[j].x -cx)*zoomFactor + xoff; 
	    gInfo[i].polygon[j].y = (gInfo[i].polygon[j].y -cy)*zoomFactor + yoff; 
	}
     }
   }

   delete[] ind;
   return np;
}

// fill ind[], i.e., indexes of traces.
// ind is allocated by caller. 
// May select a slice for multi-slice 2D data or 3D data (xy plane only).
// slice index starts from 1. ind array size is nt=nx*ny
// if slice <= 0, calc ind for all slices. ind array size is nt=nx*ny*nz*ns
// if nt doesn'y match, set nx=nt, ny=nz=ns=1. 
void SpecViewMgr::getVoxInds(int nt, int nx, int ny, int nz, int ns, int slice, int *ind) {
   int i,j,k,count;
   int first, last;

   // check slice
   if(nz*ns == 1) slice=1; // single slice data
   if(slice > nz*ns) slice=1; // slice out of bound

   if(slice < 1) {
     if(nt == (nx*ny*nz*ns)) {
        first = 1;
        last = nz*ns;
     } else {
        Winfoprintf("Warning: array dim inconsistent. return ind as 1D array.");
        first = last = ny = 1;
        nx=nt;
     }
   } else if(nt == (nx*ny)) {
        first = last = slice;
   } else {
        Winfoprintf("Warning: array dim inconsistent. return ind as 1D array.");
        first = last = ny = 1;
        nx=nt;
   }

   count=0;
   for(k=first-1;k<last;k++)
      for(j=0;j<ny;j++)
         for(i=0;i<nx;i++) {
            // Note, nx and ny are swapped, and nx is reversed
            ind[count]=csi_getInd(j,nx-i-1,k,ny,nx,nz*ns);
            count++;
   }
}

void SpecViewMgr::specViewUpdate() {
   GframeManager *gfm = GframeManager::get();
   spGframe_t gf;
   GframeList::iterator gfi;
   for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
	  if(gf->hasSpec()) {
	     gf->draw();
	  } 
   }
}

// Note, ROI is specified by data points of base image, which 
// may have different size and resolution from CSI grid data.
void SpecViewMgr::setShowRoi(spGframe_t gf) {
   SpecViewList *specList = gf->getSpecList();
   if(!specList->hasSpec()) return;
    
   spImgInfo_t img = gf->getFirstImage();
   if (img == nullImg) return;

   spDataInfo_t data = img->getDataInfo();
   if(data == (spDataInfo_t)NULL)  return;
 
   // get base image info
   int ddimx,ddimy;
   double lpe,lro,ppe,pro;
   ddimx=data->getMedium();
   ddimy=data->getFast();
   data->st->GetValue("span", lpe, 0);
   data->st->GetValue("span", lro, 1);
   data->st->GetValue("location", ppe, 0);
   data->st->GetValue("location", pro, 1);

   // get csi grid info
   int nx,ny,nz,ns;
   csi_getCSIDims(&nx,&ny,&nz,&ns);
   if(nx < 1 || ny < 1) return;

   iplan_stack *stack = (iplan_stack *)malloc(sizeof(iplan_stack));
   getStack(stack,CSI2D);
   stack->ppe=-stack->ppe; // reverse x 

   // scaling factor to convert points of base image to csi grid
   double xfactor, yfactor;
   if(stack->lpe == 0 || stack->lro == 0 || ddimx < 1 || ddimy < 1) {
     xfactor=1;
     yfactor=1;
   } else {
     xfactor = (double)nx*lpe/((double)ddimx*(stack->lpe));
     yfactor = (double)ny*lro/((double)ddimy*(stack->lro));
   }

   // offset relative to base image
   double xoff, yoff;
   if(stack->lpe <= 0 || stack->lro <= 0) {
     xoff=0.0;
     yoff=0.0;
   } else {
     xoff = (0.5*stack->lpe - stack->ppe - 0.5*lpe + ppe)*ddimx/lpe;
     yoff = (0.5*stack->lro - stack->pro - 0.5*lro + pro)*ddimy/lro;
   }
   
   specList->setRoi(false);
   int count = 0;
   int ind,x,y,z=0;
   int ix,iy;
   int xmin,ymin,xmax,ymax;
   Roi *roi;
   D3Dpoint_t minpnt, maxpnt;
   for (roi = gf->getFirstRoi(); roi; roi = gf->getNextRoi()) {
       if(roi->GetType() == ROI_LINE || roi->GetType() == ROI_POINT) continue;

       roi->setMinMaxDataPnts();
       minpnt = roi->getminpntData();
       maxpnt = roi->getmaxpntData();
       if(minpnt.x == maxpnt.x || minpnt.y == maxpnt.y) continue;

       xmin = (int)(xfactor*(minpnt.x+xoff)+0.5);
       ymin = (int)(yfactor*(minpnt.y+yoff)+0.5);
       xmax = (int)(xfactor*(maxpnt.x+xoff)+0.5);
       ymax = (int)(yfactor*(maxpnt.y+yoff)+0.5);
       // check boundary
       if(xmin < 0) xmin=0;
       if(ymin < 0) ymin=0;
       if(xmax > nx) xmax=nx;
       if(ymax > ny) ymax=ny;

       for(y=ymin; y<ymax; y++) {
         for(x=xmin; x<xmax; x++) {
           ix = (int)(((double)x)/xfactor-xoff+0.5);
           iy = (int)(((double)y)/yfactor-yoff+0.5);
           if(roi->contains(ix,iy)) {
             ind = csi_getInd(y,nx-x-1,z,ny,nx,1);
             specList->showSpecView(ind+1);
           }
         }
       }
       count++;
   }
   if(count==0) { // show all
     specList->setRoi(true);
   }
}

int SpecViewMgr::makeMaps(spGframe_t gf, char *mappath, string key, int flag,
	double freq, double freq1, double freq2, double sw) {

   if(gf == nullFrame) return 0;

   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return 0;

   spImgInfo_t img = gf->getFirstImage();
   if (img == nullImg) return 0;

   spDataInfo_t data = img->getDataInfo();
   if(data == (spDataInfo_t)NULL)  return 0;

   // get base image info
   int ddimx,ddimy;
   double lpe,lro,ppe,pro;
   ddimx=data->getMedium();
   ddimy=data->getFast();
   data->st->GetValue("span", lpe, 0);
   data->st->GetValue("span", lro, 1);
   data->st->GetValue("location", ppe, 0);
   data->st->GetValue("location", pro, 1);

   // get csi grid info
   int nx,ny,nz,ns;
   csi_getCSIDims(&nx,&ny,&nz,&ns);
   if(nx < 1 || ny < 1) return 0;

   iplan_stack *stack = (iplan_stack *)malloc(sizeof(iplan_stack));
   getStack(stack,CSI2D);
   stack->ppe=-stack->ppe; // reverse x 

   // scaling factor to convert points of base image to csi grid
   double xfactor, yfactor;
   if(stack->lpe == 0 || stack->lro == 0 || ddimx < 1 || ddimy < 1) {
     xfactor=1;
     yfactor=1;
   } else {
     xfactor = (double)nx*lpe/((double)ddimx*(stack->lpe));
     yfactor = (double)ny*lro/((double)ddimy*(stack->lro));
   }

   // offset relative to base image
   double xoff, yoff;
   if(stack->lpe <= 0 || stack->lro <= 0) {
     xoff=0.0;
     yoff=0.0;
   } else {
     xoff = (0.5*stack->lpe - stack->ppe - 0.5*lpe + ppe)*ddimx/lpe;
     yoff = (0.5*stack->lro - stack->pro - 0.5*lro + pro)*ddimy/lro;
   }
   
   specStruct_t *ss = gf->getSpecList()->get2DCSISpecStruct(key);

   int maxtraces, maxpts;
   if(ss) {
     maxpts = ss->matrix[0];
     maxtraces = ss->matrix[1]*ss->matrix[2];
//Winfoprintf("spec data size %d %d",maxpts,maxtraces);
   } else {
     maxpts = getSpecPoints();
     maxtraces = getNtraces(); 
//Winfoprintf("SPEC data size %d %d",maxpts,maxtraces);
   }

  int pos = (int) ((sw - freq) * (double)maxpts / sw);
  int pos1 = (int) ((sw - freq1) * (double)maxpts / sw);
  int pos2 = (int) ((sw - freq2) * (double)maxpts / sw);
//Winfoprintf("peak info %f %f %f %f %d %d %d",freq, freq1, freq2, sw,pos,pos1,pos2);

   char path[MAXSTR];
   int count = 0;
   int ind,x,y,z=0;
   int ix,iy;
   int xmin,ymin,xmax,ymax;
   double ux1,uy1,uz1,ux2,uy2,uz2;
   double px,py;
   double vsFactor = aip_getVsFactor();
   Roi *roi;
   D3Dpoint_t minpnt, maxpnt;
   for (roi = gf->getFirstRoi(); roi; roi = gf->getNextRoi()) {
       if(roi->GetType() == ROI_LINE || roi->GetType() == ROI_POINT) continue;

       roi->setMinMaxDataPnts();
       minpnt = roi->getminpntData();
       maxpnt = roi->getmaxpntData();
       if(minpnt.x == maxpnt.x || minpnt.y == maxpnt.y) continue;

       xmin = (int)(xfactor*(minpnt.x+xoff)+0.5);
       ymin = (int)(yfactor*(minpnt.y+yoff)+0.5);
       xmax = (int)(xfactor*(maxpnt.x+xoff)+0.5);
       ymax = (int)(yfactor*(maxpnt.y+yoff)+0.5);
       // check boundary
       if(xmin < 0) xmin=0;
       if(ymin < 0) ymin=0;
       if(xmax > nx) xmax=nx;
       if(ymax > ny) ymax=ny;

       // determine roi FOV and pos
       int minpntX = (int)(((double)xmin)/xfactor-xoff+0.5); 
       int maxpntX = (int)(((double)xmax)/xfactor-xoff+0.5); 
       int minpntY = (int)(((double)ymin)/yfactor-yoff+0.5); 
       int maxpntY = (int)(((double)ymax)/yfactor-yoff+0.5); 
       AxisInfo::dataToPix(view,minpntX,minpntY,px,py);
       AxisInfo::pixToUser(view,px,py,0.0,ux1,uy1,uz1);
       AxisInfo::dataToPix(view,maxpntX,maxpntY,px,py);
       AxisInfo::pixToUser(view,px,py,0.0,ux2,uy2,uz2);
       ppe = 0.5*(ux1+ux2);
       pro = 0.5*(uy1+uy2);
       lpe = fabs(ux1-ux2);
       lro = fabs(uy1-uy2);
//Winfoprintf("ppe,pro,lpe,lro %f %f %f %f",ppe,pro,lpe,lro);

       int rows = xmax-xmin;
       int cols = ymax-ymin;

       int ntraces = rows*cols;
       float *data = new float[ntraces];

       float *d;
       int j, i = 0;
       for(y=ymin; y<ymax; y++) {
         for(x=xmin; x<xmax; x++) {
           ix = (int)(((double)x)/xfactor-xoff+0.5);
           iy = (int)(((double)y)/yfactor-yoff+0.5);
	   data[i] = 0.0;
           if(roi->contains(ix,iy)) {
             ind = csi_getInd(y,nx-x-1,z,ny,nx,1);
	     if(ind >= maxtraces) break;

//Winfoprintf("#ind,maxtraces,maxpts %d %d %d",ind,x,y);
             if(ss) d = ss->data + ind*maxpts;
             else d = aip_GetSpec(ind);
             if(!d) continue;

	     if(flag == LL) data[i] = *(d+pos);
	     else for(j=pos1;j<pos2;j++) data[i] += *(d+j);
//Winfoprintf("map data %d %f %f",ind,*d,data[i]);
	     data[i] *= vsFactor;
           }
	   i++;
         }
       }
       
       count++;

       // write map 
       sprintf(path,"%s_roi%d.fdf",mappath,count);
//return 1 if map writen
// load path 
       aip_writeCSIMap(path,data,count,rows,cols,ppe,pro,lpe,lro); 

       delete[] data;
   }

   return count;
}

// type: "ratio" or "rratio"
// flag: LI or LL
// key SPEC or spec
// name: file name
int SpecViewMgr::calcRatioMap(char *type,int flag, string key, string name) {
  
  // get cursor frequency, integral region, and sw
  double freq1, freq11, freq12, freq2, freq21, freq22, sw;

  if(strcmp(type,"rratio") == 0) {
    getCursorFreq2(&freq1, &freq11, &freq12, &sw);
    getCursorFreq(&freq2, &freq21, &freq22, &sw);
  } else {
    getCursorFreq(&freq1, &freq11, &freq12, &sw);
    getCursorFreq2(&freq2, &freq21, &freq22, &sw);
  }

  // determine path and name
  char path[MAXSTR], mappath[MAXSTR];
  sprintf(path,"%s",name.c_str());
  if(strlen(path) < 1) { 
     if(flag==LL) sprintf(path,"rmap_ll_%.2f_%.2f",freq_to_csPPM(freq1),freq_to_csPPM(freq2));
     else sprintf(path,"rmap_li_%.2f_%.2f",freq_to_csPPM(freq1),freq_to_csPPM(freq2));
  } else if(path[strlen(path)-1]=='_') {
     char tmp[MAXSTR];
     sprintf(tmp,"%s%.2f_%.2f",path,freq_to_csPPM(freq1),freq_to_csPPM(freq2));
     strcpy(path,tmp);
  }
  if(path[0] != '/') {
     char dir[MAXSTR];
     getDefaultMapPath(dir);
     sprintf(mappath,"%s/%s",dir,path);
  } else {
     sprintf(mappath,"%s",path);
  }

  GframeManager *gfm = GframeManager::get();
  GframeList::iterator gfi;
  spGframe_t gf = gfm->getFirstSelectedFrame(gfi);
  if(gf == nullFrame) gf = gfm->getFirstFrame(gfi);

   if(gf == nullFrame) return 0;

   spViewInfo_t view = gf->getFirstView();
   if(view == nullView) return 0;

   spImgInfo_t img = gf->getFirstImage();
   if (img == nullImg) return 0;

   spDataInfo_t data = img->getDataInfo();
   if(data == (spDataInfo_t)NULL)  return 0;

   // get base image info
   int ddimx,ddimy;
   double lpe,lro,ppe,pro;
   ddimx=data->getMedium();
   ddimy=data->getFast();
   data->st->GetValue("span", lpe, 0);
   data->st->GetValue("span", lro, 1);
   data->st->GetValue("location", ppe, 0);
   data->st->GetValue("location", pro, 1);

   // get csi grid info
   int nx,ny,nz,ns;
   csi_getCSIDims(&nx,&ny,&nz,&ns);
   if(nx < 1 || ny < 1) return 0;

   iplan_stack *stack = (iplan_stack *)malloc(sizeof(iplan_stack));
   getStack(stack,CSI2D);
   stack->ppe=-stack->ppe; // reverse x 

   // scaling factor to convert points of base image to csi grid
   double xfactor, yfactor;
   if(stack->lpe == 0 || stack->lro == 0 || ddimx < 1 || ddimy < 1) {
     xfactor=1;
     yfactor=1;
   } else {
     xfactor = (double)nx*lpe/((double)ddimx*(stack->lpe));
     yfactor = (double)ny*lro/((double)ddimy*(stack->lro));
   }

   // offset relative to base image
   double xoff, yoff;
   if(stack->lpe <= 0 || stack->lro <= 0) {
     xoff=0.0;
     yoff=0.0;
   } else {
     xoff = (0.5*stack->lpe - stack->ppe - 0.5*lpe + ppe)*ddimx/lpe;
     yoff = (0.5*stack->lro - stack->pro - 0.5*lro + pro)*ddimy/lro;
   }
   
   specStruct_t *ss = gf->getSpecList()->get2DCSISpecStruct(key);

   int maxtraces, maxpts;
   if(ss) {
     maxpts = ss->matrix[0];
     maxtraces = ss->matrix[1]*ss->matrix[2];
//Winfoprintf("spec data size %d %d",maxpts,maxtraces);
   } else {
     maxpts = getSpecPoints();
     maxtraces = getNtraces(); 
//Winfoprintf("SPEC data size %d %d",maxpts,maxtraces);
   }

  int pos1 = (int) ((sw - freq1) * (double)maxpts / sw);
  int pos11 = (int) ((sw - freq11) * (double)maxpts / sw);
  int pos12 = (int) ((sw - freq12) * (double)maxpts / sw);
  int pos2 = (int) ((sw - freq2) * (double)maxpts / sw);
  int pos21 = (int) ((sw - freq21) * (double)maxpts / sw);
  int pos22 = (int) ((sw - freq22) * (double)maxpts / sw);
//Winfoprintf("1peak info %f %f %f %f %d %d %d",freq1, freq11, freq12, sw,pos1,pos11,pos12);
//Winfoprintf("2peak info %f %f %f %f %d %d %d",freq2, freq21, freq22, sw,pos2,pos21,pos22);

   int count = 0;
   int ind,x,y,z=0;
   int ix,iy;
   int xmin,ymin,xmax,ymax;
   double ux1,uy1,uz1,ux2,uy2,uz2;
   double px,py;
   Roi *roi;
   D3Dpoint_t minpnt, maxpnt;
   for (roi = gf->getFirstRoi(); roi; roi = gf->getNextRoi()) {
       if(roi->GetType() == ROI_LINE || roi->GetType() == ROI_POINT) continue;

       roi->setMinMaxDataPnts();
       minpnt = roi->getminpntData();
       maxpnt = roi->getmaxpntData();
       if(minpnt.x == maxpnt.x || minpnt.y == maxpnt.y) continue;

       xmin = (int)(xfactor*(minpnt.x+xoff)+0.5);
       ymin = (int)(yfactor*(minpnt.y+yoff)+0.5);
       xmax = (int)(xfactor*(maxpnt.x+xoff)+0.5);
       ymax = (int)(yfactor*(maxpnt.y+yoff)+0.5);
       // check boundary
       if(xmin < 0) xmin=0;
       if(ymin < 0) ymin=0;
       if(xmax > nx) xmax=nx;
       if(ymax > ny) ymax=ny;

       // determine roi FOV and pos
       int minpntX = (int)(((double)xmin)/xfactor-xoff+0.5); 
       int maxpntX = (int)(((double)xmax)/xfactor-xoff+0.5); 
       int minpntY = (int)(((double)ymin)/yfactor-yoff+0.5); 
       int maxpntY = (int)(((double)ymax)/yfactor-yoff+0.5); 
       AxisInfo::dataToPix(view,minpntX,minpntY,px,py);
       AxisInfo::pixToUser(view,px,py,0.0,ux1,uy1,uz1);
       AxisInfo::dataToPix(view,maxpntX,maxpntY,px,py);
       AxisInfo::pixToUser(view,px,py,0.0,ux2,uy2,uz2);
       ppe = 0.5*(ux1+ux2);
       pro = 0.5*(uy1+uy2);
       lpe = fabs(ux1-ux2);
       lro = fabs(uy1-uy2);
//Winfoprintf("ppe,pro,lpe,lro %f %f %f %f",ppe,pro,lpe,lro);

       int rows = xmax-xmin;
       int cols = ymax-ymin;

       int ntraces = rows*cols;
       float *data = new float[ntraces];

       float *d,d1,d2;
       int j, i = 0;
       for(y=ymin; y<ymax; y++) {
         for(x=xmin; x<xmax; x++) {
           ix = (int)(((double)x)/xfactor-xoff+0.5);
           iy = (int)(((double)y)/yfactor-yoff+0.5);
	   data[i] = 0.0;
           if(roi->contains(ix,iy)) {
             ind = csi_getInd(y,nx-x-1,z,ny,nx,1);
	     if(ind >= maxtraces) break;

             if(ss) d = ss->data + ind*maxpts;
             else d = aip_GetSpec(ind);
             if(!d) continue;

	     if(flag == LL) {
		d1=*(d+pos1);
		d2=*(d+pos2);
//Winfoprintf("#ind,maxtraces,maxpts %d %d %d %f %f %f",ind,x,y,d1,d2,d1/d2);
		if(d2!=0.0) data[i] = d1/d2;
		else {
		   data[i] = d1;
		   Winfoprintf("Warning: denominator height is zero for voxel %d %d",ix,iy);
		}
	     } else {
		d1=d2=0.0;	
		for(j=pos11;j<pos12;j++) d1 += *(d+j);
		for(j=pos21;j<pos22;j++) d2 += *(d+j);
//Winfoprintf("###ind,maxtraces,maxpts %d %d %d %f %f %f",ind,x,y,d1,d2,d1/d2);
		if(d2!=0.0) data[i] = d1/d2;
                else {
                   data[i] = d1;
                   Winfoprintf("Warning: denominator integ is zero for voxel %d %d",ix,iy);
                }
	     }
           }
	   i++;
         }
       }
       
       count++;

       // write map 
       sprintf(path,"%s_roi%d.fdf",mappath,count);
       aip_writeCSIMap(path,data,count,rows,cols,ppe,pro,lpe,lro); 

       delete[] data;
   }

   // TODO: 
/*
   if(count==0) { // no ROI, so calculate map for the entire FOV
   }
*/

  return proc_complete;
}

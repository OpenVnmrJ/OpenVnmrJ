/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <cmath>
using std::sqrt;
#include <algorithm>
using std::find;

#include <string.h>

#include "aipCommands.h"
using namespace aip;
#include "aipDataInfo.h"
#include "aipDataManager.h"
#include "aipImgInfo.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipVnmrFuncs.h"
#include "aipInterface.h"
#include "aipCInterface.h"
#include "aipSpecDataMgr.h"
#include "aipSpecViewMgr.h"
#include "AspFrameMgr.h"
#include "AspMouse.h"

namespace {
    DisplayListenerList displayListenerList;
    MouseListenerList mouseListenerList;
}

void
aipDeleteFrames()
{
    GframeManager::get()->deleteAllFrames();
}

int
aipDisplayFile(char *path, int frame)
{
    string key = DataManager::get()->displayFile(path, frame);
    return (key == "") ? 0 : 1;
}

void
aipAutoScale()
{
    int vsMode = VsInfo::getVsMode();
    if (vsMode == VS_UNIFORM) {
        DataManager *dm = DataManager::get();
        set<string> keys = dm->getKeys(DATA_ALL);
        VsInfo::autoVsGroup(keys);
    }
}

int
aipJavaFile(char *path, int *nx, int *ny, int *ns, float *sx, float *sy, float *sz, float **jdata)
{
  string key = DataManager::get()->javaFile(path, nx, ny, ns,sx,sy,sz,jdata);
    return (key == "") ? 0 : 1;
}

void
aipRegisterDisplayListener(PDLF_t func)
{
    DisplayListenerList::iterator begin = displayListenerList.begin();
    DisplayListenerList::iterator end = displayListenerList.end();
    if (func && find(begin, end, func) == end) {
	// Only insert it if non-null and not already there
	displayListenerList.push_back(func);
    }
}

void
aipUnregisterDisplayListener(PDLF_t func)
{
    if (func) {
	displayListenerList.remove(func);
    }
}

int
aipGetNumberOfImages()
{
    return GframeManager::get()->getNumberOfImages();
}

int
aipOwnsScreen()
{
    return (int)aipHasScreen();
}

int
aipGetImageIds(int len, int *ids)
{
    spGframe_t gf;
    GframeManager *gfm = GframeManager::get();
    int n = 0;
    GframeList::iterator gfi;
    for (gf=gfm->getFirstFrame(gfi);
	 gf != nullFrame && n < len;
	 gf=gfm->getNextFrame(gfi)) {
	if (gf->getFirstView() != nullView)
	{
	    ids[n++] = gf->id;
	}
    }
    return n+1;
}

int
aipGetImagePath(int id, char *buf, int buflen)
{
    int pathlen = 0;
    buf[0] = '\0';
    spGframe_t gf;
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    for (gf=gfm->getFirstFrame(gfi);
         gf != nullFrame;
         gf=gfm->getNextFrame(gfi))
    {
	if (gf->id == id) {
	    spImgInfo_t img = gf->getFirstImage();
            if (img != nullImg) {
                const char *path = img->getDataInfo()->getFilepath();
                pathlen = strlen(path);
                if (pathlen < buflen) {
                    strcpy(buf, path);
                } else {
                    strncpy(buf, path, buflen);
                    buf[buflen - 1] = '\0';
                }
            }
            return pathlen;
        }
    }
    return pathlen;
}

int
aipGetFrameInfo(int id, CFrameInfo_t *cinfo)
{
    spGframe_t gf;
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    for (gf=gfm->getFirstFrame(gfi);
         gf != nullFrame;
         gf=gfm->getNextFrame(gfi))
    {
	if (gf->id == id) {
	        cinfo->framestx = gf->minX();
	        cinfo->framesty = gf->minY();
	        cinfo->framewd = gf->maxX() - cinfo->framestx + 1;
	        cinfo->frameht = gf->maxY() - cinfo->framesty + 1;
		return true;
        }
    }
    return false;
}

int
aipGetImageInfo(int id, CImgInfo_t *cinfo)
{
    int i, j;
    spGframe_t gf;
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    for (gf=gfm->getFirstFrame(gfi);
         gf != nullFrame;
         gf=gfm->getNextFrame(gfi))
    {
	if (gf->id == id) {
	    cinfo->id = id;
	    spViewInfo_t vi = gf->getFirstView();
	    if (vi == nullView) {
	        cinfo->framestx = gf->minX();
	        cinfo->framesty = gf->minY();
	        cinfo->framewd = gf->maxX() - cinfo->framestx + 1;
	        cinfo->frameht = gf->maxY() - cinfo->framesty + 1;
		return false;
	    }
	    spDataInfo_t di = gf->getFirstImage()->getDataInfo();
	    for (i=0; i<3; i++) {
		cinfo->location[i] = di->getLocation(i);
		cinfo->roi[i] = di->getRoi(i);
	    }
 	    di->getEuler(cinfo->euler);

	    cinfo->framestx = gf->minX();
	    cinfo->framesty = gf->minY();
	    cinfo->framewd = gf->maxX() - cinfo->framestx + 1;
	    cinfo->frameht = gf->maxY() - cinfo->framesty + 1;

	    cinfo->pixstx = vi->pixstx;
	    cinfo->pixsty = vi->pixsty;
	    cinfo->pixwd = vi->pixwd;
	    cinfo->pixht = vi->pixht;

            cinfo->coils = 1;
            vi->imgInfo->getDataInfo()->st->GetValue("coils",cinfo->coils);
            cinfo->coil = 1;
            vi->imgInfo->getDataInfo()->st->GetValue("coil",cinfo->coil);
            cinfo->origin[0] = 0.0;
            vi->imgInfo->getDataInfo()->st->GetValue("origin",cinfo->origin[0]);
            cinfo->origin[1] = 0.0;
            vi->imgInfo->getDataInfo()->st->GetValue("origin",cinfo->origin[1]);
            cinfo->origin[2] = 0.0;
            vi->imgInfo->getDataInfo()->st->GetValue("origin",cinfo->origin[2]);

	    for (i=0; i<3; i++) {
		for (j=0; j<4; j++) {
		    cinfo->p2m[i][j] = gf->p2m[i][j];
		    cinfo->m2p[i][j] = gf->m2p[i][j];
		}
	    }

	    for (i=0; i<3; i++) {
	        for (j=0; j<3; j++) {
		    cinfo->b2m[i][j] = di->b2m[i][j];
		}
	    }

	    double x = cinfo->m2p[0][0];
	    double y = cinfo->m2p[0][1];
	    double z = cinfo->m2p[0][2];
	    cinfo->pixelsPerCm = sqrt(x * x + y * y + z * z);
	    return true;
	}
    }
    return false;
}

int
aipRefreshImage(int id)
{
    spGframe_t gf;
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    for (gf=gfm->getFirstFrame(gfi);
         gf != nullFrame;
         gf=gfm->getNextFrame(gfi))
    {
	if (gf->id == id && gf->getFirstView() != nullView) {
	    gf->draw();
	    return true;
	}
    }
    return false;
}

/*
 * Notify listeners of one changed image
 */
void
aipCallDisplayListeners(int id, bool changeFlag)
{
    DisplayListenerList::iterator itr;
    int k = displayListenerList.size();

    // for (itr=displayListenerList.begin();
    //	 itr != displayListenerList.end();
    //	 ++itr)

    itr = displayListenerList.begin();
    while ( k > 0 )
    {
	PDLF_t func = *itr;
	int ids[1];
	int flags[1];
	*ids = id;
	*flags = changeFlag;
	(*func)(1, ids, flags);
        k--;
        itr++;
    }
}

/*
 *
 * Mouse Listener Stuff
 *
 */

void
aipRegisterMouseListener(PMLF_t func, PMQF_t )
{
    MouseListenerList::iterator begin = mouseListenerList.begin();
    MouseListenerList::iterator end = mouseListenerList.end();
    if (func && find(begin, end, func) == end) {
	// Only insert it if non-null and not already there
	mouseListenerList.push_back(func);
    }
}

void
aipUnregisterMouseListener(PMLF_t func, PMQF_t )
{
    if (func) {
	mouseListenerList.remove(func);
    }
}

void
aipCallMouseListeners(int x, int y, int button, int mask, int dummy)
{
    MouseListenerList::iterator itr;
    int k = mouseListenerList.size();

    //   for (itr=mouseListenerList.begin();
    // 	 itr != mouseListenerList.end();
    //	 ++itr)
    itr = mouseListenerList.begin();
    while ( k > 0 )
    {
	PMLF_t func = *itr;
	(*func)(x, y, button, mask, dummy);
        k--;
        itr++;
    }
}


//
//
//   TEST STUFF
//
//

extern "C"
{
    static void testListener(int len, int *ids, int *changeFlags);
    static void testMouseListener(int x, int y, int button, int mask, int dum);
}

static void
testListener(int len, int *ids, int *changeFlags)
{
    int i;
    CImgInfo_t cinfo;

    for (i=0; i<len; i++) {
	fprintf(stderr,"testListener: id=%4d,  change=%d\n",
		ids[i], changeFlags[i]);
	aipGetImageInfo(ids[i], &cinfo);
	fprintf(stderr,"%dx%d+%d+%d\n",
		cinfo.pixwd, cinfo.pixht, cinfo.pixstx, cinfo.pixsty);
    }
}

/* VNMRCOMMAND */
int
aipTestDisplayListener(int argc, char *argv[], int retc, char *retv[])
{
    if (argc > 1) {
	aipUnregisterDisplayListener(testListener);
    } else {
	aipRegisterDisplayListener(testListener);
	int n = aipGetNumberOfImages();
	int *ids = new int[n];
	aipGetImageIds(n, ids);
	for (int i=0; i<n; i++) {
	    CImgInfo_t ci;
	    aipGetImageInfo(ids[i], &ci);
	    fprintf(stderr,"id=%d: %dx%d+%d+%d\n",
		    ci.id, ci.pixwd, ci.pixht, ci.pixstx, ci.pixsty);
	}
	delete[] ids;
    }
    return proc_complete;
}

static void
testMouseListener(int x, int y, int button, int mask, int dum)
{
    fprintf(stderr,"testMouseListener: x=%d, y=%d, mask=0x%08x\n", x, y, mask);
}

/* VNMRCOMMAND */
int
aipTestMouseListener(int argc, char *argv[], int retc, char *retv[])
{
    if (argc > 1) {
	aipUnregisterMouseListener(testMouseListener, NULL);
    } else {
	aipRegisterMouseListener(testMouseListener, NULL);
    }
    return proc_complete;
}

int 
aipGetHdrReal(char *ky, char *nm, int idx, double *value)
{
    string key = ky;
    string name = nm;
    if(DataManager::get()->getHeaderReal(key, name, idx, value)) {
	return proc_complete;
    } else {
	return proc_error;
    }
}

int 
aipGetHdrInt(char *ky, char *nm, int idx, int *value)
{
    string key = ky;
    string name = nm;
    if(DataManager::get()->getHeaderInt(key, name, idx, value)) {
	return proc_complete;
    } else {
	return proc_error;
    }
}

int 
aipGetHdrStr(char *ky, char *nm, int idx, char **value)
{
    string key = ky;
    string name = nm;
    if(DataManager::get()->getHeaderStr(key, name, idx, value)) {
	return proc_complete;
    } else {
	return proc_error;
    }
}

/* VNMRCOMMAND */
int
aipScreen(int argc, char *argv[], int retc, char *retv[])
{
    if(retc > 0 && aipHasScreen()) {
       retv[0] = realString((double)1.0);
    } else if(retc > 0) {
       retv[0] = realString((double)0.0);
    }
    return proc_complete;
}

void 
aipSetReconDisplay(int nimages)
{

    ReviewQueue::get()->setReconDisplay(nimages);
}

void
aipUpdateRQ()
{
    ReviewQueue::get()->updateStudies();
    ReviewQueue::get()->notifyVJ("loadData");
}

void aipMouseWheel(int clicks, double factor)
{
    Mouse::wheelEvent(clicks, factor);
}

specStruct_t *aipGetSpecStruct(char *key) {
    spSpecData_t sd =  SpecDataMgr::get()->getDataByKey(string(key));
    if(sd != (spSpecData_t)NULL) return sd->getSpecStruct();
    else return NULL;
}

void aipSpecViewUpdate() {
   SpecViewMgr::get()->specViewUpdate();
}

float *aipGetTrace(char *key, int ind, double scale, int npt) {
    return SpecDataMgr::get()->getTrace(string(key), ind, scale, npt);
}

int loadFdfSpec(char *path, char *key) {
    return SpecDataMgr::get()->loadSpec(string(key), path);
}

void aipSelectViewLayer(int imgId, int order, int mapId) {
  GframeManager *gfm = GframeManager::get();
  GframeList::iterator gfi;
  spGframe_t gf=gfm->getFirstSelectedFrame(gfi);
  if(gf == nullFrame) gf=gfm->getFirstFrame(gfi); 
  if(gf != nullFrame) gf->selectViewLayer(imgId,order,mapId); 
}

int aspFrame(char *keyword, int frameID, int x, int y, int w, int h) {
   return AspFrameMgr::get()->frameFunc(keyword, frameID, x, y, w, h);
}

void aspMouseWheel(int clicks, double factor)
{
    AspMouse::wheelEvent(clicks, factor);
}


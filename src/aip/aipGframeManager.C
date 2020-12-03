/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <iostream>
#include <list>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

using namespace std;

#include "sharedPtr.h"
#include "aipStderr.h"
#include "aipCommands.h"
#include "aipGraphicsWin.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipDataManager.h"
#include "aipVnmrFuncs.h"
#include "aipVsInfo.h"
#include "aipMovie.h"
#include "aipWinMovie.h"
#include "aipOrthoSlices.h"
#include "aipVolData.h"
#include "aipImgOverlay.h"

#include "group.h"

using namespace aip;

extern "C" {
void redrawCanvas();
void graph_batch(int on);
}

GframeManager* GframeManager::gframeManager= NULL;
frameKey_t GframeManager::nullKey = make_pair(-1, -1);
bool GframeManager::isFullScreen = false;
bool GframeManager::overlaid = false;
//spGframe_t    GframeManager::blownUpFrame; // Initialized by toggleFullScreen


#ifdef TEST
/***********  TEST  **************/
extern bool memTest();
bool memTest() {
    int i;
    void *c[10000];
    bool rtn = DataManager::get()->dataCheck();
    for (i = 0; i < 10000; ++i) {
        //for (i = 10000 - 1; i >= 0; --i) {
        c[i] = malloc((int)(2 + i/4));
    }
    for (i = 0; i < 10000; ++i) {
        //for (i = 10000 - 1; i >= 0; --i) {
        free(c[i]);
    }
    return rtn;
}
/***********  END TEST  **************/
#endif

/* PRIVATE CONSTRUCTOR */
GframeManager::GframeManager() {
    gframeList = new GframeList;
    selectedGframeList = new GframeList;
    gframeCache = new GframeCache_t;
    loadKey = nullKey;
    nCols = nRows = 1;
    splitWinWd = splitWinHt = 0;
    activeGframe = nullFrame;
}

/*
 * Returns the one GframeManager instance.  Creates one if it has
 * not been created yet.  Updates window size.
 */
/* PUBLIC STATIC */
GframeManager *GframeManager::get() {
    if (!gframeManager) {
        gframeManager = new GframeManager();
    }
    gframeManager->getWinsize();
    return gframeManager;
}

/**
 * Get the key for a given frame.  (Used to put it in the gframeList.)
 */
frameKey_t GframeManager::getKey(spGframe_t gf) {
    return make_pair(gf->row, gf->col);
}

/**
 * Get the frame for a given key.
 */
spGframe_t GframeManager::getFrameByKey(int row, int col) {
    frameKey_t key = make_pair(row, col);
    GframeList::iterator p = gframeList->find(key);
    if (p != gframeList->end()) {
        return p->second;
    } else {
        return nullFrame;
    }
}

/**
 * Retrieve a frame from the cache
 */
spGframe_t GframeManager::getCachedFrame(const string key) {
    spGframe_t gf = nullFrame;
    GframeCache_t::iterator p = gframeCache->find(key);
    if (p != gframeCache->end()) {
        return p->second;
    } else {
        return nullFrame;
    }
}

/**
 * Remove a frame from the cache
 */
spGframe_t GframeManager::unsaveFrame(const string key) {
    spGframe_t gf = getCachedFrame(key);
    gframeCache->erase(key);
    return gf;
}

/**
 * Remove all frames from the cache
 */
void GframeManager::clearFrameCache() {
    gframeCache->clear();
}

/**
 * Put a frame in the cache
 */
bool GframeManager::saveFrame(spGframe_t gf) {
    if (gf == nullFrame) {
        return false;
    }
    /*fprintf(stderr,"saveFrame(): (col %d, row %d) at (%d, %d)\n",
     gf->col, gf->row, gf->pixstx, gf->pixsty); */ /*CMP*/
    spImgInfo_t img = gf->getFirstImage();
    if (img == nullImg) {
        return false;
    }
    const string key = img->getDataInfo()->getKey();
    gframeCache->erase(key);
    gframeCache->insert(GframeCache_t::value_type(key, gf));
    return true;
}

/*
 * Split the window into some number of gframes.
 * Syntax: aipSplitWindow(nframes <,width, height>)
 *         aipSplitWindow(nrows, ncols)
 * Examples: aipSplitWindow(4)      Make 4 gframes
 *           aipSplitWindow(4, 7, 10)   Make 4 gframes with 7:10 aspect ratio
 *           aipSplitWindow(4, 0.7, 1)  Same as above
 *           aipSplitWindow(3, 5)   Split into 3 rows by 5 columns
 */
/* PUBLIC STATIC VNMRCOMMAND */
int GframeManager::aipSplitWindow(int argc, char *argv[], int retc,
        char *retv[]) {
    double aspect = 1; // Width / height ratio desired
    int nr; // Number of rows
    int nc; // Number of columns
    int nf; // Number of frames

    GframeManager *gfm = GframeManager::get(); // Get the gframe manager
    DataManager *dm = DataManager::get();
    
 //   if (VolData::get()->showingObliquePlanesPanel()) {
   //     ib_errmsg("Cannot change layout in 2D Extract Mode - see 3D Tools Panel\n");
     //   return proc_complete;
  //  }


    switch (argc) {
    case 1: { // No args: Default split
        //aspect = dm->getAspectRatio();
        nf = (int)getReal("aipFrameDefaultMax", 3);
        int ni = dm->getNumberOfImages();
        if (0 < ni && ni < nf) {
            nf = ni;
        }
        gfm->splitWindow(nf, aspect);
        break;
    }
    case 4: { // 3 args: argv[2]/argv[3] = aspect ratio
        double w;
        if (sscanf(argv[2], "%lf", &w) != 1|| w <= 0) {
            w = 1;
        }
        double h;
        if (sscanf(argv[3], "%lf", &h) != 1|| h <= 0) {
            h = 1;
        }
        aspect = w / h;
        /* FALL THROUGH */
    }
    case 2: { // One arg: argv[1] = min number of gframes
        if (strcmp(argv[1], "all") == 0) {
            nf = dm->getNumberOfImages();
        } else {
            nf = atoi(argv[1]);
        }
        if (nf <= 0) {
            nf = 1;
        }
        gfm->splitWindow(nf, aspect);
        //gfm->getSplit(nf, aspect, nr, nc);
        break;
    }
    case 3: { // Two args: argv[1] = nrows, argv[2] = ncols
        nr = atoi(argv[1]);
        if (nr <= 0) {
            nr = 1;
        }
        nc = atoi(argv[2]);
        if (nc <= 0) {
            nc = 1;
        }
        gfm->splitWindow(nr, nc);
        break;
    }
    default: {
        // Error msg?
        return proc_error;
    }
    }

    //gfm->makeFrames(nr, nc);
    //gfm->draw();

    return proc_complete;
}

int GframeManager::aipOverlayGroup(int argc, char *argv[], int retc, char *retv[]) {
   if(argc<2) {
     Winfoprintf("Usage: aipOverlayGroup(fullpath).");
     return proc_complete;
   }

   GframeManager *gfm = GframeManager::get();
   spGframe_t gf;
   GframeList::iterator gfi;

   RQgroup *group = ReviewQueue::get()->getGroup(argv[1]);
   if(group == (RQgroup *)NULL) {
	// in case of load 3D data, set the flag so DataManager will not replace dataInfo in VolData. 
	VolData::get()->setOverlayFlg(true); 
	ReviewQueue::get()->loadData(argv[1],"",1,false);
	group = ReviewQueue::get()->getGroup(argv[1]);
	VolData::get()->setOverlayFlg(false);
   }

   if(group == (RQgroup *)NULL) {
	return proc_complete;
   }

   for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
            =gfm->getNextFrame(gfi)) {
	if(gf->getFirstView() == nullView) continue;

	list<string> keys = ImgOverlay::getOverlayImages(gf, group);
        if(keys.size() < 1) continue;

        list<string>::iterator itr;
	for (itr = keys.begin(); itr != keys.end(); ++itr) {
	  gf->loadOverlayImg((char *)itr->c_str(),"default.color"); 
	}
   }
   return proc_complete;
}

// aipMoveOverlay(x,y)  move selected overlay image by x,y
// aipMoveOverlay(x,y,'all') move all overlay images
// aipMoveOverlay:$x,$y return offset for selected overlay image.
// x,y is in mm, and is offset of position relative to the base image.
int GframeManager::aipMoveOverlay(int argc, char *argv[], int retc, char *retv[]) {

    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeList::iterator gfi;
    spViewInfo_t view;

    if(argc==1 && retc>0) {
	// try selected frames
       for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextSelectedFrame(gfi)) {
	  if(gf->getViewCount() < 2) continue; 

          view = gf->getSelView();
          if (view != nullView) {
	    retv[0]=realString(10.0*view->cm_off[0]);
	    if(retc>1)
	      retv[1]=realString(10.0*view->cm_off[1]);
	    if(retc>2)
	      retv[2]=realString(10.0*view->cm_off[2]);
    	    return proc_complete;
	  }
       }

       // try all frames if no frame is selected
       for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
            =gfm->getNextFrame(gfi)) {
	  if(gf->getViewCount() < 2) continue; 

          view = gf->getSelView();
          if (view != nullView) {
	    retv[0]=realString(10.0*view->cm_off[0]);
	    if(retc>1)
	      retv[1]=realString(10.0*view->cm_off[1]);
	    if(retc>2)
	      retv[2]=realString(10.0*view->cm_off[2]);
    	    return proc_complete;
	  }
       }
       return proc_complete;
    }
   
    // move overlay

    double x=0.0, y=0.0;
    // note, input is in mm that need to be convered to cm.
    if(argc>1) x = 0.1*atof(argv[1]);
    if(argc>2) y = 0.1*atof(argv[2]);

    bool sel=true;
    if(argc>3 && strcasecmp(argv[3],"all")==0) sel=false;

    if(sel) 
    for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextSelectedFrame(gfi)) {
      if(gf->getViewCount() < 2) continue; 

      view = gf->getSelView();
      if (view != nullView) {
        gf->resetOverlayPan();
	view->cm_off[0]=x;
	view->cm_off[1]=y;
        gf->fitOverlayView(view);
	gf->draw();
	if(sel) return proc_complete;
      }
    }

    for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
            =gfm->getNextFrame(gfi)) {
      if(gf->getViewCount() < 2) continue; 

      view = gf->getSelView();
      if (view != nullView) {
        gf->resetOverlayPan();
	view->cm_off[0]=x;
	view->cm_off[1]=y;
        gf->fitOverlayView(view);
	gf->draw();
	if(sel) return proc_complete;
      }
    }

    return proc_complete;
}

// aipOverlayFrames('overlay'<,frameID1,frameID2,...>) overlay images in selected frames.
// aipOverlayFrames('unoverlay') unoverlay images.
// aipOverlayFrames('overlaid'):$ret
// aipOverlayFrames('canOverlay'):$ret
int GframeManager::aipOverlayFrames(int argc, char *argv[], int retc, char *retv[]) {
  if(argc<1) {
    Winfoprintf("Usage: aipOverlayFrames('canOverlay'/'overlaid'/'overlay'/'unoverlay')<:$ret>");
    return proc_complete;
  }

  GframeManager *gfm = GframeManager::get();

  if(retc>0 && argc>1 && strcasecmp(argv[1],"overlaid")==0) {
    if(GframeManager::overlaid)
      retv[0] = realString((double)1);
    else
      retv[0] = realString((double)0);

    return proc_complete;
  }

  if(retc>0 && argc>1 && strcasecmp(argv[1],"canOverlay")==0) {
    if(gfm->getNumSelectedFrames()>1)
      retv[0] = realString((double)1);
    else
      retv[0] = realString((double)0);

    return proc_complete;
  }

  if(argc>1 && strcasecmp(argv[1],"overlay")==0) {
    if(gfm->getNumSelectedFrames()<=1) {
      Winfoprintf("Use ctrl+left mouse button to select two or more images to overlay.");
      GframeManager::overlaid=false;
      return proc_complete;
    } 

    spGframe_t gf;
    GframeList::iterator gfi;
    spViewInfo_t view;
    list<string> keyList;
    list<int> mapList;
    for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextSelectedFrame(gfi)) {
          view = gf->getFirstView();
          if (view == nullView) continue; 
          keyList.push_back(view->imgInfo->getDataInfo()->getKey());
          mapList.push_back(view->imgInfo->getColormapID());
    }
      
    if(keyList.size()<=1) {
        Winfoprintf("Use ctrl+left mouse button to select two or more images to overlay.");
        GframeManager::overlaid=false;
        return proc_complete;
    }
  
    // load base image
    list<string>::iterator itr = keyList.begin();
    list<int>::iterator mapItr = mapList.begin();
    DataMap *dmap = DataManager::get()->getDataMap();
    DataMap::iterator pd = dmap->find(*itr);
    if(pd == dmap->end()) { 
         Winfoprintf("Cannot overlay images: base image is not loaded.");
         GframeManager::overlaid=false;
         return proc_complete;
    } 
    
    setReal("aipAutoLayout",0,true);
    gfm->splitWindow(1, 1);

    if(!gfm->loadData(pd->second)) {
         Winfoprintf("Cannot overlay images: error loading base image.");
         GframeManager::overlaid=false;
         return proc_complete;
    } 
   
    // load overlay image(s)
    gf = gfm->getFirstFrame(gfi);
    itr++;
    mapItr++;
    if(gf != nullFrame) {
	while(itr != keyList.end()) {
           if((*mapItr) != 4) // 4 is default gray scale 
	     gf->loadOverlayImg((char *)(*itr).c_str(),"default.color", *mapItr);
	   else
	     gf->loadOverlayImg((char *)(*itr).c_str(),"default.color");
	   itr++;
	   mapItr++;
        }
	gf->sendImageInfo();
    }

    GframeManager::overlaid=true;

    return proc_complete;
  }

  if(argc>1 && strcasecmp(argv[1],"unoverlay")==0) {
	
    spGframe_t gf;
    GframeList::iterator gfi;
    for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
            =gfm->getNextFrame(gfi)) {
	gf->removeOverlayImg();
    }
    
    setReal("aipAutoLayout",1,true);
    ReviewQueue::get()->display();
    GframeManager::overlaid=false;
    gf=gfm->getFirstFrame(gfi);
    if(gf != nullFrame) gf->sendImageInfo();
  }
  return proc_complete;
}

// aipViewLayers('names'<,frameID>):$n,$names
// aipViewLayers('keys'<,frameID>):$n,$keys
// aipViewLayers('hasOverlay'<,frameID>):$n
// aipViewLayers('remove'<,frameID>) remove overlaid images
int GframeManager::aipViewLayers(int argc, char *argv[], int retc, char *retv[]) {

  if(argc<1) {
    Winfoprintf("Usage: aipViewLayers('remove'/'names'/'keys')<:$names>");
    return proc_complete;
  }

  GframeManager *gfm = GframeManager::get();
  spGframe_t gf;
  GframeList::iterator gfi;
  
/*
  if(argc>1 && strcasecmp(argv[1],"setImgInfo")==0) {
    int f = 1;
    if(argc>2) f = atoi(argv[2]);
    if(f<1) set_aip_image_info(0,0,0,0,"",0);
    else {
      int i = 0;
      for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
            =gfm->getNextFrame(gfi), i++) {
	if(gf != nullFrame && i == (f-1)) gf->sendImageInfo();
      }
    }
  
    return proc_complete;
  }
*/

  if(argc>1 && strcasecmp(argv[1],"hasOverlay")==0) {
    if(retc<1) return proc_complete;
    int i=0;
    if(argc>2) {
      gf = gfm->getFrameByNumber(atoi(argv[2]));
      if(gf != nullFrame && gf->getFirstView() != nullView) i = (gf->getViewCount()-1);
    } else { 
      for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
            =gfm->getNextFrame(gfi)) {
	if(gf->getFirstView() != nullView) i += (gf->getViewCount()-1);
      }
    }
    if(i>0) retv[0] = realString((double)1);
    else retv[0]  = realString((double)0); 
    return proc_complete;
  }

  if(argc>1 && strcasecmp(argv[1],"remove")==0) {
    if(argc>2) {
      gf = gfm->getFrameByNumber(atoi(argv[2]));
      if(gf != nullFrame) {
	gf->removeOverlayImg();
    	gf->sendImageInfo();
      }
    } else { 
      for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
            =gfm->getNextFrame(gfi)) {
	gf->removeOverlayImg();
      }
      gf=gfm->getFirstFrame(gfi);
      if(gf != nullFrame) gf->sendImageInfo();
    }
    GframeManager::overlaid=false;
    grabMouse();  // to update graphics toolbar
    return proc_complete;
  }

/*
  if(argc>2) {
     gf = gfm->getFrameByNumber(atoi(argv[2]));
  } else gf = gfm->getFirstSelectedFrame(gfi);
  
  if(gf == nullFrame) return proc_complete;

  int nv = gf->getViewCount();
  string names;
  if(argc>1 && strcasecmp(argv[1],"names") == 0) {
     names = gf->getViewNames();
  } else if(argc>1 && strcasecmp(argv[1],"keys") == 0) {
     names = gf->getViewKeys();
  } 

  char str[MAXSTR];
  sprintf(str,"%d %s",nv,names.c_str());
  retv[0] = newString(str);
*/
  return proc_complete;
}

// aipColormapOpt = 0, all loaded images 
// aipColormapOpt = 1, displayed images 
// aipColormapOpt = 2, selected image 
int GframeManager::aipSaveColormap(int argc, char *argv[], int retc, char *retv[]) {
   int mode = (int)getReal("aipColormapOpt",0);
   if(argc>1) { // overwrite aipColormapOpt
      if(strcasecmp(argv[1],"all")==0) mode = 0;
      else if(strstr(argv[1],"dis")==argv[1]) mode = 1;
      else if(strstr(argv[1],"sel")==argv[1]) mode = 2;
   }


   GframeManager *gfm = GframeManager::get();
   spGframe_t gf;
   GframeList::iterator gfi;
   spImgInfo_t img;

   if(mode == 1) { // save for displayed images
       for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
          if (!gfm->isFrameDisplayed(gf)) continue;
	  img = gf->getSelImage();
          if(img == nullImg) continue;

          string srcpath = img->getColormapPath();
          string dstpath = img->getImageCmapPath();
	  if(srcpath.find("/") != string::npos) {
	      char path[MAXSTR];
	      strcpy(path,dstpath.substr(0,dstpath.find_last_of("/")).c_str()); 
	      if(access(path, W_OK) == 0) {
	         copy_file((char *)srcpath.c_str(), (char *)dstpath.c_str());
	      } else {
		 Winfoprintf("Error create %s file.\n",dstpath.c_str());
	      }
	  }
       }
   } else if(mode == 2) {
       int i=0;
       for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextSelectedFrame(gfi)) {
	  img = gf->getSelImage();
          if(img == nullImg) continue;

          string srcpath = img->getColormapPath();
          string dstpath = img->getImageCmapPath();
	  if(srcpath.find("/") != string::npos) {
	      char path[MAXSTR];
	      strcpy(path,dstpath.substr(0,dstpath.find_last_of("/")).c_str()); 
	      if(access(path, W_OK) == 0) {
	         copy_file((char *)srcpath.c_str(), (char *)dstpath.c_str());
	      } else {
		 Winfoprintf("Error create %s file.\n",dstpath.c_str());
	      }
	  }
          i++;
       }
       if(i==0) { // no frame is selected. use first frame.
          gf=gfm->getFirstFrame(gfi);
          if(gf != nullFrame) {
	    img = gf->getSelImage();
            if(img != nullImg) { 

              string srcpath = img->getColormapPath();
              string dstpath = img->getImageCmapPath();
	      if(srcpath.find("/") != string::npos) {
	        char path[MAXSTR];
	        strcpy(path,dstpath.substr(0,dstpath.find_last_of("/")).c_str()); 
	        if(access(path, W_OK) == 0) {
	          copy_file((char *)srcpath.c_str(), (char *)dstpath.c_str());
	        } else {
		  Winfoprintf("Error create %s file.\n",dstpath.c_str());
	        }
	      }
            }
          }
       }
   } else { // save for the scan (group)
       std::list<string> paths;
       std::list<string>::iterator ip;
       for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
	  img = gf->getSelImage();
          if(img == nullImg) continue;

          string dstpath = img->getGroupCmapPath();
          int found=0;
          for (ip = paths.begin(); ip != paths.end(); ++ip) {
		if(dstpath == (*ip)) {
		  found=1;
		  break;
		} 
	  }

	  if(!found) {
             paths.push_back(dstpath);
             string srcpath = img->getColormapPath();
	     if(srcpath.find("/") != string::npos) {
	       char path[MAXSTR],name[MAXSTR];
	       strcpy(path,dstpath.substr(0,dstpath.find_last_of("/")).c_str()); 
	       string str = srcpath.substr(0,srcpath.find_last_of("/")); 
	       strcpy(name,str.substr(str.find_last_of("/")+1).c_str());
               int isDefault = (strcasecmp(name,"default") == 0);
	       if(access(path, W_OK) == 0 && isDefault) {
                 // delete existing cmap in the directory
		 gfm->deleteCmaps(path);
 	       } else if(access(path, W_OK) == 0) {
                 // delete existing cmap in the directory
		 gfm->deleteCmaps(path);
	         copy_file((char *)srcpath.c_str(), (char *)dstpath.c_str());
	       } else {
		 Winfoprintf("Error create %s file.\n",dstpath.c_str());
	       }
	     }
	  }
       }
   }
   
   return proc_complete;
}

// note, transparency = 0 to 1.0 for text, 0 to 100 for images
int GframeManager::aipSetTransparency(int argc, char *argv[], int retc, char *retv[]) {

   if(argc<2) {
        Winfoprintf("Usage: aipSetTransparency(n)");
        return proc_complete;
   }

   int mode = (int)getReal("aipColormapOpt",0);
   if(argc>2) { // overwrite aipColormapOpt
      if(strcasecmp(argv[2],"all")==0) mode = 0; // all images
      else if(strstr(argv[2],"dis")==argv[2]) mode = 1; // displayed images
      else if(strstr(argv[2],"sel")==argv[2]) mode = 2; // selected images
      else if(strstr(argv[2],"text")==argv[2]) mode = 3; // annotation, axis, text, etc...
   }

   GframeManager *gfm = GframeManager::get();
   spGframe_t gf;
   GframeList::iterator gfi;

   if(mode == 3) { // set transparency for text, axis etc...
       set_transparency_level(atof(argv[1]));
       
   } else if(mode == 1) {
       for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
                =gfm->getNextFrame(gfi)) {
            if (!gfm->isFrameDisplayed(gf)) continue;

            gf->setTransparency(atoi(argv[1]));
            gf->imgBackupOod=true;
            gf->draw();
        }

   } else if(mode == 2) {
       int i=0;
       for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextSelectedFrame(gfi)) {
          gf->setTransparency(atoi(argv[1]));
	  gf->imgBackupOod=true;
          gf->draw();
          i++;
       }
       if(i==0) { // no frame is selected. use first frame.
          gf=gfm->getFirstFrame(gfi);
          if(gf != nullFrame) {
             gf->setTransparency(atoi(argv[1]));
	     gf->imgBackupOod=true;
             gf->draw();
          }
       }
    } else {
       for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
          gf->setTransparency(atoi(argv[1]));
	  gf->imgBackupOod=true;
          gf->draw();
       }
    }
    return proc_complete;
}

int GframeManager::aipSetColormap(int argc, char *argv[], int retc, char *retv[]) {

   if(argc<2) {
        Winfoprintf("Usage: aipColormap(colormapName)");
        return proc_complete;
   }

   int mode = (int)getReal("aipColormapOpt",0);
   if(argc>2) { // overwrite aipColormapOpt
      if(strcasecmp(argv[2],"all")==0) mode = 0;
      else if(strstr(argv[2],"dis")==argv[2]) mode = 1;
      else if(strstr(argv[2],"sel")==argv[2]) mode = 2;
      else if(strstr(argv[2],"refresh")==argv[2]) mode = 3;
   }

   GframeManager *gfm = GframeManager::get();
   spGframe_t gf;
   GframeList::iterator gfi;

   if(mode == 3) {
       for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
                =gfm->getNextFrame(gfi)) {
            if (!gfm->isFrameDisplayed(gf)) continue;

            gf->imgBackupOod=true;
            gf->draw();
        }
        return proc_complete;
   }
   if(mode == 1) {
       for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
                =gfm->getNextFrame(gfi)) {
            if (!gfm->isFrameDisplayed(gf)) continue;

            gf->setColormap(argv[1]);
            gf->imgBackupOod=true;
            gf->draw();
        }

   } else if(mode == 2) {
       int i=0;
       for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextSelectedFrame(gfi)) {
          gf->setColormap(argv[1]);
	  gf->imgBackupOod=true;
          gf->draw();
          i++;
       }
       if(i==0) { // no frame is selected. use first frame.
          gf=gfm->getFirstFrame(gfi);
          if(gf != nullFrame) {
             gf->setColormap(argv[1]);
	     gf->imgBackupOod=true;
             gf->draw();
          }
       }
    } else {
       if(strcasecmp(argv[1],"default")==0) { // delete existing cmaps
         spImgInfo_t img;
         std::list<string> paths;
         std::list<string>::iterator ip;
         for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
	    img = gf->getSelImage();
            if(img == nullImg) continue;

            string dstpath = img->getGroupCmapPath();
            int found=0;
            for (ip = paths.begin(); ip != paths.end(); ++ip) {
		if(dstpath == (*ip)) {
		  found=1;
		  break;
		} 
	    }

	    if(!found) {
               paths.push_back(dstpath);
	       char path[MAXSTR];
	       strcpy(path,dstpath.substr(0,dstpath.find_last_of("/")).c_str()); 
	       if(access(path, W_OK) == 0) {
                 // delete existing cmap in the directory
		 gfm->deleteCmaps(path);
	       }
	    }
         }
       }
       for (gf = gfm->getFirstFrame(gfi); gf != nullFrame; gf
                = gfm->getNextFrame(gfi)) {
          gf->setColormap(argv[1]);
	  gf->imgBackupOod=true;
          gf->draw();
       }
    }
    return proc_complete;
}

/* PUBLIC STATIC VNMRCOMMAND */
int GframeManager::aipFullScreen(int argc, char *argv[], int retc, char *retv[]) {
    GframeManager *gfm = get();
    GframeList::iterator gfi;
    spGframe_t gf= gfm->getFirstSelectedFrame(gfi);
    gfm->toggleFullScreen(gf); // NB: gf may be NULL
    return proc_complete;
}

/* PUBLIC STATIC VNMRCOMMAND */
int GframeManager::aipClearFrames(int argc, char *argv[], int retc,
        char *retv[]) {
    if(!VolData::get()->showingObliquePlanesPanel()){
        // graph_batch(1);
        if (argc > 1&& strstr(argv[1], "sel") != NULL) {
            GframeManager::get()->clearSelectedFrames();
        } else {
            GframeManager::get()->clearAllFrames();
        }
    }
    // GframeManager::get()->draw();
    // graph_batch(0);
    return proc_complete;
}

/* PUBLIC STATIC VNMRCOMMAND */
int GframeManager::aipDeleteFrames(int argc, char *argv[], int retc,
        char *retv[]) {
    if(!VolData::get()->showingObliquePlanesPanel())
        GframeManager::get()->deleteAllFrames();
    return proc_complete;
}

/* PUBLIC STATIC VNMRCOMMAND */
int GframeManager::aipSelectFrames(int argc, char *argv[], int retc,
        char *retv[]) {
    if(!VolData::get()->showingObliquePlanesPanel()){
        strArg_t cmd = all;
        if (argc > 1) {
            const char *arg = argv[1];
            if (strcasecmp(arg, "all") == 0) {
                cmd = all;
            } else if (strcasecmp(arg, "none") == 0) {
                cmd = none;
            }
            get()->selectFrames(cmd);
        } else if(retc > 0) {
	    retv[0] = realString((double)get()->getNumSelectedFrames());
	} else {
            get()->selectFrames(cmd);
        }
    }
    return proc_complete;
}

/* STATIC VNMRCOMMAND */
int GframeManager::aipPrintImage(int argc, char *argv[], int retc, char *retv[]) {
    GframeManager *gfm = get();
    spGframe_t gf;
    double scale = getReal("aipPrintScale", 100) / 100;
    GframeList::iterator gfi;
    for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextSelectedFrame(gfi)) {
        gf->print(scale);
    }
    return proc_complete;
}

/* STATIC VNMRCOMMAND */
int GframeManager::aipDupFrame(int argc, char *argv[], int retc, char *retv[]) {
    if (argc != 3) {
        return proc_error;
    }

    int srcIdx = atoi(argv[1]);
    int dstIdx = atoi(argv[2]);

    get()->dupFrame(srcIdx, dstIdx);

    //dstFrame->loadFrame(srcFrame);


    return proc_complete;
}

#define MAXSTRLENGTH 2048

/* PUBLIC STATIC VNMRCOMMAND */
int GframeManager::aipGetPrintFrames(int argc, char *argv[], int retc,
        char *retv[]) {
    if (retc <=0)
        return proc_complete;

    char slct[MAXSTRLENGTH];
    if (argc > 1) {
        strcpy(slct, argv[1]);
    } else {
        strcpy(slct, "all");
    }

    static char buf[MAXSTRLENGTH];
    static char str[MAXSTRLENGTH];
    int nf = 0;
    *buf = '\0';

    spGframe_t gframe;
    spViewInfo_t view;

    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;

    if (strncasecmp(slct, "sel", 3) == 0) {
        for (gframe = gfm->getFirstSelectedFrame(gfi); gframe != nullFrame; gframe
                = gfm->getNextSelectedFrame(gfi)) {

            view = gframe->getFirstView();
            if (view == nullView)
                continue;

            sprintf(str, "%d %d %d %d ", view->pixstx, view->pixsty,
                    view->pixwd, view->pixht);

            if (strlen(buf) + strlen(str) > MAXSTRLENGTH - 4)
                continue;
            strcat(buf, str);

            nf++;
        }
    } else {
        for (gframe = gfm->getFirstFrame(gfi); gframe != nullFrame; gframe
                = gfm->getNextFrame(gfi)) {

            view = gframe->getFirstView();
            if (view == nullView)
                continue;

            sprintf(str, "%d %d %d %d ", view->pixstx, view->pixsty,
                    view->pixwd, view->pixht);

            if (strlen(buf) + strlen(str) > MAXSTRLENGTH - 4)
                continue;
            strcat(buf, str);

            nf++;
        }
    }

    sprintf(str, "%d ", nf);
    if (strlen(buf) + strlen(str) < MAXSTRLENGTH)
        strcat(str, buf);
    else
        strcpy(str, "0");

    retv[0] = newString(str);
    return proc_complete;
}

/* PUBLIC STATIC VNMRCOMMAND */
int GframeManager::aipGetSelectedKeys(int argc, char *argv[], int retc,
        char *retv[]) {
    if (retc <=0)
        return proc_complete;

    int nf = 0;

    spGframe_t gframe;
    spViewInfo_t view;

    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    for (gframe = gfm->getFirstSelectedFrame(gfi); gframe != nullFrame; gframe
            = gfm->getNextSelectedFrame(gfi)) {

        if (nf < retc) {
            view = gframe->getSelView();
            if (view == nullView)
                continue;

            retv[nf] =newString(view->imgInfo->getDataInfo()->getNameKey().c_str());

            nf++;
        }
    }

    if (nf == 0) {

        gframe = gfm->getFirstFrame(gfi);
        if (gframe != nullFrame) {
            view = gframe->getSelView();
            if (view != nullView) {

                retv[nf] =newString(view->imgInfo->getDataInfo()->getNameKey().c_str());

                nf++;
            }
        }
    }
    return proc_complete;
}

int GframeManager::aipGetFrame(int argc, char *argv[], int retc, char *retv[]) {
    if (retc < 1)
        return proc_complete;

    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    if (argc > 2) {
        int x = atoi(argv[1]);
        int y = atoi(argv[2]);
        gf = gfm->getGframeFromCoords(x, y);
    } else {
        GframeList::iterator gfi;
        gf= gfm->getFirstSelectedFrame(gfi);
    }

    if (gf == nullFrame) {
        retv[0] = realString((double)0);
    } else if (retc > 1) {
        retv[0] = realString((double)gf->row);
        retv[1] = realString((double)gf->col);
    } else {
        int n = (gf->row)*gfm->nCols+ gf->col;
        retv[0] = realString((double)n);
    }
    return proc_complete;
}

int GframeManager::aipGetDataKey(int argc, char *argv[], int retc, char *retv[]) {
    if (retc < 1)
        return proc_complete;

    GframeManager *gfm = GframeManager::get();
    spGframe_t gf = nullFrame;
    string type = "";
    if (argc > 3) {
        int x = atoi(argv[1]);
        int y = atoi(argv[2]);
        type = argv[3];
        gf = gfm->getGframeFromCoords(x, y);
    } else if (argc > 2) {
        int x = atoi(argv[1]);
        int y = atoi(argv[2]);
        gf = gfm->getGframeFromCoords(x, y);
    } else {
        GframeList::iterator gfi;
        gf= gfm->getFirstSelectedFrame(gfi);
    }

    string key = "";
    if (gf != nullFrame) {
        spViewInfo_t view = gf->getSelView();
        if (view != nullView && type == "name")
            key = view->imgInfo->getDataInfo()->getNameKey();
        else if (view != nullView)
            key = view->imgInfo->getDataInfo()->getKey();
    }

    retv[0] = newString(key.c_str());
    return proc_complete;
}

int GframeManager::aipGetFrameToStart(int argc, char *argv[], int retc,
        char *retv[]) {
    int x = -1;
    int y = -1;
    if (argc > 2) {
        x = atoi(argv[1]);
        y = atoi(argv[2]);
    }

    spGframe_t gf = nullFrame;
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    if (x > 0&& y > 0) {
        gf = gfm->getGframeFromCoords(x, y);
        /*
         } else { 
         gf= gfm->getFirstSelectedFrame(gfi);
         */
    }

    if (gf == nullFrame) {
        // get first empty frame.
        for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
                =gfm->getNextFrame(gfi)) {
            if (gf->getFirstView() == nullView) {
                break;
            }
        }
    }

    if (gf == nullFrame) {
        int n = gfm->getNumberOfFrames();
        retv[0] = realString((double)n);
    } else if (retc > 1) {
        retv[0] = realString((double)gf->row);
        retv[1] = realString((double)gf->col);
    } else {
        int n = (gf->row)*gfm->nCols+ gf->col;
        if (gf->getFirstView() != nullView)
            n=-n;
        retv[0] = realString((double)n);
    }
    return proc_complete;
}

bool GframeManager::dupFrame(int srcIdx, int dstIdx) {
    /*fprintf(stderr,"dupFrame(%d, %d)\n", srcIdx, dstIdx); */ /*CMP*/
    spGframe_t srcFrame = nullFrame;
    spGframe_t dstFrame = nullFrame;
    spGframe_t gf;
    int idx;
    GframeList::iterator gfi;
    for (gf=getFirstFrame(gfi), idx = 1; gf != nullFrame && (srcFrame
            == nullFrame || dstFrame == nullFrame); gf=getNextFrame(gfi), idx++) {
        if (idx == srcIdx) {
            srcFrame = gf;
        } else if (idx == dstIdx) {
            dstFrame = gf;
        }
    }
    return dupFrame(srcFrame, dstFrame);
}

bool GframeManager::dupFrame(spGframe_t srcFrame, spGframe_t dstFrame) {
    if (srcFrame == nullFrame || dstFrame == nullFrame) {
        return false;
    }

    // Take down any current display of the frame we're about to show
    if (isFrameDisplayed(srcFrame)) {
        replaceFrame(srcFrame);
    }

    int srcWd = srcFrame->pixwd;
    int srcHt = srcFrame->pixht;
    int dstWd = dstFrame->pixwd;
    int dstHt = dstFrame->pixht;
    int dx = dstFrame->pixstx - srcFrame->pixstx;
    int dy = dstFrame->pixsty - srcFrame->pixsty;
    srcFrame->setDisplayOOD(true);
    srcFrame->row = dstFrame->row;
    srcFrame->col = dstFrame->col;
    srcFrame->pixstx = dstFrame->pixstx;
    srcFrame->pixsty = dstFrame->pixsty;
    srcFrame->pixwd = dstWd;
    srcFrame->pixht = dstHt;
    if (srcWd == dstWd && srcHt == dstHt) {
        srcFrame->moveAllViews(dx, dy); // Move views to new location
    } else {
        // TODO: This does not align multiple views wrt each other
        ViewInfoList *viewList = srcFrame->viewList;
        ViewInfoList::iterator pView;
        int i=0;
        for (pView=viewList->begin(); pView!=viewList->end(); pView++) {
            if(i==0) srcFrame->fitView(*pView);
	    else srcFrame->fitOverlayView(*pView); 
	    i++;
        }

    }
    srcFrame->updateRoiPixels();
    srcFrame->setSelect(false, true); // Make frame unselected

    // transfer spec (if exists) from dstFrame to srcFrames
    if(dstFrame->hasSpec()) {
          std::list<string> specKeys;
          dstFrame->getSpecList()->getSpecKeys(&specKeys);
          if(specKeys.size()>0) {
	     int sliceInd = dstFrame->getSpecList()->getSliceInd();
	     srcFrame->getSpecList()->setSpecKeys(specKeys);
	     srcFrame->getSpecList()->setSliceInd(sliceInd);
          }
    }

    frameKey_t dstKey = getKey(dstFrame);
    gframeList->erase(dstKey);
    gframeList->insert(GframeList::value_type(dstKey, srcFrame));
    srcFrame->draw();

    return true;
}

/**
 * Replace a frame in the gframeList with a new, empty frame.
 */
spGframe_t GframeManager::replaceFrame(spGframe_t gf, bool drawFlag) {
    bool isDisplayed = isFrameDisplayed(gf);
    if (isDisplayed) {
        spGframe_t newFrame = spGframe_t(new Gframe(gf->row,
                gf->col,
                gf->pixstx,
                gf->pixsty,
                gf->pixwd,
                gf->pixht));

	// transfer spec (if displayed) from gf to newFrames
	if(gf->hasSpec()) {
          std::list<string> specKeys;
          gf->getSpecList()->getSpecKeys(&specKeys);
          if(specKeys.size()>0) {
	     int sliceInd = gf->getSpecList()->getSliceInd();
	     newFrame->getSpecList()->setSpecKeys(specKeys);
	     newFrame->getSpecList()->setSliceInd(sliceInd);
	  }
        }

        frameKey_t key = getKey(gf);
        gframeList->erase(key);
        gframeList->insert(GframeList::value_type(key, newFrame));
        if (drawFlag) {
            newFrame->draw();
        }
        return newFrame;
    } else {
        return gf;
    }
}

int GframeManager::aipRedisplay(int argc, char *argv[], int retc, char *retv[]) {
    bool b = WinMovie::get()->movieRunning();
    if (isDebugBit(DEBUGBIT_4)) {
        fprintf(stderr,"aipRedisplay(");
        for (int i=0; i<argc; i++) {
            fprintf(stderr,"\"%s\" ", argv[i]);
        }
        fprintf(stderr,")\n");
    }

    if (argc > 1&& strcmp(argv[argc-1], "redisplay parameters") == 0) {
        // Don't do anything for parameter changes
        return proc_complete;
    }

    if (b) {
        WinMovie::get()->pauseMovie();
    }
    GframeManager::get()->resizeWindow(true);
    if (b) {
        WinMovie::get()->resumeMovie();
    }
    return proc_complete;
}

void GframeManager::resizeWindow(bool drawFlag) {
    // See if window has changed size since last frame arrangement
    if (!gframeList || nCols < 1 || nRows < 1) return;
    if (windowSizeChanged()) {
        GframeManager *gfm = GframeManager::get();
        DataManager *dm = DataManager::get();
        if (getReal("aipFrameResplitOnResize", 0)) {
            int nf = gfm->getNumberOfImages();
            double aspect = dm->getAspectRatio();
            gfm->splitWindow(nf, aspect);
            if (drawFlag) {
                dm->displayFirstN();
            }
        } else {
            gfm->resizeFrames(drawFlag);
        }
    } else if (drawFlag) {
        draw(false);
    }
}

void GframeManager::selectDeselectAllFrames(spGframe_t gframe) {
    if (getSelect(gframe)) {
        // Frame is selected; deselect this (and all other) frames
        selectFrames(none);
    } else {
        // Frame is unselected; select this (and all other) frames
        selectFrames(all);
    }
}

void GframeManager::selectFrames(strArg_t cmd) {
    spGframe_t gf;
    GframeList::iterator gfi;
    switch (cmd) {
    case none: // Deselect all frames
        clearSelectedList();
        drawFrames();
        break;

    case all:
        clearSelectedList();
        for (gf=getFirstFrame(gfi); gf != nullFrame; gf=getNextFrame(gfi)) {
            gf->setSelect(true, true); // Append
        }
        drawFrames();
        break;

    default:
        break;
    }
    //if (getReal("aipVsBind", 0) != 2) {
    if (VsInfo::getVsMode() != 2) {
        VsInfo::setVsHistogram();
    }
}

/* PUBLIC */
void GframeManager::splitWindow(int nrows, int ncols) {
    makeFrames(nrows, ncols);
    draw();
}

/* PUBLIC */
void GframeManager::splitWindow(int nframes, double aspect) {
    int nr, nc;
    getSplit(nframes, aspect, nr, nc);
    // NB: added trap to prevent recreation and clearing of frames when 
    //     redrawing with new data (e.g. obliquePlanesMode) 
    //     - redrawing causes background flicker
/* background flicker resolved
    if (nRows==nr && nCols==nc)
        return;
*/
    makeFrames(nr, nc);
    draw();
}

void GframeManager::resizeFrames(bool drawFlag) {
    if (!gframeList || nCols < 1 || nRows < 1) return;

    if (!getWinsize()) {
        return; // 
    }
    splitWinWd = winWd;
    splitWinHt = winHt;
    int fWd = winWd / nCols;
    int fHt = winHt / nRows;
    if (fWd < minFrameWidth || fHt < minFrameHeight) {
        redrawCanvas(); // copy the previous images back
        return; // Can't do it
    }

    spGframe_t gf;
    int x = 0;
    int y = 0;
    int icol = 0;
    GframeList::iterator gfi;
    for (gf=getFirstFrame(gfi); gf != nullFrame; gf=getNextFrame(gfi)) {
        gf->pixstx = x;
        gf->pixsty = y;
        gf->pixwd = fWd;
        gf->pixht = fHt;
        // TODO: This does not align multiple views wrt each other
        ViewInfoList *viewList = gf->viewList;
        ViewInfoList::iterator pView;
        int i=0;
        for (pView=viewList->begin(); pView!=viewList->end(); pView++) {
            if(i==0) gf->fitView(*pView); // Updates ROIs
	    else gf->fitOverlayView(*pView); 
	    i++;
        }

        if (++icol == nCols) {
            icol = 0;
            x = 0;
            y += fHt;
        } else {
            x += fWd;
        }
    }
    if (drawFlag) {
        draw(false);
    }
}

/* PRIVATE */
void GframeManager::fillFrameList(GframeKeyList_t& list) {
    list.clear();
    GframeList::iterator gfi;
    spGframe_t gf;
    int i = 0;
    for (gf=getFirstFrame(gfi); gf != nullFrame; gf=getNextFrame(gfi), i++) {
        spImgInfo_t img;
        if ((img = gf->getFirstImage()) != nullImg) {
            const string key = img->getDataInfo()->getKey();
            list.push_back(key);
        } else if (!WinMovie::get()->movieStopped()) {
            if ((i+1) == WinMovie::get()->getOwnerFrame()) {
                list.push_back(WinMovie::get()->getOwnerData()->getKey());
            } else if ((i+1) == WinMovie::get()->getGuestFrame()) {
                list.push_back(WinMovie::get()->getGuestData()->getKey());
            }
        } else {
            list.push_back("");
        }
    }
}

/* PRIVATE */
void GframeManager::unToggleFullScreen() {
    if (isFullScreen) {
        // Delete this frame and switch in the normal framelist
        isFullScreen = false; // NB: Do this before "deleteAllFrames()"
        //deleteAllFrames();
        //delete gframeList;
        GraphicsWin::clearWindow();
        makeFrames(saveNRows, saveNCols);
        DataManager *dm = DataManager::get();
        GframeKeyList_t::iterator gfki;
        int iFrame;
        drawFrames();
        for (gfki = saveFrameKeyList.begin(), iFrame = 0; gfki
                != saveFrameKeyList.end(); ++gfki, ++iFrame) {
            const string key = *gfki;
            if (key.length() > 0) {
                dm->displayData(key.c_str(), iFrame);
            }
        }
        //draw();
    }
}

void GframeManager::toggleFullScreen(spGframe_t gf) {
    bool b = WinMovie::get()->movieRunning();
    if (b) {
        WinMovie::get()->pauseMovie();
    }
    if (isFullScreen) {
        // Delete this frame and switch in the normal framelist
        //GframeList::iterator gfi;
        //spGframe_t bigFrame = getFirstFrame(gfi);
        unToggleFullScreen();
        //spGframe_t smallFrame = getFrameByKey(blownUpFrame->row,
        //                                      blownUpFrame->col);
        //fprintf(stderr,"toggleFullScreen: restore row %d, col %d\n",
        //        blownUpFrame->row, blownUpFrame->col);/*CMP*/
        //dupFrame(bigFrame, smallFrame); // Fit the big frame into small space
        //draw();
    } else {
        // Save the framelist and put the specified frame's views in a
        // full screen frame.

        spImgInfo_t img;
        GframeList::iterator gfi;
        if (gf != nullFrame && (img = gf->getFirstImage()) != nullImg) {
            // Remember normal frame parameters
            //blownUpFrame = spGframe_t(new Gframe(gf->row,
            //                                     gf->col,
            //                                     gf->pixstx,
            //                                     gf->pixsty,
            //                                     gf->pixwd,
            //                                     gf->pixht));
            clearSelectedList(); // TODO: May save it for later restoration?
            fillFrameList(saveFrameKeyList);
            saveNCols = nCols;
            saveNRows = nRows;
            //saveGframeList = gframeList;
            //gframeList = new GframeList;
            /* don't call splitWindow, it will draw frames immediately. */
            // splitWindow(1, 1);
            makeFrames(1, 1);
            //spGframe_t bigFrame = getFirstFrame(gfi);
            //bigFrame->loadFrame(gf);
            const string key = img->getDataInfo()->getKey();
            DataManager::get()->displayData(key, 0);
            //draw();
            isFullScreen = true;
        }
    }
    VsInfo::setVsHistogram();
    RoiStat::get()->calculate();
    if (b) {
        WinMovie::get()->resumeMovie();
    }
}

void GframeManager::deleteAllFrames(bool drawFlag) {
    // unToggleFullScreen();
    isFullScreen = false;
    selectedGframeList->clear();
    selectedKeyList.clear();
    gframeList->clear();
    setFrameToLoad(0);
    if (drawFlag) {
        GraphicsWin::clearWindow();
    }
    VsInfo::setVsHistogram();
}

void GframeManager::clearAllFrames() {
    unToggleFullScreen();
    spGframe_t gf;
    GframeList::iterator gfi;
    for (gf=getFirstFrame(gfi); gf != nullFrame; gf=getNextFrame(gfi)) {
        replaceFrame(gf);
    }
    VsInfo::setVsHistogram();
    setFrameToLoad(0);
}

void GframeManager::clearSelectedFrames() {
    unToggleFullScreen();
    GframeList::iterator itr;
    for (itr = selectedGframeList->begin(); itr != selectedGframeList->end(); itr++) {
        replaceFrame(itr->second);
    }
    VsInfo::setVsHistogram();

}

bool GframeManager::setFrameToLoad(int frameNumber) {
    GframeList::iterator p;

    if (!gframeList || gframeList->size() == 0) {
        loadKey = nullKey;
        return false;
    }
    if (frameNumber < 0) {
        frameNumber = 0;
    }
    int i;
    for (i=0, p = gframeList->begin(); p != gframeList->end(); p++, i++) {
        if (i == frameNumber) {
            loadKey = p->first;
            return true;
        }
    }
    loadKey = nullKey;
    return false;
}

/* PUBLIC */
bool GframeManager::setFrameToLoad(spGframe_t gf) {
    GframeList::iterator p;

    if (!gframeList || gframeList->size() == 0) {
        loadKey = nullKey;
        return false;
    }
    if (gf == nullFrame) {
        // Null arg ==> load to first frame
        loadKey = gframeList->begin()->first;
        return true;
    } else {
        loadKey = getKey(gf);
        return true;
    }
    loadKey = nullKey;
    return false;
}

bool GframeManager::windowSizeChanged() {
    getWinsize();
    return winWd != splitWinWd || winHt != splitWinHt;
}

int GframeManager::getNumberOfFrames() {
    return gframeList->size();
}

int GframeManager::getNumberOfImages() {
    GframeList::iterator itr;
    int n = 0;
    for (itr = gframeList->begin(); itr != gframeList->end(); ++itr) {
        if (itr->second->getFirstImage() != nullImg) {
            ++n;
        }
    }
    return n;
}

// frame # starts from 0
int GframeManager::getFirstAvailableFrame() {
    GframeList::iterator itr;
    int n = 0;
    for (itr = gframeList->begin(); itr != gframeList->end(); ++itr) {
        if (itr->second->getFirstImage() != nullImg) {
            ++n;
        } else return n;
    }
    return n;
}

spGframe_t GframeManager::getFirstFrame(GframeList::iterator& frameItr) {
    if (gframeList && gframeList->size() > 0) {
        frameItr = gframeList->begin();
        if (frameItr != gframeList->end()) {
            return frameItr->second;
        }
    }
    return (spGframe_t)NULL;
}

spGframe_t GframeManager::getNextFrame(GframeList::iterator& frameItr) {
    if (++frameItr == gframeList->end()) {
        return (spGframe_t)NULL;
    }
    return frameItr->second;
}

/*
 * For this, frame numbers start at one (1).
 */
spGframe_t GframeManager::getFrameByNumber(int n) {
    spGframe_t gptr;
    int i;

    GframeList::iterator gfi;
    for (i=1, gptr= getFirstFrame(gfi); i<n && gptr != nullFrame; i++, gptr
            = getNextFrame(gfi)) {
    }
    if (i != n) {
        return nullFrame;
    } else {
        return gptr; // (Could be nullFrame)
    }
}

spGframe_t GframeManager::getFrameByID(int id) {
    spGframe_t gptr;

    GframeList::iterator gfi;
    for (gptr= getFirstFrame(gfi); gptr != nullFrame; gptr = getNextFrame(gfi)) {
       if (gptr->id == id) return gptr;
    }
    return nullFrame;
}

void GframeManager::listBadFrames() {
    GframeList::iterator frameItr;

    if (gframeList && gframeList->size() > 0) {

        for (frameItr = gframeList->begin(); frameItr != gframeList->end(); ++frameItr) {
            frameKey_t key = frameItr->first;
            spGframe_t gf = frameItr->second;
            if (key.first != gf->row|| key.second != gf->col) {
                fprintf(stderr,"****Bad frame: key=(%d,%d), row=%d, col=%d\n",
                key.first, key.second, gf->row, gf->col);
            }
        }
    }
}

void GframeManager::listAllFrames() {
    spGframe_t gf;

    fprintf(stderr,"%d frames:\n", getNumberOfFrames());
    GframeList::iterator gfi;
    for (gf= getFirstFrame(gfi); gf != nullFrame; gf= getNextFrame(gfi)) {
        fprintf(stderr,"   (col %d, row %d) at (%d, %d)\n",
        gf->col, gf->row, gf->pixstx, gf->pixsty);
    }
}

void GframeManager::listFrameToLoad(GframeList::iterator& next) {
    spGframe_t gf;

    if (loadKey == nullKey) {
        fprintf(stderr,"loadKey=null\n");
    } else {
        fprintf(stderr,"loadKey=(%d,%d)\n", loadKey.first, loadKey.second);
    }
    frameKey_t nextKey = next->first;
    fprintf(stderr,"   nextKey=(%d,%d)\n", nextKey.first, nextKey.second);
}

bool GframeManager::isFrameDisplayed(spGframe_t gframe) {
    if (gframe == nullFrame) {
        return false;
    }
    GframeList::iterator p;
    if (gframeList && gframeList->size() > 0) {
        frameKey_t key = getKey(gframe);
        p = gframeList->find(key);
        if (p != gframeList->end() && p->second == gframe) {
            return true;
        }
    }
    return false;
}

/*
 frameKey_t
 GframeManager::isFrameDisplayed(spGframe_t gframe)
 {
 GframeList::iterator frameItr;
 if (gframeList && gframeList->size() > 0) {
 for (frameItr = gframeList->begin();
 frameItr != gframeList->end();
 ++frameItr)
 {
 if (gframe == frameItr->second) {
 return frameItr->first;
 }
 }
 }
 return nullKey;
 }
 */

void GframeManager::clearSelectedList() {
    GframeList::iterator itr;
    for (itr = selectedGframeList->begin(); itr != selectedGframeList->end(); itr++) {
        // Tell it that it's no longer selected
        itr->second->setSelected(false);
    }
    selectedGframeList->clear();
    selectedKeyList.clear();
}

void GframeManager::setSelect(Gframe *gf, bool selectFlag) {
    gf->selected = selectFlag;
    GframeList::iterator itr;
    for (itr = gframeList->begin(); itr != gframeList->end(); itr++) {
        if (gf == itr->second.get()) {
            if (selectFlag) {
                // Add entry to "selected" list with same key and value
                //selectedGframeList->insert(GframeList::value_type(itr->first,
                //        itr->second));
		// 04-10-12 2012: change key to pair(ns,ns) so the list is sorted 
		// according to the order frame is selected.
		// (original key was pair(row,col), and was never used)
		int ns = selectedGframeList->size();
                selectedGframeList->insert(GframeList::value_type(make_pair(ns,ns),
                        itr->second));
                spImgInfo_t img = itr->second->getFirstImage();
                if (img != nullImg) {
                    selectedKeyList.push_back(img->getDataInfo()->getKey());
                }
            } else {
                // Delete entry with key from the "selected" list
                selectedGframeList->erase(itr->first);
                spImgInfo_t img = itr->second->getFirstImage();
                if (img != nullImg) {
                    selectedKeyList.remove(img->getDataInfo()->getKey());
                }
            }
            break;
        }
    }
}

bool GframeManager::getSelect(spGframe_t gf) {
    GframeList::iterator itr;
    for (itr = selectedGframeList->begin(); itr != selectedGframeList->end(); itr++) {
        if (gf == itr->second) {
            return true;
        }
    }
    return false;
}

set<string> GframeManager::getSelectedKeys() {
    set<string> keys;
    GframeList::iterator itr;
    for (itr = selectedGframeList->begin(); itr != selectedGframeList->end(); ++itr) {
        spGframe_t gf = itr->second;
        spImgInfo_t img = gf->getFirstImage();
        if (img != nullImg) {
            keys.insert(img->getDataInfo()->getKey());
        }
    }
    return keys;
}

/* PUBLIC */
spGframe_t GframeManager::getFirstSelectedFrame(GframeList::iterator& frameItr) {
    if (selectedGframeList && selectedGframeList->size() > 0) {
        frameItr = selectedGframeList->begin();
        if (frameItr != selectedGframeList->end()) {
            return frameItr->second;
        }
    }
    return nullFrame;
}

/* PUBLIC */
spGframe_t GframeManager::getNextSelectedFrame(GframeList::iterator& frameItr) {
    if (++frameItr == selectedGframeList->end()) {
        return nullFrame;
    }
    return frameItr->second;
}

void /* PUBLIC */
GframeManager::updateViews() {
    if (!gframeList || gframeList->size() == 0) {
        return;
    }
    GframeList::iterator p;
    for (p = gframeList->begin(); p != gframeList->end(); p++) {
        p->second->updateViews();
    }
}

//void
//GframeManager::clearScreen()
//{
//    GraphicsWin::clearWindow();
//}

void GframeManager::draw(bool resize) {
    if (resize) {
        resizeWindow(true);
    } else {
        grabMouse();

        GraphicsWin::clearWindow();
        GframeList::iterator p;
        for (p = gframeList->begin(); p != gframeList->end(); p++) {
            p->second->draw(); // Draw each frame and contents
        }
    }
}

void GframeManager::drawFrames() {
    GframeList::iterator p;
    for (p = gframeList->begin(); p != gframeList->end(); p++) {
        p->second->drawFrame(); // Draw each frame
    }
}

/* PUBLIC */
bool GframeManager::setFrameToLoad(GframeList::iterator next) {
    loadKey = next->first;
    return true;
}

spGframe_t GframeManager::getPtr(Gframe *gf) {
    GframeList::iterator gfi;
    for (gfi=gframeList->begin(); gfi != gframeList->end(); gfi++) {
        if (gf == gfi->second.get()) {
            return gfi->second;
        }
    }
    return nullFrame;
}

/* PUBLIC */
spGframe_t GframeManager::getFrameToLoad() {
    GframeList::iterator next;
    return getFrameToLoad(next); // Lose the "next" value.
}

/* PUBLIC */
spGframe_t GframeManager::getFrameToLoad(GframeList::iterator& next) {
    GframeList::iterator p;
    GframeList::iterator end;

    if (!gframeList || gframeList->size() == 0) {
        return nullFrame;
    }
    end = gframeList->end();
    if (loadKey == nullKey || (p = gframeList->find(loadKey)) == end) {
        // TODO maybe look for 1st empty frame
        p = gframeList->begin();
        loadKey = p->first;
    }
    next = p;
    next++;
    if (next == end) {
        next = gframeList->begin();
    }
    return p->second;
}

/* PUBLIC */
bool GframeManager::loadData(spDataInfo_t data, spGframe_t dstFrame) {
    /*fprintf(stderr,"loadData(): nframes=%d\n",
     GframeManager::get()->getNumberOfFrames()); */ /*CMP*/
    bool status = false;

    spGframe_t gf;
    if (dstFrame == nullFrame) {
        gf = getFrameToLoad();
    } else {
        gf = dstFrame;
    }
    if (gf == nullFrame) {
        return false;
    }
    setFrameToLoad(gf);

    const string key = data->getKey();
    
    spGframe_t cachedFrame = getCachedFrame(key);
    //if (cachedFrame == nullFrame || isFrameDisplayed(cachedFrame)) {
    if (cachedFrame == nullFrame) {
        status = loadImage(data);
    } else {
        status = loadFrame(cachedFrame);
        //load roi from dataInfo to cachedFrame. 
        data->loadRoi(cachedFrame);
    }

    /*fprintf(stderr,"loadData done: nframes=%d\n",
     GframeManager::get()->getNumberOfFrames()); */ /*CMP*/
    return status;
}

/* PUBLIC */
bool GframeManager::loadImage(spDataInfo_t data) {
    spImgInfo_t imgInfo = spImgInfo_t(new ImgInfo(data));

    /*fprintf(stderr,"loadImage(): nframes=%d\n",
     GframeManager::get()->getNumberOfFrames()); */ /*CMP*/
    if (WinMovie::get()->movieStopped() && Movie::get()->movieStopped())
        unToggleFullScreen();
    if (imgInfo == nullImg) {
        return false;
    }
    int vsMode = VsInfo::getVsMode();
    bool drawFirst = vsMode != VS_GROUP && vsMode != VS_OPERATE;
    GframeList::iterator next;
    spGframe_t gf = getFrameToLoad(next);
    if (gf == nullFrame) {
        return false;
    }
    gf = replaceFrame(gf, false);

    gf->loadView(imgInfo); // Displays image if VS has been set

    //load roi from dataInfo to gf
    data->loadRoi(gf);

    saveFrame(gf);
    setFrameToLoad(next);

    // update image info field   
    if(gf->getGframeNumber() == 1) gf->sendImageInfo();

    if (!drawFirst) {
        /*
         * For these modes, data has not been displayed yet.
         * Need to calculate VS and redisplay a group of images.
         */
        DataManager *dm = DataManager::get();
	string mykey = imgInfo->getDataInfo()->getKey();
        if (vsMode == VS_GROUP) {
	    string grp = mykey.substr(0,mykey.find_first_of(" "));
            set<string> keys = dm->getKeys(DATA_GROUP, grp);
            if (keys.count(mykey)) {
                VsInfo::autoVsGroup(keys);
            } else {
                VsInfo::autoVs(mykey);
            }
        } else if (vsMode == VS_OPERATE) {
            set<string> opkeys = dm->getKeys(DATA_OPERATE);
            if (opkeys.count(mykey)) {
                VsInfo::autoVsGroup(opkeys);
            } else {
                VsInfo::autoVs(mykey);
            }
        }
    }

    /*fprintf(stderr,"..*loadImage done: nframes=%d\n",
     GframeManager::get()->getNumberOfFrames()); */ /*CMP*/
    return true;
}

/* PUBLIC */
bool GframeManager::loadFrame(spGframe_t gframe) {
    /*fprintf(stderr,"loadFrame(): nframes=%d\n",
     GframeManager::get()->getNumberOfFrames()); */ /*CMP*/
    //unToggleFullScreen();
    if (gframe == nullFrame) {
        return false;
    }
    GframeList::iterator next;
    spGframe_t gf = getFrameToLoad(next);
    if (gf == nullFrame) {
        return false;
    }
    dupFrame(gframe, gf);
    setFrameToLoad(next);
    /*fprintf(stderr,"..**loadFrame done: nframes=%d\n",
     GframeManager::get()->getNumberOfFrames()); */ /*CMP*/
    return true;
}

/* PUBLIC */
/*
 void
 GframeManager::quickVs(int x, int y, bool maxmin)
 {
 spGframe_t gf = getGframeFromCoords(x, y);
 gf->quickVs(x, y, maxmin);
 }
 */

/* PUBLIC */
spGframe_t GframeManager::getGframeFromCoords(int x, int y) {
    /* NB: The Gframe manager may know enough to do this a faster way.
     * E.g., use x,y to calculate row,column position.
     */
    GframeList::iterator gfi;
    for (gfi=gframeList->begin(); gfi != gframeList->end(); gfi++) {
        spGframe_t gf = gfi->second;
        if (gf->pixstx <= x && x < gf->pixstx + gf->pixwd&&gf->pixsty <= y && y
                < gf->pixsty + gf->pixht) {
            return gf;
        }
    }
    return (spGframe_t)NULL;
}

void GframeManager::setMovieFrame(int w, int h) {
    winWd = w;
    winHt = h;
    GraphicsWin::clearWindow();
    makeFrames(1, 1);
}

void GframeManager::restoreFrames(int rows, int cols) {
    getWinsize();
    makeFrames(rows, cols);
}

/* PRIVATE */
bool GframeManager::getWinsize() {
    winWd = getWinWidth();
    winHt = getWinHeight();
    if (winWd < 0) {
        return false;
    } else {
        return true;
    }
}

/* PRIVATE */
void GframeManager::makeFrames(int nrows, int ncols) {
    if (nrows <= 0|| ncols <= 0 || winWd<0 || winHt<0) {
        return; // Don't even try
    }
    setFrameToLoad(0);

    int fWd = winWd / ncols;
    int fHt = winHt / nrows;
    if (fWd < minFrameWidth) {
        fWd = minFrameWidth;
        ncols = winWd / fWd;
    }
    if (fHt < minFrameHeight) {
        fHt = minFrameHeight;
        nrows = winHt / fHt;
    }

    nRows = nrows; // Set class vars
    nCols = ncols;

    if (!gframeList) {
        gframeList = new GframeList;
    }

    deleteAllFrames(false);
    frameKey_t key;
    spGframe_t entry;
    int x;
    int y = 0;
    for (int i=0; i<nrows; i++, y += fHt) {
        x = 0;
        for (int j=0; j<ncols; j++, x += fWd) {
            entry = spGframe_t(new Gframe(i, j, x, y, fWd, fHt));
            key = getKey(entry);
            gframeList->insert(GframeList::value_type(key, entry));
        }
    }
    setReal("aipWindowSplit", 1, nrows, false);
    setReal("aipWindowSplit", 2, ncols, true);
    char str[MAXSTR];
    sprintf(str, "%dx%d", nrows, ncols);
    setString("framelayoutName", str, true);
    OrthoSlices::get()->updateDisplayMode();
}

/*
 * Given number of frames and aspect ratio desired, returns number of
 * rows and columns to split the window into.  Note that "nf" is
 * treated as the minimum number of gframes needed.  The "aspect" is
 * the width/height ratio desired for each gframe.
 */
/* PRIVATE */
void GframeManager::getSplit(int nf, double aspect, int &nr, int &nc) {
    splitWinWd = winWd;
    splitWinHt = winHt;
    int maxnc = winWd / minFrameWidth;
    int maxnr = winHt / minFrameHeight;
    if (maxnc * maxnr <= nf) {
        // Just make as many frames as possible
        nc = maxnc;
        nr = maxnr;
    } else {
        // Calculate an approximate split
        double dnc = sqrt((winWd * nf) / (aspect * winHt));
        nc = (int)dnc;
        if (nc < 1) {
            nc = 1;
        }
        if (nc > maxnc) {
            nc = maxnc;
        }
        nr = (nf + nc - 1) / nc;
        if (nr < 1) {
            nr = 1;
        }
        if (nr > maxnr) {
            nr = maxnr;
        }

        // That tends to make the aspect ratio too large
        // Try this too:
        int imWd, imHt;
        Gframe::getFitInFrame(aspect, 1.0, winWd / nc, winHt / nr, 0, imWd,
                imHt);
        while (nr > 1) {
            int nr2 = nr - 1;
            int nc2 = (nf + nr2 - 1) / nr2;
            if (nc2 > maxnc) {
                break;
            } else {
                int imWd2, imHt2;
                Gframe::getFitInFrame(aspect, 1.0, winWd / nc2, winHt / nr2, 0,
                        imWd2, imHt2);
                if (imWd2 < imWd) {
                    break;
                } else {
                    nr = nr2;
                    nc = nc2;
                    imWd = imWd2;
                    imHt = imHt2;
                }
            }
        }

        // Now see if we can add columns w/o making images smaller.
        // Needed when window is short and wide.
        while (nc < maxnc) {
            int nc2 = nc + 1;
            int imWd2, imHt2;
            Gframe::getFitInFrame(aspect, 1.0, winWd / nc2, winHt / nr, 0,
                    imWd2, imHt2);
            if (imWd2 < imWd) {
                break;
            } else {
                nc = nc2;
                imWd = imWd2;
                imHt = imHt2;
            }
        }
    }
}

spGframe_t GframeManager::getFirstCachedFrame(GframeCache_t::iterator& frameItr) {
    if (gframeCache && gframeCache->size() > 0) {
        frameItr = gframeCache->begin();
        if (frameItr != gframeCache->end()) {
            return frameItr->second;
        }
    }
    return (spGframe_t)NULL;
}

spGframe_t GframeManager::getNextCachedFrame(GframeCache_t::iterator& frameItr) {
    if (++frameItr == gframeCache->end()) {
        return (spGframe_t)NULL;
    }
    return frameItr->second;
}

int GframeManager::aipIsIplanObj(int argc, char *argv[], int retc, char *retv[]) {
    if (argc < 3|| retc < 1)
        return proc_complete;

    int x = atoi(argv[1]);
    int y = atoi(argv[2]);
    spGframe_t gf = GframeManager::get()->getGframeFromCoords(x, y);

    if (gf != nullFrame) {
        int b = isIplanObj(x, y, gf->id);
        retv[0] = realString((double)b);
    } else {
        retv[0] = realString((double)0);
    }

    return proc_complete;
}

spGframe_t GframeManager::getOwnerFrame(string key) {
    GframeManager *gfm = GframeManager::get();
    spGframe_t gframe;
    spImgInfo_t img;

    GframeList::iterator gfi;
    for (gframe = gfm->getFirstFrame(gfi); gframe != nullFrame; gframe
            = gfm->getNextFrame(gfi)) {
        img = gframe->getFirstImage();
        if (img != nullImg && img->getDataInfo()->getKey() == key) {
            return gframe;
        }
    }
    return nullFrame;
}

int GframeManager::getFrameToStart(int x, int y) {
    spGframe_t gf = nullFrame;
    GframeList::iterator gfi;

    if (x > 0&& y > 0) {
        gf = getGframeFromCoords(x, y);
        /*
         } else { 
         gf= getFirstSelectedFrame(gfi);
         */
    }

    if (gf != nullFrame && gf->getFirstView() != nullView)
        gf = nullFrame;

    if (gf == nullFrame) {
        // get first empty frame.
        for (gf=getFirstFrame(gfi); gf != nullFrame; gf=getNextFrame(gfi)) {
            if (gf->getFirstView() == nullView) {
                break;
            }
        }
    }

    if (gf == nullFrame) {
        return -1;
    } else {
        return (gf->row)*nCols + gf->col;
    }
}

// delete existing colormaps in given directory
void GframeManager::deleteCmaps(char *path) {
   char *ptr;
   char file[MAXSTR+16];
   struct dirent **namelist;
   struct stat fstat;

   if (stat(path, &fstat) != 0) return;
   if (!S_ISDIR(fstat.st_mode)) return;

   int n = scandir(path, &namelist, 0, alphasort);
   if (n < 0) return;
       
   // Loop through the file list looking for .cmap files.
   for(int i=0; i<n; i++) {
     ptr = namelist[i]->d_name;
     if(strstr(ptr+strlen(ptr)-5,".cmap") != NULL) {
        sprintf(file, "%s/%s", path, namelist[i]->d_name);
	unlink(file);
     }
     free(namelist[i]);
   }
   free(namelist);
}

void GframeManager::setActiveGframe(spGframe_t gf) {
  activeGframe  = gf;

  if(gf == nullFrame) return;
  spImgInfo_t img = gf->getSelImage();
  if(img == nullImg) return;
  setString("aipCurrentKey", img->getDataInfo()->getKey().c_str(), true);
}

void GframeManager::shiftSelectFrames(spGframe_t gf) {

    if (!gframeList || gframeList->size() == 0) return;

    clearSelectedList();

    frameKey_t last = getKey(gf);

    bool sel = false;
    GframeList::iterator p;
    for (p = gframeList->begin(); p != gframeList->end(); p++) {
        if(p->first == loadKey) sel = true;
        if(sel) p->second->setSelect(true, true);
        if(p->first == last) sel = false;
    }
}

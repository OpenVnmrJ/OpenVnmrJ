/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPGFRAMEMANAGER_H
#define AIPGFRAMEMANAGER_H

#include <map>
#include <utility>
#include <set>
using std::set;

#include "aipGframe.h"
#include "aipImgInfo.h"

//class ImgInfo;
//class Gframe;

typedef std::pair<int, int> frameKey_t;	// Access by row,column position
typedef std::map<frameKey_t, spGframe_t> GframeList;
typedef std::map<string, spGframe_t> GframeCache_t;
typedef std::list<string> GframeKeyList_t;

class GframeManager
{
public:
    static frameKey_t nullKey;
    static bool overlaid;

    typedef enum {
	all,
	none
    } strArg_t;

    /* Vnmr commands */
    static int aipSplitWindow(int argc, char *argv[], int retc, char *retv[]);
    static int aipFullScreen(int argc, char *argv[], int retc, char *retv[]);
    static int aipClearFrames(int argc, char *argv[], int retc, char *retv[]);
    static int aipDeleteFrames(int argc, char *argv[], int retc, char *retv[]);
    static int aipSelectFrames(int argc, char *argv[], int retc, char *retv[]);
    static int aipPrintImage(int argc, char *argv[], int retc, char *retv[]);
    static int aipRedisplay(int argc, char *argv[], int retc, char *retv[]);
    static int aipDupFrame(int argc, char *argv[], int retc, char *retv[]);
    static int aipGetPrintFrames(int argc, char *argv[], int retc, char *retv[]);
    static int aipGetFrame(int argc, char *argv[], int retc, char *retv[]);
    static int aipGetDataKey(int argc, char *argv[], int retc, char *retv[]);
    static int aipGetFrameToStart(int argc, char *argv[], int retc, char *retv[]);
    static int aipGetSelectedKeys(int argc, char *argv[], int retc, char *retv[]);
    static int aipIsIplanObj(int argc, char *argv[], int retc, char *retv[]);
    static int aipSetColormap(int argc, char *argv[], int retc, char *retv[]);
    static int aipSetTransparency(int argc, char *argv[], int retc, char *retv[]);
    static int aipSaveColormap(int argc, char *argv[], int retc, char *retv[]);
    static int aipViewLayers(int argc, char *argv[], int retc, char *retv[]);
    static int aipOverlayFrames(int argc, char *argv[], int retc, char *retv[]);
    static int aipMoveOverlay(int argc, char *argv[], int retc, char *retv[]);
    static int aipOverlayGroup(int argc, char *argv[], int retc, char *retv[]);

    bool dupFrame(int src, int dst);
    bool dupFrame(spGframe_t srcFrame, spGframe_t dstFrame);
    spGframe_t replaceFrame(spGframe_t gf, bool drawFlag=true);
    void splitWindow(int nframes, double aspect);
    void splitWindow(int nrows, int ncols);
    void resizeWindow(bool drawFlag);
    void resizeFrames(bool drawFlag = true);
    void toggleFullScreen(spGframe_t);
    void clearAllFrames();
    void clearSelectedFrames();
    void deleteAllFrames(bool drawFlag=true);
    void selectDeselectAllFrames(spGframe_t gframe);
    void selectFrames(strArg_t);

    static GframeManager *get(); // Returns instance; may call constructor.

    frameKey_t getKey(spGframe_t); // Get the key for the gframe
    spGframe_t getPtr(Gframe *); // Get shared ptr that points to given frame
    spGframe_t getFrameToLoad(); // Returns the gframe that gets the next image
    spGframe_t getFrameToLoad(GframeList::iterator& next); // ... and rtns next
    bool setFrameToLoad(int frameNumber); // Sets who to load next
    bool setFrameToLoad(spGframe_t); // Sets who to load next
    bool setFrameToLoad(GframeList::iterator); // Sets who to load next
    bool loadData(spDataInfo_t, spGframe_t = nullFrame );
    bool loadImage(spDataInfo_t);
    bool loadFrame(spGframe_t);
    bool saveFrame(spGframe_t);
    spGframe_t unsaveFrame(const string key);
    spGframe_t getCachedFrame(const string key);
    void clearFrameCache();
    //void quickVs(int x, int y, bool maxmin);
    bool windowSizeChanged();
    int getNumberOfFrames();
    int getNumberOfImages();
    spGframe_t getGframeFromCoords(int x, int y);
    spGframe_t getFirstFrame(GframeList::iterator& frameItr);
    spGframe_t getNextFrame(GframeList::iterator& frameItr);
    spGframe_t getFrameByNumber(int n);
    spGframe_t getFrameByID(int id);
    spGframe_t getFrameByKey(int row, int col);
    spGframe_t getOwnerFrame(string key);
    void listBadFrames();
    void listAllFrames();
    void listFrameToLoad(GframeList::iterator& next);
    bool isFrameDisplayed(spGframe_t gframe);
    set<string> getSelectedKeys();
    list<string> getSelectedKeylist() {return selectedKeyList;}
    spGframe_t getFirstSelectedFrame(GframeList::iterator& frameItr);
    spGframe_t getNextSelectedFrame(GframeList::iterator& frameItr);
    void clearSelectedList();
    void setSelect(Gframe *, bool selectFlag);
    bool getSelect(spGframe_t);
    void updateViews();
    void draw(bool resize = true);
    void drawFrames();
    spGframe_t getFirstCachedFrame(GframeCache_t::iterator& frameItr);
    spGframe_t getNextCachedFrame(GframeCache_t::iterator& frameItr);
    int getFrameToStart(int x, int y);
    void setMovieFrame(int w, int h);
    void restoreFrames(int rows, int cols);
    int getFirstAvailableFrame();
    int getNumSelectedFrames() {return selectedGframeList->size();}
    void setActiveGframe(spGframe_t gf);
    spGframe_t getActiveGframe() {return activeGframe;}
    void shiftSelectFrames(spGframe_t gf);

private:
    static GframeManager *gframeManager; // Support one instance
    static bool isFullScreen;

    GframeList *gframeList;
    GframeList *selectedGframeList;
    GframeCache_t *gframeCache;
    GframeKeyList_t saveFrameKeyList;
    GframeKeyList_t selectedKeyList;

    spGframe_t activeGframe;

    frameKey_t loadKey;
    int winWd;
    int winHt;
    int splitWinWd;             // Window size at last split
    int splitWinHt;
    int nRows;
    int nCols;
    int saveNRows;
    int saveNCols;

    GframeManager();		// Private constructor only
    void getSplit(int nf, double aspect, int &nr, int &nc);
    bool getWinsize();
    void makeFrames(int nrows, int ncols);
    void unToggleFullScreen();
    void fillFrameList(GframeKeyList_t& list);
    void deleteCmaps(char *path);
};

#endif /* AIPGFRAMEMANAGER_H */

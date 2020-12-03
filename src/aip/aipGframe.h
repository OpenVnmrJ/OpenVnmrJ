/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPGFRAME_H
#define AIPGFRAME_H


#include <list>
//using namespace std;

#include "sharedPtr.h"
#include "aipViewInfo.h"
#include "aipImgInfo.h"
#include "aipVsInfo.h"
#include "aipDataInfo.h"
#include "aipRoi.h"
#include "aipSpecViewList.h"

namespace aip {
    const int minFrameWidth = 10;
    const int minFrameHeight = 10;
}

class GframeLocation
{
public:
    string dataKey;
    int row, col;		// Position of frame in matrix
    int pixstx, pixsty;		// Upper-left corner of frame on window
    int pixwd, pixht;		// Size of frame in pixels

    GframeLocation(string key, int row, int col, int x, int y, int wd, int ht);
};


class Gframe: public GframeLocation
{
public:
    int id;			// Unique ID for this gframe
    double pixelsPerCm;		// Scale of image
    bool selected;		// Whether frame is currently selected
    bool imgBackupOod;
    double p2m[4][4];		// Pixel address to magnet coord conversion
    double m2p[4][4];		// Magnet coord to pixel address conversion

    // add the following for empty frame zooming (nullView).
    double zoomFactor;
    int zoomSpecID;

    // List of views to display:
    ViewInfoList *viewList;

    // List of ROIs and On-Image Annotation
    RoiList *roiList;

    Gframe(int row, int col, int x, int y, int wd, int ht);
    ~Gframe();

    void clearFrame();		// Deletes all frame contents
    int minX() { return pixstx + pixstxOffset; }
    int minY() { return pixsty + pixstyOffset; }
    int maxX() { return pixstx + pixwd - pixstxOffset - 1; }
    int maxY() { return pixsty + pixht - pixstyOffset - 1; }
    void loadView(spImgInfo_t img, bool display = true);
    void fitView(spViewInfo_t view);
    void loadFrame(spGframe_t gf, bool samePixmap = false);
    void updateViews();
    void moveAllViews(int deltax, int deltay);
    int getViewCount();
    void setSelect(bool selectFlag, bool appendFlag);
    void setSelected(bool);
    bool isSelected() { return selected;}
    spGframe_t highlight(int x, int y, spGframe_t prevGframe);
    void setHighlight(bool);
    double distanceFrom(int x, int y);
    void drawFrame();
    void drawFOV();
    void drawFrameLabel();
    void draw();
    void print(double scale);
    bool updateScaleFactors();
    void pixToMagnet(double px, double py, double pz,
		     double &mx, double &my, double &mz);
    void setClipRegion(ClipStyle style);
    spViewInfo_t getFirstView();
    spViewInfo_t getSelView();
    spViewInfo_t getNextView();
    spViewInfo_t getViewByIndex(int ind);
    ViewInfoList *copyViewList(bool sameBackingStore = false);
    spImgInfo_t getFirstImage();
    spImgInfo_t getSelImage();
    int getGframeNumber();
    string getGframeName();
    string getViewName(int ind);
    string getViewNames();
    string getViewKeys();
    int getDataNumber();
    void sendImageInfo();

    void vscaleOtherFrames(double min, double max, spVsInfo_t vsi);
    void vscaleOtherImages(int mode = -1);
    void startInteractiveVs(int x, int y);
    void doInteractiveVs(int x, int y);
    void finishInteractiveVs();
    void quickVs(int x, int y, bool maxmin);
    void autoVs();
    spVsInfo_t getVs();
    void setVs(spVsInfo_t);
    void resetZoom(bool master=true);
    void quickZoom(int x, int y, double factor);
    void setBase(int x, int y);
    void setPan(int x, int y, bool movingFlag);
    void segmentSelectedRois(bool minFlag, double minData,
			     bool maxFlag, double maxData, bool clear);
    void segmentImage(bool minFlag, double minData,
		      bool maxFlag, double maxData);
    Roi *getFirstRoi();
    Roi *getNextRoi();
    Roi *getFirstUnselectedRoi();
    RoiList *copyRoiList();
    bool containsRoi(Roi *);
    void addRoi(Roi *);
    bool deleteRoi(Roi *);
    bool hasASelectedRoi();
    void updateRoiPixels();
    bool setDisplayOOD(bool);
    void setViewDataOOD(bool);
    void saveRoi();
    bool saveCanvasBackup(Roi *roi);
    bool drawCanvasBackup(int x1, int y1, int x2, int y2);
    bool imgBackupOOD();
    void flagUpdateAllRoi();
    void rot90DataCoordsAllRoi(int datawidth);
    void flipDataCoordsAllRoi(int datawidth);
    static double getFitInFrame(double spaceWd, double spaceHt,
				int frameWd, int FrameHt,
				int rotation,
				int& imgWd, int& imgHt);
    void setZoom(double xCenterData, double yCenterData, double newPixPerCm);
    SpecViewList *getSpecList() {return specList;}
    void setSpecKeys(std::list<string> keys);
    void disSpec(int fx, int fy, int fw, int fh, bool update);
    void selectSpecView(int x, int y, int mask);
    void deleteSpecList();
    bool hasSpec();
    void resetSpecFrame();
    void zoomSpecFrame(int x,int y,double factor);
    void setColormap(const char *name);
    void setTransparency(int value);
    void removeOverlayImg();
    void loadOverlayImg(const char *path, const char *cmapName);
    void loadOverlayImg(const char *path, const char *cmapName, int colormapId);
    void selectViewLayer(int imgId, int order, int mapId);
    bool parallelPlane(spViewInfo_t view, spViewInfo_t baseView);
    bool coPlane(spViewInfo_t view, spViewInfo_t baseView);
    int calcZoom(spViewInfo_t view, double xDataCenter, double yDataCenter, double z, double newPixPerCm, spViewInfo_t baseView=nullView);
    void fitOverlayView(spViewInfo_t view);
    void setOverlayPan(int x, int y, bool movingFlag);
    int resetOverlayPan();
    void setPositionInfo(int x, int y);
    int getPositionInfo(spViewInfo_t view, int x, int y, int &dx, int &dy, double &mag);
    void checkOffset(spViewInfo_t baseView, double *offset);

private:
    static int nextId;
    static int pixstxOffset;
    static int pixstyOffset;
    static int normalColor;
    static int highlightColor;
    static int selectColor;
    static int bkgColor;
    static int FOVColor;
    static int markLen;		// Length of frame corner markers
    static double pixelsPerCm_fit; // Scale of image when fit to frame

    // Used for interactive VS:
    static int vsXRef;
    static int vsYRef;
    static double vsCenterRef;
    static double vsRangeRef;
    static int vsDynamicBinding;
    static int  levelOfDraw;
    static bool inSaveState;
    static bool inDrawState;

    int color;
    bool highlighted;

    double basex;		// Used for panning
    double basey;
    double overlayOff[3];  // used for panning overlay.

    //int bkgstx;			// Right edge of image + 1
    //int bkgsty;			// Bottom edge of image + 1
    BackStore_t imgBackup;	// Holds pixmap of (superposition of) images

    bool displayOod;		// Display is Out Of Date
    void clearViewList();
    void clearRoiList();
    RoiList::iterator roiItr;
    ViewInfoList::iterator viewItr;

    SpecViewList *specList;

    int layerID;
};

typedef boost::shared_ptr<Gframe> spGframe_t;
extern spGframe_t nullFrame;

#endif /* AIPGFRAME_H */

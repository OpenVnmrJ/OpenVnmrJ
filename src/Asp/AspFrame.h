/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPFRAME_H
#define ASPFRAME_H


#include <list>

#include "sharedPtr.h"
#include "AspDataInfo.h"
#include "AspCellList.h"
#include "AspRoiList.h"
#include "AspTraceList.h"
#include "AspPeakList.h"
#include "AspAnnoList.h"
#include "AspIntegList.h"

class AspFrame;
typedef boost::shared_ptr<AspFrame> spAspFrame_t;
extern spAspFrame_t nullAspFrame;

typedef void (*DFUNC_t)(int frameID);
typedef std::list<DFUNC_t> AspDisplayListenerList;

class AspFrame
{
public:

    int id;			// Unique ID for this gframe
    double pixstx, pixsty;         // Upper-left corner of frame on window
    double pixwd, pixht;           // Size of frame in pixels

    double pstx,pwd,psty,pht; // pixels of FOV 

        void setDefaultFOV();
        void setCellFOV(int rows, int cols);
        void setFullSize();

    AspFrame(int id, double x, double y, double wd, double ht);
    ~AspFrame();

    void setLoc(int x, int y);
    void setSize(int w, int h);

    AspCellList *getCellList() {return cellList;}
    AspRoiList *getRoiList() {return roiList;}

    void draw();
    void drawSpec();
    void drawRois();
    void clear();

    void displayTop();

    spAspCell_t  getFirstCell();
    spAspCell_t selectCell(int x, int y);
    spAspRoi_t selectRoi(int x, int y, bool handle=false);
    void unselectRois();

    spAspRoi_t addRoi(int type, int x, int y);
    void deleteRoi(spAspRoi_t roi);
    void deleteRoi(int x, int y);
    void deleteRoi();
    void addRoiFromCursors();
    void addRoiFromCursors(double c1, double c2);
    void addRoiFromCursors(double c11, double c12, double c21, double c22);
    void modifyRoi(spAspRoi_t roi, int x, int y);
    void selectRoiHandle(spAspRoi_t roi, int x, int y, bool handle=false);
    void clearRois();
    void showRois(bool b);
    void setRoiColor(char *);
    void setRoiOpaque(int op);
    void setRoiHeight(int h);
    void loadRois(char *path);
    spAspDataInfo_t getDefaultDataInfo(bool update=false);
    spAspTrace_t getDefaultTrace();
    int getCursorMode();

    void registerAspDisplayListener(DFUNC_t func);
    void unregisterAspDisplayListener(DFUNC_t func);
    void callAspDisplayListeners();

    void updateDataMap();
    void initSpecFlag(int flag);
    void initDisFlag(int flag);
    void initAxisFlag(int flag);
    void initAnnoFlag(int flag);
    void initPeakFlag(int flag);
    void initIntegFlag(int flag);
    void setSpecFlag(int flag, bool on=true);
    void setDisFlag(int flag, bool on=true);
    void setAxisFlag(int flag, bool on=true);
    void setAnnoFlag(int flag, bool on=true);
    void setPeakFlag(int flag, bool on=true);
    void setIntegFlag(int flag, bool on=true);
    void clearAnnotations();
    void dsAgain();

    AspTraceList *getTraceList() {return traceList;}
    AspTraceList *getSelTraceList() {return selTraceList;}
    AspPeakList *getPeakList() {return peakList;}
    AspIntegList *getIntegList() {return integList;}
    AspAnnoList *getAnnoList() {return annoList;}

    bool doDs();
    bool ownScreen();
    void specVS(int clicks, double factor);

    void showCursor(int x, int y, int mode, int cursor);
    void startZoom(int x, int y);
    void setZoom(int x, int y, int prevX, int prevY);
    void zoomSpec(int x, int y, int mode);
    void panSpec(int x, int y, int prevX, int prevY, int mode);

    void writeFields(char *str);
    int getAxisFlag() {return axisFlag;}
    int getSpecFlag() {return specFlag;}
    int getDisFlag() {return disFlag;}
    int getAnnoFlag() {return annoFlag;}
    int getPeakFlag() {return peakFlag;}
    int getIntegFlag() {return integFlag;}

    int saveSession(char *path, bool full = false);
    int loadSession(char *path);
    int testSession(char *path, int &ntraces, int &straces, int &dataok);

    int select(int x, int y);
    void drawPlotBox();
    void moveFrame(int x,int y,int prevX,int prevY);
    void resizeFrame(int x,int y,int prevX,int prevY);
    int getSelect() {return frameSel;}
    void getFOVLimits(double &x, double &y, double &w, double &h);

    int currentTrace;
    bool threshSelected;

    int rows, cols;

    void updateMenu();
    bool annoTop;

private:

        spAspDataInfo_t defaultData;
        AspDataMap *dataMap;
        AspCellList *cellList;
	AspRoiList *roiList;
	AspTraceList *traceList;
	AspTraceList *selTraceList;
	AspPeakList *peakList;
	AspIntegList *integList;
	AspAnnoList *annoList;

 	int disFlag;
	int axisFlag;
	int annoFlag;
	int peakFlag;
	int integFlag;
        int specFlag;

        AspDisplayListenerList displayListenerList;

	spAspRoi_t prevRoi;

	int frameSel;
        bool aspMode;
};

#endif /* ASPFRAME_H */

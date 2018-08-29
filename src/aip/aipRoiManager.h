/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPROIMANAGER_H
#define AIPROIMANAGER_H

#include "aipRoi.h"

class RoiManager
{
public:
    static Roi *infoPoint;
    static Roi *infoLine;

    typedef enum {
	all,
	none
    } strArg_t;

    RoiManager();
    ~RoiManager();

    static RoiManager *get();

    // VNMR commands
    static int aipDeleteRois(int argc, char *argv[], int retc, char *retv[]);
    static int aipSelectRois(int argc, char *argv[], int retc, char *retv[]);
    static int aipSomeInfoUpdate(int argc, char *argv[],
				 int retc, char *retv[]);
    static int aipSaveRois(int argc, char *argv[], int retc, char *retv[]);
    static int aipLoadRois(int argc, char *argv[], int retc, char *retv[]);

    Roi *getFirstSelected();
    Roi *getNextSelected();
    void clearSelectedList();
    void addSelection(Roi *);	// Add to front of selected list
    bool removeSelection(Roi *); // Remove from selected list
    int numberSelected();
    bool isSelected(Roi *);	// Whether this ROI is in selected list
    Roi *selectRoi(int x, int y, bool append);
    Roi *highlightRoi(int x, int y, bool ignoreHandles,
		      Roi *prevRoi, int& prevHandle);
    int deleteSelectedRois();
    void selectDeselectBoundRois(Roi *roi);
    void selectDeselectAllRois(Roi *roi);
    void selectRois(strArg_t cmd);
    void removeAllRois();
    void removeActiveRois();
    void remove(Roi *);
    void clearInfoPointers(Roi *roi);
    void updateLineInfo();
    void updatePointInfo();
    //Roi *getFirstRoi();
    //Roi *getNextRoi();

    Roi *getFirstActive();
    Roi *getNextActive();
    void clearActiveList();
    void addActive(Roi *);	// Add to front of active list
    bool removeActive(Roi *); // Remove from active list
    int numberActive();
    bool isActive(Roi *);	// Whether this ROI is in active list
    Roi *activateRoi(int x, int y); // Activate by location
    Roi *findRoi(int x, int y);
    void roi_menu_save(const char *dirpath, // directory path name
                       const char *name, bool pixflag=false); // filename to be loaded
    void roi_menu_load(const char *dirpath, // directory path name
                       const char *name); // filename to be loaded
    void load_roi(ifstream &infile);
    bool isCopy(Roi *r1, Roi *r2);

    void selectDeselectAllNotdisplayedRois(bool b);
    void selectDeselectNotDisplayedBoundRois(Roi *src, bool b);
    void removeAllNotDisplayedRois();
    void removeNotDisplayedSelectedRois();
    void removeNotDisplayedActiveRois();
    void clearNotDisplayedActiveRois();
    void rebuildSelectedList();
    void rebuildActiveList();
    bool isLastDisplayedBoundRoi(Roi *src);
    void removeNotDisplayedBoundRoi(Roi *src);


private:
    static RoiManager *roiManager; // Support one instance

    RoiList *selectedRoiList;
    RoiList::iterator selectedRoiItr;
    RoiList *activeRoiList;
    RoiList::iterator activeRoiItr;

};

#endif /* AIPROIMANAGER_H */

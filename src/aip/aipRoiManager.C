/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <algorithm>
#include <iostream>
#include <unistd.h>

using std::find;
using std::ofstream;

#include "aipRoi.h"
#include "aipPoint.h"
#include "aipLine.h"
#include "aipRoiManager.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipCommands.h"
#include "aipInitStart.h"
#include "aipDataManager.h"

extern char UserName[];
extern char HostName[];

using namespace aip;

RoiManager *RoiManager::roiManager= NULL;
Roi *RoiManager::infoPoint= NULL;
Roi *RoiManager::infoLine= NULL;

RoiManager::RoiManager() {
    selectedRoiList = new RoiList;
    activeRoiList = new RoiList;
}

/* STATIC */
RoiManager *RoiManager::get() {
    if (!roiManager) {
        roiManager = new RoiManager();
    }
    return roiManager;
}

/* STATIC VNMRCOMMAND */
int RoiManager::aipDeleteRois(int argc, char *argv[], int retc, char *retv[]) {
    if (argc > 1) {
        fprintf(stderr,"Usage: %s\n", argv[0]);
        return proc_error;
    }
    get()->deleteSelectedRois();
    RoiStat::get()->calculate(false);
    Mouse::reset();
    return proc_complete;
}

/* STATIC VNMRCOMMAND */
int RoiManager::aipSelectRois(int argc, char *argv[], int retc, char *retv[]) {
    strArg_t cmd = all;
    if (argc > 1) {
        const char *arg = argv[1];
        if (strcasecmp(arg, "all") == 0) {
            cmd = all;
        } else if (strcasecmp(arg, "none") == 0) {
            cmd = none;
        }
    }
    get()->selectRois(cmd);
    RoiStat::get()->calculate(false);
    return proc_complete;
}

/* STATIC VNMRCOMMAND */
int RoiManager::aipSaveRois(int argc, char *argv[], int retc, char *retv[]) {
    argc--;
    argv++;

    if (argc < 1) {
        return proc_complete;
    }
    char *filename = argv[0];
    char fname[1024];
    if (*filename == '/') {
        *fname = '\0';
    } else {
        init_get_env_name(fname); /* Get the directory path */
        strcat(fname, "/roi/");
    }
    strcat(fname, filename);

    // set flag to save roi as data pixels
    bool pixflag = false;
    if(argc > 1 && strstr(argv[1],"pix") != NULL) pixflag = true;

    get()->roi_menu_save("", fname, pixflag);
    return proc_complete;
}

/* STATIC VNMRCOMMAND */
int RoiManager::aipLoadRois(int argc, char *argv[], int retc, char *retv[]) {
    argc--;
    argv++;

    if (argc != 1) {
        return proc_complete;
    }
    char *filename = argv[0];
    char fname[1024];
    if (*filename == '/') {
        *fname = '\0';
    } else {
        init_get_env_name(fname); /* Get the directory path */
        strcat(fname, "/roi/");
    }
    strcat(fname, filename);
    get()->roi_menu_load("", fname);
    RoiStat::get()->calculate(false);

    grabMouse();
    return proc_complete;

}

void RoiManager::roi_menu_save(const char *dirpath, // directory path name
        const char *name, bool pixflag) // filename to be loaded
{
    ofstream outfile; // output stream
    char filename[1024]; // complete filename
    time_t clock; // number of today seconds
    char *tdate; // pointer to the time
    char datetim[26];

    if (numberSelected() <= 0) {
        fprintf(stderr,"No ROIs are selected");
        return;
    }

    if (*name == '\0') {
        fprintf(stderr,"Need to specify output filename for saving");
        return;
    }

    (void)sprintf(filename, "%s/%s", dirpath, name);

    outfile.open(filename, ios::out);
    if (outfile.fail()) {
        fprintf(stderr,"Couldn't open \"%s\" for writing", filename);
        return;
    }

    // Output the comments
    clock = time(NULL);
    tdate = ctime(&clock);
    if (tdate != NULL) {
        strcpy(datetim,tdate);
        datetim[24] = '\0';
    } else {
        strcpy(datetim,"???");
    }
    outfile << "# ** Created by "<< UserName << " on "<< datetim << " at machine "
            << HostName << " **"<< "\n";
    if(pixflag) 
       outfile << "# data coordinates" << "\n";
    else
       outfile << "# physical coordinates" << "\n";

    // Now save the ROIs

    RoiList savedList;
    RoiList::iterator saveditr;
    bool saved;

    RoiList::iterator itr;
    if (selectedRoiList && selectedRoiList->size() > 0) {
        for (itr=selectedRoiList->begin(); itr!=selectedRoiList->end();) {
            saved = false;
            if (savedList.size() > 0)
                for (saveditr=savedList.begin(); saveditr!=savedList.end();) {
                    if (isCopy(*saveditr, *itr)) {
                        saved = true;
                        break;
                    }
                    saveditr++;
                }

            if (!saved) {
                savedList.push_back(*itr);
                (*itr)->save(outfile, pixflag);
            }
            itr++;
        }
    }

    outfile.close();
}

bool RoiManager::isCopy(Roi *r1, Roi *r2) {
    if (r1 == r2)
        return true;
    if (!r1 || !r2)
        return false;

    if (strcasecmp(r1->name(), r2->name()) != 0)
        return false;

    spCoordVector_t dat1 = r1->getMagnetCoords();
    spCoordVector_t dat2 = r2->getMagnetCoords();

    if (dat1 == dat2)
        return true;
    //if(!dat1 || !dat2) return false;

    if (dat1->coords.size() != dat2->coords.size())
        return false;

    for (unsigned int i=0; i<dat1->coords.size(); i++) {
        if (dat1->coords[i].x != dat2->coords[i].x||dat1->coords[i].y
                != dat2->coords[i].y)
            return false;
    }

    return true;
}

void RoiManager::roi_menu_load(const char *dirpath, // directory path name
        const char *name) // filename to be loaded
{
    ifstream infile; // input stream
    char filename[128]; // complte filename

    if (*name == '\0') {
        fprintf(stderr,"Need to specify input filename for loading");
        return;
    }

    (void)sprintf(filename, "%s/%s", dirpath, name);

    infile.open(filename, ios::in);
    if (infile.fail()) {
        fprintf(stderr,"Couldn't open \"%s\" for reading", filename);
        return;
    }

    Roi::load_roi(infile);
    infile.close();
}

Roi *RoiManager::getFirstSelected() {
    if (selectedRoiList && selectedRoiList->size() > 0) {
        selectedRoiItr = selectedRoiList->begin();
        if (selectedRoiItr != selectedRoiList->end()) {
            return *selectedRoiItr;
        }
    }
    return NULL;
}

Roi *RoiManager::getNextSelected() {
    if (++selectedRoiItr == selectedRoiList->end()) {
        return NULL;
    }
    return *selectedRoiItr;
}

void RoiManager::clearSelectedList() {
    // TODO: Delayed update of views of deselected ROIs
    Roi *roi;
    // NB: We do "getFirstSelected()" every time through, because
    //     we always remove the old first one;
    for (roi=getFirstSelected(); roi; roi=getFirstSelected()) {
        roi->deselect();
        roi->refresh(ROI_COPY);
    }
    selectDeselectAllNotdisplayedRois(false);
}

// Add to front of selected list
void RoiManager::addSelection(Roi *roi) {
    selectedRoiList->push_front(roi);
}

// Remove from selected list
bool RoiManager::removeSelection(Roi *roi) {
    bool rtn = false;
    RoiList::iterator itr;
    if (selectedRoiList && selectedRoiList->size() > 0) {
        for (itr=selectedRoiList->begin(); itr!=selectedRoiList->end();) {
            if (*itr == roi) {
                itr = selectedRoiList->erase(itr);
                rtn = true;
            } else {
                itr++;
            }
        }
    }
    return rtn;
}

int RoiManager::numberSelected() {
    if (selectedRoiList) {
        return selectedRoiList->size();
    } else {
        return 0;
    }
}

// Whether this ROI is in selected list
bool RoiManager::isSelected(Roi *roi) {
    RoiList::iterator itr;
    itr = find(selectedRoiList->begin(), selectedRoiList->end(), roi);
    return itr != selectedRoiList->end();
}

Roi *RoiManager::selectRoi(int x, int y, bool appendFlag) {
    Roi *roi = findRoi(x, y);
    if (roi) {
        roi->select(ROI_COPY, appendFlag);
    } else {
        clearSelectedList();
    }
    return roi;
}

Roi *RoiManager::highlightRoi(int x, int y, bool ignoreHandles, Roi *prevRoi,
        int& prevHandle) {
    Roi *roi = findRoi(x, y);
    int handle = -1;
    if (roi && !ignoreHandles) {
        // See if we're near a handle
        handle = roi->getHandle(x, y);
    }
    if (prevRoi && prevRoi != roi) {
        // Unhighlight a previous ROI
        prevRoi->setRollover(false);
    }
    if (roi && roi != prevRoi) {
        if (handle < 0) {
            roi->setRollover(true);
        } else {
            roi->setRolloverHandle(handle);
        }
    } else if (roi && roi == prevRoi && handle != prevHandle) {
        if (handle < 0) {
            roi->setRollover(true);
        } else {
            roi->setRolloverHandle(handle);
        }
    }
    prevHandle = handle;

    return roi;
}

int RoiManager::deleteSelectedRois() {
    clearActiveList();

    // We first make a list of what we are going to delete, because
    // "remove()" modifies the "selected" list, which would break the
    // iteration over selected ROIs.
    int n = 0;
    Roi *roi;
    RoiList tmpList;
    for (roi=getFirstSelected(); roi; roi=getNextSelected()) {
        if (!roi->isActive()) {
            tmpList.push_back(roi);
            ++n;
        }
    }
    RoiList::iterator itr;
    for (itr=tmpList.begin(); itr!=tmpList.end(); itr++) {
        remove(*itr);
    }

    removeNotDisplayedSelectedRois();

    if (n) {
        GframeManager::get()->draw(); // Refresh screen
    }
    return n;
}

void RoiManager::selectDeselectBoundRois(Roi *roi) {
    bool select = isSelected(roi) ? FALSE : TRUE;
    if (select) {
        roi->select(ROI_COPY, TRUE);
    } else {
        roi->deselect();
    }
    roi = getFirstActive();
    while (roi) {
        if (select) {
            roi->select(ROI_COPY, TRUE);
        } else {
            roi->deselect();
        }
        roi = getNextActive();
    }

    selectDeselectNotDisplayedBoundRois(roi, select);
}

void RoiManager::selectDeselectAllRois(Roi *roi) {
    if (isSelected(roi)) {
        // ROI is selected; deselect this (and all other) ROIs
        selectRois(none);
    } else {
        // ROI is unselected; select this (and all other) ROIs
        selectRois(all);
    }
}

void RoiManager::selectRois(strArg_t cmd) {
    spGframe_t gf;
    
    GframeManager *gfm = GframeManager::get();
    int binding = (int)getReal("aipRoiBind", 0);
    switch (cmd) {
    case none: // Deselect all frames
        clearSelectedList();
        break;

    case all:
        if (binding == DATA_ALL) {
            GframeCache_t::iterator gfci;
            for (gf=gfm->getFirstCachedFrame(gfci); gf != nullFrame; gf
                    =gfm->getNextCachedFrame(gfci)) {
                Roi *roi;
                while ( (roi=gf->getFirstUnselectedRoi()) ) {
                    roi->select(ROI_COPY, true);
                }
            }
        }
        else{
            GframeList::iterator gfi;
            for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
                    =gfm->getNextFrame(gfi)) {
                Roi *roi;
                while ( (roi=gf->getFirstUnselectedRoi()) ) {
                    roi->select(ROI_COPY, true);
                }
            }
            selectDeselectAllNotdisplayedRois(true);
        }
    
        break;

    default:
        break;
    }
}

void RoiManager::removeAllRois() {
    clearActiveList();

    spGframe_t gf;
    GframeList::iterator gfi;
    GframeManager *gfm = GframeManager::get();
    for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf=gfm->getNextFrame(gfi)) {
        Roi *roi;
        while ( (roi=gf->getFirstRoi()) ) {
            remove(roi);
        }
    }
    removeAllNotDisplayedRois();
}

void RoiManager::removeActiveRois() {
    int numActive = numberActive();
    Roi **roivec = new Roi*[numActive];

    Roi *roi = getFirstActive();
    for (int i = 0; i < numActive && roi; ++i) {
        roivec[i] = roi;
        roi = getNextActive();
    }
    for (int i = 0; i < numActive; ++i) {
        removeActive(roivec[i]);
        //removeSelection(roi);
        remove(roivec[i]);
    }
    //clearActiveList();

    removeNotDisplayedActiveRois();
}

void RoiManager::remove(Roi *roi) {

    if (!roi || roi->isActive()) {
        return;
    }

    delete roi;
}

void RoiManager::clearInfoPointers(Roi *roi) {
    if (infoPoint == roi) {
        Point::clearInfo();
        infoPoint = NULL;
    } else if (infoLine == roi) {
        Line::clearInfo();
        infoLine = NULL;
    }
}

/* STATIC VNMRCOMMAND */
int RoiManager::aipSomeInfoUpdate(int argc, char *argv[], int retc,
        char *retv[]) {
    RoiManager *roim = RoiManager::get();
    Roi *roi;

    if (argc > 1&& strcasecmp(argv[1], "roiInfo") == 0&& retc == 1) {
        retv[0] = realString((double)roim->selectedRoiList->size());
        return proc_complete;
    } else if (argc > 1&& strcasecmp(argv[1], "roiInfo") == 0&& retc == 2) {
        retv[0] = realString((double)roim->selectedRoiList->size());
        std::set<string> seltypes;
        for (roi=roim->getFirstSelected(); roi; roi=roim->getNextSelected()) {
            seltypes.insert(roi->getpntData()->name);
        }
        retv[1] = realString((double)seltypes.size());
        return proc_complete;
    } else if (argc > 1&& strcasecmp(argv[1], "roiInfo") == 0&& retc == 3) {
        retv[0] = realString((double)roim->selectedRoiList->size());
        std::set<string> seltypes;
        for (roi=roim->getFirstSelected(); roi; roi=roim->getNextSelected()) {
            seltypes.insert(roi->getpntData()->name);
        }
        retv[1] = realString((double)seltypes.size());
        string types;
        set<string>::iterator sitr;
        for (sitr = seltypes.begin(); sitr != seltypes.end(); ++sitr) {
            types += *sitr;
            types += " ";
        }
        retv[2] = newString(types.c_str());
        return proc_complete;

    } else if (argc > 1) {
        fprintf(stderr,"Usage: %s\n", argv[0]);
        return proc_error;
    }
    get()->updateLineInfo();
    get()->updatePointInfo();
    return proc_complete;
}

void RoiManager::updateLineInfo() {
    if (infoLine != NULL) {
        infoLine->someInfo(false);
    } else {
        Line::clearInfo();
    }
}

void RoiManager::updatePointInfo() {
    if (infoPoint != NULL) {
        infoPoint->someInfo(false);
    } else {
        Point::clearInfo();
    }
}

Roi *RoiManager::getFirstActive() {
    if (activeRoiList && activeRoiList->size() > 0) {
        activeRoiItr = activeRoiList->begin();
        if (activeRoiItr != activeRoiList->end()) {
            return *activeRoiItr;
        }
    }
    return NULL;
}

Roi *RoiManager::getNextActive() {
    if (++activeRoiItr == activeRoiList->end()) {
        return NULL;
    }
    return *activeRoiItr;
}

void RoiManager::clearActiveList() {
    RoiList::iterator itr;
    if (activeRoiList && activeRoiList->size() > 0) {
        for (itr=activeRoiList->begin(); itr!=activeRoiList->end(); ++itr) {
            (*itr)->setActive(false);
        }
    }

    activeRoiList->clear();

    spGframe_t gf;
    GframeList::iterator gfi;
    GframeManager *gfm = GframeManager::get();
    for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf=gfm->getNextFrame(gfi)) {
        Roi *roi;
        for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
            roi->setActive(false);
        }
    }

    clearNotDisplayedActiveRois();
}

// Add to front of active list
void RoiManager::addActive(Roi *roi) {
    activeRoiList->push_back(roi);
    roi->setActive(true);
}

// Remove from active list
bool RoiManager::removeActive(Roi *roi) {
    bool rtn = false;
    RoiList::iterator itr;
    if (activeRoiList && activeRoiList->size() > 0) {
        for (itr=activeRoiList->begin(); itr!=activeRoiList->end();) {
            if (*itr == roi) {
                itr = activeRoiList->erase(itr);
                roi->setActive(false);
                rtn = true;
            } else {
                itr++;
            }
        }
    }
    return rtn;
}

int RoiManager::numberActive() {
    if (activeRoiList) {
        return activeRoiList->size();
    } else {
        return 0;
    }
}

// Whether this ROI is in active list
bool RoiManager::isActive(Roi *roi) {
    bool rtn = false;
    RoiList::iterator itr;
    if (activeRoiList && activeRoiList->size() > 0) {
        for (itr=activeRoiList->begin(); itr!=activeRoiList->end();) {
            if (*itr == roi) {
                rtn = true;
            } else {
                itr++;
            }
        }
    }
    return rtn;
}

Roi *RoiManager::activateRoi(int x, int y) {
    Roi *roi = findRoi(x, y);
    if (roi) {
        clearActiveList();
        addActive(roi);
    }
    return roi;
}

Roi *RoiManager::findRoi(int x, int y) {
    spGframe_t gf = GframeManager::get()->getGframeFromCoords(x, y);
    if (gf == nullFrame) {
        return NULL;
    }
    Roi *roi;
    Roi *minRoi;
    double dist;
    double minDist;
    if ((roi=gf->getFirstRoi()) == NULL) {
        return NULL;
    }
    dist = minDist = roi->distanceFrom(x, y, -1);
    minRoi = roi;
    for (roi=gf->getNextRoi() ; roi; roi=gf->getNextRoi()) {
        dist = roi->distanceFrom(x, y, minDist);
        // NB: Use "<=" so that if 2 ROIs are exactly superimposed, we
        //     find the one on top.  Otherwise, changing the found
        //     ROI's color will have no visible effect.
        //     (ROIs are drawn in the their order in the list.)
        if (dist <= minDist) {
            minDist = dist;
            minRoi = roi;
        }
    }
    if (minDist <= Roi::aperture) {
        return minRoi;
    } else {
        return NULL;
    }
}

void RoiManager::selectDeselectAllNotdisplayedRois(bool b) {
    spGframe_t gf;
    GframeCache_t::iterator gfi;
    GframeManager *gfm = GframeManager::get();
    for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextCachedFrame(gfi)) {
        if (gfm->isFrameDisplayed(gf))
            continue;

        Roi *roi;
        for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
            if (b)
                roi->roi_set_state(ROI_STATE_MARK);
            else
                roi->roi_clear_state(ROI_STATE_MARK);
        }
    }

    DataManager *dm = DataManager::get();
    DataMap *dataMap = dm->getDataMap();
    DataMap::iterator pd;
    for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
        pd->second->setRoiSelected_all(b);
    }
}

void RoiManager::removeAllNotDisplayedRois() {
    spGframe_t gf;
    GframeCache_t::iterator gfi;
    GframeManager *gfm = GframeManager::get();
    Roi *roi;
    for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextCachedFrame(gfi)) {
        if (gfm->isFrameDisplayed(gf))
            continue;

        while ( (roi=gf->getFirstRoi()) ) {
            remove(roi);
        }
    }

    DataManager *dm = DataManager::get();
    DataMap *dataMap = dm->getDataMap();
    DataMap::iterator pd;
    for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
        pd->second->removeAllRois();
    }
}

void RoiManager::removeNotDisplayedSelectedRois() {
    spGframe_t gf;
    GframeCache_t::iterator gfi;
    GframeManager *gfm = GframeManager::get();
    RoiList tmpList;
    Roi *roi;
    for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextCachedFrame(gfi)) {
        if (gfm->isFrameDisplayed(gf))
            continue;

        for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
            if (roi->roi_state(ROI_STATE_MARK)) {
                tmpList.push_back(roi);
            }
        }
    }

    if (tmpList.size() > 0) {
        RoiList::iterator itr;
        for (itr=tmpList.begin(); itr!=tmpList.end(); itr++) {
            remove(*itr);
        }
    }

    DataManager *dm = DataManager::get();
    DataMap *dataMap = dm->getDataMap();
    DataMap::iterator pd;
    for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
        pd->second->removeSelectedRois();
    }
}

// called in selectDeselectBoundRois, which is called in aipMouse
// to select active Rois. so bound rois are both activated and selected. 
void RoiManager::selectDeselectNotDisplayedBoundRois(Roi *src, bool b) {
    if (!src)
        return;

    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeCache_t::iterator gfi;
    for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextCachedFrame(gfi)) {
        if (gfm->isFrameDisplayed(gf))
            continue;

        if (gf.get() != src->pOwnerFrame) {
            Roi *roi;
            for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
                if (roi->pntData == src->pntData) {
                    // ROI is bound to this one if has same data points
                    if (b) {
                        roi->roi_set_state(ROI_STATE_MARK);
                    } else {
                        roi->roi_clear_state(ROI_STATE_MARK);
                    }
                    roi->setNotDisplayedActive(b);
                    break; // Only one per frame!
                }
            }
        }
    }

    DataManager *dm = DataManager::get();
    DataMap *dataMap = dm->getDataMap();
    DataMap::iterator pd;
    for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
        pd->second->setRoiSelected(src->pntData, b);
        pd->second->setRoiActive(src->pntData, b);
    }
}

void RoiManager::clearNotDisplayedActiveRois() {
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeCache_t::iterator gfi;
    for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextCachedFrame(gfi)) {
        if (gfm->isFrameDisplayed(gf))
            continue;

        Roi *roi;
        for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
            if (roi->isActive())
                roi->setNotDisplayedActive(false);
        }
    }

    DataManager *dm = DataManager::get();
    DataMap *dataMap = dm->getDataMap();
    DataMap::iterator pd;
    for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
        pd->second->setRoiActive_all(false);
    }
}

void RoiManager::removeNotDisplayedActiveRois() {
    spGframe_t gf;
    GframeCache_t::iterator gfi;
    GframeManager *gfm = GframeManager::get();
    RoiList tmpList;
    Roi *roi;
    for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextCachedFrame(gfi)) {
        if (gfm->isFrameDisplayed(gf))
            continue;

        for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
            if (roi->isActive())
                tmpList.push_back(roi);
        }
    }

    if (tmpList.size() > 0) {
        RoiList::iterator itr;
        for (itr=tmpList.begin(); itr!=tmpList.end(); itr++) {
            remove(*itr);
        }
    }

    DataManager *dm = DataManager::get();
    DataMap *dataMap = dm->getDataMap();
    DataMap::iterator pd;
    for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
        pd->second->removeActiveRois();
    }
}

void RoiManager::rebuildSelectedList() {

    selectedRoiList->clear();

    spGframe_t gf;
    GframeCache_t::iterator gfi;
    GframeManager *gfm = GframeManager::get();
    Roi *roi;
    for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextCachedFrame(gfi)) {
        if (gfm->isFrameDisplayed(gf))
            for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
                if (roi->roi_state(ROI_STATE_MARK)) {
                    selectedRoiList->push_back(roi);
                }
            }
    }
}

void RoiManager::rebuildActiveList() {
    clearActiveList();
    /*
     activeRoiList->clear();

     spGframe_t gf;
     GframeCache_t::iterator gfi;
     GframeManager *gfm = GframeManager::get();
     Roi *roi;
     for (gf=gfm->getFirstCachedFrame(gfi);
     gf != nullFrame;
     gf=gfm->getNextCachedFrame(gfi))
     {
     if(gfm->isFrameDisplayed(gf))
     for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
     if(roi->isActive()) {
     activeRoiList->push_back(roi);
     }
     }
     }
     */
}

bool RoiManager::isLastDisplayedBoundRoi(Roi *src) {

    spGframe_t gf;
    GframeCache_t::iterator gfi;
    GframeManager *gfm = GframeManager::get();
    Roi *roi;
    for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextCachedFrame(gfi)) {
        if (gfm->isFrameDisplayed(gf))
            for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
                if (roi == src)
                    continue;
                if (roi->pntData == src->pntData)
                    return false;
            }
    }
    return true;
}

void RoiManager::removeNotDisplayedBoundRoi(Roi *src) {
    spGframe_t gf;
    GframeCache_t::iterator gfi;
    GframeManager *gfm = GframeManager::get();
    RoiList tmpList;
    for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextCachedFrame(gfi)) {
        if (gfm->isFrameDisplayed(gf))
            continue;

        Roi *roi;
        for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
            if (roi == src)
                continue;
            if (roi->pntData == src->pntData)
                tmpList.push_back(roi);
        }
    }

    if (tmpList.size() > 0) {
        RoiList::iterator itr;
        for (itr=tmpList.begin(); itr!=tmpList.end(); itr++) {
            delete *itr;
        }
    }

    DataManager *dm = DataManager::get();
    DataMap *dataMap = dm->getDataMap();
    DataMap::iterator pd;
    for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
        pd->second->removeRoi(src->pntData);
    }
}


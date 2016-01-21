/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <iostream>
#include <string>
using std::string;
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>             // atoi, atof
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "aipUtils.h"
#include "aipVnmrFuncs.h"
#include "aipCommands.h"
using namespace aip;
#include "aipStderr.h"
#include "aipVsInfo.h"
#include "aipMouse.h"
#include "aipDataManager.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipHistogram.h"

const string VsInfo::baseFilename = "/tmp/VjVsHistogram";
palette_t *VsInfo::palette = NULL;
palette_t *VsInfo::paletteList = NULL;
spVsInfo_t nullVs = spVsInfo_t(NULL);

VsInfo::VsInfo()
{
    aipDprint(DEBUGBIT_0,"VsInfo()\n");
    size = 0;
    table = NULL;
    localPalette = palette;
    minData = 0.0;
    maxData = 0.0;
    uFlowColor = (uchar_t)0;
    oFlowColor = (uchar_t)0;
}

VsInfo::VsInfo(spVsInfo_t oldVs) {
    table = NULL;
    localPalette = palette;
    init(oldVs, oldVs->size);
}

VsInfo::VsInfo(double min, double max)
{
    table = NULL;
    localPalette = palette;
    setup(min, max);
}

VsInfo::VsInfo(double min, double max, int colormapId)
{
    if (paletteList != NULL && colormapId >= 0) {
        localPalette = &paletteList[colormapId];
        if (localPalette->numColors < 2)
            localPalette = NULL;
    }
    if (localPalette == NULL) 
       localPalette = palette;
    table = NULL;
    setup(min, max);
}

VsInfo::VsInfo(spVsInfo_t oldVs, int cmapSize)
{
    table = NULL;
    localPalette = palette;
    init(oldVs, cmapSize);
}

void
VsInfo::setup(double min, double max)
{
    int i;
    aipDprint(DEBUGBIT_0,"VsInfo(min, max)\n");
    minData = min;
    maxData = max;
    uFlowColor = localPalette->firstColor;
    oFlowColor = localPalette->firstColor + localPalette->numColors - 1;
    if (table != NULL)
        delete[] table;
    size = localPalette->numColors;
    table = new u_char[size];
    for (i=0; i<size; i++) {
	table[i] = localPalette->firstColor + i;
    }
    command = "curve 0.5 0.5 imin 0 imax 1 ";
    setString("aipVsFunction", getCommand(), true);
}

void
VsInfo::init(spVsInfo_t oldVs, int cmapSize)
{
    aipDprint(DEBUGBIT_0,"VsInfo(oldVs, cmapSize)\n");
    minData = oldVs->minData;
    maxData = oldVs->maxData;
    uFlowColor = oldVs->uFlowColor;
    oFlowColor = oldVs->oFlowColor;
    command = oldVs->command;
    size = cmapSize;
    table = new u_char[size];
    u_char *oldTable = oldVs->table;
    if (size == oldVs->size) {
        for (int i=0; i< size; ++i) {
            table[i] = oldTable[i];
        }
    } else {
//      int ci = oldVs->table[0];
//      int cf = oldVs->table[oldVs->size - 1];
        double r = (double)oldVs->size / size;
        for (int i=0; i<size; i++) {
            int j = (int)(i * r + 0.5);
            if (j >= oldVs->size) {
                j = oldVs->size - 1;
            }
            table[i] = oldVs->table[j];
        }
    }
}

VsInfo::VsInfo(double mindat, double maxdat, string path, int& lastId)
{
    aipDprint(DEBUGBIT_0,"VsInfo(min, max, path)\n");
    minData = mindat;
    maxData = maxdat;
    table = NULL;
    localPalette = palette;
    uFlowColor = localPalette->firstColor;
    oFlowColor = localPalette->firstColor + localPalette->numColors - 1;

    FILE *fin;
    fin = fopen(path.c_str(), "r");
    if (!fin) {
	fprintf(stderr,"Cannot open VS Function file: %s\n", path.c_str());
        size = 0;
	return;
    }

    // Set header info
    char buf[1024];
    size = 1024;		// Default
    while (fgets(buf, sizeof(buf), fin)) {
	if (strncmp("data", buf, 4) == 0) {
	    break;
        } else if (strncmp("id", buf, 2) == 0) {
            int id = atoi(buf+3);
            if (id == lastId) {
                size = 0;
                fclose(fin);
                return;
            } else {
                lastId = id;
            }
	} else if (strncmp("size", buf, 4) == 0) {
	    size = atoi(buf+5);
	    if (size < 1) { size = 1; }
	    if (size > 32768) { size = 32768; }
	} else if (strncmp("minData", buf, 7) == 0) {
            minData = atof(buf+8);
            setReal("aipVsDataMin", minData, true);
        } else if (strncmp("maxData", buf, 7) == 0) {
            maxData = atof(buf+8);
            setReal("aipVsDataMax", maxData, true);
        } else if (strncmp("uflowColor", buf, 10) == 0) {
            // NB: TODO: This is a relative colormap index for now
            uFlowColor = (uchar_t)atoi(buf+11) + localPalette->firstColor;
        } else if (strncmp("oflowColor", buf, 10) == 0) {
            // NB: TODO: This is a relative colormap index for now
            oFlowColor = (uchar_t)atoi(buf+11) + localPalette->firstColor;
        } else if (strncmp("function ", buf, 9) == 0) {
            command = (buf + 9);
            std::string::size_type idx;
            while ((idx = command.find('\n')) != std::string::npos) {
                command.erase(idx, 1); // Delete linefeed chars
            }
        }
    }
    setString("aipVsFunction", getCommand(), true);

    // Read data
    // For now, we get passed the relative index, which we use as an
    // offset into the colormap table.  TODO: Deal with absolute indices
    int minval = 0;
    int maxval = localPalette->numColors - 1;
    table = new u_char[size];
    int idx;
    for (idx=0; fgets(buf, sizeof(buf), fin) && idx < size; ++idx) {
	int val = atoi(buf);
	if (val < minval) {
	    val = 0;
	} else if (val > maxval) {
	    val = maxval;
	}
	table[idx] = val + localPalette->firstColor;
    }
    size = idx;			// In case some values were missing
    fclose(fin);
}

VsInfo::VsInfo(string cmd)
{
    aipDprint(DEBUGBIT_0,"VsInfo()\n");
    size = 0;
    table = NULL;
    localPalette = palette;
    makeTableFromCommand(cmd);
    uFlowColor = table[0];      // No special under/over-flow colors
    oFlowColor = table[size - 1];
}

VsInfo::VsInfo(string cmd, int colormapId)
{
    if (paletteList != NULL && colormapId > 0) {
        localPalette = &paletteList[colormapId];
        if (localPalette->numColors < 2)
           localPalette = NULL;
    }
    if (localPalette == NULL) 
       localPalette = palette;

    size = 0;
    table = NULL;
    makeTableFromCommand(cmd);
    uFlowColor = table[0];      // No special under/over-flow colors
    oFlowColor = table[size - 1];
}

VsInfo::~VsInfo()
{
    aipDprint(DEBUGBIT_0,"~VsInfo()\n");
    delete[] table;
}

/*STATIC*/
string
VsInfo::getVsCmdFromFile(string path, int& lastId)
{
    string cmd = "";
    string dmin = "";
    string dmax = "";
    FILE *fin;
    fin = fopen(path.c_str(), "r");
    if (!fin) {
	fprintf(stderr,"Cannot open VS Function file: %s\n", path.c_str());
	return cmd;
    }
    char buf[1024];
    int fields = 0;
    while (fields < 4 && fgets(buf, sizeof(buf), fin)) {
        if (strncmp("id", buf, 2) == 0) {
            ++fields;
            int id = atoi(buf+3);
            if (id == lastId) {
                fclose(fin);
                return cmd;
            } else {
                lastId = id;
            }
	} else if (strncmp("minData", buf, 7) == 0) {
            ++fields;
            dmin = buf + 8;
            std::string::size_type idx;
            while ((idx = dmin.find('\n')) != std::string::npos) {
                dmin.erase(idx, 1); // Delete linefeed chars
            }
            double minData = atof(buf+8);
            setReal("aipVsDataMin", minData, true);
        } else if (strncmp("maxData", buf, 7) == 0) {
            ++fields;
            dmax = buf + 8;
            std::string::size_type idx;
            while ((idx = dmax.find('\n')) != std::string::npos) {
                dmax.erase(idx, 1); // Delete linefeed chars
            }
            double maxData = atof(buf+8);
            setReal("aipVsDataMax", maxData, true);
	} else if (strncmp("function ", buf, 9) == 0) {
            ++fields;
            cmd = (buf + 9);
            std::string::size_type idx;
            while ((idx = cmd.find('\n')) != std::string::npos) {
                cmd.erase(idx, 1); // Delete linefeed chars
            }
        }
    }
    cmd = cmd + " dmin " + dmin + " dmax " + dmax;
    fclose(fin);
    return cmd;
}

/* STATIC VNMRCOMMAND */
int
VsInfo::aipSetVsFunction(int argc, char *argv[], int retc, char *retv[])
{
    static int prevId = 0;
    string vsCommand = "";
    int iarg;
    bool fromCommand = false;
    bool fromFile = false;
    bool makeHistogram = false;
    for (iarg=1; iarg < argc; ++iarg) {
	if (strcasecmp(argv[iarg], "cmd") == 0) {
            if (argc - iarg > 1) {
                fromCommand = true;
                vsCommand = argv[iarg + 1];
            }
	} else if (strcasecmp(argv[iarg], "hist") == 0) {
	    makeHistogram = true;
	} else if (strcasecmp(argv[iarg], "file") == 0) {
	    fromFile = true;
	}
    }

    int mode = getVsMode();
    int rtn = proc_error;
    if (makeHistogram) {
        setVsHistogram();
        return proc_complete;
    } else if (fromCommand) {   // TODO: Implement this at VJ end
        rtn = setVsFromCommand(vsCommand);
    } else if (fromFile) {
	string path = getString("aipVsFunctionFile", "/tmp/cltJunk");
        string vscmd = getVsCmdFromFile(path, prevId);
        rtn = setVsFromCommand(vscmd);
    } else {
        rtn = applyVsMode(mode);
    }

    setVsDifference();
    return rtn;
}

/* STATIC VNMRCOMMAND */
int
VsInfo::aipSaveVs(int argc, char *argv[], int retc, char *retv[])
{
    DataManager::get()->saveAllVs();
    return proc_complete;
}

/*STATIC*/
int
VsInfo::setVsFromFile(string filepath, int& prevId)
{
    string vscmd = getVsCmdFromFile(filepath, prevId);
    return setVsFromCommand(vscmd);
}

/*STATIC*/
int
VsInfo::setVsFromCommand(string vscmd)
{
    if (vscmd.length() == 0) {
        vscmd = "curve .5 .5 imin 0 imax 1 dmin 0 dmax 0.1";
    }
    DataManager *dm = DataManager::get();
    int mode = getVsMode();
    set<string> keys;
    switch (mode) {
    case VS_HEADER:
/*
        keys = dm->getKeys(DATA_SELECTED_FRAMES);
	if(keys.size() <= 0) keys = dm->getKeys(DATA_ALL);
        break;
*/
    case VS_UNIFORM:
        keys = dm->getKeys(DATA_ALL);
        break;
    case VS_DISPLAYED:
        keys = dm->getKeys(DATA_DISPLAYED);
        break;
    case VS_SELECTEDFRAMES:
        keys = dm->getKeys(DATA_SELECTED_FRAMES);
        break;
    case VS_INDIVIDUAL:
        keys = dm->getKeys(DATA_SELECTED_FRAMES);
        if (keys.size() == 0) {
            keys = dm->getKeys(DATA_ALL);
        }
        break;
    case VS_OPERATE:
        keys = dm->getKeys(DATA_SELECTED_FRAMES);
        if (keys.size() == 0) {
            keys = dm->getKeys(DATA_ALL); // None selected; do all images
        } else {
            // See if any selected images are in operate-on group
            set<string> opkeys = dm->getKeys(DATA_OPERATE);
            set<string>::iterator keyItr;
            bool match = false;
            for (keyItr = keys.begin(); keyItr != keys.end(); ++keyItr) {
                if (opkeys.count(*keyItr) > 0) {
                    match = true; // Already in op-group
                }
            }
            if (match) {
                // Use the combined sets
                keys.insert(opkeys.begin(), opkeys.end());
            }
            // "keys" now contains all selected frames
            // ... + the op-group if any selected frames are in op-group
        }
        break;
    case VS_GROUP:
        keys = dm->getKeys(DATA_SELECTED_FRAMES);
        if (keys.size() == 0) {
            keys = dm->getKeys(DATA_ALL); // None selected; do all images
        } else {
            set<string> grps = dm->getGroups();
            unsigned int nGrps = grps.size();
            set<string> myGrps;
            set<string>::iterator itr;
            for (itr = keys.begin(); itr != keys.end(); ++itr) {
                // Is this key in one of the groups?
                string grp = dm->getDataInfoByKey(*itr)->getGroup();
                int nFound = grps.count(grp);
                if (nFound > 0) {
                    myGrps.insert(grp);
                    if (myGrps.size() == nGrps) {
                        // Already selected all the groups
                        break;
                    }
                }
            }
            for (itr = myGrps.begin(); itr != myGrps.end(); ++itr) {
                set<string> gkeys = dm->getKeys(DATA_GROUP, *itr);
                keys.insert(gkeys.begin(), gkeys.end());
            }
        }
        break;
    }

    // Now set VS for all keys in our list
    setVsFromCommand(keys, vscmd);
    setVsDifference();
    return proc_complete;
}

/*STATIC*/
int
VsInfo::applyVsMode(int mode)
{
    DataManager *dm = DataManager::get();
    if (mode == VS_GROUP) {
        set<string> groups = dm->getGroups();
        set<string>::iterator gItr;
        for (gItr = groups.begin(); gItr != groups.end(); ++gItr) {
            set<string> keys = dm->getKeys(DATA_GROUP, *gItr);
            autoVsGroup(keys);
        }
    } else if (mode == VS_OPERATE) {
        set<string> keys = dm->getKeys(DATA_OPERATE);
        autoVsGroup(keys);
    } else if (mode == VS_SELECTEDFRAMES) {
        set<string> keys = dm->getKeys(DATA_SELECTED_FRAMES);
        autoVsGroup(keys);
    } else if (mode == VS_DISPLAYED) {
        set<string> keys = dm->getKeys(DATA_DISPLAYED);
        autoVsGroup(keys);
    } else if (mode == VS_INDIVIDUAL) {
        set<string> keys = dm->getKeys(DATA_ALL);
        set<string>::iterator keyItr;
        for (keyItr = keys.begin(); keyItr != keys.end(); ++keyItr) {
            autoVs(*keyItr);
        }
    } else if (mode == VS_UNIFORM) {
        string cmd = getString("aipVsFunction", "");
        set<string> keys = dm->getKeys(DATA_ALL);
        //setVsFromCommand(keys, cmd);
        autoVsGroup(keys);
    } else if (mode == VS_HEADER) {
        set<string> keys = dm->getKeys(DATA_ALL);
        set<string>::iterator keyItr;
        for (keyItr = keys.begin(); keyItr != keys.end(); ++keyItr) {
            spDataInfo_t di = dm->getDataInfoByKey(*keyItr);
            string cmd = di->getString("vsFunction", "");
            if (cmd.length() > 0) {
                setVsFromCommand(*keyItr, cmd);
            }
        }
    } else if (mode == VS_NONE) {
        set<string> keys = dm->getKeys(DATA_ALL);
        set<string>::iterator keyItr;
        for (keyItr = keys.begin(); keyItr != keys.end(); ++keyItr) {
            useDefaultVs(*keyItr);
        }
    }
    return proc_complete;
}

/*STATIC*/
bool
VsInfo::setVsDifference()
{
    double d1, d2;
    std::string cmd;
    return setVsDifference(d1, d2, cmd, true);
}

/*STATIC*/
bool
VsInfo::setVsDifference(double& vsMin, double& vsMax,
                        std::string& vsCmd, bool notify)
{
    bool vsDifferent = false;
    vsMin = getReal("aipVsDataMin", 0);
    vsMax = getReal("aipVsDataMax", 0.1);
    vsCmd = getString("aipVsFunction", "");
    spGframe_t gf;
    spViewInfo_t view;
    spVsInfo_t vsi;
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    bool maxMinSet = false;
    //int vsMode = (int)getReal("aipVsBind", 0);
    int vsMode = getVsMode();
    if (vsMode == 2 || gfm->getFirstSelectedFrame(gfi) == nullFrame) {
        // Look at all frames
        for (gf=gfm->getFirstFrame(gfi);
             gf != nullFrame;
             gf=gfm->getNextFrame(gfi))
        {
            if ((view = gf->getSelView()) != nullView
                && (vsi = view->imgInfo->getVsInfo()) != nullVs)
            {
                if (maxMinSet) {
                    if (vsMin != view->imgInfo->getVsMin()
                        || vsMax != view->imgInfo->getVsMax()
                        || vsCmd != view->imgInfo->getVsCommand())
                    {
                        vsDifferent = true;
                    }
                } else {
                    vsCmd = view->imgInfo->getVsCommand();
                    vsMin = view->imgInfo->getVsMin();
                    vsMax = view->imgInfo->getVsMax();
                    maxMinSet = true;
                }
            }
        }
    } else {
        // Look only at selected frames
        for (gf=gfm->getFirstSelectedFrame(gfi);
             gf != nullFrame;
             gf=gfm->getNextSelectedFrame(gfi))
        {
            if ((view = gf->getSelView()) != nullView
                && (vsi = view->imgInfo->getVsInfo()) != nullVs)
            {
                if (maxMinSet) {
                    if (vsMin != view->imgInfo->getVsMin()
                        || vsMax != view->imgInfo->getVsMax()
                        || vsCmd != view->imgInfo->getVsCommand())
                    {
                        vsDifferent = true;
                    }
                } else {
                    vsCmd = view->imgInfo->getVsCommand();
                    vsMin = view->imgInfo->getVsMin();
                    vsMax = view->imgInfo->getVsMax();
                    maxMinSet = true;
                }
            }
        }
    }
    if (notify) {
        setReal("aipVsDataMin", vsMin, true);
        setReal("aipVsDataMax", vsMax, true);
        setString("aipVsFunction", vsCmd, true);
    }
    setReal("aipVsDifferent", (vsDifferent ? 1 : 0), true);
    //char vsInfo[MAXSTR];
    //sprintf(vsInfo,"vsRange %g,%g", vsMin, vsMax);
    //setString("aipVsInfo",  vsInfo, true);
    return vsDifferent;
}

/*STATIC*/
Histogram*
VsInfo::makeComboHistogram(set<string> keys)
{
    // Get histograms for individual images
    std::list<Histogram *> histList;
    set<string>::iterator keyItr;
    GframeManager *gfm = GframeManager::get();
    for (keyItr = keys.begin(); keyItr != keys.end(); ++keyItr) {
        spGframe_t gf = gfm->getCachedFrame(*keyItr);
        if (gf != nullFrame) {
            spViewInfo_t view = gf->getSelView();
            if (view  != nullView) {
                Histogram *ph = view->getHistogram();
                if (ph != NULL) {
                    histList.push_back(ph);
                }
            }
        }
    }

    Histogram *histogram;
    int nhist = histList.size();
    if (nhist <= 0) {
        return NULL;
    } else {
        // Make a combined histogram
        std::list<Histogram *>::iterator ppHist;
        ppHist = histList.begin();
        Histogram *pHist = *ppHist;

        // Set limits of output histogram
        float min = pHist->bottom;
        float max = pHist->top;
        for (ppHist++; ppHist != histList.end(); ppHist++) {
            pHist = *ppHist;
            if (min > pHist->bottom) {
                min = pHist->bottom;
            }
            if (max < pHist->top) {
                max = pHist->top;
            }
        }

        // Fill the new histogram
        histogram = new Histogram(100);
        int *obuf = histogram->counts;
        int obins = histogram->nbins;
        histogram->bottom = min;
        histogram->top = max;
        if (max == min) {
            histogram->counts[obins/2] = 1;
        } else {
            for (ppHist = histList.begin();
                 ppHist != histList.end();
                 ppHist++)
            {
                pHist = *ppHist;
                double stx = obins * (pHist->bottom - min) / (max - min);
                double wd = obins * (pHist->top - pHist->bottom) / (max - min);
                boxcarRebinSum(pHist->counts, pHist->nbins, 1,
                               stx, wd, obuf, obins, 1);
            }
        }
    }
    return histogram;
}

/*STATIC*/
bool
VsInfo::setVsHistogram(Gframe *mainFrame)
{
    static int firstTime = true;
    static int id = 0;
    if (firstTime) {
        firstTime = false;
	deleteOldFiles(baseFilename);
    }
        
    // Make a list of histograms we're interested in
    std::list<Histogram *> histList;
    spGframe_t gf;
    spViewInfo_t view = nullView;

    double vsMin = getReal("aipVsDataMin", 0);
    double vsMax = getReal("aipVsDataMax", 0.1);
    std::string vsCommand = getString("aipVsFunction", "y");
    /* bool vsDifferent = */ setVsDifference(vsMin, vsMax, vsCommand, false);
    if (mainFrame != NULL && (view = mainFrame->getSelView()) != nullView) {
        vsMin = view->imgInfo->getVsMin();
        vsMax = view->imgInfo->getVsMax();
        vsCommand = view->imgInfo->getVsCommand();
    }
    setString("aipVsFunction", vsCommand, true);
    setReal("aipVsDataMin", vsMin, true);
    setReal("aipVsDataMax", vsMax, true);

    /*
     * TODO: Combining all the histograms can take a while, so maybe
     * we should not do it unless the VS window is showing.
     */

    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    //int vsMode = (int)getReal("aipVsBind", 0);
    int vsMode = getVsMode();
    gf = gfm->getFrameByNumber(1);
    if (gf != nullFrame && gf->getViewCount() > 1 && (view = gf->getSelView()) != nullView) {
	// first frame has multiple images
        Histogram *ph = view->getHistogram();
        if (ph != NULL) {
           histList.push_back(ph);
        }
    } else if (vsMode == 2 || gfm->getFirstSelectedFrame(gfi) == nullFrame) {
        // Look at all frames
        for (gf=gfm->getFirstFrame(gfi);
             gf != nullFrame;
             gf=gfm->getNextFrame(gfi))
        {
            if ((view = gf->getSelView()) != nullView) {
                Histogram *ph = view->getHistogram();
                if (ph != NULL) {
                    histList.push_back(ph);
                }
            }
        }
    } else {
        // Look only at selected frames
        for (gf=gfm->getFirstSelectedFrame(gfi);
             gf != nullFrame;
             gf=gfm->getNextSelectedFrame(gfi))
        {
            if ((view = gf->getSelView()) != nullView) {
                Histogram *ph = view->getHistogram();
                if (ph != NULL) {
                    histList.push_back(ph);
                }
            }
        }
    }

    Histogram *histogram;
    int nhist = histList.size();
    if (nhist <= 0) {
        return false;
    } else {
        // Make a combined histogram
        std::list<Histogram *>::iterator ppHist;
        ppHist = histList.begin();
        Histogram *pHist = *ppHist;
        float min = pHist->bottom;
        float max = pHist->top;
        for (ppHist++; ppHist != histList.end(); ppHist++) {
            pHist = *ppHist;
            if (min > pHist->bottom) {
                min = pHist->bottom;
            }
            if (max < pHist->top) {
                max = pHist->top;
            }
        }
        histogram = new Histogram(100);
        if (histogram == NULL) {
            return false;
        }
        int *obuf = histogram->counts;
        int obins = histogram->nbins;
        if (min > vsMin) {
            min = vsMin;
        }
        if (max < vsMax) {
            max = vsMax;
        }
        histogram->bottom = min;
        histogram->top = max;
        if (max == min) {
            histogram->counts[obins/2] = 1;
        } else {
            for (ppHist = histList.begin();
                 ppHist != histList.end();
                 ppHist++)
            {
                pHist = *ppHist;
                double stx = obins * (pHist->bottom - min) / (max - min);
                double wd = obins * (pHist->top - pHist->bottom) / (max - min);
                boxcarRebinSum(pHist->counts, pHist->nbins, 1,
                               stx, wd, obuf, obins, 1);
            }
        }
    }

    // Open file
    char fname[128];
    sprintf(fname,"%s%d", baseFilename.c_str(), getpid());
    int oldUmask = umask(0);
    FILE *fd = fopen(fname, "w");
    umask(oldUmask);
    if (!fd) {
	fprintf(stderr,"Cannot open file for VS histogram: %s\n", fname);
        delete histogram;
	return false;
    }

    // Write out header
    fprintf(fd,"Histogram %d\n", histogram->nbins);
    fprintf(fd,"ID %d\n", ++id);
    //fprintf(fd,"VsRange %g %g\n", vsMin, vsMax);
    //fprintf(fd,"VsDifferent %s\n", (vsDifferent ? "true" : "false"));
    fprintf(fd,"XMin %g\n", histogram->bottom);
    fprintf(fd,"XMax %g\n", histogram->top);
    fprintf(fd,"YMin 0\n");
    fprintf(fd,"XLabel Data Value\n");
    fprintf(fd,"YLabel Number of pixels\n");
    fprintf(fd,"Data\n");

    // Write out the data
    for (int i=0; i < histogram->nbins; i++) {
	fprintf(fd,"%d\n", histogram->counts[i]);
    }

    fclose(fd);

    // Set parameters pointing to file
    setString("aipVsHistFile", fname, true);// Send notification of change

    delete histogram;
    return true;
}

/* STATIC */
void
VsInfo::setDefaultPalette(palette_t *pal)
{
    palette = pal;
}

void
VsInfo::setPaletteList(palette_t *list)
{
    paletteList = list;
}

/* STATIC */
int
VsInfo::getVsMode()
{
    string smode = getString("aipVsMode", "individual");
    if (smode == "individual") {
        return VS_INDIVIDUAL;
    } else if (smode == "uniform") {
        return VS_UNIFORM;
    } else if (smode == "header") {
        return VS_HEADER;
    } else if (smode == "operate") {
        return VS_OPERATE;
    } else if (smode == "groups") {
        return VS_GROUP;
    } else if (smode == "selected frames") {
        return VS_SELECTEDFRAMES;
    } else if (smode == "displayed") {
        return VS_DISPLAYED;
    } else {
        return VS_NONE;
    }
}

/**
* Determine an appropriate vertical scaling given a histogram of the
* data. This means finding appropriate minimum and maximum data values,
* which map to the beginning and end, respectively, of the image's
* colormap. Normally, the very highest and lowest points are ignored
* for purposes of scaling. The fraction of points to ignore at each
* end of the intensity distribution is given by the parameter
* "aipVsTailPercentile", or defaults to 0.1 (0.1%).
* If the minimum data value determined is near zero, it is made exactly zero.
* "Near zero" means positive and less than 5% of the maximum level found.
*
* STATIC
*/
spVsInfo_t
VsInfo::getVsFromHistogram(Histogram *hist, bool phaseImage)
{
    if (hist == NULL || hist->top <= hist->bottom || hist->nbins < 1) {
        return spVsInfo_t(new VsInfo(""));
    }
    int i;
    int n;
    double percentile = getReal("aipVsTailPercentile", 0.1);
    // This is percent, we need a fraction
    percentile = percentile/100.0;
    int *buf = hist->counts;
    int nbins = hist->nbins;
    int npts = 0;
    for (int i = 0; i < nbins; ++i) {
        npts += buf[i];
    }
    // Get the scale factor for converting histogram index to intensity
    double scale = (nbins - 1) / (hist->top - hist->bottom);

    // Number of data points to ignore in the tails
    int nPixels = (int) (percentile * npts);
    if(nPixels < 1)
        nPixels = 1;

    // Determine length of bottom tail
    for (i = n = 0; n < nPixels && i < nbins; n += buf[i++]);
    double minVs = hist->bottom + (i - 1) / scale;

    // Determine length of top tail
    for (i = nbins, n = 0; n < nPixels && i > 0; n += buf[--i]);
    double maxVs = hist->bottom + i / scale;

    // Clean up bottom limit, if appropriate
    if (minVs > 0 && minVs / maxVs < 0.05){
        minVs = 0;
    }

    // If data is pos/neg (phase images), then put 0 in the middle and
    // use the max excursion from 0 for the max and min
    if(phaseImage && minVs < 0.0) {
        if(-minVs > maxVs)
            maxVs = -minVs;
        else
            minVs = -maxVs;
    } else if (minVs < 0.0) minVs = 0;
    
    spVsInfo_t vsInfo = spVsInfo_t(new VsInfo(minVs, maxVs));
    return vsInfo;
}

/* STATIC */
void
VsInfo::autoVs(string key)
{
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf = gfm->getCachedFrame(key);
    if (gf != nullFrame) {
        spImgInfo_t img = gf->getSelImage();
        if (img  != nullImg) {
            img->autoVscale();
        }
        if (gfm->isFrameDisplayed(gf)) {
            gf->draw();
        }
    }
}

/* STATIC */
void
VsInfo::autoVsGroup(set<string> keys)
{
    // determine whether is phaseImage
    bool phaseImage = false;
    GframeManager *gfm = GframeManager::get();
    set<string>::iterator keyItr;
    keyItr = keys.begin();
    if(keyItr != keys.end()) {
       spGframe_t gf = gfm->getCachedFrame(*keyItr);
       if (gf != nullFrame) {
        spImgInfo_t img = gf->getSelImage();
	if (img  != nullImg) {
    	   phaseImage = (img->getDataInfo()->getString("type","absval") != "absval");
	}
       }
    }

    // Calculate the VS for this group
    Histogram *hist = makeComboHistogram(keys);
    spVsInfo_t vsInfo = getVsFromHistogram(hist, phaseImage);
    delete hist;

    // Set VS for everyone in the group
    for (keyItr = keys.begin(); keyItr != keys.end(); ++keyItr) {
        spGframe_t gf = gfm->getCachedFrame(*keyItr);
        if (gf != nullFrame) {
            spImgInfo_t img = gf->getSelImage();
            if (img  != nullImg) {
                // This makes a copy of vsInfo
                img->setVsInfo(vsInfo); // TODO: Let all point to one VsInfo
            }
            if (gfm->isFrameDisplayed(gf)) {
                gf->draw();
            }
        }
    }
    
}

/* STATIC */
void
VsInfo::setVsFromCommand(set<string> keys, string vscmd)
{
    string mykey;
    spGframe_t gf;
    spImgInfo_t img;
    GframeManager *gfm = GframeManager::get();
    spVsInfo_t vsi;
    double min, max;

    set<string>::iterator itr;
    // make VsInfo for the first img
    for (itr = keys.begin(); itr != keys.end(); ++itr) {
            gf = gfm->getCachedFrame(*itr);
            if (gf != nullFrame) {
		mykey = *itr;
		img = gf->getSelImage();
		if(img  != nullImg) {
                   img->setVsInfo(vscmd);
		   vsi = img->getVsInfo();
		   min = img->getVsMin();
		   max = img->getVsMax();
                   gf->imgBackupOod = true;
                   if (gfm->isFrameDisplayed(gf)) {
                    gf->draw();
		   }
		   break;
                }
	    }
    }
    // set VS for other keys in the list
    for (itr = keys.begin(); itr != keys.end(); ++itr) {
	if(*itr != mykey) {
            gf = gfm->getCachedFrame(*itr);
            if (gf != nullFrame) {
                img = gf->getSelImage();
		if (img  != nullImg) {
                   img->setVsInfo(vsi);
		}
                gf->imgBackupOod = true;
                if (gfm->isFrameDisplayed(gf)) {
                    gf->draw();
                }
            }
	}
    }
}

/* STATIC */
void
VsInfo::setVsFromCommand(string key, string vsCmd)
{
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf = gfm->getCachedFrame(key);
    if (gf != nullFrame) {
        spImgInfo_t img = gf->getSelImage();
        if (img  != nullImg) {
            img->setVsInfo(vsCmd);
        }
        if (gfm->isFrameDisplayed(gf)) {
            gf->draw();
        }
    }
}

/* STATIC */
string
VsInfo::getDefaultVsCommand()
{
    string defaultCmd = getString("aipVsFunction", "");
    //if (defaultCmd.length() == 0) {
    //    defaultCmd = "curve .5 .5 imin 0 imax 1 ";
    //}
    return defaultCmd;
}

/* STATIC */
void
VsInfo::useDefaultVs(string key)
{
    string defaultCmd = getDefaultVsCommand();
    setVsFromCommand(key, defaultCmd);
}

std::string
VsInfo::getCommand()
{
    string rtn = "";
    if (command.length() > 0) {
/*
        if (command.find("dmin") == string::npos) {
            rtn = command + " " + getRangeString();
        } else {
            rtn = command;
        }
*/
       rtn = command.substr(0,command.find("dmin")) + getRangeString();
    }
    string::size_type idx;
    while ((idx = rtn.find("  ")) != string::npos) {
        // Found 2 spaces - remove one
        rtn.erase(idx, 1);
    }
    return rtn;
}

std::string
VsInfo::getRangeString()
{
    static char cbuf[100];

    sprintf(cbuf," dmin %g dmax %g", minData, maxData);
    return cbuf;
}

bool
VsInfo::makeTableFromCommand(string cmd)
{

    const double bmax = 10.0;
    const double amax = 1000.0;
    int i;
    
    double x1 = 0.5;            // Control point
    double y1 = 0.5;
    double y0 = 0;              // Intensity limits
    double y2 = 1;
    minData = 0;
    maxData = 0.01;

    /*
     * NB: If the command is "", we construct a reasonable default VS
     * but leave command set to "", indicating that the image is not
     * really ready for displaying.
     */
    command = cmd;

    // Decode command into ControlPoint, IntensityLimits, and DataLimits
    char *pcCmd;
    pcCmd = strdup(cmd.c_str());
    char *tok;
    char *pc = pcCmd;           // Changed to NULL after strtok is initialized
    while ( (tok = strtok(pc, " ")) ) {
        pc = NULL;
        if (strcmp(tok, "curve") == 0) {
            if ( (tok = strtok(NULL, " ")) ) {
                x1 = atof(tok);
            }
            if ( (tok = strtok(NULL, " ")) ) {
                y1 = atof(tok);
            }
        } else if (strcmp(tok, "imin") == 0) {
            if ( (tok = strtok(NULL, " ")) ) {
                y0 = atof(tok);
            }
        } else if (strcmp(tok, "imax") == 0) {
            if ( (tok = strtok(NULL, " ")) ) {
                y2 = atof(tok);
            }
        } else if (strcmp(tok, "dmin") == 0) {
            if ( (tok = strtok(NULL, " ")) ) {
                minData = atof(tok);
            }
        } else if (strcmp(tok, "dmax") == 0) {
            if ( (tok = strtok(NULL, " ")) ) {
                maxData = atof(tok);
            }
        }
    }
    free(pcCmd);
    //pc = new char[256];
    //sprintf(pc,"curve %f %f imin %g imax %g", x1, y1, y0, y2);
    //command = pc;
    //delete pc;

    // Note that the action depends on which "quadrant" the control
    // point is in.  "Quadrants" are divided like this:
    //
    // 	 +--------------------+
    //	 |\                 / |
    //	 |  \      3      /   |
    //	 |    \         /     |
    //	 |      \     /       |
    //	 |  2     \ /    4    |
    //	 |        / \         |
    //	 |      /     \       |
    //	 |    /         \     |
    //	 |  /      1      \   |
    //	 |/     	    \ |
    //	 +--------------------+
    int quadrant;
    if (x1 + y1 <= 1) {
        if (x1 <= y1) {
            quadrant = 2;		// Left quadrant
            double tmp = x1;
            x1 = y1;
            y1 = tmp;
        } else {
            quadrant = 1;		// Bottom quadrant
        }
    } else {
        if (x1 < y1) {
            quadrant = 3;		// Top quadrant
            x1 = 1 - x1;
            y1 = 1 - y1;
        } else {
            quadrant = 4;		// Right quadrant
            double tmp = x1;
            x1 = 1 - y1;
            y1 = 1 - tmp;
        }
    }

    // Determine curve parameters
    double a;
    double b;
    if (x1 == 1 || y1 == 0) {
        a = amax;
        b = bmax;
    } else {
        double d1 = x1 > 0.5 ? 1 - x1 : x1;
        b = d1 / y1;
        if (y1 < d1 * pow(2, 1.0 - bmax)) {
            b = bmax;
        } else {
            b = log(2 * d1 / y1) / log(2.0);
        }
        a = (x1 * pow(y1, -1/b) - x1) / (1 - x1);
    }

    if (localPalette == NULL)
       localPalette = palette;
    // Construct lookup table
    double icol = localPalette->firstColor;
    double fcol = icol + localPalette->numColors - 1;
    double ncol1 = fcol - icol;
    if (false) {
        // Not used for now: negative image
        icol = fcol - ncol1 * y0;
        fcol = fcol - ncol1 * y2;
    } else {
        fcol = icol + ncol1 * y2;
        icol = icol + ncol1 * y0;
    }
    double ncol = fcol - icol + 1;
    if (ncol < 2) {
        ncol = 2;
    }

    int newsize = (x1 == y1) ? (int) ncol : 1024;
    if (table == NULL || newsize > size) {
        size = newsize;
        if (table != NULL)
            delete[] table;
        table = new u_char[size];
    }
    for (i=0, x1=0; i<size; i++, x1 += 1.0/(size-1)) {
        if (x1 == 0) {
            y1 = 0;
        } else if (x1 == 1) {
            y1 = 1;
        } else {
            switch (quadrant) {
            case 2:
                y1 = a * pow(x1, 1/b)
			/ (1 + pow(x1, 1/b) * (a - 1));
                break;
            case 1:
                y1 = pow(x1 / (x1 - a * x1 + a), b);
                break;
            case 4:
                y1 = 1 - (a * pow(1-x1, 1/b)
                          / (1 + pow(1-x1, 1/b) * (a - 1)));
                break;
            case 3:
                y1 = 1 - pow((1-x1) / ((1-x1) - a * (1-x1) + a), b);
                break;
            }
        }
        int j = (int)(y1 * ncol);
        if (j >= ncol) {
            j = (int) ncol - 1;
        }
        table[i] = (u_char) (icol + j);
    }
    return true;
}


void
VsInfo::setColormapID(int n)
{
    if (n < 0 || paletteList == NULL)
        return;
    palette_t *newPalette = &paletteList[n];
    if (localPalette != NULL) {
        if (newPalette->numColors == localPalette->numColors) {
            if (newPalette->firstColor == localPalette->firstColor) {
               localPalette = newPalette;
               return;
            }
        }
    }
    if (table != NULL)
        delete[] table;
    table = NULL;
    localPalette = newPalette;
    setup(minData, maxData); 
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPVSINFO_H
#define AIPVSINFO_H

#include <iostream>
#include <string>
using std::string;
#include <list>
using std::list;
#include <set>
using std::set;

#include <sys/types.h>
#include "sharedPtr.h"

#include "aipCStructs.h"
#include "aipMouse.h"
#include "aipHistogram.h"

#ifndef uchar_t
typedef u_char uchar_t;
#endif

#define VS_NONE 0
#define VS_INDIVIDUAL 1
#define VS_UNIFORM 2
#define VS_HEADER 3
#define VS_OPERATE 4
#define VS_GROUP 5
#define VS_DISPLAYED 6
#define VS_SELECTEDFRAMES 7

class Gframe;
//typedef boost::shared_ptr<Gframe> spGframe_t;
//extern spGframe_t nullFrame;

class VsInfo;
typedef boost::shared_ptr<VsInfo> spVsInfo_t;

class VsInfo
{
    /*
     * VERTICAL SCALING
     *
     * Data values <= minData or >= maxData are displayed as uFlowColor or
     * oFlowColor, respectively.  The range (minData < range < maxData) is
     * divided into "size" bins, with minData at the bottom of the first
     * bin and maxData at the top of the last bin.  The display color for
     * any datum in (range) is found in the lookup table:
     *       color = table[i]
     * where     i = (int)((datum - minData) * scale
     * and   scale = (double)(size / (maxData - minData))
     */
public:
    int size;			/* Length of the lookup table */
    uchar_t *table;		/* Lookup table */
    double minData;		/* Data value at start of lookup table */
    double maxData;		/* Data value at end of lookup table */
    uchar_t uFlowColor;		/* Color of underflowed data */
    uchar_t oFlowColor;		/* Color of overflowed data */
    string command;	/* Command string to produce this vsfunc */
    palette_t *localPalette;		/* Color of overflowed data */

    VsInfo();
    VsInfo(double min, double max);
    VsInfo(double min, double max, int colormap);
    VsInfo(double mindat, double maxdat, std::string path, int& lastId);
    VsInfo(spVsInfo_t old, int cmapSize);
    VsInfo(spVsInfo_t old);
    VsInfo(std::string command);
    VsInfo(std::string command, int colormap);
    ~VsInfo();

    /* VNMR commands */
    static int aipSetVsFunction(int argc, char *argv[], int retc, char *retv[]);
    static int aipSaveVs(int argc, char *argv[], int retc, char *retv[]);

    static string getVsCmdFromFile(string path, int& lastId);
    static spVsInfo_t getVsFromHistogram(Histogram *, bool phaseImage=false);
    static Histogram *makeComboHistogram(set<string> keys);
    static bool setVsHistogram(Gframe *mainFrame = NULL);
    static bool setVsDifference();
    static bool setVsDifference(double& vsMin, double& vsMax,
                                std::string& command, bool notify);
    static void setDefaultPalette(palette_t *);
    static void setPaletteList(palette_t *);
    static int getVsMode();
    static void autoVsGroup(set<string> group);
    static void autoVs(string key);
    static void useDefaultVs(string key);
    static string getDefaultVsCommand();
    static void setVsFromCommand(string key, string vsCommand);
    static void setVsFromCommand(set<string> keys, string vsCommand);
    static int setVsFromCommand(string vsCommand);
    static int setVsFromFile(string filepath, int& id);
    static int applyVsMode(int mode);
    void setColormapID(int id);

    std::string getCommand();

private:
    static palette_t *palette;	// The default color palette to use
    static palette_t *paletteList;
    static const string baseFilename;

    void init(spVsInfo_t old, int cmapSize);
    void setup(double min, double max);
    std::string getRangeString();
    bool makeTableFromCommand(string command);
};

extern spVsInfo_t nullVs;

#endif /* AIPVSINFO_H */

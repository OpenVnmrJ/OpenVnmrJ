/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef AIPVOLDATA_H
#define AIPVOLDATA_H

#include "aipDataInfo.h"
#include "mfileObj.h"


// Types of clip regions that may be set
typedef enum {
    FRONT_PLANE,
    TOP_PLANE,
    SIDE_PLANE
} OrientationType;


// Class used to create volumn data slice extraction service
class VolData
{
  public:
    VolData(void);
    ~VolData();
    static VolData *get();
    static int Extract(int argc, char **argv, int, char **); 
    static int ExtractObl(int argc, char **argv, int, char **);
    static int Mip(int argc, char **argv, int, char **);
    static int show3Planes(int argc, char **argv, int, char **);
    void enableExtractSlicesPanel(bool show);
    void showObliquePlanesPanel(bool show);
    void setVolImagePath(const char *newPath);
    bool validVolImagePath();
    bool validData();
    char *getVolImagePath();
    spDataInfo_t dataInfo;
    int getIntArgs(int argc, char **argv, int *values, int nvals);
    void extract_planes(int orientation, int first, int last, int incr);
    void extract_oblplanes(float *angles, float *rots, int nslices, int *slicelist, int mipflag, int mag_frame);
    bool showingObliquePlanesPanel();
    void setMatrixLimits();
    int extractRotations[3];
    
    bool validMapFile();
    bool setMapFile(const char *newPath);
    bool volDataIsMapped();
    void setDfltMapFile();
    char *getMapFile();
    char *mapVolData(DDLSymbolTable *st);
    char *setVolData(DDLSymbolTable *st);
    void freeMapData();
    void freeData();
    void setOverlayFlg(bool b);
    bool getOverlayFlg();
   
  private:
    static VolData volData;  // support one instance
    char volImagePath[PATH_MAX];
    char volMapFile[PATH_MAX];
    bool data_valid;
    bool overlayFlg;
    MFILE_ID mobj;
    void extract_mip(int orientation, int first, int last, int incr);
    int clip_slice(int slice_orient, int slice);
    void extract_plane(int orientation, int nslices, int *slicelist, int first = 0, int last = 0);
    void extract_plane_header( DDLSymbolTable *, int , int, int *);
    void extract_plane_header_obl( DDLSymbolTable *, int , int, int *, double *);
};

#endif /*AIPVOLDATA_H */

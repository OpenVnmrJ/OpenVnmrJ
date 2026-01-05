/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPDATAINFO_H
#define AIPDATAINFO_H

#include <string>
using std::string;

#include "sharedPtr.h"

#include "ddlSymbol.h"
#include "aipStructs.h"
#include "aipDataStruct.h"
#include "aipHistogram.h"
//#include "aipRoi.h"

class Roi;
class CoordList;
typedef boost::shared_ptr<CoordList> spCoordVector_t;
typedef boost::shared_ptr<Gframe> spGframe_t;

typedef struct roiInfo {
   string name;
   spCoordVector_t data;
   bool loaded;
   bool selected;
   bool active;
} roiInfo_t;

typedef std::list<roiInfo_t> RoiInfoList;

class DataInfo
{
public:
    dataStruct_t *dataStruct;
    DDLSymbolTable *st;		// Contains the DDL symbol table
    DDLSymbolTable *st2;        // Auxiliary DDL symbol table
    double d2m[3][3];		// Data address to magnet coord conversion
    double b2m[3][3];		// body to magnet coord conversion
    Histogram *histogram;       // Histogram for this data

    DataInfo(dataStruct_t *data, DDLSymbolTable *st, DDLSymbolTable *st2=NULL);
    ~DataInfo();

    float *getData();
    int getFast();
    int getMedium();
    int getSlow();
    double getRatioFast();
    double getRatioMedium();
    double getRatioSlow();
    int getOrientation(double *orientation);
    int getEuler(double *orientation);
    int getLocation(double *location);
    double getLocation(int i);
    int getSpan(double *span, int maxn);
    double getSpan(int i);
    int getSpatialSpan(double *span);
    string getSpatialRank();
    string getAbscissa(int i);
    double getRoi(int i);
    int getRank();
    const char *getFilepath();
    string getKey();
    string getNameKey() {return nameKey;}
    string getShortName() { return shortName;}
    void setShortName(string str) {shortName = str;}
    string getImageNumber() { return imageNumber;}
    void setImageNumber(string str) {imageNumber = str;}

    bool updateScaleFactors();
    void dataToMagnet(double dx, double dy, double &mx, double &my, double &mz);
    bool getMinMax(double& min, double& max);
    D3Dpoint_t p2m(Dpoint_t pixel_posn);
    void rot90_header();
    void flip_header();
    void GetOrientation(double *orientation);
    void get_location(double *x, double *y, double *z);
    void initializeSymTab(int, int, char *, int , int, int, int);
    int getDataNumber() { return relativeIndex; }
    void setDataNumber(int n) { relativeIndex = n; }
    string getGroup();
    bool saveHeader();

    int getInt(const string name, int dflt=0);
    int getIntElement(const string name, int idx, int dflt=0);
    double getDouble(const string name, double dflt=0);
    double getDoubleElement(const string name, int idx, double dflt=0);
    string getString(const string name, string dflt="");
    string getStringElement(const string name, int idx, string dflt="");

    bool setInt(const string name, int value);
    bool setDouble(const string name, double value);
    bool setString(const string name, string value);

    static bool setDataStructureFromSymbolTable(dataStruct_t *ds,
					     DDLSymbolTable *st);
    static string getKey(DDLSymbolTable *st);
    static string getNameKey(DDLSymbolTable *st);

    RoiInfoList *getRoiInfoList() {return roilist;}
    void addRoi(const char *name, spCoordVector_t data, bool loaded, bool selected);
    void loadRoi(spGframe_t gf);
    void setRoiSelected_all(bool b);
    void setRoiSelected(spCoordVector_t data, bool b);
    void setRoiActive_all(bool b);
    void setRoiActive(spCoordVector_t data, bool b);
    void removeAllRois();
    void removeSelectedRois();
    void removeActiveRois();
    void removeRoi(spCoordVector_t data);
    void calcBodyToMagnetRotation();

private:
    static unsigned int nextIndex;

    unsigned int index;        // Order data was loaded in
    int relativeIndex;          // Counting only currently loaded data
    string key;
    string shortName;
    string imageNumber;
    string group;
    string nameKey;

    RoiInfoList *roilist;

    void setGroup();
};

typedef boost::shared_ptr<DataInfo> spDataInfo_t;
extern spDataInfo_t nullData;

#endif /* AIPDATAINFO_H */

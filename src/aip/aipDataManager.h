/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPDATAMANAGER_H
#define AIPDATAMANAGER_H

#include <map>
#include <string>
#include <utility>
#include <list>
using std::list;
#include <set>
using std::set;

#include "aipDataInfo.h"

#define DATA_ALL 1
#define DATA_GROUP 2
#define DATA_OPERATE 3
#define DATA_SELECTED_FRAMES 4
#define DATA_DISPLAYED 6

typedef struct RQdisplayInfo {
   string key;
   string index;
   string groupIndex;
   int copyNumber;
   int frameIndex;
} RQdisplayInfo_t;

typedef std::map<std::string, spDataInfo_t> DataMap;

class DataManager
{
public:
    /* Vnmr commands */
    static int aipDisplay(int argc, char *argv[], int retc, char *retv[]);
    static int aip2Dto3D(int argc, char *argv[], int retc, char *retv[]);
    static int aipLoadFile(int argc, char *argv[], int retc, char *retv[]);
    static int aipLoadDir(int argc, char *argv[], int retc, char *retv[]);
    static int aipDeleteData(int argc, char *argv[], int retc, char *retv[]);
    static int aipGetHeaderParam(int argc, char *argv[], int retc, char *retv[]);
    static int aipGetHeaderString(int argc, char *argv[], int retc, char *retv[]);
    static int aipGetHeaderInt(int argc, char *argv[], int retc, char *retv[]);
    static int aipGetHeaderReal(int argc, char *argv[], int retc, char *retv[]);
    static int aipNumOfImgs(int argc, char *argv[], int retc, char *retv[]);
    static int aipNumOfCopies(int argc, char *argv[], int retc, char *retv[]);
    static int aipLoadImgList(int argc, char *argv[], int retc, char *retv[]);
    static int aipSaveHeaders(int argc, char *argv[], int retc, char *retv[]);
    static int aipGetImgKey(int argc, char *argv[], int retc, char *retv[]);

    static DataManager *get(); // Returns instance; may call constructor.
    void deleteSelectedData();
    string javaFile(const char *path, int *nx, int *ny, int *ns, float *sx, float *sy, float *sz, float **jdata);
    string loadFile(const char *path, bool fast=false); // Returns key - or null
    string loadFile(const char *inpath,
                    DDLSymbolTable *st, DDLSymbolTable *st2=NULL, char *orientStr=NULL);
    void deleteAllData();
    bool displayMovieFrame(const string key, bool fast=false);
    bool displayMovieData(const string key, bool fast=false);
    bool displayData(const string key, bool fast=false);
    bool displayData(const string key, int frameNumber);
    bool displayData(int imgNumber, int frameNumber);
    string displayFile(const char *path, int frameNumber); // Returns key
    void displayAll();
    void aip2Dto3Dwork(char *);
    void create3Dheader(const char *, const char *);
    void displayFirstN();
    void displayBatch(int cmdbits);
    void insert(spDataInfo_t dataInfo);
    int getNumberOfImages();
    spDataInfo_t getDataByNumber(int imgNumber);
    void numberData();
    void makeShortNames();
    double getAspectRatio();
    DataMap * getDataMap();
    void deleteDataByKey(string key);
    bool getHeaderStr(string key, string name, int idx, char **value);
    bool getHeaderInt(string key, string name, int idx, int *value);
    bool getHeaderReal(string key, string name, int idx, double *value);
    spDataInfo_t getDataInfoByKey(string key, bool load=false);
    void addToRQ(const char *path);
    void makeImgList(const char *path, std::set<string>& list);
    void deleteDataInDir(const char *path);
    void deleteDataInList(const char *path);
    int findCopiesInDataMap(const char *path);
    string getFirstSelectedKey();
    std::set<string> getKeys(int mode, string arg = "");
    std::set<string> getGroups();
    int saveAllHeaders();
    int saveAllVs();
    std::list<string> getKeylist(int mode);
    void displaySelected(int mode, int layout, int batch);
    string getHeaderParam(string key, string name, int index = -1);
    void erasePlane(char *orient);

    // TEST STUFF:
    void listAllData();
    bool dataCheck();

private:
    static DataManager *dataManager; // Support one instance

    DataMap *dataMap;

    DataManager();		// Private constructor only

    static std::set<string> selectedSet;

    static const int batchDisplay = 1;
    static const int batchNext = 2;
    static const int batchPrevious = 4;
    static const int batchFirst = 8;
    static const int batchLast = 16;
};

#endif /* AIPDATAMANAGER_H */

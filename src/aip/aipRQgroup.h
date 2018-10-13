/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPRQGROUP_H
#define AIPRQGROUP_H

#include <string>
#include <fstream>
#include <list>
#include "aipRQnode.h" 
#include "aipRQimage.h" 

class RQgroup : public RQnode
{

public:

   RQgroup();
   RQgroup(string str, int firstFrame = 1, bool show = false);
   void make3Dgroup(string str);
   
   static void getRealFromProcpar(string path, string *pars, double *vals, int n);
   static int getDimFromProcpar(string path, int *slices, int *array_dim, int *echoes);

   void setSelection(string str);
   void setImageSelection(string str);
   void setSliceSelection(string str);
   void setEchoSelection(string str);
   void setArraySelection(string str);
   void setFrameSelection(string str);
   void sortImages(string str);
   bool hasKey(string key);
   bool hasImage(string key);

   void updateImageList(string str);
   int updateFrames(string str);
   int getSelSize();
   int getScanSize();
   int getGroupSize();
   int getSlices() {return atoi(getAttribute("ns").c_str());}
   int getEchoes() {return atoi(getAttribute("ne").c_str());}
   int getArray_dim() {return atoi(getAttribute("na").c_str());}
   void updateIndexMap();

   string getStudyPath();
   void setDirAndName(string path);

   int resetChildren();
   int getGnum();

   string getDisplay() {return getAttribute("display");}

   string getDir() {return getAttribute("dir");}
   list<RQnode *> *getImageList() {return &imageList;}
   RQnode *getImageNode(string key);

   void deleteImage(string key);

   int getMaxFrame();

   void makeChildren(const char *path);
   bool canSort(string);
   void makeMaps();
   void sortNodes(string str);
   void makeShortNames();
   void selectByPar(const char *name, int value);
   
   map<int, RQnode *> * getIndexMap() {return &indexMap;}
   map<string, RQnode *> * getDimMap() {return &dimMap;}

/*
protected:
// these has been moved to base class.
   list<RQnode *> imageList;
   map<int, RQnode *> indexMap;
   map<string, RQnode *>  dimMap;
   spDataInfo_t m_dataInfo;

// these are attributes now:
   int rank;
   int matrix[3];
   int tsize;
*/

private:

   void initGroup();
   void initGroup(string str, bool show = false);

   void initSelection(int f = 1);
};

#endif /* AIPRQGROUP_H */

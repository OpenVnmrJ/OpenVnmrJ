/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPREVIEWQUEUE_H
#define AIPREVIEWQUEUE_H

#include <set>
#include "aipRQstudy.h"
#include "aipRQgroup.h"
#include "aipRQimage.h"

#define STUDY 1
#define SCAN 2
#define IMAGE 3

#define DATA_ALL 1
#define DATA_GROUP 2
#define DATA_SELECTED_RQ 3
#define DATA_SELECTED_FRAMES 4
#define DATA_SELECTED_IMAGES 5
#define DATA_DISPLAYED 6
#define DATA_SELECTED 7

class ReviewQueue
{
public:

   static ReviewQueue *get();

   static int aipRQcommand(int argc, char *argv[], int retc, char *retv[]);
   static void printTime(string label);

   int getNodeMapSize();
   void readNodes(const char *path);
   int writeNodes(const char *path);

   RQgroup *getRQgroup(int ind);
   RQgroup *getRQgroup(string aname, string avalue);
   RQnode *getRQnode(string aname, string avalue);
   RQnode *getRQnode(int s, int g, int i);
   RQnode *getRQnode(string nid);

   RQgroup *getGroup(string str);
   RQnode *getNode(string str);
   list<RQnode *> getImages4Node(RQnode *node);
   list<RQnode *> getSelectedFrames();
   

   int getGnum(string gid);

   void deleteData(string str);
   void notifyVJ(const char *action);
   int loadData(string path, string nid ="", int firstFrame = 1, bool show = false);
   int loadFile(string path, string nid ="", int firstFrame = 1, bool show = false);
   int loadDir(string path,  string nid ="", int firstFrame = 1, bool show = false);
   void loadFid(string path);
   void addImage(string key, bool display);
   void addImage(RQimage& image, bool display);
   void addImage(RQimage& image, string nid);
   void addGroups(list<RQgroup>& groups, string nid);
   void removeOldnode(RQstudy *study, string *nid);
   void removeOldnode(RQgroup *group, string *nid);
   void removeOldnode(RQimage *image, string *nid);
   void appendNodes(RQstudy& study);
   void appendNodes(RQgroup& group);
   void appendNodes(RQimage& image);
   string getNextCopy(int type, string path);
   void makeGroups(const char *path, std::list<RQgroup>& groups,
        int firstFrame = 1, bool show = false);
   int getNumGroups();
   int getNumImages();
   void unselectDisplay();
   list<string> getKeylist(int mode = -1, string groupPath="");
   set<string> getKeyset(int mode = -1, string groupPath="");
   string loadKey(string key, string id="");
   string getActiveKey();
   void makeImageList_Group();
   void makeImageList_RQ(int globalSort);
   void makeImageList_RQ(int displayMode, int globalSort, int layoutMode, string selection);
   void makeImageList(int displayMode, int globalSort, int layoutMode, string selection);
   void makeSelections(int globalSort, int layoutMode, string selection);
   string getFirstSelectedImage();
   double getAspectRatio();
   void setSelection(string str);
   void setGroupSelection(int gid, string str);
   void makeGlobalSelection(int globalSort, string str);

   void selectImages(int mode=DATA_SELECTED_RQ);
   void display(int batch=1);
   void displayRQ();
   void displayData(int batch);
   void displayBatch(int cmdbits);
   void displaySel(int mode, int sort = 1, int layoutMode = 1, string = "");
   void displayNode(string str, int sort = 1, int layoutMode = 1, int frame = 1);
   void setGroupAttribute(string nid, string name, string value);
   string getNodeAttribute(string nid, string name);
   string getRQvalue(string name);
   void removeData(string);
   void moveNodes(string, string);
   void copyNodes(string, string);
   string getKey(string);
   int getSelSize() {return m_imageList.size();}
   int getFrameToStart(int f);
   void insertNode(RQstudy&, RQnode *);
   void insertNode(RQgroup&, RQnode *);
   void insertNode(RQimage&, RQnode *);
   void copyNode(RQstudy *, string);
   void copyNode(RQgroup *, string);
   void copyNode(RQimage *, string);
   void moveNode(RQstudy *, string);
   void moveNode(RQgroup *, string);
   void moveNode(RQimage *, string);
   void removeNode(RQstudy *);
   void removeNode(RQgroup *);
   void removeNode(RQimage *);
   RQstudy *getInsertStudy(RQnode *);
   RQgroup *getInsertGroup(RQnode *);
   int getCurrentBatch() {return m_batch;}
   int getBatches() {return m_batches;}
   int getDisplayMode();
   int getGlobalSort();
   int getLayoutMode();
   string getSelection();
   void reloadData(int mode, string sel);
   void moveImage(int x1, int y1, int x2, int y2);
   void copyImage(int x1, int y1, int x2, int y2);
   void removeImage(int x, int y);
   string getShortName(string key, int type);
   string getRQSelection();
   void setDragging(bool b);
   set<string> makeCachedImageSet();
   void updateStudies();
   void removeExtractedNodes();
   void addImagePlane(spDataInfo_t dataInfo, string key, int n, int slice, int first, int last);
   void disImagePlane(spDataInfo_t dataInfo, string key, int n, int slice, int first, int last);
   int getPlaneOrient(string key);
   string getGroupPath(string key);
   string getGroupPath(string dir, string name);
   void displayPlanes();
   void setReconDisplay(int nimages);
   bool hasKey(string key);

protected:

private:

  static ReviewQueue *reviewQueue;

  list<RQstudy> *studies;

  map<string, RQnode *> nodeMap;
  map<int, string> m_imageList;

  // this two are changed by selection made by display, movie, roi
  // other classes.
  list<RQnode *> m_selectedImages;
  list<RQgroup *> m_groups;

  ReviewQueue();

  int m_batches;
  int m_batch;
  bool m_remakeImageList;

  int m_dragging;
};

#endif /* AIPREVIEWQUEUE_H */

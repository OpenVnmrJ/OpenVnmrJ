/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPFRAMEMGR_H
#define ASPFRAMEMGR_H

#include <map>
#include <list>

#include "AspFrame.h"

typedef std::map<int, spAspFrame_t> aspFrameList;

class AspFrameMgr
{
public:

    static int aspFrame(int argc, char *argv[], int retc, char *retv[]);
    static int aspRoi(int argc, char *argv[], int retc, char *retv[]);
    static int aspSession(int argc, char *argv[], int retc, char *retv[]);

    static AspFrameMgr *get();

    void draw();
    void drawFrame(int id);
    void clearFrame(int id);
    void clearAllFrame();
    
    void removeFrames();
    void removeFrame(int id);
    void makeFrames(int n, string type="auto");
    int getNumFrames();
    spAspFrame_t getFrame(int id);
    spAspFrame_t getFrame(AspFrame *frame);
    spAspFrame_t getCurrentFrame();
    int frameFunc(char *keyword, int frameID, int x, int y, int w, int h);

    spAspFrame_t getFirstFrame(aspFrameList::iterator& frameItr);
    spAspFrame_t getNextFrame(aspFrameList::iterator& frameItr);

    int getCurrentFrameID() {return currentFrameID;}

private:

    static AspFrameMgr *aspFrameMgr;

    int currentFrameID;

    aspFrameList *frameList;

    AspFrameMgr();
};

#endif /* ASPFRAMEMGR_H */

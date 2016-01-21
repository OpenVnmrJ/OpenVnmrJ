/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPWINMOVIE_H
#define AIPWINMOVIE_H

#include <map>
#include <string>
#include <utility>

#include "aipDataInfo.h"
#include "aipDataManager.h"

typedef std::pair<int, spDataInfo_t> imgFrame_t;

class WinMovie
{
public:
    XtIntervalId  movieTimerId;
    double mSec;
    static std::list<string> imgList; 
    static std::list<string>::iterator movieIter;
    static WinMovie *winMovie; // Support one instance

    /* Vnmr commands */


    static WinMovie *get(); // Returns instance; may call constructor.
    static int updateList();

    static int startMovie(int argc, char *argv[], int retc, char *retv[]);
    static int continueMovie(int argc, char *argv[], int retc, char *retv[]);
    static int stopMovie(int argc, char *argv[], int retc, char *retv[]);
    static int resetMovie(int argc, char *argv[], int retc, char *retv[]);
    static int stepMovie(int argc, char *argv[], int retc, char *retv[]);
    static void stepMovie(int direction);
    static void dataMapChanged() { movieIteratorOod = true; }
    void movieTimerInit(double timesec);
    void setOwnerFrame(int f, spDataInfo_t data);
    void setGuestFrame(int f, spDataInfo_t data);
    int getOwnerFrame();
    spDataInfo_t getOwnerData();
    int getGuestFrame();
    spDataInfo_t getGuestData();
    void restoreDisplay();
    void resetWinMovie();
    void resetMovieFrame();
    bool movieRunning();
    bool moviePaused();
    bool movieStopped();
    void pauseMovie();
    void resumeMovie();

private:
    static bool movieIteratorOod; // Movie iterator needs to be reinitialized

    WinMovie();		// Private constructor only
    static void movieTimerProc(void *, unsigned long *);
    static int direction;
    static imgFrame_t ownerFrame;
    static imgFrame_t guestFrame;
};

#endif /* AIPWINMOVIE_H */

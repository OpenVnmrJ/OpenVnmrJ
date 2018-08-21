/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIMOVIE_H
#define AIMOVIE_H

#include <map>
#include <string>
#include <utility>

#include "aipDataInfo.h"
#include "aipDataManager.h"

typedef std::pair<int, spDataInfo_t> imgFrame_t;

class Movie
{
public:
    XtIntervalId  movieTimerId;
    double mSec;
    static std::list<string> imgList; 
    static std::list<string>::iterator movieIter;
    static Movie *movie; // Support one instance

    /* Vnmr commands */


    static Movie *get(); // Returns instance; may call constructor.
    static int updateList();

    static int doIt(int argc, char *argv[], int retc, char *retv[]);

    static void dataMapChanged() { movieIteratorOod = true; }
    void movieTimerInit(double timesec);

    void setMovieFrame();
    void restoreDisplay();
    bool movieRunning();
    bool moviePaused();
    bool movieStopped();
    void pauseMovie();
    void stopMovie();
    void continueMovie();
    void next();
    void startMovie(char *str, const char *path);
    void stepMovie(int direction);
    void setMovieSize(const char *str);

private:
    static bool movieIteratorOod; // Movie iterator needs to be reinitialized

    Movie();		// Private constructor only
    static void movieTimerProc(void *, unsigned long *);
    static int direction;
    static int width;
    static int height;
    static int framePerSecond;
    static int savedRows;
    static int savedCols;
    static bool capture;

/*
    static imgFrame_t ownerFrame;
    static imgFrame_t guestFrame;
*/
};

#endif /* AIMOVIE_H */

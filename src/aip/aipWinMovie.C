/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>

#include <iostream>
#include <map>
#include <string>
#include  <sys/time.h>
#include <unistd.h>

using namespace std;

#include "vjXdef.h"
//#include "ddlSymbol.h"
#include "aipCommands.h"
#include "aipDataInfo.h"
#include "aipCFuncs.h"
#include "aipGframeManager.h"
#include "aipDataManager.h"
#include "aipImgInfo.h"
#include "aipMouse.h"
#include "aipVnmrFuncs.h"
#include "aipWinMovie.h"
#include "aipMovie.h"
#include "aipReviewQueue.h"
#include "aipJFuncs.h"

extern "C" {
    int is_vj_ready_for_movie(int reset);
}

using namespace aip;

bool WinMovie::movieIteratorOod = true;
WinMovie *WinMovie::winMovie = NULL;
std::list<string>::iterator WinMovie::movieIter;
std::list<string> WinMovie::imgList;
int WinMovie::direction = 1;
std::pair<int, spDataInfo_t> WinMovie::ownerFrame;
std::pair<int, spDataInfo_t> WinMovie::guestFrame;

/* Class to show images as a movie. */


WinMovie::WinMovie()
{
    movieTimerId = 0;
    ownerFrame = make_pair(-1, (spDataInfo_t)NULL);
    guestFrame = make_pair(-1, (spDataInfo_t)NULL);

    //WinMovie::updateList();
}

int
WinMovie::updateList()
{
    imgList.clear();
    int mode = (int) getReal("aipMovieMode",1);
    ReviewQueue *rq = ReviewQueue::get();
    imgList = rq->getKeylist(mode);

    direction = (int) getReal("aipMovieSettings",3, 1); 
    // Init the static iterator for use. 
    if(imgList.size() > 0) {
	if(direction > 0) WinMovie::movieIter = imgList.begin();
	else {
	     WinMovie::movieIter = imgList.end();
	     WinMovie::movieIter--;
	}
    } else WinMovie::movieIter = imgList.end();
    return imgList.size();
}

/* Keep only one instance of WinMovie.  Allow anyone who needs it to
   get it via this static member call.
*/
WinMovie *WinMovie::get()
{
    if (!winMovie) {
	winMovie = new WinMovie();
    }
    return winMovie;
}

void WinMovie::setOwnerFrame(int f, spDataInfo_t data)
{
    ownerFrame.first = f;
    ownerFrame.second = data;
}

void WinMovie::setGuestFrame(int f, spDataInfo_t data)
{
    guestFrame.first = f;
    guestFrame.second = data;
}

int
WinMovie::getOwnerFrame()
{
    return ownerFrame.first;
}

int
WinMovie::getGuestFrame()
{
    return guestFrame.first;
}

spDataInfo_t 
WinMovie::getOwnerData()
{
    return ownerFrame.second;
}

spDataInfo_t 
WinMovie::getGuestData()
{
    return guestFrame.second;
}

void WinMovie::restoreDisplay() 
{
// display guest frame first.
    DataManager *dm = DataManager::get();
    int f = getGuestFrame();
    spDataInfo_t data = getGuestData();
    if(f > 0 && data != (spDataInfo_t)NULL) {
	dm->displayData(data->getKey(), f-1);
    }

    f = getOwnerFrame();
    data = getOwnerData();
    if(f > 0 && data != (spDataInfo_t)NULL) {
	dm->displayData(data->getKey(), f-1);
    }

    resetMovieFrame();
}

void WinMovie::resetMovieFrame() 
{
    guestFrame.first = -1;
    guestFrame.second = (spDataInfo_t)NULL;
    ownerFrame.first = -1;
    ownerFrame.second = (spDataInfo_t)NULL;
}

/* continue movie with a vnmr command. */
int WinMovie::continueMovie(int argc, char *argv[], int retc, char *retv[])
{
    double d;
    if(!Movie::get()->movieStopped()) {
        Winfoprintf("Error: Movie capture in process.");
        return 1;
    }
    if(argc < 2) {
	fprintf(stderr, "usage: continueMovie frames/sec");
	return 1;
    }

    WinMovie *wm = WinMovie::get();
    if (movieIteratorOod) {
	movieIteratorOod = false;
        wm->updateList();
    }
    if(wm->imgList.size() < 2) {
	movieIteratorOod = true;
	wm->imgList.clear();
	return 1;
    }

    d = atof(argv[1]);
    if(d > 0) wm->mSec = 1000.0 / d;
    else wm->mSec = 1000.0;
    wm->resumeMovie();
    return 0;
}

void  WinMovie::resumeMovie()
{
    // Start the timer
    WinMovie *wm = WinMovie::get();
    is_vj_ready_for_movie(1);
    if(wm->mSec <= 0.5) {
       double d = getReal("aipMovieRate", 1); 
       if(d > 0) wm->mSec = 1000.0 / d;
       else wm->mSec = 1000.0;
    }
    wm->movieTimerInit(wm->mSec);
    
}

/* Start movie with a vnmr command. */
int WinMovie::startMovie(int argc, char *argv[], int retc, char *retv[])
{
    double d;
    if(!Movie::get()->movieStopped()) {
        Winfoprintf("Error: Movie capture in process.");
        return 1;
    }
    if(argc < 2) {
	fprintf(stderr, "usage: startMovie frames/sec");
	return 1;
    }

    WinMovie *wm = WinMovie::get();
    if(wm->updateList() < 2) {
	wm->imgList.clear();
	return 1;
    }
   
    wm->restoreDisplay();

    d = atof(argv[1]);
    if( d > 0) wm->mSec = 1000.0 / d;
    else wm->mSec = 1000.0;
    // Start the timer
    wm->movieTimerInit(wm->mSec);

    return 0;
    
}

/* Stop the movie with a vnmr command. */
int WinMovie::stopMovie(int argc, char *argv[], int retc, char *retv[])
{
    if(!Movie::get()->movieStopped()) {
        Winfoprintf("Error: Movie capture in process.");
        return 1;
    }
    WinMovie::get()->pauseMovie();
    return 0;
}

void WinMovie::pauseMovie()
{
    WinMovie *wm = WinMovie::get();
    if(wm->movieTimerId != 0)
    {
#if defined (MOTIF) && (ORIG)
	XtRemoveTimeOut(wm->movieTimerId);
#else
	aip_removeTimeOut(wm->movieTimerId);
#endif
	wm->movieTimerId = 0;
    }
    wm->mSec = 0.0;
}

/* Reset the movie with a vnmr command. */
int WinMovie::resetMovie(int argc, char *argv[], int retc, char *retv[])
{
    if(!Movie::get()->movieStopped()) {
        Winfoprintf("Error: Movie capture in process.");
        return 1;
    }
   WinMovie::get()->resetWinMovie();
   return 0;
}

void WinMovie::resetWinMovie()
{
    WinMovie *wm = WinMovie::get();
    if(wm->movieTimerId != 0)
    {
#if defined (MOTIF) && (ORIG)
	XtRemoveTimeOut(wm->movieTimerId);
#else
	aip_removeTimeOut(wm->movieTimerId);
#endif
	wm->movieTimerId = 0;
    }
    updateList();
    wm->restoreDisplay();
    wm->mSec = 0.0;
    movieIteratorOod = true;
    GframeManager::get()->clearSelectedList();
}

/* Step movie images forwards or backwards with a vnmr command. */
int WinMovie::stepMovie(int argc, char *argv[], int retc, char *retv[])
{
    if(!Movie::get()->movieStopped()) {
        Winfoprintf("Error: Movie capture in process.");
        return 1;
    }
    if(argc < 2) {
	fprintf(stderr, "usage: stepMovie +/-");
	return 1;
    }

    WinMovie *wm = WinMovie::get();
    if (movieIteratorOod) {
	movieIteratorOod = false;
        wm->updateList();
    }
    if(wm->imgList.size() < 2) {
	wm->imgList.clear();
	movieIteratorOod = true;
	return 1;
    }

    if(argv[1][0] == '-') {
	stepMovie(-1);		// Previous image
    } else {
	stepMovie(1);		// Next image
    }
    return proc_complete;
}

/* STATIC */
void
WinMovie::stepMovie(int direct)
{
    DataManager *dm = DataManager::get();
    DataMap *dataMap = dm->getDataMap();

    if (movieIteratorOod) {
	movieIteratorOod = false;
        if(updateList() < 2) return;
    }
    if (movieIter == imgList.end()) {
	movieIteratorOod = true;
	return;
    }
    if (direct < 0) {
	// Decrement to previous image
	// If at beginning, start at end.  end() puts us one past end
	if(WinMovie::movieIter == imgList.begin()) {
	    WinMovie::movieIter = imgList.end();
	}
 	--WinMovie::movieIter;
    } else {
	// Increment to next image
	++WinMovie::movieIter;
	if(WinMovie::movieIter == imgList.end()) {
	    // If at end, start over
	    WinMovie::movieIter = imgList.begin();
	}
    }
    // Display image in first selected frame
    spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(*movieIter, true);
    if(dataInfo != (spDataInfo_t)NULL) {
	dm->displayMovieData(dataInfo->getKey());
    }
/* 
    DataMap::iterator pd = dataMap->find(*movieIter); 
    if(pd != dataMap->end())
    dm->displayMovieData(pd->first.c_str());
*/
}    

/* Non member function to be passed in as function pointer to timer.
   This fxn is executed at the end of each timer countdown.  It also
   is responsible for restarting the timer. The timer only counts down
   one time each time it is set.
*/
void
WinMovie::movieTimerProc(void *, unsigned long *)
{
    static bool inuse = false;
    int waitCount;

    WinMovie *wm = WinMovie::get();
    DataManager *dm = DataManager::get();

    // Avoid reentry
    if(inuse) {
	// Even if we are still in this function from the last call, we need
	// to restart the timer. 
	wm->movieTimerInit(wm->mSec);
	return;
    }

    if (is_vj_ready_for_movie(0) < 1) {
        waitCount = 0;
        while (waitCount < 3) {
            waitCount++;
            usleep(200000);
            if (is_vj_ready_for_movie(0) > 0)
                break;
        }
    }

    inuse = true;

    // Restart the timer to keep the cycle going.  
    wm->movieTimerInit(wm->mSec);

    DataMap::iterator pd;
    int forward = (int) getReal("aipMovieSettings",3, 1); 

    // Make sure iterator is still valid
    DataMap *dataMap = dm->getDataMap();
    if (movieIteratorOod) {
	movieIteratorOod = false;
        if(updateList() < 2) {
	   inuse = false;
	   return;
        }
    }

    if (movieIter == imgList.end()) {
	movieIteratorOod = true;
	inuse = false;
        wm->resetWinMovie();
	return;
    } else if(forward < 0 && movieIter == imgList.begin()) {
        spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(*movieIter, true);
        if(dataInfo != (spDataInfo_t)NULL) {
	    dm->displayMovieData(dataInfo->getKey(), true);
        }
/*
        pd = dataMap->find(*movieIter); 
        if(pd != dataMap->end())
        dm->displayMovieData(pd->first.c_str(), true);
*/
	if(getReal("aipMovieSettings",1, 1) > 0) {
	   movieIteratorOod = true;
	} else {
	 if(wm->movieTimerId != 0) {
#if defined (MOTIF) && (ORIG)
           XtRemoveTimeOut(wm->movieTimerId);
#else
           aip_removeTimeOut(wm->movieTimerId);
#endif
           wm->movieTimerId = 0;
           wm->resetWinMovie();
	 }
         wm->mSec = 0.0;
        } 
        inuse = false;
	return;
    }

    // Display image in first selected frame
    if(forward == direction) {
       spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(*movieIter, true);
       if(dataInfo != (spDataInfo_t)NULL) {
	    dm->displayMovieData(dataInfo->getKey(), true);
       }
/*
       pd = dataMap->find(*movieIter); 
       if(pd != dataMap->end())
       dm->displayMovieData(pd->first.c_str(), true);
*/
    }

    direction = forward;

    // Increment to next image
    if(forward > 0) { 

      WinMovie::movieIter++;

      if(WinMovie::movieIter == imgList.end()) {
        if(getReal("aipMovieSettings",1, 1) > 0) {
	// If at end, start over
	   WinMovie::movieIter = imgList.begin();
	} else if(wm->movieTimerId != 0) {
#if defined (MOTIF) && (ORIG)
	   XtRemoveTimeOut(wm->movieTimerId);
#else
           aip_removeTimeOut(wm->movieTimerId);
#endif
	   wm->movieTimerId = 0;
           wm->resetWinMovie();
           wm->mSec = 0.0;
	}
      }
    } else {

      WinMovie::movieIter--;
    }

    inuse = false;
}


/* Start the movie timer.  It will timeout one time and call movieTimerProc().
   To keep the cycle going, movieTimerProc() must call this function again
   to start another single countdown.
*/
void WinMovie::movieTimerInit(double ms)
{
    void (* fp)(void *, unsigned long *);

    fp = &movieTimerProc;

    WinMovie *wm = WinMovie::get();
    if(WinMovie::movieTimerId != 0)
    {
#if defined (MOTIF) && (ORIG)
	XtRemoveTimeOut(WinMovie::movieTimerId);
#else
	aip_removeTimeOut(WinMovie::movieTimerId);
#endif
	WinMovie::movieTimerId = 0;
    }

    mSec = ms;  /* milliseconds */
#if defined (MOTIF) && (ORIG)
    WinMovie::movieTimerId = XtAddTimeOut(mSec, fp, NULL);
#else
    WinMovie::movieTimerId = aip_addTimeOut((unsigned long) mSec, (void *)fp, NULL);
#endif
    
}

// movie running  - movieTimerId != NULL && movieIteratorOod == false
// (timer is running)
// movie paused  - movieTimerId == NULL && movieIteratorOod == false
// (timer is not running, but movie is not stopped, i.e. reset)
// movie stopped - movieTimerId == NULL && movieIteratorOod == true
// (timer is not running, movie is rest)
bool WinMovie::movieRunning()
{
    if(movieTimerId != 0 && !movieIteratorOod) return true;
    else return false;
}

bool WinMovie::movieStopped()
{
    if(movieTimerId == 0 && movieIteratorOod) return true;
    else return false;
}

bool WinMovie::moviePaused()
{
    if(movieTimerId == 0 && !movieIteratorOod) return true;
    else return false;
}

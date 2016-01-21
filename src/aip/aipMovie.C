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

using namespace std;

#include "vjXdef.h"
//#include "ddlSymbol.h"
#include "aipGraphicsWin.h"
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

using namespace aip;

extern "C" {
    void aip_movieCmd(char *cmd, const char *message, int x, int y, int w, int h);
}

bool Movie::movieIteratorOod = true;
Movie *Movie::movie = NULL;
std::list<string>::iterator Movie::movieIter;
std::list<string> Movie::imgList;
int Movie::direction = 1;
int Movie::width;
int Movie::height;
int Movie::framePerSecond;
int Movie::savedRows;
int Movie::savedCols;
bool Movie::capture;

/* Class to show images as a movie. */


Movie::Movie()
{
    movieTimerId = 0;
    capture = false;
    width = (int) getReal("aipMovieSettings",4, 400);
    height = (int) getReal("aipMovieSettings",5, 400);
    framePerSecond = (int) getReal("aipMovieRate", 1);

    //Movie::updateList();
}

int
Movie::updateList()
{
    imgList.clear();
    int mode = (int) getReal("aipMovieMode",1);
    ReviewQueue *rq = ReviewQueue::get();
    imgList = rq->getKeylist(mode);

    if(capture) direction = 1;
    else direction = (int) getReal("aipMovieSettings",3, 1); 

    // Init the static iterator for use. 
    if(imgList.size() > 0) {
	if(direction > 0) Movie::movieIter = imgList.begin();
	else {
	     Movie::movieIter = imgList.end();
	     Movie::movieIter--;
	}
    } else Movie::movieIter = imgList.end();

    return imgList.size();
}

/* Keep only one instance of Movie.  Allow anyone who needs it to
   get it via this static member call.
*/
Movie *Movie::get()
{
    if (!movie) {
	movie = new Movie();
    }
    return movie;
}

void Movie::restoreDisplay() 
{
   // char cmd[256];
   Movie::movieIter = imgList.end();
   GframeManager::get()->restoreFrames(savedRows, savedCols);
   int batch = (int)getReal("aipBatch", 1);
   ReviewQueue::get()->displayData(batch);

   if(capture) {
     capture=false;
     // sprintf(cmd,"vnmrjcmd('movie','done')\n");
     // execString(cmd);
     aip_movieCmd("done", NULL,  0, 0, 0, 0);
   }

}

void Movie::setMovieFrame() 
{
   int minSize=50; // pixels
   int minRate=1; // frame pe second
   int maxRate=30; // frame pe second

    // check size and rate 
   if(width < minSize) {
      width = minSize;
      setReal("aipMovieSettings",4,width,true);
   }
   if(height < minSize) {
      height = minSize;
      setReal("aipMovieSettings",5,height,true);
   }
   if(width > getWinWidth()) {
      width = getWinWidth();
      setReal("aipMovieSettings",4,width,true);
   }
   if(height > getWinHeight()) {
      height = getWinHeight();
      setReal("aipMovieSettings",5,height,true);
   }
   if(framePerSecond > maxRate) {
      framePerSecond = maxRate;
      setReal("aipMovieRate",framePerSecond,true);
   }
   if(framePerSecond < minRate) {
      framePerSecond = minRate;
      setReal("aipMovieRate",framePerSecond,true);
   }
 
   savedRows = (int)getReal("aipWindowSplit", 1, 1);
   savedCols = (int)getReal("aipWindowSplit", 2, 1);

    GframeManager *gfm = GframeManager::get();
    spGframe_t gf = gfm->getFrameByNumber(0);
    int nf = gfm->getNumberOfFrames();
    if(gf == nullFrame || nf != 1 || gf->pixwd != width || gf->pixht != height) {
        gfm->setMovieFrame(width, height);
    }
}

int Movie::doIt(int argc, char *argv[], int retc, char *retv[])
{
    if(!WinMovie::get()->movieStopped()) {
	Winfoprintf("Error: Movie is not stopped.");
	return 1;
    }
    if(argc < 2) {
	fprintf(stderr, "usage: aipMovie('start'/'continue'/'stop'/'reset'/'step', ...");
	return 1;
    }

    Movie *wm = Movie::get();

    if(strcasecmp(argv[1],"start") == 0) {
      if(argc > 2) framePerSecond=atoi(argv[2]);
      else framePerSecond = (int) getReal("aipMovieRate", 1);
      if(argc > 3) width=atoi(argv[3]);
      else width = (int) getReal("aipMovieSettings",4, 400);
      if(argc > 4) height=atoi(argv[4]);
      else height = (int) getReal("aipMovieSettings",5, 400);
      wm->startMovie(argv[1],NULL);
    } else if(strcasecmp(argv[1],"capture") == 0) {
      string path;
      if(argc > 2) framePerSecond=atoi(argv[2]);
      else framePerSecond = (int) getReal("aipMovieRate", 1);
      if(argc > 3) width=atoi(argv[3]);
      else width = (int) getReal("aipMovieSettings",4, 400);
      if(argc > 4) height=atoi(argv[4]);
      else height = (int) getReal("aipMovieSettings",5, 400);
      if(argc > 5) path = argv[5];
      else path = getString("aipMoviePath","/tmp/myMovie.mov");
      wm->startMovie(argv[1], path.c_str());
    } else if(strcasecmp(argv[1],"continue") == 0) {
      if(argc > 2) framePerSecond=atoi(argv[2]);
      else framePerSecond = (int) getReal("aipMovieRate", 1);
      if(argc > 3) width=atoi(argv[3]);
      else width = (int) getReal("aipMovieSettings",4, 400);
      if(argc > 4) height=atoi(argv[4]);
      else height = (int) getReal("aipMovieSettings",5, 400);
      wm->continueMovie();
    } else if(strcasecmp(argv[1],"pause") == 0) {
      wm->pauseMovie();
    } else if(strcasecmp(argv[1],"next") == 0) {
      wm->next();
    } else if(strcasecmp(argv[1],"stop") == 0) {
      wm->stopMovie();
    } else if(strcasecmp(argv[1],"cancel") == 0) {
      capture=false;
      wm->stopMovie();
    } else if(strcasecmp(argv[1],"step") == 0) {
      int direct = 1;
      if(argc > 2 && argv[2][0] == '-') direct = -1;
      if(argc > 3) width=atoi(argv[3]);
      else width = (int) getReal("aipMovieSettings",4, 400);
      if(argc > 4) height=atoi(argv[4]);
      else height = (int) getReal("aipMovieSettings",5, 400);
      wm->stepMovie(direct);
    } else if(strcasecmp(argv[1],"setsize") == 0) {
      string str = getString("aipMovieSpec","50");
      if(argc > 2) str = argv[2]; 
      wm->setMovieSize(str.c_str());
    }
    return 0;
}

void  Movie::next()
{
    // char cmd[256];

    if(movieIter == imgList.end()) {
      stopMovie();
      return;
    }

    DataManager *dm = DataManager::get();

    // Display image in first selected frame
    spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(*movieIter, true);
    if(dataInfo != (spDataInfo_t)NULL) {
        dm->displayMovieFrame(dataInfo->getKey());
    }

    ++movieIter;

    // sprintf(cmd,"vnmrjcmd('movie','next')\n");
    //  execString(cmd);
    aip_movieCmd("next", NULL, 0, 0, 0, 0);
}

void  Movie::setMovieSize(const char *str)
{
    if(strcasecmp(str,"WxH") == 0) return;

    double w = getWinWidth();
    double wh = getWinHeight();
    if(strcasecmp(str,"full") == 0) {
      setReal("aipMovieSettings",4,(int)w,true);
      setReal("aipMovieSettings",5,(int)wh,true);
      return;
    }

    double aspec= DataManager::get()->getAspectRatio();
    double h = w * aspec;
    if(h > 0 && h > wh) {
      aspec = wh/h;
      w *= aspec;
      h *= aspec;
    }

    double percent=100;
    if(strcasecmp(str,"max") != 0) {
       percent=atoi(str);	
    }
    percent /= 100;
    w *= percent;
    h *= percent;
    setReal("aipMovieSettings",4,(int)w,true);
    setReal("aipMovieSettings",5,(int)h,true);
}

void  Movie::continueMovie()
{
    double d;

    Movie *wm = Movie::get();
    if (movieIteratorOod) {
	movieIteratorOod = false;
        wm->updateList();
        wm->setMovieFrame();
    }
    if(wm->imgList.size() < 2) {
	movieIteratorOod = true;
	wm->imgList.clear();
        wm->stopMovie();
	return;
    }

    // Start the timer
    d = framePerSecond;
    if(d > 0) wm->mSec = 1000.0 / d;
    else wm->mSec = 1000.0;

    if(wm->mSec <= 0.5) wm->mSec = 0.5;
    
    wm->movieTimerInit(wm->mSec);
}

void Movie::startMovie(char *str, const char *path)
{
    double d;
    // char cmd[256];
  
    if(strcasecmp(str,"capture") == 0) capture=true;

    Movie *wm = Movie::get();
    if(wm->updateList() < 2) {
	wm->imgList.clear();
	return;
    }
     
    if(Movie::movieIter != imgList.begin() && 
       Movie::movieIter != imgList.end()) wm->stopMovie();

    wm->setMovieFrame();

    if(capture && path != NULL) {
      // sprintf(cmd,"vnmrjcmd('movie','start','%s', %d, %d, %d, %d)\n", 
      //    path, width, height, wm->imgList.size(), framePerSecond);
      //  execString(cmd);
       aip_movieCmd("start", path, width, height, wm->imgList.size(), framePerSecond);
    } else {
       d = framePerSecond;
       if( d > 0) wm->mSec = 1000.0 / d;
       else wm->mSec = 1000.0;
       // Start the timer
       wm->movieTimerInit(wm->mSec);

    } 
}

void Movie::pauseMovie()
{
    Movie *wm = Movie::get();
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

void Movie::stopMovie()
{
    Movie *wm = Movie::get();
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
}

void
Movie::stepMovie(int direct)
{
    Movie *wm = Movie::get();
    if (movieIteratorOod) {
	movieIteratorOod = false;
        wm->updateList();
        wm->setMovieFrame();
    }
    if(wm->imgList.size() < 2) {
	wm->imgList.clear();
	movieIteratorOod = true;
        wm->stopMovie();
	return;
    }
    
    DataManager *dm = DataManager::get();

    if (direct < 0) {
	// Decrement to previous image
	// If at beginning, start at end.  end() puts us one past end
	if(Movie::movieIter == imgList.begin()) {
	    Movie::movieIter = imgList.end();
	}
 	--Movie::movieIter;
    } else {
	// Increment to next image
	++Movie::movieIter;
	if(Movie::movieIter == imgList.end()) {
	    // If at end, start over
	    Movie::movieIter = imgList.begin();
	}
    }
    // Display image in first selected frame
    spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(*movieIter, true);
    if(dataInfo != (spDataInfo_t)NULL) {
	dm->displayMovieFrame(dataInfo->getKey());
    }
}    

/* Non member function to be passed in as function pointer to timer.
   This fxn is executed at the end of each timer countdown.  It also
   is responsible for restarting the timer. The timer only counts down
   one time each time it is set.
*/
void
Movie::movieTimerProc(void *, unsigned long *)
{
    char cmd[256];
    static bool inuse = false;

    Movie *wm = Movie::get();
    DataManager *dm = DataManager::get();

    // Avoid reentry
    if(inuse) {
	// Even if we are still in this function from the last call, we need
	// to restart the timer. 
	wm->movieTimerInit(wm->mSec);
	return;
    }

    inuse = true;

    // Restart the timer to keep the cycle going.  
    wm->movieTimerInit(wm->mSec);

    DataMap::iterator pd;
    int forward = (int) getReal("aipMovieSettings",3, 1); 
    if(capture) forward = 1;

    // Make sure iterator is still valid
    // DataMap *dataMap = dm->getDataMap();
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
        wm->stopMovie();
	return;
    } else if(forward < 0 && movieIter == imgList.begin()) {
        spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(*movieIter, true);
        if(dataInfo != (spDataInfo_t)NULL) {
	    dm->displayMovieFrame(dataInfo->getKey(), true);
        }

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
           wm->stopMovie();
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
	    dm->displayMovieFrame(dataInfo->getKey(), true);
       }

    }

    if(capture) {
       sprintf(cmd,"vnmrjcmd('movie','next')\n");
       execString(cmd);
    }

    direction = forward;

    // Increment to next image
    if(forward > 0) { 

      Movie::movieIter++;

      if(Movie::movieIter == imgList.end()) {
        if(!capture && getReal("aipMovieSettings",1, 1) > 0) {
	// If at end, start over
	   Movie::movieIter = imgList.begin();
	} else if(wm->movieTimerId != 0) {
#if defined (MOTIF) && (ORIG)
	   XtRemoveTimeOut(wm->movieTimerId);
#else
           aip_removeTimeOut(wm->movieTimerId);
#endif
	   wm->movieTimerId = 0;
           wm->stopMovie();
           wm->mSec = 0.0;
	}
      }
    } else {

      Movie::movieIter--;
    }

    inuse = false;

}


/* Start the movie timer.  It will timeout one time and call movieTimerProc().
   To keep the cycle going, movieTimerProc() must call this function again
   to start another single countdown.
*/
void Movie::movieTimerInit(double ms)
{
    void (* fp)(void *, unsigned long *);

    fp = &movieTimerProc;

    // Movie *wm = Movie::get();
    if(Movie::movieTimerId != 0)
    {
#if defined (MOTIF) && (ORIG)
	XtRemoveTimeOut(Movie::movieTimerId);
#else
	aip_removeTimeOut(Movie::movieTimerId);
#endif
	Movie::movieTimerId = 0;
    }

    mSec = ms;  /* milliseconds */
#if defined (MOTIF) && (ORIG)
    Movie::movieTimerId = XtAddTimeOut(mSec, fp, NULL);
#else
    Movie::movieTimerId = aip_addTimeOut((unsigned long) mSec, (void *)fp, NULL);
#endif
    
}

// movie running  - movieTimerId != NULL && movieIteratorOod == false
// (timer is running)
// movie paused  - movieTimerId == NULL && movieIteratorOod == false
// (timer is not running, but movie is not stopped, i.e. reset)
// movie stopped - movieTimerId == NULL && movieIteratorOod == true
// (timer is not running, movie is rest)
bool Movie::movieRunning()
{
    if(movieTimerId != 0 && !movieIteratorOod) return true;
    else return false;
}

bool Movie::movieStopped()
{
    if(movieTimerId == 0 && movieIteratorOod) return true;
    else return false;
}

bool Movie::moviePaused()
{
    if(movieTimerId == 0 && !movieIteratorOod) return true;
    else return false;
}

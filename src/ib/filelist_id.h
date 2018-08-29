/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _FILELIST_ID_H
#define _FILELIST_ID_H
/************************************************************************
*									
*
*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/
#include "filelist.h"

#ifdef SVS_IDS

// ID for file-browser used to load/save image data
#define	FILELIST_IMAGE_ID	1

// ID for file-browser used to load/save image data
#define	FILELIST_SPECTRA_ID	2

// ID for file-browser used to save ROI tool's position
#define	FILELIST_MENU_ID	3

// ID for file-browser used to save GFRAME position
#define	FILELIST_GFRAME_ID	4

// ID for file-browser used to save error message 
#define	FILELIST_ERRMSG_ID	5

// ID for file-browser used to save info message 
#define	FILELIST_INFOMSG_ID	6

// ID for file-browser used to save filter's mask
#define	FILELIST_FILTER_ID	7
  
// ID for file-browser used to save GFRAME image saving
#define	FILELIST_GFRAME_IMAGE_ID	8

#else

// ID for file-browser used to load/save spectrum/image data
#define	FILELIST_WIN_ID		1

// ID for file-browser used to save ROI tool's position
#define	FILELIST_MENU_ID	2

// ID for file-browser used to save GFRAME position
#define	FILELIST_GFRAME_ID	3

// ID for file-browser used to save error message 
#define	FILELIST_ERRMSG_ID	4

// ID for file-browser used to save info message 
#define	FILELIST_INFOMSG_ID	5

// ID for file-browser used to save filter's mask
#define	FILELIST_FILTER_ID	6

// ID for file-browser used to recursive load all data files
#define FILELIST_LOAD_ALL_ID    7
  
// ID for file-browser used to find movie frames
#define FILELIST_MOVIE_ID       8
  
// ID for file-browser used to load/save movie playlists
#define FILELIST_PLAYLIST_LOAD_ID	9
#define FILELIST_PLAYLIST_SAVE_ID	10

// ID for file-browser used to save GFRAME image saving
#define	FILELIST_GFRAME_IMAGE_ID	11

#endif
#endif _FILELIST_ID_H

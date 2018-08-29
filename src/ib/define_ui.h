/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* 
 */

#ifndef _DEFINE_UI_H
#define	_DEFINE_UI_H

/************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/

/* This value is used for major id IPG msg */
#define	UI_COMMAND	1

/* This value is used by minor_id at IPG msg with major ID UI_COMMAND */
typedef enum
{
   FILE_LOAD,
   FILE_SAVE,
   FILE_FORMAT,
   FILE_RESTART,
   FILE_EXIT,
   FILE_LOAD_ALL,
   PARA_DISPLAY,
   PARA_SEQUENCE,
   PARA_PLOTTING,
   PARA_GLOBAL,
   VIEW_REDISPLAY,
   VIEW_PROPERTIES,
   PROCESS_STATISTICS,
   PROCESS_ROTATION,
   PROCESS_ARITHMETICS,
   PROCESS_MATH,
   PROCESS_HISTOGRAM,
   PROCESS_FILTERING,
   PROCESS_CURSOR,
   PROCESS_LINE,
   TOOLS_GRAPHICS,
   TOOLS_COLORMAP,
   TOOLS_SLICER,
   MISC_ERR_MSG,
   MISC_ERR_MSG_CLEAR,
   MISC_ERR_MSG_WRITE,
   MISC_ERR_MSG_SHOW,
   MISC_INFO_MSG,
   MISC_INFO_MSG_CLEAR,
   MISC_INFO_MSG_WRITE,
   MISC_INFO_MSG_SHOW,
   MISC_HELP,
   MISC_TIME,
   MOVIE,
   MACRO_EXECUTE,
   MACRO_RECORD,
   MACRO_SHOW,
   MACRO_SAVE,
   CANCEL
} Define_ui;

#define USAGE_MSG "\n\
Usage: %s <macro> <XView args>\n\
\t\t<-image image_filepath>\n\
\t\t<-imagelist list_filepath>\n"

#endif _DEFINE_UI_H

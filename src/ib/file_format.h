/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
/************************************************************************
*									
*  @(#)file_format.h 18.1 03/21/08 (c)1994 SISCO 
*
*************************************************************************/

#ifndef _FILE_FORMAT_H
#define __FILE_FORMAT_H

#include "graphics.h"
#include "gframe.h"
class Fileformat
{
  public:
    Fileformat(void);
    ~Fileformat(void);
    static void show_window();
    int want_fdf();
    void write_data(Gframe *gframe, char *path);
    int data_size();
    char *data_type();

  private:
    Frame frame;		// Parent
    Frame popup;		// Popup frame

    char label[16];		// The basic format type name
    char datatype[16];		// "integer" or "float"
    int datasize;		// 8, 16, or 32
    char conversion_script[1024]; // Converts from base format to target format
    FILE *fd_init;
    Panel_item formattype_widget;
    Panel_item datatype_widget;
    Panel_item datasize_widget;

    void close_init_file();
    static void format_type_callback(Panel_item, int value, Event *);
    static void data_type_callback(Panel_item, int value, Event *);
    static void data_size_callback(Panel_item, int value, Event *);
    int get_next_label(char *buf, int buflen);
    char *get_script(char *infile, char *outfile);
    int open_init_file();
    void set_data_size(int nbits);
    void set_data_type(char *);
};
extern Fileformat *fileformat;

#endif (__FILE_FORMAT_H)

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _LABEL_H
#define _LABEL_H

/************************************************************************
*
*  @(#)label.h 18.1 03/21/08 (c)1991-92 SISCO
*
*************************************************************************
*
*  Ramani Pichumani
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA  94538
*
*************************************************************************/

#include "box.h"

class Label : public Box {

public:
  Label();
  Label(float, float, Gfont_size, char *, Gframe *);
  ~Label();
  ReactionType create(short x, short y, short action = NULL);
  ReactionType create_done(short x, short y, short action);
  int handle_text(char c);
  char *name(void) { return "label"; }
  void draw();
  void drawstring();
  void erasestring();
  void drawlabel();
  void eraselabel();
  ReactionType move_done(short x, short y);
  Roitool *copy(Gframe *);
  ReactionType resize(short x, short y);
  ReactionType resize_done(short x, short y);
  virtual Roitool* recreate() {return new Label;}
  void save(ofstream &);
  void load(ifstream &);
  float *FirstPixel();
  float *NextPixel();
  void rot90_data_coords(int datawidth);
  void flip_data_coords(int datawidth);
  void update_screen_coords();
  Gfont_size font_size;
  Fpoint start_baseline;
  int font_ascent;
  int font_descent;
  static Gfont_size default_font_size;
 protected:
  TextEdit *textedit;
  
};

#endif

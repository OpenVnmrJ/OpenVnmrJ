/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#define MAXCMSIZE 256
typedef unsigned char byte;

typedef struct _dimNode {
    char *name;
    int  x;
    int  y;
    int  w;
    int  h;
    int  ploted;
    int  molFile;
    int  rotateDegree;
    double vx; /* the x on window  */
    double vy; /* the y on window */
    double vw; // the width on window
    double vh; // the height on window
    double rx; // the relative x
    double ry; // the relative y
    double rw;
    double rh;
    double rs; // the ratio of height/width
    double rTop; // the ratio of distance between two tops
    double rBottom; // the ratio of distance between two bottoms
    struct _dimNode *next;
} DIM_NODE;


struct IMGelement {
  char *name;
  char *fpath; /* file path */
  time_t mtime;
  byte *data;  /* image data */
  int  name_len;
  int  id;
  int  width; // image width
  int  height; // image height
  int  offset_x;
  int  offset_y;
  int  x;
  int  y;
  int  transparent;
  int  used;
  int  ploted;
  int  bgColor;
  int  hilit;
  int  hide;
  DIM_NODE *dimNode;
  struct IMGelement *prev;
  struct IMGelement *next;
  unsigned long pixels[MAXCMSIZE];
};

typedef struct IMGelement IMGNode;

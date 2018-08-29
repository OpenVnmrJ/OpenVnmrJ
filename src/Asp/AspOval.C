/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspOval.h"
#include "AspUtil.h"

AspOval::AspOval(char words[MAXWORDNUM][MAXSTR], int nw) : AspBox(words,nw) {
}

AspOval::AspOval(spAspCell_t cell, int x, int y) : AspBox(cell,x,y) {
  created_type = ANNO_OVAL;
  disFlag = ANN_SHOW_ROI;
}


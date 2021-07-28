/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspArrow.h"
#include "AspUtil.h"

AspArrow::AspArrow(char words[MAXWORDNUM][MAXSTR], int nw) : AspLine(words,nw) {
}

AspArrow::AspArrow(spAspCell_t cell, int x, int y, bool trCase) : AspLine(cell,x,y,trCase) {
  created_type = ANNO_ARROW;
  disFlag = ANN_SHOW_ROI;
}

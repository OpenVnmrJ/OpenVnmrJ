/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include "AspRenderedTrace.h"

AspRenderedTrace::AspRenderedTrace(spAspTrace_t tr, double px, double pw, double vx, double vw, double scale, double off) {
  trace = tr;
  pstx=px;
  pwd=pw;
  vstx=vx;
  vwd=vw;
  voff=off;
  vscale=scale;
}

AspRenderedTrace::~AspRenderedTrace() {
}


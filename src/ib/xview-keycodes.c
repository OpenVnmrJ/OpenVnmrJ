/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
#ifndef LINT
static char *Sid(){
    return "@(#)xview-keycodes.c 18.1 03/21/08 (c)1991 SISCO";
}
#endif

#include "xview-keycodes.h"
#include "stringedit.h"

/****************************************************************************
 *
 *  xview_to_ascii(short action)
 *
 *  Returns the ascii equivalent of any xview key event
 *
 ***************************************************************************/
  
     short xview_to_ascii(short action)
{
  switch(action) {

  case CONTROL_A_KEY: return CONTROL_A;
  case CONTROL_B_KEY: return CONTROL_B;
  case CONTROL_E_KEY: return CONTROL_E;
  case CONTROL_F_KEY: return CONTROL_F;
  case CONTROL_N_KEY: return CONTROL_N;
  case CONTROL_P_KEY: return CONTROL_P;
  case CONTROL_U_KEY: return CONTROL_U;
  case CONTROL_W_KEY: return CONTROL_W;
  case CONTROL_SPACE_KEY: return CONTROL_SPACE;
  case ACTION_ERASE_CHAR_BACKWARD: return DEL;
  case DELETE_KEY:        return DEL;
  default: return action;
  }
}

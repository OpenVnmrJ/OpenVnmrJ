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
*  Description								*
*  -----------								*
*									*
*  This file conatins routines related to image math.
*									*
*************************************************************************/

#include <math.h>
#include <ctype.h>
#include "ddllib.h"
#include "stderr.h"
#include "inputwin.h"
#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "common.h"
#include "win_process.h"

extern short xview_to_ascii(short);

// Menu properties
typedef enum
{
   Z_MATH,
   Z_UNMATH
} Math_props_menu;

/************************************************************************
*									*
*  Creator of math-routine.						*
*  (This function can only be called once.)                             *
*									*/
Math_routine::Math_routine(Gdev *)
{
   active = FALSE;

   // Create properties menu for Math
   props_menu =
   xv_create(NULL,          MENU,
	     MENU_GEN_PIN_WINDOW, Gtools::get_gtools_frame(), "Math Props",
	     MENU_ITEM,
	     MENU_STRING,            "No Items",
	     MENU_NOTIFY_PROC,               &Math_routine::menu_handler,
	     MENU_CLIENT_DATA,       Z_UNMATH,
	     NULL,
	     NULL);
}

/************************************************************************
*									*
*  Execute user selcted menu.						*
*									*/
void
Math_routine::menu_handler(Menu, Menu_item i)
{
    Math_props_menu props = (Math_props_menu)xv_get(i, MENU_CLIENT_DATA);
    switch (props){
      case Z_MATH:
	fprintf(stderr,"math\n");
	break;
      case Z_UNMATH:
	//fprintf(stderr,"unmath\n");
	break;
      default:
	break;
    }
}

/************************************************************************
*									*
*  Initialize anything related to math.  It is called when the user just*
*  selects the gtool Math.						*
*									*/
void
Math_routine::start(Panel props, Gtype)
{
    active = TRUE;
    xv_set(props, PANEL_ITEM_MENU, props_menu, NULL);
    Gtools::set_props_label("Math Properties");
}

/************************************************************************
*									*
*  Clean-up routine (about to leave gtool Math).  It is called when    	*
*  the user has selected another tool.                                  *
*                                                                       */
void
Math_routine::end(void)
{
    active = FALSE;
}

/************************************************************************
*                                                                       *
*  Mouse event Graphics-tool: Math.                                    	*
*  This function will be called if there is an event related to Math.	*
*                                                                       */
Routine_type
Math_routine::process(Event *e)
{
    int x = event_x(e);
    int y = event_y(e);
    int frameno;

    switch (event_action(e))   {
      case ACTION_SELECT:
	if (event_is_down(e)) {
	    Gframe *gf = Gframe::get_gframe_with_pos(x, y, &frameno);
	    /*fprintf(stderr,"gf=0x%x, frameno=%d\n", gf, frameno);/*CMP*/
	    char buf[20];
	    sprintf(buf,"%d", frameno);
	    winpro_math_insert(buf, MATH_ISIMAGE);
	}
	break;
	
      default:
	// Check for keyboard events
	int chr;
	if (event_is_down(e) && isascii(chr=xview_to_ascii(event_action(e)))){
	    char buf[] = "x";
	    *buf = (char)chr;
	    winpro_math_insert(buf, MATH_NOTIMAGE);
	}
	break;
	
    }
    return(ROUTINE_DONE);
}


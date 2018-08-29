/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
static char *Sid(){
    return "@(#)label.c 18.1 03/21/08 (c)1991 SISCO";
}

#include "stderr.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "roitool.h"
#include "label.h"

extern Roitool *selected_text;

// Initialize static class members
Gfont_size Label::default_font_size = FONT_MEDIUM;

Label::Label() : Box()
{
    created_type = ROI_TEXT;
    textedit = new TextEdit;
    visibility = VISIBLE_NEVER;
    //visibility = VISIBLE_ALWAYS;	// To see position for debugging
    visible = TRUE;
    resizable = FALSE;
    markable = FALSE;
}

Label::Label(float x0, float y0, Gfont_size font, char *text, Gframe *frame)
: Box()
{
    created_type = ROI_TEXT;
    textedit = new TextEdit;
    visibility = VISIBLE_NEVER;
    //visibility = VISIBLE_ALWAYS;	// To see position for debugging
    visible = TRUE;
    resizable = FALSE;
    markable = FALSE;

    start_baseline.x = x0;
    start_baseline.y = y0;

    // Initialize the text
    textedit->SetText(text);
    textedit->SetCurPos(strlen(text));
    font_size = font;

    // Figure out the corners of the bounding box.  Note that the left
    // end of the baseline is given in data coordinates, but the size
    // is fixed in screen pixels--depending on the string length and
    // font size.
    int width, height, ascent, descent, direction;
    g_get_string_width(gdev, font, text, width, height,
		       ascent, descent, direction);
    font_ascent = ascent;
    font_descent = descent;

    // Need to set the owner frame before coordinate conversion
    // functions will work!
    owner_frame = frame;
    int baseline_x = data_to_xpix(x0);
    int baseline_y = data_to_ypix(y0);
    pnt[0].x = x_min = baseline_x;		// Upper left corner on screen
    pnt[0].y = y_min = baseline_y - ascent;
    pnt[1].x = x_max = baseline_x + width;	// Lower right corner on screen
    pnt[1].y = y_max = baseline_y + descent;
    update_data_coords();
    Roi_routine::AppendObject(this, frame);
}

Label::~Label()
{
    delete textedit;
}

ReactionType
Label::resize(short, short)
{
    return REACTION_NONE;
}

ReactionType
Label::resize_done(short, short)
{
    return REACTION_NONE;
}

ReactionType
Label::move_done(short x, short y)
{
    int width, height, ascent, descent, direction;
    g_get_string_width(gdev, font_size, textedit->GetPrintText(0),
		       width, height, ascent, descent, direction);
    start_baseline.x = xpix_to_data(pnt[0].x);
    start_baseline.y = ypix_to_data(pnt[0].y + ascent);
    return Box::move_done(x,y);
}

/************************************************************************
*                                                                       *
*  Copy this ROI to another Gframe
*                                                                       */
Roitool *
Label::copy(Gframe *gframe)
{
    if (!gframe || !gframe->imginfo){
	return NULL;
    }
    Roitool *tool;
    tool = new Label(start_baseline.x, start_baseline.y, font_size,
		     textedit->GetPrintText(0), gframe);
    return tool;
}

ReactionType
Label::create(short, short, short)
{
    return REACTION_NONE;
}

ReactionType
Label::create_done(short x, short y, short action)
{
    ReactionType retvalue;
    
    int width, height, ascent, descent, direction;
    font_size = default_font_size;
    g_get_string_width(gdev, font_size, "_", width, height,
		       ascent, descent, direction);
    font_ascent = ascent;
    font_descent = descent;
    basex = pnt[0].x = x;
    basey = pnt[0].y = y - ascent;
    x_min = x;
    y_min = y - ascent;
    x_max = x + width;
    y_max = y + descent;
    roi_set_state(ROI_STATE_MARK);
    Box::create(x+width, y+descent, action);
    retvalue = Box::create_done(x+width, y+descent, action);
    start_baseline.x = xpix_to_data(x);
    start_baseline.y = ypix_to_data(y);
    drawlabel();
    selected_text = this;
    return retvalue;
}

void
Label::draw()
{
    Box::draw();
    drawlabel();
}


void
Label::drawstring()
{
    char cursor;
    
    if ( ! visible ){
	return;
    }
    
    if (roi_state(ROI_STATE_MARK)){
	cursor = '_';
    }else{
	cursor = '\0';
    }
    char* tmp = textedit->GetPrintText(cursor);
    
    set_clip_region(FRAME_CLIP_TO_IMAGE);
    g_draw_string(gdev, pnt[0].x, pnt[0].y+font_ascent, font_size, tmp, color);
    roi_set_state(ROI_STATE_EXIST);
    set_clip_region(FRAME_NO_CLIP);
    delete tmp;
}


void
Label::erasestring()
{
    drawstring();
}

void
Label::drawlabel()
{
    char cursor;
    
    if (pnt[0].x == G_INIT_POS){
	return;
    }
    
    if (roi_state(ROI_STATE_MARK)){
	cursor = '_';
    }else{
	cursor = '\0';
    }
    char* tmp = textedit->GetPrintText(cursor);
    int width, height, ascent, descent, direction;
    g_get_string_width(gdev, font_size, tmp, width, height,
		       ascent, descent, direction);
    pnt[1].x = pnt[0].x + width;
    pnt[1].y = pnt[0].y + ascent + descent;
    drawstring();
    
    delete tmp;
}

void
Label::eraselabel()
{
    drawstring();
}

int
Label::handle_text(char c)
{
    int retvalue = textedit->handle(c);
    owner_frame->display_data();
    return retvalue;
}


/************************************************************************
*									*
*  Save the current label in the following format:
*
*     # <comments>
*     label				(The word "label")
*     X Y				(Position of label--left baseline)
*     size				(Font size)
*     This line is the text of the label
*                                                                       *
*									*/
void
Label::save(ofstream &outfile)
{
   outfile << name() << "\n";
   outfile << start_baseline.x << " " << start_baseline.y << "\n";
   outfile << font_size << "\n";
   outfile << textedit->GetPrintText(0) << "\n";
}

/************************************************************************
*									*
*  Load ROI box from a file which has a format described in "save" 	*
*  routine.								*
*									*/
void
Label::load(ifstream &infile)
{
    const int buflen=128;
    char buf[buflen];
    int ndata=0;
    Fpoint temp[2];
    Gfont_size font;
    char *text;

    //
    // Get the location
    //
    while ((ndata != 1) && infile.getline(buf, buflen)){
	if (buf[0] == '#'){
	    continue;
	}else if (strspn(buf, "\t ") == strlen(buf)){	// Ignore blank lines
	    continue;
	}else{
	    if (sscanf(buf,"%f %f", &(temp[ndata].x), &(temp[ndata].y)) != 2){
		msgerr_print("ROI label: Missing location input");
		return;
	    }
	    ndata++;
	}
    }

    if (ndata != 1){
	msgerr_print("ROI label: coordinates of label missing");
	return;
    }

    //
    // Get the font
    //
    ndata = 0;
    while ((ndata != 1) && infile.getline(buf, buflen)){
	if (buf[0] == '#'){
	    continue;
	}else if (strspn(buf, "\t ") == strlen(buf)){	// Ignore blank lines
	    continue;
	}else{
	    if (sscanf(buf,"%d", &font) != 1){
		msgerr_print("ROI label: Missing font input");
		return;
	    }
	    ndata++;
	}
    }

    if (ndata != 1){
	msgerr_print("ROI label: coordinates of label missing");
	return;
    }

    //
    // Get the text
    //
    ndata = 0;
    while ((ndata != 1) && infile.getline(buf, buflen)){
	if (buf[0] == '#'){
	    continue;
	}else if (strspn(buf, "\t ") == strlen(buf)){	// Ignore blank lines
	    continue;
	}else{
	    text = buf;
	    ndata++;
	}
    }

    if (ndata != 1){
	msgerr_print("ROI label: text of label missing");
	return;
    }

    // Create a new label ROI

    // Put it in the appropriate frames
    Gframe *gframe;
    Roitool *tool;
    int i = 1;
    for (gframe=Frame_select::get_selected_frame(i);
	 gframe;
	 gframe = Frame_select::get_selected_frame(++i))
    {
	if (gframe->imginfo){
	    tool = new Label(temp[0].x, temp[0].y, font, text, gframe);
	    tool->select(ROI_NOREFRESH, TRUE);
	    //gframe->display_data();
	}
    }
}

/************************************************************************
*									*
*  These are the default "iterator" functions used by those ROI objects
*  for which no valid histogram can be computed.
*									*/
float *
Label::FirstPixel()
{
    return 0;
}

float *
Label::NextPixel()
{
    return 0;
}

// ************************************************************************
//
// ************************************************************************
void
Label::update_screen_coords()
{
    int baseline_x = data_to_xpix(start_baseline.x);
    int baseline_y = data_to_ypix(start_baseline.y);
    
    int width, height, ascent, descent, direction;
    g_get_string_width(gdev, font_size, textedit->GetPrintText(0),
		       width, height, ascent, descent, direction);

    pnt[0].x = x_min = baseline_x;		// Upper left corner on screen
    pnt[0].y = y_min = baseline_y - ascent;
    pnt[1].x = x_max = baseline_x + width;	// Lower right corner on screen
    pnt[1].y = y_max = baseline_y + descent;
    font_ascent = ascent;
    font_descent = descent;
}

void
Label::rot90_data_coords(int datawidth)
{
    double t = start_baseline.x;
    start_baseline.x = start_baseline.y;
    start_baseline.y = datawidth - t;
}

void
Label::flip_data_coords(int datawidth)
{
    start_baseline.x = datawidth - start_baseline.x;
}

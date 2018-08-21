/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/****************************************************************/
/* banner('message')    - display large-size characters         */
/****************************************************************/

#include <stdio.h>
#include "vnmrsys.h"
#include "graphics.h"
#include <string.h>

/******************************************************************************
*   This program will call /vnmr/bin/banner to display large-size characters  *
*   on the vnmr graphics  window.                                             *
******************************************************************************/

extern int xcharpixels, ycharpixels;
extern int graf_width, graf_height;
extern int scalable_font_size();
extern int Bnmr;
extern int Wissun();
extern int jbanner(char *str);
extern int textWidth(char *str, int type);
extern int colorindex(char *colorname, int *index);
extern void Werrprintf(char *format, ...);
extern void Wsetgraphicsdisplay(char *s);
extern void graphicsOnTop();
extern void Wturnoff_mouse();
extern void Wturnoff_buttons();
extern void popBackingStore();
extern void scaledText(int scale, char *text);
extern void banner_font(double scale, char *font);

#ifdef LINUX
static char  *def_font = "utopia";
#else
static char  *def_font = "times";
#endif

static char spare[512];
static int  scolor = 0;
static int  slen = 0;

int banner(argc, argv)
int argc;
char *argv[];
{
        int  color_no, lines, text_len;
        int  pos_x, pos_y, i, yoffset, org_ch_width;
        int  maxLen, scale, ch_size, ch_height;
        int  n, newBanner;
	double  xscale;
        char str[512];
        char outstr[512];
        char *tokptr, *strptr;
	char *fontName;


        if (argc < 2)
        {
          Werrprintf("usage:  banner('text','color')");
          RETURN;
        }
        if (Bnmr || !Wissun())
        {
          Werrprintf("banner: %s", argv[1]);
          RETURN;
        }

	Wturnoff_buttons();
	Wturnoff_mouse();
	Wsetgraphicsdisplay("banner");
        newBanner = 0;
        text_len = strlen(argv[1]); 
        if (text_len < 1)
            RETURN;
        color_no = PARAM_COLOR;
#ifdef VNMRJ
        if (strcmp(argv[1], "again") == 0) {
            if (slen < 1) {
               strcpy(spare, argv[1]);
               slen = text_len;
               newBanner = 1;
            }
            else {
               color_no = scolor;
            }
        }
        else {
            newBanner = 1;
            if (text_len > 500) {
               text_len = 500;
               strncpy(spare, argv[1], 500);
            }
            else
               strcpy(spare, argv[1]);
            slen = text_len;
        }
        // if (newBanner)
        //     startGraphFunc("banner", 0, 2);
#endif
	fontName = NULL;
        for(i = 2; i < argc; i++)
        {
                if ( colorindex(argv[i], &color_no) == 0)
		      fontName = argv[i];
        }
        setdisplay();
	graphicsOnTop();
        graf_clear();
        color(color_no);
#ifdef VNMRJ
        scolor = color_no;
        if (jbanner(spare)) { /* if java graphics */
           // if (newBanner)
           //   finishGraphFunc();
           popBackingStore();
           RETURN;
        }
#endif
        strcpy (str, argv[1]);
        lines = 0;
        strptr = str;
        if (fontName == NULL)
	    fontName = def_font;
        banner_font(2.0, fontName);
	maxLen = 0;
        while ((tokptr = strtok(strptr, "\\")) != (char *) 0)
        {
            n = (int) strlen(tokptr);
            if (n > 510) {
               n = 510;
               strncpy (outstr, tokptr, n);
               outstr[n] = '\0';
            }
            else
               strcpy (outstr, tokptr);
            strptr = (char *) 0;
            lines++;
            text_len = textWidth(outstr, 0);
 	    if (text_len > maxLen) {
		maxLen = text_len;
            	strcpy (spare, outstr);
            }
        }
/*
#ifdef LINUX
	maxLen = maxLen * 1.3;
#endif
*/
        org_ch_width = xcharpixels;
#ifdef VNMRJ
        xscale = (double)(graf_width - 10) / (double) maxLen;
	if (xscale <= 1.0) {
/*
             banner_font(1.0, fontName); 
*/
	     scale = 1;
	}
	else {
#ifdef LINUX
            ch_size = (int) (xscale * (double) xcharpixels);
#else
            ch_size = (int) (xscale * (double) xcharpixels) + 3;
#endif
            if (ch_size > 120)
                ch_size = 120;
	    while (1)
	    {
	        scale = scalable_font_size(ch_size);
                yoffset = scale * lines - graf_height;
                text_len = textWidth(spare, 0);
#ifdef LINUX
                pos_x = graf_width - text_len - xcharpixels;
#else
                pos_x = graf_width - text_len - 12;
#endif
                if (yoffset < 1 && pos_x > 0)
                    break;
                if (ch_size <= org_ch_width)
                    break;
                yoffset = yoffset / lines;
                if (pos_x < 0) {
                    if (xcharpixels > 50)
                       yoffset = 3;
                    else
                       yoffset = 2;
                }
                else {
                   if (yoffset < 1)
                       yoffset = 1;
                   if (yoffset > 5)
                       yoffset = 5;
                }
                ch_size -= yoffset;
                if (ch_size <= org_ch_width)
                    ch_size = org_ch_width;
            }
            if (scale == 1) /* not scalable font */
                 scale = (int) xscale;
            else
                 scale = 1;
	}
	ch_height = ycharpixels * scale; 
        yoffset = (graf_height - ch_height * lines) / 2;
        if (yoffset < 0)
            yoffset = 0;
        strcpy (str, argv[1]);
        strptr = str;
        pos_y = graf_height - yoffset;
        text_len = textWidth(spare, 0);
        pos_x = (graf_width - text_len * scale) / 2;
	if (pos_x < 0)
	    pos_x = 0;
        while ((tokptr = strtok(strptr, "\\")) != (char *) 0)
        {
            n = (int) strlen(tokptr);
            if (n > 510) {
               n = 510;
               strncpy (outstr, tokptr, n);
               outstr[n] = '\0';
            }
            else
               strcpy (outstr, tokptr);
            strptr = (char *) 0;
            amove(pos_x, pos_y);
	    scaledText(scale, outstr);
            pos_y = pos_y - ch_height;
        }
#else
	if (maxLen < 5)
	     maxLen = graf_width / 20;
        scale = (int) (graf_width / maxLen);
	if (scale < 1)
	{
             banner_font(1.0, fontName); 
	     scale = 1;
	}
	else
	{
	     if (xcharpixels  * scale > 120)
		scale = 120 / xcharpixels;
	}
	ch_height = ycharpixels * scale; 
	while (ch_height * lines > graf_height)
	{
		scale = scale - 1;
		if ( scale < 1)
		{
             	     banner_font(1.0, fontName); 
		     scale = 1;
		     break;
		}
		ch_height = ycharpixels * scale; 
	}
	scale = scalable_font_size(scale);
	ch_height = ycharpixels * scale; 
        yoffset = (graf_height - ch_height * lines) / 2;
        if (yoffset < 0)
            yoffset = 0;
        strcpy (str, argv[1]);
        strptr = str;
        pos_y = graf_height - yoffset;
        text_len = textWidth(spare, 0);
        pos_x = (graf_width - text_len * scale) / 2;
	if (pos_x < 0)
	    pos_x = 0;
        while ((tokptr = strtok(strptr, "\\")) != (char *) 0)
        {
            strcpy (outstr, tokptr);
            strptr = (char *) 0;
            amove(pos_x, pos_y);
	    scaledText(scale, outstr);
            pos_y = pos_y - ch_height;
        }
#endif
        banner_font(1.0, fontName); 
	RETURN;
}

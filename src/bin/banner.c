/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
*/
/***************************************************************************
*  This program will display large-size chatacters on the vnmr graphics    *
*  window.                                                                 *
***************************************************************************/
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <pixrect/pixrect_hs.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <cgipw.h>

unsigned char     redcolor[8]   = {0, 255, 255, 0,   0,   0,   210, 240};
unsigned char     greencolor[8] = {0, 0,   255, 255, 200, 0,   100, 240};
unsigned char     bluecolor[8]  = {0, 0,   0,   0,   255, 255, 255, 240};

char *color_index[] = {
                        "BLACK", "RED", "YELLOW", "GREEN",
                        "CYAN", "BLUE", "MAGENTA", "WHITE"
                      };
char *text_index[] =  {
	                "ROMAN", "GREEK", "SCRIPT", "OLDENG",
                        "STICK", "SYMBOLS", "SOLID"
                      };
int  isColor();
int  isText();

Ccentry   colorlist;

char    str[256];
char    spare[256];
char    outstr[80];
int     lines;
int     color_no;
int     text_no;
int     solid;
float   height, width;
char    *tokptr, *strptr;
struct  pixfont  *font;
struct  pr_size  size;
Pixwin  *px;

main(argc, argv)
int argc;
char *argv[];
{
        Ccgiwin des;
        int    fd;
        int    i, name;
        float  ch_x, ch_y;
	float  space, extent_x, extent_y;
	float  chars;
        Ccoor  pos;
        Ccoor  concat, lleft, uleft, uright;
      
        if (argc < 5)
          return;
        fd = atoi(argv[1]);
        height = atof(argv[2]);
        width = atof(argv[3]);
        strcpy (str, argv[4]);
        text_no = 0;
        solid = 0;
    	lines = 0;
	chars = 0.0;
        strcpy(spare, str);
        px = pw_open(fd);

        color_no = 2;
        for(i = 5; i < 7; i++)
        {
	     if (argv[i] != NULL)
	     {
		 if ( isColor(argv[i]) < 0)
                 {
                      isText(argv[i]);
                 }
             }
        }
        if (px->pw_pixrect->pr_depth < 8)
        {
	    color_no = 2;
        }
        if (solid)
        {
     
            solidText();
            pw_close(px);
            exit(0);
        }
        /*   set up color map  */
        colorlist.ra = redcolor;
        colorlist.ga = greencolor;
        colorlist.ba = bluecolor;
        colorlist.n = 8;
                       
        /*    open cgi graphics package    */
        open_pw_cgi();
        open_cgi_pw(px, &des, &name);
        cgipw_color_table(&des, 0, &colorlist);
        cgipw_text_precision(&des, CHARACTER);
        cgipw_text_font_index(&des, text_no);
        cgipw_text_color(&des, color_no);

        ch_x = 500.0;
        strptr = spare;
        while ((tokptr = strtok(strptr, "\\")) != (char *) 0)
	{
            strcpy (outstr, tokptr);
            strptr = (char *) 0;
            lines++;
            chars = (float) (strlen(tokptr));
            if (ch_x > (width - 40.0) / (chars * 1.1))
            {
                  ch_x = (width - 40.0) / (chars * 1.1);
            }
            for(;;)
            {
                cgipw_character_height(&des, (int)ch_x);
                cgipw_inquire_text_extent(&des, outstr, ' ', &concat,
                                            &lleft, &uleft, &uright);
                if (uright.x >= width) 
                {
		     ch_x = ch_x - 3.0;
                     cgipw_character_height(&des, (int)ch_x);
                }
                else
                     break;
            }
        }
        if (ch_x > 0.7 * height / (lines + 1.0))
        {
           ch_x = 0.7 * height / (lines +1.0);
           cgipw_character_height(&des, (int)ch_x);
        }

        ch_y = height / (lines + 1);
        pos.y = (int) (ch_y + ch_x * 0.3);
        strptr = str;
        while ((tokptr = strtok(strptr, "\\")) != (char *) 0)
	{
            strcpy (outstr, tokptr);
            strptr = (char *) 0;
            cgipw_inquire_text_extent(&des, outstr, ' ', &concat,
                                      &lleft, &uleft, &uright);
            pos.x = (int)(width - uright.x) / 2.0;
            cgipw_text (&des, &pos, outstr);
            pos.y = pos.y + ch_y;
        }
        close_cgi_pw(&des);
        close_pw_cgi();
        pw_close(px);
        exit(0);
}



int isColor(arg)
char  *arg;
{
	int i;

	for(i = 0; i < 8; i++)
        {
	      if (strcmp(color_index[i], arg) == 0)
              {
                   color_no = i;
		   return(i);
              }
        }
        return(-1);
}





int isText(arg)
char  *arg;
{
	int i;

	for(i = 0; i < 7; i++)
	{
	      if (strcmp(text_index[i], arg) == 0)
	      {
                   text_no = i;
                   if (i == 6)
                        solid = 1;
		   return(i);
	      }
        }
        return(-1);
}




solidText()
{
	struct  pixrect  *mem_temp, *mainpix;
	struct  pr_prpos temp; 
	int     scaler,
                length,
                line_len,
		pos_x,
		pos_y,
		sx,
		sy,
                loop,
                count,
                value,
                i,
                Height,
		Width,
                scrn_x,
                scrn_y,
                x_len,
                y_len;
 

        if ((font = pf_open("/usr/lib/fonts/tekfonts/tekfont0")) == NULL)
        {
	     font = pf_default();
        }
        Height = (int) height;
        Width = (int) width;
        mainpix = px->pw_pixrect;
        length = 0;
        lines = 0;
        switch(color_no) {
	case 0:  color_no = 16;
	         break;
	case 1:  color_no = 12; 
	         break;
	case 2:  color_no = 9; 
	         break;
	case 3:  color_no = 7; 
	         break;
	case 4:  color_no = 5; 
	         break;
	case 5:  color_no = 15; 
	         break;
	case 6:  color_no = 3; 
	         break;
	case 7:  color_no = 0; 
	         break;
        }
        if (px->pw_pixrect->pr_depth < 8)
        {
	    color_no = 2;
        }
        strptr = spare;
        while ((tokptr = strtok(strptr, "\\")) != (char *) 0)
	{
            strcpy (outstr, tokptr);
            strptr = (char *) 0;
            lines++;
            line_len = strlen(tokptr);
            size = pf_textwidth(line_len, font, tokptr);
            if (size.x > length)
            {
		length = size.x;
       	    }
	}
        y_len = font->pf_defaultsize.y * 1.2 + 1; 
        scaler = Width / length;
        for(;;)
	{
	    if (Height <= (lines + 1) * y_len * scaler)
            {
		scaler--;
	    }
            else
	    {
		break;
	    }
	}
        scrn_x = px->pw_clipdata->pwcd_screen_x;
        scrn_y = px->pw_clipdata->pwcd_screen_y;
        count = 0;
        strptr = str;
        while ((tokptr = strtok(strptr, "\\")) != (char *) 0)
	{
            strcpy (outstr, tokptr);
            strptr = (char *) 0;
            count++;
            pos_y = count * Height / (lines + 1) - scaler * size.y * 0.8
                    + scrn_y;
            line_len = strlen(outstr);
            size = pf_textwidth(line_len, font, outstr);
            mem_temp = mem_create(size.x, y_len, 8);
            temp.pr = mem_temp;
	    temp.pos.x = 0;
	    temp.pos.y = size.y;
 	    pf_text(temp, PIX_SRC | PIX_COLOR(color_no), font, outstr);
            for (sy = 0; sy < y_len; sy++)
	    {
		for(loop = 0; loop < scaler; loop++)
		{
		     pos_x = (Width - size.x * scaler) / 2 + scrn_x;
		     for (sx  = 0; sx < size.x; sx++)
		     {
			 value = pr_get(mem_temp, sx, sy);
                         if (value != 0)
			 {
                                for(i = 0; i < scaler; i++)
				     pr_put(mainpix, pos_x++, pos_y, value); 
                         }
                         else
			 {
				pos_x = pos_x + scaler;
			 }
		     }
		     pos_y++;
		}
	    }
            pr_close(mem_temp);
      	}
}
 


/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*******************************************************************
* File rasdum:  Contains code for a raster dump function, "rasdum" *
*******************************************************************/

#include "vnmrsys.h"
#include "graphics.h"
#include <stdio.h>
#include <sys/file.h>

#ifdef SUN
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <pixrect/pixrect_hs.h>
#endif

extern Canvas canvas;
extern int mnumxpnts, mnumypnts, right_edge;
#define COLORMAP_SIZE 256

/******************************************************************************/
raster_dump ( argc, argv, retc, retv )
/* 
Purpose:
-------
     Routine raster_dump is the main routine for the raster dump function.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc  :  (I  )  Number of entries in the following argument.
argv  :  (I  )  Array of command line arguments from the interpreter.
retc  :  (I  )  Number of entries in the following array.
retv  :  (   )  Array of returned values from this routine.  Not used.
*/
int argc, retc;
char *argv[], *retv[];

{ /* Begin function raster_dump */
   /*
   Local Variables:
   ---------------
   rasfile   :  The stdio.h file handle for the raster file to write out.
   pw	     :  The pixwin associated with the canvas which is to be rasterized
	        and dumped.
   pr	     :  Memory pixrect containing image data to dump.
   width     :  Width of the image to dump, in pixels.
   height    :  Height of the image to dump, in pixels.
   depth     :  Depth of the image to dump, one or eight.
   red       :  Red portion of colormap.
   green     :  Green portion of colormap.
   blue      :  Blue portion of colormap.
   colormap  :  Color map structure.
   string    :  Used to get a response from the user.
   i	     :  Loop control variable.
   */
   char string[3];
   unsigned char red[COLORMAP_SIZE], green[COLORMAP_SIZE], blue[COLORMAP_SIZE];
   int width, height, depth, i;
   FILE *rasfile;
   Pixwin *pw = canvas_pixwin ( canvas );
   struct pixrect *pr;
   colormap_t colormap;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Refuse service if terminal is not a Sun */
   if ( !Wissun() ) {
      Werrprintf ( "Raster dump routine only available on Sun" );
      ABORT;
   }

   /* Refuse service if terminal is not a color Sun.  In fact, the following  */
   /* code would work, but would dump a color raster file, which is wasteful. */
   if ( !WisSunColor() ) {
      Werrprintf ( "Raster dump only available for color Sun monitors" );
      ABORT;
   }

#ifdef SUN

   /* Look for correct number of input arguments */
   if ( argc != 2 ) {
      Werrprintf ( "Usage:  rasdum('file-name')" );
      ABORT;
   }

   /* If file already exists, get confirmation from user */
   if ( access ( argv[1], F_OK ) == 0 ) {
      W_getInput ( "File already exists.  Overwrite (Y/N)?", string,
                   sizeof(string) );
      if ( string[0] != 'y' && string[0] != 'Y' ) 
         ABORT;
   } /* End if file already exists */

   /* Open the output file */
   if ( ( rasfile = fopen ( argv[1], "w" ) ) < 0 ) {
      Werrprintf ( "Could not open output file %s", argv[1] );
      ABORT;
   }

   /* Determine some sizes necessary to create a temporary memory pixrect */
   /* width = (int)window_get ( canvas, WIN_WIDTH ); */
   /* height = (int)window_get ( canvas, WIN_HEIGHT ); */
   width = mnumxpnts - right_edge;
   if ( width % 2 )
      width--;
   height = mnumypnts;
   /* depth = ( WisSunColor() ? 8 : 1 ); */
   depth = 8;

   /* Get colormap if on color Sun */
   if ( WisSunColor() ) {
      /* The following routine requests the vnmr colormap.  It asks for only */
      /* those entries which really exist.  If we ask for 256 entries, it    */
      /* will silently fail, putting nothing in the color arrays.            */
      /* This sure looks like a bug from Sun.				     */
      if ( pw_getcolormap ( pw, 0, CMS_VNMRSIZE, red, green, blue ) != 0 ) {
	 Werrprintf ( "Could not obtain color map" );
	 fclose ( rasfile );
	 ABORT;
      }
      for ( i = CMS_VNMRSIZE ; i <COLORMAP_SIZE ; i++ ) {
	 red[i]   = 0;
	 green[i] = 0;
	 blue[i]  = 0;
      }
   } else {
      red[0] = green[0] = blue[0] = 0;
      red[255] = green[255] = blue[255] = 255;
   }
   colormap.type = RMT_EQUAL_RGB;
   colormap.length = COLORMAP_SIZE;
   colormap.map[0] = red;
   colormap.map[1] = green;
   colormap.map[2] = blue;

   /* Create memory pixrect to hold data */
   if ( ( pr = mem_create ( width, height, depth ) ) == 0 ) {
      Werrprintf ( "Unable to create memory pixrect" );
      fclose ( rasfile );
      ABORT;
   }

   /* Copy data into memory pixrect */
   if ( pw_read ( pr, 0, 0, width, height, PIX_SRC, pw, 0, 0 ) != 0 ) {
      Werrprintf ( "Unable to copy data into memory pixrect" );
      fclose ( rasfile );
      pr_close ( pr );
      ABORT;
   }

   /* Dump the data, and release resources */
   if ( pr_dump ( pr, rasfile, &colormap, RT_STANDARD, 0 ) != 0 ) {
      Werrprintf ( "Unable to dump data to file" );
      fclose ( rasfile );
      pr_close ( pr );
      ABORT;
   }
   /* map_dump ( pw, colormap.length, red, green, blue ); */
   /* pix_dump ( pw, width, height ); */
   /* self_dump(); */
   if ( pr_close ( pr ) != 0 ) {
      Werrprintf ( "Unable to close memory pixrect" );
      fclose ( rasfile );
      ABORT;
   }
   if ( fclose ( rasfile ) != 0 ) {
      Werrprintf ( "Unable to close raster file" );
      ABORT;
   }

   /* Normal successful return */
   RETURN;

#else SUN

   Werrprintf ( "Programming error, raster dump not on Sun" );
   ABORT;

#endif SUN

} /* End function raster_dump */

/******************************************************************************/
/* 
self_dump()
{
   kill ( getpid(), SIGQUIT );
}
*/
/******************************************************************************/
/* 
map_dump ( pw, size, red, green, blue )
int size;
unsigned char red[], green[], blue[];
Pixwin *pw;
{
   char name[CMS_NAMESIZE];
   int i;
   FILE *spec;

   extern unsigned char mred[CMS_VNMRSIZE];
   extern unsigned char mgrn[CMS_VNMRSIZE];
   extern unsigned char mblu[CMS_VNMRSIZE];

   spec = fopen ( "spec", "w" );

   if ( size == 0 ) {
      fprintf ( spec, "No color map\n" );
   } else {
      pw_getcmsname ( pw, name );
      fprintf ( spec, "Colormap segment name %s", name );
      fprintf ( spec, "\nRed colors:...................................." );
      for ( i = 0 ; i < size ; i++ ) {
	 if ( i % 20 == 0 )  fprintf ( spec, "\n" );
	 fprintf ( spec, "%3d ", (int)red[i] );
      }
      fprintf ( spec, "\nGreen colors:...................................." );
      for ( i = 0 ; i < size ; i++ ) {
	 if ( i % 20 == 0 )  fprintf ( spec, "\n" );
	 fprintf ( spec, "%3d ", (int)green[i] );
      }
      fprintf ( spec, "\nBlue colors:...................................." );
      for ( i = 0 ; i < size ; i++ ) {
	 if ( i % 20 == 0 )  fprintf ( spec, "\n" );
	 fprintf ( spec, "%3d ", (int)blue[i] );
      }
      fprintf ( spec, "\n\n***********************************************\n" );

      fprintf ( spec, "\nMred:...................................." );
      for ( i = 0 ; i < CMS_VNMRSIZE ; i++ ) {
	 if ( i % 20 == 0 )  fprintf ( spec, "\n" );
	 fprintf ( spec, "%3d ", (int)mred[i] );
      }
      fprintf ( spec, "\nMgrn:...................................." );
      for ( i = 0 ; i < CMS_VNMRSIZE ; i++ ) {
	 if ( i % 20 == 0 )  fprintf ( spec, "\n" );
	 fprintf ( spec, "%3d ", (int)mgrn[i] );
      }
      fprintf ( spec, "\nmblu colors:...................................." );
      for ( i = 0 ; i < CMS_VNMRSIZE ; i++ ) {
	 if ( i % 20 == 0 )  fprintf ( spec, "\n" );
	 fprintf ( spec, "%3d ", (int)mblu[i] );
      }
      fprintf ( spec, "\n" );
   }

   fclose ( spec );
}
*/
/******************************************************************************/
/* 
pix_dump ( pw, width, height )
int width, height;
Pixwin *pw;
{
   int i, j, k;
   FILE *spec;
   
   spec = fopen ( "spec", "w" );
   fprintf ( spec, "Image pixels:" );

   for ( j = 0 ; j < height ; j++ ) {
      fprintf ( spec, "\nrow %d:  .......................................", j );
      k = 0;
      for ( i = 0 ; i < width ; i++ ) {
	 if ( k++ % 24 == 0 )  fprintf ( spec, "\n" );
	 fprintf ( spec, "%d ", pw_get(pw,i,j) );
      }
   }

   fclose ( spec );
}
*/

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

#include <stdio.h>
#include <malloc.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#if defined(__STDC__) || defined(__cplusplus)

static int parse_file(char *, int *, char ***, char **,
		      int *, int *, char **);
static int parse_string(char **, int *, char ***, char **,
			int *, int *, char **);
static void fmt_error(FILE *fd, char *fname, int line, char *msg);

static int create_cmap(Display *dpy, Window xid);
#else

static int parse_file();
static int parse_string();
static void fmt_error();

#endif

static Colormap cmap = NULL;
static XWindowAttributes  win_attr;


#ifdef __cplusplus
extern "C" {
#endif
    int set_color_icon(Display *display,
		  Window window,
		  char *filename,
		  int width,
		  int height);
    Pixmap get_icon_pixmap(Display *display,
			   Window window,
			   char *filename,
			   int width,
			   int height);
    Pixmap get_mem_icon_pixmap(Display *display,
			       Window window,
			       char **data,
			       int width,
			       int height);
#ifdef __cplusplus
}
#endif

static int
#if defined(__STDC__) || defined(__cplusplus)
create_cmap(Display *dpy, Window xid)
#else
create_cmap(dpy, xid)
Display *dpy;
Window  xid;
#endif
{
	XWindowAttributes  win_attr;

	if (cmap == NULL)
	{
            if (XGetWindowAttributes(dpy,  xid, &win_attr))
    	    {
	        if (win_attr.colormap)
	       	    cmap = win_attr.colormap;
	    }
	    if (cmap == NULL)
		cmap = XDefaultColormap(dpy, XDefaultScreen(dpy));
	}
    	return TRUE;
}


int
#if defined(__STDC__) || defined(__cplusplus)
set_color_icon(Display *display,
	       Window window,
	       char *filename,
	       int width,
	       int height)
#else
set_color_icon(display,
	       window,
	       filename,
	       width,
	       height)
  Display *display;
  Window window;
  char *filename;
  int width;
  int height;
#endif
{
    GC p_gc;
    Pixmap iconmap;
    XColor xcolor;
    Pixel *pixelvals;
    XWMHints xhint;
    XSetWindowAttributes winattr;
    Window iconwin;
    int ncolors;
    char **colornames;
    char *colorchrs;
    int dheight;
    int dwidth;
    char *data;
    char *pdata;
    int i;
    int j;
    int k;

    /*
     * Create the icon
     */
    /* Read the icon file */
    if (!parse_file(filename,
		    &ncolors, &colornames, &colorchrs,
		    &dwidth, &dheight, &data))
    {
	return FALSE;
    }

    /* Create a "simple window" */
    iconmap = XCreatePixmap(display, window, dwidth, dheight,
			    DefaultDepth(display, DefaultScreen(display)));
    p_gc = XCreateGC(display, iconmap, 0, 0);
    iconwin = XCreateSimpleWindow(display, window, 0, 0, dwidth, dheight,
				  0, 0, 0);

    /* Set the window manager hints */
    xhint.flags = IconWindowHint;
    xhint.icon_window = iconwin;
    XSetWMHints(display, window, &xhint);
    winattr.backing_store = Always;
    XChangeWindowAttributes(display, iconwin, CWBackingStore, &winattr);

    /* Allocate colors */
    create_cmap(display, iconwin);
    pixelvals = (Pixel *)malloc(ncolors * sizeof(Pixel));
    for (i=0; i<ncolors; i++){
	XParseColor(display, cmap, colornames[i], &xcolor);
	XAllocColor(display, cmap, &xcolor);
	pixelvals[i] = xcolor.pixel;
    }

    /* Load the pixmap */
    for (k=0; k<ncolors; k++){
	XSetForeground(display, p_gc, pixelvals[k]);
	pdata = data;
	for (j=0; j<dheight; j++){
	    for (i=0; i<dwidth; i++){
		if (*pdata++ == colorchrs[k]){
		    XDrawPoint(display, iconmap, p_gc, i, j);
		}
	    }
	}
    }
    free(pixelvals);

    /* Copy pixmap into icon window */
    XCopyArea(display, iconmap, iconwin, p_gc, 0, 0, dwidth, dheight, 0, 0);

    return TRUE;
}

Pixmap
#if defined(__STDC__) || defined(__cplusplus)
get_icon_pixmap(Display *display,
		Window window,
		char *filename,
		int width,
		int height)
#else
get_icon_pixmap(display,
		window,
		filename,
		width,
		height)
  Display *display;
  Window window;
  char *filename;
  int width;
  int height;
#endif
{
    GC p_gc;
    Pixmap iconmap;
    XColor xcolor;
    Pixel *pixelvals;
    XWMHints xhint;
    XSetWindowAttributes winattr;
    Window iconwin;
    int ncolors;
    char **colornames;
    char *colorchrs;
    int dheight;
    int dwidth;
    char *data;
    char *pdata;
    int i;
    int j;
    int k;

    /* Read the icon file */
    if (!parse_file(filename,
		    &ncolors, &colornames, &colorchrs,
		    &dwidth, &dheight, &data))
    {
	return NULL;
    }

    /* Allocate colors */
    iconmap = XCreatePixmap(display, window, dwidth, dheight,
			    DefaultDepth(display, DefaultScreen(display)));
    p_gc = XCreateGC(display, iconmap, 0, 0);
    create_cmap(display, window);
    pixelvals = (Pixel *)malloc(ncolors * sizeof(Pixel));
    for (i=0; i<ncolors; i++){
	XParseColor(display, cmap, colornames[i], &xcolor);
	XAllocColor(display, cmap, &xcolor);
	pixelvals[i] = xcolor.pixel;
    }

    /* Load the pixmap */
    for (k=0; k<ncolors; k++){
	XSetForeground(display, p_gc, pixelvals[k]);
	pdata = data;
	for (j=0; j<dheight; j++){
	    for (i=0; i<dwidth; i++){
		if (*pdata++ == colorchrs[k]){
		    XDrawPoint(display, iconmap, p_gc, i, j);
		}
	    }
	}
    }
    free(pixelvals);

    return iconmap;
}

Pixmap
#if defined(__STDC__) || defined(__cplusplus)
get_mem_icon_pixmap(Display *display,
		Window window,
		char **idata,
		int width,
		int height)
#else
get_mem_icon_pixmap(display,
		window,
		idata,
		width,
		height)
  Display *display;
  Window window;
  char **idata;
  int width;
  int height;
#endif
{
    GC p_gc;
    Pixmap iconmap;
    XColor xcolor;
    Pixel *pixelvals;
    XWMHints xhint;
    XSetWindowAttributes winattr;
    Window iconwin;
    int ncolors;
    char **colornames;
    char *colorchrs;
    int dheight;
    int dwidth;
    char *data;
    char *pdata;
    int i;
    int j;
    int k;

    /* Read the icon file */
    if (!parse_string(idata,
		      &ncolors, &colornames, &colorchrs,
		      &dwidth, &dheight, &data))
    {
	return NULL;
    }

    /* Allocate colors */
    iconmap = XCreatePixmap(display, window, dwidth, dheight,
			    DefaultDepth(display, DefaultScreen(display)));
    p_gc = XCreateGC(display, iconmap, 0, 0);
    create_cmap(display, window);
    pixelvals = (Pixel *)malloc(ncolors * sizeof(Pixel));
    for (i=0; i<ncolors; i++){
	XParseColor(display, cmap, colornames[i], &xcolor);
	XAllocColor(display, cmap, &xcolor);
	pixelvals[i] = xcolor.pixel;
    }

    /* Load the pixmap */
    for (k=0; k<ncolors; k++){
	XSetForeground(display, p_gc, pixelvals[k]);
	pdata = data;
	for (j=0; j<dheight; j++){
	    for (i=0; i<dwidth; i++){
		if (*pdata++ == colorchrs[k]){
		    XDrawPoint(display, iconmap, p_gc, i, j);
		}
	    }
	}
    }
    free(pixelvals);

    return iconmap;
}

static int
#if defined(__STDC__) || defined(__cplusplus)
parse_file(char *filename,
	   int *ncolors,
	   char ***colornames,
	   char **colorchrs,
	   int *width,
	   int *height,
	   char **data)
#else
parse_file(filename,
	   ncolors,
	   colornames,
	   colorchrs,
	   width,
	   height,
	   data)
  char *filename;
  int *ncolors;
  char ***colornames;
  char **colorchrs;
  int *width;
  int *height;
  char **data;
#endif
{
    char buf[100];
    char *linebuf;
    FILE *fd;
    int line = 0;
    int cpp;
    int i;

    fd = fopen(filename, "r");
    if (!fd){
	fmt_error(fd, filename, line, "File not found");
	return FALSE;
    }
    if (!fgets(buf, sizeof(buf), fd)){
	fmt_error(fd, filename, line, "Empty file");
	return FALSE;
    }
    line = 1;
    if (!strstr(buf, "XPM2")){
	fmt_error(fd, filename, line, "File not in XPM2 format");
	return FALSE;
    }

    /* Read in the image size and number of colors */
    line++;
    if (!fgets(buf, sizeof(buf), fd)){
	fmt_error(fd, filename, line, "File ends prematurely");
	return FALSE;
    }
    if (sscanf(buf,"%d %d %d %d", width, height, ncolors, &cpp) != 4){
	fmt_error(fd, filename, line, "Format error");
	return FALSE;
    }
    if (cpp != 1){
	fmt_error(fd, filename, line, "Must use 1 char per pixel");
	return FALSE;
    }

    /* Read in the color names */
    *colornames = (char **)malloc(*ncolors * sizeof(char *));
    *colorchrs = (char *)malloc(*ncolors + 1);
    **colorchrs = '\0';
    for (i=0; i<*ncolors; i++){
	line++;
	if (!fgets(buf, sizeof(buf), fd)){
	    fmt_error(fd, filename, line, "File ends while reading colors");
	    return FALSE;
	}
	strncat(*colorchrs, buf, 1); /* Read the char code */
	(*colornames)[i] = (char *)malloc(strlen(buf));
	if (sscanf(buf+1," c %s", (*colornames)[i]) != 1){
	    fmt_error(fd, filename, line, "Format error reading color name");
	    return FALSE;
	}
    }

    /* Read the picture data */
    *data = (char *)malloc(*width * *height + 1);
    linebuf = (char *)malloc(*width + 100);
    for (i=0; i< *height; i++){
	line++;
	if (!fgets(linebuf, *width + 100, fd)){
	    fmt_error(fd, filename, line, "File ends while reading data");
	    return FALSE;
	}
	strncpy(*data + i * *width, linebuf, *width);
    }
    free(linebuf);
    return TRUE;
}

static int
#if defined(__STDC__) || defined(__cplusplus)
parse_string(char **buf,
	     int *ncolors,
	     char ***colornames,
	     char **colorchrs,
	     int *width,
	     int *height,
	     char **data)
#else
parse_string(buf,
	     ncolors,
	     colornames,
	     colorchrs,
	     width,
	     height,
	     data)
  char **buf;
  int *ncolors;
  char ***colornames;
  char **colorchrs;
  int *width;
  int *height;
  char **data;
#endif
{
    char filename[] = "Internal data";
    FILE *fd = NULL;
    int line = 0;
    int cpp;
    int i;

    /* Read in the image size and number of colors */
    if (sscanf(buf[line],"%d %d %d %d", width, height, ncolors, &cpp) != 4){
	fmt_error(fd, filename, line+1, "Format error");
	return FALSE;
    }
    if (cpp != 1){
	fmt_error(fd, filename, line+1, "Must use 1 char per pixel");
	return FALSE;
    }

    /* Read in the color names */
    *colornames = (char **)malloc(*ncolors * sizeof(char *));
    *colorchrs = (char *)malloc(*ncolors + 1);
    **colorchrs = '\0';
    for (i=0; i<*ncolors; i++){
	line++;
	strncat(*colorchrs, buf[line], 1); /* Read the char code */
	(*colornames)[i] = (char *)malloc(strlen(buf[line]));
	if (sscanf(buf[line]+1," c %s", (*colornames)[i]) != 1){
	    fmt_error(fd, filename, line+1, "Format error reading color name");
	    return FALSE;
	}
    }

    /* Read the picture data */
    *data = (char *)malloc(*width * *height + 1);
    for (i=0; i< *height; i++){
	line++;
	strncpy(*data + i * *width, buf[line], *width);
    }
    return TRUE;
}

static void
#if defined(__STDC__) || defined(__cplusplus)
fmt_error(FILE *fd, char *fname, int line, char *msg)
#else
fmt_error(fd, fname, line, msg)
  FILE *fd;
  char *fname;
  int line;
  char *msg;
#endif
{
    if (line){
	fprintf(stderr,"Icon error: file \"%s\", line %d: %s\n",
		fname, line, msg);
    }else{
	fprintf(stderr,"Icon error: file \"%s\": %s\n",
		fname, msg);
    }
    if (fd){
	fclose(fd);
    }
    return;
}

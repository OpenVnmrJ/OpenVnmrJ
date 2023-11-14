/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "vnmrsys.h"
#include "graphics.h"
#include "group.h"
#include "aipCStructs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "graphics_util.h"
#include "allocate.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"

#ifdef MOTIF
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <Xm/Xm.h>
#include <Xm/Protocols.h>
#include <Xm/BulletinB.h>
#include <Xm/DrawingAP.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#else
#include "vjXdef.h"
#endif

extern int graf_width, graf_height;
#ifdef VNMRJ
extern int useXFunc;
extern void recover_origin_font();
extern void window_redisplay();
extern void set_rgb_color();
extern void set_x_font();
extern void sun_draw_image();
extern void save_origin_font();
extern void releaseGifNode();
extern int get_text_width();
extern int get_font_ascent();
extern int get_font_descent();
extern int graphToVnmrJ();
extern char *AnnotateExpression();
extern XFontStruct *open_annotate_font();
#endif

typedef struct  _xnode {
        int     type;
        int     nodeId;
        float   locx;
        float   locy;
        int     cols;  /* for table */
        int     rows;  /* fot table */
        int     tx;    /* for table, # of col */
        int     ty;    /* for table, # of row */
        int     tableMember;  /* the member of table */
        int     sizeX;
        int     sizeY;
        int     cw; /* new width */
        int     ch; /* new height */
        int     oldw; /* previous width */
        int     oldh; /* previous height */
        int     show; /* show condition */
        int     drawable; /* show condition */
        int     tw; /* width of text */
        int     th; /* height of text */
        int     minW; /* the minimum width of text */
        int     minH; /* the minimum height of text */
        int     iconW; /* width of icon */
        int     iconH; /* height of icon */
        int     def_fontSize;
        int     bold;
        int     italic;
        int     fontType;
        int     font_scale; /* how to scale font, fit only, variable, fixed */
        int     fixedFont;
        int     ascent;
        int     color;
        int     orientId; /* orientation object, if > 0 */
        int     orient; /* display orientation, horizontal or vertical */
        int     halign; /* horizontal alignment */
        int     valign; /* vertical alignment */
        int     vLen;  /* the size of string value */
        int     units;
        int     iLen;  /* size of incoInfo */
        double  fontSize;
        char    *digits;
        char    *value;
        char    *setVal;
        char    *showStr;
        char    *icon;
        char    *iconInfo;
        char    *psfont;
        char    *dock;
        char    *fontName;
        XFontStruct *fontStruct;
	IMGNode *imgNode;
        struct  _xnode *parent;
        struct  _xnode *children;
        struct  _xnode *brother;
        struct  _xnode *orig;  /* the original node */
    } ANN_NODE;

typedef struct  _iconnode {
        int     type;
        int     subtype;
        int     x;
        int     y;
        int     w;
        int     h;
        int     hpos;
        int     vpos;
        int     len;
        char    *icon;
    } ICON_NODE;

typedef struct  _head {
        int     x;
        int     y;
        int     w;
        int     h;
        int     imgId;
        int     newTemp;
        int     keyLen;
	char    orientInfo[5];
        char    *aipKey;
        ANN_NODE *start;
	ANN_NODE *orientNodes[4];
    } HEAD_NODE;


static IMGNode  *imgList = NULL;

static struct {
        int     type;
        int     code;
        int     id;
        int     p0;
        int     p1;
        int     p2;
        int     p3;
        int     len;
	char    info[256];
      } jInfo;


static char      *iconNode = NULL;
static ANN_NODE  *mainNode = NULL;
static ANN_NODE  *startNode = NULL;
static ANN_NODE  *curNode = NULL;
static ANN_NODE  *checkNode = NULL;
static ANN_NODE  **orientNodes;
static ANN_NODE  *defOrientNodes[4];
static HEAD_NODE **headNode = NULL;
static HEAD_NODE *curHnode = NULL;
static int  grid = 40;
static int  isActive = 1;
static int  headNodeSize = 0;
static int  org_x = 0;
static int  org_y = 0;
static int  org_w = 0;
static int  org_h = 0;
static int  win_y = 0;
static int  width = 0;
static int  height = 0;
static int  frame_width = 0;
static int  frame_height = 0;
static int  new_str_len = 0;
static int  annotatePlot = 0;
static int  debug = 0;
static int  preview = 0;
static int  printErr = 0;
static int  curColor = 0;
static int  show_orientation = 0;
#ifdef MOTIF
static int  *table_widths = NULL;
static int  *table_heights = NULL;
static int  *table_posX = NULL;
static int  *table_posY = NULL;
#endif
static int  table_col_size = 0;
static int  table_row_size = 0;
static int  objId = 0;
static int  new_template = 1;
static int  jHLen;
static float  gridWidth = 0;
static float  gridHeight = 0;
static char  inputs[256];
static char  attrName[32];
static char  tempPath[256];
static char  oldPath[256];
static char  *orientInfo;
static char  *tempName = NULL;
static char  *inPtr = NULL;
static const char  *aipKey = NULL;
static char  defaultName[] = "default";
static char  previewName[] = "annpreview";
static time_t old_mtime = 0;
static CImgInfo_t imageInfo;

#define LABEL   1
#define BOX     2
#define ATEXT   3
#define VALUE   4
#define UNITS   5
#define SETVAL  6
#define SHOW    7
#define DIGITS  8
#define HALIGN  9
#define VALIGN  10
#define ORIENT  11
#define ROW     12
#define COL     13
#define LOCX    14
#define LOCY    15
#define ICON    16
#define GRID    17
#define PSFONT  18
#define FONTH   19
#define COLOR   20
#define DOCK   21
#define FONTNAME   22
#define FONTSTYLE   23
#define SHOWORIENT   24
#define CHECK   25
#define XEND    26
#define ORIENTID   27
#define ELASTIC   28
#define ROWS   29
#define COLS   30
#define TABLE  31
#define ICONW  32
#define ICONH  33
#define IDNUM  34

#define BOLD   1
#define ITALIC 2
#define MINW   5
#define MINH   6
#define CENTER 1
#define TOP    2
#define BOTTOM 3
#define LEFT   4
#define RIGHT  5
#define VERTICAL 1

/* the attribute of font */
#define FITONLY  0
#define SCALABLE 1
#define FIXED    2
#define FIXEDFIT 3

#define TYPENUM  29

static char *types[] = {"label", "units", "set", "justify", "orientation",
             "row", "column", "locx", "locy", "halignment",
             "valignment", "show", "digits", "icon", "grid",
             "psfont", "fontSize", "color", "dockat", "fontName",
             "fontStyle", "show_orientation", "orientid", "elastic", "rows",
             "columns", "iconWidth", "iconHeight", "idnumber" };

static int typeVals[TYPENUM] = { VALUE, UNITS, SETVAL, HALIGN, ORIENT,
              ROW, COL, LOCX, LOCY, HALIGN,
              VALIGN, SHOW, DIGITS, ICON, GRID,
              PSFONT, FONTH, COLOR, DOCK, FONTNAME,
              FONTSTYLE, SHOWORIENT, ORIENTID, ELASTIC, ROWS,
              COLS, ICONW, ICONH, IDNUM };

#define FONTNUM  5
static char *jFonts[] = {"Dialog", "Serif", "SansSerif", "Monospaced",
              "DialogInput" };

static char *vFonts[] = {"arial", "times new roman", "arial", "courier new", "courier new" };

#define XFONTS   12
#define MAXSIZE  24

#ifdef MOTIF
static char *xFonts[] = {"5x7", "5x8", "5x8", "6x10", "6x10", "6x13", "7x13",
               "8x13", "9x15", "8x16", "10x20", "12x24" };
static char *bFonts[] = {"5x7", "5x8", "5x8", "6x10", "6x10", "6x13", "6x13bold",
               "7x14bold", "8x13bold", "9x15bold", "10x20", "12x24" };
#endif


extern int  aipGetImageInfo();
extern int  aipGetHdrReal();
extern int  aipGetHdrInt();
extern int  aipGetHdrStr();
extern XFontStruct *open_annotate_xfont();
extern IMGNode *open_imagefile();

static int table_set_font(ANN_NODE *node); 
#ifdef MOTIF
static XFontStruct *minFsStruct = NULL;
static XFontStruct **fontList = NULL;
static int         size_fontList = 0;
static int         index_fontList = 0;
#endif
static struct stat fileStat;

static char *annAllocate(int size)
{
   char *d;

   d = (char *)allocateWithId (size, "annotate");
   if (d != NULL)
	bzero(d, size);
   return (d);
}

static ANN_NODE
*new_node(int type)
{
    ANN_NODE  *newNode;

    if ((newNode = (ANN_NODE *) annAllocate (sizeof(ANN_NODE))) == NULL)
        return(NULL);
    newNode->type = type;
    newNode->rows = 1;
    newNode->cols = 1;
    newNode->sizeX = 1;
    newNode->sizeY = 1;
    newNode->show = 1;
    newNode->oldw = 999;
    newNode->oldh = 999;
    newNode->def_fontSize = 12;
    newNode->fontSize = 12;
    newNode->drawable = 1;
    newNode->nodeId = 0;
    newNode->halign = CENTER;
    newNode->valign = CENTER;
    newNode->fontName = vFonts[0];
    newNode->children = NULL;
    newNode->brother = NULL;
    newNode->orig = NULL;
    newNode->fontStruct = NULL;
    newNode->value = NULL;
    newNode->imgNode = NULL;
    newNode->icon = NULL;
    newNode->iconInfo = NULL;
    return(newNode);
}

static void
add_node(int type)
{
    ANN_NODE  *newNode;
    ANN_NODE  *link;

    newNode = new_node(type);
    if (newNode == NULL)
        return;
    newNode->parent = curNode;
    newNode->nodeId = objId;
    objId++;
    if (curNode->children == NULL)
        curNode->children = newNode;
    else {
        link = curNode->children;
        while (link->brother != NULL)
            link = link->brother;
        link->brother = newNode;
    }
    curNode = newNode;
}

static char *
get_value()
{
    char *d, *d1, *data;
    int len;

    if (inPtr == NULL)
        return (inPtr);
    d = inPtr;
    d1 = NULL;
    while (*d == ' ' || *d == '\t') d++;
    while (*d != '\n' && *d != '\0')
    {
        if (*d == '"') { 
             d1 = d + 1;
             break;
        }
        d++;
    }
    if (d1 == NULL)
        return (NULL);
    d = d1;
    while (*d1 != '\n' && *d1 != '\0')
    {
        if (*d1 == '"') { 
             break;
        }
        d1++;
    }
    len = d1 - d;
    if (len <= 0)
        return (NULL);
    if ((data = (char *) annAllocate (len+1)) == NULL)
        return(NULL);
    strncpy(data, d, len);
    data[len] = '\0';
    return (data);
}

static int
get_type(char *s)
{
    int type, len, k;
    char *d, *d1;

    type = 0;
    d = s;
    inPtr = NULL;
    while (*d == ' ' || *d == '\t') d++;
    if (*d == '\0' || *d == '\n')
        return (0);
    if (*d == '<') {
        if (*(d+1) == '/')
            return (XEND);
        if (strncmp(d, "<annotatebox>", 13 ) == 0)
            return (BOX);
        if (strncmp(d, "<label>", 7) == 0)
            return (LABEL);
        if (strncmp(d, "<textline>", 10) == 0)
            return (ATEXT);
        if (strncmp(d, "<textmessage>", 13) == 0)
            return (ATEXT);
        if (strncmp(d, "<check>", 7) == 0)
            return (CHECK);
        if (strncmp(d, "<annotatetable>", 15) == 0)
            return (TABLE);
        return (0);
    }
    d1 = d;
    while (*d1 != '\n' && *d1 != '\0')
    {
        if (*d1 == '=')
             break;
        d1++;
    }
    if (*d1 != '=')
        return (0);
    inPtr = d1 + 1;
    len = d1 - d;
    if (len <= 0 || len > 30)
        return (0);
    strncpy(attrName, d, len);
    attrName[len] = '\0';
    for (k = 0; k < TYPENUM; k++) {
        if (strcmp(attrName, types[k]) == 0)
           return (typeVals[k]);
    }
    return (0);
}

static int
get_fontIndex(char *s)
{
    int  k;

    for (k = 0; k < FONTNUM; k++) {
        if (strcmp(s, jFonts[k]) == 0)
           return (k);
    }
    return (0);
}


static void
dumpNode(ANN_NODE *node, int gap)
{
     int  k, m;
     char str[120];

     k = gap * 2;
     m = 0;
     while (m < k) {
         str[m] = ' ';
         m++;
     }
     str[m] = '\0';
     fprintf(stderr, "%s", str);
     fprintf(stderr, "type:  %d\n", node->type);
     if (node->dock != NULL) {
        fprintf(stderr, "%s", str);
        fprintf(stderr, "dock:  %s\n", node->dock);
     }
     if (node->children != NULL) {
          fprintf(stderr, "%s", str);
          fprintf(stderr, " disp child \n");
          dumpNode(node->children, gap+2);
     }
     if (node->brother != NULL) {
          fprintf(stderr, "%s", str);
          fprintf(stderr, " disp brother \n");
          dumpNode(node->brother, gap);
     }
}

static void
dump()
{
     int level;

     level = 0;
     curNode = startNode;
     while (curNode != NULL) {
          if (curNode->children != NULL) {
               dumpNode(curNode->children, level+1);
          }
          curNode = curNode->brother;
     }
}

#ifdef MOTIF
static void
clear_font(ANN_NODE *node)
{
     node->fontStruct = NULL;
     node->fontSize = node->def_fontSize;
     node->oldw = 999;
     node->oldh = 999;
     if (node->children != NULL) {
         clear_font(node->children);
     }
     if (node->brother != NULL) {
         clear_font(node->brother);
     }
}
#endif /* MOTIF */

#ifdef MOTIF
static void
add_font_list(XFontStruct *f)
{
     int  new_size, k;
     XFontStruct **new_list;

     if (!useXFunc)
         return;
     if (size_fontList <= index_fontList) {
         if (size_fontList == 0)
             new_size = 50;
         else
             new_size = size_fontList + 20;
         new_list = (XFontStruct **) malloc(sizeof(XFontStruct *) * new_size);
         if (new_list == NULL) {
             return;
         }
         if (fontList != NULL) {
             for (k = 0; k < size_fontList; k++)
                 new_list[k] = fontList[k];
             free(fontList);
         }
         for (k = size_fontList; k < new_size; k++)
             new_list[k] = NULL;
         fontList = new_list;
         size_fontList = new_size;
      }
      if (fontList != NULL) {
         fontList[index_fontList] = f;
         index_fontList++;
      }
}
#endif

static void
free_font_list()
{
#ifdef MOTIF
     ANN_NODE *xnode;
     int  k;

     index_fontList = 0;
     if (fontList == NULL)
         return;
     for (k = 0; k < size_fontList; k++) {
         if (fontList[k] != NULL) {
             XFreeFontInfo(NULL, fontList[k], 1);
             fontList[k] = NULL;
         }
     }
     xnode = mainNode;
     while (xnode != NULL) {
          xnode->fontStruct = NULL;
          xnode->fontSize = xnode->def_fontSize;
          xnode->oldw = 999;
          xnode->oldh = 999;
          if (xnode->children != NULL)
               clear_font(xnode->children);
          xnode = xnode->brother;
     }
#endif
}

static int
set_pfont(ANN_NODE *node) 
{
     int  len, k, result, fs;
     int h;
     double size;

     result = 1;
     if (node->font_scale == FIXED || node->font_scale == FIXEDFIT)
        node->fixedFont = 1;
     if (node->type == TABLE) {
         result = table_set_font(node);
     }
     else if (node->drawable && (node->value != NULL)) {
         size =  node->def_fontSize / 14.0;
	 h = node->ch;
         if (h < size) {
	    if (node->font_scale != FIXED)
                size = h;
	    else
                h = size;
	 }
         len = strlen(node->value);
	 if (len < 1)
	    len = 1;
         fs = ppmm * 1.2;
         while (1) {
              fontsize(size);
              if (xcharpixels < fs)
                  return(0);
              if (node->orient == VERTICAL) {   
		 k = ycharpixels * len - 2;
                 if (k <= h || node->font_scale == FIXED) {
		     node->tw = xcharpixels;
		     node->th = k;
                     break;
		 }
              }
              else {
		 k = xcharpixels * len - 2;
                 if (k <= node->cw || node->font_scale == FIXED) {
		     node->tw = k;
		     node->th = ycharpixels;
                     break;
		 }
              }
              size = size - 0.1;
              if (size < 0.3) {
		  node->tw = 0;
		  node->th = 0;
		  node->drawable = 0;
                  break;
	      }
         }
         node->fontSize = size;
     }
     if (node->tableMember)
         return(result);
     if (node->type != TABLE) {
         if (node->children != NULL) {
             result = set_pfont(node->children);
         }
     }
     if (node->brother != NULL) {
          result = set_pfont(node->brother);
     }
     return(result);
}

#ifdef MOTIF
static int
set_xfont(ANN_NODE *node) 
{
     int result = 1;
     int size, len, k, index;
     int h, fit_only, fh;
     XFontStruct *fstruct;
     char *fname;

     if (node->value == NULL)
	return(result);
     if (node->fixedFont && node->fontStruct != NULL)
	return(result);
     if (node->font_scale == FITONLY || node->font_scale == FIXEDFIT)
        fit_only = 1;
     else
	fit_only = 0;
     size = node->def_fontSize;
     h = node->ch;
     if (h < size) {
	 if (node->font_scale == FIXEDFIT)
	    return (0);
	 if (node->font_scale != FIXED)
             size = h;
	 else
             h = size;
     }
     index = size - 7;
     if (index < 0)
         index = 0;
     if (index >= XFONTS)
         index = XFONTS - 1;
     len = strlen(node->value);
     if (len < 1) {
	len = 1;
        strcpy(node->value, " ");
     }
     fstruct = NULL;
     while (1) {
        if (fstruct != NULL)
               XFreeFontInfo(NULL, fstruct, 1);
        if (node->bold)
               fname = bFonts[index];
        else
               fname = xFonts[index];
        fstruct = (XFontStruct *) open_annotate_xfont(fname);
	fh = 0;
        if (fstruct != NULL) {
             fh = fstruct->ascent + fstruct->descent;
	     if (fstruct->descent < 2) /* font info not correct */
		fh++;
	     if (node->fixedFont) {
		if (fh < node->def_fontSize)
		    index = 0;
		if (fh != node->def_fontSize)
		    fh = 0;
	     }
	}
        if (fstruct != NULL && fh > 0) {
             if (node->orient == VERTICAL) {   
		 k = fh * len - 2;
                 if (k <= h || node->font_scale == FIXED) {
                     node->tw = XTextWidth(fstruct, "M", 1);
                     node->th = k;
                     break;
		 }
              }
              else {
                 k = XTextWidth(fstruct, node->value, len) - 2;
                 if (k <= node->cw) {
                    if (fh <= (h + 1) || node->font_scale == FIXED) {
                        node->tw = k;
                        node->th = fh;
                        break;
		    }
                 }
              }
        }
        index--;
        if (index < 0) {
	      if (fit_only) {
                 if (fstruct != NULL)
                     XFreeFontInfo(NULL, fstruct, 1);
                 fstruct = NULL;
              }
              break;
        }
     } /* while */
     if (fstruct == NULL) {
         if (printErr)
                 Werrprintf("No enough room for '%s'", node->value);
         return (0);
     }
     add_font_list(fstruct);
     node->fontStruct = fstruct;
     node->ascent = fstruct->ascent;
     return(result);
}
#endif


static int
set_font(ANN_NODE *node) 
{
     int result = 0;
#ifdef MOTIF
     int size, len, k, ret;
     int h, m, fh, doLoop;
     XFontStruct *fstruct;
     ICON_NODE *jnode;

     ret = 1;
     fstruct = NULL;
     if (node->font_scale == FIXED || node->font_scale == FIXEDFIT)
        node->fixedFont = 1;
     doLoop = 0;
     if (node->type == TABLE) {
         ret = table_set_font(node);
     }
     else if (node->drawable && (node->value != NULL)) {
         if (node->fontStruct != NULL) {
             if (node->tableMember && node->fixedFont)
                return(1);
             if (node->oldw > node->cw || node->oldh > node->ch)
                doLoop = 1;
	 }
         else
             doLoop = 1;
     }
     if(doLoop) {
         node->oldw = node->cw;
         node->oldh = node->ch;
#ifdef VNMRJ
         size = node->def_fontSize;
         h = node->ch;
	 k = 0;
	 m = 11;
	 if (m >= size)
	     m = size - 1;
	 if (node->font_scale == FIXEDFIT)
	     m = size - 1;
         if (h < size) {
	    if (node->font_scale != FIXED)
                size = h;
	    else
                h = size;
	 }
         len = strlen(node->value);
         fstruct = node->fontStruct;
         ret = 0;
         while (1) {
           if (fstruct != NULL) {
              fh = fstruct->ascent + fstruct->descent;
     	      node->ascent = fstruct->ascent;
              if (node->font_scale == FIXED)
		 ret = 1;
              if (node->orient == VERTICAL) {   
		 k = fh * len - 2;
                 if (k <= h || ret) {
                     node->tw = XTextWidth(fstruct, "M", 1);
                     node->th = k;
		     ret = 1;
		     break;
		 }
              }
              else {
                 k = XTextWidth(fstruct, node->value, len) - 2;
                 if (node->font_scale == FIXED)
		    ret = 1;
                 else if (k <= node->cw && (fh <= (h + 1)))
		    ret = 1;
                 if (ret) {
                     node->tw = k;
                     node->th = fh - 1;
		     break;
		 }
              }
              if (fstruct != node->fontStruct)
                 XFreeFontInfo(NULL, fstruct, 1);
              fstruct = NULL;
           }
           if (size <= m) {
               break;
	   }
 	   size--;
           fstruct = (XFontStruct *) open_annotate_font(
                   "monotype",node->fontName, node->bold, node->italic, size);
         }  /* end of while */
#endif
	 ret = 1;
         if (fstruct == NULL) {
	     if (set_xfont(node) < 1) {
 		 node->drawable = 0;
 		 node->tw = 0;
 		 node->th = 0;
	         ret = 0;
                 node->fontStruct = NULL;
             }
         }
	 else {
             if (fstruct != node->fontStruct) {
                node->fontStruct = fstruct;
                add_font_list(fstruct);
             }
         }
     }
     if ((node->iconInfo != NULL) && node->drawable) {
	 if (node->cw >= node->iconW) {
	      if (node->tw < node->iconW)
		 node->tw = node->iconW;
	 }
	 else {
     	     if (node->font_scale == FIXEDFIT) {
	         node->tw = 0;
 		 node->drawable = 0;
	     }
	     else {
	         node->tw = node->cw;
                 if (node->cw < (node->iconW / 4)) 
 		     node->drawable = 0;
	     }
	 }
	 if (node->ch >= node->iconH) {
	      if (node->th < node->iconH)
		 node->th = node->iconH;
	 }
	 else {
     	     if (node->font_scale == FIXEDFIT) {
	         node->th = 0;
 		 node->drawable = 0;
	     }
	     else {
	         node->th = node->ch;
                 if (node->ch < (node->iconH / 4)) 
 		     node->drawable = 0;
	     }
	 }
         jnode = (ICON_NODE *) node->iconInfo;
         jnode->type = 1;
         jnode->subtype = 25;
         jnode->w = node->cw;
         jnode->h = node->ch;
         jnode->hpos = 0; /* center */
         if (node->halign == RIGHT)
               jnode->hpos = 1;
         else if (node->halign == LEFT)
               jnode->hpos = 2;
         jnode->vpos = 0;
         if (node->valign == TOP)
               jnode->vpos = 1;
         else if (node->valign == BOTTOM)
               jnode->vpos = 2;
         jnode->len = (int) strlen(node->icon);
     }
     if (ret > 0)
	 result = 1;
     if (node->tableMember)
	 return(result);
     if (node->type != TABLE) {
        if (node->children != NULL) {
            ret = set_font(node->children);
            if (ret > 0)
	        result = 1;
	}
     }
     if (node->brother != NULL) {
         ret = set_font(node->brother);
     }
     if (ret > 0)
	 result = 1;
#endif /* MOTIF */
     return(result);
}

static int
table_set_font(ANN_NODE *table) 
{
#ifdef MOTIF
     int k, xw, xh;
     int w, h, mh, minW, loops, dw, dh;
     ANN_NODE *node;

     node = table->children;
     if (table_col_size <= table->cols) {
         table_col_size = table->cols + 1;
         table_widths = (int *) annAllocate (sizeof(int) * table_col_size);
         table_posX = (int *) annAllocate (sizeof(int) * table_col_size);
     }
     if (table_row_size <= table->rows) {
         table_row_size = table->rows + 1;
         table_heights = (int *) annAllocate (sizeof(int) * table_row_size);
         table_posY = (int *) annAllocate (sizeof(int) * table_row_size);
     }
     for (k = 0; k < table_col_size; k++) {
	 table_widths[k] = 0;
     }
     for (k = 0; k < table_row_size; k++)
	 table_heights[k] = 0;
     w = table->cw - 2 - (table->cols - 1) * xcharpixels; 
     if (w < MINW)
	w = MINW;
     h = table->ch - 2;
     if (table->rows > 1) {
        if (annotatePlot)
	   h = h - ycharpixels / 4; 
        else
	   h = h - 2;
     }
     if (h < MINH)
	h = MINH;
     if (minFsStruct == NULL)
        minFsStruct = (XFontStruct *) open_annotate_xfont(xFonts[0]);
     node = table->children;
     while (node != NULL) {
	node->ch = h;
	node->cw = w;
        if (annotatePlot)
           set_pfont(node);
	else
           set_font(node); 
	if (node->tx >= table->cols) {
	    node->tx = table->cols - 1;
	}
	if (node->tw > table_widths[node->tx])
	    table_widths[node->tx] = node->tw; 
	if (node->ty >= table->rows) {
	    node->ty = table->rows - 1;
	}
	if (node->th > table_heights[node->ty])
	    table_heights[node->ty] = node->th; 
        if (node->drawable == 0 && node->fixedFont == 0) {
            table->drawable = 0;
            return(1);
        }
        node = node->brother;
     }
     xw = 0;
     for (k = 0; k < table->cols; k++) {
	xw += table_widths[k];
     }
     xh = 0;
     for (k = 0; k < table->rows; k++) {
	xh += table_heights[k];
     }
     dw = xw - table->cw;
     dh = xh - table->ch;
     for (k = 0; k < table->cols; k++)
         table_posX[k] = 0;
     if (dw > 0 && minFsStruct != NULL ) {
	 /* to calculate the minimum width with the smallest font */
         node = table->children;
         while (node != NULL) {
	     if (node->fixedFont)
		node->minW = node->tw;
	     else {
	        if (node->value != NULL)
	            node->minW = XTextWidth(minFsStruct, node->value, 
			strlen(node->value));
	        else
	            node->minW = node->tw / 4;
	     }
	     if (node->minW > table_posX[node->tx])
	        table_posX[node->tx] = node->minW; 
             node = node->brother;
	 }
     }
     minW = 0;
     for (k = 0; k < table->cols; k++)
         minW += table_posX[k];
     if (dh > 0 && dw < 0) {
	dw = (table->cw - xw) / table->cols;
        for (k = 0; k < table->cols; k++)
	   table_widths[k] += dw;
	dw = 0;
     }
     loops = 0;
     while (dw > 0 || dh > 0) {
	  loops++;
	  if (loops > 5)
	     break;
	  if (dw > 0) {
	     dw = (table->cw - minW) / table->cols; 
	     if (dw < 0) {
		loops = 5;
	     }
             for (k = 0; k < table->cols; k++) {
		  table_widths[k] = table_posX[k] + dw;
	     }
          }
	  while (dh > 0) {
	     mh = 999;
	     h = 0;
             for (k = 0; k < table->rows; k++) {
	     	if (table_heights[k] > h)
		   h = table_heights[k];
	     	if (table_heights[k] < mh)
		   mh = table_heights[k];
	     }
	     h = (mh + h ) / 2;
             dh = dh / table->rows;
	     if (dh < 1)
		dh = 1;
	     xh = 0;
             for (k = 0; k < table->rows; k++) {
	     	if (table_heights[k] >= h)
		   table_heights[k] -= dh;
	        xh += table_heights[k];
	     }
	     dh = xh - table->ch;
          }

          node = table->children;
          while (node != NULL) {
	      node->cw = table_widths[node->tx];
	      node->ch = table_heights[node->ty];
              if (annotatePlot)
                  set_pfont(node);
	      else
                  set_font(node); 
              if (node->drawable == 0)
                  table->drawable = 0;
              node = node->brother;
	  }
          for (k = 0; k < table->cols; k++)
	      table_widths[k] = 0;
          for (k = 0; k < table->rows; k++)
	      table_heights[k] = 0;
          node = table->children;
          while (node != NULL) {
	      if (node->tw > table_widths[node->tx])
	    	table_widths[node->tx] = node->tw; 
	      if (node->th > table_heights[node->ty])
	    	table_heights[node->ty] = node->th; 
              node = node->brother;
	  }
	  xw = 0;
          for (k = 0; k < table->cols; k++)
	      xw += table_widths[k];
	  xh = 0;
          for (k = 0; k < table->rows; k++)
          {
	      xh += table_heights[k];
	  }
	  dw = xw - table->cw;
	  dh = xh - table->ch;
     }
     w = (table->cw - xw) / table->cols;
     if (w > 4)
	w = 4;
     xw = 0;
     for (k = 0; k < table->cols; k++) {
          table_posX[k] = xw;
	  table_widths[k] += w;
	  xw += table_widths[k];
     }
     h = (table->ch - xh) / table->rows;
     xh = 0;
     for (k = 0; k < table->rows; k++) {
          table_posY[k] = xh;
	  xh += table_heights[k];
     }
     node = table->children;
     k = 1;
     w = 0;
     while (node != NULL) {
	  node->locx = table_posX[node->tx];
	  node->cw = table_widths[node->tx];
	  node->locy = table_posY[node->ty];
	  node->ch = table_heights[node->ty];
	  if (node->drawable) {
	      if (node->fixedFont || node->font_scale == SCALABLE)
		   w = 1;
	  }
	  else
	      k = 0;
          node = node->brother;
     }
     if (k == 0 && w == 0)
	  table->drawable = 0;
     else
	  table->drawable = 1;
     table->tx = table->locx * gridWidth;
     table->ty = table->locy * gridHeight;
     if (table->halign != 0) {
	 dw = 0;
	 if (table->halign == CENTER)
	     dw = (table->cw - xw) / 2;
	 else if (table->halign == RIGHT)
	     dw = table->cw - xw;
         table->tx += dw;
     }
     if (table->valign != 0) {
	 dh = 0;
	 if (table->valign == CENTER)
	     dh = (table->ch - xh) / 2;
	 else if (table->valign == BOTTOM)
	     dh = table->ch - xh;
         table->ty += dh;
     }
     for (k = 0; k < table->rows; k++)
	 table_heights[k] = 0;
     node = table->children;
     while (node != NULL) {
	  if (node->ascent > table_heights[node->ty])
		table_heights[node->ty] = node->ascent;
          node = node->brother;
     }
     node = table->children;
     while (node != NULL) {
	  node->ascent = table_heights[node->ty];
          node = node->brother;
     }

#endif /* MOTIF */
     return(1);
}


#ifdef OLD
static void
free_xfont(ANN_NODE *node) 
{
#ifdef MOTIF
     if (node != checkNode && node->fontStruct != NULL) {
         XFreeFontInfo(NULL, node->fontStruct, 1);
         node->fontStruct = NULL;
     }
     if (node->children != NULL) {
         free_xfont(node->children);
     }
     if (node->brother != NULL) {
         free_xfont(node->brother);
     }
#endif /* MOTIF */
}

static void
free_font(ANN_NODE *node)
{
/*
     ANN_NODE *xnode;

     xnode = node;
     while (xnode != NULL) {
          if (xnode->children != NULL) {
               free_xfont(xnode->children);
          }
          xnode = xnode->brother;
     }
*/
}
#endif


static int
adjust_font()
{
     ANN_NODE *xnode;
     int ret, result;

     if (!useXFunc) {
         if (!annotatePlot)
            return(1);
     }
     xnode = mainNode;
     result = 0;
     while (xnode != NULL) {
          if (xnode->children != NULL) {
             if (annotatePlot)
                  ret = set_pfont(xnode->children);
             else
                  ret = set_font(xnode->children);
/*
               if (result < 1)
                   break;
*/
	     if (ret > 0)
		result = 1;
          }
          xnode = xnode->brother;
     }
     return (result);
}

static int
check_table_size(ANN_NODE *table)
{
     ANN_NODE *node;

/*
     int  mw, mh;

     mw = table->cw / table->cols;
     mh = table->ch / table->rows;
     if (mh < MINH || mw < MINW) {
        table->drawable = 0;
     }
*/
     table->drawable = 1;
     node = table->children;
     while (node != NULL) {
	node->drawable = 1;
	node = node->brother;
     }
     return (1);
}

static int
check_size(ANN_NODE *node, int w, int h) 
{
     int result, ret, mw, mh, mx, my;

     result = 0;
/*
     if (node->show > 0) {
*/
         node->drawable = 1;
         mx = (float)node->locx * gridWidth;
         my = (float)node->locy * gridHeight;
         mw = (float)node->sizeX * gridWidth;
         mh = (float)node->sizeY * gridHeight;
         if ((mx+mw) > w)
              mw = w - mx;
         if ((my+mh) > h)
              mh = h - my;
         node->cw = mw;
         node->ch = mh;
         if (node->value != NULL) {
	      if (strlen(node->value) < 1) {
		  strcpy(node->value, " ");
	      }
              if (node->orient == VERTICAL) {
                  mh = mh / (int) strlen(node->value);
                  if (mh < node->fontSize)
                      node->fontSize = mh;
              }
              else
                  mw = mw / (int) strlen(node->value);
              if (mh < MINH || mw < MINW)
              {
                 if (node->font_scale == FITONLY || node->font_scale == FIXEDFIT)
                     node->drawable = 0;
              }
         }
         if (node->children != NULL) {
             if (node->type == TABLE) {
                ret = check_table_size(node);
             }
             else
                ret = check_size(node->children, node->cw, node->ch);
         }
         else
             ret = node->drawable;
         if (ret > 0)
	     result = 1;
/*
     }
*/
     if (node->brother != NULL) {
          ret = check_size(node->brother, w, h);
	  if (ret > 0)
	      result = 1;
     }
     return(result);
}


static int
adjust_size()
{
     ANN_NODE *xnode;
     int result, ret;

     if (!useXFunc) {
	return (1);
     }
     gridWidth = (float)width / (float)grid;
     gridHeight = (float)height / (float)grid;
     xnode = mainNode;
     result = 0;
     while (xnode != NULL) {
          if (xnode->children != NULL) {
               ret = check_size(xnode->children, width, height);
		if (ret > 0)
		    result = 1;
/*
               if (result < 1)
                   break;
*/
          }
          xnode = xnode->brother;
     }
     if (result < 1)
        return (result);
/*
     result = adjust_font();
*/
     return (result);
}


static void
draw_border(ANN_NODE *node, int px, int py)
{
     int  x, y, w, h;

     if (!debug || annotatePlot)
        return;
     if (node->tableMember) {
        x = node->locx + px;
        y = node->locy + py;
        w = node->cw;
        h = node->ch;
     }
     else {
        x = node->locx * gridWidth + px;
        y = node->locy * gridHeight + py;
        w = node->sizeX * gridWidth;
        h = ((float)node->sizeY) * gridHeight;
     }
     color(BLUE);
     amove(x + org_x, org_y - y);
     rdraw(w, 0);
     rdraw(0, -h);
     rdraw(-w, 0);
     rdraw(0, h);
}

static void
draw_node(ANN_NODE *node, int px, int py)
{
     int  x, y, w, h, ascent;
     int r, g, b;
     ICON_NODE *jnode;
     ANN_NODE  *xnode;
     ANN_NODE  *snode;
     IMGNode  *gf;

     if (node->orig != NULL) 
         snode = node->orig;
     else
         snode = node;
     if (node->show > 0 && debug)
         draw_border(snode, px, py);
     if (node->show > 0 && snode->drawable && (node->value != NULL)) {
         if (annotatePlot) {
               charsize(snode->fontSize);
               ascent = ycharpixels * 0.7;
         }
         else {
               if (curColor != node->color) {
                  r = (node->color >> 16) & 0xFF;
                  g = (node->color >> 8) & 0xFF;
                  b = node->color & 0xFF;
                  set_rgb_color(r, g, b);
                  curColor = node->color;
               }
               if (snode->fontStruct != NULL)
                    set_x_font(snode->fontStruct);
               ascent = get_font_ascent();
         }
	 if (snode->tableMember) {
            x = snode->locx + px;
	    if (snode->ascent > 0)
               y = snode->locy + py + snode->ascent;
	    else
               y = snode->locy + py + ascent;
	 }
	 else {
            x = snode->locx * gridWidth + px;
            y = snode->locy * gridHeight + py + ascent;
	 }
         if (snode->orient == VERTICAL) {
               w = xcharpixels;
               h = ycharpixels * strlen(node->value);
         }
         else {
               if (annotatePlot)
                   w = xcharpixels * strlen(node->value);
   	       else
                   w = get_text_width(node->value);
               h = ycharpixels;
         }
         w = snode->cw - w;
	 if (w < 0)
	     w = 0;
         if (snode->halign == CENTER)
               x = x + w / 2;
         else if (snode->halign == RIGHT)
               x = x + w;
         else
	       x++;
	 r = snode->ch - h;
	 if (r < 0)
	       r = 0;
         if (snode->valign == CENTER)
               y = y + r / 2;
         else if (snode->valign == BOTTOM)
               y = y + r;
	 y = org_y - y;
	 if (y < 2)
	       y = 2;
         amove(x+org_x, y);
         if (snode->orient == VERTICAL) {
               vstring(node->value);
         }
         else
               dstring(node->value);
     }
#ifdef VNMRJ
     if (node->show > 0 && snode->drawable && (node->iconInfo != NULL)) {
         if (snode->imgNode != NULL) {
	     gf = node->imgNode;
	     if (snode->tableMember) {
          	  x = snode->locx + px;
            	  y = snode->locy + py;
	     }
	     else {
            	  x = snode->locx * gridWidth + px;
            	  y = snode->locy * gridHeight + py;
	     }
	     w = gf->width;
	     h = gf->height;
             if (w < snode->cw) {
            	  if (snode->halign == RIGHT)
		      x = x + snode->cw - w;
            	  else if (snode->halign == CENTER)
		      x = x + (snode->cw - w) / 2;
	     }
             if (h < snode->ch) {
            	   if (snode->valign == BOTTOM)
		      y = y + snode->ch - h;
            	   else if (snode->valign == CENTER)
		      y = y + (snode->ch - h) / 2;
	     }
	     else {
		   if (y + snode->ch >= org_y)
		      y -= 2;
	     }
	     x += org_x;
	     y = org_y - y;
	     if (y < 2)
		    y = 2;
	     sun_draw_image(gf->data, w, h,  x, y, snode->cw, snode->ch, gf->pixels, gf->bgColor);
	 }
	 else {
             jnode = (ICON_NODE *) node->iconInfo;
             if (node->tableMember) {
                  jnode->x = snode->locx + px + org_x; 
                  jnode->y = snode->locy + py + win_y; 
	     }
	     else {
                  jnode->x = snode->locx * gridWidth + px + org_x; 
                  jnode->y = snode->locy * gridHeight + py + win_y; 
	     }
             graphToVnmrJ(iconNode, node->iLen);
         }
     }
#endif
     if (node->tableMember)
	 return;
     if (node->show > 0  && (node->children != NULL)) {
	 if (node->type == TABLE) {
	     if (snode->drawable) {
             	x = snode->tx + px;
             	y = snode->ty + py;
             	xnode = node->children;
	     	while (xnode != NULL) {
		    draw_node(xnode, x, y);
		    xnode = xnode->brother;
		}
	     }
	 }
	 else {
             x = snode->locx * gridWidth + px;
             y = snode->locy * gridHeight + py;
             draw_node(node->children, x, y);
	 }
     }
     if (node->brother != NULL)
          draw_node(node->brother, px, py);
}

#ifdef OLD
static int
getParamVal(char *param, int type, char *retVal)
{
    int   ival, len;
    char  *sVal;
    char  tmpStr[64];

    if (aipKey == NULL)
       return (0);
    ival = 0; 
    sprintf(tmpStr, "annPar('%s'):$VALUE", param);
    sVal = (char *)AnnotateExpression(tmpStr, NULL);
    if (sVal != NULL) {
        tmpStr[0] = '\0';
        sscanf(sVal,"%*s%s",tmpStr);
        len = strlen(tmpStr);
        if (type == 1) {
           if (len > 0)
               ival = atoi(tmpStr);
        }
        else {   /* string */
	   if (len > 30)
 		len = 30;
           for (ival = 0; ival < len; ival++) {
		retVal[ival] = tolower(tmpStr[ival]);
  	   }
	   retVal[len] = '\0';
        }
    }
    else {
        if (retVal != NULL) {
	   retVal[0] = ' ';
	   retVal[1] = '\0';
	}
    }
    return (ival);
}
#endif

static char orientLabel[8] = "LRAPSI";

static void
set_orientation(int id)
{
    double V[6][3];  /* L, R, A, P, S, I */
    double R[3];
    double *T, *N;
    double r,d;
    int i, k, j, z1,z2;

    if (orientNodes == NULL)
	return;

    V[0][0] = -1;  V[0][1] = 0;  V[0][2] = 0;  /* L */
    V[1][0] = 1; V[1][1] = 0;  V[1][2] = 0;  /* R */
    V[2][0] = 0;  V[2][1] = 1;  V[2][2] = 0; /* A */
    V[3][0] = 0;  V[3][1] = -1; V[3][2] = 0;  /* P */
    V[4][0] = 0;  V[4][1] = 0;  V[4][2] = 1;  /* S */
    V[5][0] = 0;  V[5][1] = 0;  V[5][2] = -1;  /* I */

    aipGetImageInfo(id, &imageInfo);
/* rotate annotation labels from body to magnet. */
    for (k = 0; k < 6; k++) {
        T = V[k];
        for (i = 0; i < 3; i++) {
           N = imageInfo.b2m[i];
           R[i] = N[0] * T[0] + N[1] * T[1] + N[2] * T[2];
        }
        for (i = 0; i < 3; i++) {
           T[i] = R[i];
        }
    }

/**
    if (P_getstring(CURRENT, "aipRotationPolicy", inputs, 1, 20) < 0)
    {
       if (P_getstring(SYSTEMGLOBAL, "aipRotationPolicy", inputs, 1, 20) < 0)
         inputs[0] = '\0';
    }
    if (strcmp(inputs, "neurological") == 0) {
        V[0][0] = 1;
        V[1][0] = -1;
    }
**/
    r = imageInfo.pixelsPerCm;
    if (r == 0)
        r = 1;
    for (k = 0; k < 3; k++) {
        for (i = 0; i < 3; i++)  {
            if (k == 1)
                imageInfo.m2p[k][i] = -imageInfo.m2p[k][i] / r;
            else
                imageInfo.m2p[k][i] = imageInfo.m2p[k][i] / r;
        }
    }

    for (k = 0; k < 6; k++) {
        T = V[k];
        for (i = 0; i < 3; i++) {
           N = imageInfo.m2p[i];
           R[i] = N[0] * T[0] + N[1] * T[1] + N[2] * T[2];
        }
        for (i = 0; i < 3; i++) {
           T[i] = R[i];
        }
    }
    /*  orientation label position  0: left  1: right  2: top  3: bottom  */
    for (k = 0; k < 4; k++) {
        if (orientNodes[k]->value != NULL)
           strcpy(orientNodes[k]->value, " ");
    }

    /* set the two vectors with largest z value to (0,0,1). */
    d = 0;
    z1 = z2 = 0;
    for (k = 0; k < 6; k++) {
        if(fabs(V[k][2]) > d) {
            d = fabs(V[k][2]);
            z1 = k;
        }
    }
    V[z1][0] = 0; 
    V[z1][1] = 0; 
    V[z1][2] = 1; 
    d = 0;
    for (k = 0; k < 6; k++) {
        if(k != z1 && fabs(V[k][2]) > d) {
            d = fabs(V[k][2]);
            z2 = k;
        }
    }
    V[z2][0] = 0; 
    V[z2][1] = 0; 
    V[z2][2] = 1; 

    /* determine the plane */
    if((z1 == 0 && z2 == 1) || (z1 == 1 && z2 == 0))
        j = 2; /*sag*/  
    else if((z1 == 2 && z2 == 3) || (z1 == 3 && z2 == 2))
        j = 1; /*cor*/ 
    else if((z1 == 4 && z2 == 5) || (z1 == 5 && z2 == 4))
        j = 0; /*trans*/ 
    else {
        fprintf(stderr,"don't know how to place orientation labels\n.");
        return;
    }
	
    /* r is for the round off error */
    r = 1.0e-4;
    for (k = 0; k < 6; k++) {
        d = fabs(V[k][0]) - fabs(V[k][1]);	
        if (fabs(V[k][2])+r < 1) {
            i = -1;
            if(d < -r && V[k][1] > r) /* x=0  y=1 */
                i = 2;
            else if(d < -r && V[k][1] < -r) /* x=0  y=-1 */
                i = 3;
            else if(d > r && V[k][0] > r) /* x=1  y=0 */
                i = 1;
            else if(d > r && V[k][0] < -r) /* x=-1  y=0 */
                i = 0;
            else if(j == 0) {
                /* case of |x| == |y| */
                if(V[k][0] > 0 && V[k][1] > 0)
                    i = 1;
                else if(V[k][0] < 0 && V[k][1] > 0)
                    i = 2;
                else if(V[k][0] < 0 && V[k][1] < 0)
                    i = 0;
                else if(V[k][0] > 0 && V[k][1] < 0)
                    i = 3;
            } else if(j == 1) {
	            /* the same as j == 0. */
                if(V[k][0] > 0 && V[k][1] > 0)
                    i = 1;
                else if(V[k][0] < 0 && V[k][1] > 0)
                    i = 2;
                else if(V[k][0] < 0 && V[k][1] < 0)
                    i = 0;
                else if(V[k][0] > 0 && V[k][1] < 0)
                    i = 3;
            } else if(j == 2) {
	            /* the same as j == 0. */
                if(V[k][0] > 0 && V[k][1] > 0)
                    i = 1;
                else if(V[k][0] < 0 && V[k][1] > 0)
                    i = 2;
                else if(V[k][0] < 0 && V[k][1] < 0)
                    i = 0;
                else if(V[k][0] > 0 && V[k][1] < 0)
                    i = 3;
            } 
            if (i != -1 && orientNodes[i]->value != NULL)
               sprintf(orientNodes[i]->value, "%c", orientLabel[k]);
        }
    }
}



#ifdef OLD

static void
set_orientation()
{
    int psi, phi, theta;
    char pos1[32], pos2[32];
    int k, i, a;
    double  tmp;
    double rad_per_deg;
    double cospsi;
    double sinpsi;
    double cosphi;
    double sinphi;
    double costheta;
    double sintheta;

    double d0;
    double d;

    double L[3], R[3], A[3], P[3], S[3], I[3]; 
    double V1[3], V2[3], V3[3], V4[3];
    double tR[3][3];
    double Vp[4][3];

    L[0] = -1; L[1] = 0;  L[2] = 0;
    R[0] = 1;  R[1] = 0;  R[2] = 0;
    A[0] = 0;  A[1] = 1;  A[2] = 0;
    P[0] = 0;  P[1] = -1; P[2] = 0;
    S[0] = 0;  S[1] = 0;  S[2] = -1;
    I[0] = 0;  I[1] = 0;  I[2] = 1;
    V1[0] = 0;  V1[1] = 1;  V1[2] = 0;
    V2[0] = 0;  V2[1] = -1; V2[2] = 0;
    V3[0] = -1; V3[1] = 0;  V3[2] = 0;
    V4[0] = 1;  V4[1] = 0;  V4[2] = 0;

    psi = getParamVal("psi", 1, NULL);
    phi = getParamVal("phi", 1, NULL);
    theta = getParamVal("theta", 1, NULL);
    getParamVal("position1", 2, pos1);
    getParamVal("position2", 2, pos2);
    if (strncpy(pos1, "feet", 4) == 0 ) {  /* Feet first, swap S/I and L/R */
        for (k = 0; k < 3; k++) {
            tmp = S[k];  S[k] = I[k]; I[k] = tmp;
            tmp = L[k];  L[k] = R[k]; R[k] = tmp;
        }
    }
    if (strncpy(pos2, "prone", 5) == 0) {  /* Pron, swap A/P and L/R */
        for (k = 0; k < 3; k++) {
            tmp = A[k];  A[k] = P[k]; P[k] = tmp;
            tmp = L[k];  L[k] = R[k]; R[k] = tmp;
        }
    }
    if (strncpy(pos2, "left", 4) == 0) {  /* Left side */
        if (strncpy(pos1, "head", 4) == 0) { /* Head first, rotate conuterclockwise */
            for (k = 0; k < 3; k++) {
                tmp = L[k];  L[k] = A[k]; A[k] = R[k];  
 		R[k] = P[k];  P[k] = tmp;
            }
        }
        else {  /* Feet first, rotate clockwise */
            for (k = 0; k < 3; k++) {
                tmp = P[k];  P[k] = R[k]; R[k] = A[k];  
                A[k] = L[k]; L[k] = tmp;
            }
        }
    }
    if (strncpy(pos2, "right", 5) == 0) {  /* Right side */
        if (strncpy(pos1, "head", 4) == 0) { /* Head first, conuterclockwise */
            for (k = 0; k < 3; k++) {
                tmp = P[k];  P[k] = R[k]; R[k] = A[k];  
                A[k] = L[k]; L[k] = tmp;
            }
        }
        else {  /* Feet first, rotate clockwise */
            for (k = 0; k < 3; k++) {
                tmp = L[k];  L[k] = A[k]; A[k] = R[k];  
                R[k] = P[k]; P[k] = tmp;
            }
        }
    }
    rad_per_deg = 3.14159265358979323846/180.0;
    cospsi = cos(psi*rad_per_deg);
    sinpsi = sin(psi*rad_per_deg);
    cosphi = cos(phi*rad_per_deg);
    sinphi = sin(phi*rad_per_deg);
    costheta = cos(theta*rad_per_deg);
    sintheta = sin(theta*rad_per_deg);

    tR[0][0] = (sinphi*cospsi - cosphi*costheta*sinpsi);
    tR[0][1] = (-1.0*cosphi*cospsi - sinphi*costheta*sinpsi);
    tR[0][2] = (sinpsi*sintheta);

    tR[1][0] = (-1.0*sinphi*sinpsi - cosphi*costheta*cospsi);
    tR[1][1] = (cosphi*sinpsi - sinphi*costheta*cospsi);
    tR[1][2] = (cospsi*sintheta);

    tR[2][0] = (sintheta*cosphi);
    tR[2][1] = (sintheta*sinphi);
    tR[2][2] = (costheta);

    Vp[0][0] = tR[0][0]*V1[0] + tR[0][1]*V1[1] + tR[0][2]*V1[2];
    Vp[0][1] = tR[1][0]*V1[0] + tR[1][1]*V1[1] + tR[1][2]*V1[2];
    Vp[0][2] = tR[2][0]*V1[0] + tR[2][1]*V1[1] + tR[2][2]*V1[2];

    Vp[1][0] = tR[0][0]*V2[0] + tR[0][1]*V2[1] + tR[0][2]*V2[2];
    Vp[1][1] = tR[1][0]*V2[0] + tR[1][1]*V2[1] + tR[1][2]*V2[2];
    Vp[1][2] = tR[2][0]*V2[0] + tR[2][1]*V2[1] + tR[2][2]*V2[2];

    Vp[2][0] = tR[0][0]*V3[0] + tR[0][1]*V3[1] + tR[0][2]*V3[2];
    Vp[2][1] = tR[1][0]*V3[0] + tR[1][1]*V3[1] + tR[1][2]*V3[2];
    Vp[2][2] = tR[2][0]*V3[0] + tR[2][1]*V3[1] + tR[2][2]*V3[2];

    Vp[3][0] = tR[0][0]*V4[0] + tR[0][1]*V4[1] + tR[0][2]*V4[2];
    Vp[3][1] = tR[1][0]*V4[0] + tR[1][1]*V4[1] + tR[1][2]*V4[2];
    Vp[3][2] = tR[2][0]*V4[0] + tR[2][1]*V4[1] + tR[2][2]*V4[2];

    orientInfo[4] = '\0';
    for (i = 0; i < 4; i++) {
       d0 = -10.0;
       orientInfo[i] = ' ';
       d = Vp[i][0]*L[0] + Vp[i][1]*L[1] + Vp[i][2]*L[2];
       if (d > d0) {
         d0 = d;  orientInfo[i] = 'L';
       }
       d = Vp[i][0]*R[0] + Vp[i][1]*R[1] + Vp[i][2]*R[2];
       if (d > d0) {
         d0 = d;  orientInfo[i] = 'R';
       }
       d = Vp[i][0]*A[0] + Vp[i][1]*A[1] + Vp[i][2]*A[2];
       if (d > d0) {
         d0 = d;  orientInfo[i] = 'A';
       }
       d = Vp[i][0]*P[0] + Vp[i][1]*P[1] + Vp[i][2]*P[2];
       if (d > d0) {
         d0 = d;  orientInfo[i] = 'P';
       }
       d = Vp[i][0]*S[0] + Vp[i][1]*S[1] + Vp[i][2]*S[2];
       if (d > d0) {
         d0 = d;  orientInfo[i] = 'S';
       }
       d = Vp[i][0]*I[0] + Vp[i][1]*I[1] + Vp[i][2]*I[2];
       if (d > d0) {
         d0 = d;  orientInfo[i] = 'I';
       }
       if (orientNodes[i] != NULL) {
          if (orientNodes[i]->value != NULL)
           sprintf(orientNodes[i]->value, "%c", orientInfo[i]);
       }
    }
}

#endif

static void
disp_orientation()
{
    int k, i, a, r, g, b;

    if (orientNodes[0] != NULL) /* new style */
        return;
    if (checkNode == NULL)
        return;
    r = (checkNode->color >> 16) & 0xFF;
    g = (checkNode->color >> 8) & 0xFF;
    b = checkNode->color & 0xFF;

    if (annotatePlot) {
         charsize(checkNode->fontSize);
    }
    else {
         set_rgb_color(r, g, b);
         if (checkNode->fontStruct != NULL)
               set_x_font(checkNode->fontStruct);
    }
    if (annotatePlot) {
       a = ycharpixels * 0.7;
       b = ycharpixels * 0.3;
    }
    else {
       a = get_font_ascent() + 2;
       b = get_font_descent() + 1;
    }
    k = (width - xcharpixels) / 2;
    i = (height - ycharpixels) / 2;
    amove(org_x + k, org_y - a);
    dchar(orientInfo[2]);
    amove(org_x + 2, org_y - i);
    dchar(orientInfo[0]);
    amove(org_x + width - xcharpixels - 1 , org_y - i);
    dchar(orientInfo[1]);
    amove(org_x + k, b);
    dchar(orientInfo[3]);
}


static void
paint()
{
     ANN_NODE *xnode;

     xnode = startNode;
     if (checkNode != NULL) {
          checkNode->value = NULL;
     } 
     curColor = 0;
     while (xnode != NULL) {
          if (xnode->children != NULL) {
               draw_node(xnode->children, 2, 2);
          }
          xnode = xnode->brother;
     }
}

static void
jdraw_node(ANN_NODE *node)
{
     int  k;
     ANN_NODE  *xnode;

     k = 0;
     jInfo.id = htonl(node->nodeId);
     jInfo.p1 = htonl(0);
     if (node->show > 0) {
         jInfo.p1 = htonl(1);
         if (node->setVal != NULL) {
            if (node->value != NULL) {
                k = strlen(node->value);
                if (k > 255) {
                    k = 255;
                    strncpy(jInfo.info, node->value, k);
                }
                else
                    strcpy(jInfo.info, node->value);
            }
            else {
                k = 1;
                strcpy(jInfo.info, " ");
            }
        }
     }
     jInfo.len = htonl(k);
     graphToVnmrJ(&jInfo, k + jHLen);

     if (node->tableMember)
        return;
     if (node->show > 0  && (node->children != NULL)) {
        if (node->type == TABLE) {
            xnode = node->children;
            while (xnode != NULL) {
                jdraw_node(xnode);
                xnode = xnode->brother;
            }
        }
        else {
             jdraw_node(node->children);
        }
     }
     if (node->brother != NULL)
          jdraw_node(node->brother);
}

static void
jpaint()
{
     ANN_NODE *xnode;

     jInfo.code = htonl(3);
     jInfo.p0 = htonl(debug);

     xnode = startNode;
     if (checkNode != NULL) {
          checkNode->value = NULL;
     } 
     while (xnode != NULL) {
          if (xnode->children != NULL) {
               jdraw_node(xnode->children);
          }
          xnode = xnode->brother;
     }
}


#ifdef OLD
static char
*getAipVal(char *param, char *format)
{
    char *p, *p2, *p3, *fp;
    int  ival, ret, len, index;
    double  dval;
    char  *sVal;
    char  *retVal;
    char  tmpStr[64];

    fp = NULL;
    if (format != NULL) {
       if (isReal(format)) {
          fp = format;
          while (*fp == ' ' || *fp == '\t')
             fp++;
          p = fp;
          while (*p != '\0') {
             if (*p == ' ') {
                *p = '\0';
                break;
             }
             p++;
          } 
       }
    }
    p = param;
    while (*p != '\0') { /* remove '$VALUE'  */
       if (*p == '=') {
          p++;
          break;
       } 
       p++;
    }
    if (*p == '\0')
       p = param;
    while (*p == ' ' || *p == '\t')  p++;
    p2 = p;
    while (*p2 != '\0') {
       if (*p2 == '+' || *p2 == '-')
          return (NULL);
       if (*p2 == '*' || *p2 == '/')
          return (NULL);
       p2++;
    }
    p2 = p;
    while (*p2 != '\0') {
       if (*p2 == ' ' || *p2 == '\n') {
          *p2 = '\0';
          break;
       }
       p2++;
    }
    if (aipKey == NULL)
       return (p);
    index = -1; /* not array */
    p3 = NULL;
    p2 = p;
    while (*p2 != '\0') {
       if (*p2 == '[') {
           p3 = p2 + 1;
           *p2 = '\0';
       }
       if (*p2 == ']') {
           *p2 = '\0';
       }
       p2++;
    }
    if (p3 != NULL && strlen(p3) > 0) {
       index = atoi(p3);
    }

    ret = aipGetHdrStr(aipKey, p, index, &sVal);
    if (ret == 0) {
        if (sVal != NULL)
           return (sVal);
    }

    ret = aipGetHdrReal(aipKey, p, index, &dval);
    if (ret == 0) {
        if (fp == NULL) 
            sprintf(tmpStr, "%g", dval);
        else {
            p2 = fp;             
            while (*p2 != '\0') {
               if (*p2 == '.') {
                  ret = 1;
                  break;
               }
               p2++;
            }
            if (ret > 0)
               sprintf(attrName, "%%%sf", fp);
            else
               sprintf(attrName, "%%.%sf", fp);
            sprintf(tmpStr, attrName, dval);
        }
        retVal = annAllocate(strlen(tmpStr) + 1);
        strcpy(retVal, tmpStr);
        return (retVal);
    }
    ret = aipGetHdrInt(aipKey, p, index, &ival);
    if (ret == 0) {
        sprintf(tmpStr, "%d", ival);
        retVal = annAllocate(strlen(tmpStr) + 1);
        strcpy(retVal, tmpStr);
        return (retVal);
    }
/*
    ret = aipGetHdrStr(aipKey, p, index, &sVal);
    if (ret == 0) {
         retVal = sVal;
    }
    else {
        retVal = annAllocate(6);
        sprintf(retVal, "null");
    }
*/
    retVal = annAllocate(6);
    sprintf(retVal, "null");
    return (retVal);
}
#endif

static void
set_node_val(ANN_NODE *node)
{
     int   len;
     char  *d, *tok;
     ANN_NODE *snode;

#ifdef VNMRJ

     if (node->showStr != NULL) {
/*
        d = (char *)AnnotateShow(node->showStr);
*/
        d = (char *)AnnotateExpression(node->showStr, NULL);
        if (d != NULL) {
            sscanf(d,"%*s%s",inputs);
            if (inputs[0] != 'E')
               node->show = atoi(inputs);
            else
               node->show = 0;
        }
     }
     len = 1;
     if (node->setVal != NULL) {
        d = (char *)AnnotateExpression(node->setVal, node->digits);
/*
        d = (char *)getAipVal(node->setVal, node->digits);
*/
        if (d != NULL) {
            while (*d == ' ' || *d == '\t')
                d++;
/*
            sscanf(d,"%*s%s",inputs);
*/
            if ((tok = strstr(d, " ")) != NULL) {
                while (*tok == ' ' || *tok == '\t')
                    tok++;
                strcpy(inputs, tok);
                if (strlen(inputs) < 1)
                    strcpy(inputs, " ");
	    }
            else
                strcpy(inputs, " ");
            len = (int)strlen(inputs);
            if (node->vLen < len) {
                node->vLen = len;
                node->value = annAllocate(len + 2);
            }
            strcpy(node->value, inputs);
        }
        else {
           if (node->value != NULL)
              strcpy(node->value, " ");
        }
     }
     if (node->orig != NULL && node->value != NULL && node->show) {
        snode = node->orig;
        if (snode->vLen < len) {
            snode->vLen = len;
            new_str_len = 1;
            snode->value = node->value;
        }
     }
     if (node->children != NULL) {
          set_node_val(node->children);
     }
     if (node->brother != NULL)
          set_node_val(node->brother);
#endif
}

static void
set_value()
{
     ANN_NODE *xnode;

     xnode = startNode;
     while (xnode != NULL) {
          if (xnode->children != NULL) {
               set_node_val(xnode->children);
          }
          xnode = xnode->brother;
     }
}

static void
set_table(ANN_NODE *node)
{
     ANN_NODE *xnode;
     int k;

     if (node->type == TABLE) {
	xnode = node->children;
        while (xnode != NULL) {
	    xnode->tx = (int) xnode->locx;
	    xnode->ty = (int) xnode->locy;
	    xnode->locx = 0;
	    xnode->locy = 0;
	    xnode->tableMember = 1;
	    xnode = xnode->brother;
	}
	if (node->dock != NULL) {
	    k = sscanf(node->dock,"%s%s",inputs, attrName);
	    node->valign = 0;
	    node->halign = 0;
	    if (k == 2) {
		if (inputs[0] == 'U')
		    node->valign = TOP;
		else if (inputs[0] == 'M')
		    node->valign = CENTER;
		else if (inputs[0] == 'L')
		    node->valign = BOTTOM;

		if (attrName[0] == 'C')
		    node->halign = CENTER;
		else if (attrName[0] == 'R')
		    node->halign = RIGHT;
		else if (attrName[0] == 'L')
		    node->halign = LEFT;
	    }
	}
        else
	    node->dock = "n";
     }
     else if (node->children != NULL)
          set_table(node->children);
     if (node->brother != NULL)
          set_table(node->brother);
}

static void
releaseIcon()
{
     int      k, k2, k3;
     IMGNode  *nd, *pnd, *nd2;

     if (imgList == NULL)
	return;
     nd = imgList;
     k = 0;
     k2 = 0;
     k3 = 0;
     while (nd != NULL) {
	k3++;
	if (nd->used != 0) {
	    if (stat(nd->fpath, &fileStat) >= 0) {
                if (fileStat.st_mtime != nd->mtime) {
		    nd->used = 0;
		    k2++;
		}
	    }
	}
	else
	    k++;
	nd = nd->next;
     }
     if (k2 > 0 || k > 2) {
        nd = imgList;
        pnd = NULL;
        while (nd != NULL) {
	    nd2 = nd->next;
	    if (nd->used == 0) {
		if (pnd != NULL)
		    pnd->next = nd2;
		else  /* nd == imgList */
		    imgList = nd2;
		if (nd2 != NULL)
		    nd2->prev = pnd;
	        releaseGifNode(nd);
	    }
	    else
		pnd = nd;
	    nd = nd2;
	}
     }
     nd = imgList;
     while (nd != NULL) {
	nd->used = 0;
	nd = nd->next;
     }
}

static void
createIcon(ANN_NODE *node)
{
     IMGNode  *nd;
     char   *s;
     int    a;

     
     if (node->icon == NULL)
         return;
     a = sizeof(int) * 9 + (int) strlen(node->icon) + 2;
     s = (char *) annAllocate(a);
     if (s == NULL)
         return;
     node->iconInfo = s;
     node->iLen = a;
     s = (char *) (s + sizeof(int) * 9);
     strcpy(s, node->icon);

     if (!useXFunc)
         return;

     nd = imgList;
     while (nd != NULL) {
	if (strcmp(nd->name, node->icon) == 0) {
	    nd->used = 1;
	    node->imgNode = nd;
	    return;
	}
	nd = nd->next;
     }
     nd = open_imagefile(node->icon, 0);
     if (nd == NULL)
	return;
     node->imgNode = nd;
     if (imgList != NULL)
	imgList->prev = nd;
     nd->next = imgList;
     nd->prev = NULL;
     nd->used = 1;
     imgList = nd;
}


static void
buildNode(FILE *fd)
{
     int type, found;
     int hasTable;
     char *data;
     ANN_NODE *node;

     grid = 40;

     if (fd == NULL)
         return;
     new_template = 1;
     releaseIcon();
     for (found = 0; found < 4; found++)
         orientNodes[found] = NULL;
     show_orientation = 0;
     startNode = new_node(0);
     curNode = startNode;
     mainNode = startNode;
     checkNode = NULL;
     table_col_size = 0;
     table_row_size = 0;
     hasTable = 0;
     objId = 1;
     while (fgets(inputs, 250, fd) != NULL)
     {
         type = get_type(inputs);
         if (type <= 0) 
             continue;
         found = 0;
         switch (type) {
            case BOX:
            case LABEL:
            case ATEXT:
                     add_node(type);
                     found = 1;
                     break;
            case TABLE:
                     add_node(type);
                     found = 1;
                     hasTable = 1;
                     break;
            case CHECK:
                     add_node(type);
                     found = 1;
                     checkNode = curNode;
                     checkNode->sizeX = 10;
                     checkNode->sizeY = 10;
                     break;
            case XEND:
                     if (curNode != NULL && curNode->parent != NULL)
                        curNode = curNode->parent;
                     found = 1;
                     break;
            case GRID:
                     found = 1;
                     data = get_value();
                     if (data != NULL)
   			grid = atoi(data); 
                     if (grid <= 10)
                        grid = 10;
                     break;
            case SHOWORIENT:
                     found = 1;
                     data = get_value();
                     show_orientation = 0;
                     if (data != NULL) {
                        if (strcmp(data, "yes") == 0)
                          show_orientation = 1;
                     }
                     break;
         }
         if (found || curNode == NULL) {
             continue;
         }
         data = get_value();
         if (data == NULL)
             continue;
         switch (type) {
            case VALUE:
                     curNode->value = data; 
                     break;
            case SHOW:
                     curNode->showStr = data; 
                     break;
            case UNITS:
                     curNode->units = atoi(data); 
                     break;
            case SETVAL:
                     curNode->setVal = data; 
                     break;
            case DIGITS:
                     curNode->digits = data; 
                     break;
            case HALIGN:
                     if (data != NULL) {
                        if (strcmp(data, "Left") == 0)
                           curNode->halign = LEFT; 
                        else if (strcmp(data, "Right") == 0)
                           curNode->halign = RIGHT; 
		        else
                           curNode->halign = CENTER; 
                     }
                     break;
            case VALIGN:
                     if (data != NULL) {
                        if (strcmp(data, "Top") == 0)
                           curNode->valign = TOP; 
                        else if (strcmp(data, "Bottom") == 0)
                           curNode->valign = BOTTOM; 
		        else
                           curNode->valign = CENTER; 
                     }
                     break;
            case ORIENT:
                     if (data != NULL) {
                       if (strcmp(data, "Vertical") == 0)
                          curNode->orient = VERTICAL; 
		       else
                          curNode->orient = 0; 
    		     } 
                     break;
            case ROW:
                     curNode->sizeY = atoi(data); 
                     break;
            case COL:
                     curNode->sizeX = atoi(data); 
                     break;
            case LOCX:
                     curNode->locx = atoi(data); 
                     break;
            case LOCY:
                     curNode->locy = atoi(data); 
                     break;
            case ICON:
                     curNode->icon = data; 
		     if (data != NULL)
			createIcon(curNode);
                     break;
            case PSFONT:
                     curNode->psfont = data; 
                     break;
            case FONTH:
                     curNode->def_fontSize = atoi(data) + 2; 
                     curNode->fontSize = curNode->def_fontSize;
                     break;
            case COLOR:
                     curNode->color = atoi(data); 
                     break;
            case DOCK:
                     curNode->dock = data; 
                     break;
            case FONTNAME:
		     curNode->fontType = get_fontIndex(data);
                     curNode->fontName = vFonts[curNode->fontType];
                     break;
            case FONTSTYLE:
                     if (strcmp(data, "bold") == 0)
                         curNode->bold = 1;
                     else if (strcmp(data, "italic") == 0)
                         curNode->italic = 1;
                     break;
            case ORIENTID:
                     found = atoi(data); 
      		     if (found >= 1 && found <= 4) {
                        show_orientation = 1;
                        orientNodes[found-1] = curNode;
			curNode->orientId = found;
			curNode->halign = CENTER;
			curNode->valign = CENTER;
			if (found == 1) {
			   curNode->halign = LEFT;
                        }
			else if (found == 2) {
			   curNode->halign = RIGHT;
                        }
			else if (found == 3) {
			   curNode->valign = TOP;
                        }
			else if (found == 4) {
			   curNode->valign = BOTTOM;
                        }
  		     }
                     break;
            case ELASTIC:
                     if (data != NULL) {
                        if (strcmp(data, "Variable") == 0)
			   curNode->font_scale = SCALABLE;
                        else if (strcmp(data, "Fixed") == 0)
			   curNode->font_scale = FIXED;
                        else if (strcmp(data, "Fixed and Fit") == 0)
			   curNode->font_scale = FIXEDFIT;
  		     }
                     break;
            case ROWS:
                     curNode->rows = atoi(data); 
                     break;
            case COLS:
                     curNode->cols = atoi(data); 
                     break;
            case ICONW:
                     curNode->iconW = atoi(data); 
                     break;
            case ICONH:
                     curNode->iconH = atoi(data); 
                     break;
            case IDNUM:
                     curNode->nodeId = atoi(data); 
                     break;
         }
     }
     if (hasTable) {
         node = startNode;
         while (node != NULL) {
	     if (node->children != NULL) {
                 set_table(node->children);
             }
             node = node->brother;
	}
     }

}

static void
dup_node(ANN_NODE *snode, ANN_NODE *dnode, int type)
{
    ANN_NODE *node;

    node = new_node(snode->type);
    bcopy(snode, node, sizeof(ANN_NODE));
    node->vLen = 0;
    node->fontName = snode->fontName;
    node->imgNode = snode->imgNode;
    node->iconInfo = snode->iconInfo;
    node->iLen = snode->iLen;
    // node->nodeId = snode->nodeId;
    node->children = NULL;
    node->brother = NULL;
    node->parent = dnode;
    node->orig = snode;
    if (node->orientId > 0) {
	orientNodes[node->orientId - 1] = node;
        node->value = (char *) annAllocate (2);
	strcpy(node->value, " ");
        node->vLen = 1;
    }
    if (type == 0) /* add new child) */
    {
	dnode->children = node;
    }
    else
	dnode->brother = node;
    if (snode->children != NULL)
        dup_node(snode->children, node, 0);
    if (snode->brother != NULL)
        dup_node(snode->brother, node, 1);
}

static void
dup_template_node(HEAD_NODE *hnode)
{
    int k;
    ANN_NODE *node, *xnode;

    if (hnode == NULL)
	return;
    node = mainNode;
    xnode = new_node(0);
    hnode->start = xnode;
    for (k = 0; k < 4; k++)
         orientNodes[k] = NULL;
    xnode->orig = node;
    while (node != NULL) {
        if (node->children != NULL) {
               dup_node(node->children, xnode, 0);
        }
	if (node->brother != NULL)
            dup_node(node->brother, xnode, 1);
        node = node->brother;
        xnode = xnode->brother;
    }
}


static void
*get_headnode(int n, int id)
{
    int k, s, m;
    HEAD_NODE **newHead;
    HEAD_NODE *node;

    if (headNodeSize <= n) {
    	if (n < 20)
           m = n + 12;
    	else
           m = n + 6;
        k = sizeof(HEAD_NODE *);
        newHead = (HEAD_NODE **) calloc(m, k);
    	if (newHead == NULL)
	   return(NULL);
       	if (headNode != NULL) {
           for (s = 0; s < headNodeSize; s++) {
          	newHead[s] = headNode[s];
           }
           free(headNode);
    	}
        for (s = headNodeSize; s < m; s++)
           newHead[s] = NULL;
        headNode = newHead;
        headNodeSize = m;
    }
    for (k = n; k < headNodeSize; k++) {
	if (headNode[k] != NULL) {
	    if (headNode[k]->imgId == id)
		return (headNode[k]);
	}
    }
    if (headNode[n] == NULL) {
        node = (HEAD_NODE *) malloc(sizeof(HEAD_NODE));
        headNode[n] = node;
        node->newTemp = 1;
        node->keyLen = 0;
        node->imgId = 0;
        node->aipKey = NULL;
        node->start = NULL;
    }
    return (headNode[n]);
}

static int
check_template(char *name, char *path, int isUser)
{
    if (path != NULL) {
       if (isUser)
          sprintf(tempPath, "%s/templates/vnmrj/annotation/%s.txt",path,name);
       else
          sprintf(tempPath, "%s/imaging/templates/vnmrj/annotation/%s.txt",path,name);
    }
    else
       sprintf(tempPath, "%s", name);
    if (stat(tempPath, &fileStat) >= 0) {
        if (strcmp(tempPath, oldPath) == 0) {
            if (fileStat.st_mtime == old_mtime)
		return (1);
	}
        return (0);
    }
    return (-1);
}

static void
check_aipAnnotation()
{
    int   k;
    char  *tname;
    FILE  *fin;

    inputs[0] = '\0';
    isActive = 0;
    new_template = 0;
    if (P_getstring(CURRENT,"aipAnnotation",inputs,1,250) < 0)
    {
        if (P_getstring(GLOBAL,"aipAnnotation",inputs,1,250) < 0)
           return;
    }
    if (inputs[0] == '\0')
        return;
    tname = inputs;
    while (*tname == ' ' || *tname == '\t')
        tname++;
    if ((int) strlen(tname) < 1)
        return;
    if ((strcmp(tname, "none") == 0) || (strcmp(tname, "None") == 0))
        return;
    fin = NULL;
    if (*tname == '/') {
	if ((isActive = check_template(tname, NULL, 1)) > 0) {
	    return;
	}
	if (isActive == 0)
            fin = fopen(tempPath,"r");
    }
    if (fin == NULL) {
	if ((isActive = check_template(tname, userdir, 1)) > 0)
	    return;
	if (isActive == 0)
            fin = fopen(tempPath,"r");
    }
    if (fin == NULL) {
	if ((isActive = check_template(tname, systemdir, 0)) > 0)
	    return;
	if (isActive == 0)
            fin = fopen(tempPath,"r");
    }
    isActive = 0;
    if (fin == NULL)
	return;
    old_mtime = fileStat.st_mtime;

    strcpy(oldPath, tempPath);
    for (k = 0; k < headNodeSize; k++) {
        if (headNode[k] != NULL) {
/*
	       free_font(headNode[k]->start);
*/
               headNode[k]->newTemp = 1;
               headNode[k]->start = NULL;
               headNode[k]->w = 0;
               headNode[k]->h = 0;
	}
    }
    releaseWithId("annotate"); 
    mainNode = NULL;
    startNode = NULL;
    free_font_list();
    table_col_size = 0;
    table_row_size = 0;
    frame_width = 0;
    frame_height = 0;
    orientNodes = defOrientNodes;
    buildNode(fin);
    fclose(fin);
    isActive = 1;
    if (!useXFunc) {
        jInfo.code = htonl(1);
        strcpy(jInfo.info, oldPath);
	k = strlen(jInfo.info);
        tname = jInfo.info + k - 4;
	strcpy(tname, ".xml");
        jInfo.len = htonl(k);
	k = k + jHLen;
        graphToVnmrJ(&jInfo, k);
    }
}

int drawAnnotation(const char *key, int x, int y, int w, int h,
                   int frameNum, int frameId, int newFlag)
{
    int  newKey, newSize;
    int  i, k;

    (void) newFlag;
    if (w < 50 || h < 50) {
        if (printErr)
           Werrprintf("Window size is too small for annotation.");
        printErr = 0;
        RETURN;
    }
    if (key == NULL || frameNum < 1 || frameId < 0)
        RETURN;
    if (preview)
        RETURN;
    aipKey = key;

/*
    strcpy(attrName, "aipWindowSplit");
    if (P_getreal(GLOBAL, attrName, &v, 1)) {
        if (P_getreal(CURRENT, attrName, &v, 1))
	    v = 0;
    }
    r = (int) v;
    if (P_getreal(GLOBAL, attrName, &v, 2)) {
        if (P_getreal(CURRENT, attrName, &v, 2))
	    v = 0;
    }
    c = (int) v;
*/
    if (!useXFunc) {
        x = x + 2;
        y = y + 2;
        w = w - 4;
        h = h - 2;
        jInfo.type = htonl(9);
        jInfo.id = htonl(frameNum);
        jInfo.p0 = htonl(x);
        jInfo.p1 = htonl(y);
        jInfo.p2 = htonl(w);
        jInfo.p3 = htonl(h);
        jInfo.len = htonl(0);
        jHLen = sizeof(int) * 8;
    }

    if (frameNum < 3 || mainNode == NULL)
        check_aipAnnotation();
    if (mainNode == NULL || !isActive)
        RETURN;
    annotatePlot = 0;
    debug = 0;
    curHnode = get_headnode(frameNum, frameId);
    if (curHnode == NULL)
        RETURN;
    if (!useXFunc) {
        jInfo.len = htonl(0);
    }

    orientInfo = curHnode->orientInfo;
    orientNodes = curHnode->orientNodes;
/*
    setdisplay();
*/
    newSize = new_template;
    if (w != frame_width || h != frame_height) {
        if (abs(w - frame_width) > 5)    
            newSize = 1;
        else if (abs(h - frame_height) > 5)    
            newSize = 1;
        if (newSize) {
            curHnode->w = w;
            curHnode->h = h;
            frame_height = h;
            frame_width = w;
        }
    }
    newKey = 1;
    if (curHnode->imgId == frameId && curHnode->aipKey != NULL) {
        if (strcmp(aipKey, curHnode->aipKey) == 0)
            newKey = 0;
    }
    if (newKey) {
        k = strlen(aipKey);
        if (curHnode->keyLen <= k) {
            if (curHnode->aipKey != NULL)
                free(curHnode->aipKey);
            curHnode->aipKey = malloc(k+2);
            if (curHnode->aipKey != NULL)
                curHnode->keyLen = k + 2;	
            else
                curHnode->keyLen = 0;
        }
        if (curHnode->aipKey != NULL)
            strcpy(curHnode->aipKey, aipKey);
    }
    curHnode->imgId = frameId;

    startNode = curHnode->start;
    if (curHnode->newTemp || startNode == NULL) {
        newKey = 1;
        dup_template_node(curHnode);
        curHnode->newTemp = 0;
        startNode = curHnode->start;
    }
/*
    else if (newSize) {
        if (useXFunc)
            free_font(startNode);
    }
*/
    if (!useXFunc) {
        jInfo.code = htonl(2);
        graphToVnmrJ(&jInfo, jHLen);
    }

    if (useXFunc)
        save_origin_font();
    width = w - 4;
    height = h - 2;
    org_w = mnumxpnts;
    org_h = mnumypnts;
    org_x = x;
    org_y = height;
    win_y = y;
    mnumxpnts = org_x + width;
    mnumypnts = y + h;
    gridWidth = (float)width / (float)grid;
    gridHeight = (float)height / (float)grid;
    if (debug)
        dump();

    sprintf(inputs, "aipCurrentKey='%s'\n", aipKey); 
    execString(inputs);
    if (newKey) {
    	set_value();
    }
    if (show_orientation)
        set_orientation(frameId);
    k = 2;
    if (useXFunc) {
        // if (newKey || newSize)
        if (newSize)
           adjust_size();
        if (new_str_len || newSize) {
           free_font_list();
           k = adjust_font();
        }
        new_str_len = 0;
    
        if (k < 1) {
            printErr = 0;
        }
        else {
/*
            printErr = 1;
*/
            paint();
            if (show_orientation)
                disp_orientation();
        }

        recover_origin_font();
    }
    else {
        jpaint();

        jInfo.code = htonl(3);
        jInfo.id = htonl(1999);
        if (show_orientation)
           jInfo.p1 = htonl(1);
        else
           jInfo.p1 = htonl(0);
        jInfo.len = htonl(4);
        for (i = 0; i < 4; i++)
           jInfo.info[i] = ' ';
        for (i = 0; i < 4; i++) {
           if (orientNodes[i] != NULL && orientNodes[i]->value != NULL)
              jInfo.info[i] = *orientNodes[i]->value;
        }
        graphToVnmrJ(&jInfo, jHLen + 4);

        jInfo.code = htonl(4);
        jInfo.id = htonl(0);
        jInfo.p0 = htonl(x);
        jInfo.p1 = htonl(y);
        jInfo.p2 = htonl(w);
        jInfo.p3 = htonl(h);
        jInfo.len = htonl(0);
        graphToVnmrJ(&jInfo, jHLen);
    }
    mnumxpnts = org_w;
    mnumypnts = org_h;
    new_template = 0;
    return (1);
}

int
annotation(int args, char *argv[], int retc, char *retv[])
{
    int ap;
    float  argX, argY, argW, argH;
    FILE  *fd;
    
    (void) retc;
    (void) retv;
    if (!useXFunc) {
        RETURN;
    }
#ifdef VNMRJ
    show_orientation = 0;
    tempName = NULL;
    aipKey = NULL;
    preview = 0;
    annotatePlot = 0;
    debug = 0;
    ap = 1;
    if (argv[0][0] == 'p')
       annotatePlot = 1;
    while (ap < args) {
       if (argv[ap][0] == '-') {
           if (strcmp(argv[ap], "-preview") == 0)
                preview = 1;
           else if (strcmp(argv[ap], "-debug") == 0)
                debug = 1;
           else if (strcmp(argv[ap], "-d") == 0)
                debug = 1;
           else if (strcmp(argv[ap], "-f") == 0) {
               ap++;
               if (ap < args)
                 tempName = argv[ap];
           }
       }       
       else
           break;
       ap++;
    }
    if (tempName == NULL && ap < args)
       tempName = argv[ap];
    ap++;
    if (annotatePlot) {
/**
       if(setplotter()) {
           Werrprintf("Annotation set plotter failed! ");
           preview = 0;
           RETURN;
       }
       color(CYAN);
**/
        Werrprintf("Annotation  plotter failed! ");
        RETURN;
    }
    else
       setdisplay();

    org_x = 0;
    org_y = 0;
    win_y = 0;
    org_w = mnumxpnts;
    org_h = mnumypnts;
    argX = 0.0;
    argY = 0.0;
    argW = wcmax;
    argH = wc2max;

    if (ap < args) {
         argX = atof(argv[ap]);
         if (argX > wcmax * 0.8)
             argX = wcmax * 0.8;
    }
    ap++;
    if (ap < args) {
         argY = atof(argv[ap]);
         if (argY > wc2max * 0.8)
             argY = wc2max * 0.8;
    }
    ap++;
    if (ap < args) {
         argW = atof(argv[ap]);
         if (argW > wcmax)
              argW = wcmax;
         if (argW < wcmax * 0.2)
              argW = wcmax * 0.2;
    }
    ap++;
    if (ap < args) {
         argH = atof(argv[ap]);
         if (argH > wc2max)
              argH = wc2max;
         if (argH < wc2max * 0.2)
              argH = wc2max * 0.2;
    }
    if (argW + argX > wcmax)
         argW = wcmax - argX;
    if (argH + argY > wc2max)
         argH = wc2max - argY;
    if (annotatePlot) { 
       width = mnumxpnts;
       height = mnumypnts;
    }
    else {
       width = graf_width;
       height = graf_height;
    }
    org_x = width * (argX / wcmax);
    org_y = height * (argY / wc2max);
    width = width * (argW / wcmax);
    height = height * (argH / wc2max);

    if (width < 50 || height < 50) {
        if (printErr)
           Werrprintf("Window size is too small for annotation.");
        if (annotatePlot)
           setdisplay();
        preview = 0;
        RETURN;
    }
    if (tempName == NULL) {
        if (preview)
            tempName = previewName;
        else
            tempName = defaultName;
    }

    org_y = org_y + height;

    if (preview) {
        sunGraphClear();
        window_redisplay();
    }

    startNode = NULL;
    if (tempName[0] != '/')
       sprintf(tempPath, "%s/templates/vnmrj/annotation/%s.txt",userdir,tempName);
    else
       sprintf(tempPath, "%s", tempName);
    fd = fopen(tempPath,"r");
    if(fd == NULL) {
       if (preview) {
           preview = 0;
           RETURN;
       }
       sprintf(tempPath, "%s/imaging/templates/vnmrj/annotation/%s.txt",systemdir,tempName);
       fd = fopen(tempPath,"r");
       if (fd == NULL) {
           Werrprintf("Could not find template '%s'", tempName);
           RETURN;
       }
    }
    curHnode = get_headnode(0, 0);
    if (curHnode == NULL) {
        fclose(fd);
        preview = 0;
        RETURN;
    }
    curHnode->imgId = 0;
/*
    if (headNode == NULL || headNode[0] == NULL)
	RETURN;
*/
    for (ap = 0; ap < headNodeSize; ap++) {
        if (headNode[ap] != NULL) {
/*
            free_font(headNode[ap]->start);
*/
            headNode[ap]->newTemp = 1;
            headNode[ap]->start = NULL;
        }
    }
    releaseWithId("annotate");
    mainNode = NULL;
    free_font_list();
    frame_width = 0;
/*
    curHnode = headNode[0];
*/
    table_col_size = 0;
    table_row_size = 0;
    orientInfo = curHnode->orientInfo;
    orientNodes = curHnode->orientNodes;
    buildNode(fd);
    fclose(fd);
    curHnode->start = startNode;
    old_mtime = 0;
    save_origin_font();
    if (debug)
        dump();
    if (!annotatePlot) {
        mnumxpnts = graf_width;
        mnumypnts = graf_height;
    }
    set_value();
    if (adjust_size() < 1) {
        if (printErr)
            Werrprintf("Window size is too small for annotation.");
    }
    else {
        adjust_font();
        paint();
        if (show_orientation)
            disp_orientation();
    }
    if (!annotatePlot) {
        mnumxpnts = org_w;
        mnumypnts = org_h;
    }
    if (preview) {
      /*  if (strcmp(tempName, previewName) == 0) */
            unlink(tempPath);
    }
    recover_origin_font();
#endif
    preview = 0;
    RETURN;
}


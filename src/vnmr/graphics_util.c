/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include "vnmrsys.h"
#include "graphics.h"
#include "graphics_util.h"
#include "wjunk.h"
#include "group.h"
#include "pvars.h"

#ifdef MOTIF
#include <X11/Intrinsic.h>
#else
typedef unsigned long Pixel;
#endif

extern void image_convert(char *options, char *fmt1, char *file1, char *fmt2, char *file2);
extern int exec_shell(char *cmd);

#ifdef VNMRJ
extern int useXFunc;
extern Pixel search_pixel();
extern Pixel get_vnmr_pixel();
extern void clear_overlay_area();
extern void clear_overlay_map();
extern void disp_overlay_image();
extern void draw_overlay_image();
extern void imageFrameOp(int id, int fromFrameCmd, char *cmd);
extern void showError(char *msg);
extern void setVjUiPort(int on);
extern void setVjPrintMode(int on);
extern int setPlotterName(char *name);
extern void start_iplot(FILE *fd, FILE *topFd, int width, int height);
extern double getScreenDpi();
#else
static int useXFunc = 1;
static Pixel search_pixel() { return (0); }
static Pixel get_vnmr_pixel() { return (0); }
static void  clear_overlay_area() {}
static void  clear_overlay_map() {}
static void  disp_overlay_image() {}
static void  draw_overlay_image() {}
static void  writelineToVnmrJ() {}
#endif
extern FILE *popen_call(char *cmdstr, char *mode);
extern int  pclose_call(FILE *pfile);
extern int  plot_locx();
extern int  plot_locy();
extern int  plot_raster();
extern void replace_plot_file();
extern void Wturnoff_message();
extern void ps_font(char *font);
extern void ps_flush();
extern void ps_rgb_color(double r, double g, double b);
extern void ps_dark_color();
extern void ps_light_color();
extern void ps_info_font();
extern void ps_info_light_font();
extern void ps_rstring(char *s);

extern int  is_vplot_session();
extern FILE *plot_file();
extern char *plot_file_name();
extern char *realString(double d);
extern int get_window_width();
extern int get_window_height();
extern int getPlotWidth();
extern int getPlotHeight();
extern int jplot_charsize(double f);
extern int set_wait_child(int pid);
extern int ps_charsize(double d);
extern void systemPlot(char *filename, int retc, char *retv[]);

extern int P_getstring(int tree, const char *name, char *buf, int index, int maxbuf);
extern int  setplotter();
extern int  getPSoriginX();
extern int  getPSoriginY();
extern int  graphics_colors_loaded();
extern int  ps_colors_loaded();
extern double  getpaperheight();
extern double  getpaperwidth();
extern double  getpaperwidth();
extern double  get_ploter_ppmm();
extern double  getpaper_leftmargin();
extern double  getpaper_topmargin();
extern double  getPSscaleX();
extern double  getPSscaleY();

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

#define CM_RED          0
#define CM_GREEN        1
#define CM_BLUE         2
#define INVALID         99999

#define MAX_LWZ_BITS    12

#define GIFterminator 0x3b
#define GIFextension 0x21
#define GIFimage 0x2c

#define GIFcomment 0xfe
#define GIFapplication 0xff
#define GIFplaintext 0x01
#define GIFgraphicctl 0xf9

struct gif_scr {
  unsigned int  Width;
  unsigned int  Height;
  unsigned int  ColorMap_present;
  unsigned int  BitPixel;
  unsigned int  ColorResolution;
  int           Background;
  unsigned int  AspectRatio;
};

struct gif_scr GifData;

typedef struct _plot_node {
  int  x, y;
  int  x2, y2;
  int  width, height;
  int  covered;
  int  colorNum;
  int  whiteColor;
  IMGNode *gf;
  byte cR[256];
  byte cG[256];
  byte cB[256];
  struct _plot_node *next;
} PLOT_NODE;

typedef struct _ov_node {
  int  x, y;
  int  x2, y2;
  int  width, height;
  int  covered;
  int  rotateDegree;
  struct _ov_node *next;
} OV_NODE;


#define INTERLACE       0x40
#define GLOBALCOLORMAP  0x80
#define LOCALCOLORMAP   0x80
#define BitSet(byte, bit)       (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len) (fread(buffer, len, 1, file) != 0)

#define LM_to_uint(a,b)                 (((b)<<8)|(a))

static IMGNode *currentGif = NULL;
static IMGNode *imgGifList = NULL;
static IMGNode *oldGifList = NULL;
static IMGNode *freeGifList = NULL;
static DIM_NODE *dimList = NULL; /* image info list */
static PLOT_NODE *plotList = NULL; /* images to be plotted  */
static PLOT_NODE *newPlotNode = NULL;
static OV_NODE *ovList = NULL; /* images overlaied  */

static int readColorMap(FILE *fd, int number);
static int DoExtension(FILE *fd);
static int readImage(FILE *fd, int x, int y, int w, int h, int cmapSize, int interlace);

static char  inputs[1024];
static char  imgPath[1024+64];
static char  iplotPath[MAXPATH*2];
static char  *expDir = NULL;
static char  *osPrinterName = NULL;
static char  *imageMark = "start plotting";
static int recover = 0;
static int bgColor = 0;
static int bImgFlush = 0;
static int whiteColor = 0;
static int imagecount;
static int debug = 0;
static int fromVj = 0;
static int colorNum = 0;
static int plotMode = 0;
static int maxImgWidth = 0;
static int maxImgHeight = 0;
static int recover_message; /* ==TRUE message already printed */
static double scrnDpi = 90.0;
static byte cR[256];
static byte cG[256];
static byte cB[256];
static byte *ovR; /* overlay memory map */
static byte *ovG;
static byte *ovB;
static FILE *iplotFd = NULL;
static FILE *iplotTopFd = NULL;

#ifdef OLD

static byte *access_data(IMGNode *e, unsigned long pos)
{
  return e->data+pos;
}

#endif

struct _pageinfo {
         char   *type;
         int    width;
         int    height;
       };

/**
static struct _pageinfo page_table[] = {
       {"letter",   612, 792},
       {"Letter",   612, 792},
       {"legal",    612, 1008},
       {"Legal",    612, 1008},
       {"ledger",  1224, 792},
       {"Ledger",  1224, 792},
       {"11x17",    792, 1224},
       {"A0",      2380, 3368},
       {"A1",      1684, 2380},
       {"A2",      1190, 1684},
       {"A3",       842, 1190},
       {"A4",       595, 842},
       {"A5",       421, 595},
       {"A6",       297, 421},
       {"A7",       210, 297},
       {"A8",       148, 210},
       {"B0",      2836, 4008},
       {"B1",      2004, 2386},
       {"B2",      1418, 2004},
       {"B3",      1002, 1418},
       {"B4",       709, 1002},
       {"B5",       501,  709},
       {"C0",      2600, 3677},
       {"C1",      1837, 2600},
       {"C2",      1298, 1837},
       {"C3",       918, 1298},
       {"C4",       649,  918},
       {"C5",       459,  649},
       {"C6",       323,  459},
       {"Flsa",     612,  936},
       {"Flse",     612,  936},
       {"Halfletter", 396,  612},
       {NULL,   0, 0 }
     };
**/


static void save_imagelist_geom(IMGNode *list)
{
    DIM_NODE  *nd;
    IMGNode   *node;
    FILE  *fd;

    if (expDir == NULL)
        sprintf(inputs, "%s/.iconmap", curexpdir);
    else
        sprintf(inputs, "%s/.iconmap", expDir);
    if (debug)
	fprintf(stderr, "save_imagefile_geom to %s \n", inputs); 
    fd = fopen(inputs, "w");
    if (fd == NULL)
	return;
    node = list;
    while (node != NULL) {
	nd = node->dimNode;
	if (nd != NULL && nd->name != NULL) {
	   fprintf(fd, "name: %s\n", nd->name);
	   if (nd->molFile)
	      fprintf(fd, "mol: yes\n");
	   fprintf(fd, "geom: %g %g %g %g %.3g %.3g %.5g %.5g %.5g\n",
	     nd->vx, nd->vy, nd->vw, nd->vh, nd->rx, nd->ry, nd->rw, nd->rh,
	     nd->rs);
	}
	node = node->next;
    }
    fclose(fd);
}

void save_imagefile_geom()
{
    if (imgGifList != NULL)
         save_imagelist_geom(imgGifList);
    else if (oldGifList != NULL)
         save_imagelist_geom(oldGifList);
}

static DIM_NODE
*read_imagefile_geom()
{
    DIM_NODE *nd, *pnd, *head;
    FILE  *fd;

    head = NULL;
    if (expDir == NULL) {
	expDir = (char *) malloc(MAXPATHL+1);
        if (expDir == NULL)
	    return(head);
	strcpy(expDir, curexpdir);
    }
    head = (DIM_NODE *) calloc(1, sizeof(DIM_NODE));
    if (head == NULL)
	return(head);
    head->name = malloc(8);
    sprintf(head->name, "nonexx");
    head->next = NULL;
    sprintf(inputs, "%s/.iconmap", expDir);
    if (debug)
	fprintf(stderr, "image geom file: %s \n", inputs); 
    fd = fopen(inputs, "r");
    if (fd == NULL)
	return(head);

    pnd = head;
    nd = NULL;
    while (fgets(inputs, 500, fd) != NULL) {
	if (debug)
	    fprintf(stderr, "  %s", inputs);
        if (strncmp(inputs,"name:",5) == 0) {
	    nd = (DIM_NODE *) calloc(1, sizeof(DIM_NODE));
	    if (nd == NULL)
		break;
	    sscanf(inputs,"%*s%s",imgPath);
	    nd->name = (char *) malloc(strlen(imgPath) + 2);
	    strcpy(nd->name, imgPath);
	    nd->next = NULL;
	    pnd->next = nd;
	    pnd = nd;
	}
	else if (nd != NULL) {
            if (strncmp(inputs,"geom:",5) == 0) {
		sscanf(inputs,"%*s%lg%lg%lg%lg%lg%lg%lg%lg%lg",&nd->vx,&nd->vy,
		     &nd->vw,&nd->vh,&nd->rx,&nd->ry,&nd->rw,&nd->rh,&nd->rs);
		nd->w = (int) nd->vw;
		nd->h = (int) nd->vh;
	    }
            else if (strncmp(inputs,"mol:",4) == 0) {
	        sscanf(inputs,"%*s%s",imgPath);
		if (strcmp(imgPath, "yes") == 0)
		     nd->molFile = 1;
	    }
	}
    }

    fclose(fd);
    return(head);
}

static void
clear_dimNode(hd)
DIM_NODE *hd;
{
    DIM_NODE *d1, *d2;

    d1 = hd;
    while (d1 != NULL) {
	d2 = d1->next;
	if (d1->name != NULL)
	    free(d1->name);
	free(d1);
	d1 = d2;
    }
}

void releaseGifNode(node)
IMGNode *node;
{
    if (node == NULL)
	return;
    if (node->data != NULL)
	free(node->data);
    if (node->name != NULL)
	free(node->name);
    if (node->fpath != NULL)
	free(node->fpath);
    node->prev = NULL;
    node->data = NULL;
    node->name = NULL;
    node->fpath = NULL;
    node->used = 0;
    node->ploted = 0;
    if (node->dimNode != NULL)
        clear_dimNode(node->dimNode);
    node->dimNode = NULL;
    node->next = freeGifList;
    if (freeGifList != NULL)
        freeGifList->prev = node;
    freeGifList = node;
}

static void 
releasePlotList()
{
    PLOT_NODE *n1, *n2;
    OV_NODE *o1, *o2;
    IMGNode  *gf;
    DIM_NODE *nd;

    n1 = plotList;
    while (n1 != NULL) {
	n2 = n1->next;
	gf = n1->gf;
	if (gf != NULL) {
	   if (gf->dimNode != NULL) {
	   	nd = gf->dimNode;
	   	if (nd->name != NULL)
        	    free(nd->name);
    	   	free(nd);
	   }
    	   gf->dimNode = NULL;
    	   releaseGifNode(gf);
	}
	free(n1);
	n1 = n2;
    }
    plotList = NULL;
    o1 = ovList;
    while (o1 != NULL) {
	o2 = o1->next;
	free(o1);
	o1 = o2;
    }
    ovList = NULL;
}


/* check if recover option is active, if not, print warning */

static int check_recover(int some_data)
{
  if(recover)
    return imagecount;
  if(!recover_message && (imagecount>0 || some_data)) {
    if (debug) {
       if(imagecount>0)
      	    fprintf(stderr, "%d complete image%s ", imagecount,
              (imagecount>1?"s":""));
       if(imagecount>0 && some_data)
      	    fprintf(stderr, "and ");
       if(some_data)
      	    fprintf(stderr, "partial data of a broken image");
       fprintf(stderr, "\n");
    }
    recover_message=TRUE;
  }
  return -1;
}

static int
readGIF(FILE *fd)
{
  unsigned char buf[16];
  unsigned char c;
  int           useGlobalColormap;
  int           bitPixel;
  int i, r, g, b;
  int w, h, x_off, y_off;


  if (currentGif == NULL)
    return(0);
  imagecount = 0;
  bgColor = -1;
  whiteColor = -1;
  recover_message = FALSE;

  GifData.Background = -1;
/***
  if (! ReadOK(fd,buf,6)) {
    return(0);
  }

  if (strncmp((char *)buf,"GIF",3) != 0) {
    return(0);
  }
**/

  if (! ReadOK(fd,buf,7)) {
    return(0);
  }

  GifData.Width           = LM_to_uint(buf[0],buf[1]);
  GifData.Height          = LM_to_uint(buf[2],buf[3]);
  GifData.BitPixel        = 2<<(buf[4]&0x07);
  GifData.ColorResolution = (((buf[4]&0x70)>>3)+1);
  GifData.ColorMap_present = BitSet(buf[4], GLOBALCOLORMAP);
  GifData.AspectRatio     = buf[6];
  if (!useXFunc && !plotMode) {
	currentGif->width = GifData.Width;
	currentGif->height = GifData.Height;
	return (1);
  }

  if (GifData.ColorMap_present) {     /* Global Colormap */
    GifData.Background = buf[5];
    if (readColorMap(fd,GifData.BitPixel)) {
      return(0);
    }
  }
  else {
    /* the GIF spec says that if neither global nor local
     * color maps are present, the decoder should use a system
     * default map, which should have black and white as the
     * first two colors. So we use black, white, red, green, blue,
     * yellow, purple and cyan.
     * I don't think missing color tables are a common case,
     * at least it's not handled by most GIF readers.
     */

    static int colors[]={0, 7, 1, 2, 4, 3, 5, 6};
    unsigned long *pixs = currentGif->pixels;

    GifData.Background       = -1;
    for(i=0 ; i<MAXCMSIZE-8 ; i++) {
      r = (colors[i&7]&1) ? ((255-i)&0xf8) : 0;
      g = (colors[i&7]&2) ? ((255-i)&0xf8) : 0;
      b = (colors[i&7]&4) ? ((255-i)&0xf8) : 0;
      if (plotMode) {
	cR[i] = r;
        cG[i] = g;
        cB[i] = b;
      }
      else
        pixs[i] = search_pixel(r, g, b);
    }
    for(i=MAXCMSIZE-8 ; i < MAXCMSIZE; i++) {
      r = (colors[i&7]&1) ? 4 : 0;
      g = (colors[i&7]&2) ? 4 : 0;
      b = (colors[i&7]&4) ? 4 : 0;
      if (plotMode) {
	cR[i] = r;
        cG[i] = g;
        cB[i] = b;
      }
      else
         pixs[i] = search_pixel(r, g, b);
    }
    colorNum = 256;
  }

  while (1) {
    if (! ReadOK(fd,&c,1))
      return check_recover(FALSE);

    if (c == GIFterminator)
      return(imagecount);

    if (c == GIFextension) {    /* Extension */
      if(!DoExtension(fd))
        return check_recover(FALSE);
      continue;
    }

    if (c != GIFimage) {                /* Not a valid start character */
      return check_recover(FALSE);
    }

    if (! ReadOK(fd,buf,9))
      return check_recover(FALSE);

    useGlobalColormap = ! BitSet(buf[8], LOCALCOLORMAP);

    bitPixel = 1<<((buf[8]&0x07)+1);
    x_off = LM_to_uint(buf[0],buf[1]);
    y_off = LM_to_uint(buf[2],buf[3]);
    w = LM_to_uint(buf[4],buf[5]);
    h = LM_to_uint(buf[6],buf[7]);

    if (! useGlobalColormap) {
      if (readColorMap(fd,bitPixel))
        return check_recover(FALSE);
      if(readImage(fd, x_off, y_off, w, h, bitPixel,
                    BitSet(buf[8], INTERLACE)))
        imagecount++;
    }
    else {
      if(readImage(fd, x_off, y_off, w, h, GifData.BitPixel,
                    BitSet(buf[8], INTERLACE)))
        imagecount++;
    }
  } /* while loop */

}

static int
readColorMap(FILE *fd, int number)
{

  int   i, k, r, g, b;
  byte  rgb[3];
  unsigned long *pixs = currentGif->pixels;

  k = number;
  if (k > MAXCMSIZE)
     k = MAXCMSIZE;
  colorNum = k;
  for (i = 0; i < k; i++) {
    if (! ReadOK(fd, rgb, sizeof(rgb))) {
      return(TRUE);
    }
    if (plotMode) {
        cR[i] = rgb[0];
        cG[i] = rgb[1];
        cB[i] = rgb[2];
        if (cR[i] == 255 && cG[i] == 255 && cB[i] == 255)
	    whiteColor = i;
    }
    else {
        r = rgb[0];
        g = rgb[1];
        b = rgb[2];
        pixs[i] = search_pixel(r, g, b);
    }
  }
  if (plotMode)
     return FALSE;

  for (i = number; i < MAXCMSIZE; i++) {
    pixs[i] = 0;
  }
  if (bgColor >= 0)
    pixs[bgColor] = get_vnmr_pixel(-1); /* get vnmr background pixel */

  return FALSE;
}


static int
DoExtension(FILE *fd)
{
  int buf_cnt;
  unsigned char c, code;

  if (! ReadOK(fd,&code,1))
	 return (0);
  /* skip any remaining image data */
  do
  {
      if (! ReadOK(fd,&c,1))
	 return (0);
      buf_cnt = c;
      inputs[0] = 0;
      if (buf_cnt > 0) {
         if (! ReadOK(fd, inputs, buf_cnt))
	    return (0);
      }
      if (code == GIFgraphicctl) {
         if ((inputs[0] & 0x1) != 0) { /* transparent */
	    bgColor = (unsigned char) inputs[3];
            if (debug)
		fprintf(stderr, "   image transparent color %d \n", bgColor);
	    currentGif->bgColor = bgColor;
	    currentGif->transparent = 1;
	    currentGif->pixels[bgColor] = get_vnmr_pixel(-1); // -1 is background color
	 }
      }
  } while (buf_cnt > 0);

  return (1);
}

static int      ZeroDataBlock = FALSE;

static int
GetDataBlock(FILE *fd, unsigned char *buf)
{
  unsigned char count;

  count = 0;
  if (! ReadOK(fd, &count, 1)) {
      return -1;
  }

  ZeroDataBlock = count == 0;

  if ((count != 0) && (! ReadOK(fd, buf, count))) {
    return -1;
  }

  return((int)count);
}

/*
**  Pulled out of nextCode
*/
static  int             curbit, lastbit, get_done, last_byte;
static  int             return_clear;
/*
**  Out of nextLWZ
*/
static int      stack[(1<<(MAX_LWZ_BITS))*2], *sp;
static int      code_size, set_code_size;
static int      max_code, max_code_size;
static int      clear_code, end_code;
static int interlace_start[4]= { /* start line for interlacing */
  0, 4, 2, 1
};

static int interlace_rate[4]= { /* rate at which we accelerate vertically */
  8, 8, 4, 2
};


static void initLWZ(int input_code_size)
{
  set_code_size = input_code_size;
  code_size     = set_code_size + 1;
  clear_code    = 1 << set_code_size ;
  end_code      = clear_code + 1;
  max_code_size = 2 * clear_code;
  max_code      = clear_code + 2;

  curbit = lastbit = 0;
  last_byte = 2;
  get_done = FALSE;

  return_clear = TRUE;

  sp = stack;
}

static int nextCode(FILE *fd, int code_size)
{
  static unsigned char    buf[280];
  static int maskTbl[16] = {
    0x0000, 0x0001, 0x0003, 0x0007,
    0x000f, 0x001f, 0x003f, 0x007f,
    0x00ff, 0x01ff, 0x03ff, 0x07ff,
    0x0fff, 0x1fff, 0x3fff, 0x7fff,
  };
  int i, j, end;
  long ret;

  if (return_clear) {
    return_clear = FALSE;
    return clear_code;
  }

  end = curbit + code_size;

  if (end >= lastbit) {
    int     count;

    if (get_done) {
      return -1;
    }
    buf[0] = buf[last_byte-2];
    buf[1] = buf[last_byte-1];

    if ((count = GetDataBlock(fd, &buf[2])) == 0)
      get_done = TRUE;

    if (count<0) return -1;

    last_byte = 2 + count;
    curbit = (curbit - lastbit) + 16;
    lastbit = (2+count)*8 ;

    end = curbit + code_size;
  }

  j = end / 8;
  i = curbit / 8;

  if (i == j)
    ret = (long)buf[i];
  else if (i + 1 == j)
    ret = (long)buf[i] | ((long)buf[i+1] << 8);
  else
    ret = (long)buf[i] | ((long)buf[i+1] << 8) | ((long)buf[i+2] << 16);

  ret = (ret >> (curbit % 8)) & maskTbl[code_size];

  curbit += code_size;

  return (int)ret;
}

#define readLWZ(fd) ((sp > stack) ? *--sp : nextLWZ(fd))

static int nextLWZ(FILE *fd)
{
  static int       table[2][(1<< MAX_LWZ_BITS)];
  static int       firstcode, oldcode;
  int              code, incode;
  register int     i;

  while ((code = nextCode(fd, code_size)) >= 0) {
    if (code == clear_code) {

      /* corrupt GIFs can make this happen */
      if (clear_code >= (1<<MAX_LWZ_BITS)) {
        return -2;
      }

      for (i = 0; i < clear_code; ++i) {
        table[0][i] = 0;
        table[1][i] = i;
      }
      for (; i < (1<<MAX_LWZ_BITS); ++i)
        table[0][i] = table[1][i] = 0;
      code_size = set_code_size+1;
      max_code_size = 2*clear_code;
      max_code = clear_code+2;
      sp = stack;
      do {
        firstcode = oldcode = nextCode(fd, code_size);
      } while (firstcode == clear_code);

      return firstcode;
    }
    if (code == end_code) {
      int             count;
      unsigned char   buf[260];

      if (ZeroDataBlock)
        return -2;

      while ((count = GetDataBlock(fd, buf)) > 0)
        ;
      return -2;
    }

    incode = code;

    if (code >= max_code) {
      *sp++ = firstcode;
      code = oldcode;
    }

    while (code >= clear_code) {
      *sp++ = table[1][code];
      if (code == table[0][code]) {
        return(code);
      }
      if (((char *)sp - (char *)stack) >= (int) sizeof(stack)) {
        return(code);
      }
      code = table[0][code];
    }

    *sp++ = firstcode = table[1][code];

    if ((code = max_code) <(1<<MAX_LWZ_BITS)) {
      table[0][code] = oldcode;
      table[1][code] = firstcode;
      ++max_code;
      if ((max_code >= max_code_size) &&
          (max_code_size < (1<<MAX_LWZ_BITS))) {
        max_code_size *= 2;
        ++code_size;
      }
    }

    oldcode = incode;

    if (sp > stack)
      return *--sp;
  }
  return code;
}

#ifdef OLD
/* return the actual line # used to store an interlaced line */
static int interlace_line(int height, int line)
{
    int res;

    if ((line & 7) == 0) {
        return line >> 3;
    }
    res = (height+7) >> 3;
    if ((line & 7) == 4) {
        return res+((line-4) >> 3);
    }
    res += (height+3) >> 3;
    if ((line & 3) == 2) {
        return res + ((line-2) >> 2);
    }
    return res + ((height+1) >> 2) + ((line-1) >> 1);
}

/* inverse function of above, used for recovery of interlaced images */
static int inv_interlace_line(int height, int line)
{
    if ((line << 3)<height) {
        return line << 3;
    }
    line -= (height+7) >> 3;
    if ((line << 3) + 4 < height) {
        return (line << 3) + 4;
    }
    line -= (height+3) >> 3;
    if ((line << 2) + 2 < height) {
        return (line << 2) + 2;
    }
    line -= (height + 1) >> 2;
    return (line << 1) + 1;
}


/* find the nearest line above that already has data in it */

static byte *
get_prev_line(int width, int height, int line)
{
  int prev_line;
  byte *res;

  prev_line=inv_interlace_line(height,line)-1;
  while(interlace_line(height, prev_line)>=line)
    prev_line--;

  res=access_data(currentGif, width*interlace_line(height, prev_line));

  return res;
}
#endif

static void
interlaceImage(FILE *fd, int width, int height, int cmapSize)
{
  int   x, y, pass, scanlen;
  int   v;
  unsigned char *image, *dp;

  for (pass= 0; pass < 4; pass++) {
      y = interlace_start[pass];
      image = currentGif->data + y * width;
      scanlen= width * interlace_rate[pass];
      while (y < height) {
         dp = image;
         for (x = 0; x < width; x++) {
      	    v = readLWZ(fd);
      	    if (v < 0 || v >= cmapSize) {
		v = 0;
	    }
            *dp++=v;
	 }
	 y += interlace_rate[pass];
         image += scanlen;
      }
   }
   while(readLWZ(fd)>=0)
      continue;
}


static int
readImage(FILE *fd, int x_off, int y_off, int width, int height, int cmapSize,
          int interlace)
{
  unsigned char *dp, c;
  int           v;
  int           xpos = 0, ypos = 0;
  unsigned char *image;

  /*
   **  Initialize the compression routines
   */

  if (! ReadOK(fd,&c,1)) {
    return(0);
  }

  recover = 0;
  initLWZ(c);

  /* since we know how large the image will be, we set the size in advance.
     This saves a lot of realloc calls, which may require copying memory. */

  currentGif->data = (byte *)malloc(width*height);
  if (currentGif->data == NULL) {
    return(0);
  }
  
  currentGif->offset_x = x_off;
  currentGif->offset_y = y_off;
  currentGif->width    = width;
  currentGif->height   = height;

  if (interlace) {
	interlaceImage(fd, width, height, cmapSize);
	return(1);	
  }
  for (ypos = 0; ypos < height; ypos++) {
    image= currentGif->data + ypos * width;
    dp = image;
    for (xpos = 0; xpos < width; xpos++) {
      v = readLWZ(fd);
      if (v < 0 || v >= cmapSize) {
        if(xpos>0 || ypos>0) {
          if(recover) {
#ifdef OLD
            if(!interlace) {
              /* easy part, just fill the rest of the `screen' with color 0 */
              memset(image+xpos,0, width-xpos);
            }
            else {
               /* interlacing recovery is a bit tricky */
               if(xpos>0) {
                 if((inv_interlace_line(height, ypos)&7)==0) {
                    /* in 1st pass */
                    memset(image+xpos, 0, width-xpos);
                 } else {
                    /* pass >=2 */
                    memcpy(image+xpos, get_prev_line(width, height, ypos)+xpos,
                         width-xpos);
                 }
                 ypos++;
               }
               memset(image, 0, width);
            }
#else
            memset(image+xpos,0, width-xpos);
#endif
            ypos = height;
            xpos = width;
            break;
          } /* if recover */
          else {
            check_recover(TRUE);
            return(0);
          }
        } else {
          return(0);
        }
      }
      *dp++=v;
    }
  }
  while(readLWZ(fd)>=0)
     continue;

  return(1);
}

IMGNode
*open_imagefile(char *file, int isMol)
{
    char   tbuf[8];
    char   tmpName[MAXPATH];
    char   *mol;
    struct stat fileStat;
    FILE   *gfd;
    int    ret, loop;

    (void) isMol;
    if (file == NULL)
	return(NULL);
    mol = NULL;
    loop = 1;
    strcpy(inputs, file);
    if (plotMode) { 
	mol = strrchr(inputs, '.');
	if (mol != NULL) {
	     if (strcmp(mol, ".display") == 0) {
		strcpy(mol, ".plot");
		loop = 2;
	     }
	}
    }
    strcpy(imgPath, inputs);
    if (debug) {
	fprintf(stderr, "open image: %s \n", imgPath);
	if (plotMode)
	   fprintf(stderr, " for plotting ... \n");
    }
    gfd = fopen(imgPath, "r");
    while (gfd == NULL && loop > 0) {
        if ((file[0] != '/') && (file[0] != '.')) {
          // if (isMol) {
               sprintf(imgPath, "%s/mollib/%s", userdir, inputs);
               gfd = fopen(imgPath, "r");
               if (gfd == NULL) {
                  sprintf(imgPath, "%s/mollib/%s", systemdir, inputs);
                  gfd = fopen(imgPath, "r");
	       }
	  // }
	  if (gfd == NULL) {
               sprintf(imgPath, "%s/mollib/icons/%s", getenv("vnmruser"), inputs);
               gfd = fopen(imgPath, "r");
               if (gfd == NULL) {
                  sprintf(imgPath, "%s/mollib/icons/%s", getenv("vnmrsystem"), inputs);
                  gfd = fopen(imgPath, "r");
               }
          }
	  if (gfd == NULL) {
               sprintf(imgPath, "%s/iconlib/%s", getenv("vnmruser"), inputs);
               gfd = fopen(imgPath, "r");
               if (gfd == NULL) {
                  sprintf(imgPath, "%s/iconlib/%s", getenv("vnmrsystem"), inputs);
                  gfd = fopen(imgPath, "r");
	       }
	   }
	}
	loop--;
	if (loop > 0 && gfd == NULL) {
    	    strcpy(inputs, file);
    	    strcpy(imgPath, file);
    	    gfd = fopen(imgPath, "r");
	}
    }
    if (gfd == NULL)
	return(NULL);
    if (debug)
	fprintf(stderr, "   new img %s \n", imgPath);
    if (! ReadOK(gfd, tbuf,6)) {
    	fclose(gfd);
	return(NULL);
    }
    if (strncmp(tbuf,"GIF",3) != 0) {
    	fclose(gfd);
	if (debug)
	   fprintf(stderr, "  not gif format \n");
        sprintf(tmpName, "/vnmr/tmp/gifXXXXXX");
        ret = mkstemp(tmpName);
        if (ret < 0){
            Werrprintf("Could not create temporary image file %s.",tmpName);
            return(NULL);
        }
        close(ret);
/*
        sprintf(inputs, "%s/bin/convert %s GIF:%s", getenv("vnmrsystem"),
             imgPath, tmpName);
*/
        image_convert("", "", imgPath, "GIF", tmpName);
        gfd = fopen(tmpName, "r");
	if (gfd == NULL) {
	   Werrprintf("Could not convert GIF image file %s" ,imgPath);
	   return(NULL);
        }
        if (! ReadOK(gfd, tbuf,6) || (strncmp(tbuf,"GIF",3) != 0)) {
    	   fclose(gfd);
	   unlink(tmpName);
	   Werrprintf("Could not convert GIF image file %s" ,imgPath);
	   return(NULL);
        }
    }
    else {
        tmpName[0] = '\0';
    }

    if (freeGifList != NULL) {
	currentGif = freeGifList;
	freeGifList = currentGif->next;
	if (freeGifList != NULL)
	    freeGifList->prev = NULL;
    }
    else
        currentGif = (IMGNode *) calloc(1, sizeof (IMGNode));
    
    if (currentGif == NULL) {
    	fclose(gfd);
        if (strlen(tmpName) > 1)
	      unlink(tmpName);
	return(NULL);
    }
    currentGif->next = NULL;
    currentGif->prev = NULL;
    currentGif->data = NULL;
    currentGif->dimNode = NULL;
    currentGif->bgColor = -1;
    currentGif->transparent = 0;
    currentGif->name = (char *) malloc(strlen(file) + 2);
    currentGif->fpath = (char *) malloc(strlen(imgPath) + 2);
    if (currentGif->name == NULL || currentGif->fpath == NULL) {
	releaseGifNode(currentGif);
    	fclose(gfd);
        if (strlen(tmpName) > 1)
	    unlink(tmpName);
	return(NULL);
    }
    strcpy(currentGif->name, file);
    strcpy(currentGif->fpath, imgPath);
    if (stat(imgPath, &fileStat) >= 0) {
	currentGif->mtime = fileStat.st_mtime;
    }

    ret = readGIF(gfd);
    fclose(gfd);
    if (strlen(tmpName) > 1)
	unlink(tmpName);
    if (plotMode) {
	if (bgColor >= 0) {
	   cR[bgColor] = 255;
	   cG[bgColor] = 255;
	   cB[bgColor] = 255;
        }
    }
    if (ret <= 0) {
	releaseGifNode(currentGif);
	currentGif = NULL;
    }
    return (currentGif);
}


extern int graf_height;
extern int graf_width;

#ifdef INTERACT
static int      Bnmr = 1;
#else
extern int      Bnmr;
#endif

#define IDISP   1
#define ISIZE   2
#define ION     3
#define IOFF    4
#define IDEL    5
#define IDISPALL  6
#define ICLEAR  7
#define IPLOT   8
#define IPLTALL 9
#define DEBUGMODE 10
#define IHIDE  11 
#define ILOAD  12 

#define CMDNUM  13

static char *cmds[] = {"null", "display", "resize", "on", "off", "delete",
		"displayall", "clear", "plot", "plotall", "debug", "hide",
                "load" };

static int imgNum = 0;
static int winWidth = 0;
static int winHeight = 0;
static int plotHeight = 0;
static int plotWidth = 0;
static int paperWidth = 0;
static int paperHeight = 0;
static int vymin = 0;
static int clearFlag = 0;
static int plotState = 0;
static double vppm = 0;
static char  newPlotPath[MAXPATH];
static FILE  *newPlotFd = NULL;

static void setPlotDimension()
{
   /***
    double pw, ph;
    double ppmm, dv;
    int    iv, raster;
   ***/
    
    plotWidth = getPlotWidth();
    plotHeight = getPlotHeight();

   /***
    plotWidth = mnumxpnts;
    plotHeight = mnumypnts;
    pw = getpaperwidth();
    ph = getpaperheight();
    if (pw < 60.0 || ph < 60.0)  // too small
        return;
    raster = plot_raster();
    if (raster == 2 || raster == 4)  // landscape
    {
        if (ph > pw)
        {
            dv = ph;
            ph = pw;
            pw = dv;
         }
    } 
    if (wcmax > pw || wc2max > ph) // information was not correct
        return;
    ppmm = get_ploter_ppmm();
    if (ppmm < 1.0)
        return;
    pw = pw - (double)left_edge - (double)right_edge;
    ph = ph - (double)top_edge - (double)bottom_edge;
    iv = (int) (ppmm * pw);
    if (plotWidth > iv)
        plotWidth = iv;
    iv = (int) (ppmm * ph);
    if (plotHeight > iv)
        plotHeight = iv;
   ***/
}

static void
copy_dim_node(snode, dnode)
DIM_NODE *snode, *dnode;
{
    if (snode == NULL || dnode == NULL)
        return;
    dnode->x = snode->x;
    dnode->y = snode->y;
    dnode->w = snode->w;
    dnode->h = snode->h;
    dnode->vx = snode->vx;
    dnode->vy = snode->vy;
    dnode->vw = snode->vw;
    dnode->vh = snode->vh;
    dnode->rx = snode->rx;
    dnode->ry = snode->ry;
    dnode->rw = snode->rw;
    dnode->rh = snode->rh;
    dnode->rs = snode->rs;
    dnode->rTop = snode->rTop;
    dnode->rBottom = snode->rBottom;
}

static DIM_NODE
*dup_info_node(node)
DIM_NODE *node;
{
    DIM_NODE *nd;

    nd = (DIM_NODE *) calloc(1, sizeof(DIM_NODE));
    if (nd == NULL) {
	if (debug)
	   fprintf(stderr, " Error:  malloc failed \n");
	return (NULL);
    }
    copy_dim_node(node, nd);
    nd->name = NULL;
    return (nd);
}

static void
clear_icon_info_node(node)
DIM_NODE *node;
{
    if (node == NULL)
       return;
    node->vx = 0.0;
    node->vy = 0.0;
    node->vw = 0.0;
    node->vh = 0.0;
    node->rx = 0.0;
    node->ry = 0.0;
    node->rw = 0.0;
    node->rh = 0.0;
    node->rs = 0.0;
    node->rTop = 0.0;
    node->rBottom = 1.0;
}

static DIM_NODE
*get_new_icon_info_node(name)
char *name;
{
    DIM_NODE *node;

    node = (DIM_NODE *) calloc(1, sizeof(DIM_NODE));
    if (node == NULL)
	return (NULL);
    node->name = (char *) malloc(strlen(name) + 2);
    if (node->name == NULL) {
	free(node);
	return (NULL);
    }
    strcpy(node->name, name);
    node->vy = INVALID;
    node->ry = 1.0;
    node->rTop = 0.0;
    node->rBottom = 1.0;
    return (node);
}

/***
static DIM_NODE
*get_icon_info_node(name)
char *name;
{
    DIM_NODE *node, *pnode;

    if (dimList == NULL) {
	dimList = read_imagefile_geom();
        if (dimList == NULL)
	    return (NULL);
    }
    node = dimList->next;
    pnode = dimList;
    while (node != NULL) {
        if (node->name == NULL) {
            if (debug)
                fprintf(stderr, " info_node is NULL \n");
        }
	else if (strcmp(name, node->name) == 0) {
	    if (plotMode) {
		if (node->ploted == 0)
		   break;
	    }
	    else {
	        pnode->next = node->next;
	        return (node);
	    }
	}
	pnode = node;
	node = node->next;
    }
    if (plotMode && node != NULL) {
	node->ploted = 1;
	return (dup_info_node(node));
    }
    return (get_new_icon_info_node(name));
}
*****/


static void
clean_icon_area(node)
 IMGNode *node;
{

     IMGNode *gf;
     DIM_NODE *nd;
     int x, y, w, h;
     int x2, y2;

     nd = node->dimNode;
     x = nd->x;
     y = nd->y;
     w = nd->w + 1;
     h = nd->h + 1;
     clear_overlay_area(x, y, w, h);
     w = x + w;
     h = y + h;
     gf = imgGifList;
     while (gf != NULL) {
	if (gf != node) {
     	   nd = gf->dimNode;
	   x2 = nd->x + nd->w;
	   y2 = nd->y + nd->h;
	   if (x2 > x && nd->x < w) {
	      if (y2 > y && nd->y < h) {
                 draw_overlay_image(gf->data, gf->width, gf->height, nd->x,
           	     nd->y, nd->w, nd->h, gf->pixels, gf->bgColor, gf->hilit);
	      }
	   }
	}
	gf = gf->next;
     }
}

static void
set_img_geom(imgNode, type, sx, sy, sw, sh)
IMGNode *imgNode;
int   type;
int   sx, sy, sw, sh;
{
     DIM_NODE *nd;
     int    ih, iw, w, h;
     int    winW, winH;
     double d, xppmm, yppmm;
     double rx, ry;

     if (plotMode) {
        vymin = ymin;
	winW = plotWidth; 
	winH = plotHeight; 
        iw = mnumxpnts;
        ih = mnumypnts - ymin;
        xppmm = ppmm;
        yppmm = ppmm;
     }
     else {
        if (wc2max <= 5 || wc2max >= INVALID) /* wc2max not initiated */
	    setdisplay();
        winWidth = get_window_width();
        winHeight = get_window_height();
        if (winWidth < 100)
            winWidth = 800;
        if (winHeight < 100)
            winHeight = 600;
	winW = winWidth; 
	winH = winHeight; 
        iw = winW;
        ih = winH;
        xppmm = (double) iw / wcmax;
        yppmm =  (double) ih / wc2max;
        vppm = ppmm;
        vymin = 0;
     }
     nd = imgNode->dimNode;
     if (debug) {
	fprintf(stderr, " set geom %d:  %d %d %d %d \n", imgNode->id, sx, sy, sw, sh);
	fprintf(stderr, "   wcmax: %g  wc2max: %g\n", wcmax, wc2max);
	fprintf(stderr, "   page wh: %d %d  ppmm %g %g\n", winW, winH, xppmm, yppmm);
        if (plotMode)
	    fprintf(stderr, "   rx: %g  ry: %g  rw: %g  rh: %g  rs: %g\n", nd->rx, nd->ry, nd->rw, nd->rh, nd->rs);
     }
     if (type == IDISP) {
	if (sx == INVALID) {
	    if (plotMode) {
		rx = nd->rx * wcmax;
            }
	    else
	        rx = nd->vx;
	}
	else
	    rx = (double) sx;
	if (sy == INVALID) {
	    if (plotMode)
	        ry = nd->ry * wc2max;
	    else
	        ry = nd->vy;
            if (ry < 0.01) // not initialized
                ry = wc2max;
	}
	else
	    ry = (double) sy;
	if (rx < 0)
            rx = 0;
	nd->vx = rx;
	d = rx * xppmm + 0.48;
	nd->x = (int) d;
        if (plotMode == 0) {
	    if (ry > wc2max)
	       ry = wc2max;
        }
        nd->vy = ry;
        d = ry * yppmm + 0.48 + vymin;
        if (plotMode)
            nd->y = (int) d;
        else
            nd->y = ih - (int) d;
	w = 0;
	h = 0;
	if (sw <= 0) {
	    if (plotMode) {
	       if (sw == 0 || nd->rw < 0.001) {
                   d = (double) imgNode->width / scrnDpi;
                   d = d * 25.4 * xppmm;
	           w = (int) d;
	       }
	       else {
                   d = (double) iw * nd->rw;
	           w = (int) d;
	       }
	    }
	    else
               w = nd->w;
	}
	else {
            w = (int) ((double) sw * xppmm);
	}
	if (sh <= 0) {
	    if (plotMode) {
	       if (sh == 0 || nd->rh < 0.001) {
                   d = (double) imgNode->height / scrnDpi;
                   d = d * 25.4 * yppmm;
	           h = (int) d;
	       }
	       else {
                   if (nd->rs > 0.0)
                      d = nd->rs * (double) w;
                   else
                      d = nd->rh * (double) ih;
	           h = (int) d;
	       }
	    }
	    else
               h = nd->h;
	}
	else {
            h = (int) ((double) sh * yppmm);
	}
	if (w <= 0)
	    nd->w = imgNode->width; // image original width
	else
	    nd->w = w;
	if (h <= 0)
	    nd->h = imgNode->height; // image original height
	else
	    nd->h = h;
        if (maxImgWidth > 0 && maxImgHeight > 0) {
            if (nd->w > maxImgWidth || nd->h > maxImgHeight) {
                d = (double) maxImgWidth / (double) imgNode->width;
                rx = (double) maxImgHeight / (double) imgNode->height;
                if (d > rx)
                   d = rx;
                nd->w = (int)(d * imgNode->width);
                nd->h = (int)(d * imgNode->height);
            }
        }
     }
     else { /* it's from VJ resize call */
        nd->x = sx;
        nd->y = sy;
	d = (double) sx / xppmm;
        if (d >= wcmax)
           d = wcmax - 2.0;
        if (d < 0)
           d = 0;
	nd->vx = d;
        d = (double)(ih - sy) / yppmm;
        if (d > wc2max)
           d = wc2max;
        else if (d < 0)
           d = 0;
        nd->vy = d;

        if (sw <= 0) 
	    nd->w = imgNode->width;
	else
            nd->w = sw;
        if (sh <= 0)
	    nd->h = imgNode->height;
	else
            nd->h = sh;
        ry = (double) ih - (sy + nd->h);
        if (ry < 0)
            ry = 0;
        nd->rBottom = ry / (double) ih;
        nd->rTop = (double) sy / (double) ih;
     }
     if (nd->w > winW)
	nd->w = winW;
     if (nd->h > winH)
	nd->h = winH;
     if (nd->x >= winW)
	nd->x = winW - nd->w;
     if (nd->x < 0)
	nd->x = 0;
     if (plotMode == 0) {
        if (nd->y >= winH)
	    nd->y = winH - nd->h;
        if (nd->y < 0)
	    nd->y = 0;
        if ((nd->y + nd->h) > winH) {
	   if (sy != INVALID)
	      nd->y = winH - nd->h;
	   else
	      nd->h = winH - nd->y;
        }
        if ((nd->x + nd->w) > winW) {
   	   if (sy != INVALID)
	      nd->x = winW - nd->w;
	   else
	      nd->w = winW - nd->x;
        }
     }
     nd->vw = (double) nd->w; // for screen display
     nd->vh = (double) nd->h;
     nd->rw = nd->vw / (double) iw;
     nd->rh = nd->vh / (double) ih;
     nd->rs = nd->vh / nd->vw;
     nd->rx = nd->vx / wcmax;
     nd->ry = nd->vy / wc2max;
     ry = nd->vy + nd->vh;
     if (debug) {
	fprintf(stderr, "   img geom:  %d %d %d %d \n", nd->x, nd->y, nd->w, nd->h);
	fprintf(stderr, "   rx: %g  ry: %g  rw: %g  rh: %g  rs: %g\n", nd->rx, nd->ry, nd->rw, nd->rh, nd->rs);
	fprintf(stderr, "    top: %g  bottom: %g \n", nd->rTop, nd->rBottom);
     }
}

static IMGNode
*get_used_node(name)
char *name;
{
     IMGNode *node, *unode;
     struct stat fileStat;

     node = oldGifList;
     unode = NULL;
     while (node != NULL) {
         if (node->name == NULL) {
             if (debug)
                fprintf(stderr, "  used_node is NULL \n");
         }
         else if (strcmp(name, node->name) == 0) {
             if (stat(node->fpath, &fileStat) >= 0) {
         	if (fileStat.st_mtime == node->mtime) {
		    node->used = 0;
                    break;
		}
	     }
	     if (unode == NULL)
		unode = node;
	 }
         node = node->next;
     }
     if (node == NULL) {
	 if (unode == NULL)
	    return(node);
	 node = unode;
	 node->used = 1;
     }
     if (node->prev != NULL)
	 node->prev->next = node->next;
     if (node->next != NULL)
	 node->next->prev = node->prev;
     if (node == oldGifList)
	 oldGifList = node->next;
     node->prev = NULL;
     if(node->used) {
	releaseGifNode(node);
        node = NULL;
     }
     return (node);
}

static void
remove_node(node)
IMGNode  *node;
{
     if (node == NULL)
        return;
     if (node == imgGifList) {
        imgGifList = node->next;
        if (imgGifList != NULL)
            imgGifList->prev = NULL;
     }
     else {
        if (node->next != NULL)
            node->next->prev = node->prev;
        if (node->prev != NULL)
            node->prev->next = node->next;
     }
     releaseGifNode(node);
}


void
count_overlay_image()
{
     IMGNode *gf;

     gf = imgGifList;
     imgNum = 0;
     while (gf != NULL) {
	if (gf->id > imgNum)
	    imgNum = gf->id;
	gf = gf->next;
     }
}

void
repaint_overlay_image()
{
     IMGNode *gf;
     DIM_NODE *nd;

     gf = imgGifList;
     imgNum = 0;
     clear_overlay_map();
     while (gf != NULL) {
        nd = gf->dimNode;
        draw_overlay_image(gf->data, gf->width, gf->height, nd->x, 
		nd->y, nd->w, nd->h, gf->pixels, gf->bgColor, gf->hilit);
	if (gf->id > imgNum)
	    imgNum = gf->id;
	gf = gf->next;
     }
     if (debug)
        fprintf(stderr, " repaint_overlay_image  %d \n", imgNum);
}


static void
clear_imgList(IMGNode *list)
{
     IMGNode *gf, *ngf;

     gf = list;
     while (gf != NULL) {
	ngf = gf->next;
	releaseGifNode(gf);
	gf = ngf;
     }
}

static void
clean_house()
{
    if (imgGifList != NULL)
	clear_imgList(imgGifList);
    if (oldGifList != NULL)
	clear_imgList(oldGifList);
    imgGifList = NULL;
    oldGifList = NULL;
    clear_dimNode(dimList);
    dimList = NULL;
    imgNum = 0;
}

void
clear_overlay_image()
{
     if (imgGifList != NULL) {
	clear_imgList(imgGifList);
        imgGifList = NULL;
     }
     imgNum = 0;
     if (fromVj == 0) {
        sprintf(inputs, "icon clear");
        writelineToVnmrJ("vnmrjcmd", inputs);
     }
}

static void
create_all()
{
     DIM_NODE *nd;
     IMGNode  *ngf, *pnode;

     if (debug)
	fprintf(stderr, "display all images \n");

     if (dimList == NULL) {
        dimList = read_imagefile_geom();
        if (dimList == NULL)
	   return;
     }
     nd = dimList->next;
     pnode = imgGifList;
     imgNum = 1;
     while (nd != NULL) {
        dimList->next = nd->next;
        nd->next = NULL;
	ngf = NULL;
	if (nd->name != NULL)
	    ngf = open_imagefile(nd->name, nd->molFile);
	if (ngf != NULL) {
	    ngf->dimNode = nd;
            ngf->next = NULL;
            ngf->prev = NULL;
	    if (imgGifList == NULL)
                imgGifList = ngf;
            else {
                pnode->next = ngf;
                ngf->prev = pnode;
            }
            ngf->hilit = 0;
            pnode = ngf;
	    ngf->id = imgNum;
            imgNum++;
	    set_img_geom(ngf, IDISP, (int)nd->vx, (int)nd->vy,
		 -1, -1);
	    if (nd->molFile) {
              sprintf(inputs, "icon %s %d %s %d %d %d %d mol", cmds[IDISP],
		 ngf->id, ngf->fpath, nd->x, nd->y, nd->w, nd->h);
	    }
            else {
              sprintf(inputs, "icon %s %d %s %d %d %d %d", cmds[IDISP],
		 ngf->id, ngf->fpath, nd->x, nd->y, nd->w, nd->h);
	    }
            writelineToVnmrJ("vnmrjcmd", inputs);
	    if (debug)
		fprintf(stderr, " %s \n", inputs);
	}
	nd = dimList->next;
     }
}


void
redraw_overlay_image()
{
     int    dh;
     double d, dx, dy;
     IMGNode *gf;
     DIM_NODE *nd;

     if (debug)
        fprintf(stderr, " redraw_overlay_image  %d \n", imgNum);
     if (imgNum <= 0 || !useXFunc)
	return;

     if (winWidth != graf_width || winHeight != graf_height) {
	if (vppm == ppmm && vymin == ymin)
	    setdisplay();
        vppm = ppmm;
        vymin = ymin;
        winWidth = graf_width;
        winHeight = graf_height;
	dx = (double) (mnumxpnts - right_edge) / wcmax;
        dh =  mnumypnts - ymin;
        dy =  (double) dh / wc2max;
        gf = imgGifList;
	while (gf != NULL) {
	     nd = gf->dimNode;
	     d = (double) nd->vx * dx + 0.48;
	     nd->x = (int) d;
	     if (nd->vy == INVALID)
	        d = wc2max * dy + 0.48;
	     else
	        d = (double) nd->vy * dy + 0.48;
             nd->y = dh - (int) d;
	     if ((nd->x + nd->w) > winWidth)
		nd->x = winWidth - nd->w;
	     if ((nd->y + nd->h) > winHeight)
		nd->y = winHeight - nd->h;
             sprintf(inputs, "icon dimension %d %d %d %d %d\n", gf->id,
		 nd->x, nd->y, nd->w, nd->h);
             writelineToVnmrJ("vnmrjcmd", inputs);
	     gf = gf->next;
	}
	repaint_overlay_image();
	if (!useXFunc)
             writelineToVnmrJ("vnmrjcmd", "icon redraw \n");
     }
     if (useXFunc)
         disp_overlay_image(0, 1);
}

void update_imagefile()
{
    if (debug)
	 fprintf(stderr, " update_imagefile \n");
    if (expDir == NULL) {
	 expDir = (char *) malloc(MAXPATHL+1);
	 strcpy(expDir, "  ");
    }
    else {
	 if (strcmp(curexpdir, expDir) == 0)
	    return;
	 save_imagefile_geom();
    } 
    clean_house();
    clearFlag = 0;
    strcpy(expDir, curexpdir);
}


#define Min(x,y)  (((x) < (y)) ? (x) : (y))


void
write_ps_image_header(fd)
 FILE  *fd;
{
   //   "%%BeginProlog",
   //   "%",
   static const char
    *PostscriptProlog[]=
    {
      "/DirectClassPacket",
      "{",
      "  currentfile color_packet readhexstring pop pop",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 3 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add 3 mul def",
      "  } ifelse",
      "  0 3 number_pixels 1 sub",
      "  {",
      "    pixels exch color_packet putinterval",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/DirectClassImage",
      "{",
      "  systemdict /colorimage known",
      "  {",
      "    columns rows 8",
      "    [",
      "      columns 0 0",
      "      rows neg 0 rows",
      "    ]",
      "    { DirectClassPacket } false 3 colorimage",
      "  }",
      "  {",
      "    %",
      "    % No colorimage operator;  convert to grayscale.",
      "    %",
      "    columns rows 8",
      "    [",
      "      columns 0 0",
      "      rows neg 0 rows",
      "    ]",
      "    { GrayDirectClassPacket } image",
      "  } ifelse",
      "} bind def",
      "",
      "/GrayDirectClassPacket",
      "{",
      "  currentfile color_packet readhexstring pop pop",
      "  color_packet 0 get 0.299 mul",
      "  color_packet 1 get 0.587 mul add",
      "  color_packet 2 get 0.114 mul add",
      "  cvi",
      "  /gray_packet exch def",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 1 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add def",
      "  } ifelse",
      "  0 1 number_pixels 1 sub",
      "  {",
      "    pixels exch gray_packet put",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/GrayPseudoClassPacket",
      "{",
      "  currentfile byte readhexstring pop 0 get",
      "  /offset exch 3 mul def",
      "  /color_packet colormap offset 3 getinterval def",
      "  color_packet 0 get 0.299 mul",
      "  color_packet 1 get 0.587 mul add",
      "  color_packet 2 get 0.114 mul add",
      "  cvi",
      "  /gray_packet exch def",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 1 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add def",
      "  } ifelse",
      "  0 1 number_pixels 1 sub",
      "  {",
      "    pixels exch gray_packet put",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/PseudoClassPacket",
      "{",
      "  currentfile byte readhexstring pop 0 get",
      "  /offset exch 3 mul def",
      "  /color_packet colormap offset 3 getinterval def",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 3 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add 3 mul def",
      "  } ifelse",
      "  0 3 number_pixels 1 sub",
      "  {",
      "    pixels exch color_packet putinterval",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/PseudoClassImage",
      "{",
      "  currentfile buffer readline pop",
      "  token pop /class exch def pop",
      "  class 0 gt",
      "  {",
      "    currentfile buffer readline pop",
      "    token pop /depth exch def pop",
      "    /grays columns 8 add depth sub depth mul 8 idiv string def",
      "    columns rows depth",
      "    [",
      "      columns 0 0",
      "      rows neg 0 rows",
      "    ]",
      "    { currentfile grays readhexstring pop } image",
      "  }",
      "  {",
      "    currentfile buffer readline pop",
      "    token pop /colors exch def pop",
      "    /colors colors 3 mul def",
      "    /colormap colors string def",
      "    currentfile colormap readhexstring pop pop",
      "    systemdict /colorimage known",
      "    {",
      "      columns rows 8",
      "      [",
      "        columns 0 0",
      "        rows neg 0 rows",
      "      ]",
      "      { PseudoClassPacket } false 3 colorimage",
      "    }",
      "    {",
      "      %",
      "      % No colorimage operator;  convert to grayscale.",
      "      %",
      "      columns rows 8",
      "      [",
      "        columns 0 0",
      "        rows neg 0 rows",
      "      ]",
      "      { GrayPseudoClassPacket } image",
      "    } ifelse",
      "  } ifelse",
      "} bind def",
      "",
      "/DisplayImage",
      "{",
      "  /buffer 512 string def",
      "  /byte 1 string def",
      "  /color_packet 3 string def",
      "  /pixels 768 string def",
      "  currentfile buffer readline pop",
      "  token pop /x exch def",
      "  token pop /y exch def pop",
      "  x y translate",
      "  currentfile buffer readline pop",
      "  token pop /z exch def pop",
      "  z rotate",
      "  currentfile buffer readline pop",
      "  token pop /x exch def",
      "  token pop /y exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /pointsize exch def pop",
      "  /Times-Roman findfont pointsize scalefont setfont",
      "  x y scale",
      "  currentfile buffer readline pop",
      "  token pop /columns exch def",
      "  token pop /rows exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /class exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /compression exch def pop",
      "  class 0 gt { PseudoClassImage } { DirectClassImage } ifelse",
      "} bind def",
      (char *) NULL
    };
    const char **q;

    for (q = PostscriptProlog; *q; q++)
        fprintf(fd, "%s\n", *q);
    // fprintf(fd, "%%%%EndProlog\n");
}

static void
WriteImageTransform(fd)
 FILE  *fd;
{
   fprintf(fd,"gsave\n");
   if (bImgFlush < 1)
      return; 
   fprintf(fd,"%d %d translate\n", getPSoriginX(), getPSoriginY());
   if (plot_raster() == 4)
      fprintf(fd,"90 rotate\n");
   fprintf(fd,"%.4g %.4g scale\n", getPSscaleX(), getPSscaleY());
}

#ifdef NOTUSED
/* output Direct Color image */
static void
WriteDirectPSImage(fd, gf)
 FILE  *fd;
 IMGNode *gf;
{
   int r, c;
   long x, y, i;
   byte  *src;
   byte  index;
   int   bg, fg;
   unsigned long length;
   DIM_NODE *nd;

   bg = gf->bgColor;
   nd = gf->dimNode;
/*
   x = plot_locx(nd->x);
   y = mnumypnts - nd->y - nd->h;
   y = plot_locy(y);
*/
   x = nd->x;
   y = nd->y;
   //  fprintf(fd, "gsave\n");
    WriteImageTransform(fd);
   fprintf(fd, "DisplayImage\n");
   fprintf(fd, "%ld %ld\n", x, y);
   fprintf(fd, "%d\n", nd->rotateDegree);
   fprintf(fd, "%d %d\n", nd->w, nd->h);
   fprintf(fd, "12.0\n"); /* font point size */
   fprintf(fd, "%d %d\n", gf->width, gf->height);
   fprintf(fd, "0\n1\n"); /* DirectClassImage RLECompression */
   i = 0;
   for (r = 0; r < gf->height; r++) {
       src = gf->data + r * gf->width;
       index = *src;
       fg = (int) index;
       length=255;
       for (c = 0; c < gf->width; c++) {
           if ((index == *src) && (length < 255) && (c < gf->width- 1))
                length++;
           else {
                if (c > 0) { if (fg == bg)
                       fprintf(fd, "ffffff%02lX", length);
                    else
                       fprintf(fd, "%02X%02X%02X%02lX", cR[fg], cG[fg], cB[fg], length);
                    i++;
                    if (i == 9) {
                        fprintf(fd, "\n");
                        i = 0;
                    }
                }
                length = 0;
           }
           index = *src;
           fg = (int) index;
           src++;
       }
       if (fg == bg)
           fprintf(fd, "ffffff%02lX", length);
       else
           fprintf(fd, "%02X%02X%02X%02lX",
                        cR[fg], cG[fg], cB[fg], length);
    }
    fprintf(fd, "\n");
    fprintf(fd, "grestore\n");
}
#endif

static void
WritePseudoImage(fd, gf)
 FILE  *fd;
 IMGNode *gf;
{
   int k, r, c, y;
   long i;
   byte  *src;
   byte  index;
   unsigned long length;
   DIM_NODE *nd;

   nd = gf->dimNode;

   y = nd->y;
   if (y > plotHeight)
      y = plotHeight;
   fprintf(fd, "gsave\n");
   // WriteImageTransform(fd);
   fprintf(fd, "DisplayImage\n");
   fprintf(fd, "%d %d\n", nd->x, y);
   fprintf(fd, "%d\n", nd->rotateDegree);
   fprintf(fd, "%d %d\n", nd->w, nd->h);
   fprintf(fd, "12.0\n"); /* font point size */
   fprintf(fd, "%d %d\n", gf->width, gf->height);
   fprintf(fd, "1\n1\n0\n"); /* PseudoClass RLECompression */
   fprintf(fd, "%d\n", colorNum);
   for (k = 0; k < colorNum; k++) {
       fprintf(fd, "%02X%02X%02X\n", cR[k], cG[k], cB[k]);
   }
   i = 0;
   for (r = 0; r < gf->height; r++) {
       src = gf->data + r * gf->width;
       index = *src;
       length=255;
       for (c = 0; c < gf->width; c++) {
           if ((index == *src) && (length < 255) && (c < gf->width- 1))
                length++;
           else {
                if (c > 0) {
                    fprintf(fd, "%02X%02lX",
                        index,(unsigned long) Min(length,0xff));
                    i++;
                    if (i == 18) {
                        fprintf(fd, "\n");
                        i = 0;
                    }
                }
                length = 0;
           }
           index = *src;
           src++;
       }
       fprintf(fd, "%02X%02lX", index,(unsigned long) Min(length,0xff));
    }
    fprintf(fd, "\n");
    fprintf(fd, "grestore\n");
}


/* output Direct Color image */
static void
WriteOverlayImage(fd, node)
 FILE  *fd;
 OV_NODE *node;
{
   int k, row, col;
   int  w, iw, y, h;
   long i;
   byte  *r, *g, *b;
   byte  ir, ig, ib;
   unsigned long length;

   if (fd == NULL)
       return;
   w = node->x2 - node->x;
   h = node->y2 - node->y;
   if (node->width < 1 || node->height < 1)
       return;
   if (h < 1 || w < 1)
       return;
   y = node->y;
   if (y > plotHeight)
      y = plotHeight;
   // fprintf(fd, "gsave\n");
   WriteImageTransform(fd);
   fprintf(fd, "DisplayImage\n");
   fprintf(fd, "%d %d\n", node->x, y);
   fprintf(fd, "%d\n", node->rotateDegree);
   fprintf(fd, "%d %d\n", w, h);
   fprintf(fd, "12.0\n"); /* font point size */
   fprintf(fd, "%d %d\n", node->width, node->height);
   fprintf(fd, "0\n1\n"); /* DirectClassImage RLECompression */
   i = 0;
   for (row = 0; row < node->height; row++) {
       k = node->width * row;
       r = ovR + k;
       g = ovG + k;
       b = ovB + k;
	ir = *r;
	ig = *g;
	ib = *b;
       length=255;
	w = node->width;
	iw = w - 1;
       for (col = 0; col < w; col++) {
	   k = 0;
	   if (*r == ir && *g == ig && *b == ib) {
             if ((length < 255) && (col < iw)) {
                length++;
		k = 1;
	     }
	   }
           if (k == 0) {
                if (col > 0) {
                    fprintf(fd, "%02X%02X%02X%02lX", ir, ig, ib, length);
                    i++;
                    if (i == 9) {
                        fprintf(fd, "\n");
                        i = 0;
                    }
                }
                length = 0;
	        ir = *r;
	        ig = *g;
	        ib = *b;
           }
           r++;
           g++;
           b++;
       }
       fprintf(fd, "%02X%02X%02X%02lX", ir, ig, ib, length);
    }
    fprintf(fd, "\n");
    fprintf(fd, "grestore\n");
}



/* output Pseudo Color image */

static void
WritePSImage(fd, node)
 FILE  *fd;
 PLOT_NODE *node;
{
   int k, r, c;
   long x, y, i, colors;
   byte  *src;
   byte  index;
   byte  *cr, *cg, *cb;
   unsigned long length;
   DIM_NODE *nd;
   IMGNode *gf;

   gf = node->gf;
   if (gf->data == NULL)
	return;
   nd = gf->dimNode;
   if (nd == NULL)
	return;
   x = nd->x;
   y = nd->y;

   colors = node->colorNum;
   cr = node->cR;
   cg = node->cG;
   cb = node->cB;

   if ((y + nd->h) > plotHeight)
      y = plotHeight - nd->h;
   if ((x + nd->w) > plotWidth)
      x = plotWidth - nd->w;
   if (debug) {
      fprintf(stderr, "write ps image %s \n", gf->name);
      fprintf(stderr, "  size %d %d -> %d %d -> %d %d\n",gf->width, gf->height, nd->w, nd->h, node->width, node->height);
      fprintf(stderr, "  loc  %d %d -> %ld %ld\n",nd->x, nd->y, x, y);
      fprintf(stderr, "  page wh: %d %d  %d %d\n",plotWidth, plotHeight,mnumxpnts,mnumypnts);
   }
   // fprintf(fd, "gsave\n");
   WriteImageTransform(fd);
   fprintf(fd, "DisplayImage\n");
   fprintf(fd, "%ld %ld\n", x, y);
   fprintf(fd, "%d\n", nd->rotateDegree);
   fprintf(fd, "%d %d\n", nd->w, nd->h);
   fprintf(fd, "12.0\n"); /* font point size */
   fprintf(fd, "%d %d\n", gf->width, gf->height);
   fprintf(fd, "1\n1\n0\n"); /* PseudoClass RLECompression */
   fprintf(fd, "%ld\n", colors);
   for (k = 0; k < colors; k++) {
       fprintf(fd, "%02X%02X%02X\n", cr[k], cg[k], cb[k]);
   }
   i = 0;
   for (r = 0; r < gf->height; r++) {
       src = gf->data + r * gf->width;
       index = *src;
       length=255;
       for (c = 0; c < gf->width; c++) {
           if ((index == *src) && (length < 255) && (c < gf->width- 1))
                length++;
           else {
                if (c > 0) {
                    fprintf(fd, "%02X%02lX",
                        index,(unsigned long) Min(length,0xff));
                    i++;
                    if (i == 18) {
                        fprintf(fd, "\n");
                        i = 0;
                    }
                }
                length = 0;
           }
           index = *src;
           src++;
       }
       fprintf(fd, "%02X%02lX", index,(unsigned long) Min(length,0xff));
    }
    fprintf(fd, "\n");
    fprintf(fd, "grestore\n");
}


static int
add_plot_node(gf)
IMGNode  *gf;
{
     PLOT_NODE  *node, *pnode, *n2;
     int  k;
     int  x, y;
     double dh;
     DIM_NODE *dim;
     byte *r, *g, *b;
     
     dim = gf->dimNode;
     if (debug) {
	fprintf(stderr, "add_plot_img: %s \n", gf->name);
	fprintf(stderr, " size: %d %d\n", gf->width, gf->height);
	fprintf(stderr, "   rx: %g  ry: %g  rw: %g  rh: %g  rs: %g\n", dim->rx, dim->ry, dim->rw, dim->rh, dim->rs);
     }

     node = (PLOT_NODE *) malloc(sizeof(PLOT_NODE));
     if (node == NULL) {
	if (debug)
	    fprintf(stderr, "Error: malloc failed. \n");
	if (dim != NULL) {
	    if (dim->name != NULL)
		free(dim->name);
	    free(dim);
	}
	gf->dimNode = NULL;
	releaseGifNode(gf);
	return(0);
     }
     node->next = NULL;
     node->gf = gf;
     node->colorNum = colorNum;
     node->whiteColor = whiteColor;
     node->covered = 0;
     newPlotNode = node;
     r = node->cR;
     g = node->cG;
     b = node->cB;
     for (k = 0; k < colorNum; k++) {
	*r++ = cR[k];
	*g++ = cG[k];
	*b++ = cB[k];
     }
     x = plot_locx(dim->x);
     // y = plotHeight - dim->y - dim->h;
     y = dim->y - dim->h;
     y = plot_locy(y);
     if (y < 0)
         y = 0;
     if ((y + dim->h) > mnumypnts)
         y = mnumypnts - dim->h;
     dim->x = x;
     dim->y = y;
     node->x = x;
     node->y = y;
     node->x2 = x + dim->w;
     node->y2 = y + dim->h;
     node->width = dim->w;
     node->height = dim->h;
     if (dim->rs > 0.01) {
         node->height = (int) (dim->rs * dim->w);
         if (debug)
            fprintf(stderr, " top: %g  bottom: %g \n", dim->rTop, dim->rBottom);
         if (dim->rTop > 0.02) {
             dh = (double) (mnumypnts - vymin);
             if (dim->rBottom < dim->rTop)
                 y = (int) (dim->rBottom * dh);
             else {
                 y = (int) (dim->rTop * dh);
                 y = mnumypnts - y - dim->h;
             }
             y = plot_locy(y);
             if (y < 0)
                y = 0;
             if ((y + dim->h) > mnumypnts)
                y = mnumypnts - dim->h;
             if (debug)
                fprintf(stderr, " change y: %d, ymin %d  -> %d \n", dim->y, vymin, y);
             dim->y = y;
             node->y = y;
             node->y2 = y + dim->h;
         }
     }

     if (debug) {
	fprintf(stderr, " plot geom: %d %d %d %d \n", dim->x, dim->y, dim->w, dim->h);
	fprintf(stderr, "     view: %g %g %g %g \n", dim->vx, dim->vy, dim->vw, dim->vh);
	fprintf(stderr, "     wh ratio: %g  wh: %d %d \n", dim->rs, node->width, node->height);
     }

     if (plotList == NULL) {
        plotList = node;
        return(1);
     }
     pnode = plotList;
     if (gf->bgColor < 0) {
	n2 = pnode;
	while (n2 != NULL) {
	    if (n2->gf->bgColor >= 0)
		break;
	    pnode = n2;
	    n2 = n2->next;
	}
        if (n2 != NULL) {
	    node->next = n2;
	    if (n2 == plotList)
		plotList = node;
	    else
	        pnode->next = node;
	}
	else
	    pnode->next = node;
        return(1);
     }
     while (pnode->next != NULL)
	pnode = pnode->next;
     pnode->next = node;
     return(1);
}

static void
plot_all_image()
{
     DIM_NODE *dim, *dim2;
     // DIM_NODE *dlist;
     IMGNode  *ngf, *list;

     if (debug)
	fprintf(stderr, " plot all images... \n");
     list = imgGifList;
     if (list == NULL) { 
     	if (!clearFlag)
	   list = oldGifList;
     }
     if (list != NULL) {
        while (list != NULL) {
	   dim = list->dimNode;
	   ngf = NULL;
           if (list->hide == 0)
               ngf = open_imagefile(list->fpath, dim->molFile);
           if (ngf != NULL) {
               dim2 = dup_info_node(dim);
    	       if (dim2 == NULL) {
		  releaseGifNode(ngf);
		  return;
    	       }
               ngf->dimNode = dim2;
               ngf->id = list->id;
    	       set_img_geom(ngf, IDISP, INVALID, INVALID, -1, -1);
    	       if ( add_plot_node(ngf) < 1)
		  return;
           }
	   list = list->next;
	}
	return;
     }
     if (debug)
	 fprintf(stderr, "  image list is empty. \n");
     if (clearFlag)
	 return;
     /**********
     if (debug)
	 fprintf(stderr, "  create image list...\n");
     dlist = read_imagefile_geom();
     if (dlist == NULL)
	 return;
     dim = dlist->next;
     while (dim != NULL) {
         ngf = NULL;
         if (dim->name != NULL)
              ngf = open_imagefile(dim->name, dim->molFile);
         if (ngf != NULL) {
              dim2 = dup_info_node(dim);
    	      if (dim2 == NULL) {
		  releaseGifNode(ngf);
		  break;
    	      }
              ngf->dimNode = dim2;
    	      set_img_geom(ngf, IDISP, INVALID, INVALID, -1, -1);
    	      if (add_plot_node(ngf) < 1)
		  break;
	 }
	 dim = dim->next;
     }
     clear_dimNode(dlist);
     **********/
}

static int
plot_image(name, mol, x, y, w, h)
char *name;
int  mol, x, y, w, h;
{
    int isJplot;
    IMGNode  *gf, *ngf;
    DIM_NODE *nd, *nd2;

    isJplot = is_vplot_session(1);
    if (debug) {
	fprintf(stderr, " plot %s %d %d %d %d \n", name,x, y, w,h);
	if (isJplot)
	   fprintf(stderr, "  in  jplot mode \n");
    }
    ngf = NULL;
    gf = imgGifList;
    while (gf != NULL) {
	if (strcmp(name, gf->name) == 0) {
           if (gf->data != NULL && gf->ploted == 0)
		break;
	}
	gf = gf->next;
    }
    if (gf == NULL) {
        gf = oldGifList;
	while (gf != NULL) {
	   if (strcmp(name, gf->name) == 0) {
               if (gf->data != NULL && gf->ploted == 0)
                   break;
           }
           gf = gf->next;
        }
    }
    nd = NULL;
    if (gf != NULL) {
	gf->ploted = 1;
        ngf = open_imagefile(gf->fpath, mol);
	nd = gf->dimNode;
    }
    if (ngf == NULL) {
        ngf = open_imagefile(name, mol);
        if (ngf == NULL) {
            Werrprintf("Could not open image file %s\n",name);
	    return(0);
        }
    }
    if (ngf->data == NULL) {
	releaseGifNode(ngf);
	return(0);
    }
    if (nd == NULL) 
	nd2 = get_new_icon_info_node(name);
    else
	nd2 = dup_info_node(nd);
    if (nd2 == NULL) {
	releaseGifNode(ngf);
	return(0);
    }
    clear_icon_info_node(nd2);
    ngf->dimNode = nd2;
    if (isJplot) {
	if (w < 0 || h < 0)
	    nd2->rs = (double)ngf->height / (double)ngf->width;
	else
	    nd2->rs = 0;
	if (w < 0)
	    nd2->rw = 0;
	if (h < 0)
	    nd2->rh = 0;
	nd2->rx = 0;
	nd2->ry = 1.0;
    }
    set_img_geom(ngf, IDISP, x, y, w, h);
    add_plot_node(ngf);
    return(1);
}

// the last node will be the topmost to display
static void
raiseNodeToTop(node)
IMGNode  *node;
{
    IMGNode  *n1;

    if (node->next == NULL || imgGifList == NULL)
        return;
    if (node->prev != NULL)
        node->prev->next = node->next;
    node->next->prev = node->prev;
    if (imgGifList == node) {
        imgGifList = node->next;
        imgGifList->prev = NULL;
    }
    n1 = imgGifList;
    while (n1->next != NULL)
        n1 = n1->next;
    n1->next = node;
    node->next = NULL;
    node->prev = n1;
}

static char *lookupPrinter(char *fname, char *name, char *data)
{
    FILE  *fin;
    char  *p;
    int    k;
    char   type[128];

    fin = fopen(fname,"r");
    if (fin == NULL)
       return NULL;
    k = 0;
    while ((p = fgets(inputs, 300, fin)) != NULL) {
       if (sscanf(p,"%s%s",type, data) == 2) {
           if (!strcmp(type,"Name")) {
               if (!strcmp(name, data)) {
                    k = 1;
                    break;
               }
           }
       }
    }
    if (k == 0) {
       fclose(fin);
       return NULL;
    }
    k = 0;
    while ((p = fgets(inputs,300, fin)) != NULL) {
       if (sscanf(p,"%s%s",type, data) != 2)
          continue;
       if (!strcmp(type,"Name")) {
          if (strcmp(name, data) != 0)
              break;
       }
       if (!strcmp(type,"Printer")) {
          k = 1;
          break;
       }
    }
    fclose(fin);
    if (k == 0)
       return NULL;
    return data;
}

static void getOsPrinter(char *name)
{
    char *plotter;
    char   data[128];

    if (name == NULL)
        return;
    sprintf(inputs, "%s/devicenames",systemdir);
    plotter = lookupPrinter(inputs,name,data);
    if (plotter == NULL)
        plotter = name;
    if (osPrinterName != NULL)
    {
        if (!strcmp(plotter,osPrinterName))
            return;
    }
    if (osPrinterName != NULL)
        free(osPrinterName);
    osPrinterName = malloc(strlen(plotter) + 1);
    strcpy(osPrinterName, plotter);
}

void
printjimage(int args, char *argv[])
{
    int   k, n;
//    int   imgWidth, imgHeight;
    int   paperWidth, paperHeight;
    int   psWidth, psHeight;
    int   marginX, marginY;
    int   transX, transY;
    int   mX, mY;
    int   doPrint;
    int   doClear;
    int   isColor;
//    int   quiet;
    int   dpi;
    int   rotateDegree;
    int   numCopies;
    IMGNode  *ngf;
    DIM_NODE *dnode;
    char  *inFile = NULL;
//    char  *inType = NULL;
    char  *outFile = NULL;
    char  *outFormat = NULL;
    char  *orient = NULL;
//    char  *paperSize = NULL;
//    char  *prtSize = NULL;
    char  *plotter = NULL;
    char  tmpPath[256];
    char  tmpCmd[512];
    FILE  *outfd;
    int   ret __attribute__((unused));

    n = args - 1;
    paperWidth = 0;
    paperHeight = 0;
    psWidth = 0;
    psHeight = 0;
//    imgWidth = 0;
//    imgHeight = 0;
    doPrint = 0;
    doClear = 0;
    outfd = NULL;
    dpi = 300;
    isColor = 1;
//    quiet = 0;
    mX = -1;
    mY = -1;
    numCopies = 1;

    for (k = 1; k < args; k++) {
        if (argv[k][0] == '-') {
            if (strcmp(argv[k], "-print") == 0) {
                doPrint = 1;
                continue;
            }
            if (strcmp(argv[k], "-jpg") == 0) {
                //inType = argv[k];
                continue;
            }
            if (strcmp(argv[k], "-clear") == 0) {
                doClear = 1;
                continue;
            }
            if (strcmp(argv[k], "-mono") == 0) {
                isColor = 0;
                continue;
            }
            if (strcmp(argv[k], "-quiet") == 0) {
                //quiet = 1;
                continue;
            }
            if (k >= n)
                break;
            if (argv[k+1][0] == '-')
                continue;
            if (strcmp(argv[k], "-file") == 0) {
                k++;
                inFile = argv[k];
                continue;
            }
            if (strcmp(argv[k], "-outfile") == 0) {
                k++;
                outFile = argv[k];
                continue;
            }
            if (strcmp(argv[k], "-orient") == 0) {
                k++;
                orient = argv[k];
                continue;
            }
            if (strcmp(argv[k], "-plotter") == 0) {
                k++;
                plotter = argv[k];
                continue;
            }
            if (strcmp(argv[k], "-paper") == 0) {
                k++;
                //paperSize = argv[k];
                continue;
            }
            if (strcmp(argv[k], "-size") == 0) {
                k++;
                //prtSize = argv[k];
                continue;
            }
            if (strcmp(argv[k], "-iformat") == 0) {
                k++;
                outFormat = argv[k];
                continue;
            }
            if (strcmp(argv[k], "-width") == 0) {
                k++;
                //imgWidth = atoi(argv[k]);
                continue;
            }
            if (strcmp(argv[k], "-height") == 0) {
                k++;
                //imgHeight = atoi(argv[k]);
                continue;
            }
            if (strcmp(argv[k], "-pwidth") == 0) {
                k++;
                paperWidth = atoi(argv[k]);
                continue;
            }
            if (strcmp(argv[k], "-pheight") == 0) {
                k++;
                paperHeight = atoi(argv[k]);
                continue;
            }
            if (strcmp(argv[k], "-dwidth") == 0) {
                k++;
                psWidth = atoi(argv[k]);
                continue;
            }
            if (strcmp(argv[k], "-dheight") == 0) {
                k++;
                psHeight = atoi(argv[k]);
                continue;
            }
            if (strcmp(argv[k], "-dpi") == 0) {
                k++;
                dpi = atoi(argv[k]);
            }
            if (strcmp(argv[k], "-dx") == 0) {
                k++;
                mX = atoi(argv[k]);
            }
            if (strcmp(argv[k], "-dy") == 0) {
                k++;
                mY = atoi(argv[k]);
            }
            if (strcmp(argv[k], "-copy") == 0) {
                k++;
                numCopies = atoi(argv[k]);
            }
        }
    }
    if (inFile == NULL)
        return;
    if (doPrint == 0 && outFile == NULL) {
        Werrprintf("Convert image error: no output file name.");
        if (doClear)
            unlink(inFile);
        return;
    }

    if (paperWidth <= 0)
        paperWidth = 612;  // letter 
    if (paperHeight <= 0)
        paperHeight = 792;  // letter 
    if (psWidth <= 0)
        psWidth = paperWidth - 60;
    if (psHeight <= 0)
        psHeight = paperHeight - 60;
    plotMode = 1;
    ngf = open_imagefile(inFile, 0);
    if (doClear)
        unlink(inFile);
    if (ngf == NULL) {
        Werrprintf("Could not open image file %s\n",inFile);
        return;
    }
    if (ngf->dimNode == NULL) {
        dnode = (DIM_NODE *) calloc(1, sizeof(DIM_NODE));
        if (dnode == NULL) {
           releaseGifNode(ngf);
           plotMode = 0;
           return;
        }
        ngf->dimNode = dnode;
    }

    dnode = ngf->dimNode;
    
    marginY = (paperHeight - psHeight) / 2;
    marginX = (paperWidth - psWidth) / 2;
    transX = marginX;
    transY = marginY;
    rotateDegree = 0;
    dnode->rotateDegree = 0;
    if (orient != NULL) {
        if (strcmp(orient, "landscape") == 0 || psWidth > paperWidth) {
            if (paperHeight > paperWidth) {
               if (psWidth > paperHeight)
                  psWidth = paperHeight;
               if (psHeight > paperWidth)
                  psHeight = paperWidth;
               marginX  = (paperWidth - psHeight) / 2 + psHeight;
               marginY = (paperHeight - psWidth) / 2;
               // dnode->rotateDegree = 90;
               rotateDegree = 90;
            }
        }
    }
    transX = marginX;
    transY = marginY;
    if (mX >= 0 || mY >= 0) {
        if (mX < 0)
            mX = marginX;
        else
            marginX = mX;
        if (mY < 0)
            mY = marginY;
        else
            marginY = mY;
        if (orient != NULL) {
           if (strcmp(orient, "landscape") == 0 && (paperHeight > paperWidth))
           {
               if (psWidth > paperHeight)
                  psWidth = paperHeight;
               if (psHeight > paperWidth)
                  psHeight = paperWidth;
               marginX = paperWidth - mY;
               if (marginX > paperWidth)
                  marginX = paperWidth;
               marginY = mX;
               // dnode->rotateDegree = 90;
               rotateDegree = 90;
           }
        }
    }
    // dnode->x = marginX;
    // dnode->y = marginY;
    dnode->x = 0;
    dnode->y = 0;
    dnode->w = psWidth;
    dnode->h = psHeight;
    
    sprintf(newPlotPath,"/vnmr/tmp/prtXXXXXX");
    k = mkstemp(newPlotPath);
    if (k < 0) {
        Werrprintf("Print image error: could not open temporary file(1).");
        return;
    }

    close(k);
    if (outFile == NULL) {
        sprintf(tmpPath,"/vnmr/tmp/prtYYYYYY");
        k = mkstemp(tmpPath);
        if (k < 0) {
            Werrprintf("Print image error: could not open temporary file(2).");
            return;
        }
        close(k);
        outFile = tmpPath;
    }

    outfd = fopen(newPlotPath, "w");
    if (outfd == NULL) {
        releaseGifNode(ngf);
        unlink(newPlotPath);
        plotMode = 0;
        Werrprintf("Print image error: could not open temporary file(3).");
        return;
    }
    // convert from unix to windows paths if needed (otherwise just copy path strings)

    fprintf(outfd, "%%!PS-Adobe-3.0\n%%%%Title:VNMR Plotting\n");
    fprintf(outfd, "%%%%Creator:Vnmr Spectroscopist\n");
    fprintf(outfd, "%%%%BoundingBox: 0 0 %d %d\n", paperWidth, paperHeight );
    fprintf(outfd, "%%%%page 1 1\n");
    fprintf(outfd, "gsave\n");

    fprintf(outfd,"/setpagedevice where\n");
    fprintf(outfd,"{  pop 1 dict\n");
    fprintf(outfd,"   dup /PageSize [%d %d] put\n", paperWidth, paperHeight);
    fprintf(outfd,"   setpagedevice\n");
    fprintf(outfd,"} if\n");

    fprintf(outfd, "%%%%BeginProlog\n");
    write_ps_image_header(outfd);
    fprintf(outfd, "%%%%EndProlog\n");
    fprintf(outfd, "%d %d translate\n", transX, transY);
    if (rotateDegree != 0)
        fprintf(outfd, "%d rotate\n", rotateDegree);

    // WriteDirectPSImage(outfd, ngf);
    WritePseudoImage(outfd, ngf);

    fprintf(outfd, "stroke\n");
    fprintf(outfd, "/Courier-Bold findfont 12 scalefont setfont\n");
    fprintf(outfd, "1.0 1.0 1.0 setrgbcolor\n");
    fprintf(outfd, "0 0 moveto\n");
    fprintf(outfd, "(  ) show\n");
    fprintf(outfd, "%%plotpage\n");
    fprintf(outfd, "showpage grestore\n");
    fprintf(outfd, "%%%%Trailer\n");
    fprintf(outfd, "%%%%Pages: 1\n");
    fprintf(outfd, "%%%%EOF\n");
    fclose(outfd);
    plotMode = 0;
    free(ngf->dimNode);
    ngf->dimNode = NULL;
    releaseGifNode(ngf);

    sprintf(tmpCmd, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -dDEVICEWIDTHPOINTS=%d -dDEVICEHEIGHTPOINTS=%d -sOutputFile=%s ",paperWidth, paperHeight, outFile);
    if (outFormat == NULL || strcmp(outFormat, "ps") == 0) {
        // sprintf(inputs, "%s -sDEVICE=pswrite -q %s", tmpCmd, newPlotPath);
        sprintf(inputs, "mv %s %s", newPlotPath, outFile);
        ret = system(inputs);
    }
    else if (strcmp(outFormat, "pcl") == 0) {
        if (isColor)
           sprintf(inputs, "%s -sDEVICE=cljet5c -r%d -q %s", tmpCmd, dpi, newPlotPath);
        else
           sprintf(inputs, "%s -sDEVICE=ljet4 -r%d -q %s", tmpCmd, dpi, newPlotPath);
                 // sprintf(inputs, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=laserjet -sOutputFile=%s -r%d -q %s", outFile, dpi, newPlotPath);
        ret = system(inputs);
    }
    else {
        if (strcmp(outFormat, "pdf") == 0) {
           sprintf(inputs, "%s -sDEVICE=pdfwrite -r%d -q %s", tmpCmd, dpi, newPlotPath);
           ret = system(inputs);
        }
        else {
			image_convert("", "", newPlotPath, outFormat, outFile);
        }
    }
    unlink(newPlotPath);
    if (doPrint == 0) {
         sprintf(inputs, "chmod 644 %s", outFile);
         ret = system(inputs);
         Winfoprintf("Saving file: %s\n", outFile);
         return;
    }
    if (plotter != NULL)
         getOsPrinter(plotter);
         
    if (numCopies > 1) {
         if (plotter != NULL)
             sprintf(inputs, "print toprinter printer: %s copies: %d file: %s",
                               osPrinterName, numCopies, outFile);
         else
             sprintf(inputs, "print toprinter  copies: %d file: %s",
                               numCopies, outFile);
         writelineToVnmrJ("vnmrjcmd", inputs);
         return;
    }
    if (plotter == NULL) {
         // Winfoprintf("printjimage: systemPlot %s\n", outFile);
         systemPlot(outFile, 0, NULL);
         return;
    }
    doClear = 0;
    if (getenv("nmrplotter") != NULL) {
        sprintf(tmpCmd, "%s",getenv("nmrplotter"));
        setenv("nmrplotter", osPrinterName, 1);
        doClear = 1;
    }
    sprintf(inputs, "vnmrplot \"%s\" \"%s\"", outFile, osPrinterName);
       // Winfoprintf("printjimage: %s\n", inputs);
    exec_shell(inputs);

    if (access(outFile, F_OK) == 0)
       unlink(outFile);
    if (doClear) {
       setenv("nmrplotter", tmpCmd, 1);
    }
} 

/********************************************
imagefile('display', 'imagefile' [,x,y,w,h,'mol'] )
 send to VJ:  vnmrjcmd icon display image_id imagefile x y w h [ mol ]
imagefile('resize', image_id, x,y ,w,h)
 send to VJ:vnmrjcmd icon resize image_id  x y w h
imagefile('delete', image_id )
 send to VJ: vnmrjcmd icon delete image_id
imagefile('on', image_id )   // show border
 send to VJ: vnmrjcmd icon on image_id
imagefile('off', image_id )   // no border
 send to VJ: vnmrjcmd icon off image_id
imagefile('clear')  // clear all images
 send to VJ: vnmrjcmd icon clear
imagefile('displayall')  // display all previous images
 send to VJ: vnmrjcmd icon displayall

 send to VJ: vnmrjcmd icon dimension image_id  x y w h  // image size changed

*************************************************/
 
int
imagefile(int args, char *argv[], int retc, char *retv[])
{

     int   x, y, w, h, k, k2;
     int   num, imgId, isMol;
     int   mode;
     char  *file;
     char  *retStr;
     IMGNode *gf, *oldGf;
     DIM_NODE *nd;

     (void) retc;
     (void) retv;
     if (args < 2)
	RETURN;
     if (retc > 0)
        retv[0] = realString(0.0);

     mode = 0;
     fromVj = 0;
     if (debug) {
        for (k = 0; k < args; k++)
           fprintf(stderr, "%s, ", argv[k]);
        fprintf(stderr, " \n");
     }

     if (strcmp(argv[args-1], "-vj") == 0) {
         fromVj = 1;
         args--;
     }
     if (args < 2)
	RETURN;
     for (k = 1; k < CMDNUM; k++) {
        if (strcmp(argv[1], cmds[k]) == 0) {
		mode = k;
		break;
	}
     }
     if (mode == 0) {
        if (strcmp(argv[1], "plotter") == 0)
            mode = IPLOT;
        else
            RETURN;
     }
     plotMode = 0;
     if (mode == IPLOT || mode == IPLTALL) {
        if (setplotter())
            RETURN;
	plotState = 1;
        if (plot_raster() < 3)
            RETURN;
        scrnDpi = getScreenDpi();
        setPlotDimension();
        plotMode = 1;
        if (mode == IPLTALL) {
	    plot_all_image();
            plotMode = 0;
            RETURN;
	}
     }
     if (mode == IDISPALL) {
	if (plotState) {
	   setdisplay();
	   plotState = 0;
	}
	if (imgGifList == NULL) {
     	   if (!clearFlag) {
	      imgGifList = oldGifList;
	      oldGifList = NULL;
	   }
	}	
	if (imgGifList == NULL) {
	   if (debug)
	      fprintf(stderr, "   image list is empty \n");
     	   if (clearFlag)
              RETURN;
	   clear_overlay_image();
	   create_all();    
        }
        sprintf(inputs, "icon");
	for (k = 1; k < args; k++) {
	      strcat (inputs, " ");
	      strcat (inputs, argv[k]);
	}
        if (fromVj == 0)
           writelineToVnmrJ("vnmrjcmd", inputs);

        gf = imgGifList;
        while (gf != NULL) {
            gf->hide = 0;
            gf = gf->next;
        }
        if (winWidth != graf_width || winHeight != graf_height) {
	   count_overlay_image();
	   redraw_overlay_image();
	}
	else {
	   repaint_overlay_image();
           disp_overlay_image(0, 1);
	}
        RETURN;
     }
     if (mode == ICLEAR) {
        clearFlag = 1;
	clear_overlay_image();
	clear_overlay_map();
        disp_overlay_image(1, 1);
        save_imagelist_geom(NULL);
        clean_house();
        if (fromVj == 0) {
           sprintf(inputs, "icon deleteall");
           writelineToVnmrJ("vnmrjcmd", inputs);
        }
        RETURN;
     }
     if (mode == DEBUGMODE) {
	if (debug)
	    debug = 0;
	else
	    debug = 1;
        RETURN;
     }
     if (args < 3)
	RETURN;
     file = argv[2];
     imgId = 0;
     x = 0; y = wc2max;
     w = -1; h = -1;
     if (mode == IDISP || mode == IPLOT) {
	k2 = 1; // no id
        num = 3;
     }
     else {
        k2 = 0;
        num = 2;
     }
     retStr = NULL;
     isMol = 0;
     while (num < args) {
	if ((argv[num][0] == '-' || argv[num][0] == '+'))
	    k = 1;
	else
	    k = 0;
        if (isdigit(argv[num][k]) != 0) {
            k = atoi(argv[num]);
            if (k2 == 0)
		imgId = k;
            if (k2 == 1)
                x = k;
            else if (k2 == 2)
                y = k;
            else if (k2 == 3)
                w = k;
            else if (k2 == 4)
                h = k;
            k2++;
        }
        else {
            retStr = argv[num];
	    if (strcmp(retStr, "mol") == 0)
		isMol = 1;
	}
        num++;
     }
     if (mode == IPLOT) {
	plot_image(file, isMol, x, y, w, h);
	plotMode = 0;
	RETURN;
     }
     gf = NULL;
     if (mode == IDISP) {
	gf = get_used_node(file);
	if (gf == NULL) {
           gf = open_imagefile(file, isMol);
           if (gf == NULL) {
              Werrprintf("Could not open image file %s\n",file);
              RETURN;
           }
	}
	if (gf->dimNode == NULL) {
	    gf->dimNode = get_new_icon_info_node(file);
	    if (gf->dimNode == NULL) {
		releaseGifNode(gf);
		RETURN;
	    }
	}
	gf->dimNode->molFile = isMol;
        if (imgGifList != NULL)
            imgGifList->prev = gf;
	gf->next = imgGifList;
        imgGifList = gf;
	imgNum++;
	gf->id = imgNum;
        gf->hilit = 0;
        gf->hide = 0;
     }
     else if (imgId > 0) {
        gf = imgGifList;
        while (gf != NULL) {
            if (gf->id == imgId)
		break;
	    gf = gf->next;
	}
     }

     if (gf == NULL)
         RETURN;
     nd = gf->dimNode;
     if (nd == NULL)
        RETURN;

     if (retc > 0)
        retv[0] = realString((double)gf->id);
     if (mode == ION) {
        if (gf->hilit)
     	   RETURN;
        gf->hilit = 1;
     }
     else if (mode == IOFF) {
        if (!gf->hilit)
     	   RETURN;
        gf->hilit = 0;
     }

     if (mode != IDISP) /* not new image */
	clean_icon_area(gf);
     if (mode == IDISP || mode == ISIZE) {
	if (plotState) {
	   setdisplay();
	   plotState = 0;
	}
	set_img_geom(gf, mode, x, y, w, h);
     }
     if (mode == IDEL) {
	remove_node(gf);
        save_imagelist_geom(imgGifList);
	if (imgGifList == NULL) {
	    clear_overlay_image();
            clearFlag = 1;
	}
        imageFrameOp(imgId, 0, "delete");
     }
     if (mode == IHIDE) {
        gf->hide = 1;
     }
     if (mode != IDEL && mode != IHIDE) {
        draw_overlay_image(gf->data, gf->width, gf->height,
           nd->x, nd->y, nd->w, nd->h, gf->pixels, gf->bgColor, gf->hilit);
     }

     if (mode == IDISP) {
        clearFlag = 0;
        gf->hide = 0;
        disp_overlay_image(0, 1);
        if (w > 0)
            w = nd->w;
        else
            w = 0; 
        if (h > 0)
            h = nd->h;
        else
            h = 0; 
        if (retStr != NULL)
          sprintf(inputs, "icon %s %d %s %d %d %d %d %s", cmds[mode], gf->id, 
		gf->fpath, nd->x, nd->y, w, h, retStr);
        else
          sprintf(inputs, "icon %s %d %s %d %d %d %d", cmds[mode], gf->id, 
		gf->fpath, nd->x, nd->y, w, h);
        if (fromVj == 0)
           writelineToVnmrJ("vnmrjcmd", inputs);
        RETURN;
     }
     if (mode == ILOAD) {
        if (args < 4)
            RETURN;
        oldGf = gf;
        file = argv[3];
        gf = open_imagefile(file, isMol);
        if (gf == NULL)
            RETURN;
        if (gf->dimNode == NULL) {
            gf->dimNode = get_new_icon_info_node(file);
            if (gf->dimNode == NULL) {
                releaseGifNode(gf);
                RETURN;
            }
        }
        gf->dimNode->molFile = isMol;
        if (imgGifList != NULL)
            imgGifList->prev = gf;
        gf->next = imgGifList;
        imgGifList = gf;
        gf->id = imgId; 
        if (fromVj == 0) {
            nd = gf->dimNode;
            sprintf(inputs, "icon display %d %s -1 -1 0 0", gf->id, gf->fpath);
            if (isMol)
               strcat(inputs, " mol");
            writelineToVnmrJ("vnmrjcmd", inputs);
        }
        if (oldGf != NULL) {
            copy_dim_node(oldGf->dimNode, gf->dimNode);
            remove_node(oldGf);
        }
        RETURN;
     }
     disp_overlay_image(1, 1);
     if (mode == ISIZE) {
          // sprintf(inputs, "icon %s %d %d %d %d %d", cmds[mode], gf->id, 
          //		nd->x, nd->y, nd->w, nd->h);
           raiseNodeToTop(gf);
     }
     else if (fromVj == 0) {
           sprintf(inputs, "icon %s %d", cmds[mode], gf->id);
           writelineToVnmrJ("vnmrjcmd", inputs);
     }

     RETURN;
}

void
imagefile_init()
{
     IMGNode *gf;
     DIM_NODE *nd;
     FILE  *plotFd;

     if (plot_raster() < 3)  /* not PS */
         return;
     gf = imgGifList;
     while (gf != NULL) {
	gf->ploted = 0;
	gf = gf->next;
     }
     gf = oldGifList;
     while (gf != NULL) {
	gf->ploted = 0;
	gf = gf->next;
     }
     nd = dimList;
     while (nd != NULL) {
	nd->ploted = 0;
	nd = nd->next;
     }

     releasePlotList();

     plotFd = plot_file();
     if (plotFd != NULL)
         fprintf(plotFd, "%%%s\n", imageMark);
}

/* look for overlay area */
static void
find_overlay_area()
{
    PLOT_NODE  *node, *n2;
    OV_NODE  *ov, *ov2;
    int	  x1, x2, y1, y2;
    int	  ix1, ix2, iy1, iy2;
    int	  iw, ih, k, overlaied;
    float rk;

    if (plotList == NULL)
        return;
    ov2 = ovList;
    node = plotList;
    while (node->next != NULL) {
	x1 = node->x;
	x2 = node->x2;
	y1 = node->y;
	y2 = node->y2;
        if (node->covered)
	   n2 = NULL;
	else
	   n2 = node->next;
	while (n2 != NULL) {
	    overlaied = 0;
	    if (n2->x < x2 && n2->x2 > x1) {
	        if (n2->y < y2 && n2->y2 > y1) {
		     if (n2->x < x1)
			ix1 = x1;
		     else
			ix1 = n2->x;
		     if (n2->x2 < x2)
			ix2 = n2->x2;
		     else
			ix2 = x2;
		     if (n2->y < y1)
			iy1 = y1;
		     else
			iy1 = n2->y;
		     if (n2->y2 < y2)
			iy2 = n2->y2;
		     else
			iy2 = y2;
		     ov = (OV_NODE *) malloc(sizeof(OV_NODE));
		     if (ov == NULL)
			return;
	    	     overlaied = 1;
		     ov->x = ix1;
		     ov->y = iy1;
		     ov->x2 = ix2;
		     ov->y2 = iy2;
		     ov->covered = 0;
		     ov->rotateDegree = 0;
		     ov->next = NULL;
		     rk = (float) (ix2 - ix1) / (float) node->width;
		     k = (int) ((float) node->gf->width * rk);
		     rk = (float) (ix2 - ix1) / (float) n2->width;
		     iw = (int) ((float) n2->gf->width * rk);
		     if (k > iw)
			iw = k;
		     ov->width = iw;
		     rk = (float) (iy2 - iy1) / (float) node->height;
		     k = (int) ((float) node->gf->height * rk);
		     rk = (float) (iy2 - iy1) / (float) n2->height;
		     ih = (int) ((float) n2->gf->height * rk);
		     if (k > ih)
			ih = k;
		     ov->height = ih;
		     if (ovList == NULL)
			ovList = ov;
		     else
			ov2->next = ov;
		     ov2 = ov;
		}
	    }
	    if (overlaied) {
	        if (ov->x <= x1 && ov->x2 >= x2) {
	            if (ov->y <= y1 && ov->y2 >= y2) {
			node->covered = 1;
			break;
		    }
		}
	        if (ov->x <= n2->x && ov->x2 >= n2->x2) {
	            if (ov->y <= n2->y && ov->y2 >= n2->y2)
			n2->covered = 1;
		}
	    }
	    n2 = n2->next;
	}
        node = node->next;
    }
    if (ovList != NULL) {
       /* remove duplicated overlay areas */
	ov = ovList;
	while (ov != NULL) {
	    x1 = ov->x;
	    x2 = ov->x2;
	    y1 = ov->y;
	    y2 = ov->y2;
	    ov2 = ov->next;
	    while (ov2 != NULL) {
	      if (ov2->x < x2 && ov2->x2 > x1 && ov2->y < y2 && ov2->y2 > y1) {
		 if (ov2->x < x1)
		    ix1 = x1;
		 else
		    ix1 = ov2->x;
		 if (ov2->x2 < x2)
		    ix2 = ov2->x2;
		 else
		    ix2 = x2;
		 if (ov2->y < y1)
		    iy1 = y1;
		 else
		    iy1 = ov2->y;
		 if (ov2->y2 < y2)
		    iy2 = ov2->y2;
		 else
		    iy2 = y2;
	    	 overlaied = 0;
	         if (ix1 <= ov2->x && ix2 >= ov2->x2) {
	            if (iy1 <= ov2->y && iy2 >= ov2->y2) {
			ov2->covered = 1;
	    	        overlaied = 1;
		    }
		 }
		 if (!overlaied) {
	            if (ix1 <= x1 && ix2 >= x2) {
	               if (iy1 <= y1 && iy2 >= y2)
			  ov->covered = 1;
		    }
		 }
	      }
	      ov2 = ov2->next;	
	    }
	    ov = ov->next;
	}
    }  /* if ovList != NULL */
}


static void
map_image(ov, pnode)
OV_NODE  *ov;
PLOT_NODE  *pnode;
{
     IMGNode    *gf;
     byte       *r, *g, *b, *s, *s2;
     byte       bg;
     int	x, y, k, c;
     int	sx, sy, sx2, sy2;
     int	dx, dy, dx2, dy2;
     int	x1, x2, y1, y2;
     float      rx, ry, rv, rx2;

     gf = pnode->gf;
     r = pnode->cR;
     g = pnode->cG;
     b = pnode->cB;
     if (ov->x < pnode->x)
	x1 = pnode->x;
     else
	x1 = ov->x;
     if (ov->x2 > pnode->x2)
	x2 = pnode->x2;
     else
	x2 = ov->x2;
     if (ov->y > pnode->y)
	y1 = ov->y;
     else
	y1 = pnode->y;
     if (ov->y2 > pnode->y2)
	y2 = pnode->y2;
     else
	y2 = ov->y2;
     rx = (float) (x1 - pnode->x) / (float) pnode->width;
     sx = (int) (rx * (float) gf->width + 0.5);
     rx = (float) (x2 - pnode->x) / (float) pnode->width;
     sx2 = (int) (rx * (float) gf->width + 0.5);
     rx = (float) (pnode->y2 - y2) / (float) pnode->height;
     sy = (int) (rx * (float) gf->height + 0.5);
     rx = (float) (pnode->y2 - y1) / (float) pnode->height;
     sy2 = (int) (rx * (float) gf->height + 0.5);

     rx = (float) (x1 - ov->x) / (float) (ov->x2 - ov->x);
     dx = (int) (rx * (float) ov->width + 0.5);
     rx = (float) (x2 - ov->x) / (float) (ov->x2 - ov->x);
     dx2 = (int) (rx * (float) ov->width + 0.5);
     rx = (float) (ov->y2 - y2) / (float) (ov->y2 - ov->y);
     dy = (int) (rx * (float) ov->height + 0.5);
     rx = (float) (ov->y2 - y1) / (float) (ov->y2 - ov->y);
     dy2 = (int) (rx * (float) ov->height + 0.5);

     rx = (float)(sx2 - sx) / (float) (dx2 - dx);
     ry = (float)(sy2 - sy) / (float) (dy2 - dy);

     if (gf->bgColor < 0)
	bg = pnode->whiteColor;
     else
        bg = gf->bgColor;
     if (gf->transparent == 0)
        bg = 255;
     for (y = dy; y < dy2; y++) {
	rv = (float) (y - dy) * ry;
        k = (int) rv + sy;
	s = gf->data + gf->width * k; /* source data */
	rx2 = 0;
	for (x = dx; x < dx2; x++) {
	    rx2 = (float) (x - dx) * rx;
            k = (int) rx2 + sx;
	    s2 = s + k;
	    if (*s2 != bg) {
		k = y * ov->width + x;
		c = (int) *s2;
		ovR[k] = r[c];
		ovG[k] = g[c];
		ovB[k] = b[c];
	    }
	}
     }
}

static void
plot_overlay_area()
{
    PLOT_NODE  *np;
    OV_NODE  *ov;
    int    k, num;

    if (ovList == NULL)
        return;
    num = 0;
    ov = ovList;
    while (ov != NULL) {
	k = ov->width * ov->height;
	if (k > num)
	    num = k;
	ov = ov->next;
    }
    if (num < 1)
        return;
    k = sizeof(byte) * num;
    ovR = (byte *) malloc(k);
    ovG = (byte *) malloc(k);
    ovB = (byte *) malloc(k);
    if (ovR == NULL || ovG == NULL || ovB == NULL) {
	if (ovB != NULL)
	   free(ovB);
	if (ovR != NULL)
	   free(ovR);
	if (ovG != NULL)
	   free(ovG);
        return;
    }

    ov = ovList;
    while (ov != NULL) {
       if (!ov->covered) {
          num = ov->width * ov->height;
          /* clear image map */
	  for (k = 0; k < num; k++) {
	    ovR[k] = 255;
	    ovG[k] = 255;
	    ovB[k] = 255;
	  }
	  num = 0;
          np = plotList;
	  while (np != NULL) {
	     if (np->x < ov->x2 && np->x2 > ov->x) {
	       if (np->y < ov->y2 && np->y2 > ov->y) {
		   map_image(ov, np);
		   num++;
		}
	     }
	     np = np->next;
	  }
	  if (num > 0 && newPlotFd != NULL)
	     WriteOverlayImage(newPlotFd, ov);
	}
	ov = ov->next;
    }
    free(ovR);
    free(ovG);
    free(ovB);
}


/*  plot all images */
int
imagefile_flush()
{
     PLOT_NODE  *node;
//     IMGNode    *gf;
     char  *d, *oldPath;
     FILE  *plotFd;
//     int  isMarked = 0;

    if (plotList == NULL)
	return (0);
    if (setplotter())
        return (0);
    if (plot_raster() < 3)
        return (0);
    plotFd = plot_file();
    if (plotFd == NULL)
        return (0);
    oldPath = plot_file_name();
    if (oldPath == NULL)
        return (0);

    setPlotDimension();
    if (debug) {
       color(RED);
       amove(1, 1);
       rdraw(mnumxpnts, 0);
       amove(1, mnumypnts -1);
       rdraw(mnumxpnts, 0);
       amove(1, 1);
       rdraw(0, mnumypnts);
       amove(mnumxpnts - 1, 1);
       rdraw(0, mnumypnts);
       amove(mnumxpnts / 2, mnumypnts -ycharpixels);
       dstring(" ");
    }

    sprintf(newPlotPath,"%s0", oldPath);
    newPlotFd = fopen(newPlotPath, "w+");
    if (newPlotFd == NULL)
        return (0);
    rewind(plotFd);
    /*******
    if (fseek(plotFd, 0L, SEEK_SET) != 0) {
	fclose(newPlotFd);
        newPlotFd = NULL;
        return (0);
    }
    *******/
    while ((d = fgets(inputs, 500, plotFd)) != NULL) {
	if (*d == '%' && strncmp(d+1, imageMark,(int) strlen(imageMark)) == 0) {
            //isMarked = 1;
	    break;
        }
	else
	    fprintf(newPlotFd, "%s", inputs);
    }

    if (plotList != NULL) {
        plotState = 1;
        bImgFlush = 1;
        // WriteImageTransform(newPlotFd);
        // if (isMarked)
        //     write_ps_image_header(newPlotFd);
        find_overlay_area();

        node = plotList;
        while (node != NULL) {
           // gf = node->gf;
           if (!node->covered)
	        WritePSImage(newPlotFd, node);
           node = node->next;
        }

        plot_overlay_area();
    }
     
    plotMode = 0;
    while (fgets(inputs, 1000, plotFd) != NULL) {
	fprintf(newPlotFd, "%s", inputs);
    }
     
    bImgFlush = 0;
    fclose(plotFd);
    unlink(oldPath);
    replace_plot_file(newPlotPath, newPlotFd);
    releasePlotList();
    newPlotFd = NULL;
    return (1);
}

int
ps_imagefile_flush()
{
     PLOT_NODE  *node;
//     IMGNode    *gf;
     FILE  *plotFd;

    if (plotList == NULL) {
         return (0);
    }
    if (setplotter())
        return (0);
    if (plot_raster() < 3)
        return (0);
    plotFd = plot_file();
    if (plotFd == NULL)
        return (0);
    newPlotFd = plotFd;
    setPlotDimension();
    plotState = 1;
    find_overlay_area();

    node = plotList;
    while (node != NULL) {
        //gf = node->gf;
        if (!node->covered)
             WritePSImage(newPlotFd, node);
        node = node->next;
    }

    plot_overlay_area();
    plotMode = 0;
    releasePlotList();
    newPlotFd = NULL;
    return (1);
}


//#define DEBUG_CONVERT
void image_convert(char *options, char *fmt1, char *file1, char *fmt2, char *file2) {
    char data[MAXPATH];
    char f1[16];
    char f2[16];
    int  ret __attribute__((unused));

    strncpy(f1,fmt1,14);
    strncpy(f2,fmt2,14);
    if(strlen(f1)>0)
    	strcat(f1,":");
    if(strlen(f2)>0)
    	strcat(f2,":");

#ifdef __INTERIX
    extern void unixPathToWin(char *path,char *buff,int maxlength);
    char src_path[MAXPATH];
    char dst_path[MAXPATH];
	unixPathToWin(file1,src_path,MAXPATH);
	unixPathToWin(file2,dst_path,MAXPATH);
	sprintf(data, "convert %s %s%s %s%s", options, f1,src_path, f2, dst_path);
	exec_shell(data);
#else
	sprintf(data, "convert %s %s%s %s%s", options, f1,file1, f2, file2);
	ret = system(data);
#endif
#ifdef DEBUG_CONVERT
	Winfoprintf("image_convert:%s",data);
#endif
}

static void
iplot_images()
{
#ifdef VNMRJ
    PLOT_NODE  *node;
    IMGNode    *gf;
    DIM_NODE   *dim;
    int x, y;

    plotMode = 1;
    plot_all_image();

    node = plotList;
    while (node != NULL) {
        gf = node->gf;
        if (gf->data == NULL || gf->fpath == NULL)
             break;
        if (gf->dimNode == NULL)
             break;
        dim = gf->dimNode;
        x = dim->x;
        if ((x + dim->w) > plotWidth)
            x = plotWidth - dim->w;
        y = plotHeight - (dim->y + dim->h);
        if (y < 0)
            y = 0;
        fprintf(iplotFd, "#func 25 4 ");  // ICON
        fprintf(iplotFd, "%d %d %d %d \n",x, y, dim->w, dim->h);
        fprintf(iplotFd, "%s\n", gf->fpath);
        node = node->next;
    }
    plotMode = 0;
    releasePlotList();
#endif
}

int iplot(int argc, char *argv[], int retc, char *retv[])
{
#ifdef VNMRJ
    int    pid, k, orient;
    int    preview, origBnmr, idebug;
    char   *oldPath, *outFile, *outFormat;
    char   pltName[MAXPATHL];
    char   operatorName[MAXSTR];
    double dv;
    double fv;
    struct timeval clock;
    int    ret __attribute__((unused));

    outFile = NULL;
    outFormat = NULL;
    preview = 0;
    idebug = 0;
    origBnmr = Bnmr;

    for (k = 1; k < argc; k++) {
        if (argv[k][0] == '-') {
            if (strcmp(argv[k], "-format") == 0) {
                k++;
                if (k < argc)
                    outFormat = argv[k];
                continue;
            }
            if (strcmp(argv[k], "-view") == 0) {
                preview = 1;
                continue;
            }
            if (strcmp(argv[k], "-preview") == 0) {
                preview = 1;
                continue;
            }
            if (strcmp(argv[k], "-debug") == 0) {
                idebug = 1;
                continue;
            }
        }
        else {
            if (outFile == NULL)
                outFile = argv[k];
            else if (outFormat == NULL)
                outFormat = argv[k];
        }
    }

    fflush(NULL);

    if (origBnmr != 0)
         preview = 1;

    Wturnoff_message();
    if (graphics_colors_loaded() < 3) {
         execString("loadcolors('DEFAULT')\n");
    }
    if (ps_colors_loaded() < 3)
         execString("loadcolors\n");
    Wturnon_message();

    if ((pid = fork()) < 0) {
          showError("Could not fork background process to do iplot!");
          RETURN;
    }

    if (pid != 0) {
          set_wait_child(pid);
          RETURN;
    }

    Wturnoff_message();
    scrnDpi = getScreenDpi();
    setVjUiPort(0);
    iplotFd = plot_file();
    oldPath = plot_file_name();
    if (setplotter()) {
         setPlotterName("");
         if (setplotter()) {
              showError("Could not set plot environment!");
              exit(1);
         }
    }
    if (iplotFd == NULL) {
         iplotFd = plot_file();
         oldPath = plot_file_name();
         if (iplotFd != NULL) {
             fclose(iplotFd);
             if (strlen(oldPath) > 1) // remove the file was created by setplotter
	        unlink(oldPath);
         }
    }
    pltName[0] = '\0';
    if (outFile == NULL) {
        if (P_getstring(GLOBAL,"plotter",pltName,1,MAXPATHL) == 0)
            getOsPrinter(pltName);
    }

    if (P_getstring(GLOBAL,"operator",operatorName,1,MAXSTR) != 0)
        strcpy(operatorName, "vnmrj");

    sprintf(iplotPath,"%s/persistence/plotpreviews/%s_tmpplot_XXXXXX", userdir, operatorName);
    if (mkstemp(iplotPath) < 0)
         sprintf(iplotPath,"%s/tmp/PLT123", systemdir);
    iplotFd = fopen(iplotPath,"w");
    if (iplotFd == NULL) {
         sprintf(inputs, "Could not open plot file %s", iplotPath);
         showError(inputs);
         exit(1);
    }
    sprintf(inputs,"%s/persistence/plotpreviews/%s_tmpplot_TopXXXXXX", userdir, operatorName);
    if (mkstemp(inputs) < 0)
         sprintf(inputs,"%s/tmp/PLT123", systemdir);
    iplotTopFd = fopen(inputs,"w");
    if (iplotTopFd != NULL)
        fprintf(iplotFd, "#topfile  %s\n", inputs);

    orient = plot_raster();
    plotWidth = getPlotWidth();  // mnumxpnts
    plotHeight = getPlotHeight();  // mnumypnts
    dv = getpaperwidth();
      // convert millimeters to ps unit (72 per inch)
    paperWidth =  (int) (dv * 72.0 / 25.4 + 0.5);
    dv = getpaperheight();
    paperHeight = (int) (dv * 72.0 / 25.4 + 0.5);
    if (outFile != NULL)
        fprintf(iplotFd, "#file  %s\n", outFile);
    if (outFormat != NULL)
        fprintf(iplotFd, "#format  %s\n", outFormat);
    if (osPrinterName != NULL)
        fprintf(iplotFd, "#printer  %s\n", osPrinterName);
    fprintf(iplotFd, "#raster  %d\n", orient);
    if (orient == 2 || orient == 4)
        fprintf(iplotFd, "#orientation  landscape\n");
    else
        fprintf(iplotFd, "#orientation  portrait\n");
    fprintf(iplotFd, "#paperwidth  %d\n", paperWidth);
    fprintf(iplotFd, "#paperheight  %d\n", paperHeight);
    fprintf(iplotFd, "#imagewidth  %d\n", plotWidth);
    fprintf(iplotFd, "#imageheight  %d\n", plotHeight);
    dv =  getpaper_leftmargin();
    dv =  dv * 72.0 / 25.4;
    fprintf(iplotFd, "#leftmargin  %g\n", dv);
    dv =  getpaper_topmargin();
    dv =  dv * 72.0 / 25.4;
    fprintf(iplotFd, "#topmargin  %g\n", dv);
    fv = getPSscaleX();
    if (fv > 0.0 && fv < 1.0)
        fprintf(iplotFd, "#scalex  %.4g\n", fv);
    fv = getPSscaleY();
    if (fv > 0.0 && fv < 1.0)
        fprintf(iplotFd, "#scaley  %.4g\n", fv);
    if (preview)
        fprintf(iplotFd, "#view  1\n");
    if (idebug)
        fprintf(iplotFd, "#debug  1\n");
    jplot_charsize(1.0);
    fprintf(iplotFd, "#charx  %d\n", xcharpixels);
    fprintf(iplotFd, "#chary  %d\n", ycharpixels);
    dv = get_ploter_ppmm();
    if (dv < 1.0)
        dv = 11.811;
    dv = dv * 25.4 + 0.5;
    fprintf(iplotFd, "#dpi  %d\n", (int) dv);
    fprintf(iplotFd, "#noUi  %d\n", origBnmr);
    fprintf(iplotFd, "#userdir  %s\n", userdir);
    fprintf(iplotFd, "#systemdir  %s\n", systemdir);
    fprintf(iplotFd, "#operator  %s\n", operatorName);
    if (P_getreal(GLOBAL,"pslw" ,&dv, 1))
        dv = 1.0;
    fprintf(iplotFd, "#pslw  %d\n", (int) dv);
    fprintf(iplotFd, "#colors  %d\n", MAX_COLOR);
    gettimeofday(&clock, NULL);
    outFormat = ctime(&(clock.tv_sec));
    if (outFormat != NULL)
        fprintf(iplotFd, "#date  %s\n", outFormat);
    // fprintf(iplotFd, "#endSetup\n");
    Bnmr = 0;

    iplot_images();

    start_iplot(iplotFd, iplotTopFd, plotWidth, plotHeight);

    fclose(iplotFd);
    if (iplotTopFd != NULL)
        fclose(iplotTopFd);

    if (origBnmr == 0) {
         setVjUiPort(1);
         setVjPrintMode(0);
         sprintf(inputs, "iplot %s", iplotPath);
         writelineToVnmrJ("vnmrjcmd", inputs);
    }
    else {
         sprintf(inputs, "vnmr_iplot %s", iplotPath);
         ret = system(inputs);
    }

    exit(1);

    RETURN;
#endif
}

static int headerX, headerY;
static int headerWidth, headerHeight;

static void
ps_header_data(int row, int col, char *str)
{
   int x, y, d;
   double l;

   if (str == NULL || (strlen(str) < 1))
      return;
   if (row < 0 || col < 0)
      return;
   d = headerWidth / 4;
   x = headerX + d * col;
   l = 0.0;
   if (row > 0)
      l = 0.8 + row;
   y = headerY - (int)(l * ycharpixels);
   l = 12;
   switch (col) {
      case 1:
            l = 12.5;
            if (row == 2)
               l = 7;
            break;
      case 2:
            l = 11;
            break;
      case 3:
            l = 10.5;
            if (row == 2)
               l = 7.5;
            break;
   }
   x = x + l * xcharpixels;
   amove(x, y);
   dstring(str);
}

static void
ps_header(int row, int col, char *str)
{
   int x, y, d;
   double l;

   if (str == NULL || (strlen(str) < 1))
      return;
   d = headerWidth / 4;
   x = headerX + d * col;
   l = 0.0;
   if (row > 0)
      l = 0.8 + row;
   y = headerY - (int)(l * ycharpixels);
   amove(x, y);
   dstring(str);
}

void
plot_std_header(int x, int y, int w, int h)
{
    int  y2;
    double dv;
    char  data[MAXSTR];

    setplotter();
    setPlotDimension();
    headerX = x;
    headerY = y;
    headerWidth = w;
    headerHeight = h;
    ps_info_font();
    ps_dark_color();
    ps_header(0, 0, "SAMPLE INFORMATION");
    ps_light_color();
    y2 = headerY - ycharpixels * 0.5;
    amove(headerX, y2);
    rdraw(headerWidth, 0);
    y2 = headerY - ycharpixels * 3.3;
    amove(headerX, y2);
    rdraw(headerWidth, 0);

    ps_info_light_font();
    ps_header(1, 0, "Sample name");
    ps_header(1, 1, "Pulse sequence");
    ps_header(1, 2, "Temperature");
    ps_header(1, 3, "Study owner");
    ps_header(2, 0, "Date collected");
    ps_header(2, 1, "Solvent");
    ps_header(2, 2, "Spectrometer");
    ps_header(2, 3, "Operator");
    if (P_getstring(CURRENT,"file",data,1,MAXSTR) == 0) {
        y2 = ycharpixels * 2;
        ps_light_color();
        amove(xcharpixels * 7, -y2);
        dstring(data);
        ps_rgb_color(120.0, 120.0, 120.0);
        amove(0, -y2);
        dstring("Data file ");
    }

    ps_dark_color();
    ps_info_font();
    if (P_getstring(CURRENT,"samplename",data,1,MAXSTR) == 0)
        ps_header_data(1, 0, data);
    if (P_getstring(CURRENT,"seqfil",data,1,MAXSTR) == 0)
        ps_header_data(1, 1, data);
    if (P_getreal(CURRENT,"temp" ,&dv, 1) == 0) {
        sprintf(data, "%.1f C", dv);
        ps_header_data(1, 2, data);
    }
    if (P_getstring(GLOBAL,"owner",data,1,MAXSTR) == 0)
        ps_header_data(1, 3, data);
    if (P_getstring(CURRENT,"date",data,1,MAXSTR) == 0)
        ps_header_data(2, 0, data);
    if (P_getstring(CURRENT,"solvent",data,1,MAXSTR) == 0)
        ps_header_data(2, 1, data);
    if (P_getstring(CURRENT,"systemname_",data,1,MAXSTR) == 0)
        ps_header_data(2, 2, data);
    if (P_getstring(GLOBAL,"operator",data,1,MAXSTR) == 0)
        ps_header_data(2, 3, data);
}

int
plot_header(char *fname, int x, int y, int w, int h) {
    FILE  *fd;
    FILE  *fout;
    char  *data, *key, *value;
    char  data1[64];
    int   r, c, y2, dock;
    int maxrow = 1;

    if (fname == NULL || (strlen(fname) < 1)) {
        plot_std_header(x, y, w, h);
        return(0);
    }
    setplotter();
    fout = plot_file();
    if (fout == NULL)
        return(0);
    fd = fopen(fname, "r");
    if (fd == NULL)
        return(0);
    setPlotDimension();
    headerX = x;
    headerY = y;
    headerWidth = w;
    headerHeight = h;
    ps_info_font();
    ps_light_color();
    y2 = headerY - ycharpixels * 0.5;

    while ((data = fgets(inputs, 500, fd)) != NULL) {
        while (*data == ' ' || *data == '\t')
            data++;
        if (strlen(data) < 1)
            continue;
        key = strtok(data, ":\n");
        if (key == NULL || (strlen(key) < 2))
            continue;
        value = strtok(NULL, "\n");
        if (value != NULL) {
            while (*value == ' ' || *value == '\t')
                value++;
        }
        if (strcmp(key, "Left") == 0) {
            headerX = 0;
            continue;
        }
        if (strcmp(key, "Center") == 0) {
            headerWidth += headerX;
            headerX = 0;
            continue;
        }
        if (strcmp(key, "Title") == 0) {
            amove(headerX, y2);
            rdraw(headerWidth, 0);
            if ((data = fgets(inputs, 500, fd)) != NULL) {
                while (*data == ' ' || *data == '\t')
                    data++;
                ps_dark_color();
                ps_info_font();
                ps_header(0, 0, data);
            }
            continue;
        }
        if (strcmp(key, "Header") == 0) {
            if (value == NULL)
                continue;
            if (sscanf(value, "%d%d", &r, &c) == 2) {
                if (r > maxrow)
                   maxrow = r;
                ps_info_light_font();
                if ((data = fgets(inputs, 500, fd)) != NULL) {
                    while (*data == ' ' || *data == '\t')
                        data++;
                    ps_light_color();
                    ps_info_light_font();
                    ps_header(r, c - 1, data);
                    if ((data = fgets(inputs, 500, fd)) != NULL) {
                        while (*data == ' ' || *data == '\t')
                            data++;
                        ps_dark_color();
                        ps_info_font();
                        fprintf(fout, "%d 0 rv\n", xcharpixels);
                        fprintf(fout, "(%s) show\n", data);
                     }
                }
            }
            continue;
        }
        if (strcmp(key, "Footer") == 0) {
            dock = 0;
            if (value != NULL) {
                if (strcmp(value, "right") == 0)
                    dock = 1;
            }
            y2 = ycharpixels * 2;
            data1[0] = '\0';
            if ((data = fgets(inputs, 500, fd)) == NULL)
                break;
            while (*data == ' ' || *data == '\t')
                data++;
            key = strtok(data, "\n");  // remove '\n'
            strncpy(data1, key, 60);
            value = NULL;
            data = fgets(inputs, 500, fd);
            if (data != NULL) {
                while (*data == ' ' || *data == '\t')
                    data++;
                value = strtok(data, "\n");
            }
            ps_flush();
            ps_info_light_font();
            if (dock == 1) {  // right side
                amove(mnumxpnts - xcharpixels, -y2);
                if (value != NULL)
                   fprintf(fout, "(%s) stringwidth pop  neg 0 rmoveto\n", value);
                if (strlen(data1) > 0)
                   fprintf(fout, "(%s) stringwidth pop  neg 0 rmoveto\n", data1);
            }
            else
                amove(0, -y2);
            ps_rgb_color(120.0, 120.0, 120.0);
            if (strlen(data1) > 0)
                fprintf(fout, "(%s ) show\n", data1);
            if (value != NULL) {
                ps_light_color();
                // fprintf(fout, "%d  0 rv\n", xcharpixels);
                fprintf(fout, "(%s) show\n", value);
            }
        }
    }
    y2 = headerY - ycharpixels * (maxrow + 1.3);
    amove(headerX, y2);
    rdraw(headerWidth, 0);

    fclose(fd);
    return (1);
}

//  put logo at top-left corner
int
plot_logo(char *name, double x, double y, double w, double h, int sizescale)
{
    DIM_NODE *dim;
    int retV;

    if (name == NULL || (strlen(name) < 1))
        return(0);
    setplotter();
    setPlotDimension();
    newPlotNode = NULL;
    maxImgWidth = w;
    maxImgHeight = h;
    plotMode = 1;
    retV = plot_image(name, 0, (int) x, (int) y, -1,-1);
    plotMode = 0;
    maxImgWidth = 0;
    maxImgHeight = 0;
    dim = NULL;
    if (newPlotNode != NULL && newPlotNode->gf != NULL)
         dim = newPlotNode->gf->dimNode;
    if (dim != NULL) {
         dim->y = (int) (y * plotHeight / wc2max);
         dim->x = (int) (x * plotWidth / wcmax);
         if (sizescale)
         {
            dim->h = (int) (h * plotHeight / wc2max);
            dim->w = (int) (w * plotWidth / wcmax);
         }
         else
         {
            dim->h = (int) h;
            dim->w = (int) w;
         }
    }
    return (retV);
}

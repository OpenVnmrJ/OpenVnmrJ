/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
/* 
 */

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Programming Notes:							*
*  -----------------							*
*  									*
*  This file contains Gframe class routines that are only used for	*
*  image browser.							*
*									*
*  Description								*
*  -----------								*
*									*
*  A frame is the place where an image or spectrum can be drawn.  This	*
*  file contains routines to create/resize/move/copy/delete a frame.  	*
*									*
*  To create a frame:							*
*	Hold the LEFT mouse button down, drag, and relase the button.	*
*  To move a frame:							*
* 	Click the LEFT mouse button inside the frame, hold the LEFT	*
*  	mouse button down, and drag.					*
*  To resize a frame:							*
*	Click the LEFT mouse button inside the frame, position the mouse*
*	cursor close the either four corners of the frame, hold the LEFT*
*	mouse button down, and drag.					*
*  To copy a frame:							*
*	Click the LEFT mouse button inside the frame, hold down 'ctrl'	*
*   	key, hold the LEFT mouse button down, and drag.			*
*  To delete a frame:							*
*	User the properties menu.					*
*  To select a frame:							*
*	User the properties menu.					*
*									*
*  LEFT mouse button is used to select a specific frame, and unselect 	*
*       all other  frames.						*
*  MIDDLE mouse button is used to toggle between 'select' and 'unselect'*
*       of a frame.  It doesn't affect all other frames.		*
*  RIGHT mouse button is NOT functional.				*
*  									*
*  NOTE 								*
*  ----									*
*  - a frame should be large enough in order to exist.			*
*  - a frame must not overlap with other frames.			*
*									*
*************************************************************************/

//#define DEBUG

// This debug should be taken out at FINAL release
#define	DEBUG_BETA	1


#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stream.h>
#ifdef LINUX
#include <strstream>
#else
#include <strstream.h>
#endif
#include <math.h>
#include <unistd.h>


#include <sys/stat.h>
#include <sys/swap.h>
#include <ulimit.h>
#include <sys/resource.h>
#include <sys/param.h>
extern int _end;

#include "ddllib.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "stderr.h"
#include "common.h"
#include "msgprt.h"
#include "interrupt.h"
#include "filelist.h"
#include "filelist_id.h"
#include "initstart.h"
#include "zoom.h"
#include "inputwin.h"
#include "confirmwin.h"
#include "ddlfile.h"
#include "statmacs.h"
#include "debug.h"
#include "ibcursors.h"
#include "macroexec.h"
#include "voldata.h"

#define	Corner_Check(a,b,p,q,v)  com_point_in_rect(a, b, p-v, q-v, p+v, q+v)

// The following two definitions should be defined but are not in the
// current release of SABER-C++



extern void win_print_msg(char *, ...);

extern int canvas_width ;
extern int canvas_height ;
extern int canvas_stx ;
extern int canvas_sty ;
extern void canvas_repaint_proc(void);

// This variable consists a list of all frames created on the screen,
// except the first item. The first item of the list is a TEMPORARY item
// serves as a 'working' buffer (item).  DON'T count the first item as
// a frame.
extern Gframe *framehead;

// targetframe points the the next frame that data will be displayed in.

extern Gframe *targetframe;

extern int auto_skip_frame;
extern int load_silently;

// This variable consists a list of all selected frames, which its
// pointers (item) 'frameptr' point to the same address as in Gframe.
// Also note that the first item of the list is a TEMPORARY item which
// serves as a 'working' buffer.  DON'T count the first item as a 
// selected frame.
extern Frame_select *selecthead;

int Frame_routine::memPctThreshold = 80;


#ifdef DEBUG
#define DEBUG_FRAME debug_frame();
#else
#define DEBUG_FRAME
#endif DEBUG

void
loaddir(char* dirname)
{
    char **contents;
    char **dp;
    char *name;

    int cursor = set_cursor_shape(IBCURS_BUSY);

    interrupt_begin();
    contents = get_dir_contents(dirname);
    if (!contents){
	return;
    }
    for (dp=contents; *dp && !interrupt(); dp++){
	name = *dp;
	while ( name[strlen(name)-1] == '/'){
	    name[strlen(name)-1] = '\0';	// Strip off terminating "/"
	}
	
	if (!strcmp(name, ".")) continue;
	if (!strcmp(name, "..")) continue;
	if (!strcmp(name, "core")) continue;
	if (!strcmp(name, "curpar")) continue;
	
	struct stat statbuf;
	struct stat procstat;
	
	char *filename = new char[strlen(dirname) + strlen(name) + 2];
	char *curpar = new char[strlen(dirname) + strlen(name)
				+ strlen("/curpar") + 2];
	strcpy(filename, dirname);
	strcat(filename, "/");
	strcat(filename, name);

	strcpy(curpar, filename);
	strcat(curpar, "/curpar");
	
	stat(filename, &statbuf);
	if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
	    // This is a directory
	    if (stat(curpar, &procstat) == 0) {
		// filename/curpar exists; load as old style data
		//cout << filename << " [DATA DIRECTORY]"<<endl;
		Frame_routine::load_data(filename, "/");
	    }
	} else if (((statbuf.st_mode & S_IFMT) == S_IFREG)) {
	    // This is a regular file; load as DDL file
	    //cout << endl << "Should load:" << filename << " !!!" << endl;
	    Frame_routine::load_data("/", filename);
	}
	delete [] filename;
	delete [] curpar;
	free(*dp);
    }
    interrupt_end();
    free(contents);
    (void)set_cursor_shape(cursor);
}

      
/***********************************************************************
*								       *
*  Load data into a frame from specified directory
*  [MACRO interface]
*  argv[0]: (char *) Full pathname of directory
*
*  [STATIC function]						       *
*								       */
int
Frame_routine::LoadAll(int argc, char **argv, int, char **)
{
    argc--; argv++;

    char *path = 0;
    switch (argc){
      case 0:
	path=get_filelist_dir(FILELIST_WIN_ID);
	break;
      case 1:
	path = strdup(argv[0]);
	break;
      default:
	ABORT;
    }
    load_data_all(path, "");
    if (path){
	free(path);
    }
    return PROC_COMPLETE;
}

int
Frame_routine::load_data_all(char *dirpath, char *)
{
  // NB: The unused second argument is present to make the interface the
  // same as for Frame_routine::load_data().
  int silent = load_silently;  // Save the old value
  load_silently = TRUE;
  loaddir(dirpath);
  load_silently = silent;      // Restore the old value
  macroexec->record("data_load_all('%s')\n", dirpath);
  return TRUE;
}
  
int
Frame_data::load_ddl_data(Gframe *gframe, char *dirpath, char *filename,
			  int *display_data_flag,
			  int,
			  DDLSymbolTable *ddlst,
			  int math_result)
/*
 *  Procedure to load in data which is in the DDL format and displays
 *  it in the selected gframe.
 *
 */
{
  Imginfo *oldImg;
  Imginfo *img;
  char errmsg[128];
  errmsg[0] = '\0';
  // This is new ddl format data file
  // debug << "New DDL format file!" << endl;
  
  if (gframe == 0) return NOT_OK;

  attach_imginfo(oldImg, gframe->imginfo);
  detach_imginfo(gframe->imginfo);
  gframe->imginfo = new Imginfo;
  attach_imginfo(img, gframe->imginfo);

  if (img->LoadImage(dirpath, filename, errmsg, ddlst) == NULL) {
    // An error has occurred while loading the data.
    if (errmsg) {
      msgerr_print("LoadImage:%s",errmsg);
    }
    delete img;
    gframe->imginfo = NULL;
    return(NOT_OK);
  }

  // Use gray-scale colormap
  img->cmsindex = SISCMS_2;
			       
  // Check for 3D data
  char *spatial_rank;
  img->st->GetValue("spatial_rank", spatial_rank);
  if (strcmp(spatial_rank, "3dfov") == 0){
      if (!math_result) {
	  *display_data_flag = FALSE;
	  /* Restore any old image to the gframe we were using */
	  detach_imginfo(gframe->imginfo);
	  attach_imginfo(gframe->imginfo, oldImg);
      }
      VolData::extract_slices(img);
  }
  if (*display_data_flag) {
      Frame_data::display_data(gframe, 0, 0,
			       img->GetFast(),
			       img->GetMedium(),
			       img->vs);
  }
  detach_imginfo(oldImg);
  detach_imginfo(img);
  return OK;
}

/***********************************************************************
*								       *
*  Check the first line of a file against a list of regular expressions.
*  The magic string list is terminated by a NULL pointer.
*  Return TRUE is there is a match, otherwise FALSE.
*								       */
int
ValidMagicNumber(char* filename, char *magic_string_list[])
{
    ifstream f;
    char header[1024];
    struct stat fname_stat;
    
    if (stat(filename, &fname_stat) != 0){
	return FALSE;
    }
    
    if ((fname_stat.st_mode & S_IFMT) != S_IFREG){
	return FALSE;
    }
    
    f.open(filename, ios::in);
    
    if (!f){
	msgerr_print("Can't open input file!");
	return FALSE;
    }

    // Read in the header line (first line)
    f.getline(header, sizeof(header));

    // Check against all magic strings passed to us
    int rtn = FALSE;
    char **str = magic_string_list;
    for (str=magic_string_list; *str; str++){
	if (strstr(header, *str)){
	    rtn = TRUE;		// Found a match
	}
    }
    return rtn;
}
      
/************************************************************************
*									*
*  Set warning level for memory usage
*  [MACRO interface]
*  argv[0]: (int) Limit in percent
*  [STATIC Function]							*
*									*/
int
Frame_routine::MemThreshold(int argc, char **argv, int, char **)
{
    double thresh;

    argc--; argv++;

    if (argc != 1){
	ABORT;
    }

    if (MacroExec::getDoubleArgs(argc, argv, &thresh, 1) != 1){
	ABORT;
    }
    memPctThreshold = (int)thresh;
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Save data
*  [MACRO interface]
*  argv[0]: (char *) Relative or full path of output file
*  [STATIC Function]							*
*									*/
int
Frame_routine::Save(int argc, char **argv, int, char **)
{
    char *filename;
    char *dirpath;
    
    argc--; argv++;

    if (argc != 1){
	ABORT;
    }

    // Deal with relative file paths
    filename = *argv;
    if (*filename != '/') {
	dirpath = get_filelist_dir(FILELIST_WIN_ID);
	if (!dirpath) {
	    dirpath = "";
	}
    }

    Gframe::menu_save_image(dirpath, filename);

    return PROC_COMPLETE;
}

/***********************************************************************
*								       *
*  Load data into a frame from specified file
*  [MACRO interface]
*  argv[0]: (char *) Full pathname of file
*
*  [STATIC function]						       *
*								       */
int
Frame_routine::Load(int argc, char **argv, int, char **)
{
    argc--; argv++;

    switch (argc) {
      case 0:
	filelist_win_show(FILELIST_WIN_ID, FILELIST_LOAD_ALL, NULL, 
			  "File Browser: Data");
	break;
      case 1:
	load_data_file("", argv[0]);
	break;
      default:
	ABORT;
	break;
    }
    return PROC_COMPLETE;
}

/***********************************************************************
*								       *
*  Load parameters and sisdata into a frame from specified directory   *
*  and filename                                                        *
*  [STATIC function]						       *
*								       */
int
Frame_routine::load_data_file(char *dirpath, char *filename)
{
    char *dir = 0;

    // Deal with relative file paths
    if (*dirpath == '\0' && *filename != '/') {
	dir = get_filelist_dir(FILELIST_WIN_ID);
	if (dir) {
	    dirpath = dir;
	}
    }

    int rtn = load_data(dirpath, filename);
    if (rtn == OK){
	macroexec->record("data_load('%s/%s')\n", dirpath, filename);
    }
    if (dir) {
	free(dir);
    }
    return rtn;
}

/***********************************************************************
*								       *
*  Load parameters and sisdata into a frame from specified directory   *
*  and filename                                                        *
*  [STATIC function]						       *
*								       */
int
Frame_routine::load_data(char *dirpath, char *filename)
{
  Gframe *gframe = NULL;	// selected frame
  
  if (*filename == NULL){
    msgerr_print("load_data:Must specify filename to be loaded");
    return NOT_OK;
  }


  struct rlimit limits;
  /*if (getrlimit(RLIMIT_VMEM, &limits) == -1){
      perror("getrlimit");
  }
  fprintf(stderr,"VirtualMem=%d, %d\n",
	  limits.rlim_cur, limits.rlim_max);
  if (getrlimit(RLIMIT_AS, &limits) == -1){
      perror("getrlimit");
  }
  fprintf(stderr,"AvailableMem=%d, %d\n",
	  limits.rlim_cur, limits.rlim_max);*/
#ifndef LINUX
  int i;
  int nswap = swapctl(SC_GETNSWP, 0);
  swaptbl_t *swaptab = (swaptbl_t *)malloc(nswap * sizeof(swapent_t)
				+ sizeof(struct swaptable));
  swaptab->swt_n = nswap;
  char *strtab = (char *)malloc(nswap * 1024);
  for (i=0; i<nswap; i++){
      swaptab->swt_ent[i].ste_path = strtab + i * 1024;
  }
  swapctl(SC_LIST, swaptab);
  int oldswapfree = 0;
  for (i=0; i<nswap; i++){
      oldswapfree += swaptab->swt_ent[i].ste_free;
      /*fprintf(stderr,"free=%d in %s\n",
	      swaptab->swt_ent[i].ste_free, swaptab->swt_ent[i].ste_path);*/
  }
  int oldbrk = (int)sbrk(0);
  /*i = ((int)sbrk(0) + oldswapfree * PAGESIZE) / (1<<20);
  fprintf(stderr,"***brk+free = %d MB\n", i);*/
#endif

  // Make sure we're loading into a valid frame
  if ( targetframe == NULL || !framehead->exists(targetframe) ){
      targetframe = framehead;
      FindNextFreeFrame();
  }

  if (targetframe){
      gframe = targetframe;
  }else{
      gframe = Gframe::big_gframe();
  }
  
  selecthead->deselect();
  gframe->mark();
  selecthead->insert(gframe);
  
  
  // Confirm to overwrite the data
  if (gframe->imginfo && load_silently == FALSE){
    char strmsg[128];
    (void)sprintf(strmsg, "Overwrite data: %s%s ?",
		  com_clip_len_front(gframe->imginfo->GetDirpath(),12),
		  gframe->imginfo->GetFilename() ?
		  gframe->imginfo->GetFilename() : "<no-name>");
    if (confirmwin_popup("Yes", "No", strmsg, NULL) == FALSE){
	return NOT_OK;
    }
  }

  int cursor = set_cursor_shape(IBCURS_BUSY);

  // First find out if the file is a new ddl file or the old vnmr directory

  int rtn = NOT_OK;

  char *fname = new char[strlen(dirpath) + strlen(filename) + 2];
  strcpy(fname, dirpath);
  if (*filename != '/' && *dirpath != '\0') {
      strcat(fname, "/");
  }
  strcat(fname, filename);

  if (ValidMagicNumber(fname, magic_string_list)){
      int display_data = TRUE;
      if (Frame_data::load_ddl_data(gframe, dirpath, filename, &display_data)
	  == OK)
      {
	  if (auto_skip_frame && display_data){
	      Frame_routine::FindNextFreeFrame();
	  }
	  rtn = OK;
      }
  }else if (filename[strlen(filename)-1] == '/'){
      // This may be an old vnmr format data file
      if (Frame_data::load_data_oldformat(gframe, dirpath, filename,
					  TRUE) == OK)
      {
	  if (auto_skip_frame) Frame_routine::FindNextFreeFrame();
	  rtn = OK;
      }
  }

  delete [] fname;
  (void)set_cursor_shape(cursor);

#ifdef MEMFIDDLE
  /*getrlimit(RLIMIT_VMEM, &limits);
  fprintf(stderr,"limits=%d, %d\n",
	  limits.rlim_cur, limits.rlim_max); /*CMP*/
  static int database = 0;
  if (!database){
      char *xbrk = (char *)sbrk(0);
      if (brk((void *)(xbrk+0x10)) == -1){
	  perror("A: brk(xbrk+inc)");
      }else{
	  fprintf(stderr,"brk A is OK\n");
      }
      if (brk((void *)(xbrk)) == -1){
	  perror("brk(xbrk)");
      }
      if (getrlimit(RLIMIT_DATA, &limits) == -1){
	  perror("getrlimit");
      }
      fprintf(stderr,"DATA LIMIT=%d, %d\n",
	      limits.rlim_cur, limits.rlim_max);
      fprintf(stderr,"brk=%d\n", (int)xbrk);

      rlim_t tstlim = (rlim_t)xbrk;
      rlim_t oldlim = limits.rlim_cur;
      do {
	  tstlim -= 0x10;
	  limits.rlim_cur = tstlim;
	  if (setrlimit(RLIMIT_DATA, &limits) == -1){
	      break;
	  }
	  if ((int)sbrk(1) == -1){
	      break;
	  }else{
	      sbrk(-1);
	  }
	  //fprintf(stderr,"tstlim=%d OK\n", tstlim);
      } while (tstlim > 0);
      database = (rlim_t)xbrk - tstlim;
      fprintf(stderr,"DATABASE = %d, NEW DATA LIMIT = %d\n", database, tstlim);
      if (brk((void *)(xbrk+0x20)) == -1){
	  perror("B: brk(xbrk+inc)");
      }else{
	  fprintf(stderr,"brk B is OK\n");
      }

      limits.rlim_cur = oldlim;
      setrlimit(RLIMIT_DATA, &limits);
      if (brk((void *)(xbrk)) == -1){
	  perror("brk(xbrk)");
      }
  }
#endif /*MEMFIDDLE*/
#ifndef LINUX
  swapctl(SC_LIST, swaptab);
  int newswapfree = 0;
  int newswappages = 0;
  for (i=0; i<nswap; i++){
      newswapfree += swaptab->swt_ent[i].ste_free;
      newswappages += swaptab->swt_ent[i].ste_pages;
      /*fprintf(stderr,"total=%dMB, free=%dMB in %s\n",
	      PAGESIZE * swaptab->swt_ent[i].ste_pages / (1<<20),
	      PAGESIZE * swaptab->swt_ent[i].ste_free / (1<<20),
	      swaptab->swt_ent[i].ste_path);*/
  }
  int newbrk = (int)sbrk(0);
  float swapused = 1 - (float)newswapfree / newswappages;
  int swappercent = (int)(swapused*100 + 0.5);

  if (getrlimit(RLIMIT_VMEM, &limits) == -1){
      perror("getrlimit");
  }
  int meg = (1<<20);
  char *mybrk = (char *)sbrk(0);
  char *maxbrk;
  for (maxbrk=mybrk+meg; maxbrk<=(char *)limits.rlim_max; maxbrk+=meg){
      if (brk(maxbrk) == -1){
	  break;
      }
  }
  brk(mybrk);
  maxbrk--;
  float sysmem = (float)sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE);
  float availbytes = (float)(maxbrk - mybrk);
  float vmembytes = (float)newswappages * sysconf(_SC_PAGE_SIZE) + sysmem;
  /*if (maxbrk <= (void *)limits.rlim_max){
      fprintf(stderr,"AVAIL=%d, TOTMEM=%d, USED = %d%%\n",
	      (int)(availbytes/meg), (int)(vmembytes/meg),
	      (int)(100*(1 - (float)availbytes/vmembytes)));
  }*/
  swappercent = (int)(100*(1 - (float)availbytes/vmembytes));
  win_print_msg("Memory: %d%% used, %dMB remaining",
		swappercent, (int)(availbytes/meg));
  if (swappercent > memPctThreshold
      && newswapfree < oldswapfree
      && newbrk > oldbrk)
  {
      msgerr_print("WARNING: memory %d%% used, %dMB remaining",
		   swappercent, (int)(availbytes/meg));
  }

  free(swaptab);
  free(strtab);
#endif

  return rtn;
}

/************************************************************************
*                                                                       *
*  Save sisdata.                                                        *
*  [STATIC Function]							*
*                                                                       */
void
Frame_routine::save_data(char *dirpath, char *filename)
{
  win_print_msg("Save: %s/%s", dirpath, filename);
}


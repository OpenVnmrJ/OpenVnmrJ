/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/****************************************/
/* ll2d - 2D line listing		*/
/* peak2d - find 2D maximum		*/
/* acosy - automatic cosy analysis	*/
/****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "data.h"
#include "init2d.h"
#include "pvars.h"
#include "vnmrsys.h"
#include "disp.h"
#include "tools.h"
#include "graphics.h"
#include "group.h"
#include "allocate.h"
#include "buttons.h"
#include "init_display.h"
#include "init_proc.h"
#include "dscale.h"
#include "wjunk.h"
/******************************************************************************
*	LL2D stuff
******************************************************************************/
#define PEAK_MAX	8192	/* maximum number of peaks */
#define	PEAK_FILE_MAX	PEAK_MAX/* maximum number of peaks in peak file */
#define	PEAK_FILE_EMPTY	0	/* flag to tell if peak file record is unused */
#define	PEAK_FILE_FULL	1		/* or in use */

#define LABEL_LEN	20	/* length of peak label */
#define COMMENT_LEN	80	/* length of peak comment */
#define EMPTY_LABEL	"                    "	/* make sure it's LABEL_LEN
						   blanks long */
#define TICK_LEN	1.2	/* length of ticks for peak markers */
#define CURRENT_VERSION 1.1	/* current version of peak file 
				   (starting at 1.0) */
				/* as of v1.1, to change version number,
				   you need may need to change :
					read_peak_file_header()
					write_peak_file_header()
					calc_header_size()
				     and most importantly :
					update_peak_file_version()
				     and for text peak files :
					read_ascii_peak_file()
					write_ascii_peak_file() */

/*****************************************************************************
*  structure for a peak record
*****************************************************************************/
typedef struct pk_struct {
	double		f1, f2;		/* peak frequencies */
	double		amp;		/* peak amplitude */
	double		f1_min, f1_max,
			f2_min, f2_max; /* peak bounds for volume calculation */
	double		fwhh1, fwhh2;	/* hwhh of peak in each direction */
	double		vol;		/* peak volume within bounds of peak */
	int		key;		/* key : peak number */
	struct pk_struct *next;		/* next peak in list */
	char		label[LABEL_LEN+1];/* character label for peak */
	char		comment[COMMENT_LEN+1];/* character comment for peak */
	} peak_struct;

/*****************************************************************************
*  structure for peak table
*  	First entry in header is the number of peaks in the table, next
*	entries tell whether the corresponding key is in use (PEAK_FILE_FULL)
*	or not (PEAK_FILE_EMPTY). (i.e. header[20] tells whether a peak with
*	key 20 is currently in existence.)
*****************************************************************************/
typedef struct {
	int		num_peaks;	/* number of peaks in table */
	FILE *file;			/* peak file */
	float version;			/* version of peak file */
	peak_struct	*head;		/* pointer to head of peak list */
	short	header[PEAK_FILE_MAX+1];/* peak file header to mark if key
					   is in use or not. */
	char		f1_label,f2_label; /* labels for freq. directions */
	int experiment;			/* which experiment peak table was
					   created in */
	int planeno;			/* which plane of a 3D experiment */
	} peak_table_struct;


#define CURSOR_MODE     1	/* must be consistent with dconi.c */
#define BOX_MODE        5	/* must be consistent with dconi.c */

#define TRUE            1
#define FALSE           0
#define NO		FALSE
#define YES		TRUE

#define NORMALMODE	0	/* normal mode for graphics display */
#define XORMODE		1	/* xor mode for graphics display */

#define INTERP_PTS 	5	/* number of points used by interpolate_cg
				   routine => peak + 1 point on either side */
#define PEAK_THRESH     0.80	/* default value for th2d parameter */
#define SHOW_PEAKS	0	/* index for peak crosses in "ll2dmode" param.*/
#define SHOW_NUMS	1	/* index for peak key in "ll2dmode" param. */
#define SHOW_BOXES	2	/* index for peak bounds in "ll2dmode" param. */
#define SHOW_LABELS	3	/* index for peak label in "ll2dmode" param. */
#define NUM_LL2DMODE	4	/* number of char. in "ll2dmode" param. */

#define MINPOINTS_IN_PEAK     1

#define REG_FORMAT	0	/* print output in regular peak file format */
#define INFO_FORMAT	1	/* print output in format for ll2d('info') */

#define POS_NEG		0	/* do ll2d operation on both pos and neg peaks */
#define POS_ONLY	1	/* do ll2d operation on only pos peaks */
#define NEG_ONLY	-1	/* do ll2d operation on only neg peaks */

/*#define PEAK_COLOR	RED	 color for peak crosses */
/*#define NUM_COLOR	CYAN	 color for peak keys */
/*#define AV_BOX_COLOR	RED	 color for peak bounds boxes for AV spectra */
/*#define PH_BOX_COLOR	RED	 color for peak bounds boxes for PH spectra */
/*#define LABEL_COLOR	GREEN	 color peak labels */


#define FILE_NOT_FOUND	-1
#define COMPLETE 	 0
#define ERROR 		 1

static peak_table_struct *peak_table=NULL;	/* peak table for all ll2d
							operations */
extern int expdir_to_expnum(char *expdir);
extern void set_line_thickness(const char *thick);

double dist_from_diag(double f1, double f2, int horiz_f1);
static int peak_bounds(peak_table_struct *peak_table, float stdev,
           double f1, double f2, double amp,
           double *f1_1, double *f1_2, double *f2_1, double *f2_2,
           double *fwhh1, double *fwhh2, double *vol,
           double thresh, int **array_status);
static int argtest(int argc, char *argv[], char *argname);
static int ll2d_unmark(int mark_mode, int clear_flag,
                       int pos_neg_flag, int unmark_peak, int retc);
static int ll2d_label(int mark_mode, char *name, int num);
static int ll2d_comment(int mark_mode, char *name, int num);
static int ll2d_info(int mark_mode, int num, int retc, char *retv[]);
static double calc_volume(peak_table_struct *peak_table,
                          double f1_min, double f1_max,
                          double f2_min, double f2_max);
int              ll2d_frq_to_dp(double frq, double sw, int fn);
double           ll2d_dp_to_frq(double, double, int);
static int       abs_vol_flag;

/* forward declarations for routines below */
static int check_if_max(float array[INTERP_PTS][INTERP_PTS], float stdev,
                        int *pt1, int *pt, int **array_status);
static int check_if_peak(float array[INTERP_PTS][INTERP_PTS], float peak_thresh,
                         float stdev, int *N_ppts, int **point_status,
                         int ctrace, int cpoint);
static int check_axis_labels(peak_table_struct *peak_table);
static void init_peak_table(peak_table_struct **);
static peak_table_struct *create_peak_table();
static int insert_peak(peak_table_struct *peak_table, peak_struct *peak);
static void delete_peak(peak_table_struct *peak_table, double f1, double f2);
static void insert_peak_bounds(peak_table_struct *peak_table,
       double f1, double f2, double x1, double x2, double y1, double y2,
       double fwhh1, double fwhh2, double vol);
static int peak_in_table(peak_table_struct *, double, double);
static char *get_filename();
static void pad_label(char *);
static int create_peak_file(peak_table_struct *peak_table, char *filename);
static int calc_header_size(peak_table_struct *peak_table);
static void write_peak_file_header(peak_table_struct *peak_table);
static void insert_peak_in_file(peak_table_struct *peak_table,
            peak_struct *peak);
static int read_peak_file_header(peak_table_struct *peak_table);
static void update_peak_file_version(peak_table_struct **peak_table);
static int read_ascii_peak_file(peak_table_struct **peak_table, char *filename);
static int write_ascii_peak_file(peak_table_struct *peak_table,
                                 char *filename, int peakMode);
static int calculate_noise(float *, float *, float, int, int, int, int);
static void interpolate_cg(double *, double *, double *);
static int peak_table_valid(peak_table_struct *);
static void write_label(int, int, char *, int);
static void write_peak_num(int, int, int, int);
static int in_display(double, double);
static int intRound(double);
static void get_formats(char *, char *, char *, char *, char *, char *, int);
static void ll2d_init();
void ll2d_draw_peaks();

static void adjust_peak_bounds(peak_table_struct *peak_table);
static void draw_peaks(peak_table_struct *peak_table);
static int ll2d_combine(int mark_mode, int weight, double *list, int numlist,
                        int retc, char *retv[]);
static void ll2d_autocombine(int weight, double f1box, double f2box);
static int ll2d_mark(int mark_mode, int retc);
static void rekey_peaks(peak_table_struct *peak_table);
static void delete_peak_from_table(peak_table_struct *peak_table, double f1, double f2);

peak_struct *create_peak(double f1, double f2, double amp);
void delete_peak_table(peak_table_struct **peak_table);
int read_peak_file(peak_table_struct **peak_table, char *filename);
void write_peak_file_record(peak_table_struct *peak_table,
                            peak_struct *peak, int record);

#ifdef DEBUG
void dispPeak(peak_struct *peak)
{
   fprintf(stderr,"Peak %d\n",peak->key);
   fprintf(stderr,"  Label:   %s\n",peak->label);
   fprintf(stderr,"  Comment: %s\n",peak->comment);
   fprintf(stderr,"  f1: %g f2: %g amp: %g vol: %g\n",peak->f1, peak->f2, peak->amp, peak->vol);
   fprintf(stderr,"  f1_min: %g f1_max: %g fwhh1: %g\n",peak->f1_min, peak->f1_max, peak->fwhh1);
   fprintf(stderr,"  f2_min: %g f2_max: %g fwhh2: %g\n",peak->f2_min, peak->f2_max, peak->fwhh2);
}

int showPeaks(char *msg)
{
  peak_struct *pk_ptr;
  int index=1;
  fprintf(stderr,"%s\n",msg );
  fprintf(stderr,"Peak Table num_peaks %d\n",(peak_table)->num_peaks );
  fprintf(stderr,"header[0] = %d\n", (peak_table)->header[0]);
  pk_ptr = peak_table->head;
  while (pk_ptr != NULL)  {
//    fprintf(stderr,"header[%d] = %d key= %d\n",index, (peak_table)->header[index], pk_ptr->key);
    fprintf(stderr,"key:%d amp= %g f2= %g f1= %g\n",
                    pk_ptr->key, pk_ptr->amp, pk_ptr->f2, pk_ptr->f1);
    index++;
    pk_ptr = pk_ptr->next;
    }
  return 0;

}
#endif

/*************/
int ll2d(int argc, char *argv[], int retc, char *retv[])
/*************/
{  int i,ctrace,j, j1, res, N_ppts;
   double vs1;
   double points[INTERP_PTS], temp, value;
   float *trace;
   float array[INTERP_PTS][INTERP_PTS];
/*   float prev1_pts[INTERP_PTS],prev_pts[INTERP_PTS], trace_pts[INTERP_PTS], next_pts[INTERP_PTS], next_pts[INTERP_PTS];
*/
   int **point_status, **array_status;
   float stdev, noise;

   double f1_pk,amp1_pk,f2_pk,amp2_pk,fraction1,fraction2;
   double f1_rflrfp, f2_rflrfp;
   double peak_thresh;
   peak_struct *peak, *peak_ptr;
   peak_table_struct *temp_peak_table=NULL;
 				/* flags for various ll2d options */
   int reset_flag, peak_flag, volume_flag, read_flag, label_flag, info_flag,
	mark_flag, unmark_flag, adjust_flag, pos_neg_flag, clear_flag,
	draw_flag, writetext_flag, comment_flag, readtext_flag,
	combine_flag, label_peak, unmark_peak;
   int peakno, peakmax, peakcnt;
   int writepeak_flag;
   int autocombine_flag, weight_flag;
   char ch, *filename = NULL, *label = NULL, mode_string[NUM_LL2DMODE+1];
   char *fname0;
   double h_rflrfp, v_rflrfp, cr, cr1, delta, delta1, diag_dist;
   double *comblist;
   int new_fpnt, new_npnt, new_fpnt1, new_npnt1;

   extern int dconi_newdcon();/* from dconi.c */
   extern int dconi_cursor();  /* from dconi.c */
   extern int dconi_mode;  /* from dconi.c */

   extern int set_turnoff_routine(int (*funct)()), turnoff_dconi();


    Wturnoff_buttons();
    if (WgraphicsdisplayValid( "dconi" ))
      set_turnoff_routine(turnoff_dconi);
    if (argv[0][0] == 'p')  {	/* plotter */
      if (init2d(1,2)) ABORT;
      }
    else {			/* screen */
      if (init2d(1,1)) ABORT;
      }
    if (!d2flag)
     { Werrprintf("no 2D data in data file");
       ABORT;
     }

    if (!peak_table_valid(peak_table))
      delete_peak_table(&peak_table);

    /* if command is pll2d */
    if (argv[0][0] == 'p')  {
      if (!WgraphicsdisplayValid( "dconi" ) &&
	  !WgraphicsdisplayValid( "dcon"  ) &&
	  !WgraphicsdisplayValid( "dpcon" ) &&
	  !WgraphicsdisplayValid( "ds2d"  ))  { /* delete peak table if
						   not in dconi mode */
        ll2d_init();
        }
      else  {
        init_peak_table(&peak_table);
        check_axis_labels(peak_table);
        }
      if (peak_table)  {
	if (peak_table->num_peaks > 0)  {
	  ll2d_draw_peaks();
	  }
	}
      if (!WgraphicsdisplayValid( "dconi" ) &&
	  !WgraphicsdisplayValid( "dcon"  ) &&
	  !WgraphicsdisplayValid( "dpcon" ) &&
	  !WgraphicsdisplayValid( "ds2d"  ))  {
	delete_peak_table(&peak_table);
	}
      RETURN;
     }

    /* defaults */
    reset_flag = NO;
    peak_flag = NO;
    volume_flag = NO;
    abs_vol_flag = NO;
    read_flag = NO;
    readtext_flag = NO;
    mark_flag = NO;
    unmark_flag = NO;
    label_flag = NO;
    comment_flag = NO;
    info_flag = NO;
    adjust_flag = NO;
    pos_neg_flag = POS_NEG;
    clear_flag = NO;
    draw_flag = NO;
    writetext_flag = NO;
    writepeak_flag = NO;
    combine_flag = NO;
    autocombine_flag = NO;
    weight_flag = NO;
    label_peak = 0;
    unmark_peak = 0;
    peakmax = peakno = peakcnt = 0;
    comblist = NULL;
    /* see if argv overrides defaults */
    if (argc == 1)  {
      peak_flag = YES;
      }
    else if (argc == 2)  {
	if (strcmp(argv[1],"draw")==0)  {
	  draw_flag = YES;
	  }
	else if (strcmp(argv[1],"writetext")==0) {
	  writetext_flag = YES;
	  filename = NULL;
	  }
	else if (strcmp(argv[1],"writepeaks")==0) {
	  writetext_flag = YES;
          writepeak_flag = YES;
	  filename = NULL;
	  }
	else if (strcmp(argv[1],"reset")==0) {
	  reset_flag = YES;
	  }
	else  if (strcmp(argv[1],"read")==0)  {
	  read_flag = YES;
	  filename = NULL;
	  }
	else  if (strcmp(argv[1],"readtext")==0)  {
	  readtext_flag = YES;
	  filename = NULL;
	  }
	else if (strcmp(argv[1],"peak")==0)  {
	  peak_flag = YES;
	  }
	else if (strcmp(argv[1],"volume")==0)  {
	  volume_flag = YES;
	  if (!peak_table)  peak_flag = YES;
	  else if (peak_table->num_peaks == 0)   peak_flag = YES;
	  }
	else if (strcmp(argv[1],"adjust")==0)  {
	  adjust_flag = YES;
	  }
	else if (strcmp(argv[1],"clear")==0)  {
	  if (!WgraphicsdisplayValid( "dconi" )) {
		Werrprintf("ll2d:  must be in dconi mode to do clear");
		ABORT;
	    }
	  clear_flag = YES;
	  }
	else if (strcmp(argv[1],"mark")==0) {
	  if (!WgraphicsdisplayValid( "dconi" )) {
		Werrprintf("ll2d:  must be in dconi mode to do mark");
		ABORT;
	    }
	  mark_flag = YES;
	  }
	else if (strcmp(argv[1],"unmark")==0) {
	  if (!WgraphicsdisplayValid( "dconi" )) {
		Werrprintf("ll2d:  must be in dconi mode to do unmark");
		ABORT;
	    }
	  pos_neg_flag = POS_NEG;
	  unmark_flag = YES;
	  }
	else if (strcmp(argv[1],"label")==0) {
          if (!WgraphicsdisplayValid( "dconi" )) {
                Werrprintf("ll2d:  must be in dconi mode to do label");
                ABORT;
            }
	  label_flag = YES;
	  label = NULL;
	  }
	else if (strcmp(argv[1],"comment")==0) {
          if (!WgraphicsdisplayValid( "dconi" )) {
                Werrprintf("ll2d:  must be in dconi mode to do comment");
                ABORT;
            }
	  comment_flag = YES;
	  label = NULL;
	  }
	else if (strcmp(argv[1],"info")==0) {
          if (!WgraphicsdisplayValid( "dconi" )) {
                Werrprintf("ll2d:  must be in dconi mode to get info");
                ABORT;
            }
	  ll2d_info(dconi_mode,0,retc,retv);
	  info_flag = YES;
	  }
	else if ( (strcmp(argv[1],"combine")==0) || (strcmp(argv[1],"combinewt")==0) )  {
	  if (!WgraphicsdisplayValid( "dconi" )) {
		Werrprintf("ll2d:  must be in dconi mode to do combine");
		ABORT;
	    }
	  combine_flag = YES;
          weight_flag = (strcmp(argv[1],"combinewt")) ? 0 : 1;
	  }
	else if ( (strcmp(argv[1],"autocombine")==0) || (strcmp(argv[1],"autocombinewt")==0) )  {
	  Werrprintf("ll2d: %s option requires F1 and F2 box bounds\n",argv[1]);
	  ABORT;
          }
	else if (strcmp(argv[1],"peakno")==0) {
             peakno=1;
             peak_flag = 1;
             peakmax=0;
          }
	else  {
	  Werrprintf("ll2d: illegal option \"%s\"\n",argv[1]);
	  ABORT;
	  }
	}
    else if ((argc > 2) &&
             ((strcmp(argv[1],"combine")==0) || (strcmp(argv[1],"combinewt")==0)) )  {
      comblist = (double *)allocateWithId(sizeof(double)*(argc-2),"ll2d");
      for (i=0;i<argc-2;i++)
	comblist[i] = stringReal(argv[i+2]);
      combine_flag = YES;
      weight_flag = (strcmp(argv[1],"combinewt")) ? 0 : 1;
      }
    else if (argc == 3)  {
      if (strcmp(argv[1],"read")==0)  {
	read_flag = YES;
	filename = argv[2];
	}
      else if (strcmp(argv[1],"readtext")==0)  {
	readtext_flag = YES;
	filename = argv[2];
	}
      else if (strcmp(argv[1],"writetext")==0)  {
	writetext_flag = YES;
	filename = argv[2];
	}
      else if (strcmp(argv[1],"writepeaks")==0)  {
	writetext_flag = YES;
	writepeak_flag = YES;
	filename = argv[2];
	}
      else if (strcmp(argv[1],"label")==0) {
          if (!WgraphicsdisplayValid( "dconi" )) {
                Werrprintf("ll2d:  must be in dconi mode to do label");
                ABORT;
            }
	  label_flag = YES;
	  label = argv[2];
	  }
      else if (strcmp(argv[1],"comment")==0) {
          if (!WgraphicsdisplayValid( "dconi" )) {
                Werrprintf("ll2d:  must be in dconi mode to do comment");
                ABORT;
            }
	  comment_flag = YES;
	  label = argv[2];
	  }
      else if (argtest(argc,argv,"reset") && argtest(argc,argv,"peak"))  {
	reset_flag = YES;
	peak_flag = YES;
	}
      else if (argtest(argc,argv,"reset") && argtest(argc,argv,"volume"))  {
	reset_flag = YES;
	volume_flag = YES;
	peak_flag = YES;
	}
      else if (argtest(argc,argv,"peak") && argtest(argc,argv,"volume"))  {
	peak_flag = YES;
	volume_flag = YES;
	}
      else if (argtest(argc,argv,"adjust") && argtest(argc,argv,"volume"))  {
	adjust_flag = YES;
	volume_flag = YES;
	}
      else if (argtest(argc,argv,"peak") && argtest(argc,argv,"pos"))  {
	peak_flag = YES;
	pos_neg_flag = POS_ONLY;
	}
      else if (argtest(argc,argv,"volume") && argtest(argc,argv,"pos"))  {
	volume_flag = YES;
	pos_neg_flag = POS_ONLY;
	}
      else if (argtest(argc,argv,"clear") && argtest(argc,argv,"pos"))  {
	if (!WgraphicsdisplayValid( "dconi" )) {
		Werrprintf("ll2d:  must be in dconi mode to do clear");
		ABORT;
	    }
	pos_neg_flag = POS_ONLY;
	clear_flag = YES;
	}
      else if (argtest(argc,argv,"peak") && argtest(argc,argv,"neg"))  {
	peak_flag = YES;
	pos_neg_flag = NEG_ONLY;
	}
      else if (argtest(argc,argv,"volume") && argtest(argc,argv,"neg"))  {
	volume_flag = YES;
	pos_neg_flag = NEG_ONLY;
	}
      else if (argtest(argc,argv,"volume") && argtest(argc,argv,"abs"))  {
        volume_flag = YES;
        abs_vol_flag = YES;
        }
      else if (argtest(argc,argv,"clear") && argtest(argc,argv,"neg"))  {
	if (!WgraphicsdisplayValid( "dconi" )) {
		Werrprintf("ll2d:  must be in dconi mode to do clear");
		ABORT;
	    }
	pos_neg_flag = NEG_ONLY;
	clear_flag = YES;
	}
      else if (strcmp(argv[1],"info")==0)  {
        if (!peak_table)
          ll2d_init();
	if (isReal(argv[2]))  {
	  i = (int) (stringReal(argv[2]) + 0.5);
	  ll2d_info(dconi_mode,i,retc,retv);
	  info_flag = YES;
	  }
	else if (strcmp(argv[2],"total")==0)  {
	  if (retc > 0)
	    retv[0] = realString((double)(peak_table->num_peaks));
	  else
	    Winfoprintf("Number of peaks in 2D spectrum:  %d",peak_table->num_peaks);
	  info_flag = YES;
	  }
        else  {
	  Werrprintf("ll2d: argument after 'info' must be a peak number or 'total");
	  ABORT;
	  }
	}
      else if (strcmp(argv[1],"unmark")==0) {
	if (isReal(argv[2]))  {
	  unmark_peak = (int) (stringReal(argv[2]) + 0.5);
	  pos_neg_flag = POS_NEG;
	  unmark_flag = YES;
	  }
	else   {
	  Werrprintf("ll2d:  argument after 'unmark' must be a peak number");
	  ABORT;
	  }
	}
      else if (strcmp(argv[1],"peakno")==0) {
          peakno = 1;
          peak_flag = 1;
	  if (isReal(argv[2]))  {
	     peakmax = (int) (stringReal(argv[2]) + 0.5);
	  }
	else   {
	  Werrprintf("ll2d:  argument after 'peakno' must be maximum peaks");
	  ABORT;
	  }
        }
      else  {
	Werrprintf("ll2d: illegal option combination \"%s\" and \"%s\"\n",argv[1],argv[2]);
	ABORT;
	}
      }
    else if (argc == 4)  {
      if ( (strcmp(argv[1],"autocombine")==0) || (strcmp(argv[1],"autocombinewt")==0) )  {
	if ( isReal(argv[2]) && isReal(argv[3]) )  {
          weight_flag = (strcmp(argv[1],"autocombinewt")) ? 0 : 1;
          autocombine_flag = YES;
          } else {
	  Werrprintf("ll2d: %s option requires F1 and F2 box bounds\n",argv[1]);
	  ABORT;
          }
        }
      else if (argtest(argc,argv,"reset") && argtest(argc,argv,"peak") &&
		argtest(argc,argv,"volume"))  {
	reset_flag = YES;
	peak_flag = YES;
	volume_flag = YES;
	}
      else if (argtest(argc,argv,"pos") && argtest(argc,argv,"peak") &&
		argtest(argc,argv,"volume"))  {
	pos_neg_flag = POS_ONLY;
	peak_flag = YES;
	volume_flag = YES;
	}
      else if (argtest(argc,argv,"neg") && argtest(argc,argv,"peak") &&
		argtest(argc,argv,"volume"))  {
	pos_neg_flag = NEG_ONLY;
	peak_flag = YES;
	volume_flag = YES;
	}
      else if (argtest(argc,argv,"adjust") && argtest(argc,argv,"peak") &&
		argtest(argc,argv,"volume"))  {
	adjust_flag = YES;
	peak_flag = YES;
	volume_flag = YES;
	}
      else if (strcmp(argv[1],"label")==0)  {
	if (!isReal( argv[3] ))  {
	  Werrprintf("ll2d:  third argument with 'label' must be a peak number");
	  ABORT;
	  }
	label_peak = (int) (stringReal(argv[3]) + 0.5);
	label_flag = YES;
	label = argv[2];
	}
      else if (strcmp(argv[1],"comment")==0)  {
	if (!isReal( argv[3] ))  {
	  Werrprintf("ll2d:  third argument with 'comment' must be a peak number");
	  ABORT;
	  }
	label_peak = (int) (stringReal(argv[3]) + 0.5);
	comment_flag = YES;
	label = argv[2];
	}
      else  {
	Werrprintf("ll2d: illegal option combination \"%s\", \"%s\" and \"%s\"",argv[1],argv[2],argv[3]);
	ABORT;
	}
      }
    else if (argc == 5)  {
      if (argtest(argc,argv,"reset") && argtest(argc,argv,"peak") &&
		argtest(argc,argv,"volume") && argtest(argc,argv,"adjust"))  {
	reset_flag = YES;
	peak_flag = YES;
	volume_flag = YES;
	adjust_flag = YES;
	}
      }
    else  if (argc != 1)  {
      Werrprintf("ll2d: illegal number of options %d",argc-1);
      ABORT;
      }

    /* reset peak file */
    if (reset_flag == YES)  {
      delete_peak_table(&peak_table);
      init_peak_table(&peak_table);
      fname0 = get_filename();
      if (create_peak_file(peak_table,fname0))  {
        release(fname0);
        delete_peak_table(&peak_table);
	return(ERROR);
	}
      release(fname0);
      }

    /* read binary peak file */
    if (read_flag == YES)  {
      init_peak_table(&temp_peak_table);
      if ((res = read_peak_file(&temp_peak_table,filename))==FILE_NOT_FOUND)  {
        Werrprintf("Peak file not found");
        delete_peak_table(&temp_peak_table);
	return(COMPLETE);
	}
      else if (res == ERROR)  {
        delete_peak_table(&temp_peak_table);
	return(COMPLETE);
	}
      else if (temp_peak_table->num_peaks == 0)  {
        Werrprintf("Peak file is empty.");
        delete_peak_table(&temp_peak_table);
	return(COMPLETE);
	}
      else if (temp_peak_table->num_peaks > 0)  {
        delete_peak_table(&peak_table);
	peak_table = temp_peak_table;
	}
      else  {
        delete_peak_table(&temp_peak_table);
	return(COMPLETE);
	}
      }
    /* read ascii peak file */
    if (readtext_flag == YES)  {
      init_peak_table(&temp_peak_table);
      if ((res = read_ascii_peak_file(&temp_peak_table,filename)) ==
		FILE_NOT_FOUND)  {
        Werrprintf("Peak file not found");
        delete_peak_table(&temp_peak_table);
	return(COMPLETE);
	}
      else if (res == ERROR)  {
        delete_peak_table(&temp_peak_table);
	return(COMPLETE);
	}
      else if (temp_peak_table->num_peaks == 0)  {
        Werrprintf("Peak file is empty.");
        delete_peak_table(&temp_peak_table);
	return(COMPLETE);
	}
      else if (temp_peak_table->num_peaks > 0)  {
        delete_peak_table(&peak_table);
	peak_table = temp_peak_table;
	}
      else  {
        delete_peak_table(&temp_peak_table);
	return(COMPLETE);
	}
      }

    /* make sure everything is properly initialized */
    if (! peakno)
    {
       if (!WgraphicsdisplayValid( "dconi" ) &&
	!WgraphicsdisplayValid( "dcon"  ) &&
	!WgraphicsdisplayValid( "dpcon" ) &&
	!WgraphicsdisplayValid( "ds2d"  ))  {	 /* delete peak table if
						   not in dconi mode */
       if (!read_flag)
	ll2d_init();
      }
      else  {
       init_peak_table(&peak_table);
       check_axis_labels(peak_table);
      }
      if (!peak_table->file)  {
        fname0 = get_filename();
        if (create_peak_file(peak_table,fname0))  {
          release(fname0);
          delete_peak_table(&peak_table);
	  return(ERROR);
	 }
        release(fname0);
      }

    }
    if (mark_flag == YES)  {
      ll2d_mark(dconi_mode, retc);
      }
    if (unmark_flag == YES)  {
      if (peak_table)
	if (peak_table->num_peaks > 0)  {
	  ll2d_unmark(dconi_mode, FALSE,pos_neg_flag,unmark_peak,retc);
	  }
      }
    if (clear_flag == YES)  {
      if (peak_table)
	if (peak_table->num_peaks > 0)  {
	  ll2d_unmark(dconi_mode, TRUE,pos_neg_flag,unmark_peak,retc);
	  }
      }
    if (comment_flag == YES)
    {
      if (peak_table)
      {
	if (peak_table->num_peaks > 0)
	  ll2d_comment(dconi_mode,label,label_peak);
	else
	  Werrprintf("Error: No peaks in peak table");
      }
    }
    if (label_flag == YES)
    {
      if (peak_table)
      {
	if (peak_table->num_peaks > 0)
	  ll2d_label(dconi_mode,label,label_peak);
	else
	  Werrprintf("Error: No peaks in peak table");
      }
    }
    if (combine_flag == YES)  {
      if (peak_table)
	if (peak_table->num_peaks > 0)
	  if (ll2d_combine(dconi_mode,weight_flag,comblist,argc-2,retc,retv))  {
	    dconi_mode = -1;
	    dconi_cursor();
	    if (comblist)
	      release(comblist);
	    RETURN;
	    }
      if (comblist)
	release(comblist);
      }
    if (autocombine_flag == YES)  {
      if (peak_table && (peak_table->num_peaks > 0))
	  ll2d_autocombine(weight_flag, stringReal(argv[2]), stringReal(argv[3]) );
//      dconi_mode = -1;
//      dconi_cursor();
//      RETURN;
    }
    if (draw_flag == YES)  {
      ll2d_draw_peaks();
      if (!WgraphicsdisplayValid( "dconi" ) &&
	  !WgraphicsdisplayValid( "dcon"  ) &&
	  !WgraphicsdisplayValid( "dpcon" ) &&
	  !WgraphicsdisplayValid( "ds2d"  ))
        delete_peak_table(&peak_table);
      RETURN;
      }
    if (writetext_flag == YES)  {
      if (write_ascii_peak_file(peak_table,filename, writepeak_flag))  {
	return(ERROR);
	}
      }
  
   /* find threshold for peak-picking */
   if (normflag)
     vs1 = normalize * vs2d;
   else
     vs1 = vs2d;
   if (th < 0.001)
     vs1 *= 2.0;
   for (i=1; i<th; i++)
     vs1 /= 2.0;

   /* find threshold for peak bounds determination */
   if (P_getreal(CURRENT,"th2d",&peak_thresh,1))
     peak_thresh = PEAK_THRESH;
   if (peak_thresh < 1.0e-18)
     peak_thresh = 1.0e-18;
   if (peak_thresh > 1.0)
     peak_thresh = 1.0;
   peak_thresh /= vs1;

   /* calculate noise level and standard deviation */
   if ( (i=calculate_noise(&noise, &stdev, vs1, 0, fn1/4, 0, fn/4)) == 0) ABORT;

   /* find peaks */
   if (peak_flag == YES)  {
     char horiz_ch;
     char horiz_label = '\0';

     if ((npnt1 < 3) || (npnt < 3)) 
       { Werrprintf("ll2d: Display must have at least 3 points in each dimension");
         ABORT;
       }
     get_display_label(HORIZ,&horiz_ch);
     if (peakno)
     {
      char ch2;
      get_display_label(VERT,&ch2);
      if (horiz_ch < ch2)
	horiz_label = horiz_ch;
      else
	horiz_label = ch2;
     }
     else
     {
        horiz_label = peak_table->f1_label;
     }
     if (WgraphicsdisplayValid( "dconi" )) {
       get_cursor_pars(HORIZ,&cr,&delta);
       get_cursor_pars(VERT,&cr1,&delta1);
       get_rflrfp(HORIZ,&h_rflrfp);
       get_rflrfp(VERT,&v_rflrfp);

       if (dconi_mode == BOX_MODE) {
	 new_fpnt = ll2d_frq_to_dp(cr+h_rflrfp,sw,fn);
	 new_npnt = ll2d_frq_to_dp(cr-delta+h_rflrfp,sw,fn) - new_fpnt + 1;
	 new_fpnt1 = ll2d_frq_to_dp(cr1+v_rflrfp,sw1,fn1);
	 new_npnt1 = ll2d_frq_to_dp(cr1-delta1+v_rflrfp,sw1,fn1)-new_fpnt1+1;
         }
       else  {
         new_fpnt = fpnt;
         new_npnt = npnt;
         new_fpnt1 = fpnt1;
         new_npnt1 = npnt1;
         }
       if (new_fpnt < fpnt)  new_fpnt = fpnt;
       if ((new_npnt > npnt) || (new_npnt < 3))  new_npnt = npnt;
       if (new_fpnt1 < fpnt1)  new_fpnt1 = fpnt1;
       if ((new_npnt1 > npnt1) || (new_npnt1 < 3))  new_npnt1 = npnt1;
       }
     else  {
       new_fpnt = fpnt;
       new_npnt = npnt;
       new_fpnt1 = fpnt1;
       new_npnt1 = npnt1;
       }

     if (P_getreal(CURRENT,"xdiag",&diag_dist,1))
       diag_dist = 0.0;


     point_status = (int **) allocateWithId(sizeof(int *) * new_npnt1,"ll2d");
     for (i = 0; i<new_npnt1; i++)
	point_status [i] = (int *) allocateWithId(sizeof(int) * new_npnt,"ll2d");

     /* set point_status for future use */
     for (i = 0; i<new_npnt1; i++)
       for (j = 0; j<new_npnt; j++)
	 point_status [i][j] = 0;

     for (ctrace=INTERP_PTS/2; (ctrace<new_npnt1-INTERP_PTS/2)&&(!interuption);
			ctrace++)  {
       for (i=INTERP_PTS/2;i<new_npnt-INTERP_PTS/2;i++)  {

	   if ((trace = gettrace(new_fpnt1+ctrace,new_fpnt)) == 0) break;
	   value = fabs(*(trace+i))*vs1;
	   if (value >= 1.0 && point_status[ctrace][i]==0)  {   /* found a point above thresh */

	   /* get and save data points around current point */
           for (j1=0; j1<INTERP_PTS; j1++)
             {
	       if ((trace = gettrace(new_fpnt1+ctrace+j1-INTERP_PTS/2,new_fpnt)) == 0) break;
	       for (j=0;j<INTERP_PTS;j++)
	          array[j1][j] = *(trace+i+j-INTERP_PTS/2);
              }

	   /* check if point is the maximum */
	   if (check_if_peak(array, peak_thresh, stdev, &N_ppts, point_status, ctrace, i)) 
	    {

	     if (N_ppts >= MINPOINTS_IN_PEAK)  {    /* the point is a peak */
	       point_status[ctrace][i] = 1;

 	       /* interpolate along trace */
	       for (j=0;j<INTERP_PTS;j++)
	          points[j] = array[INTERP_PTS/2][j];
	       interpolate_cg(points,&fraction1,&amp1_pk);
	       /* interpolate across traces */
	       for (j=0;j<INTERP_PTS;j++)
	          points[j] = array[j][INTERP_PTS/2];
	       interpolate_cg(points,&fraction2,&amp2_pk);


	       /* calculate frequencies correctly depending on display direction*/
	       if (horiz_ch == horiz_label)  {  /* f1 is HORIZ */
	         f1_pk = ll2d_dp_to_frq((double)(new_fpnt+i)+fraction1,sw,fn);
	         f2_pk = ll2d_dp_to_frq((double)(ctrace+new_fpnt1)+fraction2,sw1,fn1);
	         }
	       else  {
	         f2_pk = ll2d_dp_to_frq((double)(new_fpnt+i)+fraction1,sw,fn);
	         f1_pk = ll2d_dp_to_frq((double)(ctrace+new_fpnt1)+fraction2,sw1,fn1);
	         }
	       /* peak amplitude is maximum of two interpolated amplitudes */
	       amp1_pk = fabs(amp1_pk) > fabs(amp2_pk) ? amp1_pk : amp2_pk;
	       /* insert peak in peak table */
	       if ((peak_flag == YES) && ((pos_neg_flag == POS_NEG) ||
		  ((pos_neg_flag == POS_ONLY) && (amp1_pk > 0.0)) ||
		  ((pos_neg_flag == NEG_ONLY) && (amp1_pk < 0.0))))  {
                 if ( (peakno) &&
			((diag_dist <= 0.0) ||
			 (diag_dist < dist_from_diag(f1_pk,f2_pk,
                                         (horiz_ch == horiz_label)))))
                 {
                    peakcnt++;
                    if (peakmax)
                    {
                      if (peakcnt > peakmax)
                         break;
                    }
                 }
	         else if (!peak_in_table(peak_table,f1_pk,f2_pk) &&
			((diag_dist <= 0.0) ||
			 (diag_dist < dist_from_diag(f1_pk,f2_pk,
                                         (horiz_ch == horiz_label)))))  {
	           peak = create_peak(f1_pk,f2_pk,(double)(amp1_pk));
	           if (insert_peak(peak_table,peak))  {
		     /* if peak insert fails (table is full), clean up and abort*/
		     delete_peak_table(&peak_table);
		     init_peak_table(&peak_table);
		     fname0 = get_filename();
		     if (create_peak_file(peak_table,fname0))  {
		       release(fname0);
		       delete_peak_table(&peak_table);
		       D_allrelease();
		       releaseAllWithId("ll2d");
		       return(ERROR);
		      }
		     release(fname0);
		     D_allrelease();
		     releaseAllWithId("ll2d");
		     ABORT;
		     }
 	           }
	         }
	       }
             }
           }
	 }
         if (peakmax && (peakcnt > peakmax) )
            break;
       }
      release(point_status);
     }

   /* find peak bounds and volumes */
   if (volume_flag == YES)  {
     get_rflrfp(HORIZ,&f2_rflrfp);
     get_rflrfp(VERT,&f1_rflrfp);
     get_display_label(HORIZ,&ch);
     if (ch == peak_table->f1_label)  {    /* f1 is horizontal */ 
       temp = f1_rflrfp;
       f1_rflrfp = f2_rflrfp;
       f2_rflrfp = temp;
       }

     array_status = (int **) allocateWithId(sizeof(int *) * INTERP_PTS,"ll2d");
     for (i = 0; i<INTERP_PTS; i++)
	array_status [i] = (int *) allocateWithId(sizeof(int) * INTERP_PTS,"ll2d");

     /* get first peak in table */
     peak_ptr = peak_table->head;
     while (peak_ptr && (!interuption))  {	/* search through peak table */
       if (in_display(peak_ptr->f1-f1_rflrfp,peak_ptr->f2-f2_rflrfp) &&
	   ((pos_neg_flag == POS_NEG) ||
	    ((pos_neg_flag == POS_ONLY) && (peak_ptr->amp > 0.0)) ||
	    ((pos_neg_flag == NEG_ONLY) && (peak_ptr->amp < 0.0))))  {
         if ((peak_ptr->f1_min == 0.0) && (peak_ptr->f1_max == 0.0) &&
	     (peak_ptr->f2_min == 0.0) && (peak_ptr->f2_max == 0.0))  {
	   peak_bounds(peak_table, stdev, peak_ptr->f1,peak_ptr->f2,peak_ptr->amp,
			&(peak_ptr->f1_min),&(peak_ptr->f1_max),
			&(peak_ptr->f2_min),&(peak_ptr->f2_max),
			&(peak_ptr->fwhh1),&(peak_ptr->fwhh2),
			&(peak_ptr->vol),peak_thresh,array_status);
	    write_peak_file_record(peak_table,peak_ptr,peak_ptr->key);
	    }
         else  {
	   peak_ptr->vol = calc_volume(peak_table,peak_ptr->f1_min,
		peak_ptr->f1_max,peak_ptr->f2_min,peak_ptr->f2_max);
	   write_peak_file_record(peak_table,peak_ptr,peak_ptr->key);
	   }
         }
       peak_ptr = peak_ptr->next;
       }

     release(array_status);

     }
   if (peakno)
   {
      if (retc)
      {
        if (peakmax)
	   retv[0] = intString((peakcnt > peakmax) ? 0 : 1);
        else
	   retv[0] = intString(peakcnt);
      }
      else
      {
        if (peakmax)
           Winfoprintf("ll2d: number of peaks <= %d is %s", peakmax,
              (peakcnt <= peakmax) ? "true" : "false");
        else
           Winfoprintf("ll2d: number of peaks = %d", peakcnt);
      }
      D_allrelease();
      releaseAllWithId("ll2d");
      RETURN;
   }
   /* adjust peak bounds */
   if (adjust_flag == YES)
     adjust_peak_bounds(peak_table);
   if (WgraphicsdisplayValid( "dconi" )) {
     if ((retc==0) && ((peak_flag) || (reset_flag) || (clear_flag)))
       Winfoprintf("Number of peaks in 2D Spectrum:  %d",peak_table->num_peaks);
     }
   if ((!info_flag) && (!combine_flag) && (retc>0))
     retv[0] = (char *)realString((double) peak_table->num_peaks);
   if (P_getstring(GLOBAL,"ll2dmode",mode_string,1,NUM_LL2DMODE+1))  {
     mode_string[SHOW_PEAKS] = 'y';
     mode_string[SHOW_NUMS] = 'y';
     mode_string[SHOW_BOXES] = 'y';
     mode_string[SHOW_LABELS] = 'y';
     }
   /* redraw display if unmark, reset, or clear done, or if new peaks or
	bounds are picked, just draw them on top of existing display */
   if ((strcmp(mode_string,"nnnn") != 0) && (!info_flag) && (!mark_flag) &&
	(!comment_flag) && (!label_flag)&& (!peak_flag) && (!volume_flag) &&
	(!writetext_flag)){
     if (WgraphicsdisplayValid( "dconi" )) {
       if ((peak_table->num_peaks > 0) || (reset_flag) || (clear_flag) ||
		(unmark_flag))  {
	 dconi_mode = (dconi_mode == BOX_MODE) ? CURSOR_MODE : -1;
	 dconi_newdcon();
	 }
       else   {
	 if (WgraphicsdisplayValid( "dconi" ))  {
	   dconi_mode = (dconi_mode == BOX_MODE) ? CURSOR_MODE : -1;
	   dconi_cursor();
	   }
	 }
       }
     }
   else if (peak_flag || volume_flag)  {
     if (WgraphicsdisplayValid( "dconi" ))  {
       if (peak_table->num_peaks > 0)  {
	 ll2d_draw_peaks();
	 }
       dconi_mode = (dconi_mode == BOX_MODE) ? CURSOR_MODE : -1;
       dconi_cursor();
       }
     }
   else if (label_flag || comment_flag || writetext_flag)  {
     if (WgraphicsdisplayValid("dconi"))  {
       dconi_mode = (dconi_mode == BOX_MODE) ? CURSOR_MODE : -1;
       dconi_cursor();
       }
     }
   D_allrelease();
   releaseAllWithId("ll2d");
   /* if not in dconi, delete in-memory peak table */
   if (!WgraphicsdisplayValid( "dconi" ) &&
	!WgraphicsdisplayValid( "dcon"  ) &&
	!WgraphicsdisplayValid( "dpcon" ) &&
	!WgraphicsdisplayValid( "ds2d"  ))  {
     delete_peak_table(&peak_table);
     }
   if (peak_table)
     if (peak_table->file)
       fflush(peak_table->file);
   RETURN;
}

/*****************************************************************************
*      argtest(argc,argv,argname)
*      test whether argname is one of the arguments passed
*****************************************************************************/
static int argtest(int argc, char *argv[], char *argname)
{
  int found = 0;

  while ((--argc) && !found)
    found = (strcmp(*++argv,argname) == 0);
  return(found);
}


static int calculate_noise(float *noise, float *std, float vs1,
                           int fpnt1, int npnt1, int fpnt, int npnt)
{
 float t1, stdev, *trace;
 int nn, ctrace, i;

   t1 = 1.0/vs1;
   *noise = 0.0;
   nn=0;
   for (ctrace=fpnt1; ctrace<fpnt1+npnt1; ctrace++)
     { if ((trace = gettrace(ctrace,fpnt)) == 0)
	 return 0;
       i = 0;
       while (i<npnt)	/* go though the trace */
         { 
	   if (fabs(*trace)<t1) {*noise += fabs(*trace); nn++;}
           i++;
           trace++;
         }
     }

     if (nn!=0) *noise /= nn;
     stdev = 0.0;
     nn = 0;

     for (ctrace=fpnt1; ctrace<fpnt1+npnt1; ctrace++)
      { if ((trace = gettrace(ctrace,fpnt)) == 0)
	  return 0;
        i = 0;
        while (i<npnt)	
         { 
	  if (fabs(*trace)<t1){
	    stdev += (fabs(*trace) - *noise)*(fabs(*trace) - *noise);
	    nn++;
	    }
           i++;
           trace++;
         }
       }
     if (nn!=0) stdev /=(double) nn;
     stdev = sqrt (stdev);

   *noise += 3.0*stdev;
   *std = stdev;
   return 1;
}

/*****************************************************************************
*   CHECK_IF_PEAK() - takes pointers to the data points in the
*	array and determines if the point pointed to by
*	"array[2][2]" is a maximum. If it's max => return TRUE, else return FALSE.
*****************************************************************************/
static int check_if_peak(float array[INTERP_PTS][INTERP_PTS], float peak_thresh,
                         float stdev, int *N_ppts, int **point_status,
                         int ctrace, int cpoint)
{
  int i, j, N;
  float t1, aa, bb, cc;

  N=0;
  i=INTERP_PTS/2;
  t1 = 0.35*fabs(array[i][i]);
/*  if (t1 < peak_thresh) t1 = peak_thresh;
*/

    /* check points around current point */

    if (array[INTERP_PTS/2][INTERP_PTS/2] >= 0.0)  {
       for (i=0; i<INTERP_PTS; i++)
         for (j=0; j<INTERP_PTS; j++)
          {
	   if ((i!=0&&i!=INTERP_PTS-1&&j!=0&&j!=INTERP_PTS-1))
	    {
	     if (array[i][j] >= t1) N++;
             if (array[INTERP_PTS/2][INTERP_PTS/2] > array[i][j])
	        point_status[ctrace+i-INTERP_PTS/2][cpoint+j-INTERP_PTS/2] = -1;
	     else if (array[INTERP_PTS/2][INTERP_PTS/2] < array[i][j])
		{
	         point_status[ctrace][cpoint] = -1;
		 return FALSE;
		}
	    }
	   else if ((i==0||i==INTERP_PTS-1)&&j==2)
            {
	     if (array[i][j] >= t1) N++;
	     if (array[INTERP_PTS/2][INTERP_PTS/2]<array[i][j])
	       {
	        aa = array[INTERP_PTS/2][INTERP_PTS/2];
 	        if (i==0) bb = (aa-array[i+1][j]);
	        else bb = (aa-array[i-1][j]);
	        cc = bb/aa;
	        if (fabs(bb)<0.7*stdev)
		 {
	          point_status[ctrace][cpoint] = -1;
		  return FALSE;
		 }
	        else if (fabs(bb)<2.0*stdev)
		 {
		  aa = array[INTERP_PTS/2][1];	       
	 	  if (i==0) bb = (aa-array[i+1][1]);
	       	  else bb = (aa-array[i-1][1]);
		  bb = bb/aa;
		  if (bb/cc<0.3)
		   {
	            point_status[ctrace][cpoint] = -1;
		    return FALSE;
		   }	  
		  aa = array[INTERP_PTS/2][3];
	 	  if (i==0) bb = (aa-array[i+1][3]);
	       	  else bb = (aa-array[i-1][3]);
		  bb = bb/aa;
		  if (bb/cc<0.3)
		   {
	            point_status[ctrace][cpoint] = -1;
		    return FALSE;
		   }	  
		 }
	       }
	    }

/*	     if (array[INTERP_PTS/2][INTERP_PTS/2]>=array[i][j])
	      {
	       aa = array[i][j];	       
	       if (i==0) bb = fabs(aa-array[i+1][j]);
	       else bb = fabs(aa-array[i-1][j]);
	       if (bb<2.0*stdev||bb/fabs(aa)<0.075)
		{
	         point_status[ctrace+i-INTERP_PTS/2][cpoint+j-INTERP_PTS/2] = -1;
		}
	       else if (bb<3.0*stdev||bb/fabs(aa)<0.15)
		{
		 aa = array[i][1];	       
	       	 if (i==0) bb = fabs(aa-array[i+1][1]);
	       	 else bb = fabs(aa-array[i-1][1]);
	       	 if (bb/fabs(aa)<0.075)
	           point_status[ctrace+i-INTERP_PTS/2][cpoint+j-INTERP_PTS/2] = -1;		  
		 aa = array[i][3];
	       	 if (i==0) bb = fabs(aa-array[i+1][3]);
	       	 else bb = fabs(aa-array[i-1][3]);
	       	 if (bb/fabs(aa)<0.075)
	           point_status[ctrace+i-INTERP_PTS/2][cpoint+j-INTERP_PTS/2] = -1;		  
		}
	       }
	      else
	       {
	        aa = array[INTERP_PTS/2][INTERP_PTS/2];
	        if (i==0) bb = fabs(aa-array[i+1][j]);
	        else bb = fabs(aa-array[i-1][j]);
	        if (bb<2.0*stdev||bb/fabs(aa)<0.075)
		 {
	          point_status[ctrace][cpoint] = -1;
		  return FALSE;
		 }
	        else if (bb<3.0*stdev||bb/fabs(aa)<0.15)
		 {
		  aa = array[INTERP_PTS/2][1];	       
	          if (i==0) bb = fabs(aa-array[i+1][1]);
	          else bb = fabs(aa-array[i-1][1]);
	       	  if (bb/fabs(aa)<0.075)
		   {
	            point_status[ctrace][cpoint] = -1;
		    return FALSE;
		   }	  
		  aa = array[INTERP_PTS/2][3];
	       	  if (i==0) bb = fabs(aa-array[i+1][3]);
 	          else bb = fabs(aa-array[i-1][3]);
	       	  if (bb/fabs(aa)<0.075)
		   {
	            point_status[ctrace][cpoint] = -1;
		    return FALSE;
		   }	  
		 }
	      }
	    }
*/

	   else if ((j==0||j==INTERP_PTS-1)&&i==2)
            {
	     if (array[i][j] >= t1) N++;
	     if (array[INTERP_PTS/2][INTERP_PTS/2]<array[i][j])
	      {
	       aa = array[INTERP_PTS/2][INTERP_PTS/2];
 	       if (j==0) bb = (aa-array[i][j+1]);
	       else bb = (aa-array[i][j-1]);
	       cc = bb/aa;
	       if (fabs(bb)<0.7*stdev)
		{
	         point_status[ctrace][cpoint] = -1;
		 return FALSE;
		}
	       else if (fabs(bb)<2.0*stdev)
		 {
		  aa = array[1][INTERP_PTS/2];  
	          if (j==0) bb =(aa-array[1][j+1]);
	          else bb = (aa-array[1][j-1]);
		  bb = bb/aa;
	       	  if (bb/cc<0.3)
		   {
	            point_status[ctrace][cpoint] = -1;
		    return FALSE;
		   }	  
		  aa = array[3][INTERP_PTS/2];
	       	  if (j==0) bb = (aa-array[3][j+1]);
 	          else bb = (aa-array[3][j-1]);
		  bb = bb/aa;
	       	  if (bb/cc<0.3)
		   {
	            point_status[ctrace][cpoint] = -1;
		    return FALSE;
		   }	  
	        }
	     }
	  }

 	}
       *N_ppts = N;
       return TRUE;
      }
    else  {
       for (i=0; i<INTERP_PTS; i++)
         for (j=0; j<INTERP_PTS; j++)
	  {
	   if ((i!=0&&i!=INTERP_PTS-1&&j!=0&&j!=INTERP_PTS-1))
	    {
 	     if (array[i][j] <= -t1) N++;
             if (array[INTERP_PTS/2][INTERP_PTS/2] < array[i][j])
	        point_status[ctrace+i-INTERP_PTS/2][cpoint+j-INTERP_PTS/2] = -1;
	     else if (array[INTERP_PTS/2][INTERP_PTS/2] > array[i][j])
		{
	         point_status[ctrace][cpoint] = -1;
		 return FALSE;
		}
	    }
	   else if ((i==0||i==INTERP_PTS-1)&&j==2)
            {
	     if (array[i][j] <= -t1) N++;

	     if (array[INTERP_PTS/2][INTERP_PTS/2]>array[i][j])
	       {
	        aa = array[INTERP_PTS/2][INTERP_PTS/2];
 	        if (i==0) bb = (aa-array[i+1][j]);
	        else bb = (aa-array[i-1][j]);
	        cc = bb/aa;
	        if (fabs(bb)<0.7*stdev)
		 {
	          point_status[ctrace][cpoint] = -1;
		  return FALSE;
		 }
	        else if (fabs(bb)<2.0*stdev)
		 {
		  aa = array[INTERP_PTS/2][1];	       
	 	  if (i==0) bb = (aa-array[i+1][1]);
	       	  else bb = (aa-array[i-1][1]);
		  bb = bb/aa;
		  if (bb/cc<0.3)
		   {
	            point_status[ctrace][cpoint] = -1;
		    return FALSE;
		   }	  
		  aa = array[INTERP_PTS/2][3];
	 	  if (i==0) bb = (aa-array[i+1][3]);
	       	  else bb = (aa-array[i-1][3]);
		  bb = bb/aa;
		  if (bb/cc<0.3)
		   {
	            point_status[ctrace][cpoint] = -1;
		    return FALSE;
		   }	  
		 }
	       }
	    }
	   else if ((j==0||j==INTERP_PTS-1)&&i==2)
            {
	     if (array[i][j] <= -t1) N++;

	     if (array[INTERP_PTS/2][INTERP_PTS/2]>array[i][j])
	      {
	       aa = array[INTERP_PTS/2][INTERP_PTS/2];
 	       if (j==0) bb = (aa-array[i][j+1]);
	       else bb = (aa-array[i][j-1]);
	       cc = bb/aa;
	       if (fabs(bb)<0.7*stdev)
		{
	         point_status[ctrace][cpoint] = -1;
		 return FALSE;
		}
	       else if (fabs(bb)<2.0*stdev)
		 {
		  aa = array[1][INTERP_PTS/2];  
	          if (j==0) bb =(aa-array[1][j+1]);
	          else bb = (aa-array[1][j-1]);
		  bb = bb/aa;
	       	  if (bb/cc<0.3)
		   {
	            point_status[ctrace][cpoint] = -1;
		    return FALSE;
		   }	  
		  aa = array[3][INTERP_PTS/2];
	       	  if (j==0) bb = (aa-array[3][j+1]);
 	          else bb = (aa-array[3][j-1]);
		  bb = bb/aa;
	       	  if (bb/cc<0.3)
		   {
	            point_status[ctrace][cpoint] = -1;
		    return FALSE;
		   }	  
	        }
	     }
	  }
  	}
       *N_ppts = N;
       return TRUE;
      }

}


/*****************************************************************************
*  CHECK_IF_MAX compare all the points and gives the peak point in the array 
*
*****************************************************************************/

static int check_if_max(float array[INTERP_PTS][INTERP_PTS], float stdev,
                        int *pt1, int *pt, int **array_status)
{
  int i, j;
  float aa, bb, cc, max;

  max=0.0;
  array_status[INTERP_PTS/2][INTERP_PTS/2] = 1;

    /* check points around current point */

    if (array[INTERP_PTS/2][INTERP_PTS/2] >= 0.0)  {
       for (i=0; i<INTERP_PTS; i++)
         for (j=0; j<INTERP_PTS; j++)
          {
	   if ((i!=0&&i!=INTERP_PTS-1&&j!=0&&j!=INTERP_PTS-1))
	    {
	     if (array[i][j]>max) {max = array[i][j]; *pt1=i; *pt=j;}
             if (array[INTERP_PTS/2][INTERP_PTS/2] < array[i][j])
	         array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;		
	    }
	   else if ((i==0||i==INTERP_PTS-1)&&j==2)
            {
	     if (array[INTERP_PTS/2][INTERP_PTS/2]>=array[i][j])
	       {
	  	aa = array[i][j];
 	        if (i==0) bb = array[i+1][j];
	        else bb = array[i-1][j];
		if (bb>=aa) array_status[i][j]=-1;
		else { 	bb=aa-bb;
			if (bb<0.7*stdev) array_status[i][j] = -2;		 	 
			else array_status[i][j] = 2;
 		     }
	       }
             else
	       {
	        if (array[i][j]>max) {max = array[i][j]; *pt1=i; *pt=j;}
	        aa = array[INTERP_PTS/2][INTERP_PTS/2];
 	        if (i==0) bb = aa-array[i+1][j];
	        else bb = aa-array[i-1][j];
		cc=bb/aa;
	        if (bb<0.7*stdev)
	            array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
	        else if (bb<2.0*stdev)
		 {
		  aa = array[INTERP_PTS/2][1];	       
	 	  if (i==0) bb = (aa-array[i+1][1]);
	       	  else bb = (aa-array[i-1][1]);
		  bb = bb/aa;
		  if (bb/cc<0.3)
	               array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
		  else 
		   {
		     aa = array[INTERP_PTS/2][3];
	 	     if (i==0) bb = (aa-array[i+1][3]);
	             else bb = (aa-array[i-1][3]);
		     bb = bb/aa;
		     if (bb/cc<0.3)
	                array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
		   }		  	  
		 }
	       }
            }

	   else if ((j==0||j==INTERP_PTS-1)&&i==2)
            {
	     if (array[INTERP_PTS/2][INTERP_PTS/2]>=array[i][j])
	       {
	  	aa = array[i][j];
 	        if (j==0) bb = array[i][j+1];
	        else bb = array[i][j-1];
		if (bb>=aa) array_status[i][j]=-1;
		else { 	bb=aa-bb;
			if (bb<0.7*stdev) array_status[i][j] = -2;		 	 
			else array_status[i][j] = 2;
 		     }
	       }
             else
	       {
	        if (array[i][j]>max) {max = array[i][j]; *pt1=i; *pt=j;}
	        aa = array[INTERP_PTS/2][INTERP_PTS/2];
 	        if (j==0) bb = aa-array[i][j+1];
	        else bb = aa-array[i][j-1];
		cc=bb/aa;
	        if (bb<0.7*stdev)
	            array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
	        else if (bb<2.0*stdev)
		 {
		  aa = array[1][INTERP_PTS/2];	       
	 	  if (j==0) bb = (aa-array[1][j+1]);
	       	  else bb = (aa-array[1][j-1]);
		  bb = bb/aa;
		  if (bb/cc<0.3)
	               array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
		  else 
		   {
		     aa = array[3][INTERP_PTS/2];
	 	     if (j==0) bb = (aa-array[3][j+1]);
	             else bb = (aa-array[3][j-1]);
		     bb = bb/aa;
		     if (bb/cc<0.3)
	                array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
		   }		  	  
		 }
	       }
            }
 	}

       if (array_status[INTERP_PTS/2][INTERP_PTS/2]==1) return TRUE;
       else return FALSE;
      }
    else  {
       for (i=0; i<INTERP_PTS; i++)
         for (j=0; j<INTERP_PTS; j++)
	  {
	   if ((i!=0&&i!=INTERP_PTS-1&&j!=0&&j!=INTERP_PTS-1))
	    {
	     if (array[i][j]<max) {max = array[i][j]; *pt1=i; *pt=j;}
             if (array[INTERP_PTS/2][INTERP_PTS/2] > array[i][j])
	         array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;		
	    }
	   else if ((i==0||i==INTERP_PTS-1)&&j==2)
            {
	     if (array[INTERP_PTS/2][INTERP_PTS/2]<=array[i][j])
	       {
	  	aa = array[i][j];
 	        if (i==0) bb = array[i+1][j];
	        else bb = array[i-1][j];
		if (bb<=aa) array_status[i][j]=-1;
		else { 	bb=bb-aa;
			if (bb<0.7*stdev) array_status[i][j] = -2;		 	 
			else array_status[i][j] = 2;
 		     }
	       }
             else
	       {
	        if (array[i][j]<max) {max = array[i][j]; *pt1=i; *pt=j;}
	        aa = array[INTERP_PTS/2][INTERP_PTS/2];
 	        if (i==0) bb = aa-array[i+1][j];
	        else bb = aa-array[i-1][j];
		cc=bb/aa; 
		bb=-bb;
	        if (bb<0.7*stdev)
	            array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
	        else if (bb<2.0*stdev)
		 {
		  aa = array[INTERP_PTS/2][1];	       
	 	  if (i==0) bb = (aa-array[i+1][1]);
	       	  else bb = (aa-array[i-1][1]);
		  bb = bb/aa;
		  if (bb/cc<0.3)
	               array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
		  else 
		   {
		     aa = array[INTERP_PTS/2][3];
	 	     if (i==0) bb = (aa-array[i+1][3]);
	             else bb = (aa-array[i-1][3]);
		     bb = bb/aa;
		     if (bb/cc<0.3)
	                array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
		   }		  	  
		 }
	       }
            }

	   else if ((j==0||j==INTERP_PTS-1)&&i==2)
            {
	     if (array[INTERP_PTS/2][INTERP_PTS/2]<=array[i][j])
	       {
	  	aa = array[i][j];
 	        if (j==0) bb = array[i][j+1];
	        else bb = array[i][j-1];
		if (bb<=aa) array_status[i][j]=-1;
		else { 	bb=bb-aa;
			if (bb<0.7*stdev) array_status[i][j] = -2;		 	 
			else array_status[i][j] = 2;
 		     }
	       }
             else
	       {
	        if (array[i][j]<max) {max = array[i][j]; *pt1=i; *pt=j;}
	        aa = array[INTERP_PTS/2][INTERP_PTS/2];
 	        if (j==0) bb = aa-array[i][j+1];
	        else bb = aa-array[i][j-1];
		cc=bb/aa;
		bb=-bb;
	        if (bb<0.7*stdev)
	            array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
	        else if (bb<2.0*stdev)
		 {
		  aa = array[1][INTERP_PTS/2];	       
	 	  if (j==0) bb = (aa-array[1][j+1]);
	       	  else bb = (aa-array[1][j-1]);
		  bb = bb/aa;
		  if (bb/cc<0.3)
	               array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
		  else 
		   {
		     aa = array[3][INTERP_PTS/2];
	 	     if (j==0) bb = (aa-array[3][j+1]);
	             else bb = (aa-array[3][j-1]);
		     bb = bb/aa;
		     if (bb/cc<0.3)
	                array_status[INTERP_PTS/2][INTERP_PTS/2] = -1;
		   }		  	  
		 }
	       }
            }
 	}

       if (array_status[INTERP_PTS/2][INTERP_PTS/2]==1) return TRUE;
       else return FALSE;
      }

}


/*****************************************************************************
 *   INTERPOLATE_CG(pointbuffer,point,inten) - center of gravity interpolation
 *	about a peak.  pointbuffer[1] contains the peak, pointbuffer[0]
 *	contains the point to the left of pointbuffer[1], and pointbuffer[2]
 *	contains the point to the right of pointbuffer[1].  The center of
 *	gravity is calculated, and "point" returns the fraction off of
 *	pointbuffer[1] that the interpolated peak lies (-0.5 < point < 0.5)
 *	and inten returns the interpolated intensity of the max.
 ****************************************************************************/
static void interpolate_cg(double *pointbuffer, double *point, double *inten)
{
    double sum,l_diff,r_diff;

    l_diff = fabs(pointbuffer[INTERP_PTS/2] - pointbuffer[INTERP_PTS/2-1]);
    r_diff = fabs(pointbuffer[INTERP_PTS/2] - pointbuffer[INTERP_PTS/2+1]);
    sum = l_diff + r_diff;
    *point = (l_diff - r_diff)/(2.0*sum);
    if (*point < -0.5)		/* make sure interpolated point is less than */
      *point = -0.499999999999999999999999;	/* 0.5 away from real point so that it will */
    if (*point > 0.5)		/* round back to correct point */
      *point = 0.499999999999999999999999;
    if (pointbuffer[1] > 0.0)
      *inten = pointbuffer[INTERP_PTS/2]+fabs(*point)*((l_diff>r_diff) ? l_diff : r_diff);
    else
      *inten = pointbuffer[INTERP_PTS/2]-fabs(*point)*((l_diff>r_diff) ? l_diff : r_diff);
}

/****************************************************************************
*   LL2D_DP_TO_FRQ(pt,sw,fn) - returns the frequency value correspoinding
*	to point "pt", given "sw" and "fn".  NOTE : "pt" is a double to
*	allow conversion of fractional points for interpolation.
****************************************************************************/
double ll2d_dp_to_frq(double pt, double sw, int fn)
{
    return(sw*(1.0 - (pt/((double)(fn/2)))));
}

/****************************************************************************
*   LL2D_FRQ_TO_DP(frq,sw,fn) - returns the point value correspoinding
*	to frequency "frq", given "sw" and "fn".  NOTE : rounds to nearest
*	point.
****************************************************************************/
int ll2d_frq_to_dp(double frq, double sw, int fn)
{
    return(intRound((1.0-(frq/sw))*(fn/2)));
}

static int peak_table_valid(peak_table_struct *peak_table)
{
    char h_ch, v_ch;
    int result = FALSE;

    if (peak_table)  {
      if (getnd() == 3)  {
        get_display_label(HORIZ,&h_ch);
        get_display_label(VERT,&v_ch);
        if ((((h_ch==peak_table->f1_label)&&(v_ch == peak_table->f2_label)) ||
              ((h_ch==peak_table->f2_label)&&(v_ch == peak_table->f1_label))) &&
	     (getplaneno() == peak_table->planeno) &&
	     (peak_table->experiment == expdir_to_expnum(curexpdir)))
	  result = TRUE;
	}
      else if (peak_table->experiment == expdir_to_expnum(curexpdir))
	result = TRUE;
      }
    return(result);
}

   
/* return Euclidean distance from diagonal for spectra with any sw and sw1 */
double dist_from_diag(double f1, double f2, int horiz_f1)
{
    double tmp_sw1, tmp_sw2, f1_diag, f2_diag;
 
    if (horiz_f1)  {  /* f1 is HORIZ */
      tmp_sw1 = sw;
      tmp_sw2 = sw1;
      }
    else  {
      tmp_sw1 = sw1;
      tmp_sw2 = sw;
      }
    f1_diag = (f1+(f2*tmp_sw1/tmp_sw2))/2.0;
    f2_diag = (f2+(f1*tmp_sw2/tmp_sw1))/2.0;
    return(sqrt((f1-f1_diag)*(f1-f1_diag)+(f2-f2_diag)*(f2-f2_diag)));
}

    
#define BOUNDS_THRESH 0.1
/******************************************************************************
*  PEAK_BOUNDS(peak_table,f1,f2,amp,f1_1,f1_2,f2_1,f2_2,fwhh1,fwhh2,vol,thresh)-
*	calculates peak bounds for peak at frequency f1,f2.
*	Returns bounds of peak in f1_1, f1_2, f2_1, f2_2, and returns fwhh1
*	and fwhh2 (all Hz).  These are actual f1 and f2 (not horizontal and
*	vertical).  Also returns volume of area bounded by f1_1,f1_2,f2_1,f2_2.
*	Algorithm for bounds and fwhh are to go along f1 and f2 from the peak
*	until either a minimum is reached or the intensity of a point is <
*	peak_thresh from "peakthresh" parameter or the default PEAK_THRESH.
*	FWHH is found by going along f1 and f2 until amplitude is 0.5*amp
*	or a minimum is found.  volume is calculated by summing all points
*	within the region bounded by f1_1, f1_2, f2_1, and f2_2.
******************************************************************************/
static int peak_bounds(peak_table_struct *peak_table, float stdev,
           double f1, double f2, double amp,
           double *f1_1, double *f1_2, double *f2_1, double *f2_2,
           double *fwhh1, double *fwhh2, double *vol,
           double thresh, int **array_status)
{
    int i, j, le, ri, up, dn, pt, pt1;
    double r_le, r_ri, r_up, r_dn;
    int s_le, s_ri, s_up, s_dn;

    double hh, lehh, rihh, dnhh, uphh, fact;
    float *trace, prev_pt;

    float array[INTERP_PTS][INTERP_PTS];
    int j1, pk_ok;

    char ch;
    double slope, slope_thresh = 0.0;

    /* find data points corresponding to f1, f2 */
    get_display_label(HORIZ,&ch);
    if (ch == peak_table->f1_label)  {
      pt = ll2d_frq_to_dp(f1,sw,fn);
      pt1 = ll2d_frq_to_dp(f2,sw1,fn1);
      }
    else  {
      pt = ll2d_frq_to_dp(f2,sw,fn);
      pt1 = ll2d_frq_to_dp(f1,sw1,fn1);
      }
    if ((pt < 0) || (pt >= fn/2))  {
      Werrprintf("Peak frequencies not within spectrum.");
      return 1;
      }
    if ((pt1 < 0) || (pt1 >= fn1/2))  {
      Werrprintf("Peak frequencies not within spectrum.");
      return 1;
      }



/*
    if ((trace = gettrace(pt1-1,0)) == 0) return 1;
    prev[0] = trace[pt-1];
    prev[1] = trace[pt];
    prev[2] = trace[pt+1];
    if ((trace = gettrace(pt1,0)) == 0) return 1;
    curr[0] = trace[pt-1];
    curr[1] = trace[pt];
    curr[2] = trace[pt+1];
    if ((trace = gettrace(pt1+1,0)) == 0) return 1;
    next[0] = trace[pt-1];
    next[1] = trace[pt];
    next[2] = trace[pt+1];

    max = -1.0e+20;
    max_i = 1;   max_j = 1;

*/

    /* find the largest point of (pt1,pt) and the 8 points around it that has
	the same sign as point(pt1,pt).  (Make sure we actually have the 
	point which is the peak.) */

/*    for (j=0;j<3;j++)  {
      if ((fabs(prev[j]) > max) && (fabs(prev[j] - amp) !=
		(fabs(prev[j]) + fabs(amp))))  {
	max = fabs(prev[j]);
	max_i = 0; max_j = j;
	}
      if ((fabs(curr[j]) > max) && (fabs(curr[j] - amp) !=
		(fabs(curr[j]) + fabs(amp))))  {
	max = fabs(curr[j]);
	max_i = 1; max_j = j;
	}
      if ((fabs(next[j]) > max) && (fabs(next[j] - amp) !=
		(fabs(next[j]) + fabs(amp))))  {
	max = fabs(next[j]);
	max_i = 2; max_j = j;
	}
      }
    pt = pt - 1 + max_j - fpnt;
    pt1 = pt1 - 1 + max_i - fpnt1;
*/


     /* set initial array_status for future use */
     for (i = 0; i<INTERP_PTS; i++)
       for (j = 0; j<INTERP_PTS; j++)
	 array_status [i][j] = 0;

     pk_ok=FALSE;

     while(!pk_ok)
      {
       /* get and save data points around current point */
       for (j1=0; j1<INTERP_PTS; j1++)
        {
         if ((trace = gettrace(pt1+j1-INTERP_PTS/2,0)) == 0) return 1;
	   for (j=0;j<INTERP_PTS;j++)
	       array[j1][j] = trace[pt+j-INTERP_PTS/2];
        }

       /* check if point is the maximum */
        if (check_if_max(array, stdev, &j1, &j, array_status)) pk_ok=TRUE;
 	else { pt = pt + j -2;  pt1 = pt1 + j1 -2;}
      }

     s_le = array_status[2][0];
     s_ri = array_status[2][4];
     s_up = array_status[0][2];
     s_dn = array_status[4][2]; 

     if (s_le == 2)
      {
       for (j1=0; j1<INTERP_PTS; j1++)
         {
          if ((trace = gettrace(pt1+j1-INTERP_PTS/2,0)) == 0) return 1;
	    for (j=0;j<INTERP_PTS;j++)
	       array[j1][j] = trace[pt-2+j-INTERP_PTS/2];
         }

       	if (check_if_peak(array, thresh, stdev, &j1, array_status, INTERP_PTS/2, INTERP_PTS/2)
		&&j1 >= MINPOINTS_IN_PEAK)  s_le = 1;
	else s_le = -2;
      }

     if (s_ri == 2)
      {
       for (j1=0; j1<INTERP_PTS; j1++)
         {
          if ((trace = gettrace(pt1+j1-INTERP_PTS/2,0)) == 0) return 1;
	    for (j=0;j<INTERP_PTS;j++)
	       array[j1][j] = trace[pt+2+j-INTERP_PTS/2];
         }

       	if (check_if_peak(array, thresh, stdev, &j1, array_status, INTERP_PTS/2, INTERP_PTS/2)
		&&j1 >= MINPOINTS_IN_PEAK)  s_ri = 1;
	else s_ri = -2;
      }

     if (s_up == 2)
      {
       for (j1=0; j1<INTERP_PTS; j1++)
         {
          if ((trace = gettrace(pt1-2+j1-INTERP_PTS/2,0)) == 0) return 1;
	    for (j=0;j<INTERP_PTS;j++)
	       array[j1][j] = trace[pt+j-INTERP_PTS];
         }

       	if (check_if_peak(array, thresh, stdev, &j1, array_status, INTERP_PTS/2, INTERP_PTS/2)
		&&j1 >= MINPOINTS_IN_PEAK)  s_up = 1;
	else s_up = -2;
      }
     if (s_dn == 2)
      {
       for (j1=0; j1<INTERP_PTS; j1++)
         {
          if ((trace = gettrace(pt1+2+j1-INTERP_PTS/2,0)) == 0) return 1;
	    for (j=0;j<INTERP_PTS;j++)
	       array[j1][j] = trace[pt+j-INTERP_PTS];
         }

       	if (check_if_peak(array, thresh, stdev, &j1, array_status, INTERP_PTS/2, INTERP_PTS/2)
		&&j1 >= MINPOINTS_IN_PEAK)  s_dn = 1;
	else s_dn = -2;
      }

    pt = pt-fpnt;
    pt1 = pt1-fpnt1;


    lehh = 0.0; rihh = 0.0; uphh = 0.0; dnhh = 0.0;

    if ((trace = gettrace(fpnt1+pt1,fpnt)) == 0) return 1;


    if (*(trace+pt) < 0.0)
      fact = -1.0;
    else
      fact = 1.0;
    hh = *(trace+pt)*fact/2.0;

    /* go out from the peak point in each direction (le = left, ri = right,
	up, dn = down) until amplitude is below thresh or a minimum is found.
	"fact" makes sure this works for both positive and negative peaks */
    i = pt-1;

    slope = ((*(trace+i)) - (*(trace+pt)))*fact;
    while ((i > 0) && (slope < slope_thresh) && (*(trace+i)*fact > thresh))  {
      if ((*(trace+i)*fact < hh) && (*(trace+i+1)*fact >= hh))
	lehh = -((*(trace+i+1))-hh*fact)/((*(trace+i+1))-(*(trace+i)))
		+ i + 1;
      i--;
      if (s_le == -2 && i == pt-2) i--;
      slope = ((*(trace+i)) - (*(trace+i+1)))*fact;
      }
    if ((*(trace+i)*fact <= thresh) && (*(trace+i+1)-(*(trace+i)) != 0.0))
	r_le = -((*(trace+i+1))-thresh*fact)/((*(trace+i+1))-(*(trace+i)))
		+ i + 1;
    else
      r_le = i+1;
    le = i+1;
    if (fabs(r_le - le) > 1.0)
      r_le = le;

    i = pt+1;
    slope = ((*(trace+i)) - (*(trace+pt)))*fact;
    while ((i < npnt-1) && (slope < slope_thresh)&&(*(trace+i)*fact > thresh)) {
      if ((*(trace+i)*fact < hh) && (*(trace+i-1)*fact >= hh))
	rihh = ((*(trace+i-1))-hh*fact)/((*(trace+i-1))-(*(trace+i)))
		+ i - 1;
      i++;
      if (s_ri == -2 && i == pt+2) i++;
      slope = ((*(trace+i)) - (*(trace+i-1)))*fact;
      }
    if ((*(trace+i)*fact <= thresh) && (*(trace+i-1)-(*(trace+i)) != 0.0))
	r_ri = ((*(trace+i-1))-thresh*fact)/((*(trace+i-1))-(*(trace+i)))
		+ i - 1;
    else
      r_ri = i-1;
    ri = i-1;
    if (fabs(r_ri - ri) > 1.0)
      r_ri = ri;
      
    prev_pt = trace[pt];
    i = pt1-1;
    if ((trace = gettrace(fpnt1+i,fpnt)) == 0) return 1;
    slope = ((*(trace+pt)) - prev_pt)*fact;
    while ((i > 0) && (slope < slope_thresh) && (*(trace+pt)*fact > thresh))  {
      if ((*(trace+pt)*fact < hh) && (prev_pt*fact >= hh))
	dnhh = -(prev_pt-hh*fact)/(prev_pt-(*(trace+pt)))
		+ i + 1;
      i--;
      if (s_up == -2 && i == pt1-2) i--;
      prev_pt = trace[pt];
      if ((trace = gettrace(fpnt1+i,fpnt)) == 0) return 1;
      slope = ((*(trace+pt)) - prev_pt)*fact;
      }
    if ((*(trace+pt)*fact <= thresh) && (prev_pt-(*(trace+pt)) != 0.0))
	r_dn = -(prev_pt-thresh*fact)/(prev_pt-(*(trace+pt)))
		+ i + 1;
    else
      r_dn = i + 1;
    dn = i+1;
    if (fabs(r_dn - dn) > 1.0)
      r_dn = dn;

    if ((trace = gettrace(fpnt1+pt1,fpnt)) == 0) return 1;
    prev_pt = trace[pt];
    i = pt1+1;
    if ((trace = gettrace(fpnt1+i,fpnt)) == 0) return 1;
    slope = ((*(trace+pt)) - prev_pt)*fact;
    while ((i < npnt1-1)&&(slope<slope_thresh)&&(*(trace+pt)*fact > thresh))  {
      if ((*(trace+pt)*fact < hh) && (prev_pt*fact >= hh))
	uphh = (prev_pt-hh*fact)/(prev_pt-(*(trace+pt)))
		+ i - 1;
      i++;
      if (s_dn == -2 && i == pt1+2) i++;
      prev_pt = trace[pt];
      if ((trace = gettrace(fpnt1+i,fpnt)) == 0) return 1;
      slope = ((*(trace+pt)) - prev_pt)*fact;
      }
    if ((*(trace+pt)*fact <= thresh) && (prev_pt-(*(trace+pt)) != 0.0))
	r_up = (prev_pt-thresh*fact)/(prev_pt-(*(trace+pt)))
		+ i - 1;
    else
      r_up = i - 1;
    up = i-1;
    if (fabs(r_up - up) > 1.0)
      r_up = up;

    if (lehh == 0.0)
	lehh = r_le;
    if (rihh == 0.0)
	rihh = r_ri;
    if (uphh == 0.0)
	uphh = r_up;
    if (dnhh == 0.0)
	dnhh = r_dn;

    get_display_label(HORIZ,&ch);
    if (ch == peak_table->f1_label)  {    /* f1 is horizontal */ 
      *f1_2 = ll2d_dp_to_frq((double)(r_le+fpnt),sw,fn);
      *f1_1 = ll2d_dp_to_frq((double)(r_ri+fpnt),sw,fn);
      *f2_2 = ll2d_dp_to_frq((double)(r_dn+fpnt1),sw1,fn1);
      *f2_1 = ll2d_dp_to_frq((double)(r_up+fpnt1),sw1,fn1);
      *fwhh1 = fabs(ll2d_dp_to_frq(lehh+fpnt,sw,fn) -
			ll2d_dp_to_frq(rihh+fpnt,sw,fn));
      *fwhh2 = fabs(ll2d_dp_to_frq(dnhh+fpnt1,sw1,fn1) -
			ll2d_dp_to_frq(uphh+fpnt1,sw1,fn1));
      }
    else  {
      *f2_2 = ll2d_dp_to_frq((double)(r_le+fpnt),sw,fn);
      *f2_1 = ll2d_dp_to_frq((double)(r_ri+fpnt),sw,fn);
      *f1_2 = ll2d_dp_to_frq((double)(r_dn+fpnt1),sw1,fn1);
      *f1_1 = ll2d_dp_to_frq((double)(r_up+fpnt1),sw1,fn1);
      *fwhh2 = fabs(ll2d_dp_to_frq(lehh+fpnt,sw,fn) -
			ll2d_dp_to_frq(rihh+fpnt,sw,fn));
      *fwhh1 = fabs(ll2d_dp_to_frq(dnhh+fpnt1,sw1,fn1) -
			ll2d_dp_to_frq(uphh+fpnt1,sw1,fn1));
      }
    /* check that bounds surround the peak point */
    if (f1 <= *f1_1)
      *f1_1 = f1 - BOUNDS_THRESH;
    if (f1 >= *f1_2)
      *f1_2 = f1 + BOUNDS_THRESH;
    if (f2 <= *f2_1)
      *f2_1 = f2 - BOUNDS_THRESH;
    if (f2 >= *f2_2)
      *f2_2 = f2 + BOUNDS_THRESH;

    *vol = calc_volume(peak_table,*f1_1,*f1_2,*f2_1,*f2_2);

/*    get_rflrfp(HORIZ,&h_rflrfp);
    get_rflrfp(VERT,&v_rflrfp);
	fprintf(stderr,"bounds: f1_min=%f,f1_max=%f,f2_min=%f,f2_max=%f\n",*f1_1-h_rflrfp,*f1_2-h_rflrfp,*f2_1-v_rflrfp,*f2_2-v_rflrfp);
	fprintf(stderr,"f1_min=%d,f1_max=%d,f2_min=%d,f2_max=%d\n",le+fpnt,ri+fpnt,dn+fpnt1,up+fpnt1);
	fprintf(stderr,"f1_min=%f,f1_max=%f,f2_min=%f,f2_max=%f\n",
	    ll2d_dp_to_frq((double)(le+fpnt),sw,fn)-h_rflrfp,
	    ll2d_dp_to_frq((double)(ri+fpnt),sw,fn)-h_rflrfp,
	    ll2d_dp_to_frq((double)(dn+fpnt1),sw1,fn1)-v_rflrfp,
	    ll2d_dp_to_frq((double)(up+fpnt1),sw1,fn1)-v_rflrfp);*/
   return(0);
}

/******************************************************************************
*  ADJUST_PEAK_BOUNDS(peak_table) - adjust peak	bounds of all peaks within
*	the display region so that none overlap.  Algorithm just does pairwise
*	comparisons and shrinks peak bounds of a pair of peaks in the direction
*	of least overlap such that the new bounds of the overlapping peaks
*	are the same in the adjusted direction (one peak doesn't have its
*	bound adjusted and the other has its bound shrunk to coincide with
*	the others bound) or halfway between the two peak centers, whichever
*	maximizes the sizes of both bounds.  Bounds are never made larger,
*	only smaller.  This algorithm doesn't always work in an intelligent
*	manner, since it only compares peaks two at a time.
*	Recalcuates volumes of peaks based on new bounds.
******************************************************************************/
static void adjust_peak_bounds(peak_table_struct *peak_table)
{
    struct node {
	peak_struct *ptr;
	struct node *next;
	};
    struct node *peaks, *pres, *new;
    peak_struct *pk_ptr;
    char ch;
    double ave, temp, f1_rflrfp, f2_rflrfp;
//    double f1_min, f1_max, f2_min, f2_max;

    init_peak_table(&peak_table);
    get_rflrfp(HORIZ,&f2_rflrfp);
    get_rflrfp(VERT,&f1_rflrfp);
    get_display_label(HORIZ,&ch);
    if (ch == peak_table->f1_label)  {    /* f1 is horizontal */ 
      temp = f1_rflrfp;
      f1_rflrfp = f2_rflrfp;
      f2_rflrfp = temp;
      }
    
#ifdef XXX
    if (ch == peak_table->f1_label)
      {  /* f1 horizontal */
      f1_max = ll2d_dp_to_frq((double)fpnt,sw,fn);
      f1_min = ll2d_dp_to_frq((double)(fpnt+npnt),sw,fn);
      f2_max = ll2d_dp_to_frq((double)fpnt1,sw1,fn1);
      f2_min = ll2d_dp_to_frq((double)(fpnt1+npnt1),sw1,fn1);
      }
    else
      {  /* f2 horizontal */
      f2_max = ll2d_dp_to_frq((double)fpnt,sw,fn);
      f2_min = ll2d_dp_to_frq((double)(fpnt+npnt),sw,fn);
      f1_max = ll2d_dp_to_frq((double)fpnt1,sw1,fn1);
      f1_min = ll2d_dp_to_frq((double)(fpnt1+npnt1),sw1,fn1);
      }
#endif
    peaks = NULL;
    pres = peaks;
/* adjust all peaks within current display region */
    pk_ptr = peak_table->head;
    while (pk_ptr)  {
      if (in_display(pk_ptr->f1-f1_rflrfp,pk_ptr->f2-f2_rflrfp))  {
	if ((new = (struct node *)allocateWithId(sizeof(struct node),
		"peak_temp")) == 0) {
          Werrprintf("adjust_peak_bounds: cannot allocate node");
          }
	if (peaks == NULL)  {
	  peaks = new;
	  pres = new;
	  new->ptr = pk_ptr;
	  new->next = NULL;
	  }
	else  {
	  new->ptr = pk_ptr;
	  pres->next = new;
	  new->next = NULL;
	  pres = new;
	  }
	}
      pk_ptr = pk_ptr->next;
      }
/* search through peak records and adjust bounds of peaks whose edges
   overlap */
    pres = peaks;
    while (pres)  {
      new = pres->next;
      while (new)  {
	if (((pres->ptr->f1_min >= new->ptr->f1_min) &&
	    (pres->ptr->f2_min >= new->ptr->f2_min) &&
	    (pres->ptr->f1_max <= new->ptr->f1_max) &&
	    (pres->ptr->f2_max <= new->ptr->f2_max)) ||
	   ((new->ptr->f1_min >= pres->ptr->f1_min) &&
	    (new->ptr->f2_min >= pres->ptr->f2_min) &&
	    (new->ptr->f1_max <= pres->ptr->f1_max) &&
	    (new->ptr->f2_max <= pres->ptr->f2_max)))  {
	  /* skip because one peak's bounds are completely inside another's */
	  }
	else if (((pres->ptr->f1_min == 0.0) &&
	    (pres->ptr->f2_min == 0.0) &&
	    (pres->ptr->f1_max == 0.0) &&
	    (pres->ptr->f2_max == 0.0)) ||
	   ((new->ptr->f1_min == 0.0) &&
	    (new->ptr->f2_min == 0.0) &&
	    (new->ptr->f1_max == 0.0) &&
	    (new->ptr->f2_max == 0.0)))  {
	  /* skip because no bounds are defined for a peak */
	  }
	else  {
	  /* check upper right corner of pres */
	  if (((pres->ptr->f1_min < new->ptr->f1_max) &&
	       (pres->ptr->f1_min > new->ptr->f1_min)) &&
	      ((pres->ptr->f2_min < new->ptr->f2_max) &&
	       (pres->ptr->f2_min > new->ptr->f2_min)))  {
	    if (fabs(pres->ptr->f1_min-new->ptr->f1_max) <
	        fabs(pres->ptr->f2_min-new->ptr->f2_max)) { /*adjust f1 bounds*/
	      ave = (pres->ptr->f1 + new->ptr->f1)/2.0;
	      if ((ave > pres->ptr->f1_min) && (ave < new->ptr->f1_max))  {
		pres->ptr->f1_min = ave;
		new->ptr->f1_max = ave;
		}
	      if (ave > new->ptr->f1_max)
		pres->ptr->f1_min = new->ptr->f1_max;
	      if (ave < pres->ptr->f1_min)
		new->ptr->f1_max = pres->ptr->f1_min;
	      }
	    else  {    /* adjust f2 bounds */
	      ave = (pres->ptr->f2 + new->ptr->f2)/2.0;
	      if ((ave > pres->ptr->f2_min) && (ave < new->ptr->f2_max))  {
		  pres->ptr->f2_min = ave;
		  new->ptr->f2_max = ave;
		  }
	      if (ave > new->ptr->f2_max)
		pres->ptr->f2_min = new->ptr->f2_max;
	      if (ave < pres->ptr->f2_min)
		new->ptr->f2_max = pres->ptr->f2_min;
	      }
	    }
	  /* check upper left corner of pres */
	  if (((pres->ptr->f1_max < new->ptr->f1_max) &&
	       (pres->ptr->f1_max > new->ptr->f1_min)) &&
	      ((pres->ptr->f2_min < new->ptr->f2_max) &&
	       (pres->ptr->f2_min > new->ptr->f2_min)))  {
	    if (fabs(pres->ptr->f1_max-new->ptr->f1_min) <
	        fabs(pres->ptr->f2_min-new->ptr->f2_max)) { /*adjust f1 bounds*/
	      ave = (pres->ptr->f1 + new->ptr->f1)/2.0;
	      if ((ave < pres->ptr->f1_max) && (ave > new->ptr->f1_min))  {
		pres->ptr->f1_max = ave;
		new->ptr->f1_min = ave;
		}
	      if (ave > pres->ptr->f1_max)
		new->ptr->f1_min = pres->ptr->f1_max;
	      if (ave < new->ptr->f1_min)
		pres->ptr->f1_max = new->ptr->f1_min;
	      }
	    else  {  /* adjust f2 bounds */
	      ave = (pres->ptr->f2 + new->ptr->f2)/2.0;
	      if ((ave > pres->ptr->f2_min) && (ave < new->ptr->f2_max))  {
		pres->ptr->f2_min = ave;
		new->ptr->f2_max = ave;
		}
	      if (ave > new->ptr->f2_max)
		pres->ptr->f2_min = new->ptr->f2_max;
	      if (ave < pres->ptr->f2_min)
		new->ptr->f2_max = pres->ptr->f2_min;
	      }
	    }
	  /* check lower left corner of pres */
	  if (((pres->ptr->f1_max < new->ptr->f1_max) &&
	       (pres->ptr->f1_max > new->ptr->f1_min)) &&
	      ((pres->ptr->f2_max < new->ptr->f2_max) &&
	       (pres->ptr->f2_max > new->ptr->f2_min)))  {
	    if (fabs(pres->ptr->f1_max-new->ptr->f1_min) <
	        fabs(pres->ptr->f2_max-new->ptr->f2_min)) { /*adjust f1 bounds*/
	      ave = (pres->ptr->f1 + new->ptr->f1)/2.0;
	      if ((ave < pres->ptr->f1_max) && (ave > new->ptr->f1_min))  {
		pres->ptr->f1_max = ave;
		new->ptr->f1_min = ave;
		}
	      if (ave > pres->ptr->f1_max)
		new->ptr->f1_min = pres->ptr->f1_max;
	      if (ave < new->ptr->f1_min)
		pres->ptr->f1_max = new->ptr->f1_min;
	      }
	    else  {  /* adjust f2 bounds */
	      ave = (pres->ptr->f2 + new->ptr->f2)/2.0;
	      if ((ave < pres->ptr->f2_max) && (ave > new->ptr->f2_min))  {
		pres->ptr->f2_max = ave;
		new->ptr->f2_min = ave;
		}
	      if (ave > pres->ptr->f2_max)
		new->ptr->f2_min = pres->ptr->f2_max;
	      if (ave < new->ptr->f2_min)
		pres->ptr->f2_max = new->ptr->f2_min;
	      }
	    }
	  /* check lower right corner of pres */
	  if (((pres->ptr->f1_min < new->ptr->f1_max) &&
	       (pres->ptr->f1_min > new->ptr->f1_min)) &&
	      ((pres->ptr->f2_max < new->ptr->f2_max) &&
	       (pres->ptr->f2_max > new->ptr->f2_min)))  {
	    if (fabs(pres->ptr->f1_min-new->ptr->f1_max) <
	        fabs(pres->ptr->f2_max-new->ptr->f2_min)) { /*adjust f1 bounds*/
	      ave = (pres->ptr->f1 + new->ptr->f1)/2.0;
	      if ((ave > pres->ptr->f1_min) && (ave < new->ptr->f1_max))  {
		pres->ptr->f1_min = ave;
		new->ptr->f1_max = ave;
		}
	      if (ave > new->ptr->f1_max)
		pres->ptr->f1_min = new->ptr->f1_max;
	      if (ave < pres->ptr->f1_min)
		new->ptr->f1_max = pres->ptr->f1_min;
	      }
	    else  {  /* adjust f2 bounds */
	      ave = (pres->ptr->f2 + new->ptr->f2)/2.0;
	      if ((ave < pres->ptr->f2_max) && (ave > new->ptr->f2_min))  {
		pres->ptr->f2_max = ave;
		new->ptr->f2_min = ave;
		}
	      if (ave > pres->ptr->f2_max)
		new->ptr->f2_min = pres->ptr->f2_max;
	      if (ave < new->ptr->f2_min)
		pres->ptr->f2_max = new->ptr->f2_min;
	      }
	    }
	  }
	new = new->next;
	}
      pres = pres->next;
      }
	      
/* search through again and fix any bounds that are still overlapping */
    pres = peaks;
    while (pres)  {
      new = pres->next;
      while (new)  {
	if (((pres->ptr->f1_min >= new->ptr->f1_min) &&
	    (pres->ptr->f2_min >= new->ptr->f2_min) &&
	    (pres->ptr->f1_max <= new->ptr->f1_max) &&
	    (pres->ptr->f2_max <= new->ptr->f2_max)) ||
	   ((new->ptr->f1_min >= pres->ptr->f1_min) &&
	    (new->ptr->f2_min >= pres->ptr->f2_min) &&
	    (new->ptr->f1_max <= pres->ptr->f1_max) &&
	    (new->ptr->f2_max <= pres->ptr->f2_max)))  {
	  /* skip because one peak's bounds are completely inside another's */
	  }
	else if (((pres->ptr->f1_min == 0.0) &&
	    (pres->ptr->f2_min == 0.0) &&
	    (pres->ptr->f1_max == 0.0) &&
	    (pres->ptr->f2_max == 0.0)) ||
	   ((new->ptr->f1_min == 0.0) &&
	    (new->ptr->f2_min == 0.0) &&
	    (new->ptr->f1_max == 0.0) &&
	    (new->ptr->f2_max == 0.0)))  {
	  /* skip because no bounds are defined for a peak */
	  }
	else  {
	  /* check f1_min edge of pres */
	  if (((pres->ptr->f1_min < new->ptr->f1_max) &&
	       (pres->ptr->f1_min > new->ptr->f1_min)) &&
	      (((pres->ptr->f2_max > new->ptr->f2_min) &&
		(pres->ptr->f2_min < new->ptr->f2_min)) ||
	       ((pres->ptr->f2_max > new->ptr->f2_max) &&
		(pres->ptr->f2_min < new->ptr->f2_max)) ||
	       ((pres->ptr->f2_max < new->ptr->f2_max) &&
		(pres->ptr->f2_min > new->ptr->f2_min))))  {
	    ave = (pres->ptr->f1 + new->ptr->f1)/2.0;
	    pres->ptr->f1_min = ave;
	    new->ptr->f1_max = ave;
	    }
	  /* check f1_max edge of pres */
	  if (((pres->ptr->f1_max > new->ptr->f1_min) &&
	       (pres->ptr->f1_max < new->ptr->f1_max)) &&
	      (((pres->ptr->f2_max > new->ptr->f2_min) &&
		(pres->ptr->f2_min < new->ptr->f2_min)) ||
	       ((pres->ptr->f2_max > new->ptr->f2_max) &&
		(pres->ptr->f2_min < new->ptr->f2_max)) ||
	       ((pres->ptr->f2_max < new->ptr->f2_max) &&
		(pres->ptr->f2_min > new->ptr->f2_min))))  {
	    ave = (pres->ptr->f1 + new->ptr->f1)/2.0;
	    pres->ptr->f1_max = ave;
	    new->ptr->f1_min = ave;
	    }
	  /* check f2_min edge of pres */
	  if (((pres->ptr->f2_min < new->ptr->f2_max) &&
	       (pres->ptr->f2_min > new->ptr->f2_min)) &&
	      (((pres->ptr->f1_max > new->ptr->f1_min) &&
		(pres->ptr->f1_min < new->ptr->f1_min)) ||
	       ((pres->ptr->f1_max > new->ptr->f1_max) &&
		(pres->ptr->f1_min < new->ptr->f1_max)) ||
	       ((pres->ptr->f1_max < new->ptr->f1_max) &&
		(pres->ptr->f1_min > new->ptr->f1_min))))  {
	    ave = (pres->ptr->f2 + new->ptr->f2)/2.0;
	    pres->ptr->f2_min = ave;
	    new->ptr->f2_max = ave;
	    }
	  /* check f2_max edge of pres */
	  if (((pres->ptr->f2_max > new->ptr->f2_min) &&
	       (pres->ptr->f2_max < new->ptr->f2_max)) &&
	      (((pres->ptr->f1_max > new->ptr->f1_min) &&
		(pres->ptr->f1_min < new->ptr->f1_min)) ||
	       ((pres->ptr->f1_max > new->ptr->f1_max) &&
		(pres->ptr->f1_min < new->ptr->f1_max)) ||
	       ((pres->ptr->f1_max < new->ptr->f1_max) &&
		(pres->ptr->f1_min > new->ptr->f1_min))))  {
	    ave = (pres->ptr->f2 + new->ptr->f2)/2.0;
	    pres->ptr->f2_max = ave;
	    new->ptr->f2_min = ave;
	    }
	  }
	new = new->next;
	}
      pres = pres->next;
      }
/* calculate volumes of new peaks */
    pres = peaks;
    while (pres)  {
      if ((pres->ptr->f1_min != 0.0) ||
	    (pres->ptr->f2_min != 0.0) ||
	    (pres->ptr->f1_max != 0.0) ||
	    (pres->ptr->f2_max != 0.0))  {
	/* make sure bounds actually contain the peak */
	if (pres->ptr->f1_min >= pres->ptr->f1)
	  pres->ptr->f1_min = pres->ptr->f1-BOUNDS_THRESH;
	if (pres->ptr->f1_max <= pres->ptr->f1)
	  pres->ptr->f1_max = pres->ptr->f1+BOUNDS_THRESH;
	if (pres->ptr->f2_min >= pres->ptr->f2)
	  pres->ptr->f2_min = pres->ptr->f2-BOUNDS_THRESH;
	if (pres->ptr->f2_max <= pres->ptr->f2)
	  pres->ptr->f2_max = pres->ptr->f2+BOUNDS_THRESH;
	pres->ptr->vol = calc_volume(peak_table,pres->ptr->f1_min,
			pres->ptr->f1_max,pres->ptr->f2_min,pres->ptr->f2_max);
	write_peak_file_record(peak_table,pres->ptr,pres->ptr->key);
	}
      pres = pres->next;
      }
    releaseAllWithId("peak_temp");
}

/******************************************************************************
*  CALC_VOLUME(peak_table,f1_min,f1_max,f2_min,f2_max) - calculates volume of
*	area within frequency bounds (sums all data points within area).
******************************************************************************/
static double calc_volume(peak_table_struct *peak_table,
                          double f1_min, double f1_max,
                          double f2_min, double f2_max)
{
    int h_min_pt, h_max_pt, v_min_pt, v_max_pt, i, j;
    char ch;
    float *trace;
    double vol;

    get_display_label(HORIZ,&ch);
    if (ch == peak_table->f1_label)  {    /* f1 is horizontal */ 
      h_min_pt = ll2d_frq_to_dp(f1_min,sw,fn);
      if (f1_min > ll2d_dp_to_frq((double)h_min_pt,sw,fn))
	h_min_pt--;
      h_max_pt = ll2d_frq_to_dp(f1_max,sw,fn);
      if (f1_max < ll2d_dp_to_frq((double)h_max_pt,sw,fn))
	h_max_pt++;
      v_min_pt = ll2d_frq_to_dp(f2_min,sw1,fn1);
      if (f2_min > ll2d_dp_to_frq((double)v_min_pt,sw1,fn1))
	v_min_pt--;
      v_max_pt = ll2d_frq_to_dp(f2_max,sw1,fn1);
      if (f2_max < ll2d_dp_to_frq((double)v_max_pt,sw1,fn1))
	v_max_pt++;
      }
    else   {
      v_min_pt = ll2d_frq_to_dp(f1_min,sw1,fn1);
      if (f1_min > ll2d_dp_to_frq((double)v_min_pt,sw1,fn1))
	v_min_pt--;
      v_max_pt = ll2d_frq_to_dp(f1_max,sw1,fn1);
      if (f1_max < ll2d_dp_to_frq((double)v_max_pt,sw1,fn1))
	v_max_pt++;
      h_min_pt = ll2d_frq_to_dp(f2_min,sw,fn);
      if (f2_min > ll2d_dp_to_frq((double)h_min_pt,sw,fn))
	h_min_pt--;
      h_max_pt = ll2d_frq_to_dp(f2_max,sw,fn);
      if (f2_max < ll2d_dp_to_frq((double)h_max_pt,sw,fn))
	h_max_pt++;
      }
    if ((h_min_pt < 0) || (h_min_pt >= fn/2))  {
      Werrprintf("Peak bounds frequencies not within spectrum.");
      return 1.0;
      }
    if ((h_max_pt < 0) || (h_max_pt >= fn/2))  {
      Werrprintf("Peak bounds frequencies not within spectrum.");
      return 1.0;
      }
    if ((v_min_pt < 0) || (v_min_pt >= fn1/2))  {
      Werrprintf("Peak bounds frequencies not within spectrum.");
      return 1.0;
      }
    if ((v_max_pt < 0) || (v_max_pt >= fn1/2))  {
      Werrprintf("Peak bounds frequencies not within spectrum.");
      return 1.0;
      }
    vol = 0.0;
    for (i=v_max_pt;i<=v_min_pt;i++)  {
      trace = gettrace(i,h_max_pt);
      for (j=0;j<=h_min_pt-h_max_pt;j++)
        {
        if (abs_vol_flag == NO)         /* Normal volume calculation */
                {
                vol += *(trace+j);
                }
        else                            /* Special volume calculation */
                {
                if (*(trace+j) > 0.0)   vol += *(trace+j);
                else                    vol -= *(trace+j);
                }
        }
      }
    return(vol);
}
    

/******************************************************************************
*  PEAK_TO_PIXEL() -  get drawing coordinates for the position of a peak
******************************************************************************/
static void peak_to_pixel(peak_struct *peak_ptr, char ch,
                          double h_rflrfp, double v_rflrfp, int *x, int *y)
{
    double f1, f2;

    if (ch == peak_table->f1_label)  {
      f1 = peak_ptr->f1 - h_rflrfp;
      f2 = peak_ptr->f2 - v_rflrfp;
      }
    else  {
      f1 = peak_ptr->f1 - v_rflrfp;
      f2 = peak_ptr->f2 - h_rflrfp;
      }
    if (in_display(f1,f2))  {
      if (ch == peak_table->f1_label)  {
	*x = dfpnt + dnpnt - intRound((double) dnpnt * (f1-sp) / wp);
	*y = dfpnt2 + dnpnt2-intRound((double) dnpnt2 * (f2-sp1) / wp1);
	}
      else  {
	*x = dfpnt + dnpnt - intRound((double) dnpnt * (f2-sp) / wp);
	*y = dfpnt2 + dnpnt2-intRound((double) dnpnt2 * (f1-sp1) / wp1);
	}
      if (*x < dfpnt) *x = dfpnt;
      if (*x >= dfpnt+dnpnt) *x = dfpnt+dnpnt-1;
      if (*y < dfpnt2) *y = dfpnt2;
      if (*y >= dfpnt2+dnpnt2) *y = dfpnt2+dnpnt2-1;
      }
    else  {
      *x = -1;
      *y = -1;
      }
    /* This calculation is apparently off by one from the rest of Vnmr */
    *x -= 1;
    *y -= 1;
}

/******************************************************************************
*  BOX_TO_PIXEL() -  get drawing coordinates for the sides of the peak
*	bounds box.
******************************************************************************/
static void box_to_pixel(peak_struct *peak_ptr, char ch,
            double h_rflrfp, double v_rflrfp,
            int *x1, int *y1, int *x2, int *y2)
{
    double f1, f2, f1_min, f1_max, f2_min, f2_max;

    if (ch == peak_table->f1_label)  {
         f1 = peak_ptr->f1 - h_rflrfp;
         f1_min = peak_ptr->f1_min - h_rflrfp;
         f1_max = peak_ptr->f1_max - h_rflrfp;
         f2 = peak_ptr->f2 - v_rflrfp;
         f2_min = peak_ptr->f2_min - v_rflrfp;
         f2_max = peak_ptr->f2_max - v_rflrfp;
         }
    else  {
         f1 = peak_ptr->f1 - v_rflrfp;
         f1_min = peak_ptr->f1_min - v_rflrfp;
         f1_max = peak_ptr->f1_max - v_rflrfp;
         f2 = peak_ptr->f2 - h_rflrfp;
         f2_min = peak_ptr->f2_min - h_rflrfp;
         f2_max = peak_ptr->f2_max - h_rflrfp;
         }
    if (in_display(f1,f2))  {
      if (ch == peak_table->f1_label)  {
	*x2 = dfpnt + dnpnt - intRound((double) dnpnt * (f1_min-sp) / wp);
	*y2 = dfpnt2 + dnpnt2-intRound((double) dnpnt2 * (f2_min-sp1) / wp1);
	*x1 = dfpnt + dnpnt - intRound((double) dnpnt * (f1_max-sp) / wp);
	*y1 = dfpnt2 + dnpnt2-intRound((double) dnpnt2 * (f2_max-sp1) / wp1);
	}
      else  {
	*x2 = dfpnt + dnpnt - intRound((double) dnpnt * (f2_min-sp) / wp);
	*y2 = dfpnt2 + dnpnt2-intRound((double) dnpnt2 * (f1_min-sp1) / wp1);
	*x1 = dfpnt + dnpnt - intRound((double) dnpnt * (f2_max-sp) / wp);
	*y1 = dfpnt2 + dnpnt2-intRound((double) dnpnt2 * (f1_max-sp1) / wp1);
	}
      if (*x1 < dfpnt) *x1 = dfpnt;
      if (*x1 >= dfpnt+dnpnt) *x1 = dfpnt+dnpnt-1;
      if (*y1 < dfpnt2) *y1 = dfpnt2;
      if (*y1 >= dfpnt2+dnpnt2) *y1 = dfpnt2+dnpnt2-1;
      if (*x2 < dfpnt) *x2 = dfpnt;
      if (*x2 >= dfpnt+dnpnt) *x2 = dfpnt+dnpnt-1;
      if (*y2 < dfpnt2) *y2 = dfpnt2;
      if (*y2 >= dfpnt2+dnpnt2) *y2 = dfpnt2+dnpnt2-1;
      }
    else  {
      *x1 = -1;
      *y1 = -1;
      *x2 = -1;
      *y2 = -1;
      }
}

/******************************************************************************
*  DRAW_PEAKS(peak_table) - draws "+", number, box and label for each
*	peak in the peak table based on the value of ll2dmode.
******************************************************************************/
static void draw_peaks(peak_table_struct *peak_table)
{
    peak_struct *peak_ptr;
    char ch, mode_string[NUM_LL2DMODE+1];
    int x1,y1,x2,y2;
    double h_rflrfp, v_rflrfp;

    get_rflrfp(HORIZ,&h_rflrfp);
    get_rflrfp(VERT,&v_rflrfp);
    get_display_label(HORIZ,&ch);
    if (P_getstring(GLOBAL,"ll2dmode",mode_string,1,NUM_LL2DMODE+1))  {
      mode_string[SHOW_PEAKS] = 'y';
      mode_string[SHOW_NUMS] = 'y';
      mode_string[SHOW_BOXES] = 'y';
      mode_string[SHOW_LABELS] = 'y';
      }
    normalmode();
    Wshow_graphics();
    Wgmode();
    peak_ptr = peak_table->head;
    while (peak_ptr)  {
      peak_to_pixel(peak_ptr,ch,h_rflrfp,v_rflrfp,&x1,&y1);
      if ((x1 > -1) && (y1 > -1))  {
        if (mode_string[SHOW_PEAKS] == 'y')  {
          color(PEAK_COLOR);
	  amove(x1-intRound(TICK_LEN*ppmm),y1);
	  adraw(x1+intRound(TICK_LEN*ppmm),y1);
	  amove(x1,y1-intRound(TICK_LEN*ppmm));
	  adraw(x1,y1+intRound(TICK_LEN*ppmm));
	  }
        if (mode_string[SHOW_NUMS] == 'y')
          write_peak_num(x1,y1,peak_ptr->key,intRound((TICK_LEN-1)*ppmm));
        if (mode_string[SHOW_LABELS] == 'y')
          write_label(x1,y1,peak_ptr->label,intRound(TICK_LEN*ppmm));
        if (mode_string[SHOW_BOXES] == 'y')  {
          if ((peak_ptr->f1_min != 0.0) || (peak_ptr->f1_max != 0.0) || 
	     (peak_ptr->f2_min != 0.0) || (peak_ptr->f2_max != 0.0))  {
	    box_to_pixel(peak_ptr,ch,h_rflrfp,v_rflrfp,&x1,&y1,&x2,&y2);
	    if ((x1 > -1) && (y1 > -1) && (x2 > -1) && (y2 > -1))  {
	      if (get_phase_mode(HORIZ)&&get_phase_mode(VERT)) { /* phase mode */
		char thickName[64];
		getOptName(PHPeakBox,thickName);
		set_line_thickness(thickName);
                color(PH_BOX_COLOR);
	      } else {						/* AV mode */
		char thickName[64];
		getOptName(AVPeakBox,thickName);
		set_line_thickness(thickName);
                color(AV_BOX_COLOR);
	      }
	      amove(x1,y1);
	      adraw(x1,y2);
	      adraw(x2,y2);
	      adraw(x2,y1);
	      adraw(x1,y1);
   	      }
	    }
          }
        }
      peak_ptr = peak_ptr->next;
      }
}

/******************************************************************************
*  DRAW_ONE_PEAK(peak_ptr,mode) - draws "+", number, and label for peak
*	"peak_ptr".
******************************************************************************/
static void draw_one_peak(peak_struct *peak_ptr, int mode)
{
    char ch, mode_string[NUM_LL2DMODE+1];
    int x1,y1;
    double h_rflrfp, v_rflrfp;

    get_rflrfp(HORIZ,&h_rflrfp);
    get_rflrfp(VERT,&v_rflrfp);
    get_display_label(HORIZ,&ch);
    if (P_getstring(GLOBAL,"ll2dmode",mode_string,1,NUM_LL2DMODE+1))  {
      mode_string[SHOW_PEAKS] = 'y';
      mode_string[SHOW_NUMS] = 'y';
      mode_string[SHOW_BOXES] = 'y';
      mode_string[SHOW_LABELS] = 'y';
      }
    peak_to_pixel(peak_ptr,ch,h_rflrfp,v_rflrfp,&x1,&y1);
    if ((x1 > -1) && (y1 > -1))  {
         Wshow_graphics();
         Wgmode();
	 if (mode == XORMODE)
	   xormode();
	 else
           normalmode();
         color(PEAK_COLOR);
	 if (mode_string[SHOW_PEAKS] == 'y')  {
	   amove(x1-intRound(TICK_LEN*ppmm),y1);
	   adraw(x1+intRound(TICK_LEN*ppmm),y1);
	   amove(x1,y1-intRound(TICK_LEN*ppmm));
	   adraw(x1,y1+intRound(TICK_LEN*ppmm));
	   }
	 if (mode_string[SHOW_NUMS] == 'y')
	   write_peak_num(x1,y1,peak_ptr->key,intRound((TICK_LEN-1)*ppmm));
	 if (mode_string[SHOW_LABELS] == 'y')
	   write_label(x1,y1,peak_ptr->label,intRound(TICK_LEN*ppmm));
         }
}

/******************************************************************************
*  DRAW_ONE_BOX(peak_ptr,mode) - draws bounds for peak "peak_ptr" if bounds
*	exist.
******************************************************************************/
static void draw_one_box(peak_struct *peak_ptr, int mode)
{
    char ch, mode_string[NUM_LL2DMODE+1];
    int x1,y1,x2,y2;
    double h_rflrfp, v_rflrfp;

    get_rflrfp(HORIZ,&h_rflrfp);
    get_rflrfp(VERT,&v_rflrfp);
    get_display_label(HORIZ,&ch);
    if (P_getstring(GLOBAL,"ll2dmode",mode_string,1,NUM_LL2DMODE+1))  {
      mode_string[SHOW_PEAKS] = 'y';
      mode_string[SHOW_BOXES] = 'y';
      mode_string[SHOW_NUMS] = 'y';
      mode_string[SHOW_LABELS] = 'y';
      }
    if (mode_string[SHOW_BOXES] == 'y')  {
      if ((peak_ptr->f1_min != 0.0) || (peak_ptr->f1_max != 0.0) || 
	(peak_ptr->f2_min != 0.0) || (peak_ptr->f2_max != 0.0))  {
	box_to_pixel(peak_ptr,ch,h_rflrfp,v_rflrfp,&x1,&y1,&x2,&y2);
	if ((x1 > -1) && (y1 > -1) && (x2 > -1) && (y2 > -1))  {
           Wshow_graphics();
           Wgmode();
	   if (mode == XORMODE)
	     xormode();
	   else
             normalmode();
	   if (get_phase_mode(HORIZ) && get_phase_mode(VERT)) { /* phase mode */
		char thickName[64];
		getOptName(PHPeakBox,thickName);
		set_line_thickness(thickName);
                color(PH_BOX_COLOR);
	   } else	 {					/* AV mode */
		char thickName[64];
		getOptName(AVPeakBox,thickName);
		set_line_thickness(thickName);
                color(AV_BOX_COLOR);
	   }
	   amove(x1,y1);
	   adraw(x1,y2);
	   adraw(x2,y2);
	   adraw(x2,y1);
	   adraw(x1,y1);
	   }
	}
     }
}

/******************************************************************************
*  WRITE_ONE_LABEL(peak_ptr,mode) - writes label for peak "peak_ptr".
******************************************************************************/
static void write_one_label(peak_struct *peak_ptr, int mode)
{
    char ch;
    int x1,y1;
    double h_rflrfp, v_rflrfp;
    char mode_string[NUM_LL2DMODE+1];

    get_rflrfp(HORIZ,&h_rflrfp);
    get_rflrfp(VERT,&v_rflrfp);
    get_display_label(HORIZ,&ch);
    if (P_getstring(GLOBAL,"ll2dmode",mode_string,1,NUM_LL2DMODE+1))  {
      mode_string[SHOW_PEAKS] = 'y';
      mode_string[SHOW_NUMS] = 'y';
      mode_string[SHOW_BOXES] = 'y';
      mode_string[SHOW_LABELS] = 'y';
      }
    peak_to_pixel(peak_ptr,ch,h_rflrfp,v_rflrfp,&x1,&y1);
    if ((x1 > -1) && (y1 > -1))  {
         Wshow_graphics();
         Wgmode();
	 if (mode == XORMODE)
	   xormode();
	 else
           normalmode();
/*	 if (mode_string[SHOW_NUMS] == 'y')
	   write_peak_num(x1,y1,peak_ptr->key,intRound((TICK_LEN-1)*ppmm));*/
	 if (mode_string[SHOW_LABELS] == 'y')
	   write_label(x1,y1,peak_ptr->label,intRound(TICK_LEN*ppmm));
	 }
}

/******************************************************************************
*  WRITE_LABEL(x,y,label,offset) - writes label at location x, y (in
*	pixels) offset to the left and up by "offset" pixels 
******************************************************************************/
static void write_label(int x, int y, char *label, int offset)
{
    int i, charoff;
    char buf[50],tmp_label[LABEL_LEN+1];
        
      strcpy(tmp_label,label);
      i = strlen(tmp_label);
      while ((i > 0) && (tmp_label[i-1] == ' ')) {i--;}
      tmp_label[i] = '\0';
      sprintf(buf,"%s",tmp_label);
      charsize(0.5);
      charoff = strlen(buf)*xcharpixels;
      color(LABEL_COLOR);
      amove(x-offset-charoff,y+offset);
      dstring(buf);
}

/******************************************************************************
*  WRITE_PEAK_NUM(x,y,num,offset) - writes integer "num" at location x, y (in
*	pixels) offset to the right and down by "offset" pixels 
******************************************************************************/
static void write_peak_num(int x, int y, int num, int offset)
{
    char buf[50];
        
      if (getnd() == 3)
	sprintf(buf,"%d-%d",getplaneno(),num);
      else
        sprintf(buf,"%d",num);
      color(NUM_COLOR);
      charsize(0.5);
      amove(x+offset,y-ycharpixels-offset);
      dstring(buf);
}

/******************************************************************************
*  LL2D_DRAW_PEAKS() - draws peaks, numbers, boxes, and labels based on the
*	value of the variable ll2dmode.
******************************************************************************/
void ll2d_draw_peaks()
{
    char mode_string[NUM_LL2DMODE+1];
 
      if (!peak_table_valid(peak_table))
        delete_peak_table(&peak_table);
      if (!peak_table)
        ll2d_init();
      if (peak_table)  {
        if (P_getstring(GLOBAL,"ll2dmode",mode_string,1,NUM_LL2DMODE+1))  {
          if (peak_table->num_peaks > 0)  {
            draw_peaks(peak_table);
            }
	  }
	else  if (strncmp(mode_string,"nnnn",NUM_LL2DMODE) != 0)  {
          if (peak_table->num_peaks > 0)  {
            draw_peaks(peak_table);
            }
	  }
        }
}
 

/******************************************************************************
*  IN_DISPLAY(f1,f2) - checks if point at frequencies f1,f2 is in the
*	currently displayed area
******************************************************************************/
static int in_display(double f1, double f2)
{
    char ch;

    get_display_label(HORIZ,&ch);
    if (ch == peak_table->f1_label)  {
      if ((f1 < sp) || (f1 > sp+wp) || (f2 < sp1) || (f2 > sp1+wp1))
	return(FALSE);
      }
    else  {
      if ((f2 < sp) || (f2 > sp+wp) || (f1 < sp1) || (f1 > sp1+wp1))
	return(FALSE);
      }
    return(TRUE);
}

/*************************/
static int intRound(double x)		/* utility routine */
/*************************/
{
      return((int)(x+0.5));
}

/******************************************************************************
*  LL2D_INIT() - initializes the peak table and reads in the peak file,
*	if it exists.
******************************************************************************/
static void ll2d_init()
{
    char *fname0, fname[MAXPATHL];

    delete_peak_table(&peak_table);
    init_peak_table(&peak_table);
    strcpy(fname,curexpdir);
    strcat(fname,"/ll2d/");
    fname0 = get_filename();
    strcat(fname,fname0);
    release(fname0);
    read_peak_file(&peak_table,fname);
}

/*****************************************************************************
LL2D_MARK(mark_mode)
*	This routine is derived from dconi.c routine dconi_mark.  If in dconi
*	CURSOR mode, adds a peak at the current cursor location to the peak
*	table.  If in dconi BOX mode, adds peak bounds defined by the cursors
*	to any peaks within the area defined by the cursors which don't already
*	have peak bounds associated with them.  If no peaks are within the
*	area defined by the cursors, finds the highest point in that area,
*	marks it as a peak, and assigns the cursor area as that peak's bounds.
*****************************************************************************/
static int ll2d_mark(int mark_mode, int retc)
{
	int		 first_point, first_trace;
	float		*phasfl;
        int              reversed;
	peak_struct	*peak = NULL;
	peak_struct	*draw_peak=NULL;
	double		 h_rflrfp, v_rflrfp, h_axis_scl, v_axis_scl;
	double		 intval, datamax;
	double		 f1_min, f1_max, f2_min, f2_max;
	double		 start, len;
	double		 cr, cr1, delta, delta1;
	double		 points[INTERP_PTS],max;
	float		*trace, *prev, *next;
	double		 f1_pk,amp1_pk,f2_pk,amp2_pk,fraction1,fraction2;
	int		 h_min_pt, h_max_pt, v_min_pt, v_max_pt, i, j;
	int		 max_i, max_j;
	char 		 ch;

extern  int		dconi_mode;  /* from dconi */
extern  int		dconi_reset(), dconi_cursor();


        get_cursor_pars(HORIZ,&cr,&delta);
        get_cursor_pars(VERT,&cr1,&delta1);
	get_rflrfp(HORIZ,&h_rflrfp);
	get_rflrfp(VERT,&v_rflrfp);
	get_display_label(HORIZ,&ch);
	get_scale_pars(HORIZ,&start,&len,&h_axis_scl,&reversed);
	get_scale_pars(VERT,&start,&len,&v_axis_scl,&reversed);


/*  2D box mode.  */

	if (mark_mode == BOX_MODE) {
	     init_peak_table(&peak_table);
             if (peak_table->f2_label == ch)
             {	/* f2 horizontal */
	        f2_max = cr+h_rflrfp;
	        f2_min = cr-delta+h_rflrfp;
	        f1_max = cr1+v_rflrfp;
	        f1_min = cr1-delta1+v_rflrfp;
             }
             else
             {  /* f1 horizontal */
	        f1_max = cr+h_rflrfp;
	        f1_min = cr-delta+h_rflrfp;
	        f2_max = cr1+v_rflrfp;
	        f2_min = cr1-delta1+v_rflrfp;
             }

	    intval = calc_volume(peak_table,f1_min,f1_max,f2_min,f2_max);
	/* find all peaks within the area defined by the cursors and insert
	   current cursor box as peak bounds for peaks without already defined
	   bounds */
	/* search through peaks within area defined by cursors */
	     peak = peak_table->head;
	     while (peak)  {
		if ((peak->f1 > f1_min) && (peak->f1 < f1_max) &&
		    (peak->f2 > f2_min) && (peak->f2 < f2_max))  {
		 if ((peak->f1_min == 0.0)&&(peak->f1_max == 0.0)&&
		    (peak->f2_min == 0.0)&&(peak->f2_max == 0.0))  {
		  insert_peak_bounds(peak_table,peak->f1,
			peak->f2,f1_min,f1_max,f2_min,f2_max,
			(double)0.0,(double)0.0,intval);
		  draw_peak = peak;
                  if (retc == 0)
                  {
		     if (peak_table->f2_label == ch)
		       Winfoprintf("Peak: %d  f%c= %5.3f  f%c= %5.3f  vol= %f",
			peak->key,peak_table->f1_label,
			(peak->f1-v_rflrfp)/v_axis_scl, peak_table->f2_label,
			(peak->f2-h_rflrfp)/h_axis_scl, intval*ins2val);
		     else
		       Winfoprintf("Peak: %d  f%c= %5.3f  f%c= %5.3f  vol= %f",
			peak->key,peak_table->f1_label,
			(peak->f1-h_rflrfp)/h_axis_scl,peak_table->f2_label,
			(peak->f2-v_rflrfp)/v_axis_scl,intval*ins2val);
                   }
		  }
		 }
		peak = peak->next;
		}
	     if (draw_peak != NULL)  {
	       draw_one_box(draw_peak,NORMALMODE);
	       }
	     else  {  /* find highest point within box */
		if (ch == peak_table->f1_label)  {    /* f1 is horizontal */ 
		  h_min_pt = ll2d_frq_to_dp(f1_min,sw,fn);
		  if (f1_min > ll2d_dp_to_frq((double)h_min_pt,sw,fn))
		    h_min_pt--;
		  h_max_pt = ll2d_frq_to_dp(f1_max,sw,fn);
		  if (f1_max < ll2d_dp_to_frq((double)h_max_pt,sw,fn))
		    h_max_pt++;
		  v_min_pt = ll2d_frq_to_dp(f2_min,sw1,fn1);
		  if (f2_min > ll2d_dp_to_frq((double)v_min_pt,sw1,fn1))
		    v_min_pt--;
		  v_max_pt = ll2d_frq_to_dp(f2_max,sw1,fn1);
		  if (f2_max < ll2d_dp_to_frq((double)v_max_pt,sw1,fn1))
		    v_max_pt++;
		  }
		else   {
		  v_min_pt = ll2d_frq_to_dp(f1_min,sw1,fn1);
		  if (f1_min > ll2d_dp_to_frq((double)v_min_pt,sw1,fn1))
		    v_min_pt--;
		  v_max_pt = ll2d_frq_to_dp(f1_max,sw1,fn1);
		  if (f1_max < ll2d_dp_to_frq((double)v_max_pt,sw1,fn1))
		    v_max_pt++;
		  h_min_pt = ll2d_frq_to_dp(f2_min,sw,fn);
		  if (f2_min > ll2d_dp_to_frq((double)h_min_pt,sw,fn))
		    h_min_pt--;
		  h_max_pt = ll2d_frq_to_dp(f2_max,sw,fn);
		  if (f2_max < ll2d_dp_to_frq((double)h_max_pt,sw,fn))
		  h_max_pt++;
		  }
		max = 0.0; max_i = v_min_pt; max_j = h_min_pt;
		for (i=v_max_pt;i<=v_min_pt;i++)  {
		  trace = gettrace(i,0);
		  for (j=h_max_pt;j<=h_min_pt;j++)
		    if (fabs(*(trace + j)) > max)  {
		      max = fabs(*(trace + j));
		      max_i = i;  max_j = j;
		      }
		  }
		prev = gettrace(max_i-1,0);
		trace = gettrace(max_i,0);
		next = gettrace(max_i+1,0);
		points[0] = *(trace+max_j-1);
		points[1] = *(trace+max_j);
		points[2] = *(trace+max_j+1);
		interpolate_cg(points,&fraction1,&amp1_pk);
		points[0] = *(prev+max_j);
		points[1] = *(trace+max_j);
		points[2] = *(next+max_j);
		interpolate_cg(points,&fraction2,&amp2_pk);
		if (ch == peak_table->f1_label)  {  /* f1 is HORIZ */
		  f1_pk = ll2d_dp_to_frq((double)(max_j)+fraction1,sw,fn);
		  f2_pk = ll2d_dp_to_frq((double)(max_i)+fraction2,sw1,fn1);
		  }
		else  {
		  f2_pk = ll2d_dp_to_frq((double)(max_j)+fraction1,sw,fn);
		  f1_pk = ll2d_dp_to_frq((double)(max_i)+fraction2,sw1,fn1);
		  }
		amp1_pk = fabs(amp1_pk) > fabs(amp2_pk) ? amp1_pk : amp2_pk;
		/* make sure peak is still inside bounds after interpolation */
		if (f1_pk <= f1_min)
		  f1_pk = f1_min + BOUNDS_THRESH;
		if (f1_pk >= f1_max)
		  f1_pk = f1_max - BOUNDS_THRESH;
		if (f2_pk <= f2_min)
		  f2_pk = f2_min + BOUNDS_THRESH;
		if (f2_pk >= f2_max)
		  f2_pk = f2_max - BOUNDS_THRESH;
		/* insert peak in peak table */
		if (!peak_in_table(peak_table,f1_pk,f2_pk)){
		  peak = create_peak(f1_pk,f2_pk,(double)(amp1_pk));
		  insert_peak(peak_table,peak);
		  insert_peak_bounds(peak_table,peak->f1,
			peak->f2,f1_min,f1_max,f2_min,f2_max,
			(double)0.0,(double)0.0,intval);
                  if (retc == 0)
                  {
		     if (peak_table->f2_label == ch)
		       Winfoprintf("Peak: %d  f%c= %5.3f  f%c= %5.3f  vol= %f",
			peak->key,peak_table->f1_label,
			(peak->f1-v_rflrfp)/v_axis_scl, peak_table->f2_label,
			(peak->f2-h_rflrfp)/h_axis_scl, intval*ins2val);
		     else
		       Winfoprintf("Peak: %d  f%c= %5.3f  f%c= %5.3f  vol= %f",
			peak->key,peak_table->f1_label,
			(peak->f1-h_rflrfp)/h_axis_scl,peak_table->f2_label,
			(peak->f2-v_rflrfp)/v_axis_scl,intval*ins2val);
                   }
	          draw_one_peak(peak,NORMALMODE);
	          draw_one_box(peak,NORMALMODE);
		  }
		}
	dconi_mode = CURSOR_MODE;
	dconi_cursor();
	}

/*  2D cursor mode.  */

	else {
	      first_point = ll2d_frq_to_dp(cr+h_rflrfp,sw,fn);
	      first_trace = ll2d_frq_to_dp(cr1+v_rflrfp,sw1,fn1);

	      if ((phasfl=gettrace(first_trace,first_point))==0) {
			Wturnoff_buttons();
			return 1;
		}

                datamax = *phasfl;

		init_peak_table(&peak_table);
	/* insert peak at current cursor location into peak table */
             if (peak_table->f2_label == ch)
             {  /* f2 horizontal */
		if (!peak_in_table(peak_table,cr1+v_rflrfp,cr+h_rflrfp))  {
		  peak = create_peak(cr1+v_rflrfp,cr+h_rflrfp,datamax);
		  insert_peak(peak_table,peak);
                  if (retc == 0)
		     Winfoprintf("Peak: %d  f%c= %5.3f  f%c= %5.3f  amp= %f",
			peak->key,peak_table->f1_label,
			(peak->f1-v_rflrfp)/v_axis_scl, peak_table->f2_label,
			(peak->f2-h_rflrfp)/h_axis_scl, datamax);
		  }
             }
             else
             {  /* f1 horizontal */
		if (!peak_in_table(peak_table,cr+h_rflrfp,cr1+v_rflrfp))  {
		  peak = create_peak(cr+h_rflrfp,cr1+v_rflrfp,datamax);
		  insert_peak(peak_table,peak);
                  if (retc == 0)
		     Winfoprintf("Peak: %d  f%c= %5.3f  f%c= %5.3f  amp= %f",
			peak->key,peak_table->f1_label,
			(peak->f1-h_rflrfp)/h_axis_scl, peak_table->f2_label,
			(peak->f2-v_rflrfp)/v_axis_scl, datamax);
		  }
             }
	     if (WgraphicsdisplayValid("dconi")) {
	       dconi_reset();
               if (peak != NULL)
	          draw_one_peak(peak,NORMALMODE);
	       dconi_mode = -1;   /* force dconi CURSOR_MODE */
	       dconi_cursor();
	       }
	}

	return 0;
}

/*****************************************************************************
LL2D_UNMARK(mark_mode, clear_flag)
*	This routine is derived from dconi.c routine dconi_mark.  If in dconi
*	CURSOR mode, deletes the nearest peak from the peak table.  If in
*	dconi BOX mode, deletes the peak bounds around any peak whose bounds
*	are completely within the area defined by the cursors.  If called
*	with clear_flag set, deletes all peaks within the current display
*	region in CURSOR mode or all peaks within the cursor area in BOX mode.
*****************************************************************************/
static int ll2d_unmark(int mark_mode, int clear_flag,
                       int pos_neg_flag, int unmark_peak, int retc)
/*************************/
{
	peak_struct	*peak,*min_dist_peak = NULL;
	double		 h_rflrfp, v_rflrfp;
	double		 f1_min, f1_max, f2_min, f2_max;
	double		 dist, min_dist;
	double		 cr, delta, cr1, delta1;

	char 		 ch;

        get_cursor_pars(HORIZ,&cr,&delta);
        get_cursor_pars(VERT,&cr1,&delta1);
	get_rflrfp(HORIZ,&h_rflrfp);
	get_rflrfp(VERT,&v_rflrfp);
	get_display_label(HORIZ,&ch);

/* clear_flag set */

	if (clear_flag)  {
	  if (mark_mode == BOX_MODE) {   /*  2D box mode.  */
	     init_peak_table(&peak_table);
             if (peak_table->f2_label == ch)
             {  /* f2 horizontal */
	        f2_max = cr + h_rflrfp;
	        f2_min = cr - delta + h_rflrfp;
	        f1_max = cr1 + v_rflrfp;
	        f1_min = cr1 - delta1 + v_rflrfp;
             }
             else
             {  /* f1 horizontal */
	        f1_max = cr + h_rflrfp;
	        f1_min = cr - delta + h_rflrfp;
	        f2_max = cr1 + v_rflrfp;
	        f2_min = cr1 - delta1 + v_rflrfp;
             }
	/* delete peak bounds which are entirely within area defined by
		cursors from peaks in table */
	/* search through peaks within area defined by cursors */
	     peak = peak_table->head;
	     while (peak)  {
		if ((peak->f1 >= f1_min)&&(peak->f1 <= f1_max)&&
		    (peak->f2 >= f2_min)&&(peak->f2 <= f2_max))  {
		  if (pos_neg_flag == POS_NEG)
		    delete_peak(peak_table,peak->f1,peak->f2);
		  else if ((pos_neg_flag == POS_ONLY) &&
			   (peak->amp > 0.0))
		    delete_peak(peak_table,peak->f1,peak->f2);
		  else if ((pos_neg_flag == NEG_ONLY) &&
			   (peak->amp < 0.0))
		    delete_peak(peak_table,peak->f1,peak->f2);
		  }
		peak = peak->next;
		}
	    }
	  else  {   /* 2D cursor mode */
	     init_peak_table(&peak_table);
             if (peak_table->f2_label == ch)
             {  /* f2 horizontal */
	        f2_min = ll2d_dp_to_frq((double)fpnt+npnt,sw,fn);
	        f2_max = ll2d_dp_to_frq((double)fpnt,sw,fn);
	        f1_min = ll2d_dp_to_frq((double)fpnt1+npnt1,sw1,fn1);
	        f1_max = ll2d_dp_to_frq((double)fpnt1,sw1,fn1);
             }
             else
             {  /* f1 horizontal */
	        f1_min = ll2d_dp_to_frq((double)fpnt+npnt,sw,fn);
	        f1_max = ll2d_dp_to_frq((double)fpnt,sw,fn);
	        f2_min = ll2d_dp_to_frq((double)fpnt1+npnt1,sw1,fn1);
	        f2_max = ll2d_dp_to_frq((double)fpnt1,sw1,fn1);
             }
	/* delete all peaks within current display region */
	/* search through peaks within area defined by cursors */
	     peak = peak_table->head;
	     while (peak)  {
		if ((peak->f1 >= f1_min)&&(peak->f1 <= f1_max)&&
		    (peak->f2 >= f2_min)&&(peak->f2 <= f2_max))  {
		  if (pos_neg_flag == POS_NEG)
		    delete_peak(peak_table,peak->f1,peak->f2);
		  else if ((pos_neg_flag == POS_ONLY) &&
			   (peak->amp > 0.0))
		    delete_peak(peak_table,peak->f1,peak->f2);
		  else if ((pos_neg_flag == NEG_ONLY) &&
			   (peak->amp < 0.0))
		    delete_peak(peak_table,peak->f1,peak->f2);
		  }
		peak = peak->next;
		}
	     }
	   }

/*  clear flag not set.  */

	else  if (unmark_peak <= 0)  {
	   if (mark_mode == BOX_MODE) {
             if (peak_table->f2_label == ch)
             {  /* f2 horizontal */
	        f2_max = cr + h_rflrfp;
	        f2_min = cr - delta + h_rflrfp;
	        f1_max = cr1 + v_rflrfp;
	        f1_min = cr1 - delta1 + v_rflrfp;
             }
             else
             {  /* f1 horizontal */
	        f1_max = cr + h_rflrfp;
	        f1_min = cr - delta + h_rflrfp;
	        f2_max = cr1 + v_rflrfp;
	        f2_min = cr1 - delta1 + v_rflrfp;
             }
	/* delete peak bounds which are entirely within area defined by
		cursors from peaks in table */
	/* search through peaks within area defined by cursors */
	     peak = peak_table->head;
	     while (peak)  {
		if ((peak->f1_min >= f1_min)&&(peak->f1_max <= f1_max)&&
		    (peak->f2_min >= f2_min)&&(peak->f2_max <= f2_max))  {
		     peak->f1_min = 0.0;
		     peak->f1_max = 0.0;
		     peak->f2_min = 0.0;
		     peak->f2_max = 0.0;
		     peak->fwhh1 = 0.0;
		     peak->fwhh2 = 0.0;
		     peak->vol = 0.0;
		  }
		peak = peak->next;
		}
	}

/*  2D cursor mode.  */

	else {
	     init_peak_table(&peak_table);
             if (peak_table->f2_label == ch)
             {  /* f2 horizontal */
		f1_min = cr1+v_rflrfp;
		f2_min = cr+h_rflrfp;
             }
             else
             {  /* f1 horizontal */
		f2_min = cr1+v_rflrfp;
		f1_min = cr+h_rflrfp;
             }
	/* delete peak nearest current cursor position from peak table */
	     min_dist = 1.0e+20;
	/* search through entire peak table tree */
	     peak = peak_table->head;
	     while (peak)  {
		dist = sqrt((peak->f1-f1_min)*(peak->f1-f1_min) +
			(peak->f2-f2_min)*(peak->f2-f2_min));
		if (dist < min_dist)  {
		  min_dist = dist;
		  min_dist_peak = peak;
		  }
		peak = peak->next;
		}
	      if (min_dist < 1.0e+20)
		delete_peak(peak_table,min_dist_peak->f1,
					min_dist_peak->f2);
              if (!retc)
                 Winfoprintf("Number of peaks in 2D Spectrum:  %d",peak_table->num_peaks);
	  }
	}
      else  {
	/* search through entire peak table tree */
	peak = peak_table->head;
	while (peak && peak->key != unmark_peak)  {
	  peak = peak->next;
	  }
	if (peak != NULL)
	  delete_peak(peak_table,peak->f1,peak->f2);
	}
      rekey_peaks(peak_table);

      return 0;
}


/*****************************************************************************
LL2D_COMBINE(mark_mode)
*	This routine is derived from dconi.c routine dconi_mark.  If in dconi
*	CURSOR mode, returns without doing anything.  If in dconi BOX mode,
*	deletes all peaks that are within the area defined by the cursors,
*	replacing them with a single peak whose center is the average
*	of the peaks' centers, and whose bounds contain the bounds of all
*	deleted peaks.
*****************************************************************************/
static int ll2d_combine(int mark_mode, int weight, double *list, int numlist,
                        int retc, char *retv[])
/*************************/
{
	peak_struct	*peak, *first_peak = NULL;
	double		 h_rflrfp, v_rflrfp;
	double		 f1_min, f1_max, f2_min, f2_max;
	double		 cr, delta, cr1, delta1;
	double		 min_f1, min_f2, max_f1, max_f2, ave_f1, ave_f2;
        double           sum;
	int		 num=0, done, all_found, i, *tmplist;

	char 		 ch, label[LABEL_LEN+1], comment[COMMENT_LEN+1];

        get_cursor_pars(HORIZ,&cr,&delta);
        get_cursor_pars(VERT,&cr1,&delta1);
	get_rflrfp(HORIZ,&h_rflrfp);
	get_rflrfp(VERT,&v_rflrfp);
	get_display_label(HORIZ,&ch);
	init_peak_table(&peak_table);
//  showPeaks("start of combine");

      if (numlist <= 0)  {
	if (mark_mode != BOX_MODE) {
	  Werrprintf("Must be in BOX mode to do combine.");
	  return(ERROR);
	  }
	else  {   /*  2D box mode.  */
             if (peak_table->f2_label == ch)
             {  /* f2 horizontal */
	        f2_max = cr + h_rflrfp;
	        f2_min = cr - delta + h_rflrfp;
	        f1_max = cr1 + v_rflrfp;
	        f1_min = cr1 - delta1 + v_rflrfp;
             }
             else
             {  /* f1 horizontal */
	        f1_max = cr + h_rflrfp;
	        f1_min = cr - delta + h_rflrfp;
	        f2_max = cr1 + v_rflrfp;
	        f2_min = cr1 - delta1 + v_rflrfp;
             }
	     min_f1 = 1.0e18;
	     min_f2 = 1.0e18;
	     max_f1 = -1.0e18;
	     max_f2 = -1.0e18;
	     ave_f1 = 0.0;
	     ave_f2 = 0.0;
	     num = 0;
             sum = 0.0;
	     done = FALSE;
	/* search through peaks within area defined by cursors */
	     peak = peak_table->head;
	     while (peak)  {
		if ((peak->f1 >= f1_min)&&(peak->f1 <= f1_max)&&
		    (peak->f2 >= f2_min)&&(peak->f2 <= f2_max))  {
		  if (num == 0)  {	/* save 1st peak's label and comment */
		    first_peak = peak;
		    strcpy(label,peak->label);
		    strcpy(comment,peak->comment);
		    }
		  num++;
                  if (weight)
                  {
                     double absAmp = fabs(peak->amp);
                     sum += absAmp;
		     ave_f1 += peak->f1 * absAmp;
		     ave_f2 += peak->f2 * absAmp;
                  }
                  else
                  {
		     ave_f1 += peak->f1;
		     ave_f2 += peak->f2;
                  }
		  if ((peak->f1_min != 0.0) && (peak->f1_min < min_f1))
		    min_f1 = peak->f1_min;
		  if ((peak->f2_min != 0.0) && (peak->f2_min < min_f2))
		    min_f2 = peak->f2_min;
		  if ((peak->f1_max != 0.0) && (peak->f1_max > max_f1))
		    max_f1 = peak->f1_max;
		  if ((peak->f2_max != 0.0) && (peak->f2_max > max_f2))
		    max_f2 = peak->f2_max;
		  if (!done)  {	/* check if all peaks' labels are the same */
		    if (strcmp(label,peak->label) != 0)
		      done = TRUE;
		    }
		  if (num > 1)
		    delete_peak(peak_table,peak->f1,peak->f2);
		  }
		peak = peak->next;
		}
	   if (num > 1)  {
             int doVol;
	     delete_peak(peak_table,first_peak->f1,first_peak->f2);
             if (weight)
             {
	        ave_f1 /= sum;
	        ave_f2 /= sum;
             }
             else
             {
	        ave_f1 /= num;
	        ave_f2 /= num;
             }
	     peak = create_peak(ave_f1,ave_f2,0.0);
             if ( (min_f1 == 1.0e+18) &&
	          (max_f1 == -1.0e+18) &&
	          (min_f2 == 1.0e+18) &&
	          (max_f2 == -1.0e+18) )
                doVol = 0;
             else
                doVol = 1;
	     if (min_f1 == 1.0e+18)
	       min_f1 = 0.0;
	     else if (min_f1 >= ave_f1)
	       min_f1 = ave_f1-BOUNDS_THRESH;
	     if (max_f1 == -1.0e+18)
	       max_f1 = 0.0;
	     else if (max_f1 <= ave_f1)
	       max_f1 = ave_f1+BOUNDS_THRESH;
	     if (min_f2 == 1.0e+18)
	       min_f2 = 0.0;
	     else if (min_f2 >= ave_f2)
	       min_f2 = ave_f2-BOUNDS_THRESH;
	     if (max_f2 == -1.0e+18)
	       max_f2 = 0.0;
	     else if (max_f2 <= ave_f2)
	       max_f2 = ave_f2+BOUNDS_THRESH;
	     peak->f1_min = min_f1;
	     peak->f2_min = min_f2;
	     peak->f1_max = max_f1;
	     peak->f2_max = max_f2;
             if (doVol)
	        peak->vol = calc_volume(peak_table,min_f1,max_f1,min_f2,max_f2);
             else
	        peak->vol = 0.0;
	     if (!done)  {	/* if all peaks' labels were the same, use
					that label and comment for new peak */
	       strcpy(peak->label,label);
	       strcpy(peak->comment,comment);
	       }
	     insert_peak(peak_table,peak);
             rekey_peaks(peak_table);
	     }
	   else if (num == 1)  {
	     peak = first_peak;
	     }
	  }
	}
      else if (numlist > 1)  {
	tmplist = (int *)allocateWithId(sizeof(int)*numlist,"ll2d");
	for (i=0;i<numlist;i++)
	  tmplist[i] = (int)(list[i] + 0.5);
	min_f1 = 1.0e18;
	min_f2 = 1.0e18;
	max_f1 = -1.0e18;
	max_f2 = -1.0e18;
	ave_f1 = 0.0;
	ave_f2 = 0.0;
	num = 0;
        sum = 0.0;
	done = FALSE;
	all_found = FALSE;
	peak = peak_table->head;
	while (peak && !all_found)  {
	  i = 0;
	  while (i < numlist && (tmplist[i] != peak->key))
	    i++;
	  if (i < numlist)  {
	    if (num == 0)  {	/* save 1st peak's label and comment */
	      strcpy(label,peak->label);
	      strcpy(comment,peak->comment);
	      }
	    num++;
	    if (num == numlist)
	      all_found = TRUE;
            if (weight)
            {
               double absAmp = fabs(peak->amp);
               sum += absAmp;
	       ave_f1 += peak->f1 * absAmp;
	       ave_f2 += peak->f2 * absAmp;
            }
            else
            {
	       ave_f1 += peak->f1;
	       ave_f2 += peak->f2;
            }
	    if ((peak->f1_min != 0.0) && (peak->f1_min < min_f1))
	      min_f1 = peak->f1_min;
	    if ((peak->f2_min != 0.0) && (peak->f2_min < min_f2))
	      min_f2 = peak->f2_min;
	    if ((peak->f1_max != 0.0) && (peak->f1_max > max_f1))
	      max_f1 = peak->f1_max;
	    if ((peak->f2_max != 0.0) && (peak->f2_max > max_f2))
	      max_f2 = peak->f2_max;
	    if (!done)  {	/* check if all peaks' labels are the same */
	      if (strcmp(label,peak->label) != 0)
	        done = TRUE;
	      }
	    delete_peak(peak_table,peak->f1,peak->f2);
	    }
	  peak = peak->next;
	  }
	release(tmplist);
	if (num > 0)  {
          if (weight)
          {
	     ave_f1 /= sum;
	     ave_f2 /= sum;
          }
          else
          {
	     ave_f1 /= num;
	     ave_f2 /= num;
          }
	  peak = create_peak(ave_f1,ave_f2,0.0);
	  if (min_f1 == 1.0e+18)
	    min_f1 = 0.0;
	  else if (min_f1 >= ave_f1)
	    min_f1 = ave_f1-BOUNDS_THRESH;
	  if (max_f1 == -1.0e+18)
	    max_f1 = 0.0;
	  else if (max_f1 <= ave_f1)
	    max_f1 = ave_f1+BOUNDS_THRESH;
	  if (min_f2 == 1.0e+18)
	    min_f2 = 0.0;
	  else if (min_f2 >= ave_f2)
	    min_f2 = ave_f2-BOUNDS_THRESH;
	  if (max_f2 == -1.0e+18)
	    max_f2 = 0.0;
	  else if (max_f2 <= ave_f2)
	    max_f2 = ave_f2+BOUNDS_THRESH;
	  peak->f1_min = min_f1;
	  peak->f2_min = min_f2;
	  peak->f1_max = max_f1;
	  peak->f2_max = max_f2;
	  peak->vol = calc_volume(peak_table,min_f1,max_f1,min_f2,max_f2);
	  if (!done)  {	/* if all peaks' labels were the same, use
					that label and comment for new peak */
	    strcpy(peak->label,label);
	    strcpy(peak->comment,comment);
	    }
	  insert_peak(peak_table,peak);
          rekey_peaks(peak_table);
	  }
	}
      else if (numlist == 1)  {
	retv[0] = realString((double) list[0]);
	return(COMPLETE);
	}
      if (num > 0 && retc > 0)
	retv[0] = realString((double)(peak->key));
//  showPeaks("end of combine");
      return(COMPLETE);
}

/*****************************************************************************
LL2D_AUTOCOMBINE
*****************************************************************************/
static void ll2d_autocombine(int weight, double f1box, double f2box)
{
   peak_struct	*peak, *largest;
   double	 f1, f2;
   double	 ave_f1, ave_f2, sum, vol;
   double        maxAmp;
   double        absAmp;
   int		 done;
   int num;

   done = 0;
   while ( !done )
   {
      largest = NULL;
      peak = peak_table->head;
      maxAmp = 0.0;
      while (peak)
      {
         if (peak->key > 0)
         {
            absAmp = fabs(peak->amp);
            if (absAmp > maxAmp)
            {
               maxAmp = absAmp;
               largest = peak;
            }
         }
         peak = peak->next;
      }
      if (largest)
      {
         f1 = largest->f1;
         f2 = largest->f2;
         peak = peak_table->head;
	 ave_f1 = 0.0;
	 ave_f2 = 0.0;
	 num = 0;
         vol = sum = 0.0;
         while (peak)
         {
            if (peak->key > 0)
            {
               if ( ( fabs(f1 - peak->f1) <= f1box/2.0 ) &&
                    ( fabs(f2 - peak->f2) <= f2box/2.0 ) )
               {
		  num++;
                  if (weight)
                  {
                     double absAmp = fabs(peak->amp);
                     sum += absAmp;
		     ave_f1 += peak->f1 * absAmp;
		     ave_f2 += peak->f2 * absAmp;
                  }
                  else
                  {
		     ave_f1 += peak->f1;
		     ave_f2 += peak->f2;
                  }
                  vol += peak->vol;
                  if (peak == largest)
                     peak->key = 0;
                  else
                     peak->key = -1;
                  if (peak->f1_min < largest->f1_min)
                     largest->f1_min = peak->f1_min;
                  if (peak->f1_max < largest->f1_max)
                     largest->f1_max = peak->f1_max;
                  if (peak->f2_min < largest->f2_min)
                     largest->f2_min = peak->f2_min;
                  if (peak->f2_max < largest->f2_max)
                     largest->f2_max = peak->f2_max;
               }
            }
            peak = peak->next;
         }
         if (num)
         {
            if (weight)
            {
	       ave_f1 /= sum;
	       ave_f2 /= sum;
            }
            else
            {
	       ave_f1 /= num;
	       ave_f2 /= num;
            }
            largest->f1 = ave_f1;
            largest->f2 = ave_f2;
            largest->vol = vol;
            if (num > 1)
            {
               largest->amp = 0.0;
               largest->fwhh1 = 0.0;
               largest->fwhh2 = 0.0;
            }
         }
         largest->key = 0;
      }
      else
      {
         done = 1;
      }
   }
   peak = peak_table->head;
   num = 0;
   while (peak)
   {
      if (peak->key < 0)
      {
         delete_peak_from_table(peak_table,peak->f1,peak->f2);
         num++;
      }
      peak = peak->next;
   }
   rekey_peaks(peak_table);
   if (num)
   {
      write_peak_file_header(peak_table);
      peak = peak_table->head;
      while (peak)
      {
         write_peak_file_record(peak_table,peak,peak->key);
         peak = peak->next;
      }
   }
}

/*****************************************************************************
LL2D_INFO(mark_mode)
*	Print info about peak nearest to the current cursor position.
*****************************************************************************/
static int ll2d_info(int mark_mode, int num, int retc, char *retv[])
/*************************/
{
	int		 reversed;
	peak_struct	*peak,*min_dist_peak = NULL;
        double		 f1, f1_min, f1_max, f2, f2_min, f2_max;
	double		 dist, min_dist, min_area;
	double		 f1_rflrfp, f2_rflrfp, start, len, f1_axis_scl,
			 f2_axis_scl, tmp;
	double		 cr, cr1, delta, delta1;
	char 		 ch;
        char		 line1[133], line2[133], line3[133];
        char		 format1[133], format2[133], format3[133];
	char		 *ptr, label[LABEL_LEN+1];

	get_rflrfp(HORIZ,&f1_rflrfp);
	get_rflrfp(VERT,&f2_rflrfp);
        get_cursor_pars(HORIZ,&cr,&delta);
        get_cursor_pars(VERT,&cr1,&delta1);
	get_scale_pars(HORIZ,&start,&len,&f1_axis_scl,&reversed);
	get_scale_pars(VERT,&start,&len,&f2_axis_scl,&reversed);
	get_display_label(HORIZ,&ch);

	if (ch == peak_table->f2_label)  {
	  tmp = f1_rflrfp;
	  f1_rflrfp = f2_rflrfp;
	  f2_rflrfp = tmp;
	  tmp = f1_axis_scl;
	  f1_axis_scl = f2_axis_scl;
	  f2_axis_scl = tmp;
	  }

	init_peak_table(&peak_table);

	if (ch == peak_table->f2_label)  {
	  f1_min = cr1+f1_rflrfp;
	  f2_min = cr+f2_rflrfp;
	  }
	else  {
	  f2_min = cr1+f2_rflrfp;
	  f1_min = cr+f1_rflrfp;
	  }

	if (num < 1)
        {
	/* get info about peak nearest current cursor position */
	     min_dist = 1.0e+20;
             min_area = 0.0;
	/* search through entire peak table tree */
	     peak = peak_table->head;
	   while (peak)
           {
              if ((peak->f1_min == 0.0) && (peak->f1_max == 0.0) &&
	          (peak->f2_min == 0.0) && (peak->f2_max == 0.0) &&
                   (min_area <= 0.0))
              {
		dist = sqrt((peak->f1-f1_min)*(peak->f1-f1_min) +
			(peak->f2-f2_min)*(peak->f2-f2_min));
		if (dist < min_dist)  {
		  min_dist = dist;
		  min_dist_peak = peak;
		  }
              }
              else
              {
                 if ((peak->f1_min <= f1_min) && (peak->f1_max >= f1_min) &&
                  (peak->f2_min <= f2_min) && (peak->f2_max >= f2_min) )
                 {
                    double area;
                    area = (peak->f1_max - peak->f1_min) *
                           (peak->f2_max - peak->f2_min);
                    if ( (min_area <= 0.0) || (area < min_area) )
                    {
                       min_area = area;
                       min_dist = 1.0;
                       min_dist_peak = peak;
                    }
                 }
              }
	      peak = peak->next;
           }
	   if (min_dist < 1.0e+20)
	      peak = min_dist_peak;
	   else
	      peak = NULL;
	}
	else  {  /* get info about specific peak */
	     peak = peak_table->head;
	     while (peak && peak->key != num)  {
		peak = peak->next;
		}
	  }
	/* at this point peak == NULL if peak wasn't found */

	if (peak != NULL)  {
	      f1 = (peak->f1 - f1_rflrfp)/f1_axis_scl;
	      f2 = (peak->f2 - f2_rflrfp)/f2_axis_scl;
		/* check if peak bounds have been defined, then correct for
		   reference frequency, otherwise write zeros */
	      if ((peak->f1_min != 0.0) || (peak->f1_max != 0.0) ||
		    (peak->f2_min != 0.0) || (peak->f2_max != 0.0))  {
		  f1_min = (peak->f1_min - f1_rflrfp)/f1_axis_scl;
		  f1_max = (peak->f1_max - f1_rflrfp)/f1_axis_scl;
		  f2_min = (peak->f2_min - f2_rflrfp)/f2_axis_scl;
		  f2_max = (peak->f2_max - f2_rflrfp)/f2_axis_scl;
		  }
	      else  {
		  f1_min = 0.0;
		  f1_max = 0.0;
		  f2_min = 0.0;
		  f2_max = 0.0;
		  }
	      if (retc < 1) {  /* print info to text window */
		get_formats(line1,line2,line3,format1,format2,format3,
				INFO_FORMAT);
		Wscrprintf(line1);
		Wscrprintf(line2);
		Wscrprintf(line3);
		Wscrprintf(format1,peak->label,peak->comment,peak->key,f1,f2,
			peak->amp,peak->vol*ins2val);
		Wscrprintf(format2,peak->fwhh1/f1_axis_scl,
			peak->fwhh2/f2_axis_scl);
		Wscrprintf(format3,f1_min,f1_max,f2_min,f2_max);
		}
	      else  {
		retv[0] = realString((double)peak->key);
		if (retc > 1)
		  retv[1] = realString(f1);
		if (retc > 2)
		  retv[2] = realString(f2);
		if (retc > 3)
		  retv[3] = realString(peak->amp);
		if (retc > 4)
		  retv[4] = realString(peak->vol);
		if (retc > 5)  {
		  strcpy(label,peak->label);
		  if (strcmp(peak->label,EMPTY_LABEL))  {
		    ptr = label + strlen(label) - 1;
		    while (*ptr == ' ')
		      ptr--;
		    *(ptr+1) = '\0';
		    }
		  else
		    label[0] = '\0';
		  retv[5] = newString(label);
		  }
		if (retc > 6)
		  retv[6] = newString(peak->comment);
		}
		if (retc > 7)
		  retv[7] = realString(peak->fwhh1/f1_axis_scl);
		if (retc > 8)
		  retv[8] = realString(peak->fwhh2/f2_axis_scl);
		if (retc > 9)
		  retv[9] = realString(f1_min);
		if (retc > 10)
		  retv[10] = realString(f1_max);
		if (retc > 11)
		  retv[11] = realString(f2_min);
		if (retc > 12)
		  retv[12] = realString(f2_max);
	  }

	return 0;
}

#define LINE_LEN 132
/*****************************************************************************
LL2D_LABEL(mark_mode,name)
*	This routine is derived from dconi.c routine dconi_mark.  If in dconi
*	CURSOR mode, labels the nearest peak in the peak table.  If in
*	dconi BOX mode, labels all peaks that are within the area defined by
*	the cursors.
*****************************************************************************/
static int ll2d_label(int mark_mode, char *name, int num)
/*************************/
{
	peak_struct	*peak,*min_dist_peak = NULL;
	double		 h_rflrfp, v_rflrfp;
	double		 f1_min, f1_max, f2_min, f2_max;
	double		 cr, cr1, delta, delta1;
	double		 dist, min_dist;
	char		 ch;
	char 		 label[LINE_LEN+1];

extern  int		dconi_reset();


	get_rflrfp(HORIZ,&h_rflrfp);
	get_rflrfp(VERT,&v_rflrfp);
        get_cursor_pars(HORIZ,&cr,&delta);
        get_cursor_pars(VERT,&cr1,&delta1);
	get_display_label(HORIZ,&ch);

	if (name == NULL)  {
          W_getInput("Enter label (up to 15 characters)? ",label,LINE_LEN+1);
	  if (label[0] == '\0')   return(1);
	  }
	else  {
	  strncpy(label,name,LABEL_LEN);
	  }
	pad_label(label);

	init_peak_table(&peak_table);

      if (num <= 0)  {
	if (mark_mode == BOX_MODE) {   /*  2D box mode.  */
             if (ch == peak_table->f2_label)
             {  /* f2 horizontal */
	        f2_max = cr + h_rflrfp;
	        f2_min = cr - delta + h_rflrfp;
	        f1_max = cr1 + v_rflrfp;
	        f1_min = cr1 - delta1 + v_rflrfp;
             }
             else
             {  /* f1 horizontal */
	        f1_max = cr + h_rflrfp;
	        f1_min = cr - delta + h_rflrfp;
	        f2_max = cr1 + v_rflrfp;
	        f2_min = cr1 - delta1 + v_rflrfp;
             }
	/* label all peaks which are entirely within area defined by
		cursors from peaks in table */
	/* search through peaks within area defined by cursors */
	     peak = peak_table->head;
	     dconi_reset();
	     while (peak)  {
		if ((peak->f1 >= f1_min) && (peak->f1 <= f1_max) &&
		    (peak->f2 >= f2_min) && (peak->f2 <= f2_max))  {
		  if (strcmp(peak->label,EMPTY_LABEL))
		    write_one_label(peak,XORMODE);
		  strcpy(peak->label,label);
		  write_one_label(peak,NORMALMODE);
                  write_peak_file_record(peak_table,peak,peak->key);
		  }
		peak = peak->next;
		}
	}

/*  2D cursor mode.  */

	else {
             if (ch == peak_table->f2_label)
             {  /* f2 horizontal */
		f1_min = cr1 + v_rflrfp;
		f2_min = cr + h_rflrfp;
             }
             else
             {  /* f1 horizontal */
		f2_min = cr1 + v_rflrfp;
		f1_min = cr + h_rflrfp;
             }
	/* label peak nearest current cursor position */
	     min_dist = 1.0e+20;
	/* search through entire peak table tree */
	     peak = peak_table->head;
	     while (peak)  {
		dist = sqrt((peak->f1-f1_min)*(peak->f1-f1_min) +
			(peak->f2-f2_min)*(peak->f2-f2_min));
		if (dist < min_dist)  {
		  min_dist = dist;
		  min_dist_peak = peak;
		  }
		peak = peak->next;
		}
	      if (min_dist < 1.0e+20)  {
	        dconi_reset();
		if (strcmp(min_dist_peak->label,EMPTY_LABEL))
		  write_one_label(min_dist_peak,XORMODE);
		strcpy(min_dist_peak->label,label);
		write_one_label(min_dist_peak,NORMALMODE);
                write_peak_file_record(peak_table,min_dist_peak,
				min_dist_peak->key);
		}
	  }
	  }
	else  {
	  peak = peak_table->head;
	  while (peak && peak->key != num)  {
	    peak = peak->next;
	    }
	  if (peak != NULL)  {
	    if (WgraphicsdisplayValid("dconi"))  {
	      if (strcmp(peak->label,EMPTY_LABEL))
		write_one_label(peak,XORMODE);
	      }
	    strcpy(peak->label,label);
	    if (WgraphicsdisplayValid("dconi"))  {
	      write_one_label(peak,NORMALMODE);
	      }
	    write_peak_file_record(peak_table,peak,peak->key);
	    }
	  }

	return 0;
}

/*****************************************************************************
LL2D_COMMENT(mark_mode,name)
*	This routine is derived from dconi.c routine dconi_mark.  If in dconi
*	CURSOR mode, adds comment to the peak table for the nearest peak. 
*	If in dconi BOX mode, adds comment to all peaks in the area defined
*	by the cursors.
*****************************************************************************/
static int ll2d_comment(int mark_mode, char *name, int num)
/*************************/
{
	peak_struct	*peak,*min_dist_peak = NULL;
	double		 h_rflrfp, v_rflrfp;
	double		 f1_min, f1_max, f2_min, f2_max;
	double		 cr, cr1, delta, delta1;
	double		 dist, min_dist;
	char		 ch;
	char 		 comment[LINE_LEN+1];

extern  int		dconi_reset();


	get_rflrfp(HORIZ,&h_rflrfp);
	get_rflrfp(VERT,&v_rflrfp);
        get_cursor_pars(HORIZ,&cr,&delta);
        get_cursor_pars(VERT,&cr1,&delta1);
	get_display_label(HORIZ,&ch);

	if (name == NULL)  {
          W_getInput("Enter comment (up to 80 characters)? ",comment,LINE_LEN+1);
	  if (comment[0] == '\0')   return(1);
	  }
	else  {
	  strncpy(comment,name,COMMENT_LEN);
	  }
	comment[COMMENT_LEN] = '\0';

	init_peak_table(&peak_table);

      if (num <= 0)  {
	if (mark_mode == BOX_MODE) {   /*  2D box mode.  */
             if (ch == peak_table->f2_label)
             {  /* f2 horizontal */
	        f2_max = cr + h_rflrfp;
	        f2_min = cr - delta + h_rflrfp;
	        f1_max = cr1 + v_rflrfp;
	        f1_min = cr1 - delta1 + v_rflrfp;
             }
             else
             {  /* f1 horizontal */
	        f1_max = cr + h_rflrfp;
	        f1_min = cr - delta + h_rflrfp;
	        f2_max = cr1 + v_rflrfp;
	        f2_min = cr1 - delta1 + v_rflrfp;
             }
	/* comment all peaks which are entirely within area defined by
		cursors from peaks in table */
	/* search through peaks within area defined by cursors */
	     peak = peak_table->head;
	     dconi_reset();
	     while (peak)  {
		if ((peak->f1 >= f1_min) && (peak->f1 <= f1_max) &&
		    (peak->f2 >= f2_min) && (peak->f2 <= f2_max))  {
		  strcpy(peak->comment,comment);
                  write_peak_file_record(peak_table,peak,peak->key);
		  }
		peak = peak->next;
		}
	}

/*  2D cursor mode.  */

	else {
             if (ch == peak_table->f2_label)
             {  /* f2 horizontal */
		f1_min = cr1 + v_rflrfp;
		f2_min = cr + h_rflrfp;
             }
             else
             {  /* f1 horizontal */
		f2_min = cr1 + v_rflrfp;
		f1_min = cr + h_rflrfp;
             }
	/* comment peak nearest current cursor position */
	     min_dist = 1.0e+20;
	/* search through entire peak table tree */
	     peak = peak_table->head;
	     while (peak)  {
		dist = sqrt((peak->f1-f1_min)*(peak->f1-f1_min) +
			(peak->f2-f2_min)*(peak->f2-f2_min));
		if (dist < min_dist)  {
		  min_dist = dist;
		  min_dist_peak = peak;
		  }
		peak = peak->next;
		}
	      if (min_dist < 1.0e+20)  {
	        dconi_reset();
		strcpy(min_dist_peak->comment,comment);
                write_peak_file_record(peak_table,min_dist_peak,
			min_dist_peak->key);
		}
	}
	}
      else  {
	peak = peak_table->head;
	while (peak && peak->key != num)  {
	  peak = peak->next;
	  }
	if (peak != NULL)  {
	  strcpy(peak->comment,comment);
	  write_peak_file_record(peak_table,peak,peak->key);
	  }
	}

      return 0;
}

/*****************************************************************************
******************************************************************************
Following this are the routines used to create and maintain the peak table.
	The peak table is implemented as a linked list.
******************************************************************************
*****************************************************************************/

#define FRQ_ACC 5.0e-3

/*****************************************************************************
*   FRQ_EQ - check if frequencies "a" and "b" are equal to the level of
*	FRQ_ACC.
*****************************************************************************/
static int frq_eq(double a, double b)
{
   if (fabs(a-b) < FRQ_ACC)
     return(TRUE);
   else
     return(FALSE);
}


/*****************************************************************************
*   CHECK_AXIS_LABELS - check to be sure axis labels in peak table are
*	consistent with those in current display.
*****************************************************************************/
static int check_axis_labels(peak_table_struct *peak_table)
{
    char ch1, ch2;

    get_display_label(HORIZ,&ch1);
    get_display_label(VERT,&ch2);
    if (((peak_table->f1_label == ch1) && (peak_table->f2_label == ch2)) ||
        ((peak_table->f1_label == ch2) && (peak_table->f2_label == ch1)))
      return 0;
    else  {
      Werrprintf("Peak table axis labels (f%c, f%c) not consistent with display axis labels (f%c,f%c)",peak_table->f1_label,peak_table->f2_label,ch1,ch2);
      return 1;
      }
}

/*****************************************************************************
*   INIT_PEAK_TABLE - initialize peak table
*****************************************************************************/
static void init_peak_table(peak_table_struct **peak_table)
{
    char ch1, ch2;

    if (*peak_table == NULL)  {
      *peak_table = create_peak_table();
      }
    get_display_label(HORIZ,&ch1);
    get_display_label(VERT,&ch2);
    if ((((*peak_table)->f1_label == '0') || ((*peak_table)->f2_label == '0')) ||
	 ((*peak_table)->num_peaks == 0))
    {
      if (ch1 < ch2)  {
	(*peak_table)->f1_label = ch1;
	(*peak_table)->f2_label = ch2;
	}
      else  {
	(*peak_table)->f1_label = ch2;
	(*peak_table)->f2_label = ch1;
	}
     }
}

/*****************************************************************************
*  CREATE_PEAK_TABLE - allocates a new peak table structure and initializes
*	its variables.
*****************************************************************************/
static peak_table_struct *create_peak_table()
{
    int i;
    peak_table_struct *peak_table;

    if ((peak_table =
		(peak_table_struct *)allocateWithId(sizeof(peak_table_struct),
			"peak_table")) == 0) {
      Werrprintf("create_peak_table: cannot allocate peak table");
      }
    peak_table->f1_label = '0';
    peak_table->f2_label = '0';
    peak_table->num_peaks = 0;
    peak_table->head = NULL;
    peak_table->file = NULL;
    peak_table->header[0] = 0;
    peak_table->version = CURRENT_VERSION;
    peak_table->experiment = expdir_to_expnum(curexpdir);
    if (getnd() == 3)
      peak_table->planeno = getplaneno();
    else
      peak_table->planeno = -1;
    for (i=1;i<PEAK_FILE_MAX+1;i++)
      peak_table->header[i] = PEAK_FILE_EMPTY;
    return peak_table;
}


/*****************************************************************************
*  DELETE_PEAK_TABLE - deletes all peak structures and peak table symbol tree.
*****************************************************************************/
void delete_peak_table(peak_table_struct **peak_table)
{
    peak_table_struct *pk_tab;
    peak_struct *pk_ptr, *tmp_ptr;

    pk_tab = *peak_table;
    if (pk_tab) {
      if (pk_tab->file)
        fclose(pk_tab->file);
      if (pk_tab->head)  {
        pk_ptr = pk_tab->head;
        while (pk_ptr)  {
	  tmp_ptr = pk_ptr;
	  pk_ptr = pk_ptr->next;
          release(tmp_ptr);
          }
        }
      release(pk_tab);
      }
    *peak_table = NULL;
}

/*****************************************************************************
*  CREATE_PEAK - allocate space for a peak structure and initialize it with
*	the values of f1 and f2 passed to the routine.  Return pointer to
*	newly allocated peak structure.
*****************************************************************************/
peak_struct *create_peak(double f1, double f2, double amp)
{
    peak_struct *peak;
    
    if ((peak = (peak_struct *)allocateWithId(sizeof(peak_struct),
				"peak_table")) == 0) {
      Werrprintf("create_peak: cannot allocate peak");
      }
    peak->label[0] = '\0';
    strcpy(peak->label,EMPTY_LABEL);
    peak->comment[0] = '\0';
    peak->f1 = f1;
    peak->f2 = f2;
    peak->amp = amp;
    peak->f1_min = 0.0;
    peak->f1_max = 0.0;
    peak->f2_min = 0.0;
    peak->f2_max = 0.0;
    peak->fwhh1 = 0.0;
    peak->fwhh2 = 0.0;
    peak->vol = 0.0;
    peak->key = 0;
    peak->next = NULL;
    return peak;
}


/*****************************************************************************
*  INSERT_PEAK_IN_TABLE - insert peak "peak" into the in-memory peak table.
*****************************************************************************/
static int insert_peak_in_table(peak_table_struct *peak_table, peak_struct *peak)
{
  int done;
  peak_struct *pk_ptr;
    
  pk_ptr = peak_table->head;
  if (pk_ptr == NULL)  {              /* table empty */
        peak_table->head = peak;
        peak->next = NULL;
    }
  else if (peak->f1 < pk_ptr->f1) { /* peak should go first in list */
        peak->next = pk_ptr;
        peak_table->head = peak;
    }
  else {
    done = 0;
    while (!done)  {
        /* if peak belongs at end of list, insert it there */
        if (pk_ptr->next == NULL)  {
          pk_ptr->next = peak;
          peak->next = NULL;
          done = 1;
          }
        /* if peak belongs at this point in list, insert it */
        else if (peak->f1 < pk_ptr->next->f1) {
          peak->next = pk_ptr->next;
          pk_ptr->next = peak;
          done = 1;
          }
        else  {
          pk_ptr = pk_ptr->next;
          }
        }
      }

  peak_table->num_peaks++;
  if (peak->key == 0)
      peak->key = peak_table->num_peaks;
  peak_table->header[0]++;
  return(COMPLETE);
}

static void rekey_peaks(peak_table_struct *peak_table)
{
   peak_struct *pk_ptr;
   int key;
   key = peak_table->num_peaks;
   pk_ptr = peak_table->head;
   while (pk_ptr)  {
      pk_ptr->key = key;
      key -= 1;
      pk_ptr = pk_ptr->next;
   }
}

/*****************************************************************************
*  DELETE_PEAK_FROM_TABLE - delete peak "peak" from the in-memory peak table.
*****************************************************************************/
static void delete_peak_from_table(peak_table_struct *peak_table, double f1, double f2)
{
  peak_struct *pk_ptr, *last_ptr;
  int done;
    
  pk_ptr = peak_table->head;
  last_ptr = pk_ptr;
  done = FALSE;
  while (pk_ptr != NULL && !done)  {
    if (frq_eq(pk_ptr->f1, f1))
      if (frq_eq(pk_ptr->f2, f2))  {
	if (pk_ptr != last_ptr)  {
	  last_ptr->next = pk_ptr->next;
	  }
	else  {	  /* first peak in table */
	  peak_table->head = pk_ptr->next;
	  }
	release(pk_ptr);
        peak_table->num_peaks--;
        peak_table->header[0]--;
        done = TRUE;
	}
    last_ptr = pk_ptr;
    pk_ptr = last_ptr->next;
    }
}

/*****************************************************************************
*  INSERT_PEAK - insert peak into the in-memory peak table
*       and the peak file.
*****************************************************************************/
static int insert_peak(peak_table_struct *peak_table, peak_struct *peak)
{
    int res;

    res = insert_peak_in_table(peak_table,peak);
    if (!res)
    {
      insert_peak_in_file(peak_table,peak);
    }
    return(res);
}

/*****************************************************************************
*  DELETE_PEAK - delete peak from the in-memory peak table
*       and the peak file.
*****************************************************************************/
static void delete_peak(peak_table_struct *peak_table, double f1, double f2)
{
      delete_peak_from_table(peak_table,f1,f2);
      write_peak_file_header(peak_table);
//  showPeaks("from delete_peak");
    
}

/*****************************************************************************
*  INSERT_PEAK_BOUNDS - insert peak bounds into the in-memory peak table and
*	peak file.  The peak must already be in the table.
*****************************************************************************/
static void insert_peak_bounds(peak_table_struct *peak_table,
       double f1, double f2, double x1, double x2, double y1, double y2,
       double fwhh1, double fwhh2, double vol)
{
  peak_struct *pk_ptr;
  int done;
    
  pk_ptr = peak_table->head;
  done = FALSE;
  while (pk_ptr != NULL && !done)  {
    if (frq_eq(pk_ptr->f1, f1))
      if (frq_eq(pk_ptr->f2, f2))  {
        pk_ptr->f1_min = x1;
        pk_ptr->f1_max = x2;
        pk_ptr->f2_min = y1;
        pk_ptr->f2_max = y2;
        pk_ptr->fwhh1 = fwhh1;
        pk_ptr->fwhh2 = fwhh2;
        pk_ptr->vol = vol;
        write_peak_file_record(peak_table,pk_ptr,pk_ptr->key);
	done = TRUE;
        }
    pk_ptr = pk_ptr->next;
    }
  if (!done)  {
    Werrprintf("insert_peak_bounds: peak not in table");
    }
}

/*****************************************************************************
*  PEAK_IN_TABLE - returns 1 if peak at (peak->f1, peak->f2) is in in-memory
*	peak table, 0 otherwise.
*****************************************************************************/
static int peak_in_table(peak_table_struct *peak_table, double f1, double f2)
{
  peak_struct *pk_ptr;
    
  pk_ptr = peak_table->head;
  while (pk_ptr)  {
    if (frq_eq(pk_ptr->f1, f1))
      if (frq_eq(pk_ptr->f2, f2))
      {
	return(1);
     }
    pk_ptr = pk_ptr->next;
    }
  return(0);
}

#define MAX_FILENAME_SIZE	40
/*****************************************************************************
*  GET_FILENAME - return a pointer to a filename for ll2d output file.
*	If 2-D data, file is "peaks.bin".  If 3-D data, creates filename
*	"peaks_f#f#_#.bin" where f#f# gives the plane direction (e.g. f1f3),
*	and the	last # gives the number of the current plane.
*****************************************************************************/
static char *get_filename()
{
  int planeno;
  char *name, str[MAX_FILENAME_SIZE], h_ch, v_ch;

  if ((name = (char *)allocateWithId(MAX_FILENAME_SIZE*sizeof(char),"peak_table")) == 0)  {
      Werrprintf("get_filename: cannot allocate name");
      }
  if (getnd() == 2)
    strcpy(name,"peaks.bin");
  else if (getnd() == 3)  {
    strcpy(name,"peaks_");
    get_display_label(HORIZ,&h_ch);
    get_display_label(VERT,&v_ch);
    planeno = getplaneno();
    if (((h_ch == '1') && (v_ch == '3')) ||
	((h_ch == '3') && (v_ch == '1')))  {
      sprintf(str,"f1f3_%1d.bin",planeno);
      strcat(name,str);
      }
    else if (((h_ch == '2') && (v_ch == '3')) ||
	((h_ch == '3') && (v_ch == '2')))  {
      sprintf(str,"f2f3_%1d.bin",planeno);
      strcat(name,str);
      }
    else if (((h_ch == '1') && (v_ch == '2')) ||
	((h_ch == '2') && (v_ch == '1')))  {
      sprintf(str,"f1f2_%1d.bin",planeno);
      strcat(name,str);
      }
    else  {
      Werrprintf("get_filename:  Error in display labels - f%cf%c",h_ch,v_ch);
      }
    }
  return name;
}

/*****************************************************************************
*  PAD_LABEL - pad peak label out to 15 characters
*****************************************************************************/
static void pad_label(char *label)
{
    int i, found;

    found = FALSE;
    for (i=0;i<LABEL_LEN;i++)  {
      if (label[i] == '\0')
	found = TRUE;
      if ((label[i] <= ' ') || found)
	label[i] = ' ';
      }
    label[LABEL_LEN] = '\0';
}

/*****************************************************************************
*  CREATE_PEAK_FILE - initializes peak file header, and writes the header to
*	the file "filename".
*****************************************************************************/
static int create_peak_file(peak_table_struct *peak_table, char *filename)
{
    int i, ival;
    char dirname[MAXPATHL];
    char cmd_str[MAXPATHL];
    struct stat unix_fab;

    strcpy(dirname,curexpdir);
    strcat(dirname,"/ll2d");
    ival = stat(dirname, &unix_fab);
    if (ival || !S_ISDIR(unix_fab.st_mode))  {
      sprintf(cmd_str,"mkdir('%s')\n",dirname);
      execString(cmd_str);
      }
    ival = stat(dirname, &unix_fab);
    if (ival || !S_ISDIR(unix_fab.st_mode))  {
      return(ERROR);
      }
    
    strcat(dirname,"/");
    strcat(dirname,filename);
    if ((peak_table->file = fopen(dirname,"w")) == NULL) {
      Werrprintf("create_peak_file : Error - cannot create file %s",dirname);
      return(ERROR);
      }
    peak_table->header[0] = 0;	/* zero peaks in files */
    for (i=1;i<PEAK_FILE_MAX+1;i++)
      peak_table->header[i] = PEAK_FILE_EMPTY;
    write_peak_file_header(peak_table);
    return(COMPLETE);
}

/*****************************************************************************
*  CALC_HEADER_SIZE - return size of peak file header
*****************************************************************************/
static int calc_header_size(peak_table_struct *peak_table)
{
    int size;

    size = sizeof(float)+sizeof(char)*2+sizeof(int)*(PEAK_FILE_MAX+1);
    if (peak_table->version > 1.0)
      size += sizeof(double)*4;
    return(size);
}

/*****************************************************************************
*  WRITE_PEAK_FILE_HEADER - writes peak header structure to beginning of file
*	peak_table->file which must be open prior to calling this routine.
*****************************************************************************/
static void write_peak_file_header(peak_table_struct *peak_table)
{
    char ch[2], ch1;
    double f1_freq, f2_freq, f1_rflrfp, f2_rflrfp, info[4], tmp;

    fseek(peak_table->file,0L,SEEK_END);	/* go to beginning of file */
    if (fwrite(&(peak_table->version),sizeof(float),1,peak_table->file) != 1) {
      Werrprintf("write_peak_file_header: cannot write peak file header");
      }
    ch[0] = peak_table->f1_label;
    ch[1] = peak_table->f2_label;
    if (fwrite(ch,sizeof(char),2,peak_table->file) != 2) {
      Werrprintf("write_peak_file_header: cannot write peak file header");
      }
    if (peak_table->version > 1.0)  {
      f1_freq = get_axis_freq(HORIZ);
      f2_freq = get_axis_freq(VERT);
      get_rflrfp(HORIZ,&f1_rflrfp);
      get_rflrfp(VERT,&f2_rflrfp);
      get_display_label(VERT,&ch1);
      if (ch1 == peak_table->f1_label)  {
        tmp = f1_rflrfp;
        f1_rflrfp = f2_rflrfp;
        f2_rflrfp = tmp;
        tmp = f1_freq;
        f1_freq = f2_freq;
        f2_freq = tmp;
        }
      info[0] = f1_freq;
      info[1] = f2_freq;
      info[2] = f1_rflrfp;
      info[3] = f2_rflrfp;
      if (fwrite(info,sizeof(double),4,peak_table->file) != 4) {
        Werrprintf("write_peak_file_header: cannot write peak file header");
        }
      }
    if (fwrite(peak_table->header,sizeof(short),PEAK_FILE_MAX+1,
		peak_table->file) != PEAK_FILE_MAX+1) {
      Werrprintf("write_peak_file_header: cannot write peak file header");
      }
}


/*****************************************************************************
*  WRITE_PEAK_FILE_RECORD - writes a peak structure into the peak file at
*	a position "record" records after the peak file header or at the end
*	of the file if the requested record is not already in the file.
*****************************************************************************/
void write_peak_file_record(peak_table_struct *peak_table,
                            peak_struct *peak, int record)
{
    if (fseek(peak_table->file,calc_header_size(peak_table)+ 
			(record-1)*sizeof(peak_struct),SEEK_SET))  {
					/* if file not big enough, */
      fseek(peak_table->file,0L,SEEK_END);	/* go to end of file */
      }
    /* write peak record into file */
    if (fwrite(peak,sizeof(peak_struct),1,peak_table->file) != 1)  {
      Werrprintf("write_peak_file_record: cannot write peak into peak file");
      }
}

/*****************************************************************************
*  INSERT_PEAK_IN_FILE - writes peak "peak" into presently open peak file.
*	updates peak file header, too.
*****************************************************************************/
static void insert_peak_in_file(peak_table_struct *peak_table,
            peak_struct *peak)
{
    write_peak_file_header(peak_table);
    write_peak_file_record(peak_table,peak,peak->key);
}

/*****************************************************************************
*  READ_PEAK_FILE_HEADER - read peak header from file.  Assume file is already
*	open.
*****************************************************************************/
static int read_peak_file_header(peak_table_struct *peak_table)
{
    char ch[2];
    double info[4];

    fseek(peak_table->file,0L,SEEK_SET);	/* go to beginning of file */
    if (fread(&(peak_table->version),sizeof(float),1,peak_table->file) != 1)  {
      Werrprintf("read_peak_file_header: cannot read peak file header");
      }
    if (fread(ch,sizeof(char),2,peak_table->file) != 2)  {
      Werrprintf("read_peak_file_header: cannot read peak file header");
      }
    peak_table->f1_label = ch[0];
    peak_table->f2_label = ch[1];
    if (peak_table->version > 1.0)  {
      if (fread(info,sizeof(double),4,peak_table->file) != 4)  {
        Werrprintf("read_peak_file_header: cannot read peak file header");
        }  /* ignore this info about axis freqs and rflrfps, since the freq.
		values of the peaks are in Hertz and unreferenced */
      }
    if (fread(peak_table->header,sizeof(short),PEAK_FILE_MAX+1,peak_table->file)
		!= PEAK_FILE_MAX+1)  {
      Werrprintf("read_peak_file_header: cannot read peak file header");
      return(ERROR);
      }
    return(COMPLETE);
}


/*****************************************************************************
*  READ_PEAK_FILE_RECORD - read a particular peak record from file.
*	Assume file is already open.
*****************************************************************************/
static peak_struct *read_peak_file_record(peak_table_struct *peak_table,
                    int record)
{
    peak_struct *peak;


    if ((peak = (peak_struct *)allocateWithId(sizeof(peak_struct),
			"peak_table")) == 0) {
      Werrprintf("read_peak_file_record: cannot allocate peak");
      }
    if (fseek(peak_table->file,calc_header_size(peak_table)+
			(record-1)*sizeof(peak_struct),SEEK_SET))  {
      Werrprintf("read_peak_file_record: peak file record does not exist");
      release(peak);
      return(NULL);
      }
    if (fread(peak,sizeof(peak_struct),1,peak_table->file) != 1)  {
      Werrprintf("read_peak_file_record: cannot read peak file record #%d",
			record);
      release(peak);
      return(NULL);
      }
    return(peak);
}

/*****************************************************************************
*  READ_PEAK_FILE - read peak table from file.  If filename is NULL, open
*	default filename "peakfile.ll2d", otherwise open filename.
*****************************************************************************/
int read_peak_file(peak_table_struct **peak_table, char *filename)
{
    int j, npeaks, header_count;
    char filenm0[MAXPATHL], *peak_fn;
    char filenm[MAXPATHL], cmdstr[MAXPATHL];
    char str[MAXPATHL], *name;
    peak_struct *peak;
    FILE *fp;
    struct stat unix_fab;
    int ival;

    delete_peak_table(peak_table);
    init_peak_table(peak_table);
    if (filename == NULL)  {	/* if no filename given, prompt for one */
      name = get_filename();
      sprintf(str,"File name (enter name and <return>) [default is \"%s\"]? ",
		name);
      W_getInput(str,filenm0,MAXPATHL-1);
      if (strlen(filenm0)==0)  {  /* if no filename typed in, use default */
        strcpy(filenm0,name);
        }
      release(name);
      }
    else  {	/* otherwise use filename passed in as argument */
      strcpy(filenm0,filename);
      }
    /* try to open file */
    strcpy(filenm,filenm0);
    if ((fp = fopen(filenm,"r+")) == NULL) {
      if (filenm0[0] != '/')  {	  /* if file not found, try curexpdir path */
        strcpy(filenm,curexpdir);
        strcat(filenm,"/ll2d/");
        strcat(filenm,filenm0);
        if ((fp = fopen(filenm,"r+")) == NULL)
          return(FILE_NOT_FOUND);
	}
      else
        return(FILE_NOT_FOUND);
      }
    /* file was found and opened */
    /* set filenm0 to have default ll2d filename */
    strcpy(filenm0,curexpdir);
    strcat(filenm0,"/ll2d");
    ival = stat(filenm0, &unix_fab);
    if (ival || !S_ISDIR(unix_fab.st_mode))  {
      char cmd_str[MAXPATH];
      sprintf(cmd_str,"mkdir('%s')\n",filenm0);
      execString(cmd_str);
      ival = stat(filenm0, &unix_fab);
    }
    if (ival || !S_ISDIR(unix_fab.st_mode))  {
      fclose(fp);
      return(ERROR);
    }
    
    strcat(filenm0,"/");
    peak_fn = get_filename();
    strcat(filenm0,peak_fn);
    release(peak_fn);
    /* copy requested file to temp file and open that file */
    if (strcmp(filenm,filenm0) != 0)  {
      fclose(fp);
      strcat(filenm0,"~");
      sprintf(cmdstr,"cp('%s','%s')\n",filenm,filenm0);
      execString(cmdstr);
      if ((fp = fopen(filenm0,"r+")) == NULL)
        return(FILE_NOT_FOUND);
      }
    (*peak_table)->file = fp;
    if (read_peak_file_header(*peak_table))  {
      return(ERROR);
      }
    if (((*peak_table)->version > CURRENT_VERSION+0.01) ||
	((*peak_table)->version < 0.99))  {
      Werrprintf("read_peak_file: incorrect peak file format");
      return(ERROR);
      }
    if ((*peak_table)->header[0]>PEAK_FILE_MAX||(*peak_table)->header[0]<0)  {
      Werrprintf("read_peak_file: incorrect peak file format");
      return(ERROR);
      }
    (*peak_table)->num_peaks = 0;
    header_count = (*peak_table)->header[0];
    (*peak_table)->header[0] = 0;
    npeaks = 0;
    j = 0;
    while ((npeaks < header_count) && (j < PEAK_FILE_MAX))  {
        j++;
	  peak = read_peak_file_record(*peak_table,j);
          if (peak == NULL)  {
	    delete_peak_table(peak_table);
	    init_peak_table(peak_table);
	    return(ERROR);
	    }
          if (insert_peak_in_table(*peak_table,peak))  {
	    delete_peak_table(peak_table);
	    init_peak_table(peak_table);
	    return(ERROR);
	    }
	  npeaks++;
        }
    if (header_count != npeaks)  {
      Werrprintf("peak file inconsistency detected - cannot be read");
      delete_peak_table(peak_table);
      init_peak_table(peak_table);
      return(ERROR);
      }
    /* set filenm0 to have default ll2d filename */
    strcpy(filenm0,curexpdir);
    peak_fn = get_filename();
    strcat(filenm0,"/ll2d/");
    strcat(filenm0,peak_fn);
    release(peak_fn);
    if (strcmp(filenm,filenm0) != 0)  {
      if ((*peak_table)->num_peaks > 0)  {
        sprintf(cmdstr,"mv('%s~','%s')\n",filenm0,filenm0);
        execString(cmdstr);
        }
      else  {
        sprintf(cmdstr,"rm('%s~')\n",filenm0);
        execString(cmdstr);
        }
      }
    if ((*peak_table)->version < CURRENT_VERSION)
      update_peak_file_version(peak_table);
    return(COMPLETE);
}

static void update_peak_file_version(peak_table_struct **peak_table)
{
    char filenm0[MAXPATHL],cmdstr[MAXPATHL],*peak_fn;
    peak_table_struct *tmp_peak_table=NULL;
    peak_struct *tmp_peak;
    FILE *tmp_file;
    int index, num_real_peaks;

    /* set filenm0 to have default ll2d filename */
    strcpy(filenm0,curexpdir);
    peak_fn = get_filename();
    strcat(filenm0,"/ll2d/");
    strcat(filenm0,peak_fn);
    sprintf(cmdstr,"mv('%s','%s.v%3.1f')\n",filenm0,filenm0,(*peak_table)->version);
    execString(cmdstr);
    Winfoprintf("Updating ll2d peak file version. Old peak file saved as %s.v%3.1f",
		peak_fn,(*peak_table)->version);
    init_peak_table(&tmp_peak_table);
    if (create_peak_file(tmp_peak_table,peak_fn))  {
      delete_peak_table(peak_table);
      delete_peak_table(&tmp_peak_table);
      return;
      }
    release(peak_fn);
    /* now comes the good stuff....  */
    /* switch file pointers and close file with the old version peak file */
    tmp_file = (*peak_table)->file;
    (*peak_table)->file = tmp_peak_table->file;
    tmp_peak_table->file = tmp_file;
    /* delete new peak table from memory */
    delete_peak_table(&tmp_peak_table);
    /* now we have the old peak table in memory and a new file which has
	an empty header and no peaks in it.  Now we just have to write
	the header and all the peaks in the in-memory peak table to the
	file and we're done. */
    (*peak_table)->version = CURRENT_VERSION;
    write_peak_file_header(*peak_table);
    tmp_peak = create_peak(0.0,0.0,0.0);
    num_real_peaks = 0;
    index = 1;
    while (num_real_peaks < (*peak_table)->num_peaks)  {
      write_peak_file_record(*peak_table,tmp_peak,index);
      if ((*peak_table)->header[index] == PEAK_FILE_FULL)
        num_real_peaks++;
      index++;
      }
    release(tmp_peak);
    tmp_peak = (*peak_table)->head;
    while (tmp_peak != NULL)  {
      write_peak_file_record(*peak_table,tmp_peak,tmp_peak->key);
      tmp_peak = tmp_peak->next;
      }
}

/*****************************************************************************
*  READ_ASCII_PEAK_FILE - read peak table from file.  If filename is NULL,
*	prompt for filename.
*****************************************************************************/
static int read_ascii_peak_file(peak_table_struct **peak_table, char *filename)
{
    int i, key, length;
    char filenm0[MAXPATHL],label[LABEL_LEN+1], line[133], *ch_ptr, *peak_fn;
    char filenm[MAXPATHL], comment[COMMENT_LEN+1];
    char str[MAXPATHL];
    double f1, f2, f1_min, f1_max, f2_min, f2_max, amp, vol, fwhh1, fwhh2;
    double f1_rflrfp, f2_rflrfp, f1_axis_scl, f2_axis_scl;
    float ver;
    peak_struct *peak;
    FILE *fp;
    char *ret __attribute__((unused));

    delete_peak_table(peak_table);
    init_peak_table(peak_table);
    peak_fn = get_filename();
    length = strlen(peak_fn);
    if (strcmp(peak_fn + length - 4,".bin") == 0)  {
      peak_fn[length-4] = '\0';
      strcat(peak_fn,".txt");
      }
    if (filename == NULL)  {	/* if no filename given, prompt for one */
      sprintf(str,"File name (enter name and <return>) [default is \"%s\"]? ",
		peak_fn);
      W_getInput(str,filenm0,MAXPATHL-1);
      if (strlen(filenm0)==0)  {  /* if no filename typed in, use default */
        strcat(filenm0,peak_fn);
        }
      }
    else  {	/* otherwise use filename passed in as argument */
      strcpy(filenm0,filename);
      }
    release(peak_fn);
    /* try to open file */
    if ((fp = fopen(filenm0,"r")) == NULL) {
      if (filenm0[0] != '/')  {	  /* if file not found, try curexpdir path */
        strcpy(filenm,curexpdir);
        strcat(filenm,"/ll2d/");
        strcat(filenm,filenm0);
        if ((fp = fopen(filenm,"r")) == NULL)
          return(FILE_NOT_FOUND);
	}
      else
        return(FILE_NOT_FOUND);
      }

    if (fscanf(fp,"%f%lf%lf%lf%lf",&ver,&f1_axis_scl,&f2_axis_scl,&f1_rflrfp,
			&f2_rflrfp) != 5)  {
      Werrprintf("Error reading peak file header: %s",filenm);
      fclose(fp);
      return(ERROR);
      }
    (*peak_table)->version = ver;
    if ((*peak_table)->version < CURRENT_VERSION)  {
      /* No differences between versions 1.0 and 1.1 */
      (*peak_table)->version = CURRENT_VERSION;
      }
    ret = fgets(line,133,fp);
    ret = fgets(line,133,fp);
    ret = fgets(line,133,fp);
    ret = fgets(line,133,fp);
    if  ((ch_ptr = strchr(line,'F')) == NULL)  {
      Werrprintf("Error reading peak file header: %s",filenm);
      fclose(fp);
      return(ERROR);
      }
    (*peak_table)->f1_label = *(ch_ptr+1);
    if  ((ch_ptr = strchr(ch_ptr+1,'F')) == NULL)  {
      Werrprintf("Error reading peak file header: %s",filenm);
      fclose(fp);
      return(ERROR);
      }
    (*peak_table)->f2_label = *(ch_ptr+1);
    ret = fgets(line,133,fp);
    ret = fgets(line,133,fp);

    peak_fn = get_filename();
    strcpy(filenm,peak_fn);
    release(peak_fn);
    strcat(filenm,"~");
    if (create_peak_file(*peak_table,filenm))  {
      delete_peak_table(peak_table);
      fclose(fp);
      return(ERROR);
      }

    while ( (ret = fgets(line,133,fp)) ) {
      i = 0;
      while ((i < LABEL_LEN) && (line[i] != '\n'))     i++;
      line[i] = '\0';
      strncpy(label,line,LABEL_LEN);
      label[LABEL_LEN] = '\0';
      ret = fgets(line,133,fp);
      i = 0;
      while ((i < COMMENT_LEN) && (line[i] != '\n'))     i++;
      line[i] = '\0';
      strncpy(comment,line,COMMENT_LEN);
      comment[COMMENT_LEN] = '\0';
      if (fscanf(fp,"%d%lf%lf%lf%lf",&key,&f1,&f2,&amp,&vol) != 5)  {
        Werrprintf("Error reading peak file record #%d",
			(*peak_table)->num_peaks);
        fclose(fp);
        return(ERROR);
        }
      if (fscanf(fp,"%lf%lf",&fwhh1,&fwhh2) != 2)  {
        Werrprintf("Error reading peak file record #%d",
			(*peak_table)->num_peaks);
        fclose(fp);
        return(ERROR);
        }
      if (fscanf(fp,"%lf%lf%lf%lf",&f1_min,&f1_max,&f2_min,&f2_max) != 4)  {
        Werrprintf("Error reading peak file record #%d",
			(*peak_table)->num_peaks);
        fclose(fp);
        return(ERROR);
        }
      ret = fgets(line,133,fp);
      f1 = f1*f1_axis_scl + f1_rflrfp;
      f2 = f2*f2_axis_scl + f2_rflrfp;
	/* check if peak bounds have been defined, then correct for reference
		frequency, otherwise write zeros */
      if ((f1_min != 0.0) || (f1_max != 0.0) ||
	  (f2_min != 0.0) || (f2_max != 0.0))  {
	f1_min = f1_min*f1_axis_scl + f1_rflrfp;
	f1_max = f1_max*f1_axis_scl + f1_rflrfp;
	f2_min = f2_min*f2_axis_scl + f2_rflrfp;
	f2_max = f2_max*f2_axis_scl + f2_rflrfp;
	}
      fwhh1 *= f1_axis_scl;
      fwhh2 *= f2_axis_scl;
      peak = create_peak(f1,f2,amp);
      peak->key = key;
      peak->f1_min = f1_min;
      peak->f1_max = f1_max;
      peak->f2_min = f2_min;
      peak->f2_max = f2_max;
      peak->fwhh1 = fwhh1;
      peak->fwhh2 = fwhh2;
      peak->vol = vol/ins2val;
      strcpy(peak->label,label);
      pad_label(peak->label);
      strcpy(peak->comment,comment);
      if (insert_peak(*peak_table,peak))  {
	delete_peak_table(peak_table);
	init_peak_table(peak_table);
	fclose(fp);
	return(ERROR);
	}
      }
    fclose(fp);
    strcpy(filenm,curexpdir);
    strcat(filenm,"/ll2d/");
    peak_fn = get_filename();
    strcat(filenm,peak_fn);
    release(peak_fn);
    if ((*peak_table)->num_peaks > 0)  {
      sprintf(str,"mv('%s~','%s')\n",filenm,filenm);
      execString(str);
      }
    else  {
      sprintf(str,"rm('%s~')\n",filenm);
      execString(str);
      }

    return(COMPLETE);
}
    

/*****************************************************************************
*  WRITE_PEAK_FILE_ENTRY - write peak into ascii file.  Assume file is already
*	open.
*****************************************************************************/
static void write_peak_file_entry(FILE *fp, peak_struct *peak,
                double f1_rflrfp, double f2_rflrfp,
		double f1_axis_scl, double f2_axis_scl,
                char *format1,char *format2,char *format3)
{
    double f1, f1_min, f1_max, f2, f2_min, f2_max;

    f1 = (peak->f1 - f1_rflrfp)/f1_axis_scl;
    f2 = (peak->f2 - f2_rflrfp)/f2_axis_scl;
	/* check if peak bounds have been defined, then correct for reference
		frequency, otherwise write zeros */
    if ((peak->f1_min != 0.0) || (peak->f1_max != 0.0) ||
	  (peak->f2_min != 0.0) || (peak->f2_max != 0.0))  {
	f1_min = (peak->f1_min - f1_rflrfp)/f1_axis_scl;
	f1_max = (peak->f1_max - f1_rflrfp)/f1_axis_scl;
	f2_min = (peak->f2_min - f2_rflrfp)/f2_axis_scl;
	f2_max = (peak->f2_max - f2_rflrfp)/f2_axis_scl;
	}
    else  {
	f1_min = 0.0;
	f1_max = 0.0;
	f2_min = 0.0;
	f2_max = 0.0;
	}
    fprintf(fp,format1,peak->label,peak->comment,peak->key,f1,f2,peak->amp,
		peak->vol*ins2val);
    fprintf(fp,format2,peak->fwhh1/f1_axis_scl,peak->fwhh2/f2_axis_scl);
    fprintf(fp,format3,f1_min,f1_max,f2_min,f2_max);
}


/*****************************************************************************
*  WRITE_ASCII_PEAK_FILE - write peak table into ascii file.  If filename ==
*	NULL, prompt for filename.
*****************************************************************************/
static int write_ascii_peak_file(peak_table_struct *peak_table,
                                 char *filename, int peakMode)
{
    int done, reversed, length;
    double f1_rflrfp, f2_rflrfp, tmp, f1_axis_scl, f2_axis_scl, start, len;
    FILE *fp;
    char filenm0[MAXPATHL], str[MAXPATHL], ch;
    char *peak_fn;
    char line1[133], line2[133], line3[133];
    char format1[133], format2[133], format3[133];
    struct node {
	peak_struct *ptr;
	struct node *next;
	};
    struct node *peaks, *temp, *pointer;
    peak_struct *pk_ptr;

    peak_fn = get_filename();
    length = strlen(peak_fn);
    if (strcmp(peak_fn + length - 4,".bin") == 0)  {
      peak_fn[length-4] = '\0';
      strcat(peak_fn,".txt");
      }
    if (filename == NULL)  {	/* if no filename given, prompt for one */
      sprintf(str,"File name (enter name and <return>) [default is \"%s\"]? ",
		peak_fn);
      W_getInput(str,filenm0,MAXPATHL-1);
      if (strlen(filenm0)==0)  {  /* if no filename typed in, use default */
        strcpy(filenm0,peak_fn);
        }
      }
    else  {	/* otherwise use filename passed in as argument */
      strcpy(filenm0,filename);
      }
    release(peak_fn);
    if ((fp = fopen(filenm0,"w")) == NULL) {
      Werrprintf("write_ascii_peak_file: cannot create ascii peak output file");
      return(ERROR);
      }

    get_scale_pars(HORIZ,&start,&len,&f1_axis_scl,&reversed);
    get_scale_pars(VERT,&start,&len,&f2_axis_scl,&reversed);
    get_rflrfp(HORIZ,&f1_rflrfp);
    get_rflrfp(VERT,&f2_rflrfp);
    get_display_label(VERT,&ch);
    if (ch == peak_table->f1_label)  {
      tmp = f1_rflrfp;
      f1_rflrfp = f2_rflrfp;
      f2_rflrfp = tmp;
      tmp = f1_axis_scl;
      f1_axis_scl = f2_axis_scl;
      f2_axis_scl = tmp;
      }
    if (peakMode)
    {
       fprintf(fp,"# Index    F1(ppm)      F2(ppm)      Amplitude\n");
    }
    else
    {
       get_formats(line1,line2,line3,format1,format2,format3,REG_FORMAT);
       fprintf(fp,"%3.1f  %17.8f  %17.8f  %17.8f  %17.8f\n",peak_table->version,
		f1_axis_scl,f2_axis_scl,f1_rflrfp,f2_rflrfp);
       fprintf(fp,line1);
       fprintf(fp,line2);
       fprintf(fp,line3);
    }
    
    peaks = NULL;
    pk_ptr = peak_table->head;
    while (pk_ptr)  {
      if ((temp = (struct node *)allocateWithId(sizeof(struct node),
		"peak_temp")) == 0) {
        Werrprintf("write_peak_table: cannot allocate node");
        fclose(fp);
        return(ERROR);
        }
      temp->ptr = pk_ptr;
      if ((peaks==NULL) || (temp->ptr->key < peaks->ptr->key)) {
	temp->next = peaks;
        peaks = temp;
	}
      else {
        pointer = peaks;
        done = 0;
        while ((pointer) && (pointer->next) && (!done))  {
	  if ((temp->ptr->key > pointer->ptr->key) &&
	      (temp->ptr->key < pointer->next->ptr->key))
	    done = 1;
	  else
	    pointer = pointer->next;
	  }
        temp->next = pointer->next;
        pointer->next = temp;
	}
      pk_ptr = pk_ptr->next;
      }
    pointer = peaks;
    while (pointer)  {
      if (peakMode)
         fprintf(fp,"%5d    %9.3f    %9.3f      %g\n", 
            pointer->ptr->key,
            (pointer->ptr->f1 - f1_rflrfp)/f1_axis_scl,
            (pointer->ptr->f2 - f2_rflrfp)/f2_axis_scl,
            pointer->ptr->amp);
      else
         write_peak_file_entry(fp,pointer->ptr,f1_rflrfp,f2_rflrfp,f1_axis_scl,f2_axis_scl,
                               format1,format2,format3);
      pointer = pointer->next;
      }
    releaseAllWithId("peak_temp");
    fclose(fp);
    return(COMPLETE);
}

/*****************************************************************************
*  GET_FORMATS - returns header lines for ascii peak file in "line1,2,3" and
*	returns format strings for writing peak records to ascii peak file
*	in "format1,2,3".  "type" selects either REG_FORMAT or INFO_FORMAT.
*****************************************************************************/
static void get_formats(char *line1, char *line2, char *line3,
                        char *format1, char *format2, char *format3, int type)
{
    char h_ch,v_ch,h_ch_l,v_ch_l,*format_str_1,*format_str_2,ch_1,ch_2;
    char h_label[10], v_label[10], label_1[10], label_2[10];
    int i;

	get_display_label(HORIZ,&h_ch);
	get_scale_axis(HORIZ,&h_ch_l);
	get_display_label(VERT,&v_ch);
	get_scale_axis(VERT,&v_ch_l);
        h_label[0] = '\0';
        v_label[0] = '\0';
	get_label(HORIZ,UNIT4,h_label);
	get_label(VERT,UNIT4,v_label);
	if (h_ch < v_ch)  {
	  strcpy(label_1,h_label);
	  for (i=strlen(label_1);i<7;i++)
	    label_1[i] = ' ';
	  label_1[7] = '\0';
	  strcpy(label_2,v_label);
	  for (i=strlen(label_2);i<7;i++)
	    label_2[i] = ' ';
	  label_2[7] = '\0';
	  ch_1 = h_ch;
	  ch_2 = v_ch;
	  if (h_ch_l == 'h')
	    if (type == REG_FORMAT)
	      format_str_1 = "%14.5f";
	    else
	      format_str_1 = "  %11.2f ";
	  else
	    if (type == REG_FORMAT)
	      format_str_1 = "%14.8f";
	    else
	      format_str_1 = "   %9.3f  ";
	  if (v_ch_l == 'h')
	    if (type == REG_FORMAT)
	      format_str_2 = "%14.5f";
	    else
	      format_str_2 = "  %11.2f ";
	  else
	    if (type == REG_FORMAT)
	      format_str_2 = "%14.8f";
	    else
	      format_str_2 = "   %9.3f  ";
	  }
	else  {
	  strcpy(label_1,v_label);
	  for (i=strlen(label_1);i<7;i++)
	    label_1[i] = ' ';
	  label_1[7] = '\0';
	  strcpy(label_2,h_label);
	  for (i=strlen(label_2);i<7;i++)
	    label_2[i] = ' ';
	  label_2[7] = '\0';
	  ch_1 = v_ch;
	  ch_2 = h_ch;
	  if (h_ch_l == 'h')
	    if (type == REG_FORMAT)
	      format_str_2 = "%14.5f";
	    else
	      format_str_2 = "  %11.2f ";
	  else
	    if (type == REG_FORMAT)
	      format_str_2 = "%14.8f";
	    else
	      format_str_2 = "   %9.3f  ";
	  if (v_ch_l == 'h')
	    if (type == REG_FORMAT)
	      format_str_1 = "%14.5f";
	    else
	      format_str_1 = "  %11.2f ";
	  else
	    if (type == REG_FORMAT)
	      format_str_1 = "%14.8f";
	    else
	      format_str_1 = "   %9.3f  ";
	  }
	sprintf(line1,"Peak Label\nComment Line\n Num.      %s        %s      Amplitude        Volume\n",
		label_1,label_2);
	sprintf(line2,"           FWHH%c          FWHH%c\n",
		ch_1,ch_2);
	sprintf(line3,"           F%c Min.        F%c Max.        F%c Min.        F%c Max.\n",
		ch_1,ch_1,ch_2,ch_2);
	sprintf(format1,"%%s\n%%s\n%%4d %s %s %%14.8f %%14.8f\n",
		 format_str_1,format_str_2);
	sprintf(format2,"     %s %s\n",
		 format_str_1,format_str_2);
	sprintf(format3,"     %s %s %s %s\n",
		 format_str_1,format_str_1,format_str_2,format_str_2);
}

/******************************************************************************
*	ACOSY and PEAK2D Stuff follows...
******************************************************************************/
#define TABLE2SIZE	8192
#define TABLE3SIZE	8192
#define TABLE4SIZE      1024

extern int Rflag;

struct tb2
  { short x1,x2,y1,y2;
  };

struct tb3
  { short left,right,nx,ny;
  };

struct tb4
  { short x,y;
  };

/******************/
static int get_symbol(int n)
/******************/
{ int num;
  num = 'Z'-'A'+1;
  if (n<0)
    return 0;
  else if (n<=num)
    return (n+'A'-1);
  else if (n<=2*num)
    return (n-num+'a'-1);
  else if (n<=2*num+9)
    return (n-2*num+'0');
  else
    return 0;
}

/******************************/
static void sorttable3(struct tb3 table3[], int *tb3max)
/******************************/
{ int i,j,s;
/* this would eliminate any peaks, which are not referenced from both axes
  i = 0;
  while (i<*tb3max)
    { if ((table3[i].nx==0) || (table3[i].ny==0))
        { j = *tb3max-1;
          table3[i].left = table3[j].left;
          table3[i].right = table3[j].right;
          table3[i].nx = table3[j].nx;
          table3[i].ny = table3[j].ny;
          (*tb3max)--;
        }
      else
        i++;
    } */
  /* now sort the table */
  for (i=0; i<*tb3max-1; i++)
    { for (j=i+1; j<*tb3max; j++)
        if (table3[j].left+table3[j].right<table3[i].left+table3[i].right)
          { s = table3[i].left;
            table3[i].left=table3[j].left;
            table3[j].left=s;
            s = table3[i].right;
            table3[i].right=table3[j].right;
            table3[j].right=s;
            s = table3[i].nx;
            table3[i].nx=table3[j].nx;
            table3[j].nx=s;
            s = table3[i].ny;
            table3[i].ny=table3[j].ny;
            table3[j].ny=s;
          }
    }
}

/******************************/
static void symmetrize(struct tb2 table2[], int tb2max)
/******************************/
{ int i,j,x1,x2,x3,x4,y1,y2,y3,y4,mdif;
  /* first remove diagonal peaks */
  mdif = (int) (90.0 * npnt / wp);	/* remove, what is within 90 Hz */
  for (i=0; i<tb2max; i++)
    { if ((table2[i].x1<=table2[i].y2+mdif) &&
          (table2[i].x2>=table2[i].y1-mdif))
        { table2[i].x1=0;
          table2[i].x2=0;
          table2[i].y1=0;
          table2[i].y2=0;
        }
    }
  /* then symmetrize */
  mdif = (int) (20.0 * npnt / wp);	/* combine, what is within 20 Hz */
  for (i=0; i<tb2max; i++)
    { x1 = table2[i].x1;
      x2 = table2[i].x2;
      y1 = table2[i].y1;
      y2 = table2[i].y2;
      x3 = 32767;
      x4 = -32767;
      y3 = 32767;
      y4 = -32767;
      for (j=0; j<tb2max; j++)
        { if ((table2[j].y2>=x1-mdif)&&(table2[j].y1<=x2+mdif)
              &&(table2[j].x2>=y1-mdif)&&(table2[j].x1<=y2+mdif))
            { if (table2[j].y1<x3)
                x3 = table2[j].y1;
              if (table2[j].y2>x4)
                x4 = table2[j].y2;
              if (table2[j].x1<y3)
                y3 = table2[j].x1;
              if (table2[j].x2>y4)
                y4 = table2[j].x2;
            }
        }
     if (x3<32767)
       { if (table2[i].x1<x3)
           x3 = table2[i].x1;
         if (table2[i].x2>x4)
           x4 = table2[i].x2;
         if (table2[i].y1<y3)
           y3 = table2[i].y1;
         if (table2[i].y2>y4)
           y4 = table2[i].y2;
         for (j=0; j<tb2max; j++)
          if ((table2[j].y2>=x1)&&(table2[j].y1<=x2)
              &&(table2[j].x2>=y1)&&(table2[j].x1<=y2))
            { table2[j].y1 = x3;
              table2[j].y2 = x4;
              table2[j].x1 = y3;
              table2[j].x2 = y4;;
            }
         table2[i].y1 = y3;
         table2[i].y2 = y4;
         table2[i].x1 = x3;
         table2[i].x2 = x4;;
       }
     else
       { table2[i].x1=0;
         table2[i].x2=0;
         table2[i].y1=0;
         table2[i].y2=0;
       }
   }
}

/*****************************/
static int ll2d_cosy(struct tb2 table2[], int tb2max)
/*****************************/
{ struct tb3 table3[TABLE3SIZE];
  struct tb4 table4[TABLE4SIZE];
  int tb2index,tb3index,tb3max,found,size2,pos2,size3,pos3,numcross,i,tb4index;
  int xdiff,ydiff,xdiff0,ydiff0,x,y,curx,cury,newx,tableflag,ytable,tb4max;
  char str[32];
  float exp_horiz,exp_vert;
  double r_newx;

  color(BLUE);
  tb3max = 0;
  numcross = 0;
  symmetrize(table2,tb2max);
  /* find all signals */
  for (tb2index=0; tb2index<tb2max; tb2index++)
    { if ((table2[tb2index].x1<=table2[tb2index].y2) &&
          (table2[tb2index].x2>=table2[tb2index].y1))
        { /* ignore diagonal peaks */
        }
      else
        { /* is x1,x2 already in table 3 */
          numcross++;
          tb3index = 0;
          found = 0;
          while ((tb3index<tb3max) && (!found))
            { if ((table2[tb2index].x1<table3[tb3index].right)
               && (table2[tb2index].x2>table3[tb3index].left))
                { /* something is in table already */
                  /* is it good enough ? */
                  size2 = table2[tb2index].x2-table2[tb2index].x1;
                  pos2  = table2[tb2index].x2+table2[tb2index].x1;
                  size3 = table3[tb3index].right-table3[tb3index].left;
                  pos3  = table3[tb3index].right+table3[tb3index].left;
                  if (8*abs(pos2-pos3)/(size2+size3)<1)
                    { /* yes, calculate new limits */
                      found = 1;
                      table3[tb3index].left =
                        (table3[tb3index].nx * table3[tb3index].left +
                         table2[tb2index].x1) / (table3[tb3index].nx+1);
                      table3[tb3index].right =
                        (table3[tb3index].nx * table3[tb3index].right +
                         table2[tb2index].x2) / (table3[tb3index].nx+1);
                      table3[tb3index].nx++;
                    }
                }
              tb3index++;
            }
          if (!found)
            { /* need to enter in table */
              table3[tb3max].left = table2[tb2index].x1;
              table3[tb3max].right = table2[tb2index].x2;
              table3[tb3max].nx = 1;
              table3[tb3max].ny = 0;
              tb3max++;
              if (tb3max>=TABLE3SIZE)
                { Werrprintf("too many peaks, reduce vs2d");
                    ABORT;
                }
            }
          /* is y1,y2 already in table 3 */
          tb3index = 0;
          found = 0;
          while ((tb3index<tb3max) && (!found))
            { if ((table2[tb2index].y1<table3[tb3index].right)
               && (table2[tb2index].y2>table3[tb3index].left))
                { /* something is in table already */
                  /* is it good enough ? */
                  size2 = table2[tb2index].y2-table2[tb2index].y1;
                  pos2  = table2[tb2index].y2+table2[tb2index].y1;
                  size3 = table3[tb3index].right-table3[tb3index].left;
                  pos3  = table3[tb3index].right+table3[tb3index].left;
                  if (8*abs(pos2-pos3)/(size2+size3)<1)
                    { /* yes, calculate new limits */
                      found = 1;
                      table3[tb3index].left =
                        (table3[tb3index].nx * table3[tb3index].left +
                         table2[tb2index].y1) / (table3[tb3index].nx+1);
                      table3[tb3index].right =
                        (table3[tb3index].nx * table3[tb3index].right +
                         table2[tb2index].y2) / (table3[tb3index].nx+1);
                      table3[tb3index].ny++;
                    }
                }
              tb3index++;
            }
          if (!found)
            { /* need to enter in table */
              table3[tb3max].left = table2[tb2index].y1;
              table3[tb3max].right = table2[tb2index].y2;
              table3[tb3max].nx = 0;
              table3[tb3max].ny = 1;
              tb3max++;
              if (tb3max>=TABLE3SIZE)
                { Werrprintf("too many peaks, reduce vs2d");
                    ABORT;
                }
            }
        }
    }
  if (tb3max==0) RETURN;
  sorttable3(table3,&tb3max);
  Wgmode();
  Wshow_graphics();
  color(SCALE_COLOR);
  charsize(0.5);
  exp_horiz = (float) expf_dir(HORIZ);
  exp_vert = (float) expf_dir(VERT);
  if (Rflag>1)
   for (tb2index=0; tb2index<tb2max; tb2index++)
    { if ((table2[tb2index].x1<=table2[tb2index].y2) &&
          (table2[tb2index].x2>=table2[tb2index].y1))
        { /* ignore diagonal peaks */
        }
      else
        { amove((int)(dfpnt+(float)(table2[tb2index].x1-fpnt)*exp_horiz),
            (int)(dfpnt2+(float)(table2[tb2index].y1-fpnt1)*exp_vert));
          adraw((int)(dfpnt+(float)(table2[tb2index].x2-fpnt)*exp_horiz),
            (int)(dfpnt2+(float)(table2[tb2index].y1-fpnt1)*exp_vert));
          adraw((int)(dfpnt+(float)(table2[tb2index].x2-fpnt)*exp_horiz),
            (int)(dfpnt2+(float)(table2[tb2index].y2+1-fpnt1)*exp_vert));
          adraw((int)(dfpnt+(float)(table2[tb2index].x1-fpnt)*exp_horiz),
            (int)(dfpnt2+(float)(table2[tb2index].y2+1-fpnt1)*exp_vert));
          adraw((int)(dfpnt+(float)(table2[tb2index].x1-fpnt)*exp_horiz),
            (int)(dfpnt2+(float)(table2[tb2index].y1-fpnt1)*exp_vert));
        }
    }
  curx = cury = ytable = 0;
  for (i=0; i<tb3max; i++)
    { newx=(int)(dfpnt+(float)(table3[i].right+table3[i].left-
                 2*fpnt)*exp_horiz/2);
      if ((newx>=dfpnt)&&(newx<=dfpnt+dnpnt))
        { amove(newx,dfpnt2+dnpnt2+1);
          rdraw(0,ycharpixels/2);
          if (get_symbol(tb3max-i)&&(newx>=curx+xcharpixels))
            { amove(newx-xcharpixels/2,dfpnt2+dnpnt2+2*ycharpixels/3);
              dchar(get_symbol(tb3max-i));
              curx = newx;
            }
        }
    }
  if (numcross==0) RETURN;
  tableflag = (get_symbol(tb3max) && ((tb3max*2+10)*xcharpixels<dfpnt));
  if (!tableflag)
    Werrprintf("No room to display COSY connection table");
  if (tableflag)
    {
      double start,len,axis_scl;
      int reversed;

      color(PARAM_COLOR);
      ytable = mnumypnts - 3*ycharpixels/2;
      amove(xcharpixels,ytable);
      dstring("AUTOMATIC COSY ANALYSIS");
      ytable -= 3*ycharpixels/2;
      amove(4*xcharpixels,ytable);
      for (i=0; i<tb3max; i++)
        { dchar(get_symbol(tb3max-i)); dchar(' ');
        }
      if ((tb3max*2+18)*xcharpixels<dfpnt)
        {
            char units[15];
            char label[20];

            get_label(HORIZ,UNIT1,units);
            sprintf(label,"(%s)",units);
            sprintf(units,"%10.10s",label);
            dstring(units);
        }
      get_scale_pars(HORIZ,&start,&len,&axis_scl,&reversed);
      ytable -= ycharpixels;
      for (i=0; i<tb3max; i++)
        { if (ytable - (tb3max-i-1)*ycharpixels > 0)
            { amove(xcharpixels,ytable-(tb3max-i-1)*ycharpixels);
              dchar(get_symbol(tb3max-i));
              if ((tb3max*2+18)*xcharpixels<dfpnt)
                { r_newx=(dfpnt+(float)(table3[i].right+table3[i].left-
                    2*fpnt)*exp_horiz/2);
                  amove((tb3max*2+5)*xcharpixels,
                    ytable-(tb3max-i-1)*ycharpixels);
                  dpreal((sp+(double)(dfpnt+dnpnt-1-r_newx)*wp/(dnpnt-1))/axis_scl,
                    2,str);
                  dstring(str);
                }
            }
        }
    }
  color(BLUE);
  tb4max = 0;
  for (tb2index=0; tb2index<tb2max; tb2index++)
    { if ((table2[tb2index].x1<=table2[tb2index].y2) &&
          (table2[tb2index].x2>=table2[tb2index].y1))
        { /* ignore diagonal peaks */
        }
      else
        { ydiff = xdiff = 10000000;
          for (tb3index=0; tb3index<tb3max; tb3index++)
            { xdiff0 = table3[tb3index].right + table3[tb3index].left
                       - table2[tb2index].x2 - table2[tb2index].x1;
              ydiff0 = table3[tb3index].right + table3[tb3index].left
                       - table2[tb2index].y2 - table2[tb2index].y1;
              if (abs(xdiff0)<xdiff)
                { xdiff = abs(xdiff0);
                  curx = tb3index;
                }
              if (abs(ydiff0)<ydiff)
                { ydiff = abs(ydiff0);
                  cury = tb3index;
                }
            }
          /* now enter this cross peak in table 4 */
          tb4index = 0;
          while ((tb4index<tb4max)&&
                   ((table4[tb4index].x!=curx)||(table4[tb4index].y!=cury)))
            tb4index++;
          if (tb4index>=tb4max) /* enter, if not there already */
            { table4[tb4max].x = curx;
              table4[tb4max].y = cury;
              tb4max++;
              if (tb4max>=TABLE4SIZE)
                { Werrprintf("Too many cross peaks");
                  ABORT;
                }
            }
        }
    }
  for (tb4index=0; tb4index<tb4max; tb4index++)
    { curx = table4[tb4index].x;
      x = (table3[curx].right + table3[curx].left)/2;
      cury = table4[tb4index].y;
      y = (table3[cury].right + table3[cury].left)/2;
      if ((x>=fpnt)&&(x<=fpnt+npnt)&&
          (y>=fpnt1)&&(y<=fpnt1+npnt1))
        { if (x<=fpnt1)
            amove((int)(dfpnt+(float)(x-fpnt)*exp_horiz),dfpnt2+1);
          else if (x>=fpnt+npnt)
            amove((int)(dfpnt+(float)(x-fpnt)*exp_horiz),dfpnt2+dnpnt2-1);
          else
            amove((int)(dfpnt+(float)(x-fpnt)*exp_horiz),
                            (int)(dfpnt2+(float)(x-fpnt1)*exp_vert));
          adraw((int)(dfpnt+(float)(x-fpnt)*exp_horiz),
                            (int)(dfpnt2+(float)(y-fpnt1)*exp_vert));
          if (y<=fpnt)
            adraw(dfpnt+1,(int)(dfpnt2+(float)(y-fpnt1)*exp_vert));
          else if (y>=fpnt+npnt)
            adraw(dfpnt+dnpnt-1,(int)(dfpnt2+(float)(y-fpnt1)*exp_vert));
          else
            adraw((int)(dfpnt+(float)(y-fpnt)*exp_horiz),
                            (int)(dfpnt2+(float)(y-fpnt1)*exp_vert));
        }
    }
  if (tableflag)
    { color(PARAM_COLOR);
      for (tb4index=0; tb4index<tb4max; tb4index++)
        { curx = table4[tb4index].x;
          cury = table4[tb4index].y;
          if (ytable - (tb3max-cury-1) * ycharpixels > 0)
            { amove(4*xcharpixels + 2*curx * xcharpixels,
                ytable - (tb3max-cury-1) * ycharpixels);
              dchar(get_symbol(tb3max-curx));
            }
        }
    }
   RETURN;
}


/********************************************************************************
*	New ACOSY and PEAK2D Stuff follows...
*********************************************************************************/

#define	PEAK_AREA_MAX	100  /* maximum number of peaks in one cross peak area  */

struct new_tb2
  {  
    double x1,x2,y1,y2,amp;
  };

struct new_tb3
  { short x, y, cr;
  };


/******************************/
static int cluster(struct new_tb2 table2[], int tb2max,
           float corr_size, float diag_size, float comb_size)
/******************************/
{ int i,j,numcross,cor;
  double amp,x1,x2,y1,y2,mdif;

  /* first remove diagonal peaks */

  if (diag_size<0.0)  diag_size = 90.0;
  mdif = diag_size;    /* remove, what is within diag_size Hz */

  for (i=0; i<tb2max; i++)
    { if ((((table2[i].x1-mdif<=table2[i].y1)&&(table2[i].x1+mdif>=table2[i].y1))&&
           ((table2[i].x2-mdif<=table2[i].y2)&&(table2[i].x2+mdif>=table2[i].y2))&&
	   ((table2[i].x1-mdif<=table2[i].y2)&&(table2[i].x1+mdif>=table2[i].y2))) ||
          ((table2[i].x2-mdif<=table2[i].y1)&&(table2[i].x2+mdif>=table2[i].y1)))

        { table2[i].x1=0.0;
          table2[i].x2=0.0;
          table2[i].y1=0.0;
          table2[i].y2=0.0;
          table2[i].amp=0.0;
        }
    }


   if (comb_size<0.0)  comb_size = 0.0;
  /*  comb_size = sw - (float) ll2d_dp_to_frq( (double) npnt/40.0, sw, fn);  */

  mdif = comb_size;	/* combine, if distance from edges less than comb_size Hz */

  for (i=0; i<tb2max; i++)
    {
      amp = table2[i].amp;
      if (amp > 0.0)
       {
	x1 = table2[i].x1;
      	x2 = table2[i].x2;
      	y1 = table2[i].y1;
        y2 = table2[i].y2;

        for (j=i+1; j<tb2max; j++)
         {
	  if ((table2[j].amp<=8.0*amp&&table2[j].amp>=0.125*amp)&&
	      ((table2[j].y2>=y1-mdif)&&(table2[j].y1<=y2+mdif))&&
              ((table2[j].x2>=x1-mdif)&&(table2[j].x1<=x2+mdif)))
            { 
     	      /* combine peak j and peak i */

	      if (table2[j].y1<y1)
                y1 = table2[j].y1;
              if (table2[j].y2>y2)
                y2 = table2[j].y2;
              if (table2[j].x1<x1)
                x1 = table2[j].x1;
              if (table2[j].x2>x2)
                x2 = table2[j].x2;
	      if (table2[j].amp>amp)
	        amp = table2[j].amp;

		/* delete peak j from table 2 */

	      table2[j].x1=0.0;
 	      table2[j].x2=0.0;
  	      table2[j].y1=0.0;
   	      table2[j].y2=0.0;
    	      table2[j].amp=0.0;
            }
         }
        table2[i].y1 = y1;
        table2[i].y2 = y2;
        table2[i].x1 = x1;
        table2[i].x2 = x2;
        table2[i].amp = amp;
       }
   }

  /* find cross peaks */

  if (corr_size<0.0)  corr_size = 20.0;
    /*  corr_size = sw - (float) ll2d_dp_to_frq( (double) npnt/20.0, sw, fn); */
  mdif = corr_size;  /* correlatee, if distance from edges less than corr_size Hz */

  numcross=0;
  for (i=0; i<tb2max; i++)
    { cor=tb2max;
      amp = table2[i].amp;
      if (amp > 0.0)
       {
        for (j=0; j<tb2max; j++)
          {
          if (table2[j].amp>0.0&&
              ((table2[j].x2>=table2[i].y1-mdif)&&(table2[j].x1<=table2[i].y2+mdif))&&
              ((table2[j].y2>=table2[i].x1-mdif)&&(table2[j].y1<=table2[i].x2+mdif)))
            {
              cor=j;
            }
          }

        if (cor == tb2max)
          {
                /* delete peak i from table 2 */
            table2[i].x1=0.0;
            table2[i].x2=0.0;
            table2[i].y1=0.0;
            table2[i].y2=0.0;
            table2[i].amp=0.0;
          }
        else numcross++;

      }
   }

 return(numcross);

}

/******************************/
static int cal_symm(double x1, double x2, double y1, double y2,
           int *nc, int cx[], int cy[],
           float symm_size, float diag_size)
/******************************/
{ int i,j, begin_pt, end_pt, begin_pt1, end_pt1, XPTS, YPTS;
/*  double symm1[512][512], array1[512][512];  */
   double DELTA, SIGMA, MAXSYMM, symm;
   float *fsfl1, *fsfl2;
   int m, m1, k, l, maxx, maxy, dd;
 
   if (diag_size < 0.0) diag_size = 90.0;
   dd = fn/2 - (int) ll2d_frq_to_dp(diag_size,sw,fn);

   /* find data points corresponding to f1, f2 */

   begin_pt = ll2d_frq_to_dp(x2,sw,fn);
   end_pt = ll2d_frq_to_dp(x1,sw,fn);
   begin_pt1 = ll2d_frq_to_dp(y2,sw1,fn1);
   end_pt1 = ll2d_frq_to_dp(y1,sw1,fn1);


   if ((begin_pt < 0) || (begin_pt >= fn/2))  {
      Werrprintf("Peak frequencies not within spectrum.");
      return(0);
   }
   if ((end_pt < 0) || (end_pt >= fn/2))  {
      Werrprintf("Peak frequencies not within spectrum.");
      return(0);
   }
   if ((begin_pt1 < 0) || (begin_pt1 >= fn1/2))  {
      Werrprintf("Peak frequencies not within spectrum.");
      return(0);
   }
   if ((end_pt1 < 0) || (end_pt1 >= fn1/2))  {
      Werrprintf("Peak frequencies not within spectrum.");
      return(0);
   }

   XPTS = end_pt - begin_pt;
   YPTS = end_pt1 - begin_pt1;

   if (symm_size < 0.0) symm_size = 40.0;
   m = m1 = fn/2 - ll2d_frq_to_dp(symm_size,sw,fn);
   if (m>XPTS/2) 
       m=XPTS/2;
   if (m1>YPTS/2)
       m1=YPTS/2;
   if (m<1)
       m=1;
   if(m1<1)
      m1=1;
/*
   if (m1>m)
      m1=m;
   else 
     m=m1;
*/
  
   *nc = 0;
   MAXSYMM=0.0;
   maxx = maxy = 0;

   for (j=0; j<YPTS; j++)
   {
      for (i=0;i<XPTS;i++)
      {
         DELTA = 0.0;
         SIGMA = 0.0;
         for (l=1; l<=m1; l++)
         {
            if (begin_pt1+j+l<fn1/2 && begin_pt1+j-l >= 0)
            {
	       if ((fsfl1 = gettrace(begin_pt1+j+l,0)) == 0) 
 	          return 0;
               if ((fsfl2 = gettrace(begin_pt1+j-l,0)) == 0)
                  return 0;
	       for (k=1; k<=m; k++)
	       {
	          if (begin_pt+i+k<fn/2 && begin_pt+i-k>=0)
	          {
	             DELTA += fabs(fsfl1[begin_pt+i+k]-fsfl2[begin_pt+i-k]);
		     DELTA += fabs(fsfl2[begin_pt+i+k]-fsfl1[begin_pt+i-k]);
	             SIGMA += fabs(fsfl1[begin_pt+i+k]+fsfl2[begin_pt+i-k]); 
		     SIGMA += fabs(fsfl2[begin_pt+i+k]+fsfl1[begin_pt+i-k]);
	          }
	       }
	       if (begin_pt+i<fn/2)
	       {
		  DELTA += fabs(fsfl1[begin_pt+i]-fsfl2[begin_pt+i]);
		  SIGMA += fabs(fsfl1[begin_pt+i]+fsfl2[begin_pt+i]);
	       }
            }
	 }

	 if (begin_pt1+j<fn/2)
	 {
            if ((fsfl1 = gettrace(begin_pt1+j,0)) == 0)
               return 0;
	    for (k=1; k<=m; k++)
            {
	       DELTA += fabs(fsfl1[begin_pt+k]-fsfl1[begin_pt-k]);
	       SIGMA += fabs(fsfl1[begin_pt+k]+fsfl1[begin_pt-k]); 
	    }
	 }

	 if (DELTA < SIGMA)
	    symm = 1.0 - DELTA / SIGMA;
	 else
	    symm = 0.0;

         if (MAXSYMM<symm)
         {
            MAXSYMM=symm;
            maxx=i;
            maxy=j;
         }
         if (symm > 0.9 && abs(j+begin_pt1-i-begin_pt)>=dd )
         {
            if ( *nc < PEAK_AREA_MAX) 
	    {
               cy[ *nc ] = j+begin_pt1;
               cx[ *nc ] = i+begin_pt;
            }
            *nc += 1;
         }
      }
   }

   if (*nc == 0 && abs(maxy+begin_pt1-maxx-begin_pt)>=dd && MAXSYMM>0.3)
   {
      cy[ *nc ] = maxy+begin_pt1;
      cx[ *nc ] = maxx+begin_pt;
      *nc += 1;
   }
 return(1);
}


/******************************/
static int correlate_table(struct new_tb3 table3[], int tb3max,
           int table4[], int *tb4max, struct new_tb3 table5[], int *tb5max,
           int size)
/******************************/
{ 
  int i,j;
  double s,d;
  int tb4index, tb5index, found;


 for (j=0; j<tb3max; j++)
  {
   found = 0;
   d=32767.0;
   for (i=0; i<tb3max; i++)
     { 
       s = (double) ( (table3[i].x - table3[j].y)*(table3[i].x - table3[j].y)
       + (table3[i].y - table3[j].x)*(table3[i].y - table3[j].x));    
       s = sqrt(s);
       if ( d > s ) 
          {
           d = s;
           if (d <= size)
             { 
              found=1; 
              table3[j].cr=i;
             }
    	  }
      }
    if (!found)
         { 
              table3[j].cr=tb3max;
         }
  }

 tb5index = 0;
 for (j=0; j<tb3max; j++)
  {
   i = table3[j].cr;
   if (table3[j].cr<tb3max)
    {
     if (table3[i].cr == j)
       {
        table5[tb5index].cr = tb5index + 1;
 	table5[tb5index +1].cr = tb5index; 
        table5[tb5index].x=table3[j].x;
        table5[tb5index].y=table3[j].y;
        tb5index++;
        table5[tb5index].x=table3[i].x;
        table5[tb5index].y=table3[i].y;
	tb5index++;
	table3[i].cr = tb3max;
       }
    }
  }

 *tb5max = tb5index;

 j = *tb4max = 0;
 i=0; 
 while (i< *tb5max)
    {
      tb4index = 0;
      found = 0;
      while ((tb4index < j) && (!found))
        {   
         if (  table4[tb4index]== (table5[i].x + table5[i+1].y)/2 ) 
           { 
             found = 1;
             table5[i].x = table4[tb4index];
             table5[i+1].y = table4[tb4index];

           }
         tb4index++;
        }

     if (!found)
         { 
              table4[j] = (table5[i].x + table5[i+1].y)/2;
              table5[i].x = table4[j];
              table5[i+1].y = table4[j];
              j++;
              if (j>= TABLE4SIZE)
                   { *tb4max = j; ABORT;}
         }

        tb4index = 0;
        found = 0;
        while ((tb4index<j) && (!found))
          {  
              if  (table4[tb4index] == (table5[i].y + table5[i+1].x)/2)
                { 
                      found = 1;
                      table5[i].y = table4[tb4index];
                      table5[i+1].x = table4[tb4index];
                }
              tb4index++;
            }

         if (!found)
            { 
              table4[j] = (table5[i].y + table5[i+1].x)/2;
              table5[i].y = table4[j];
              table5[i+1].x = table4[j];
              j++;
              if (j>= TABLE4SIZE)
                   { *tb4max = j; ABORT;}
            }
    i += 2;   
    }

  *tb4max = j; 
  for (i=0; i< *tb4max-1; i++)
    { 
     for (j=i+1; j< *tb4max; j++)
        if (table4[j]<table4[i])
          { s = table4[i];
            table4[i]=table4[j];
            table4[j]=s;

          }
    }


  for (i=0; i< *tb5max; i++)
   {
    for (j=0; j< *tb4max; j++)
      {
        if ( table5[i].x == table4[j])
          table5[i].x = j;
        if ( table5[i].y == table4[j])
          table5[i].y = j;
      }  
   }
   
 RETURN;   

}



/*****************************/
static int ll2d_cosy_new(struct new_tb2 table2[], int tb2max,
           double h_rflrfp, double v_rflrfp,
           float symm_size, float corr_size, float diag_size, float comb_size)
/*****************************/
{ struct new_tb3 table3[TABLE3SIZE];
  int table4[TABLE4SIZE];
  struct new_tb3 table5[TABLE3SIZE];

  int tb2index,tb3index,tb3max,numcross;

  int x,y,curx,cury,newx,tableflag,ytable,tb4max;

  char str[32];
  float exp_horiz,exp_vert;
  double x1, x2, y1, y2, r_newx;
  int nc, tb5max, tb5index, size, i, cx[PEAK_AREA_MAX], cy[PEAK_AREA_MAX];


  color(BLUE);
  numcross=cluster(table2,tb2max,corr_size,diag_size,comb_size);
  if (numcross>=TABLE3SIZE)
          { Werrprintf("too many peaks, reduce vs2d or increase threshold");
            ABORT;
          }

  /* calculate symmetry measurements for all peak clusters */

  tb3index = 0;

  for (tb2index=0; tb2index<tb2max; tb2index++)
    { if (table2[tb2index].amp > 0.0) 
        { 
    	 x1 = table2[tb2index].x1;
      	 x2 = table2[tb2index].x2;
      	 y1 = table2[tb2index].y1;
         y2 = table2[tb2index].y2;

	 if (!cal_symm(x1,x2,y1,y2,&nc,cx,cy,symm_size,diag_size))
	    {Werrprintf("cannot access data properly");
              ABORT;
 	    }
	 else 
	  {
           if (nc>PEAK_AREA_MAX)
	    {Werrprintf("too many peaks in one cross peak area, reduce vs2d or increase threshold");
              ABORT;
 	    }
	   else
	    { for (i=0; i<nc; i++)
               {
	 	table3[tb3index].x = cx[i];
	 	table3[tb3index].y = cy[i];
               	tb3index++;
               	if (tb3index>=TABLE3SIZE)
                 { Werrprintf("too many peaks, reduce vs2d or increase threshold");
                    ABORT;
                 }
	       }
	    }
	  }

        }
    }
           
  tb3max = tb3index;
  if (tb3max==0) RETURN;


  if (corr_size < 0.0)  corr_size = 20.0;
  size = fn/2 - ll2d_frq_to_dp(corr_size,sw,fn);
  if (size <= 0) size = 1;

  correlate_table(table3, tb3max, table4, &tb4max, table5, &tb5max, size);
  if (tb4max>= TABLE4SIZE)
         { Werrprintf("too many peaks, reduce vs2d or increase threshold");
             ABORT;
         }
 
  Wgmode();
  Wshow_graphics();
  color(SCALE_COLOR);
  charsize(0.7);
  exp_horiz = (float) expf_dir(HORIZ);
  exp_vert = (float) expf_dir(VERT);


  if (Rflag>1)        /* draw rectagles for each cross peak cluster*/
   for (tb2index=0; tb2index<tb2max; tb2index++)
    { if (table2[tb2index].amp > 0.0) 
        {     
         x2 =
            fpnt+npnt-((table2[tb2index].x1-h_rflrfp -sp)*(float)npnt/wp);
         x1 =
            fpnt+npnt-((table2[tb2index].x2-h_rflrfp -sp)*(float)npnt/wp);
         y2 =
            fpnt1+npnt1-((table2[tb2index].y1-v_rflrfp -sp1)*(float)npnt1/wp1);
         y1 =
            fpnt1+npnt1-((table2[tb2index].y2-v_rflrfp -sp1)*(float)npnt1/wp1);


	  amove((int)(dfpnt+(float)(x1-fpnt)*exp_horiz),
            (int)(dfpnt2+(float)(y1-fpnt1)*exp_vert));
          adraw((int)(dfpnt+(float)(x2-fpnt)*exp_horiz),
            (int)(dfpnt2+(float)(y1-fpnt1)*exp_vert));
          adraw((int)(dfpnt+(float)(x2-fpnt)*exp_horiz),
            (int)(dfpnt2+(float)(y2+1-fpnt1)*exp_vert));
          adraw((int)(dfpnt+(float)(x1-fpnt)*exp_horiz),
            (int)(dfpnt2+(float)(y2+1-fpnt1)*exp_vert));
          adraw((int)(dfpnt+(float)(x1-fpnt)*exp_horiz),
            (int)(dfpnt2+(float)(y1-fpnt1)*exp_vert));
        }
    }

  /* draw lines and characters on the top of the 2D spectrum */

  curx = ytable = 0;
  for (i=0; i<tb4max; i++)
    {
      newx = table4[i];
      newx=(int)(dfpnt+(float)(newx-fpnt)*exp_horiz);
      if ((newx>=dfpnt)&&(newx<=dfpnt+dnpnt))
        { amove(newx,dfpnt2+dnpnt2+1);
          rdraw(0,ycharpixels/2);
          if (get_symbol(tb4max-i)&&(newx>=curx+xcharpixels))
            { amove(newx-xcharpixels/2,dfpnt2+dnpnt2+2*ycharpixels/3);
              dchar(get_symbol(tb4max-i));
              curx = newx;
            }
        }
    }

  
  if (tb4max==0) RETURN;


  /* display the connection table on the screen */

  tableflag = (get_symbol(tb4max) && ((tb4max*2+10)*xcharpixels<dfpnt));
  if (!tableflag)
    Werrprintf("No room to display COSY connection table");
  if (tableflag)
    {
      double start,len,axis_scl;
      int reversed;

      color(PARAM_COLOR);
      ytable = mnumypnts - 3*ycharpixels/2;
      amove(xcharpixels,ytable);
      dstring("AUTOMATIC COSY ANALYSIS");
      ytable -= 3*ycharpixels/2;
      amove(4*xcharpixels,ytable);
      for (i=0; i<tb4max; i++)
        { dchar(get_symbol(tb4max-i)); dchar(' ');
        }
      if ((tb4max*2+18)*xcharpixels<dfpnt)
        {
            char units[15];
            char label[20];

            get_label(HORIZ,UNIT1,units);
            sprintf(label,"(%s)",units);
            sprintf(units,"%10.10s",label);
            dstring(units);
        }
      get_scale_pars(HORIZ,&start,&len,&axis_scl,&reversed);
      ytable -= ycharpixels;
      for (i=0; i<tb4max; i++)
        { if (ytable - (tb4max-i-1)*ycharpixels > 0)
            { amove(xcharpixels,ytable-(tb4max-i-1)*ycharpixels);
              dchar(get_symbol(tb4max-i));
              if ((tb4max*2+18)*xcharpixels<dfpnt)
                { r_newx=(dfpnt+(float)(table4[i]-fpnt)*exp_horiz);
                  amove((tb4max*2+5)*xcharpixels,
                    ytable-(tb4max-i-1)*ycharpixels);
                  dpreal((sp+(double)(dfpnt+dnpnt-1-r_newx)*wp/(dnpnt-1))/axis_scl,

                    2,str);
                  dstring(str);
                }
            }
        }
    }


 /* draw rectangles of corelated cross peaks for connection */     


  color(BLUE);

  for (tb5index=0; tb5index<tb5max; tb5index++)
  {
    if (table5[tb5index].cr < tb5max)
      {
      curx = table5[tb5index].x;
      x = table4[curx];
      cury = table5[tb5index].y;
      y = table4[cury];
      i = table5[tb5index].cr;
      table5[i].cr = tb5max;

      if ((x>=fpnt)&&(x<=fpnt+npnt)&&
          (y>=fpnt1)&&(y<=fpnt1+npnt1))
        { if (x<=fpnt1)
            amove((int)(dfpnt+(float)(x-fpnt)*exp_horiz),dfpnt2+1);
          else if (x>=fpnt+npnt)
            amove((int)(dfpnt+(float)(x-fpnt)*exp_horiz),dfpnt2+dnpnt2-1);
          else
            amove((int)(dfpnt+(float)(x-fpnt)*exp_horiz),
                            (int)(dfpnt2+(float)(x-fpnt1)*exp_vert));
          adraw((int)(dfpnt+(float)(x-fpnt)*exp_horiz),
                            (int)(dfpnt2+(float)(y-fpnt1)*exp_vert));
          if (y<=fpnt)
            adraw(dfpnt+1,(int)(dfpnt2+(float)(y-fpnt1)*exp_vert));
          else if (y>=fpnt+npnt)
            adraw(dfpnt+dnpnt-1,(int)(dfpnt2+(float)(y-fpnt1)*exp_vert));
          else
            adraw((int)(dfpnt+(float)(y-fpnt)*exp_horiz),
                            (int)(dfpnt2+(float)(y-fpnt1)*exp_vert));
	}
	
      if ((y>=fpnt)&&(y<=fpnt+npnt)&&
          (x>=fpnt1)&&(x<=fpnt1+npnt1))	
        { if (y<=fpnt1)
            amove((int)(dfpnt+(float)(y-fpnt)*exp_horiz),dfpnt2+1);
          else if (y>=fpnt+npnt)
            amove((int)(dfpnt+(float)(y-fpnt)*exp_horiz),dfpnt2+dnpnt2-1);
          else
            amove((int)(dfpnt+(float)(y-fpnt)*exp_horiz),
                            (int)(dfpnt2+(float)(y-fpnt1)*exp_vert));
          adraw((int)(dfpnt+(float)(y-fpnt)*exp_horiz),
                            (int)(dfpnt2+(float)(x-fpnt1)*exp_vert));
          if (x<=fpnt)
            adraw(dfpnt+1,(int)(dfpnt2+(float)(x-fpnt1)*exp_vert));
          else if (x>=fpnt+npnt)
            adraw(dfpnt+dnpnt-1,(int)(dfpnt2+(float)(x-fpnt1)*exp_vert));
          else
            adraw((int)(dfpnt+(float)(x-fpnt)*exp_horiz),
                            (int)(dfpnt2+(float)(x-fpnt1)*exp_vert));
        }
     }
  }
  
  if (tableflag)
    { color(PARAM_COLOR);
      for (tb5index=0; tb5index<tb5max; tb5index++)
        { curx = table5[tb5index].x;
          cury = table5[tb5index].y;
          if (ytable - (tb4max-cury-1) * ycharpixels > 0)
            { amove(4*xcharpixels + 2*curx * xcharpixels,
                ytable - (tb4max-cury-1) * ycharpixels);
              dchar(get_symbol(tb4max-curx));
            }
        }
    }

 RETURN;

}


/**************/
int acosy(int argc, char *argv[], int retc, char *retv[])
/**************/
{ int plotflag,tb2index,ntb2index,i;
  struct tb2 table2[TABLE2SIZE];
  struct new_tb2 ntable2[TABLE2SIZE];
  char filename[MAXPATHL], *peak_fn;
  char ch;
  double w1, w2, h_rflrfp, v_rflrfp;
  peak_table_struct *temp_peak_table = NULL;
  peak_struct *peak;
  double amp, maxw;
  float symm_size, corr_size, diag_size, comb_size;
  char *callName;
/* 
  symm_size=-1.0;
  corr_size=-2.0; 
  diag_size=-3.0; 
  comb_size=-4.0;
*/ 
  symm_size=40.0;
  corr_size=20.0;
  diag_size=90.0;
  comb_size=0.0; 

  Wturnoff_buttons();
  disp_status("ACOSY");
  if (argv[0][0]=='p')
  {
     plotflag = 2;
     callName = &argv[0][1];
  }
  else
  {
     Wsetgraphicsdisplay("acosy");
     plotflag = 1;
     callName = &argv[0][0];
  }
  if (init2d(1,plotflag))
    ABORT;
  if (!d2flag)
    { Werrprintf("no 2D data in data file");
      ABORT;
    }
  if (fn!=fn1)
    { Werrprintf("fn must be equal to fn1");
      ABORT;
    }
  /* read binary peak file */
  delete_peak_table(&peak_table);
  strcpy(filename,curexpdir);
  peak_fn = get_filename();
  strcat(filename,"/ll2d/");
  strcat(filename,peak_fn);
  release(peak_fn);
  if (read_peak_file(&temp_peak_table,filename))
    { Werrprintf("cannot read file %s",filename);
      ABORT;
    }
  get_display_label(VERT,&ch);
  get_rflrfp(HORIZ,&h_rflrfp);
  get_rflrfp(VERT,&v_rflrfp);
  tb2index = 0;
  peak = temp_peak_table->head;

if (strcmp(callName, "acosyold") == 0)
 {
  while (peak)  {
    if ((peak->f1_max - peak->f1_min) > 0.0)
	w1 = peak->f1_max - peak->f1_min;
    else
	w1 = 0.0;
    if ((peak->f2_max - peak->f2_min) > 0.0)
	w2 = peak->f2_max - peak->f2_min;
    else
	w2 = 0.0;
    if (ch == temp_peak_table->f1_label)  {
      table2[tb2index].x1 =
            fpnt+npnt-(int)((peak->f1-h_rflrfp+w1/2.0-sp)*(float)npnt/wp);
      table2[tb2index].x2 =
            fpnt+npnt-(int)((peak->f1-h_rflrfp-w1/2.0-sp)*(float)npnt/wp);
      table2[tb2index].y1 =
            fpnt1+npnt1-(int)((peak->f2-v_rflrfp+w2/2.0-sp1)*(float)npnt1/wp1);
      table2[tb2index].y2 =
            fpnt1+npnt1-(int)((peak->f2-v_rflrfp-w2/2.0-sp1)*(float)npnt1/wp1);
      tb2index++;
      }
    else  {
      table2[tb2index].x1 =
            fpnt+npnt-(int)((peak->f2-h_rflrfp+w2/2.0-sp)*(float)npnt/wp);
      table2[tb2index].x2 =
            fpnt+npnt-(int)((peak->f2-h_rflrfp-w2/2.0-sp)*(float)npnt/wp);
      table2[tb2index].y1 =
            fpnt1+npnt1-(int)((peak->f1-v_rflrfp+w1/2.0-sp1)*(float)npnt1/wp1);
      table2[tb2index].y2 =
            fpnt1+npnt1-(int)((peak->f1-v_rflrfp-w1/2.0-sp1)*(float)npnt1/wp1);
      tb2index++;
      }
    peak = peak->next;
    }

  if (!tb2index)
    { Werrprintf("no islands found");
      ABORT;
    }
  if (!plot) execString("dcon\n");
  delete_peak_table(&temp_peak_table);
  ll2d_cosy(table2,tb2index);

 }

if (strcmp(callName, "acosy") == 0)
 {
   
        if((argc-- > 1) && isReal(*++argv))
                symm_size = stringReal (*argv);
        if((argc-- > 1) && isReal(*++argv))
                corr_size = stringReal (*argv);
        if((argc-- > 1) && isReal(*++argv))
                diag_size = stringReal (*argv);
        if((argc-- > 1) && isReal(*++argv))
                comb_size = stringReal (*argv);

  ntb2index=0;
  peak = temp_peak_table->head;

  maxw = 0.0;
  while (peak)  {

    if ((peak->f1_max - peak->f1_min) > 0.0)
        w1 = peak->f1_max - peak->f1_min;
    else
        w1 = 0.0;
    if ((peak->f2_max - peak->f2_min) > 0.0)
        w2 = peak->f2_max - peak->f2_min;
    else
        w2 = 0.0;
    if (maxw < w1) maxw = w1;
    if (maxw < w2) maxw = w2;

    amp = peak->amp;
    ntable2[ntb2index].amp = fabs(amp);
    if (ch == temp_peak_table->f1_label)  {
      ntable2[ntb2index].x1 = peak->f1-w1/2.0;
      ntable2[ntb2index].x2 = peak->f1+w1/2.0;
      ntable2[ntb2index].y1 = peak->f2-w2/2.0;
      ntable2[ntb2index].y2 = peak->f2+w2/2.0;
      ntb2index++;
      }
    else  {
      ntable2[ntb2index].x1 = peak->f2-w2/2.0;
      ntable2[ntb2index].x2 = peak->f2+w2/2.0;
      ntable2[ntb2index].y1 = peak->f1-w1/2.0;
      ntable2[ntb2index].y2 = peak->f1+w1/2.0;
      ntb2index++;
      }
    peak = peak->next;
    }

  if (!ntb2index)
    { Werrprintf("no islands found");
      ABORT;
    }
  if ( maxw == 0.0)
   {  
    if (symm_size>=0.0) maxw = symm_size;
    else  maxw = 50.0;
    for ( i=0; i<ntb2index; i++)
      {
	ntable2[i].x1 -= maxw/2.0;
      	ntable2[i].x2 += maxw/2.0;
      	ntable2[i].y1 -= maxw/2.0;
      	ntable2[i].y2 += maxw/2.0;
      }
   }
  if (!plot) execString("dconi\n");
  delete_peak_table(&temp_peak_table);
  ll2d_cosy_new(ntable2,ntb2index,h_rflrfp,v_rflrfp,
  		symm_size,corr_size,diag_size,comb_size);

 }
/*
  delete_peak_table(&temp_peak_table);
  if (!plot) execString("dcon\n");
  delete_peak_table(&temp_peak_table);
*/ 

  endgraphics();
  disp_status("     ");
  RETURN;
}

/*************************/
int peak2d(int argc, char *argv[], int retc, char *retv[])
/*************************/
{  int i,ctrace,maxtrace,maxpoint;
   double max;
   float *phasfl;
   double noise;
   double sum, sumsq, ave;
   int switchf1f2 = 0;

   if (argc == 5)
   {
      char trace[5];

      if ( ! P_getstring(CURRENT, "trace", trace, 1, 4) )
         if ( ! strcmp(trace,"f1") )
         {
            switchf1f2 = 1;
            P_setstring(CURRENT, "trace", "f2", 1);
         }
   }
   if (init2d(1,1))
   {
      if (switchf1f2)
         P_setstring(CURRENT, "trace", "f1", 1);
      ABORT;
   }
   if (!d2flag)
     { Werrprintf("no 2D data in data file");
       if (switchf1f2)
          P_setstring(CURRENT, "trace", "f1", 1);
       ABORT;
     }
   if (argc == 5)
   {
      int errorExit = 0;
      if (isReal(argv[1]))
      {
         npnt = ll2d_frq_to_dp( stringReal(argv[1]) + rflrfp ,sw,fn);
      }
      else
      {
         Werrprintf("first argument must be the F2 high field frequency");
         errorExit = 1;
      }
      if (isReal(argv[2]))
      {
         fpnt = ll2d_frq_to_dp( stringReal(argv[2]) + rflrfp ,sw,fn);
         npnt -= fpnt;
      }
      else
      {
         Werrprintf("second argument must be the F2 low field frequency");
         errorExit = 1;
      }
      if (isReal(argv[3]))
      {
         npnt1 = ll2d_frq_to_dp( stringReal(argv[3])+rflrfp1 ,sw1,fn1);
      }
      else
      {
         Werrprintf("third argument must be the F1 high field frequency");
         errorExit = 1;
      }
      if (isReal(argv[4]))
      {
         fpnt1 = ll2d_frq_to_dp( stringReal(argv[4])+rflrfp1 ,sw1,fn1);
         npnt1 -= fpnt1;
      }
      else
      {
         Werrprintf("fourth argument must be the F1 low field frequency");
         errorExit = 1;
      }
      if (errorExit)
      {
         if (switchf1f2)
            P_setstring(CURRENT, "trace", "f1", 1);
         ABORT;
      }
   }
   max = 0;
   maxtrace = 0;
   maxpoint = 0;
   sum = sumsq = noise = 0.0;
   for (ctrace=fpnt1; ctrace<fpnt1+npnt1; ctrace++)
     { if ((phasfl = gettrace(ctrace,fpnt)) == 0)
	 return 1;
       i = 0;
       while (i<npnt)	/* go though the trace */
         { 
            sum += *phasfl;
            sumsq += *phasfl * *phasfl;
	    if (fabs(*phasfl)>fabs(max))
             { max = *phasfl;
               maxtrace = ctrace;
               maxpoint = i;
             }
           i++;
           phasfl++;
         }
     }

   if (retc > 3)
   {
      double stddev;
      int cnt;

      ave = sum / (double) (npnt*npnt1);
      stddev = sqrt( sumsq/((double) (npnt*npnt1)) - ave*ave);
      noise = fabs(ave) + 3.0*stddev;
      sum = sumsq = 0.0;
      cnt = 0;
      for (ctrace=fpnt1; ctrace<fpnt1+npnt1; ctrace++)
     { if ((phasfl = gettrace(ctrace,fpnt)) == 0)
	 return 1;
       i = 0;
       while (i<npnt)	/* go though the trace */
         { 
	    if (fabs(*phasfl)<noise)
            {
            sum += *phasfl;
            sumsq += *phasfl * *phasfl;
            cnt++;
            }
           i++;
           phasfl++;
         }
        }
      ave = sum / (double) cnt;
      stddev = sqrt( sumsq/((double) cnt) - ave*ave);
      noise = fabs(ave) + 3.0*stddev;
   }

   if (argc == 5)
   {
      double val_cr;
      double val_cr1;

      val_cr = ll2d_dp_to_frq((double)(fpnt+maxpoint),sw,fn) - rflrfp;
      val_cr1 = ll2d_dp_to_frq((double)(maxtrace),sw1,fn1) - rflrfp1;
      if (switchf1f2)
         P_setstring(CURRENT, "trace", "f1", 1);
      if (retc)
      {
         retv[0] = realString(vs2d*max);
         if (retc > 1)
         {
            retv[1] = realString(val_cr);
            if (retc > 2)
               retv[2] = realString(val_cr1);
         }
      }
      else
      {
         Winfoprintf("maximum = %g, F2 freq=%g, F1 freq=%g",vs2d*max,val_cr,val_cr1);
      }
   }


#ifdef XXX
   noise /= (double) (npnt*npnt1);
   if (retc >= 3)
   {
/* calculates the threshold for 2-D displaying such that the number of points above the threshold is between 70% to 90% of all the points and the difference of mean(peak) and mean(noise) is maximized.
*/

     double level, stdev, av;
     int loops, nn;

     double L[5], S[5], MB[5], MP[5], PB[5], Pmin, Pmax;
     int j, N[5], ni, na, count;

     stdev = 0.0;
     for (ctrace=fpnt1; ctrace<fpnt1+npnt1; ctrace++)
      { if ((phasfl = gettrace(ctrace,fpnt)) == 0)
	  return 1;
        i = 0;
        while (i<npnt)	/* go though the trace */
         { 
	    stdev += (fabs(*phasfl) - noise)*(fabs(*phasfl) - noise);
            i++;
            phasfl++;
         }
       }

     stdev /=(double) (npnt*npnt1);
     stdev = sqrt (stdev);

     if (retc == 4)
     {
        Winfoprintf("init noise = %g",noise);
        noise = 0.0;
        for (ctrace=fpnt1; ctrace<fpnt1+npnt1; ctrace++)
          { if ((phasfl = gettrace(ctrace,fpnt)) == 0)
     	 return 1;
            i = 0;
            while (i<npnt)	/* go though the trace */
            {      

                 level = fabs( (double) *phasfl);
                 if (level > stdev)
  	            noise += level;
              i++;
              phasfl++;
            }
        }
        noise /= (double) (npnt*npnt1);
        Winfoprintf("final noise = %g",noise);
        retv[3] = realString(noise);
        RETURN;

     }

/*
     level = noise + 3.0*stdev;
     av = 0.0;
     stdev = 0.0;
     nn = 0;

     for (ctrace=fpnt1; ctrace<fpnt1+npnt1; ctrace++)
      { 
       if ((phasfl = gettrace(ctrace,fpnt)) == 0)
	 return 1;
       i = 0;
       while (i<npnt)	
         { if (fabs(*phasfl)<level)
             {  nn++;
                av += fabs(*phasfl);
		stdev += (fabs(*phasfl) - noise)*(fabs(*phasfl) - noise);
             }
           phasfl++;
           i++;
         }
      }
      av /= (double) nn;
      stdev = sqrt (stdev/(double) nn);
      noise = av;

*/


     level = 1.5*stdev;
     noise += 2.0*level;
     count=0;


    for (loops=0; loops < 2; loops++)
     {
     count++;
     for (j=0; j<5; j++)
      {
       N[j]=0;
       L[j] = noise + (double) (j-2)*level;
       if (L[j] < 0.0) L[j] = 0.0;
       MB[j] = 0.0;
       MP[j] = 0.0;
      }
    
     for (ctrace=fpnt1; ctrace<fpnt1+npnt1; ctrace++)
      { 
       if ((phasfl = gettrace(ctrace,fpnt)) == 0)
	 return 1;
       i = 0;
       while (i<npnt)	
         { 
	  for (j=0; j<5; j++)
 	   {
	    if (fabs(*phasfl)<L[j])
             {  N[j]++;
                MB[j] += fabs(*phasfl);
             }
	    else  MP[j] += fabs(*phasfl);
	   }

           phasfl++;
           i++;
         }
       }

     av = 0.0;
     nn = -1;
     Pmin=1.0;
     Pmax=0.0;
     ni=0;
     na=4;

     for (j=0; j<5; j++)
       {
        if (N[j]!=0) MB[j] /= (double) N[j];
        else MB[j]=0.0;
	if (N[j]!=npnt*npnt1) MP[j] /= (double) (npnt*npnt1 - N[j]);
        else MP[j]=0.0;
	PB[j] = (double) N[j]/ (double) (npnt*npnt1);
      	if (Pmin>PB[j]&&PB[j]!=0.0) {Pmin=PB[j]; ni=j;}
      	if (Pmax<PB[j]&&PB[j]!=1.0) {Pmax=PB[j]; na=j;}
	S[j] =  (MP[j]-MB[j]);
	if ( (av<S[j]) && (PB[j]>=0.7) && (PB[j]<=0.9) ) {av=S[j]; nn=j;} 
      }

     if (nn==-1) 
       {
	loops -=1;
	if (Pmax<0.7) noise= L[na]; 

	else if (Pmin>0.9) 
	 {
	  if (ni!=0) level /= 5.0;
 	  noise=L[ni] - 2.0*level; 
	 }

	else if (Pmin<0.7) 
	 {
	  level /= 4.0;
	  if (PB[ni+1]>0.9) noise=(L[ni]+L[ni+1])/2.0;
	  else if (PB[ni+2]>0.9) noise=(L[ni+1]+L[ni+2])/2.0;
	  else if (PB[ni+3]>0.9) noise=(L[ni+2]+L[ni+3])/2.0;
	  else if (PB[ni+4]>0.9) noise=(L[ni+3]+L[ni+4])/2.0;

	 }

	else {level /= 2.0; nn=(ni+na)/2;}

       }
     else  
       { 
	 if (nn==0||nn==4) {level = level/2.0; noise=L[nn];}
	 else if (PB[nn-1]<0.7&&PB[nn+1]>0.9) {level = level/3.0; noise=L[nn];}
	 else if (PB[nn-1]>=0.7&&PB[nn+1]>0.9) {level = level/4.0; noise=L[nn]; }
	 else if (PB[nn-1]<0.7&&PB[nn+1]<=0.9) {level = level/4.0; noise=L[nn]+level;}
       }

      if (count == 10) loops=3;
     }

  }
#endif


/*  noise /= (npnt * npnt1);  */


  D_allrelease();
  if (argc != 5)
  {
  if (retc==0)
    Winfoprintf("maximum = %g, trace=%d, point=%d",vs2d*max,maxtrace,maxpoint);
  if (retc>0)
    retv[0] = realString(vs2d*max);
  if (retc>1)
    retv[1] = realString((double)maxtrace);
  if (retc>2)
    retv[2] = realString((double)maxpoint);
  }
  if (retc>3)
    retv[3] = realString((double)noise*vs2d);
  RETURN;
}

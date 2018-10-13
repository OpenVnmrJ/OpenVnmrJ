/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* makeslice.c Manchester version 6.1B 20iv99 	*/
/* makeslice.c builds up a 2D spectrum from the input table diffusion_display_3D.inp 	*/
/*
                  Dr. Herve BARJAT               
                  Deparment of Chemistry         
                  University of Manchester       
                  Manchester    M13 9PL          
                        UK                       
 */
/* it only shows peaks with a diffusion integral in the slice of CERTAINTY
 * times the full integral
 */
/* Don't insist on data being S_COMPLEX, otherwise absolute value data are
 * only usable if pmode='partial'
 */
/* Try allowing real, complex or hypercomplex data; f1pts per data point in F1 		*/

/*MN changed spurious userdir to curexpdir and directory name  "Dosy" to "dosy"		*/
/*GM 13iv09 don't bother keeping copy in data.orig - dosy macro takes care of this	*/
/*GM 14iv09 test to ensure 2D data; add debugging							*/
/*GM 14iv09 Change order in init_makeslice to check data format before reading in parameters	*/


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "group.h"
#include "tools.h"
#include "data.h"
#include "variables.h"
#include "vnmrsys.h"
#include "disp.h"
#include "pvars.h"
#include "wjunk.h"

/* #define DEBUG_MAKESLICE 1   */		 /*Comment out not to compile the debugging code */
#define ERROR	1
#define TRUE	1
#define FALSE	0
#define NOT_DONE	0
#define PARTIAL		2
#define DONE		1
#define COMPLETE	0
#define MAXPOINTS       50
#define MAXBLOCKS       128
#define	MAXNUMPEAKS	8192
#define	SQR(a)		(sqrarg=(a),sqrarg*sqrarg)
#define LABEL_LEN       20
#define COMMENT_LEN     80
#define CERTAINTY    	0.95
#define NINTSTEPS    	100

#ifdef DEBUG_MAKESLICE
FILE *debug;
#endif

extern double ll2d_dp_to_frq(double, double, int);
extern int ll2d_frq_to_dp(double, double, int);

static int init_makeslice(int, char * []);
static int compute_integral(int, double);

static dfilehead phasehead,fidhead;
extern int start_from_ft;
/* MANCHESTER VERSION 6.1 14 vii 98 GAM */
void swap(double *a, double *b);/* added by PB to swap two doubles	*/

static int	f1pts,r,fn_int,fn1_int;
static char	display_mode;
static double	fn,fn1,sw,sw1,sqrarg,mean_diff,diff_window,start_diff,finish_diff;
static struct	region {	int	num, status;
				double	diff_coef, std_dev, intensity;
				int	block,two_blks;
				int	h_min_pt,h_max_pt,v_min_pt,v_max_pt;
		}	pk[MAXNUMPEAKS];

/*****************************************************************************
*  structure for a peak record
*****************************************************************************/
typedef struct pk_struct {
        double          f1, f2;
        double          amp;
        double          f1_min, f1_max,
                        f2_min, f2_max;
        double          fwhh1, fwhh2;
        double          vol;
        int             key;
        struct pk_struct *next;
        char            label[LABEL_LEN+1];
        char            comment[COMMENT_LEN+1];
        } peak_struct;

/*****************************************************************************
*  structure for peak table
*       First entry in header is the number of peaks in the table, next
*       entries tell whether the corresponding key is in use (PEAK_FILE_FULL)
*       or not (PEAK_FILE_EMPTY). (i.e. header[20] tells whether a peak with
*       key 20 is currently in existence.)
*****************************************************************************/
typedef struct {
        int   num_peaks;
        FILE *file;
        float version;
        peak_struct     *head;
        short   header[MAXNUMPEAKS+1];
        char            f1_label,f2_label;
        int experiment;
        int planeno;
        } peak_table_struct;
 
static	peak_table_struct       *peak_table = NULL;
static	peak_struct		*peak;
static  int			within_diffusion_boundaries(int);
extern void     delete_peak_table(/*peak_table*/);

char		rubbish[1024];
/*************************************

	 makeslice()

**************************************/
int makeslice(int argc, char *argv[], int retc, char *retv[])
{
int		i,j,k,l,m,n,h_min_pt,h_max_pt,v_min_pt,v_max_pt;
int		trace_number_in_block,ending_block;
float		*inp,*buffer;
dpointers	inblock;
int		data_in_block[MAXBLOCKS];

/* MANCHESTER VERSION 6.1 14 vii 98 GAM */
/* This function present in file ll2d.c */

#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "w"); /* file for debugging information */
  fprintf (debug, "Start of makeslice\n");
  fclose(debug);
#endif

/* initialization bits */
/*
Wclear_text();
*/
disp_index(0);
Wsettextdisplay("clear");
if (init_makeslice(argc,argv))
	ABORT;

#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "a"); /* file for debugging information */
  fprintf (debug, "After init_makeslice\n");
  fclose(debug);
#endif

for (i=0;i<MAXBLOCKS;i++)	data_in_block[i] = FALSE;

m = 0;
h_min_pt = h_max_pt = 0;
/* Start of the loop to evaluate the blocks in which the various peaks are */ 
for (n = 0; n < peak_table->num_peaks; n++)
	{
	if (pk[m].num == peak->key)
		{

	/* Calculate the limits of peak region, always as follows (does not depend
	on trace value) because working with data file */


      		v_min_pt = ll2d_frq_to_dp(peak->f1_min,sw1,fn1_int*f1pts);
      		if (peak->f1_min > ll2d_dp_to_frq((double)v_min_pt,sw1,fn1_int*f1pts))
       			v_min_pt--;
      		v_max_pt = ll2d_frq_to_dp(peak->f1_max,sw1,fn1_int*f1pts);
      		if (peak->f1_max < ll2d_dp_to_frq((double)v_max_pt,sw1,fn1_int*f1pts))
       			v_max_pt++;
      		h_min_pt = ll2d_frq_to_dp(peak->f2_min,sw,fn_int);
      		if (peak->f2_min > ll2d_dp_to_frq((double)h_min_pt,sw,fn_int))
       			h_min_pt--;
      		h_max_pt = ll2d_frq_to_dp(peak->f2_max,sw,fn_int);
      		if (peak->f2_max < ll2d_dp_to_frq((double)h_max_pt,sw,fn_int))
       			h_max_pt++;
		pk[m].h_min_pt = h_min_pt;
		pk[m].h_max_pt = h_max_pt;
		pk[m].v_min_pt = v_min_pt;
		pk[m].v_max_pt = v_max_pt;
	/* find out in which block(s) the peak lives */
		pk[m].block = (int)(h_max_pt/fidhead.ntraces);
		if (!data_in_block[pk[m].block])	data_in_block[pk[m].block] = TRUE;
		ending_block = (int)(h_min_pt/fidhead.ntraces);
		pk[m].status = NOT_DONE;
		if (ending_block == pk[m].block)	pk[m].two_blks = FALSE;
		else
			{
			pk[m].two_blks = TRUE;
			if (!data_in_block[pk[m].block+1]) data_in_block[pk[m].block+1] = TRUE;
			}
		m++;
		}
	peak = peak->next;
	}

/* allocate memory for the temporary data buffer */
if ((buffer = (float *)malloc(sizeof(float)*fidhead.ntraces*fidhead.np)) == 0)
	{
           Werrprintf("makeslice: could not allocate memory\n");
	   ABORT;
	}

for (n = 0; n < fidhead.nblocks; n++)
	{
	disp_index(n+1);
	/* First get the data buffer ...  */
	if ( (r = D_getbuf(D_DATAFILE, fidhead.nblocks, n, &inblock)) )
    		{
        	D_error(r);
     		}
	inp = (float *)inblock.data;
	/* ... and zero 'inp' while keeping a copy in 'buffer' */
	for (j = 0; j < fidhead.np*fidhead.ntraces; j++)
                {
                buffer[j] = inp[j];
		inp[j] = 0.0;
                }

	if (data_in_block[n])	/* If some peaks of interest in the block */
		{
		m = 0;
		peak = peak_table->head;
		for (l = 0; l < peak_table->num_peaks; l++)
			{
			if (pk[m].num == peak->key)	/* found one peak */
				{
				if (pk[m].block == n && pk[m].status != DONE)
					{
					if (pk[m].status == NOT_DONE)
						{
						if (pk[m].two_blks == FALSE)
							{
							h_max_pt = pk[m].h_max_pt;
							h_min_pt = pk[m].h_min_pt;
							}
						if (pk[m].two_blks == TRUE)
							{
							h_max_pt = pk[m].h_max_pt;
							h_min_pt = ((n+1)*fidhead.ntraces)-1;
							}
						}
					else if (pk[m].status == PARTIAL)
						{
						h_max_pt = (n*fidhead.ntraces);
						h_min_pt = pk[m].h_min_pt;
						}
					if (display_mode == 's')
						{
						for (i = h_max_pt; i <= h_min_pt; i++)
							{
							trace_number_in_block = (n > 0 ? (i % (n*fidhead.ntraces)) : i);
							for (j = 0; j <= pk[m].v_min_pt-pk[m].v_max_pt; j++)
								{
								k = j+(trace_number_in_block)*fidhead.np+pk[m].v_max_pt;
								inp[k] = buffer[k];
								}
							}
						}
					else if (display_mode == 'i')
						{
						for (i = h_max_pt; i <= h_min_pt; i++)
                                                	{
                                                	trace_number_in_block = (n > 0 ? (i % (n*fidhead.ntraces)) : i);                                          
                                                	for (j = 0; j <= pk[m].v_min_pt-pk[m].v_max_pt; j++)
                                                        	{
                                                        	k = j+(trace_number_in_block)*fidhead.np+pk[m].v_max_pt;
                /* The signal intensity is multiplied by the (part integral / full integral) ratio */
                                                        	inp[k] = pk[m].intensity*buffer[k];
                                                        	}
                                                	}
						}
					if (pk[m].two_blks == FALSE)	pk[m].status = DONE;
					if (pk[m].two_blks == TRUE)
						{
						if (pk[m].status == PARTIAL)	pk[m].status = DONE;
						else if (pk[m].status == NOT_DONE)
							{
							pk[m].status = PARTIAL;
							(pk[m].block)++;
							}
						}
					}
				m++;
				}
			peak = peak->next;
			}
		}
	if ( (r = D_markupdated(D_DATAFILE,n)) )
		{
		D_error(r);
		ABORT;
		}
	if ( (r = D_release(D_DATAFILE,n)) )
		{
		D_error(r);
		ABORT;
		}
	}
delete_peak_table(&peak_table);
free(buffer);

start_from_ft=TRUE;
releasevarlist();
appendvarlist("dconi");

Wsetgraphicsdisplay("dconi");

disp_index(0);
RETURN;
}

/*---------------------------------------
|					|
|		init_makeslice()		|
|					|
+--------------------------------------*/
static int init_makeslice(int argc, char *argv[])
{
char	path1[MAXPATHL],diffname[MAXPATHL];
short	status_mask;
int	i;
FILE	*diffusion_file;
double	integ_step;
extern	int	read_peak_file(/*peak_table,filename*/);


#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "a"); /* file for debugging information */
  fprintf (debug, "At start of init_makeslice\n");
  fclose(debug);
#endif
if (
   P_getreal(PROCESSED,"fn",&fn,1)	||
   P_getreal(CURRENT,"sw",&sw,1)	||
   P_getreal(PROCESSED,"fn1",&fn1,1)	||
   P_getreal(CURRENT,"sw1",&sw1,1)
   )
		{
                Werrprintf("makeslice: Error accessing parameters\n");
                return(ERROR);
                }
/* PB: The following lines have been added to check the number of arguments to makeslice */
#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "a"); /* file for debugging information */
  fprintf (debug, "fn = %f\n",fn);
  fprintf (debug, "sw = %f\n",sw);
  fprintf (debug, "fn1 = %f\n",fn);
  fprintf (debug, "sw1 = %f\n",sw1);
  fprintf (debug, "After reading parameters in init_makeslice\n");
  fclose(debug);
#endif

if((argc<3)||(argc>4)){
		Werrprintf("makeslice: incorrect number of arguments \n");
		Werrprintf("makeslice('<mode>(optional)',start,finish)\n");
                return(ERROR);
		}
if(argc==3) {
		display_mode='i'; 
		start_diff = atof(argv[1]);
		finish_diff = atof(argv[2]);
	}	

else {
	if((argv[1][0]!='i')&&(argv[1][0]!='s')){
		Werrprintf("makeslice: if 'mode' is specified, it must be supplied as first argument!");
		return(ERROR);
		}
		display_mode = argv[1][0];
		start_diff = atof(argv[2]);
		finish_diff = atof(argv[3]);
	}

if(start_diff>finish_diff) swap(&start_diff,&finish_diff);
mean_diff = 0.5*(start_diff+finish_diff);
diff_window = finish_diff-start_diff;


/* initialise the data files in the present experiment ... */
if ( (r = D_gethead(D_DATAFILE, &fidhead)) )
	{
      	if (r == D_NOTOPEN)
      		{
/*		Wscrprintf("spectrum had to be re-opened?\n"); */
         	strcpy(path1, curexpdir);
         	strcat(path1, "/datdir/data");
         	r = D_open(D_DATAFILE, path1, &fidhead); /* open the file */
      		}
      	if (r)
      		{
         	D_error(r);
 	        return(ERROR);
      		}
  	}

#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "a"); /* file for debugging information */
  fprintf (debug, "After reading header in init_makeslice\n");
  fclose(debug);
#endif
f1pts=1;
status_mask = (S_COMPLEX);
if ((fidhead.status & status_mask) == status_mask ) f1pts=2;
status_mask = (S_HYPERCOMPLEX);
if ((fidhead.status & status_mask) == status_mask ) f1pts=4;
status_mask = (S_DATA|S_SPEC|S_FLOAT|S_SECND);
if ( (fidhead.status & status_mask) != status_mask )
	{
#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "a"); /* file for debugging information */
  fprintf (debug, "Status is %d in init_makeslice\n",fidhead.status);
  fclose(debug);
#endif
     	Werrprintf("No 2D spectrum available, please use undosy3D or do 2D transform before using makeslice\n");
     	return(ERROR);
  	}
#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "a"); /* file for debugging information */
  fprintf (debug, "Before initialising peak table in init_makeslice\n");
  fclose(debug);
#endif
fn_int = (int) fn;
fn1_int = (int) fn1;
/* Initialise the peak table */
strcpy(path1,curexpdir);
strcat(path1,"/ll2d/peaks.bin");
if (read_peak_file(&peak_table,path1))
	{
	Wscrprintf("makeslice: Could not read ll2d peak file !\n");
	delete_peak_table(&peak_table);
	return(ERROR);
	}

#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "a"); /* file for debugging information */
  fprintf (debug, "Opening diffusion_display_3d.inp file in init_makeslice\n");
  fclose(debug);
#endif
peak = peak_table->head;
strcpy(diffname,curexpdir);
strcat(diffname,"/dosy/diffusion_display_3D.inp");
if ((diffusion_file=fopen(diffname,"r"))==NULL)
	{
	Werrprintf("Error opening %s file\n",diffname);
	return(ERROR);
	}
#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "a"); /* file for debugging information */
  fprintf (debug, "Reading diffusion_display_3d.inp file in init_makeslice\n");
  fclose(debug);
#endif

/* Read in the diffusion information, and check whether peaks are in the diffusion band */
i=0;
integ_step = diff_window/(2.0*(double)NINTSTEPS);	/* the calculation is divided in NINTSTEPS steps */
/*Wscrprintf("\n\tDiffusion range visible: %.2lf +/- %.2lf\n\n",mean_diff,0.5*diff_window);*/
Wscrprintf("\n\tDiffusion range visible: from %.2lf to %.2lf (*10e-10m2s-1)\n\n",start_diff,finish_diff);
while(fscanf(diffusion_file,"%d %lf %lf\n",&pk[i].num,&pk[i].diff_coef,&pk[i].std_dev)!=EOF && i<MAXNUMPEAKS)
	{
#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "a"); /* file for debugging information */
  fprintf (debug, "Reading peak %d in init_makeslice\n",i);
  fclose(debug);
#endif
	if (display_mode == 's')
		{
		if (within_diffusion_boundaries(i))
			{
			if (compute_integral(i,integ_step) == TRUE)	i++;
			}
		}
	else if (display_mode == 'i')
		{
		r = compute_integral(i,integ_step);
		/* If less than 1 % of intensity, then assume 0 */
        	if (pk[i].intensity >= 0.01)
               		{
		/* If at least 95 % of the intensity within, then assume 100 % */
                	if (pk[i].intensity >= 0.95) pk[i].intensity = 1.0;
                	i++;
                	}
		}
	}
if (display_mode == 's')
	{
	/* Check that the last peak IS within the slice because it does not get eliminated by the above loop */
	if (!within_diffusion_boundaries(i))
		{
		pk[i].num = 0;
		pk[i].diff_coef = 0.0;
		pk[i].std_dev = 0.0;
		}
	else if (compute_integral(i,integ_step) != TRUE)
		{
		pk[i].num = 0;
		pk[i].diff_coef = 0.0;
		pk[i].std_dev = 0.0;
		}
	}
if (display_mode == 'i')
	{
	/* Check the final peak */
	if (pk[i-1].intensity < 0.01)
		{
		pk[i-1].num = 0.0;
		pk[i-1].diff_coef = 0.0;
		pk[i-1].std_dev = 0.0;
		pk[i-1].intensity = 0.0;
		}
	}
#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "a"); /* file for debugging information */
  fprintf (debug, "Before closing diffusion_display_3d.inp file in init_makeslice\n");
  fclose(debug);
#endif
fclose(diffusion_file);
if (i == MAXNUMPEAKS)
	{
	Wscrprintf("number of peaks reduced to %d\n",MAXNUMPEAKS);
	}
#ifdef DEBUG_MAKESLICE
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_makeslice");
  debug = fopen (rubbish, "a"); /* file for debugging information */
  fprintf (debug, "After reading peak file in init_makeslice\n");
  fclose(debug);
#endif

/*   Set PHASFILE status to !S_DATA - this is required to
   force a recalculation of the display from the new data
   in DATAFILE (in the ds routine, see proc2d.c)          */

if ( (r = D_gethead(D_PHASFILE,&phasehead)) )
	{
	if (r == D_NOTOPEN)
		{
/*		Wscrprintf("phas NOTOPEN\n"); */
		strcpy(path1,curexpdir);
		strcat(path1,"/datdir/phasefile");
		r = D_open(D_PHASFILE,path1,&phasehead);
		}
	if (r)
		{
		D_error(r);
		return(ERROR);
		}
	}
phasehead.status = 0;
if ( (r = D_updatehead(D_PHASFILE, &phasehead)) )
	{
	D_error(r);
	Wscrprintf("PHASE updatehead\n");
	return(ERROR);
	}

return(COMPLETE);
}

static int	within_diffusion_boundaries(int index)
{
if (pk[index].diff_coef >=  (start_diff)
	&& pk[index].diff_coef <= (finish_diff))	return(TRUE);
else	return(FALSE);
}

/*
The function 'compute_integral' calculates the integral of exp{-(x/sigma)^2}
It returns TRUE as soon as the value of the integral between the diffusion . . .
. . . slice reaches (CERTAINTY*full_integral)
It returns FALSE if the integral value in the slice is less than (CERTAINTY*full_integral)
*/

static int compute_integral(int index, double step)
{
register int i;
int	max_steps;
double	function_value,part_integral,full_integral,limit_value,old_int,delta,diffusion_difference;

delta = 0.00005;
max_steps = display_mode == 'i' ? 5*NINTSTEPS : 3*NINTSTEPS;
/* initialise the full integral with the value of the function at zero */
full_integral = 1.0;
old_int = 0.0;
/*	
First the "full" integral (over 3 ('s' mode) or 5 ('i' mode) times the slice width) is calculated
It is considered that if the change in full integral from one loop
to the next is less than (100*delta) per cent, than full integral calculation is complete
*/
for (i=0;i<max_steps && ((full_integral-old_int)/old_int) > delta;i++)
	{
	old_int = full_integral;
	function_value = ((double)(i+1)*step)/pk[index].std_dev;
	function_value = exp(-1.0*SQR(function_value));
	full_integral += 2.0*function_value;
	}

/*
Now can calculate the part within the slice width.
*/

/* initialise the integral with the value of the function at centre of diffusion slice */
diffusion_difference = mean_diff - pk[index].diff_coef;
function_value = diffusion_difference/pk[index].std_dev;
function_value = exp(-1.0*SQR(function_value));
part_integral = function_value;
/*
Start the integration loop, it is terminated when NINTSTEPS is reached, . . .
. . . or when the integral is larger than (full_integral*CERTAINTY)
*/
if (display_mode == 's')
	{
	limit_value = CERTAINTY*full_integral;
	for (i=0;i<NINTSTEPS && part_integral <= limit_value;i++)
		{
		function_value = (diffusion_difference+(double)(i+1)*step)/pk[index].std_dev;
		function_value = exp(-1.0*SQR(function_value));
		part_integral += function_value;
		function_value = (diffusion_difference-(double)(i+1)*step)/pk[index].std_dev;
		function_value = exp(-1.0*SQR(function_value));
		part_integral += function_value;
		}
	if (part_integral > limit_value)	return(TRUE);
	else					return(FALSE);
	}
else if (display_mode == 'i')
	{
	for (i=0;i<NINTSTEPS;i++)
       		{
       		function_value = (diffusion_difference+(double)(i+1)*step)/pk[index].std_dev;
       		function_value = exp(-1.0*SQR(function_value));
       		part_integral += function_value;
       		function_value = (diffusion_difference-(double)(i+1)*step)/pk[index].std_dev;
       		function_value = exp(-1.0*SQR(function_value));
       		part_integral += function_value;
       		}
	pk[index].intensity = part_integral/full_integral;
	return(TRUE);
	}
   return(FALSE);
}

void swap(double *a, double *b) {
	double temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

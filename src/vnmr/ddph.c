/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************************************************** 
* ddph.c  -  display data phase file                                  * 
* command(args):                                                      * 
*   wrspec(output_file [, trace number ] )                            * 
*     write numeric text file using an element from current phased    * 
*     spectrum							      * 
*   wrspec(output_file, trace number, 0 )			      * 
*     write text file of phase of spectrum, using one element         * 
*     (values of rp and lp are ignored)			              * 
*   wrspec(output_file, trace 1, trace 2 )			      * 
*     calculate and write file of difference between two phase files  * 
*     phi(2) - phi(1)						      * 
*   wrspec(output_file, trace 1, trace 2, 0)			      * 
*     calculate the inverse sum of the inverses of two amplitude      * 
*     files 1/(1/a+1/b) from phased data  (error in phi(2)-phi(1))    * 
*     -NOT to be described to users				      *
*   wrspec(output_file, trace 1, trace 2, -1)			      * 
*     calculate the inverse sum of the inverses of two amplitude      * 
*     files 1/(1/a+1/b) from complex data (error in phi(2)-phi(1))    * 
*     -NOT to be described to users				      *
*   wrspec(output_file, trace 1, trace 2, -2 )			      * 
*     calculate and write file of phase difference times amplitude    *
*     between two files (phi(2) - phi(1)) * 1/(1/a+1/b)		      * 
*     -NOT to be described to users				      *
*   wrspec(output_file, trace 1, trace 2, trace 3, trace 4 )          * 
*     calculate and write file of difference of the difference of     * 
*     two pairs of phase files (generate phase map with reference)    * 
*     (phi(4)-phi(3))-(phi(2)-phi(1))				      * 
*   wrspec(output_file, trace 1, trace 2, -3)			      *
*     calculate and write out a whole display file for sets of        *
*     phase difference differences for large array                    *
*     (shim maps for gradient shimming)				      *
*     values of trace 1 and trace 2 are irrelevant ( != 0 )           *
*     -NOT to be described to users				      *
**********************************************************************/

/*   wrspec with variable numbers of arguments for above functions     */
/*   use rotate2() for phasing - not used		               */
/*   dc correction done on both x and y before phase calculation       */
/*   if output_file='ds' then put into ds spectrum display             */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>		/*  to define iswspace macro */
#include <unistd.h>		/*  bits for 'access' in SVR4 */
#include <string.h>

#include "vnmrsys.h"
#include "group.h"		/* contains CURRENT, for P_getreal */
#include "data.h"
#include "tools.h"
#include "variables.h"
#include "init2d.h"
#include "pvars.h"
#include "wjunk.h"

#ifdef UNIX
#include <sys/file.h>		/*  bits for `access', e.g. F_OK */
#else 
#define	F_OK		0	/* does file exist */
#define	X_OK		1	/* is it executable by caller */
#define	W_OK		2	/* writable by caller */
#define	R_OK		4	/* readable by caller */
#endif 

#ifdef  DEBUG
extern int debug1;
extern int Eflag;
#define EPRINT0(level, str) \
        if (Eflag >= level) fprintf(stderr,str)
#define EPRINT1(level, str, arg1) \
	if (Eflag >= level) fprintf(stderr,str,arg1)
#define EPRINT2(level, str, arg1, arg2) \
        if (Eflag >= level) fprintf(stderr,str,arg1,arg2)
#else 
#define EPRINT0(level, str)	/* ??? */
#define EPRINT1(level, str, arg1)
#define EPRINT2(level, str, arg1, arg2)
#endif 

#define sqrtf(axx)  ((float) sqrt((double)axx))
#define atan2f(axx,ayy)  ((float) atan2( (double)axx,(double)ayy))

#define  TRUE		1
#define  FALSE		0

extern int     Bnmr;  /* background flag */
extern int  rel_data();

static float  *phase_data;  
static float  *spectrum;    
static int     updateflag;

extern dfilehead datahead;    
static int wrspec_args(int argc, char *argv[], int *proctype,
       int *elem1_num, int *elem2_num, int *elem3_num, int *elem4_num );
static int wrspec_calcdata( char *cmd_name, int proctype, int elem1_num,
       int elem2_num, int elem3_num, int elem4_num, float *vdata);
static int wrspec_output( char *cmd_name, char *outfile_name, FILE *tfile,
            float *vdata, int proctype, float epscale, float *rms );
static void do_expl_out(char xaxis[], char yaxis[], float xmin, float xmax,
                        float ymin, float ymax, int setnum);
int calc_avg_scale(char *cmd_name, char *file_name );
int wrspec_find_epscale(float *epscale, int proctype, int elem1_num, int elem2_num );


#define		MAXNEW_ARGS	6
#define 	MINNEW_ARGS	2
#define		WRFILE_ARG	1
#define		NEW_FIRST_PARSED_ARG	2
#define		NEW_SECOND_PARSED_ARG	3
#define		NEW_THIRD_PARSED_ARG	4
#define		NEW_FOURTH_PARSED_ARG	5
#define		NEW_AMP_TOL		1e-10	/* amplitude tolerance */

int wrspec(int argc, char *argv[], int retc, char *retv[] )
{
	char		*cmd_name, *outfile_name;
	char		 ow_prompt[ MAXPATHL + 20 ], ow_ans[ 4 ];
	int		 elem1_num, elem2_num, elem3_num, elem4_num,
			 i, ival, np_out;
	int		 proctype; /* proctype for processing function type */
	float		*vdata, /* output data */
			 epscale = 0.0,
			 rms = 0.0;
	double		 swmagic, tmp;
	FILE		*tfile=NULL; 

/*	retv[0]=0 ok; retv[0]=1 failed */

        cmd_name = argv[ 0 ];
	if (argc < MINNEW_ARGS)		/* compare MAXNEW_ARGS in wrspec_args */
	{
                Werrprintf( "Usage:  %s( 'file_name' <, trace number > )", cmd_name);
        	if (retc>0) retv[0] = intString(1);
        	return( 0 );
        }
	outfile_name = argv[ WRFILE_ARG ];        /* address of (output) file name: argv[1] */

/* do avg_scale if asked for it */
	if (strcmp(outfile_name, "expl_avg_scale")==0) {
		if (argc < 3) {
        		if (retc>0) retv[0] = intString(1);
        		return( 0 );
			}
		if (calc_avg_scale( cmd_name, argv[2] ) != 0) {
			/* errors reported in function */
        		if (retc>0) retv[0] = intString(1);
        		return( 0 );
			}
        	if (retc>0) retv[0] = intString(0);
        	return( 0 );
		}

	init2d(1,1); /* get data headers */
	if ((datahead.status & S_SPEC) == 0)
		{
		Werrprintf("%s:  data must be ft'd first", cmd_name); 
        	if (retc>0) retv[0] = intString(1);
        	return( 0 );
		} 

	if (wrspec_args( argc, argv, &proctype, &elem1_num, &elem2_num, &elem3_num, &elem4_num ) != 0)
		{
        	if (retc>0) retv[0] = intString(1);
        	return( 0 );
		/* `writefid_args' reports error */  /* depends on argc */
		}
	EPRINT0(1,"calling wrspec...\n");
	EPRINT1(1,"  wrspec: proctype %d, ", proctype);
	EPRINT2(1,"trace numbers %d %d ",elem1_num,elem2_num);
	EPRINT2(1,"%d %d\n",elem3_num,elem4_num);

/*	if (proctype == 8) { outfile_name="ds"; for dmg; use outfile_name="dmg" and proctype=2?
		 } else { below; } */

        vdata=(float *)malloc((fn/2) * sizeof(float));
        if (!vdata) { 
		Werrprintf( "ddph: memory allocation failure" );
        	if (retc>0) retv[0] = intString(1);
        	return( 0 );
		}

	if ( strcmp( outfile_name, "ds") != 0) {

/*  Handle situation where the output file is already present.  */
        if (access( outfile_name, F_OK ) == 0) {
                if (!Bnmr) {
                        sprintf( &ow_prompt[ 0 ], "OK to overwrite %s? ", outfile_name);
                        W_getInputCR( &ow_prompt[ 0 ], &ow_ans[ 0 ], sizeof( ow_ans ) - 1);
                        if (ow_ans[ 0 ] != 'Y' && ow_ans[ 0 ] != 'y') {
        			free((char*) vdata);
                                Winfoprintf( "%s:  operation aborted", cmd_name );
        			if (retc>0) retv[0] = intString(1);
        			return( 0 );
                        }
                }
                ival = unlink( outfile_name );
                if (ival != 0) {
        		free((char*) vdata);
                        Werrprintf( "%s:  cannot remove %s", cmd_name, outfile_name );
        		if (retc>0) retv[0] = intString(1);
        		return( 0 );
                }
        }
        tfile = fopen( outfile_name, "w" );   /* open file */
        if (tfile == NULL) {
        	free((char*) vdata);
                Werrprintf( "%s:  problem opening %s", cmd_name, outfile_name );
                if (retc>0) retv[0] = intString(1);
                return( 0 );
                }
		} /* end "not ds" mode */

	if (proctype==2 || proctype==4 || proctype==5 || proctype==7) {
	if (wrspec_find_epscale( &epscale, proctype, elem1_num, elem2_num) != 0) {
		Werrprintf( "%s:  find epscale failed", cmd_name);
        	free((char*) vdata);
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		}
		}

	if (proctype != 5) {
	if (wrspec_calcdata(cmd_name, proctype, elem1_num, elem2_num, elem3_num, elem4_num, vdata) != 0)
		{
        	free((char*) vdata);
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		/* calcdata reports error */
		}
	if (wrspec_output( cmd_name, outfile_name, tfile, vdata, proctype, epscale, &rms) != 0)
		{
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		/* write output to file or 'ds' */
		}
	}
	else { /* proctype == 5 */

	if ( (ival=P_getreal(CURRENT, "gzwin", &swmagic, 1)) ) {
		swmagic = 100.0;
		}
	else {
		if (swmagic < 0.0) swmagic =  -swmagic; 
		tmp = 100.0 * 1.0/((float) (fn/2-1));
		if (swmagic < tmp) swmagic = tmp; 
		if (swmagic > 100.0) swmagic = 100.0;
	}
        swmagic /= 100.0;
	np_out = fn/2 - 2 * (int) ((1.0 - ((float) swmagic)) * ((float) (fn/4)) );

	ival = ((datahead.nblocks + 1) / 2) - 2; /* ival is gzsize, number of z shims */

	if ( epscale < NEW_AMP_TOL ) {
		fprintf( tfile, "exp 4\n  %d  %d\nFrequency (hz) vs Phase\n", 
			ival, np_out );
			}
		else {
		fprintf( tfile, "exp 4\n  %d  %d\nFrequency (hz) vs Field (hz)\n",
			ival, np_out ); 
			}
	for (i=0; i<ival; i++) {
	    fprintf( tfile, "\n%d  0  0  0\n", i+1 );
	    elem3_num = 2 * i + 3;
	    elem4_num = 2 * i + 4;
	    if (wrspec_calcdata(cmd_name, 4, elem1_num, elem2_num, elem3_num, elem4_num, vdata) != 0)
		{
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		/* calcdata reports error; use proctype=4 */
		}
	    rms = (float) elem3_num;
	    if (wrspec_output( cmd_name, outfile_name, tfile, vdata, proctype, epscale, &rms) != 0)
		{
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		/* write output to file or 'ds' */
		}
	}

	}

	if (strcmp( outfile_name, "ds") != 0)   fclose( tfile ); 
	free((char*) vdata);

	if (retc>0) retv[0] = intString(0);
	if (retc>1) retv[1] = realString( ((double)rms) );

	EPRINT0(1,"wrspec done!\n");
	return( 0 );
}


static int wrspec_args(int argc, char *argv[], int *proctype,
       int *elem1_num, int *elem2_num, int *elem3_num, int *elem4_num )
{
	int	ival;

/*	if (*elem1_num == -8) { *elem1_num = *elem2_num = *elem3_num = *elem4_num = 0;
				 *proctype = 8; return(0); for dmg output; } */

	*proctype = 0;  /* proctype=0 wrspec, 1 wrphase, 2 writeb0, 3 wrspecav, 4 wrcalb0 */
		/* proctype=5 wrcalb0 of array, 6 wrspecav with absval, 7 writeb0 with wt */
	*elem1_num = 1;	
	*elem2_num = 0;
	*elem3_num = 0;
	*elem4_num = 0;  	/* default values */

	if (argc == MINNEW_ARGS )
	  return( 0 );
	else if (argc > MAXNEW_ARGS)
	  argc = MAXNEW_ARGS;

	if (isReal( argv[ NEW_FIRST_PARSED_ARG ] )) {
		ival = atoi( argv[ NEW_FIRST_PARSED_ARG ] );
		if (ival < 1) {
			Werrprintf( "%s:  invalid trace number %d", argv[ 0 ], ival);
			return( -1 ); 
			}
		*elem1_num = ival;
		}
	else {
                Werrprintf( "Usage:  %s( 'file_name' <, trace number > )", argv[ 0 ]);
		return( -1 );
		}

	if (argc > NEW_SECOND_PARSED_ARG) {
        if (isReal( argv[ NEW_SECOND_PARSED_ARG ] )) {
                ival = atoi( argv[ NEW_SECOND_PARSED_ARG ] );
                if (ival < 1) { *proctype = 1; argc = NEW_SECOND_PARSED_ARG + 1; }	/* wrphase */
		else { *proctype = 2; } 						/* writeb0 */
                *elem2_num = ival;
        	}
        else {
                Werrprintf( "Usage:  %s( 'file_name' <, trace number > )", argv[ 0 ]);
                return( -1 );
		}
	}

	if (argc > NEW_THIRD_PARSED_ARG) {
        if (isReal( argv[ NEW_THIRD_PARSED_ARG ] )) {
                ival = atoi( argv[ NEW_THIRD_PARSED_ARG ] );
                if (ival < 1) {
			if (ival == -1) *proctype = 6;		/* wrspecav with absval */
			else if (ival == -2) *proctype = 7;	/* writeb0 with weighting */
			else if (ival == -3) *proctype = 5;	/* wrcalb0 with array */
			else *proctype = 3;			/* wrspecav */
			argc = NEW_THIRD_PARSED_ARG + 1;
			}
                *elem3_num = ival;
        	}
        else {
                Werrprintf( "Usage:  %s( 'file_name' <, trace number > )", argv[ 0 ]);
                return( -1 );
        	}
	}

	if (argc > NEW_FOURTH_PARSED_ARG) {
        if (isReal( argv[ NEW_FOURTH_PARSED_ARG ] )) {
                ival = atoi( argv[ NEW_FOURTH_PARSED_ARG ] );
                if (ival < 1) {
                        Werrprintf( "%s:  invalid trace number %d", argv[ 0 ], ival);
                        return( -1 );
                	}
                *elem4_num = ival;
		*proctype = 4; /* wrcalb0 */
        	}
        else {
                Werrprintf( "Usage:  %s( 'file_name' <, trace number > )", argv[ 0 ]);
                return( -1 );
        	}
	}

	if ( *proctype == 0 || *proctype == 1 ) {
		if ( *elem1_num > datahead.nblocks ) {
			Werrprintf( "%s: invalid trace number %d", argv[0], *elem1_num );
			return( -1 );
			}
		}
	else if ( *proctype == 2 || *proctype == 3 || *proctype == 6 ) { 
		if (datahead.nblocks < 2) {
			Werrprintf( "%s: invalid file, contains %d blocks", argv[0], datahead.nblocks );
			return( -1 );
			}
		if ( *elem1_num > datahead.nblocks ) {
			Werrprintf( "%s: invalid trace number %d", argv[0], *elem1_num);
			return( -1 );
			}
                if ( *elem2_num > datahead.nblocks || *elem1_num == *elem2_num ) {
                        Werrprintf( "%s: invalid trace number %d", argv[0], *elem2_num );
                        return( -1 );
                        }
		}
	else if ( *proctype == 4 ) {
		if (datahead.nblocks < 4) {
			Werrprintf( "%s: invalid file  contains %d blocks", argv[0], datahead.nblocks );
			return( -1 );
			}
                if ( *elem1_num > datahead.nblocks ) {
                        Werrprintf( "%s: invalid trace number %d", argv[0], *elem1_num);
                        return( -1 );
                        }
                if ( *elem2_num > datahead.nblocks ) {
                        Werrprintf( "%s: invalid trace number %d", argv[0], *elem2_num);
                        return( -1 );
                        }
                if ( *elem3_num > datahead.nblocks ) {
                        Werrprintf( "%s: invalid trace number %d", argv[0], *elem3_num);
                        return( -1 );
                        }
                if ( *elem4_num > datahead.nblocks ) {
                        Werrprintf( "%s: invalid trace number %d", argv[0], *elem4_num);
                        return( -1 );
                        }
	        if (*elem1_num == *elem2_num || *elem1_num == *elem3_num || *elem1_num == *elem4_num
	         || *elem2_num == *elem3_num || *elem2_num == *elem4_num || *elem3_num == *elem4_num) {
			Werrprintf( "%s: trace numbers must all be different", argv[0] );
			return( -1 );
			}
		}
	else if ( *proctype == 5 ) {
		if (datahead.nblocks < 4) {
			Werrprintf( "%s: invalid file  contains %d blocks", argv[0], datahead.nblocks );
			return( -1 );
			}
		if ( *elem1_num > datahead.nblocks ) { 
			Werrprintf( "%s: invalid trace number %d", argv[0], *elem1_num);
			return( -1 );	/* this check is irrelevant, but will prevent user error */
			} 
                if ( *elem2_num > datahead.nblocks ) {
                        Werrprintf( "%s: invalid trace number %d", argv[0], *elem2_num );
                        return( -1 );	/* this check is irrelevant, but will prevent user error */
                        } 
		*elem1_num=1; *elem2_num=2; *elem3_num=3; *elem4_num=4;
		}

	return( 0 );
}

/*
 *  returns 0 if successful, -1 if error.
 *  reports error if it returns -1.
 */



static int wrspec_calcdata( char *cmd_name, int proctype, int elem1_num, int elem2_num, int elem3_num, int elem4_num, float *vdata) 
{
	float	 x1, z1, ph1, x2, z2, ph2,
		 x3, z3, ph3, x4, z4, ph4,
		 ph1diff=0.0, ph2diff, phdiffdiff,
		 ph_old, ph_cen, ph_cut=270.0; 
	float	 radtodeg = 180.0/M_PI;
        int      i, npxx;
	float	*phase1, *phase2, *phase3, *phase4;
	float	 dcx, dcy; /* baseline correction for phased data */
	double	 rp_val, lp_val;

	npxx = fn;

/*	if (proctype == 8) { obtain data from dmg; ft; put result into 1/2 vdata; } else { below } */

	/*   proctype=0 wrspec, 1 wrphase, 2 writeb0, 3 wrspecav, 4 wrcalb0 */
        if ((spectrum = calc_spec(elem1_num-1,0,FALSE,TRUE,&updateflag))==0) 
        {
           Werrprintf("%s: calc_spec spectrum %d not found", cmd_name, elem1_num); 
           return(-1);
        }
        else if ((phase_data = get_data_buf(elem1_num-1))==0) 
        { 
           Werrprintf("%s: phase_data spectrum %d not found", cmd_name, elem1_num);
           return(-1);     
        } 
        phase2 = phase3 = phase4 = NULL;
	if ( proctype == 0 || proctype == 3 ) {
		phase1=(float *)malloc((npxx/2) * sizeof(float)); /* change to allocateWithId */
		if (!phase1) { Werrprintf( "%s: memory allocation failure", cmd_name ); return(-1); }
                for (i = 0; i < npxx/2; phase1[i] = spectrum[i], i++)
                   ; 
/*		for (i=0, dcx=0.0; i < 4; dcx += (phase1[i] + phase1[npxx/2-i-1]), i++); 
		dcx /= 8.0;
		printf( "#1: dcx %g\n", dcx);
		for (i=0; i < npxx/2; phase1[i] -= dcx, i++); */
		EPRINT0(2, "    wrspec: #1 amp done\n");
		}
	else {
		phase1=(float *)malloc((npxx) * sizeof(float)); 
		if (!phase1) { Werrprintf( "%s: memory allocation failure", cmd_name ); return(-1); }
/*		if (proctype==8) { phase1[i] = dmg_data[i] } else { below } */
		for (i = 0; i < npxx; phase1[i] = phase_data[i], i++)
                   ; 
		/* take average of first four and last four points in x and y, baseline correct */
		for (i=0, dcx=0.0, dcy=0.0; i < 8; 
			dcx += (phase1[i] + phase1[npxx-2*i-2]), 
			dcy += (phase1[i+1] + phase1[npxx-2*i-1]),
			i += 2);
		dcx /= 8.0; dcy /= 8.0;
		EPRINT2(2, "    wrspec: #1 dcx %g, dcy %g\n", dcx, dcy); 
		for (i=0; i<npxx; phase1[i] -= dcx, phase1[i+1] -= dcy, i += 2);
		}
	if ((rel_data()) != 0) {
		Werrprintf("%s:  spectrum data memory error", cmd_name);
		ABORT;
		}

	if ( proctype > 1 ) { /* use for writeb0, wrspecav, wrcalb0 */
/*	if (proctype==8) { obtain data from dmg } else { below } */
        if ((spectrum = calc_spec(elem2_num-1,0,FALSE,TRUE,&updateflag))==0)
        {
           Werrprintf("%s: calc_spec spectrum %d not found", cmd_name, elem2_num);
           return(-1);
        }
        else if ((phase_data = get_data_buf(elem2_num-1))==0)
        {
           Werrprintf("%s: phase_data spectrum %d not found", cmd_name, elem2_num);
           return(-1);
        } 
	if ( proctype == 3) {
                phase2=(float *)malloc((npxx/2) * sizeof(float));
                if (!phase2) { Werrprintf( "%s: memory allocation failure", cmd_name ); return(-1); }
                for (i = 0; i < npxx/2; phase2[i] = spectrum[i], i++)
                   ;
/*              for (i=0, dcx=0.0; i < 4; dcx += (phase2[i] + phase2[npxx/2-i-1]), i++);
                dcx /= 8.0;
                printf( "#2: dcx %g\n", dcx); 
                for (i=0; i < npxx/2; phase2[i] -= dcx, i++); */
		EPRINT0(2,"    wrspec: #2 amp done\n");
		}
	else {
        	phase2=(float *)malloc((npxx) * sizeof(float));
        	if (!phase2) { Werrprintf( "%s: memory allocation failure", cmd_name ); return(-1); }
/*		if (proctype==8) { phase2[i] = ... } else { below } */
        	for (i = 0; i < npxx; phase2[i] = phase_data[i], i++)
                   ; 
                for (i=0, dcx=0.0, dcy=0.0; i < 8;
                        dcx += (phase2[i] + phase2[npxx-2*i-2]),
                        dcy += (phase2[i+1] + phase2[npxx-2*i-1]),
                        i += 2);
                dcx /= 8.0; dcy /= 8.0;
		EPRINT2(2, "    wrspec: #2 dcx %g, dcy %g\n", dcx, dcy); 
                for (i=0; i<npxx; phase2[i] -= dcx, phase2[i+1] -= dcy, i += 2);
		}
	if ((rel_data()) != 0) {
		Werrprintf("%s:  spectrum data memory error", cmd_name);
		ABORT;
		}
	} /* end proctype > 1 */

	if (proctype == 4) { /* use only for wrcalb0 */
        if ((spectrum = calc_spec(elem3_num-1,0,FALSE,TRUE,&updateflag))==0)
        {
           Werrprintf("%s: calc_spec spectrum %d not found", cmd_name, elem3_num);
           return(-1);
        }
        else if ((phase_data = get_data_buf(elem3_num-1))==0)
        {
           Werrprintf("%s: phase_data spectrum %d not found", cmd_name, elem3_num);
           return(-1);
        } 
        phase3=(float *)malloc((npxx) * sizeof(float));
        if (!phase3) { Werrprintf( "%s: memory allocation failure", cmd_name); return(-1); }
        for (i = 0; i < npxx; phase3[i] = phase_data[i], i++)
           ;
                for (i=0, dcx=0.0, dcy=0.0; i < 8;
                        dcx += (phase3[i] + phase3[npxx-2*i-2]),
                        dcy += (phase3[i+1] + phase3[npxx-2*i-1]),
                        i += 2);
                dcx /= 8.0; dcy /= 8.0;
                EPRINT2(2, "    wrspec: #3 dcx %g, dcy %g\n", dcx, dcy); 
                for (i=0; i<npxx; phase3[i] -= dcx, phase3[i+1] -= dcy, i += 2);
	if ((rel_data()) != 0) {
		Werrprintf("%s:  spectrum data memory error", cmd_name);
		ABORT;
		}

        if ((spectrum = calc_spec(elem4_num-1,0,FALSE,TRUE,&updateflag))==0)
        {
           Werrprintf("%s: calc_spec spectrum %d not found", cmd_name, elem4_num);
           return(-1);
        }
        else if ((phase_data = get_data_buf(elem4_num-1))==0)
        {
           Werrprintf("%s: phase_data spectrum %d not found", cmd_name, elem4_num);
           return(-1);
        } 
        phase4=(float *)malloc((npxx) * sizeof(float));
        if (!phase4) { Werrprintf( "%s: memory allocation failure", cmd_name ); return(-1); }
        for (i = 0; i < npxx; phase4[i] = phase_data[i], i++)
           ;
                for (i=0, dcx=0.0, dcy=0.0; i < 8;
                        dcx += (phase4[i] + phase4[npxx-2*i-2]),
                        dcy += (phase4[i+1] + phase4[npxx-2*i-1]),
                        i += 2);
                dcx /= 8.0; dcy /= 8.0;
                EPRINT2(2, "    wrspec: #4 dcx %g, dcy %g\n", dcx, dcy); 
                for (i=0; i<npxx; phase4[i] -= dcx, phase4[i+1] -= dcy, i += 2);
	if ((rel_data()) != 0) {
		Werrprintf("%s:  spectrum data memory error", cmd_name);
		ABORT;
		}
	} /* end proctype == 4 */

/*	Calculate difference in phase differences, then write to file */
/*	assume data is REAL_NUMBERS (float)	*/

	if ( proctype==1 || proctype==2 || proctype==4 ) {	/* || proctype==8 */
        	for ( i=0; 2 * i < npxx; i++) {    /* calc data */
                        x1 = phase1[ 2 * i ];
                        z1 = phase1[ 2 * i + 1 ];
                        ph1 = atan2f( x1, z1 ) * radtodeg;
			phdiffdiff = ph1;

			if ( proctype > 1) {
                        x2 = phase2[ 2 * i ];
                        z2 = phase2[ 2 * i + 1 ];
                        ph2 = atan2f( x2, z2 ) * radtodeg;
                        ph1diff = ph2 - ph1;
			phdiffdiff = ph1diff;
			}

			if ( proctype == 4) {
			x3 = phase3[ 2 * i ];
			z3 = phase3[ 2 * i + 1 ];
			ph3 = atan2f( x3, z3 ) * radtodeg;
			x4 = phase4[ 2 * i ];
			z4 = phase4[ 2 * i + 1 ];
			ph4 = atan2f( x4, z4 ) * radtodeg;
			ph2diff = ph4 - ph3;
			phdiffdiff = ph2diff - ph1diff;
			}

                        while (phdiffdiff >= 180.0)
                          phdiffdiff -= 360.0;
                        while (phdiffdiff < -180.0)
                          phdiffdiff += 360.0;
			vdata[i] = phdiffdiff;
                	}
		}
	else if ( proctype == 3) { 
		for ( i=0; 2 * i < npxx; i++) {
			x1 = phase1[i]; if ( x1<0 ) x1 = -x1;	/* these are amplitudes, not phases */
			x2 = phase2[i]; if ( x2<0 ) x2 = -x2;
			if ( x1 < NEW_AMP_TOL ) {
				if ( x1 < x2) { x3 = 0.5 * x1; } /* is this what it should be doing? */
				else { x3 = 0.5 * x2; }		 /* 0.5 only if x1 ~ x2 */
				}			 /* this is only used for gshim, usually true */
			else if ( x2 < NEW_AMP_TOL ) {
				x3 = 0.5 * x2; 	/* x1<x2 already checked above */
				}
			else { x3 = 1.0 / ((1.0/x1) + (1.0/x2)); }
			vdata[i] = x3;
			}
		}
	else if ( proctype == 6) { 
		for ( i=0; 2 * i < npxx; i++) {
                        x1 = phase1[ 2 * i ];	
                        z1 = phase1[ 2 * i + 1 ];
                        x2 = phase2[ 2 * i ];
                        z2 = phase2[ 2 * i + 1 ];
			x1 = sqrtf(x1*x1 + z1*z1);	/* calculate amplitudes */
			x2 = sqrtf(x2*x2 + z2*z2);
			if ( x1 < NEW_AMP_TOL ) {
				if ( x1 < x2) { x3 = 0.5 * x1; } /* is this what it should be doing? */
				else { x3 = 0.5 * x2; }		 /* 0.5 only if x1 ~ x2 */
				}			 /* this is only used for gshim, usually true */
			else if ( x2 < NEW_AMP_TOL ) {
				x3 = 0.5 * x2; 	/* x1<x2 already checked above */
				}
			else { x3 = 1.0 / ((1.0/x1) + (1.0/x2)); }
			vdata[i] = x3;
			}
		}
	else if ( proctype == 7) { 
		for ( i=0; 2 * i < npxx; i++) {
                        x1 = phase1[ 2 * i ];		/* calculate phases */
                        z1 = phase1[ 2 * i + 1 ];
                        ph1 = atan2f( x1, z1 ) * radtodeg;
                        x2 = phase2[ 2 * i ];
                        z2 = phase2[ 2 * i + 1 ];
                        ph2 = atan2f( x2, z2 ) * radtodeg;
                        phdiffdiff = ph2 - ph1;
                        while (phdiffdiff >= 180.0)
                          phdiffdiff -= 360.0;
                        while (phdiffdiff < -180.0)
                          phdiffdiff += 360.0;
			vdata[i] = phdiffdiff;
			}
		ph_cen=0.0; ph_old=vdata[0];
	        for (i = 1; i < npxx/2; i++ ) {
	                phdiffdiff = vdata[i] - ph_old;
                	if (phdiffdiff < -ph_cut) { ph_cen += 360.0; }
                	if (phdiffdiff >  ph_cut) { ph_cen -= 360.0; }
                	ph_old = vdata[i];
                	vdata[i] += ph_cen;
			}
		ph_cen = 0.25 * (vdata[npxx/4-3] + vdata[npxx/4-2] + vdata[npxx/4+1] + vdata[npxx/4+2]);
        	for (i=0; i < npxx/2; vdata[i] -= ph_cen, i++); 

/*	should recenter phase with ph_cen BEFORE mult by x3 and z3 ??? */

		z3 = 0.0;
		for ( i=0; 2 * i < npxx; i++) {
                        x1 = phase1[ 2 * i ];		/* calculate phases */
                        z1 = phase1[ 2 * i + 1 ];
                        x2 = phase2[ 2 * i ];
                        z2 = phase2[ 2 * i + 1 ];
			x1 = sqrtf(x1*x1 + z1*z1);	/* calculate amplitudes */
			x2 = sqrtf(x2*x2 + z2*z2);
			if ( x1 < NEW_AMP_TOL ) {
				if ( x1 < x2) { x3 = 0.5 * x1; } /* is this what it should be doing? */
				else { x3 = 0.5 * x2; }		 /* 0.5 only if x1 ~ x2 */
				}			 /* this is only used for gshim, usually true */
			else if ( x2 < NEW_AMP_TOL ) {
				x3 = 0.5 * x2; 	/* x1<x2 already checked above */
				}
			else { x3 = 1.0 / ((1.0/x1) + (1.0/x2)); }
			vdata[i] *= x3; 
			if (x3 > z3) z3=x3;
			}
		if (z3 > NEW_AMP_TOL) {
			z3 = 1.0/z3;
			for (i=0; 2*i<npxx; i++) vdata[i] *= z3;  	/* normalize */
			}
		}
	else {		/* proctype==0 */
		for ( i=0; 2 * i < npxx; vdata[i] = phase1[i], i++);
		}

	if ( proctype == 1) {
        	if ( P_getreal(CURRENT, "rp", &rp_val, 1) ) {
			rp_val = 0.0;
		}
        	if ( P_getreal(CURRENT, "lp", &lp_val, 1) ) {
			lp_val = 0.0;
		}
		for (i=0; i<npxx/2; i++) {
			vdata[i] += (float)rp_val - (float)((lp_val/2.0) * i) / (float)(npxx/4); 
		}
        	}

        free((char*) phase1);
	if ( proctype > 1 ) {
        	free((char*) phase2); 
		}
	if ( proctype == 4) {
        	free((char*) phase3);
        	free((char*) phase4); 
		}

        return( 0 );
}


/* cmd_name      calling command name */
/* outfile_name  output file name */
/* tfile	 pointer to file */
/* vdata  	 data points */
/* proctype      function type */
/* epscale	 scale for eps */
/* rms		 rms average of data */
static int wrspec_output( char *cmd_name, char *outfile_name, FILE *tfile,
            float *vdata, int proctype, float epscale, float *rms )
{
	float	 phdiffdiff, swstep, swvalue, eps_tmp;
	double	 swmagic, phmagic, tmp;
        int      ival, npstart, npfin, j, npxx, epscale_flag;
        float    ph_cut, ph_int, ph_offset, ph_old, ph_cen;

	npxx = fn;

	if ( (ival=P_getreal(CURRENT, "gzwin", &swmagic, 1)) ) {
		swmagic = 100;
		}
	if (swmagic < 0) { swmagic =  -swmagic; }
	tmp = 100.0 / ((float)(npxx/2-1));
	if (swmagic < tmp) { swmagic = tmp; }
        if (swmagic > 100.0) {
                swmagic = 100;
                }
        swmagic /= 100.0;		/* could keep gzwin, set everthing outside to zero */

	phmagic = 270.0;
        if ( (P_getreal(CURRENT, "wrspec_phcut", &phmagic, 1)) ) {
		phmagic = 270;
		if ((proctype==1) && (ival != 0)) phmagic=361;	/* do not use phmagic */
        }
        ph_cut = (float) phmagic;
	if (ph_cut < 0) { ph_cut = -ph_cut; }
	if (ph_cut < 30) { ph_cut = 270; } 
	EPRINT2(1,"  wrspec: gzwin %g, phcut %g\n", (float)(100 * swmagic), ph_cut);

/*      assume data is REAL_NUMBERS (float)     */

        swvalue = -(sw/2.0) * ((float) (npxx/2-1))/((float) (npxx/2));
        swstep = sw / ((float) (npxx/2));   /* same denominator */
        npstart = (int) ( (1.0 - ((float) swmagic)) * ((float) (npxx/4)) );
        npfin = npxx/2 - npstart;
        swvalue += swstep * (0.5 + (float)(npstart));

/*	if (proctype==0) swvalue += (rflrfp - sw/2); */

/* 	put in checks - $phcut related to $npts, may be too few points?? */
/*      ph_cut = 270.0; see above */
        ph_int = 0.0;
        ph_offset = 0.0;

        eps_tmp = epscale;
	if (eps_tmp < 0.0) { eps_tmp = -eps_tmp; } 
        if (eps_tmp < NEW_AMP_TOL) { epscale_flag = 0; }
        else { epscale_flag = 1; }

	if ( strcmp( outfile_name, "ds") != 0) { 	/* using file mode */

	if (proctype == 1) {
                fprintf( tfile,
                        "exp 4\n  1  %d\nFrequency (hz) vs Phase\n\n1  0  0  0\n",
                        (npfin - npstart) );
		}
	else if (proctype == 2 || proctype == 7) {
		if ( epscale_flag != 0) {
                        fprintf( tfile,
                                "exp 4\n  1  %d\nFrequency (hz) vs Field (hz)\n\n1  0  0  0\n",
                                (npfin - npstart) );

			}
		else {
                	fprintf( tfile,
                        	"exp 4\n  1  %d\nFrequency (hz) vs Phase\n\n1  0  0  0\n",
                        	(npfin - npstart) );
			}
		}
	else if (proctype == 4) {
		if ( epscale_flag != 0) {
                        fprintf( tfile,
                                "exp 4\n  1  %d\nFrequency (hz) vs Field (hz)\n\n1  0  0  0\n",
                                (npfin - npstart) );
			}
		else {
			fprintf( tfile, 
				"exp 4\n  1  %d\nFrequency (hz) vs Phase\n\n1  0  0  0\n",
				(npfin - npstart) ); 
			}
		}
	else if (proctype != 5) {
                fprintf( tfile,
                        "exp 4\n  1  %d\nFrequency (hz) vs Amplitude\n\n1  0  0  0\n",
                        (npfin - npstart) );
		}

	if ((proctype==1 || proctype==2 || proctype==4 || proctype==5) && ph_cut < 360.0) {
		ph_old = vdata[npstart];
	        for (j = npstart + 1; j < npfin; j++ ) {
	                phdiffdiff = vdata[j] - ph_old;
                	if (phdiffdiff < -ph_cut) { ph_offset += 360.0; }
                	if (phdiffdiff >  ph_cut) { ph_offset -= 360.0; }
                	ph_old = vdata[j];
                	vdata[j] += ph_offset;
                	ph_int += vdata[j];
                	}

		if (((float) phmagic >= 0.0) && ((float)swmagic < (1.0-tmp/100.0))) {
		  if (proctype==4 || proctype==5) {
	     	    ph_cen = 0.25 * (vdata[npxx/4-2] + vdata[npxx/4+1]);
/*		    ph_cen += 0.25 * (vdata[npstart] + vdata[npfin-1]); */
/*		    ival = (int)( 0.01 * ((float)(npfin - npstart)) );
	     	    ph_cen = 0.25 * (vdata[npxx/4-2] + vdata[npxx/4+1]);
		    ph_cen += 0.25 * (vdata[npstart + ival] + vdata[npfin - 1 - ival]); */
                    ph_cen += 0.05 * (vdata[npstart+0] + vdata[npfin-1]);
                    ph_cen += 0.05 * (vdata[npstart+1] + vdata[npfin-2]);
                    ph_cen += 0.05 * (vdata[npstart+2] + vdata[npfin-3]);
                    ph_cen += 0.05 * (vdata[npstart+3] + vdata[npfin-4]);
                    ph_cen += 0.05 * (vdata[npstart+4] + vdata[npfin-5]);
		  }
		  else
		  {
		    ph_cen = 0.25 * (vdata[npxx/4-3] + vdata[npxx/4-2] + vdata[npxx/4+1] + vdata[npxx/4+2]);
		  }
		    EPRINT1(1,"  wrspec: b0 recentered by %g degrees\n", ph_cen);
        	    for (j=npstart; j < npfin; j++ ) { vdata[j] -= ph_cen; }
		    }
		else { EPRINT0(1,"  wrspec: b0 not recentered\n"); } 
        if ((epscale_flag != 0) && (proctype==2 || proctype==4 || proctype==5)) {
                for (j = npstart; j < npfin; j++) vdata[j] *= epscale; 
                }

		} /* end proctype == 1, 2, 4 */

	if (proctype==7 && ph_cut < 360.0)
                for (j = npstart; j < npfin; j++) vdata[j] *= epscale; 

	for (j=npstart; j < npfin; j++ ) {
		fprintf( tfile, "%g  %g\n", swvalue, vdata[j] );
		swvalue += swstep;
		}

	} /* end != "ds" */

	else { Winfoprintf( "%s:  using ds mode", cmd_name ); 
	EPRINT0(1,"  wrspec: using ds mode");
	ph_cen = 360.0;
        if ((proctype==1 || proctype==2 || proctype==4 || proctype==5) && ph_cut < 360.0) {
		ph_old = vdata[0];
                for (j = 1; j < npxx/2; j++ ) {	 	/* use all data */
                        phdiffdiff = vdata[j] - ph_old;
                        if (phdiffdiff < -ph_cut) { ph_offset += 360.0; }
                        if (phdiffdiff >  ph_cut) { ph_offset -= 360.0; }
                        ph_old = vdata[j];
                        vdata[j] += ph_offset;
                        ph_int += vdata[j];
                        }
	        if ((epscale_flag != 0) && (proctype==2 || proctype==4 || proctype==5)) {
                	for (j = 0; j < npxx/2; j++) { vdata[j] *= epscale; }
			ph_cen *= epscale;	/* min must be less than this */ 
			}
		} 
	for (j = npstart; j < npfin; j++) {	/* use only points in window */
		if (vdata[j] < ph_cen) { ph_cen = vdata[j]; }
		}
	for (j=0; j < npxx/2; j++ ) { vdata[j] -= ph_cen; }
	EPRINT1(1, "  wrspec: b0 recentered by %g degrees\n", ph_cen ); 

/*	if (proctype == 8) { write into dgm_spectrum[j]; } else { below }	*/
	ival = 0;
	if (proctype==5) ival = ((int)(*rms+1.5))/2-2;
        if ((spectrum = calc_spec(ival,0,FALSE,TRUE,&updateflag))==0) /* set spectrum ival+1 */
        {
           Werrprintf("%s: spectrum %d not found", cmd_name, ival+1 );
           return(-1);
        }
        for (j=0; j<npxx/2; spectrum[j] = vdata[j], j++); /* output to ds spectrum dislay */
                                                          /* sent to last spectrum calculated */
	} /* end "ds" mode */

	*rms = 0.0;
	for (j=npstart; j < npfin; j++ ) *rms += vdata[j] * vdata[j];
	*rms = sqrtf( *rms / (float)(npfin - npstart) );

        return( 0 ); 
}

int wrspec_find_epscale(float *epscale, int proctype, int elem1_num, int elem2_num )
{
	float		*epsdelay, deps1, deps2;
	double		 eps_tmp;
	vInfo		 info;
	int		 i;

        (void) proctype;
/*	if (proctype == 6) { *epscale = 0.0; else do below; } */
/*	calc scale from eps */
/*	note only valid for proctype == 2 || proctype == 4 || proctype == 5 */
	*epscale = 0.0;	/* *epscale=0 translates to no scaling - see wrspec_output */
	if ( P_getVarInfo(CURRENT, "d3", &info) ) {
		*epscale = 0.0; 
	}
        else {  
		epsdelay=(float *)malloc((info.size) * sizeof(float));
		for( i=0; i< (int) info.size; i++ ) {
                        P_getreal(CURRENT, "d3", &eps_tmp, i+1);
                        epsdelay[i] = (float) eps_tmp; 
			}
		*epscale = 1.0;
		deps1 = epsdelay[(elem1_num - 1) % info.size]; 
		deps2 = epsdelay[(elem2_num - 1) % info.size]; 
		deps1 = deps2 - deps1;
		deps2 = deps1;
		if ( deps2 < 0 ) { deps2 = -deps2; }
		if ( deps2 < NEW_AMP_TOL ) { *epscale = 0.0; }
		else { *epscale = 1.0/(360.0 * deps1); }		/* true value */
		free((char *) epsdelay);
		}
	EPRINT1(1, "  wrspec: *epscale %f\n", *epscale ); 
/*	if (*epscale < 0.0) { *epscale = -1.0 * *epscale; } */

	return(0);
}


/* overwrite expl.out to scale for input file, set scalelimits from avg endpoints */
int calc_avg_scale(char *cmd_name, char *file_name )
{
  char 	curexplout[MAXPATHL], jstr[MAXPATHL], setnum[MAXPATHL];
  FILE	*tfile;
  int	ma=0, np=0, ival, i, j;
  float *fbase, ph, freq, freq1=0.0, freq2=0.0; 
  float	xmin, xmax, ymin, ymax, sc, wc, sc2, wc2;
  double tmp;

	if (P_getstring( GLOBAL, "curexp", curexplout, 1, MAXPATHL )!=0) {
		Werrprintf("%s:  parameter 'curexp' not found", cmd_name );
		return(-1);
		}
	strcat(curexplout, "/expl.out");
	if (P_getreal( CURRENT, "sc", &tmp, 1 )!=0) {
		Werrprintf("%s:  parameter 'sc' not found", cmd_name );
		return(-1);
		}
	else sc = (float)tmp;
	if (P_getreal( CURRENT, "wc", &tmp, 1 )!=0) {
		Werrprintf("%s:  parameter 'wc' not found", cmd_name );
		return(-1);
		}
	else wc = (float)tmp;
	if (P_getreal( CURRENT, "sc2", &tmp, 1 )!=0) {
		Werrprintf("%s:  parameter 'sc2' not found", cmd_name );
		return(-1);
		}
	else sc2 = (float)tmp;
	if (P_getreal( CURRENT, "wc2", &tmp, 1 )!=0) {
		Werrprintf("%s:  parameter 'wc2' not found", cmd_name );
		return(-1);
		}
	else wc2 = (float)tmp;

/*  Handle situation where input file is not present.  */
        if (access( file_name, F_OK ) != 0) {
                Werrprintf( "%s:  file %s not found", cmd_name, file_name );
                return( -1 );
                }
        tfile = fopen( file_name, "r" );   /* open file */
        if (tfile == NULL) {
                Werrprintf( "%s:  problem opening file %s", cmd_name, file_name );
                return( -1 );
                }
/* read input file */
        for (i=0; i<4; i++) {
                if ( fscanf( tfile, "%s", jstr) == EOF) {
                        Werrprintf("%s: invalid format for file %s", cmd_name, file_name);
                        return(-1);
                        }
                switch (i) {
                        case 2 : if (isReal(jstr)) ma = atoi( jstr ); break;
                        case 3 : if (isReal(jstr)) np = atoi( jstr ); break;
                        default : ;
                        }
                }
        if ((np < 1) || (ma < 1)) {
                Werrprintf("%s:  invalid number of points in file %s", cmd_name, file_name);
                return(-1);
                }
        do {
                if (fscanf( tfile, "%s", jstr) == EOF) strcpy(jstr,"errflag");
                }       /* fscanf for string until data start */
                while ( (strcmp(jstr,"1") != 0) && (strcmp(jstr,"errflag") != 0) );
        if (strcmp(jstr,"errflag") == 0) {
                Werrprintf("%s: invalid format for file %s", cmd_name, file_name );
                return(-1);
                }
        for (i=0; i<3; i++) {
                if ((fscanf( tfile, "%s", jstr) == EOF) || (strcmp(jstr,"0") != 0)) {
                        Werrprintf("%s: invalid format for file %s", cmd_name, file_name);
                        return(-1);
                        }
                }

/* malloc fbase, read file */
        fbase=(float *)malloc((np * ma) * sizeof(float));
        if (!fbase) {
                Werrprintf( "%s: memory allocation failure", cmd_name );
                return( -1 );
                }
        for (i = 0; i < (np * ma);  i++ )  fbase[i] = 0.0;
        for (j = 1; j <= ma; j++ )
                {
                if (j > 1) for (i = 0; i < 4; i++) {
                        if (fscanf( tfile, "%s", jstr ) == EOF) {
                                Werrprintf("%s:  end of file %s reached", cmd_name, file_name);
				free((char*) fbase); fclose(tfile);
                                return(-1);
                                }
                        }
		ival = (j-1) * np;
                if ((fscanf( tfile, "%f", &freq)==EOF) || (fscanf( tfile, "%f", &ph)==EOF))
			{
                	Werrprintf("%s:  end of file %s reached", cmd_name, file_name);
			free((char*) fbase); fclose(tfile);
                	return(-1);
			}
		fbase[ ival ] = ph;
		freq1 = freq;
        	for (i=1; i < (np-1); i++) {
                	if ((fscanf( tfile, "%f", &freq)==EOF) || (fscanf( tfile, "%f", &ph)==EOF))
				{
                        	Werrprintf("%s:  end of file %s reached", cmd_name, file_name);
				free((char*) fbase); fclose(tfile);
                        	return(-1);
				}
                	else fbase[ival + i] = ph;
                	}
        	if ((fscanf( tfile, "%f", &freq)==EOF) || (fscanf( tfile, "%f", &ph)==EOF))
			{
                	Werrprintf("%s:  end of file %s reached", cmd_name, file_name);
			free((char*) fbase); fclose(tfile);
                	return(-1);
			}
		fbase[ ival + np-1 ] = ph;
		freq2 = freq;
                }
	fclose( tfile );

/*  calculate new limits */
/*
        endpt=(float *)malloc((3 * ma) * sizeof(float));
        if (!endpt) {
                Werrprintf( "%s: memory allocation failure", cmd_name );
		free((char *)fbase);
                return( -1 );
                }
	for (j=0; j<ma; j++) {
		ival = j * np;
		endpt[ 3*j+0 ] = 0.2 * (fbase[ival+0] + fbase[ival+1] + fbase[ival+2] + fbase[ival+3] + fbase[ival+4]);
		ival = j * np + np/2;
		endpt[ 3*j+1 ] = 0.25 * (fbase[ival-2] + fbase[ival-1] + fbase[ival] + fbase[ival+1]);
		ival = (j+1) * np;
		endpt[ 3*j+2 ] = 0.2 * (fbase[ival-1] + fbase[ival-2] + fbase[ival-3] + fbase[ival-4] + fbase[ival-5]);
		}
	if (freq1 < freq2) { xmin=freq1; xmax=freq2; }
	else { xmax=freq1; xmin=freq2; }

	ymin=endpt[0]; ymax=endpt[0];
	for (j=1; j < (3*ma); j++) {
		if (endpt[j] < ymin) ymin=endpt[j];
		if (endpt[j] > ymax) ymax=endpt[j];
		}
	free((char *)fbase);
	free((char *)endpt);
*/

	ymin=fbase[5]; ymax=fbase[5];
	for (j=1; j<=ma; j++)
		{
		ival = (j-1) * np;
		for (i=5; i < np-5; i++)  /* ignore first five, last five points */
			{
			if (fbase[ival + i] < ymin)  ymin=fbase[ival + i];
			if (fbase[ival + i] > ymax)  ymax=fbase[ival + i];
			}
		}
	free((char *)fbase);
	if (freq1 < freq2) { xmin=freq1; xmax=freq2; }
	else { xmax=freq1; xmin=freq2; }

	ph = xmax - xmin;
	xmin = xmin - 0.1 * ph;
	xmax = xmax + 0.1 * ph;
	ph = ymax - ymin;
	ymin = ymin - 0.1 * ph;
	ymax = ymax + 0.1 * ph;
	if ((ymax-ymin) < 10.0) {
		ph = 0.5 * (ymax + ymin);
		ymax = ph + 5.0;
		ymin = ph - 5.0;
		}

	strcpy( setnum, "0" );

          do_expl_out("linear","linear",xmin,xmax,ymin,ymax, 0);

/*  Remove expl.out file if already present.  */
/**        if (access( curexplout, F_OK ) == 0) {  **/
/*  open expl.out, check FLAG; follow scalelimits macro */
/*		ufile = fopen( curexplout, "r");
		if (ufile == NULL) {
			Werrprintf( "%s: problem opening %s", cmd_name, curexplout);
			return( -1 );
		}
		j = 0; 
		for (i=0; j==0, i<11; i++)
		    if ( fscanf(ufile, "%s", jstr) == EOF) {
			j = 1;
			break;
			}
		if ((j == 0) && (strcmp(jstr, "FLAGS") == 0)) {
		    for (i=0; j==0, i<3; i++)
			if ( fscanf(ufile, "%s", jstr) == EOF) {
			    j = 1;
			    break;
			}
		    if (j == 0)
			strcpy( setnum, jstr );
		}
		fclose( ufile );
*/
/**                ival = unlink( curexplout );
                if (ival != 0) {
                        Werrprintf( "%s:  cannot remove %s", cmd_name, curexplout );
                        return( -1 );
                }
        }
        ufile = fopen( curexplout, "w" ); **/  /* open file */
/**        if (ufile == NULL) {
                Werrprintf( "%s:  problem opening file %s", cmd_name, curexplout );
                return( -1 );
                } **/
/* write curexplout file */
/**	fprintf( ufile, "%f   %f   %f   %f\n", sc, wc, sc2, wc2);
	fprintf( ufile, "%f   %f   linear\n", xmin, xmax);
	fprintf( ufile, "%f   %f   linear\nFLAG 0 0\n%s\n", ymin, ymax, setnum);
	fclose( ufile ); **/

	return(0);
}

/*************************************************/
static void do_expl_out(char xaxis[], char yaxis[], float xmin, float xmax,
                        float ymin, float ymax, int setnum)
/*************************************************/
/*      save information about expl box and scales      */
{
  char  filename[MAXPATHL];
  FILE *datafile;

  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/expl.out");
#else 
  strcat(filename,"expl.out");
#endif 
  if ( (datafile=fopen(filename,"w+")) )
  {
    fprintf(datafile,"%f   %f   %f   %f\n",sc,wc,sc2,wc2);
    fprintf(datafile,"%f   %f   %s\n",xmin,xmax,xaxis);

    fprintf(datafile,"%f   %f   %s\n",ymin,ymax,yaxis);
    fprintf(datafile,"%s %d %d\n","FLAG",0,0); /* this is only difference in dll.c */
    fprintf(datafile,"%d\n",setnum);
    fclose(datafile);
  }
  else Werrprintf("Unable to open '%s'",filename);
}

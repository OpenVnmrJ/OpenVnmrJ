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
/****************************************************************/
/*								*/
/*	######  ######           #####  ######			*/
/*	#     # #     #         #     # #     #			*/
/*	#     # #     #               # #     #			*/
/*	######  ######           #####  #     #			*/
/*	#     # #                     # #     #			*/
/*	#     # #               #     # #     #			*/
/*	######  #       #######  #####  ######			*/
/*								*/
/****************************************************************/
/*								*/
/*	authors:	Martin Staemmler			*/
/*			Harald Niederlaender			*/
/*								*/
/*	Institute for biomedical Engineering (IBMT)		*/
/*	D - 66386 St. Ingbert, Germany				*/
/*	Ensheimerstrasse 48					*/
/*	Tel.: (+49) 6894 980251					*/
/*	Fax:  (+49) 6894 980400					*/
/*								*/
/****************************************************************/
/*								*/
/*	date:		15.02.92				*/
/*	revision:	initial release				*/
/*	1.01   mst	error variable image file name fixed	*/
/*	1.02   mst      fixed setting of [phi,theta]start and   */
/*                      [phi,theta]end cancelled		*/
/*      1.03   mst      messages from \b to '.' changed         */
/*								*/
/****************************************************************/
#include	"bp.h"
/* 1 = intermediate files will be deleted / 0 = not deleted */
#define		AUTO_DELETE	1
#define		PAR_PRINT	0

extern FILE *create_3d_file();

main (argc,argv)
int	argc;
char	*argv[];
{
/*
 * step 0:  set up parameter for 3d BP using the file given
 *          on the command line
 *	make n_theta 2D reconstructions for the first cascade
 *	in the cascaded 3D reconstruction approach
 * step 1:   initialization for first step cascaded 3D BP
 * step 2: filter n_theta profile slices
 * step 3:   backproject all n_theta profile slices
 * step 4:   free profile and delete unnecessary files
 *	make i_size 2D reconstructions for the second cascade
 *	in the cascaded 3D reconstruction approach
 * step 5:   initialization for second step cascaded 3D BP
 * step 6:   filter i_size profile slices
 * step 7:   backproject all i_size profile slices
 * step 8:   free profile, delete files
 */
char		*par_file;
char		*par_string;
char		cmd[MAX_PAR_STRING];
int		n_prof, n_slices;
int		*p_image_int;
int		in_memory_prof;
int		i;
int		log2_size;
int		min_int, max_int;
int		last;
double		theta;
double		limit;
int		exp_of_2[LOG2_MAX_SIZE+1];
float		*p_prof_float_start, *p_prof_float;
float		*p_prof_float_end,  *p_prof_float_act;
float		*p_filt_float, *p_filt_float_act;
float		sine[MAX_SIZE/4+1], tangens[MAX_SIZE/4+1];
float		min, max;
double		offset_r;
struct	boundary	*bound;
struct	datafilehead	fhp, fhf1, fhm, fhf2, fhi;
struct	datablockhead	bhp, bhf1, bhm, bhf2, bhi;
BP_PAR			bp;
BP_CTRL			bpc;
FILE		*file_out;


/*
 * step 0:  set up parameter for 3d BP using the file given
 *          on the command line
 */
if (argc == 1) {
    printf ("usage: bp_3d commandfile\n");
    exit (0);
    }
printf ("bp_3d  ---  15.3.1994  ---  updated release\n\n");
par_file = (char *)malloc ((unsigned)strlen(argv[1]));
strcpy (par_file,argv[1]);
par_string = (char *)malloc ((unsigned) MAX_PAR_STRING);

/* step 0.1: start accessing the essential parameter */
/* step 0.1.1: profile file name */
get_parameter_string (par_file,"prof_file",par_string);
if (strlen(par_string) == 0) { 
    fprintf (stderr,"BP_3D: profile file name not found\n");
    fprintf (stderr,"usage: bp_3d commandfile\n");
    exit (0);
    }
else { 
    bp.prof_file = (char *)malloc ((unsigned)strlen(par_string));
    strcpy (bp.prof_file, par_string);
    }
/* step 0.1.2: measurement size */
get_parameter_string (par_file,"m_size",par_string);
if (strlen(par_string) == 0) { 
    fprintf (stderr,"BP_3D: warning: measurement size not found\n");
    bp.m_size = 0;
    }
else { 
    bp.m_size = atoi (par_string);
    }
/* step 0.1.3: image size */
get_parameter_string (par_file,"i_size",par_string);
if (strlen(par_string) == 0) { 
    fprintf (stderr,"BP_3D: warning: image size not found\n");
    bp.i_size = 0;
    }
else { 
    bp.i_size = atoi (par_string);
    }
/* step 0.1.4: number of projection in theta abd phi */
get_parameter_string (par_file,"n_theta",par_string);
if (strlen(par_string) == 0) { bp.n_theta = 0; }
else                         { bp.n_theta = atoi(par_string); }
get_parameter_string (par_file,"n_phi",par_string);
if (strlen(par_string) == 0) { bp.n_phi = 0; }
else                         { bp.n_phi = atoi(par_string); }

/* step 0.2: access the optional parameter */
/* step 0.2.1: parameter discribing the filter function */
get_parameter_string (par_file,"in_memory_prof",par_string);
if (strlen(par_string) == 0) { in_memory_prof = FALSE; }
else                         { in_memory_prof = atoi(par_string); }

/* step 0.2.2: parameter dsicribing the filter function */
get_parameter_string (par_file,"filter_name",par_string);
if (strlen(par_string) == 0) { 
    bp.filter_name = (char *)malloc ((unsigned)strlen("band_pass0"));
    strcpy (bp.filter_name,"band_pass"); 
    }
else {
    bp.filter_name = (char *)malloc ((unsigned)strlen(par_string));
    strcpy (bp.filter_name,par_string);
    }
get_parameter_string (par_file,"filter_bw",par_string);
if (strlen(par_string) == 0) { bp.filter_bw = 1.0; }
else                         { bp.filter_bw = atof (par_string); }
    
/* step 0.2.3: filenames for intermediate and final results */
get_parameter_string (par_file,"meta_image",par_string);
if (strlen(par_string) == 0) { 
    bp.meta_image = (char *)malloc ( (unsigned) (strlen(bp.prof_file)
                                           + strlen(".meta") + 1) );
    sprintf (bp.meta_image, "%s.meta",bp.prof_file);
    }
else { 
    bp.meta_image = (char *)malloc ((unsigned)strlen(par_string));
    strcpy (bp.meta_image, par_string);
    }
if (in_memory_prof == FALSE) {
    get_parameter_string (par_file,"prof_filt1",par_string);
    if (strlen(par_string) == 0) { 
        bp.prof_filt1 = (char *)malloc ( (unsigned) (strlen(bp.prof_file)
                                           + strlen(".filt1") + 1) );
        sprintf (bp.prof_filt1, "%s.filt1",bp.prof_file);
        }
    else { 
        bp.prof_filt1 = (char *)malloc (strlen(par_string));
        strcpy (bp.prof_filt1, par_string);
        }

    get_parameter_string (par_file,"prof_filt2",par_string);
    if (strlen(par_string) == 0) { 
        bp.prof_filt2 = (char *)malloc ( (unsigned) (strlen(bp.prof_file)
                                           + strlen(".filt2") + 1) );
        sprintf (bp.prof_filt2, "%s.filt2",bp.prof_file);
        }
    else { 
        bp.prof_filt2 = (char *)malloc ((unsigned)strlen(par_string));
        strcpy (bp.prof_filt2, par_string);
        }
    }
get_parameter_string (par_file,"image_file",par_string);
if (strlen(par_string) == 0) { 
    bp.image_file = (char *)malloc ( (unsigned) (strlen(bp.prof_file)
                                           + strlen(".000") + 1) );
    sprintf (bp.image_file, "%s.000", bp.prof_file);
    }
else { 
    bp.image_file = (char *)malloc (strlen(par_string) + strlen(".000") + 1);
    sprintf (bp.image_file, "%s.000", par_string);
    }
/* step 0.2.4: get reconstruction size */
get_parameter_string (par_file,"r_size",par_string);
if (strlen(par_string) == 0) { bp.r_size = bp.m_size; }
else                         { bp.r_size = atof (par_string); }

/* step 0.2.5: get measurement center */
get_parameter_string (par_file,"m_center_x",par_string);
if (strlen(par_string) == 0) { bp.m_center_x = bp.m_size/2.0; }
else                         { bp.m_center_x = atof (par_string); }
get_parameter_string (par_file,"m_center_y",par_string);
if (strlen(par_string) == 0) { bp.m_center_y = bp.m_size/2.0; }
else                         { bp.m_center_y = atof (par_string); }
get_parameter_string (par_file,"m_center_z",par_string);
if (strlen(par_string) == 0) { bp.m_center_z = bp.m_size/2.0; }
else                         { bp.m_center_z = atof (par_string); }

/* step 0.2.6: get reconstruction center */
get_parameter_string (par_file,"r_center_x",par_string);
if (strlen(par_string) == 0) { bp.r_center_x = bp.m_size/2.0; }
else                         { bp.r_center_x = atof (par_string); }
get_parameter_string (par_file,"r_center_y",par_string);
if (strlen(par_string) == 0) { bp.r_center_y = bp.m_size/2.0; }
else                         { bp.r_center_y = atof (par_string); }
get_parameter_string (par_file,"r_center_z",par_string);
if (strlen(par_string) == 0) { bp.r_center_z = bp.m_size/2.0; }
else                         { bp.r_center_z = atof (par_string); }

/* step 0.2.7: get angles (start and end) */
get_parameter_string (par_file,"theta_start",par_string);
if (strlen(par_string) == 0) { bp.theta_start = 0.0; }
else                         { bp.theta_start = atof (par_string); }
get_parameter_string (par_file,"theta_end",par_string);
if (strlen(par_string) == 0) { bp.theta_end = bp.theta_start + 180.0; }
else                         { bp.theta_end = atof (par_string); }
get_parameter_string (par_file,"phi_start",par_string);
if (strlen(par_string) == 0) { bp.phi_start = 0.0; }
else                         { bp.phi_start = atof (par_string); }
get_parameter_string (par_file,"phi_end",par_string);
if (strlen(par_string) == 0) { bp.phi_end = bp.phi_start + 180.0; }
else                         { bp.phi_end = atof (par_string); }


/* 
 * step 1:   initialization for first step cascaded 3D BP
 */
/* step 1.1: get profile file fileheader */
/* this is the raw fid file  - mrk */
 if (access_file (bp.prof_file, &fhp, &bhp, TRUE) == ERROR) {
    fprintf (stderr,"BP_3D: can't access profiles\n");
    return (ERROR);
    }
if (fhp.nblocks * fhp.ntraces != bp.n_theta * bp.n_phi) {
    fprintf (stderr,"BP_3D: mismatch data structure %s and n_theta * n_phi\n",
                     bp.prof_file);
    exit (0);
    }
if (bp.m_size == 0) {
    bp.m_size = fhp.np;
    bp.m_center_x = bp.m_center_y = bp.m_center_z = bp.m_size / 2.0;
    bp.r_center_x = bp.r_center_y = bp.r_center_z = bp.m_size / 2.0;
    }
if (bp.r_size == 0) {
    bp.r_size = bp.m_size;
    }
if (bp.i_size == 0) {
    bp.i_size = bp.m_size;
    }
bp.i_center_x = bp.i_center_y = bp.i_center_z = (bp.i_size -1)/ 2.0;
/* step 1.2: allocate memory for profiles */
if (in_memory_prof == TRUE) {
    if ((p_prof_float_start = (float *) malloc ((unsigned) bp.n_theta *
                         bp.n_phi * bp.m_size * sizeof(float))) == 0) {
        fprintf (stderr,"BP_3D: can't allocate profile buffer\n");
        return (ERROR);
        }
    if (read_vnmr_data (bp.prof_file, &fhp, 0,
                         bp.n_theta*bp.n_phi, 1,
                         (char *)p_prof_float_start, FALSE) == ERROR) {
        fprintf (stderr,"BP_3D: can't read to profile buffer\n");
        return (ERROR);
        }
    }
else {
    if ((p_prof_float_start = (float *) malloc ((unsigned) bp.n_phi *
                         bp.m_size * sizeof(float))) == 0) {
        fprintf (stderr,"BP_3D: can't allocate profile buffer\n");
        return (ERROR);
        }
    }
/* step 1.3: allocate memory and generate filter function */
bp.filter_amp = 1.0 / ((float)(bp.m_size * bp.n_phi));
if (gen_filter_float (bp.m_size, (float)bp.m_size/2.0, &p_filt_float, &bp) == ERROR) {
    fprintf (stderr,"BP_3D: can't first generate filter\n");
    return (ERROR);
    }
swap_float (bp.m_size, p_filt_float);

/* step 1.4: allocate memory for boundary table for reconstruction */
if ((bound = (struct boundary *) malloc ((unsigned) bp.i_size * 
             sizeof(struct boundary))) == 0) {
    fprintf (stderr,"BP_3D: can't allocate bound buffer\n");
    return (ERROR);
    }

/* step 1.5: set up coefficients for Hartley transform */
init_fht_float (bp.m_size, &log2_size, exp_of_2, sine, tangens);

/* step 1.6: set initial min/max and pointers, check sizes */
min = MAXFLOAT;
max = MINFLOAT;
offset_r = sqrt ((bp.r_center_x - bp.m_center_x) * (bp.r_center_x - bp.m_center_x) +
                 (bp.r_center_y - bp.m_center_y) * (bp.r_center_y - bp.m_center_y) +
                 (bp.r_center_z - bp.m_center_z) * (bp.r_center_z - bp.m_center_z));
if (bp.r_size > bp.m_size - (int)(offset_r + 0.5)) {
    bp.r_size = bp.m_size - (int)(offset_r + 0.5);
    fprintf (stderr,"WARNING: r_size clipped to maximum value = %d\n",bp.r_size);
    }
bpc.p_size      = bp.m_size;
bpc.i_size      = bp.i_size;
bpc.resize      = (double)bp.r_size / (double) bp.i_size;
bpc.n_proj      = bp.n_phi;
bpc.i_center_x  = bp.i_center_x;
bpc.i_center_y  = bp.i_center_y;
bpc.i_center_z  = bp.i_center_z;
bpc.angle_start = 90.0 + bp.phi_start;
bpc.angle_end   = 90.0 - bp.phi_end;
p_prof_float    = p_prof_float_start; 

/* step 1.7: set up file/block header for filtered profile file */
fhf1.nblocks   = fhp.nblocks;
fhf1.ntraces   = fhp.ntraces;
fhf1.np        = fhp.np;
fhf1.ebytes    = fhp.ebytes;
fhf1.tbytes    = fhp.tbytes;
fhf1.bbytes    = fhp.bbytes;
fhf1.vers_id   = fhp.vers_id;
fhf1.status    = fhp.status;
fhf1.nbheaders = fhp.nbheaders;
bhf1.status    = bhp.status;
bhf1.index     = bhp.index;
bhf1.mode      = bhp.mode;
bhf1.ctcount   = bhp.ctcount;
bhf1.lpval     = bhp.lpval;
bhf1.rpval     = bhp.rpval;
bhf1.lvl       = bhp.lvl;
bhf1.tlt       = bhp.tlt;

/* step 1.8: create filtered profile file if necessary */
if (in_memory_prof == FALSE) {
    if (create_file (bp.prof_filt1, &fhf1,&bhf1) == ERROR) {
        fprintf (stderr,"BP_3D: can't create filtered profile file\n");
        return (ERROR);
        }
    }

/* 
 * step 2: filter n_theta profile slices
 */
printf ("\nfirst filter: 0%%");
for (n_slices=last=0; n_slices<bp.n_theta; n_slices++) {
    if (n_slices * 50 / bp.n_theta > last) {
	last = n_slices * 50 / bp.n_theta;
        printf (".");
        fflush (stdout);
        }
    /* step 2.1: access n_phi profiles of current profile slice */
    if (in_memory_prof == FALSE) {
        if (read_vnmr_data (bp.prof_file, &fhp, n_slices * bp.n_phi,
                                 (n_slices+1) * bp.n_phi, 1,
                                 (char *)p_prof_float, FALSE) == ERROR) {
            fprintf (stderr,"BP_3D: can't read %d profile slice\n",
                     n_slices);
            return (ERROR);
            }
        }
    /* step 2.2: filter profiles with prepared filter function */
    for (n_prof=0; n_prof<bp.n_phi; n_prof++) {
        p_prof_float_act = p_prof_float + n_prof * bp.m_size;
        p_prof_float_end = p_prof_float + (n_prof + 1) * bp.m_size;
        p_filt_float_act = p_filt_float;
        /* step 2.2.1: Hartley transform the n_phi profiles */
        swap_float (bp.m_size, p_prof_float_act);
        fht_float (bp.m_size, log2_size, exp_of_2, sine, tangens, 
                   p_prof_float_act);
        /* step 2.2.2  multiply the HT of the profile with filter */
        for ( ; p_prof_float_act < p_prof_float_end; p_prof_float_act++) {
             *p_prof_float_act *= *p_filt_float_act;
             p_filt_float_act++;
             }
        p_prof_float_act = p_prof_float + n_prof * bp.m_size;
        /* step 2.2.3: Inverse Hartley transform the n_phi profiles */
        fht_float (bp.m_size, log2_size, exp_of_2, sine, tangens, 
                   p_prof_float_act);
        swap_float (bp.m_size, p_prof_float + n_prof * bp.m_size); 
        }
    /* step 2.3: calculate minimum and maximum value for later scaling */
    get_min_max_float (p_prof_float, bp.m_size * bp.n_phi, &min, &max);
    /* step 2.4: data saved in memory or meta file */
    if (in_memory_prof == TRUE) {
        p_prof_float += bp.n_phi * bp.m_size;
        }
    else {
        p_prof_float = p_prof_float_start; 
        if (write_vnmr_data (bp.prof_filt1, &fhf1, n_slices * bp.n_phi,
                              (n_slices + 1) * bp.n_phi, 1,
                              (char *)p_prof_float) == ERROR) {
            fprintf (stderr,"BP_3D: can't write filtered profiles\n");
            return (ERROR);
            }
        } 
    }   /* end of loop for all n_theta profile slices */
printf ("100%%\n");

/* 
 * step 3:   backproject all n_theta profile slices
 */
/* step 3.1: reset pointer */
p_prof_float = p_prof_float_start;

/* step 3.2:allocate memory for reconstructed image */
if ((p_image_int = (int *) malloc ((unsigned) bp.i_size * bp.i_size *
             sizeof(int))) == 0) {
    fprintf (stderr,"BP_3D: can't allocate image buffer\n");
    return (ERROR);
    }
/* step 3.3: set up scale, save it in bp structure */
if (max > -min) { bp.scale1 = bpc.scale = 24576.0/max; }
else            { bp.scale1 = bpc.scale = 24576.0 / (- min); }
printf ("first scale: min=%f, max=%f, scale=%f\n", min,max,bp.scale1);

/* step 3.4: initializations */
min_int = MAXINT;
max_int = - MAXINT;
make_bound (bp.i_size, (double)(bp.i_size-1), 
            bp.i_center_x, bp.i_center_y, bound);

/* step 3.5: set up file/block header */
fhm.nblocks   = bp.n_theta;
fhm.ntraces   = bp.i_size;
fhm.np        = bp.i_size;
fhm.ebytes    = sizeof(float);
fhm.tbytes    = fhm.np * fhm.ebytes;
fhm.bbytes    = fhm.tbytes * fhm.ntraces + sizeof(struct datablockhead);
fhm.vers_id   = 0x2800;
fhm.status    = S_NI|S_NP|S_SECND|S_FLOAT|S_SPEC|S_DATA|S_TRANSF; 
fhm.nbheaders = bp.n_theta;
bhm.status    = S_DATA | S_SPEC | S_FLOAT;
bhm.index     = 0;
bhm.mode      = NP_AVMODE | NI_AVMODE;
bhm.ctcount   = 1;
bhm.lpval     = 0.0;
bhm.rpval     = 0.0;
bhm.lvl       = 0.0;
bhm.tlt       = 0.0;
if (in_memory_prof == FALSE) {
  if (access_file (bp.prof_filt1, &fhf1,&bhf1, FALSE) == ERROR) {
        fprintf (stderr,"BP_3D: can't access prof_filt1\n");
        return (ERROR);
        }
    }
if (create_file (bp.meta_image, &fhm,&bhm) == ERROR) {
    fprintf (stderr,"BP_3D: can't create meta file\n");
    return (ERROR);
    }

/* step 3.6: reconstruct n_theta profile slices */
printf ("\nfirst reconstruction: 0%%");
for (n_slices=last=0; n_slices<bp.n_theta; n_slices++) {
    if (n_slices * 50 / bp.n_theta > last) {
	last = n_slices * 50 / bp.n_theta;
        printf (".");
        fflush (stdout);
        }
    /* step 3.6.1: access n_phi profiles */
    if (in_memory_prof == TRUE) {
        p_prof_float = p_prof_float_start + n_slices * 
                       bp.n_phi * bp.m_size;
        }
    else {
        p_prof_float = p_prof_float_start;	/* reset pointer */
        if (read_vnmr_data (bp.prof_filt1, &fhf1, n_slices * bp.n_phi,
                                 (n_slices+1) *  bp.n_phi, 1,
                                 (char *)p_prof_float,FALSE) == ERROR) {
            fprintf (stderr,"BP_3D: can't read %d meta slice\n",
                     n_slices);
            }
        }
    /* step 3.6.2: set offsets for reconstruction of this profile slice */
    theta    = M_PI / 180.0 * (bp.theta_start + n_slices * 
               (bp.theta_end - bp.theta_start) / bp.n_theta);
    bpc.offset_y = bp.r_center_z - bp.m_center_z;
    if (bp.r_center_x - bp.m_center_x == 0.0) {
        bpc.offset_a = (bpc.offset_y > 0.0) ? M_PI/2 : - M_PI/2;
        }
    else {
        bpc.offset_a = atan2(bpc.offset_y, bp.r_center_x - bp.m_center_x);
        }
    bpc.offset_r = sqrt (bpc.offset_y * bpc.offset_y + 
                         (bp.r_center_x - bp.m_center_x) *
                         (bp.r_center_x - bp.m_center_x) );
    bpc.offset_x = bpc.offset_r * sin (bpc.offset_a - theta);
    
    /* step 3.6.3: reset image array */
    for (i=0; i<bp.i_size * bp.i_size; i++) { *(p_image_int + i) = 0; }

    /* step 3.6.4: reconstruct profile slice */
    if (bp.n_theta % 2 == 0) {
        bp_2d_tp(bound, &bpc, p_prof_float, p_image_int);
        }
    else {
        bp_2d_ti(bound, &bpc, p_prof_float, p_image_int);
        }

    /* step 3.6.5: get min_int and max_int for scaling in 2. cascade */
    /*             currently not evaluated, no prescaling */
    /* get_min_max_int (p_image_int, bp.i_size * bp.i_size, 
                        &min_int, &max_int);
    printf ("%2d nach reco: min_int=%d, max_int=%d\n",
             n_slices, min_int,max_int); */

    /* step 3.6.6: convert from int to float */
    for (i=0; i<bp.i_size * bp.i_size; i++) {
        *(float *)(p_image_int + i) = 
              (float) *(p_image_int + i);
        }
    /* step 3.6.7: save reconstructed "image" */
    if (write_vnmr_data (bp.meta_image, &fhm, bp.i_size * n_slices,
        bp.i_size * (n_slices+1), 1, (char *)p_image_int) == ERROR) {
        fprintf (stderr,"BP_3D: can't write %d image\n", n_slices);
        return (ERROR);
        }
    }   /* end of loop for all n_theta profile slices, 1. cascade */
printf ("100%%\n");

/* 
 * step 4:   free profile and delete unnecessary files
 */
free ((char *) p_prof_float_start);
free ((char *) p_filt_float);
free ((char *) bound);
#if	AUTO_DELETE
if (in_memory_prof == FALSE) {
    sprintf (cmd,"rm %s\n",bp.prof_filt1);
    system (cmd);
    free (bp.prof_filt1);
    }
#endif

/* 
 * step 5:   initialization for second step cascaded 3D BP
 */
/* step 5.1: allocate memory for profiles */
if (in_memory_prof == TRUE) {
    if ((p_prof_float_start = (float *) malloc ((unsigned) bp.n_theta *
                         bp.i_size * bp.i_size * sizeof(float))) == 0) {
        fprintf (stderr,"BP_3D: can't allocate meta image buffer\n");
        return (ERROR);
        }
    for (n_slices=0; n_slices < bp.i_size; n_slices++) {
        p_prof_float = p_prof_float_start + n_slices * bp.n_theta * bp.i_size;
        if (read_vnmr_data (bp.meta_image, &fhm, n_slices,
                             bp.n_theta * bp.i_size, bp.i_size,
                             (char *)p_prof_float,FALSE) == ERROR) {
            fprintf (stderr,"BP_3D: can't read to meta image \n");
            return (ERROR);
            }
        }
    }
else {
    if ((p_prof_float_start = (float *) malloc ((unsigned) bp.n_theta *
                         bp.i_size * sizeof(float))) == 0) {
        fprintf (stderr,"BP_3D: can't allocate meta image buffer\n");
        return (ERROR);
        }
    }
/* step 5.2: allocate memory and generate filter function */
bp.filter_amp = 1.0 / ((float)(bp.i_size * bp.n_theta));
if (gen_filter_float (bp.i_size, (float)bp.i_size/2.0, &p_filt_float, &bp) == ERROR) {
    fprintf (stderr,"BP_3D: can't generate second filter\n");
    return (ERROR);
    }
swap_float (bp.i_size, p_filt_float);

/* step 5.3: allocate memory for boundary table for reconstruction */
if ((bound = (struct boundary *) malloc ((unsigned) bp.i_size * 
             sizeof(struct boundary))) == 0) {
    fprintf (stderr,"BP_3D: can't allocate bound buffer\n");
    return (ERROR);
    }

/* step 5.4: set up coefficients for Hartley transform */
init_fht_float (bp.i_size, &log2_size, exp_of_2, sine, tangens);

/* step 5.5: set initial min/max and pointers */
min = MAXFLOAT;
max = MINFLOAT;
bpc.p_size      = bp.i_size;
bpc.resize      = 1.0;
bpc.n_proj      = bp.n_theta;
bpc.i_center_x  = bp.i_center_x;
bpc.i_center_y  = bp.i_center_y;
bpc.i_center_z  = bp.i_center_z;
bpc.angle_start = bp.theta_start;
bpc.angle_end   = bp.theta_end;
p_prof_float    = p_prof_float_start; 

/* step 5.6: set up file/block header second filtered profile file */
fhf2.nblocks   = bp.i_size;
fhf2.ntraces   = bp.n_theta;
fhf2.np        = bp.i_size;
fhf2.ebytes    = sizeof(float);
fhf2.tbytes    = fhf2.np * fhf2.ebytes;
fhf2.bbytes    = fhf2.tbytes * fhf2.ntraces + sizeof(struct datablockhead);
fhf2.vers_id   = fhp.vers_id;
fhf2.status    = fhp.status;
fhf2.nbheaders = 1;
bhf2.status    = bhp.status;
bhf2.index     = 0;
bhf2.mode      = bhp.mode;
bhf2.ctcount   = bhp.ctcount;
bhf2.lpval     = bhp.lpval;
bhf2.rpval     = bhp.rpval;
bhf2.lvl       = bhp.lvl;
bhf2.tlt       = bhp.tlt;

/* step 5.7: create second filtered profile file */
if (in_memory_prof == FALSE) {
    if (create_file (bp.prof_filt2, &fhf2, &bhf2) == ERROR) {
        fprintf (stderr,"BP_3D: can't create second filtered profile file\n");
        return (ERROR);
        }
    }

/*
 * step 6:   filter i_size profile slices
 */
printf ("\nsecond filter: 0%%");
for (n_slices=last=0; n_slices<bp.i_size; n_slices++) {
    if (n_slices * 50 / bp.i_size > last) {
	last = n_slices * 50 / bp.i_size;
        printf (".");
        fflush (stdout);
        }
    /* step 6.1: access n_theta profiles of cuurent profile slice */
    if (in_memory_prof == FALSE) {
        if (read_vnmr_data (bp.meta_image, &fhm, n_slices,
                                 bp.n_theta * bp.i_size, bp.i_size,
                                 (char *)p_prof_float,FALSE) == ERROR) {
            fprintf (stderr,"BP_3D: can't read %d profile slice\n",
                     n_slices);
            return (ERROR);
            }
        }
    /* step 6.2: filter profiles with prepared filter function */
    for (n_prof=0; n_prof<bp.n_theta; n_prof++) {
        p_prof_float_act = p_prof_float + n_prof * bp.i_size;
        p_prof_float_end = p_prof_float + (n_prof + 1) * bp.i_size;
        p_filt_float_act = p_filt_float;
        /* step 6.2.1: Hartley transform the n_phi profiles */
        swap_float (bp.i_size, p_prof_float_act);
        fht_float (bp.i_size, log2_size, exp_of_2, sine, tangens, 
                   p_prof_float_act);
        /* step 6.2.2  multiply the HT of the profile with filter */
        for ( ; p_prof_float_act < p_prof_float_end; p_prof_float_act++) {
             *p_prof_float_act *= *p_filt_float_act;
             p_filt_float_act++;
             }
        p_prof_float_act = p_prof_float + n_prof * bp.i_size;
        /* step 6.2.3: Inverse Hartley transform the n_phi profiles */
        fht_float (bp.i_size, log2_size, exp_of_2, sine, tangens, 
                   p_prof_float_act);
        swap_float (bp.i_size, p_prof_float + n_prof * bp.i_size); 
        }
    /* step 6.3: calculate minimum and maximum value for later scaling */
    get_min_max_float (p_prof_float, bp.i_size * bp.n_theta, &min, &max);
    /* printf ("%2d after filt: min=%f, max=%f\n", n_slices, min,max); */
    /* step 6.4: data saved in memory or meta file */
    if (in_memory_prof == TRUE) {
        p_prof_float += bp.n_theta * bp.i_size;
        }
    else {
        p_prof_float = p_prof_float_start; 
        if (write_vnmr_data (bp.prof_filt2, &fhf2, n_slices * bp.n_theta,
                              (n_slices + 1) * bp.n_theta, 1,
                              (char *)p_prof_float) == ERROR) {
            fprintf (stderr,"BP_3D: can't write second filtered profiles\n");
            return (ERROR);
            }
        } 
    }   /* end of loop for all n_theta profile slices */
printf ("100%%\n");

/*
 * step 7:   backproject all i_size profile slices
 */
/* step 7.1: reset pointer */
p_prof_float = p_prof_float_start;

/* step 7.2: allocate memory for reconstructed image */
/*           not necessary - still allocated */

/* step 7.3: set up scale, save it in bp structure */
if (max > -min) { bp.scale2 = bpc.scale = 24576.0/max; }
else            { bp.scale2 = bpc.scale = 24576.0 / (- min); }
printf ("second scale: min=%f, max=%f, scale=%f\n", min,max,bp.scale2);

/* step 7.4: initializations */
min_int = MAXINT;
max_int = - MAXINT;

/* step 7.5: set up file/block header */
fhi.nblocks   = 1;
fhi.ntraces   = bp.i_size;
fhi.np        = bp.i_size;
fhi.ebytes    = sizeof(float);
fhi.tbytes    = fhi.np * fhi.ebytes;
fhi.bbytes    = fhi.tbytes * fhi.ntraces + sizeof(struct datablockhead);
fhi.vers_id   = 0x2800;
fhi.status    = S_NI|S_NP|S_SECND|S_FLOAT|S_SPEC|S_DATA|S_TRANSF; 
fhi.nbheaders = 1;
bhi.status    = S_DATA | S_SPEC | S_FLOAT;
bhi.index     = 0;
bhi.mode      = NP_AVMODE | NI_AVMODE;
bhi.ctcount   = 1;
bhi.lpval     = 0.0;
bhi.rpval     = 0.0;
bhi.lvl       = 0.0;
bhi.tlt       = 0.0;
if (in_memory_prof == FALSE) {
  if (access_file (bp.prof_filt2, &fhf2,&bhf2, FALSE) == ERROR) {
        fprintf (stderr,"BP_3D: can't access prof_filt2\n");
        return (ERROR);
        }
    }

/* step 7.6: reconstruct bp.i_size, profile slices */
printf ("\nsecond reconstruction: 0%%");
for (n_slices=last=0; n_slices<bp.i_size; n_slices++) {
    if (n_slices * 50 / bp.i_size > last) {
	last = n_slices * 50 / bp.i_size;
        printf (".");
        fflush (stdout);
        }
    /* step 7.6.1: access n_phi profiles */
    if (in_memory_prof == TRUE) {
        p_prof_float = p_prof_float_start + n_slices * 
                       bp.n_theta * bp.i_size;
        }
    else {
        p_prof_float = p_prof_float_start;	/* reset pointer */
        if (read_vnmr_data (bp.prof_filt2, &fhf2, n_slices * bp.n_theta,
                                 (n_slices + 1) * bp.n_theta, 1,
                                 (char *)p_prof_float,FALSE) == ERROR) {
            fprintf (stderr,"BP_3D: can't read %d image meta slice\n",
                     n_slices);
            }
        }
    /* step 7.6.2: set offsets for reconstruction of this profile slice */
    bpc.offset_x = 0.0;
    bpc.offset_y = 0.0;

    /* step 7.6.3: reset image array */
    for (i=0; i<bp.i_size * bp.i_size; i++) { *(p_image_int + i) = 0; }

    /* step 7.6.4: reconstruct profile slice */
    limit = sqrt ((double)(n_slices * (bp.i_size - n_slices)));
    /* make_bound (bp.i_size, (double)(bp.i_size - 1), */
    make_bound (bp.i_size, 2.0 * limit,
            bp.i_center_x, bp.i_center_y, bound);
    if (bp.i_size % 2 == 0) {
        bp_2d_tp(bound, &bpc, p_prof_float, p_image_int);
        }
    else {
        bp_2d_ti(bound, &bpc, p_prof_float, p_image_int);
        }

    /* step 7.6.4: get min_int and max_int for scaling in 2. cascade */
    get_min_max_int (p_image_int, bp.i_size * bp.i_size, &min_int, &max_int);

    /* step 7.6.5: convert from int to float */
    for (i=0; i<bp.i_size * bp.i_size; i++) {
        *(float *)(p_image_int + i) = 
              (float) *(p_image_int + i);
        }

    /* step 7.6.6: save reconstructed "image" */
    sprintf (bp.image_file + strlen(bp.image_file) - 3, "%03d",n_slices);
    /* if (create_file (bp.image_file, &fhi,&bhi) == ERROR) { */
    /* 2d slices */
    /* if (create_ib_file (bp.image_file, &fhi,&bhi) == ERROR) { */
    /*   fprintf (stderr,"BP_3D: can't create image file\n"); */
    /*   return (ERROR); */
    /* } */
    /* if (write_ib_data (bp.image_file, &fhi, 0, */
    /*     bp.i_size, 1, (char *)p_image_int) == ERROR) { */
    /*     fprintf (stderr,"BP_3D: can't write %d image\n", n_slices); */
    /*     close(file_out); */
    /*     return (ERROR); */
    /*     } */

    if (n_slices == 0)
    {
      fhi.nblocks = bp.i_size;
      if ((file_out = create_3d_file(bp.image_file, &fhi,&bhi)) == NULL) {
        fprintf (stderr,"BP_3D: can't create image file\n");
        return (ERROR);
        }
     }
    /* if (write_vnmr_data (bp.image_file, &fhi, 0, */
    if (write_3d_data (file_out, &fhi, 0,
        bp.i_size, 1, (char *)p_image_int) == ERROR) {
        fprintf (stderr,"BP_3D: can't write %d image\n", n_slices);
	fclose(file_out);
        return (ERROR);
        }
    }   /* end of loop for all n_theta profile slices, 1. cascade */
fclose(file_out);
printf ("100%%\n");
printf ("reco: min_int=%d, max_int=%d\n", min_int,max_int);

/* 
 * step 8:   free profile, delete files
 */
free ((char *) p_prof_float_start);
free ((char *) p_filt_float);
free ((char *) bound);
free ((char *) p_image_int);
#if	AUTO_DELETE
sprintf (cmd,"rm %s\n",bp.meta_image);
system (cmd);
free (bp.meta_image);
if (in_memory_prof == FALSE) {
    sprintf (cmd,"rm %s\n",bp.prof_filt2);
    system (cmd);
    free (bp.prof_filt2);
    }
#endif
return(TRUE);
}


get_parameter_string (par_file,par_name,par_string)
char	*par_file;
char	*par_name;
char	*par_string;
{
char	*cmd;
FILE	*awk_fp;

if ((cmd = (char *)malloc (sizeof(par_name) + 80)) == NULL) {
    fprintf (stderr,"GET_PARAMETER: can't allocate par_string memory");
    exit (0);
    }
/* build command for accessing parameter file */
sprintf (cmd,"cat %s | awk '{if ($1 == \"%s\") {print $2}}'\n",par_file,par_name);
if ((awk_fp = popen (cmd, "r")) == NULL) {
    fprintf (stderr,"GET_PARAMETER: can't popen awk_fp\n");
    return (ERROR);
    }
if (fgets(par_string, MAX_PAR_STRING, awk_fp) == NULL) {
    fprintf (stderr,"BP_3D: can't access parameter\n");
    *par_string = 0;
    } 
else {
    *(par_string + strlen(par_string) - 1) = 0;
    }
pclose (awk_fp);
free (cmd);
#if 	PAR_PRINT
printf ("parameter %s >%s<\n",par_name,par_string);
#endif
return (0);
}


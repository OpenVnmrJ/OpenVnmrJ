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
/*	bp.h		headerfile for backprojection modules	*/
/*								*/
/****************************************************************/
/*								*/
/*	authors:	Martin Staemmler			*/
/*			Harald Niederlaender			*/
/*								*/
/*	Institute for biomedical Engineering (IBMT)		*/
/*	D - 6670 St. Ingbert, Germany				*/
/*	Ensheimerstrasse 48					*/
/*	Tel.: (+49) 6894 89740					*/
/*	Fax:  (+49) 6894 89750					*/
/*								*/
/****************************************************************/
/*								*/
/*	date:		15.02.91				*/
/*	revision:	initial release				*/
/*								*/
/****************************************************************/

/*-----	include */
#include	<stdio.h>
#include	<math.h>

#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include	"data.h"
#include	"bpvalues.h"

/*-----	define */
#define		FALSE		0
#define		TRUE		1
#define		ERROR		-1
#define		MAX_SIZE	512
#define		LOG2_MAX_SIZE	9
#define		MAX_PAR_STRING	256

/*----- expansion resolution */
#define		RES		16
#define		RES_POW		4

/*-----	system dependency */
/* reversed these for linux - mrk */
#define		SUN		0
#define		ASPECT_X32	1

/*----- struct definitions */
struct	boundary {
		short	l;
		short	r;
		short	c;
		};
typedef	struct	bp_parameter {
		char	*filter_name;	/* bp_2d filter name */
		float	filter_bw;	/* bp_2d filter bandwidth */
		float	filter_amp;	/* filter amplitude / scale */
		char	*prof_file;	/* name of profile filename  */
		char	*prof_filt1;	/* 1.cascade: filtered prof.filename */
		char	*meta_image;	/* 1.cascade: recon.meta filename */
		char	*prof_filt2;	/* 2.cascade: filtered prof.filename */
		char	*image_file;	/* 2.cascade: recon. image mask */
		float	scale1;		/* scaling constant 1. step */
		float	scale2;		/* scaling constant 2. step */
		int	m_size;		/* measurement size */
		double	m_center_x,
                        m_center_y,
                        m_center_z;	/* measurement center */
		int	r_size;		/* reconstruction size */
		double	r_center_x,
                        r_center_y,
                        r_center_z;	/* reconstruction center */
		int	i_size;		/* reconstructed image size */
		double	i_center_x,
                        i_center_y,
                        i_center_z;	/* image center */
                int	n_theta;	/* # of projections for theta */
                int	n_phi;		/* # of projections for phi */
                double	theta_start;	/* initial offset theta */
                double	theta_end;	/* final theta */
                double	phi_start;	/* initial offset phi */
                double	phi_end;	/* final phi */
                } BP_PAR;

typedef	struct	bp_control {
		int	p_size;		/* profile size */
		int	i_size;		/* reconstructed image size */
		double	resize;		/* resize value for images */
		float	scale;		/* scaling of profile values */
                int	n_proj;		/* actual # of projections */
                double	offset_x;
		double	offset_y;
                double	offset_a;	/* angle off center acquisition */
                double  offset_r;	/* off center radius */
		double	i_center_x,
                        i_center_y,
                        i_center_z;	/* image center */
		double	angle_start;	/* actual angle offset */
                double	angle_end;	/* final actual angle */
                } BP_CTRL;

/*  function prototypes */
int gen_filter_float(int, float, float **, BP_PAR *);
int band_pass_float(int, float, float [], float, float);
int si_low_pass_float(int, float, float [], float, float);
int cos_low_pass_float(int, float, float [], float, float);
int hamming_float(int, float, float [], float, float);

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* aslmirtime.h */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* aslmirtime.h: Global includes and defines                                 */
/*                                                                           */
/* Copyright (C) 2011 Paul Kinchesh                                          */
/*                                                                           */
/* This file is part of aslmirtime.                                          */
/*                                                                           */
/* aslmirtime is free software: you can redistribute it and/or modify        */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* aslmirtime is distributed in the hope that it will be useful,             */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with aslmirtime. If not, see <http://www.gnu.org/licenses/>.        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/**/


/*--------------------------------------------------------*/
/*---- Wrap the header to prevent multiple inclusions ----*/
/*--------------------------------------------------------*/
#ifndef _H_aslmirtime_H
#define _H_aslmirtime_H


/*----------------------------------*/
/*---- Include standard headers ----*/
/*----------------------------------*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stddef.h>
#include <sys/time.h>


/*-----------------------------*/
/*---- Include GSL Headers ----*/
/*-----------------------------*/
#include <gsl/gsl_multimin.h>


/*-------------------------------------------*/
/*---- Make sure __FUNCTION__ is defined ----*/
/*-------------------------------------------*/
#ifndef __FUNCTION__
#define __FUNCTION__ __func__
#endif


/*------------------------------------------------------*/
/*---- Floating point comparison macros (as in SGL) ----*/
/*------------------------------------------------------*/
/* EPSILON is the largest allowable deviation due to floating point storage */
#define EPSILON 1e-9
#define FP_LT(A,B)  (((A)<(B)) && (fabs((A)-(B))>EPSILON))  /* A less than B */
#define FP_GT(A,B)  (((A)>(B)) && (fabs((A)-(B))>EPSILON))  /* A greater than B */
#define FP_EQ(A,B)  (fabs((A)-(B))<=EPSILON)                /* A equal to B */
#define FP_NEQ(A,B) (!FP_EQ(A,B))                           /* A not equal to B */
#define FP_GTE(A,B) (FP_GT(A,B) || FP_EQ(A,B))              /* A greater than or equal to B */
#define FP_LTE(A,B) (FP_LT(A,B) || FP_EQ(A,B))              /* A less than or equal to B */


/*---------------------------------------------------------------*/
/*---- So we can simply include aslmirtime.h in aslmirtime.c ----*/
/*---------------------------------------------------------------*/
#ifdef LOCAL
#define EXTERN
#else
#define EXTERN extern
#endif


/*-----------------------------------*/
/*---- Functions in aslmirtime.c ----*/
/*-----------------------------------*/
double my_f (const gsl_vector *v, void *params);
void my_df (const gsl_vector *v, void *params, gsl_vector *df);
void my_fdf (const gsl_vector *x, void *params, double *f, gsl_vector *df);
void getinput(double **pars,int argc,char *argv[]);
int nomem(char *file,const char *function,int line);
int usage_terminate();

/*----------------------------*/
/*---- End of header wrap ----*/
/*----------------------------*/
#endif

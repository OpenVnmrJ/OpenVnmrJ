/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************************
*   HISTORY:
*     Revision 1.1  2006/08/23 14:09:56  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/08/22 23:30:04  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/07/07 01:12:42  mikem
*     modification to compile with psg
*
*
***************************************************************************/
#ifndef SGLPRESPULSES
			#define SGLPRESPULSES


			extern char     rfcoil[MAXSTR];                    /* RF-coil name */
			extern int      sgldisplay;

			/* pre pulse variables - FATSAT*/
			extern char     fsat[MAXSTR],fsatpat[MAXSTR];	    /* Fat suppression flag, FATSAT pulse pattern*/
			extern double   flipfsat;                          /* fatsat flip angle */
			extern double   pfsat,fsatfrq;                     /* FATSAT duration, FATSAT frequency */
			extern double   gcrushfs,tcrushfs;                 /* FATSAT crusher amplitude / duration */
			extern double   fsatTime;                          /* duration of FATSAT segment */

			/* pre pulse variables - MTC */
			extern double   flipmt;                            /* MTC flip angle*/
			extern double   mtfrq;                             /* MTC duration, MTC frequency */
			extern double   gcrushmt,tcrushmt;                 /* MTC crusher amplitude / duration */
			extern double   mtTime;                            /* duration of MTC segment */

			/* pre pulse variables - SATBAND */
			extern char     sat[MAXSTR];                       /* SATBAND flag */
			extern double   flipsat;                           /* SATBAND pulse flip angle */    
			extern double   satpos[MAXNSAT];                   /* SATBAND position */
			extern double   satthk[MAXNSAT];                   /* SATBAND thickness*/   
			extern double   satamp[MAXNSAT];                   /* SATBAND grad amplitudes */   
			extern double   satpsi[MAXNSAT];                   /* SATBAND grad angle */   
			extern double   satphi[MAXNSAT];                   /* SATBAND grad angle */   
			extern double   sattheta[MAXNSAT];                 /* SATBAND grad angle */   
			extern double   satTime;                           /* Duration of SATBAND segment */
			extern int      nsat;                              /* Number of SATBANDS */
			extern double   gcrushsat,tcrushsat;               /* SATBAND crusher amplitude / duration */

			/* pre pulse variables RF-tagging */
			extern double  wtag;                                /* spatial width of tag [cm] */
			extern double  dtag;                                /* Sspatial separation of tag [cm] */
			extern double  ptag ;                               /* Total duration of RF train [usec] */
			extern char    tag[MAXSTR];                         /* tagging flag: y = ON, n = OFF */
			extern char    tagpat[MAXSTR];                      /* tagging pulse pattern - for calibration */
			extern int     tagdir;                              /* tag direction : 0-OFF, 1 - readout,
                                              			2-Phase, 3-Readout & Phase */
			extern double  fliptag;                             /* tagging flip angle */
			extern double  tagtime;                             /* duration of tagging segment */
			extern double  gcrushtag, tcrushtag;                /* tagging crusher strength and duration*/
			extern int     rfamp[1024];
			extern int     ntag;                                /* number of tags */

			/* pre pulse variables IR */
			extern double  flipir;                              /* IR flip angle */
			extern double  gcrushir;                            /* IR cruhser duration */
			extern double  tcrushir;                            /* IR crusher strength */
			extern double  thkirfact;                           /* IR slice thickness factor */
			extern double  ti1_delay;                           /* IR delay 1 */
			extern double  ti2_delay;                           /* IR delay 2 */
			extern double  ti3_delay;                           /* IR delay 3 */
			extern double  ti4_delay;                           /* IR delay 4 */
			extern int     irsequential;                        /* Sequential IR and readout for each slice */
			extern int     nsirblock;                           /* Number of slices per IR block (slices not blocked nsirblock=ns) */
			extern int     nirinterleave;                       /* Number of interleaved IR slices per IR block */
			extern double  irmincycle;                          /* Minimum IR cycle time */
			extern double  tiaddTime;                           /* Additional time beyond IR component to be included in ti */
			extern double  tauti;                               /* Additional time beyond IR component to be included in ti */
			extern double  irgradTime;                          /* Duration of IR gradient components for a single inversion */
			extern double  irTime;                              /* Duration of IR components for all slices */

			/* Blocked slices */
			extern int blockslices;                             /* Block slices */
			extern int nsblock;                                 /* Number of slices per block (slices not blocked nsblock=1) */

			extern char checkduty[MAXSTR];

			/* Gradient structures */
			extern SLICE_SELECT_GRADIENT_T  sat_grad;           /* satband slice select gradient */  
			extern SATBAND_GRADIENT_T       satss_grad[6];      /* slice select - SATBAND array*/   
			extern SATBAND_INFO_T           satband_info;       /* arrayed satband values */   
			extern GENERIC_GRADIENT_T       fsatcrush_grad;     /* crusher gradient structure */
			extern GENERIC_GRADIENT_T       mtcrush_grad;	     /* crusher gradient structure */
			extern GENERIC_GRADIENT_T       satcrush_grad;      /* crusher gradient structure */
			extern GENERIC_GRADIENT_T       tagcrush_grad;      /* crusher gradient structure */
			extern GENERIC_GRADIENT_T       tag_grad;           /* tagging gradient structure */

			/* RF-pulse structures */
			extern RF_PULSE_T    mt_rf;              /* MTC RF pulse structure */
			extern RF_PULSE_T    fsat_rf;            /* fatsat RF pulse structure */
			extern RF_PULSE_T    sat_rf;             /* SATBAND RF pulse structure */
			extern RF_PULSE_T    tag_rf;             /* RF TAGGIGN pulse structure */

			/* F_initval */
			extern void F_initval(double val, int rtvar);

  extern int dps_new_shapelist(char *name, int code, int dcode,
     char *pattern,double width,double *listarray,double nsval,double frac,char mode, int chan,
     char *s1, char *s2, char *s3, char *s4, char *s5, char *s6, char *s7);

  extern int dps_shapelist(char *name,int code,int dcode,char *pattern,
     double width,double *listarray,double nsval,double frac,char mode,
     char *s1, char *s2, char *s3, char *s4, char *s5, char *s6);

  extern void dps_new_shapedpulselist(char *name, int code, int dcode,
     int listId,double width,int rtvar,double rof1,double rof2,char mode,int vvar, int chan,
     char *s1,char *s2,char *s3,char *s4,char *s5,char *s6,char *s7, char *s8);

   extern void dps_shapedpulselist(char *name, int code, int dcode, int listId, double width,
          int rtvar, double rof1, double rof2, char mode, int vvar,
          char *s1, char *s2, char *s3, char *s4, char *s5, char *s6, char *s7);

			void create_fatsat();
			void create_mtc();
			void create_satbands();
			void create_inversion_recovery();
			void create_arterial_spin_label();
			void create_tag();
			void fatsat();
			void mtc();
			void satbands();
			double calc_irTime(double tiadd,double trepmin,char mintrepflag,double trep,int *treptype);
			void calc_minirTime(double tinv,double trep);
			double tune_irTime(double tiadd,double mintrep,double trep,int *treptype);
			void init_vinvrec();
			void inversion_recovery();
			double calc_aslTime(double asladd,double trepmin,int *treptype);
			void arterial_spin_label();
			void asl_xmton();
			void asl_xmtoff();
			void asl_tagcoilon();
			void asl_tagcoiloff();
			void create_vascular_suppress();
			void vascular_suppress();
			void tag_sinc();
			void do_profile();

			void t_calc_minirTime(double tinv,double trep);
			double t_tune_irTime(double tiadd,double mintrep,double trep,int *treptype);
			void t_init_vinvrec();
			double t_calc_aslTime(double asladd,double trepmin,int *treptype);
			void t_arterial_spin_label();
			void t_asl_xmton();
			void t_asl_xmtoff();
			void t_asl_tagcoilon();
			void t_asl_tagcoiloff();
			void t_create_vascular_suppress();
			void t_vascular_suppress();

			void x_calc_minirTime(double tinv,double trep);
			double x_tune_irTime(double tiadd,double mintrep,double trep,int *treptype);
			void x_init_vinvrec();
			double x_calc_aslTime(double asladd,double trepmin,int *treptype);
			void x_arterial_spin_label();
			void x_asl_xmton();
			void x_asl_xmtoff();
			void x_asl_tagcoilon();
			void x_asl_tagcoiloff();
			void x_create_vascular_suppress();
			void x_vascular_suppress();

#endif

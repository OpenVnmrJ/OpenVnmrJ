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
*     Revision 1.7  2006/12/05 22:00:02  mikem
*     Fixed bug that caused error when zero-filling required 0 addon - original time = new padded time
*
*     Revision 1.6  2006/11/29 01:25:10  mikem
*     Cleaned up code , removed print statements
*
*     Revision 1.5  2006/11/29 01:24:19  mikem
*     Fixed bug in )th moment calculation of readout for readout refocus / dephase
*
*     Revision 1.4  2006/11/10 00:10:33  mikem
*     modified calcButterfly to return max amplitude
*
*
*
***************************************************************************/
#include "standard.h"

#include "sgl.h"
#include "vfilesys.h"


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*--- S T A R T   O F   P A R A M E T E R    D E C L A R A T I O N  ----*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/

SGL_EULER_MATRIX_T mainRotationMatrix;
SGL_MEAN_SQUARE_CURRENTS_T msCurrents;
SGL_MEAN_SQUARE_CURRENTS_T rmsCurrents;

/* PDDacquire */
char     PDDacquire[MAXSTR];                /* PDD acquire flag: y = PDD switches around acquisition, n = PDD switches around transmition */

/* fixed ramp time variable / flag */
int      sglabort;
int      sgldisplay;
int      sglerror;
int      sglarray;
int      sglpower;

double   glim;                              /* gradient limiting factor: % in VJ, fraction in SGL  */
double   glimpe=0;                          /* PE gradient limiting factor [%] */

double   trampfixed;                        /* duration of fixed ramp for system with gradient */
                                            /* and/or amplifier resonance.  If none existent or */

GRAD_FORM_LIST_T gradList;
GRAD_WRITTEN_LIST_T *gradWListP;

SGL_EVENT_DEBUG_T	sglEventDebug;
struct SGL_GRAD_NODE_T  *cg = NULL;         /* a list of gradient pulses */
struct SGL_GRAD_NODE_T  *gl = NULL;         /* a list of gradient pulses */
struct SGL_GRAD_NODE_T	*sk = NULL;
struct SGL_GRAD_NODE_T  **workingList; 	    /* a list of gradient pulses */
int gradEventsOverlap;

int trisesqrt3=FALSE;                       /* flag to determine if euler_test has increased trise */

/*  variables - GENERAL*/

int      trigger;                           /* trigger select */
double   arraydim;
double   flip1, flip2, flip3, flip4, flip5; /* RF pulse flip angles */
char     rfcoil[MAXSTR];                    /* RF-coil name */
double   b1max;                             /* Maximum B1 (Hz) for the rf coil */
char     profile[MAXSTR];                   /* profile flag: y = ON, n = OFF */
double   pe_steps, pe2_steps, pe3_steps;    /* number of phase encode steps; usually pe_stesp=nv*/
char     fc[MAXSTR];                        /* flowcomp flag: y = ON, n = OFF */
char     perewind[MAXSTR];                  /* Phase rewind flag : y = ON, n = OFF */
char     spoilflag[MAXSTR];                 /* spoiler flag  : y = ON, n = OFF */
double   echo_frac;                         /* echo fraction */
double   nseg;                              /* number of segments for multi shot experiment */
double   etl;                               /* Echo Train Length */
char     navigator[MAXSTR];                 /* navigator flag: y = ON, n = OFF */
double   slewlim;                           /* gradient slew rate limiting factor [%] */
double   ssc;                               /* Compressed Steady State scans */
double   ss;                                /* Steady State scans */

/* timing variables */
char     minte[MAXSTR];                     /* minimum TE flag: y = ON, n = OFF */
char     mintr[MAXSTR];                     /* minimum TR flag: y = ON, n = OFF */
char     minti[MAXSTR];                     /* minimum TI (inversion time) flag: y = ON, n = OFF */
char     mintm[MAXSTR];                     /* minimum TM (mixing time) flag: y = ON, n = OFF */
char     minesp[MAXSTR];                    /* minimum ESP (fsems echo spacing) */
char     spinecho[MAXSTR];                  /* spin echo flag: y = ON, n = OFF */
double   temin;                             /* minimum TE */
double   trmin;                             /* minimum TR */
double   timin;                             /* minimum TR */
double   tmmin;                             /* minimum TM */
double   tpemin;                            /* minimum duration of PE gradient */
double   espmin;                            /* minimum ESP */
double   esp;                               /* echo spacing */
double   tep;                               /* group delay - gradient*/
double   tau1;                              /* sum of events 1 */
double   tau2;                              /* sum of events 2  */
double   tau3;                              /* sum of events 3  */
double   tau4;                              /* sum of events 4  */
double   trimage;                           /* inter-image delay */
int      trtype;                            /* pack or distribute tr_delay */

/* Readout butterfly crushers */
double   gcrushro;
double   tcrushro;

/* pre pulse variables - FATSAT*/
char     fsat[MAXSTR],fsatpat[MAXSTR];      /* Fat suppression flag, FATSAT pulse pattern*/
double   flipfsat;                          /* fatsat flip angle */
double   pfsat,fsatfrq;                     /* FATSAT duration, FATSAT frequency */
double   gcrushfs,tcrushfs;                 /* FATSAT crusher amplitude / duration */
double   fsatTime;                          /* duration of FATSAT segment */

/* pre pulse variables - MTC */
double   flipmt;                            /* MTC flip angle*/
double   mtfrq;                             /* MTC duration, MTC frequency */
double   gcrushmt,tcrushmt;                 /* MTC crusher amplitude / duration */
double   mtTime;                            /* duration of MTC segment */

/* pre pulse variables - SATBAND */
char     sat[MAXSTR];                       /* SATBAND flag */
double   flipsat;                           /* SATBAND pulse flip angle */
double   satpos[MAXNSAT];                   /* SATBAND position */
double   satthk[MAXNSAT];                   /* SATBAND thickness*/
double   satamp[MAXNSAT];                   /* SATBAND grad amplitudes */
double   satpsi[MAXNSAT];                   /* SATBAND grad angle */
double   satphi[MAXNSAT];                   /* SATBAND grad angle */
double   sattheta[MAXNSAT];                 /* SATBAND grad angle */
double   satTime;                           /* Duration of SATBAND segment */
int      nsat;                              /* Number of SATBANDS */
double   gcrushsat,tcrushsat;               /* SATBAND crusher amplitude / duration */

/* pre pulse variables RF-tagging */
double  wtag;                                /* spatial width of tag [cm] */
double  dtag;                                /* Sspatial separation of tag [cm] */
double  ptag ;                               /* Total duration of RF train [usec] */
char    tag[MAXSTR];                         /* tagging flag: y = ON, n = OFF */
char    tagpat[MAXSTR];                      /* tagging pulse pattern - for calibration */
int     tagdir;                              /* tag direction : 0-OFF, 1 - readout,
                                              2-Phase, 3-Readout & Phase */
double  fliptag;                             /* tagging flip angle */
double  tagtime;                             /* duration of tagging segment */
double  gcrushtag, tcrushtag;                /* tagging crusher strength and duration*/
int     rfamp[1024];
int     ntag;                                /* number of tags */

/* pre pulse variables IR */
double  flipir;                              /* IR flip angle */
double  gcrushir;                            /* IR cruhser duration */
double  tcrushir;                            /* IR crusher strength */
double  thkirfact;                           /* IR slice thickness factor */
double  ti1_delay;                           /* IR delay 1 */
double  ti2_delay;                           /* IR delay 2 */
double  ti3_delay;                           /* IR delay 3 */
double  ti4_delay;                           /* IR delay 4 */
int     irsequential;                        /* Sequential IR and readout for each slice */
int     nsirblock;                           /* Number of slices per IR block (slices not blocked nsirblock=ns) */
int     nirinterleave;                       /* Number of interleaved IR slices per IR block */
double  irmincycle;                          /* Minimum IR cycle time */
double  tiaddTime;                           /* Additional time beyond IR component to be included in ti */
double  tauti;                               /* Additional time beyond IR component to be included in ti */
double  irgradTime;                          /* Duration of IR gradient components for a single inversion */
double  irTime;                              /* Duration of IR components for all slices */

/* mean nt and mean tr of arrayed values */
double ntmean;                               /* Mean nt of arrayed nt values */
double trmean;                               /* Mean tr of arrayed tr values */

/* Blocked slices */
int blockslices;                             /* Block slices */
int nsblock;                                 /* Number of slices per block (slices not blocked nsblock=1) */

/* Diffusion */
char    diff[MAXSTR];                        /* Diffusion flag */
double  dro, dpe, dsl;                       /* Multipliers onto diffusion gradients */
double  taudiff=0.0;                         /* Additional time beyond diffusion gradients to be included in DELTA */
double  droval,dpeval,dslval;                /* dro, dpe and dsl values for b-value calculation */
DIFFUSION_T diffusion;                       /* Diffusion variables */

/* Water Suppression */
char    wspat[MAXSTR];                       /* RF shape */
double  pws, flipws;                         /* pulse width, flip angle */

/* EPI tweakers and others */
double  groa;                                /* EPI tweaker - readout */
double  grora;                               /* EPI tweaker - dephase */
double  image;                               /* EPI repetitions */
double  images;                              /* EPI repetitions */
double  ssepi;                               /* EPI readput steady state */
char    rampsamp[MAXSTR];                    /* ramp sampling */

/* phase encode order */
char    ky_order[MAXSTR];                    /* phase order flag : y = ON, n = OFF */
double  fract_ky;

/* velocity encoding */
double  venc;                                /* max encoded velocity [cm/s] */

/* Arterial Spin Labelling (ASL) */
char    asl[MAXSTR];                         /* ASL flag */
int     asltype;                             /* type of ASL: FAIR, STAR, PICORE, CASL */
char    aslplan[MAXSTR];                     /* ASL graphical planning flag */
int     asltag;                              /* ASL tag off/tag/control, 0,1,-1 */
char    asltagcoil[MAXSTR];                  /* flag for separate tag coil (requires 3rd RF channel) */
char    aslrfcoil[MAXSTR];                   /* tag coil name */
RF_PULSE_T asl_rf;                           /* ASL tagging pulse */
char    paslpat[MAXSTR];                     /* Pulsed ASL (PASL) pulse shape */
double  pasl;                                /* PASL pulse duration */
double  flipasl;                             /* PASL pulse flip */
double  pssAsl[MAXSLICE];                    /* Position of ASL tag pulse */
double  freqAsl[MAXSLICE];                   /* Frequency of ASL tag pulse */
char    asltagrev[MAXSTR];                   /* Flag to reverse ASL tag position w.r.t. imaging slices */
int     shapeAsl;                            /* Shape list ID for ASL tag pulse */
char    pcaslpat[MAXSTR];                    /* Continuous ASL (CASL) pulse shape */
double  pcasl;                               /* CASL pulse duration */
double  flipcasl;                            /* CASL pulse flip */
double  caslb1;                              /* B1 for CASL pulse */
char    caslctrl[MAXSTR];                    /* CASL control type (default or sine modulated) */
char    caslphaseramp[MAXSTR];               /* Flag to phase ramp CASL pulses (long pulses take time to phase ramp) */
int     aslphaseramp=TRUE;                   /* Default flag to phase ramp ASL pulses */
char    starctrl[MAXSTR];                    /* STAR control type (default or double tag) */
RF_PULSE_T aslctrl_rf;                       /* ASL control pulse (if different to tag pulse) */
double  pssAslCtrl[MAXSLICE];                /* Position of ASL control pulse */
double  freqAslCtrl[MAXSLICE];               /* Frequency of ASL control pulse */
int     shapeAslCtrl;                        /* Shape list ID for ASL control pulse */
SLICE_SELECT_GRADIENT_T asl_grad;            /* ASL tag gradient */
double  asltaggamp;                          /* ASL tag gradient amplitude */
double  aslthk;                              /* ASL tag slice thickness */
double  asladdthk;                           /* ASL additional tag slice thickness (FAIR) */
double  asltagthk;                           /* Input STAR/PICORE ASL tag thickness */
double  aslgap;                              /* ASL tag slice gap (to image slice) */
double  aslpos;                              /* ASL tag slice position */
double  aslpsi,aslphi,asltheta;              /* ASL tag slice orientation */
double  aslctrlthk;                          /* ASL control slice thickness */
double  aslctrlpos;                          /* ASL control slice position */
double  aslctrlpsi,aslctrlphi,aslctrltheta;  /* ASL control slice orientation */
double  caslgamp;                            /* Amplitude of CASL tag gradient (G/cm) */
GENERIC_GRADIENT_T aslspoil_grad;            /* ASL tag spoil gradient */
double  gspoilasl;                           /* ASL tag spoil gradient amplitude */
double  tspoilasl;                           /* ASL tag spoil gradient duration */
double  aslti;                               /* ASL inflow time */
char    minaslti[MAXSTR];                    /* minimum ASL inflow time flag: y = ON, n = OFF */
double  aslti_delay;                         /* ASL inflow delay */
double  tauasl;                              /* Additional time beyond ASL component to be included in inflow time */
double  aslTime;                             /* ASL module duration */
double  slicetr;                             /* TR to next slice (multislice mode) */
char    minslicetr[MAXSTR];                  /* minimum TR to next slice (multislice mode) */
double  asltr_delay;                         /* ASL tr delay */

char    ips[MAXSTR];                         /* In Plane Saturation (IPS) flag */
char    ipsplan[MAXSTR];                     /* IPS graphical planning flag */
RF_PULSE_T ips_rf;                           /* IPS pulse */
char    ipspat[MAXSTR];                      /* IPS pulse shape */
double  pips;                                /* IPS pulse duration */
double  flipips;                             /* IPS pulse flip */
double  flipipsf;                            /* IPS pulse flip factor */
double  pssIps[MAXSLICE];                    /* Position of IPS pulse */
double  freqIps[MAXSLICE];                   /* Frequency of IPS pulse */
int     shapeIps;                            /* Shape list ID for IPS pulse */
int     nips;                                /* number of IPS pulses */
SLICE_SELECT_GRADIENT_T ips_grad;            /* IPS gradient */
double  ipsthk;                              /* IPS slice thickness */
double  ipsaddthk;                           /* IPS additional slice thickness */
double  ipspos;                              /* IPS slice position */
double  ipspsi,ipsphi,ipstheta;              /* IPS slice orientation */
GENERIC_GRADIENT_T ipsspoil_grad;            /* IPS spoil gradient */
double  gspoilips;                           /* IPS spoil gradient amplitude (input) */
double  ipsgamp;                             /* IPS spoil gradient amplitude (actual) */
double  tspoilips;                           /* IPS spoil gradient duration */
char    wetips[MAXSTR];                      /* WET IPS flag */
double  ipsTime;                             /* IPS module duration */

char    mir[MAXSTR];                         /* Multiple Inversion Recovery (MIR) flag */
RF_PULSE_T mir_rf;                           /* MIR pulse */
char    mirpat[MAXSTR];                      /* MIR pulse shape */
double  pmir;                                /* MIR pulse duration */
double  rofmir;                              /* MIR pulse rof */
double  flipmir;                             /* MIR pulse flip */
double  freqMir;                             /* Frequency of MIR pulse */
int     delayMir;                            /* Delay list ID for MIR pulses */
int     nmir;                                /* number of MIR pulses */
int     nmirq2;                              /* number of MIR pulses after Q2TIPS */
double  irduration;                          /* duration of single IR component */
double  mir_delay[MAXMIR];                   /* the delays to MIR pulses */
GENERIC_GRADIENT_T mirspoil_grad;            /* MIR spoil gradient */
double  gspoilmir;                           /* MIR spoil gradient amplitude */
double  tspoilmir;                           /* MIR spoil gradient duration */
char    autoirtime[MAXSTR];                  /* Auromatic MIR time calculation flag */

char    ps[MAXSTR];                          /* Pre Saturation (PS) flag */
char    psplan[MAXSTR];                      /* PS graphical planning flag */
RF_PULSE_T ps_rf;                            /* PS pulse */
char    pspat[MAXSTR];                       /* PS pulse shape */
double  pps;                                 /* PS pulse duration */
double  flipps;                              /* PS pulse flip */
double  flippsf;                             /* PS pulse flip factor */
double  pssPs[MAXSLICE];                     /* Position of PS pulse */
double  freqPs[MAXSLICE];                    /* Frequency of PS pulse */
int     shapePs;                             /* Shape list ID for PS pulse */
int     nps;                                 /* number of PS pulses */
SLICE_SELECT_GRADIENT_T ps_grad;             /* PS gradient */
double  psthk;                               /* PS slice thickness */
double  psaddthk;                            /* PS additional slice thickness */
double  pspos;                               /* PS slice position */
double  pspsi,psphi,pstheta;                 /* PS slice orientation */
GENERIC_GRADIENT_T psspoil_grad;             /* PS spoil gradient */
double  gspoilps;                            /* PS spoil gradient amplitude (input) */
double  psgamp;                              /* PS spoil gradient amplitude (actual) */
double  tspoilps;                            /* PS spoil gradient duration */
char    wetps[MAXSTR];                       /* WET PS flag */
double  psTime;                              /* PS module duration */

char    q2tips[MAXSTR];                      /* Q2TIPS flag */
char    q2plan[MAXSTR];                      /* Q2TIPS graphical planning flag */
RF_PULSE_T q2_rf;                            /* Q2TIPS pulse */
char    q2pat[MAXSTR];                       /* Q2TIPS pulse shape */
double  pq2;                                 /* Q2TIPS pulse duration */
double  flipq2;                              /* Q2TIPS pulse flip */
double  pssQ2[MAXSLICE];                     /* Position of Q2TIPS pulse */
double  freqQ2[MAXSLICE];                    /* Frequency of Q2TIPS pulse */
int     shapeQ2;                             /* Shape list ID for Q2TIPS pulse */
double  nq2;                                 /* number of Q2TIPS pulses */
SLICE_SELECT_GRADIENT_T q2_grad;             /* Q2TIPS gradient */
double  q2thk;                               /* Q2TIPS slice thickness */
double  q2pos;                               /* Q2TIPS slice position */
double  q2psi,q2phi,q2theta;                 /* Q2TIPS slice orientation (FAIR) */
GENERIC_GRADIENT_T q2spoil_grad;             /* Q2TIPS spoil gradient */
double  gspoilq2;                            /* Q2TIPS spoil gradient amplitude */
double  tspoilq2;                            /* Q2TIPS spoil gradient duration */
double  q2ti;                                /* ASL inflow time to Q2TIPS */
char    minq2ti[MAXSTR];                     /* minimum ASL inflow time to Q2TIPS flag: y = ON, n = OFF */
double  q2ti_delay;                          /* Q2TIPS inflow delay */
double  q2Time;                              /* Q2TIPS module duration */

char    vascsup[MAXSTR];                     /* Vascular suppression (VS) flag */
GENERIC_GRADIENT_T vs_grad;                  /* VS gradient */
double  gvs;                                 /* VS gradient amplitude */
double  tdeltavs;                            /* VS gradient delta */
double  bvalvs;                              /* VS b-value */
double  vsTime=0.0;                          /* VS module duration */

char    asltest[MAXSTR];                     /* ASL test parameter for test mode (asltestmode='y') */

enum {TAGOFF=0,FAIR=1,STAR=2,PICORE=3,CASL=4};

/* Below are additional variables required for asl sequence */
char   quipss[MAXSTR];  // QUIPSS II/Q2TIPS saturation flag
double irthk,           // thickness of IR slab
       irgap,           // distance from imaging slice
       asl_ssiamp,      // amplitude of ssi gradient; depends on asltag
       asl_shapeIR,     // RF phase-ramped shapes
       asl_shapeQtag,
       aslgcrush, asltcrush,
       qgcrush,qtcrush, // Crushers after Q2TIPS saturation
       asl_ti1,         // Time to apply Q2TIPS sat bands
       quipssTime=0;    // Duration of Q2TIPS saturation block
int    nquipss;

GENERIC_GRADIENT_T      aslcrush_grad;
SLICE_SELECT_GRADIENT_T q2tips_grad;
GENERIC_GRADIENT_T      qcrush_grad;
/* Above are additional variables required for asl sequence */


/* Gradient structures */
SATBAND_GRADIENT_T       satss_grad[6];      /* slice select - SATBAND array*/
SATBAND_INFO_T           satband_info;       /* arrayed satband values */

SLICE_SELECT_GRADIENT_T  ss_grad;            /* slice select gradient (90) */
SLICE_SELECT_GRADIENT_T  ss2_grad;           /* slice select gradient (180) */
SLICE_SELECT_GRADIENT_T  ssi_grad;           /* IR slice select gradient */
SLICE_SELECT_GRADIENT_T  sat_grad;           /* satband slice select gradient */

REFOCUS_GRADIENT_T       ssr_grad;           /* slice refocus gradient */
REFOCUS_GRADIENT_T       ror_grad;           /* dephase gradient */

PHASE_ENCODE_GRADIENT_T  pe_grad;            /* phase encode gradient */
PHASE_ENCODE_GRADIENT_T  per_grad;           /* phase rewind gradient */
PHASE_ENCODE_GRADIENT_T  pe2_grad;           /* phase encode gradient in 2nd dimension */
PHASE_ENCODE_GRADIENT_T  pe2r_grad;          /* phase rewind gradient in 2nd dimension */
PHASE_ENCODE_GRADIENT_T  pe3_grad;           /* phase encode gradient in 2nd dimension */
PHASE_ENCODE_GRADIENT_T  pe3r_grad;          /* phase rewind gradient in 2nd dimension */

READOUT_GRADIENT_T       ro_grad;            /* readout gradient */
READOUT_GRADIENT_T       nav_grad;           /* navigator gradient */

PHASE_ENCODE_GRADIENT_T  epipe_grad;         /* EPI phase encode gradient */
READOUT_GRADIENT_T       epiro_grad;         /* EPI readout gradient */
EPI_GRADIENT_T epi_grad;                     /* General EPI struct */

GENERIC_GRADIENT_T       fsatcrush_grad;     /* crusher gradient structure */
GENERIC_GRADIENT_T       mtcrush_grad;       /* crusher gradient structure */
GENERIC_GRADIENT_T       satcrush_grad;      /* crusher gradient structure */
GENERIC_GRADIENT_T       tagcrush_grad;      /* crusher gradient structure */
GENERIC_GRADIENT_T       tag_grad;           /* tagging gradient structure */
GENERIC_GRADIENT_T       crush_grad;         /* crusher gradient structure */
GENERIC_GRADIENT_T       spoil_grad;         /* spoiler gradient structure */
GENERIC_GRADIENT_T       diff_grad;          /* diffusion gradient structure */
GENERIC_GRADIENT_T       gg_grad;            /* generic gradient structure */
GENERIC_GRADIENT_T       ircrush_grad;       /* IR crusher gradient strucuture */
GENERIC_GRADIENT_T       navror_grad;        /* Navigator refocus gradient */

REFOCUS_GRADIENT_T ssd_grad;
REFOCUS_GRADIENT_T rod_grad;
        
NULL_GRADIENT_T          null_grad;          /* NULL gradient strucuture */

KERNEL_MARKER_T		marker;

/* RF-pulse structures */
RF_PULSE_T    p1_rf;              /* RF pulse structure */
RF_PULSE_T    p2_rf;              
RF_PULSE_T    p3_rf;              
RF_PULSE_T    p4_rf;              
RF_PULSE_T    p5_rf;              
RF_PULSE_T    mt_rf;              /* MTC RF pulse structure */
RF_PULSE_T    fsat_rf;            /* fatsat RF pulse structure */
RF_PULSE_T    sat_rf;             /* SATBAND RF pulse structure */
RF_PULSE_T    tag_rf;             /* RF TAGGIGN pulse structure */
RF_PULSE_T    ir_rf;              /* IR RF-pulse */
RF_PULSE_T    ws_rf;              /* Water suppression */

VINFO_T       dvarinfo;           /* variable info as defined in variables.h */
ARRAYPARS_T   arraypars;          /* array parameters */

char checkduty[MAXSTR];

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----- E N D   O F   P A R A M E T E R   D E C L A R A T I O N  -------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*-------------------- S T A R T  O F    S G L   ---- ------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/


/***********************************************************************
*  Function Name: forceRampTime
*  Example:    forceRampTime(double *tramp);
*  Purpose:    Enforces a constant ramp time if trampfixed exists and
*              is larger 0.0.  The ramp time will be set to the larger
*              value of T_RISE and trampfixed
*
*  Input
*     Formal:  *tramp - pointer to ramp time in  structure
*     Private: none
*     Public:  
*  Output
*     Return:  none
*     Formal:  *tramp - pointer to ramp time in  structure
*     Private: none
*     Public:  none
*  Notes:      This is a workaround to mitigate the gradient amplifier 
*              and gradient coil oscillation in some systems  
***********************************************************************/
void forceRampTime( double *tramp)
{
			if (FP_GT(trampfixed, 0.0))
			   {
						/* enforce minimum of T_RISE */
						if ( FP_LT(*tramp, T_RISE)) 
						   {
   						*tramp = granularity(T_RISE, GRADIENT_RES);
									}
					
						/* use the larger of tramp and trampfixed */
						if ( FP_LT(*tramp,trampfixed))
						   {
						   *tramp = granularity(trampfixed, GRADIENT_RES);   /* set to predetermined ramp time - fixed */
									}
						}
}

/***********************************************************************
*  Function Name: adjustCalcFlag
*  Example:    adjustCalcFlag(CALC_FLAG_T *falg);
*  Purpose:    Enforces that the  calcFalg is set correctly if the 
*              contant ramp time variablew is set and larger 0.0
*
*  Input
*     Formal:  *flag - pointer to calcFlag  in  structure
*     Private: none
*     Public:  
*  Output
*     Return:  none
*     Formal:  *flag - pointer to calcFlag  in  structure
*     Private: none
*     Public:  none
*  Notes:      This is a workaround to mitigate the gradient amplifier 
*              and gradient coil oscillation in some systems  
***********************************************************************/
void adjustCalcFlag( CALC_FLAG_T *flag)
{
   if (FP_GT(trampfixed, 0.0))
  {
    switch ( *flag)
    {
      case MOMENT_FROM_DURATION_AMPLITUDE:
        *flag = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
        break;
      case AMPLITUDE_FROM_MOMENT_DURATION:
        *flag = AMPLITUDE_FROM_MOMENT_DURATION_RAMP;
        break;
      case DURATION_FROM_MOMENT_AMPLITUDE:
        *flag = DURATION_FROM_MOMENT_AMPLITUDE_RAMP;
        break;				
      case SHORTEST_DURATION_FROM_MOMENT:		
        *flag = SHORTEST_DURATION_FROM_MOMENT_RAMP;
        break;
      case MOMENT_FROM_DURATION_AMPLITUDE_RAMP:
        break;
      case AMPLITUDE_FROM_MOMENT_DURATION_RAMP:
        break;
      case DURATION_FROM_MOMENT_AMPLITUDE_RAMP:
        break;
      case SHORTEST_DURATION_FROM_MOMENT_RAMP:
        break;
      default:
        break;
    }
  }
}

/***********************************************************************
*  Function Name: initSliceSelect
*  Example:    initSliceSelect(&ss);
*  Purpose:    Initialize slice select gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRAD_MAX      - maximum possible gradient amplitude
*              GRADIENT_RES  - system timing resolution
*              MAX_SLEW_RATE - maximum slew rate of system
*  Output
*     Return:  none
*     Formal:  *slice - pointer to slice select gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initSliceSelect (SLICE_SELECT_GRADIENT_T *slice)
{
   slice->type             = SLICE_SELECT_GRADIENT_TYPE;
   slice->amp              = 0;
   slice->dataPoints       = NULL;
   slice->duration         = 0;
   slice->enableButterfly  = FALSE;
   slice->error            = ERR_NO_ERROR;
   slice->maxGrad          = GRAD_MAX;
   slice->m0               = 0;
   slice->m0ref            = 0;
   slice->m0def            = 0;
   slice->m1               = 0;
   slice->m1ref            = 0;
   strcpy(slice->name, "slice_");
   slice->numPoints        = 0;
   slice->pad1             = 0;
   slice->pad2             = 0;
   slice->plateauDuration  = 0;
   slice->rampShape        = PRIMITIVE_LINEAR;
   slice->tramp            = 0;
   slice->resolution       = GRADIENT_RES;
   slice->rfBandwidth      = 0;
   slice->rfDelayFront     = 0;
   slice->rfDelayBack      = 0;
   slice->rfDuration       = 0;
   slice->rfFraction       = 0.5;
   slice->rfCenterFront    = 0;
   slice->rfCenterBack     = 0;
   strcpy(slice->rfName, "slice_");
   slice->rollOut          = ROLL_OUT;
   slice->slewRate         = MAX_SLEW_RATE;
   slice->ssamp            = 0.0;
   slice->ssDuration       = 0;
   slice->thickness        = 0;
   slice->gamma            = nuc_gamma();
   slice->writeToDisk      = TRUE;
			
   /* first crusher */
   slice->cr1amp                        = 0;
   slice->crusher1CalcFlag              = MOMENT_FROM_DURATION_AMPLITUDE;
   slice->crusher1Duration              = 0;
   slice->crusher1Moment0               = 0;
   slice->crusher1RampToCrusherDuration = 0;
   slice->crusher1RampToSsDuration      = 0;

   /* second crusher */
   slice->cr2amp                        = 0;
   slice->crusher2CalcFlag              = MOMENT_FROM_DURATION_AMPLITUDE;
   slice->crusher2Duration              = 0;
   slice->crusher2Moment0               = 0;
   slice->crusher2RampToCrusherDuration = 0;
   slice->crusher2RampToSsDuration      = 0;
			
			/**********************************************************/
			/* modify initialization for fixed ramp time (trampfixed) */
			/* to mitigate the COupley amp gradient problem           */
			forceRampTime(&slice->tramp);
   forceRampTime(&slice->crusher1RampToCrusherDuration);
   forceRampTime(&slice->crusher1RampToSsDuration);
 		adjustCalcFlag(&slice->crusher1CalcFlag);
   forceRampTime(&slice->crusher2RampToCrusherDuration);
   forceRampTime(&slice->crusher2RampToSsDuration);
 		adjustCalcFlag(&slice->crusher2CalcFlag);			

}

/***********************************************************************
*  Function Name: initRefocus
*  Example:    initRefocus(&refocusGradient);
*  Purpose:    Initialize refocus gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRAD_MAX     - maximum possible gradient amplitude
*              GRADIENT_RES - system timing resolution
*  Output
*     Return:  none
*     Formal:  *refocus - pointer to refocus gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initRefocus (REFOCUS_GRADIENT_T *refocus)
{
   refocus->amp              = 0;
   refocus->amplitudeOffset  = 0;                 /* not utilized in this structure */
   refocus->areaOffset       = 0;                 /* not utilized in this structure */
   refocus->balancingMoment0 = 0;
   refocus->gmult            = 1.0;
   refocus->calcFlag         = SHORTEST_DURATION_FROM_MOMENT;
   refocus->dataPoints       = NULL;
   refocus->display          = sgldisplay;
   refocus->error            = ERR_NO_ERROR;
   refocus->fov              = 0;                 /* not utilized in this structure */
   refocus->increment        = 0;                 /* not utilized in this structure */
   refocus->duration         = 0;
   refocus->maxGrad          = GRAD_MAX;
   refocus->m0               = 0;
   refocus->m1               = 0;
   strcpy(refocus->name, "refocus_");
   refocus->noPlateau        = FALSE;             /* not utilized in this structure */   
   refocus->numPoints        = 0;
   strcpy(refocus->param1,"");
   strcpy(refocus->param2,"");
   refocus->polarity         = FALSE;             /* not utilized in this structure */   
   refocus->rampShape        = PRIMITIVE_LINEAR;
   refocus->resolution       = GRADIENT_RES;
   refocus->rollOut          = ROLL_OUT;
   refocus->shape            = TRAPEZOID;
   refocus->startAmplitude   = 0;                 /* not utilized in this structure */      
   refocus->steps            = 0;                 /* not utilized in this structure */
   refocus->slewRate         = MAX_SLEW_RATE;
   refocus->tramp            = 0;
   refocus->type             = REFOCUS_GRADIENT_TYPE;
   refocus->gamma            = nuc_gamma();
   refocus->writeToDisk      = TRUE;
   
   /* initialize primitive gradient structures */
   initPrimitive(&refocus->ramp1);                /* not utilized in this structure */      
   initPrimitive(&refocus->plateau);              /* not utilized in this structure */      
   initPrimitive(&refocus->ramp2);                /* not utilized in this structure */      

				/**********************************************************/
			/* modify initialization for fixed ramp time (trampfixed) */
			/* to mitigate the COupley amp gradient problem           */
			forceRampTime(&refocus->tramp);
 		adjustCalcFlag(&refocus->calcFlag);

}

/***********************************************************************
*  Function Name: initDephase
*  Example:    initDephase(&dephaseGradient);
*  Purpose:    Initialize refocus gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRAD_MAX     - maximum possible gradient amplitude
*              GRADIENT_RES - system timing resolution
*  Output
*     Return:  none
*     Formal:  *dephase - pointer to dephase gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initDephase (REFOCUS_GRADIENT_T *dephase)
{
   dephase->amp              = 0;
   dephase->amplitudeOffset  = 0;                 /* not utilized in this structure */
   dephase->areaOffset       = 0;                 /* not utilized in this structure */
   dephase->balancingMoment0 = 0;
   dephase->gmult            = 1.0;
   dephase->calcFlag         = SHORTEST_DURATION_FROM_MOMENT;
			dephase->dataPoints       = NULL;
   dephase->display          = sgldisplay;
   dephase->duration         = 0;
   dephase->error            = ERR_NO_ERROR;
   dephase->fov              = 0;                 /* not utilized in this structure */
   dephase->increment        = 0;                 /* not utilized in this structure */
   dephase->maxGrad          = GRAD_MAX;
   dephase->m0               = 0;
   dephase->m1               = 0;
   strcpy(dephase->name, "dephase_");
   dephase->noPlateau        = FALSE;             /* not utilized in this structure */
   dephase->numPoints        = 0;
   strcpy(dephase->param1,"");
   strcpy(dephase->param2,"");
   dephase->polarity         = FALSE;             /* not utilized in this structure */
   dephase->rampShape        = PRIMITIVE_LINEAR;
   dephase->resolution       = GRADIENT_RES;
   dephase->rollOut          = ROLL_OUT;
   dephase->shape            = TRAPEZOID;
   dephase->slewRate         = MAX_SLEW_RATE;
   dephase->startAmplitude   = 0;                 /* not utilized in this structure */      
   dephase->steps            = 0;                 /* not utilized in this structure */
   dephase->tramp            = 0;
   dephase->type             = DEPHASE_GRADIENT_TYPE;			
   dephase->gamma            = nuc_gamma();
   dephase->writeToDisk      = TRUE;
   
   /* initialize primitive gradient structures */
   initPrimitive(&dephase->ramp1);                /* not utilized in this structure */      
   initPrimitive(&dephase->plateau);              /* not utilized in this structure */      
   initPrimitive(&dephase->ramp2);                /* not utilized in this structure */      
			
				/**********************************************************/
			/* modify initialization for fixed ramp time (trampfixed) */
			/* to mitigate the COupley amp gradient problem           */
			forceRampTime(&dephase->tramp);
 		adjustCalcFlag(&dephase->calcFlag);

}

/***********************************************************************
*  Function Name: initReadout
*  Example:    initReadout(&readoutGradient);
*  Purpose:    Initialize readout gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRADIENT_RES  - system timing resolution
*              MAX_SLEW_RATE - maximum slew rate of system
*              GRAD_MAX      - maximum possible gradient amplitude
*  Output
*     Return:  none
*     Formal:  *readout - pointer to readout gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initReadout (READOUT_GRADIENT_T *readout)
{
   readout->acqTime          = 0;
   readout->amp              = 0;
   readout->atDelayFront     = 0;
   readout->atDelayBack      = 0;
   readout->bandwidth        = 0;
   readout->dataPoints       = NULL;			
   readout->duration         = 0;
   readout->echoFraction     = 1;
   readout->enableButterfly  = FALSE;
   readout->error            = ERR_NO_ERROR;
   readout->fov              = 0;
   readout->fracAcqTime      = 0;
   readout->maxGrad          = GRAD_MAX;
   readout->m0               = 0;
   readout->m0ref            = 0;
   readout->m0def            = 0;
   readout->m1               = 0;
   readout->m1ref            = 0;
   strcpy(readout->name, "readout_");
   readout->numPoints        = 0;
   readout->numPointsFreq    = 0;
   readout->pad1             = 0;
   readout->pad2             = 0;
   readout->plateauDuration  = 0;
   readout->rampShape        = PRIMITIVE_LINEAR;
   readout->tramp            = 0;
   readout->timeToEcho       = 0;
   readout->timeFromEcho       = 0;
   readout->resolution       = GRADIENT_RES;
   readout->roamp            = 0;
   readout->rollOut          = TRUE;
   readout->slewRate         = MAX_SLEW_RATE;
   readout->type             = READOUT_GRADIENT_TYPE;
   readout->gamma            = nuc_gamma();
   readout->writeToDisk      = TRUE;
			   
   /* first crusher */
   readout->cr1amp            = 0;
   readout->crusher1CalcFlag  = MOMENT_FROM_DURATION_AMPLITUDE;
   readout->crusher1Duration  = 0;
   readout->crusher1Moment0   = 0;
   readout->crusher1RampToCrusherDuration = 0;
   readout->crusher1RampToSsDuration = 0;
   /* second crusher */
   readout->cr2amp            = 0;
   readout->crusher2CalcFlag  = MOMENT_FROM_DURATION_AMPLITUDE;
   readout->crusher2Duration  = 0;
   readout->crusher2Moment0   = 0;
   readout->crusher2RampToCrusherDuration = 0;
   readout->crusher2RampToSsDuration = 0;
			
				/**********************************************************/
			/* modify initialization for fixed ramp time (trampfixed) */
			/* to mitigate the COupley amp gradient problem           */
			forceRampTime(&readout->tramp);

   forceRampTime(&readout->crusher1RampToCrusherDuration);
   forceRampTime(&readout->crusher1RampToSsDuration);
 		adjustCalcFlag(&readout->crusher1CalcFlag);
   forceRampTime(&readout->crusher2RampToCrusherDuration);
   forceRampTime(&readout->crusher2RampToSsDuration);
 		adjustCalcFlag(&readout->crusher2CalcFlag);
}

/***********************************************************************
*  Function Name: initBandwidth
*  Example:    initBandwidth(&bandwidth)
*  Purpose:    Initialize bandwidth structure.
*  Input
*     Formal:  none
*     Private: none 
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *bw - pointer to bandwidth structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initBandwidth (BANDWIDTH_T *bw)
{
   bw->acqTime      = 0;
   bw->bw           = 0;
   bw->error        = 0;
   bw->points       = 0;
   bw->readFraction = 1;
   bw->resolution   = GRADIENT_RES;
}

/***********************************************************************
*  Function Name: initPhase
*  Example:    initPhase(&phaseEncode);
*  Purpose:    Initialize phase encode gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRADIENT_RES  - system timing resolution
*              MAX_SLEW_RATE - maximum slew rate of system
*              GRAD_MAX      - maximum possible gradient amplitude
*  Output
*     Return:  none
*     Formal:  *phase - pointer to phase encode gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initPhase (PHASE_ENCODE_GRADIENT_T *phase)
{

   phase->amp             = 0;
   phase->amplitudeOffset = 0;
   phase->areaOffset      = 0;
   phase->balancingMoment0= 0;              /* not utilized in this structure */      
   phase->gmult           = 1.0;
   phase->calcFlag        = SHORTEST_DURATION_FROM_MOMENT;
			phase->dataPoints      = NULL;
   phase->display         = sgldisplay;
   phase->duration        = 0;
   phase->error           = ERR_NO_ERROR;
   phase->fov             = 0;
   phase->increment       = 0;
   phase->maxGrad         = GRAD_MAX;
   phase->m0              = 0;
   phase->m1              = 0;
   strcpy(phase->name, "phase_");
   phase->noPlateau       = FALSE;
   phase->numPoints       = 0;
   phase->offsetamp       = 0;
   phase->peamp           = 0;
   strcpy(phase->param1,"");
   strcpy(phase->param2,"");
   phase->polarity        = FALSE;        /* not utilized in this structure */       
   phase->rampShape       = PRIMITIVE_LINEAR;
   phase->resolution      = GRADIENT_RES;
   phase->rollOut         = ROLL_OUT;
   phase->shape           = TRAPEZOID;
   phase->slewRate        = MAX_SLEW_RATE;
   phase->startAmplitude  = 0;           /* not utilized in this structure */       
   phase->steps           = 0;
   phase->tramp           = 0;
   phase->type            = PHASE_GRADIENT_TYPE;
   phase->gamma            = nuc_gamma();
   phase->writeToDisk     = TRUE;
   
   /* initialize primitive gradient structures */
   initPrimitive(&phase->ramp1);                /* not utilized in this structure */      
   initPrimitive(&phase->plateau);              /* not utilized in this structure */      
   initPrimitive(&phase->ramp2);                /* not utilized in this structure */      

				/**********************************************************/
			/* modify initialization for fixed ramp time (trampfixed) */
			/* to mitigate the COupley amp gradient problem           */
			forceRampTime(&phase->tramp);
 		adjustCalcFlag(&phase->calcFlag);

}

/***********************************************************************
*  Function Name: initZeroFillGradient
*  Example:    initZeroFillGradient(&zg);
*  Purpose:    Initialize zero fill gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  zg - zero filling gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initZeroFillGradient (ZERO_FILL_GRADIENT_T *zg)
{
			zg->dataPoints  = NULL;
   zg->error       = ERR_NO_ERROR;
   zg->location    = BACK;
   strcpy(zg->name, "zeroFill_");
   zg->newDuration = 0;
   zg->numPoints   = 0;
   zg->resolution  = GRADIENT_RES;
   zg->rollOut     = ROLL_OUT;
   zg->timeToPad   = 0;
   zg->writeToDisk = TRUE;
}

/***********************************************************************
*  Function Name: initRfHeader
*  Example:    initRfHeader(&someRfHeader);
*  Purpose:    Initialize RF header structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *rfHeader - pointer to rf header structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initRfHeader (RF_HEADER_T *rfHeader)
{
   rfHeader->bandwidth   = 0;
   rfHeader->integral    = 0;
   rfHeader->inversionBw = 0;
   strcpy(rfHeader->modulation, "");
   rfHeader->rfFraction  = 0.5;
   strcpy(rfHeader->type, "");
   rfHeader->version     = 0;   
}

/***********************************************************************
*  Function Name: initRf
*  Example:    initRf(&someRf);
*  Purpose:    Initialize RF structure.
*  Input
*     Formal:  none
*     Private: none 
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *rf - pointer to rf structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initRf (RF_PULSE_T *rf)
{
   rf->bandwidth   = 0;
   rf->display     = sgldisplay;
   rf->error       = 0;
   rf->flip        = 90;
   rf->flipmult    = 1;
   initRfHeader(&rf->header);
   rf->powerCoarse = 0;
   rf->powerFine   = 0;
   strcpy(rf->pulseName, "sinc");
   rf->rfDuration  = 0;
   rf->rof1        = RF_UNBLANK;
   rf->rof2        = RF_BLANK;
   rf->sar         = 0;
}

/***********************************************************************
*  Function Name: initGradFormList
*  Example:    initGradFormList(&list);
*  Purpose:    Initialize gradient waveform list.
*  Input
*     Formal:  none
*     Private: none 
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *list - pointer to rf structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initGradFormList (GRAD_FORM_LIST_T *list)
{
   long i;

   list->number = 0; /* set counter to start */
   for (i = 0; i < MAX_GRAD_FORMS; i++)
   {
      /* initialize all waveforms to NULL */
      strcpy(list->name[i], "");
   }
}

/***********************************************************************
*  Function Name: initShape
*  Example:    initShape(&shape);
*  Purpose:    Initialize shape structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *shape - pointer to shape structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initShape (PRIMITIVE_SHAPE_T *shape)
{
   shape->function    = PRIMITIVE_LINEAR;
   shape->startX      = 0;
   shape->endX        = 0;
   shape->powerFactor = 1;
}

/***********************************************************************
*  Function Name: initPrimitive
*  Example:    initPrimitive(&primitive);
*  Purpose:    Initialize primitive gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRAD_MAX      - maximum possible gradient amplitude
*              GRADIENT_RES  - system timing resolution
*              MAX_SLEW_RATE - maximum slew rate of system
*  Output
*     Return:  none
*     Formal:  *prim - pointer to primitive gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initPrimitive (PRIMITIVE_GRADIENT_T *prim)
{
   prim->dataPoints			  = NULL;
   prim->duration       = 0;
   prim->endAmplitude   = 0;
   prim->error          = ERR_NO_ERROR;
   prim->maxGrad        = GRAD_MAX;
   prim->m0             = 0;
   prim->m1             = 0;
   strcpy(prim->name, "primitive_");
   prim->numPoints      = 0;
   prim->resolution     = GRADIENT_RES;
   prim->rollOut        = ROLL_OUT;
   initShape(&prim->shape);
   prim->slewRate       = MAX_SLEW_RATE;
   prim->startAmplitude = 0;
   prim->writeToDisk    = TRUE;
}

/***********************************************************************
*  Function Name: initGeneric
*  Example:    initGeneric(&genericGradient);
*  Purpose:    Initialize generic gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRAD_MAX      - maximum possible gradient amplitude
*              GRADIENT_RES  - system timing resolution
*              MAX_SLEW_RATE - maximum slew rate of system
*  Output
*     Return:  none
*     Formal:  *generic - pointer to generic gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initGeneric (GENERIC_GRADIENT_T *generic)
{
   generic->type             = GENERIC_GRADIENT_TYPE;
   generic->amp              = 0;
   generic->amplitudeOffset  = 0;                 /* not utilized in this structure */
   generic->areaOffset       = 0;                 /* not utilized in this structure */
   generic->balancingMoment0 = 0;                 /* not utilized in this structure */
   generic->calcFlag         = MOMENT_FROM_DURATION_AMPLITUDE;
   generic->dataPoints       = NULL;
   generic->display          = sgldisplay;
   generic->duration         = 0;
   generic->error            = 0;
   generic->fov              = 0;                 /* not utilized in this structure */
   generic->increment        = 0;                 /* not utilized in this structure */
   generic->maxGrad          = GRAD_MAX;
   generic->m0               = 0;
   generic->m1               = 0;
   strcpy(generic->name, "generic_");
   generic->noPlateau        = FALSE;
   generic->numPoints        = 0;
   strcpy(generic->param1,"");
   strcpy(generic->param2,"");
   generic->polarity         = FALSE;
   generic->resolution       = GRADIENT_RES;
   generic->rollOut          = ROLL_OUT;
   generic->shape            = TRAPEZOID;
   generic->slewRate         = MAX_SLEW_RATE;
   generic->startAmplitude   = 0;
   generic->steps            = 0;                 /* not utilized in this structure */
   generic->tramp            = 0;
   generic->gamma            = nuc_gamma();
   generic->writeToDisk      = TRUE;

   /* initialize primitive gradient structures */
   initPrimitive(&generic->ramp1);
   initPrimitive(&generic->plateau);
   initPrimitive(&generic->ramp2);
			
				/**********************************************************/
			/* modify initialization for fixed ramp time (trampfixed) */
			/* to mitigate the COupley amp gradient problem           */
			forceRampTime(&generic->tramp);
 		adjustCalcFlag(&generic->calcFlag);

}

/***********************************************************************
*  Function Name: initNull
*  Example:    initNull(&nul;lGradient);
*  Purpose:    Initialize null / dummy gradient (empty generic structure).
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRAD_MAX      - maximum possible gradient amplitude
*              GRADIENT_RES  - system timing resolution
*              MAX_SLEW_RATE - maximum slew rate of system
*  Output
*     Return:  none
*     Formal:  *null_grad - pointer to null / dummy gradient structure
*     Private: none
*     Public:  none
*  Notes:     Is not used to calculate anything - only a placeholder!
***********************************************************************/
void initNull (NULL_GRADIENT_T *null_grad)
{
   null_grad->amp              = 0;
   null_grad->amplitudeOffset  = 0;               
   null_grad->areaOffset       = 0;               
   null_grad->balancingMoment0 = 0;               
   null_grad->calcFlag         = MOMENT_FROM_DURATION_AMPLITUDE;
   null_grad->dataPoints       = NULL;
   null_grad->display          = DISPLAY_NONE;
   null_grad->duration         = 0;
   null_grad->error            = 0;
   null_grad->fov              = 0;               
   null_grad->increment        = 0;               
   null_grad->maxGrad          = 0;;
   null_grad->m0               = 0;
   null_grad->m1               = 0;
   strcpy(null_grad->name, "null");
   null_grad->noPlateau        = FALSE;
   null_grad->numPoints        = 0;
   strcpy(null_grad->param1,"");
   strcpy(null_grad->param2,"");
   null_grad->polarity         = FALSE;
   null_grad->resolution       = 0;
   null_grad->rollOut          = 0;
   null_grad->shape            = TRAPEZOID;
   null_grad->slewRate         = MAX_SLEW_RATE;
   null_grad->startAmplitude   = 0;
   null_grad->steps            = 0;
   null_grad->tramp            = 0;
   null_grad->type             = NULL_GRADIENT_TYPE;
   null_grad->gamma            = nuc_gamma();
   null_grad->writeToDisk      = FALSE;
   
   /* initialize primitive gradient structures */
   initPrimitive(&null_grad->ramp1);
   initPrimitive(&null_grad->plateau);
   initPrimitive(&null_grad->ramp2);
			
			/**********************************************************/
			/* modify initialization for fixed ramp time (trampfixed) */
			/* to mitigate the COupley amp gradient problem           */
			forceRampTime(&null_grad->tramp);
 		adjustCalcFlag(&null_grad->calcFlag);
					
}

/***********************************************************************
*  Function Name: initButterfly
*  Example:    initButterfly(&bfly);
*  Purpose:    Initialize butterfly gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRAD_MAX      - maximum possible gradient amplitude
*              MAX_SLEW_RATE - maximum slew rate of system
*              GRADIENT_RES  - system timing resolution
*  Output
*     Return:  none
*     Formal:  *bfly - pointer to butterfly gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initButterfly(BUTTERFLY_GRADIENT_T *bfly)
{
   bfly->amp         = 0;
   bfly->dataPoints  = NULL;
   bfly->error       = 0;
   bfly->maxGrad     = GRAD_MAX;
   bfly->m0          = 0;
   bfly->m1          = 0;
   strcpy(bfly->name, "butterfly_");
   bfly->numPoints   = 0;
   bfly->resolution  = GRADIENT_RES;
   bfly->rollOut     = ROLL_OUT;
   bfly->slewRate    = MAX_SLEW_RATE;
   bfly->gamma       = nuc_gamma();
   bfly->writeToDisk = TRUE;

   /* first crusher */
   bfly->cr1amp           = 0;
   bfly->cr1CalcFlag      = MOMENT_FROM_DURATION_AMPLITUDE;
   bfly->cr1Moment0       = 0;
   bfly->cr1TotalDuration = 0;

   /* slice select */
   bfly->ssamp           = 0;
   bfly->ssCalcFlag      = MOMENT_FROM_DURATION_AMPLITUDE;
   bfly->ssMoment0       = 0;
   bfly->ssTotalDuration = 0;

   /* second crusher */
   bfly->cr2amp           = 0;
   bfly->cr2CalcFlag      = MOMENT_FROM_DURATION_AMPLITUDE;
   bfly->cr2Moment0       = 0;
   bfly->cr2TotalDuration = 0;

   /* initialize primitive gradient structures */
   initPrimitive(&bfly->ramp1);
   initPrimitive(&bfly->cr1);
   initPrimitive(&bfly->ramp2);
   initPrimitive(&bfly->ss);
   initPrimitive(&bfly->ramp3);
   initPrimitive(&bfly->cr2);
   initPrimitive(&bfly->ramp4);
}

/***********************************************************************
*  Function Name: initGradientHeader
*  Example:    initGradientHeader(&gHeader);
*  Purpose:    Initialize gradient header structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRADIENT_RES - system timing resolution
*  Output
*     Return:  none
*     Formal:  *header - pointer to gradient header structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initGradientHeader (GRADIENT_HEADER_T *header)
{
   header->error      = 0;
   strcpy(header->name, "");
   header->points     = 100;
   header->resolution = GRADIENT_RES;
   header->strength   = 2.3;
}

/***********************************************************************
*  Function Name: initSliceFlowcomp
*  Example:    initSliceFlowcomp(&sliceFlowcomp);
*  Purpose:    Initialize slice flowcomp gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRAD_MAX     - maximum possible gradient amplitude
*              GRADIENT_RES - system timing resolution
*  Output
*     Return:  none
*     Formal:  *flowcomp - pointer to flowcomp gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initSliceFlowcomp (FLOWCOMP_T *flowcomp)
{
   flowcomp->type         = FLOWCOMP_TYPE;
   flowcomp->amp          = 0;
   flowcomp->amplitude1   = 0;
   flowcomp->amplitude2   = 0;
			flowcomp->calcFlag     = SHORTEST_DURATION_FROM_MOMENT;
			flowcomp->dataPoints   = NULL;
   flowcomp->duration     = 0;
   flowcomp->duration1    = 0;
   flowcomp->duration2    = 0;
			flowcomp->separation   = 0;
   flowcomp->error        = 0;

   initGeneric(&flowcomp->lobe1);
			initPrimitive(&flowcomp->plat);
   initGeneric(&flowcomp->lobe2);

   flowcomp->m0           = 0;
			flowcomp->m1           = 0;
   flowcomp->maxGrad      = GRAD_MAX*glim;
   strcpy(flowcomp->name, "flowcomp_slice_");
   flowcomp->numPoints    = 0;
   flowcomp->rampTime1    = 0;
   flowcomp->rampTime2    = 0;
   flowcomp->resolution   = GRADIENT_RES;
   flowcomp->rollOut      = ROLL_OUT;
   flowcomp->writeToDisk  = TRUE;

			/**********************************************************/
			/* modify initialization for fixed ramp time (trampfixed) */
			/* to mitigate the COupley amp gradient problem           */
			forceRampTime(&flowcomp->rampTime1);
			forceRampTime(&flowcomp->rampTime2);
 		adjustCalcFlag(&flowcomp->calcFlag);
}

/***********************************************************************
*  Function Name: initReadoutFlowcomp
*  Example:    initReadoutFlowcomp(&readoutGlowcomp);
*  Purpose:    Initialize readout flowcomp gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  GRAD_MAX     - maximum possible gradient amplitude
*              GRADIENT_RES - system timing resolution
*  Output
*     Return:  none
*     Formal:  *flowcomp - pointer to flowcomp gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void initReadoutFlowcomp (FLOWCOMP_T *flowcomp)
{
   flowcomp->type       = FLOWCOMP_TYPE;
   flowcomp->amp        = 0;
   flowcomp->amplitude1 = 0;
   flowcomp->amplitude2 = 0;
			flowcomp->calcFlag   = SHORTEST_DURATION_FROM_MOMENT;
			flowcomp->dataPoints = NULL;
   flowcomp->duration   = 0;
   flowcomp->duration1  = 0;
   flowcomp->duration2  = 0;
			flowcomp->separation = 0;			
   flowcomp->error      = 0;

   initGeneric(&flowcomp->lobe1);
			initPrimitive(&flowcomp->plat);
   initGeneric(&flowcomp->lobe2);

   flowcomp->m0           = 0;
			flowcomp->m1           = 0;
   flowcomp->maxGrad      = GRAD_MAX*glim;
   strcpy(flowcomp->name, "flowcomp_readout_");
   flowcomp->numPoints    = 0;
   flowcomp->rampTime1    = 0;
   flowcomp->rampTime2    = 0;
   flowcomp->resolution   = GRADIENT_RES;
   flowcomp->rollOut      = ROLL_OUT;
   flowcomp->writeToDisk  = TRUE;

			/**********************************************************/
			/* modify initialization for fixed ramp time (trampfixed) */
			/* to mitigate the COupley amp gradient problem           */
			forceRampTime(&flowcomp->rampTime1);
			forceRampTime(&flowcomp->rampTime2);
 		adjustCalcFlag(&flowcomp->calcFlag);
}

/***********************************************************************
*  Function Name: displayError
*  Example:    displayError(error, file, function, lineNumber);
*  Purpose:    Display the error message and the line number where the
*              error was generated.
*  Input
*     Formal:  error     - error flag
*              *file     - string of file name
*              *function - string of function name
*              lineNum   - line number of where the error occurred
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayError (ERROR_NUM_T error, char *file, char *function,
                   long lineNum)
{
   sglerror = (error != ERR_NO_ERROR); /* always set sglerror flag */
   
   if ((error) && (sglabort))  /* display information if sglabort > 0 */
   {
      printf("\n!!!!!!!!!!!!!!!!! E R R O R !!!!!!!!!!!!!!!!!!!!!\n");
      switch (error) 
      {
	 case ERR_AMPLITUDE:
            printf("Gradient amplitude exceeds maximum allowed value.\n");
            break;
	 case ERR_GRAD_FORM_LIST:
            printf("Maximum number of waveform names exceeded.\n");
            break;
	 case ERR_SHAPE:
            printf("Invalid shape.\n");
            break;
	 case ERR_PARAMETER_REQUEST:
            printf("Input parameter(s) not supplied or invalid.\n");
            break;
	 case ERR_CALC_MAX:
            printf("Calculated gradient exceeds maximum gradient");
            printf(" strength.\n");
            break;
	 case ERR_DURATION:
            printf("Duration invalid.\n");
            break;
	 case ERR_CALC_INVALID:
            printf("Invalid result of gradient calculation.\n");
            break;
	 case ERR_GRADIENT_OVERDETERMINED:
            printf("Over-determined gradient.\n");
            break;
	 case ERR_SPECTRAL_WIDTH_OVERDETERMINED:
            printf("Spectral width overdetermined.\n");
            break;
	 case ERR_SPECTRAL_WIDTH_MAX:
            printf("Spectral width exceeds maximum spectral width of");
            printf(" system.\n");
            break;
	 case ERR_RF_PULSE:
            printf("RF-pulse not found.\n");
            break;
	 case ERR_GRANULARITY:
            printf("Granularity violation - Timing does not adhere to");
            printf(" granularity.\n");
            break;
	 case ERR_SLEW_RATE:
            printf("Slew rate violation or invalid slew rate.\n");
            break;
	 case ERR_START_VALUE:
            printf("Illegal gradient start value.\n");
            break;
	 case ERR_FILE_OPEN:
            printf("File open failed.\n");
            break;
	 case ERR_GRADIENT_PADDING:
            printf("Invalid duration for gradient padding.\n");
            break;
	 case ERR_RESOLUTION:
            printf("Resolution is invalid.\n");
            break;
	 case ERR_PADDING_LOCATION:
            printf("Illegal location for gradient padding.\n");
            break;
	 case ERR_STEPS:
            printf("Steps invalid.\n");
            break;
	 case ERR_TRIANGULAR_NEEDED:
            printf("Triangular gradient required.\n");
            break;
	 case ERR_PLATEAU_MISSING:
            printf("Waveform does not have plateau region.\n");
            break;
	 case ERR_RF_FRACTION:
            printf("Invalid RF-fraction.\n");
            break;
	 case ERR_RF_HEADER_ENTRIES:
            printf("Not all RF-pulse header entries could be located.\n");
            break;
	 case ERR_RF_SHAPE_MISSING:
            printf("RF-pulse shape does not exist.\n");
            break;
	 case ERR_HEADER_MISSING:
            printf("Not all shaped gradient header entries could be");
            printf(" located.\n");
            break;
	    case ERR_FILE_DELETE:
            printf("Deletion of shaped gradient file failed.\n");
            break;
	 case ERR_UNDEFINED_ACTION:
            printf("Gradient administration function called with");
            printf(" undefined action\n");
            break;
	 case ERR_FILE_1:
            printf("Input file 1 doesn't exist.\n");
            break; 
	 case ERR_FILE_2:
            printf("Input file 2 doesn't exist.\n");
            break; 
	 case ERR_EMPTY_SS_AMP:
            printf("Slice select amplitude must be supplied.\n");
            break; 
	 case ERR_MERGE_POINTS:
            printf("Points in header and file do not agree.\n");
            break;
	 case ERR_MERGE_SCALE:
            printf("Scaling of waveform exceeds maximum DAC value.\n");
            break;
	 case ERR_CALC_FLAG:
            printf("Invalid calc flag.\n");
            break;
	 case ERR_MALLOC:
            printf("Memory allocation error.\n");
            break;
	 case ERR_NO_SOLUTION:
            printf("Solution cannot be found from input parameters.\n");
            break;
	 case ERR_INCONSISTENT_PARAMETERS:
            printf("Input parameters are inconsistent.\n");
            break;
	 case ERR_NUM_POINTS:
            printf("Number of points not valid.\n");
            break;
	 case ERR_MAX_GRAD:
            printf("Maximum Gradient magnitude is invalid.\n");
            break;
	 case ERR_LIBRARY_ERROR:
            printf("A software error has occurred in the library.\n");
            break;
	 case ERR_BANDWIDTH:
            printf("Bandwidth invalid.\n");
            break;
	 case ERR_FOV:
            printf("Field of view invalid.\n");
            break;
	 case ERR_INCREMENT:
            printf("Increment invalid.\n");
            break;
	 case ERR_RF_BANDWIDTH:
            printf("RF Bandwidth invalid.\n");
            break;
	 case ERR_ECHO_FRACTION:
            printf("Invalid Echo-fraction.\n");
            break;
	 case ERR_RAMP_SHAPE:
            printf("Unsupported ramp shape.\n");
            break;
	 case ERR_RAMP_TIME:
            printf("Invalid ramp time.\n");
            break;
	 case ERR_RF_DURATION:
            printf("RF Duration not valid.\n");
            break;
	 case ERR_CR_AMPLITUDE:
            printf("Crusher amplitude exceeds maximum allowed value.\n");
            break;
	 case ERR_CRUSHER_DURATION:
            printf("Invalid crusher duration.\n");
            break;
	 case ERR_ZERO_PAD_DURATION:
            printf("The new duration supplied is less than the waveform duration.\n");
            break;
	 case ERR_REMOVE_FILE:
            printf("Unable to remove file.\n");
            break;
	 case ERR_GRANULARITY_TRIANGLE:
            printf("Granularity violation of TRIANGULAR shape\n");
	    printf("Duration not an even multiple of granularity.\n");
            break;
	 case ERR_FLIP_ANGLE:
            printf("Invalid flip angle - Flip angle smaller than 0.\n");
            break;	 
	 case ERR_RF_CALIBRATION_FILE_MISSING:
            printf("RF calibration file not found.\n");      
	    break;
	 case ERR_RF_COIL:
            printf("No entry for RF coil in RF calibration file.\n");            	 
	    break;
	 case ERR_RF_CAL_CASE:
            printf("Unknown calibraion case - case not implemented.\n");            	 
	    break;
	 case ERR_RFPOWER_COARSE:
            printf("RF power level - coarse - exceeds maximum value.\n");            	 
	    break;
	 case ERR_RFPOWER_FINE:
            printf("RF power level - fine - exceeds maximum value.\n");            	 
	    break;
	 case ERR_BUTTEFLY_CRUSHER_1:
            printf("RF amplifier unblank duration larger \n");
	    printf("then butterfly crusher 1 duration.\n");            	 
	    break;
	 case ERR_BUTTEFLY_CRUSHER_2:
            printf("RF amplifier blank duration larger \n");
	    printf("then butterfly crusher 1 duration.\n");            	 
	    break;
	 case ERR_POLARITY:
            printf("No negative Polarity allowed if polarity\n");
	    printf("flag is set to FALSE.\n");            	 
	    break;
	case ERR_PHASE_ENCODE_OFFSET:
	    printf("More then one phase encode offset parameter supplied.\n");
	    break;
	case ERR_PARAMETER_NOT_USED_REFOCUS:  
	    printf("Refocus gradient structure contains a populated \n");
	    printf("parameter / structure element that is not used.\n");
	    break;  
	case ERR_PARAMETER_NOT_USED_DEPHASE:  
	    printf("Dephase gradient structure contains a populated \n");
	    printf("parameter / structure element that is not used.\n");
	    break;  
	case ERR_PARAMETER_NOT_USED_PHASE:  
	    printf("Phase encode gradient structure contains a populated \n");
	    printf("parameter /structure element that is not used.\n");
	    break;  
	case ERR_PARAMETER_NOT_USED_GENERIC:  
	    printf("Generic gradient structure contains a populated \n");
	    printf("parameter /structure element that is not used.\n");
	    break;  
	case 	ERR_RF_FLIP:  
	    printf("Invalid RF flip angle. Flip smaller or equal zero.\n");
	    break;
	case ERR_GRAD_PULSES:
		 printf("All gradient waveforms are undefined.\n");
		 break;
	case ERR_GRAD_DURATION:
		 printf("Gradient durations not equal.\n");
		 break;
	case ERR_DUTY_CYCLE:
		 printf("Gradient duty cycle exceeded.\n");
		 break;
 case ERR_VENC:
	  printf("Parameter VENC invalid. \n");
			break;	    
 case ERR_CR_DURATION:
	  printf("Butterfly crusher duration too short.\n");
			break;	    			
 case	ERR_FLOWCOMP_CALC:
   printf("No solution found for flowcomp gradients.\n");
			break;	    				
 case	ERR_FLOWCOMP_GARDIENT_SEPARATION:
   printf("Flowcomp gradient Separation error.\n");
			break;	    				

			
	 default:
            printf("P A N I C - Unknown error occured.\n");
            break;
      }  
      printf("\n");
      printf("ERROR No.: %i\n", (int)error);
      printf("File:      %s\n", file);
      printf("Function:  %s\n", function);  
      printf("Line:      %li\n", lineNum);
      printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
      if (sglabort == SGL_ABORT) {
        abort_message("ERROR: See Text Output for more information\n");   
      }
   }  
}

/***********************************************************************
*  Function Name: getGenericShape
*  Example:    getGenericShape(shape, &shapeName);
*  Purpose:    Translates a generic shape name to a string.
*  Input
*     Formal:  shape - enumerated type
*              *name - string containing name
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void getGenericShape (GRADIENT_SHAPE_T shape, char *name)
{
   switch (shape)
   {
   case TRAPEZOID:
      strcpy(name, "Trapezoid");
      break;
   case SINE:
      strcpy(name, "Sine");
      break;
   case RAMP:
      strcpy(name, "Ramp");
      break;
   case PLATEAU:
      strcpy(name, "Plateau");
      break;
   case GAUSS:
      strcpy(name, "Gauss");
      break;
   case TRIANGULAR:
      strcpy(name, "Triangular");
      break;
   default:
      strcpy(name, "UNKNOWN");
   }
}

/***********************************************************************
*  Function Name: getFunction
*  Example:    getFunction(function, &functionName);
*  Purpose:    Translates a function name to a string.
*  Input
*     Formal:  function - enumerated type
*              *name    - string containing name
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void getFunction (PRIMITIVE_FUNCTION_T function, char *name)
{
   switch (function)
   {
   case PRIMITIVE_LINEAR:
      strcpy(name, "Linear");
      break;
   case PRIMITIVE_SINE:
      strcpy(name, "Sine");
      break;
   case PRIMITIVE_GAUSS:
      strcpy(name, "Gauss");
      break;
   default:
      strcpy(name, "UNKNOWN");
   }
}

/***********************************************************************
*  Function Name: getTrueFalseString
*  Example:    getTrueFalseString(value, &tfString);
*  Purpose:    Translates an integer 0 or 1 to a string. 
*  Input
*     Formal:  value     - 0 or 1 integer
*              *tfString - pointer to string
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void getTrueFalseString (int value, char *tfString)
{
   if (value)
   {
      strcpy(tfString, "True");
   }
   else
   {
      strcpy(tfString, "False");
   }
}

/***********************************************************************
*  Function Name: getCalcFlagString
*  Example:    getCalcFlagString(calcFlag, &cfString);
*  Purpose:    Translates a calc flag value into a string. 
*  Input
*     Formal:  calcFlag  - calculation flag
*              *cfString - pointer to string
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void getCalcFlagString (CALC_FLAG_T calcFlag, char *cfString)
{
   switch (calcFlag)
   {
   case MOMENT_FROM_DURATION_AMPLITUDE: 
      strcpy(cfString, "Moment from duration &\n\t\t\tamplitude");
      break;
   case AMPLITUDE_FROM_MOMENT_DURATION: 
      strcpy(cfString, "Amplitude from moment &\n\t\t\tduration");
      break;
   case DURATION_FROM_MOMENT_AMPLITUDE: 
      strcpy(cfString, "Duration from moment &\n\t\t\tamplitude");
      break;
   case SHORTEST_DURATION_FROM_MOMENT: 
      strcpy(cfString, "Shortest duration &\n\t\t\tamplitude from moment");
      break;
   case MOMENT_FROM_DURATION_AMPLITUDE_RAMP: 
      strcpy(cfString, "Moment from duration &\n\t\t\tamplitude & ramp time");
      break;
   case AMPLITUDE_FROM_MOMENT_DURATION_RAMP: 
      strcpy(cfString, "Amplitude from moment,\n\t\t\tduration & ramp time");
      break;
   case DURATION_FROM_MOMENT_AMPLITUDE_RAMP: 
      strcpy(cfString, "Duration from moment,\n\t\t\tamplitude & ramp time");
      break;
   case SHORTEST_DURATION_FROM_MOMENT_RAMP: 
      strcpy(cfString, "Shortest duration &\n\t\t\tamplitude from moment &\n\t\t\tramp time");
      break;
   default:
      strcpy(cfString, "UNKNOWN");
      break;
   }
}

/***********************************************************************
*  Function Name: getPadLocation
*  Example:    getPadLocation(location, &name);
*  Purpose:    Translates a pad location to a string. 
*  Input
*     Formal:  location - location of padding 
*              *name    - string containing name
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void getPadLocation (PAD_LOCATION_T location, char *name)
{
   switch (location)
   {
   case BACK:
      strcpy(name, "Back");
      break;
   case FRONT:
      strcpy(name, "Front");
      break;
   case BOTH:
      strcpy(name, "Both");
      break;
   default:
      strcpy(name, "UNKNOWN");
   }
}

/***********************************************************************
*  Function Name: doubleToScientificString
*  Example:    doubleToScientificString(value, &string);
*  Purpose:    Change a double value into a string.  The number is
*              represented in scientific notation.
*  Input
*     Formal:  value - value to be converted to a string
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *string - pointer to string
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void doubleToScientificString (double value, char *string)
{
   double a;
   char sign[10];

   a = fabs(value);

   if(FP_LT(value, 0))
   {
      strcpy(sign, "-");
   }
   else
   {
      strcpy(sign, "");
   }

   /* see if its zero */   
   if (fabs(a) < 1e-12)
   {
     	a=0.0;
						sprintf(string, "%s%1.8f", sign, a);
   }

   /* pico */
   else if (a < 1e-11 && a >= 1e-12)
   {
      sprintf(string, "%s%1.8f", sign, a * 1e12);
      strcat(string, "e-12");
   }
   else if (a < 1e-10 && a >= 1e-11)
   {
      sprintf(string, "%s%2.7f", sign, a * 1e12);
      strcat(string, "e-12");
   }
   else if (a < 1e-9 && a >= 1e-10)
   {
      sprintf(string, "%s%3.6f", sign, a * 1e12);
      strcat(string, "e-12");
   }

   /* nano */
   else if (a < 1e-8 && a >= 1e-9)
   {
      sprintf(string, "%s%1.8f", sign, a * 1e9);
      strcat(string, "e-09");
   }
   /* floating point comparisons with macros can now be done, see value of */
   /* EPSILON in sgl.h */
   else if (FP_LT(a, 1e-7) && FP_GTE(a, 1e-8))
   {
      sprintf(string, "%s%2.7f", sign, a * 1e9);
      strcat(string, "e-09");
   }
   else if (FP_LT(a, 1e-6) && FP_GTE(a, 1e-7))
   {
      sprintf(string, "%s%3.6f", sign, a * 1e9);
      strcat(string, "e-09");
   }

   /* micro */
   else if (FP_LT(a, 1e-5) && FP_GTE(a, 1e-6))
   {
      sprintf(string, "%s%1.8f", sign, a * 1e6);
      strcat(string, "e-06");
   }
   else if (FP_LT(a, 1e-4) && FP_GTE(a, 1e-5))
   {
      sprintf(string, "%s%2.7f", sign, a * 1e6);
      strcat(string, "e-06");
   }
   else if (FP_LT(a, 1e-3) && FP_GTE(a, 1e-4))
   {
      sprintf(string, "%s%3.6f", sign, a * 1e6);
      strcat(string, "e-06");
   }

   /* milli */
   else if (FP_LT(a, 1e-2) && FP_GTE(a, 1e-3))
   {
      sprintf(string, "%s%1.8f", sign, a * 1e3);
      strcat(string, "e-03");
   }
   else if (FP_LT(a, 1e-1) && FP_GTE(a, 1e-2))
   {
      sprintf(string, "%s%2.7f", sign, a * 1e3);
      strcat(string, "e-03");
   }
   else if (FP_LT(a, 1) && FP_GTE(a, 1e-1))
   {
      sprintf(string, "%s%3.6f", sign, a * 1e3);
      strcat(string, "e-03");
   }

   /* ones */
   else if (FP_LT(a, 1e+1) && FP_GTE(a, 1))
   {
      sprintf(string, "%s%1.8f", sign, a);
      strcat(string, "e+00");
   }
   else if (FP_LT(a, 1e+2) && FP_GTE(a, 1e+1))
   {
      sprintf(string, "%s%2.7f", sign, a);
      strcat(string, "e+00");
   }
   else if (FP_LT(a, 1e+3) && FP_GTE(a, 1e+2))
   {
      sprintf(string, "%s%3.6f", sign, a);
      strcat(string, "e+00");
   }

   /* Kilo */
   else if (FP_LT(a, 1e+4) && FP_GTE(a, 1e+3))
   {
      sprintf(string, "%s%1.8f", sign, a / 1e+3);
      strcat(string, "e+03");
   }
   else if (FP_LT(a, 1e+5) && FP_GTE(a, 1e+4))
   {
      sprintf(string, "%s%2.7f", sign, a / 1e+3);
      strcat(string, "e+03");
   }
   else if (FP_LT(a, 1e+6) && FP_GTE(a, 1e+5))
   {
      sprintf(string, "%s%3.6f", sign, a / 1e+3);
      strcat(string, "e+03");
   }

   /* Mega */
   else if (FP_LT(a, 1e+7) && FP_GTE(a, 1e+6))
   {
      sprintf(string, "%s%1.8f", sign, a / 1e+6);
      strcat(string, "e+06");
   }
   else if (FP_LT(a, 1e+8) && FP_GTE(a, 1e+7))
   {
      sprintf(string, "%s%2.7f", sign, a / 1e+6);
      strcat(string, "e+06");
   }
   else if (FP_LT(a, 1e+9) && FP_GTE(a, 1e+8))
   {
      sprintf(string, "%s%3.6f", sign, a / 1e+6);
      strcat(string, "e+06");
   }

   /* Giga */
   else if (FP_LT(a, 1e+10) && FP_GTE(a, 1e+9))
   {
      sprintf(string, "%s%1.8f", sign, a / 1e+9);
      strcat(string, "e+09");
   }
   else if (FP_LT(a, 1e+11) && FP_GTE(a, 1e+10))
   {
      sprintf(string, "%s%2.7f", sign, a / 1e+9);
      strcat(string, "e+09");
   }
   else if (FP_LT(a, 1e+12) && FP_GTE(a, 1e+11))
   {
      sprintf(string, "%s%3.6f", sign, a / 1e+9);
      strcat(string, "e+09");
   }

   /* Tera */
   else if (FP_LT(a, 1e+13) && FP_GTE(a, 1e+12))
   {
      sprintf(string, "%s%1.8f", sign, a / 1e+12);
      strcat(string, "e+12");
   }
   else if (FP_LT(a, 1e+14) && FP_GTE(a, 1e+13))
   {
      sprintf(string, "%s%2.7f", sign, a / 1e+12);
      strcat(string, "e+12");
   }
   else if (FP_LT(a, 1e+15) && FP_GTE(a, 1e+14))
   {
      sprintf(string, "%s%3.6f", sign, a / 1e+12);
      strcat(string, "e+12");
   }

   /* LARGER Tera */
   else if ((FP_GTE(a, 1e+15)) || ( a <= 1e-12) )
   {
      sprintf(string, "ERROR: Input number out of range - FUNCTION INTENDED FOR INTERNAL SGL USE ONLY !\n");
      warn_message("WARNING: Function <doubleToScientificString> detected a value GTE 1e+15 --> out of range!");
   }

   /* check if value is not-a-number (NaN) */
   else if (isnan(a))
   {
      sprintf(string, "NaN     ");
   }
   else
   {
      sprintf(string, " ");
   }
}

/***********************************************************************
*  Function Name: displaySlice
*  Example:    displaySlice(&sliceSelect);
*  Purpose:    Displays the values of a SLICE_SELECT_GRADIENT_T
*              structure.
*  Input
*     Formal:  *slice - pointer to SLICE_SELECT_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displaySlice (SLICE_SELECT_GRADIENT_T *slice)
{
   char string[MAX_STR];

   if (slice->enableButterfly)
   {
      printf("\n------------ BUTTERFLY SLICE SELECT ----------------\n");
   }
   else
   {
      printf("\n----------------- SLICE SELECT ---------------------\n");
   }

   printf("Amplitude:\t\t%1.8f\t[G/cm]\n", slice->amp);
   if (slice->enableButterfly)
   {
     printf("Slice Select Amplitude:\t%1.8f\t[G/cm]\n", slice->ssamp);
   }   
   doubleToScientificString(slice->duration, string);
   printf("Duration:\t\t%s\t[s]\n", string);

   getTrueFalseString(slice->enableButterfly, string);
   printf("Enable Butterfly:\t%s\n", string);

   printf("Error:\t\t\t%i\n", slice->error);
   printf("Max. Overall Amp.:\t%1.8f\t[G/cm]\n", slice->amp);
   printf("Maximum Amplitude:\t%1.8f\t[G/cm]\n", slice->maxGrad);

   doubleToScientificString(slice->m0, string);
   printf("Moment 0th:\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(slice->m0ref, string);
   printf("Moment 0th To Refocus:\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(slice->m0def, string);
   printf("Moment 0th To Dephase:\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(slice->m1, string);
   printf("Moment 1st:\t\t%s\t[G/cm * s^2]\n", string);

   doubleToScientificString(slice->m1ref, string);
   printf("Moment 1st To Refocus:\t%s\t[G/cm * s^2]\n", string);

   printf("Name:\t\t\t%s\n", slice->name);
   printf("Number of Points:\t%li\n", slice->numPoints);

   doubleToScientificString(slice->pad2, string);
   printf("Pad Duration Back:\t%s\t[s]\n", string);

   doubleToScientificString(slice->pad1, string);
   printf("Pad Duration Front:\t%s\t[s]\n", string);

   doubleToScientificString(slice->plateauDuration, string);
   printf("Plateau duration:\t%s\t[s]\n", string);

   if (slice->enableButterfly != TRUE)
   {
      doubleToScientificString(slice->tramp, string);
      printf("Ramp Time:\t\t%s\t[s]\n", string);

      getFunction(slice->rampShape, string);
      printf("Ramp Shape:\t\t%s\n", string);
   }

   doubleToScientificString(slice->resolution, string);
   printf("Resolution:\t\t%s\t[s]\n", string);

   doubleToScientificString(slice->rfBandwidth, string);
   printf("RF Bandwidth:\t\t%s\t[Hz]\n", string);

   doubleToScientificString(slice->rfDelayFront, string);
   printf("RF Delay Front:\t\t%s\t[s]\n", string);

   doubleToScientificString(slice->rfDelayBack, string);
   printf("RF Delay Back:\t\t%s\t[s]\n", string);

   doubleToScientificString(slice->rfDuration, string);
   printf("RF Duration:\t\t%s\t[s]\n", string);

   printf("RF fraction:\t\t%f\n", slice->rfFraction);

   doubleToScientificString(slice->rfCenterBack, string);
   printf("time from RF center to gradient end:\t\t%s\t[s]\n", string);

   doubleToScientificString(slice->rfCenterFront, string);
   printf("time from gradient start to RF center:\t\t%s\t[s]\n", string);

   printf("RF Pulse Name:\t\t%s\n", slice->rfName);

   getTrueFalseString(slice->rollOut, string);
   printf("RollOut:\t\t%s\n", string);

   doubleToScientificString(slice->slewRate, string);
   printf("Slew Rate:\t\t%s\t[[G/cm] / s]\n", string);
			
   printf("Slice Thickness:\t%f\t[mm]\n", slice->thickness);

   getTrueFalseString(slice->writeToDisk, string);
   printf("Write to Disk:\t\t%s\n", string);

   if (slice->enableButterfly == TRUE)
   {
      /* first crusher */
      printf("\n---- Crusher 1 ----\n");
      printf("   Amplitude:\t\t%1.8f\t[G/cm]\n", slice->cr1amp);
   
      getCalcFlagString(slice->crusher1CalcFlag, string);
      printf("   Calculation Flag:\t%s\n", string);
   
      doubleToScientificString(slice->crusher1Duration, string);
      printf("   Total Duration:\t%s\t[s]\n", string);

      doubleToScientificString(slice->crusher1Moment0, string);
      printf("   Moment 0:\t\t%s\t[G/cm * s]\n", string);

      doubleToScientificString(slice->crusher1RampToCrusherDuration, string);
      printf("   Ramp To Cr Duration:\t%s\t[s]\n", string);

      doubleToScientificString(slice->crusher1RampToSsDuration, string);
      printf("   Ramp To SS Duration:\t%s\t[s]\n", string);
 
      /* slice select */
      printf("\n---- Slice Select ----\n");
      printf("   Amplitude:\t\t%1.8f\t[G/cm]\n", slice->ssamp);

      doubleToScientificString(slice->ssDuration, string);
      printf("   Duration:\t\t%s\t[s]\n", string);

      /* second crusher */
      printf("\n---- Crusher 2 ----\n");
      printf("   Amplitude:\t\t%1.8f\t[G/cm]\n", slice->cr2amp);
   
      getCalcFlagString(slice->crusher2CalcFlag, string);
      printf("   Calculation Flag:\t%s\n", string);
   
      doubleToScientificString(slice->crusher2Duration, string);
      printf("   Total Duration:\t%s\t[s]\n", string);

      doubleToScientificString(slice->crusher2Moment0, string);
      printf("   Moment 0:\t\t%s\t[G/cm * s]\n", string);

      doubleToScientificString(slice->crusher2RampToCrusherDuration, string);
      printf("   Ramp To Cr Duration:\t%s\t[s]\n", string);

      doubleToScientificString(slice->crusher2RampToSsDuration, string);
      printf("   Ramp To Zero Dur:\t%s\t[s]\n", string);
   }

   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayRefocusContents
*  Example:    displayRefocusContents(&refocus);
*  Purpose:    Displays the values of a REFOCUS_GRADIENT_T structure.
*  Input
*     Formal:  *refocus - pointer to REFOCUS_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayRefocusContents (REFOCUS_GRADIENT_T *refocus)
{
   char string[MAX_STR];

   printf("Amplitude:\t\t%f\t[G/cm]\n", refocus->amp);

   doubleToScientificString(refocus->balancingMoment0, string);
   printf("Balancing 0th Moment:\t%s\t[G/cm * s]\n", string);

   getCalcFlagString(refocus->calcFlag, string);
   printf("Calculation Flag:\t%s\n", string);

   printf("Error:\t\t\t%i\n", refocus->error);

   doubleToScientificString(refocus->duration, string);
   printf("Duration:\t\t%s\t[s]\n", string);

   printf("Maximum Amplitude:\t%1.8f\t[G/cm]\n", refocus->maxGrad);

   doubleToScientificString(refocus->m0, string);
   printf("Moment 0th:\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(refocus->m1, string);
   printf("Moment 1st:\t\t%s\t[G/cm * s^2]\n", string);

   printf("Name:\t\t\t%s\n", refocus->name);
   printf("Number of Points:\t%li\n", refocus->numPoints);

   getFunction(refocus->rampShape, string);
   printf("Ramp Shape:\t\t%s\n", string);

   doubleToScientificString(refocus->tramp, string);
   printf("Ramp Time:\t\t%s\t[s]\n", string);

   doubleToScientificString(refocus->resolution, string);
   printf("Resolution:\t\t%s\t[s]\n", string);

   getTrueFalseString(refocus->rollOut, string);
   printf("RollOut:\t\t%s\n", string);

   getGenericShape(refocus->shape, string);
   printf("Shape:\t\t\t%s\n", string);

   doubleToScientificString(refocus->slewRate, string);
   printf("Slew Rate:\t\t%s\t[[G/cm] / s]\n", string);

   getTrueFalseString(refocus->writeToDisk, string);
   printf("Write to Disk:\t\t%s\n", string);

   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayRefocus
*  Example:    displayRefocus(&refocus);
*  Purpose:    Displays the values of a REFOCUS_GRADIENT_T structure.
*  Input
*     Formal:  *refocus - pointer to REFOCUS_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayRefocus (REFOCUS_GRADIENT_T *refocus)
{

   printf("\n------------------ REFOCUS -------------------------\n");
   displayRefocusContents(refocus);
}

/***********************************************************************
*  Function Name: displayDephase
*  Example:    displayDephase(&dephase);
*  Purpose:    Displays the values of a REFOCUS_GRADIENT_T structure.
*  Input
*     Formal:  *dephase - pointer to REFOCUS_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayDephase (REFOCUS_GRADIENT_T *dephase)
{

   printf("\n------------------ DEPHASE -------------------------\n");
   displayRefocusContents(dephase);
}

/***********************************************************************
*  Function Name: displayReadout
*  Example:    displayReadout(&readout);
*  Purpose:    Displays the values of a READOUT_GRADIENT_T structure.
*  Input
*     Formal:  *readout - pointer to READOUT_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayReadout (READOUT_GRADIENT_T *readout)
{
   char string[MAX_STR];

   if (readout->enableButterfly)
   {
      printf("\n--------------- BUTTERFLY READOUT ------------------\n");
   }
   else
   {
      printf("\n------------------ READOUT -------------------------\n");
   }

   doubleToScientificString(readout->acqTime, string);
   printf("Acquisition Time:\t\t%s\t[s]\n", string);

   doubleToScientificString(readout->fracAcqTime, string);
   printf("Acquisition Time (fraction):\t%s\t[s]\n", string);

   printf("Amplitude:\t\t\t%1.8f\t[G/cm]\n", readout->amp);
   if (readout->enableButterfly)
   {
     printf("Readout Amplitude:\t\t%1.8f\t[G/cm]\n", readout->roamp);
   }
   doubleToScientificString(readout->bandwidth, string);
   printf("Bandwidth:\t\t\t%s\t[Hz]\n", string);

   doubleToScientificString(readout->atDelayFront, string);
   printf("Delay Before Acq.:\t\t%s\t[s]\n", string);

   doubleToScientificString(readout->atDelayBack, string);
   printf("Delay After Acq.:\t\t%s\t[s]\n", string);

   doubleToScientificString(readout->duration, string);
   printf("Duration:\t\t\t%s\t[s]\n", string);

   printf("Echo Fraction:\t\t\t%f\t[s]\n", readout->echoFraction);

   getTrueFalseString(readout->enableButterfly, string);
   printf("Enable Butterfly:\t\t%s\n", string);

   printf("Error:\t\t\t\t%i\n", readout->error);
   printf("Field of View (FOV):\t\t%f\t[mm]\n", readout->fov);

   printf("Maximum Amplitude:\t\t%1.8f\t[G/cm]\n", readout->maxGrad);
   
   doubleToScientificString(readout->m0, string);
   printf("Moment 0th:\t\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(readout->m0ref, string);
   printf("Moment 0th To Dephase:\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(readout->m0def, string);
   printf("Moment 0th To Rephase:\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(readout->m1, string);
   printf("Moment 1st:\t\t\t%s\t[G/cm * s^2]\n", string);

   doubleToScientificString(readout->m1ref, string);
   printf("Moment 1st To Dephase:\t\t%s\t[G/cm * s^2]\n", string);

   printf("Name:\t\t\t\t%s\n", readout->name);
   printf("Number of Points:\t\t%li\n", readout->numPoints);

   if (readout->enableButterfly == FALSE)
   {
      doubleToScientificString(readout->plateauDuration, string);
      printf("Plateau Duration:\t\t%s\t[s]\n", string);
   }
   
   printf("Points In Frequency:\t\t%li\t\t[Pts]\n", readout->numPointsFreq);

   doubleToScientificString(readout->pad2, string);
   printf("Pad Duration Back:\t\t%s\t[s]\n", string);

   doubleToScientificString(readout->pad1, string);
   printf("Pad Duration Front:\t\t%s\t[s]\n", string);

   if (readout->enableButterfly != TRUE)
   {
      getFunction(readout->rampShape, string);
      printf("Ramp Shape:\t\t\t%s\n", string);

      doubleToScientificString(readout->tramp, string);
      printf("Ramp Time:\t\t\t%s\t[s]\n", string);
   }

   doubleToScientificString(readout->resolution, string);
   printf("Resolution:\t\t\t%s\t[s]\n", string);

   doubleToScientificString(readout->slewRate, string);
   printf("Slew Rate:\t\t%s\t[[G/cm] / s]\n", string);

   doubleToScientificString(readout->timeToEcho, string);
   printf("Time to echo:\t\t\t%s\t[s]\n", string);

   doubleToScientificString(readout->timeFromEcho, string);
   printf("Time from echo:\t\t\t%s\t[s]\n", string);

   getTrueFalseString(readout->rollOut, string);
   printf("RollOut:\t\t\t%s\n", string);

   getTrueFalseString(readout->writeToDisk, string);
   printf("Write to Disk:\t\t\t%s\n", string);

   if (readout->enableButterfly == TRUE)
   {
      /* first crusher */
      printf("\n---- Crusher 1 ----\n");
      printf("   Amplitude:\t\t\t%1.8f\t[G/cm]\n", readout->cr1amp);
   
      getCalcFlagString(readout->crusher1CalcFlag, string);
      printf("   Calculation Flag:\t\t%s\n", string);
   
      doubleToScientificString(readout->crusher1Duration, string);
      printf("   Total Duration:\t\t%s\t[s]\n", string);

      doubleToScientificString(readout->crusher1Moment0, string);
      printf("   Moment 0:\t\t\t%s\t[G/cm * s]\n", string);

      doubleToScientificString(readout->crusher1RampToCrusherDuration, string);
      printf("   Ramp To Cr Duration:\t\t%s\t[s]\n", string);

      doubleToScientificString(readout->crusher1RampToSsDuration, string);
      printf("   Ramp To SS Duration:\t\t%s\t[s]\n", string);

      /* readout plateau */
      printf("\n---- Plateau ----\n");
      printf("   Amplitude:\t\t\t%1.8f\t[G/cm]\n", readout->roamp);

      doubleToScientificString(readout->plateauDuration, string);
      printf("   Duration:\t\t\t%s\t[s]\n", string);

      /* second crusher */
      printf("\n---- Crusher 2 ----\n");
      printf("   Amplitude:\t\t\t%1.8f\t[G/cm]\n", readout->cr2amp);
   
      getCalcFlagString(readout->crusher2CalcFlag, string);
      printf("   Calculation Flag:\t\t%s\n", string);
   
      doubleToScientificString(readout->crusher2Duration, string);
      printf("   Total Duration:\t\t%s\t[s]\n", string);

      doubleToScientificString(readout->crusher2Moment0, string);
      printf("   Moment 0:\t\t\t%s\t[G/cm * s]\n", string);

      doubleToScientificString(readout->crusher2RampToCrusherDuration, string);
      printf("   Ramp To Cr Duration:\t\t%s\t[s]\n", string);

      doubleToScientificString(readout->crusher2RampToSsDuration, string);
      printf("   Ramp To Zero Dur:\t\t%s\t[s]\n", string);
   }

   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayPhase
*  Example:    displayPhase(&phaseEncodeGradient);
*  Purpose:    Displays the values of a PHASE_ENCODE_GRADIENT_T
*              structure.
*  Input
*     Formal:  *phase - pointer to PHASE_ENCODE_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayPhase (PHASE_ENCODE_GRADIENT_T *phase)
{
   char string[MAX_STR];

   printf("\n-------------------- PHASE -------------------------\n");
   printf("Max Amplitude:\t\t%f\t[G/cm]\n", phase->amp);
   printf("Max Grad:\t\t%f\t[G/cm]\n", phase->maxGrad);
   printf("PE amplitude:\t\t%f\t[G/cm]\n", phase->peamp);
   printf("Offset Amplitude:\t%f\t[G/cm]\n", phase->offsetamp);

   printf("User Amp Offset:\t%f\t[G/cm]\n", phase->amplitudeOffset);

   doubleToScientificString(phase->areaOffset, string);
   printf("User Area Offset:\t%s\t[G/cm * s]\n", string);

   getCalcFlagString(phase->calcFlag, string);
   printf("Calculation Flag:\t%s\n", string);

   doubleToScientificString(phase->duration, string);
   printf("Duration:\t\t%s\t[s]\n", string);

   printf("Error:\t\t\t%i\n", phase->error);
   printf("Field of View (FOV):\t%f\t[mm]\n", phase->fov);
   printf("Increment:\t\t%f\t[G/cm]\n", phase->increment);

   doubleToScientificString(phase->m0, string);
   printf("Moment 0th:\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(phase->m1, string);
   printf("Moment 1st:\t\t%s\t[G/cm * s^2]\n", string);

   printf("Name:\t\t\t%s\n", phase->name);
   printf("Number of Points:\t%li\n", phase->numPoints);

   getFunction(phase->rampShape, string);
   printf("Ramp Shape:\t\t%s\n", string);

   doubleToScientificString(phase->tramp, string);
   printf("Ramp Time:\t\t%s\t[s]\n", string);

   doubleToScientificString(phase->resolution, string);
   printf("Resolution:\t\t%s\t[s]\n", string);

   getTrueFalseString(phase->rollOut, string);
   printf("RollOut:\t\t%s\n", string);

   getGenericShape(phase->shape, string);
   printf("Shape:\t\t\t%s\n", string);

   doubleToScientificString(phase->slewRate, string);
   printf("Slew Rate:\t\t%s\t[[G/cm] / s]\n", string);

   printf("Steps:\t\t\t%li\n", phase->steps);

   getTrueFalseString(phase->writeToDisk, string);
   printf("Write to Disk:\t\t%s\n", string);

   printf("----------------------------------------------------\n");
}


/***********************************************************************
*  Function Name: displayZeroFillGradient
*  Example:    displayZeroFillGradient(&zg);
*  Purpose:    Display zero fill gradient structure.
*  Input
*     Formal:  none
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  zg - zero filling gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayZeroFillGradient (ZERO_FILL_GRADIENT_T *zg)
{
   char string[MAX_STR];

   printf("\n--------------- ZERO FILL GRADIENT -----------------\n");

   printf("Error:\t\t\t%i\n", zg->error);

   getPadLocation(zg->location, string);
   printf("Pad Location:\t\t%s\n", string);
   printf("Name:\t\t\t%s\n", zg->name);

   doubleToScientificString(zg->newDuration, string);
   printf("New Duration:\t\t%s\t[s]\n", string);

   printf("Number of Points:\t%li\n", zg->numPoints);

   doubleToScientificString(zg->resolution, string);
   printf("Resolution:\t\t%s\t[s]\n", string);

   getTrueFalseString(zg->rollOut, string);
   printf("RollOut:\t\t%s\n", string);

   doubleToScientificString(zg->timeToPad, string);
   printf("Time to Pad:\t\t%s\t[s]\n", string);

   getTrueFalseString(zg->writeToDisk, string);
   printf("Write to Disk:\t\t%s\n", string);

   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayShape
*  Example:    displayShape(&shape);
*  Purpose:    Displays the values of a PRIMITIVE_SHAPE_T structure.
*  Input
*     Formal:  *shape - pointer to PRIMITIVE_SHAPE_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayShape (PRIMITIVE_SHAPE_T *shape)
{
   char name[MAX_STR];

   getFunction(shape->function, name);

   printf("\n--------------------- SHAPE ------------------------\n");
   printf("Function:\t\t%s\n", name);
   if (shape->function == PRIMITIVE_SINE)
   {
      printf("Start X:\t\t%1.8f\t[rad]\n", shape->startX);
      printf("End X:\t\t\t%1.8f\t[rad]\n", shape->endX);
   }
   else if (shape->function == PRIMITIVE_LINEAR)
   {
      printf("Start X:\t\t%1.8f\t[IGNORED]\n", shape->startX);
      printf("End X:\t\t\t%1.8f\t[IGNORED]\n", shape->endX);
   }
   else
   {
      printf("Start X:\t\t%1.8f\n", shape->startX);
      printf("End X:\t\t\t%1.8f\n", shape->endX);
   }
   printf("Power factor:\t\t%1.1f\t\n", shape->powerFactor);
   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayPrimitive
*  Example:    displayPrimitive(&primGrad);
*  Purpose:    Wrapper for displayPrimitiveContents.
*  Input
*     Formal:  *prim - pointer to PRIMITIVE_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayPrimitive (PRIMITIVE_GRADIENT_T *prim)
{
   printf("\n------------------- PRIMITIVE ----------------------\n");
   displayPrimitiveContents(prim);
   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayGenericPrimitive
*  Example:    displayGenericPrimitive(&primGrad, RAMP_UP);
*  Purpose:    Wrapper for displayPrimitiveContents().
*  Input
*     Formal:  *prim - pointer to PRIMITIVE_GRADIENT_T structure
*              i     - determines which primitive of the generic to
*                      display
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayGenericPrimitive (PRIMITIVE_GRADIENT_T *prim,
                              GRADIENT_PRIMITIVE_T i)
{
   switch (i)
   {
   case GENERIC_RAMP_UP:
      printf("\n-------------------- RAMP UP -----------------------\n");
      break;
   case GENERIC_PLATEAU:
      printf("\n-------------------- PLATEAU -----------------------\n");
      break;
   case GENERIC_RAMP_DOWN:
      printf("\n------------------- RAMP DOWN ----------------------\n");
      break;
   }
   displayPrimitiveContents(prim);
   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayPrimitiveContents
*  Example:    displayPrimitiveContents(&primGrad);
*  Purpose:    Displays the values of a PRIMITIVE_GRADIENT_T structure.
*  Input
*     Formal:  *prim - pointer to PRIMITIVE_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayPrimitiveContents (PRIMITIVE_GRADIENT_T *prim)
{
   char string[MAX_STR];

   doubleToScientificString(prim->duration, string);
   printf("Duration:\t\t%s\t[s]\n", string);

   printf("End Amplitude:\t\t%1.8f\t[G/cm]\n", prim->endAmplitude);
   printf("Error:\t\t\t%i\n", prim->error);

   printf("Maximum Amplitude:\t%1.8f\t[G/cm]\n", prim->maxGrad);

   doubleToScientificString(prim->m0, string);
   printf("Moment 0th:\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(prim->m1, string);
   printf("Moment 1st:\t\t%s\t[G/cm * s^2]\n", string);

   printf("Name:\t\t\t%s\n", prim->name);
   printf("Number of Points:\t%li\n", prim->numPoints);
   
   doubleToScientificString(prim->resolution, string);
   printf("Resolution:\t\t%s\t[s]\n", string);

   getTrueFalseString(prim->rollOut, string);
   printf("RollOut:\t\t%s\n", string);

   getFunction(prim->shape.function, string);
   printf("Shape:\t\t\t%s\n", string);

   doubleToScientificString(prim->slewRate, string);
   printf("Slew Rate:\t\t%s\t[[G/cm] / s]\n", string);

   printf("Start Amplitude:\t%1.8f\t[G/cm]\n", prim->startAmplitude);

   getTrueFalseString(prim->writeToDisk, string);
   printf("Write to Disk:\t\t%s\n", string);

   displayShape(&prim->shape);
}

/***********************************************************************
*  Function Name: displayGeneric
*  Example:    displayGeneric(&genericGrad);
*  Purpose:    Wrapper for displayGenericContents().
*  Input
*     Formal:  *generic - pointer to GENERIC_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayGeneric (GENERIC_GRADIENT_T *generic)
{
   printf("\n------------------- GENERIC ------------------------\n");
   displayGenericContents(generic);
   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayGenericContents
*  Example:    displayGenericContents(&genericGrad);
*  Purpose:    Displays the values of a GENERIC_GRADIENT_T structure.
*  Input
*     Formal:  *generic - pointer to GENERIC_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayGenericContents (GENERIC_GRADIENT_T *generic)
{
   char string[MAX_STR];

   printf("Amplitude:\t\t%1.8f\t[G/cm]\n", generic->amp);

   getCalcFlagString(generic->calcFlag, string);
   printf("Calculation Flag:\t%s\n", string);

   doubleToScientificString(generic->duration, string);
   printf("Duration:\t\t%s\t[s]\n", string);

   printf("Error:\t\t\t%i\n", generic->error);

   doubleToScientificString(generic->m0, string);
   printf("Moment 0:\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(generic->m1, string);
   printf("Moment 1:\t\t%s\t[G/cm * s^2]\n", string);

   printf("Name:\t\t\t%s\n", generic->name);

   getTrueFalseString(generic->noPlateau, string);
   printf("No Plateau:\t\t%s\n", string);

   printf("Number of Points:\t%li\n", generic->numPoints);

   getTrueFalseString(generic->polarity, string);
   printf("Polarity Flag:\t\t%s\n", string);

   doubleToScientificString(generic->tramp, string);
   printf("Ramp Time:\t\t%s\t[[G/cm] / s]\n", string);

   doubleToScientificString(generic->resolution, string);
   printf("Resolution:\t\t%s\t[s]\n", string);

   getTrueFalseString(generic->rollOut, string);
   printf("RollOut:\t\t%s\n", string);

   getGenericShape(generic->shape, string);
   printf("Shape:\t\t\t%s\n", string);

   doubleToScientificString(generic->slewRate, string);
   printf("Slew Rate:\t\t%s\t[[G/cm] / s]\n", string);

   printf("Start Amplitude:\t%1.8f\t[G/cm]\n", generic->startAmplitude);

   getTrueFalseString(generic->writeToDisk, string);
   printf("Write to Disk:\t\t%s\n", string);

   displayGenericPrimitive(&generic->ramp1, GENERIC_RAMP_UP);
   displayGenericPrimitive(&generic->plateau, GENERIC_PLATEAU);
   displayGenericPrimitive(&generic->ramp2, GENERIC_RAMP_DOWN);

   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayBandwidth
*  Example:    displayBandwidth(&bandwidth);
*  Purpose:    Displays the values of a BANDWIDTH_T structure.
*  Input
*     Formal:  *bw - pointer to BANDWIDTH_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayBandwidth(BANDWIDTH_T *bw)
{
   char string[MAX_STR];

   printf("\n\n------------------- BANDWIDTH ----------------------\n");
   doubleToScientificString(bw->acqTime, string);
   printf("Acquisition time (AT):\t%s\t[s]\n", string);

   printf("Points:\t\t\t%li\t\t[Pts]\n", bw->points);

   doubleToScientificString(bw->bw, string);
   printf("Bandwidth:\t\t%s\t[HZ]\n", string);

   printf("Readout fraction:\t%f\t[s]\n", bw->readFraction);

   doubleToScientificString(bw->resolution, string);
   printf("Resolution:\t\t%s\t[s]\n", string);

   printf("Error:\t\t\t%i\n", bw->error);
   printf("----------------------------------------------------\n\n");
}

/***********************************************************************
*  Function Name: displayRfHeader
*  Example:    displayRfHeader(&my_header);
*  Purpose:    Displays the values of a RF_HEADER_T structure.
*  Input
*     Formal:  *header - pointer to RF_HEADER_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayRfHeader (RF_HEADER_T *header)
{
   printf("\n------------------- RF HEADER ----------------------\n");
   printf("Version:\t\t%f\n", header->version);
   printf("Type:\t\t\t%s\n", header->type);
   printf("Modulation:\t\t%s\n", header->modulation);
   printf("Time-BW Producy:\t%f\n", header->bandwidth);
   printf("Inversion TBW Product:\t%f\n", header->inversionBw);
   printf("Integral:\t\t%f\n", header->integral);
   printf("Refocus fraction:\t%f\n", header->rfFraction);
   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayRf
*  Example:    displayRf(&pulse);
*  Purpose:    Displays the values of a RF_PULSE_T structure.
*  Input
*     Formal:  *pulse - pointer to RF_PULSE_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayRf (RF_PULSE_T *pulse)
{
   char string[MAX_STR];

   printf("\n---------------------- RF --------------------------\n");
   printf("Modulation:\t\t%s\n", pulse->header.modulation);

   doubleToScientificString(pulse->rfDuration, string);
   printf("Duration:\t\t%s\t[s]\n", string);
   printf("RF pulse bandwidth:\t%f\n",pulse->bandwidth);
   printf("Flip angle:\t\t%f\t[%c]\n", pulse->flip,176);
   printf("Flip multiplier:\t\t%f\n", pulse->flipmult);
   printf("Time-BW Product:\t%f\n", pulse->header.bandwidth);
   printf("Inversion TBW Product:\t%f\n", pulse->header.inversionBw);
   printf("Integral:\t\t%f\n", pulse->header.integral);
   printf("Refocus fraction:\t%f\n", pulse->header.rfFraction); 
   printf("Power - Coarse:\t\t%f\t[dB]\n",pulse->powerCoarse);
   printf("Power - Fine:\t\t%f\n",pulse->powerFine);
   doubleToScientificString(pulse->rof1, string);
   printf("Blank time:\t\t%s\n",string);
   doubleToScientificString(pulse->rof2, string);   
   printf("Unblank time:\t\t%s\n",string);   
   printf("SAR:\t\t\t%f\t[W/kg]\n", pulse->sar);
   printf("Error:\t\t\t%i\n", pulse->error);
   printf("RF-pulse name:\t\t%s\n", pulse->pulseName);
   printf("Type:\t\t\t%s\n", pulse->header.type);   
   printf("Version:\t\t%f\n", pulse->header.version);   
   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayGradientHeader
*  Example:    displayGradientHeader(&gradHeader);
*  Purpose:    Displays the values of a GRADIENT_HEADER_T structure.
*  Input
*     Formal:  *gradientHeader - pointer to GRADIENT_HEADER_T structure
*     Private: none 
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayGradientHeader (GRADIENT_HEADER_T *gradientHeader)
{
   printf("\n----------------- GRADIENT HEADER ------------------\n");
   printf("Name:\t\t\t%s\n", gradientHeader->name);
   printf("Points:\t\t\t%li\t[Pts]\n", gradientHeader->points);
   printf("Resolution:\t\t%f\t[s]\n", gradientHeader->resolution);
   printf("Strength:\t\t%f\t[G/cm]\n", gradientHeader->strength);
   printf("Error:\t\t\t%i\n", gradientHeader->error);
   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayButterfly
*  Example:    displayButterfly(&butterfly);
*  Purpose:    Displays the values of a BUTTERFLY_GRADIENT_T structure.
*  Input
*     Formal:  *bfly - pointer to BUTTERFLY_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayButterfly (BUTTERFLY_GRADIENT_T *bfly)
{
   char string[MAX_STR];

   printf("\n------------------ BUTTERFLY -----------------------\n");
   printf("Amplitude:\t\t%1.8f\t[G/cm]\n", bfly->amp);

   printf("Error:\t\t\t%i\n", bfly->error);
   printf("Maximum Amplitude:\t%1.8f\t[G/cm]\n", bfly->maxGrad);

   doubleToScientificString(bfly->m0, string);
   printf("Moment 0:\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(bfly->m1, string);
   printf("Moment 1:\t\t%s\t[G/cm * s^2]\n", string);

   printf("Name:\t\t\t%s\n", bfly->name);
   printf("Number of Points:\t%li\n", bfly->numPoints);

   doubleToScientificString(bfly->resolution, string);
   printf("Resolution:\t\t%s\t[s]\n", string);

   getTrueFalseString(bfly->rollOut, string);
   printf("RollOut:\t\t%s\n", string);

   doubleToScientificString(bfly->slewRate, string);
   printf("Slew Rate:\t\t%s\t[[G/cm] / s]\n", string);

   getTrueFalseString(bfly->writeToDisk, string);
   printf("Write to Disk:\t\t%s\n", string);

   /* first crusher */
   printf("---- Crusher 1 ----\n");
   printf("   Amplitude:\t\t%1.8f\t[G/cm]\n", bfly->cr1amp);

   getCalcFlagString(bfly->cr1CalcFlag, string);
   printf("   Calculation Flag:\t%s\n", string);

   doubleToScientificString(bfly->cr1Moment0, string);
   printf("   Moment 0:\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(bfly->cr1TotalDuration, string);
   printf("   Total Duration:\t%s\t[s]\n", string);

   /* slice select */
   printf("---- Slice Select ----\n");
   printf("   Amplitude:\t\t%1.8f\t[G/cm]\n", bfly->ssamp);

   getCalcFlagString(bfly->ssCalcFlag, string);
   printf("   Calculation Flag:\t%s\n", string);

   doubleToScientificString(bfly->ssMoment0, string);
   printf("   Moment 0:\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(bfly->ssTotalDuration, string);
   printf("   Total Duration:\t%s\t[s]\n", string);

   /* second crusher */
   printf("---- Crusher 2 ----\n");
   printf("   Amplitude:\t\t%1.8f\t[G/cm]\n", bfly->cr2amp);

   getCalcFlagString(bfly->cr2CalcFlag, string);
   printf("   Calculation Flag:\t%s\n", string);

   doubleToScientificString(bfly->cr2Moment0, string);
   printf("   Moment 0:\t\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(bfly->cr2TotalDuration, string);
   printf("   Total Duration:\t%s\t[s]\n", string);

   printf("\n-------------------- RAMP 1 ------------------------\n");
   displayPrimitiveContents(&bfly->ramp1);
   printf("\n------------------ CRUSHER 1 -----------------------\n");
   displayPrimitiveContents(&bfly->cr1);
   printf("\n-------------------- RAMP 2 ------------------------\n");
   displayPrimitiveContents(&bfly->ramp2);
   printf("\n---------------- SLICE SELECT ----------------------\n");
   displayPrimitiveContents(&bfly->ss);
   printf("\n-------------------- RAMP 3 ------------------------\n");
   displayPrimitiveContents(&bfly->ramp3);
   printf("\n------------------ CRUSHER 2 -----------------------\n");
   displayPrimitiveContents(&bfly->cr2);
   printf("\n-------------------- RAMP 4 ------------------------\n");
   displayPrimitiveContents(&bfly->ramp4);

   printf("----------------------------------------------------\n");
   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displayFlowcompContents
*  Example:    displayFlowcompContents(&flowcomp);
*  Purpose:    Displays the values of the flow comp structure
*              (FLOWCOMP_T).
*  Input
*     Formal:  *flowcomp - pointer to FLOWCOMP_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayFlowcompContents (FLOWCOMP_T *flowcomp)
{
   char string[MAX_STR];
   printf("Amplitude:\t\t%f\t[G/cm]\n", flowcomp->amp);

   printf("---- LOBE 1 ----\n");
   printf("Amplitude:\t\t%f\t[G/cm]\n", flowcomp->amplitude1);

   doubleToScientificString(flowcomp->duration1, string);
   printf("Duration:\t\t%s\t[s]\n", string);

   doubleToScientificString(flowcomp->rampTime1, string);
   printf("Ramp Time:\t\t%s\t[s]\n", string);

   getGenericShape(flowcomp->lobe1.shape, string);
   printf("Shape:\t\t\t%s\n\n", string);

   printf("---- LOBE SEPARATION ----\n");
   doubleToScientificString(flowcomp->separation, string);
   printf("Duration:\t\t%s\t[s]\n", string);

   printf("---- LOBE 2 ----\n");
   printf("Amplitude:\t\t%f\t[G/cm]\n", flowcomp->amplitude2);

   doubleToScientificString(flowcomp->duration2, string);
   printf("Duration:\t\t%s\t[s]\n", string);

   doubleToScientificString(flowcomp->rampTime2, string);
   printf("Ramp Time:\t\t%s\t[s]\n", string);

   getGenericShape(flowcomp->lobe2.shape, string);
   printf("Shape:\t\t\t%s\n\n", string);

   printf("---- OTHER PARAMETERS ----\n");
   doubleToScientificString(flowcomp->duration, string);
   printf("Combined duration:\t%s\n", string);

   doubleToScientificString(flowcomp->m0, string);
   printf("Combined Moment 0:\t%s\t[G/cm * s]\n", string);

   doubleToScientificString(flowcomp->m1, string);
   printf("Combined Moment 1:\t%s\t[G/cm * s^2]\n", string);

   doubleToScientificString(flowcomp->duration, string);
   printf("Duration:\t\t%s\t[s]\n", string);

   printf("Error:\t\t\t%i\n", flowcomp->error);
   printf("Max. Gradient:\t\t%f\t[G/cm]\n", flowcomp->maxGrad*glim);
   printf("Name:\t\t\t%s\n", flowcomp->name);
   printf("Number of Points:\t%li\n", flowcomp->numPoints);

   doubleToScientificString(flowcomp->resolution, string);
   printf("Resolution:\t\t%s\t[s]\n", string);

   getTrueFalseString(flowcomp->rollOut, string);
   printf("RollOut:\t\t%s\n", string);

   getTrueFalseString(flowcomp->writeToDisk, string);
   printf("Write to Disk:\t\t%s\n", string);

   printf("\n------------------- LOBE 1 -------------------------\n");
   displayGenericContents(&flowcomp->lobe1);
   printf("\n---------------LOBE SEPARATION ---------------------\n");
   displayPrimitiveContents(&flowcomp->plat);	
   printf("\n------------------- LOBE 2 -------------------------\n");
   displayGenericContents(&flowcomp->lobe2);
   
   printf("----------------------------------------------------\n");
}

/***********************************************************************
*  Function Name: displaySliceFlowcomp
*  Example:    displaySliceFlowcomp(&sliceFlowcomp);
*  Purpose:    Displays the values of the slice flow comp structure
*              (FLOWCOMP_T).
*  Input
*     Formal:  *flowcomp - pointer to FLOWCOMP_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displaySliceFlowcomp (FLOWCOMP_T *flowcomp)
{
   printf("\n------------------ SLICE FLOWCOMP -----------------\n");
   displayFlowcompContents(flowcomp);
}

/***********************************************************************
*  Function Name: displayReadoutFlowcomp
*  Example:    displayReadoutFlowcomp(&readoutFlowcomp);
*  Purpose:    Displays the values of the readout flow comp structure
*              (FLOWCOMP_T).
*  Input
*     Formal:  *flowcomp - pointer to FLOWCOMP_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayReadoutFlowcomp (FLOWCOMP_T *flowcomp)
{
   printf("\n------------------- READOUT FLOWCOMP ------------------\n");
   displayFlowcompContents(flowcomp);
}

/***********************************************************************
*  Function Name: checkForFilename
*  Example:    checkForFilename (*name, *path);
*  Purpose:    Appends a number to the passed in file name and checks
*              for the existence of the newly created name.  If a file
*              with the same name already exists, the appended number
*              is increased by one until no existing file has the same
*              name.
*  Input
*     Formal:  *name   - name of waveform file
*     Private: none
*     Public:  USER_DIR - user directory
*  Output
*     Return:  none
*     Formal:  *name - name of waveform file
*              *path - path to waveform file
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void checkForFilename (char *name, char *path)
{
   char  wavefile[MAX_STR]; /* data file */
   long  counter = 1;
   short flag = 0;
   FILE  *fpout;

   /* check if file exists */
   do
   {
      strcpy(path, USER_DIR);      /* add user directory to path */
      strcat(path, SHAPE_LIB_DIR); /* add shapelib directory to path */
      strcpy(wavefile, name);      /* assign waveform name */
      sprintf(wavefile, "%s%li", wavefile, counter); /* add number */
      strcat(path, wavefile);      /* add path and waveform name */
      strcat(path, ".GRD");        /* add suffix to complete path */

      /* check if file already exists */
      if ((fpout = fopen(path, "r")) == NULL)
      {
         flag = 1; /* if file does not exist leave loop and use name */
      }
      else
      {
         fclose(fpout); /* if file exists continue searching */
         counter++;
      }
   }
   while (!flag);

   /* assign found (free) file name to string */
   strcpy(name, wavefile);
}

/***********************************************************************
*  Function Name: gradAdmin
*  Example:    gradAdmin(*name, action);
*  Purpose:    Maintains a list of all created gradient waveforms.
*              Gradients files and their path are stored in the array
*              <name> contained in the GRAD_FORM_LIST_T structure.
*  Input
*     Formal:  *name  - full path and name of waveform file
*              action - action to be performed
*     Private: none
*     Public:  none
*  Output
*     Return:  int - integer cast of ERROR_NUM_T error
*     Formal:  none
*     Private: gradList - gradient waveform list
*     Public:  none
*  Notes:      none
***********************************************************************/
int gradAdmin (char *name, LIST_ACTION_T action)
{
   long index;
   int  retVal;

   /* Check arguments */
   switch (action)
   {
   case INIT:
      /* initializes list */
      initGradFormList(&gradList);
      break;
   case ADD:
      /* add file to list */
      strcpy(gradList.name[gradList.number], name);
      if(++gradList.number > MAX_GRAD_FORMS)
      {
         displayError(ERR_GRAD_FORM_LIST, __FILE__, __FUNCTION__, __LINE__);
         return ERR_GRAD_FORM_LIST;
      }
      break;
   case REMOVE:
      /* remove an entry from list */

      /* check if name is in list */
      index = 0;
      do
      {
         if(!strcmp(gradList.name[index], name))
         {
            break;
         }
         index++;
      }
      while (index < MAX_GRAD_FORMS);

      /* see if we found an entry that matches */
      if (index == MAX_GRAD_FORMS)
      {
         /* entry not found in list, unable to delete a non-existent entry */
         displayError(ERR_FILE_DELETE, __FILE__, __FUNCTION__, __LINE__);
         return ERR_FILE_DELETE;
      }

      /* delete the file from the disk */    
      retVal = deleteFile(gradList.name[index]);
      if (retVal)
      {
         return (retVal);
      }

      /* move up all other entries */
      while (strcmp(gradList.name[index], ""))
      {
         strcpy(gradList.name[index], gradList.name[index + 1]);
         index++;
      }
      gradList.number = gradList.number - 1;
      break;     
   case REMOVE_ALL:
      /* delete the entire list */
      for (index = 0; index < gradList.number; index++)
      {
         retVal = deleteFile(gradList.name[index]);
         if (retVal)
         {
            return (retVal);
         }
            strcpy(gradList.name[index], "");
         }

      /* reset gradient index */
      gradList.number = 0;
      break;     
   default:
      /* default action */
      displayError(ERR_UNDEFINED_ACTION, __FILE__, __FUNCTION__, __LINE__);
      return ERR_UNDEFINED_ACTION;
   }

   return(0);
}

/***********************************************************************
*  Function Name: gradShapeWritten
*  Example:    gradShapeWritten(basename, params, filename);
*  Purpose:   checks gradient database to determine whether
*                 shape (as characterized by basename and params)
*                 has been created & wriiten to disk
*                 NOTE:
*                     If filename is an empty string, copy disk file name FROM database
*                     If filename is not empty, copy its contents TO the database
*
*  Input
*     Private: none
*     Public:  none
*  Output
*     Return:  int- TRUE if gradient shape has been written 
*                        FALSE if not
*     Formal:  none
*     Private: gradWList - gradient waveform list
*     Public:  none
*  Notes:      none
***********************************************************************/
int gradShapeWritten (char *basename, char *params, char *filename)
{
   char fname[MAX_STR];
   int wFlag = FALSE; 
   GRAD_WRITTEN_LIST_T *tempP;
   GRAD_WRITTEN_LIST_T *lastP;
   GRAD_WRITTEN_LIST_T *tP, *lP;
   long counter;
   int match;

   lastP=NULL;
   tP=NULL;
   lP=NULL;

   /* search for same entry */
   tempP=gradWListP;
			
   while(tempP&&(wFlag==FALSE))
      {
      if(strcmp(tempP->basename, basename) || strcmp(tempP->params, params))
   	     {
      	  lastP=tempP;
     	  tempP=lastP->nextEntry;
        	}
      else  /* found a match */
	        {
      	  (void)strcpy(filename,tempP->filename);
      	  wFlag=TRUE;
     	}
   }
  
   if(!wFlag)  /* waveform not found, so enter it in database */
      {
      tempP=(GRAD_WRITTEN_LIST_T *)malloc(sizeof(GRAD_WRITTEN_LIST_T));
      if(!tempP)
	        {
         displayError(ERR_MALLOC, __FILE__, __FUNCTION__, __LINE__); 
      	  return(wFlag);
        	}
      (void)strcpy(tempP->basename,basename);
      (void)strcpy(tempP->params,params);

      /*assign a unique filename */
      match=TRUE;
      counter=0;
      while(match)
	        {
      	  counter++;
	        strcpy(fname, basename);
      	  sprintf(fname, "%s%li", fname, counter);   /* append number */
      	  tP=gradWListP;
      	  match=FALSE;
         while((tP)&&(tP!=tempP))
	           {
    	       if(!strcmp(fname,tP->filename))
          		   match=TRUE;
    	       lP=tP;
	           tP=lP->nextEntry;
	           }
         }
      
      (void)strcpy(tempP->filename,fname);
      (void)strcpy(filename,fname);
      tempP->nextEntry=NULL;
      if(!gradWListP)
         gradWListP=tempP;
      else
     	   lastP->nextEntry=tempP;
      }


   /* temporary debugging stuff */
   /*
   FILE *f1;
  char path[MAX_STR];
  strcpy(path, USER_DIR);     
  strcat(path, SHAPE_LIB_DIR);
  strcat(path,"gradlist.tmp");
  f1=fopen(path,"w+");
  tempP=gradWListP;
  while(tempP)
    {
      (void)fprintf(f1,"base name: %s\n",tempP->basename);
      (void)fprintf(f1,"parameter string: %s\n",tempP->params);
      (void)fprintf(f1,"file name: %s\n",tempP->filename);
      tempP=tempP->nextEntry;
    }
  fclose(f1);
  */

   return(wFlag);
}


/***********************************************************************
*  Function Name: writeGradientHeader
*  Example:    writeGradientHeader(&header, fileHandle);
*  Purpose:    Writes a gradient header.
*  Input
*     Formal:  *header - Gradient header structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *fpout - File handle
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void writeGradientHeader(GRADIENT_HEADER_T *header, FILE *fpout)
{
   fprintf(fpout, "#\n");
   fprintf(fpout, "# NAME:       %s\n", header->name);
   fprintf(fpout, "# POINTS:     %li\n", header->points);
   fprintf(fpout, "# RESOLUTION: %f\n", header->resolution);
   fprintf(fpout, "# STRENGTH:   %f\n", header->strength);
   fprintf(fpout, "#:\n");
}

/***********************************************************************
*  Function Name: deleteFile
*  Example:    deleteFile(file);
*  Purpose:    Attempts to delete a file from the disk.
*  Input
*     Formal:  *name - full path and name of file to be deleted
*     Private: none
*     Public:  none
*  Output
*     Return:  int - error number
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
int deleteFile (char *name)
{
   FILE *fp;

   /* see if the file is ont he disk */
   if ((fp = fopen(name, "r")) ==  NULL)
   {
      displayError(ERR_REMOVE_FILE, __FILE__, __FUNCTION__, __LINE__);
      return(ERR_REMOVE_FILE);
   }
   else
   {
      /* delete file from disk */
      remove(name);
      fclose(fp);      
   }

   return(0);
}

/***********************************************************************
*  Function Name: removeFiles
*  Example:    removeFiles();
*  Purpose:    Removes the waveform files from gradList.
*  Input
*     Formal:  none
*     Private: none
*     Public:  none
*  Output
*     Return:  int - integer cast of ERROR_NUM_T error
*     Formal:  none
*     Private: gradList - gradient waveform list
*     Public:  none
*  Notes:      none
***********************************************************************/
int removeFiles (void)
{
   char notUsed[1];

   return(gradAdmin(notUsed, REMOVE_ALL));
}

/***********************************************************************
*  Function Name: readGradientHeader
*  Example:    readGradientHeader(&header, fileHandle);
*  Purpose:    Reads gradient header data from a file and places it in
*              a gradient header structure.
*  Input
*     Formal:  *fpin - File handle
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *header - Gradient header structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void readGradientHeader(GRADIENT_HEADER_T *header, FILE *fpin)
{
   char inString[MAX_STR];
   char seekString[GRAD_HEADER_ENTRIES][MAX_STR];
   long counter;

   /* initialize variables */
   counter = 1;       /* loop counter */
   strcpy(seekString[NAME], "NAME:"); /* search strings for header */
   strcpy(seekString[POINTS], "POINTS:");
   strcpy(seekString[RESOLUTION], "RESOLUTION:");
   strcpy(seekString[STRENGTH], "STRENGTH:");

   /* look for the strings defined in seekString */
   counter = 0; /* initialize counter */
   while (!feof(fpin) && (counter < (int)GRAD_HEADER_ENTRIES))
   {
      fscanf(fpin, "%s", inString); /* read string from file */

      /* assign value if string was found */
      /* check if string has been found */
      if (strstr(inString, seekString[counter]))
      {
         fscanf(fpin, "%s", inString); /* read value following string */

         /* assign values to structure */
         switch ((GRADIENT_HEADER_ENTRIES_T)counter)
         {
         case NAME:
            strcpy(header->name, inString);
            break;
         case POINTS:
            header->points = atoi(inString);
             break;
         case RESOLUTION:
            header->resolution = atof(inString);
            break;
         case STRENGTH:
            header->strength = atof(inString);
            break;
         case GRAD_HEADER_ENTRIES:
         default:
            displayError(ERR_RF_HEADER_ENTRIES, __FILE__, __FUNCTION__, __LINE__);
            return;
         }
         counter++;

         /* search for the next string starting at top of the file */
         fseek(fpin, 0, 0);
      }
   }

   /* check if all strings were found */
   if (counter != (int)GRAD_HEADER_ENTRIES)
   {
      /* Not all strings have been found */
      header->error = ERR_HEADER_MISSING;
      displayError(header->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* display structure for debugging */      
   if (sgldisplay == DISPLAY_LOW_LEVEL) displayGradientHeader(header);
}

/***********************************************************************
*  Function Name: readRfPulse
*  Example:    readRfPulse(&mypulse);
*  Purpose:    Reads RF-pulse header data from a file and places it in
*              a structure.
*  Input
*     Formal:  *rfPulse - rf pulse structure
*     Private: none
*     Public:  USER_DIR - user directory
*  Output
*     Return:  none
*     Formal:  *rfPulse - rf pulse structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void readRfPulse (RF_PULSE_T *rfPulse)
{
   char path[MAX_STR], rfFile[MAX_STR]; /* data file */
   char shapename[MAX_STR];
   char inString[MAX_STR];
   int  errorFlag;
   char seekString[RF_HEADER_ENTRIES] [MAX_STR];
   long counter;
   long safetyCheck;
   long searchSteps;
   FILE *fpin;

   /* initialize variables */
   counter     = 1;     /* loop counter */
   safetyCheck = 1;     /* safety counter ot prevent endless loops */
   searchSteps = 500;   /* maximum number of strings to the read */
                        /* before moving on */
   errorFlag   = 0;     /* initialize error flag with 0 - no error */
   strcpy(seekString[VERSION], "VERSION");
   strcpy(seekString[TYPE], "TYPE");
   strcpy(seekString[MODULATION], "MODULATION");
   strcpy(seekString[EXCITEWIDTH], "EXCITEWIDTH");
   strcpy(seekString[INVERTWIDTH], "INVERTWIDTH");
   strcpy(seekString[INTEGRAL], "INTEGRAL"); 
   strcpy(seekString[RF_FRACTION], "RF_FRACTION");
   
   /* check if file exists */
   strcpy(rfFile, rfPulse->pulseName); /* assign waveform name */
   strcpy(shapename, rfFile);   /* add path and waveform name */
   strcat(shapename, ".RF");    /* add suffix to complete path */
   if (appdirFind(shapename,"shapelib",path,"",R_OK))
   {
      if ((fpin = fopen(path, "r")) == NULL)
      {
         /* Terminate if file is missing */
         rfPulse->error = ERR_RF_SHAPE_MISSING;
         displayError(rfPulse->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }
   else
   {
      /* Terminate if file is missing */
      rfPulse->error = ERR_RF_SHAPE_MISSING;
      displayError(rfPulse->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* look for the strings defined in seekString */
   counter = 0; /* initialize counter */
   do
   {
      fscanf(fpin, "%s", inString); /* read string from file */
      /* assign value if string was found */
      /* check if string has been found */
      if (strstr(inString, seekString[counter]))
      {
         fscanf(fpin, "%s", inString); /* read value following string */
         /* assign values to structure -> order is important ! */
         switch ((RF_PULSE_HEADER_ENTRIES_T)counter)
         {
         case VERSION:
            rfPulse->header.version = atof(inString);
            break;
         case TYPE:
            strcpy(rfPulse->header.type, inString);
            break;
         case MODULATION:
            strcpy(rfPulse->header.modulation, inString);
            break;
         case EXCITEWIDTH:
            rfPulse->header.bandwidth = atof(inString);
            break;
         case INVERTWIDTH:
            rfPulse->header.inversionBw = atof(inString);
            break;
         case INTEGRAL:
            rfPulse->header.integral = atof(inString);
            break;
         case RF_FRACTION:
            rfPulse->header.rfFraction = atof(inString);
            break;   
         case RF_HEADER_ENTRIES:
         default:
            rfPulse->error = ERR_RF_HEADER_ENTRIES;
            displayError(ERR_RF_HEADER_ENTRIES, __FILE__, __FUNCTION__, __LINE__);
            return;
         }
         counter++;
         safetyCheck = 0; /* set safetyCheck */

         /* search for next string starting at the top of the file */
         fseek(fpin, 0, 0);
      }

      /* after X strings look for next string */
      if (safetyCheck > searchSteps)
      {
         safetyCheck = 0;  /* reset safetyCheck */

         /* Do not produce an error if the  RF fraction or VERSION */
         /* is missing in the header - not all RF pulse   */
         /* contain this entry. The default for the RF    */
         /* fraction if not found is 0.5 as define in the */
         /* RF initialization (initRfHeader).             */
         if ((counter != (int)RF_FRACTION) && (counter != (int)VERSION))
         {
            errorFlag = 1; /* set error flag - header entry not found */
         }
         counter++;

         /* search for next string starting at the top of the file */
         fseek(fpin, 0, 0);
      }
      safetyCheck++;
   }
   /* stop when all strings have been found */
   while ((int)RF_HEADER_ENTRIES > counter);

   /* check if all string were found */
   if ((safetyCheck >= searchSteps) || (errorFlag)) 
   {
      /* Not all strings have been found */
      rfPulse->error = ERR_RF_HEADER_ENTRIES;
      displayError(rfPulse->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   strcpy(rfPulse->pulseName, rfFile);
   fclose(fpin);
   
   /* display structure for debugging */      
   if (rfPulse->display == DISPLAY_LOW_LEVEL) displayRf(rfPulse);
}

/***********************************************************************
*  Function Name: calcSlice
*  Example:    calcSlice(&slice);
*  Purpose:    Calculates the required values for the slice select
*              gradient, creates the waveform file, updates the waveform
*              administration list and propagates the out structure with
*              the calculated values.  The calcFlag determines how the
*              calculations are executed.  Butterfly crushers are
*              supported.
*  Input
*     Formal:  *slice - slice select structure
*     Private: none
*     Public:  GRAD_MAX - Maximum possible gradient strength
*  Output
*     Return:  none
*     Formal:  *slice - slice select structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calcSlice (SLICE_SELECT_GRADIENT_T *slice, RF_PULSE_T *rfPulse)
{
   GENERIC_GRADIENT_T gg;
   BUTTERFLY_GRADIENT_T bfly;
   long startIndex = 0;
   double delta = 0;
   double pulseDuration = 0.0;
   double tramp1 = 0.0;
   double tramp2 = 0.0;       
   
   initGeneric(&gg);
   initButterfly(&bfly);

	/* make sure that calc flag is consistent with trampfixed */
	adjustCalcFlag( &(slice->crusher1CalcFlag) );
	adjustCalcFlag( &(slice->crusher2CalcFlag) );

   /* assign rfDuration */
   slice->rfDuration = rfPulse->rfDuration;

   /* check input values */
   if (FP_LTE(slice->maxGrad, 0) ||
       FP_GT(slice->maxGrad, GRAD_MAX))
   {
      /* maxGrad must be positive */
      slice->error = ERR_MAX_GRAD;
      displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LTE(slice->resolution, 0))
   {
      /* resolution invalid */
      slice->error = ERR_RESOLUTION;
      displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(slice->duration, 0)) 
   {
      /* duration cannot be negative */
      slice->error = ERR_DURATION;
      displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(slice->tramp, 0)) 
   {
      /* ramp time cannot be negative */
      slice->error = ERR_RAMP_TIME;
      displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (slice->enableButterfly == TRUE) 
   {
      if ((slice->crusher1CalcFlag == MOMENT_FROM_DURATION_AMPLITUDE &&
           FP_LTE(slice->crusher1Duration, 0)) ||
          (slice->crusher2CalcFlag == MOMENT_FROM_DURATION_AMPLITUDE &&
           FP_LTE(slice->crusher2Duration, 0)))
      {
         /* duration cannot be negative */
         slice->error = ERR_CRUSHER_DURATION;
         displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }

   if (FP_LT(slice->pad1, 0) ||
       FP_LT(slice->pad2, 0))
   {
      /* pad duration cannot be negative */
      slice->error = ERR_GRADIENT_PADDING;
      displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(slice->rfFraction, 0) ||
       FP_GT(slice->rfFraction, 1))
   {
      /* rfFraction out of range */
      slice->error = ERR_RF_FRACTION;
      displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   if (slice->rampShape == GAUSS)
   {
      slice->error = ERR_RAMP_SHAPE;
      displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* Check RF pulse parameter */
//   if( FP_LT(rfPulse->flip, 0.0))
//   {
//      /* Flip angle cannot be negative */
//      slice->error = ERR_DURATION;
//      displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
//      return;
//   }
   if (FP_LT(rfPulse->rfDuration, 0))
   {
      /* rfDuration out of range */
      slice->error = ERR_RF_DURATION;
      displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }   

   /* copy the rf pulse file name */
   strcpy(slice->rfName,rfPulse->pulseName);

   /* suppress display for high and mid level function */
   if ((sgldisplay == DISPLAY_TOP_LEVEL) || (sgldisplay == DISPLAY_MID_LEVEL))
      {
      rfPulse->display = DISPLAY_NONE;
      }      

   /* Read the rf file */
   readRfPulse(rfPulse);

   /* check for error */  
   if (rfPulse->error != ERR_NO_ERROR)
   {
      slice->error = rfPulse->error;
      return;
   }
 
   if (FP_GT(rfPulse->bandwidth, 0.0))
   {
      /* use provided bandwidth */
      slice->rfBandwidth = rfPulse->bandwidth;
   }  
   else
      {
      /* calculate slice select bandwidth */
      if ( (FP_LT(rfPulse->flip, FLIPLIMIT_LOW)) ||
           (FP_GT(rfPulse->flip, FLIPLIMIT_HIGH)) )
      {
	 /* use excitation TBW product */
	 rfPulse->bandwidth = rfPulse->header.bandwidth / rfPulse->rfDuration;
      }
      else 	
      {
	 /* use inversion TBW product */
	 rfPulse->bandwidth = rfPulse->header.inversionBw / rfPulse->rfDuration;
      }
      slice->rfBandwidth = rfPulse->bandwidth;
   }

   /* assign rf refocus fraction */
   
   slice->rfFraction = rfPulse->header.rfFraction;
   
   /* calculate amplitude */
   slice->ssamp = slice->rfBandwidth /
                      (slice->thickness * MM_TO_CM * slice->gamma); /* in [G/cm] */

   /* check gradient amplitude */
   if (FP_GT(slice->ssamp, slice->maxGrad))
   {
      slice->error = ERR_AMPLITUDE;
      displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* granulate padding */
   slice->pad2  = granularity(slice->pad2, slice->resolution);
   slice->pad1 = granularity(slice->pad1, slice->resolution);

   /* calculate ramp time if not supplied */
   if (FP_EQ(slice->tramp, 0))
   {
      if (slice->rampShape == PRIMITIVE_LINEAR)
      {
         slice->tramp = granularity(slice->ssamp / slice->slewRate, slice->resolution);
      }
      else if (slice->rampShape == PRIMITIVE_SINE)
      {
         slice->tramp = granularity(slice->ssamp * PI / (2.0 * slice->slewRate),
                                       slice->resolution);
      }
   }

   if ( FP_LT((slice->tramp + slice->pad1),rfPulse->rof1) ||
        FP_LT((slice->tramp + slice->pad2),rfPulse->rof2) )
   {
      /* determine ramp time so that pad + ramp grater equal rof */
      tramp1 = granularity( (rfPulse->rof1 - slice->pad1), slice->resolution);
      tramp2 = granularity( (rfPulse->rof2 - slice->pad2), slice->resolution);
      /* pick largest ramp time required */
      slice->tramp = MAX(tramp1, tramp2);    
   } 


   /* granulate rfDuration */
   pulseDuration = granularity(rfPulse->rfDuration, slice->resolution);
   
   if (slice->enableButterfly == TRUE)
   {
      if (strcmp(slice->name, "") == 0)
      {
         /* string is empty, supply a default name */
         strcpy(slice->name, "slice_butterfly_");
      }

      /* ensure that ramp times are granulated correctly */
   			if (FP_NEQ(slice->crusher1RampToCrusherDuration,
			              granularity(slice->crusher1RampToCrusherDuration, slice->resolution)))
      {
         /* ramp duration must conform to resolution of system */
         slice->error = ERR_GRANULARITY;
         displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }

      /* populate butterfly structure */
      /* first crusher */
      bfly.cr1amp           = slice->cr1amp;
      bfly.cr1CalcFlag      = slice->crusher1CalcFlag;
      bfly.cr1TotalDuration = slice->crusher1Duration;
      bfly.cr1Moment0       = slice->crusher1Moment0;
      bfly.ramp1.duration   = slice->crusher1RampToCrusherDuration;
      bfly.ramp2.duration   = slice->crusher1RampToSsDuration;  

      /* second crusher */
      bfly.cr2amp           = slice->cr2amp;
      bfly.cr2CalcFlag      = slice->crusher2CalcFlag;
      bfly.cr2TotalDuration = slice->crusher2Duration;
      bfly.cr2Moment0       = slice->crusher2Moment0;
      bfly.ramp3.duration   = slice->crusher2RampToSsDuration;  
      bfly.ramp4.duration   = slice->crusher2RampToCrusherDuration;


      bfly.ssCalcFlag       = MOMENT_FROM_DURATION_AMPLITUDE;
						
      /* slice select */
      bfly.ss.startAmplitude = slice->ssamp;

      bfly.resolution       = slice->resolution;
      bfly.ramp1.resolution = slice->resolution;
      bfly.cr1.resolution   = slice->resolution;
      bfly.ramp2.resolution = slice->resolution;
      bfly.ss.resolution    = slice->resolution;
      bfly.ramp3.resolution = slice->resolution;
      bfly.cr2.resolution   = slice->resolution;
      bfly.ramp4.resolution = slice->resolution;
      bfly.ssamp            = slice->ssamp;
      bfly.ssTotalDuration  = pulseDuration + slice->pad1 +
                              slice->pad2;
      bfly.writeToDisk      = slice->writeToDisk;
      bfly.rollOut          = slice->rollOut;
      strcpy(bfly.name, slice->name);
       
      bfly.amp              = MAX( fabs(slice->cr1amp), fabs(slice->cr2amp) );
      bfly.amp              = MAX(bfly.amp, fabs(slice->ssamp) );
   
      /* make the waveform */
      calcButterfly(&bfly);

      /* check crusher duration against rof1 and rof2 */
      if (FP_GT(RF_UNBLANK,bfly.cr1TotalDuration))
      {
         slice->error =  ERR_BUTTEFLY_CRUSHER_1;
         displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);	
         return;      
      }
      if (FP_GT(RF_BLANK,bfly.cr2TotalDuration))
      {
         slice->error =  ERR_BUTTEFLY_CRUSHER_2;
         displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);	 
         return;      
      }
	
      /* check for errors */
      if (bfly.error != ERR_NO_ERROR)
      {
         slice->error = bfly.error;
         displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   
      /* assign data to slice structure */
      /* first crusher */
      slice->cr1amp                        = bfly.cr1amp;
      slice->crusher1Duration              = bfly.cr1TotalDuration;
      slice->crusher1Moment0               = bfly.cr1Moment0;
      slice->crusher1RampToCrusherDuration = bfly.ramp1.duration;
      slice->crusher1RampToSsDuration      = bfly.ramp2.duration;

      /* slice select */
      slice->ssDuration                    = bfly.ssTotalDuration;

      /* second crusher */
      slice->cr2amp                        = bfly.cr2amp;
      slice->crusher2Duration              = bfly.cr2TotalDuration;
      slice->crusher2Moment0               = bfly.cr2Moment0;
      slice->crusher2RampToCrusherDuration = bfly.ramp3.duration;
      slice->crusher2RampToSsDuration      = bfly.ramp4.duration;

      slice->ssamp      = bfly.ssamp;
      slice->dataPoints = bfly.dataPoints;
      slice->duration   = bfly.cr1TotalDuration + bfly.ssTotalDuration + bfly.cr2TotalDuration;
      slice->error      = bfly.error;
      slice->m0         = bfly.m0;
      slice->m1         = bfly.m1;
      strcpy(slice->name, bfly.name);
      slice->numPoints  = bfly.numPoints;
      slice->slewRate   = bfly.slewRate;
      /* find data point index for center of rf pulse */
      startIndex = bfly.ramp1.numPoints + bfly.cr1.numPoints + bfly.ramp2.numPoints +
                   (long)ROUND(slice->pad1 / slice->resolution) +
                   (long)ceil(pulseDuration * (1.0 - slice->rfFraction) /
                              slice->resolution) + 1;

      /* if rfFractions places the start index between system ticks, we'll need */
      /* a delta to account for the added time */
      delta = (ceil(pulseDuration * (1.0 - slice->rfFraction) / slice->resolution) -
                   (pulseDuration * (1.0 - slice->rfFraction) / slice->resolution)) *
               slice->resolution;

      if (FP_NEQ(delta, 0))
      {
         startIndex--;
      }

      /* calculate moments */
      calculateMoments(bfly.dataPoints, bfly.numPoints,
                       1,  /* start index */
                       0,  /* start amplitude */
                       bfly.resolution, &bfly.m0,
                       &bfly.m1,
                       0,  /* start delta offset */
                       0); /* end delta offset */
      calculateMoments(bfly.dataPoints, bfly.numPoints,
                       startIndex,       /* start index */
                       bfly.ssamp,       /* start amplitude */
                       bfly.resolution, &slice->m0ref,
                       &slice->m1ref,
                       delta, /* start delta offset */
                       0);    /* end delta offset */

      /* copy moment data */
      slice->m0      = bfly.m0;
      slice->m1      = bfly.m1;
      slice->plateauDuration = pulseDuration + slice->pad1 +
                               slice->pad2;
      slice->rfDelayFront = (pulseDuration - slice->rfDuration)/2.0+
                             slice->pad1 + slice->crusher1Duration-
			     rfPulse->rof1;
      slice->rfDelayBack = (pulseDuration- slice->rfDuration)/2.0+
                            slice->pad2+slice->crusher2Duration-
			    rfPulse->rof2;		       

      /* RF refocus time - time between center of RF and end of gradient */
      slice->rfCenterBack = rfPulse->rfDuration * slice->rfFraction +
                             slice->pad2 + slice->crusher2Duration;
      slice->rfCenterFront = slice->duration - slice->rfCenterBack;
			    
    }
   else if (slice->enableButterfly == FALSE)
   {
      if (strcmp(slice->name, "") == 0)
      {
         /* string is empty, supply a default name */
         strcpy(slice->name, "slice_");
      }

      /* populate generic structure */
      gg.calcFlag   = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
      gg.resolution = slice->resolution;
      gg.amp        = slice->ssamp;
      gg.duration   = slice->tramp * 2 + pulseDuration +
                      slice->pad1 + slice->pad2;
      strcpy(gg.name, slice->name);
      gg.plateau.duration     = pulseDuration + slice->pad1 +
                                slice->pad2;
      gg.tramp                = slice->tramp;
      gg.ramp1.duration       = slice->tramp;
      gg.ramp2.duration       = slice->tramp;
      gg.ramp1.shape.function = slice->rampShape;
      gg.ramp2.shape.function = slice->rampShape;
      gg.writeToDisk          = slice->writeToDisk;
      gg.rollOut              = slice->rollOut;

      /* suppress display for high level function */
      if (sgldisplay == DISPLAY_TOP_LEVEL) 
         {
         gg.display = DISPLAY_NONE;
       	 }      

      /* make the waveform */
      calcGeneric(&gg);
   
      /* check for errors */
      if (gg.error != ERR_NO_ERROR)
      {
         slice->error = gg.error;
         displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);	 
         return;
      }
   
      /* assign data to slice structure */
      slice->ssamp      = gg.amp;
      slice->dataPoints = gg.dataPoints;
      slice->duration   = gg.duration;
      slice->error      = gg.error;
      slice->m0         = gg.m0;
      slice->m1         = gg.m1;
      strcpy(slice->name, gg.name);
      slice->numPoints  = gg.numPoints;
      slice->tramp      = gg.tramp;
						slice->slewRate   = gg.slewRate;

      /* find data point index for center of rf pulse */
      startIndex = gg.ramp1.numPoints +
                   (long)ROUND(slice->pad1 / slice->resolution) +
                   (long)ceil(pulseDuration * (1.0 - slice->rfFraction) /
                              slice->resolution) + 1;

      /* if rfFractions places the start index between system ticks, we'll need */
      /* a delta to account for the added time */
      delta = (ceil(pulseDuration * (1.0 - slice->rfFraction) / slice->resolution) -
                   (pulseDuration * (1.0 - slice->rfFraction) / slice->resolution)) *
                slice->resolution;

      if (FP_NEQ(delta, 0))
      {
         startIndex--;
      }

      /* calculate moments */
      calculateMoments(gg.dataPoints, gg.numPoints,
                       1,            /* start index */
                       0,            /* start amplitude */
                       gg.resolution, &gg.m0,
                       &gg.m1,
                       0,            /* start delta offset */
                       0);           /* end delta offset */
      calculateMoments(gg.dataPoints, gg.numPoints,
                       startIndex,   /* start index */
                       gg.amp, /* start amplitude */
                       gg.resolution, &slice->m0ref,
                       &slice->m1ref,
                       delta,        /* start delta offset */
                       0);           /* end delta offset */
      slice->plateauDuration = pulseDuration + slice->pad1 +
                               slice->pad2;
      slice->rfDelayFront = (pulseDuration - slice->rfDuration)/2.0+
                             slice->pad1 + slice->tramp - rfPulse->rof1;
      slice->rfDelayBack = (pulseDuration- slice->rfDuration)/2.0+
                            slice->pad2+slice->tramp - rfPulse->rof2;		       
 
      /* RF refocus time - time between center of RF and end of gradient */
      slice->rfCenterBack = rfPulse->rfDuration * slice->rfFraction +
                             slice->pad2 + slice->tramp;			    
      slice->rfCenterFront = slice->duration - slice->rfCenterBack;

   }
   else
   {
      slice->error = ERR_PARAMETER_REQUEST;
      displayError(slice->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* RF refocus time - time between center of RF and end of gradient */
   slice->rfCenterBack = rfPulse->rfDuration * slice->rfFraction + 
                        + rfPulse->rof2 + slice->rfDelayBack;
   slice->rfCenterFront = slice->duration - slice->rfCenterBack;
   
   slice->m0def = slice->m0 - slice->m0ref;
   
   /* find the maximum amplitude of the waveform */
   slice->amp = findMax(slice->dataPoints, slice->numPoints, 0);
   slice->amp = fabs(slice->amp);

   /* display structure for debugging */   
   if ((sgldisplay == DISPLAY_TOP_LEVEL) ||(sgldisplay ==DISPLAY_MID_LEVEL) ||
       (sgldisplay == DISPLAY_LOW_LEVEL))  
       {
       displayRf(rfPulse);
       displaySlice(slice);
       }
}

/***********************************************************************
*  Function Name: calcRefocus
*  Example:    calcRefocus(&refocus);
*  Purpose:    Calculates the required values for the slice refocusing
*              gradient, creates the waveform file, updates the waveform
*              administration list and propagates the out structure with
*              the calculated values.  The calcFlag determines how the
*              calculations are executed.
*  Input
*     Formal:  *refocus      - refocus gradient structure
*     Private: none
*     Public:  MAX_SLEW_RATE - maximum slew rate of system
*              GRAD_MAX      - maximum possible gradient amplitude
*  Output
*     Return:  none
*     Formal:  *refocus - refocus gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calcRefocus (REFOCUS_GRADIENT_T *refocus)
{
   GENERIC_GRADIENT_T gg;
   
   initGeneric(&gg);
	/* make sure that calc flag is consistent with trampfixed */
	adjustCalcFlag( &(refocus->calcFlag) );

   /* check input values */
   if (FP_LTE(refocus->maxGrad, 0) ||
       FP_GT(refocus->maxGrad, GRAD_MAX))
   {
      /* maxGrad must be positive */
      refocus->error = ERR_MAX_GRAD;
      displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (refocus->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE ||
       refocus->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
       refocus->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE ||
       refocus->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP)
   {
      if (FP_GT(fabs(refocus->amp), refocus->maxGrad))
      {
         /* amplitude can't exceed maxGrad */
         refocus->error = ERR_AMPLITUDE;
         displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
      if (FP_EQ(refocus->amp, 0.0))
         {
	 /* make sure amplitude is larger 0 */
	 refocus->error = ERR_PARAMETER_REQUEST;
         displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }

   /* if the user supplied the duration then we need to check it for granularity */
   if (refocus->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE ||
       refocus->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
       refocus->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION ||
       refocus->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP)
   {
      if (FP_NEQ(refocus->duration, granularity(refocus->duration, refocus->resolution)))
      {
         /* duration must conform to resolution of system */
         refocus->error = ERR_GRANULARITY;
         displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
      if (FP_LT(refocus->duration, refocus->resolution))
      {
         /* in this case duration shouldn't be zero */
         refocus->error = ERR_PARAMETER_REQUEST;
         displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }   
   if ((refocus->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION      || 
        refocus->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP || 
        refocus->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE      || 
        refocus->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP || 
        refocus->calcFlag == SHORTEST_DURATION_FROM_MOMENT       || 
        refocus->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP) && 
       FP_EQ(refocus->balancingMoment0, 0))
   {
      /* make sure the moment is supplied */
      refocus->error = ERR_PARAMETER_REQUEST;
      displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }   

   if (refocus->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
       refocus->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP ||
       refocus->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP ||
       refocus->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP)
   {
      /* make sure a ramp time is supplied */
      if (FP_EQ(refocus->tramp, 0))
      {
         refocus->error = ERR_RAMP_TIME;
         displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }
   
   if (FP_LTE(refocus->resolution, 0))
   {
      /* resolution invalid */
      refocus->error = ERR_RESOLUTION;
      displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(refocus->duration, 0))
   {
      /* duration cannot be negative */
      refocus->error = ERR_DURATION;
      displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(refocus->tramp, 0))
   {
      /* ramp time cannot be negative */
      refocus->error = ERR_RAMP_TIME;
      displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (refocus->rampShape == GAUSS)
   {
      refocus->error = ERR_RAMP_SHAPE;
      displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   if (FP_NEQ(refocus->areaOffset, 0) ||
       FP_NEQ(refocus->amplitudeOffset, 0) ||
       FP_NEQ(refocus->fov,0) ||
       FP_NEQ(refocus->increment,0) ||
       FP_NEQ(refocus->startAmplitude,0) ||
       FP_NEQ(refocus->noPlateau,0) ||
       FP_NEQ(refocus->polarity, 0) ||
       FP_NEQ(refocus->steps, 0))
   {
      /* Parameter is not used in refocus calculation */
      refocus->error = ERR_PARAMETER_NOT_USED_REFOCUS;
      displayError(refocus->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   initGeneric(&gg);

   /* populate generic structure */
   gg.amp                  = fabs(refocus->amp);
   gg.calcFlag             = refocus->calcFlag;
   gg.duration             = refocus->duration;
   gg.maxGrad              = refocus->maxGrad;
   gg.m0                   = refocus->balancingMoment0;
   strcpy(gg.name, refocus->name);
   gg.tramp                = refocus->tramp;
   gg.ramp1.duration       = refocus->tramp;
   gg.ramp2.duration       = refocus->tramp;
   gg.ramp1.shape.function = refocus->rampShape;
   gg.ramp2.shape.function = refocus->rampShape;
   gg.resolution           = refocus->resolution;
   gg.rollOut              = refocus->rollOut;
   gg.shape                = refocus->shape;
   gg.writeToDisk          = refocus->writeToDisk;

   /* suppress display for high level function */
   if (sgldisplay == DISPLAY_TOP_LEVEL) 
      gg.display = DISPLAY_NONE;
   
   /* generate the waveform */
   calcGeneric(&gg);

   /* check for errors */
   if (gg.error != ERR_NO_ERROR)
   {
      refocus->error = gg.error;
      return;
   }

   /* assign data to phase structure */
   refocus->amp        = gg.amp;
   refocus->dataPoints = gg.dataPoints;
   refocus->duration   = gg.duration;
   refocus->m0         = gg.m0;
   refocus->m1         = gg.m1;
   refocus->numPoints  = gg.numPoints;
   refocus->tramp      = gg.tramp;
			refocus->slewRate   = gg.slewRate;
   strcpy(refocus->name, gg.name);

   /* calculate moments */
   calculateMoments(gg.dataPoints, gg.numPoints,
                    1,  /* start index */
                    0,  /* start amplitude */
                    gg.resolution, &gg.m0,
                    &gg.m1,
                    0,  /* start delta offset */
                    0); /* end delta offset */

   refocus->amp = fabs(refocus->amp);

   /* display structure for debugging */   
   if ((sgldisplay == DISPLAY_TOP_LEVEL) ||(sgldisplay ==DISPLAY_MID_LEVEL) ||
       (sgldisplay == DISPLAY_LOW_LEVEL))  displayRefocus(refocus);

}

/***********************************************************************
*  Function Name: calcReadout
*  Example:    calcReadout(&readout);
*  Purpose:    Creates a readout gradient depending on the input
*              arguments.  It calculates the readout gradient strength
*              from the parameters contained in the input structure,
*              creates the gradient waveform file, updates the waveform
*              administration list, and propagates the out structure
*              with the calculated values.  The calcFlag determines
*              how the calculations are executed.  Butterfly crushers
*              are supported.
*  Input
*     Formal:  *ro      - readout gradient structure
*     Private: none
*     Public:  GRAD_MAX - maximum possible gradient amplitude
*  Output
*     Return:  none
*     Formal:  *ro - readout gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calcReadout (READOUT_GRADIENT_T *ro)
{
   GENERIC_GRADIENT_T gg;
   BUTTERFLY_GRADIENT_T bfly;
   long endIndex;
   BANDWIDTH_T bw;
   double delta;
   double acqTime       = 0.0;   /* granulated acquisition time */
   double fracAcqTime   = 0.0;   /* granulated fractional acquisition time */
	double deltaFracTime = 0.0;   /* differnce between calculated and granulated fractional acquisition time */
	
	double echoFraction = 0.0;    /* echo fraction to adjust gradient duration */
   double echoFracFront = 0.0;   /* fraction of gradient to dephase */
   long longduration; /* used to regranulate fracAcqTime */
					  
   initGeneric(&gg);
   initButterfly(&bfly);
   initBandwidth(&bw);

	/* make sure that calc flag is consistent with trampfixed */
	adjustCalcFlag( &(ro->crusher1CalcFlag) );
	adjustCalcFlag( &(ro->crusher2CalcFlag) );

   /* check input values */
   if (FP_LTE(ro->maxGrad, 0) ||
       FP_GT(ro->maxGrad, GRAD_MAX))
   {
      /* maxGrad must be positive */
      ro->error = ERR_MAX_GRAD;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LTE(ro->resolution, 0))
   {
      /* resolution invalid */
      ro->error = ERR_RESOLUTION;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(ro->acqTime, 0))
   {
      /* acqTime cannot be negative */
      ro->error = ERR_PARAMETER_REQUEST;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(ro->fov, 0))
   {
      /* fov cannot be negative */
      ro->error = ERR_PARAMETER_REQUEST;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(ro->bandwidth, 0))
   {
      /* bandwidth cannot be negative */
      ro->error = ERR_BANDWIDTH;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(ro->pad1, 0) ||
       FP_LT(ro->pad2, 0))
   {
      /* padding cannot be negative */
      ro->error = ERR_GRADIENT_PADDING;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(ro->numPointsFreq, 0)) 
   {
      /* number of points cannot be negative */
      ro->error = ERR_NUM_POINTS;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(ro->echoFraction, 0.5) ||
       FP_GT(ro->echoFraction, 1.5))
   {
      /* echoFraction out of range */
      ro->error = ERR_ECHO_FRACTION;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(ro->tramp, 0))
   {
      ro->error = ERR_RAMP_TIME;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(ro->acqTime, 0))
   {
      /* acqTime out of range */
      ro->error = ERR_RF_DURATION;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (ro->rampShape == GAUSS || ro->rampShape == SINE) 
   {
      ro->error = ERR_RAMP_SHAPE;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* granulate padding */
   ro->pad2  = granularity(ro->pad2, ro->resolution);
   ro->pad1 = granularity(ro->pad1, ro->resolution);

   /* check to see if we need to do some calculations */
   if ((FP_EQ(ro->acqTime, 0) && ro->numPointsFreq >= 0 && FP_GT(ro->bandwidth, 0)) ||
       (FP_GT(ro->acqTime, 0) && ro->numPointsFreq == 0 && FP_GT(ro->bandwidth, 0)) ||
       (FP_GT(ro->acqTime, 0) && ro->numPointsFreq > 0 && FP_EQ(ro->bandwidth, 0)))
   {
      /* copy data to bandwidth structure */
      bw.points     = ro->numPointsFreq * 2.0;
      bw.bw         = ro->bandwidth;
      bw.acqTime    = ro->acqTime;
      bw.resolution = ro->resolution;
   
      calcBandwidth(&bw);
      /* check for errors */
      if (bw.error != ERR_NO_ERROR)
      {
         ro->error = bw.error;
         return;
      }

      /* copy data to readout structure */
      ro->numPointsFreq = bw.points / 2;
      ro->bandwidth     = bw.bw;
      ro->acqTime       = bw.acqTime;
   }
   else
   {
      /* user has supplied spectral information, check it */
      if (FP_NEQ(ro->bandwidth, ro->numPointsFreq / ro->acqTime))
      {
         ro->error = ERR_INCONSISTENT_PARAMETERS;
         displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }

   /* calculate readout gradient amplitude */
   ro->roamp = ro->numPointsFreq / (ro->acqTime * ro->gamma * ro->fov * MM_TO_CM);

   /* check amplitude */
   if ((FP_GT(ro->roamp, ro->maxGrad)) ||
       (FP_GT(ro->amp, ro->maxGrad)))
   {
      ro->error = ERR_AMPLITUDE;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* calculate ramp time */
   if (FP_EQ(ro->tramp, 0))
   {
      if (ro->rampShape == PRIMITIVE_LINEAR)
      {
         ro->tramp = ro->roamp / ro->slewRate;
      }
      else if (ro->rampShape == PRIMITIVE_SINE)
      {
         ro->tramp = ro->amp * PI / (2.0 * ro->slewRate);
      }
   }

   /* granulate ramp time*/
   ro->tramp = granularity(ro->tramp, ro->resolution);

   /* granulate acquisition time */
   acqTime = granularity(ro->acqTime, ro->resolution);

   /* calculate reduce acquisition time and echo fractions */
	/* We expect that the already reduced echo time and np is passed in correctly */
   /* fracAcqTime = granularity((ro->acqTime*ro->echoFraction), ro->resolution);*/
	fracAcqTime = granularity(ro->acqTime, ro->resolution);

/* fracAcqTime doesn't seem to get granulated with great enough accuracy ???*/
/* The following line will show it */
/* fprintf(stdout,"fracAcqTime = %.20f\n", fracAcqTime); */
/* regranulate */
  longduration = (long)floor(fracAcqTime/ro->resolution);
  if (fracAcqTime/ro->resolution-(double)longduration >= 0.5) longduration++;
  fracAcqTime = longduration*ro->resolution;
/* The following line will show new value */
/* fprintf(stdout,"fracAcqTime = %.20f\n", fracAcqTime); */

   echoFraction   = ro->echoFraction;
   echoFracFront  = ro->echoFraction - 0.5; 
   /* 2.3A and earlier used the non-linear    echoFracFront  = 1 - (0.5 /ro->echoFraction);   */
	/* allow shifting of echo to end of gradient without extending gradient */
	/*  This no no longer needed in the current aplication */
	 /*
	if (FP_GT(ro->echoFraction,1.0))
		 {
					echoFraction = 2.0 - ro->echoFraction;
     fracAcqTime = granularity((ro->acqTime * echoFraction), ro->resolution);
     echoFracFront = 0.5;
		}
	*/
			
			/* Delta to account for difference between at and granulated at */
			/*deltaFracTime = (fracAcqTime - ro->acqTime * echoFraction)) / 2.0;*/
			deltaFracTime = (acqTime - ro->acqTime ) / 2.0;

   /* calculate number of points */
   /*   ro->duration  = 2 * ro->tramp + acqTime;
   ro->duration  = granularity(ro->duration, ro->resolution);
   ro->numPoints = (long)ROUND(ro->duration / ro->resolution);
  */
   if (ro->enableButterfly == TRUE)
   {
      if (strcmp(ro->name, "") == 0)
      {
         /* string is empty, supply a default name */
         strcpy(ro->name, "readout_butterfly_");
      }

      /* populate butterfly structure */
      /* first crusher */
      bfly.cr1amp           = ro->cr1amp;
      bfly.cr1CalcFlag      = ro->crusher1CalcFlag;
      bfly.cr1TotalDuration = ro->crusher1Duration;
      bfly.cr1Moment0       = ro->crusher1Moment0;
      bfly.ramp1.duration   = ro->crusher1RampToCrusherDuration;
      bfly.ramp2.duration   = ro->crusher1RampToSsDuration;  

      /* second crusher */
      bfly.cr2amp           = ro->cr2amp;
      bfly.cr2CalcFlag      = ro->crusher2CalcFlag;
      bfly.cr2TotalDuration = ro->crusher2Duration;
      bfly.cr2Moment0       = ro->crusher2Moment0;
      bfly.ramp3.duration   = ro->crusher1RampToSsDuration;  
      bfly.ramp4.duration   = ro->crusher1RampToCrusherDuration;
						
      bfly.resolution       = ro->resolution;
      bfly.ramp1.resolution = ro->resolution;
      bfly.cr1.resolution   = ro->resolution;
      bfly.ramp2.resolution = ro->resolution;
      bfly.ss.resolution    = ro->resolution;
      bfly.ramp3.resolution = ro->resolution;
      bfly.cr2.resolution   = ro->resolution;
      bfly.ramp4.resolution = ro->resolution;

      bfly.ssCalcFlag      = MOMENT_FROM_DURATION_AMPLITUDE;
      bfly.ssamp           = ro->roamp;
      /* bfly.ssTotalDuration = fracAcqTime + ro->pad1 + ro->pad2;*/
						bfly.ssTotalDuration = acqTime + ro->pad1 + ro->pad2;
      bfly.writeToDisk     = ro->writeToDisk;
      bfly.rollOut         = ro->rollOut;
      strcpy(bfly.name, ro->name);

      /* make the waveform */
      calcButterfly(&bfly);

      /* check for errors */
      if (bfly.error != ERR_NO_ERROR)
      {
         ro->error = bfly.error;
         return;
      }
   
      /* assign data to readout structure */
      /* first crusher */
      ro->cr1amp                        = bfly.cr1amp;
      ro->crusher1Duration              = bfly.cr1TotalDuration;
      ro->crusher1Moment0               = bfly.cr1Moment0;
      ro->crusher1RampToCrusherDuration = bfly.ramp1.duration;
      ro->crusher1RampToSsDuration      = bfly.ramp2.duration;

      /* readout plateau duration */
      ro->plateauDuration               = bfly.ssTotalDuration;
      ro->roamp                         = bfly.ssamp;

      /* second crusher */
      ro->cr2amp                        = bfly.cr2amp;
      ro->crusher2Duration              = bfly.cr2TotalDuration;
      ro->crusher2Moment0               = bfly.cr2Moment0;
      ro->crusher2RampToCrusherDuration = bfly.ramp3.duration;
      ro->crusher2RampToSsDuration      = bfly.ramp4.duration;
      ro->slewRate                      = bfly.slewRate;    
      /* assign output values */
      ro->amp        = bfly.amp;
      ro->dataPoints = bfly.dataPoints;
      ro->duration   = bfly.cr1TotalDuration + bfly.ssTotalDuration + bfly.cr2TotalDuration;
      ro->numPoints  = bfly.numPoints;

      ro->m0         = bfly.m0;
      ro->m1         = bfly.m1;
      strcpy(ro->name, bfly.name);


      /* calculate delay times */
   	ro->atDelayFront = deltaFracTime + ro->crusher1Duration + ro->pad1;
      ro->atDelayBack =  deltaFracTime + ro->crusher2Duration + ro->pad2;

      /* time from start of gradient to center of echo */
      ro->timeToEcho = ro->atDelayFront + (ro->acqTime * echoFracFront);
						
      /* time from center of echo to end of gradient */
      /*ro->timeToEcho = ro->atDelayFront + (fracAcqTime * echoFracFront);*/
      ro->timeFromEcho =  ro->duration - ro->timeToEcho;

      /* find data point index for beginning of echo */
      endIndex = bfly.ramp1.numPoints + bfly.cr1.numPoints + bfly.ramp2.numPoints +
                 (long)ROUND(ro->pad1 / ro->resolution) +
                 (long)floor(((ro->acqTime * echoFracFront) + deltaFracTime)/ ro->resolution);

      /* if echoFraction places the end index between system ticks, we'll need */
      /* a delta to account for the extra time */
      delta = ( (((ro->acqTime * echoFracFront) + deltaFracTime) / ro->resolution) -
               floor( ((ro->acqTime * echoFracFront) + deltaFracTime) / ro->resolution) ) *
              ro->resolution;

      if (FP_NEQ(delta, 0))
      {
         endIndex++;
      }

      /* calculate moments */
      calculateMoments(bfly.dataPoints, bfly.numPoints,
                       1,  /* start index */
                       0,  /* start amplitude */
                       bfly.resolution, &bfly.m0,
                       &bfly.m1,
                       0,  /* start delta offset */
                       0); /* end delta offset */
   
      /* calculate moments to be dephased */
      calculateMoments(bfly.dataPoints, endIndex,
                       1,      /* start index */
                       0,      /* start amplitude */
                       bfly.resolution, &ro->m0ref,
                       &ro->m1ref,
                       0,      /* start delta offset */
                       delta); /* end delta offset */

      /* copy moment data */
      ro->m0 = bfly.m0;
      ro->m1 = bfly.m1;

   }
   else if (ro->enableButterfly == FALSE)
   {
      if (strcmp(ro->name, "") == 0)
      {
         /* string is empty, supply a default name */
         strcpy(ro->name, "readout_");
      }

      /* populate generic structure */
      gg.calcFlag       = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
      gg.resolution     = ro->resolution;
      gg.amp            = ro->roamp;
      gg.duration       = (ro->tramp * 2) + fracAcqTime +
                          ro->pad1 + ro->pad2;
      gg.tramp          = ro->tramp;
      gg.ramp1.duration = ro->tramp;
      gg.ramp2.duration = ro->tramp;
      gg.ramp1.shape.function = ro->rampShape;
      gg.ramp2.shape.function = ro->rampShape;
      gg.plateau.duration     = acqTime + ro->pad1 +
                                ro->pad2;
      gg.writeToDisk = ro->writeToDisk;
      gg.rollOut     = ro->rollOut;
      strcpy(gg.name, ro->name);

      /* suppress display for high level function */
      if (sgldisplay == DISPLAY_TOP_LEVEL) 
         gg.display = DISPLAY_NONE;
      
      /* make the waveform */
      calcGeneric(&gg);

      /* check for errors */
      if (gg.error != ERR_NO_ERROR)
      {
         ro->error = gg.error;
         return;
      }
   
      /* assign output values */
      ro->roamp      = gg.amp;
      ro->duration   = gg.duration;
      ro->tramp      = gg.tramp;
      ro->dataPoints = gg.dataPoints;
      ro->numPoints  = gg.numPoints;
      ro->slewRate   = gg.slewRate;
						
      ro->m0         = gg.m0;
      ro->m1         = gg.m1;
      strcpy(ro->name, gg.name);

      /* calculate delay times */
      ro->atDelayFront = deltaFracTime + ro->tramp + ro->pad1;
      ro->atDelayBack = deltaFracTime +  ro->tramp + ro->pad2;
      ro->plateauDuration = fracAcqTime + ro->pad1 + ro->pad2;

      /* time from start of gradient to center of echo */
		ro->timeToEcho = ro->atDelayFront + (ro->acqTime * echoFracFront);
      
      /* time from center of echo to end of gradient */
		ro->timeFromEcho =  ro->duration - ro->timeToEcho;
						
      /* find data point index for beginning of echo */
      endIndex = gg.ramp1.numPoints +
                 (long)ROUND(ro->pad1 / ro->resolution) +
                 (long)floor(((ro->acqTime * echoFracFront) + deltaFracTime)/ ro->resolution);

      /* if echoFraction places the end index between system ticks, we'll need */
      /* a delta to account for the extra time */
      delta = (( ((ro->acqTime * echoFracFront) + deltaFracTime) / ro->resolution) -
               floor( ((ro->acqTime * echoFracFront) + deltaFracTime) / ro->resolution)) *
              ro->resolution;

      if (FP_NEQ(delta, 0))
      {
         endIndex++;
      }

      /* calculate moments */
      calculateMoments(gg.dataPoints, gg.numPoints,
                       1,  /* start index */
                       0,  /* start amplitude */
                       gg.resolution, &gg.m0,
                       &gg.m1,
                       0,  /* start delta offset */
                       0); /* end delta offset */
   
      /* calculate moments to be dephased */
      calculateMoments(gg.dataPoints, endIndex,
                       1,                 /* start index */
                       gg.startAmplitude, /* start amplitude */
                       gg.resolution, &ro->m0ref,
                       &ro->m1ref,
                       0,                 /* start delta offset */
                       delta);            /* end delta offset */

   }
   else
   {
      ro->error = ERR_PARAMETER_REQUEST;
      displayError(ro->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* reset fractional acq time to actual (none-granulated value) */
  			 ro->fracAcqTime = ro->acqTime * echoFraction ;
			
   ro->m0def = ro->m0 - ro->m0ref;

   
   /* find the maximum amplitude of the waveform */
   ro->amp = findMax(ro->dataPoints, ro->numPoints, 0);
   ro->amp = fabs(ro->amp);
   
   /* display structure for debugging */
   if ((sgldisplay == DISPLAY_TOP_LEVEL) ||(sgldisplay ==DISPLAY_MID_LEVEL) ||
       (sgldisplay == DISPLAY_LOW_LEVEL)) displayReadout(ro);
}

/***********************************************************************
*  Function Name: calcDephase
*  Example:    calcDephase(&dephase);
*  Purpose:    Calculates the required values for the de-phasing
*              gradient, creates the waveform file, updates the waveform
*              administration list and propagates the out structure
*              with the calculated values.  The calcFlag determines
*              how the calculations are executed.
*  Input
*     Formal:  *dephase - refocus gradient structure
*     Private: none
*     Public:  GRAD_MAX - maximum possible gradient amplitude
*  Output
*     Return:  none
*     Formal:  *dephase - refocus gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calcDephase (REFOCUS_GRADIENT_T *dephase)
{
   GENERIC_GRADIENT_T gg;

   initGeneric(&gg);

	/* make sure that calc flag is consistent with trampfixed */
	adjustCalcFlag( &(dephase->calcFlag) );

   /* check input values */
   if (FP_LTE(dephase->maxGrad, 0) ||
       FP_GT(dephase->maxGrad, GRAD_MAX))
   {
      /* maxGrad invalid */
      dephase->error = ERR_MAX_GRAD;
      displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (dephase->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE ||
       dephase->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
       dephase->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE ||
       dephase->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP)
   {
      if (FP_GT(dephase->amp, dephase->maxGrad))
      {
         dephase->error = ERR_AMPLITUDE;
         displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
      if (FP_EQ(dephase->amp, 0.0))
         {
	 /* make sure amplitude is larger 0 */
	 dephase->error = ERR_PARAMETER_REQUEST;
         displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }
   /* if the user supplied the duration then we need to check it for granularity */
   if (dephase->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE ||
       dephase->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
       dephase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION ||
       dephase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP)
   {
      if (FP_NEQ(dephase->duration, granularity(dephase->duration, dephase->resolution)))
      {
         /* duration must conform to resolution of system */
         dephase->error = ERR_GRANULARITY;
         displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
      if (FP_LT(dephase->duration, dephase->resolution))
      {
         /* in this case duration shouldn't be zero */
         dephase->error = ERR_PARAMETER_REQUEST;
         displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }   
   if ((dephase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION      || 
        dephase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP || 
        dephase->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE      || 
        dephase->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP || 
        dephase->calcFlag == SHORTEST_DURATION_FROM_MOMENT       || 
        dephase->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP) && 
       FP_EQ(dephase->balancingMoment0, 0))
   {
      /* make sure the moment is supplied */
      dephase->error = ERR_PARAMETER_REQUEST;
      displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }   

   if (dephase->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
       dephase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP ||
       dephase->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP ||
       dephase->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP)
   {
      /* make sure a ramp time is supplied */
      if (FP_EQ(dephase->tramp, 0))
      {
         dephase->error = ERR_RAMP_TIME;
         displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }

   if (FP_LT(dephase->duration, 0))
   {
      /* duration invalid */
      dephase->error = ERR_DURATION;
      displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(dephase->tramp, 0))
   {
      dephase->error = ERR_RAMP_TIME;
      displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LTE(dephase->resolution, 0))
   {
      /* resolution invalid */
      dephase->error = ERR_RESOLUTION;
      displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   if (dephase->rampShape == GAUSS)
   {
      dephase->error = ERR_RAMP_SHAPE;
      displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
      
   if (FP_NEQ(dephase->areaOffset, 0) ||
       FP_NEQ(dephase->amplitudeOffset, 0) ||
       FP_NEQ(dephase->fov,0) ||
       FP_NEQ(dephase->increment,0) ||
       FP_NEQ(dephase->startAmplitude,0) ||
       FP_NEQ(dephase->noPlateau,0) ||
       FP_NEQ(dephase->polarity, 0) ||
       FP_NEQ(dephase->steps, 0))
   {
      /* Parameter is not used in dephase calculation */
      dephase->error = ERR_PARAMETER_NOT_USED_DEPHASE;
      displayError(dephase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* populate generic structure */
   gg.amp                  = fabs(dephase->amp);
   gg.calcFlag             = dephase->calcFlag;
   gg.duration             = dephase->duration;
   gg.m0                   = dephase->balancingMoment0;
   gg.maxGrad              = dephase->maxGrad;
   strcpy(gg.name, dephase->name);
   gg.tramp                = dephase->tramp;
   gg.ramp1.shape.function = dephase->rampShape;
   gg.ramp2.shape.function = dephase->rampShape;
   gg.resolution           = dephase->resolution;
   gg.rollOut              = dephase->rollOut;
   gg.shape                = dephase->shape;
   gg.writeToDisk          = dephase->writeToDisk;

   /* suppress display for high level function */
   if (sgldisplay == DISPLAY_TOP_LEVEL) 
      gg.display = DISPLAY_NONE;

   /* make the waveform */
   calcGeneric(&gg);
   
   /* check for errors */
   if (gg.error != ERR_NO_ERROR)
   {
      dephase->error = gg.error;
      return;
   }
   
   /* assign output values */
   dephase->amp        = gg.amp;
   dephase->duration   = gg.duration;
   dephase->tramp      = gg.tramp;
   dephase->slewRate   = gg.slewRate;
			dephase->dataPoints = gg.dataPoints;
   dephase->numPoints  = gg.numPoints;
   
   dephase->m0         = gg.m0;
   dephase->m1         = gg.m1;
   strcpy(dephase->name, gg.name);

   /* display structure for debugging */
   if ((sgldisplay == DISPLAY_TOP_LEVEL) ||(sgldisplay ==DISPLAY_MID_LEVEL) ||
       (sgldisplay == DISPLAY_LOW_LEVEL)) displayDephase(dephase);
}

/***********************************************************************
*  Function Name: calcPhase
*  Example:    calcPhase(&phaseEncodeStructure);
*  Purpose:    Calculates the required values for the phase encode
*              gradient, creates the waveform file, updates the waveform
*              administration list and propagates the out structure
*              with the calculated values.  The calcFlag determines
*              how the calculations are executed.
*  Input
*     Formal:  *phase   - phase encode gradient structure
*     Private: none
*     Public:  GRAD_MAX - maximum possible gradient amplitude
*  Output
*     Return:  none
*     Formal:  *phase - phase encode gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calcPhase (PHASE_ENCODE_GRADIENT_T *phase)
{
   GENERIC_GRADIENT_T gg;
   double  offsetMoment = 0.0;
   double  delta_tramp  = 0.0;
   double  tPhase       = 0.0;
   
   initGeneric(&gg);

	/* make sure that calc flag is consistent with trampfixed */
	adjustCalcFlag( &(phase->calcFlag) );

   /* check input values */
   if (FP_LTE(phase->maxGrad, 0) ||
       FP_GT(phase->maxGrad, GRAD_MAX))
   {
      /* maxGrad must be positive */
         printf("error phase->maxGrad=%g gmax=%g\n",phase->maxGrad,GRAD_MAX);
      
      phase->error = ERR_MAX_GRAD;
      displayError(phase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LTE(phase->resolution, 0))
   {
      /* resolution must be larger 0 */
      phase->error = ERR_RESOLUTION;
      displayError(phase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(phase->duration, 0))
   {
      phase->error = ERR_DURATION;
      displayError(phase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(phase->tramp, 0))
   {
      phase->error = ERR_RAMP_TIME;
      displayError(phase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_GT(fabs(phase->amplitudeOffset), phase->maxGrad))
   {
      phase->error = ERR_AMPLITUDE;
      displayError(phase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LTE(phase->fov, 0))
   {
      /* FOV cannot be negative */
      phase->error = ERR_FOV;
      displayError(phase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LTE(phase->steps, 0))
   {
      /* steps cannot be negative */
      phase->error = ERR_STEPS;
      displayError(phase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(phase->increment, 0))
   {
      /* Increment cannot be negative */
      phase->error = ERR_INCREMENT;
      displayError(phase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (phase->rampShape == GAUSS)
   {
      phase->error = ERR_RAMP_SHAPE;
      displayError(phase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
 
   if (FP_NEQ(phase->balancingMoment0, 0) ||
       FP_NEQ(phase->startAmplitude,0) ||
       FP_NEQ(phase->polarity, 0) )
   {
      /* Parameter is not used in dephase calculation */
      phase->error = ERR_PARAMETER_NOT_USED_PHASE;
      displayError(phase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   if (phase->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE ||
       phase->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
       phase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION ||
       phase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP)
   {
      /* granulate duration of phase encode gradient */
      phase->duration = granularity(phase->duration, phase->resolution);
   }

   /* only one offset can be provided */
   if ((FP_NEQ(phase->amplitudeOffset, 0))   &&
       (FP_NEQ(phase->areaOffset, 0)))
   {
      phase->error = ERR_PHASE_ENCODE_OFFSET;
      displayError(phase->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }       

   /* see if moment0 is required in waveform generation */
   if (phase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION      ||
       phase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP ||
       phase->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE      ||
       phase->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP ||
       phase->calcFlag == SHORTEST_DURATION_FROM_MOMENT       ||
       phase->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP)
   {
      /* only calculate it when the user hasn't set it */
      if (FP_EQ(phase->m0, 0))
      {
         /* calculate phase area */
         phase->m0 = 1.0 / (phase->gamma * phase->fov * MM_TO_CM) *
                          (phase->steps / 2.0);
      }
   }
   /* we need to determine the offset moment if the offset amplitude or are is supplied */
   if ((FP_NEQ(phase->amplitudeOffset, 0)) ||
       (FP_NEQ(phase->areaOffset, 0)))
   {
      offsetMoment = phase->areaOffset;          /* assign area offset */
      
      /* if the amplitude is supplied determine the area */
      if (FP_NEQ(phase->amplitudeOffset, 0))      
      {
	 if (phase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP ||
             phase->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP ||
             phase->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP)
         {
	    /* We need to adjust the ramp time to allow amplitude offset */
	    delta_tramp = granularity( (fabs(phase->amplitudeOffset)/MAX_SLEW_RATE),phase->resolution);
	 }
     
         if (phase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP ||
             phase->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION      ||
             phase->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
	     phase->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE )
         {
	    /* we need to adjust the duration for the offset ramp time */
	    delta_tramp =  fabs(phase->amplitudeOffset)/MAX_SLEW_RATE;
            tPhase = phase->duration - granularity( (2.0 * delta_tramp), phase->resolution );
	    delta_tramp = granularity( delta_tramp,phase->resolution);	    
	 }

         if (phase->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP  ||
    	     phase->calcFlag == SHORTEST_DURATION_FROM_MOMENT       ||
	     phase->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP ||
	     phase->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE )
	 {    	         
		

            /* populate generic structure */
            gg.calcFlag             = phase->calcFlag;
            gg.duration             = phase->duration - (2.0*delta_tramp);
            gg.maxGrad              = phase->maxGrad - fabs(phase->amplitudeOffset);
            gg.m0                   = phase->m0;
            gg.tramp                = phase->tramp - delta_tramp;
            gg.ramp1.duration       = phase->tramp - delta_tramp;
            gg.ramp2.duration       = phase->tramp - delta_tramp;
            gg.ramp1.shape.function = phase->rampShape;
            gg.ramp2.shape.function = phase->rampShape;
            gg.resolution           = phase->resolution;
            gg.rollOut              = phase->rollOut;
            gg.shape                = phase->shape;
            gg.writeToDisk          = FALSE; /* we don't want to write this to disk, its */
                                             /* just an intermediate step */
            strcpy(gg.name, phase->name);																																													
	    
            /* suppress display for high level function */
            if (sgldisplay == DISPLAY_TOP_LEVEL) 
	       gg.display = DISPLAY_NONE;

            /* find the duration of the phase encode without offset*/
            calcGeneric(&gg);

            /* check for errors */
            if (gg.error != ERR_NO_ERROR)
            {
               phase->error = gg.error;
               return;
            }
            tPhase = gg.duration;
         }
         /* calculate area resulting from amplitude offset */
         offsetMoment = ((fabs(phase->amplitudeOffset)/MAX_SLEW_RATE) + tPhase)*fabs(phase->amplitudeOffset);
	 phase->areaOffset = offsetMoment;
      }
      /* recalculate gradient - this is the waveform to use */
      gg.m0 = fabs(phase->m0) + fabs(offsetMoment);
      gg.maxGrad      = phase->maxGrad; 
      gg.calcFlag     = phase->calcFlag;
      gg.duration     = phase->duration;
      gg.tramp        = phase->tramp;
      gg.writeToDisk  = phase->writeToDisk;
      strcpy(gg.name,phase->name);
      
      calcGeneric(&gg);

      /* check for errors */
      if (gg.error != ERR_NO_ERROR)
      {
         phase->error = gg.error;
         return;
      }
 
      /* assign data of interest to phase structure */
      phase->duration   = gg.duration;
      phase->tramp      = gg.tramp;
      phase->dataPoints = gg.dataPoints;
      phase->numPoints  = gg.numPoints;
      phase->amp        = gg.amp;  /* Maximum gradient */
						phase->slewRate   = gg.slewRate;

      /* scale phase amplitude down by ratio of m0 to m0+areaOffset */
      phase->peamp      = phase->m0/gg.m0*gg.amp;
      phase->offsetamp  = phase->areaOffset/gg.m0*gg.amp;
      phase->m1         = phase->areaOffset/gg.m0*gg.m1; 

      phase->increment  = phase->peamp/(phase->steps/2);
      strcpy(phase->name, gg.name);
	  }
   else    /* no offset given */
   {
      /* populate generic structure */
      gg.amp                  = fabs(phase->amp);
      gg.calcFlag             = phase->calcFlag;
      gg.duration             = phase->duration;
      gg.maxGrad              = phase->maxGrad;
      gg.m0                   = phase->m0;
      strcpy(gg.name, phase->name);
      gg.tramp                = phase->tramp;
      gg.ramp1.duration       = phase->tramp;
      gg.ramp2.duration       = phase->tramp;
      gg.ramp1.shape.function = phase->rampShape;
      gg.ramp2.shape.function = phase->rampShape;
      gg.resolution           = phase->resolution;
      gg.rollOut              = phase->rollOut;
      gg.shape                = phase->shape;
      gg.writeToDisk          = phase->writeToDisk;

      /* suppress display for high level function */
      if (sgldisplay == DISPLAY_TOP_LEVEL) 
         gg.display = DISPLAY_NONE;
      
      /* make the waveform */
      calcGeneric(&gg);

      /* check for errors */
      if (gg.error != ERR_NO_ERROR)
      {
       	 phase->error = gg.error;
       	 return;
      }
  
      /* assign data to phase structure */
      phase->amp        = gg.amp;
      phase->peamp      = gg.amp;
      phase->offsetamp  = 0;
      phase->dataPoints = gg.dataPoints;
      phase->duration   = gg.duration;
      phase->m0         = gg.m0;
      phase->m1         = gg.m1;
      phase->numPoints  = gg.numPoints;
      phase->tramp      = gg.tramp;
						phase->slewRate   = gg.slewRate;
      strcpy(phase->name, gg.name);
      
      /* calculate increment */
      phase->increment = (phase->amp) / (phase->steps / 2);
      phase->amp = fabs(phase->amp);
      
      /* calculate moments */
      calculateMoments(gg.dataPoints, gg.numPoints,
                       1,  /* start index */
                       0,  /* start amplitude */
                       gg.resolution, &gg.m0,
                       &gg.m1,
                      0,  /* start delta offset */
                      0); /* end delta offset */
   }
   /* display structure for debugging */
   if ((sgldisplay == DISPLAY_TOP_LEVEL) ||(sgldisplay ==DISPLAY_MID_LEVEL) ||
       (sgldisplay == DISPLAY_LOW_LEVEL)) displayPhase(phase);
}


/***********************************************************************
*  Function Name: calcBandwidth
*  Example:    calcBandwidth(&bandwidth);
*  Purpose:    Calculates the bandwidth and associated acquisition
*              parameters depending on how the input structure is filled
*              in.
*                 1. Calculates the acquisition time when bandwidth and
*                    number of points are supplied.
*                 2. Calculates the number of sample points when
*                    bandwidth and acquisition time are supplied.
*                 3. Calculates the bandwidth when acquisition time and
*                    number of points is supplied.
*  Input
*     Formal:  *bw          - bandwidth structure
*     Private: none
*     Public:  GRADIENT_RES - system timing resolution
*  Output
*     Return:  none
*     Formal:  *bw - bandwidth structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calcBandwidth (BANDWIDTH_T *bw)
{
   /* check input parameters */
   if (FP_LT(bw->acqTime, 0) ||
       bw->points < 0 ||
       FP_LT(bw->bw, 0))
   {
      bw->error = ERR_PARAMETER_REQUEST;
      displayError(bw->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if ((FP_NEQ(bw->acqTime, 0) && bw->points <= 0 && FP_EQ(bw->bw, 0)) ||
       (FP_EQ(bw->acqTime, 0) && bw->points != 0 && FP_EQ(bw->bw, 0))  ||
       (FP_EQ(bw->acqTime, 0) && bw->points == 0 && FP_NEQ(bw->bw, 0)))
   {
      bw->error = ERR_PARAMETER_REQUEST;
      displayError(bw->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* force duration to gradient interval */
/*   bw->acqTime = granularity(bw->acqTime, bw->resolution);*/

   /* all three parameters are supplied */
   if (bw->acqTime && bw->points && bw->bw)
   {
      bw->error = ERR_SPECTRAL_WIDTH_OVERDETERMINED;
      displayError(bw->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   /* points and bandwidth supplied, need acqTime */
   else if (!bw->acqTime && bw->points && bw->bw)
   {
      /* calculate acquisition time */
      bw->acqTime = bw->points / (2 * bw->bw);
   }
   /* acqTime and bandwidth supplied, need points */
   else if (bw->acqTime && !bw->points && bw->bw)
   {
      /* calculate sample points */
      bw->points = ROUND(bw->acqTime * 2 * bw->bw);
   }

   /* calculate bandwidth */
   bw->bw = bw->points / (2 * bw->acqTime);

   /* check if max bandwidth is exceeded */
   if (FP_GT(bw->bw, MAX_BW))
   {
      bw->error = ERR_SPECTRAL_WIDTH_MAX;
      displayError(bw->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   
   /* display structure for debugging */
   if ((sgldisplay ==DISPLAY_MID_LEVEL) ||(sgldisplay == DISPLAY_LOW_LEVEL)) 
      displayBandwidth(bw);
}

/***********************************************************************
*  Function Name: calcSliceFlowcomp
*  Example:    calcSliceFlowcomp(&flowcompSlice, &slice);
*  Purpose:    Creates the slice select flow compensation gradient
*              waveform.
*  Input
*     Formal:  *flowcomp - slice flow compensation gradient structure
*              *slice    - slice select gradient structure
*     Private: none
*     Public:  T_RISE    - time to get from zero to GRAD_MAX 
*  Output
*     Return:  none
*     Formal:  *flowcomp_slice - slice flow compensation gradient
*                                structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calcSliceFlowcomp(FLOWCOMP_T *fc, SLICE_SELECT_GRADIENT_T *slice)
{
   double risetime;        /* gradient rise time [ms] */
   double rampArea;        /* area under ramp */
   double area;            /* area of slice select - 0th moment */
   double areaSquare;      /* squared slice select area */
   double rampAreaSquared; /* squared ramp area */
   double temp1, temp2;    /* variables holding intermediate results */
   double temp3, temp4;    /* variables holding intermediate results */
   double temp5;           /* variables holding intermediate results */
   double areaLobe1;       /* area of 1st compensation lobe */
   double areaLobe2;       /* area of 2nd compensation lobe */
   double plateau1;        /* duration of gradient plateau 1st compensation lobe */
   double plateau2;        /* duration of gradient plateau 2nd compensation lobe */
   double areaFactor;      /* Area factor for fixed ramp calculation (arbitrary pick) */
			double areaFactorDec;   /* Area factor decrement for iteration */
   double momentLobe1;     /* first moment of lobe 1 */
			double momentLobe2;     /* first moment of lobe2 */
			int    noIterator;      /* Number of iterations */
			int    maxIterations;   /* Max number of iterations */
   double deltaT;          /* time shift for 2nd lobe  */
			
   MERGE_GRADIENT_T mg1;
   MERGE_GRADIENT_T mg2;

//   char grad_params[MAX_STR];
   char *gParamStr;
   char tempname[MAX_STR];
//   char tempstr[MAX_STR];

			/* check calculation Flag */
			if ((fc->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE) ||
       (fc->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION) ||
							(fc->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE) ||
							(fc->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP) ||
							(fc->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP) ||
							(fc->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP) )
			{
			   fc->error = ERR_CALC_FLAG;
      displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
      return;
 			}				
   /* initialize structures */
   initGeneric(&fc->lobe1);
			initPrimitive(&fc->plat); 
	  initGeneric(&fc->lobe2);

   /* If constant ramp time is required */
 		if ( (FP_GT(trampfixed, 0.0)) || 
 			    (fc->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP))
			{ 
        risetime = granularity(fc->rampTime1, fc->resolution);
			}	
   else
			{
     			risetime = granularity(T_RISE, fc->resolution);
			}

   /* initialize variables */		
   rampArea        = risetime * fc->maxGrad;
   area            = fabs(slice->m0ref);
   areaSquare      = area * area;
   rampAreaSquared = rampArea * rampArea;
			
   areaLobe1       = 0;
   areaLobe2       = 0;
   temp1           = 0;
   temp2           = 0;
   temp3           = 0;
			deltaT          = 0.0;  
   areaFactor	     = 1.2;
			areaFactorDec	  = 0.01;
			noIterator      = 0;
			maxIterations  = 25;

   /* save  features for later comparison */
 
   gParamStr = NULL;
	
	appendFormattedString( &gParamStr, "%f ", slice->m0ref );
	appendFormattedString( &gParamStr, "%f ",slice->m1ref );
	appendFormattedString( &gParamStr, "%f ",fc->maxGrad );
	appendFormattedString( &gParamStr, "%f ",fc->resolution );

   if ( (FP_GT(trampfixed, 0.0)) ||
		      (fc->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP) )
		 {

      do
						{
						   noIterator = noIterator +1;
									deltaT = 0.0;
         
									if ((noIterator > maxIterations) || (FP_LTE(areaFactor, 1.0)))
									{
 							   fc->error = ERR_CALC_FLAG;
											break;
									}
						   /* Fixed ramp calculations */
         areaLobe1      = -areaFactor * (slice->m0ref);
									
									if ( FP_GT(areaLobe1, (fc->maxGrad*risetime)) )
									{
   									fc->duration1 = (areaLobe1 / fc->maxGrad ) + risetime;
								
	   								/* granulate duration */
				   					fc->duration1 = granularity(fc->duration1, fc->resolution);
									}
         else
									{
									   fc->duration1 = 2.0 * risetime;
									}   
									
									fc->amplitude1 = areaLobe1 / (fc->duration1 - risetime);
         momentLobe1 = areaLobe1 * (fc->duration1 / 2.0);
									momentLobe2 = -slice->m1ref - momentLobe1;
									areaLobe2 = -slice->m0ref - areaLobe1;
									deltaT = momentLobe2 / areaLobe2;
									fc->amplitude2 = areaLobe2 / risetime;
									
									if (FP_GT(fc->amplitude2, fc->maxGrad) )
									{
							   		fc->amplitude2 = fc->maxGrad;
									   fc->duration2 = (areaLobe2 / fc->amplitude2) + risetime;
									}
									else
									{
									   fc->duration2 = 2 * risetime;
									}

								/* time between gradients */	
        fc->separation = deltaT - fc->duration1 - (fc->duration2/2.0);									
   					fc->separation = granularity(fc->separation, fc->resolution);
        areaFactor = areaFactor - areaFactorDec;
      }   
						while (FP_LT(fc->separation, 0));
				
						if (fc->error) 
									{
								   fc->error = ERR_FLOWCOMP_CALC;
           displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
           return;
									}

						/* Assign calculated values */		
						fc->rampTime1 = risetime;		
						fc->rampTime2 = risetime;		

						/* create 1st lobe waveform */
						fc->lobe1.calcFlag   = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
   			fc->lobe1.resolution = fc->resolution;
   			fc->lobe1.amp        = fc->amplitude1;
   			fc->lobe1.duration   = fc->duration1;
   			fc->lobe1.polarity   = TRUE;
						fc->lobe1.tramp      = fc->rampTime1;
   			strcpy(fc->lobe1.name, "lobe1_");

   			/* suppress display for high level function */
   			if (sgldisplay == DISPLAY_TOP_LEVEL) 
      			fc->lobe1.display = DISPLAY_NONE;

   			calcGeneric(&fc->lobe1); 

   			/* check for errors */
   			if (fc->lobe1.error != ERR_NO_ERROR)
   			{
      			fc->error = fc->lobe1.error;
      			return;
   			}

      if (FP_GT(fc->separation,0))
						{
      			/* create gradient separation waveform */
      			fc->plat.startAmplitude = 0.0;
      			fc->plat.endAmplitude   = 0.0;
   						fc->plat.duration       = fc->separation;
									fc->plat.resolution     = fc->resolution;
      			fc->plat.numPoints      = (long)ROUND(fc->plat.duration / fc->plat.resolution);

      			calcPrimitive(&fc->plat);

   						/* check for errors */
      			if (fc->plat.error != ERR_NO_ERROR)
      			{
         			fc->error = fc->plat.error;
						   			fc->error = ERR_FLOWCOMP_GARDIENT_SEPARATION;
         			displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
         			return;
      			}
      }
  
     	/* create 2nd lobe waveform */
	  			fc->lobe2.calcFlag   = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
   			fc->lobe2.resolution = fc->resolution;
   			fc->lobe2.amp        = fc->amplitude2;
   			fc->lobe2.duration   = fc->duration2;
   			fc->lobe2.polarity   = TRUE;   
						fc->lobe2.tramp      = fc->rampTime2;
   			strcpy(fc->lobe2.name, "lobe2_");

   			/* suppress display for high level function */
   			if (sgldisplay == DISPLAY_TOP_LEVEL) 
      			fc->lobe2.display = DISPLAY_NONE;

   			calcGeneric(&fc->lobe2);
						

   			/* merge lobe1 plateau */
   			concatenateGradients(fc->lobe1.dataPoints, fc->lobe1.numPoints,
                        			fc->plat.dataPoints, fc->plat.numPoints,
                        			&mg1);

						/* merge second lobe */
      concatenateGradients(mg1.dataPoints, mg1.numPoints,
                           fc->lobe2.dataPoints, fc->lobe2.numPoints,
                           &mg2);

   			/* copy data pointer and number of points */
   			fc->dataPoints = mg2.dataPoints;
   			fc->numPoints  = mg2.numPoints;

   			/* check slew rate of concatenated points */
   			if((fc->error = checkSlewRate(fc->dataPoints,
                                 			fc->numPoints,
                                 			0,
                                 			fc->resolution)) != ERR_NO_ERROR)
   			{
      			/* zero out the array */
      			zeroData(fc->dataPoints, fc->numPoints);
      			displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
      			return;
   			}
							/* END OF FIXED RAMP */	
			}				
   else
			{
   			/* calculate 2nd flowcomp lobe */
   			temp1     = rampArea * area;
   			temp2     = 2 * fc->maxGrad * fabs(slice->m1ref);
   			temp3     = 2 * (temp1 + areaSquare + temp2);
   			temp4     = sqrt(rampAreaSquared + temp3);
   			areaLobe2 = (-rampArea + temp4) / 2;


   			if (FP_LT(areaLobe2, rampArea))
   			{
			   			/* triangular 2nd lobe */
      			fc->lobe2.shape = TRIANGULAR;
      			plateau2  = 0;
      			temp1     = 2 * fabs(slice->m1ref) * fc->maxGrad;
      			temp2     = area * (area + rampArea);
      			temp3     = sqrt(temp2 + temp1);
      			temp4     = rampArea + (2 * temp3);
      			temp5     = sqrt(1 + (4 / rampArea) * temp3);
      			areaLobe2 = (temp4 - rampArea * temp5) / 2;

      			/* enforce ganularity */
      			fc->duration2 = 2 * sqrt(areaLobe2 * risetime / fc->maxGrad);
      			fc->rampTime2 = fc->duration2 / 2;

      			/* enforce granularity */
      			fc->rampTime2 = granularity(fc->rampTime2, fc->resolution);

      			/* recalculate values */
      			fc->duration2 = 2 * fc->rampTime2;
      			fc->amplitude2 = areaLobe2 / fc->rampTime2;

									if (FP_GT(fc->amplitude2, fc->maxGrad))
      			{
         			fc->error = ERR_AMPLITUDE;
         			displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
         			return;
      			}

   			}
   			else
   			{
      			/* trapezoidal 2nd lobe */
									fc->lobe2.shape = TRAPEZOID;
      			fc->duration2 = (areaLobe2 / fc->maxGrad) + risetime;
      			plateau2 = fc->duration2 - (2 * risetime);

      			/* enforce ganularity */
      			plateau2 = granularity(plateau2, fc->resolution); /* enforce granularity */

      			/* recalculate values */
      			fc->duration2  = (2 * risetime) + plateau2; /* new duration */
      			fc->amplitude2 = areaLobe2 / (fc->duration2 - risetime);
      			fc->rampTime2  = risetime;
   			}

   			/* calculate 1st flowcomp lobe */
   			areaLobe1 = slice->m0ref + areaLobe2;

   			if (FP_LT(areaLobe1, rampArea))
   			{
      			fc->lobe1.shape = TRIANGULAR;
      			plateau1        = 0;

      			/* triangular 1st lobe */
      			fc->duration1   = 2 * sqrt(areaLobe1 * risetime / fc->maxGrad);
      			fc->rampTime1   = fc->duration1 / 2;

      			/* enforce granularity */
      			fc->rampTime1 = granularity(fc->rampTime1, fc->resolution);

      			/* recalculate values */
      			fc->duration1  = 2 * fc->rampTime1;
      			fc->amplitude1 = areaLobe1 / fc->rampTime1;

      			if (FP_GT(fc->amplitude1, fc->maxGrad))
      			{
         			fc->error = ERR_AMPLITUDE;
         			displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
         			return;
      			}
   			}
   			else
   			{
      			/* trapezoidal 1st lobe */
									fc->lobe1.shape = TRAPEZOID;
      			fc->duration1 = (areaLobe1 / fc->maxGrad) + risetime;
      			plateau1 = fc->duration1 - (2 * risetime);

      			/* enforce ganularity */
      			plateau1 = granularity(plateau1, fc->resolution); /* enforce granularity */

      			/* recalculate values */
      			fc->duration1  = (2 * risetime) + plateau1; /* new duration */
      			fc->amplitude1 = areaLobe1 / (fc->duration1 - risetime);
      			fc->rampTime1  = risetime;
   			}

   			/* Check for error */
   			if (fc->error)
   			{
      			return;
   			}

   			fc->amplitude1 *= -1;

   			/* create waveform */
						fc->lobe1.calcFlag   = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
   			fc->lobe1.resolution = fc->resolution;
   			fc->lobe1.amp        = fc->amplitude1;
   			fc->lobe1.duration   = fc->duration1;
   			fc->lobe1.polarity   = TRUE;
						fc->lobe1.tramp      = fc->rampTime1;
   			strcpy(fc->lobe1.name, "lobe1_");

   			/* suppress display for high level function */
   			if (sgldisplay == DISPLAY_TOP_LEVEL) 
      			fc->lobe1.display = DISPLAY_NONE;

   			calcGeneric(&fc->lobe1);

   			/* check for errors */
   			if (fc->lobe1.error != ERR_NO_ERROR)
   			{
      			fc->error = fc->lobe1.error;
      			return;
   			}
						fc->lobe2.calcFlag   = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
   			fc->lobe2.resolution = fc->resolution;
   			fc->lobe2.amp        = fc->amplitude2;
   			fc->lobe2.duration   = fc->duration2;
   			fc->lobe2.polarity   = TRUE;   
						fc->lobe2.tramp      = fc->rampTime2;
   			strcpy(fc->lobe2.name, "lobe2_");

   			/* suppress display for high level function */
   			if (sgldisplay == DISPLAY_TOP_LEVEL) 
      			fc->lobe2.display = DISPLAY_NONE;

   			calcGeneric(&fc->lobe2);

   			/* check for errors */
   			if (fc->lobe2.error != ERR_NO_ERROR)
   			{
      			fc->error = fc->lobe2.error;
      			return;
   			}

   			/* merge lobe1 and lobe2 */
   			concatenateGradients(fc->lobe1.dataPoints, fc->lobe1.numPoints,
                        			fc->lobe2.dataPoints, fc->lobe2.numPoints,
                        			&mg1);

   			/* copy data pointer and number of points */
   			fc->dataPoints = mg1.dataPoints;
   			fc->numPoints  = mg1.numPoints;

   }   /* End of flowcop calculation */


   /* This section is common to both calculations */
			/* for fixed ramp and slew rate limited lobes */ 
			
   /* check slew rate of concatenated points */
   if((fc->error = checkSlewRate(fc->dataPoints,
                                 fc->numPoints,
                                 0,
                                 fc->resolution)) != ERR_NO_ERROR)
   {
      /* zero out the array */
      zeroData(fc->dataPoints, fc->numPoints);

      displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* write gradient waveform to disk if required */
   if(fc->writeToDisk)
   {
      if(!gradShapeWritten(fc->name, gParamStr, tempname))
	     {
      	  (void)strcpy(fc->name, tempname); /* save file name */
      	  fc->error = writeToDisk(fc->dataPoints, fc->numPoints, 0, fc->resolution,
				     fc->rollOut, fc->name);
     	}
   else
    	 (void)strcpy(fc->name, tempname); /* save file name */
   }

   fc->duration = fc->numPoints * fc->resolution;

   fc->amp = fabs( MAX(fabs(fc->amplitude1), fabs(fc->amplitude2)) );

   /* do moment calculations for whole generic */
   calculateMoments(fc->dataPoints, fc->numPoints,
                    1,                  /* start index */
                    0, /* start amplitude */
                    fc->resolution, &fc->m0,
                    &fc->m1,
                    0,  /* start delta offset */
                    0); /* end delta offset */
   
   /* display structure for debugging */
   if ((sgldisplay == DISPLAY_TOP_LEVEL) ||(sgldisplay ==DISPLAY_MID_LEVEL) ||
       (sgldisplay == DISPLAY_LOW_LEVEL)) displaySliceFlowcomp(fc);
}

/***********************************************************************
*  Function Name: calcReadoutFlowcomp
*  Example:    calcReadoutFlowcomp(&flowcompReadout, &readout);
*  Purpose:    Creates the readout flow compensation gradient
               waveform.
*  Input
*     Formal:  *flowcomp - readout flow compensation gradient structure
*              *readout  - readout gradient structure
*     Private: none
*     Public:  T_RISE    - time to get from zero to GRAD_MAX 
*              GRAD_MAX  - maximum possible gradient strength
*  Output
*     Return:  none
*     Formal:  *flowcomp - readout flow compensation gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calcReadoutFlowcomp(FLOWCOMP_T *fc, READOUT_GRADIENT_T *readout)
{
   double risetime;        /* gradient rise time [ms] */
   double rampArea;        /* area under ramp */
   double area;            /* area of readout - 0th moment */
   double areaSquare;      /* squared readout area */
   double rampAreaSquared; /* squared ramp area */
   double temp1, temp2;    /* variables holding intermediate results */
   double temp3, temp4;    /* variables holding intermediate results */
   double temp5;           /* variables holding intermediate results */
   double areaLobe1;       /* area of 1st compensation lobe */
   double areaLobe2;       /* area of 2nd compensation lobe */
   double plateau1;        /* duration of gradient plateau 1st compensation lobe */
   double plateau2;        /* duration of gradient plateau 2nd compensation lobe */
   double areaFactor;      /* Area factor for fixed ramp calculation (arbitrary pick) */
			double areaFactorDec;   /* Area factor decrement for iteration */
   double momentLobe1;     /* first moment of lobe 1 */
			double momentLobe2;     /* first moment of lobe2 */
			int    noIterator;      /* Number of iterations */
			int    maxIterations;   /* Max number of iterations */
   double deltaT;          /* time shift for 2nd lobe  */
   double slew;            /* gradient slew */
   MERGE_GRADIENT_T mg1;
   MERGE_GRADIENT_T mg2;
//   char grad_params[MAX_STR];
	char *gParamStr;
   char tempname[MAX_STR];
//   char tempstr[MAX_STR];

   /* initialize structures */
   initGeneric(&fc->lobe1);
			initPrimitive(&fc->plat); 
   initGeneric(&fc->lobe2);

   /* If constant ramp time is required */

 		if ( (FP_GT(trampfixed, 0.0)) || 
 			    (fc->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP))
			{ 
				   risetime = granularity(fc->rampTime1, fc->resolution);
			}	
   else
			{
			   risetime = granularity(T_RISE, fc->resolution);
			}

   /* initialize variables */
 //  risetime        = granularity(T_RISE, fc->resolution);
   rampArea        = risetime * fc->maxGrad;
   area            = fabs(readout->m0ref);
   areaSquare      = area * area;
   rampAreaSquared = rampArea * rampArea;
   slew            = risetime / GRAD_MAX;
   areaLobe1       = 0.0;
   areaLobe2       = 0.0;
			momentLobe1     = 0.0;
			momentLobe2     = 0.0;
   temp1           = 0.0;
   temp2           = 0.0;
   temp3           = 0.0;
			deltaT          = 0.0;  
   areaFactor	     = 1.5;
			areaFactorDec	  = 0.01;
			noIterator      = 0;
			maxIterations   = 50;

	gParamStr = NULL;
   /* save  features for later comparison */
	appendFormattedString( &gParamStr, "%f ",readout->m0ref );
	appendFormattedString( &gParamStr, "%f ",readout->m1ref );
	appendFormattedString( &gParamStr, "%f ",fc->maxGrad );
	appendFormattedString( &gParamStr, "%f ",fc->resolution );

   if ( (FP_GT(trampfixed, 0.0)) ||
		      (fc->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP) )
		 {

      do
						{

						   noIterator = noIterator +1;
									deltaT = 0.0;

									if ((noIterator > maxIterations) || (FP_LTE(areaFactor, 1.0)))
									{
 							   fc->error = ERR_CALC_FLAG;
											break;
									}
						   /* Fixed ramp calculations */
         areaLobe1      = areaFactor * (readout->m0ref);

									if ( FP_GT(areaLobe1, (fc->maxGrad*risetime)) )
									{
   									fc->duration1 = (areaLobe1 / fc->maxGrad ) + risetime;

	   								/* granulate duration */
				   					fc->duration1 = granularity(fc->duration1, fc->resolution);
									}
         else
									{
									   fc->duration1 = 2.0 * risetime;
									}   
									/* first lobe is always negative (counter from readout towards RF)*/
									areaLobe1 = -areaLobe1;
									
									fc->amplitude1 = areaLobe1 / (fc->duration1 - risetime);
									
									momentLobe1 = areaLobe1 * (fc->duration1 / 2.0);
		
									momentLobe2 = readout->m1ref - momentLobe1;
		
									areaLobe2 = -areaLobe1 - readout->m0ref;
										
									deltaT = momentLobe2 / areaLobe2;
										
									fc->amplitude2 = areaLobe2 / risetime;
									
									if (FP_GT(fc->amplitude2, fc->maxGrad) )
									{
							   		fc->amplitude2 = fc->maxGrad;
									   fc->duration2 = (areaLobe2 / fc->amplitude2) + risetime;
									}
									else
									{
									   fc->duration2 = 2 * risetime;
									}

								/* time between gradients */	
        fc->separation = deltaT - fc->duration1 - (fc->duration2/2.0);									
   					fc->separation = granularity(fc->separation, fc->resolution);
        areaFactor = areaFactor - areaFactorDec;
      }   
						while (FP_LT(fc->separation, 0));
				
						if (fc->error) 
									{
								   fc->error = ERR_FLOWCOMP_CALC;
           displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
           return;
									}

						/* Assign calculated values */		
						fc->rampTime1 = risetime;		
						fc->rampTime2 = risetime;		

						/* create 1st lobe waveform */
						fc->lobe1.calcFlag   = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
   			fc->lobe1.resolution = fc->resolution;
   			fc->lobe1.amp        = fc->amplitude1;
   			fc->lobe1.duration   = fc->duration1;
   			fc->lobe1.polarity   = TRUE;
						fc->lobe1.tramp      = fc->rampTime1;
   			strcpy(fc->lobe1.name, "lobe1_");

   			/* suppress display for high level function */
   			if (sgldisplay == DISPLAY_TOP_LEVEL) 
      			fc->lobe1.display = DISPLAY_NONE;

   			calcGeneric(&fc->lobe1); 

   			/* check for errors */
   			if (fc->lobe1.error != ERR_NO_ERROR)
   			{
      			fc->error = fc->lobe1.error;
      			return;
   			}

      if (FP_GT(fc->separation,0))
						{
      			/* create gradient separation waveform */
      			fc->plat.startAmplitude = 0.0;
      			fc->plat.endAmplitude   = 0.0;
   						fc->plat.duration       = fc->separation;
									fc->plat.resolution     = fc->resolution;
      			fc->plat.numPoints      = (long)ROUND(fc->plat.duration / fc->plat.resolution);

      			calcPrimitive(&fc->plat);

   						/* check for errors */
      			if (fc->plat.error != ERR_NO_ERROR)
      			{
         			fc->error = fc->plat.error;
						   			fc->error = ERR_FLOWCOMP_GARDIENT_SEPARATION;
         			displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
         			return;
      			}
      }
  
     	/* create 2nd lobe waveform */
	  			fc->lobe2.calcFlag   = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
   			fc->lobe2.resolution = fc->resolution;
   			fc->lobe2.amp        = fc->amplitude2;
   			fc->lobe2.duration   = fc->duration2;
   			fc->lobe2.polarity   = TRUE;   
						fc->lobe2.tramp      = fc->rampTime2;
   			strcpy(fc->lobe2.name, "lobe2_");

   			/* suppress display for high level function */
   			if (sgldisplay == DISPLAY_TOP_LEVEL) 
      			fc->lobe2.display = DISPLAY_NONE;

   			calcGeneric(&fc->lobe2);
						

   			/* merge lobe1 plateau */
   			concatenateGradients(fc->lobe2.dataPoints, fc->lobe2.numPoints,
                        			fc->plat.dataPoints, fc->plat.numPoints,
                        			&mg1);

						/* merge second lobe */
      concatenateGradients(mg1.dataPoints, mg1.numPoints,
                           fc->lobe1.dataPoints, fc->lobe1.numPoints,
                           &mg2);

   			/* copy data pointer and number of points */
   			fc->dataPoints = mg2.dataPoints;
   			fc->numPoints  = mg2.numPoints;

   			/* check slew rate of concatenated points */
   			if((fc->error = checkSlewRate(fc->dataPoints,
                                 			fc->numPoints,
                                 			0,
                                 			fc->resolution)) != ERR_NO_ERROR)
   			{
      			/* zero out the array */
      			zeroData(fc->dataPoints, fc->numPoints);
      			displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
      			return;
   			}
							/* END OF FIXED RAMP */	
			}				
   else
			{
   			/* calculate 2nd flowcomp lobe */
   			temp1     = rampArea * area;
   			temp2     = 2 * fc->maxGrad * fabs(readout->m1ref);
   			temp3     = 2 * (temp1 + areaSquare + temp2);
   			temp4     = sqrt(rampAreaSquared + temp3);

   			areaLobe2 = (-rampArea + temp4) / 2;

   			if (FP_LT(areaLobe2, rampArea))
   			{
      			/* triangular 2nd lobe */
      			fc->lobe2.shape = TRIANGULAR;
      			plateau2  = 0;
      			temp1     = 2 * fabs(readout->m1ref) * fc->maxGrad;
      			temp2     = area * (area + rampArea);
      			temp3     = sqrt(temp2 + temp1);
      			temp4     = rampArea + (2 * temp3);
      			temp5     = sqrt(1 + (4 / rampArea) * temp3);
      			areaLobe2 = (temp4 - rampArea * temp5) / 2;

      			/* enforce ganularity */
      			fc->duration2 = 2.0 *sqrt(areaLobe2 * risetime / fc->maxGrad);
      			fc->rampTime2 = fc->duration2 / 2;

      			/* enforce granularity */
      			fc->rampTime2 = granularity(fc->rampTime2, fc->resolution);

      			/* recalculate values */
      			fc->duration2  = 2 * fc->rampTime2;
      			fc->amplitude2 = areaLobe2 / fc->rampTime2;
      			if (FP_GT(fc->amplitude2, fc->maxGrad))
      			{
         			fc->error = ERR_AMPLITUDE;
         			displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
         			return;
      			}
   			}
   			else
   			{
      			/* trapezoidal 2nd lobe */
									fc->lobe2.shape = TRAPEZOID;
      			fc->duration2 = (areaLobe2 / fc->maxGrad) + risetime;
      			plateau2 = fc->duration2 - (2 * risetime);

      			/* enforce ganularity */
      			plateau2 = granularity(plateau2, fc->resolution); /* enforce granularity */

      			/* recalculate values */
      			fc->duration2  = (2 * risetime) + plateau2; /* new duration */
      			fc->amplitude2 = areaLobe2 / (fc->duration2 - risetime);
      			fc->rampTime2  = risetime;
   			}

   			/* calculate 1st flowcomp lobe */
   			areaLobe1 = readout->m0ref + areaLobe2;

   			if (FP_LT(areaLobe1, rampArea))
   			{
      			/* triangular 1st lobe */
				  			fc->lobe1.shape = TRIANGULAR;
      			plateau1        = 0;
      			fc->duration1   = 2 * sqrt(areaLobe1 * risetime / fc->maxGrad);
      			fc->rampTime1   = fc->duration1 / 2;

      			/* enforce granularity */
      			fc->rampTime1 = granularity(fc->rampTime1, fc->resolution);

      			/* recalculate values */
      			fc->duration1  = 2 * fc->rampTime1;
      			fc->amplitude1 = areaLobe1 / fc->rampTime1;

      			if (FP_GT(fc->amplitude1, fc->maxGrad))
      			{
         			fc->error = ERR_AMPLITUDE;
         			displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
         			return;
      			}
   			}
   			else
   			{
      			/* trapezoidal 1st lobe */
									fc->lobe1.shape = TRAPEZOID;
      			fc->duration1 = (areaLobe1 / fc->maxGrad) + risetime;
      			plateau1 = fc->duration1 - (2 * risetime);

      			/* enforce ganularity */
      			plateau1 = granularity(plateau1, fc->resolution); /* enforce granularity */

      			/* recalculate values */
      			fc->duration1  = (2 * risetime) + plateau1; /* new duration */
      			fc->amplitude1 = areaLobe1 / (fc->duration1 - risetime);
      			fc->rampTime1  = risetime;
    			}

   			/* Check for error */
   			if (fc->error)
   			{
      			return;
   			}

   			fc->amplitude1 *= -1;


   			/* create waveform */
						fc->lobe1.calcFlag   = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
   			fc->lobe1.resolution = fc->resolution;
   			fc->lobe1.amp        = fc->amplitude1;
   			fc->lobe1.duration   = fc->duration1;
   			fc->lobe1.polarity   = TRUE;
						fc->lobe1.tramp      = fc->rampTime1;
   			strcpy(fc->lobe1.name, "lobe1_");

   			/* suppress display for high level function */
   			if (sgldisplay == DISPLAY_TOP_LEVEL) 
      			fc->lobe1.display = DISPLAY_NONE;

   			calcGeneric(&fc->lobe1);

   			/* check for errors */
   			if (fc->lobe1.error != ERR_NO_ERROR)
   			{
      			fc->error = fc->lobe1.error;
      			return;
   			}
   			fc->lobe2.calcFlag   = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
   			fc->lobe2.resolution = fc->resolution;
   			fc->lobe2.amp        = fc->amplitude2;
   			fc->lobe2.duration   = fc->duration2;
   			fc->lobe2.polarity   = TRUE;
						fc->lobe2.tramp      = fc->rampTime2;
   			strcpy(fc->lobe2.name, "lobe2_");

   			/* suppress display for high level function */
   			if (sgldisplay == DISPLAY_TOP_LEVEL) 
      			fc->lobe2.display = DISPLAY_NONE;

   			calcGeneric(&fc->lobe2);

   			/* check for errors */
   			if (fc->lobe2.error != ERR_NO_ERROR)
   			{
      			fc->error = fc->lobe2.error;
      			return;
   			}

   			/* merge lobe1 and lobe2 */
   			concatenateGradients(fc->lobe2.dataPoints, fc->lobe2.numPoints,
                        			fc->lobe1.dataPoints, fc->lobe1.numPoints,
                        			&mg1);

   			/* copy data pointer and number of points */
   			fc->dataPoints = mg1.dataPoints;
   			fc->numPoints  = mg1.numPoints;
   }   /* End of flowcop calculation */


   /* This section is common to both calculations */
			/* for fixed ramp and slew rate limited lobes */ 


   /* check slew rate of concatenated points */
   if((fc->error = checkSlewRate(fc->dataPoints,
                                 fc->numPoints,
                                 0,
                                 fc->resolution)) != ERR_NO_ERROR)
   {
      /* zero out the array */
      zeroData(fc->dataPoints, fc->numPoints);

      displayError(fc->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* write gradient waveform to disk if required */
   if (fc->writeToDisk)
      {
      if(!gradShapeWritten(fc->name, gParamStr, tempname))
	        {
   	    (void)strcpy(fc->name, tempname); /* save file name */
	       fc->error = writeToDisk(fc->dataPoints, fc->numPoints, 0, fc->resolution,
				    fc->rollOut, fc->name);
         }
      else
      	 (void)strcpy(fc->name, tempname); /* save file name */
     }

   fc->duration = fc->numPoints * fc->resolution;
   
   fc->amp = fabs( MAX(fabs(fc->amplitude1), fabs(fc->amplitude2)) );

   /* do moment calculations for whole generic */
   calculateMoments(fc->dataPoints, fc->numPoints,
                    1,                  /* start index */
                    0, /* start amplitude */
                    fc->resolution, &fc->m0,
                    &fc->m1,
                    0,  /* start delta offset */
                    0); /* end delta offset */

   /* calculate 1st moment for readout flowcomp*/
			/* This is required since we can not calculate the 1st moment backwards time reversed */
			fc->m1 = fc->lobe1.m1 + ( areaLobe2 * (fc->separation + fc->lobe1.duration + (fc->lobe2.duration/2.0)) );


   /* display structure for debugging */
   if ((sgldisplay == DISPLAY_TOP_LEVEL) ||(sgldisplay ==DISPLAY_MID_LEVEL) ||
       (sgldisplay == DISPLAY_LOW_LEVEL)) displayReadoutFlowcomp(fc);
}

/***********************************************************************
*  Function Name: mergeGradient
*  Example:    mergeGradient(&input1, &input2, &output, error);
*  Purpose:    Concatenates two arbitrary gradient waveforms.
*  Input
*     Formal:  *waveform1 - 1st file name
*              *waveform2 - 2nd file name
*     Private: none
*     Public:  none
*  Output
*     Return:  number of points in new waveform file, unless error
*     Formal:  *waveformOut - output file name.  If an empty pointer is
*                             passed in the output file name will be
*                             assigned by function.
*              error        - error flag
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
int mergeGradient (char *waveform1, char *waveform2,
                   char *waveformOut, ERROR_NUM_T error)
{
   char   path[MAX_STR], wavefile1[MAX_STR], wavefile2[MAX_STR];
   char   defaultName[MAX_STR];
   char   start[MAX_STR];
   char   inString1[MAX_STR], inString2[MAX_STR];
   long   dacValue;
   long   counter;
   long   points1, points2; /* points from header, calc points*/
   double scale;
			
   GRADIENT_HEADER_T header1;
   GRADIENT_HEADER_T header2;
   GRADIENT_HEADER_T header3;
   FILE   *fpin1;
   FILE   *fpin2;
   FILE   *fpout;

   /* initialize variables */
   counter = 1; /* conter variable */
   points1 = 0; /* points for merged waveform from headers*/
   points2 = 0; /* points for merged waveform ticks*/
   error   = ERR_NO_ERROR; /* error flag */

   /* default output file name - number will be added later */
   strcpy(defaultName, "composite_grad_");

   strcpy(start, "#:");
   scale = 1; /* waveform scaling factor */

   /* initialize gradient header structure for first file */
   initGradientHeader(&header1);

   /* initialize gradient header structure for second file */
   initGradientHeader(&header2);

   /* initialize gradient header structure for second file */
   initGradientHeader(&header3);

   /* check arguments */
   if (!strcmp(waveformOut, ""))
   {
      strcpy(waveformOut, defaultName);
   }

   /* check if file exists */
   strcpy(wavefile1, USER_DIR);      /* add user directory to path */
   strcat(wavefile1, SHAPE_LIB_DIR); /* add shapelib directory to path */
   strcat(wavefile1, waveform1);     /* add path and waveform name */
   strcat(wavefile1, ".GRD");        /* add suffix to complete path */

   /* check if input file exists */
   if ((fpin1 = fopen(wavefile1, "r")) == NULL)
   {
      error = ERR_FILE_1;
      displayError(error, __FILE__, __FUNCTION__, __LINE__);
      return 0;
   }
   
   readGradientHeader(&header1, fpin1);
   points1 = header1.points;
   fclose(fpin1);

   strcpy(wavefile2, USER_DIR);      /* add user directory to path*/
   strcat(wavefile2, SHAPE_LIB_DIR); /* add shapelib directory to path */
   strcat(wavefile2, waveform2);     /* add path and waveform name */
   strcat(wavefile2, ".GRD");        /* add suffix to complete path */

   /* check if input file exists */
   if ((fpin2 = fopen(wavefile2, "r")) == NULL)
   {
      error = ERR_FILE_2;
      displayError(error, __FILE__, __FUNCTION__, __LINE__);
     return 0;
   }
   readGradientHeader(&header2, fpin2);
   fclose(fpin2);
   points1 = points1 + header2.points;
   /* check file parameters */
   if (FP_NEQ(header1.resolution, header2.resolution))
   {
      error = ERR_RESOLUTION;
      displayError(error, __FILE__, __FUNCTION__, __LINE__);
      return 0;
   }

   /* open output waveform file */
   checkForFilename(waveformOut, path);

   /* open new file for write operation */
   /* open the file for read/write operations */
   if ((fpout = fopen(path, "w+")) == NULL)
   {
      error = ERR_FILE_OPEN;
      displayError(error, __FILE__, __FUNCTION__, __LINE__);
      return 0;
   }

   /* Setup header for new file */
   strcpy(header3.name, waveformOut);
   header3.points = points1;
   header3.resolution = header1.resolution;
   header3.strength = MAX(fabs(header1.strength), fabs(header2.strength));
   writeGradientHeader(&header3, fpout);

   /* copy first file and write into new file */
   if ((fpin1 = fopen(wavefile1, "r")) == NULL) /* open first file */
   {
      error = ERR_FILE_OPEN;
      displayError(error, __FILE__, __FUNCTION__, __LINE__);
      return 0;
   }
   scale = header1.strength / header3.strength;

   /* scaling factor to normalize gradient scale */
   while (!strstr(inString1, start))
   {
      fscanf(fpin1, "%s", inString1);
   }
   while (!feof(fpin1))
   {
      /* read waveform values */
      fscanf(fpin1, "%s %s", inString1, inString2);

      if (!feof(fpin1))
      {
         /* normalize waveform values */
         dacValue = ROUND(atoi(inString1) * scale);
         if (abs(dacValue) > MAX_DAC_NUM)
         {
            error = ERR_MERGE_SCALE;
            displayError(error, __FILE__, __FUNCTION__, __LINE__);
            return 0;
         }
         /* write new waveform values */
         fprintf(fpout, "%li\t%s\n", dacValue, inString2);

         points2 = points2 + atoi(inString2);
      }
   }
   fclose(fpin1); /* close first input file */
   /* copy second file and write into new file */
   if ((fpin2 = fopen(wavefile2, "r")) == NULL) /* open second file */
   {
      error = ERR_FILE_OPEN;
      displayError(error, __FILE__, __FUNCTION__, __LINE__);
      return 0;
   }

   /* scaling factor to normalize gradient scale */
   scale = header2.strength / header3.strength;

   while (!strstr(inString1, start))
   {
      fscanf(fpin1, "%s", inString1);
   }
   while (!feof(fpin2))
   {
      fscanf(fpin2, "%s %s", inString1, inString2);
      if ( !feof(fpin2) )
      {
         /* normalize waveform values */
         dacValue = ROUND(atoi(inString1) * scale);
         if (abs(dacValue) > MAX_DAC_NUM)
         {
            error = ERR_MERGE_SCALE;
            displayError(error, __FILE__, __FUNCTION__, __LINE__);
            return 0;
         }

         /* write new waveform values */
         fprintf(fpout, "%li\t%s\n", dacValue, inString2);

         points2 += atoi(inString2);
      }
   }
   fclose(fpin1); /* close first input file */   
   fclose(fpin2); /* close second input file */
   fclose(fpout);  /* close output file */

   /* add new file name for gradient shape to list */
   (void)gradAdmin(path, ADD);

   if (points1 != points2)
   {
      error = ERR_MERGE_POINTS;
      displayError(error, __FILE__, __FUNCTION__, __LINE__);
      return 0;
   }

   return(points2); /* return number of points in new waveform file */
}

/***********************************************************************
*  Function Name: zeroFillGradient
*  Example:    zeroFillGradient(&zeroFillStructure);
*  Purpose:    Extends / pads a gradient waveform resident in memory
*              with zeros.  Can add zeros to front, back, both front and
*              back (symmetric about the center) locations of waveform.
*  Input
*     Formal:  zg - zero filling gradient structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  zg - zero filling gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void zeroFillGradient (ZERO_FILL_GRADIENT_T * zg)
{
   long newPoints = 0;
   long i         = 0;
   long k         = 0;
   long numBytes;
   double * dp;
   double * dataPointsOut;
//   char grad_params[MAX_STR];
	char *gParamStr;
   char tempname[MAX_STR];
//   char tempstr[MAX_STR];


   /* save  features for later comparison */

	gParamStr = NULL;

	appendFormattedString( &gParamStr, "%f ",zg->newDuration );
	appendFormattedString( &gParamStr, "%f ",zg->timeToPad );
	appendFormattedString( &gParamStr, "%d ",(int)(zg->numPoints) );
	appendFormattedString( &gParamStr, "%f ",zg->resolution );

   /* copy pointer for easier referencing */
   dp = zg->dataPoints;

   /* check input parameters */
   if (FP_NEQ(zg->timeToPad, 0) && FP_NEQ(zg->newDuration, 0))
   {
      /* check that there is consistency when both paramters are supplied */
      if (FP_NEQ(zg->newDuration, (long)ROUND(zg->numPoints * zg->resolution + zg->timeToPad)))
      {
         displayError(ERR_INCONSISTENT_PARAMETERS, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }
   if (FP_NEQ(zg->newDuration, 0) && FP_LT(zg->newDuration, zg->numPoints * zg->resolution))
   {
      /* the newDuration supplied is less than the waveform duration */
      displayError(ERR_ZERO_PAD_DURATION, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(zg->timeToPad, 0))
   {
      /* time to pad cannot be less than zero */
      displayError(ERR_PARAMETER_REQUEST, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* figure out how many points we're adding */
   if (FP_NEQ(zg->timeToPad, 0))
   {
      newPoints = (long)ROUND(zg->timeToPad / zg->resolution);
   }
   else if (FP_NEQ(zg->newDuration, 0))
   {
      newPoints = (long)ROUND((zg->newDuration - zg->numPoints * zg->resolution) / zg->resolution);
   }
   else
   {
      displayError(ERR_PARAMETER_REQUEST, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* figure out how many bytes the total waveform will be */
   numBytes = sizeof(double) * (zg->numPoints + newPoints);

   /* allocate memory for new waveform */
   if ((dataPointsOut = (double *)malloc(numBytes)) == NULL)
   {
      displayError(ERR_MALLOC, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   switch (zg->location)
   {
   case FRONT:
      /* put zeros in front of waveform */
      for (i = 0; i < newPoints; i++)
      {
         dataPointsOut[i] = 0;
      }
      for (k = 0; k < zg->numPoints; i++, k++)
      {
         dataPointsOut[i] = dp[k];
      }
      break;
   case BACK:
      /* put zeros in back of waveform */
      for (i = 0; i < zg->numPoints; i++)
      {
         dataPointsOut[i] = dp[i];
      }
      for (; i < newPoints + zg->numPoints; i++)
      {
         dataPointsOut[i] = 0;
      }
      break;
   case BOTH:
      /* put zeros in front and back of waveform */
      for (i = 0; i < newPoints / 2; i++)
      {
         dataPointsOut[i] = 0;
      }
      for (k = 0; k < zg->numPoints; i++, k++)
      {
         dataPointsOut[i] = dp[k];
      }
      for (; i < (zg->numPoints - (long)zg->numPoints / 2); i++)
      {
         dataPointsOut[i] = 0;
      }
      break;
   default:
      displayError(ERR_PARAMETER_REQUEST, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* copy the pointer */
   zg->dataPoints = dataPointsOut;

   /* update the number of points */
   zg->numPoints += newPoints;

   /* output new duration of waveform */
   zg->newDuration = zg->numPoints * zg->resolution;

   if (zg->writeToDisk)
   {
     if(!gradShapeWritten(zg->name, gParamStr, tempname))
       {
	 (void)strcpy(zg->name, tempname); /* save file name */
	 zg->error = writeToDisk(zg->dataPoints, zg->numPoints, 0,
				 zg->resolution, zg->rollOut, zg->name);
       }
       else
	 (void)strcpy(zg->name, tempname); /* save file name */
   }

   /* display structure for debugging */
   if (sgldisplay == DISPLAY_LOW_LEVEL) displayZeroFillGradient(zg);

}

/***********************************************************************
*  Function Name: calcMoment0Linear
*  Example:    moment0 = calcMoment0Linear(slew, duration,
*                                          startAmp, endAmp);
*  Purpose:    Calculates the 0th moment between two amplitudes assuming
*              a linear ramp.
*  Input
*     Formal:  slew           - slew rate [[G/cm] / s]
*              duration       - time between amplitudes [s]
*              startAmplitude - starting amplitude [G/cm]
*              endAmplitude   - ending amplitude [G/cm]
*     Private: none
*     Public:  none
*  Output
*     Return:  moment0 - calculated 0th moment [G/cm * s]
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
double calcMoment0Linear(double slew, double tramp,
                         double startAmplitude, double endAmplitude)
{
   /* moment between amplitudes is calculated by the integration of */
   /* y = mx + b */
   return(slew * tramp * tramp / 2 + tramp *
          MIN(startAmplitude, endAmplitude));
}

/***********************************************************************
*  Function Name: adjustCrusherAmplitude
*  Example:    amplitude = adjustCrusherAmplitude(rampZeroToCrusher,
*                                                 plateauTime,
*                                                 rampCrusherToSliceSelect,
*                                                 moment0,
*                                                 ssAmplitude);
*  Purpose:    Re-calculates the amplitude of a crusher gradient when
*              its duration has been slightly increased by the
*              granularity of the DAC.
*  Input
*     Formal:  rampZeroToCrusher - time to ramp from zero to crusher [s]
*              plateauTime       - duration of the crusher plateau [s]
*              rampCrusherToSliceSelect - time to ramp from crusher to
*                                         slice select [s]
*              moment0     - 0th moment of the crusher [s * G/cm]
*              ssAmplitude - slice select gradient strength [G/cm]
*     Private: none
*     Public:  none
*  Output
*     Return:  amplitude - new amplitude of the crusher.  Should be
*                          equal to or less than the orignal amplitude.
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
double adjustCrusherAmplitude(double rampZeroToCrusher, double plateauTime,
                              double rampCrusherToSliceSelect, double moment0,
                              double ssAmplitude)
{
   double rz  = rampZeroToCrusher;
   double pt  = plateauTime;
   double rss = rampCrusherToSliceSelect;
   double mo  = moment0;
   double ss  = ssAmplitude;

  return((2.0 * mo - ss * rss) / (rz + 2.0 * pt + rss));
}

/***********************************************************************
*  Function Name: crusherIdealMoment
*  Example:    moment0 = crusherIdealMoment(&butterfly);
*  Purpose:    Calculates the ideal 0th moment of a butterfly crusher
*              gradient ignoring DAC granularity.
*  Input
*     Formal:  duration    - duration of crusher [s]
*              crAmplitude - crusher amplitude [G / cm]
*              slewRate    - slew rate, represents both ramp up and
*                            ramp down [[G/cm] / s]
*              ssAmplitude - slice select amplitude [G / cm]
*     Private: none
*     Public:  none
*  Output
*     Return:  moment0 - ideal 0th moment
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
double crusherIdealMoment(double duration, double crAmplitude,
                          double slewRate, double ssAmplitude)
{
   double ramp1Time;
   double plateauTime;
   double ramp2Time;

   ramp1Time = fabs(crAmplitude) / slewRate;
   ramp2Time = fabs(crAmplitude - ssAmplitude) / slewRate;
   plateauTime = duration - ramp1Time - ramp2Time;

   return(calcMoment0Linear(slewRate, ramp1Time, 0, crAmplitude) +
          plateauTime * crAmplitude +
          calcMoment0Linear(slewRate, ramp2Time, crAmplitude,
                            ssAmplitude));
}

/***********************************************************************
*  Function Name: durationFromMomentButterfly
*  Example:    durationFromMomentButterfly(&butterflyGrad, 1);
*  Purpose:    Creates a crusher gradient for the calcButterfly
*              function.
*  Input
*     Formal:  *bfly      - butterfly gradient structure
*              crusherNum - which crusher we're generating, either
*                           1 or 2
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *bfly - butterfly gradient structure
*     Private: none
*     Public:  none
*  Notes:      After calculation is complete the ramps and crusher
*              primitive structures will be populated with the values
*              to create the crusher gradient.
***********************************************************************/
void durationFromMomentButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum)
{
   CALC_FLAG_T *calcFlag;
   double *slewRate;
   double *crAmplitude;
   double *crDuration;
   ERROR_NUM_T *crError;
   double *crMoment0;
   double *crResolution;
   double *crTotalDuration;

   double *rampToCrDuration;
   double *rampToSsDuration;

   double momentTemp;
   double tempSlew;
   
   double polarity;

   crError= 0;
   polarity = 1.0;
   
   /* copy pointers */
   if (crusherNum == 1)
   {
      calcFlag        = &bfly->cr1CalcFlag;
      slewRate        = &bfly->cr1.slewRate;
      crAmplitude     = &bfly->cr1amp;
      crDuration      = &bfly->cr1.duration;
      crError         = &bfly->cr1.error;
      crMoment0       = &bfly->cr1Moment0;
      crResolution    = &bfly->cr1.resolution;
      crTotalDuration = &bfly->cr1TotalDuration;
     
      rampToCrDuration = &bfly->ramp1.duration;
      rampToSsDuration = &bfly->ramp2.duration;
      
   }
   else if (crusherNum == 2)
   {
      calcFlag        = &bfly->cr2CalcFlag;
      slewRate        = &bfly->cr2.slewRate;
      crAmplitude     = &bfly->cr2amp;
      crDuration      = &bfly->cr2.duration;
      crError         = &bfly->cr2.error;
      crMoment0       = &bfly->cr2Moment0;
      crResolution    = &bfly->cr2.resolution;
      crTotalDuration = &bfly->cr2TotalDuration;

      rampToCrDuration = &bfly->ramp4.duration;
      rampToSsDuration = &bfly->ramp3.duration;
   }
   else
   {
      *crError = ERR_PARAMETER_REQUEST;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if FP_LT(*crMoment0,0)
   {
      polarity = -1.0;
   }
 
   /* ss amplitude must be supplied or we don't know what to ramp down to */
   if(bfly->ssamp == 0)
   {
      *crError = ERR_EMPTY_SS_AMP;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   if(*calcFlag == SHORTEST_DURATION_FROM_MOMENT &&
      *crMoment0 == 0)
   {
      /* this is a valid case, duration is zero when 0th moment is zero */
      *crDuration = 0;
      return;
   }

   if(*calcFlag == SHORTEST_DURATION_FROM_MOMENT)
   {
      /* test to see if ramp up to maxGrad and down to slice select amplitude */
      /* will exhaust moment0 */
      momentTemp = 0.5 * pow(bfly->maxGrad, 2) / *slewRate +
                   bfly->ssamp * (bfly->maxGrad - bfly->ssamp) / *slewRate +
                   0.5 * pow(bfly->maxGrad - bfly->ssamp, 2) / *slewRate;

      if (FP_GTE(fabs(momentTemp), fabs(*crMoment0)))
      {
         /* we have a triangle, find its amplitude */
         *crAmplitude = sqrt(*slewRate * fabs(*crMoment0) - 0.5 * pow(bfly->ssamp, 2));
         *crDuration = 0;

         /* determine ramp durations */
         *rampToCrDuration = granularity(fabs(*crAmplitude) / *slewRate,
                                         *crResolution);

         *rampToSsDuration = granularity(fabs(*crAmplitude * polarity - bfly->ssamp) /
                                         *slewRate, bfly->resolution);


         /* find crusher total duration */
         *crTotalDuration = *rampToCrDuration + *rampToSsDuration;
	 
	        /* check moment - in some rare instances it can be smaller the m0 */
        momentTemp = ( *crAmplitude * *rampToCrDuration * 0.5) +
	             ( bfly->ssamp * *rampToSsDuration) +
	             ( 0.5 * (*crAmplitude - bfly->ssamp)* *rampToSsDuration);

	       while (FP_LT(momentTemp, *crMoment0))
       	{
            /* if new moment is smaller then passed in moment we need to iterate */
            *rampToSsDuration = *rampToSsDuration + bfly->resolution;
            *rampToCrDuration = *rampToCrDuration + bfly->resolution;
            *crTotalDuration = *rampToCrDuration + *rampToSsDuration;	 

            momentTemp = ( *crAmplitude * *rampToCrDuration * 0.5) +
                         ( bfly->ssamp * *rampToSsDuration) +
	                 ( 0.5 * (*crAmplitude - bfly->ssamp)* *rampToSsDuration);
        } 
	
         /* adjust amplitude due to granularity, preserve 0th moment */
         *crAmplitude = (2 * *crMoment0 - bfly->ssamp * *rampToSsDuration) /
                        (*rampToCrDuration + *rampToSsDuration);
         return;
      }
      else
      {
         /* we have a crusher plateau */
         /* no amplitude supplied, so assume maxGrad */
         *crAmplitude = bfly->maxGrad * polarity;
      }
   }

   /* determine ramp durations */
   *rampToCrDuration = granularity(fabs(*crAmplitude) / *slewRate, *crResolution);
   
   
   *rampToSsDuration = granularity(fabs(*crAmplitude - bfly->ssamp) / *slewRate,
                                   *crResolution);

   /* determine signed slewRate, for ramp to slice select */
   tempSlew = (*crAmplitude <= bfly->ssamp) ? 1 : -1; 
   tempSlew *= *slewRate;

   /* determine plateau duration */
   *crDuration = fabs((*crMoment0 -
                       0.5 * *crAmplitude * *rampToCrDuration -
                       0.5 * (*crAmplitude - bfly->ssamp) * *rampToSsDuration -
                       bfly->ssamp * *rampToSsDuration) /
                       *crAmplitude);


   /* check that plateau duration isn't negative */
   if(FP_LT(*crDuration, 0))
   {
      *crError = ERR_NO_SOLUTION;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* granulate durations */
   *crDuration      = granularity(*crDuration, *crResolution);
   *crTotalDuration = granularity(*rampToCrDuration + *rampToSsDuration + *crDuration,
                                  *crResolution);

   /* check that plateau duration isn't negative */
   if(FP_LT(*crDuration, 0))
   {
      *crError = ERR_DURATION;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* adjust amplitude due to granularity, preserve 0th moment */
   *crAmplitude = adjustCrusherAmplitude(*rampToCrDuration,
                                         *crDuration,
                                         *rampToSsDuration,
                                         *crMoment0,
                                         bfly->ssamp);
}

/***********************************************************************
*  Function Name: momentFromDurationAmplitudeButterfly
*  Example:    momentFromDurationAmplitudeButterfly(&butterflyGrad, 1);
*  Purpose:    Creates a crusher gradient for the calcButterfly
*              function.
*  Input
*     Formal:  *bfly      - butterfly gradient structure
*              crusherNum - which crusher we're generating, either
*                           1 or 2
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *bfly - butterfly gradient structure
*     Private: none
*     Public:  none
*  Notes:      After calculation is complete the ramps and crusher
*              primitive structures will be populated with the values
*              to create the crusher gradient.
***********************************************************************/
void momentFromDurationAmplitudeButterfly(BUTTERFLY_GRADIENT_T *bfly,
                                          int crusherNum)
{
   double *slewRate;
   double *crAmplitude;
   double *crDuration;
   ERROR_NUM_T *crError;
   double *crTotalDuration;
   double *crResolution;

   double *rampToCrDuration;
   double *rampToSsDuration;

   crError=0;
   
   /* copy pointers */
   if (crusherNum == 1)
   {
      slewRate        = &bfly->cr1.slewRate;
      crAmplitude     = &bfly->cr1amp;
      crDuration      = &bfly->cr1.duration;
      crError         = &bfly->cr1.error;
      crResolution    = &bfly->cr1.resolution;
      crTotalDuration = &bfly->cr1TotalDuration;

      rampToCrDuration = &bfly->ramp1.duration;
      rampToSsDuration = &bfly->ramp2.duration;
   }
   else if (crusherNum == 2)
   {
      slewRate        = &bfly->cr2.slewRate;
      crAmplitude     = &bfly->cr2amp;
      crDuration      = &bfly->cr2.duration;
      crError         = &bfly->cr2.error;
      crResolution    = &bfly->cr2.resolution;
      crTotalDuration = &bfly->cr2TotalDuration;
      rampToCrDuration = &bfly->ramp4.duration;
      rampToSsDuration = &bfly->ramp3.duration;
   }
   else
   {
      *crError = ERR_PARAMETER_REQUEST;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   if (FP_NEQ(*crTotalDuration, granularity(*crTotalDuration, *crResolution)))
   {
      /* duration must conform to resolution of system */
      *crError = ERR_GRANULARITY;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   
   if(*crTotalDuration == 0)
   {
      /* this is a valid case, nothing to do but return */
      return;
   }

   if (FP_LT(*crTotalDuration, *crResolution))
   {
      /* in this case duration shouldn't be zero */
      *crError = ERR_DURATION;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   *rampToCrDuration = granularity(fabs(*crAmplitude) / *slewRate, *crResolution);
   *rampToSsDuration = granularity(fabs(*crAmplitude - bfly->ssamp) /
                                   *slewRate, *crResolution);

   *crDuration = *crTotalDuration - *rampToCrDuration - *rampToSsDuration;

   /* granulate duration */
   *crDuration = granularity(*crDuration, *crResolution);

   /* check that duration isn't negative */
   if(FP_LT(*crDuration, 0))
   {
      /* negative duration here means slew rate has been violated */
      *crError = ERR_SLEW_RATE;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
}

/***********************************************************************
*  Function Name: amplitudeFromMomentDurationButterfly
*  Example:    amplitudeFromMomentDurationButterfly(&butterflyGrad, 1);
*  Purpose:    Creates a crusher gradient for the calcButterfly
*              function.
*  Input
*     Formal:  *bfly      - butterfly gradient structure
*              crusherNum - which crusher we're generating, either
*                           1 or 2
*     Private: none
*     Public:  MAX_SLEW_RATE - maximum slew rate of system
*  Output
*     Return:  none
*     Formal:  *bfly - butterfly gradient structure
*     Private: none
*     Public:  none
*  Notes:      After calculation is complete the ramps and crusher
*              primitive structures will be populated with the values
*              to create the crusher gradient.
***********************************************************************/
void amplitudeFromMomentDurationButterfly(BUTTERFLY_GRADIENT_T *bfly,
                                          int crusherNum)
{
   double *slewRate;
   double *crAmplitude;
   double *crDuration;
   ERROR_NUM_T *crError;
   double *crTotalDuration;
   double *crMoment0;
   double *crResolution;

   double *rampToCrDuration;
   double *rampToSsDuration;

   crError=0;

   /* copy pointers */
   if (crusherNum == 1)
   {
      slewRate        = &bfly->cr1.slewRate;
      crAmplitude     = &bfly->cr1amp;
      crDuration      = &bfly->cr1.duration;
      crError         = &bfly->cr1.error;
      crMoment0       = &bfly->cr1Moment0;
      crResolution    = &bfly->cr1.resolution;
      crTotalDuration = &bfly->cr1TotalDuration;

      rampToCrDuration = &bfly->ramp1.duration;
      rampToSsDuration = &bfly->ramp2.duration;
   }
   else if (crusherNum == 2)
   {
      slewRate        = &bfly->cr2.slewRate;
      crAmplitude     = &bfly->cr2amp;
      crDuration      = &bfly->cr2.duration;
      crError         = &bfly->cr2.error;
      crMoment0       = &bfly->cr2Moment0;
      crResolution    = &bfly->cr2.resolution;
      crTotalDuration = &bfly->cr2TotalDuration;

      rampToCrDuration = &bfly->ramp4.duration;
      rampToSsDuration = &bfly->ramp3.duration;
   }
   else
   {
      *crError = ERR_PARAMETER_REQUEST;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* ss amplitude must be supplied or we don't know what to ramp down to */
   if(bfly->ssamp == 0)
   {
      *crError = ERR_EMPTY_SS_AMP;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* Assume the sign of the crusher amplitude is the same as the sign of */
   /* moment0.  The following calculations will only support the case where */
   /* the majority of moment0 is on the side (positive or negative) of where */
   /* the crusher amplitude is. */
   *crAmplitude = (*slewRate * *crMoment0 - 0.5 * pow(bfly->ssamp, 2)) /
                  (*crTotalDuration * *slewRate - bfly->ssamp);

   /* check that the amplitude is in range */
   if (FP_GT(*crAmplitude, bfly->maxGrad))
   {
      *crError = ERR_CR_AMPLITUDE;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* find durations */
   *rampToCrDuration = granularity(fabs(*crAmplitude) / bfly->slewRate,
                                   *crResolution);
   *rampToSsDuration = granularity(fabs(bfly->ssamp - *crAmplitude) / *slewRate,
                                   *crResolution);
   *crDuration = granularity(*crTotalDuration - *rampToCrDuration - *rampToSsDuration,
                             *crResolution);
   
   /* find granulated crusher duration */
   *crTotalDuration = *rampToCrDuration + *crDuration + *rampToSsDuration;
   
   /* adjust amplitude due to granularity, preserve 0th moment */
   *crAmplitude = adjustCrusherAmplitude(*rampToCrDuration,
                                         *crDuration,
                                         *rampToSsDuration,
                                         *crMoment0,
                                         bfly->ssamp);

   /* second iteration */
   /* find durations */
   *rampToCrDuration = granularity(fabs(*crAmplitude) / *slewRate,
                                   *crResolution);
   *rampToSsDuration = granularity(fabs(bfly->ssamp - *crAmplitude) / *slewRate,
                                   *crResolution);
   *crDuration = granularity(*crTotalDuration - *rampToCrDuration - *rampToSsDuration,
                             *crResolution);
   
   /* find granulated crusher duration */
   *crTotalDuration = *rampToCrDuration + *crDuration + *rampToSsDuration;
   
   /* adjust amplitude due to granularity, preserve 0th moment */
   *crAmplitude = adjustCrusherAmplitude(*rampToCrDuration,
                                         *crDuration,
                                         *rampToSsDuration,
                                         *crMoment0,
                                         bfly->ssamp);

   /* check that the amplitude is in range */
   if (FP_GT(*crAmplitude, bfly->maxGrad))
   {
      *crError = ERR_CR_AMPLITUDE;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* check slew rate, ramp from zero to crusher */
   if (FP_GT(fabs(*crAmplitude) / *rampToCrDuration, MAX_SLEW_RATE))
   {
      /* violation of slew rate here means there is no solution */
      *crError = ERR_NO_SOLUTION;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
}

/***********************************************************************
*  Function Name: durationFromMomentRAmpButterfly
*  Example:    durationFromMomentButterfly(&butterflyGrad, 1);
*  Purpose:    Creates a crusher gradient for the calcButterfly
*              function.
*  Input
*     Formal:  *bfly      - butterfly gradient structure
*              crusherNum - which crusher we're generating, either
*                           1 or 2
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *bfly - butterfly gradient structure
*     Private: none
*     Public:  none
*  Notes:      After calculation is complete the ramps and crusher
*              primitive structures will be populated with the values
*              to create the crusher gradient.
***********************************************************************/
void durationFromMomentRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum)
{

   CALC_FLAG_T *calcFlag;
   double *slewRate;
   double *crAmplitude;
   double *crDuration;

   ERROR_NUM_T *crError;
   double *crMoment0;
   double *crResolution;
   double *crTotalDuration;

   double *rampToCrDuration;
   double *rampToSsDuration;
   
   double polarity;
   double tempSlew;
			double tempMoment;
			double tempRamp1;
			double tempRamp2;
			
   crError= 0;
   polarity = 1.0;
			tempSlew = 0.0;
			tempMoment = 0.0;
			tempRamp1 = 0.0;
			tempRamp2 = 0.0;
			
   /* copy pointers */
			if (crusherNum == 1)
   {
      calcFlag         = &bfly->cr1CalcFlag;
      slewRate         = &bfly->cr1.slewRate;
      crAmplitude      = &bfly->cr1amp;
      crDuration       = &bfly->cr1.duration;
      crError          = &bfly->cr1.error;
      crMoment0        = &bfly->cr1Moment0;
      crResolution     = &bfly->cr1.resolution;
      crTotalDuration  = &bfly->cr1TotalDuration;
						
      rampToCrDuration = &bfly->ramp1.duration;
      rampToSsDuration = &bfly->ramp2.duration;
   }
   else if (crusherNum == 2)
   {
      calcFlag         = &bfly->cr2CalcFlag;
      slewRate         = &bfly->cr2.slewRate;
      crAmplitude      = &bfly->cr2amp;
      crDuration       = &bfly->cr2.duration;
      crError          = &bfly->cr2.error;
      crMoment0        = &bfly->cr2Moment0;
      crResolution     = &bfly->cr2.resolution;
      crTotalDuration  = &bfly->cr2TotalDuration;
      
						rampToCrDuration = &bfly->ramp4.duration;
      rampToSsDuration = &bfly->ramp3.duration;
   }
   else
   {
      *crError = ERR_PARAMETER_REQUEST;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }   

   if (FP_LT(*crMoment0,0))
   {
      polarity = -1.0;
   }		
			
			/* ss amplitude must be supplied or we don't know what to ramp down to */
   if(bfly->ssamp == 0)
   {
      *crError = ERR_EMPTY_SS_AMP;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }


   if(*calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP)
   {
      /* for shortest duration assume Amplitude is max gradient */
      *crAmplitude = bfly->maxGrad;
			}			

   /* calculate the plateau duration of the crusher */
			*crDuration = ( *crMoment0 - (bfly->ssamp * (*rampToSsDuration /2.0)) - 
  			             (*crAmplitude * ( (*rampToCrDuration/2.0) + 
																					(*rampToSsDuration/ 2.0) )) )/ *crAmplitude;

			/* check if we need a triangle */
			if (FP_LT(*crDuration, 0))																		
			{
      if(*calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP)
						{
						   *crError = ERR_CALC_INVALID;
         displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
         return;
						}
						/* we have a triangle, find its amplitude */
					 *crTotalDuration = *rampToCrDuration + *rampToSsDuration;

						*crAmplitude = (*crMoment0 -(*rampToSsDuration/2.0 * bfly->ssamp)) /
						               ( (*rampToSsDuration/2.0) + (*rampToSsDuration/2.0));
			}
			else
			{
						*crDuration = granularity(	*crDuration, *crResolution);

			   /* adjust amplitude due to granularity, preserve 0th moment */
					 *crAmplitude = adjustCrusherAmplitude(*rampToCrDuration,
			                                         *crDuration,
                                            *rampToSsDuration,
                                            *crMoment0,
                                            bfly->ssamp);

       /* total Crusher duration */
				   *crTotalDuration = *rampToCrDuration + *rampToSsDuration + *crDuration ;

			}
}

/***********************************************************************
*  Function Name: momentFromDurationAmplitudeRampButterfly
*  Example:    durationFromMomentButterfly(&butterflyGrad, 1);
*  Purpose:    Creates a crusher gradient for the calcButterfly
*              function.
*  Input
*     Formal:  *bfly      - butterfly gradient structure
*              crusherNum - which crusher we're generating, either
*                           1 or 2
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *bfly - butterfly gradient structure
*     Private: none
*     Public:  none
*  Notes:      After calculation is complete the ramps and crusher
*              primitive structures will be populated with the values
*              to create the crusher gradient.
***********************************************************************/
void momentFromDurationAmplitudeRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum)
{
   double *slewRate;
   double *crAmplitude;
   double *crDuration;
   ERROR_NUM_T *crError;
   double *crTotalDuration;
   double *crResolution;
   double *crMoment0;
			
   double *rampToCrDuration;
   double *rampToSsDuration;

   crError=0;
   
   /* copy pointers */
   if (crusherNum == 1)
   {
      slewRate        = &bfly->cr1.slewRate;
      crAmplitude     = &bfly->cr1amp;
      crDuration      = &bfly->cr1.duration;
      crError         = &bfly->cr1.error;
      crResolution    = &bfly->cr1.resolution;
      crTotalDuration = &bfly->cr1TotalDuration;
      crMoment0       = &bfly->cr1Moment0;
												
      rampToCrDuration = &bfly->ramp1.duration;
      rampToSsDuration = &bfly->ramp2.duration;
   }
   else if (crusherNum == 2)
   {
      slewRate        = &bfly->cr2.slewRate;
      crAmplitude     = &bfly->cr2amp;
      crDuration      = &bfly->cr2.duration;
      crError         = &bfly->cr2.error;
      crResolution    = &bfly->cr2.resolution;
      crTotalDuration = &bfly->cr2TotalDuration;
      crMoment0       = &bfly->cr2Moment0;
						
      rampToCrDuration = &bfly->ramp4.duration;
      rampToSsDuration = &bfly->ramp3.duration;
   }
   else
   {
      *crError = ERR_PARAMETER_REQUEST;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   if (FP_NEQ(*crTotalDuration, granularity(*crTotalDuration, *crResolution)))
   {
      /* duration must conform to resolution of system */
      *crError = ERR_GRANULARITY;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   
   if(*crTotalDuration == 0)
   {
      /* this is a valid case, nothing to do but return */
      return;
   }

   if (FP_LT(*crTotalDuration, *crResolution))
   {
      /* in this case duration shouldn't be zero */
      *crError = ERR_DURATION;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
			
			if (FP_LT(*crTotalDuration, ( *rampToCrDuration + *rampToSsDuration)) )
			{
      /* duration must conform to resolution of system */
      *crError = ERR_DURATION;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;			
			}

   *crDuration = *crTotalDuration - *rampToCrDuration - *rampToSsDuration;

   *crMoment0 = (*crAmplitude * *rampToCrDuration / 2.0 ) +
			             (*crAmplitude * *crDuration) +
																(*crAmplitude * *rampToSsDuration / 2.0 ) +
																(bfly->ssamp * *rampToSsDuration / 2.0);
}


/***********************************************************************
*  Function Name: amplitudeFromMomentDurationRampButterfly
*  Example:    durationFromMomentButterfly(&butterflyGrad, 1);
*  Purpose:    Creates a crusher gradient for the calcButterfly
*              function.
*  Input
*     Formal:  *bfly      - butterfly gradient structure
*              crusherNum - which crusher we're generating, either
*                           1 or 2
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *bfly - butterfly gradient structure
*     Private: none
*     Public:  none
*  Notes:      After calculation is complete the ramps and crusher
*              primitive structures will be populated with the values
*              to create the crusher gradient.
***********************************************************************/
void amplitudeFromMomentDurationRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum)
{
   double *slewRate;
   double *crAmplitude;
   double *crDuration;
   ERROR_NUM_T *crError;
   double *crTotalDuration;
   double *crMoment0;
   double *crResolution;

   double *rampToCrDuration;
   double *rampToSsDuration;

   crError=0;

   /* copy pointers */
   if (crusherNum == 1)
   {
      slewRate        = &bfly->cr1.slewRate;
      crAmplitude     = &bfly->cr1amp;
      crDuration      = &bfly->cr1.duration;
      crError         = &bfly->cr1.error;
      crMoment0       = &bfly->cr1Moment0;
      crResolution    = &bfly->cr1.resolution;
      crTotalDuration = &bfly->cr1TotalDuration;

      rampToCrDuration = &bfly->ramp1.duration;
      rampToSsDuration = &bfly->ramp2.duration;
   }
   else if (crusherNum == 2)
   {
      slewRate        = &bfly->cr2.slewRate;
      crAmplitude     = &bfly->cr2amp;
      crDuration      = &bfly->cr2.duration;
      crError         = &bfly->cr2.error;
      crMoment0       = &bfly->cr2Moment0;
      crResolution    = &bfly->cr2.resolution;
      crTotalDuration = &bfly->cr2TotalDuration;

      rampToCrDuration = &bfly->ramp4.duration;
      rampToSsDuration = &bfly->ramp3.duration;
   }
   else
   {
      *crError = ERR_PARAMETER_REQUEST;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* ss amplitude must be supplied or we don't know what to ramp down to */
   if(bfly->ssamp == 0)
   {
      *crError = ERR_EMPTY_SS_AMP;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* calculate plateau duration */
			*crDuration = *crTotalDuration - *rampToCrDuration -  *rampToSsDuration;

   if (FP_LT(*crDuration, 0.0))
   {
      *crError = ERR_CR_DURATION;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
			
   /* calculate amplitude */
			*crAmplitude = (*crMoment0 - (*rampToSsDuration / 2.0 * bfly->ssamp) ) /
			               ( *crDuration + (*rampToCrDuration/2.0 )  + (*rampToSsDuration/2.0) );
		
		 /* check that the amplitude is in range */
   if (FP_GT(*crAmplitude, bfly->maxGrad))
   {
      *crError = ERR_CR_AMPLITUDE;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

	   /* check slew rate, ramp from zero to crusher */
   if (FP_GT(fabs(*crAmplitude) / *rampToCrDuration, MAX_SLEW_RATE))
   {
      /* violation of slew rate here means there is no solution */
      *crError = ERR_NO_SOLUTION;
      displayError(*crError, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
}

/***********************************************************************
*  Function Name: calcButterfly
*  Example:    calcButterfly(&butterflyGrad);
*  Purpose:    Creates a combination gradient: crusher, slice select,
*              crusher.
*              Legal combinations of inputs for crushers and slice
*              select:
*                 1.  0th moment
*                 2.  0th moment and amplitude
*                 3.  0th moment and duration
*                 4.  Duration and amplitude
*  Input
*     Formal:  *bfly - butterfly gradient structure
*     Private: none
*     Public:  MAX_SLEW_RATE - maximum slew rate of system
*  Output
*     Return:  none
*     Formal:  *bfly - butterfly gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calcButterfly(BUTTERFLY_GRADIENT_T *bfly)
{
   MERGE_GRADIENT_T mg1;
   MERGE_GRADIENT_T mg2;
   MERGE_GRADIENT_T cr1;
   MERGE_GRADIENT_T cr2;
   double junk;
//   char grad_params[MAX_STR];
	char *gParamStr;
   char tempname[MAX_STR];
//   char tempstr[MAX_STR];

   /* save  features for later comparison */
	gParamStr = NULL;
	appendFormattedString( &gParamStr, "%f ",bfly->amp );
	appendFormattedString( &gParamStr, "%f ",bfly->cr1amp );
	appendFormattedString( &gParamStr, "%f ",bfly->cr2amp );
	appendFormattedString( &gParamStr, "%f ",bfly->ssamp );
	appendFormattedString( &gParamStr, "%f ",bfly->cr1TotalDuration );
	appendFormattedString( &gParamStr, "%f ",bfly->cr2TotalDuration );
	appendFormattedString( &gParamStr, "%f ",bfly->ssTotalDuration );
	appendFormattedString( &gParamStr, "%d ",bfly->cr1CalcFlag );
	appendFormattedString( &gParamStr, "%d ",bfly->cr2CalcFlag );
	appendFormattedString( &gParamStr, "%d ",bfly->ssCalcFlag );
	appendFormattedString( &gParamStr, "%f ",bfly->ssMoment0 );
	appendFormattedString( &gParamStr, "%f ",bfly->maxGrad );
	appendFormattedString( &gParamStr, "%f ",bfly->slewRate );
	appendFormattedString( &gParamStr, "%f ",bfly->resolution );

   /* check input values */
   if (FP_GT(fabs(bfly->amp), bfly->maxGrad))
   {
      /* amplitude must be less than bfly->maxGrad */
      bfly->error = ERR_AMPLITUDE;
      displayError(bfly->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_GT(fabs(bfly->cr1amp), bfly->maxGrad))
   {
      bfly->error = ERR_CR_AMPLITUDE;
      displayError(bfly->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_GT(fabs(bfly->cr2amp), bfly->maxGrad))
   {
      bfly->error = ERR_CR_AMPLITUDE;
      displayError(bfly->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LTE(bfly->maxGrad, 0))
   {
      /* maxGrad must be positive */
      bfly->error = ERR_MAX_GRAD;
      displayError(bfly->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   /* check that slew rate isn't greater than max */
   if (FP_GT(bfly->slewRate, MAX_SLEW_RATE))
   {
      bfly->error = ERR_SLEW_RATE;
      displayError(bfly->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* calculations for slice select plateau */
   /* ss is calculated first because the ramp down of the */
   /* first crusher may require that the ss amplitude be known */
   switch (bfly->ssCalcFlag)
   {
   case SHORTEST_DURATION_FROM_MOMENT:
   case DURATION_FROM_MOMENT_AMPLITUDE:
      if(bfly->ssCalcFlag != DURATION_FROM_MOMENT_AMPLITUDE)
      {
         /* no amplitude supplied, so assume maxGrad */
         bfly->ssamp = bfly->maxGrad;
      }

      bfly->ssTotalDuration = granularity(bfly->ssMoment0 / bfly->ssamp, bfly->resolution);

      /* adjust amplitude due to granularity */
      bfly->ssamp = bfly->ssMoment0 / bfly->ssTotalDuration;
      break;
   case AMPLITUDE_FROM_MOMENT_DURATION:
      if (FP_NEQ(bfly->ssTotalDuration, granularity(bfly->ssTotalDuration, bfly->ss.resolution)))
      {
         /* duration must conform to resolution of system */
         bfly->ss.error = ERR_GRANULARITY;
         displayError(bfly->ss.error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
      if (FP_LT(bfly->ssTotalDuration, bfly->ss.resolution))
      {
         /* in this case duration shouldn't be zero */
         bfly->ss.error = ERR_DURATION;
         displayError(bfly->ss.error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }

      bfly->ssamp = bfly->ssMoment0 / bfly->ssTotalDuration;
      break;
   case MOMENT_FROM_DURATION_AMPLITUDE:
      if (FP_NEQ(bfly->ssTotalDuration, granularity(bfly->ssTotalDuration, bfly->ss.resolution)))
      {
         /* duration must conform to resolution of system */
         bfly->ss.error = ERR_GRANULARITY;
         displayError(bfly->ss.error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
      if (FP_LT(bfly->ssTotalDuration, bfly->ss.resolution))
      {
         /* in this case duration shouldn't be zero */
         bfly->ss.error = ERR_DURATION;
         displayError(bfly->ss.error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
      break;
   case MOMENT_FROM_DURATION_AMPLITUDE_RAMP:
   case AMPLITUDE_FROM_MOMENT_DURATION_RAMP:
   case DURATION_FROM_MOMENT_AMPLITUDE_RAMP:
   case SHORTEST_DURATION_FROM_MOMENT_RAMP:
   default:
      bfly->ss.error = ERR_CALC_FLAG;
      displayError(bfly->ss.error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   if (bfly->ss.error != ERR_NO_ERROR)
   {
      bfly->error = bfly->ss.error;
      return;
   }

   /* now do calculations for first crusher */
   switch (bfly->cr1CalcFlag)
   {
   case SHORTEST_DURATION_FROM_MOMENT:
   case DURATION_FROM_MOMENT_AMPLITUDE:
      durationFromMomentButterfly(bfly, 1);
      break;
   case MOMENT_FROM_DURATION_AMPLITUDE:
      momentFromDurationAmplitudeButterfly(bfly, 1);
      break;
   case AMPLITUDE_FROM_MOMENT_DURATION:
      amplitudeFromMomentDurationButterfly(bfly, 1);
      break;
   case MOMENT_FROM_DURATION_AMPLITUDE_RAMP:
    			momentFromDurationAmplitudeRampButterfly(bfly,1);
							break;
   case AMPLITUDE_FROM_MOMENT_DURATION_RAMP:
			 			amplitudeFromMomentDurationRampButterfly(bfly,1);
							break;
   case SHORTEST_DURATION_FROM_MOMENT_RAMP:
   case DURATION_FROM_MOMENT_AMPLITUDE_RAMP:
      durationFromMomentRampButterfly(bfly,1);
  				break;	
   default:
      bfly->cr1.error = ERR_CALC_FLAG;
      bfly->error = bfly->cr1.error;
      displayError(bfly->cr1.error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   if (bfly->cr1.error != ERR_NO_ERROR)
   {
      bfly->error = bfly->cr1.error;
      return;
   }

   /* populate primitive structures */
   bfly->ramp1.startAmplitude = 0;
   bfly->ramp1.endAmplitude   = bfly->cr1amp;
   bfly->ramp1.numPoints      = (long)ROUND(bfly->ramp1.duration / bfly->ramp1.resolution);
   bfly->ramp1.slewRate       = fabs(bfly->ramp1.startAmplitude - bfly->ramp1.endAmplitude)/
         			                    bfly->ramp1.duration;
			
   bfly->cr1.startAmplitude = bfly->cr1amp;
   bfly->cr1.endAmplitude   = bfly->cr1amp;
   bfly->cr1.numPoints      = (long)ROUND(bfly->cr1.duration / bfly->cr1.resolution);
   bfly->cr1.slewRate       = fabs(bfly->cr1.startAmplitude - bfly->cr1.endAmplitude)/
         			                  bfly->cr1.duration;

   bfly->ramp2.startAmplitude = bfly->cr1amp;
   bfly->ramp2.endAmplitude   = bfly->ssamp;
   bfly->ramp2.numPoints      = (long)ROUND(bfly->ramp2.duration / bfly->ramp2.resolution);
   bfly->ramp2.slewRate       = fabs(bfly->ramp2.startAmplitude - bfly->ramp2.endAmplitude)/
         			                    bfly->ramp2.duration;


   bfly->ss.startAmplitude = bfly->ssamp;
   bfly->ss.endAmplitude   = bfly->ssamp;
   bfly->ss.duration       = bfly->ssTotalDuration;
   bfly->ss.numPoints      = (long)ROUND(bfly->ss.duration / bfly->ss.resolution);
   bfly->ss.slewRate       = fabs(bfly->ss.startAmplitude - bfly->ss.endAmplitude)/
         			                    bfly->ss.duration;

   /* data structures are populated, now make first crusher and slice select */

   calcPrimitive(&bfly->ramp1);

   if (bfly->ramp1.error != ERR_NO_ERROR)
   {
      bfly->error = bfly->ramp1.error;
      return;
   }
   calcPrimitive(&bfly->cr1);

   if (bfly->cr1.error != ERR_NO_ERROR)
   {
      bfly->error = bfly->cr1.error;
      return;
   }

   calcPrimitive(&bfly->ramp2);

   if (bfly->ramp2.error != ERR_NO_ERROR)
   {
      bfly->error = bfly->ramp2.error;
      return;
   }

   calcPrimitive(&bfly->ss);

   if (bfly->ss.error != ERR_NO_ERROR)
   {
      bfly->error = bfly->ss.error;
      return;
   }

   switch (bfly->cr2CalcFlag)
   {
   case SHORTEST_DURATION_FROM_MOMENT:
   case DURATION_FROM_MOMENT_AMPLITUDE:
      durationFromMomentButterfly(bfly, 2);
      break;
   case MOMENT_FROM_DURATION_AMPLITUDE:
      momentFromDurationAmplitudeButterfly(bfly, 2);
      break;
   case AMPLITUDE_FROM_MOMENT_DURATION:
      amplitudeFromMomentDurationButterfly(bfly, 2);
      break;
   case MOMENT_FROM_DURATION_AMPLITUDE_RAMP:
    			momentFromDurationAmplitudeRampButterfly(bfly,2);
							break;
   case AMPLITUDE_FROM_MOMENT_DURATION_RAMP:
			 			amplitudeFromMomentDurationRampButterfly(bfly,2);
							break;
   case SHORTEST_DURATION_FROM_MOMENT_RAMP:
   case DURATION_FROM_MOMENT_AMPLITUDE_RAMP:
      durationFromMomentRampButterfly(bfly,2);
  				break;	
   default:
      bfly->cr2.error = ERR_CALC_FLAG;
      bfly->error = bfly->cr2.error;
      displayError(bfly->cr2.error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   if (bfly->cr2.error != ERR_NO_ERROR)
   {
      bfly->error = bfly->cr2.error;
      return;
   }

   /* populate primitive structures */
   bfly->ramp3.startAmplitude = bfly->ssamp;
   bfly->ramp3.endAmplitude   = bfly->cr2amp;
   bfly->ramp3.numPoints      = (long)ROUND(bfly->ramp3.duration / bfly->ramp3.resolution);
   bfly->ramp3.slewRate       = fabs(bfly->ramp3.startAmplitude - bfly->ramp3.endAmplitude)/
         			                    bfly->ramp3.duration;


   bfly->cr2.startAmplitude = bfly->cr2amp;
   bfly->cr2.endAmplitude   = bfly->cr2amp;
   bfly->cr2.numPoints      = (long)ROUND(bfly->cr2.duration / bfly->cr2.resolution);
   bfly->cr2.slewRate       = fabs(bfly->cr2.startAmplitude - bfly->cr2.endAmplitude)/
         			                    bfly->cr2.duration;

   bfly->ramp4.startAmplitude = bfly->cr2amp;
   bfly->ramp4.endAmplitude   = 0;
   bfly->ramp4.numPoints      = (long)ROUND(bfly->ramp4.duration / bfly->ramp4.resolution);
   bfly->ramp4.slewRate       = fabs(bfly->ramp4.startAmplitude - bfly->ramp4.endAmplitude)/
         			                    bfly->ramp4.duration;


   /* data structures are populated, now make second crusher */

   calcPrimitive(&bfly->ramp3);

   if (bfly->ramp3.error != ERR_NO_ERROR)
   {
      bfly->error = bfly->ramp3.error;
      return;
   }

   calcPrimitive(&bfly->cr2);

   if (bfly->cr2.error != ERR_NO_ERROR)
   {
      bfly->error = bfly->cr2.error;
      return;
   }

   calcPrimitive(&bfly->ramp4);
   
   if (bfly->ramp4.error != ERR_NO_ERROR)
   {
      bfly->error = bfly->ramp4.error;
      return;
   }

   /* merge primitives together */
   /* make crusher 1 */
   concatenateGradients(bfly->ramp1.dataPoints, bfly->ramp1.numPoints,
                        bfly->cr1.dataPoints, bfly->cr1.numPoints,
                        &mg1);
			
   concatenateGradients(mg1.dataPoints, mg1.numPoints,
                        bfly->ramp2.dataPoints, bfly->ramp2.numPoints,
                        &cr1);

   /* merge slice select */
   concatenateGradients(cr1.dataPoints, cr1.numPoints,
                        bfly->ss.dataPoints, bfly->ss.numPoints,
                        &mg1);

   /* make crusher 2 */
   concatenateGradients(bfly->ramp3.dataPoints, bfly->ramp3.numPoints,
                        bfly->cr2.dataPoints, bfly->cr2.numPoints,
                        &mg2);

   concatenateGradients(mg2.dataPoints, mg2.numPoints,
                        bfly->ramp4.dataPoints, bfly->ramp4.numPoints,
                        &cr2);
   
   /* merge slice select (with crusher 1) and crusher 2 */
   concatenateGradients(mg1.dataPoints, mg1.numPoints,
                        cr2.dataPoints, cr2.numPoints,
                        &mg2);

   bfly->dataPoints = mg2.dataPoints;
   bfly->numPoints  = mg2.numPoints;
			bfly->slewRate   = MAX(MAX(MAX( bfly->ramp1.slewRate , bfly->ramp2.slewRate),
			                      	bfly->ss.slewRate), bfly->cr1.slewRate);
			bfly->slewRate			= MAX(MAX(MAX(bfly->slewRate,	bfly->ramp3.slewRate	)	,
			                       bfly->ramp4.slewRate),bfly->cr2.slewRate);																	


   /* do moment calculations for first crusher */
   calculateMoments(cr1.dataPoints, cr1.numPoints,
                    1,  /* start index */
                    0,  /* start amplitude */
                    bfly->resolution, &bfly->cr1Moment0,
                    &junk,
                    0,  /* start delta offset */
                    0); /* end delta offset */

   bfly->ssMoment0 = bfly->ss.m0;

   /* do moment calculations for second crusher */
   calculateMoments(cr2.dataPoints, cr2.numPoints,
                    1,                 /* start index */
                    bfly->ssamp, /* start amplitude */
                    bfly->resolution, &bfly->cr2Moment0,
                    &junk,
                    0,  /* start delta offset */
                    0); /* end delta offset */

   /* do moment calculations for whole waveform */
   calculateMoments(bfly->dataPoints, bfly->numPoints,
                    1, /* start index */
                    0, /* start amplitude */
                    bfly->resolution, &bfly->m0,
                    &bfly->m1,
                    0,  /* start delta offset */
                    0); /* end delta offset */

   /* check slew rate of concatenated points */

   if((bfly->error = checkSlewRate(bfly->dataPoints,
                                   bfly->numPoints,
                                   0,
                                   bfly->resolution)) != ERR_NO_ERROR)
   {
      /* zero out the array */
      zeroData(bfly->dataPoints, bfly->numPoints);

      displayError(bfly->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* write gradient waveform to disk if required */
   if (bfly->writeToDisk)
   {
     if(!gradShapeWritten(bfly->name, gParamStr, tempname))
       {
	 (void)strcpy(bfly->name, tempname); /* save file name */
		
	 bfly->error = writeToDisk(bfly->dataPoints, bfly->numPoints, 0,
				   bfly->resolution, bfly->rollOut, bfly->name);
       }
       else
	 (void)strcpy(bfly->name, tempname); /* save file name */
   }

   bfly->amp = MAX( fabs(bfly->cr1amp), fabs(bfly->cr2amp));
   bfly->amp = MAX( fabs(bfly->amp), fabs(bfly->ssamp));
   
   /* Display structure for debugging */
   if ((sgldisplay == DISPLAY_MID_LEVEL) || (sgldisplay == DISPLAY_LOW_LEVEL)) displayButterfly(bfly);
   
		 /* Release memory */	

}

/***********************************************************************
*  Function Name: momentFromDurationAmplitude
*  Example:    momentFromDurationAmplitude(&generic);
*  Purpose:    Determine generic gradient properties based on generic
*              duration, amplitude and shape.
*  Input
*     Formal:  *generic - pointer to generic gradient structure
*     Private: none
*     Public:  
*  Output
*     Return:  none
*     Formal:  *generic - pointer to generic gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void momentFromDurationAmplitude(GENERIC_GRADIENT_T *gg)
{
   switch(gg->ramp1.shape.function)
   {
   case PRIMITIVE_LINEAR:
   case PRIMITIVE_SINE:
      /* see if we're trying to make a shape with no plateau */
      if (gg->noPlateau == TRUE)
      {
         if (gg->calcFlag != MOMENT_FROM_DURATION_AMPLITUDE_RAMP)
         {
            gg->ramp1.duration = gg->duration / 2;
            gg->ramp2.duration = gg->ramp1.duration;
         }
         gg->plateau.duration = 0;
         break;
      }

      /* calculate ramp times when not supplied */
      if(gg->calcFlag != MOMENT_FROM_DURATION_AMPLITUDE_RAMP)
      {
         /* max slew rate and symmetric ramp times assumed */
         if (gg->ramp1.shape.function == PRIMITIVE_LINEAR)
         {
            gg->ramp1.duration = fabs(gg->amp) / MAX_SLEW_RATE;
         }
         if (gg->ramp1.shape.function == PRIMITIVE_SINE)
         {
            gg->ramp1.duration = fabs(gg->amp) * PI / (2 * MAX_SLEW_RATE);
         }
         gg->ramp2.shape.function = gg->ramp1.shape.function;
         gg->ramp2.duration = gg->ramp1.duration;
      }

      /* check duration */
      if (FP_GT(gg->ramp1.duration + gg->ramp2.duration, gg->duration))
      {
         /* ramp time too long or generic duration too short */
         gg->error = ERR_DURATION;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }

      /* calculate plateau duration */
      gg->plateau.duration = gg->duration - granularity(gg->ramp1.duration, gg->resolution) -
                                            granularity(gg->ramp2.duration, gg->resolution);
      break;
   case PRIMITIVE_GAUSS:
   default:
      gg->error = ERR_RAMP_SHAPE;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   } 

   /* adjust amplitude because of granularity */
   adjustAmplitude(gg);
}

/***********************************************************************
*  Function Name: amplitudeFromMomentDuration
*  Example:    amplitudeFromMomentDuration(&generic);
*  Purpose:    Determine generic gradient amplitude based on generic
*              moment, duration and shape.
*  Input
*     Formal:  *generic - pointer to generic gradient structure
*     Private: none
*     Public:  
*  Output
*     Return:  none
*     Formal:  *generic - pointer to generic gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void amplitudeFromMomentDuration(GENERIC_GRADIENT_T *gg)
{
   double a = 0; /* quadratic term */
   double b = 0; /* quadratic term */
   double c = 0; /* quadratic term */
   double quadraticResult1;
   double quadraticResult2;

   switch(gg->ramp1.shape.function)
   {
   case PRIMITIVE_LINEAR:
   case PRIMITIVE_SINE:
      /* if we're trying to make a shape with no plateau */
      if (gg->noPlateau == TRUE)
      {
         gg->ramp1.duration   = gg->duration / 2;
         gg->ramp2.duration   = gg->ramp1.duration;
         gg->plateau.duration = 0;
         break;
      }

      if (gg->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP)
      {
         /* check that ramp times are not greater than total duration */
         if (FP_GT(gg->ramp1.duration + gg->ramp2.duration, gg->duration))
         {
            /* ramp time too long or generic duration too short */
            gg->error = ERR_DURATION;
            displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
            return;
         }

         /* calculate plateau time, amplitude will be calculated by adjustAmplitude() */
         gg->plateau.duration = gg->duration - gg->ramp1.duration - gg->ramp2.duration;
         break;
      }
      
      /* populate quadratic terms */
      if (gg->ramp1.shape.function == PRIMITIVE_LINEAR)
      {
         a = 1 / gg->slewRate;
         b = -1 * gg->duration;
         c = fabs(gg->m0);
      }
      if (gg->ramp1.shape.function == PRIMITIVE_SINE)
      {
         a = (2 - PI) / gg->slewRate;
         b = gg->duration;
         c = -1 * fabs(gg->m0);
      }
      quadraticResult1 = (-b + sqrt(pow(b, 2) - (4 * a * c)) ) / (2 * a);
      quadraticResult2 = (-b - sqrt(pow(b, 2) - (4 * a * c)) ) / (2 * a);

      /* determine whether we build a triangle or trapezoid */
      if (gg->shape == TRIANGULAR)
      {
         /* no plateau, amplitude may reach gg->maxGrad */
         gg->plateau.duration = 0;

         /* a plateau will not exist, calculate as triangle or half-sine */
         gg->ramp1.duration   = gg->duration / 2;
         gg->ramp2.duration   = gg->ramp1.duration;
         gg->plateau.duration = 0;
      }
      /* otherwise do the trapezoid calculation */      
      if (gg->shape == TRAPEZOID)
      {
         /* a plateau will exist, use quadratic */
         /* check to see if there is a solution to the quadratic */
         if (FP_LT(fabs(b), 2 * sqrt(a * c)))
         {
            gg->error = ERR_NO_SOLUTION;
            displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
            return;
         }

         /* two possible solutions */
         quadraticResult1 = (-b + sqrt(pow(b, 2) - (4 * a * c)) ) / (2 * a);
         quadraticResult2 = (-b - sqrt(pow(b, 2) - (4 * a * c)) ) / (2 * a);
         /* determine which result to use */
         
	 /* both results out of range <0 or > maxGrad */
         if ( (FP_LT(quadraticResult1, 0) &&  FP_LT(quadraticResult2, 0)) ||
              (FP_GT(quadraticResult1, gg->maxGrad) && FP_GT(quadraticResult2, gg->maxGrad)) )
	    {
            gg->error = ERR_NO_SOLUTION;
            displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
            return;
	    }      
         /* pick the values that is in range */
	 /* Result 1 within range (0-maxGrad) and result2 out of range */
	 if ( (FP_GTE(quadraticResult1, 0) && FP_LTE(quadraticResult1, gg->maxGrad)) &&
              (FP_LTE(quadraticResult2, 0) || FP_GTE(quadraticResult2, gg->maxGrad)) )
	    {
	    gg->amp = quadraticResult1 * SIGN(gg->m0);
	    } 	     
	 /* Result 2 within range (0-maxGrad) and result 1 out of range */	      
	 if ( (FP_GTE(quadraticResult2, 0) && FP_LTE(quadraticResult2, gg->maxGrad)) &&
              (FP_LTE(quadraticResult1, 0) || FP_GTE(quadraticResult1, gg->maxGrad)) )
	    {
	    gg->amp = quadraticResult2 * SIGN(gg->m0);
	    } 	     
         /* both results within range (0-maxGRad) */
	 if ( (FP_GTE(quadraticResult1, 0) && FP_GTE(quadraticResult2, gg->maxGrad)) &&
              (FP_LTE(quadraticResult1, 0) || FP_GTE(quadraticResult1, gg->maxGrad)) )
            {
            /* pick the smaller amplitude */
	    if ( FP_LTE(quadraticResult1,quadraticResult2))
	       {
	       gg->amp = quadraticResult1 * SIGN(gg->m0);
	       }
	    else
	       {
	       gg->amp = quadraticResult2 * SIGN(gg->m0);
	       }
	    } 	      
         /* find durations */
         if (gg->ramp1.shape.function == PRIMITIVE_LINEAR)
         {
            gg->ramp1.duration = fabs(gg->amp) / gg->slewRate;
         }
         if (gg->ramp1.shape.function == PRIMITIVE_SINE)
         {
            gg->ramp1.duration = fabs(gg->amp) * PI / (2 * gg->slewRate);
         }
         gg->ramp2.duration   = gg->ramp1.duration;
         gg->plateau.duration = gg->duration - granularity(gg->ramp1.duration, gg->resolution) -
                                               granularity(gg->ramp2.duration, gg->resolution);
      }
      break;
   case PRIMITIVE_GAUSS:
   default:
      gg->error = ERR_RAMP_SHAPE;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   } 
   /* adjust amplitude because of granularity */
   adjustAmplitude(gg);
}

/***********************************************************************
*  Function Name: durationFromMoment
*  Example:    durationFromMoment(&generic);
*  Purpose:    Determine generic gradient properties based on generic
*              moment and shape.
*  Input
*     Formal:  *generic - pointer to generic gradient structure
*     Private: none
*     Public:  
*  Output
*     Return:  none
*     Formal:  *generic - pointer to generic gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void durationFromMoment(GENERIC_GRADIENT_T *gg)
{
   double momentTemp = 0;
   double maxGrad = 0;

   /* when both moment and amplitude are supplied ... */
   if (gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE ||
       gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP)
   {
      /* ... check that the sign of amplitude and moment0 match for both cases */
      if (!( (FP_LT(gg->amp, 0) && FP_LT(gg->m0, 0)) ||
             (FP_GTE(gg->amp, 0) && FP_GTE(gg->m0, 0)) ))
      {
         gg->error = ERR_INCONSISTENT_PARAMETERS;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }

   if (gg->noPlateau == TRUE &&
       (gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP ||
        gg->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP))
   {
      /* the user has supplied all parameters needed to make the waveform, */
      /* check to see if they are consistent */
      if (gg->ramp1.shape.function == PRIMITIVE_LINEAR)
      {
         if (FP_NEQ(gg->m0, granularity(gg->tramp, gg->resolution) * gg->amp))
         {
            gg->error = ERR_INCONSISTENT_PARAMETERS;
            displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
            return;
         }
      }
      if (gg->ramp1.shape.function == PRIMITIVE_SINE)
      {
         if (FP_NEQ(gg->m0, 4 * gg->amp *
                                 granularity(gg->tramp, gg->resolution) / PI))
         {
            gg->error = ERR_INCONSISTENT_PARAMETERS;
            displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
            return;
         }
      }
   }

   switch(gg->ramp1.shape.function)
   {
   case PRIMITIVE_LINEAR:
   case PRIMITIVE_SINE:
      /* see if we're trying to make a shape with no plateau */
      if (gg->noPlateau == TRUE)
      {
         if (gg->ramp1.shape.function == PRIMITIVE_LINEAR)
         {
            if (gg->calcFlag == SHORTEST_DURATION_FROM_MOMENT)
            {
               /* all we have is the moment, use slewRate to find ramp duration */
               gg->ramp1.duration = sqrt(fabs(gg->m0) / gg->slewRate);
            }
            if (gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE)
            {
               /* we have the moment and the amplitude, find ramp duration */
               gg->ramp1.duration = fabs(gg->amp) / gg->slewRate;
            }

            /* test whether this ramp time and slew rate will exceed maxGrad */ 
            if (FP_GT(fabs(gg->m0) / gg->ramp1.duration, gg->maxGrad))
            {
               /* this will keep the amplitude at maxGrad and spread out the waveform */
               /* to acheive the desired moment0 */
               gg->ramp1.duration = fabs(gg->m0) / gg->maxGrad;
            }
         }
         if (gg->ramp1.shape.function == PRIMITIVE_SINE)
         {
            if (gg->calcFlag == SHORTEST_DURATION_FROM_MOMENT)
            {
               /* all we have is the moment, use slewRate to find ramp duration */
               gg->ramp1.duration = PI / 2 * sqrt(fabs(gg->m0) / (2 * gg->slewRate));
            }
            if (gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE)
            {
               /* we have the moment and the amplitude, find ramp duration */
               gg->ramp1.duration = fabs((gg->m0 * PI) / (2 * gg->amp));
            }

            /* test whether this ramp time and slew rate will exceed maxGrad */ 
            if (FP_GT(fabs(gg->m0) * PI / (2 *
                      granularity(gg->ramp1.duration, gg->resolution)) / 2, gg->maxGrad))
            {
               /* this will keep the amplitude at maxGrad and spread out the waveform */
               /* to acheive the desired moment0 */
               gg->ramp1.duration = fabs(gg->m0) * PI / (4 * gg->maxGrad);
            }
         }

         gg->ramp2.duration   = gg->ramp1.duration;
         gg->plateau.duration = 0;
         break;
      }

      /* figure out what slew rate to use */
      if (gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP) 
      {
         if (gg->ramp1.shape.function == PRIMITIVE_LINEAR)
         {
            /* amplitude is supplied, use it to determine slew rate */
            gg->slewRate = fabs(gg->amp) / gg->ramp1.duration;
         }
         if (gg->ramp1.shape.function == PRIMITIVE_SINE)
         {
            /* amplitude is supplied, use it to determine slew rate */
            gg->slewRate = fabs(gg->amp) * PI / (2 * gg->ramp1.duration);
         }

         /* check newly calculated slew rate */
         if (FP_GT(gg->slewRate, MAX_SLEW_RATE))
         {
            gg->error = ERR_SLEW_RATE;
            displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
            return;
         }
      }
      else
      {
         /* no ramp time given, so use highest slew rate */
         gg->slewRate = MAX_SLEW_RATE;
      }

      if (gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE ||
          gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP)
      {
         /* user has set the amplitude, don't go up to gg->maxGrad */
         maxGrad = fabs(gg->amp);
      }
      else
      {
         /* user hasn't set the amplitude, use gg->maxGrad */
         maxGrad = fabs(gg->maxGrad);
      }

      /* with our slew rate see if a ramp up and ramp down from maxGrad */
      /* will exhaust moment0 */
      if (gg->ramp1.shape.function == PRIMITIVE_LINEAR)
      {
         if (gg->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP)
         {
            /* set slew rate because user has specified the ramp duration */
            gg->slewRate = maxGrad / gg->ramp1.duration;

	           if (FP_GT(gg->slewRate, MAX_SLEW_RATE))
												{
												    maxGrad = MAX_SLEW_RATE * gg->ramp1.duration;
																gg->slewRate = maxGrad / gg->ramp1.duration;
												
												}
         }

         momentTemp = granularity(maxGrad / gg->slewRate, gg->resolution) * maxGrad;
      }
      if (gg->ramp1.shape.function == PRIMITIVE_SINE)
      {
         if (gg->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP)
         {
            /* set slew rate because user has specified the ramp duration */
            gg->slewRate = (PI / 2) * maxGrad / gg->ramp1.duration;
         }

         momentTemp = granularity(PI * maxGrad / (2 * gg->slewRate), gg->resolution) *
                      4 * maxGrad / PI;
      }

      if (FP_GTE(momentTemp, fabs(gg->m0)))
      {
         /* no plateau, amplitude may reach gg->maxGrad */
         gg->plateau.duration = 0;

         /* in the case where the amplitude is supplied, the half sine */
         /* must reach gg->maxGrad */
         if(FP_NEQ(momentTemp, fabs(gg->m0)) &&
            gg->ramp1.shape.function == PRIMITIVE_SINE &&
            (gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE ||
             gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP))
         {
            gg->error = ERR_INCONSISTENT_PARAMETERS;
            displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
            return;
         }

         /* cases other than ramps */
         if (gg->calcFlag != DURATION_FROM_MOMENT_AMPLITUDE_RAMP &&
             gg->calcFlag != SHORTEST_DURATION_FROM_MOMENT_RAMP)
         {
            if (gg->ramp1.shape.function == PRIMITIVE_LINEAR)
            {
               /* find ramp duration */
               gg->ramp1.duration = sqrt(fabs(gg->m0) / gg->slewRate);
            }
            if (gg->ramp1.shape.function == PRIMITIVE_SINE)
            {
               gg->ramp1.duration = maxGrad * PI / (2 * gg->slewRate);
            }
         }

         /* need to check consistency for DURATION_FROM_MOMENT_AMPLITUDE_RAMP */
         if (gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP)
         {
            if (gg->ramp1.shape.function == PRIMITIVE_LINEAR)
            {
               if (FP_LT(gg->amp * gg->ramp1.duration, gg->m0))
               {
                  /* m0, ramp times and amplitude are inconsistent */
                  gg->error = ERR_INCONSISTENT_PARAMETERS;
                  displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
                  return;
               }
            }
            if (gg->ramp1.shape.function == PRIMITIVE_SINE)
            {
               if (FP_LT(2 * gg->amp * gg->ramp1.duration / PI, gg->m0))
               {
                  /* moment0, ramp times and amplitude are inconsistent */
                  gg->error = ERR_INCONSISTENT_PARAMETERS;
                  displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
                  return;
               }
            }
         }
      }
      else /* amplitude will reach maxGrad, we have a trapezoid */
      {
         /* two cases already have ramps defined */
         if (gg->calcFlag != DURATION_FROM_MOMENT_AMPLITUDE_RAMP &&
             gg->calcFlag != SHORTEST_DURATION_FROM_MOMENT_RAMP)
         {
            /* find ramp durations */
            if (gg->ramp1.shape.function == PRIMITIVE_LINEAR)
            {
               gg->ramp1.duration = maxGrad / gg->slewRate;
            }
            if (gg->ramp1.shape.function == PRIMITIVE_SINE)
            {
               gg->ramp1.duration = maxGrad * PI / (2 * gg->slewRate);
            }
         }

         /* find plateau duration */
         if (gg->ramp1.shape.function == PRIMITIVE_LINEAR)
         {
            gg->ramp1.duration = granularity(gg->ramp1.duration, gg->ramp1.resolution);
            gg->ramp1.slewRate = maxGrad / gg->ramp1.duration;
            gg->plateau.duration = (fabs(gg->m0) - pow(gg->ramp1.duration, 2) *
                                    gg->ramp1.slewRate) /
                                   (gg->ramp1.duration * gg->ramp1.slewRate);
         }
         if (gg->ramp1.shape.function == PRIMITIVE_SINE)
         {
            gg->ramp1.duration = granularity(gg->ramp1.duration, gg->ramp1.resolution);
            gg->ramp1.slewRate = maxGrad / gg->ramp1.duration;
            gg->plateau.duration = (fabs(gg->m0) * pow(PI, 2) -
                                    8 * pow(gg->ramp1.duration, 2) * gg->ramp1.slewRate) /
                                   (2 * gg->ramp1.duration * PI * gg->ramp1.slewRate);
         }
      }
      break;
   case PRIMITIVE_GAUSS:
   default:
      gg->error = ERR_RAMP_SHAPE;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   } 

   adjustAmplitude(gg);
}

/***********************************************************************
*  Function Name: adjustAmplitude
*  Example:    adjustAmplitude(&generic);
*  Purpose:    Lowers the amplitude of a generic gradient due to the
*              granularity duration changes.  The idea is to preserve
*              the 0th moment of the waveform.
*  Input
*     Formal:  *generic - pointer to generic gradient structure
*     Private: none
*     Public:  
*  Output
*     Return:  none
*     Formal:  *generic - pointer to generic gradient structure
*     Private: none
*     Public:  none
*  Notes:      Duration and moment0 of each primitive must be set
*              BEFORE calling this function.  
***********************************************************************/
void adjustAmplitude(GENERIC_GRADIENT_T *gg)
{
   /* granulate durations */
      
   gg->plateau.duration = granularity(gg->plateau.duration, gg->resolution);
   gg->ramp1.duration = granularity(gg->ramp1.duration, gg->resolution);

   /* ramps are symmetric */
   gg->ramp2.duration = gg->ramp1.duration;

   if (gg->calcFlag != MOMENT_FROM_DURATION_AMPLITUDE &&
       gg->calcFlag != MOMENT_FROM_DURATION_AMPLITUDE_RAMP)
   {
      switch(gg->ramp1.shape.function)
      {
      case PRIMITIVE_LINEAR:
         if (FP_EQ(gg->plateau.duration, 0))
         {
            /* generic gradient is a triangle */
            /* find amplitude */
            /* moment0 = amplitude * ramp1.duration and */
            /* slewRate = amplitude / ramp1.duration */
            gg->amp = gg->m0 / gg->ramp1.duration;
   
            /* re-calculate slew rate */
            gg->slewRate = fabs(gg->m0) / (gg->ramp1.duration * gg->ramp1.duration);

            /* set flag indicating that we have no plateau */
            gg->noPlateau = TRUE;
         }
         else
         {
            /* generic gradient is a trapezoid */
            gg->amp = gg->m0 / (gg->ramp1.duration + gg->plateau.duration);
   
            /* re-calculate slew rate */
            if (FP_EQ(gg->amp, 0) &&
                FP_EQ(gg->ramp1.duration, 0))
            {
               gg->slewRate = 0;
            }
            else if (FP_NEQ(gg->ramp1.duration, 0))
            {
               gg->slewRate = fabs(gg->amp) / gg->ramp1.duration;
            }
         }
         break;
      case PRIMITIVE_SINE:
         if (FP_EQ(gg->plateau.duration, 0))
         {
            /* generic gradient is a half sine */
            /* find amplitude */
            gg->amp = gg->m0 * PI / (2 * gg->ramp1.duration);

            /* set flag indicating that we have no plateau */
            gg->noPlateau = TRUE;
         }
         else
         {
            /* generic gradient is trapezoid with sine ramps */
            gg->amp = gg->m0 / (4 * gg->ramp1.duration / PI + gg->plateau.duration);
         }
   
         /* re-calculate slew rate */
         gg->slewRate = fabs(gg->amp) * PI / (2 * gg->ramp1.duration);
         break;
      case PRIMITIVE_GAUSS:
      default:
         gg->error = ERR_SHAPE;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }

   switch(gg->ramp1.shape.function)
   {
      case PRIMITIVE_LINEAR:
						   gg->ramp1.slewRate = gg->amp / gg->ramp1.duration;
						   gg->ramp2.slewRate = gg->ramp1.slewRate;
									gg->plateau.slewRate = 0.0;
									break;
						
						case PRIMITIVE_SINE:
  						 gg->ramp1.slewRate = fabs(gg->amp) * PI / (2 * gg->ramp1.duration);
									gg->ramp2.slewRate = fabs(gg->amp) * PI / (2 * gg->ramp1.duration);
									gg->plateau.slewRate = 0.0;
   						break;			
      default:									
						   gg->ramp1.slewRate = MAX_SLEW_RATE;
									gg->ramp2.slewRate = MAX_SLEW_RATE;
									gg->plateau.slewRate = 0.0;
									break;
		 }

   /* define all primitive amplitudes */
   gg->ramp1.startAmplitude   = gg->startAmplitude;
   gg->ramp1.endAmplitude     = gg->amp;
   gg->plateau.startAmplitude = gg->amp;
   gg->plateau.endAmplitude   = gg->amp;
   gg->ramp2.startAmplitude   = gg->amp;
   gg->ramp2.endAmplitude     = gg->startAmplitude;

   /* copy generic resolution to each primitive */
   gg->ramp1.resolution   = gg->resolution;
   gg->plateau.resolution = gg->resolution;
   gg->ramp2.resolution   = gg->resolution;

   /* copy generic maxGrad each primitive */
   gg->ramp1.maxGrad   = gg->maxGrad;
   gg->plateau.maxGrad = gg->maxGrad;
   gg->ramp2.maxGrad   = gg->maxGrad;

   /* assign number of points for each primitive */
   gg->ramp1.numPoints   = (long)ROUND(gg->ramp1.duration   / gg->ramp1.resolution);
   gg->plateau.numPoints = (long)ROUND(gg->plateau.duration / gg->plateau.resolution);
   gg->ramp2.numPoints   = (long)ROUND(gg->ramp2.duration   / gg->ramp2.resolution);

   /* set generic duration */
   gg->duration = gg->ramp1.duration + gg->plateau.duration + gg->ramp2.duration;

   /* copy ramp1.duration to generic structure */
   gg->tramp = gg->ramp1.duration;
			
			/* copy ramp1.slewRate to generic structure */
   gg->slewRate = gg->ramp1.slewRate;			
}

/***********************************************************************
*  Function Name: genericRamp
*  Example:    genericRamp(&generic);
*  Purpose:    Generates a linear shape that may start or end at any
*              legal amplitude.  When slope is zero this function
*              generates a plateau.
*  Input
*     Formal:  *gg - pointer to generic gradient structure
*     Private: none
*     Public:  
*  Output
*     Return:  none
*     Formal:  *gg - pointer to generic gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void genericRamp (GENERIC_GRADIENT_T *gg)
{
   double a = 0; /* quadratic term */
   double b = 0; /* quadratic term */
   double c = 0; /* quadratic term */
   double quadraticResult1;
   double quadraticResult2;
   double maxGrad;

   /* when we're making a plateau, we need to ensure that startAmplitude */
   /* is the same as amplitude */
   if (gg->shape == PLATEAU &&
       FP_NEQ(gg->startAmplitude, gg->amp))
   {
      gg->startAmplitude = gg->amp;
   }

   /* build based on calc flag */
   switch (gg->calcFlag)
   {
   case MOMENT_FROM_DURATION_AMPLITUDE_RAMP:
      /* set duration to ramp time */
      gg->duration = gg->tramp;
   case MOMENT_FROM_DURATION_AMPLITUDE:
      /* granulate duration */
      gg->duration = granularity(gg->duration, gg->resolution);
      gg->tramp = gg->duration;
      break;
   case AMPLITUDE_FROM_MOMENT_DURATION_RAMP:
      /* set duration to ramp time */
      gg->duration = gg->tramp;
   case AMPLITUDE_FROM_MOMENT_DURATION:
      /* granulate duration */
      gg->duration = granularity(gg->duration, gg->resolution);
      gg->tramp = gg->duration;

      if (gg->shape == PLATEAU)
      {
         gg->amp = gg->m0 / gg->duration;
         gg->startAmplitude = gg->amp;
      }
      else
      {
         gg->amp = 2.0 * gg->m0 / gg->duration +
                         gg->startAmplitude -
                         2.0 * gg->startAmplitude;
      }
      break;
   case DURATION_FROM_MOMENT_AMPLITUDE_RAMP:
      /* check if input parameters are consistent */
      if (FP_EQ(gg->tramp, 2.0 * gg->m0 / (gg->amp * gg->startAmplitude)))
      {
         /* set duration to ramp time */
         gg->duration = granularity(gg->tramp, gg->resolution);
         gg->tramp = gg->duration;
      }
      else
      {
         gg->error = ERR_INCONSISTENT_PARAMETERS;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
      break;
   case SHORTEST_DURATION_FROM_MOMENT_RAMP:
      /* granulate duration */
      gg->duration = granularity(gg->tramp, gg->resolution);
      gg->tramp = gg->duration;

      gg->amp = 2.0 * gg->m0 / gg->duration +
                      gg->startAmplitude -
                      2.0 * gg->startAmplitude;
      break;
   case DURATION_FROM_MOMENT_AMPLITUDE:
   case SHORTEST_DURATION_FROM_MOMENT:
      if (gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE)
      {
         /* use the user supplied amplitude */
         maxGrad = gg->amp;
      }
      else
      {
         maxGrad = gg->maxGrad;
      }

      /* populate quadratic terms */
      a = gg->slewRate * SIGN(gg->m0);
      b = 2.0 * gg->startAmplitude;
      c = -2.0 * gg->m0;

      /* check to see if there is a solution to the quadratic */
      if (FP_LT(fabs(b), 2 * sqrt(a * c)))
      {
         gg->error = ERR_NO_SOLUTION;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }

      /* two possible solutions */
      quadraticResult1 = (-b + sqrt(pow(b, 2) - (4 * a * c)) ) / (2 * a);
      quadraticResult2 = (-b - sqrt(pow(b, 2) - (4 * a * c)) ) / (2 * a);
      
      /* determine which result to use */
      if (FP_GT(quadraticResult1, 0.0))
      {
         gg->duration = quadraticResult1;
      }
      else
      {
         gg->duration = quadraticResult2;
      }

      gg->duration = granularity(gg->duration, gg->resolution);

      /* adjust amplitude because of granularity */
      gg->amp = 2.0 * gg->m0 / gg->duration +
                      gg->startAmplitude -
                      2.0 * gg->startAmplitude;

      /* check to see if amplitude exceeds maxGrad */
      if (FP_GT(fabs(gg->amp), maxGrad))
      {
         /* the quadratic above used slew rate, since we've exceeded maxGrad */
         /* we need to re-calculate */
         gg->duration = fabs(2.0 * fabs(gg->m0) / (maxGrad * SIGN(gg->m0) +
                                                        gg->startAmplitude));

         gg->duration = granularity(gg->duration, gg->resolution);
   
         /* adjust amplitude because of granularity */
         gg->amp = 2.0 * gg->m0 / gg->duration +
                   gg->startAmplitude -
                   2.0 * gg->startAmplitude;
      }
      break;
   default:
      gg->error = ERR_CALC_FLAG;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   gg->numPoints = (long)ROUND(gg->duration / gg->resolution);
   gg->tramp  = gg->duration;

   /* populate primitive structure */
   if (gg->shape == RAMP)
   {
      gg->ramp1.numPoints      = gg->numPoints;
      gg->ramp1.duration       = gg->duration;
      gg->ramp1.startAmplitude = gg->startAmplitude;
      gg->ramp1.endAmplitude   = gg->amp;
      gg->ramp1.resolution     = gg->resolution;
						gg->ramp1.slewRate       = fabs(gg->ramp1.startAmplitude - gg->ramp1.endAmplitude) / 
						                           gg->ramp1.duration;
					 gg->ramp2.slewRate							= 0.0;
						gg->plateau.slewRate					= 0.0;
      gg->ramp2.duration       = 0.0;
																											
   }
   else /* PLATEAU */
   {
      gg->plateau.numPoints      = gg->numPoints;
      gg->plateau.duration       = gg->duration;
      gg->plateau.startAmplitude = gg->startAmplitude;
      gg->plateau.endAmplitude   = gg->amp;
      gg->plateau.resolution     = gg->resolution;
						gg->plateau.slewRate       = 0.0;
						gg->ramp1.slewRate         = 0.0;
						gg->ramp2.slewRate         = 0.0;
   }
}

/***********************************************************************
*  Function Name: genericSine
*  Example:    genericSine(&generic);
*  Purpose:    Generates a half sine shape that may start or end at any
*              legal amplitude.
*  Input
*     Formal:  *gg - pointer to generic gradient structure
*     Private: none
*     Public:  
*  Output
*     Return:  none
*     Formal:  *gg - pointer to generic gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void genericSine (GENERIC_GRADIENT_T *gg)
{
   /* build based on calc flag */
   switch (gg->calcFlag)
   {
   case MOMENT_FROM_DURATION_AMPLITUDE_RAMP:
      gg->duration = gg->tramp;
   case MOMENT_FROM_DURATION_AMPLITUDE:
      /* granulate duration */
      gg->duration = granularity(gg->duration, gg->resolution);
      break;
   case AMPLITUDE_FROM_MOMENT_DURATION_RAMP:
      /* set duration to ramp time */
      gg->duration = gg->tramp;
   case AMPLITUDE_FROM_MOMENT_DURATION:
      /* granulate duration */
      gg->duration = granularity(gg->duration, gg->resolution);

      /* find amplitude */
      gg->amp = gg->m0 * PI / (2 * gg->duration);
      if (FP_GT(fabs(gg->amp),gg->maxGrad))
         {
         /* amplitude(s) are out of range */
         gg->error = ERR_AMPLITUDE;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
         }
      break;
   case DURATION_FROM_MOMENT_AMPLITUDE_RAMP:
      /* check if input parameters are consistent */
      if (FP_EQ(gg->amp, gg->m0 * PI / (4 * gg->duration)))
      {
         gg->duration = granularity(gg->duration, gg->resolution);
      }
      else
      {
         gg->error = ERR_INCONSISTENT_PARAMETERS;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
      break;
   case SHORTEST_DURATION_FROM_MOMENT_RAMP:
      /* granulate duration */
      gg->duration = granularity(gg->tramp * 2, gg->resolution);
      /* find amplitude */
      gg->amp = gg->m0 * PI / (2 * gg->duration);
      break;
   case DURATION_FROM_MOMENT_AMPLITUDE:
   case SHORTEST_DURATION_FROM_MOMENT:
      /* all we have is the moment, use slewRate to find total duration */
      gg->duration = PI * sqrt(fabs(gg->m0) / (2 * gg->slewRate));

      /* granulate */
      gg->duration = granularity(gg->duration, gg->resolution);

      /* adjust amplitude due to granularity */
      gg->amp = gg->m0 * PI / (2 * gg->duration);

      /* see if amplitude exceeds maxGrad */
      if (FP_GT(fabs(gg->amp), gg->maxGrad))
      {
         /* we need to use maxGrad as ceiling */
         gg->duration = fabs(gg->m0 * PI / (2 * gg->maxGrad));
   
         /* granulate */
         gg->duration = granularity(gg->duration, gg->resolution);
   
         /* adjust amplitude due to granularity */
         gg->amp = gg->m0 * PI / (2 * gg->duration);

         /* check again */
         if (FP_GT(fabs(gg->amp), gg->maxGrad))
         {
            gg->error = ERR_AMPLITUDE;
            displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
            return;
         }
      }
      break;
   default:
      gg->error = ERR_CALC_FLAG;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   gg->numPoints = (long)ROUND(gg->duration / gg->resolution);
   gg->tramp  = gg->duration / 2;

   /* populate primitive structure */
   gg->ramp1.numPoints      = gg->numPoints;
   gg->ramp1.duration       = gg->duration;
   gg->ramp1.startAmplitude = gg->startAmplitude;
   gg->ramp1.endAmplitude   = gg->amp;
   gg->ramp1.resolution     = gg->resolution;
			gg->ramp1.slewRate       = fabs(gg->amp) * PI / (gg->ramp1.duration);

   gg->plateau.numPoints      = gg->numPoints;
   gg->plateau.duration       = 0.0;
   gg->plateau.startAmplitude = gg->amp;
   gg->plateau.endAmplitude   = gg->amp;
   gg->plateau.resolution     = gg->resolution;
			gg->plateau.slewRate       = 0.0;

   gg->ramp2.numPoints      = gg->numPoints;
   gg->ramp2.duration       = gg->duration;
   gg->ramp2.startAmplitude = gg->startAmplitude;
   gg->ramp2.endAmplitude   = gg->amp;
   gg->ramp2.resolution     = gg->resolution;
			gg->ramp2.slewRate       = fabs(gg->amp) * PI / (gg->ramp2.duration);


}

/***********************************************************************
*  Function Name: genericGauss
*  Example:    genericGauss(&generic);
*  Purpose:    Generates a gaussian shape.
*  Input
*     Formal:  *gg - pointer to generic gradient structure
*     Private: none
*     Public:  
*  Output
*     Return:  none
*     Formal:  *gg - pointer to generic gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void genericGauss (GENERIC_GRADIENT_T *gg)
{
   /* build based on calc flag */
   switch (gg->calcFlag)
   {
   case MOMENT_FROM_DURATION_AMPLITUDE:
      /* granulate duration */
      gg->duration = granularity(gg->duration, gg->resolution);
      gg->tramp = gg->duration / 2;

      /* populate primitive structure */
      gg->ramp1.duration       = gg->duration;
      gg->ramp1.startAmplitude = gg->startAmplitude;
      gg->ramp1.endAmplitude   = gg->amp;
      gg->ramp1.resolution     = gg->resolution;
      gg->ramp1.maxGrad        = gg->maxGrad;
      gg->ramp1.numPoints      = (long)ROUND(gg->ramp1.duration / gg->ramp1.resolution);
      gg->ramp1.shape.function = PRIMITIVE_GAUSS;
      break;
   case MOMENT_FROM_DURATION_AMPLITUDE_RAMP:
   case AMPLITUDE_FROM_MOMENT_DURATION_RAMP:
   case AMPLITUDE_FROM_MOMENT_DURATION:
   case DURATION_FROM_MOMENT_AMPLITUDE_RAMP:
   case SHORTEST_DURATION_FROM_MOMENT_RAMP:
   case DURATION_FROM_MOMENT_AMPLITUDE:
   case SHORTEST_DURATION_FROM_MOMENT:
   default:
      gg->error = ERR_CALC_FLAG;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
}

/***********************************************************************
*  Function Name: calcGeneric
*  Example:    calcGeneric(&generic);
*  Purpose:    Generates a three primitive gradient.  Usually this is
*              a ramp, plateau, ramp.  Each ramp may be linear or sine
*              but both ramps must be the same shape.  Plateau shape is
*              always linear.
*  Input
*     Formal:  *generic - pointer to generic gradient structure
*     Private: none
*     Public:  GRAD_MAX - maximum possible gradient amplitude
*  Output
*     Return:  none
*     Formal:  *generic - pointer to generic gradient structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calcGeneric (GENERIC_GRADIENT_T *gg)
{
   MERGE_GRADIENT_T mg1;
   MERGE_GRADIENT_T mg2;
			
//   char grad_params[MAX_STR];
	char *gParamStr;
   char tempname[MAX_STR];
//   char tempstr[MAX_STR];

   /* ensure granularity */
			gg->duration = granularity(gg->duration, gg->resolution);
			
			
   /* check polarity flag */
   if (!gg->polarity)
   {
      /* if polarity is disaallowed force positive values */
      gg->amp = fabs(gg->amp);
      gg->m0  = fabs(gg->m0);
      
      /* Function can have negative start amplitude */
      if (FP_LT(gg->startAmplitude,0))
      {
         /* Start amplitude < 0 and polarity flag = FALSE */
         gg->error = ERR_POLARITY;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }
										
   /* save  features for later comparison */
	gParamStr = NULL;
	appendFormattedString( &gParamStr, "%f ",gg->amp );
	appendFormattedString( &gParamStr, "%f ",gg->maxGrad );
	appendFormattedString( &gParamStr, "%f ",gg->resolution );
	appendFormattedString( &gParamStr, "%f ",gg->duration );
	appendFormattedString( &gParamStr, "%f ",gg->slewRate );
	appendFormattedString( &gParamStr, "%d ",gg->calcFlag );
	appendFormattedString( &gParamStr, "%f ",gg->tramp );
	appendFormattedString( &gParamStr, "%f ",gg->m0 );
	appendFormattedString( &gParamStr, "%d ",gg->shape );

   /* check input values */
   if ((gg->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE ||
        gg->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
        gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE ||
        gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP) &&
       FP_GT(fabs(gg->amp), gg->maxGrad))
   {
      /* amplitude must be less than gg->maxGrad */
      gg->error = ERR_AMPLITUDE;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   /* check input values */
   if (FP_LT(gg->maxGrad, 0) ||
       FP_GT(gg->maxGrad, GRAD_MAX))
   {
      /* maxGrad must be positive */
      gg->error = ERR_MAX_GRAD;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LTE(gg->resolution, 0))
   {
      /* maxGrad must be positive */
      gg->error = ERR_RESOLUTION;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(gg->duration, 0))
   {
      /* duration cannot be negative */
      gg->error = ERR_DURATION;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
			
			if (( gg->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
         gg->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP) &&
							( gg->shape == SINE  ||
							  gg->shape == GAUSS ) )
   {
			   if (FP_NEQ(gg->tramp * 2.0, gg->duration))
						{
									/* duration and ramp times need to match */
									gg->error = ERR_INCONSISTENT_PARAMETERS;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
						}			
			}									
							
			if (( gg->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
         gg->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP) &&
							( gg->shape == RAMP     ||
							  gg->shape == PLATEAU ) )
   {
			   if (FP_NEQ(gg->tramp , gg->duration))
						{
									/* duration and ramp times need to match */
									gg->error = ERR_INCONSISTENT_PARAMETERS;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
						}			
			}									
			
			
   if (FP_LT(gg->slewRate, 0))
   {
      /* slew rate cannot be negative */
      gg->error = ERR_SLEW_RATE;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   
   if (FP_NEQ(gg->areaOffset, 0) ||
       FP_NEQ(gg->amplitudeOffset, 0) ||
       FP_NEQ(gg->balancingMoment0,0) ||
       FP_NEQ(gg->fov,0) ||
       FP_NEQ(gg->increment,0) ||
       FP_NEQ(gg->steps, 0))
   {
      /* Parameter is not used in generic calculation */
      gg->error = ERR_PARAMETER_NOT_USED_GENERIC;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
  
   /* if the user supplied the duration then we need to check it for granularity */
   if (gg->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE ||
       gg->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
       gg->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION ||
       gg->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP)
   {
      if (FP_NEQ(gg->duration, granularity(gg->duration, gg->resolution)))
      {
         /* duration must conform to resolution of system */
         gg->error = ERR_GRANULARITY;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
      if (FP_LT(gg->duration, gg->resolution))
      {
         /* in this case duration shouldn't be zero */
         gg->error = ERR_PARAMETER_REQUEST;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }
   if (gg->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE_RAMP ||
       gg->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP ||
       gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP ||
       gg->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP)
   {
      /* make sure a ramp time is supplied */
      if (FP_EQ(gg->tramp, 0))
      {
         gg->error = ERR_RAMP_TIME;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }

      /* only symmetric ramps are supported */
      gg->ramp1.duration = gg->tramp;
      gg->ramp2.duration = gg->tramp;
   }
   if ((gg->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION      || 
        gg->calcFlag == AMPLITUDE_FROM_MOMENT_DURATION_RAMP || 
        gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE      || 
        gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP || 
        gg->calcFlag == SHORTEST_DURATION_FROM_MOMENT       || 
        gg->calcFlag == SHORTEST_DURATION_FROM_MOMENT_RAMP) && 
       FP_EQ(gg->m0, 0))
   {
      /* make sure the moment is supplied */
      gg->error = ERR_PARAMETER_REQUEST;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE  || 
       gg->calcFlag == DURATION_FROM_MOMENT_AMPLITUDE_RAMP)
   {
      if ((FP_LTE(gg->amp, 0) && FP_GTE(gg->m0, 0)) ||
          (FP_GTE(gg->amp, 0) && FP_LTE(gg->m0, 0)))
      {
         /* moment and amplitude must have the same sign */
         gg->error = ERR_PARAMETER_REQUEST;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }

   if (gg->calcFlag == MOMENT_FROM_DURATION_AMPLITUDE &&
       FP_EQ(gg->duration, 0))
   {
      /* nothing to do, just return */
      return;
   }

   /* determine what kind of shape we're making */
   if (gg->shape == TRAPEZOID)
   {
      /* trapezoid is considered a ramp, plateau, ramp.  The ramps may be of */
      /* any legal shape the user specifies.  The default is linear. */

      /* check ramp shape */
      if (gg->ramp1.shape.function == PRIMITIVE_SINE)
      {
         gg->ramp2.shape.function = PRIMITIVE_SINE;

         /* setup common sine ramp shapes (for use in mathFunction) */
         gg->ramp1.shape.startX = 0;
         gg->ramp1.shape.endX   = PI / 2;
         gg->ramp2.shape.startX = PI / 2;
         gg->ramp2.shape.endX   = PI;
      }
   }
   else if (gg->shape == SINE)
   {
      /* setup half sine shape (for use in mathFunction) */
      gg->ramp1.shape.startX   = 0;
      gg->ramp1.shape.endX     = PI;
      gg->ramp1.shape.function = PRIMITIVE_SINE;
      genericSine(gg);
   }
   else if (gg->shape == RAMP ||
            gg->shape == PLATEAU)
   {
      genericRamp(gg);
   }
   else if (gg->shape == GAUSS)
   {
      genericGauss(gg);
   }
   else if (gg->shape == TRIANGULAR)
   {
      gg->ramp1.shape.function = PRIMITIVE_LINEAR;
      gg->ramp2.shape.function = PRIMITIVE_LINEAR;
      gg->noPlateau = TRUE;
   }
   else
   {
      gg->error = ERR_SHAPE;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* copy generic resolution to primitives */
   gg->ramp1.resolution   = gg->resolution;
   gg->plateau.resolution = gg->resolution;
   gg->ramp2.resolution   = gg->resolution;

   if (gg->shape == TRAPEZOID || gg->shape == TRIANGULAR)
   {
      gg->startAmplitude = 0; /* this should be zero for trapezoidal shapes */

      /* now build generic based on calc flag */
      switch (gg->calcFlag)
      {
      case MOMENT_FROM_DURATION_AMPLITUDE:
      case MOMENT_FROM_DURATION_AMPLITUDE_RAMP:
         momentFromDurationAmplitude(gg);
         break;
      case AMPLITUDE_FROM_MOMENT_DURATION:
      case AMPLITUDE_FROM_MOMENT_DURATION_RAMP:
         if ( (FP_NEQ(granularity(gg->duration/2.0, gg->resolution)*2.0, gg->duration)) &&
	      (gg->shape == TRIANGULAR) ) 
	    {
	    gg->error = ERR_GRANULARITY_TRIANGLE;
            displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
            return;
	    }
         amplitudeFromMomentDuration(gg);
         break;
      case DURATION_FROM_MOMENT_AMPLITUDE:
      case DURATION_FROM_MOMENT_AMPLITUDE_RAMP:
      case SHORTEST_DURATION_FROM_MOMENT:
      case SHORTEST_DURATION_FROM_MOMENT_RAMP:
         durationFromMoment(gg);
         break;
      default:
         gg->error = ERR_CALC_FLAG;
         displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }

   if (gg->error != ERR_NO_ERROR)
   {
      return;
   }

   if (FP_LTE(gg->duration, 0))
   {
      /* duration cannot be zero or negative */
      gg->error = ERR_DURATION;
      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* data structures are populated, now make the primitive(s) */

   calcPrimitive(&gg->ramp1);

   if (gg->ramp1.error != ERR_NO_ERROR)
   {
      gg->error = gg->ramp1.error;
      return;
   }

   if (gg->shape == PLATEAU)
   {
				  calcPrimitive(&gg->plateau);

      if (gg->plateau.error != ERR_NO_ERROR)
      {
         gg->error = gg->plateau.error;
         return;
      }
   }

   if (gg->shape == TRAPEZOID || gg->shape == TRIANGULAR)
   {
      calcPrimitive(&gg->plateau);

      if (gg->plateau.error != ERR_NO_ERROR)
      {
         gg->error = gg->plateau.error;
         return;
      }
						
      calcPrimitive(&gg->ramp2);

      if (gg->ramp2.error != ERR_NO_ERROR)
      {
         gg->error = gg->ramp2.error;
         return;
      }
							
      /* merge primitives together */
      /* concatenate first ramp and plateau */
      concatenateGradients(gg->ramp1.dataPoints, gg->ramp1.numPoints,
                           gg->plateau.dataPoints, gg->plateau.numPoints,
                           &mg1);

      /* concatenate plateau and second ramp */
      concatenateGradients(mg1.dataPoints, mg1.numPoints,
                           gg->ramp2.dataPoints, gg->ramp2.numPoints,
                           &mg2);

      gg->dataPoints = mg2.dataPoints;
      gg->numPoints  = mg2.numPoints;
   }
   else
   {
      /* assign primitive data to generic */
      if (gg->shape == RAMP ||
          gg->shape == SINE ||
          gg->shape == GAUSS)
      {
         gg->dataPoints = gg->ramp1.dataPoints;
         gg->error      = gg->ramp1.error;
         gg->numPoints  = gg->ramp1.numPoints;

         if (gg->shape == RAMP)
									{
								   	gg->ramp1.duration = gg->duration ;
      						gg->ramp2.duration = 0.0;
												gg->slewRate = gg->ramp1.slewRate;
									}
         else
									{     
   									gg->ramp1.duration = gg->duration / 2.0;
      						gg->ramp2.duration = gg->duration / 2.0;
									}			
      }
      else /* PLATEAU */
      {
         gg->dataPoints = gg->plateau.dataPoints;
         gg->error      = gg->plateau.error;
         gg->numPoints  = gg->plateau.numPoints;
									gg->slewRate   = gg->plateau.slewRate;
      }
   }

   /* do moment calculations for whole generic */
   calculateMoments(gg->dataPoints, gg->numPoints,
                    1,                  /* start index */
                    gg->startAmplitude, /* start amplitude */
                    gg->resolution, &gg->m0,
                    &gg->m1,
                    0,  /* start delta offset */
                    0); /* end delta offset */

   /* check slew rate of concatenated points */
   if((gg->error = checkSlewRate(gg->dataPoints,
                                 gg->numPoints,
                                 gg->startAmplitude,
                                 gg->resolution)) != ERR_NO_ERROR)
   {
      /* zero out the array */
      zeroData(gg->dataPoints, gg->numPoints);

      displayError(gg->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* write generic gradient waveform to disk if required and if there */
   /* are no errors */
   if (gg->writeToDisk && gg->error == ERR_NO_ERROR)
      {
      if(!gradShapeWritten(gg->name, gParamStr, tempname))
         {
    	    (void)strcpy(gg->name, tempname); /* save file name */
       	 gg->error = writeToDisk(gg->dataPoints, gg->numPoints, gg->startAmplitude,
    				 gg->resolution, gg->rollOut, gg->name);
        }
     else
      	 (void)strcpy(gg->name, tempname); /* save file name */
      }
   
   gg->amp = MAX( fabs(gg->amp), fabs(gg->startAmplitude) );

   /* Display structure for debugging */
   if ((gg->display == DISPLAY_TOP_LEVEL) ||(gg->display == DISPLAY_MID_LEVEL) || 
       (gg->display == DISPLAY_LOW_LEVEL))
       {
       displayGeneric(gg);
       }

}

/***********************************************************************
*  Function Name: calcPrimitive
*  Example:   calcPrimitive (&primitive);
*  Purpose:   Generates data values between two points based on shape.
*  Input
*     Formal:  *primitive    - pointer to primitive structure
*     Private: none
*     Public:  GRAD_MAX      - maximum possible gradient amplitude
*              MAX_SLEW_RATE - maximum slew rate of system
*  Output
*     Return:  none
*     Formal:  *primitive - pointer to primitive structure
*     Private: none
*     Public:  none
*  Notes:      Data values generated are in double precision floating
*              point format.  Higher level functions are responsible for
*              concatenating multiple primitives.  Once all the
*              primitives are concatenated together, they can be
*              quantisized and written to disk.  This way there is
*              only one step that converts the data values to integer
*              DAC units scaling the complete waveform to GRAD_MAX.
*              Higher level functions are also responsible for setting
*              numPoints BEFORE calling this function.
*
*              When prim->numPoints is zero or when prim->duration is
*              zero no memory is allocated for the waveform since it
*              will be empty.  In this case be sure not to read or
*              write to the prim->dataPoints pointer.
***********************************************************************/
void calcPrimitive (PRIMITIVE_GRADIENT_T *prim)
{

   long   i = 0; /* looping variable */
   double maxAmplitude;

   /* check input values */
   if (FP_GT(fabs(prim->startAmplitude), prim->maxGrad) ||
       FP_GT(fabs(prim->endAmplitude), prim->maxGrad))
   {
      /* amplitude(s) are out of range */
      prim->error = ERR_AMPLITUDE;
      displayError(prim->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(prim->duration, 0))
   {
      /* duration cannot be negative */
      prim->error = ERR_DURATION;
      displayError(prim->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (prim->numPoints < 0)
   {
      /* number of points cannot be negative */
      prim->error = ERR_NUM_POINTS;
      displayError(prim->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_EQ(prim->duration, 0))
   {
      /* this is a valid case, make sure that number of points is also zero */
      prim->numPoints = 0;
						prim->dataPoints = NULL;
      return;
   }
   if (FP_NEQ(prim->duration, granularity(prim->duration, prim->resolution)))
   {
      /* duration must conform to resolution of system */
      prim->error = ERR_GRANULARITY;
      displayError(prim->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LT(prim->duration, prim->resolution))
   {
      /* in this case duration shouldn't be zero */
      prim->error = ERR_DURATION;
      displayError(prim->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (FP_LTE(prim->resolution, 0))
   {
      /* resolution must be greater than zero */
      prim->error = ERR_RESOLUTION;
      displayError(prim->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (prim->numPoints != (long)ROUND(prim->duration / prim->resolution))
   {
      /* number of points, duration and resolution are inconsistent */
      prim->error = ERR_INCONSISTENT_PARAMETERS;
      displayError(prim->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if (prim->numPoints == 0)
   {
      /* this is a valid case, make sure that duration is also zero */
      prim->duration = 0;
						prim->dataPoints = NULL;
      return;
   }

   if (FP_GT(prim->slewRate, MAX_SLEW_RATE) ||
       FP_GT(fabs(prim->startAmplitude - prim->endAmplitude) / prim->duration,
             MAX_SLEW_RATE)) /* simple linear check of slew rate */
   {
      /* slew rate value is out of range */
      prim->error = ERR_SLEW_RATE;
      displayError(prim->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   if ((prim->shape.function == SINE || prim->shape.function == GAUSS) &&
       FP_GT(prim->shape.startX, prim->shape.endX))
   {
      /* endX should be less than startX */
      prim->error = ERR_PARAMETER_REQUEST;
      displayError(prim->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* kludge to avoid core dump in malloc statement ---> Don't ask why - it ust works */
/* 		printf(" Size of pointer :\t%f\n",*(double *)malloc(sizeof(double) * prim->numPoints) );*/

   /* allocate memory for shape */			
   if ((prim->dataPoints = (double *)malloc(sizeof(double) *
                                            prim->numPoints)) == NULL)
   {
      displayError(ERR_MALLOC, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* if its a gaussian shape then give it fixed properties */
   if (prim->shape.function == PRIMITIVE_GAUSS)
   {
      prim->shape.startX = -4.8;
      prim->shape.endX   = 4.8;
   }

   /* generate all the data points */
   for (i = 1; i <= prim->numPoints; i++)
   {
      prim->dataPoints[i - 1] = mathFunction(prim, i);
   }

   /* check slew rate of generated points */
   if((prim->error = checkSlewRate(prim->dataPoints,
                                   prim->numPoints,
                                   prim->startAmplitude,
                                   prim->resolution)) != ERR_NO_ERROR)
   {
      /* zero out the array */
      zeroData(prim->dataPoints, prim->numPoints);
      displayError(prim->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* check that the amplitude is in range */
   maxAmplitude = findMax(prim->dataPoints, prim->numPoints, prim->startAmplitude);
   if(FP_GT(maxAmplitude, prim->maxGrad))
   {
      /* highest allowable gradient strengh has been exceeded */
      prim->error = ERR_AMPLITUDE;

      /* zero out the array */
      zeroData(prim->dataPoints, prim->numPoints);

      displayError(prim->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* calculate moments for this primitive */
   calculateMoments(prim->dataPoints, prim->numPoints,
                    1,  /* start index */
                    prim->startAmplitude, /* start amplitude */
                    prim->resolution, &prim->m0,
                    &prim->m1,
                    0,  /* start delta offset */
                    0); /* end delta offset */

   /* Display structure for debugging */
   if (sgldisplay == DISPLAY_LOW_LEVEL)
       displayPrimitive(prim);

}

/***********************************************************************
*  Function Name: mathFunction
*  Example:   value = mathFunction(prim, i);
*  Purpose:   Generates a single data point based on shape type and
*             other input parameters.
*  Input
*     Formal:  *primitive - pointer to primitive structure
*              i          - loop variable from generic function
*     Private: none
*     Public:  none
*  Output
*     Return:  single data point value
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      It is important to be careful when using power factors
*              that are greater than 1 as slew rate violations may
*              occur.
***********************************************************************/
double mathFunction(PRIMITIVE_GRADIENT_T * prim, long i)
{
   double result = 0;
   double percentCompleted;
   double xRange;
   double yRange;

   percentCompleted = (double)i / (double)prim->numPoints;
   xRange = prim->shape.endX - prim->shape.startX;
   yRange = prim->endAmplitude - prim->startAmplitude;

   switch (prim->shape.function)
   {
   case PRIMITIVE_LINEAR:
      /* simple line */
      result = prim->startAmplitude +
               yRange *
               percentCompleted;               
      break;
   case PRIMITIVE_SINE:
      /* see if we're making a ramp up or half sine */
      if((FP_EQ(prim->shape.startX, 0) && FP_EQ(prim->shape.endX, PI / 2)) ||
         (FP_EQ(prim->shape.startX, 0) && FP_EQ(prim->shape.endX, PI)))
      {
         /* startAmplitude is the base to start from */
         result = prim->startAmplitude +
                  yRange *
                  sin(prim->shape.startX + xRange * percentCompleted);
      }

      /* see if we're making a ramp down */
      if(FP_EQ(prim->shape.startX, PI / 2) &&
         FP_EQ(prim->shape.endX, PI))
      {
         /* endAmplitude is the base to start from */
         result = prim->endAmplitude +
                  -1 * yRange *
                  sin(prim->shape.startX + xRange * percentCompleted);
      }
      break;
   case PRIMITIVE_GAUSS:
      result = prim->startAmplitude +
               yRange *
               1 / (sqrt(2 * PI * pow(SIGMA, 2))) *
               exp( -(pow(prim->shape.startX + xRange * percentCompleted, 2) /
                     (2 * pow(SIGMA, 2))) );
      break;
   default:
      displayError(ERR_SHAPE, __FILE__, __FUNCTION__, __LINE__);
      return 0;
   }

   return(pow(result, prim->shape.powerFactor));
}

/***********************************************************************
*  Function Name: concatenateGradients
*  Example:    concatenateGradients(dataIn1, numPoints1,
*                                   dataIn2, numPoints2,
*                                   dataOut, numPointsOut);
*  Purpose:    Concatenates any two blocks of double data. 
*  Input
*     Formal:  *dataIn1   - pointer to arrary of double data
*              numPoints1 - number of points in double array dataIn1
*              *dataIn2   - pointer to arrary of double data
*              numPoints2 - number of points in double array dataIn2
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *mg - pointer to merge gradient structure
*     Private: none
*     Public:  none
*  Notes:      This function allocates memory for the concatenated
*              array.
***********************************************************************/
void concatenateGradients (double * dataIn1, long numPoints1,
                           double * dataIn2, long numPoints2,
                           MERGE_GRADIENT_T * mg)
{
   long i;
   long k;
   long numBytes;
 
   /* figure out how many bytes the total waveform will be */
   numBytes = sizeof(double) * (numPoints1 + numPoints2);

   /* total number of data points */
   mg->numPoints = numPoints1 + numPoints2;

   /* allocate memory for shape */
   if ((mg->dataPoints = (double *)malloc(numBytes)) == NULL)
   {
      displayError(ERR_MALLOC, __FILE__, __FUNCTION__, __LINE__);
      return;
   }

   /* copy the data into one array */
   for (i = 0, k = 0; i < numPoints1; i++, k++)
   {
      mg->dataPoints[k] = dataIn1[i];
   }

   for (i = 0; i < numPoints2; i++, k++)
   {
      mg->dataPoints[k] = dataIn2[i];
   }
}

/***********************************************************************
*  Function Name: calculateMoments
*  Example:    calculateMoments(dataIn, numPoints, resolution,
*                               startAmplitude, moment0, moment1);
*                               delta);
*  Purpose:    Calculates the 0th and 1st moments of an array of double
*              data.
*  Input
*     Formal:  *dataIn    - pointer to array of double data [G/cm]
*              numPoints  - number of points in double array dataIn
*              startIndex - first data point to start at (1 referenced)
*              startAmplitude - starting amplitude of the waveform
*                               [G/cm].  This is used to increase the
*                               accuracy of the moment calculations.
*              resolution - timing resolution of system [s]
*              delta      - offset when moment calculation starts
*                           between system ticks [s] 
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  moment0 - 0th moment [G/cm * s]
*              moment1 - 1st moment [G/cm * s^2]
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calculateMoments (double *dataIn, long numPoints, long startIndex,
                       double startAmplitude, double resolution,
                       double *moment0, double *moment1,
                       double startDelta, double endDelta)
{
   long   i;      /* looping variable */
   double t0 = 0; /* time 0 */
   double t1 = 0; /* time 1 */
   double a0;     /* amplitude 0 */
   double a1;     /* amplitude 1 */
   double m;      /* slope */
   double b;      /* y intercept */
   double tempResolution; /* hold resolution for a specific loop iteration */

   /* clear output variables */
   *moment0 = 0;
   *moment1 = 0;

   /* loop through data */
   for (i = startIndex; i <= numPoints; i++)
   {
      /* get resolution for this iteration */
      if (FP_NEQ(startDelta, 0) && i == startIndex)
      {
         /* for slice/echofraction */
         tempResolution = startDelta;
      }
      else if (FP_NEQ(endDelta, 0) && i == numPoints)
      {
         /* for readout/echofraction */
         tempResolution = endDelta;
      }
      else
      {
         /* for all other points we use the default system resolution */
         tempResolution = resolution;
      }

      if (i == startIndex)
      {
         /* we're on the first data point, so use the starting amplitude for */
         /* previous data point */
         a0 = startAmplitude;

         /* setup initial time values */
         t0 = 0;
         t1 = tempResolution;
      }
      else
      {
         /* get amplitude */
         a0 = dataIn[i - 2];

         /* add in next time chunk for this iteration */
         t0 = t1;
         t1 += tempResolution;
      }

      a1 = dataIn[i - 1];
      m  = (a1 - a0) / tempResolution;
      b  = a1 - m * t1;

      /* 0th moment calculation, assume line between points */
      *moment0 += tempResolution * (0.5 * (a1 - a0) + a0);

      /* 1st moment calculation, assume line between points */
      *moment1 += (1.0 / 3) * m * ((pow(t1, 3)) - (pow(t0, 3))) +
                  0.5 * b * (pow(t1, 2) - pow(t0, 2));
   }
}

/***********************************************************************
*  Function Name: checkSlewRate
*  Example:    checkSlewRate(dataIn, numPoints, startAmplitude,
*                            resolution, moment0, moment1);
*  Purpose:    Checks a double array to ensure that MAX_SLEW_RATE is not
*              violated.
*  Input
*     Formal:  *dataIn    - pointer to arrary of double data [G/cm]
*              numPoints  - number of points in double array dataIn
*              startAmplitude - starting amplitude of the waveform.
*                               This is used to increase the accuracy
*                               of the moment calculations.
*              resolution - timing resolution of system [s]
*     Private: none
*     Public:  MAX_SLEW_RATE - maximum slew rate of system
*  Output
*     Return:  none
*     Formal:  
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
ERROR_NUM_T checkSlewRate(double *dataIn, long numPoints,
                          double startAmplitude, double resolution)
{
   long i;
   double tempSlew;

   /* loop through data */
   for (i = 0; i < numPoints; i++)
   {
      if (i == 0)
      {
         /* we're on the first data point, so use the starting amplitude */
         tempSlew = (dataIn[i] - startAmplitude) / resolution;
      }
      else
      {
         /* we're on the first data point, so use the starting amplitude */
         tempSlew = (dataIn[i] - dataIn[i - 1]) / resolution;
      }

      /* test against system max slew rate */
      if(FPC_GT(tempSlew, MAX_SLEW_RATE, 1e-8))
      {
         return ERR_SLEW_RATE;
      }
   }
   return ERR_NO_ERROR;
}

/***********************************************************************
*  Function Name: findMax
*  Example:    max = findMax(&dataPoints, numPoints, startAmplitude); 
*  Purpose:    Finds the highest absolute value in a double array.
*  Input
*     Formal:  *dataPoints    - pointer to arrary of double data [G/cm]
*              numPoints      - number of points in double array dataPoints
*              startAmplitude - starting amplitude of gradient [G/cm]
*     Private: none
*     Public:  none
*  Output
*     Return:  max - absolute value of the larges element in dataPoints
*                    array
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
double findMax(double * dataPoints, long numPoints, double startAmplitude)
{
   long   i;   /* looping variable */
   double max; /* holds current max value when looping */

   max = fabs(startAmplitude);

   for(i = 0; i < numPoints; i++)
   {
      max = MAX(fabs(dataPoints[i]), max);
   }

   return(max);
}

/***********************************************************************
*  Function Name: zeroData
*  Example:    max = zeroData(&dataPoints, numPoints); 
*  Purpose:    Zero out a double array.
*  Input
*     Formal:  *dataPoints - pointer to arrary of double data [G/cm]
*              numPoints   - number of points in double array dataPoints
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void zeroData(double * dataPoints, long numPoints)
{
   long i;

   for (i = 0; i < numPoints; i++)
   {
      dataPoints[i] = 0;
   }
}

/***********************************************************************
*  Function Name: writeToDisk
*  Example:       writeToDisk(dataPoints, numPoints, resolution); 
*  Purpose:       Quantisize and write an array of data to the disk.
*  Input
*     Formal:  *dataPoints    - pointer to arrary of double data [G/cm]
*              numPoints      - number of points in double array
*                               dataPoints
*              startAmplitude - starting amplitude of the waveform.
*              resolution     - timing resolution of system [s]
*              rollOut        - roll out flag
*              *name          - name of file to be generated
*     Private: none
*     Public:  none
*  Output
*     Return:  error
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
ERROR_NUM_T writeToDisk (double * dataPoints, long numPoints,
                         double startAmplitude, double resolution,
                         int rollOut, char * name)
{
   long   i;             /* looping variable */
   double max;           /* holds maximum of dataPoint values */
   double scale;         /* scaling factor DAC values */
   FILE   *fileOut;      /* file handle for RF file */
   long   dacValue;      /* scaled DAC value */
   long   nextDacValue;  /* next value in array */
   long   sameCount = 1; /* counts the number of similar DAC values when not rolled out */
   char   path[MAX_STR]; /* data file */
   FILE   *fpout;        /* file handle */

   /* check slew rate of generated points */
   if(checkSlewRate(dataPoints,
                    numPoints,
                    startAmplitude,
                    resolution) != ERR_NO_ERROR)
   {
      displayError(ERR_SLEW_RATE, __FILE__, __FUNCTION__, __LINE__);
      return ERR_SLEW_RATE;
   }

/*  checkForFilename(name, path);*/ 
   strcpy(path, USER_DIR);      /* add user directory to path*/
   strcat(path, SHAPE_LIB_DIR); /* add shapelib directory to path */
   strcat(path, name);      /* add path and waveform name */
   strcat(path, ".GRD");        /* add suffix to complete path */

   /* open the file for read/write operations */
   if ((fpout = fopen(path, "w+")) == NULL)
   {
      displayError(ERR_FILE_OPEN, __FILE__, __FUNCTION__, __LINE__);
      return ERR_FILE_OPEN;
   }

   (void)gradAdmin(path, ADD); /* add file name for gradient shape to list */

   max = findMax(dataPoints, numPoints, startAmplitude); /* scan for maximum value */

	if( max == 0 ) {
		scale = 0; /* to deal with waveforms containing only zeroes */
	} else {
	   scale = (double)MAX_DAC_NUM / max; /* dataPoints must be scaled to highest DAC value */
	}

   if ((fileOut = fopen(path, "w+")) == NULL)
   {
      displayError(ERR_FILE_OPEN, __FILE__, __FUNCTION__, __LINE__);
      return ERR_FILE_OPEN;
   }

   /* write file header */
   fprintf(fileOut, "#\n");
   fprintf(fileOut, "# NAME:       %s\n", name);
   fprintf(fileOut, "# POINTS:     %li\n", numPoints);
   fprintf(fileOut, "# RESOLUTION: %1.8e\n", resolution);
   fprintf(fileOut, "# STRENGTH:   %1.8e\n", max);
   fprintf(fileOut, "#:\n");

   /* write data to file */
   for(i = 0; i < numPoints; i++)
   {
      dacValue = (int)ROUND(dataPoints[i] * scale);

      if(i < numPoints - 1) /* checks to see if we're on the last data point */
      {
         nextDacValue = (int)ROUND(dataPoints[i + 1] * scale);
      }
      else
      {
         /* we're on the last data point, make sure we don't compare */
         /* it to itself */
         nextDacValue = dacValue + 1;
      }

      if(rollOut)
      {
         /* each data point including duplicates are written to disk */
         fprintf(fileOut, "%li\t%i\n", dacValue, 1);
      }
      else
      {
         /* not rolled out, duplicate values are stored on the same line, */
         /* up to MAX_DAC_PTS */
         if(dacValue == nextDacValue) /* check for duplicate value */
         {
            if (sameCount == MAX_DAC_PTS) /* check for maximum count */
            {
               fprintf(fileOut, "%li\t%li\n", dacValue, sameCount);
               sameCount = 1;
            }
            else
            {
               sameCount++;
            }
         }
         else
         {
            fprintf(fileOut, "%li\t%li\n", dacValue, sameCount);
            sameCount = 1;
         }
      }
   }

   fclose(fileOut);
   fclose(fpout);   
   return ERR_NO_ERROR;
}

/***********************************************************************
*  Function Name: granularity
*  Example:       duration = granularity(dur, res);
*  Purpose:       Generate a duration that falls on a system time
*                 resolution tick.
*  Input
*     Formal:  duration   - time [s]
*              resolution - system time resolution [s]
*     Private: none
*     Public:  none
*  Output
*     Return:  double - granulated duration, ERR_GRANULARITY in case of
*                       error
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
double granularity (double duration, double resolution)
{
   double modulus;

   if (FP_EQ(duration, 0))
   {
      /* duration is zero, this is valid */
      return (0);
   }

   if (FP_LT(duration, 0))
   {
      /* duration is less than zero, return the duration passed in */
      /* let higher level functions deal with the negative */
      return (duration);
   }

   if (FP_LT(duration, resolution))
   {
      /* duration is less than resolution (but not zero), just return resolution */
      return (resolution);
   }

   /* get modulus */
   modulus = (duration / resolution) - floor(duration / resolution);

   if (FP_EQ(modulus, 0))
   {
      /* duration already falls on a resolution tick, nothing to do */
      return(duration);
   }
   else
   {
      /* duration isn't close to resolution tick */
      /* first case is that we're just under 1.0, ie 0.9999999999999 */
      /* in this case we subtract 1 and test again */
      if (FP_EQ(modulus - 1, 0))
      {
         /* duration already falls on a resolution tick, nothing to do */
         return(duration);
      }

      /* duration doesn't fall on resolution tick, raise it up to next resolution tick */
      return (floor(duration / resolution) * resolution + resolution);
   }

   displayError(ERR_GRANULARITY, __FILE__, __FUNCTION__, __LINE__);
   return ((double)ERR_GRANULARITY);
}
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/***********************************************************************
*  Function Name: calcPower
*  Example:       calcPower (RF_PULSE_T *rf_pulse);
*  Purpose:       Calculates the fine and corase RF power setting.
*  Input
*     Formal:  *rf_pulse - pointer to RF pulse structure
               *rfcoil   - pointer to RF coil name 
*     Private: none
*     Public:  CATTN_MAX - Maximum coarse attenuation
*               FATTN_MAX - maximum fine attenuation
*  Output
*     Return:  double - granulated duration, ERR_GRANULARITY in case of
*                       error
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void calcPower (RF_PULSE_T *rf_pulse, char rfcoil[MAX_STR])
{ 
   FILE   *fpin;
   char   calPath[MAX_STR];                       /* RF calibration filepath + name */
   char   coil[MAX_STR];                          /* RF coil name */
   float  calPulseWidth, calFlip, calPower;       /* calibration file entries */
   double power = 0;                              /* RF power */
   double powerFine = 0;                          /* RF power fine */
   double powerCoarse =0;                         /* RF power coarse */
   double attnStep = 0;                           /* attenuation step size */
   double flipangle;                              /* product of flip*flipmult */
   
   /* assign attenuation step size */
   if (CATTN_MAX < 127)
   {
      /* 0.5 dB step - volts */
      attnStep =20.0;
   }
   else
   {
      /* 1 dB step */
      attnStep = 40.0;
   }

   /* suppress display for high level function */
   rf_pulse->display = DISPLAY_NONE;

   flipangle = rf_pulse->flip*rf_pulse->flipmult;
 
   /* check for flip angle */
   if (FP_EQ(flipangle,0.0))
      {
      warn_message("Warning: flip angle or multiplier are zero for one or more pulses");
      rf_pulse->powerCoarse = -16;
      rf_pulse->powerFine   =   0;
      return;
      }

// always do the getvals
// then if integral is 0 below, return, which will use getvals
// make this sglpower
// 1 return
// 0 skips this section
      
   /* Read coarse and fine power levels from VnmrJ parameters */
   if (strcmp(rf_pulse->param1, "")) 
     rf_pulse->powerCoarse = getval(rf_pulse->param1);
   else
     rf_pulse->powerCoarse = -16;
  
   if (strcmp(rf_pulse->param2, "")) 
     rf_pulse->powerFine   = getval(rf_pulse->param2);
   else
     rf_pulse->powerFine   = 0;

   /* If flip<0 or sglpower=1 return and use power levels from VnmrJ parameters */
   if (FP_LT(rf_pulse->flip,0.0) || (sglpower == 1)) 
      return;
 
   /* read RF pulse header */
   readRfPulse(rf_pulse);   
   
   /* assemble path and name to RF calibration file */
   strcpy(calPath,USER_DIR);                 /* add userdir to path */
   strcat(calPath,RF_CAL_FILE);             /* add file name to pathh */

   /* check if RF calibration file is in user directory */
   /* if file is not in user directory look in system directory */
   if ((fpin = fopen(calPath, "r")) == NULL)
   {
      strcpy(calPath, SYSTEMDIR);     /* add system directory to path*/
      strcat(calPath, RF_CAL_FILE);   /* add file name to path */

      /* check if RF pulse file is in user directory */
      if ((fpin = fopen(calPath, "r")) == NULL)
      {
         /* Terminate if file is in neither directory */
         rf_pulse->error = ERR_RF_CALIBRATION_FILE_MISSING;
         displayError(rf_pulse->error, __FILE__, __FUNCTION__, __LINE__);
         return;
      }
   }
  
  /* find coil entry */ 
   do
   {
      fscanf(fpin, "%s", coil);  
   }    
   while ( (strcmp(rfcoil,coil) ) &&  (!feof(fpin)) );
   if (feof(fpin))
   {
      /* Terminate if no entry for coil in file */
      rf_pulse->error = ERR_RF_COIL;
      displayError(rf_pulse->error, __FILE__, __FUNCTION__, __LINE__);
      return;
   }
   /* read calibration entries */
   fscanf(fpin,"%f %f %f ",&calPulseWidth,&calFlip,&calPower);
 
   /* close RF calibration file */  
   fclose(fpin);
   /* convert values to correct dimensions */
   calPulseWidth *= US_TO_S; 
     
   /* Calculate pulse power */
   
   if (FP_LTE(rf_pulse->header.integral,0))
   {
      return;
   }
   else
   {
      /* determine power */
      power = (calPulseWidth /rf_pulse->rfDuration ) / rf_pulse->header.integral;

      /* adjust for flip angle */
      power = flipangle/calFlip*power;

      /* log is natural log */
      power= log(power);
      power = calPower + attnStep * power/log(10);
   }
   
   /*calculate coarse and fine power level */
   powerCoarse = ceil(power);
   powerFine = exp( (powerCoarse - power) / (-attnStep)*log(10) ) * FATTN_MAX;

   /* Round fine power for 16-bit DD2 resolution */
   /* Ideally we would not do this for VNMRS, but a conditional mechanism does not yet exist */ 
   powerFine*=16;
   powerFine = ROUND(powerFine); /* round to nearest integer */
   powerFine/=16;

   /* Check is result is valid */
   if ( (FP_GT(powerFine,FATTN_MAX)) ||
        (FP_GT(powerCoarse,CATTN_MAX)) )
   {
      /* reset RF power values in case or error */
      rf_pulse->powerCoarse = 0.0;
      rf_pulse->powerFine = 0.0;
      
      /* display error message */
      if (FP_GT(powerFine,FATTN_MAX))
      {
	     rf_pulse->error = ERR_RFPOWER_FINE;
	     displayError(rf_pulse->error, __FILE__, __FUNCTION__, __LINE__);
	     return;
      }
      if (FP_GT(powerCoarse,CATTN_MAX))
      {
	    rf_pulse->error = ERR_RFPOWER_COARSE;
	    displayError(rf_pulse->error, __FILE__, __FUNCTION__, __LINE__);
	    return;
      }
   }
   
   /* assign power values */
   rf_pulse->powerCoarse = powerCoarse;
   rf_pulse->powerFine = powerFine;
   
   /* Display RF for debugging */
   if (sgldisplay == DISPLAY_LOW_LEVEL)
      displayRf(rf_pulse);
   
   return;      
} 


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*-------------- E N D    H E L P E R    F U N C T I O N S -------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/



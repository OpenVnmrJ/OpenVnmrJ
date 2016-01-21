/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
 
/* Pbox_psg.h - See comments in Pbox_psg.c for details. */

#ifndef PBOX_PSG_H
#define PBOX_PSG_H


#ifndef MAXSTR
#define MAXSTR 512
#endif
#ifndef MAXGSTEPS
#define MAXGSTEPS 512
#endif
#ifndef FIRST_FID
#define FIRST_FID ((getval("arraydim") < 1.5) || (ix==1))   
#endif

typedef struct   {
  int     np, ok;
  double  pw, pwr, pwrf, dres, dmf, dcyc, B1max, af;
  char    name[MAXSTR];
} shape;

/* Pbox_psg variables */
extern int    ipx;    /* shape counter */
extern int    px_debug;
extern double pbox_pw, pbox_pwr, pbox_pwrf, pbox_dres, pbox_dmf;
extern char   pbox_name[MAXSTR], px_cmd[2048], px_opts[2048];

/* Pbox_psg function definitions */
extern void fixname(char *nm);
extern void pbox_pulse(shape *rshape, codeint iph, double del1, double del2);
extern void pbox_xmtron(shape *dshape);		
extern void pbox_xmtroff();		
extern void pbox_spinlock(shape *dshape, double mixdel, codeint iph);
extern void pbox_decpulse(shape *rshape, codeint iph, double del1, double del2);
extern void pbox_simpulse(shape *rshape, shape *rshape2,
                   codeint iph, codeint iph2, double del1, double del2);
extern void pbox_decon(shape *dshape);		
extern void pbox_decoff();		
extern void pbox_decspinlock(shape *dshape, double mixdel, codeint iph);
extern void pbox_dec2pulse(shape *rshape, codeint iph,
                           double del1, double del2);
extern void pbox_sim3pulse(shape *rshape, shape *rshape2, shape *rshape3,
                    codeint iph, codeint iph2, codeint iph3,
                    double del1, double del2);
extern void pbox_dec2on(shape *dshape);		
extern void pbox_dec2off();		
extern void pbox_dec2spinlock(shape *dshape, double mixdel, codeint iph);
extern void pbox_dec3pulse(shape *rshape, codeint iph,
                           double del1, double del2);
extern void pbox_dec3on(shape *dshape);		
extern void pbox_dec3off();		
extern void pbox_dec3spinlock(shape *dshape, double mixdel, codeint iph);
extern void shaped_gradient(int nstp, double glvl, double gw, char axis); 
extern shape getGsh(char *shname);
extern shape getGshape(char *shname);
extern void pbox_grad(shape *gshape, double glvl, double gof1, double gof2);
extern void pbox_xgrad(shape *gshape, double gw, double glvl,
                double gof1, double gof2);
extern void pbox_ygrad(shape *gshape, double gw, double glvl,
                double gof1, double gof2);
extern void pbox_zgrad(shape *gshape, double gw, double glvl,
                double gof1, double gof2);
extern shape getRsh(char *shname);
extern shape getDsh(char *shname);
extern shape getRshape(char *shname);
extern shape getDshape(char *shname);
extern void pbox_get();
extern void setlimit(const char *name, double param, double limt);
extern void pfg_pulse(double glvl, double gw, double gof1, double gof2);
extern void homodec(shape *decseq);
extern void pre_sat();
extern void presat();
extern void new_name(char *name);
extern int isarry(char *arrpar);
extern void opx(char *name);
extern void setwave(char *wave);
extern void putwave(char *sh, double bw, double ofs, double st,
             double pha, double fla);
extern void pboxpar(char *pxname, double pxval);
extern void pbox_par(char *pxname, char *pxval);
extern void pboxSpar(char *pxname, char *pxval);
extern void pboxUpar(char *pxname, double pxval, char *pxunits);
extern void cpx(double rfpw90, double rfpwr);
extern shape pbox_inp(char *file_name, double rfpw90, double rfpwr);
extern shape pbox_shape(char *shn, char *wvn, double pw_bw,
                 double ofs, double rf_pw90, double rf_pwr); 
extern shape pboxAshape(char *shn, char *wvn, double bw, double pw,
                 double ofs, double rf_pw90, double rf_pwr); 
extern shape  pbox_mix(char *shp, char *mixpat, double mixpwr,
                double refpw90, double refpwr);
extern shape  pbox_ad180(char *shp, double refpw90, double refpwr);
extern shape  pbox_ad180b(char *shp, double refpw90, double refpwr);

/* Pbox_HT function definitions */
extern shape pboxHT_dec(char *pwpat, const char *pshape, double jXH,
            double bw, double ref_pw, double ref_comp, double ref_pwr);
extern shape pboxHT();
extern shape pboxHT_F1();
extern shape pboxHT_F2();
extern shape pboxHT_F3();

extern shape pboxHT_F1e();
extern shape pboxHT_F1i();
extern shape pboxHT_F1s();
extern shape pboxHT_F1r();
extern shape pboxHT_F2e();
extern shape pboxHT_F2i();
extern shape pboxHT_F2s();
extern shape pboxHT_F2r();
extern shape pboxHT_F3e();
extern shape pboxHT_F3i();
extern shape pboxHT_F3s();
extern shape pboxHT_F3r();
extern shape pboxHTe();
extern shape pboxHTi();
extern shape pboxHTs();
extern shape pboxHTr();

#endif


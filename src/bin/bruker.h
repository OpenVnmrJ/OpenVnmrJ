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
/* The these data definitions and the skip and header sizes correspond to what
   is found in data transferred from an Aspect (running DISNMR under ADAKOS)
   to a Sun via Bruknet (versions ca. early 1993).  Most of the header field
   definitions come from the Pascal program NMRPAR.PAS (Leo Joncas, Bruker,
   Billerica) and is noted there as being from DISNMR 880101.1.  The
   definition of Bruker floating point wasn't in our documentation; I had to
   dope it out by trial and error.  I have not seen the definition of the
   Bruker sixbit character set anywhere, so I've made up a definition that
   acceptably translates all the examples I've seen.  Please let me know if
   any changes are needed.
   STM
*/

#define BRUKER_PARNAME	"bruker"		/* parlib filename */
#define BRUKER_TEXT	"Bruker/Aspect data"	/* since text not stored (?) */

/* Bytes of directory info at start of file.  This consists of 16 words (48
   bytes) of file definition table and 24 words (72 bytes) of commentary.
   This is present in both Bruknet and serial-dumped data.
*/
#define BRUKER_DIRSIZE	120

/* Byte offset of filename within directory info. */
#define B_FDT_filename	3

/* Size of the "-1 sector" containing parameters.  Only the first part of this
   is defined.  The rest is padding to fill out to a full sector (768 bytes
   or 256 words).  (Serial-dumped data did not have the padding.)
*/
#define BRUKER_HEADSIZE	648	/* 186 3-byte-words, then 90 bytes of padding */

/* convert signed 3-byte-int to signed 4-byte-int.  i is new int (lval); b is
   an unsigned char pointer  pointing to the 3-byte-int. */
#define intBINT(i,b) { i = (int)(b)[0]<<16 | (int)(b)[1]<<8 | (int)(b)[2]; \
                       if ((b)[0]&0x80) i |= 0xFF000000; }

/* These are data locations relative to the beginning of the "-1 sector"
   (not necessarily the file).  All items are 3-byte integers unless marked
   otherwise:  Bfloats are 6 bytes, and Bchars (I assume) are just 3-byte
   integers from which one character is to be extracted.  The offsets are
   multiplied by 3 to convert them to byte offsets.
*/
#define B_asize		3*0	/* data size in file (only after transform?) */
#define B_globex	3*1	/* data normalization constant */
#define B_swpcom	3*2	/* completed sweeps */
#define B_tdsize	3*3	/* time domain size */
#define B_dwellt	3*4	/* half-dwell for sequential acquisition */
#define B_fwidth	3*5	/* filter full width */
#define B_fn		3*6	/* filter number */
#define B_f1n		3*7	/* O1 divider settings */
#define B_f1m		3*8
#define B_f2n		3*9	/* O2 divider settings */
#define B_f2m		3*10
#define B_vdelay	3*11	/* variable delay */
#define B_dscans	3*12	/* dummy scans */
#define B_spwidth	3*13	/* (Bfloat) full spectral width */
#define B_aqmode	3*15	/* acquisition mode */
#define B_texp		3*16	/* total expts (caution:  may be nonzero even if not used) */
#define B_o1		3*17	/* (Bfloat) observe freq */
#define B_o2		3*19	/* (Bfloat) decouple freq */
#define B_dummy1	3*21
#define B_dummy2	3*22
#define B_totlb		3*23	/* (Bfloat) total EM applied (Hz) */
#define B_nthpt		3*25	/* (Bfloat) Nth point of FT */
#define B_crdel		3*27	/* relaxation delay timer word */
#define B_cpulse	3*28	/* pulse width timer word */
#define B_cdelay	3*29	/* preacquisition delay timer word */
#define B_csweep	3*30	/* sweeps to do */
#define B_drstr		3*31	/* digital resolution */
#define B_drend		3*32	/* lowest allowed drstr */
#define B_drset		3*33	/* largest allowed drstr */
#define B_cspdel	3*34	/* initial SP delay timer word */
#define B_phzflg	3*35	/* A0-A3 phase shift */
#define B_origps	3*36	/* original parameters flag */
#define B_sfreq		3*37	/* (Bfloat) spectrometer frequency */
#define B_quaflg	3*39	/* quadrature flag */
#define B_dctrlw	3*40	/* digitizer control word */
#define B_datten	3*41	/* decoupler attenuation */
#define B_deutl		3*42	/* deuterium lock setting */
#define B_harflg	3*43	/*  */
#define B_paps		3*44	/*  */
#define B_temper	3*45	/* temperature */
#define B_datastat	3*46	/*  */
#define B_cpcur		3*47	/*  */
#define B_rgain		3*48	/* receiver gain */
#define B_syncw1	3*49	/* synthesizer control words */
#define B_syncw2	3*50
#define B_imzyaxis	3*51	/*  */
#define B_imflags	3*52	/*  */
#define B_yxratio	3*53	/*  */
#define B_pd0inc	3*54	/* increment for DUR0 */
#define B_parset	3*55	/* parameter set identifier */
#define B_syncw3	3*56
#define B_syncw4	3*57
#define B_sfreq0	3*58	/* (Bfloat)  */
#define B_fx2		3*60	/* (Bfloat) initial O2 divider (was dfrq?) */
#define B_dfx1		3*62	/* divider increment */
#define B_saunm1	3*63	/* 9 bytes (12 chars) of sixbit with 
				   0x40 for . and 0x80 for blank */
#define B_saunm2	3*64
#define B_sauext	3*65
#define B_dummy3	3*66
#define B_f1cw		3*67	/* offset freq control word */
#define B_f2cw		3*68	/* decoupler control word */
#define B_dur0		3*69	/* delay timer words */
#define B_dur1		3*70
#define B_dur2		3*71
#define B_dur3		3*72
#define B_dur4		3*73
#define B_dur5		3*74
#define B_dur6		3*75
#define B_dur7		3*76
#define B_dur8		3*77
#define B_dur9		3*78
#define B_dps0		3*79	/* decoupler power settings */
#define B_dps1		3*80
#define B_dps2		3*81
#define B_dps3		3*82
#define B_dps4		3*83
#define B_dps5		3*84
#define B_dps6		3*85
#define B_dps7		3*86
#define B_dps8		3*87
#define B_dps9		3*88
#define B_plsw0		3*89	/* pulse width timer words */
#define B_plsw1		3*90
#define B_plsw2		3*91
#define B_plsw3		3*92
#define B_plsw4		3*93
#define B_plsw5		3*94
#define B_plsw6		3*95
#define B_plsw7		3*96
#define B_plsw8		3*97
#define B_plsw9		3*98
#define B_d9var		3*99	/* max percent variation in DUR9 */
#define B_rofreq	3*100	/* rotation freq */
#define B_fmflag	3*101	/* frequency mode flag */
#define B_fixflg	3*102	/*  */
#define B_p2doff	3*103	/* offset to 2D parameters */
#define B_vdscale	3*104	/*  */
#define B_acqdate	3*105	/* date of acquisition */
#define B_acqtime	3*106	/* time of acquisition */
#define B_twodflag	3*107	/*  */
#define B_xsa		3*108	/* current plot area */
#define B_xsize		3*109	/*  */
#define B_svsa		3*110	/* most recently expanded area */
#define B_svsize	3*111	/*  */
#define B_hzpmfg	3*112	/* Hz/ppm flag for EP freqs */
#define B_freq1		3*113	/* (Bfloat) left freq limit of plot */
#define B_freq2		3*115	/* (Bfloat) right freq limit of plot */
#define B_offset	3*117	/* (Bfloat) offset of spectrum */
#define B_rfreq		3*119	/* (Bfloat) spectrum reference freq */
#define B_hzpcm		3*121	/* (Bfloat) Hz/cm of plot area */
#define B_idscale	3*123	/* integral scaling factor */
#define B_intscl	3*124	/* (Bfloat) integral norm factor */
#define B_svthta	3*126	/* 0 order phase angle */
#define B_svaddi	3*127	/* 1st order phase angle */
#define B_minint	3*128	/* peak picking min intensity */
#define B_ppsign	3*129	/* peaks allowed flag */
#define B_pcons		3*130	/* (Bfloat) peak picking sensitivity */
#define B_pvsize	3*132	/* (Bfloat) distance between marks in Hz */
#define B_pvhzpm	3*134	/* Hz or ppm flag for PV */
#define B_marksz	3*135	/* (Bfloat) size of mark in cm */
#define B_pvoff		3*137	/* (Bfloat) PV offset from 0 in cm */
#define B_ymax		3*139	/* intensity of largest point */
#define B_aiflag	3*140	/* absolute intensity scaling */
#define B_ainorm	3*141	/* normalization factor for AI */
#define B_intfac	3*142	/* factor for AZF integral level */
#define B_dummy4	3*143
#define B_fp0k		3*144	/* (Bfloat) true 0-order phase (was 145?) */
#define B_fp1k		3*146	/* (Bfloat) true 1st-order phase (was 147?) */
#define B_lbroad	3*148	/* (Bfloat) Lorentzian line broadening */
#define B_gbroad	3*150	/* (Bfloat) Gaussian line broadening */
#define B_wttype	3*152	/* (Bchar?) window table type */
#define B_sbshft	3*153	/* sine-bell shift */
#define B_zpnum		3*154	/* points to zero at start */
#define B_t1pt		3*155	/* trapezoidal mult left limit */
#define B_t2pt		3*156	/* trapezoidal mult right limit */
#define B_smctr		3*157	/* smoothing count */
#define B_gamma		3*158	/* (Bfloat) amplitude imbalance */
#define B_alpha		3*160	/* (Bfloat) phase error term */
#define B_vchty		3*162	/* (Bfloat) variable height of plot */
#define B_vchtx		3*164	/* (Bfloat) variable length of plot */
#define B_poycm		3*166	/* (Bfloat) y plot offset in cm */
#define B_poxcm		3*168	/* (Bfloat) x plot offset in cm */
#define B_penloc	3*170	/* initial plot position */
#define B_penloc2	3*171	/* plot types */
#define B_dpspd		3*172	/* speed for data plots */
#define B_pvspd		3*173	/* plot grid speed */
#define B_dpopt		3*174	/* DP option specifier */
#define B_dummy5	3*175
#define B_tobloc	3*176	/* dest block for transfer */
#define B_dcon		3*177	/* (Bfloat) constant for additive transform */
#define B_t1conf	3*179	/* (Bfloat)  */
#define B_t1drift	3*181	/*  */
#define B_cexp		3*182	/*  */
#define B_nfids		3*183	/*  */
#define B_plohei	3*184	/* (Bfloat) plot height */

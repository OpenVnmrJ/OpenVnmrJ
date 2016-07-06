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
//=========================================================================
// DDR_Acq.h:  DDR_Acq.c API
//=========================================================================
#ifndef DDR_ACQ_H
#define DDR_ACQ_H

#define DDR_DEBUG_LVL -3

//#define USE_LOGPRNT // if defined, copies output to serial port window

#ifdef USE_LOGPRNT
#define PRINT0(x)				logMsg(x"\n",0,0,0,0,0,0)
#define PRINT1(x,a) 			logMsg(x"\n",a,0,0,0,0,0)
#define PRINT2(x,a,b) 			logMsg(x"\n",a,b,0,0,0,0)
#define PRINT3(x,a,b,c) 		logMsg(x"\n",a,b,c,0,0,0)
#define PRINT4(x,a,b,c,d) 		logMsg(x"\n",a,b,c,d,0,0)
#define PRINT5(x,a,b,c,d,e) 	logMsg(x"\n",a,b,c,d,e,0)
#define PRINT6(x,a,b,c,d,e,f) 	logMsg(x"\n",a,b,c,d,e,f)
#else
#define PRINT0(x)				DPRINT(DDR_DEBUG_LVL,x"\n")
#define PRINT1(x,a) 			DPRINT1(DDR_DEBUG_LVL,x"\n",a)
#define PRINT2(x,a,b) 			DPRINT2(DDR_DEBUG_LVL,x"\n",a,b)
#define PRINT3(x,a,b,c) 		DPRINT3(DDR_DEBUG_LVL,x"\n",a,b,c)
#define PRINT4(x,a,b,c,d) 		DPRINT4(DDR_DEBUG_LVL,x"\n",a,b,c,d)
#define PRINT5(x,a,b,c,d,e) 	DPRINT5(DDR_DEBUG_LVL,x"\n",a,b,c,d,e)
#define PRINT6(x,a,b,c,d,e,f) 	DPRINT6(DDR_DEBUG_LVL,x"\n",a,b,c,d,e,f)
#endif

// DDR vxWorks functions  (in DDR_Acq.c)

extern int ddr_wait(int stat,int s);  					// wait for status bit
extern int ddr_status();  					            // return current status
extern int ddr_error();  					            // return error status
extern void ddr_set_debug(int m,int i);

extern int ddr_set_exp(int *args);  							
extern int ddr_set_scan(int *args);  							
extern void markTime(long long *dat);
extern double diffTime(long long m2,long long m1);

extern double ddr_get_aqtm();
extern double ddr_get_sw();

// DDR parser functions  (in DDR_Acq.c)

extern void ddr_init();  								// DDR_INIT
extern void ddr_set_sr(double sa,double sd);  			// DDR_SET_SR
extern void ddr_set_sw1(double sw);                     // DDR_SET_SW
extern void ddr_set_os1(double os);                     // DDR_SET_OS
extern void ddr_set_xs1(double xs);                     // DDR_SET_XS
extern void ddr_set_fw1(double fw);                     // DDR_SET_FW
extern void ddr_set_sw2(double sw);                     // DDR_SET_SW
extern void ddr_set_os2(double os);                     // DDR_SET_OS
extern void ddr_set_xs2(double xs);                     // DDR_SET_XS
extern void ddr_set_fw2(double fw);                     // DDR_SET_FW
extern void ddr_set_mode(int mode, int b, int s);       // DDR_SET_MODE
extern void ddr_set_dims(int nacms, int dl, int al);  	// DDR_SET_DIMS
extern void ddr_set_xp(int nf, int nfmod, int xp);  	// DDR_SET_XP
extern void ddr_set_rp(double p);  						// DDR_SET_RP
extern void ddr_set_rf(double f);  						// DDR_SET_RF
extern void ddr_set_cp(double p);  						// DDR_SET_CP
extern void ddr_set_ca(double f);  						// DDR_SET_CA
extern void ddr_set_cf(double f);  						// DDR_SET_CF
extern void ddr_set_acm(int src, int dst, int nt);  	// DDR_SET_ACM
extern void ddr_set_pfaves(int n);  	                // DDR_PFAVES
extern void ddr_push_fid(int id, int flags);            // DDR_PUSH_FID
extern void ddr_push_scan(int id, int flags);           // DDR_PUSH_SCAN
extern void ddr_init_exp(int tt,int sf,int df);  	    // DDR_INIT_EXP
extern void ddr_start_exp(int mode);  				    // DDR_START_EXP
extern void ddr_stop_exp(int mode);  					// DDR_STOP_EXP
extern void ddr_resume_exp(int mode);  					// DDR_RESUME_EXP
extern void ddr_unlock_acm(int acm, int opt);  			// DDR_UNLOCK
extern void ddr_ss_req(int mode);  			            // DDR_SS_REQ
extern void ddr_test(int id, int a, int b);  		    // DDR_TEST
extern void ddr_sim_peaks(int id);  	                // DDR_SIM_PEAKS
extern void ddr_sim_ascale(double f);  	                // DDR_SIM_ASCALE
extern void ddr_sim_fscale(double f);  	                // DDR_SIM_FSCALE
extern void ddr_sim_lw(double f);  	                    // DDR_SIM_LW
extern void ddr_sim_noise(double f);  	                // DDR_SIM_NOISE
extern void ddr_sim_rdly(double f);  	                // DDR_SIM_RDLY
extern void ddr_peak_freq(int id,double f);  	        // DDR_PEAK_FREQ
extern void ddr_peak_ampl(int id,double f);  	        // DDR_PEAK_AMPL
extern void ddr_peak_width(int id,double f);  	        // DDR_PEAK_WIDTH

extern void host_error(int id, int arg1, int arg2);     // HOST_ERROR
#endif

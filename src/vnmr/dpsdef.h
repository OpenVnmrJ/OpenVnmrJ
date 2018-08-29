/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#undef TODEV
#undef DODEV
#undef DO2DEV
#undef DO3DEV
#undef DO4DEV
#undef DO5DEV

#define  TODEV    1
#define  DODEV    2
#define  DO2DEV   3
#define  DO3DEV   4
#define  DO4DEV   5
#define  DO5DEV   6
#define  RCVRDEV  7 
#define  GRADDEV  8
#define  GRADX    8 
#define  GRADY    9 
#define  GRADZ    10 
#define  ACQDEV   11 

#define  TOTALCH  12

#define  RFCHAN_NUM  7


#define  ACQUIRE     	1
#define  AMPBLK      	2
#define  APOVR       	3
#define  APTABLE     	4
#define  APSHPUL     	5
#define  BEGIN       	6
#define  CLRAPT      	7
#define  DECLVL      	8
#define  DECPWR      	9
#define  DELAY       	10
#define  DEVON       	11
#define  DUMMY       	12
#define  ELSENZ      	13
#define  ENDHP       	14
#define  ENDIF       	15
#define  ENDSP       	16
#define  ENDSISLP    	17
#define  GETELEM     	18
#define  GRAD        	19
#define  HKDELAY     	20
#define  HLOOP       	21
#define  HSDELAY     	22
#define  IFZERO      	23
#define  INCDLY      	24
#define  INITDLY     	25
#define  INITSCAN    	26
#define  INCGRAD     	27
#define  LKHLD       	28
#define  LKSMP       	29
#define  MSLOOP      	30
#define  OBLSHGR     	31
#define  OBLGRAD     	32
#define  OFFSET      	33
#define  PE2SHGR     	34
#define  PE3SHGR     	35
#define  PESHGR      	36
#define  PELOOP      	37
#define  PEGRAD      	38
#define  PE2GRAD     	39
#define  PE3GRAD     	40
#define  PHASE       	41
#define  PHSTEP      	42
#define  POWER       	43
#define  POFFSET     	44
#define  PRGOFF      	45
#define  PRGON       	46
#define  PSHIFT      	47
#define  PULSE       	48
#define  PWRF        	49
#define  PWRM        	50
#define  RADD        	51
#define  RASSIGN     	52
#define  RCVRON      	53
#define  RDBL        	54
#define  RDECR       	55
#define  RDIVN       	56
#define  RGRAD       	57
#define  RHLV        	58
#define  RINCR       	59
#define  RLPWR       	60
#define  RLPWRF      	61
#define  RLPWRM      	62
#define  RMOD2       	63
#define  RMOD4       	64
#define  RMODN       	65
#define  RMULT       	66
#define  ROTORP      	67
#define  ROTORS      	68
#define  RSUB        	69
#define  RVOP        	70
#define  SETOBS      	71
#define  SETRCV      	72
#define  SETST       	73
#define  SETVAL      	74
#define  SH2DVGR     	75
#define  SHGRAD      	76
#define  SHPUL       	77
#define  SHVGRAD     	78
#define  SH2DGRAD    	79
#define  SHINCGRAD   	80
#define  SLOOP       	81
#define  SMPUL       	82
#define  SMSHP       	83
#define  SPON        	84
#define  SPHASE      	85
#define  SPINLK      	86
#define  STATUS      	87
#define  VDELAY      	88
#define  VFREQ       	89
#define  VGRAD       	90
#define  VOFFSET     	91
#define  VSCAN       	92
#define  XGATE       	93
#define  ZGRAD       	94
#define  TBLOP		95
#define  TSOP		96
#define  MGPUL          97
#define  MGRAD          98
#define  LOFFSET     	99
#define  VDLST      	100
#define  CDLST      	101
#define  EXPTIME      	102
#define  GLOOP      	103

#define  CMARK      	104
#define  CBACK      	105
#define  CEND      	106
#define  CPULSE      	107
#define  CDELAY      	108
#define  CSHPUL      	109
#define  CGRAD      	110
#define  CGPUL      	111
#define  PEOBLVG      	112
#define  PEOBLG      	113
#define  PARALLEL      	114
#define  BGSHIM      	115
#define  SHSELECT	116
#define  HDSHIMINIT	117
#define  GRADANG	118
#define  SHVPUL       	119
#define  ROTATE       	120
#define  JPSGIF       	121
#define  DUP       	122
#define  DCPLON         123  /* decoupleron */
#define  DCPLOFF        124  /* decoupleroff */
#define  XDOCK          125  /* dummy dock obj */
#define  JFID       	126
#define  XRLPWR       	127  /* for pbox routines */
#define  XRLPWRF       	128
#define  XPRGON       	129
#define  XPRGOFF       	130
#define  XDEVON       	131
#define  XTUNE       	132   /* set4Tune */
#define  XMACQ       	133   /* XmtNAcquire */
#define  SHACQ       	134   /* ShapedXmtNAcquire */
#define  SPACQ       	135   /* SweepNAcquire */
#define  OFFSHP       	136   /* genRFShapedPulseWithOffset */
#define  ACQ1     	137   /* startacq  */
#define  TRIGGER     	138   /* triggerSelect */
#define  RDBYTE     	139   /* readMRIUserByte */
#define  WRBYTE     	140   /* writeMRIUserByte */
#define  SETGATE     	141   /* setMRIUserGates */
#define  VRLPWRF      	142   /* vobspwrf */
#define  IFRT      	143   /* ifrtGE, ifrtGT,... */
#define  PELOOP2      	144
#define  ROTATEANGLE  	145   // rot_angle_list
#define  GRPPULSE  	146   // GroupPulse
#define  INITGRPPULSE  	147   // initRFGroupPulse
#define  GRPNAME  	148   // modifyRFGroupName
#define  GRPPWRF  	149   // modifyRFGroupPwrf
#define  GRPPWR  	150   // modifyRFGroupPwrC
#define  GRPONOFF  	151   // modifyRFGroupOnOff
#define  GRPSPHASE  	152   // modifyRFGroupSPhase
#define  GRPFRQLIST  	153   // modifyRFGroupFreqList
#define  INITFRQLIST  	154   // initRFGroupPulse
#define  VPRGON       	155   // obsprgonOffset, decprgonOffset
#define  SHLIST       	156   // shapelist
#define  SHPLSLIST     	157   // shapedpulselist
#define  SAMPLE     	158   // sample
#define  SETANGLE     	159   // set_angle_list
#define  EXECANGLE     	160   // exe_grad_rotation
#define  DPESHGR     	161   // pe_shaped3gradient_dual
#define  DPESHGR2     	162   // 2nd part of pe_shaped3gradient_dual
#define  ACTIVERCVR   	163   // setactivercvrs
#define  PARALLELSTART  164   // parallelstart
#define  PARALLELSYNC   165   // parallelsync
#define  PARALLELEND    166   // parallelend
#define  RLLOOP         167   // rlloop
#define  RLLOOPEND      168   // rlendloop
#define  KZLOOP         169   // kzloop
#define  KZLOOPEND      170   // kzendloop
#define  NWLOOP         171   // nwloop
#define  NWLOOPEND      172   // endnwloop

#define  LASTELEM    	172

#define  XEND       	300

#define  RFPWR    	900
#define  RTADDR    	901
#define  RFID   	902
#define  NEWACQ    	903
#define  VERNUM    	910
#define  FARRAY    	911
#define  SARRAY    	912
#define  DARRAY    	913
#define  ARYDIM    	914
#define  PARRAY    	915
#define  OARRAY    	916
#define  LISTDEV    	918
#define  APTBL    	920
#define  XARRAY    	921
#define  DPSVER    	922
#define  ROTATELIST    	923  // create_rotation_list
#define  ANGLELIST    	924  // create_angle_list
#define  DPSNUM    	1

#define  DOCK    	1000

/* real time variables */

#define    NPR_PTR      1
#define    NTRT         2
#define    CT           3
#define    CTSS         4
#define    OPH          5
#define    BSVAL        6
#define    BSCTR        7
#define    SSVAL        8
#define    SSCTR        9
#define    RTTMP        10
#define    SPARE1RT     11
#define	   ID2		12
#define	   ID3		13
#define	   ID4		14

#define    ZERO         15
#define    ONE          16
#define    TWO          17
#define    THREE        18
#define	   SS		19
#define    IX      	20

/*
#define    VSPARE1      30
#define    VSPARE5      34
*/

#define    V1           21

#define    T1           86
#define    RTMAX        87

#define    OLDCT        15  /* old definition of CT */
#define    OLDT1        35  /* old definition of T1 */

/* rt operation, from ACode32.h */
#define    RT_LT      40
#define    RT_GT      41
#define    RT_GE      42
#define    RT_LE      43
#define    RT_EQ      44
#define    RT_NE      45

/* define file names */
#define    DPS_DATA  "dpsdata"
#define    DPS_TIME  "dpstime"
#define    DPS_TABLE "dpstable"
#define    DPS_ARRAY "dpsarraydata"


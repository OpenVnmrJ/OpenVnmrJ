/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  >>>>>>>>>>>>>>>>>>>>>>>   S T O P !   <<<<<<<<<<<<<<<<<<<<<<<<<<<
 *
 *  If you changed this file did you make the appropriate 
 *   changes to acqerrmsges.h (in vnmr) ?
 *              -------------
 */
/* ---------------------------------------------------------------
|
|	Acquisition Error Codes returned to Vnmr
|
+----------------------------------------------------------------*/

#define ABORTERROR	16
/* ---    Warnings  --- */
#define WARNINGS	100
#define LOWNOISE	1
#define HIGHNOISE	2
#define ADCOVER		3
#define RECVOVER	4
#define VGRADOVER	5
#define APRTOVER	6
#define FIFOMISSING	7
#define STMMISSING	8
#define ADCMISSING	9
#define AUTOMISSING	10
#define AUTO_NOTBOOTED	11
#define MEM_ALLOC_ERR	12
#define APREADFAIL	13
#define MISSINGCNTLRS	14
#define DATAOVF	   15
#define GRAD_SLEW_EXCEEDED 16


/* ---    Soft Errors --- */
#define SFTERROR	200
#define MAXCT 		0
#define LOCKLOST 	1
#define STOPACQ 	2
#define GTABERR		3
#define GTABNOPTR       4
#define ATTNADDR	5
#define READBIT  	6

#define SPINERROR	300
#define BUMPFAIL	1
#define RSPINFAIL	2
#define SPINOUT		3
#define SPINUNKNOWN	95
#define SPINNOPOWER	96
#define SPINRS232	97
#define SPINTIMEOUT	98

#define VTERROR		400
#define VTREGFAIL	0
#define VTOUT		1
#define VTMANUAL	2
#define VTSSLIMIT	3
#define VTNOGAS		4
#define VTMSONBOT	5
#define VTMSONTOP	6
#define VT_SC_SS	7
#define VT_OC_SS	8
#define VTUNKNOWN	95
#define VTNOPOWER	96
#define VTRS232		97
#define VTTIMEOUT	98

#define SMPERROR	500
/* --- changer error 1 - 15   add 20 to all error for loading sample */
#define RTVNOSAMPLE	1
#define RTVARMUPFAIL	2
#define RTVARMDOWNFAIL	3
#define RTVARMMIDFAIL	4
#define RTVBADNUMBER	5
#define RTVBADTEMP	6
#define RTVGRIPABORT	7
#define RTVRANGERR	8
#define RTVBADCMDCHAR	9
#define RTVHOMEFAIL	10
#define RTVNOTRAY	11
#define RTVPOWERFAIL	12
#define RTVBADCOMMAND	13
#define RTVGRIPOPEN	14
#define RTVAIRSUPPLY	15
#define RTVNOSAMPNODETECT 16	/* console generate errorcode */
#define INSNOSAMPLE	21
#define INSARMUPFAIL	22
#define INSARMDOWNFAIL	23
#define INSARMMIDFAIL	24
#define INSBADNUMBER	25
#define INSBADTEMP	26
#define INSGRIPABORT	27
#define INSRANGERR	28
#define INSBADCMDCHAR	29
#define INSHOMEFAIL	30
#define INSNOTRAY	31
#define INSPOWERFAIL	32
#define INSBADCOMMAND	33
#define INSGRIPOPEN	34
#define INSAIRSUPPLY	35
/* ----  Gilson Errors  ---- */
#define RACKNSAMPFILEERROR 40
#define INVALID_ZONE	   41
#define INVALID_SAMP	   42
#define TCLINTERPERROR	   43
#define TCLSCRIPTERROR	   44
#define INVALID_RACK	   45

/* ---- Errors 46 - 69 reserved for AS4896 --- */

/* ---- Hermes Errors ---- */
#define HRM_MOTORS_HALTED  70
#define HRM_HIGHPOWER      71
#define HRM_RACKTOPCS      72
#define HRM_PCSTOMAG       73
#define HRM_MAGTORACK      74
#define HRM_BARCODE        75
#define HRM_ROBOTERR       76
#define HRM_CALIBRATE      77
#define HRM_COMMERROR      78
#define HRM_BARCODE_FATAL  79

#define SMPINFAIL	92
#define SMPRMFAIL	93
#define SMPSPNFAIL	94
#define SLOWDROPFAIL	95
#define SMPNOPOWER	96
#define SMPRS232	97
#define SMPTIMEOUT	98

#define SHIMERROR	600
#define SHIMABORT	1
#define SHIMLKLOST	2
#define SHIMLKSAT	3
#define SHIMDACLIM	4
#define SHIMNOMETHOD	5
#define SHIMCOMLOST	6

#define ALKERROR	700
#define ALKABORT	1
#define ALKRESFAIL	2
#define ALKPOWERFAIL	3
#define ALKPHASFAIL	4
#define ALKGAINFAIL	5

#define AGAINERROR	800
#define AGAINFAIL	1


/* ---    Hard Errors --- */

#define HDWAREERROR	900
#define PSGIDERROR	1
#define STMERROR	2
#define FIFOERROR	3
#define NP2BIG		4
#define BUSSTRAP	5
#define HWLOOPZERO	6
#define NETBLERROR	7
#define FORPERROR	8
#define OB_IDERROR	9
#define INVALIDACODE    10
/* new for nessie */
#define FIFOSTRTEMPTY   11
#define FIFOSTRTHALT    12
#define FIFONETBAP      13
#define NOROTORSYNC     14
#define NOHSDTMADC      15
#define INVALPARALLELTYPE 20
#define PARALLELNOSPACE 21
#define TOOFEWRCVRS	22
#define INCONSISTENT_STATUS	23
#define TAG_INDEX_ERROR	24
#define FIFO_OVRFLOW	25
#define MASTER_SPI0_OVRRUN	26
#define MASTER_SPI1_OVRRUN	27
#define MASTER_SPI2_OVRRUN	28
#define PFG_XAMP_OVRRUN	29
#define PFG_YAMP_OVRRUN	30
#define PFG_ZAMP_OVRRUN	31
#define GRADIENT_AMP_OVRRUN	32
#define GRADIENT_ECC_OVRRUN	33
#define GRADIENT_SLEW_OVRRUN	34
#define GRADIENT_DUTY_OVRRUN	35
#define GRADIENT_SPI_OVRRUN	36
#define DDR_DSP_NOT_INIT	41
#define DDR_DSP_ACQ_ERR		42
#define DDR_DSP_SCANQ_ERR	43
#define DDR_DSP_DATAQ_ERR	44
#define DDR_DSP_NP_ERR		45
#define DDR_DSP_LOCK_ERR	46
#define DDR_DSP_ADC_OVRFLW	47
#define DDR_DSP_DFIFO_OVRFLW	48
#define DDR_DSP_BAD_MSG		49
#define DDR_DSP_SCANITR_ERR	50
#define DDR_DSP_BADID_ERR 	51
#define DDR_DSP_MEM_ERR		52
#define DDR_DSP_PROC_ERR	53
#define DDR_DSP_SNAPSHOT_ERR	54
#define DDR_DSP_C67Q_OVRFLW	55
#define DDR_DSP_405Q_OVRFLW	56
#define DDR_DSP_FIFO_OVRFLW	57
#define DDR_DSP_FIFO_UNDERFLW	58
#define DDR_DSP_HPI_TIMEOUT	59
#define DDR_DSP_SW_TRAP		60
#define DDR_DSP_UNKNOWN_ERR		61
#define DDR_DSP_FIDQ_ERR		62
#define DDR_DSP_SCANQ_POP_ERR	63
#define CNTLR_MISSING_ERR       64
#define CNTLR_ISYNC_ERR         65
#define CNTLR_CMPLTSYNC_ERR     66
#define GRADIENT_AMPL_OUTOFRANGE 67
#define DDR_CMPLTFAIL_ERR     68
#define FIFO_INVALID_OPCODE   69
#define DATA_UPLINK_ERR   70
#define PROG_DEC_PAT_ERR	71
#define PROG_DEC_LOAD_ERR	72

#define SCSIERROR	1000
#define READWARN	1
#define WRITEWARN	2
#define READERROR	3
#define WRITEERROR	4

#define DISKERROR	1100
#define OPENERROR	1
#define CLOSEERROR	2

#define SYSTEMERROR	1200
#define FIXBUFNOSPACE   1
#define DYNBUFNOSPACE   2
#define MALLOCNOSPACE   3
#define NOACODESET	4
#define NOACODERTV	5
#define NOACODETBL	6
#define INVALIDPCMD	7
#define NOISECMPLTWDOG  8
#define MSGQ_LOST_ERR   9
#define DWNLD_CRC_ERR   10
#define IDATA_CRC_ERR   11
#define DATA_CRC_ERR    12
#define DWNLD_CMPLT_ERR 13
#define FIFO_WRDS_LOST  14
#define NOACODEPAT	15
#define NVLOOP_ERR 	16
#define IFZERO_ERR	17
#define GRD_ROTATION_LIST_ERR 18

#define RTPSGERROR	1300

#define SAFETYERROR		1400
#define OBSERROR		30
#define DECERROR		50

/* General errors */
#define SIB_NO_ERROR		0
#define SIB_RESVD_A		1
#define SIB_RESVD_B		2
#define SIB_RESVD_C		3
#define SIB_QUAD_REFL_PWR	4
#define SIB_OPR_PANIC		5
#define SIB_PS_FAIL		6
#define SIB_ATTN_READBACK	7
#define SIB_QUAD_REFL_PWR_BP	8
#define SIB_PS_FAIL_BP		9
#define SIB_MALLOC_FAIL		10
#define SIB_COMM_FAIL		11

/* Channel specific errors */
#define SIB_PALI_MISSING	4
#define SIB_REFL_PWR		5
#define SIB_AMP_GATE_DISCONN	6
#define SIB_PALI_TRIP		7
#define SIB_W_DOG		8
#define SIB_PWR_SUPPLY		9
#define SIB_REQ_ERROR		10
#define SIB_TUSUPI_TRIP		11
#define SIB_RF_OVERDRIVE	12
#define SIB_RF_PULSE_WIDTH	13
#define SIB_RF_DUTY_CYCLE	14
#define SIB_RF_OVER_TEMP	15
#define SIB_RF_PWR_SUPPLY	16
#define SIB_PALI_BP		17
#define SIB_REFL_PWR_BP		18
#define SIB_RF_AMP_BP		19

#define ILIERROR		1500

/* General errors */
#define ILI_QUAD_REFL_PWR	1
#define ILI_RFMON_MISSING	2
#define ILI_OPR_PANIC		3
#define ILI_OPR_CABLE		4
#define ILI_GRAD_TEMP		5
#define ILI_GRAD_WATER		6
#define ILI_HEAD_TEMP		7
#define ILI_ATTN_READBACK	8
#define ILI_RFMON_WDOG		9
#define ILI_RFMON_SELFTEST	10
#define ILI_RFMON_PS		11
#define ILI_SPARE_3		12
#define ILI_PS			13
#define ILI_SDAC_DUTY_CYCLE	14
#define ILI_SPARE_1		15
#define ILI_SPARE_2		16
#define ILI_QUAD_BP		17
#define ILI_SDAC_BP		18
#define ILI_HEAD_GRAD_BP	19
#define ILI_GRAD_BP		20

/* Channel specific errors */
#define ILI_10S_SAR		1
#define ILI_5MIN_SAR		2
#define ILI_PEAK_PWR		3
#define ILI_RF_AMP_CABLE	4
#define ILI_RF_REFL_PWR		5
#define ILI_RF_DUTY_CYCLE	6
#define ILI_RF_OVER_TEMP	7
#define ILI_RF_PULSE_WIDTH	8
#define ILI_RFMON_BP		9
#define ILI_RF_AMP_BP		10

#define CH3ERROR 60		/* Uses 1561 - 1570 */
#define CH4ERROR 70		/* Uses 1571 - 1580 */

#define ILI2_SPARE_0 81
#define ILI2_SPARE_1 82
#define ILI2_SPARE_2 83
#define ILI2_SPARE_3 84
#define ILI2_SPARE_4 85
#define ILI2_SPARE_5 86
#define ILI2_SPARE_6 87
#define ILI2_SPARE_7 88
#define ILI2_SPARE_8 89
#define ILI2_SPARE_9 90
#define ILI2_SPARE_10 91
#define ILI2_SPARE_11 92
#define ILI2_SPARE_12 93
#define ILI2_SPARE_13 94
#define ILI2_SPARE_14 95
#define ILI2_SPARE_15 96

#define ILI_SPARE_BP_1 21
#define ILI_SPARE_BP_2 22
#define ILI_SPARE_BP_3 23
#define ILI_SPARE_BP_4 24

#define ISIERROR		1600

#define ISI_XGRAD_FAULT		1
#define ISI_YGRAD_FAULT		2
#define ISI_ZGRAD_FAULT		3
#define ISI_DUTY_CYCLE_ERR	4
#define ISI_OVERTEMP		5
#define ISI_NO_COOLANT		6
#define ISI_SPARE_3		7
#define ISI_SPARE_4		8
#define ISI_SPARE_1		9
#define ISI_SPARE_2		10
#define ISI_GRAD_AMP_ERR	11
#define ISI_SYS_FAULT		12
#define ISI_BYPASS		13

#define GPAERROR		1700
/* Gradient Power Amplifier errors */

#define GPA_ERROR		0
#define GPA_NO_COMM		1
#define GPA_ENABLED		2
#define GPA_WONT_TUNE		3

/* Magnet Leg / Air Pressure Sensor errors */
#define SNS_ERROR               1800
#define MAGNETLEG               0
#define AIRPRESSURE             1

/* Pneumatic box error codes */
#define PNEU_ERROR		1900
#define PS			1
#define VTTHRESH		2
#define NB_STACK		3
#define PRESSURE		4

/* iCAT RF error codes */
#define ICAT_ERROR		1950
#define IMAGE_COPY	1
#define IMAGE_LOAD   2
#define REG_CONFIG	3
#define MIXED_RFS    4



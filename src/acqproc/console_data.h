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

#define  DATA_STATION		1
#define  SPECTROMETER		2
#define  IMAGER			3

#define  CONSOLE_MAX_RF_CHAN	6
#define  CHANNELS_PER_BOARD	4

#define  STANDARD_ATTN		63
#define  DELUXE_ATTN		79

struct _objectMsg
{
	int      request;
	int	 r_len;
	int      status;
	char    *r_addr;
};


#define  MIN_RESPONSE_LEN	20

/*  Masks for Request Values  */

#define  BASIC_MSG_MASK		(~0xff)
#define  SUB_ATTR_MASK		(0xff)


/*  Possible Request Values  */
/*  Note:  If you change the shift count, change the masks accordingly  */
/*  Defintions following the first are formed by shifting the previous
    definition back to the right, incrementing and then shifting again
    to the left.  Thus generated semiautomatically are 1<<8, 2<<8, etc.

    NO_MESSAGE is a place holder so an entry in a table of messages
    can represent no message at all.					*/

#define	NO_MESSAGE		0
#define	IS_SYSTEM_HYDRA	(1<<8)
#define	GET_SYSTEMTYPE		(((IS_SYSTEM_HYDRA>>8)+1)<<8)
#define	GET_H1FREQ		(((GET_SYSTEMTYPE>>8)+1)<<8)
#define	GET_SHIMSET		(((GET_H1FREQ>>8)+1)<<8)
#define	GET_AUDIOFIL		(((GET_SHIMSET>>8)+1)<<8)
#define	GET_VT			(((GET_AUDIOFIL>>8)+1)<<8)
#define	GET_DMF			(((GET_VT>>8)+1)<<8)
#define	GET_MAXSW		(((GET_DMF>>8)+1)<<8)
#define GET_MAXNB		(((GET_MAXSW>>8)+1)<<8)
#define GET_APIFACE		(((GET_MAXNB>>8)+1)<<8)
#define	GET_FIFOLPSIZE		(((GET_APIFACE>>8)+1)<<8)
#define	GET_ROTOSYNC		(((GET_FIFOLPSIZE>>8)+1)<<8)
#define	GET_LOCKFREQ		(((GET_ROTOSYNC>>8)+1)<<8)
#define	GET_IFFREQ		(((GET_LOCKFREQ>>8)+1)<<8)
#define	GET_NUMRFCHAN		(((GET_IFFREQ>>8)+1)<<8)
#define GET_GRAD		(((GET_NUMRFCHAN>>8)+1)<<8)
#define	GET_RFTYPE		(((GET_GRAD>>8)+1)<<8)
#define	GET_PTSBASE		(((GET_RFTYPE>>8)+1)<<8)
#define	GET_LATCHING		(((GET_PTSBASE>>8)+1)<<8)
#define	GET_STEPSIZE		(((GET_LATCHING>>8)+1)<<8)
#define GET_FATTNBASE		(((GET_STEPSIZE>>8)+1)<<8)
#define	GET_CATTNBASE		(((GET_FATTNBASE>>8)+1)<<8)
#define	GET_WFGBASE		(((GET_CATTNBASE>>8)+1)<<8)
#define	GET_AMPBASE		(((GET_WFGBASE>>8)+1)<<8)
#define GET_GRADTYPE		(((GET_AMPBASE>>8)+1)<<8)
#define GET_DSPPROMTYPE		(((GET_GRADTYPE>>8)+1)<<8)
#define GET_CONSOLETYPE		(((GET_DSPPROMTYPE>>8)+1)<<8)
#define GET_NUMRCVRS		(((GET_CONSOLETYPE>>8)+1)<<8)

#define  SET_PTSBASE		(((GET_GRADTYPE>>8)+1)<<8)
#define  SET_LOCKFREQ		(((SET_PTSBASE>>8)+1)<<8)
#define  SET_H1FREQ		(((SET_LOCKFREQ>>8)+1)<<8)
#define  SET_IFFREQ		(((SET_H1FREQ>>8)+1)<<8)
#define  SET_SYSTEMTYPE		(((SET_IFFREQ>>8)+1)<<8)

/*  Special note:  RF channels are indexed 1 ... MAX_RF_CHAN
		   This follows the definition of TODEV, DODEV, etc.
		   in the PSG programs.					*/

#define  MAKE_RF_CHAN_MSG( BASE_MSG, RF_CHAN )	\
				(BASE_MSG + RF_CHAN - 1)

#define  GET_PTSVAL( N )	MAKE_RF_CHAN_MSG(GET_PTSBASE,N)
#define  GET_CATTNVAL( N )	MAKE_RF_CHAN_MSG(GET_CATTNBASE,N)
#define  GET_WFG( N )		MAKE_RF_CHAN_MSG(GET_WFGBASE,N)
#define  GET_AMP( N )		MAKE_RF_CHAN_MSG(GET_AMPBASE,N)

#define  SET_PTSVAL( N )	MAKE_RF_CHAN_MSG(SET_PTSBASE,N)


/*  Possible Status Values  */

#define  NOT_FOUND	-1		/* request not recognized.	*/
#define  NOT_VALID	-2		/* from console data valid field*/
#define  NOT_APPLICABLE -3		/* e.g. Hydra-only on a Unity   */
#define  NOT_KNOWN	-4		/* see comment below		*/
#define  NOT_ENOUGH_SPACE -5		/* in client's response buffer	*/
#define  BAD_FORMAT	-6		/* (set only) object couldn't   */
					/* convert application buffer   */
#define  OK		0

/*  Typical situation where status is NOT_KNOWN is when the console
    data field comes from the AP Bus.  If this field is MISSING_DATA
    (see definition below) then the AP Bus timed out instead of
    reading back the expected value.					*/


/*  Possible Types for Returned Data  */

#define  CHAR		1
#define  INTEGER	(CHAR+1)
#define  REALNUM	(INTEGER+1)	/*  The "C" datatype double  */


/*  Possible Scope of Applicability  */

#define  UNITY_HYDRA	1
#define  HYDRA_ONLY	2


/*  AP Bus programs return a Special Value if the AP Bus timed out.	*/

#define  MISSING_DATA	(0x1ff)

/*  These defines were taken from hardware.h, SCCS category vwacq.	*/

#define DSP_PROM_NO_DSP		0
#define DSP_PROM_NO_DOWNLOAD	1
#define DSP_PROM_DOWNLOAD	2

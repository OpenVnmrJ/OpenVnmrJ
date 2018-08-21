/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------
|  RF Channel Constants Used By initobjects
| initialized in rcchanconst.c
|		 Greg B. 
+----------------------------------------------*/
struct _chanconst {
	int ptsapadr;		/* PTS AP Address */
	int ptsapreg;		/* PTS AP Register */
	int ptsapbytes;		/* PTS AP Bytes */

	int catapadr;		/* Coarse Attenuator AP Address */
	int catapreg5;		/* Coarse Attenuator AP Register (early type) */
	int catapreg;		/* Coarse Attenuator AP Register */
	int catapbytes;		/* Coarse Attenuator AP Bytes */
	int catapmode;		/* Coarse Attenuator AP Mode */
	int cathwmax;		/* Coarse Attenuator Hardware Max */
	int cathwmin;		/* Coarse Attenuator Hardware Min */
	int cathwofset5;	/* Coarse Attenuator Hardware Offset(early type)*/
	int cathwofset;		/* Coarse Attenuator Hardware Offset */

	int fatapadr;		/* Fine Attenuator AP Address */
	int fatapreg;		/* Fine Attenuator AP Register */
	int fatapbytes;		/* Fine Attenuator AP Bytes */
	int fatapmode;		/* Fine Attenuator AP Mode */
	int fathwmax;		/* Coarse Attenuator Hardware Max */
	int fathwmin;		/* Coarse Attenuator Hardware Min */
	int fathwofset;		/* Coarse Attenuator Hardware Offset */

	int xmtr2amprelaybit;	/* Transmitter to Amp Relay AP Bit */
	int xmtrx2bit;		/* Transmitter Freq Doubler AP Bit */
	int phline0;		/* Channel 0 Phase line */
	int phline90;		/* Channel 90 Phase line */
	int phline180;		/* Channel 180 Phase line */
	int phline270;		/* Channel 270 Phase line */
	int phasebits;		/* Channel Phase Bits */
	int smphaseapadr;	/* Small Angle Phaseshifter AP Address */
	int xmtrgatebit;	/* Channel Tranmitter HS Gate Bit */
	int wggatebit;		/* Channel Waveform Generator Gate Bit */
	};
typedef struct _chanconst chanconst;

#define CONSTSTRUCTSIZE  (sizeof(struct _chanconst) / sizeof(int))

struct _rfvarinfo {
   double *rfsfrq;	/* Channel Base Freq param (sfrq,dfrq,dfrq2) */
   double *rftof;	/* Channel Offset Freq param (tof,dof,dof2) */
   double *rftpwr;	/* Channel Coarse Attn param (tpwr,dpwr,dpwr2 */
   double *rftpwrf;	/* Channel Fine Attn param (tpwrf,dpwrf) */
   char   *rftofname;	/* parameter name of freq offset (tof,dof,dof2) */
};

typedef struct _rfvarinfo rfvarinfo;


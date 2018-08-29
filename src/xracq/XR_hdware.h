/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
***************************************************************
*
*   High Speed Line assignments positive logic except where noted.
*
*	All High Speed Lines are set by passed parameter to eventops.
*	Two exceptions are the phase lines for Tx & Dec. These are set
*	in real time by rt variables which index into a phase table that
*	actually contain the High Speed line bit settings.
*	These tables are in lc and are setup by PSG.
******************************************************************************
*    15 14 13 12     11 10 9  8     7  6  5  4     3  2  1  0
*    |  |  |  |      |  |  |  |     |  |  |  |     |  |  |  |
*    |  |  |  |      |  |  |  |     |  |  |  |     |  |  |  \__ VAR1 line (0x1)
*    |  |  |  |      |  |  |  |     |  |  |  |     |  |  \__ VAR2 line 	(0x2)
*    |  |  |  |      |  |  |  |     |  |  |  |     |  \__ Spare 1 line 	(0x4)
*    |  |  |  |      |  |  |  |     |  |  |  |     \__ Spare 2 line 	(0x8)
*    |  |  |  |      |  |  |  |     |  |  |  \__ Dec high pwr sec of amp (blanking)
*    |  |  |  |      |  |  |  |     |  |  \__ Dec modualtion mode line A (0x20)
*    |  |  |  |      |  |  |  |     |  \_____ Dec modualtion mode line B (0x40)
*    |  |  |  |      |  |  |  |     \__ HomoSpoil line 			(0x80)
*    |  |  |  |      |  |  |  \__ Dec high power line			(0x100)
*    |  |  |  |      |  |  \__ Dec phase 90 degree line			(0x200)
*    |  |  |  |      |  \_____ Dec phase 180 degree line		(0x400)
*    |  |  |  |      \__ Decoupler ON - OFF				(0x800)
*    |  |  |  \__ Transmitter phase 90 degree line			(0x1000)
*    |  |  \_____ Transmitter phase 180 degree line			(0x2000)
*    |  \__ Transmitter ON - OFF					(0x4000)
*    \__ Receiver OFF - ON	(negative logic)			(0x8000)
*******************************************************************************
*	memory map for acquisition system
*
****************************************************
*
*	Pulse Control (Output) card definitions
*
****************************************************
*
****************************************************
*	Old Stlye OutPut Board
*		(Roctl - read $001)
*	-----------Check OUTPUT Status------------
*
*	7  6  5  4  3  2  1  0
*	|  |  |  |  |  |  |  |
*	|  |  |  |  |  |  |  \__ fifo busy
*	|  |  |  |  |  |  \__ loop in progress
*	|  |  |  |  |  \__ underflow
*	|  |  |  |  \__ pre loop overflow
*	|  |  |  \__ pre loop > 16 words
*	|  |  \__ pre loop ready
*	|  \__ flag1
*	\__ APINT (apb interrupt)
*
****************************************************
*    Old Style OutPut Board
*		(Roapsr - read $003) if bit set = 1
*	-----------Check ANALOG PORT Status------------
*
*	7  6  5  4  3  2  1  0
*	|  |  |  |  |  |  |  |
*	|  |  |  |  |  |  |  \__ apto (ap timeout)
*	|  |  |  |  |  |  \__ ap data ready
*	|  |  |  |  |  \__ ap data overflow
*	|  |  |  |  \__ lock reciever gate (valid lock data)
*	|  |  |  \__ apspin (spinner tach not spinning)
*	|  |  \__ apvt (vt not regulating)
*	|  \__ aplock (system not locked)
*	\__ apspare
*
****************************************************
*	New Pulse Controller Board
*		(Roctl - read $001)
*	-----------Check OUTPUT Status------------
*
*	7  6  5  4  3  2  1  0
*	|  |  |  |  |  |  |  |
*	|  |  |  |  |  |  |  \__ fifo running
*	|  |  |  |  |  |  \__ fifo loop in progress (itrp goes low)
*	|  |  |  |  |  \__ looping fifo empty
*	|  |  |  |  \__ looping fifo full
*	|  |  |  \__ Not Enough Time between Hardware loops
*	|  |  \__ pre loop fifo full 
*	|  \__ flag1  (itr when true)
*	\__ looping fifo out ran pre-fifo 
*
****************************************************
*	New Pulse Controller Board
*		(Roapsr - read $003) if bit set = 1
*	-----------Check ANALOG PORT Status------------
*
*	7  6  5  4  3  2  1  0
*	|  |  |  |  |  |  |  |
*	|  |  |  |  |  |  |  \__ apto (ap timeout)
*	|  |  |  |  |  |  \__ ap data ready
*	|  |  |  |  |  \__ ap data overflow
*	|  |  |  |  \__ lock reciever gate (valid lock data)
*	|  |  |  \__ apspin (spinner tach not spinning)
*	|  |  \__ apvt (vt not regulating)
*	|  \__ aplock (system not locked)
*	\__ ap operation in progress (check for IO chan vs fifo access) 
*
****************************************************
*
*		(read $005)
*	-----------Read ANALOG PORT DATA------------
*
*	7  6  5  4  3  2  1  0
*	|  |  |  |  |  |  |  |
*	\__\__\__\__\__\__\__\__ AP DATA (APD0-7)
*
*	end output card
*
****************************************************

****************************************************
*
*	Programable Interrupt Controller definitions
*
****************************************************
****************************************************
*			ICW1
*    A0   D7   D6   D5   D4   D3   D2   D1   D0
*    0    A7   A6   A5   1    LTIM ADI  SNGL IC4   Description when = 1/0
*    |	  |    |    |    |    |    |    |    |		
*    |	  |    |    |    |    |    |    |     \___ ICW4 needed/not needed
*    |	  |    |    |    |    |    |     \ _______ single/cascade
*    |	  |    |    |    |    |     \_____________ call address interval 4/8
*    |	  |    |    |    |     \ _________________ level trigger/edge trigger
*    |	  |    |    |     \ ______________________ ICW1 decoding
*    |	  |    |    |
*    |	   \____\ ___\____________________________ A7-A5 of vector address
*     \ __________________________________________ ICW1 decoding
********************************************************
*			ICW2
*    A0   D7   D6   D5   D4   D3   D2   D1   D0
*    1    A15  A14  A13  A12  A11  A10  A9   A8    Description when = 1/0
*    |	  |    |    |    |    |    |    |    |		
*    |	   \____\____\____\____\____\____\____\___ A15-A8 of vector
*     \ __________________________________________ ICW2 decoding
********************************************************
*			ICW3(Master)
*    A0   D7   D6   D5   D4   D3   D2   D1   D0
*    1    S7   S6   S5   S4   S3   S2   S1   S0    Description when = 1/0
*    |	  |    |    |    |    |    |    |    |		
*    |	   \____\____\____\____\____\____\____\___ IR input has slave/no slave
*     \ __________________________________________ ICW3 decoding
********************************************************
*			ICW3(slave)
*    A0   D7   D6   D5   D4   D3   D2   D1   D0
*    1    0    0    0    0    0    ID2  ID1  ID0   Description when = 1/0
*    |	  |    |    |    |    |    |    |    |		
*    |    |    |    |    |    |     \____\____\___ slave device address
*    |	  \____\____\____\____\__________________ 
*     \ __________________________________________ ICW3 decoding
********************************************************
*			ICW4
*    A0   D7   D6   D5   D4   D3   D2   D1   D0
*    1    0    0    0    SFNM BUF  M/S  AEOI uPM   Description when = 1/0
*    |	  |    |    |    |    |    |    |    |		
*    |	  |    |    |    |    |    |    |     \___ 8086-88/MCS80-85
*    |	  |    |    |    |    |    |     \ _______ auto EOI/reg EOI
*    |	  |    |    |    |    |     \_____________ master/slave
*    |	  |    |    |    |     \ _________________ buffered/non buffered
*    |	  |    |    |     \ ______________________ spec. fully nested mode/not
*     \ __ \____\ ___\____________________________ ICW4 decoding
********************************************************
*			OCW1
*    A0   D7   D6   D5   D4   D3   D2   D1   D0
*    1    M7   M6   M5   M4   M3   M2   M1   M0    Description when = 1/0
*    |	  |    |    |    |    |    |    |    |		
*    |	   \____\____\____\____\____\____\____\___ IR7-IR0 mask
*     \___________________________________________ OCW1 decoding 
********************************************************
*			OCW2
*    A0   D7   D6   D5   D4   D3   D2   D1   D0
*    0    R    SL   EOI  0    0    L2   L1   L0    Description when = 1/0
*    |	  |    |    |    |    |    |    |    |		
*    |	  |    |    |    |    |     \____\____\___ IR cleared on EOI when SL=1
*    |	  |    |    |     \____\__________________ OCW2 decoding 
*    |    |    |     \____________________________ clears interrupt
*    |    |     \_________________________________ clears specific level on EOI
*    |     \______________________________________ rotate priority on EOI
*     \ __ _______________________________________ OCW2 decoding
********************************************************
*			OCW3
*    A0   D7   D6   D5   D4   D3   D2   D1   D0
*    0    0    ESMM SMM  0    1    P    RR   RIS   Description when = 1/0
*    |	  |    |    |    |    |    |    |    |		
*    |	  |    |    |    |    |    |    |     \___ 
*    |	  |    |    |    |    |    |     \________
*    |	  |    |    |    |    |     \_____________
*    |	  |    |    |     \____\__________________ OCW3 decoding 
*    |    |    |     \____________________________ 
*    |    |     \_________________________________ 
*     \____\______________________________________ OCW3 decoding 
*****************************************************************
#define PC_BASE $f81e00/* 	pulse controler base address */
#define PC_WSR	$0/* 		control/status register (write) */
#define PC_RSR	$1/* 		control/status register (read) */
#define AP_RSR	$3/*		analog port status register (read only) */
#define AP_RDR	$5/*		analog port data register (read only) */
#define AP_RAD	$7/*		analog port address/ctl register (read only) */
#define PC_LPC  $8/*		output board ID register  */
#define PC_ID   $D/*		output board ID register  */
#define PC_ITR1 $10/*		output card fifo interrupt register 1 */
#define PC_ITR2 $12/*		output card fifo interrupt register 2 */
#define AP_ITR1 $14/*		output card analog port interrupt register 1 */
#define AP_ITR2 $16/*		output card analog port interrupt register 1 */
#define AP_WAD	$21/*		analog port address/ctl register (write only)*/
#define AP_WDR	$23/*		analog port data register (write only) */
#define HSR_RPR	$25/*		High Speed Rotor Period Read Register  */

******************************************************************
*
*	Pulse Controller (output) card (fifo) register definitions
*
******************************************************************
*
#define NULL 		$0/*		null */
#define FIFO_BASE $f81f00/*	fifo offset */
#define OFL 	$0/*		into fifo long */
#define SLOOP_BIT 	$10/*		into fifo w/start loop long */
#define ELOOP_BIT	$20/*		to fifo long w/end loop	*/
#define CTC_BIT		$40/*		to fifo long w/ctc set	*/
#define FLAG1_BIT	$80/*		to fifo long w/flag1 set	*/

/* addressing macros for hardware */
#define pc_adr(A)      PC_BASE+A
#define pc_fifo(A)     FIFO_BASE+A
#define f_bits(A,B)     A+B

obase	=	$f81e00	base address of output card
octl	=	$0	control/status register (write)
Roctl	=	$1	control/status register (read)
Roapsr	=	$3	analog port status register (read only)
Roapdr	=	$5	analog port data register (read only)
Roapar	=	$7	analog port address/ctl register (read only)
olc	=	$8	loop counter register
oid	=	$D	output board ID register 
ofir1	=	$10	output card fifo interrupt register 1
ofir2	=	$12	output card fifo interrupt register 2
oapir1	=	$14	output card analog port interrupt register 1
oapir2	=	$16	output card analog port interrupt register 1
Woapar	=	$21	analog port address/ctl register (write only)
Woapdr	=	$23	analog port data register (write only)
ofl	=	$100	to fifo long
ofbl	=	$110	to fifo long w/begin loop
ofsl	=	$120	to fifo long w/stop loop
ofbsl	=	$130	to fifo long w/begin-stop loop
ofcl	=	$140	to fifo long w/ctc set
ofbcl	=	$150	to fifo long w/ctc set & begin loop
ofscl	=	$160	to fifo long w/ctc set & stop loop
ofbscl	=	$170	to fifo long w/ctc set & begin-stop loop
offl	=	$180	to fifo long w/flag1 set
ofbfl	=	$190	to fifo long w/flag1 set & begin loop
ofsfl	=	$1A0	to fifo long w/flag1 set & stop loop
ofbsfl	=	$1B0	to fifo long w/flag1 set & begin-stop loop
ofcfl	=	$1C0	to fifo long w/ctc,flag1 set
ofbcfl	=	$1D0	to fifo long w/ctc,flag1 set & begin loop
ofscfl	=	$1E0	to fifo long w/ctc,flag1 set & stop loop
ofbscfl	=	$1F0	to fifo long w/ctc,flag1 set & begin-stop loop

****************************************************
*
*	input card definitions
*
****************************************************
*
*			(read $200)
*	-----------Check INPUT Status------------
*
*	15..8  7  6  5  4  3  2  1  0
*	|   |  |  |  |  |  |  |  |  |   16 data words/
*	|   |  |  |  |  |  |  |  |  \__ interrupt pending
*	|   |  |  |  |  |  |  |  \__ DATA overflow
*	|   |  |  |  |  |  |  \__ fifo output ready > 1 word
*	|   |  |  |  |  |  \__ interrupt enabled
*	|   |  |  |  |  \__ bit0 adc base shift		0 = 15 bit adc
*	|   |  |  |  \_____ bit1 adc base shift		3 = 13 bit adc
*	|   |  |  \________ bit2 adc base shift
*	|   |  \___________ bit3 adc base shift
*	|   \
*	\____\ unused
****************************************************
*
*			(write $202)
*	-----------Set INPUT CONTROL------------
*
*	15..8  7  6  5  4  3  2  1  0
*	|   |  |  |  |  |  |  |  |  |
*	|   |  |  |  |  |  |  |  |  \__ enable interrupts
*	|   |  |  |  |  |  |  |  \__ 0=observe, 1=lock channel
*	\___\__\__\__\__\__\__\__ unused
*
****************************************************
*
*			(write $204)
*	-----------set the input mode------------
*
*	15 .. 6  5  4  3  2  1  0
*	|  |  |  |  |  |  |  |  |
*	|  |  |  |  |  |  |  |  \_ Scale bit 0 - lsb
*	|  |  |  |  |  |  |  \____ scale bit 1
*	|  |  |  |  |  |  \_______ scale bit 2
*	|  |  |  |  |  \__________ scale bit 3 - msb
*	|  |  |  |  \___________ mode bit 1
*	|  |  |  \______________ mode bit 2
*	|  |  |__ 
*	|  \_____\ unused
*	\________/ 
*
****************************************************
*
*		(read $204 or read $001)
*	-----------Check OUTPUT Status------------
*
*	7  6  5  4  3  2  1  0
*	|  |  |  |  |  |  |  |
*	|  |  |  |  |  |  |  \__ fifo busy
*	|  |  |  |  |  |  \__ loop in progress
*	|  |  |  |  |  \__ underflow
*	|  |  |  |  \__ pre loop overflow
*	|  |  |  \__ pre loop > 16 words
*	|  |  \__ pre loop ready
*	|  \__ flag1
*	\__ APINT (apb interrupt)
*
*	end input card
****************************************************
*
#define IC_BASE $f82000/* 	ADC (input card) base address */
#define IC_RSR	$0/*		input card status	(read)	*/
#define IC_WSR	$0/*		input card reset no data	(write)	*/
#define IC_DATA	$2/*		input card data b0-15 adc data	(read)	*/
#define IC_CNTL $2/*		input card control		(write)	*/
#define IC_OS	$4/*		output card status	(read)	*/
#define IC_MODE $4/*		set input mode/scaling	(write)	*/

ics	=	$200	input card status	(read)
icsr	=	$60	lc memory location
icr	=	$200	input card reset no data	(write)
icd	=	$202	input card data b0-15 adc data	(read)
icdr	=	$62	lc memory location
icctl	=	$202	input card control		(write)
icos	=	$204	output card status	(read)
icocsr	=	$64	lc memory location
icmod	=	$204	set input mode/scaling	(write)
icmode	=	$2C	lc memory location

*
* hardware addressing macro 
*
#define ic_adr(A)      IC_BASE+A

****************************************************
*
*	SUM-TO-MEMORY card definitions
*
****************************************************
*
****************************************************
*
*			(read $300)
*	-----------Check STM Status---------------
*
*	15 . 8  7  6  5  4  3  2  1  0
*	|    |  |  |  |  |  |  |  |  |
*	|    |  |  |  |  |  |  |  |  \__ reset / overflow
*	|    |  |  |  |  |  |  |  \__ enable when ready
*	|    |  |  |  |  |  |  \__ 32/16* bits
*	|    |  |  |  |  |  \__ scale
*	|    |  |  |  |  \__ ARLD
*	|    |  |  |  \__ interrupt enabled
*	|    |  |  \__ Batch enable
*	|    |  \__ Overflow clear enable / count = 0
*	\____\_____ "CHECK" value
*
*	end STM
****************************************************
*
#define STM_BASE $f82100/* 	STM (Sum-To-Memory card) base address */
#define STM_RSR $0/*		stm status	(read)	*/
#define STM_WSR $0/*		stm control	(write)	*/
#define STM_RDA $2/*		stm fwa data transfer - long word (read) */
#define STM_WDA $2/*		stm fwa data transfer - long word (write) */
#define STM_RDC $6/*		stm count - long word	(read) */
#define STM_WDC $6/*		stm count - long word	(write) */
*
*       STM (sum to memory) card definitions
*
Rstms   =       $300    stm status      (read)
stmsr   =       $66     lc memory location
stms    =       $300    stm control     (write)
stmchk  =       $2E     lc memory location
Rstma   =       $302    stm fwa data transfer - long word (read)
stma    =       $302    stm fwa data transfer - long word (write)
stmar   =       $68     lc memory location
Rstmc   =       $306    stm count - long word   (read)
stmc    =       $306    stm count - long word   (write)
stmcr   =       $6C     lc memory location

*
* hardware addressing macro 
*
#define stm_adr(A)      STM_BASE+A

****************************************************
*
*	SOLIDS ADC Input Card definitions
*
****************************************************
*
* Wideline accessory interface definitions
*
#define WL_BASE  $f82200/*	offset to Wideline versabus address */
#define WL_RWSR  $0/*		offset to Wideline versabus address */
#define WL_WDC   $2/*		number of real/imag pairs to be digitized */
#define WL_RDATA $2/*		offset to real data within wl memory */
#define WL_IDATA $4/*		offset to imag data within wl memory */
*
wlcont  =       $f82200		offset to Wideline versabus address
wlenbl  =	$1		enable wl to ADC mode(ie, data input)
wldis   =	$0		disable wl for ADC&enable for memoryread
wlfin   =	$1		Wideline finished digitizing
wlcoun  =	$2		number of real/imag pairs to be digitized
wlreal  =	$2		offset to real data within wl memory
wlimag  =	$4		offset to imag data within wl memory
*
* hardware addressing macro 
*
#define wl_adr(A)      WL_BASE+A


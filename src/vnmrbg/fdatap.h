/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***/
/* fdatap.h: defines the NMRPipe data header array FDATA, and
 *           outlines some data format details.
 ***/

#include "dimloc.h"
#ifndef VNMRJ
#include "namelist2.h"
#endif

/***/
/* The NMRPipe parameter array FDATA currently consists of 512 4-byte 
 * floating-point values which describe the spectral data.  While all numerical
 * values in this array are floating point, many represent parameters
 * (such as size in points) which are integers.  Some parts of the
 * header contain packed ascii text.
 *
 * There are currently three variations of spectral data in the NMRPipe
 * format:
 *
 *   1. Single-File (1D and 2D): the data are stored in a single
 *      binary file consiting of the header followed by the
 *      spectral intensities, stored in sequential order as
 *      4-byte floats.
 *
 *   2. Multi-File (3D and 4D): the data are stored as a series
 *      of 2D file planes, each with its own complete header
 *      followed by the spectral intensities in sequential order.
 *
 *   3. Data Stream (1D-4D): the data are in the form of a pipeline
 *      stream, with a single header at the beginning followed by
 *      all of the spectral intensities in sequential order.
 * 
 * The header values can be manipulated directly, but this is not
 * recommended.  Instead, the functions getParm() and setParm() can
 * be used to extract or set header values according to parameter
 * codes and the dimension of interest (if any).  See the source
 * code distribution for examples of these functions.
 *
 * The NMRPipe format was created to be compatible with an older format
 * which pre-dates phase-sensitive NMR and multidimensional NMR. 
 * So, for historical reasons, there are some potentially confusing
 * aspects regarding definition of dimension sizes, data types,
 * and interleaving of real and imaginary data.
 *
 * In the NMRPipe nomenclature, the dimensions are called the X-Axis,
 * Y-Axis, Z-Axis, and A-Axis.  Some rules of thumb about the data format
 * follow:
 *
 *  1. Complex data in the X-Axis is stored as separated 1D vectors
 *     of real and imaginary points (see below). 
 *
 *  2. Complex data in the Y-Axis, Z-Axis, and A-Axis is stored as
 *     interleaved real and imaginary points.
 *
 *  3. The X-Axis size is recorded as complex points.
 *
 *  4. The Z-Axis and A-Axis sizes are recorded as total points real+imag.
 *
 *  5. If both the X-Axis and Y-Axis are complex, the Y-Axis size
 *     is reported as total points real+imag.
 *
 *  6. If the X-Axis is not complex but the Y-Axis is complex,
 *     the Y-axis size is reported as complex points.
 *
 *  7. TPPI data, and Bruker QSEQ mode data are treated as real data.
 ***/

/***/
/* 1D Real Format:
 *  (512-point FDATA)
 *  (N real points...)
 *
 * 1D Complex Format:
 *  (512-point FDATA)
 *  (N real points...)
 *  (N imag points...)
 *
 * 2D Hypercomplex Format;
 * (direct dimension = t2, indirect dimension = t1):
 *
 *  (512-point FDATA)
 *  (N t2=real points... for t1=1 Real)
 *  (N t2=imag points... for t1=1 Real)
 *  (N t2=real points... for t1=1 Imag)
 *  (N t2=imag points... for t1=1 Imag)
 *  (N t2=real points... for t1=2 Real)
 *  (N t2=imag points... for t1=2 Real)
 *  (N t2=real points... for t1=2 Imag)
 *  (N t2=imag points... for t1=2 Imag)
 *  ... etc ...
 *  (N t2=real points... for t1=M Real)
 *  (N t2=imag points... for t1=M Real)
 *  (N t2=real points... for t1=M Imag)
 *  (N t2=imag points... for t1=M Imag)
 *
 * 3D Hypercomplex format: consists of a series of 2D hypercomplex
 * planes above, which are alternating real and imaginary in the third
 * dimension.
 *
 * 4D Hypercomplex format: consists of a series of 3D hypercomplex 
 * spectra above, which are alternating real and imaginary in the
 * fourth dimension.
 ***/

/***/
/* Some useful constant definitions:
 ***/

#define FDATASIZE          512   /* Length of header in 4-byte float values. */

#define FDIEEECONS   0xeeeeeeee  /* Indicates IEEE floating point format.    */
#define FDVAXCONS    0x11111111  /* Indicates DEC VAX floating point format. */
#define FDORDERCONS       2.345  /* Constant used to determine byte-order.   */ 
#define ZERO_EQUIV       -666.0  /* Might be used as equivalent for zero.    */

/***/
/* Floating point format on this computer.
 ***/

#define FDFMTCONS FDIEEECONS

/***/
/* General Parameter locations:
 ***/

#define FDMAGIC        0 /* Should be zero in valid NMRPipe data.            */
#define FDFLTFORMAT    1 /* Constant defining floating point format.         */
#define FDFLTORDER     2 /* Constant defining byte order.                    */

#define FDSIZE        99 /* Number of points in current dim R|I.             */
#define FDREALSIZE    97 /* Number of valid time-domain pts (obsolete).      */
#define FDSPECNUM    219 /* Number of complex 1D slices in file.             */
#define FDQUADFLAG   106 /* See Data Type codes below.                       */
#define FD2DPHASE    256 /* See 2D Plane Type codes below.                   */


/***/
/* Parameters defining number of dimensions and their order in the data;
 * a newly-converted FID has dimension order (2 1 3 4). These dimension
 * codes are a hold-over from the oldest 2D NMR definitions, where the
 * directly-acquired dimension was always t2, and the indirect dimension
 * was t1.
 ***/

#define FDTRANSPOSED 221 /* 1=Transposed, 0=Not Transposed.                  */
#define FDDIMCOUNT     9 /* Number of dimensions in complete data.           */
#define FDDIMORDER    24 /* Array describing dimension order.                */

#define FDDIMORDER1   24 /* Dimension stored in X-Axis.                      */
#define FDDIMORDER2   25 /* Dimension stored in Y-Axis.                      */
#define FDDIMORDER3   26 /* Dimension stored in Z-Axis.                      */
#define FDDIMORDER4   27 /* Dimension stored in A-Axis.                      */

/***/
/* The following parameters describe the data when it is
 * in a multidimensional data stream format (FDPIPEFLAG != 0):
 ***/

#define FDPIPEFLAG    57 /* Dimension code of data stream; Added for NMRPipe. */
#define FDPIPECOUNT   75 /* Number of functions in pipe;   Added for NMRPipe. */
#define FDSLICECOUNT 443 /* Number of 1D slices in stream; Added for NMRPipe. */
#define FDFILECOUNT  442 /* Number of files in complete data.                 */

/***/
/* The following definitions are used for data streams which are
 * subsets of the complete data, as for parallel processing:
 ***/

#define FDFIRSTPLANE  77 /* First Z-Plane in subset.       Added for NMRPipe. */
#define FDLASTPLANE   78 /* Last Z-Plane in subset.        Added for NMRPipe. */
#define FDPARTITION   65 /* Slice count for server mode.   Added for NMRPipe. */

#define FDPLANELOC    14 /* Location of this plane; currently unused.         */

/***/
/* The following define max and min data values, previously used
 * for contour level setting:
 ***/

#define FDMAX        247 /* Max value in real part of data.                  */
#define FDMIN        248 /* Min value in real part of data.                  */
#define FDSCALEFLAG  250 /* 1 if FDMAX and FDMIN are valid.                  */
#define FDDISPMAX    251 /* Max value, used for display generation.          */
#define FDDISPMIN    252 /* Min value, used for display generation.          */
#define FDPTHRESH    253 /* Positive threshold for peak detection.           */
#define FDNTHRESH    254 /* Negative threshold for peak detection.           */

/***/
/* Locations reserved for User customization:
 ***/

#define FDUSER1       70
#define FDUSER2       71
#define FDUSER3       72
#define FDUSER4       73
#define FDUSER5       74
#define FDUSER6       76

/***/
/* Defines location of "footer" information appended to spectral
 * data; currently unused for NMRPipe format:
 ***/

#define FDLASTBLOCK  359
#define FDCONTBLOCK  360
#define FDBASEBLOCK  361
#define FDPEAKBLOCK  362
#define FDBMAPBLOCK  363
#define FDHISTBLOCK  364
#define FD1DBLOCK    365

/***/
/* Defines data and time data was converted:
 ***/

#define FDMONTH      294
#define FDDAY        295
#define FDYEAR       296
#define FDHOURS      283
#define FDMINS       284
#define FDSECS       285

/***/
/* Miscellaneous Parameters:
 ***/

#define FDMCFLAG      135 /* Magnitude Calculation performed.               */
#define FDNOISE       153 /* Used to contain an RMS noise estimate.         */
#define FDRANK        180 /* Estimate of matrix rank; Added for NMRPipe.    */
#define FDTEMPERATURE 157 /* Temperature, degrees C.                        */
#define FD2DVIRGIN    399 /* 0=Data never accessed, header never adjusted.  */
#define FDTAU         199 /* A Tau value (for spectral series).             */

#define FDSRCNAME    286  /* char srcFile[16]  286-289 */
#define FDUSERNAME   290  /* char uName[16]    290-293 */
#define FDOPERNAME   464  /* char oName[32]    464-471 */
#define FDTITLE      297  /* char title[60]    297-311 */
#define FDCOMMENT    312  /* char comment[160] 312-351 */

/***/
/* For meanings of these dimension-specific parameters,
 * see the corresponding ND parameters below.
 ***/

#define FDF2LABEL     16
#define FDF2APOD      95
#define FDF2SW       100
#define FDF2OBS      119
#define FDF2ORIG     101
#define FDF2UNITS    152
#define FDF2QUADFLAG  56 /* Added for NMRPipe. */
#define FDF2FTFLAG   220
#define FDF2AQSIGN    64 /* Added for NMRPipe. */
#define FDF2LB       111
#define FDF2CAR       66 /* Added for NMRPipe. */
#define FDF2CENTER    79 /* Added for NMRPipe. */
#define FDF2OFFPPM   480 /* Added for NMRPipe. */
#define FDF2P0       109
#define FDF2P1       110
#define FDF2APODCODE 413
#define FDF2APODQ1   415
#define FDF2APODQ2   416
#define FDF2APODQ3   417
#define FDF2C1       418
#define FDF2ZF       108
#define FDF2X1       257 /* Added for NMRPipe. */
#define FDF2XN       258 /* Added for NMRPipe. */
#define FDF2FTSIZE    96 /* Added for NMRPipe. */
#define FDF2TDSIZE   386 /* Added for NMRPipe. */

#define FDDMXVAL      40 /* Added for NMRPipe. */
#define FDDMXFLAG     41 /* Added for NMRPipe. */
#define FDDELTATR     42 /* Added for NMRPipe. */

#define FDF1LABEL     18
#define FDF1APOD     428
#define FDF1SW       229 
#define FDF1OBS      218 
#define FDF1ORIG     249 
#define FDF1UNITS    234 
#define FDF1FTFLAG   222 
#define FDF1AQSIGN   475 /* Added for NMRPipe. */
#define FDF1LB       243
#define FDF1QUADFLAG  55 /* Added for NMRPipe. */
#define FDF1CAR       67 /* Added for NMRPipe. */
#define FDF1CENTER    80 /* Added for NMRPipe. */
#define FDF1OFFPPM   481 /* Added for NMRPipe. */
#define FDF1P0       245
#define FDF1P1       246
#define FDF1APODCODE 414
#define FDF1APODQ1   420
#define FDF1APODQ2   421 
#define FDF1APODQ3   422
#define FDF1C1       423
#define FDF1ZF       437
#define FDF1X1       259 /* Added for NMRPipe. */
#define FDF1XN       260 /* Added for NMRPipe. */
#define FDF1FTSIZE    98 /* Added for NMRPipe. */
#define FDF1TDSIZE   387 /* Added for NMRPipe. */

#define FDF3LABEL     20
#define FDF3APOD      50 /* Added for NMRPipe. */
#define FDF3OBS       10
#define FDF3SW        11
#define FDF3ORIG      12
#define FDF3FTFLAG    13
#define FDF3AQSIGN   476 /* Added for NMRPipe. */
#define FDF3SIZE      15
#define FDF3QUADFLAG  51 /* Added for NMRPipe. */
#define FDF3UNITS     58 /* Added for NMRPipe. */
#define FDF3P0        60 /* Added for NMRPipe. */
#define FDF3P1        61 /* Added for NMRPipe. */
#define FDF3CAR       68 /* Added for NMRPipe. */
#define FDF3CENTER    81 /* Added for NMRPipe. */
#define FDF3OFFPPM   482 /* Added for NMRPipe. */
#define FDF3APODCODE 400 /* Added for NMRPipe. */
#define FDF3APODQ1   401 /* Added for NMRPipe. */
#define FDF3APODQ2   402 /* Added for NMRPipe. */
#define FDF3APODQ3   403 /* Added for NMRPipe. */
#define FDF3C1       404 /* Added for NMRPipe. */
#define FDF3ZF       438 /* Added for NMRPipe. */
#define FDF3X1       261 /* Added for NMRPipe. */
#define FDF3XN       262 /* Added for NMRPipe. */
#define FDF3FTSIZE   200 /* Added for NMRPipe. */
#define FDF3TDSIZE   388 /* Added for NMRPipe. */

#define FDF4LABEL     22
#define FDF4APOD      53 /* Added for NMRPipe. */
#define FDF4OBS       28
#define FDF4SW        29
#define FDF4ORIG      30
#define FDF4FTFLAG    31
#define FDF4AQSIGN   477 /* Added for NMRPipe. */
#define FDF4SIZE      32
#define FDF4QUADFLAG  54 /* Added for NMRPipe. */
#define FDF4UNITS     59 /* Added for NMRPipe. */
#define FDF4P0        62 /* Added for NMRPipe. */
#define FDF4P1        63 /* Added for NMRPipe. */
#define FDF4CAR       69 /* Added for NMRPipe. */
#define FDF4CENTER    82 /* Added for NMRPipe. */
#define FDF4OFFPPM   483 /* Added for NMRPipe. */
#define FDF4APODCODE 405 /* Added for NMRPipe. */
#define FDF4APODQ1   406 /* Added for NMRPipe. */
#define FDF4APODQ2   407 /* Added for NMRPipe. */
#define FDF4APODQ3   408 /* Added for NMRPipe. */
#define FDF4C1       409 /* Added for NMRPipe. */
#define FDF4ZF       439 /* Added for NMRPipe. */
#define FDF4X1       263 /* Added for NMRPipe. */
#define FDF4XN       264 /* Added for NMRPipe. */
#define FDF4FTSIZE   201 /* Added for NMRPipe. */
#define FDF4TDSIZE   389 /* Added for NMRPipe. */

/***/
/* Header locations in use for packed text; adjust function
 * isHdrStr() and isHdrStr0 if new text locations are added:
 ***/

/* 286 287 288 289                                                     */
/* 290 291 292 293                                                     */
/* 464 465 466 467  468 469 470 471                                    */
/* 297 298 299 300  301 302 303 304  305 306 307 308  309 310 311      */
/* 312 313 314 315  316 317 318 319  320 321 322 323  324 325 326 327  */
/* 328 329 330 331  332 333 334 335  336 337 338 339  340 341 342 343  */
/* 344 345 346 347  348 349 350 351                                    */

#define SIZE_NDLABEL    8
#define SIZE_F2LABEL    8
#define SIZE_F1LABEL    8
#define SIZE_F3LABEL    8
#define SIZE_F4LABEL    8

#define SIZE_SRCNAME   16
#define SIZE_USERNAME  16
#define SIZE_OPERNAME  32
#define SIZE_COMMENT  160
#define SIZE_TITLE     60

/***/
/* The following are definitions for generalized ND parameters:
 ***/

#define NDPARM        1000

#define NDSIZE        (1+NDPARM)  /* Number of points in dimension.          */
#define NDAPOD        (2+NDPARM)  /* Current valid time-domain size.         */
#define NDSW          (3+NDPARM)  /* Sweep Width Hz.                         */
#define NDORIG        (4+NDPARM)  /* Axis Origin (Last Point), Hz.           */
#define NDOBS         (5+NDPARM)  /* Obs Freq MHz.                           */
#define NDFTFLAG      (6+NDPARM)  /* 1=Freq Domain 0=Time Domain.            */
#define NDQUADFLAG    (7+NDPARM)  /* Data Type Code (See Below).             */
#define NDUNITS       (8+NDPARM)  /* Axis Units Code (See Below).            */
#define NDLABEL       (9+NDPARM)  /* 8-char Axis Label.                      */
#define NDLABEL1      (9+NDPARM)  /* Subset of 8-char Axis Label.            */
#define NDLABEL2     (10+NDPARM)  /* Subset of 8-char Axis Label.            */
#define NDP0         (11+NDPARM)  /* Zero Order Phase, Degrees.              */
#define NDP1         (12+NDPARM)  /* First Order Phase, Degrees.             */
#define NDCAR        (13+NDPARM)  /* Carrier Position, PPM.                  */
#define NDCENTER     (14+NDPARM)  /* Point Location of Zero Freq.            */
#define NDAQSIGN     (15+NDPARM)  /* Sign adjustment needed for FT.          */
#define NDAPODCODE   (16+NDPARM)  /* Window function used.                   */
#define NDAPODQ1     (17+NDPARM)  /* Window parameter 1.                     */
#define NDAPODQ2     (18+NDPARM)  /* Window parameter 2.                     */
#define NDAPODQ3     (19+NDPARM)  /* Window parameter 3.                     */
#define NDC1         (20+NDPARM)  /* Add 1.0 to get First Point Scale.       */
#define NDZF         (21+NDPARM)  /* Negative of Zero Fill Size.             */
#define NDX1         (22+NDPARM)  /* Extract region origin, if any, pts.     */
#define NDXN         (23+NDPARM)  /* Extract region endpoint, if any, pts.   */
#define NDOFFPPM     (24+NDPARM)  /* Additional PPM offset (for alignment).  */
#define NDFTSIZE     (25+NDPARM)  /* Size of data when FT performed.         */
#define NDTDSIZE     (26+NDPARM)  /* Original valid time-domain size.        */
#define MAX_NDPARM   (27)

/***/
/* Axis Units, for NDUNITS:
 ***/

#define FD_SEC       1
#define FD_HZ        2
#define FD_PPM       3
#define FD_PTS       4

/***/
/* 2D Plane Type, for FD2DPHASE:
 ***/

#define FD_MAGNITUDE 0
#define FD_TPPI      1
#define FD_STATES    2
#define FD_IMAGE     3
#define FD_ARRAY     4

/***/
/* Data Type (FDQUADFLAG and NDQUADFLAG)
 ***/

#define FD_QUAD       0
#define FD_COMPLEX    0
#define FD_SINGLATURE 1
#define FD_REAL       1
#define FD_PSEUDOQUAD 2
#define FD_SE         3
#define FD_GRAD       4

/***/
/* Sign adjustment, etc, needed for FT (NDAQSIGN):
 ***/

#define ALT_NONE            0 /* No sign alternation required.                */
#define ALT_SEQUENTIAL      1 /* Sequential data needing sign alternation.    */
#define ALT_STATES          2 /* Complex data needing sign alternation.       */
#define ALT_NONE_NEG       16 /* As above, with negation of imaginaries.      */
#define ALT_SEQUENTIAL_NEG 17 /* As above, with negation of imaginaries.      */
#define ALT_STATES_NEG     18 /* As above, with negation of imaginaries.      */

#define FOLD_INVERT        -1 /* Folding requires sign inversion.             */
#define FOLD_BAD            0 /* Folding can't be performed (extracted data). */
#define FOLD_ORDINARY       1 /* Ordinary folding, no sign inversion.         */

#define DMX_ON              1 /* Use DMX adjustment.                          */
#define DMX_OFF            -1 /* Don't use DMX adjustment.                    */
#define DMX_AUTO            0 /* Use DMX adjustment if needed.                */

/***/
/* Mapping of parameter names to codes:
 ***/

#ifdef FDATA_LOCLIST

   static struct NameVal fdataLocList[] =
      {
       "FDMAGIC",       (float)FDMAGIC,
       "FDFLTFORMAT",   (float)FDFLTFORMAT,
       "FDFLTORDER",    (float)FDFLTORDER,
       "FDSIZE",        (float)FDSIZE,
       "FDREALSIZE",    (float)FDREALSIZE,
       "FDSPECNUM",     (float)FDSPECNUM,
       "FDQUADFLAG",    (float)FDQUADFLAG,
       "FD2DPHASE",     (float)FD2DPHASE,
       "FDTRANSPOSED",  (float)FDTRANSPOSED,
       "FDDIMCOUNT",    (float)FDDIMCOUNT,
       "FDDIMORDER",    (float)FDDIMORDER,
       "FDDIMORDER1",   (float)FDDIMORDER1,
       "FDDIMORDER2",   (float)FDDIMORDER2,
       "FDDIMORDER3",   (float)FDDIMORDER3,
       "FDDIMORDER4",   (float)FDDIMORDER4,
       "FDPIPEFLAG",    (float)FDPIPEFLAG,
       "FDPIPECOUNT",   (float)FDPIPECOUNT,
       "FDSLICECOUNT",  (float)FDSLICECOUNT,
       "FDFILECOUNT",   (float)FDFILECOUNT,
       "FDFIRSTPLANE",  (float)FDFIRSTPLANE,
       "FDLASTPLANE",   (float)FDLASTPLANE,
       "FDPARTITION",   (float)FDPARTITION,
       "FDPLANELOC",    (float)FDPLANELOC,
       "FDDMXVAL",      (float)FDDMXVAL,
       "FDDMXFLAG",     (float)FDDMXFLAG,
       "FDDELTATR",     (float)FDDELTATR,
       "FDMAX",         (float)FDMAX,
       "FDMIN",         (float)FDMIN,
       "FDSCALEFLAG",   (float)FDSCALEFLAG,
       "FDDISPMAX",     (float)FDDISPMAX,
       "FDDISPMIN",     (float)FDDISPMIN,
       "FDPTHRESH",     (float)FDPTHRESH,
       "FDNTHRESH",     (float)FDNTHRESH,
       "FDUSER1",       (float)FDUSER1,
       "FDUSER2",       (float)FDUSER2,
       "FDUSER3",       (float)FDUSER3,
       "FDUSER4",       (float)FDUSER4,
       "FDUSER5",       (float)FDUSER5,
       "FDLASTBLOCK",   (float)FDLASTBLOCK,
       "FDCONTBLOCK",   (float)FDCONTBLOCK,
       "FDBASEBLOCK",   (float)FDBASEBLOCK,
       "FDPEAKBLOCK",   (float)FDPEAKBLOCK,
       "FDBMAPBLOCK",   (float)FDBMAPBLOCK,
       "FDHISTBLOCK",   (float)FDHISTBLOCK,
       "FD1DBLOCK",     (float)FD1DBLOCK,
       "FDMONTH",       (float)FDMONTH,
       "FDDAY",         (float)FDDAY,
       "FDYEAR",        (float)FDYEAR,
       "FDHOURS",       (float)FDHOURS,
       "FDMINS",        (float)FDMINS,
       "FDSECS",        (float)FDSECS,
       "FDMCFLAG",      (float)FDMCFLAG,
       "FDNOISE",       (float)FDNOISE,
       "FDRANK",        (float)FDRANK,
       "FDTEMPERATURE", (float)FDTEMPERATURE,
       "FD2DVIRGIN",    (float)FD2DVIRGIN,
       "FDTAU",         (float)FDTAU,
       "FDSRCNAME",     (float)FDSRCNAME,
       "FDUSERNAME",    (float)FDUSERNAME,
       "FDOPERNAME",    (float)FDOPERNAME,
       "FDTITLE",       (float)FDTITLE,
       "FDCOMMENT",     (float)FDCOMMENT,
       "FDF2LABEL",     (float)FDF2LABEL,
       "FDF2APOD",      (float)FDF2APOD,
       "FDF2SW",        (float)FDF2SW,
       "FDF2OBS",       (float)FDF2OBS,
       "FDF2ORIG",      (float)FDF2ORIG,
       "FDF2UNITS",     (float)FDF2UNITS,
       "FDF2QUADFLAG",  (float)FDF2QUADFLAG,
       "FDF2FTFLAG",    (float)FDF2FTFLAG,
       "FDF2AQSIGN",    (float)FDF2AQSIGN,
       "FDF2LB",        (float)FDF2LB,
       "FDF2CAR",       (float)FDF2CAR,
       "FDF2CENTER",    (float)FDF2CENTER,
       "FDF2OFFPPM",    (float)FDF2OFFPPM,
       "FDF2P0",        (float)FDF2P0,
       "FDF2P1",        (float)FDF2P1,
       "FDF2APODCODE",  (float)FDF2APODCODE,
       "FDF2APODQ1",    (float)FDF2APODQ1,
       "FDF2APODQ2",    (float)FDF2APODQ2,
       "FDF2APODQ3",    (float)FDF2APODQ3,
       "FDF2C1",        (float)FDF2C1,
       "FDF2ZF",        (float)FDF2ZF,
       "FDF2X1",        (float)FDF2X1,
       "FDF2XN",        (float)FDF2XN,
       "FDF2FTSIZE",    (float)FDF2FTSIZE,
       "FDF2TDSIZE",    (float)FDF2TDSIZE,
       "FDF1LABEL",     (float)FDF1LABEL,
       "FDF1APOD",      (float)FDF1APOD,
       "FDF1SW",        (float)FDF1SW,
       "FDF1OBS",       (float)FDF1OBS,
       "FDF1ORIG",      (float)FDF1ORIG,
       "FDF1UNITS",     (float)FDF1UNITS,
       "FDF1FTFLAG",    (float)FDF1FTFLAG,
       "FDF1AQSIGN",    (float)FDF1AQSIGN,
       "FDF1LB",        (float)FDF1LB,
       "FDF1QUADFLAG",  (float)FDF1QUADFLAG,
       "FDF1CAR",       (float)FDF1CAR,
       "FDF1CENTER",    (float)FDF1CENTER,
       "FDF1OFFPPM",    (float)FDF1OFFPPM,
       "FDF1P0",        (float)FDF1P0,
       "FDF1P1",        (float)FDF1P1,
       "FDF1APODCODE",  (float)FDF1APODCODE,
       "FDF1APODQ1",    (float)FDF1APODQ1,
       "FDF1APODQ2",    (float)FDF1APODQ2,
       "FDF1APODQ3",    (float)FDF1APODQ3,
       "FDF1C1",        (float)FDF1C1,
       "FDF1ZF",        (float)FDF1ZF,
       "FDF1X1",        (float)FDF1X1,
       "FDF1XN",        (float)FDF1XN,
       "FDF1FTSIZE",    (float)FDF1FTSIZE,
       "FDF1TDSIZE",    (float)FDF1TDSIZE,
       "FDF3LABEL",     (float)FDF3LABEL,
       "FDF3APOD",      (float)FDF3APOD,
       "FDF3OBS",       (float)FDF3OBS,
       "FDF3SW",        (float)FDF3SW,
       "FDF3ORIG",      (float)FDF3ORIG,
       "FDF3FTFLAG",    (float)FDF3FTFLAG,
       "FDF3AQSIGN",    (float)FDF3AQSIGN,
       "FDF3SIZE",      (float)FDF3SIZE,
       "FDF3QUADFLAG",  (float)FDF3QUADFLAG,
       "FDF3UNITS",     (float)FDF3UNITS,
       "FDF3P0",        (float)FDF3P0,
       "FDF3P1",        (float)FDF3P1,
       "FDF3CAR",       (float)FDF3CAR,
       "FDF3CENTER",    (float)FDF3CENTER,
       "FDF3OFFPPM",    (float)FDF3OFFPPM,
       "FDF3APODCODE",  (float)FDF3APODCODE,
       "FDF3APODQ1",    (float)FDF3APODQ1,
       "FDF3APODQ2",    (float)FDF3APODQ2,
       "FDF3APODQ3",    (float)FDF3APODQ3,
       "FDF3C1",        (float)FDF3C1,
       "FDF3ZF",        (float)FDF3ZF,
       "FDF3X1",        (float)FDF3X1,
       "FDF3XN",        (float)FDF3XN,
       "FDF3FTSIZE",    (float)FDF3FTSIZE,
       "FDF3TDSIZE",    (float)FDF3TDSIZE,
       "FDF4LABEL",     (float)FDF4LABEL,
       "FDF4APOD",      (float)FDF4APOD,
       "FDF4OBS",       (float)FDF4OBS,
       "FDF4SW",        (float)FDF4SW,
       "FDF4ORIG",      (float)FDF4ORIG,
       "FDF4FTFLAG",    (float)FDF4FTFLAG,
       "FDF4AQSIGN",    (float)FDF4AQSIGN,
       "FDF4SIZE",      (float)FDF4SIZE,
       "FDF4QUADFLAG",  (float)FDF4QUADFLAG,
       "FDF4UNITS",     (float)FDF4UNITS,
       "FDF4P0",        (float)FDF4P0,
       "FDF4P1",        (float)FDF4P1,
       "FDF4CAR",       (float)FDF4CAR,
       "FDF4CENTER",    (float)FDF4CENTER,
       "FDF4OFFPPM",    (float)FDF4OFFPPM,
       "FDF4APODCODE",  (float)FDF4APODCODE,
       "FDF4APODQ1",    (float)FDF4APODQ1,
       "FDF4APODQ2",    (float)FDF4APODQ2,
       "FDF4APODQ3",    (float)FDF4APODQ3,
       "FDF4C1",        (float)FDF4C1,
       "FDF4ZF",        (float)FDF4ZF,
       "FDF4X1",        (float)FDF4X1,
       "FDF4XN",        (float)FDF4XN,
       "FDF4FTSIZE",    (float)FDF4FTSIZE,
       "FDF4TDSIZE",    (float)FDF4TDSIZE,
       "NDSIZE",        (float)NDSIZE,
       "NDAPOD",        (float)NDAPOD,
       "NDSW",          (float)NDSW,
       "NDORIG",        (float)NDORIG,
       "NDOBS",         (float)NDOBS,
       "NDFTFLAG",      (float)NDFTFLAG,
       "NDQUADFLAG",    (float)NDQUADFLAG,
       "NDUNITS",       (float)NDUNITS,
       "NDLABEL",       (float)NDLABEL,
       "NDLABEL1",      (float)NDLABEL1,
       "NDLABEL2",      (float)NDLABEL2,
       "NDP0",          (float)NDP0,
       "NDP1",          (float)NDP1,
       "NDCAR",         (float)NDCAR,
       "NDCENTER",      (float)NDCENTER,
       "NDAQSIGN",      (float)NDAQSIGN,
       "NDAPODCODE",    (float)NDAPODCODE,
       "NDAPODQ1",      (float)NDAPODQ1,
       "NDAPODQ2",      (float)NDAPODQ2,
       "NDAPODQ3",      (float)NDAPODQ3,
       "NDC1",          (float)NDC1,
       "NDZF",          (float)NDZF,
       "NDX1",          (float)NDX1,
       "NDXN",          (float)NDXN,
       "NDOFFPPM",      (float)NDOFFPPM,
       "NDFTSIZE",      (float)NDFTSIZE,
       "NDTDSIZE",      (float)NDTDSIZE,
       (char *) NULL,   0.0
      };
#endif

#ifdef FDATA_VALLIST 

   static struct NameVal fdataValList[] =
      {
       "FDATASIZE",            (float)FDATASIZE,
       "MAX_NDPARAM",          (float)MAX_NDPARM,
       "IEEECONS",             (float)FDIEEECONS,
       "VAXCONS",              (float)FDVAXCONS,
       "ORDERCONS",            (float)FDORDERCONS,
       "FMTCONS",              (float)FDFMTCONS,
       "ZERO_EQUIV",           (float)ZERO_EQUIV,
       "SEC",                  (float)FD_SEC,
       "HZ",                   (float)FD_HZ,
       "PPM",                  (float)FD_PPM,
       "PTS",                  (float)FD_PTS,
       "MAGNITUDE",            (float)FD_MAGNITUDE,
       "TPPI",                 (float)FD_TPPI,
       "STATES",               (float)FD_STATES,
       "IMAGE",                (float)FD_IMAGE,
       "QUAD",                 (float)FD_QUAD,
       "COMPLEX",              (float)FD_COMPLEX,
       "SINGLATURE",           (float)FD_SINGLATURE,
       "REAL",                 (float)FD_REAL,
       "PSEUDOQUAD",           (float)FD_PSEUDOQUAD,
       "ALT_NONE",             (float)ALT_NONE,
       "ALT_SEQUENTIAL",       (float)ALT_SEQUENTIAL,
       "ALT_STATES",           (float)ALT_STATES,
       "ALT_NONE_NEG",         (float)ALT_NONE_NEG,
       "ALT_SEQUENTIAL_NEG",   (float)ALT_SEQUENTIAL_NEG,
       "ALT_STATES_NEG",       (float)ALT_STATES_NEG,
       "FOLD_INVERT",          (float)FOLD_INVERT,
       "FOLD_BAD",             (float)FOLD_BAD,
       "FOLD_ORDINARY",        (float)FOLD_ORDINARY,
       (char *) NULL,          0.0
      };
#endif

/***/
/* Modules for manipulating header values.
 ***/

#define HDR_OK      0
#define HDR_SWAPPED 1
#define HDR_BAD     2

int   getDim();       /* Finds actual dimension code.                        */
int   getQuad();      /* Finds a quadState (e.g. test if axis is complex).   */
int   getFold();      /* Finds folding mode (e.g. how signs are changed).    */
int   getAxis();      /* Get dimension code by axis label.                   */
int   setParm();      /* Sets header value by param and dimension code.      */
float getParm();      /* Gets header value by param and dimension code.      */
int   getParmI();     /* Gets header value, integer version.                 */
char  getAxisChar();  /* Gets the 1-letter axis code x, y, z, or a.          */
char  getAxisCharU(); /* Gets the 1-letter axis code X, Y, Z, or A.          */
char  *getParmStr();  /* Gets header text by parm and dimension code.        */ 

int is90_180();       /* Test if dimension is unextracted PS(-90,180).       */
int isInterleaved();  /* Tests if dimension is interleaved quad.             */
int isHdrStr();       /* Tests if an FDATA location is inside a string.      */
int isHdrStr0();      /* Tests if an FDATA location begins a string.         */

int testHdr();        /* Tests for valid file header.                        */
int swapHdr();        /* Perform byte-swaping of appropriate header values.  */
int copyHdr();        /* Utility to copy header contents.                    */

/***/
/* Bottom.
 ***/

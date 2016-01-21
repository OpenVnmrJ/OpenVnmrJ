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
/* 
 * C structure defining fdata
 *
 * NOTA BENE : C arrays start at zero so indeicies are 1 less than FORTRASH
 *
 * Edit History
 * A.G.M.    11-Feb-86  creation
 * A.G.M.    19-Feb-86  Redefined fdata as array because of bugs in sun
 *                      C compilier
 * A.G.M.    24-Jun-86  Brought up todate with NMR2 V3.0Beta
 */

#define FDATASIZE 512

#define magic      0     /* version number of header format must be 0.0 */
#define u2               /* unused 2-97                */
#define FDRealData 97    /* number of points that are actually data     */
                         /* Bruker can acquire non power of 2 datasets  */
#define dwell      98    /* dwell in micro seconds     */
#define size       99    /* number of real data points */
#define SweepWidth 100   /* SweepWidth in Hertz/2      */
#define LastHz     101   /* location in Hz of right most data point */
#define AcqTime    102   /* Acquisition time           */
#define nscan      103   /* number of acquisitions     */
#define TotalTime  104   /* total time (seconds)       */
#define ScanInterval  105/* relaxation delay           */
#define QuadFlag   106   /* Quad spectra flag          */
                         /* 0.0 = Quad spectra         */
                         /* 1.0 = Singulature unsorted */
                         /* 2.0 = Singulature sorted   */
#define FtFlag     107     /* first dimension FT flag    */
                           /* 0.0 = time domain          */
                           /* 1.0 = frequency domain     */
#define ZFCount    108   /* number of time spectra has been zero filled */
#define ZeroPhase  109   /* Zero order phase applied   */
#define FirstPhase 110   /* first order phase applied  */
#define lb         111   /* Total line broadening      */
#define PulseAcqDly 112  /* Pulse/Acquisition Delay    */
#define filter     113   /* filter setting in Hz       */
#define exname     114   /* experiment name  4 packed ascii characters */
#define TMCount    115   /* Number of trapazodial multiplies */
#define mrT1       116   /* Most recent T1 (used by TM) */
#define mrT2       117   /* Most recent T2 (used by TM) */
#define DecouplerFreq 118/* Decoupler Frequency (MHz)   */
#define SpecFreq   119   /* Spectrometer Frequency      */
#define units      120   /* ASCIZ units                 */
                         /* "PPM","Hz ","Sec","???"     */
#define LockFreq   121   /* lock Frequency  (MHz)       */
#define DecouplerModu 122/* Decoupler Modulation        */
                         /* 0.0 Wideband                */
                         /* 1.0 None                    */
#define DecouplerPwr 123  /* Decoupler Power (watts)     */
#define AbsScale     124 /* Power of 2 this spectra has been scaled to */
#define DecouplerPos 125 /* position in PPM of decoupler */
#define DecouplerMode 126/* decoupler mode              */
                         /* 0.0 = none                  */
                         /* 1.0 = constant              */
                         /* 2.0 = Two Level             */
                         /* 3.0 = No NOE                */
                         /* 4.0 = None (NOE only)       */
#define fPseudoEcho 127  /* 1.0 if pseudo echo called   */
#define PulseWidth  128  /* pulse width (usec)          */
#define ImaginaryFlag 129/* 0.0 Imaginaries Zeroed      */
                         /* 1.0 Imaginaries Present     */
#define BFFlag      130  /* 1.0 if data baseline flattened */
#define BFOrder     131  /* BF order of fit             */
#define BFBlockCount 132  /* number of blocks used in BF */
#define FilterCorCount 133/* number of filter corrections*/
#define PulseCorCount  134/* number of pulse corrections */
#define MCFlag      135  /* 1.0 if magnitude calculation present */
#define RightShift  136  /* number of points shifted right */
#define LeftShift   137  /* number of points shifted left  */
#define WhiteOutFlag 138  /* Oswego Winter Flag             */
                          /* 1.0 if obescure data with snow squall */
#define PlotOffset   139 /* plot offset */
#define PlotVerbosity 140/* 1.0 if desire verbose plot listing */
#define PlotUnits     141/* ASCIIZ plot label Hz,ppm etc 4 char  */
#define IntegralAvail 142/* 1.0 if Integrals Available     */
#define IntegralFlag  143/* 1.0 if integrals wanted        */
#define PlotLength    144/* plot length (inches)           */
#define RelaxAvail    145/* 1.0 if T1 data is available    */
#define LineWidthAvail 146/* 1.0 if line widths available   */
#define PeakPickFlag  147/* 1.0 if peaks picked            */
#define CrReserved    148/* Reserved for CR                */
#define SnRatio       149/* signal to noise ratio          */
#define MaxPpIntensity 150 /* Maximum intensity found by peak picker */
#define SpectrumOffset 151/* Spectrum Offset                */
#define UnitsEnum     152 /* units enumeration              */
                          /* 1.0 = Sec                      */
                          /* 2.0 = Hz                       */
                          /* 3.0 = PPM                      */
#define NoiseLevel    153 /* Spectrum noise level (MH) calculated by Ndet */
                          /* in peak picker                 */
#define LineWidthAlt  154 /* 1.0 if line width has been changed */
#define PeakDataPresent 155/* 1.0 if peak data present      */
#define T1DataPresend  156/* 1.0 if T1 data present         */
#define temperature    157 /* Temperature (C)                */
#define SRFlag        158  / Spectrum Rotate flag from spectrometer */
#define GMFlag        159  /* 1.0 if Gaussian Multiply done  */
#define G1            160  /* see gm                         */
#define G2            161
#define G3            162
#define D1            163  /* see CD                         */
#define D2            164
#define D3            165
#define E1            166  /* see DE                         */
#define E2            167
#define u169          168  /* spare                          */
#define Tt            169  /* andy turk flag ????            */
#define Aa            170  /* ??????                         */
#define Aw            171  /*              ?                 */
#define Tr            172  /*               ?                */
#define u174          173  /* spare 174-177                  */
#define DTsizef       177  /* Double block size adjust flag  */ 
                           /* 0 if no adjustment done        */
                           /* 1 if size halfed and specnum doubled */
#define TFversion     178  /* version # of transfer program  */
#define exnamel       179  /* 32 character for experiment name (2d) */
#define u188          187  /* spare */
#define u189          188  /* spare */
#define PowerSpecFlag 189  /* 1.0 if power spectrum calculated */
#define AbsValueFlag  190  /* 1.0 if Absolute value calculated */
#define u192          191  /* spare 192-199 */
#define TauValue      199  /* Tau Value                       */
#define PlotType      200  /* Plot type                       */
                           /*  1.0 = Mini plot                */
                           /*  2.0 = Short plot               */
                           /*  3.0 = Horizontal plot          */
                           /*  4.0 = mystery plot             */
                           /*  5.0 = Stacked Plot             */
                           /*  6.0 = Contour Plot             */
                           /*  51.0 = Relaxation Plot         */
#define VertPlotScale 201  /* Verticle Plot Scale             */
#define PlotUnits2    202  /* ASCIIZ plot units (spare ??)    */
#define PlotExpandFull 203 /* Plot expand/full flag           */
#define PlotRcursor   204 /* Plot right cursor               */
#define PlotLcursor   205  /* Plot Left cursor                */
#define PlotXoffset   206  /* Plot X offset                   */
#define PlotYoffset   207  /* Plot Y offset                   */
#define PlotCount     208  /* Plot Count                      */
#define NumOfContours 209  /* number of Contours              */
#define PlotDest      210  /* Plot Destiination               */
                           /* 1.0 = Plotter 1                 */
                           /* 2.0 = Plotter 2                 */
                           /* -1.0 = Inline terminal plotter  */
#define PlotTcursor   211  /* ? */
#define PlotBcursor   212  /* ? */
#define u214          213  /* spare 214-219                   */
#define SpecNum       219  /* number of spectra               */
#define FirstFTFlag   220  /* 2d first Ft flag                */
                           /* 1.0 if first FT done            */
#define TransposeFlag 221  /* 2d Transposition flag           */
                           /* 1.0 if data is transposed       */
#define SecondFTFlag  222  /* 2d second FT flag               */
                           /* 1.0 if second FT done           */
#define u224          223  /* spare 224-229                   */
#define SecondDimSw   229  /* second dimension Sweep Width    */
#define u231          230  /* spares 231-234                  */
#define SecDimUnits   234  /* Second Dimension Units          */
                           /* 0.0 = Sec                       */
                           /* 1.0 = Hz                        */
                           /* 2.0 = PPM                       */
#define u236          235  /* spares 236 - 239                */
#define SecondDimScale 239 /* 2nd dimension scaline factor    */
#define u241          240  /* spares 241 - 243                */ 
#define SecDimLB      243  /* 2nd dimension line broading     */
#define u245          244  /* spare */
#define SecondPhase0  245  /* second dimension zero order phase */
#define SecondPhase1  246  /* second dimension first order phase */
#define MaxDataPoint  247  /* Maxinum data point              */
#define MinDataPoint  248  /* Minimum data point              */
#define LastSpectrumHz 249 /* locatin in Hz of last Spectrum  */
#define ScalingFlag   250  /* Scaling Flag                    */
#define DisplayMax    251  /* Data Max for display calcs      */
#define DisplayMin    252  /* data min for display calcs      */
#define u254          253  /* spare                           */
#define u255          254
#define TwoDExperiment 255 /* future site of 2d experiment type flag */
#define TwoDPhaseMode 256  /* 2d phase sensitive Mode         */
                           /* 0.0 Not phase sensitive         */
                           /* 1.0 TPPI single block phase sensitive */
                           /* 2.0 double blok phase sensitive */
#define u258          257  /* spares 258 - 270                */
#define is2dset       270  /* 1.0 if a twod data set          */
#define u272          272  /* spares 272-279                  */
#define Spectrometer  279  /* spectrometer name char[8]       */
#define u282          281
#define u283          282
#define hours         283  /* currently time and date of transfer */
#define minutes       284
#define seconds       285
/*    char eclipseFilename[16]  Eclipse file name loaded by getfile ??? */
#define eclipseFilename 286
/*    char username[16]       username                           */  
#define USERNAME      290
#define month         294
#define day           295
#define year          296
#define title         297   /*    char title[60]                */
#define comment       312   /*    char comment[160]             */
#define ContLabelFlag 352   /* contour label flag (fdata[353])  */
#define PlotIntFlag   353   /* plot integrals if 1.0           */
#define IntReset      354   /* array of places to reset integrals */
#define IntVertScale  355   /* verticle scale factor for integrals */
#define LastFileBlock 359   /* last block in use of datafile    */
                            /* first block is zero              */
#define ContourBlock  360   /* first block where contour info is stored */
#define BaselineBlock 361   /* first block with baseline info   */
#define PeakTabBlock  362   /* first block of 2d peak table      */
#define u364          363   /* spares 364-369                    */
#define FirstBurgIter 369   /* first dim Burg mem iterations            */
                            /* negative value indicates filtered mem    */
#define SecondBurgIter 370  /* second dim Burg mem iterations           */
                            /*  negative value indicates filtered mem   */
#define u372          371   /* spares 371 - 399                 */
#define NMR2virginFlag 399  /* 0.0 if data never accessed by NMR2 */
                            /* 1.0 if accessed at least once    */
#define u401          400   /* spares 401 - 409                 */
#define BaselineCorr  409   /* 1.0 if baseline correction subtracted */
#define BogusLogs     410   /* Total bogus logs taken           */
#define Dx            411   /* Total derivatives in first dimension */
#define SecondDimDx   412   /* Total derivatives in second dimension */
#define ApodizFunc    413   /* Index in function table of first dim apodize */
#define SecDimApodFun 414   /* ditto for second dimension */
#define Apodize1      415   /* first dimension generic apodize param 1 */
#define Apodize2      416   /* generic parameter 2 */
#define Apodize3      417
#define Apodize4      418
#define Apodize5      419
#define SecApodize1   420   /* second dimension gemeric apodize parameter 1 */
#define SecApodize2   421
#define SecApodize3   422
#define SecApodize4   423
#define SecApodize5   424
#define Integrals     425  /* Total first dimension integrals taken */
#define SecDimInteg   426  /* Total 2nd dimensin integrals taken    */
#define InitialSize   427  /* First Dimension Reduce                */
                           /* value is orignal size of untransposed data set */
                           /* before size reduction                 */
#define InitialSpecNum 428 /* Second dimension reduce             */
                           /* value is orignal specnum of untransposed */
                           /* datased before size reduction        */
#define FirstReverse  429  /* 0.0 = Normal                         */
                           /* 1.0 = Reversed                       */
#define SecDimReverse 430  /* 0.0 = Normal                         */
                           /* 1.0 = Reversed                       */
#define LateralSymFunc 431 /* see 2d code for SY                   */
#define DiagSymFunc   432  /* see NMR2 DI, DD                      */
#define Tilt          433  /* Tilt parameter -                     */
                           /* right shift in points for last spectrum */
#define ImagZeroFlag  434  /* 0.0 Imags normal                     */
                           /* 1.0 Imags zeroed                     */
#define ImagsSwapped  435  /* 0.0 Imags Normal                     */
                           /* Imags Swapped with reals             */
#define ImagsErased   436  /* 0.0 Imags Normal or never present    */
                           /* 1.0 Imags once presend but now eliminated */
#define SecZeroFill   437  /* powers of two second dimension zero filling */
#define u439               /* unused 439 - 511                     */

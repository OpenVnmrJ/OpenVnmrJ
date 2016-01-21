C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    0003
C  CONTIN.  MAIN SUBPROGRAM.                                                0004
c
C  Download the Users Manual and other essential documentation from
c    http://S-provencher.COM
c
c 06-Dec-01: 500 light scattering form factors also allowed (increased from the 
c              unnecessarily small 50).
C
C 20-Jan-99: DIMENSION specifications increased to handle up to 8192 data 
C                points and about 500 grid points.  Section 3.5 of the manual 
C                explains how you can change these, if memory is limited.
C              If computation time is critical and memory limited, then 
C                reducing the maximum number of grid points as much as 
C                possible might help a little.
C              Increasing the DIMENSION specifications for more than 8192 
C                data points is no problem.
C              More than 500 grid points is not recommended, because of 
C                possible problems with numerical stability.  
C              With difficult inversions, such as Laplace transforms, NG, 
C                the number of grid points, should probably not exceed 200.  
C                However, NG is only an input parameter; you don't have to 
C                reduce the DIMENSION specifications or make any changes to 
C                the code.  
C
C              In each run, all the DIMENSION specifications are tested.  Thus, 
C                you can simply try running, and see if CONTIN aborts with a 
C                diagnostic telling you to increase a DIMENSION specification.  
C                If it does not abort, then everything is OK.
C
C==============================================================================
C              Hints for inverting noisy Laplace transforms:
C              =============================================
C              With Laplace inversions, you might use the following Control 
C                Parameters:
C                  NONNEG=T (if the solution is known to be nonnegative; without
C                            this constraint the resolution is extremely poor.)
C                  IUSER(10)=2
C                  GMNMX(1) about 0.1/(maximum time value in your data of 
C                                      signal vs. time)
C                  GMNMX(2) about 4/(time-spacing between data points at the 
C                                    shortest times)
C                  NG about 12*log10[GMNMX(2)/GMNMX(1)]
C                  IFORMY, NINTT, etc., as appropriate for your data.
C
C              The CHOSEN SOLUTION gives you a conservative (smooth) estimate 
C                of a possible continuous distribution of exponentials.
C
C              The Reference Solution (the solution with the smallest ALPHA) 
C                gives you the optimal analysis as a discrete sum of 
C                exponentials.  The number of discrete exponentials is the 
C                number of peaks.  Each amplitude is given by MOMEMT(0) for 
C                the corresponding peak.  The decay rate constant is given by 
C                MOMENT(1)/MOMENT(0).
C
C              Choose the solution with the smallest ALPHA that has the same 
C                number of peaks as the CHOSEN SOLUTION.  This solution has 
C                less smoothing bias (smaller ALPHA), but still has about 
C                the same complexity (number of peaks) as the CHOSEN SOLUTION.
C===============================================================================
C
C  5-Mar-94: COMMON blocks /MS1/-/MS6/ were added to simplify compilation on 
C              PCs that have restricted easily useable memory.
C
C           With MS-DOS, you will probably have to break this file into 3 or 4 
C             segments for compiling and then LINK these object files together.
C             You will probably also have to drastically reduce the DIMENSION 
C             specifications for the maximum number of grid points (down to 
C             about 50); Section 3.5 of the manual tells you how to do this.  
C
C  
C  FOR THE REGULARIZED SOLUTION OF LINEAR ALGEBRAIC AND                     0005
C      LINEAR FREDHOLM INTEGRAL EQUATIONS OF THE FIRST KIND, WITH           0006
C      OPTIONS FOR PEAK CONSTRAINTS AND LINEAR EQUALITY AND INEQUALITY      0007
C      CONSTRAINTS.                                                         0008
C  REFERENCES - S.W. PROVENCHER (1982) COMPUT. PHYS. COMMUN., VOL. 27,      0009
C                                      PAGES 213-227, 229-242.              0010
C                               (1984) CONTIN USERS MANUAL (EMBL            0011
C                                      TECHNICAL REPORT DA07).              0012
C
C  As a new user you should notify S.W. Provencher at:        
C      sp@S-provencher.COM
C                                                                           0027
C-----------------------------------------------------------------------    0028
C  CALLS SUBPROGRAMS - BLOCK DATA, INIT, INPUT, SETGRD, USERSI,             0029
C      WRITYT, USERSX, USERNQ, SETNNG, ANALYZ, SETWT                        0030
C  WHICH IN TURN CALL - STORIN, READYT, ERRMES, WRITIN, USERIN,             0031
C      USERGR, CQTRAP, USERTR, USEREX, RGAUSS, RANDOM, SETSCA, SEQACC,      0032
C      H12, GETROW, USERK, USERLF, SETREG, USERRG, USEREQ, ELIMEQ,          0033
C      LH1405, SVDRS2, QRBD, G1,G2, DIFF, DIAREG, DIAGA, SETGA1,            0034
C      SETVAL, LDPETC, LDP, NNLS, CVNEQ, FISHNI, GAMLN,                     0035
C      BETAIN, PLPRIN, USEROU, MOMENT, MOMOUT, RUNRES, GETPRU, GETYLY,      0036
C      PGAUSS, PLRES, SETSGN, ANPEAK, UPDSGN, UPDDON, FFLAT, UPDLLS,        0037
C      USERWT                                                               0038
C-----------------------------------------------------------------------    0039
      DOUBLE PRECISION PRECIS, RANGE                                        0040
      DOUBLE PRECISION A, AA, AEQ, AINEQ, PIVOT, REG, RHSNEQ,               0041
     1 S, SOLBES, SOLUTN, SSCALE, VALPCV, VALPHA, VK1Y1, WORK               0042
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  0043
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              0044
      LOGICAL LBIND                                                         0045
C                                                                           0046
C***********************************************************************    0047
C  THE INSTRUCTIONS SET OFF BY ASTERISKS DESCRIBE ALL POSSIBLE CHANGES      0048
C      THAT YOU MAY HAVE TO MAKE IN THIS MAIN SUBPROGRAM.  (SEE ALSO THE    0049
C      CHANGES IN THE BLOCK DATA AND USER SUBPROGRAMS.)  THESE CHANGES      0050
C      IN THE MAIN SUBPROGRAM ARE ONLY NECESSARY IF YOU CHANGE MY, MA,      0051
C      MG, MREG, MINEQ, MEQ, MDONE, OR MWORK IN THE DATA STATEMENT          0052
C      BELOW.  IF YOU DO, THEN THE FOLLOWING DIMENSIONS MUST BE             0053
C      READJUSTED AS DESCRIBED BELOW -                                      0054
C                                                                           0055
      COMMON /MS1/ AA
      COMMON /MS2/ AINEQ
      COMMON /MS3/ A
      COMMON /MS4/ REG
      COMMON /MS5/ AEQ
      COMMON /MS6/ WORK
      DIMENSION T(8192), SQRTW(8192), Y(8192), EXACT(8192), YLYFIT(8192)
      DIMENSION G(503), CQUAD(503), VK1Y1(503), S(503,3), VALPHA(503),
     1 VALPCV(503), SOLUTN(503), IISIGN(503), SOLBES(503),
     2 AA(503,503), SSCALE(503)
      DIMENSION AINEQ(501,503), RHSNEQ(501), LBIND(501)
      DIMENSION A(503,503), IWORK(503)
      DIMENSION REG(504,503)
      DIMENSION AEQ(11,503), PIVOT(11)
      DIMENSION WORK(253508)
      DIMENSION LSDONE(90,3,2), VDONE(90)                                   0065
C                                                                           0066
C  THE ABOVE DIMENSION STATEMENTS MUST BE ADJUSTED AS FOLLOWS -             0067
C                                                                           0068
C     DIMENSION T(MY), SQRTW(MY), Y(MY), EXACT(MY), YLYFIT(MY)              0069
C     DIMENSION G(MG), CQUAD(MG), VK1Y1(MG), S(MG,3), VALPHA(MG),           0070
C    1 VALPCV(MG), SOLUTN(MG), IISIGN(MG), SOLBES(MG),                      0071
C    2 AA(MG,MG), SSCALE(MG)                                                0072
C     DIMENSION AINEQ(MINEQ,MG), RHSNEQ(MINEQ), LBIND(MINEQ)                0073
C     DIMENSION A(MA,MG), IWORK(MA)                                         0074
C     DIMENSION REG(MREG,MG)                                                0075
C     DIMENSION AEQ(MEQ,MG), PIVOT(MEQ)                                     0076
C     DIMENSION WORK(MWORK)                                                 0077
C     DIMENSION LSDONE(MDONE,3,2), VDONE(MDONE)                             0078
C***********************************************************************    0079
C                                                                           0080
      COMMON /DBLOCK/ PRECIS, RANGE                                         0081
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         0082
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     0083
     2 SRANGE                                                               0084
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     0085
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       0086
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             0087
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              0088
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         0089
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            0090
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  0091
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            0092
     2 LUSER(30)                                                            0093
C                                                                           0094
C***********************************************************************    0095
C  YOU CAN SAVE STORAGE BY MAKING THE INTEGERS IN THE FOLLOWING DATA        0096
C      STATEMENT AS SMALL AS THE SIZE OF YOUR PROBLEM WILL ALLOW.  (SEE     0097
C      USERS MANUAL FOR THE MINIMUM ALLOWABLE VALUES.)                      0098
C                                                                           0099
      DATA MY/8192/, MA/503/, MG/503/, MREG/504/, MINEQ/501/, MEQ/11/,
     1 MDONE/90/, MWORK/253508/
C                                                                           0102
C  IF YOU CHANGE THE ABOVE DATA STATEMENT, THEN THE DIMENSION               0103
C      STATEMENTS ABOVE MUST BE READJUSTED, AS DESCRIBED ABOVE.             0104
C                                                                           0105
C  THIS IS THE END OF ALL POSSIBLE CHANGES THAT YOU MIGHT HAVE TO MAKE      0106
C      IN THE MAIN PROGRAM,                                                 0107
C      EXCEPT THAT IF YOUR SYSTEM DOES NOT AUTOMATICALLY                    0108
C      OPEN INPUT AND OUTPUT FILES FOR YOU, THEN YOU MIGHT HAVE TO OPEN     0109
C      THEM HERE AND GIVE THEM THE NUMBERS NIN (FOR THE INPUT) AND          0110
C      NOUT (FOR THE OUTPUT), WHERE NIN AND NOUT HAVE BEEN SET IN THE       0111
C      BLOCK DATA SUBPROGRAM.                                               0112
C  IN ADDITION, IF YOU ARE GOING TO INPUT IUNIT.GE.0, THEN YOU MAY          0113
C      HAVE TO OPEN A TEMPORARY SCRATCH FILE NUMBERED IUNIT.  DO THIS       0114
C      DIRECTLY AFTER STATEMENT 100.  THIS IS NOT NECESSARY IF IUNIT        0115
C      IS NEGATIVE OR IF YOUR SYSTEM OPENS FILES AUTOMATICALLY.             0116
C***********************************************************************    0117
C                                                                           0118
C-----------------------------------------------------------------------    0119
C  INITIALIZE VARIABLES                                                     0120
C-----------------------------------------------------------------------    0121
      CALL INIT                                                             0122
C-----------------------------------------------------------------------    0123
C  READ INPUT DATA                                                          0124
C-----------------------------------------------------------------------    0125
  100 CALL INPUT (EXACT,G,MA,MEQ,MG,MINEQ,MREG,MWORK,MY,SQRTW,T,Y)          0126
C-----------------------------------------------------------------------    0127
C  SET UP QUADRATURE GRID                                                   0128
C-----------------------------------------------------------------------    0129
      CALL SETGRD (CQUAD,G,GMNMX,IGRID,IQUAD,MG,NG,NOUT)                    0130
C-----------------------------------------------------------------------    0131
C  CALCULATE SIMULATED DATA                                                 0132
C-----------------------------------------------------------------------    0133
      IF (SIMULA) CALL USERSI (EXACT,G,MG,MY,SQRTW,T,Y)                     0134
C-----------------------------------------------------------------------    0135
C  WRITE OUT SIMULATED DATA                                                 0136
C-----------------------------------------------------------------------    0137
      IF (SIMULA) CALL WRITYT (EXACT,G,IPRINT,IUSROU,IWT,MG,NOUT,NY,        0138
     1 PRY,SIMULA,SQRTW,T,Y)                                                0139
C-----------------------------------------------------------------------    0140
C  PUT SECOND CURVE TO BE PLOTTED WITH SOLUTION IN EXACT.                   0141
C-----------------------------------------------------------------------    0142
      IF (.NOT.ONLY1) CALL USERSX (EXACT,G,MG)                              0143
      NINEQ=0                                                               0144
      NGL=NG+NLINF                                                          0145
      NGLP1=NGL+1                                                           0146
C-----------------------------------------------------------------------    0147
C  SET SPECIAL USER-SUPPLIED INEQUALITY CONSTRAINTS                         0148
C-----------------------------------------------------------------------    0149
      IF (DOUSNQ) CALL USERNQ (AINEQ,MG,MINEQ)                              0150
C-----------------------------------------------------------------------    0151
C  SET NG NONNEGATIVITY CONSTRAINTS AT ALL NG GRID POINTS                   0152
C-----------------------------------------------------------------------    0153
      IF (NONNEG) CALL SETNNG (AINEQ,MINEQ,NG,NGLP1,NINEQ,NOUT)             0154
      IF (IWT.EQ.1 .OR. IWT.EQ.4) GO TO 200                                 0155
C-----------------------------------------------------------------------    0156
C  DO A COMPLETE PRELIMINARY UNWEIGHTED ANALYSIS TO GET A SMOOTH FIT        0157
C      TO THE DATA.  THIS SMOOTH CURVE IS THEN USED TO CALCULATE THE        0158
C      WEIGHTS.                                                             0159
C-----------------------------------------------------------------------    0160
      CALL ANALYZ (1,                                                       0161
     1 A,AA,AEQ,AINEQ,CQUAD,EXACT,G,IISIGN,IWORK,LBIND,LSDONE,MA,           0162
     2 MDONE,MEQ,MG,MINEQ,MREG,MWORK,MY,PIVOT,REG,RHSNEQ,S,SOLBES,          0163
     3 SOLUTN,SQRTW,SSCALE,T,VALPCV,VALPHA,VDONE,VK1Y1,WORK,                0164
     4 Y,YLYFIT)                                                            0165
C-----------------------------------------------------------------------    0166
C  CALCULATE SQRTW (SQUARE ROOT OF LEAST SQUARES WEIGHTS).                  0167
C-----------------------------------------------------------------------    0168
      CALL SETWT (                                                          0169
     1 CQUAD,G,IUNIT,IWT,MWORK,MY,NERFIT,NG,NGL,NLINF,NOUT,NY,PRWT,         0170
     2 SOLBES,SQRTW,SRANGE,SSCALE,T,WORK,Y,YLYFIT)                          0171
C-----------------------------------------------------------------------    0172
C  DO FINAL WEIGHTED ANALYSIS.                                              0173
C-----------------------------------------------------------------------    0174
  200 CALL ANALYZ (2,                                                       0175
     1 A,AA,AEQ,AINEQ,CQUAD,EXACT,G,IISIGN,IWORK,LBIND,LSDONE,MA,           0176
     2 MDONE,MEQ,MG,MINEQ,MREG,MWORK,MY,PIVOT,REG,RHSNEQ,S,SOLBES,          0177
     3 SOLUTN,SQRTW,SSCALE,T,VALPCV,VALPHA,VDONE,VK1Y1,WORK,                0178
     4 Y,YLYFIT)                                                            0179
      IF (.NOT.LAST) GO TO 100                                              0180
      STOP                                                                  0181
      END                                                                   0182
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    0183
C  BLOCK DATA SUBPROGRAM.                                                   0184
      BLOCK DATA                                                            0185
      DOUBLE PRECISION PRECIS, RANGE                                        0186
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  0187
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              0188
      COMMON /DBLOCK/ PRECIS, RANGE                                         0189
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         0190
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     0191
     2 SRANGE                                                               0192
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     0193
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       0194
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             0195
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              0196
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         0197
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            0198
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  0199
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            0200
     2 LUSER(30)                                                            0201
C                                                                           0202
C***********************************************************************    0203
C  YOU MUST SET THE FOLLOWING 4 VARIABLES TO VALUES APPROPRIATE FOR         0204
C      YOUR COMPUTER.  (SEE USERS MANUAL.)                                  0205
C     DATA RANGE/1.E35/, SRANGE/1.E35/, NIN/5/, NOUT/6/!SP                  0206
      DATA RANGE/1.D35/, SRANGE/1.E35/, NIN/5/, NOUT/6/                     0207
C***********************************************************************    0208
C                                                                           0209
C                                                                           0210
C***********************************************************************    0211
C  CONTIN WILL USE THE VALUES OF THE CONTROL VARIABLES THAT ARE IN THE      0212
C      DATA STATEMENTS BELOW UNLESS YOU INPUT DIFFERENT VALUES.  TO SAVE    0213
C      INPUTTING THESE OFTEN, YOU CAN CHANGE THE VALUES BELOW TO THOSE      0214
C      THAT YOU WILL USUALLY USE.                                           0215
C  THE FOLLOWING DATA STATEMENTS CONTAIN (IN ALPHABETICAL ORDER) THE        0216
C      REAL, INTEGER, AND LOGICAL CONTROL VARIABLES, IN THAT ORDER.         0217
      DATA ALPST/2*0./, DFMIN/2./, GMNMX/2*0./, PLEVEL/4*.5/,               0218
     1 RSVMNX/2*1., 2*0./, RUSER/551*0./,                                   0219
     2 SRMIN/.01/                                                           0220
      DATA ICRIT/2*1/,                                                      0221
     1 IFORMT/1H(, 1H5, 1HE, 1H1, 1H5, 1H., 1H6, 1H), 62*1H /,              0222
     2 IFORMW/1H(, 1H5, 1HE, 1H1, 1H5, 1H., 1H6, 1H), 62*1H /,              0223
     3 IFORMY/1H(, 1H5, 1HE, 1H1, 1H5, 1H., 1H6, 1H), 62*1H /,              0224
     4 IGRID/2/, IPLFIT/2*2/, IPLRES/2*2/, IPRINT/2*4/, IQUAD/3/,           0225
     5 IUNIT/-1/, IUSER/9*0, 2, 7*0, 50, 32*0/, IUSROU/2*0/, IWT/1/,        0226
     6 LINEPG/60/, LSIGN/16*0/, MIOERR/5/, MOMNMX/-1, 3/, MPKMOM/5/,        0227
     7 MQPITR/35/, NENDZ/2, 2/, NEQ/0/, NERFIT/10/, NFLAT/8*0/, NG/31/,     0228
     8 NINTT/1/, NLINF/0/, NNSGN/2*0/, NORDER/2/, NQPROG/6, 6/,             0229
     9 NSGN/4*0/                                                            0230
      DATA DOCHOS/.TRUE./, DOMOM/.TRUE./, DOUSIN/.TRUE./,                   0231
     1 DOUSNQ/.FALSE./, LAST/.TRUE./,                                       0232
     2 LUSER/30*.FALSE./, NEWPG1/.FALSE./, NONNEG/.TRUE./,                  0233
     3 ONLY1/.TRUE./, PRWT/.TRUE./, PRY/.TRUE./,                            0234
     4 SIMULA/.FALSE./                                                      0235
C***********************************************************************    0236
C                                                                           0237
C                                                                           0238
C***********************************************************************    0239
C  IF YOU MAKE ANY CHANGES TO CONTIN, YOU SHOULD PUT A NAME (UP TO 6        0240
C      CHARACTERS) IN IAPACK TO UNIQUELY IDENTIFY YOUR VERSION OF           0241
C      CONTIN.  THIS WILL BE PRINTED IN THE HEADING OF VARIOUS PARTS OF     0242
C      THE OUTPUT.  YOU MUST SPECIFY THE NAME IN THE FOLLOWING STATEMENT    0243
      DATA IAPACK/1H ,1HP, 1HC, 1HS, 1H-, 1H1/                              0244
C***********************************************************************    0245
C                                                                           0246
      END                                                                   0247
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    0248
C  SUBROUTINE USEREQ.  THIS IS A USER-SUPPLIED ROUTINE (NEEDED              0249
C      WHEN NEQ IS INPUT POSITIVE) TO PUT THE EQUALITY-CONSTRAINT           0250
C      MATRIX IN THE FIRST NEQ ROWS AND NGL COLUMNS OF AEQ, AND             0251
C      THE RIGHT-HAND-SIDE OF THE EQUALITIES IN COLUMN NGLP1=NGL+1.         0252
C  CQUAD CONTAINS THE COEFFICIENTS OF THE QUADRATURE FORMULA THAT WERE      0253
C      SET IN SETGRD.                                                       0254
C  NOTE - IF WEIGHTS ARE TO BE CALCULATED (I.E., IF IWT=2,3, OR 5), THEN    0255
C      USEREQ WILL BE CALLED TWICE.  THEREFORE IT IS BEST TO HAVE ANY       0256
C      DATA NEEDED BY USEREQ READ IN ONLY ONCE AND STORED, E.G., IN         0257
C      RUSER, AS ILLUSTRATED BELOW.                                         0258
C  SEE THE USERS MANUAL FOR THE POSITION IN THE INPUT                       0259
C      DATA DECK OF ANY INPUT FOR THIS USER SUBPROGRAM.                     0260
C  BELOW IS ILLUSTRATED THE CASE WHERE THE END POINTS AND THE               0261
C      INTEGRAL OVER THE SOLUTION CAN BE CONSTRAINED -                      0262
C      IF NEQ=1, THEN THE SOLUTION IS CONSTRAINED TO BE RUSER(1) AT         0263
C        THE GRID POINT NG (THE LAST POINT).                                0264
C      IF NEQ=2, THEN, IN ADDITION, THE SOLUTION IS CONSTRAINED TO          0265
C        RUSER(2) AT GRID POINT 1                                           0266
C      IF NEQ=3, THEN, IN ADDITION, THE INTEGRAL OVER THE SOLUTION          0267
C      (USING THE QUADRATURE APPROXIMATION) IS CONSTRAINED TO BE            0268
C      RUSER(6).  NEQ = 1, 2 OR 3 AND RUSER(J), J=1, (AND 2 AND 6           0269
C      IF NEQ=2 OR 3) WOULD HAVE TO HAVE BEEN PREVIOUSLY INPUT.             0270
      SUBROUTINE USEREQ (AEQ,CQUAD,MEQ,MG)                                  0271
      DOUBLE PRECISION PRECIS, RANGE                                        0272
      DOUBLE PRECISION AEQ                                                  0273
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  0274
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              0275
      DIMENSION AEQ(MEQ,MG), CQUAD(MG)                                      0276
      COMMON /DBLOCK/ PRECIS, RANGE                                         0277
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         0278
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     0279
     2 SRANGE                                                               0280
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     0281
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       0282
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             0283
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              0284
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         0285
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            0286
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  0287
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            0288
     2 LUSER(30)                                                            0289
C     ZERO=0.E0!SP                                                          0290
      ZERO=0.D0                                                             0291
C     ONE=1.E0!SP                                                           0292
      ONE=1.D0                                                              0293
C-----------------------------------------------------------------------    0294
C  YOU MUST REPLACE THE FOLLOWING STATEMENTS WITH THOSE APPROPRIATE         0295
C      FOR YOUR APPLICATION.                                                0296
C-----------------------------------------------------------------------    0297
      IF (NEQ.GE.1 .AND. NEQ.LE.3) GO TO 105                                0298
 5105 FORMAT (/6H NEQ =,I3,28H IS NOT 1, 2 OR 3 IN USEREQ.)                 0299
      WRITE (NOUT,5105) NEQ                                                 0300
      STOP                                                                  0301
  105 L=MIN0(NEQ,2)                                                         0302
      DO 110 J=1,L                                                          0303
        DO 120 K=1,NGL                                                      0304
          AEQ(J,K)=ZERO                                                     0305
  120   CONTINUE                                                            0306
        AEQ(J,NGLP1)=RUSER(J)                                               0307
  110 CONTINUE                                                              0308
      AEQ(1,NG)=ONE                                                         0309
      IF (NEQ .GT. 1) AEQ(2,1)=ONE                                          0310
      IF (NEQ .NE. 3) GO TO 800                                             0311
      DO 130 K=1,NGL                                                        0312
        AEQ(3,K)=ZERO                                                       0313
        IF (K .LE. NG) AEQ(3,K)=CQUAD(K)                                    0314
  130 CONTINUE                                                              0315
      AEQ(3,NGLP1)=RUSER(6)                                                 0316
  800 RETURN                                                                0317
      END                                                                   0318
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    0319
C  FUNCTION USEREX.  THIS IS A USER-SUPPLIED FUNCTION (ONLY USED            0320
C      IF SIMULA=.TRUE.) TO EVALUATE THE NOISE-FREE VALUE OF THE            0321
C      SIMULATED DATA AT T(IROW) (THE INDEPENDENT VARIABLE AT THE           0322
C      DATA POINT IROW).                                                    0323
C  BELOW IS ILLUSTRATED THE CASE OF THE SOLUTION BEING COMPOSED OF          0324
C        A SUM OF IUSER(11) DIRAC DELTA FUNCTIONS LOCATED AT                0325
C        RUSER(25), RUSER(27), ... , RUSER(23+2*IUSER(11))                  0326
C        WITH RESPECTIVE AMPLITUDES                                         0327
C        RUSER(26), RUSER(28), ... , RUSER(24+2*IUSER(11)).                 0328
C      IN ADDITION, THE NLINF FUNCTIONS IN USERLF ARE ADDED WITH            0329
C        RESPECTIVE AMPLITUDES                                              0330
C        RUSER(41), RUSER(42), ... , RUSER(40+NLINF).                       0331
C      IUSER(11) AND NLINF CANNOT BOTH BE LESS THAN ONE.                    0332
C      IUSER(11) CANNOT EXCEED 7.                                           0333
C      NLINF CANNOT EXCEED 10.                                              0334
C      WHEN IWT=5, THE NORMALIZATION CONSTANT, RUSER(14), IS COMPUTED       0335
C        FOR THE SPECIAL CASE OF PHOTON CORRELATION SPECTROSCOPY SO THAT    0336
C        GAMMA = RUSER(26)+RUSER(28)+...+RUSER(24+2*IUSER(11)), WHERE       0337
C        GAMMA IS GIVEN IN EQ.(6) IN MAKROMOL. CHEM., VOL. 180, P. 201      0338
C        (1979).                                                            0339
C-----------------------------------------------------------------------    0340
C  CALLS SUBPROGRAMS - USERLF, USERK, ERRMES                                0341
C  WHICH IN TURN CALL - USERTR                                              0342
C-----------------------------------------------------------------------    0343
      FUNCTION USEREX (IROW,T,MY,G,MG)                                      0344
      DOUBLE PRECISION PRECIS, RANGE                                        0345
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  0346
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              0347
      DIMENSION T(MY), IHOLER(6), ADUM(1), G(MG)                            0348
      COMMON /DBLOCK/ PRECIS, RANGE                                         0349
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         0350
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     0351
     2 SRANGE                                                               0352
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     0353
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       0354
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             0355
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              0356
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         0357
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            0358
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  0359
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            0360
     2 LUSER(30)                                                            0361
       DATA IHOLER/1HU, 1HS, 1HE, 1HR, 1HE, 1HX/                            0362
C-----------------------------------------------------------------------    0363
C  THE FOLLOWING STATEMENTS SHOULD BE REPLACED WITH THE ONES                0364
C      APPROPRIATE FOR YOUR SIMULATION.                                     0365
C-----------------------------------------------------------------------    0366
      IF (NLINF.GT.10 .OR. IUSER(11).GT.8 .OR.                              0367
     1 MAX0(NLINF,IUSER(11)).LE.0) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)       0368
      USEREX=0.                                                             0369
      IF (NLINF .LE. 0) GO TO 150                                           0370
      DO 110 J=1,NLINF                                                      0371
        JJ=J                                                                0372
        IF (ABS(RUSER(J+40)) .GT. 0.) USEREX=USEREX+RUSER(J+40)*            0373
     1  USERLF(IROW,JJ,T,MY)                                                0374
  110 CONTINUE                                                              0375
  150 IF (IUSER(11) .LE. 0) GO TO 800                                       0376
      JJ=IUSER(11)                                                          0377
      IF (IROW .GT. 1) GO TO 158                                            0378
      RUSER(14)=1.                                                          0379
      IF (IWT .NE. 5) GO TO 158                                             0380
      RUSER(14)=0.                                                          0381
      AMPSUM=0.                                                             0382
      DO 155 J=1,JJ                                                         0383
        AMPSUM=AMPSUM+RUSER(2*J+24)                                         0384
        ADUM(1)=0.                                                          0385
        G(NG+1)=RUSER(2*J+23)                                               0386
        RUSER(14)=RUSER(14)+RUSER(2*J+24)*USERK(1,ADUM,NG+1,G)              0387
  155 CONTINUE                                                              0388
      IF (RUSER(14) .LE. 0.) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)             0389
      RUSER(14)=AMPSUM/RUSER(14)                                            0390
  158 DO 160 J=1,JJ                                                         0391
        G(NG+1)=RUSER(2*J+23)                                               0392
        USEREX=USEREX+RUSER(2*J+24)*USERK(IROW,T,NG+1,G)*RUSER(14)          0393
  160 CONTINUE                                                              0394
  800 RETURN                                                                0395
      END                                                                   0396
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    0397
C  SUBROUTINE USERGR.  THIS IS A USER-SUPPLIED ROUTINE (ONLY CALLED         0398
C      WHEN IGRID IS INPUT AS 3) FOR COMPUTING A NONSTANDARD GRID G         0399
C      AND THE QUADRATURE WEIGHTS CQUAD.                                    0400
C  IN THE EXAMPLE BELOW, G IS SIMPLY READ IN.  CQUAD IS                     0401
C      THEN COMPUTED FOR THE TRAPEZOIDAL RULE.                              0402
C  SEE THE USERS MANUAL FOR THE POSITION IN THE INPUT                       0403
C      DATA DECK OF ANY INPUT FOR THIS USER SUBPROGRAM.                     0404
C-----------------------------------------------------------------------    0405
C  CALLS SUBPROGRAMS - CQTRAP                                               0406
C-----------------------------------------------------------------------    0407
      SUBROUTINE USERGR (G,CQUAD,MG)                                        0408
      DOUBLE PRECISION PRECIS, RANGE                                        0409
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  0410
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              0411
      DIMENSION G(MG), CQUAD(MG)                                            0412
      COMMON /DBLOCK/ PRECIS, RANGE                                         0413
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         0414
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     0415
     2 SRANGE                                                               0416
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     0417
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       0418
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             0419
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              0420
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         0421
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            0422
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  0423
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            0424
     2 LUSER(30)                                                            0425
C-----------------------------------------------------------------------    0426
C  YOU MUST REPLACE THE FOLLOWING STATEMENTS WITH THOSE APPROPRIATE         0427
C      FOR YOUR APPLICATION.                                                0428
C-----------------------------------------------------------------------    0429
      READ (NIN,5100) (G(J),J=1,NG)                                         0430
 5100 FORMAT (5E15.6)                                                       0431
C-----------------------------------------------------------------------    0432
C  CQTRAP USES G TO PUT TRAPEZOIDAL RULE WEIGHTS IN CQUAD.                  0433
C-----------------------------------------------------------------------    0434
      CALL CQTRAP (G,CQUAD,NG)                                              0435
      RETURN                                                                0436
      END                                                                   0437
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    0438
C  SUBROUTINE USERIN.  THIS IS A USER-SUPPLIED ROUTINE (ONLY CALLED         0439
C      WHEN DOUSIN=.TRUE.) THAT IS CALLED RIGHT AFTER THE INITIALIZATION    0440
C      AND INPUT OF THE COMMON VARIABLES, T, Y, AND THE LEAST-SQUARES       0441
C      WEIGHTS.  THEREFORE, IT CAN BE USED TO MODIFY ANY OF THESE.          0442
C  SEE THE USERS MANUAL FOR THE POSITION IN THE INPUT                       0443
C      DATA DECK OF ANY INPUT FOR THIS USER SUBPROGRAM.                     0444
C  BELOW IS ILLUSTRATED TWO PREPARATION OF INPUT DATA FOR KERNELS OF        0445
C      THE GENERAL FORM -                                                   0446
C                                                                           0447
C      USERK(T,G)=FORMF2(G)*G**RUSER(23)*EXP(-RUSER(21)*T*G**RUSER(22))     0448
C                                                                           0449
C  WHERE RUSER(21) AND RUSER(22) ARE NOT ZERO.  THIS INCLUDES               0450
C      LAPLACE TRANSFORMS AND MANY APPLICATIONS IN PHOTON CORRELATION       0451
C      SPECTROSCOPY AS SPECIAL CASES.                                       0452
C                                                                           0453
C  THE FOLLOWING ARE USED INTERNALLY BY USERIN AND USERK,                   0454
C      AND CANNOT BE USED BY YOU FOR ANY OTHER PURPOSES -                   0455
C      IUSER(10), IUSER(18), LUSER(3), LUSER(4),                            0456
C      RUSER(J), J = 10,15,...,50+NG (AND THEREFORE NG                      0457
C      CANNOT EXCEED 50).                                                   0458
C                                                                           0459
C  THE FOLLOWING ARE THE NECESSARY INPUT -                                  0460
C                                                                           0461
C  DOUSIN = T (DOUSIN MUST ALWAYS BE .TRUE..)                               0462
C                                                                           0463
C  LUSER(3) = T, TO HAVE FORMF2, THE SQUARED FORM FACTORS, COMPUTED IN      0464
C                USERK.                                                     0465
C           = F, TO SET ALL THE FORMF2 TO 1. (AS WOULD BE APPROPRIATE       0466
C                WITH LAPLACE TRANSFORMS).                                  0467
C  RUSER(24) MAY BE NECESSARY INPUT TO SPECIFY THE FORM FACTOR (E.G.,       0468
C            THE WALL THICKNESS OF A HOLLOW SPHERE) IF LUSER(3)=T.  SEE     0469
C            COMMENTS IN USERK.                                             0470
C  IUSER(18) MAY BE NECESSARY INPUT IF LUSER(3)=T (E.G., TO SPECIFY THE     0471
C            NUMBER OF POINTS OVER WHICH THE SQUARED FORM FACTORS WILL      0472
C            BE AVERAGED). SEE COMMENTS IN USERK.                           0473
C                                                                           0474
C  RUSER(16) = WAVELENGTH OF INCIDENT LIGHT (IN NANOMETERS),                0475
C            = 0, IF RUSER(20), THE MAGNITUDE OF THE SCATTERING VECTOR      0476
C                 (IN CM**-1), IS NOT TO BE COMPUTED.  WHEN                 0477
C                 RUSER(16)=0, RUSER(15) AND RUSER(17) NEED NOT BE          0478
C                 INPUT, AND CONTIN WILL SET RUSER(21)=1                    0479
C                 (AS APPROPRIATE WITH LAPLACE TRANSFORMS).                 0480
C                                                                           0481
C  RUSER(15) = REFRACTIVE INDEX.                                            0482
C  RUSER(17) = SCATTERING ANGLE (IN DEGREES).                               0483
C                                                                           0484
C                                                                           0485
C  IUSER(10) SELECTS SPECIAL CASES OF USERK FOR MORE CONVENIENT USE.        0486
C                                                                           0487
C  IUSER(10) = 1, FOR MOLECULAR WEIGHT DISTRIBUTIONS FROM PCS               0488
C                 (WHERE THE SOLUTION, S(G), IS SUCH THAT S(G)DG IS         0489
C                 THE WEIGHT FRACTION WITH MOLECULAR WEIGHT BETWEEN         0490
C                 G AND G+DG).                                              0491
C                 CONTIN SETS -                                             0492
C                   RUSER(23) = 1.,                                         0493
C                   RUSER(21) = RUSER(18)*RUSER(20)**2.                     0494
C                               (SEE ABOVE DISCUSSION OF RUSER(16).)        0495
C                 YOU MUST INPUT -                                          0496
C                   RUSER(18) TO SATISFY THE EQUATION (IN CGS UNITS) -      0497
C                   (DIFFUSION COEFF.)=RUSER(18)*(MOL. WT.)**RUSER(22).     0498
C                   RUSER(22) (MUST ALSO BE INPUT, TYPICALLY ABOUT -.5).    0499
C                                                                           0500
C  IUSER(10) = 2, FOR DIFFUSION-COEFFICIENT DISTRIBUTONS OR LAPLACE         0501
C                 TRANSFORMS (WHERE G IS DIFF. COEFF. IN CM**2/SEC          0502
C                 OR, E.G., TIME CONSTANT).                                 0503
C                 CONTIN SETS -                                             0504
C                   RUSER(23) = 0.,                                         0505
C                   RUSER(22) = 1.,                                         0506
C                   RUSER(21) = RUSER(20)**2 (SEE ABOVE DISCUSSION          0507
C                                             OF RUSER(16).).               0508
C                                                                           0509
C  IUSER(10) = 3, FOR SPHERICAL-RADIUS DISTRIBUTIONS, ASSUMING THE          0510
C                 EINSTEIN-STOKES RELATION (WHERE THE SOLUTION, S(G),       0511
C                 IS SUCH THAT S(G)DG IS THE WEIGHT FRACTION OF             0512
C                 PARTICLES WITH RADIUS (IN CM) BETWEEN G AND G+DG.         0513
C                 WEIGHT-FRACTION DISTRIBUTIONS YIELD BETTER SCALED         0514
C                 PROBLEMS THAN NUMBER-FRACTION DISTRIBUTIONS, WHICH        0515
C                 WOULD REQUIRE RUSER(23)=6.)                               0516
C                 CONTIN SETS -                                             0517
C                   RUSER(23) = 3.,                                         0518
C                   RUSER(22) = -1.,                                        0519
C                   RUSER(21) = RUSER(20)**2*(BOLTZMANN CONST.)*            0520
C                               RUSER(18)/(.06*PI*RUSER(19)).               0521
C                               (SEE ABOVE DISCUSSION OF RUSER(16).)        0522
C                 YOU MUST HAVE INPUT -                                     0523
C                   RUSER(18) = TEMPERATURE (IN DEGREES KELVIN),            0524
C                   RUSER(19) = VISCOSITY (IN CENTIPOISE).                  0525
C                                                                           0526
C  IUSER(10) = 4, FOR GENERAL CASE, WHERE YOU MUST HAVE INPUT -             0527
C                 RUSER(J), J = 21, 22, 23.                                 0528
C                                                                           0529
C                                                                           0530
C  RUSER(J) FOR J = 18, 19, 21, 22, 23 -  SEE THE ABOVE INSTRUCTIONS        0531
C                                         FOR THE VALUE OF IUSER(10)        0532
C                                         THAT YOU ARE USING.               0533
C                                                                           0534
C                                                                           0535
C  RUSER(10) SPECIFIES HOW THE INPUT Y VALUES WILL BE CONVERTED (E.G.,      0536
C            TO THE NORMALIZED FIRST-ORDER CORRELATION FUNCTION).           0537
C  RUSER(10) = 0., FOR NO CONVERSION (I.E., IF THE INPUT Y VALUES           0538
C                  ARE ALREADY IN THE REQUIRED FORM, AS WITH                0539
C                  LAPLACE TRANSFORMS).                                     0540
C  RUSER(10) NEGATIVE, WHEN THE INPUT Y ARE NORMALIZED 2ND-ORDER            0541
C                      CORRELATION FUNCTIONS MINUS 1 (AS WITH TEST          0542
C                      DATA SET 1) THIS CAUSES -                            0543
C                      Y=SIGN(SQRT(ABS(Y)),Y).                              0544
C                      (SEE THE USERS MANUAL FOR AN                         0545
C                      EXPLANATION OF WHY SIGN() IS USED.)                  0546
C  RUSER(10) POSITIVE, WHEN THE INPUT Y ARE 2ND-ORDER CORRELATION           0547
C                      FUNCTIONS.  RUSER(10) = THE BACKGROUND TERM          0548
C                      (E.G., RUSER(10)=1 WHEN THE INPUT Y ARE              0549
C                      NORMALIZED 2ND-ORDER CORREL. FCTNS.).  THIS          0550
C                      CAUSES -                                             0551
C                        Y=SIGN(SQRT(ABS(X)),X), WHERE                      0552
C                        X=Y/RUSER(10)-1.                                   0553
C                                                                           0554
C-----------------------------------------------------------------------    0555
C  CALLS SUBPROGRAMS - ERRMES                                               0556
C-----------------------------------------------------------------------    0557
      SUBROUTINE USERIN (T,Y,SQRTW,MY)                                      0558
      DOUBLE PRECISION PRECIS, RANGE                                        0559
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  0560
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              0561
      DIMENSION T(MY), Y(MY), SQRTW(MY)                                     0562
      DIMENSION IHOLER(6)                                                   0563
      COMMON /DBLOCK/ PRECIS, RANGE                                         0564
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         0565
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     0566
     2 SRANGE                                                               0567
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     0568
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       0569
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             0570
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              0571
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         0572
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            0573
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  0574
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            0575
     2 LUSER(30)                                                            0576
      DATA IHOLER /1HU, 1HS, 1HE, 1HR, 1HI, 1HN/                            0577
C-----------------------------------------------------------------------    0578
C  YOU MUST REPLACE THE FOLLOWING STATEMENTS WITH THOSE APPROPRIATE         0579
C      FOR YOUR APPLICATION.                                                0580
C-----------------------------------------------------------------------    0581
C  INITIALIZE FLAG FOR CALCULATION OF FORM FACTORS IN USERK.                0582
C-----------------------------------------------------------------------    0583
      LUSER(4)=.FALSE.                                                      0584
      IF (IUSER(10).LT.1 .OR. IUSER(10).GT.4) CALL ERRMES (1,.TRUE.,        0585
     1 IHOLER,NOUT)                                                         0586
      IF (ABS(RUSER(10)).LE.0. .OR. SIMULA) GO TO 120                       0587
C-----------------------------------------------------------------------    0588
C  TRANSFORM INPUT Y VALUES TO NORMALIZED FIRST-ORDER CORRELATION FCTNS.    0589
C-----------------------------------------------------------------------    0590
      DO 110 J=1,NY                                                         0591
        DUM=Y(J)                                                            0592
        IF (RUSER(10) .GT. 0.) DUM=DUM/RUSER(10)-1.                         0593
        Y(J)=SIGN(SQRT(ABS(DUM)),DUM)                                       0594
  110 CONTINUE                                                              0595
  120 IF (IUSER(10) .EQ. 2) RUSER(22)=1.                                    0596
      IF (IUSER(10) .EQ. 3) RUSER(22)=-1.                                   0597
C-----------------------------------------------------------------------    0598
C  COMPUTE THE CONSTANTS RUSER(J), J=20,21 FOR USE IN USERK.                0599
C-----------------------------------------------------------------------    0600
      IF (IUSER(10) .NE. 4) RUSER(21)=1.                                    0601
      IF (RUSER(16) .LE. 0.) GO TO 800                                      0602
C-----------------------------------------------------------------------    0603
C  1.256...E8 = 4.E7*PI     8.726...E-3 = .5 RADIAN/DEGREE                  0604
C-----------------------------------------------------------------------    0605
      RUSER(20)=1.256637E8*RUSER(15)*SIN(8.726646E-3*RUSER(17))/            0606
     1 RUSER(16)                                                            0607
      IF (IUSER(10) .EQ. 4) GO TO 800                                       0608
      RUSER(21)=RUSER(20)**2                                                0609
      IF (IUSER(10) .EQ. 1) RUSER(21)=RUSER(21)*RUSER(18)                   0610
      IF (IUSER(10) .NE. 3) GO TO 800                                       0611
      IF (RUSER(19) .LE. 0.) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)             0612
C-----------------------------------------------------------------------    0613
C  7.323...E-16 = (BOLTZMANN CONSTANT)/(.06*PI)                             0614
C-----------------------------------------------------------------------    0615
      RUSER(21)=RUSER(21)*7.323642E-16*RUSER(18)/RUSER(19)                  0616
  800 RETURN                                                                0617
      END                                                                   0618
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    0619
C  FUNCTION USERK.  THIS IS A USER-SUPPLIED ROUTINE (ALWAYS NEEDED)         0620
C      TO COMPUTE THE FREDHOLM KERNEL, USERK, WHICH DEPENDS ON T(JT)        0621
C      (THE INDEPENDENT VARIABLE AT DATA POINT JT) AND G(JG) (THE           0622
C      VALUE OF THE JG TH GRID POINT.                                       0623
C  BELOW IS ILLUSTRATED THE CASE OF A GENERAL TYPE OF KERNEL                0624
C      APPLICABLE TO PHOTON CORRELATION SPECTROSCOPY AND LAPLACE            0625
C      TRANSFORMS -                                                         0626
C                                                                           0627
C      USERK(T,G)=FORMF2(G)*G**RUSER(23)*EXP(-RUSER(21)*T*G**RUSER(22))     0628
C                                                                           0629
C      SEE COMMENTS IN USERIN.                                              0630
C  THE MEAN SQUARED FORM FACTOR, FORMF2(G), COMPUTED BELOW IS FOR THE       0631
C      RAYLEIGH-DEBYE APPROXIMATION FOR HOLLOW SPHERES.                     0632
C      SEE COMMENTS BELOW                                                   0633
C      ON HOW YOU CAN MODIFY THIS FOR ANOTHER FORM FACTOR.                  0634
C  RUSER(24) = THICKNESS OF THE WALLS OF THE HOLLOW SPHERES (IN CM).        0635
C            = 0 FOR FULL SPHERE (I.E., FOR WALL THICKNESS = RADIUS OF      0636
C                SPHERE.                                                    0637
C  IUSER(18) DETERMINES THE NUMBER OF POINTS OVER WHICH THE FORM            0638
C      FACTOR WILL BE AVERAGED, AS EXPLAINED IN THE COMMENTS BELOW.         0639
C-----------------------------------------------------------------------    0640
C  CALLS SUBPROGRAMS - ERRMES, USERTR                                       0641
C-----------------------------------------------------------------------    0642
      FUNCTION USERK (JT,T,JG,G)                                            0643
      DOUBLE PRECISION PRECIS, RANGE                                        0644
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  0645
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              0646
      DIMENSION T(JT), G(*)                                                 0647
      DIMENSION IHOLER(6)                                                   0648
      COMMON /DBLOCK/ PRECIS, RANGE                                         0649
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         0650
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     0651
     2 SRANGE                                                               0652
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     0653
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       0654
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             0655
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              0656
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         0657
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            0658
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  0659
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            0660
     2 LUSER(30)                                                            0661
      DATA IHOLER/1HU, 1HS, 1HE, 1HR, 1HK, 1H /                             0662
      IF (JT.GT.NY .OR. JG.GT.NG+1 .OR. MIN0(JT,JG).LE.0) CALL              0663
     1 ERRMES (1,.TRUE.,IHOLER,NOUT)                                        0664
C-----------------------------------------------------------------------    0665
C  THE FOLLOWING STATEMENTS SHOULD BE REPLACED BY THOSE                     0666
C      APPROPRIATE FOR YOUR KERNEL.                                         0667
C  FOR EXAMPLE, FOR THE LAPLACE INTEGRAL EQUATION, YOU WOULD                0668
C      SIMPLY REPLACE ALL BUT THE LAST TWO STATEMENTS BELOW BY -            0669
C     USERK=EXP(-T(JT)*G(JG))                                               0670
C  IT MAY NOT BE NECESSARY TO GUARD AGAINST UNDERFLOW IN EXP AS IS          0671
C      DONE BELOW, BUT A FEW COMPILERS ABORT AT UNDERFLOW IN EXP.           0672
C      (EXMAX IS SET IN INIT TO ALOG(SRANGE), I.E., IT IS THE               0673
C      LARGEST REASONABLE EXPONENT IN EXP.)                                 0674
C-----------------------------------------------------------------------    0675
      IF (LUSER(4)) GO TO 150                                               0676
      LUSER(4)=.TRUE.                                                       0677
      IF (.NOT.LUSER(3)) GO TO 150
C-----------------------------------------------------------------------    0678
C  PUT MEAN SQUARED FORM FACTORS IN RUSER(J), J=51,...,50+NG.               0679
C  IF THE FORM FACTORS OSCILLATE SO RAPIDLY THAT THERE ARE OSCILLATIONS     0680
C      WITH PERIOD LESS THAN 3 OR 4 GRID POINTS, THEN YOU SHOULD USE THE    0681
C      MEAN SQUARED VALUE OVER AN INTERVAL CENTERED AT EACH GRID POINT      0682
C      AND EXTENDING HALFWAY TOWARD EACH OF ITS NEIGHBORS.                  0683
C  IUSER(18) = THE NUMBER OF POINTS ON EACH SIDE OF THE GRID POINT          0684
C      OVER WHICH THE AVERAGE WILL BE TAKEN.  I.E., THERE WILL BE A         0685
C      TOTAL OF 2*IUSER(18)+1 POINTS IN THE AVERAGE.                        0686
C  IUSER(18) = 0 FOR NO AVERAGING.  I.E., THE FORM FACTOR AT THE GRID       0687
C      POINT WILL BE USED.                                                  0688
C  IUSER(18) = 50 IS USUALLY ADEQUATE.                                      0689
C  NOTE THAT, IF IGRID=2 AS USUAL, THEN THE POINTS (AS WITH THE GRID)       0690
C      WILL BE TAKEN IN EQUAL INTERVALS OF H(G) IN USERTR (E.G.,            0691
C      USUALLY IN EQUAL INTERVALS OF LOG(G)).                               0692
C  LUSER(3) = T WILL USE THE RAYLEIGH-DEBYE APPROXIMATION FOR               0693
C               HOLLOW SPHERES FILLED WITH SOLVENT,                         0694
C           = F WILL SET ALL FORM FACTORS TO 1.  THIS ALSO HAPPENS IF       0695
C               RUSER(16).LE.0 (SINCE NO SCATTERING VECTOR WAS PUT IN       0696
C               RUSER(20)).                                                 0697
C  RUSER(24) = THICKNESS OF THE WALLS OF THE HOLLOW SPHERES (IN CM).        0698
C            = 0 FOR FULL SPHERE (I.E., FOR WALL THICKNESS = RADIUS OF      0699
C                SPHERE.                                                    0700
C  YOU CAN CHANGE THE COMPUTATION OF RUSER BELOW TO COMPUTE THE MEAN        0701
C      SQUARED FORM FACTORS THAT ARE APPROPRIATE FOR YOUR APPLICATION.      0702
C  YOU CAN ALSO READ IN THE MEAN SQUARED FORM FACTORS WITH -                0703
C      READ (NIN,...) (RUSER(J+50),J=1,NG)                                  0704
C      THIS INPUT DATA WOULD HAVE TO BE BETWEEN CARD SETS 13 AND 14A        0705
C      (SEE THE USERS MANUAL.)                                              0706
C-----------------------------------------------------------------------    0707
      IF (NG .GT. 500) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)
      DO 120 J=1,NG                                                         0709
        IF (LUSER(3) .AND. RUSER(16).GT.0.) GO TO 121                       0710
        RUSER(J+50)=1.                                                      0711
        GO TO 120                                                           0712
C-----------------------------------------------------------------------    0713
C  COMPUTE AVERAGE FORM FACTOR.                                             0714
C-----------------------------------------------------------------------    0715
  121   DELTA=0.                                                            0716
        TRSTRT=USERTR(G(J),1)                                               0717
        TREND=TRSTRT                                                        0718
        NPTS=IUSER(18)+1                                                    0719
        IF (J.NE.1 .AND. J.NE.NG) NPTS=NPTS+IUSER(18)                       0720
        IF (NPTS .LE. 1) GO TO 122                                          0721
        IF (J .NE. 1) TRSTRT=.5*(USERTR(G(J-1),1)+USERTR(G(J),1))           0722
        IF (J .NE. NG) TREND=.5*(USERTR(G(J+1),1)+USERTR(G(J),1))           0723
        DELTA=(TREND-TRSTRT)/FLOAT(NPTS-1)                                  0724
  122   SUM=0.                                                              0725
        TR=TRSTRT-DELTA                                                     0726
        DO 125 K=1,NPTS                                                     0727
          TR=TR+DELTA                                                       0728
          GPOINT=USERTR(TR,2)                                               0729
C-----------------------------------------------------------------------    0730
C  YOU MUST REPLACE THE STATEMENTS BETWEEN HERE AND NO. 135 WITH THOSE      0731
C      APPROPRIATE FOR YOUR FORM FACTOR IF IT IS NOT THE RAYLEIGH-DEBYE     0732
C      APPROXIMATION FOR A HOLLOW SPHERE.                                   0733
C  PINNER,POUTER,PAVG = (MAGNITUDE OF SCATTERING VECTOR)*(INNER,OUTER,      0734
C                                                       AVERAGE RADIUS)     0735
C  RUSER(20) = MAGNITUDE OF SCATTERING VECTOR IN CM**(-1),                  0736
C  GPOINT = OUTER RADIUS IN CM.                                             0737
C  TERM = FORM FACTOR FOR G=GPOINT.                                         0738
C-----------------------------------------------------------------------    0739
          PINNER=0.                                                         0740
          IF (RUSER(24).GT.0. .AND. RUSER(24).LT.GPOINT) PINNER=            0741
     1    RUSER(20)*(GPOINT-RUSER(24))                                      0742
          POUTER=RUSER(20)*GPOINT                                           0743
          PAVG=.5*(POUTER+PINNER)                                           0744
          PDELTA=PAVG-PINNER                                                0745
          PD2=PDELTA**2                                                     0746
          COSPAV=COS(PAVG)                                                  0747
          IF (PDELTA .LE. .2) TERM=COSPAV*PD2*(1.-PD2*(28.-PD2)/280.)       0748
     1    +PAVG*SIN(PAVG)*(3.-PD2*(.5-.025*PD2))                            0749
          IF (PDELTA .GT. .2) TERM=3.*(SIN(PDELTA)*(PAVG*SIN(PAVG)+         0750
     1    COSPAV)/PDELTA-COSPAV*COS(PDELTA))                                0751
          TERM=TERM/(POUTER*(POUTER+PINNER)+PINNER**2)                      0752
  135     SUM=SUM+TERM**2                                                   0753
  125   CONTINUE                                                            0754
        RUSER(J+50)=SUM/FLOAT(NPTS)                                         0755
  120 CONTINUE                                                              0756
      K=NG+50                                                               0757
 5120 FORMAT (/21H SQUARED FORM FACTORS/(1P10E13.3))                        0758
      IF (LUSER(3) .AND. RUSER(16).GT.0.) WRITE (NOUT,5120)                 0759
     1 (RUSER(J),J=51,K)                                                    0760
C-----------------------------------------------------------------------    0761
C  END OF CALCULATION OF MEAN SQUARED FORM FACTORS.                         0762
C-----------------------------------------------------------------------    0763
C  PUT MEAN SQUARED FORM FACTOR IN FORMF2.  NORMALLY THIS IS JUST           0764
C      RUSER(JG+50).  HOWEVER, WITH CALLS FROM USEREX, G(NG+1) CAN HAVE     0765
C      ANY VALUE.  IN THIS CASE, THE FORM FACTOR IS ESTIMATED BY LINEAR     0766
C      INTERPOLATION USING THE TWO GRID POINTS NEAREST G(NG+1).             0767
C      ADVANTAGE IS TAKEN OF THE FACT THAT G(J) IS STRICTLY MONOTONIC       0768
C      FOR J=1,...,NG.                                                      0769
C-----------------------------------------------------------------------    0770
  150 FORMF2=1.                                                             0771
      IF (.NOT.LUSER(3) .OR. RUSER(16).LE.0.) GO TO 158                     0772
      IF (JG .EQ. NG+1) GO TO 152                                           0773
      FORMF2=RUSER(JG+50)                                                   0774
      GO TO 158                                                             0775
  152 IF ((G(NG+1)-G(1))*(G(NG+1)-G(NG)) .GT. 0.) CALL ERRMES (3,           0776
     1 .TRUE.,IHOLER,NOUT)                                                  0777
      SGN=SIGN(1.,G(2)-G(1))                                                0778
      DO 155 J=2,NG                                                         0779
        JJ=J                                                                0780
        IF ((G(JJ)-G(NG+1))*SGN .GE. 0.) GO TO 157                          0781
  155 CONTINUE                                                              0782
  157 FORMF2=RUSER(JJ+49)+(RUSER(JJ+50)-RUSER(JJ+49))*(G(NG+1)-G(JJ-1))/    0783
     1 (G(JJ)-G(JJ-1))                                                      0784
  158 IF (AMIN1(ABS(RUSER(21)),ABS(RUSER(22))) .LE. 0.) CALL ERRMES (4,     0785
     1 .TRUE.,IHOLER,NOUT)                                                  0786
      USERK=0.                                                              0787
      IF (IUSER(10) .NE. 1) GO TO 200                                       0788
C-----------------------------------------------------------------------    0789
C  FOR MOLECULAR WEIGHT DISTRIBUTIONS.                                      0790
C-----------------------------------------------------------------------    0791
      IF ((RUSER(22).LE.0. .AND. G(JG).LE.0.) .OR. G(JG).LT.0.)             0792
     1 CALL ERRMES (5,.TRUE.,IHOLER,NOUT)                                   0793
      IF (ABS(.5+RUSER(22)) .GT. 1.E-5) GO TO 160                           0794
      EX=RUSER(21)*T(JT)/SQRT(G(JG))                                        0795
      GO TO 170                                                             0796
  160 EX=RUSER(21)*T(JT)*G(JG)**RUSER(22)                                   0797
  170 PREEXP=FORMF2*G(JG)                                                   0798
      GO TO 500                                                             0799
  200 IF (IUSER(10) .NE. 2) GO TO 300                                       0800
C-----------------------------------------------------------------------    0801
C  FOR DIFFUSION-COEFFICIENT DISTRIBUTIONS OR LAPLACE TRANSFORMS.           0802
C-----------------------------------------------------------------------    0803
      EX=RUSER(21)*G(JG)*T(JT)                                              0804
      PREEXP=FORMF2                                                         0805
      GO TO 500                                                             0806
  300 IF (IUSER(10) .NE. 3) GO TO 400                                       0807
C-----------------------------------------------------------------------    0808
C  FOR SPHERICAL-RADIUS DISTRIBUTIONS.                                      0809
C-----------------------------------------------------------------------    0810
      IF (G(JG) .LE. 0.) CALL ERRMES (6,.TRUE.,IHOLER,NOUT)                 0811
      EX=RUSER(21)*T(JT)/G(JG)                                              0812
      PREEXP=FORMF2*G(JG)**3                                                0813
      GO TO 500                                                             0814
C-----------------------------------------------------------------------    0815
C  GENERAL FORM OF KERNEL - WHEN IUSER(10)=4.                               0816
C-----------------------------------------------------------------------    0817
  400 EX=0.                                                                 0818
      IF (G(JG) .LE. 0.) GO TO 410                                          0819
C-----------------------------------------------------------------------    0820
C  G(JG) IS POSITIVE.                                                       0821
C-----------------------------------------------------------------------    0822
      EX=RUSER(21)*T(JT)*G(JG)**RUSER(22)                                   0823
      PREEXP=FORMF2*G(JG)**RUSER(23)                                        0824
      GO TO 500                                                             0825
  410 IF (ABS(G(JG)) .LE. 0.) GO TO 420                                     0826
C-----------------------------------------------------------------------    0827
C  G(JG) IS NEGATIVE.                                                       0828
C-----------------------------------------------------------------------    0829
      J=INT(RUSER(22)+SIGN(.5,RUSER(22)))                                   0830
      JJ=INT(RUSER(23)+SIGN(.5,RUSER(23)))                                  0831
      IF (AMAX1(ABS(RUSER(22)-FLOAT(J)),ABS(RUSER(23)-FLOAT(JJ))) .GT.      0832
     1 1.E-5) CALL ERRMES (7,.TRUE.,IHOLER,NOUT)                            0833
      EX=RUSER(21)*T(JT)*G(JG)**J                                           0834
      PREEXP=FORMF2*G(JG)**JJ                                               0835
      GO TO 500                                                             0836
C-----------------------------------------------------------------------    0837
C  G(JG)=0.                                                                 0838
C-----------------------------------------------------------------------    0839
  420 IF (RUSER(22).LT.0. .OR. RUSER(23).GT.0.) RETURN                      0840
      IF (RUSER(23) .LT. 0.) CALL ERRMES (8,.TRUE.,IHOLER,NOUT)             0841
      PREEXP=FORMF2                                                         0842
  500 IF (EX .LT. EXMAX) USERK=PREEXP*EXP(-EX)                              0843
      RETURN                                                                0844
      END                                                                   0845
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    0846
C  FUNCTION USERLF.  THIS IS A USER-SUPPLIED ROUTINE (NEEDED IF             0847
C      NLINF IS INPUT POSITIVE) TO CALCULATE THE NLINF ADDITIONAL           0848
C      KNOWN FUNCTIONS THAT ARE PRESENT IN UNKNOWN AMOUNTS                  0849
C      IN THE DATA.                                                         0850
C  JY = THE SUBSCRIPT OF THE DATA POINT.                                    0851
C  JLINF = INDEX TELLING WHICH OF THE NLINF FUNCTIONS IS TO BE              0852
C      EVALUATED.  (1 .LE. JLINF .LE. NLINF)                                0853
C  NYDIM = THE DIMENSION OF THE ARRAY T.  IT CAN BE MY OR NY.               0854
C  BELOW IS ILLUSTRATED THE CASE WHERE 2 SETS OF DATA CAN BE                0855
C      COMBINED AND EACH CAN HAVE A DIFFERENT CONSTANT ADDITIVE             0856
C      BACKGROUND (BASELINE).  THE FIRST IUSER(2) LOCATIONS IN              0857
C      Y, T, ETC. ARE OCCUPIED BY THE FIRST DATA SET.  THE SECOND SET       0858
C      OCCUPIES LOCATIONS IUSER(2)+1 THRU NY.  THEREFORE JLINF=1 OR 2,      0859
C      AND USERLF=1. OR 0. DEPENDING ON WHETHER THE (JY)TH DATA POINT       0860
C      BELONGS TO DATA SET JLINF OR NOT.                                    0861
C      IF, HOWEVER, IUSER(2)=0 AND NLINF=1, THEN IT IS ASSUMED THAT         0862
C      THERE IS ONLY ONE SET OF DATA AND ONLY ONE BASELINE.  (THIS          0863
C      SAVES YOU THE TROUBLE OF SETTING IUSER(2)=NY EVERY TIME NY           0864
C      CHANGES.)                                                            0865
C      IF NLINF=0, THEN THERE WILL BE NO BASELINE.                          0866
C-----------------------------------------------------------------------    0867
C  CALLS SUBPROGRAMS - ERRMES                                               0868
C-----------------------------------------------------------------------    0869
      FUNCTION USERLF (JY,JLINF,T,NYDIM)                                    0870
      DOUBLE PRECISION PRECIS, RANGE                                        0871
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  0872
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              0873
      DIMENSION T(NYDIM), IHOLER(6)                                         0874
      COMMON /DBLOCK/ PRECIS, RANGE                                         0875
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         0876
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     0877
     2 SRANGE                                                               0878
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     0879
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       0880
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             0881
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              0882
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         0883
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            0884
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  0885
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            0886
     2 LUSER(30)                                                            0887
      DATA IHOLER/1HU, 1HS, 1HE, 1HR, 1HL, 1HF/                             0888
      IF (JY.GT.NY .OR. JY.LE.0) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)         0889
C-----------------------------------------------------------------------    0890
C  THE FOLLOWING STATEMENTS MUST BE REPLACED BY THE APPROPRIATE             0891
C      ONES FOR YOUR ADDITIVE FUNCTIONS.                                    0892
C-----------------------------------------------------------------------    0893
      IF (JLINF.LT.1 .OR. JLINF.GT.2) CALL ERRMES (2,.TRUE.,IHOLER,         0894
     1 NOUT)                                                                0895
      USERLF=0.                                                             0896
      IF ((JY.LE.IUSER(2) .AND. JLINF.EQ.1) .OR. (JY.GT.IUSER(2) .AND.      0897
     1 JLINF.EQ.2) .OR. IUSER(2).LE.0) USERLF=1.                            0898
      RETURN                                                                0899
      END                                                                   0900
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    0901
C  SUBROUTINE USERNQ.  THIS IS A USER-SUPPLIED ROUTINE (ONLY                0902
C      CALLED WHEN DOUSNQ IS INPUT AS .TRUE.) TO SET NINEQ                  0903
C      (THE NO. OF INEQUALITY CONSTRAINTS) AND TO PUT THE                   0904
C      INEQUALITY-CONSTRAINT MATRIX IN THE FIRST NINEQ                      0905
C      ROWS OF AINEQ AND TO PUT THE RIGHT HAND SIDES OF THE                 0906
C      INEQUALITIES IN COLUMN NGLP1 OF AINEQ.                               0907
C  I.E., (SUM FROM J=1 TO NGL OF (AINEQ(I,J)*SOLUTION(J))) .GE.             0908
C      AINEQ(I,NGLP1), I=1,NINEQ.  SEE EQ. (3.6).                           0909
C  NOTE - IF THE SOLUTION IS TO BE NONNEGATIVE AT ALL NG GRID               0910
C      POINTS, DO NOT USE USERNQ TO SET THESE CONSTRAINTS -                 0911
C      SIMPLY INPUT NONNEG=.TRUE. INSTEAD.                                  0912
C  SEE THE USERS MANUAL FOR THE POSITION IN THE INPUT                       0913
C      DATA DECK OF ANY INPUT FOR THIS USER SUBPROGRAM.                     0914
C  BELOW IS ILLUSTRATED THE CASE WHERE THE NLINF COEFFICIENTS OF            0915
C      THE NLINF LINEAR FUNCTIONS ARE CONSTRAINED TO BE                     0916
C      NONNEGATIVE.                                                         0917
C-----------------------------------------------------------------------    0918
C  CALLS SUBPROGRAMS - ERRMES                                               0919
C-----------------------------------------------------------------------    0920
      SUBROUTINE USERNQ (AINEQ,MG,MINEQ)                                    0921
      DOUBLE PRECISION PRECIS, RANGE                                        0922
      DOUBLE PRECISION AINEQ, ONE, ZERO                                     0923
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  0924
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              0925
      DIMENSION AINEQ(MINEQ,MG)                                             0926
      DIMENSION IHOLER(6)                                                   0927
      COMMON /DBLOCK/ PRECIS, RANGE                                         0928
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         0929
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     0930
     2 SRANGE                                                               0931
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     0932
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       0933
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             0934
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              0935
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         0936
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            0937
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  0938
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            0939
     2 LUSER(30)                                                            0940
      DATA IHOLER/1HU, 1HS, 1HE, 1HR, 1HN, 1HQ/                             0941
C     ZERO=0.E0!SP                                                          0942
      ZERO=0.D0                                                             0943
C     ONE=1.E0!SP                                                           0944
      ONE=1.D0                                                              0945
C-----------------------------------------------------------------------    0946
C  YOU MUST REPLACE THE FOLLOWING STATEMENTS WITH THOSE APPROPRIATE         0947
C      FOR YOUR APPLICATION.                                                0948
C-----------------------------------------------------------------------    0949
      IF (NLINF .LE. 0) RETURN                                              0950
      NINEQ=NLINF                                                           0951
      IF (NINEQ .GT. MINEQ) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)              0952
      DO 110 J=1,NINEQ                                                      0953
        DO 120 K=1,NGLP1                                                    0954
          AINEQ(J,K)=ZERO                                                   0955
  120   CONTINUE                                                            0956
        K=NG+J                                                              0957
        AINEQ(J,K)=ONE                                                      0958
  110 CONTINUE                                                              0959
      RETURN                                                                0960
      END                                                                   0961
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    0962
C  SUBROUTINE USEROU.  THIS IS A USER-SUPPLIED ROUTINE (ONLY CALLED         0963
C      WHEN DOUSOU=.TRUE.) THAT YOU CAN USE TO PRODUCE YOUR OWN EXTRA       0964
C      OUTPUT.                                                              0965
C  G CONTAINS THE NG GRID POINTS.                                           0966
C  SOL CONTAINS NG+NLINF VALUES - FIRST THE NG SOLUTION VALUES AND THEN     0967
C      THE NLINF LINEAR COEFFICIENTS.                                       0968
C  EXACT CONTAINS THE NG VALUES OF THE SECOND CURVE TO BE PLOTTED THAT      0969
C      HAS BEEN COMPUTED IN USERSX (ONLY IF ONLY1=.FALSE.).                 0970
C  AA IS ( SQUARE ROOT OF THE COVARIANCE MATRIX OF THE SOLUTION)/STDDEV.    0971
C      YOU CAN USE AA AND THE LAST 4 ARGUMENTS TO COMPUTE ERROR             0972
C      ESTIMATES, AS SHOWN BELOW.                                           0973
C  (CONTIN SETS DOERR=.FALSE. IF IT WAS NOT ABLE TO COMPUTE AA OR           0974
C      STDDEV.  THE ERROR ESTIMATES THEN CANNOT BE COMPUTED AND ARE SET     0975
C      TO ZERO.)                                                            0976
C  BELOW IS ILLUSTRATED THE CASE FROM THE ANALYSIS OF CIRCULAR DICHROIC     0977
C      SPECTRA, WHERE THE NSPECT-BY-1 VECTOR SOL (THE FRACTIONS OF EACH     0978
C      REFERENCE PROTEIN SPECTRUM IN THE SPECTRUM BEING ANALYZED) IS        0979
C      RIGHT-MULTIPLIED BY THE NCLASS-BY-NSPECT MATRIX FCAP (THE X-RAY      0980
C      VALUES OF THE FRACTIONS OF THE REFERENCE PROTEINS IN EACH CLASS)     0981
C      TO YIELD THE NCLASS-BY-1 VECTOR F (THE FRACTION OF THE PROTEIN       0982
C      BEING ANALYZED IN EACH CLASS).  F AND THE ERROR ESTIMATES ARE        0983
C      THEN NORMALIZED BY DIVIDING EACH ELEMENT BY THE SUM OF THE F'S       0984
C      (WHICH IS ALSO PRINTED AS THE SCALE FACTOR, SUMF).                   0985
C  NSPECT = THE NUMBER OF REFERENCE PROTEIN CD SPECTRA.                     0986
C  NCLASS = THE NUMBER OF CONFORMATIONAL CLASSES.                           0987
      SUBROUTINE USEROU (CQUAD,G,SOL,EXACT,AA,MG,STDDEV,DOERR,NGLEY)        0988
      DOUBLE PRECISION AA                                                   0989
      DOUBLE PRECISION PRECIS, RANGE                                        0990
      LOGICAL DOCHOS, DOERR, DOMOM, DOUSIN, DOUSNQ, LAST,                   0991
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                      0992
      DIMENSION CQUAD(MG), G(MG), SOL(MG), EXACT(MG), AA(MG,MG)             0993
C-----------------------------------------------------------------------    0994
C  IF YOU CHANGE NSPECT OR NCLASS IN THE DATA STATEMENT BELOW, THEN YOU     0995
C      MUST READJUST THE DIMENSIONS IN THE FOLLOWING STATEMENT TO           0996
C     DIMENSION FCAP(NCLASS,NSPECT), F(NCLASS), ERROR(NCLASS)               0997
C      AND, OF COURSE, MODIFY FCAP (THE X-RAY FRACTIONS) AND NSPECT         0998
C      AND NCLASS IN THE DATA STATEMENT BELOW.                              0999
C-----------------------------------------------------------------------    1000
      DIMENSION FCAP(3,16), F(3), ERROR(3)                                  1001
      COMMON /DBLOCK/ PRECIS, RANGE                                         1002
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         1003
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     1004
     2 SRANGE                                                               1005
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     1006
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       1007
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             1008
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              1009
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         1010
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            1011
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  1012
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            1013
     2 LUSER(30)                                                            1014
      DATA NSPECT/16/, NCLASS/3/, FCAP/                                     1015
     1     .79,.00,.21 ,  .41,.16,.43 ,  .23,.40,.37 ,                      1016
     2     .28,.14,.58 ,  .45,.24,.31 ,  .09,.34,.57 ,                      1017
     3     .02,.51,.47 ,  .39,.00,.61 ,  .07,.52,.41 ,                      1018
     4     .24,.15,.61 ,  .51,.24,.25 ,  .62,.05,.33 ,                      1019
     5     .37,.15,.48 ,                 .28,.33,.39 ,                      1020
     6                    .54,.12,.34 ,  .26,.44,.30 /                      1021
      SUMF=0.                                                               1022
      DO 110 J=1,NCLASS                                                     1023
        F(J)=0.                                                             1024
        DO 120 K=1,NSPECT                                                   1025
          F(J)=F(J)+SOL(K)*FCAP(J,K)                                        1026
  120   CONTINUE                                                            1027
        SUMF=SUMF+F(J)                                                      1028
  110 CONTINUE                                                              1029
      DO 140 J=1,NCLASS                                                     1030
        F(J)=F(J)/SUMF                                                      1031
  140 CONTINUE                                                              1032
      FACTOR=STDDEV/SUMF                                                    1033
C-----------------------------------------------------------------------    1034
C  RUSER(14) IS THE SCALE FACTOR BY WHICH THE DATA SPECTRUM IS              1035
C      DIVIDED (SEE USERS MANUAL).  NORMALLY, FACTOR=STDDEV ABOVE.          1036
      IF (ABS(RUSER(14)) .GT. 0.) SUMF=SUMF/RUSER(14)                       1037
C-----------------------------------------------------------------------    1038
C  ABOVE, THE NCLASS-BY-1 VECTOR F WAS PRODUCED BY MULTIPLYING THE          1039
C      NSPECT-BY-1 SOLUTION SOL BY THE NCLASS-BY-NSPECT MATRIX FCAP.        1040
C      THIS IS A VERY COMMON TYPE OF TRANSFORMATION, THAT OCCURS IN         1041
C      MANY APPPLICATIONS, E.G., WHEN THE SOLUTION IS REPRESENTED BY        1042
C      A SUM OF BASIS (E.G., SPLINE) FUNCTIONS.  IN THIS LATTER CASE,       1043
C      SOL WOULD BE THE EXPANSION COEFFICIENTS AND F WOULD BE THE           1044
C      SOLUTION VALUES AT NCLASS GRID POINTS (SEE USERS MANUAL.)            1045
C  THE COMPUTATION OF THE NCLASS-BY-1 VECTOR ERROR, THE                     1046
C      STANDARD ERROR ESTIMATES FOR F, BELOW IS COMPLETELY GENERAL          1047
C      FOR ANY FCAP.  SO YOU DO NOT HAVE TO CHANGE ANYTHING BELOW IF        1048
C      FCAP, NCLASS, OR NSPECT ARE CHANGED.  (USUALLY NSPECT=NGL, BUT       1049
C      THIS IS NOT NECESSARY.)                                              1050
C-----------------------------------------------------------------------    1051
      DO 210 J=1,NCLASS                                                     1052
        ERROR(J)=0.                                                         1053
        IF (.NOT.DOERR) GO TO 210                                           1054
        DO 220 K=1,NGLEY                                                    1055
          SUM=0.                                                            1056
          DO 230 L=1,NSPECT                                                 1057
            SUM=SUM+FCAP(J,L)*AA(L,K)                                       1058
  230     CONTINUE                                                          1059
          ERROR(J)=ERROR(J)+SUM**2                                          1060
  220   CONTINUE                                                            1061
        ERROR(J)=FACTOR*SQRT(ERROR(J))                                      1062
  210 CONTINUE                                                              1063
C-----------------------------------------------------------------------    1064
C  IF YOU CHANGE THE CLASSES, THEN YOU MUST MODIFY THE FOLLOWING            1065
C      FORMAT AND WRITE STATEMENTS.                                         1066
C-----------------------------------------------------------------------    1067
 5140 FORMAT (/23X,5HHELIX,3X,10HBETA-SHEET,4X,                             1068
     1 9HREMAINDER,16X,12HSCALE FACTOR/                                     1069
     2 9H FRACTION,F19.2,2F13.2,F28.3/                                      1070
     3 15H STANDARD ERROR,1P3E13.1)                                         1071
      WRITE (NOUT,5140) F,SUMF,ERROR                                        1072
      RETURN                                                                1073
      END                                                                   1074
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    1075
C  SUBROUTINE USERRG.  THIS IS A USER-SUPPLIED ROUTINE (NEEDED              1076
C      WHEN NORDER IS INPUT NEGATIVE) TO SET NREG AND TO PUT A              1077
C      SPECIAL USER-DEFINED REGULARIZOR IN THE FIRST NREG COLUMNS           1078
C      AND ROWS OF REG AND THE RIGHT-HAND-SIDE (R.H.S.) OF THE              1079
C      REGULARIZOR IN COLUMN NGLP1 OF REG.                                  1080
C  NOTE - IF IWT = 1 OR 4, THEN USERRG IS CALLED 2 TIMES.                   1081
C         IF IWT = 2, 3, OR 5, THEN USERRG IS CALLED 4 TIMES.               1082
C         THEREFORE, IT IS BEST TO HAVE ANY DATA NEEDED BY USERRG READ      1083
C         IN ONLY ONCE AND STORED (E.G., IN RUSER, AS ILLUSTRATED           1084
C         BELOW).  OTHERWISE, THIS DATA WOULD HAVE TO BE READ 2 OR 4        1085
C         TIMES.                                                            1086
C  SEE THE USERS MANUAL FOR THE POSITION IN THE INPUT                       1087
C      DATA DECK OF ANY INPUT FOR THIS USER SUBPROGRAM.                     1088
C  BELOW IS ILLUSTRATED A REGULARIZOR THAT PENALIZES DEVIATIONS OF THE      1089
C      SOLUTION FROM AN EXPECTED SOLUTION.  THE IDENTITY MATRIX GOES        1090
C      INTO THE REGULARIZOR, AND THE EXPECTED SOLUTION IS READ INTO         1091
C      RUSER(IUSER(1)),...,RUSER(IUSER(1)+NG-1) AND THEN PUT INTO THE       1092
C      R.H.S. OF THE REGULARIZOR (COLUMN NGLP1 OF REG).  (SEE               1093
C      S. TWOMEY, JACM 10, 97 (1963).)                                      1094
C      LUSER(1) IS INPUT INITIALLY AS .FALSE. AND THEN SET TO .TRUE.        1095
C      SO THAT THE DATA IS ONLY READ ONCE.                                  1096
      SUBROUTINE USERRG (REG,MREG,MG,NREG)                                  1097
      DOUBLE PRECISION PRECIS, RANGE                                        1098
      DOUBLE PRECISION REG                                                  1099
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  1100
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              1101
      DIMENSION REG(MREG,MG)                                                1102
      COMMON /DBLOCK/ PRECIS, RANGE                                         1103
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         1104
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     1105
     2 SRANGE                                                               1106
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     1107
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       1108
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             1109
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              1110
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         1111
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            1112
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  1113
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            1114
     2 LUSER(30)                                                            1115
C-----------------------------------------------------------------------    1116
C  YOU MUST REPLACE THE FOLLOWING STATEMENTS WITH THOSE APPROPRIATE         1117
C      FOR YOUR APPLICATION.                                                1118
C-----------------------------------------------------------------------    1119
      NREG=NG                                                               1120
      DO 110 J=1,NREG                                                       1121
        DO 120 K=1,NGL                                                      1122
          REG(J,K)=0.                                                       1123
  120   CONTINUE                                                            1124
        REG(J,J)=1.                                                         1125
  110 CONTINUE                                                              1126
      J=IUSER(1)                                                            1127
      K=J+NG-1                                                              1128
      IF (LUSER(1)) GO TO 200                                               1129
 5200 FORMAT (5E15.6)                                                       1130
      READ (NIN,5200) (RUSER(L),L=J,K)                                      1131
      WRITE (NOUT,5200) (RUSER(L),L=J,K)                                    1132
      LUSER(1)=.TRUE.                                                       1133
  200 IROW=0                                                                1134
      DO 210 L=J,K                                                          1135
        IROW=IROW+1                                                         1136
        REG(IROW,NGLP1)=RUSER(L)                                            1137
  210 CONTINUE                                                              1138
      RETURN                                                                1139
      END                                                                   1140
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    1141
C  SUBROUTINE USERSI.  THIS IS A USER-SUPPLIED ROUTINE (ONLY                1142
C      CALLED WHEN SIMULA IS .TRUE.) FOR CALCULATING EXACT(J)               1143
C      (THE SIMULATED DATA BEFORE NOISE IS ADDED) AND                       1144
C      Y(J) (THE SIMULATED NOISY DATA) FOR J=1,NY.                          1145
C  EXACT(J) MUST BE COMPUTED IN USEREX.                                     1146
C  IUSER(3) = STARTING INTEGER FOR RANDOM NUMBER GENERATOR RANDOM.          1147
C  IUSER(3) AND RUSER(3) MUST BE SUPPLIED BY THE USER.                      1148
C  IUSER(3) MUST BE BETWEEN 1 AND 2147483646.  IF IT IS NOT, THEN           1149
C      IT IS SET TO THE DEFAULT VALUE OF 30171.                             1150
C  SEE THE USERS MANUAL FOR THE POSITION IN THE INPUT                       1151
C      DATA DECK OF ANY INPUT FOR THIS USER SUBPROGRAM.                     1152
C  BELOW IS ILLUSTRATED THE CASE WHERE ZERO-MEAN                            1153
C      NORMALLY DISTRIBUTED PSEUDORANDOM NOISE IS ADDED TO EXACT(J).        1154
C      SD(J), THE STANDARD DEVIATION OF THE NOISE AT POINT J, IS            1155
C      DETERMINED BY IWT AND RUSER(3) AS FOLLOWS -                          1156
C      IWT = 1 CAUSES SD(J)=RUSER(3) FOR ALL J.                             1157
C      IWT = 2 CAUSES SD(J)=RUSER(3)*SQRT(EXACT(J)), AS APPROPRIATE         1158
C              FOR POISSON STATISTICS.  IN THE POISSON CASE,                1159
C              RUSER(3) IS JUST A SCALE FACTOR FOR THE CASE THAT            1160
C              EXACT(J) IS NOT IN NUMBER OF EVENTS, I.E.,                   1161
C              RUSER(3)=SQRT(EXACT(J)/(NO. OF EVENTS IN CHANNEL J)).        1162
C              THUS, EXACT(J)/RUSER(3)**2 IS THE POISSON VARIABLE.          1163
C              IF EXACT(J) IS ALREADY IN NO. OF EVENTS, THEN YOU            1164
C              SHOULD SET RUSER(3)=1.                                       1165
C      IWT = 3 CAUSES SD(J)=RUSER(3)*EXACT(J).                              1166
C      IWT = 4 CAUSES SD(J)=RUSER(3)/SQRTW(J).                              1167
C      IWT = 5 IS FOR THE SPECIAL CASE OF A POISSON SECOND ORDER            1168
C              CORRELATION FUNCTION IN PHOTON CORRELATION SPECTROSCOPY.     1169
C              YOU SHOULD SET RUSER(3)=1/SQRT(B), AND                       1170
C              USEREX MUST COMPUTE THE SIMULATED (1ST ORDER CORRELATION     1171
C              FUNCTION)*GAMMA, WHERE GAMMA AND B ARE GIVEN IN EQ.(6)       1172
C              OF MAKROMOL. CHEM., VOL. 180, P. 201 (1979).                 1173
C      (SEE ALSO THE USERS MANUAL.)                                         1174
C-----------------------------------------------------------------------    1175
C  CALLS SUBPROGRAMS - USEREX, RGAUSS, ERRMES                               1176
C  WHICH IN TURN CALL - RANDOM, USERK, USERLF, USERTR                       1177
C-----------------------------------------------------------------------    1178
      SUBROUTINE USERSI (EXACT,G,MG,MY,SQRTW,T,Y)                           1179
      DOUBLE PRECISION PRECIS, RANGE                                        1180
      DOUBLE PRECISION DUB                                                  1181
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  1182
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              1183
      DIMENSION T(MY), EXACT(MY), Y(MY), SQRTW(MY), G(MG)                   1184
      DIMENSION RN(2), IHOLER(6)                                            1185
      COMMON /DBLOCK/ PRECIS, RANGE                                         1186
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         1187
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     1188
     2 SRANGE                                                               1189
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     1190
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       1191
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             1192
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              1193
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         1194
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            1195
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  1196
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            1197
     2 LUSER(30)                                                            1198
      DATA IHOLER/1HU, 1HS, 1HE, 1HR, 1HS, 1HI/                             1199
      TWOPI=6.2831853072D0                                                  1200
      DUB=DBLE(FLOAT(IUSER(3)))                                             1201
      IF (DUB.LT.1.D0 .OR. DUB.GT.2147483646.D0) DUB=30171.D0               1202
      DO 150 J=1,NY                                                         1203
        JJ=J                                                                1204
C-----------------------------------------------------------------------    1205
C  RGAUSS DELIVERS TWO NORMAL DEVIATES WITH ZERO MEANS AND STANDARD         1206
C      DEVIATIONS 1.  THEREFORE IT IS ONLY CALLED FOR ODD J.                1207
C-----------------------------------------------------------------------    1208
        K=2-MOD(J,2)                                                        1209
        IF (K .EQ. 1) CALL RGAUSS (RN(1),RN(2),TWOPI,DUB)                   1210
C-----------------------------------------------------------------------    1211
C  YOU MUST REPLACE THE FOLLOWING STATEMENTS WITH THOSE APPROPRIATE         1212
C      FOR YOUR APPLICATION.                                                1213
C-----------------------------------------------------------------------    1214
        EXACT(J)=USEREX(JJ,T,MY,G,MG)                                       1215
C-----------------------------------------------------------------------    1216
C  THE NEXT STATEMENT TEMPORARILY SETS THE ERROR TO A NORMAL DEVIATE        1217
C      WITH MEAN = ZERO AND STANDARD DEVIATION = RUSER(3).                  1218
C-----------------------------------------------------------------------    1219
        ERROR=RUSER(3)*RN(K)                                                1220
        IF (IWT .NE. 2) GO TO 160                                           1221
        IF (EXACT(J) .LT. 0.) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)            1222
        ERROR=ERROR*SQRT(EXACT(J))                                          1223
        GO TO 190                                                           1224
  160   IF (IWT .EQ. 3) ERROR=ERROR*EXACT(J)                                1225
        IF (IWT .NE. 4) GO TO 170                                           1226
        IF (SQRTW(J) .LE. 0.) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)            1227
        ERROR=ERROR/SQRTW(J)                                                1228
  170   IF (IWT .NE. 5) GO TO 190                                           1229
C-----------------------------------------------------------------------    1230
C  SPECIAL CASE OF A POISSON SECOND ORDER CORRELATION FUNCTION IN           1231
C      PHOTON CORRELATION SPECTROSCOPY.                                     1232
C  G2 = NOISE-FREE NORMALIZED 2ND ORDER CORRELATION FUNCTION.               1233
C  G2N1 = NOISY 2ND ORDER CORRELATION FUNCTION LESS 1.                      1234
C  Y(J) = NOISY FIRST ORDER CORRELATION FUNCTION.  IT IS EVALUATED          1235
C         BELOW WITH THE FUNCTION SIGN() SO THAT NEGATIVE DATA POINTS       1236
C         REMAIN NEGATIVE.  THIS PREVENTS A BIAS TOWARD POSITIVE            1237
C         NOISE COMPONENTS.                                                 1238
C-----------------------------------------------------------------------    1239
        G2=1.+EXACT(J)**2                                                   1240
        G2N1=G2-1.+ERROR*SQRT(G2)                                           1241
        Y(J)=SIGN(SQRT(ABS(G2N1)),G2N1)                                     1242
        GO TO 150                                                           1243
  190   Y(J)=EXACT(J)+ERROR                                                 1244
  150 CONTINUE                                                              1245
      RETURN                                                                1246
      END                                                                   1247
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    1248
C  SUBROUTINE USERSX.  THIS IS A USER-SUPPLIED ROUTINE (ONLY                1249
C      CALLED WHEN YOU HAVE INPUT ONLY1=.FALSE.)                            1250
C      TO COMPUTE EXACT(J),J=1,NG, WHICH WILL BE PLOTTED WITH               1251
C      EACH SOLUTION.  USUALLY EXACT IS THE EXACT THEORETICAL               1252
C      SOLUTION USED TO SIMULATE YOUR DATA IN USERSI EVALUATED AT           1253
C      THE GRID POINTS G(J),J=1,NG.                                         1254
C  SEE THE USERS MANUAL FOR THE POSITION IN THE INPUT                       1255
C      DATA DECK OF ANY INPUT FOR THIS USER SUBPROGRAM.                     1256
C  BELOW IS ILLUSTRATED THE CASE WHERE EXACT IS                             1257
C      G(J)**RUSER(8)*EXP(-G(J))/(FACTORIAL OF RUSER(8)), WHERE             1258
C      1. .LE. RUSER(8) .LE. 20. AND THE G(J) ARE NONNEGATIVE.              1259
C-----------------------------------------------------------------------    1260
C  CALLS SUBPROGRAMS - GAMLN                                                1261
C-----------------------------------------------------------------------    1262
      SUBROUTINE USERSX (EXACT,G,MG)                                        1263
      DOUBLE PRECISION PRECIS, RANGE                                        1264
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  1265
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              1266
      DIMENSION EXACT(MG), G(MG)                                            1267
      COMMON /DBLOCK/ PRECIS, RANGE                                         1268
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         1269
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     1270
     2 SRANGE                                                               1271
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     1272
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       1273
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             1274
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              1275
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         1276
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            1277
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  1278
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            1279
     2 LUSER(30)                                                            1280
C-----------------------------------------------------------------------    1281
C  YOU MUST REPLACE THE FOLLOWING STATEMENTS WITH THOSE APPROPRIATE         1282
C      FOR YOUR EVALUATION OF EXACT.                                        1283
C-----------------------------------------------------------------------    1284
      IF (RUSER(8).GE.1. .AND. RUSER(8).LE.20.) GO TO 120                   1285
 5120 FORMAT (/11H RUSER(8) =,E12.4,27H IS OUT OF RANGE IN USERSX.)         1286
      WRITE (NOUT,5120) RUSER(8)                                            1287
      STOP                                                                  1288
  120 EXMIN=-ALOG(SRANGE)                                                   1289
      FACTL=GAMLN(RUSER(8)+1.)                                              1290
      DO 150 J=1,NG                                                         1291
        EXACT(J)=0.                                                         1292
        IF (G(J)) 160,150,180                                               1293
  160   WRITE (NOUT,5160)                                                   1294
 5160   FORMAT (/22H NEGATIVE G IN USEREX.)                                 1295
        STOP                                                                1296
  180   EX=RUSER(8)*ALOG(G(J))-G(J)-FACTL                                   1297
        IF (EX .GE. EXMIN) EXACT(J)=EXP(EX)                                 1298
  150 CONTINUE                                                              1299
      RETURN                                                                1300
      END                                                                   1301
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    1302
C  FUNCTION USERTR.  THIS IS A USER-SUPPLIED ROUTINE                        1303
C      FOR COMPUTING THE QUADRATURE-GRID TRANSFORMATION                     1304
C      (CALL IT H) H(G) WHEN IFUNCT=1, THE INVERSE TRANSFORMATION           1305
C      WHEN IFUNCT=2, AND THE DERIVATIVE OF THE TRANSFORMATION              1306
C      WHEN IFUNCT=3.  WHEN IGRID=2, G (THE QUADRATURE GRID) WILL           1307
C      BE IN EQUAL INTERVALS OF H(G) RATHER THAN IN EQUAL                   1308
C      INTERVALS OF G (AS IT IS WHEN IGRID=1 AND H IS THE                   1309
C      IDENTITY TRANSFORMATION).                                            1310
C  BELOW IS ILLUSTRATED THE CASE H(G)=ALOG(G).  FOR ANOTHER H,              1311
C      YOU CAN REPLACE THE STATEMENTS NUMBERED 210, 220, AND 230.           1312
C      THESE ARE THE ONLY STATEMENTS THAT CAN BE REPLACED.  ALSO            1313
C      NOTE THAT ONLY AN H THAT IS MONOTONIC IN THE RANGE OF                1314
C      INTEGRATION MAKES SENSE.                                             1315
C-----------------------------------------------------------------------    1316
C  CALLS SUBPROGRAMS - ERRMES                                               1317
C-----------------------------------------------------------------------    1318
      FUNCTION USERTR (X,IFUNCT)                                            1319
      DOUBLE PRECISION PRECIS, RANGE                                        1320
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  1321
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              1322
      COMMON /DBLOCK/ PRECIS, RANGE                                         1323
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         1324
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     1325
     2 SRANGE                                                               1326
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     1327
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       1328
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             1329
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              1330
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         1331
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            1332
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  1333
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            1334
     2 LUSER(30)                                                            1335
      DIMENSION IHOLER(6)                                                   1336
      DATA IHOLER/1HU, 1HS, 1HE, 1HR, 1HT, 1HR/                             1337
      IF (IFUNCT.LT.1 .OR. IFUNCT.GT.3) CALL ERRMES (1,.TRUE.,              1338
     1 IHOLER,NOUT)                                                         1339
      IF (IGRID .NE. 1) GO TO 200                                           1340
C-----------------------------------------------------------------------    1341
C  COMPUTE TRANSFORMATION, INVERSE, AND DERIVATIVE FOR IDENTITY             1342
C      TRANSFORMATION.                                                      1343
C-----------------------------------------------------------------------    1344
      USERTR=1.                                                             1345
      IF (IFUNCT .NE. 3) USERTR=X                                           1346
      RETURN                                                                1347
  200 IF (IGRID .NE. 2) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                  1348
      GO TO (210,220,230),IFUNCT                                            1349
C-----------------------------------------------------------------------    1350
C  COMPUTE TRANSFORMATION.                                                  1351
C-----------------------------------------------------------------------    1352
  210 USERTR=ALOG(X)                                                        1353
      RETURN                                                                1354
C-----------------------------------------------------------------------    1355
C  COMPUTE INVERSE TRANSFORMATION.                                          1356
C-----------------------------------------------------------------------    1357
  220 USERTR=EXP(X)                                                         1358
      RETURN                                                                1359
C-----------------------------------------------------------------------    1360
C  COMPUTE DERIVATIVE OF TRANSFORMATION.                                    1361
C-----------------------------------------------------------------------    1362
  230 USERTR=1./X                                                           1363
      RETURN                                                                1364
      END                                                                   1365
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    1366
C  SUBROUTINE USERWT.  THIS IS A USER-SUPPLIED ROUTINE (ONLY                1367
C      NEEDED WHEN IWT IS INPUT AS 5) FOR CALCULATING SQRTW (SQUARE         1368
C      ROOTS OF THE LEAST SQUARES WEIGHTS) FROM Y, YLYFIT, AND              1369
C      ERRFIT, AS EXPLAINED IN DETAIL IN THE USERS MANUAL.                  1370
C  SEE THE USERS MANUAL FOR THE POSITION IN THE INPUT                       1371
C      DATA DECK OF ANY INPUT FOR THIS USER SUBPROGRAM.                     1372
C  BELOW IS ILLUSTRATED THE CASE FROM PHOTON CORRELATION SPECTROSCOPY,      1373
C      WHERE THE VARIANCE OF Y IS PROPORTIONAL TO (Y**2+1)/(4*Y**2).        1374
      SUBROUTINE USERWT (Y,YLYFIT,MY,ERRFIT,SQRTW)                          1375
      DOUBLE PRECISION PRECIS, RANGE                                        1376
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  1377
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              1378
      DIMENSION Y(MY), YLYFIT(MY), SQRTW(MY)                                1379
      COMMON /DBLOCK/ PRECIS, RANGE                                         1380
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         1381
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     1382
     2 SRANGE                                                               1383
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     1384
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       1385
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             1386
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              1387
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         1388
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            1389
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  1390
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            1391
     2 LUSER(30)                                                            1392
C-----------------------------------------------------------------------    1393
C  YOU MUST REPLACE THE FOLLOWING STATEMENTS WITH THOSE APPROPRIATE         1394
C      FOR YOUR APPLICATION.                                                1395
C-----------------------------------------------------------------------    1396
      DO 110 J=1,NY                                                         1397
        DUM=AMAX1(ABS(Y(J)-YLYFIT(J)),ERRFIT)                               1398
        SQRTW(J)=2.*DUM/SQRT(DUM*DUM+1.)                                    1399
  110 CONTINUE                                                              1400
      RETURN                                                                1401
      END                                                                   1402
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    1403
C  SUBROUTINE ANALYZ.  DOES COMPLETE CONSTRAINED REGULARIZED                1404
C     ANALYSIS.                                                             1405
C-----------------------------------------------------------------------    1406
C  CALLS SUBPROGRAMS - SETSCA, SEQACC, SETREG, USEREQ, ELIMEQ, SVDRS2,      1407
C     ERRMES, DIAREG, DIAGA, SETGA1, SETVAL, LDPETC, RUNRES,                1408
C     ANPEAK, SETSGN, SETNNG                                                1409
C  WHICH IN TURN CALL - H12, GETROW, USERK, USERLF, USERRG, LH1405,         1410
C     QRBD, G1, G2, DIFF, LDP, CVNEQ, FISHNI, BETAIN, GAMLN,                1411
C     NNLS, PLRES, UPDSGN, UPDDON, FFLAT, UPDLLS,                           1412
C     GETPRU, GETYLY, PGAUSS, MOMENT, MOMOUT, PLPRIN, USEROU, USERTR        1413
C-----------------------------------------------------------------------    1414
      SUBROUTINE ANALYZ (ISTAGE,                                            1415
     1 A,AA,AEQ,AINEQ,CQUAD,EXACT,G,IISIGN,IWORK,LBIND,LSDONE,MA,           1416
     2 MDONE,MEQ,MG,MINEQ,MREG,MWORK,MY,PIVOT,REG,RHSNEQ,S,SOLBES,          1417
     3 SOLUTN,SQRTW,SSCALE,T,VALPCV,VALPHA,VDONE,VK1Y1,WORK,                1418
     4 Y,YLYFIT)                                                            1419
      DOUBLE PRECISION PRECIS, RANGE                                        1420
      DOUBLE PRECISION A, AA, ABS, AEQ, AINEQ, ALPBES, ALPHA,               1421
     1 ALPOLD, DUB, ONE, PIVOT, RALPFL, REG, RHSNEQ,                        1422
     2 S, SOLBES, SOLUTN, SSCALE, VALPCV, VALPHA, VK1Y1, WORK, ZERO         1423
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  1424
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              1425
      LOGICAL LDUM, LBIND, FLAT, PPLTPR, HEADNG, NEWPAG, NOPAGE             1426
      DIMENSION A(MA,MG), T(MY), Y(MY), SQRTW(MY), G(MG), CQUAD(MG),        1427
     1 REG(MREG,MG), AEQ(MEQ,MG), PIVOT(MEQ), VK1Y1(MG), S(MG,3),           1428
     2 AINEQ(MINEQ,MG), VALPHA(MG), VALPCV(MG), RHSNEQ(MINEQ),              1429
     3 WORK(MWORK), IWORK(MA), EXACT(MG), SOLUTN(MG), LBIND(MINEQ),         1430
     4 IISIGN(MG), SOLBES(MG), LSDONE(MDONE,3,2), VDONE(MDONE),             1431
     5 YLYFIT(MY), AA(MG,MG), SSCALE(MG)                                    1432
      DIMENSION IHOLER(6), PREJ(2), LLSIGN(5), RS2MNX(2)                    1433
      COMMON /DBLOCK/ PRECIS, RANGE                                         1434
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         1435
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     1436
     2 SRANGE                                                               1437
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     1438
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       1439
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             1440
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              1441
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         1442
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            1443
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  1444
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            1445
     2 LUSER(30)                                                            1446
      DATA IHOLER/1HA, 1HN, 1HA, 1HL, 1HY, 1HZ/, RALPHA/1./                 1447
      ABS(ALPHA)=DABS(ALPHA)                                                1448
      SINGLE(ONE)=SNGL(ONE)                                                 1449
C     SINGLE(VAR)=VAR!SP                                                    1450
C     ZERO=0.E0!SP                                                          1451
      ZERO=0.D0                                                             1452
C     ONE=1.E0!SP                                                           1453
      ONE=1.D0                                                              1454
      NGLE=NGL-NEQ                                                          1455
C-----------------------------------------------------------------------    1456
C  PUT SCALE FACTORS FOR SOLUTION IN SSCALE AND SCALE INEQUALITY            1457
C      CONSTRAINTS.                                                         1458
C-----------------------------------------------------------------------    1459
      MG1=MG+1                                                              1460
      MG2=MG1+MG                                                            1461
      MG3=MG2+MG                                                            1462
      CALL SETSCA (WORK,WORK(MG1),WORK(MG2),WORK(MG3),                      1463
     1 AINEQ,CQUAD,G,ISTAGE,MG,MINEQ,MREG,MY,NGLE,REG,S,SQRTW,SSCALE,T,     1464
     2 Y)                                                                   1465
C-----------------------------------------------------------------------    1466
C  DO SEQUENTIAL HOUSEHOLDER TRANSFORMATIONS TO COMPRESS                    1467
C     COEFFICIENT MATRIX INTO AN UPPER TRIANGLE.                            1468
C-----------------------------------------------------------------------    1469
      CALL SEQACC (                                                         1470
     1 A,CQUAD,G,IUNIT,IWT,MA,MG,NG,NGL,NGLP1,NLINF,NY,                     1471
     2 RANGE,SQRTW,SSCALE,T,Y)                                              1472
      NGLY=MIN0(NGL,NY)                                                     1473
C-----------------------------------------------------------------------    1474
C  SET UP REGULARIZOR.                                                      1475
C-----------------------------------------------------------------------    1476
      CALL SETREG (MG,MREG,NENDZ,NG,NGL,NGLE,NGLP1,NORDER,                  1477
     1 NOUT,NREG,REG,SSCALE)                                                1478
      IF (NEQ .LE. 0) GO TO 200                                             1479
C-----------------------------------------------------------------------    1480
C  SET EQUALITY CONSTRAINTS IN USER-SUPPLIED PROGRAM USEREQ AND THEN        1481
C      SCALE THEM AND NORMALIZE THEIR ROWS (IN THE L1 METRIC).              1482
C-----------------------------------------------------------------------    1483
      CALL USEREQ (AEQ,CQUAD,MEQ,MG)                                        1484
      DO 150 ICOL=1,NGL                                                     1485
        DO 160 IROW=1,NEQ                                                   1486
          AEQ(IROW,ICOL)=AEQ(IROW,ICOL)*SSCALE(ICOL)                        1487
  160   CONTINUE                                                            1488
  150 CONTINUE                                                              1489
      DO 170 IROW=1,NEQ                                                     1490
        DUB=ZERO                                                            1491
        DO 180 ICOL=1,NGL                                                   1492
          DUB=DUB+ABS(AEQ(IROW,ICOL))                                       1493
  180   CONTINUE                                                            1494
        IF (DUB .LE. ZERO) CALL ERRMES (0,.TRUE.,IHOLER,NOUT)               1495
        DUB=ONE/DUB                                                         1496
        DO 190 ICOL=1,NGLP1                                                 1497
          AEQ(IROW,ICOL)=AEQ(IROW,ICOL)*DUB                                 1498
  190   CONTINUE                                                            1499
  170 CONTINUE                                                              1500
C-----------------------------------------------------------------------    1501
C  ELIMINATE EQUALITY CONSTRAINTS USING A BASIS OF NULL SPACE OF AEQ.       1502
C-----------------------------------------------------------------------    1503
  200 CALL ELIMEQ (AEQ,MEQ,MG,PIVOT,NEQ,NGL,A,MA,REG,MREG,NREG,NGLP1,       1504
     1 NGLY,VK1Y1,RANGE)                                                    1505
C-----------------------------------------------------------------------    1506
C  DO SINGULAR VALUE DECOMPOSITION AND DIAGONALIZATION OF REGULARIZOR.      1507
C-----------------------------------------------------------------------    1508
      CALL SVDRS2 (REG(1,NEQ+1),MREG,NREG,NGLE,REG(1,NGLP1),MREG,           1509
     1 1,S,IERROR,RANGE)                                                    1510
      IF (IERROR .NE. 1) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)                 1511
      CALL DIAREG (A,AEQ,MA,MEQ,MG,MREG,NEQ,NGL,NGLE,NGLY,NOUT,             1512
     1 NUNREG,PIVOT,PRECIS,RANGE,REG,S)                                     1513
C-----------------------------------------------------------------------    1514
C  DO SINGULAR VALUE DECOMPOSITION AND DIAGONALIZATION OF MODIFIED          1515
C     COMPRESSED COEFFICIENT MATRIX A.                                      1516
C-----------------------------------------------------------------------    1517
      CALL SVDRS2 (A,MA,NGLY,NGLE,A(1,NGLP1),MA,1,S,IERROR,RANGE)           1518
      IF (IERROR .NE. 1) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                 1519
      CALL DIAGA (A,MA,MG,MREG,NEQ,NGL,NGLE,NGLP1,NGLY,REG,S)               1520
      DO 320 J=1,NGLE                                                       1521
        YLYFIT(J)=S(J,1)                                                    1522
  320 CONTINUE                                                              1523
 5320 FORMAT (/16H SINGULAR VALUES/(1X,1P10E13.3))                          1524
      WRITE (NOUT,5320) (YLYFIT(J),J=1,NGLE)                                1525
C-----------------------------------------------------------------------    1526
C  LEFT MULTIPLY BY INEQUALITY MATRIX.                                      1527
C-----------------------------------------------------------------------    1528
      IF (NINEQ .GT. 0) CALL SETGA1 (NINEQ,                                 1529
     1 A,AINEQ,MA,MG,MINEQ,MREG,NGL,NGLE,REG)                               1530
      IF (MAX0(NQPROG(1),NQPROG(2)).LE.0 .AND. ALPST(ISTAGE).LE.0.) CALL    1531
     1 ERRMES (3,.TRUE.,IHOLER,NOUT)                                        1532
      RSVM2J=SRANGE                                                         1533
      IF (DFMIN.LT.0. .OR. DFMIN+FLOAT(NUNREG).GE.FLOAT(NGLE)) GO TO 390    1534
C-----------------------------------------------------------------------    1535
C  GUARANTEE AT LEAST NUNREG+DFMIN DEGREES OF FREEDOM (IGNORING             1536
C      INEQUALITY CONSTRAINTS) BY USING RSVM2J INSTEAD OF RSVMNX(2,J),      1537
C      IF NECESSARY.                                                        1538
C-----------------------------------------------------------------------    1539
      ALP2=S(1,1)**2                                                        1540
      DO 350 J=1,500                                                        1541
        IF (ALP2 .LE. 0.) GO TO 390                                         1542
        DF=0.                                                               1543
        DO 360 K=1,NGLE                                                     1544
          DUM=S(K,1)**2                                                     1545
          DF=DF+DUM/(DUM+ALP2)                                              1546
  360   CONTINUE                                                            1547
        IF (DF .GT. DFMIN+FLOAT(NUNREG)) GO TO 370                          1548
        ALP2=ALP2*.1                                                        1549
  350 CONTINUE                                                              1550
  370 RSVM2J=SQRT(10.*ALP2)/S(1,1)                                          1551
  390 BTEST=SRANGE                                                          1552
      ALPBES=ZERO                                                           1553
      VARZ=SRANGE                                                           1554
      RS2MNX(1)=-1.                                                         1555
      RS2MNX(2)=-1.                                                         1556
C-----------------------------------------------------------------------    1557
C  START OF NQPROG(1) REGULARIZED SOLUTIONS                                 1558
C-----------------------------------------------------------------------    1559
      LDUM=.TRUE.                                                           1560
      PPLTPR=MAX0(IPRINT(ISTAGE),IUSROU(ISTAGE)) .GE. 3                     1561
      NOPAGE=.NOT.PPLTPR                                                    1562
      LHEDNG=0                                                              1563
      IF (NOPAGE) LHEDNG=4                                                  1564
      K=NQPROG(1)                                                           1565
      IF (K .LE. 1) GO TO 410                                               1566
      RTOT=AMIN1(RSVM2J,RSVMNX(2,1))/(RSVMNX(1,1)*PRECIS)                   1567
      IF (RTOT .LE. 1.) RTOT=RSVMNX(2,1)/(RSVMNX(1,1)*PRECIS)               1568
      IF (RTOT .LE. 1.) CALL ERRMES (4,.TRUE.,IHOLER,NOUT)                  1569
      RALPHA=RTOT**(1./FLOAT(K-1))                                          1570
  410 ALPHA=RSVMNX(1,1)*PRECIS*S(1,1)                                       1571
      DO 420 J=1,K                                                          1572
          CALL SETVAL (ALPHA,LDUM,NINEQ,                                    1573
     1  A,AINEQ,MA,MG,MINEQ,MREG,NGL,NGLE,REG,RHSNEQ,S,VALPCV,VALPHA,       1574
     2  VK1Y1)                                                              1575
          NEWPAG=MAX0(IPRINT(ISTAGE),IUSROU(ISTAGE),                        1576
     1    IPLRES(ISTAGE)+1).GE.4 .OR. LDUM                                  1577
          HEADNG=PPLTPR .OR. LDUM .OR.                                      1578
     1    MAX0(IPLRES(ISTAGE),IPLFIT(ISTAGE)).GE.3                          1579
          CALL LDPETC (3,.TRUE.,NINEQ,.TRUE.,ICRIT(ISTAGE),DOMOM,PPLTPR,    1580
     1  .TRUE.,ALPHA,HEADNG,NEWPAG,ALPBES,VAR,                              1581
     2  A,AA,AINEQ,BTEST,CQUAD,DEGFRE,DEGFRZ,EXACT,G,IERROR,                1582
     3  ISTAGE,IWORK,LBIND,MA,MG,MINEQ,MREG,MWORK,MY,                       1583
     4  NGLE,NGLY,PREJ,REG,RHSNEQ,RS2MNX,S,SOLBES,                          1584
     5  SOLUTN,SQRTW,SSCALE,T,VALPCV,VALPHA,VARREG,VARZ,WORK,Y,YLYFIT)      1585
          IF (IERROR.EQ.1) CALL RUNRES (3,SOLUTN,.FALSE.,                   1586
     A SINGLE(ALPHA/S(1,1)),NOPAGE,                                         1587
     1 CQUAD,G,IPLFIT,IPLRES,ISTAGE,ITITLE,IUNIT,IWT,LINEPG-LHEDNG,         1588
     2 MWORK,NG,NGL,NLINF,NOUT,NY,SQRTW,SRANGE,SSCALE,T,WORK,Y,YLYFIT)      1589
          ALPHA=ALPHA*RALPHA                                                1590
          LDUM=.FALSE.                                                      1591
  420 CONTINUE                                                              1592
C-----------------------------------------------------------------------    1593
C  START OF NQPROG(2) REGULARIZED SOLUTIONS.                                1594
C-----------------------------------------------------------------------    1595
  450 IF (ALPST(ISTAGE) .LE. 0.) GO TO 455                                  1596
      K=1                                                                   1597
      ALPHA=ALPST(ISTAGE)                                                   1598
      BTEST=SRANGE                                                          1599
      GO TO 475                                                             1600
  455 IF (NQPROG(2) .LE. 0) GO TO 490                                       1601
      K=NQPROG(2)                                                           1602
      IF (AMIN1(RSVMNX(1,2),RSVMNX(2,2)) .LE. 0.) GO TO 465                 1603
      ALPHA=RSVMNX(1,2)*PRECIS*S(1,1)                                       1604
      IF (K .LE. 1) GO TO 475                                               1605
      RTOT=AMIN1(RSVM2J,RSVMNX(2,2))/(RSVMNX(1,2)*PRECIS)                   1606
      IF (RTOT .LE. 1.) RTOT=RSVMNX(2,2)/(RSVMNX(1,2)*PRECIS)               1607
      IF (RTOT .LE. 1.) CALL ERRMES (4,.TRUE.,IHOLER,NOUT)                  1608
      RALPHA=RTOT**(1./FLOAT(K-1))                                          1609
      GO TO 475                                                             1610
  465 NABUT=2                                                               1611
      IF (RS2MNX(1) .GT. 0.) NABUT=NABUT-1                                  1612
      IF (RS2MNX(2) .GT. 0.) NABUT=NABUT-1                                  1613
      L=K-NABUT+1                                                           1614
      IF (L .LE. 0) GO TO 470                                               1615
      RTOT=ABS(AMIN1(RSVM2J,RS2MNX(2))/(RS2MNX(1)*PRECIS))                  1616
      IF (RTOT .LE. 1.) RTOT=ABS(RS2MNX(2)/(RS2MNX(1)*PRECIS))              1617
      RALPHA=RTOT**(1./FLOAT(L))                                            1618
      IF (RS2MNX(1) .GT. 0.) RS2MNX(1)=RS2MNX(1)*RALPHA                     1619
  470 ALPHA=ABS(RS2MNX(1)*PRECIS)*S(1,1)                                    1620
  475 DO 480 J=1,K                                                          1621
          CALL SETVAL (ALPHA,LDUM,NINEQ,                                    1622
     1  A,AINEQ,MA,MG,MINEQ,MREG,NGL,NGLE,REG,RHSNEQ,S,VALPCV,VALPHA,       1623
     2  VK1Y1)                                                              1624
          NEWPAG=MAX0(IPRINT(ISTAGE),IUSROU(ISTAGE),                        1625
     1    IPLRES(ISTAGE)+1).GE.4 .OR. LDUM                                  1626
          HEADNG=PPLTPR .OR. LDUM .OR.                                      1627
     1    MAX0(IPLRES(ISTAGE),IPLFIT(ISTAGE)).GE.3                          1628
          CALL LDPETC (3,.TRUE.,NINEQ,.TRUE.,ICRIT(ISTAGE),DOMOM,PPLTPR,    1629
     1  .TRUE.,ALPHA,HEADNG,NEWPAG,ALPBES,VAR,                              1630
     2  A,AA,AINEQ,BTEST,CQUAD,DEGFRE,DEGFRZ,EXACT,G,IERROR,                1631
     3  ISTAGE,IWORK,LBIND,MA,MG,MINEQ,MREG,MWORK,MY,                       1632
     4  NGLE,NGLY,PREJ,REG,RHSNEQ,RS2MNX,S,SOLBES,                          1633
     5  SOLUTN,SQRTW,SSCALE,T,VALPCV,VALPHA,VARREG,VARZ,WORK,Y,YLYFIT)      1634
          IF (IERROR.EQ.1) CALL RUNRES (3,SOLUTN,.FALSE.,                   1635
     A SINGLE(ALPHA/S(1,1)),NOPAGE,                                         1636
     1 CQUAD,G,IPLFIT,IPLRES,ISTAGE,ITITLE,IUNIT,IWT,LINEPG-LHEDNG,         1637
     2 MWORK,NG,NGL,NLINF,NOUT,NY,SQRTW,SRANGE,SSCALE,T,WORK,Y,YLYFIT)      1638
          ALPHA=ALPHA*RALPHA                                                1639
          LDUM=.FALSE.                                                      1640
  480 CONTINUE                                                              1641
  490 IF (BTEST .GE. SRANGE) CALL ERRMES (5,.TRUE.,IHOLER,NOUT)             1642
      IF (NNSGN(ISTAGE) .LE. 0) GO TO 700                                   1643
C-----------------------------------------------------------------------    1644
C  START PEAK-CONSTRAINED SOLUTION BY SETTING UP NEW INEQUALITY             1645
C     MATRIX IN AINEQ AND INITIALIZING SO THAT UNSCALED SOLUTION IS         1646
C     MONOTONICALLY DECREASING.                                             1647
C-----------------------------------------------------------------------    1648
      NGM1=NG-1                                                             1649
      NNINEQ=NINEQ-1                                                        1650
      IF (NONNEG) GO TO 510                                                 1651
      NNINEQ=NINEQ+NGM1                                                     1652
      IF (NNINEQ .LE. MINEQ) GO TO 510                                      1653
      CALL ERRMES (6,.FALSE.,IHOLER,NOUT)                                   1654
      GO TO 790                                                             1655
  510 IROW=NNINEQ-NGM1                                                      1656
      DO 520 J=1,NGM1                                                       1657
       IROW=IROW+1                                                          1658
       DO 525 ICOL=1,NGLP1                                                  1659
         AINEQ(IROW,ICOL)=ZERO                                              1660
  525  CONTINUE                                                             1661
       AINEQ(IROW,J)=ONE*SSCALE(J)                                          1662
       AINEQ(IROW,J+1)=-ONE*SSCALE(J+1)                                     1663
       IISIGN(J)=1                                                          1664
  520 CONTINUE                                                              1665
      CALL SETGA1 (NNINEQ,                                                  1666
     1 A,AINEQ,MA,MG,MINEQ,MREG,NGL,NGLE,REG)                               1667
      NNSGNI=MIN0(NNSGN(ISTAGE),4)                                          1668
      ALPOLD=ZERO                                                           1669
      IF (ISTAGE .EQ. 1) BTEST=SRANGE                                       1670
C-----------------------------------------------------------------------    1671
C  START LOOP TO DO NNSGN(ISTAGE) DIFFERENT PEAK-CONSTRAINED                1672
C     ANALYSES.                                                             1673
C-----------------------------------------------------------------------    1674
      DO 600 INSGN=1,NNSGNI                                                 1675
       NSGNI=NSGN(INSGN)                                                    1676
       LNINEQ=NNINEQ                                                        1677
       IF (NONNEG) LNINEQ=LNINEQ+(NSGNI-(1+LSIGN(1,INSGN))/2)/2+1           1678
       IF (LNINEQ .LE. MINEQ) GO TO 610                                     1679
       CALL ERRMES (7,.FALSE.,IHOLER,NOUT)                                  1680
       GO TO 790                                                            1681
C-----------------------------------------------------------------------    1682
C  NNQUSR = NO. OF USER-SUPPLIED INEQUALITIES.                              1683
C     (NNQUSR+1 = STARTING ROW OF PEAK CONSTRAINTS IN AINEQ.)               1684
C  NINEQ=NO. OF INEQUALITY CONSTRAINTS IN PREVIOUS PARTS OF PROGRAM         1685
C      WITHOUT PEAK CONSTRAINTS (= NO. OF USER-SUPPLIED CONSTRAINTS +       1686
C      NONNEGATIVITY CONSTRAINTS).                                          1687
C  NNINEQ = TOTAL NO. OF INEQUALITY CONSTRAINTS INCLUDING MONOTONICITY      1688
C      CONSTRAINTS BUT NOT NONNEGATIVITY CONSTRAINTS AT MINIMA.             1689
C  LNINEQ = TOTAL NO. OF INEQUALITY CONSTRAINTS.                            1690
C-----------------------------------------------------------------------    1691
  610  NNQUSR=NINEQ                                                         1692
       IF (NONNEG) NNQUSR=NNQUSR-NG                                         1693
       RALPFL=ONE                                                           1694
       IF (MAX0(NQPROG(1),NQPROG(2)) .GT. 1) RALPFL=RALPHA                  1695
       ALPHA=ALPBES/RALPFL                                                  1696
       MFLAT=MAX0(1,NFLAT(INSGN,ISTAGE))                                    1697
       IF (RALPFL .LE. ONE) MFLAT=1                                         1698
C-----------------------------------------------------------------------    1699
C  START OF LOOP TO DO UP TO MFLAT PEAK-CONSTRAINED ANALYSES                1700
C     UNTIL THERE ARE NO FLAT SPOTS IN THE SOLUTION.                        1701
C-----------------------------------------------------------------------    1702
       DO 620 JFLAT=1,MFLAT                                                 1703
         ALPHA=ALPHA*RALPFL                                                 1704
         IF (ABS(ALPOLD/ALPHA-ONE) .LE. 1.E3*PRECIS) GO TO 630              1705
         ALPOLD=ALPHA                                                       1706
         LDUM=INSGN.EQ.1 .AND. JFLAT.EQ.1                                   1707
         CALL SETVAL (ALPHA,LDUM,NNINEQ,                                    1708
     1  A,AINEQ,MA,MG,MINEQ,MREG,NGL,NGLE,REG,RHSNEQ,S,VALPCV,VALPHA,       1709
     2  VK1Y1)                                                              1710
  630    J=INSGN                                                            1711
         CALL SETSGN (J,NSGNI,LSIGN,NOUT,LLSIGN,NG,SOLBES,SRANGE)           1712
C-----------------------------------------------------------------------    1713
C  DO COMPLETE PEAK-CONSTRAINED SOLUTION.                                   1714
C-----------------------------------------------------------------------    1715
         CALL ANPEAK (LNINEQ,                                               1716
     1    A,AA,AINEQ,ALPHA,BTEST,CQUAD,DEGFRZ,EXACT,FLAT,G,IISIGN,          1717
     2    ISTAGE,IWORK,LBIND,LLSIGN,LSDONE,MA,MDONE,MG,MINEQ,               1718
     3    MREG,MWORK,MY,NGLE,NGLY,NNINEQ,NNQUSR,                            1719
     4    NSGNI,REG,RHSNEQ,RS2MNX,S,SOLBES,                                 1720
     5    SOLUTN,SQRTW,SSCALE,T,VALPCV,VALPHA,VARZ,VDONE,WORK,Y,YLYFIT)     1721
         IF (.NOT.FLAT) GO TO 600                                           1722
  620  CONTINUE                                                             1723
  600 CONTINUE                                                              1724
      IF ((ISTAGE.EQ.2.AND..NOT.DOCHOS) .OR. .NOT.NONNEG) GO TO 700         1725
C-----------------------------------------------------------------------    1726
C  RESTORE ORIGINAL NONNEGATIVITY CONSTRAINTS.                              1727
C-----------------------------------------------------------------------    1728
      NINEQ=NINEQ-NG                                                        1729
      CALL SETNNG (AINEQ,MINEQ,NG,NGLP1,NINEQ,NOUT)                         1730
  700 IF (IPLRES(ISTAGE).NE.2 .AND. IPLFIT(ISTAGE).NE.2) GO TO 710          1731
C-----------------------------------------------------------------------    1732
C  FOR CHOSEN SOLUTION, PLOT RESIDUALS AND FIT AND REPEAT THE               1733
C      SOLUTION FOR OUTPUT AT END OF THE RUN.                               1734
C-----------------------------------------------------------------------    1735
 5700 FORMAT (22H1CONTIN 2DP (MAR 84) (,6A1,1H),3X,80A1,4X,                 1736
     1 15HCHOSEN SOLUTION)                                                  1737
      WRITE (NOUT,5700) IAPACK,ITITLE                                       1738
      CALL RUNRES (2,SOLBES,.FALSE.,SINGLE(ALPBES/S(1,1)),.TRUE.,           1739
     1 CQUAD,G,IPLFIT,IPLRES,ISTAGE,ITITLE,IUNIT,IWT,LINEPG,MWORK,NG,       1740
     2 NGL,NLINF,NOUT,NY,SQRTW,SRANGE,SSCALE,T,WORK,Y,YLYFIT)               1741
  710 IF (ISTAGE.EQ.1 .OR. .NOT.DOCHOS .OR.                                 1742
     1 MAX0(IPRINT(2),IUSROU(2)).LT.2) GO TO 800                            1743
      LDUM=NONNEG .AND. NNSGN(ISTAGE).GT.0                                  1744
      IF (LDUM) CALL SETGA1 (NINEQ,                                         1745
     1 A,AINEQ,MA,MG,MINEQ,MREG,NGL,NGLE,REG)                               1746
      CALL SETVAL (ALPBES,LDUM,NINEQ,                                       1747
     1  A,AINEQ,MA,MG,MINEQ,MREG,NGL,NGLE,REG,RHSNEQ,S,VALPCV,VALPHA,       1748
     2  VK1Y1)                                                              1749
 5710 FORMAT (32H1CONTIN VERSION 2DP (MAR 1984) (,6A1,                      1750
     1 11H PACKAGE)  ,16(2H++),19H  CHOSEN SOLUTION  ,16(2H++))             1751
      WRITE (NOUT,5710) IAPACK                                              1752
      CALL LDPETC (2,.FALSE.,NINEQ,.FALSE.,1,DOMOM,.TRUE.,                  1753
     1 .TRUE.,ALPBES,.TRUE.,.FALSE.,ALPBES,VAR,                             1754
     2  A,AA,AINEQ,BTEST,CQUAD,DEGFRE,DEGFRZ,EXACT,G,IERROR,                1755
     3  ISTAGE,IWORK,LBIND,MA,MG,MINEQ,MREG,MWORK,MY,                       1756
     4  NGLE,NGLY,PREJ,REG,RHSNEQ,RS2MNX,S,SOLBES,                          1757
     5  SOLUTN,SQRTW,SSCALE,T,VALPCV,VALPHA,VARREG,VARZ,WORK,Y,YLYFIT)      1758
  790 IF (ISTAGE .NE. 2) STOP                                               1759
  800 RETURN                                                                1760
      END                                                                   1761
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    1762
C  SUBROUTINE ANPEAK.  DOES ONE COMPLETE PEAK-CONSTRAINED ANALYSIS          1763
C      AND PLOTS AND PRINTS SOLUTION, AS WELL AS PRINTING ONE LINE          1764
C      OF INTERMEDIATE OUTPUT PER ITERATION.                                1765
C-----------------------------------------------------------------------    1766
C  CALLS SUBPROGRAMS - UPDSGN, ERRMES, LDPETC, UPDDON, FFLAT, UPDLLS,       1767
C      RUNRES                                                               1768
C  WHICH IN TURN CALL - LDP, CVNEQ, FISHNI, BETAIN, GAMLN, NNLS,            1769
C      DIFF, H12, G1, G2, ELIMEQ, SVDRS2, LH1405, QRBD,                     1770
C      GETYLY, GETROW, USERK, USERLF, PLPRIN, USEROU, MOMENT, MOMOUT,       1771
C      GETPRU, PLRES, PGAUSS, USERTR                                        1772
C-----------------------------------------------------------------------    1773
      SUBROUTINE ANPEAK (LNINEQ,                                            1774
     1    A,AA,AINEQ,ALPHA,BTEST,CQUAD,DEGFRZ,EXACT,FLAT,G,IISIGN,          1775
     2    ISTAGE,IWORK,LBIND,LLSIGN,LSDONE,MA,MDONE,MG,MINEQ,               1776
     3    MREG,MWORK,MY,NGLE,NGLY,NNINEQ,NNQUSR,                            1777
     4    NSGNI,REG,RHSNEQ,RS2MNX,S,SOLBES,                                 1778
     5    SOLUTN,SQRTW,SSCALE,T,VALPCV,VALPHA,VARZ,VDONE,WORK,Y,YLYFIT)     1779
      DOUBLE PRECISION PRECIS, RANGE                                        1780
      DOUBLE PRECISION A, AA, AINEQ, ALPHA, DUB, REG, RHSNEQ, S,            1781
     1 SOLBES, SOLUTN, SSCALE, VALPCV, VALPHA, WORK                         1782
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  1783
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              1784
      LOGICAL DONE, FLAT, LDUM, LBIND, FFLAT, PPLTPR                        1785
      DIMENSION S(MG,3), LLSIGN(5), LSDONE(MDONE,3,2), VDONE(MDONE),        1786
     1 A(MA,MG), REG(MREG,MG), RHSNEQ(MINEQ), VALPHA(MG), IISIGN(MG),       1787
     2 AINEQ(MINEQ,MG), WORK(MWORK), IWORK(MA), VALPCV(MG),                 1788
     3 G(MG), EXACT(MG), CQUAD(MG), SOLUTN(MG),                             1789
     4 LBIND(MINEQ), AA(MG,MG), SOLBES(MG),                                 1790
     5 SQRTW(MY), T(MY), Y(MY), YLYFIT(MY), SSCALE(MG), RS2MNX(2)           1791
      DIMENSION IHOLER(6), ISTAR(4), JSTAGE(4), INC(4), PREJ(2),            1792
     1 VARTRY(4), LLSTRY(5,4), LLSBES(5)                                    1793
      COMMON /DBLOCK/ PRECIS, RANGE                                         1794
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         1795
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     1796
     2 SRANGE                                                               1797
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     1798
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       1799
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             1800
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              1801
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         1802
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            1803
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  1804
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            1805
     2 LUSER(30)                                                            1806
      DATA IHOLER/1HA, 1HN, 1HP, 1HE, 1HA, 1HK/,                            1807
     1 ISTAR/1H , 1H*, 1HX, 1HF/                                            1808
      NSGNM1=NSGNI-1                                                        1809
      DONE=.FALSE.                                                          1810
      FLAT=.FALSE.                                                          1811
      NDONE=0                                                               1812
      NQPITR=0                                                              1813
      ITER=0                                                                1814
      VARRBS=SRANGE                                                         1815
      DUB=0.                                                                1816
      DO 110 J=1,NSGNI                                                      1817
        JSTAGE(J)=0                                                         1818
        INC(J)=1                                                            1819
  110 CONTINUE                                                              1820
 5200 FORMAT (1H1,I15,29H-EXTREMA-CONSTRAINED ANALYSIS/                     1821
     1 8H0ALPHA =,1PE9.2,5X,12HALPHA/S(1) =,E9.2/                           1822
     2 6H0ITER.,6X,10HOBJ. FCTN.,8X,8HVARIANCE,7X,                          1823
     3 9HSTD. DEV.,3X,11HDEG FREEDOM,3X,9HPROB1 REJ,3X,9HPROB2 REJ,10X,     1824
     4 15HEXTREMA INDICES)                                                  1825
      DUM=ALPHA/S(1,1)                                                      1826
      DDUM=ALPHA                                                            1827
      WRITE (NOUT,5200) NSGNM1,DDUM,DUM                                     1828
C-----------------------------------------------------------------------    1829
C  START OF MAIN LOOP FOR SEARCHING FOR OPTIMUM SET OF EXTREMA.             1830
C-----------------------------------------------------------------------    1831
  200 ITER=ITER+1                                                           1832
      IF (NDONE .LE. 0) GO TO 230                                           1833
C-----------------------------------------------------------------------    1834
C  CHECK TO SEE IF THESE EXTREMA ARE ALREADY INCLUDED IN A                  1835
C      PREVIOUS SOLUTION.                                                   1836
C-----------------------------------------------------------------------    1837
      DO 210 K=1,NDONE                                                      1838
        DO 220 J=1,NSGNM1                                                   1839
          L=IABS(LLSIGN(J+1))                                               1840
          LL=LSDONE(K,J,1)                                                  1841
          IF ((LL.LE.0 .AND. L.NE.-LL)  .OR.                                1842
     1    (LL.GT.0 .AND. L.LT.LL)) GO TO 210                                1843
          LL=LSDONE(K,J,2)                                                  1844
          IF ((LL.LE.0 .AND. L.NE.-LL)  .OR.                                1845
     1    (LL.GT.0 .AND. L.GT.LL)) GO TO 210                                1846
  220   CONTINUE                                                            1847
        LSTAR=3                                                             1848
        VARREG=VDONE(K)                                                     1849
        GO TO 320                                                           1850
  210 CONTINUE                                                              1851
C-----------------------------------------------------------------------    1852
C  UPDATE SIGNS OF INEQUALITY MATRIX, RIGHT-HAND SIDE, AND                  1853
C      IISIGN(J) (MONOTONICITY INDICATOR FOR J-TH GRID POINT).              1854
C-----------------------------------------------------------------------    1855
  230 CALL UPDSGN (NSGNI,LLSIGN,                                            1856
     1 A,AINEQ,IISIGN,MA,MG,MINEQ,MREG,NGLE,NGLP1,NNINEQ,                   1857
     2 NNQUSR,NONNEG,NOUT,REG,RHSNEQ,S,VALPHA)                              1858
      NQPITR=NQPITR+1                                                       1859
      IF (NQPITR .LE. MQPITR) GO TO 235                                     1860
      CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                                   1861
      GO TO 790                                                             1862
  235 LDUM=ISTAGE .EQ. 1                                                    1863
C-----------------------------------------------------------------------    1864
C  DO CONSTRAINED SOLUTION.                                                 1865
C-----------------------------------------------------------------------    1866
      CALL LDPETC (5,.FALSE.,LNINEQ,LDUM,ICRIT(ISTAGE),.FALSE.,.FALSE.,     1867
     1 .FALSE.,ALPHA,.FALSE.,.FALSE.,DUB,VAR,                               1868
     2  A,AA,AINEQ,BTEST,CQUAD,DEGFRE,DEGFRZ,EXACT,G,IERROR,                1869
     3  ISTAGE,IWORK,LBIND,MA,MG,MINEQ,MREG,MWORK,MY,                       1870
     4  NGLE,NGLY,PREJ,REG,RHSNEQ,RS2MNX,S,SOLBES,                          1871
     5  SOLUTN,SQRTW,SSCALE,T,VALPCV,VALPHA,VARREG,VARZ,WORK,Y,YLYFIT)      1872
      IF (IERROR .EQ. 1) GO TO 240                                          1873
      LSTAR=4                                                               1874
      VARREG=SRANGE                                                         1875
      GO TO 320                                                             1876
  240 NDONE=NDONE+1                                                         1877
      IF (NDONE .LE. MDONE) GO TO 250                                       1878
      CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                                   1879
      GO TO 790                                                             1880
C-----------------------------------------------------------------------    1881
C  UPDATE LSDONE (EXTREMA RANGES COVERED BY PREVIOUS SOLUTIONS),            1882
C      AND IF THIS SOLUTION IS IDENTICAL WITH A PRECEDING ONE,              1883
C      REPLACE VARREG WITH THE PRECEDING ONE.                               1884
C-----------------------------------------------------------------------    1885
  250 CALL UPDDON (                                                         1886
     1 NSGNM1,LLSIGN,LSDONE,MDONE,NDONE,NNQUSR,LBIND,MINEQ,                 1887
     2 NG,VARREG,VDONE)                                                     1888
      LSTAR=1                                                               1889
      IF (VARREG .GE. VARRBS) GO TO 320                                     1890
C-----------------------------------------------------------------------    1891
C  STORE BEST EXTREMA SO FAR IN LLSBES.                                     1892
C-----------------------------------------------------------------------    1893
      LSTAR=2                                                               1894
      VARRBS=VARREG                                                         1895
      L=NSGNI+1                                                             1896
      DO 310 J=1,L                                                          1897
        LLSBES(J)=LLSIGN(J)                                                 1898
  310 CONTINUE                                                              1899
      FLAT=FFLAT (NSGNI,NONNEG,NG,SOLUTN,SRMIN,NNQUSR,LBIND,                1900
     1 MINEQ,LLSIGN)                                                        1901
C-----------------------------------------------------------------------    1902
C  OUTPUT RESULTS OF ITERATION ITER.                                        1903
C-----------------------------------------------------------------------    1904
 5320 FORMAT (1X,A1,I4,1PE16.6,E16.5,E16.3,0PF14.3,2F12.3,8X,5I5)           1905
  320 DUM=SRANGE                                                            1906
      DDUM=FLOAT(NY)-DEGFRE                                                 1907
      IF (LSTAR.LE.2 .AND. DDUM.GT.0.) DUM=SQRT(VAR/DDUM)                   1908
      IF (LSTAR .LE. 2) WRITE (NOUT,5320) ISTAR(LSTAR),ITER,VARREG,VAR,     1909
     1 DUM,DEGFRE,PREJ,(LLSIGN(J),J=1,NSGNI)                                1910
 5322 FORMAT (1X,A1,I4,1PE16.6,78X,5I5)                                     1911
      IF (LSTAR .GE. 3) WRITE (NOUT,5322) ISTAR(LSTAR),ITER,VARREG,         1912
     1 (LLSIGN(J),J=1,NSGNI)                                                1913
C-----------------------------------------------------------------------    1914
C  UPDATE POSITIONS OF EXTREMA.                                             1915
C-----------------------------------------------------------------------    1916
      CALL UPDLLS (NSGNI,JSTAGE,NOUT,VARTRY,VARREG,LLSTRY,LLSIGN,           1917
     1 INC,DONE)                                                            1918
C-----------------------------------------------------------------------    1919
C  IF NOT DONE, THEN START ANOTHER ITERATION.                               1920
C-----------------------------------------------------------------------    1921
      IF (.NOT.DONE) GO TO 200                                              1922
C-----------------------------------------------------------------------    1923
C  REPEAT BEST SOLUTION FOR OUTPUT AT END OF PEAK-CONSTRAINED ANALYSIS.     1924
C-----------------------------------------------------------------------    1925
      CALL UPDSGN (NSGNI,LLSBES,                                            1926
     1 A,AINEQ,IISIGN,MA,MG,MINEQ,MREG,NGLE,NGLP1,NNINEQ,                   1927
     2 NNQUSR,NONNEG,NOUT,REG,RHSNEQ,S,VALPHA)                              1928
      PPLTPR=MAX0(IPRINT(ISTAGE),IUSROU(ISTAGE)) .GE. 1                     1929
      CALL LDPETC (1,.FALSE.,LNINEQ,.FALSE.,ICRIT(ISTAGE),DOMOM,PPLTPR,     1930
     1 .TRUE.,ALPHA,.TRUE.,.FALSE.,DUB,VAR,                                 1931
     2  A,AA,AINEQ,BTEST,CQUAD,DEGFRE,DEGFRZ,EXACT,G,IERROR,                1932
     3  ISTAGE,IWORK,LBIND,MA,MG,MINEQ,MREG,MWORK,MY,                       1933
     4  NGLE,NGLY,PREJ,REG,RHSNEQ,RS2MNX,S,SOLBES,                          1934
     5  SOLUTN,SQRTW,SSCALE,T,VALPCV,VALPHA,VARREG,VARZ,WORK,Y,YLYFIT)      1935
      DUM=ALPHA/S(1,1)                                                      1936
      CALL RUNRES (1,SOLUTN,.FALSE.,DUM,.FALSE.,                            1937
     1 CQUAD,G,IPLFIT,IPLRES,ISTAGE,ITITLE,IUNIT,IWT,LINEPG,MWORK,NG,       1938
     2 NGL,NLINF,NOUT,NY,SQRTW,SRANGE,SSCALE,T,WORK,Y,YLYFIT)               1939
      GO TO 795                                                             1940
  790 FLAT=.FALSE.                                                          1941
C-----------------------------------------------------------------------    1942
C  RESTORE INEQUALITY CONSTRAINTS TO ORIGINAL MONOTONICALLY                 1943
C      DECREASING CONDITION.                                                1944
C-----------------------------------------------------------------------    1945
  795 LLSTRY(1,1)=1                                                         1946
      LLSTRY(2,1)=NG                                                        1947
      CALL UPDSGN (1,LLSTRY,                                                1948
     1 A,AINEQ,IISIGN,MA,MG,MINEQ,MREG,NGLE,NGLP1,NNINEQ,                   1949
     2 NNQUSR,NONNEG,NOUT,REG,RHSNEQ,S,VALPHA)                              1950
      RETURN                                                                1951
      END                                                                   1952
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    1953
C  FUNCTION BETAIN.  APPROXIMATES THE INCOMPLETE BETA FUNCTION RATIO        1954
C      I(SUB X)(A,B) USING ABRAMOWITZ AND STEGUN (26.5.5).                  1955
C  GOOD TO ABOUT 1 PART IN TOL (TOL SET IN DATA STATEMENT BELOW).           1956
C  FOR VERY LARGE (.GT. 1.E+4) A OR B, OF THE ORDER OF MAX(A,B) TERMS       1957
C      ARE NEEDED, AND AN ASYMPTOTIC FORMULA WOULD BE BETTER.               1958
C      TAKES AN ERROR EXIT IF A OR B .GE. 2.E+4.                            1959
C-----------------------------------------------------------------------    1960
C  CALLS SUBPROGRAMS - ERRMES, GAMLN                                        1961
C-----------------------------------------------------------------------    1962
      FUNCTION BETAIN (X,A,B,NOUT)                                          1963
      LOGICAL SWAP                                                          1964
      DIMENSION IHOLER(6)                                                   1965
      DATA IHOLER/1HB, 1HE, 1HT, 1HA, 1HI, 1HN/, TOL/1.E-5/                 1966
      IF (X.LT.0. .OR. X.GT.1. .OR. AMIN1(A,B).LE.0. .OR.                   1967
     1 AMAX1(A,B).GE.2.E+4) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)              1968
      BETAIN=X                                                              1969
      IF (X.LE.0. .OR. X.GE.1.) RETURN                                      1970
      SWAP=X .GT. .5                                                        1971
      IF (SWAP) GO TO 150                                                   1972
      XX=X                                                                  1973
      AA=A                                                                  1974
      BB=B                                                                  1975
      GO TO 200                                                             1976
C-----------------------------------------------------------------------    1977
C      WHEN SWAP=.TRUE., I(SUB 1-X)(B,A)=1-I(SUB X)(A,B) IS EVALUATED       1978
C      FIRST.                                                               1979
C-----------------------------------------------------------------------    1980
  150 XX=1.-X                                                               1981
      AA=B                                                                  1982
      BB=A                                                                  1983
  200 CX=1.-XX                                                              1984
      R=XX/CX                                                               1985
C-----------------------------------------------------------------------    1986
C  TERM IMAX IS APPROXIMATELY THE MAXIMUM TERM IN THE SUM.                  1987
C-----------------------------------------------------------------------    1988
      IMAX=MAX0(0,INT((R*BB-AA-1.)/(R+1.)))                                 1989
      RI=FLOAT(IMAX)                                                        1990
      SUM=0.                                                                1991
      TERMAX=(AA+RI)*ALOG(XX)+(BB-RI-1.)*ALOG(CX)+GAMLN(AA+BB)-             1992
     1 GAMLN(AA+RI+1.)-GAMLN(BB-RI)                                         1993
      IF (TERMAX .LT. -50.) GO TO 700                                       1994
      TERMAX=EXP(TERMAX)                                                    1995
      TERM=TERMAX                                                           1996
      SUM=TERM                                                              1997
C-----------------------------------------------------------------------    1998
C  SUM TERMS FOR I=IMAX+1,IMAX+2,... UNTIL CONVERGENCE.                     1999
C-----------------------------------------------------------------------    2000
      I1=IMAX+1                                                             2001
      DO 250 I=I1,20000                                                     2002
        RI=FLOAT(I)                                                         2003
        TERM=TERM*R*(BB-RI)/(AA+RI)                                         2004
        SUM=SUM+TERM                                                        2005
        IF (ABS(TERM) .LE. TOL*SUM) GO TO 300                               2006
  250 CONTINUE                                                              2007
      CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                                    2008
  300 IF (IMAX .EQ. 0) GO TO 700                                            2009
C-----------------------------------------------------------------------    2010
C  SUM TERMS FOR I=IMAX-1,IMAX-2,... UNTIL CONVERGENCE.                     2011
C-----------------------------------------------------------------------    2012
      TERM=TERMAX                                                           2013
      RI=FLOAT(IMAX)                                                        2014
      DO 320 I=1,IMAX                                                       2015
        TERM=TERM*(AA+RI)/(R*(BB-RI))                                       2016
        SUM=SUM+TERM                                                        2017
        IF (ABS(TERM) .LE. TOL*SUM) GO TO 700                               2018
        RI=RI-1.                                                            2019
  320 CONTINUE                                                              2020
  700 BETAIN=SUM                                                            2021
      IF (SWAP) BETAIN=1.-BETAIN                                            2022
      RETURN                                                                2023
      END                                                                   2024
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2025
C  SUBROUTINE CQTRAP.  GIVEN QUADRATURE GRID G, STORES WEIGHTS FOR          2026
C      TRAPEZOIDAL RULE IN CQUAD(J), J=1,NG.                                2027
      SUBROUTINE CQTRAP (G,CQUAD,NG)                                        2028
      DIMENSION G(NG), CQUAD(NG)                                            2029
      JJ=NG-1                                                               2030
      DELOLD=0.                                                             2031
      DO 110 J=1,JJ                                                         2032
        DEL=.5*(G(J+1)-G(J))                                                2033
        CQUAD(J)=DEL+DELOLD                                                 2034
        DELOLD=DEL                                                          2035
  110 CONTINUE                                                              2036
      CQUAD(NG)=DELOLD                                                      2037
      RETURN                                                                2038
      END                                                                   2039
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2040
C  SUBROUTINE CVNEQ.  ELIMINATES ANY BINDING INEQUALITY CONSTRAINTS         2041
C      AS EQUALITY CONSTRAINTS, AND PUTS DEGREES OF FREEDOM IN DEGFRE.      2042
C      IF PPLTPR=.TRUE., THEN PUTS (SQUARE ROOT OF THE COVARIANCE MATRIX    2043
C      OF THE SCALED SOLUTION)/(STDDEV) IN AA(J,K), J=1,NGL, K=1,NGLY.      2044
C  ON RETURN, IERROR = 1 FOR NORMAL RETURN,                                 2045
C                      5 FOR ERROR RETURN FROM SVDRS2.                      2046
C-----------------------------------------------------------------------    2047
C  CALLS SUBPROGRAMS - ERRMES, ELIMEQ, SVDRS2, H12                          2048
C  WHICH IN TURN CALL - LH1405, QRBD, G1, G2, DIFF                          2049
C-----------------------------------------------------------------------    2050
      SUBROUTINE CVNEQ (ALPHA,IERROR,NNNNEQ,SOLUTN,                         2051
     1 A,AA,AINEQ,DEGFRE,LBIND,MA,MG,MINEQ,MREG,MWORK,NGL,NGLE,             2052
     2 NGLP1,NGLY,NOUT,PPLTPR,RANGE,REG,S,VALPCV,WORK)                      2053
      DOUBLE PRECISION A, AA, ABS, AINEQ, ALPHA, DUB, RANGE, REG, S,        2054
     1 SOLUTN, VALPCV, WORK, ZERO                                           2055
      LOGICAL LBIND, PPLTPR, FAIL                                           2056
      DIMENSION AA(MG,MG), WORK(MWORK), A(MA,MG), S(MG,3), SOLUTN(MG),      2057
     1 AINEQ(MINEQ,MG), VALPCV(MG), REG(MREG,MG), LBIND(MINEQ)              2058
      DIMENSION IHOLER(6)                                                   2059
      DATA IHOLER/1HC, 1HV, 1HN, 1HE, 1HQ, 1H /                             2060
      ABS(DUB)=DABS(DUB)                                                    2061
C     ZERO=0.E0!SP                                                          2062
      ZERO=0.D0                                                             2063
      FAIL=.FALSE.                                                          2064
      DEGFRE=0.                                                             2065
      IERROR=1                                                              2066
      NGLEP1=NGLE+1                                                         2067
      IY=NGLEP1*(NNNNEQ+2)                                                  2068
      IW=-NNNNEQ                                                            2069
      NBIND=0                                                               2070
      IF (NNNNEQ .EQ. 0) GO TO 180                                          2071
      DO 150 IROW=1,NNNNEQ                                                  2072
        IY=IY+1                                                             2073
        LBIND(IROW)=WORK(IY) .GT. ZERO                                      2074
        IF (.NOT.LBIND(IROW)) GO TO 150                                     2075
C-----------------------------------------------------------------------    2076
C  IF BINDING INEQUALITY CONSTRAINT ONLY INVOLVES ONE SOLUTION              2077
C      POINT, THEN SET THAT SOLUTION POINT TO THE R.H.S. OF INEQUALITY.     2078
C-----------------------------------------------------------------------    2079
        L=0                                                                 2080
        DO 152 ICOL=1,NGL                                                   2081
          IF (ABS(AINEQ(IROW,ICOL)) .LE. ZERO) GO TO 152                    2082
          IF (L .NE. 0) GO TO 154                                           2083
          L=ICOL                                                            2084
  152   CONTINUE                                                            2085
        SOLUTN(L)=AINEQ(IROW,NGLP1)/AINEQ(IROW,L)                           2086
C-----------------------------------------------------------------------    2087
C  PUT AUGMENTED MATRIX OF BINDING INEQUALITIES STARTING IN WORK(1)         2088
C      WITH COLUMN INCREMENTS OF NNNNEQ.                                    2089
C  LHS = D*K2*Z*H1**(-1)*W,  RHS = D(LOWER CASE)-D*VALPCV, WHERE            2090
C      VALPCV = K1*X1+K2*Z*H1**(-1)*R1.                                     2091
C-----------------------------------------------------------------------    2092
  154   NBIND=NBIND+1                                                       2093
        IW=IW+1                                                             2094
        IIW=IW+NNNNEQ                                                       2095
        DO 160 ICOL=1,NGLE                                                  2096
          WORK(IIW)=A(IROW,ICOL)/S(ICOL,2)                                  2097
          IIW=IIW+NNNNEQ                                                    2098
  160   CONTINUE                                                            2099
        DUB=AINEQ(IROW,NGLP1)                                               2100
        DO 170 J=1,NGL                                                      2101
          DUB=DUB-AINEQ(IROW,J)*VALPCV(J)                                   2102
  170   CONTINUE                                                            2103
        WORK(IIW)=DUB                                                       2104
  150 CONTINUE                                                              2105
      IF (NBIND .LT. NGLE) GO TO 180                                        2106
      CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                                   2107
 5180 FORMAT (1X,2I4)                                                       2108
      WRITE (NOUT,5180) NBIND,NGLE                                          2109
      STOP                                                                  2110
C-----------------------------------------------------------------------    2111
C  PUT H IN AA.                                                             2112
C-----------------------------------------------------------------------    2113
  180 DO 190 J=1,NGLY                                                       2114
        DO 195 K=1,NGLE                                                     2115
          AA(J,K)=ZERO                                                      2116
  195   CONTINUE                                                            2117
        AA(J,J)=S(J,1)                                                      2118
        AA(J,NGLEP1)=A(J,NGLP1)-S(J,1)*REG(J,NGLP1)                         2119
  190 CONTINUE                                                              2120
C-----------------------------------------------------------------------    2121
C  PUT NEW H*K IN AA.                                                       2122
C-----------------------------------------------------------------------    2123
      NGLEE=NGLE-NBIND                                                      2124
      IF (NBIND .EQ. 0) GO TO 250                                           2125
      IW=NGLP1*NNNNEQ+1                                                     2126
      IIW=IW+NGL                                                            2127
      CALL ELIMEQ (WORK,NNNNEQ,MG,WORK(IW),NBIND,NGLE,AA,MG,AA,MG,          2128
     1 0,NGLEP1,NGLY,WORK(IIW),RANGE)                                       2129
C-----------------------------------------------------------------------    2130
C  SAVE S AND THEN DO SVD OF NEW H*K2.  NEW W WILL GO IN COLUMNS            2131
C      (NBIND+1) TO NGLE OF AA.                                             2132
C-----------------------------------------------------------------------    2133
      DO 191 J=1,NGLE                                                       2134
        AA(J,MG-1)=S(J,1)                                                   2135
        AA(J,MG)=S(J,2)                                                     2136
  191 CONTINUE                                                              2137
      J=NBIND+1                                                             2138
      CALL SVDRS2 (AA(1,J),MG,NGLY,NGLEE,SOLUTN,                            2139
     1 MG,0,S,IERROR,RANGE)                                                 2140
      IF (IERROR .EQ. 1) GO TO 250                                          2141
      CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                                   2142
      FAIL=.TRUE.                                                           2143
      GO TO 340                                                             2144
  250 DUM=ALPHA**2                                                          2145
      NGLEEY=MIN0(NGLY,NGLEE)                                               2146
      DO 260 J=1,NGLEEY                                                     2147
        S(J,3)=S(J,1)/(DUM+S(J,1)**2)                                       2148
        DEGFRE=DEGFRE+S(J,1)*S(J,3)                                         2149
  260 CONTINUE                                                              2150
  280 IF (NBIND .GT. 0) GO TO 340                                           2151
C-----------------------------------------------------------------------    2152
C  IF NBIND=0, PUT (SQUARE ROOT OF THE COVARIANCE MATRIX OF THE             2153
C      SCALED SOLUTION)/(STDDEV) IN AA.  IN THIS CASE, THE NEW K2*W         2154
C      IS AN IDENTITY MATRIX.                                               2155
C-----------------------------------------------------------------------    2156
  310 DO 320 ICOL=1,NGLY                                                    2157
        IF (ICOL .GT. NGLEEY) GO TO 335                                     2158
        DUB=S(ICOL,3)                                                       2159
        DO 330 IROW=1,NGL                                                   2160
          AA(IROW,ICOL)=DUB*REG(IROW,ICOL)                                  2161
  330   CONTINUE                                                            2162
        GO TO 320                                                           2163
  335   DO 337 IROW=1,NGL                                                   2164
          AA(IROW,ICOL)=ZERO                                                2165
  337   CONTINUE                                                            2166
  320 CONTINUE                                                              2167
      GO TO 800                                                             2168
C-----------------------------------------------------------------------    2169
C  RESTORE S.                                                               2170
C-----------------------------------------------------------------------    2171
  340 DO 345 J=1,NGLE                                                       2172
        S(J,1)=AA(J,MG-1)                                                   2173
        S(J,2)=AA(J,MG)                                                     2174
  345 CONTINUE                                                              2175
      IF (.NOT.PPLTPR .OR. FAIL) GO TO 800                                  2176
C-----------------------------------------------------------------------    2177
C  MOVE NEW W DOWN NBIND ROWS IN AA AND ZERO FIRST NBIND ROWS OF AA FOR     2178
C      CONVENIENT APPLICATION OF H12 FOR CALCULATION OF NEW K2*W.           2179
C-----------------------------------------------------------------------    2180
  350 ICOL=NBIND                                                            2181
      DO 360 J=1,NGLEE                                                      2182
        ICOL=ICOL+1                                                         2183
        IROW=NGLE+1                                                         2184
        DO 370 I=1,NGLEE                                                    2185
          IROW=IROW-1                                                       2186
          IIROW=IROW-NBIND                                                  2187
          AA(IROW,ICOL)=AA(IIROW,ICOL)                                      2188
  370   CONTINUE                                                            2189
        DO 380 IROW=1,NBIND                                                 2190
          AA(IROW,ICOL)=ZERO                                                2191
  380   CONTINUE                                                            2192
  360 CONTINUE                                                              2193
C-----------------------------------------------------------------------    2194
C  MULTIPLY W (IN AA) BY K2 (IN WORK).  NEW K2*W IS THEN IN AA(J,K),        2195
C      J=1,NGLE, K=NBIND+1,NGLE.                                            2196
C-----------------------------------------------------------------------    2197
      I=NBIND                                                               2198
      IIW=IW+NBIND-1                                                        2199
      DO 390 J=1,NBIND                                                      2200
        CALL H12 (2,I,I+1,NGLE,WORK(I),NNNNEQ,WORK(IIW),AA(1,NBIND+1),      2201
     1  1,MG,NGLEE,RANGE)                                                   2202
        I=I-1                                                               2203
        IIW=IIW-1                                                           2204
  390 CONTINUE                                                              2205
C-----------------------------------------------------------------------    2206
C  PUT (SQUARE ROOT OF THE COVARIANCE MATRIX OF THE SCALED SOLUTION)/       2207
C      (STDDEV) IN AA(J,K), J=1,NGL, K=1,NGLY.                              2208
C-----------------------------------------------------------------------    2209
      DO 410 ICOL=1,NGLY                                                    2210
        IICOL=ICOL+NBIND                                                    2211
        DO 420 IROW=1,NGL                                                   2212
          AA(IROW,ICOL)=ZERO                                                2213
          IF (ICOL .GT. NGLEEY) GO TO 420                                   2214
          DUB=ZERO                                                          2215
          DO 430 J=1,NGLE                                                   2216
            DUB=DUB+REG(IROW,J)*AA(J,IICOL)                                 2217
  430     CONTINUE                                                          2218
          AA(IROW,ICOL)=DUB*S(ICOL,3)                                       2219
  420   CONTINUE                                                            2220
  410 CONTINUE                                                              2221
  800 RETURN                                                                2222
      END                                                                   2223
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2224
C  SUBROUTINE DIAGA. PUTS W**(T)*R1 INTO REG(J,NGLP1), J=1,NGLE.            2225
C      PUTS K2*Z*H1**(-1)*W INTO FIRST NGLE ROWS AND COLUMNS OF REG.        2226
C  IF NGLE.LT.NGLY, THEN REG(J,NGLP1) AND S(J,1), J=NGLE+1,NGLY ARE         2227
C      ZEROED.  IF NGLE.GT.NGLY, THEN A(J,NGLP1) AND S(J,1),                2228
C      J=NGLY+1,NGLE ARE ZEROED.                                            2229
      SUBROUTINE DIAGA (A,MA,MG,MREG,NEQ,NGL,NGLE,NGLP1,NGLY,REG,S)         2230
      DOUBLE PRECISION A, DUM, REG, S, ZERO                                 2231
      DIMENSION A(MA,MG), REG(MREG,MG), S(MG,3)                             2232
C     ZERO=0.E0!SP                                                          2233
      ZERO=0.D0                                                             2234
      IF (NGLY .GE. NGLE) GO TO 150                                         2235
      K=NGLY+1                                                              2236
      DO 140 J=K,NGLE                                                       2237
        S(J,1)=ZERO                                                         2238
        A(J,NGLP1)=ZERO                                                     2239
  140 CONTINUE                                                              2240
  150 DO 160 IROW=1,NGL                                                     2241
        DO 170 ICOL=1,NGLE                                                  2242
          IICOL=NEQ                                                         2243
          DUM=ZERO                                                          2244
          DO 180 J=1,NGLE                                                   2245
            IICOL=IICOL+1                                                   2246
            DUM=DUM+REG(IROW,IICOL)*A(J,ICOL)                               2247
  180     CONTINUE                                                          2248
          S(ICOL,2)=DUM                                                     2249
  170   CONTINUE                                                            2250
        DO 190 ICOL=1,NGLE                                                  2251
          REG(IROW,ICOL)=S(ICOL,2)                                          2252
  190   CONTINUE                                                            2253
  160 CONTINUE                                                              2254
      DO 210 IROW=1,NGLE                                                    2255
        DUM=ZERO                                                            2256
        DO 220 ICOL=1,NGLE                                                  2257
          DUM=DUM+A(ICOL,IROW)*REG(ICOL,NGLP1)                              2258
  220   CONTINUE                                                            2259
        S(IROW,2)=DUM                                                       2260
  210 CONTINUE                                                              2261
      DO 250 IROW=1,NGLE                                                    2262
        REG(IROW,NGLP1)=S(IROW,2)                                           2263
  250 CONTINUE                                                              2264
      IF (NGLE .GE. NGLY) GO TO 800                                         2265
      J=NGLE+1                                                              2266
      DO 260 IROW=J,NGLY                                                    2267
        S(IROW,1)=ZERO                                                      2268
        REG(IROW,NGLP1)=ZERO                                                2269
  260 CONTINUE                                                              2270
  800 RETURN                                                                2271
      END                                                                   2272
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2273
C  SUBROUTINE DIAREG.  CONTINUE DIAGONALIZATION OF REGULARIZOR BY           2274
C      PUTTING C*K2*Z*H1**(-1) IN A AND K2*Z*H1**(-1) IN COLUMNS            2275
C      NEQ+1,...,NGL OF REG.                                                2276
C  IF NECESSARY, MAKE REGULARIZOR FULL RANK BY MAKING ALL SINGULAR          2277
C      VALUES AT LEAST SQRT(.1*PRECIS)*(LARGEST SINGULAR VALUE).            2278
C  (IF NEQ=0, THEN K2 IS THE NGL-BY-NGL IDENTITY MATRIX.)                   2279
C-----------------------------------------------------------------------    2280
C  CALLS SUBPROGRAMS - H12                                                  2281
C-----------------------------------------------------------------------    2282
      SUBROUTINE DIAREG (A,AEQ,MA,MEQ,MG,MREG,NEQ,NGL,NGLE,NGLY,            2283
     1 NOUT,NUNREG,PIVOT,PRECIS,RANGE,REG,S)                                2284
      DOUBLE PRECISION A, AEQ, DUM, ONE, PIVOT, PRECIS, RANGE, REG,         2285
     1 S, SMALL, SQRT, ZERO                                                 2286
      DIMENSION S(MG,3), REG(MREG,MG), A(MA,MG), PIVOT(MEQ),                2287
     1 AEQ(MEQ,MG)                                                          2288
      SQRT(ONE)=DSQRT(ONE)                                                  2289
C     ZERO=0.E0!SP                                                          2290
      ZERO=0.D0                                                             2291
C     ONE=1.E0!SP                                                           2292
      ONE=1.D0                                                              2293
      SMALL=SQRT(.1*PRECIS)*S(1,1)                                          2294
      NUNREG=0                                                              2295
      ICOL=NEQ                                                              2296
      DO 120 J=1,NGLE                                                       2297
        ICOL=ICOL+1                                                         2298
        IF (S(J,1) .GT. SMALL) GO TO 125                                    2299
        NUNREG=NUNREG+1                                                     2300
        S(J,1)=SMALL                                                        2301
  125   DUM=ONE/S(J,1)                                                      2302
C-----------------------------------------------------------------------    2303
C  MOVE MATRIX Z*H1**(-1) DOWN NEQ ROWS AND ZERO FIRST NEQ ROWS FOR         2304
C      CONVENIENT APPLICATION OF H12 FOR CALCULATING K2*Z*H1**(-1).         2305
C-----------------------------------------------------------------------    2306
        IROW=NGL+1                                                          2307
        DO 130 I=1,NGLE                                                     2308
          IROW=IROW-1                                                       2309
          IIROW=IROW-NEQ                                                    2310
          REG(IROW,ICOL)=REG(IIROW,ICOL)*DUM                                2311
  130   CONTINUE                                                            2312
        IF (NEQ .EQ. 0) GO TO 120                                           2313
        DO 135 I=1,NEQ                                                      2314
          REG(I,ICOL)=ZERO                                                  2315
  135   CONTINUE                                                            2316
  120 CONTINUE                                                              2317
      WRITE (NOUT,5120) NUNREG                                              2318
 5120 FORMAT (/1X,I3,24H UNREGULARIZED VARIABLES)                           2319
C-----------------------------------------------------------------------    2320
C  PUT C*K2*Z*H1**(-1) IN A.                                                2321
C-----------------------------------------------------------------------    2322
      DO 150 IROW=1,NGLY                                                    2323
        ICOL=NEQ                                                            2324
        DO 155 J=1,NGLE                                                     2325
          ICOL=ICOL+1                                                       2326
          DUM=ZERO                                                          2327
          L=NEQ                                                             2328
          DO 160 K=1,NGLE                                                   2329
            L=L+1                                                           2330
            DUM=DUM+A(IROW,L)*REG(L,ICOL)                                   2331
  160     CONTINUE                                                          2332
          S(J,1)=DUM                                                        2333
  155   CONTINUE                                                            2334
        DO 165 J=1,NGLE                                                     2335
          A(IROW,J)=S(J,1)                                                  2336
  165   CONTINUE                                                            2337
  150 CONTINUE                                                              2338
      IF (NEQ .EQ. 0) GO TO 800                                             2339
C-----------------------------------------------------------------------    2340
C  PUT K2*Z*H1**(-1) IN COLUMNS NEQ+1,...,NGL OF REG.  (IF NEQ=0, THEN      2341
C      K2 IS THE IDENTITY MATRIX AND NOTHING IS DONE.)                      2342
C-----------------------------------------------------------------------    2343
      I=NEQ                                                                 2344
      DO 210 J=1,NEQ                                                        2345
        CALL H12 (2,I,I+1,NGL,AEQ(I,1),MEQ,PIVOT(I),REG(1,NEQ+1),           2346
     1  1,MREG,NGLE,RANGE)                                                  2347
        I=I-1                                                               2348
  210 CONTINUE                                                              2349
  800 RETURN                                                                2350
      END                                                                   2351
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2352
C  FUNCTION DIFF.                                                           2353
C     BASED ON C.L.LAWSON AND R.J.HANSON,                                   2354
C     'SOLVING LEAST SQUARES PROBLEMS', PRENTICE-HALL, 1974                 2355
C     FUNCTION DIFF(X,Y)!SP                                                 2356
      DOUBLE PRECISION FUNCTION DIFF(X,Y)                                   2357
      DOUBLE PRECISION X, Y                                                 2358
      DIFF=X-Y                                                              2359
      RETURN                                                                2360
      END                                                                   2361
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2362
C  SUBROUTINE ELIMEQ.  ELIMINATES EQUALITY CONSTRAINTS USING A BASIS        2363
C      OF THE NULL SPACE OF E (IN AEQ).  (SEE LAWSON AND HANSON,            2364
C      CHAP. 20.)                                                           2365
C-----------------------------------------------------------------------    2366
C  CALLS SUBPROGRAMS - H12, LH1405                                          2367
C-----------------------------------------------------------------------    2368
      SUBROUTINE ELIMEQ (AEQ,MEQ,MG,PIVOT,NEQ,NGL,A,MA,REG,MREG,            2369
     1 NREG,NGLP1,NGLY,VK1Y1,RANGE)                                         2370
      DOUBLE PRECISION A, AEQ, DUM, PIVOT, RANGE, REG, VK1Y1, ZERO          2371
      DIMENSION AEQ(MEQ,MG), PIVOT(1), A(MA,MG), REG(MREG,MG),              2372
     1 VK1Y1(1)                                                             2373
C     ZERO=0.E0!SP                                                          2374
      ZERO=0.D0                                                             2375
      DO 120 J=1,NGL                                                        2376
        VK1Y1(J)=ZERO                                                       2377
  120 CONTINUE                                                              2378
      IF (NEQ .EQ. 0) RETURN                                                2379
      DO 150 I=1,NEQ                                                        2380
        II=I                                                                2381
        IAEQ=MIN0(I+1,NEQ)                                                  2382
        CALL H12 (1,II,I+1,NGL,AEQ(I,1),MEQ,PIVOT(I),AEQ(IAEQ,1),           2383
     1  MEQ,1,NEQ-I,RANGE)                                                  2384
        CALL H12 (2,II,I+1,NGL,AEQ(I,1),MEQ,PIVOT(I),A(1,1),                2385
     1  MA,1,NGLY,RANGE)                                                    2386
        IF (NREG .GT. 0) CALL H12 (2,II,I+1,NGL,AEQ(I,1),MEQ,               2387
     1  PIVOT(I),REG(1,1),MREG,1,NREG,RANGE)                                2388
  150 CONTINUE                                                              2389
      VK1Y1(1)=AEQ(1,NGLP1)/AEQ(1,1)                                        2390
      IF (NEQ .EQ. 1) GO TO 200                                             2391
      DO 170 I=2,NEQ                                                        2392
        DUM=ZERO                                                            2393
        K=I-1                                                               2394
        DO 180 J=1,K                                                        2395
          DUM=DUM+AEQ(I,J)*VK1Y1(J)                                         2396
  180   CONTINUE                                                            2397
        VK1Y1(I)=(AEQ(I,NGLP1)-DUM)/AEQ(I,I)                                2398
  170 CONTINUE                                                              2399
  200 CALL LH1405 (A(1,NGLP1),NGLY,NEQ,A,MA,VK1Y1)                          2400
      IF (NREG .GT. 0) CALL LH1405 (REG(1,NGLP1),NREG,NEQ,REG,MREG,         2401
     1 VK1Y1)                                                               2402
      I=NEQ                                                                 2403
      DO 230 J=1,NEQ                                                        2404
        CALL H12 (2,I,I+1,NGL,AEQ(I,1),MEQ,PIVOT(I),VK1Y1,1,MG,1,RANGE)     2405
        I=I-1                                                               2406
  230 CONTINUE                                                              2407
      RETURN                                                                2408
      END                                                                   2409
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2410
C  SUBROUTINE ERRMES.  PRINTS ERROR MESSAGE AND STOPS IF ABORT=.TRUE..      2411
      SUBROUTINE ERRMES (N,ABORT,IHOLER,IWRITE)                             2412
      LOGICAL ABORT                                                         2413
      DIMENSION IHOLER(6)                                                   2414
 5000 FORMAT (7H0ERROR ,6A1,I2,25H.  (CHECK USERS GUIDE.)  ,                2415
     1 46(2H**))                                                            2416
      WRITE (IWRITE,5000) IHOLER,N                                          2417
      IF (ABORT) STOP                                                       2418
      RETURN                                                                2419
      END                                                                   2420
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2421
C  FUNCTION FFLAT.  FFLAT=.TRUE. IF FLAT SPOTS (BINDING INEQUALITY          2422
C      CONSTRAINTS) ARE FOUND IN SOLUTION SOLUTN.  A FLAT SPOT AT AN        2423
C      EXTREMUM IS ONLY COUNTED IF THERE ARE TWO OR MORE SUCCESSIVE         2424
C      BINDING CONSTRAINTS THERE.  IF NONNEG=.TRUE., THEN A FLAT SPOT       2425
C      IS ONLY COUNTED IF ITS MAGNITUDE EXCEEDS SRMIN*(MAX. OF SOLUTN).     2426
      LOGICAL FUNCTION FFLAT (NSGNI,NONNEG,NG,SOLUTN,SRMIN,NNQUSR,          2427
     1 LBIND,MINEQ,LLSIGN)                                                  2428
      DOUBLE PRECISION ABS, AMAX1, SMIN, SOLUTN, ZERO                       2429
      LOGICAL NONNEG, LBIND                                                 2430
      DIMENSION SOLUTN(NG), LBIND(MINEQ), LLSIGN(NSGNI)                     2431
      AMAX1(SMIN,ZERO)=DMAX1(SMIN,ZERO)                                     2432
      ABS(SMIN)=DABS(SMIN)                                                  2433
C     ZERO=0.E0!SP                                                          2434
      ZERO=0.D0                                                             2435
      SMIN=ZERO                                                             2436
      IF (.NOT.NONNEG) GO TO 120                                            2437
      DO 110 J=1,NG                                                         2438
        SMIN=AMAX1(SMIN,SOLUTN(J))                                          2439
  110 CONTINUE                                                              2440
      SMIN=SMIN*SRMIN                                                       2441
  120 FFLAT=.FALSE.                                                         2442
      K=NNQUSR                                                              2443
      NGM1=NG-1                                                             2444
      DO 130 J=1,NGM1                                                       2445
        K=K+1                                                               2446
        IF (LBIND(K) .AND. ABS(SOLUTN(J)) .GE. SMIN) GO TO 140              2447
        FFLAT=.FALSE.                                                       2448
        GO TO 130                                                           2449
  140   IF (FFLAT) GO TO 800                                                2450
        FFLAT=.TRUE.                                                        2451
        IF (NSGNI .LT. 2) GO TO 140                                         2452
        DO 145 L=2,NSGNI                                                    2453
          LL=IABS(LLSIGN(L))                                                2454
          IF (J.EQ.LL .OR. J.EQ.LL+1) GO TO 130                             2455
  145   CONTINUE                                                            2456
        GO TO 140                                                           2457
  130 CONTINUE                                                              2458
      FFLAT=.FALSE.                                                         2459
  800 RETURN                                                                2460
      END                                                                   2461
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2462
C  FUNCTION FISHNI.  CALCULATES FISHER F-DISTRIBUTION FOR ARGUMENT F        2463
C      WITH DF1 AND DF2 (NOT NECESSARILY INTEGER) DEGREES OF FREEDOM.       2464
C      (SEE ABRAMOWITZ AND STEGUN, EQS. 26.6.2 AND 26.5.2.)                 2465
C-----------------------------------------------------------------------    2466
C  CALLS SUBPROGRAMS - ERRMES, BETAIN                                       2467
C  WHICH IN TURN CALL - GAMLN                                               2468
C-----------------------------------------------------------------------    2469
      FUNCTION FISHNI (F,DF1,DF2,NOUT)                                      2470
      DIMENSION IHOLER(6)                                                   2471
      DATA IHOLER/1HF, 1HI, 1HS, 1HH, 1HN, 1HI/                             2472
      IF (AMIN1(DF1,DF2) .GT. 0.) GO TO 150                                 2473
      CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                                   2474
      FISHNI=1.                                                             2475
      RETURN                                                                2476
  150 HDF1=.5*DF1                                                           2477
      HDF2=.5*DF2                                                           2478
      DUM=DF1*F                                                             2479
      FISHNI=BETAIN(DUM/(DF2+DUM),HDF1,HDF2,NOUT)                           2480
      RETURN                                                                2481
      END                                                                   2482
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2483
C  SUBROUTINE G1.                                                           2484
      SUBROUTINE G1 (A,B,COS,SIN,SIG)                                       2485
C     BASED ON C.L.LAWSON AND R.J.HANSON,                                   2486
C     'SOLVING LEAST SQUARES PROBLEMS', PRENTICE-HALL, 1974                 2487
C                                                                           2488
C                                                                           2489
C     COMPUTE ORTHOGONAL ROTATION MATRIX..                                  2490
C     COMPUTE.. MATRIX   (C, S) SO THAT (C, S)(A) = (SQRT(A**2+B**2))       2491
C                        (-S,C)         (-S,C)(B)   (   0          )        2492
C     COMPUTE SIG = SQRT(A**2+B**2)                                         2493
C        SIG IS COMPUTED LAST TO ALLOW FOR THE POSSIBILITY THAT             2494
C        SIG MAY BE IN THE SAME LOCATION AS A OR B .                        2495
C                                                                           2496
      DOUBLE PRECISION A, ABS, B, COS, ONE, SIG, SIGN, SIN, SQRT,           2497
     1 XR, YR, ZERO                                                         2498
      ABS(A)=DABS(A)                                                        2499
      SQRT(A)=DSQRT(A)                                                      2500
      SIGN(A,B)=DSIGN(A,B)                                                  2501
C     ZERO=0.E0!SP                                                          2502
      ZERO=0.D0                                                             2503
C     ONE=1.E0!SP                                                           2504
      ONE=1.D0                                                              2505
      IF (ABS(A).LE.ABS(B)) GO TO 10                                        2506
      XR=B/A                                                                2507
      YR=SQRT(ONE+XR**2)                                                    2508
      COS=SIGN(ONE/YR,A)                                                    2509
      SIN=COS*XR                                                            2510
      SIG=ABS(A)*YR                                                         2511
      RETURN                                                                2512
   10 IF (B) 20,30,20                                                       2513
   20 XR=A/B                                                                2514
      YR=SQRT(ONE+XR**2)                                                    2515
      SIN=SIGN(ONE/YR,B)                                                    2516
      COS=SIN*XR                                                            2517
      SIG=ABS(B)*YR                                                         2518
      RETURN                                                                2519
   30 SIG=ZERO                                                              2520
      COS=ZERO                                                              2521
      SIN=ONE                                                               2522
      RETURN                                                                2523
      END                                                                   2524
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2525
C  SUBROUTINE G2.                                                           2526
      SUBROUTINE G2    (COS,SIN,X,Y)                                        2527
C     BASED ON C.L.LAWSON AND R.J.HANSON,                                   2528
C     'SOLVING LEAST SQUARES PROBLEMS', PRENTICE-HALL, 1974                 2529
C          APPLY THE ROTATION COMPUTED BY G1 TO (X,Y).                      2530
      DOUBLE PRECISION COS, SIN, X, XR, Y                                   2531
      XR=COS*X+SIN*Y                                                        2532
      Y=-SIN*X+COS*Y                                                        2533
      X=XR                                                                  2534
      RETURN                                                                2535
      END                                                                   2536
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2537
C  FUNCTION GAMLN.  COMPUTES LN OF THE GAMMA FUNCTION FOR POSITIVE          2538
C      XARG USING A VERSION OF CACM ALGORITHM NO. 291.                      2539
      FUNCTION GAMLN (XARG)                                                 2540
      X=XARG                                                                2541
      P=1.                                                                  2542
      GAMLN=0.                                                              2543
  110 IF (X .GE. 7.) GO TO 150                                              2544
      P=P*X                                                                 2545
      X=X+1.                                                                2546
      GO TO 110                                                             2547
  150 IF (XARG .LT. 7.) GAMLN=-ALOG(P)                                      2548
      Z=1./X**2                                                             2549
      GAMLN=GAMLN+(X-.5)*ALOG(X)-X+.9189385-(((Z/1680.-                     2550
     1 7.936508E-4)*Z+2.777778E-3)*Z-8.333333E-2)/X                         2551
      RETURN                                                                2552
      END                                                                   2553
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2554
C  SUBROUTINE GETPRU.  DOES CORRELATION AND RUNS TEST ON RESIDUALS          2555
C      (COMPUTED USING SOLUTION SOL).  GETPRU IS THE APPROX.                2556
C      PROBABILITY THAT A RANDOMLY CHOSEN ORDER OF SIGNS WOULD HAVE         2557
C      NO MORE RUNS THAN THE RESIDUALS.  APPROX. IS ONLY GOOD IF            2558
C      THERE ARE AT LEAST OF THE ORDER OF 10 POSITIVE AND                   2559
C      10 NEGATIVE RESIDUALS.  THEREFORE IF THERE ARE NOT AT LEAST 10 OF    2560
C      EACH SIGN, GETPRU IS SET TO -1.                                      2561
C  PUNCOR(J), J=1,...,5, IS THE APPROX. PROBABILITY THAT A RANDOM           2562
C      SAMPLE FROM A NORMAL POPULATION WOULD HAVE A SMALLER                 2563
C      AUTOCOVARIANCE FOR LAG J THAN THE RESIDUALS.  IF NY.LE.5, THEN       2564
C      ALL PUNCOR ARE SET TO -1.                                            2565
C-----------------------------------------------------------------------    2566
C  CALLS SUBPROGRAMS - GETYLY, PGAUSS, FISHNI                               2567
C  WHICH IN TURN CALL - GETROW, USERK, USERLF, ERRMES, BETAIN, GAMLN        2568
C-----------------------------------------------------------------------    2569
      SUBROUTINE GETPRU (SOL,                                               2570
     1 CQUAD,G,IUNIT,IWT,MWORK,NG,NGL,NLINF,NOUT,NY,PRUNS,PUNCOR,SQRTW,     2571
     2 SSCALE,T,WORK,Y,YLYFIT)                                              2572
      DOUBLE PRECISION SOL, SSCALE, WORK                                    2573
      DIMENSION WORK(MWORK), SQRTW(NY), CQUAD(NG), G(NG), T(NY),            2574
     1 Y(NY), YLYFIT(NY), SOL(NGL), SSCALE(NGL)                             2575
      DIMENSION PUNCOR(5)                                                   2576
      CALL GETYLY (SOL,                                                     2577
     1 CQUAD,G,IUNIT,IWT,MWORK,NG,NGL,NLINF,NY,SQRTW,SSCALE,T,WORK,Y,       2578
     2 YLYFIT)                                                              2579
C-----------------------------------------------------------------------    2580
C  RUNS TEST.                                                               2581
C-----------------------------------------------------------------------    2582
      YLYOLD=-YLYFIT(1)                                                     2583
      NRUN=0                                                                2584
      DDUM=0.                                                               2585
      DO 110 IROW=1,NY                                                      2586
        DUM=YLYFIT(IROW)                                                    2587
        DDUM=DDUM+SIGN(1.,DUM)                                              2588
        IF (DUM*YLYOLD .LT. 0.) NRUN=NRUN+1                                 2589
        YLYOLD=DUM                                                          2590
  110 CONTINUE                                                              2591
      RN1=.5*(FLOAT(NY)+DDUM)                                               2592
      RN2=RN1-DDUM                                                          2593
      DUM=2.*RN1*RN2                                                        2594
      RMU=DUM/(RN1+RN2)+1.                                                  2595
      SIG=SQRT(DUM*(DUM-RN1-RN2)/(RN1+RN2-1.))/(RN1+RN2)                    2596
      PRUNS=-1.                                                             2597
      IF (AMIN1(RN1,RN2) .GT. 9.5) PRUNS=PGAUSS((FLOAT(NRUN)-RMU+.5)/       2598
     1 SIG)                                                                 2599
C-----------------------------------------------------------------------    2600
C  TEST AUTOCOVARIANCES.                                                    2601
C-----------------------------------------------------------------------    2602
      DO 120 J=1,5                                                          2603
        PUNCOR(J)=-1.                                                       2604
  120 CONTINUE                                                              2605
      IF (NY .LE. 7) RETURN                                                 2606
      RSUMST=0.                                                             2607
      DO 130 K=1,NY                                                         2608
  130 RSUMST=RSUMST+YLYFIT(K)                                               2609
      RSUMEN=RSUMST                                                         2610
      DO 150 L=1,5                                                          2611
      NEND=NY-L                                                             2612
      RSUMST=RSUMST-YLYFIT(NEND+1)                                          2613
      RSUMEN=RSUMEN-YLYFIT(L)                                               2614
      STBAR=RSUMST/FLOAT(NEND)                                              2615
      ENBAR=RSUMEN/FLOAT(NEND)                                              2616
      SS=0.                                                                 2617
      SE=0.                                                                 2618
      EE=0.                                                                 2619
      J=L                                                                   2620
      DO 152 K=1,NEND                                                       2621
      S=YLYFIT(K)-STBAR                                                     2622
      J=J+1                                                                 2623
      E=YLYFIT(J)-ENBAR                                                     2624
      SS=SS+S*S                                                             2625
      SE=SE+S*E                                                             2626
      EE=EE+E*E                                                             2627
  152 CONTINUE                                                              2628
      RSQ=SE*SE/(SS*EE)                                                     2629
      PUNCOR(L)=0.                                                          2630
      IF (RSQ .LT. 1.) PUNCOR(L)=1.-FISHNI(FLOAT(NEND-2)*RSQ/(1.-RSQ),      2631
     1 1.,FLOAT(NEND-2),NOUT)                                               2632
  150 CONTINUE                                                              2633
      RETURN                                                                2634
      END                                                                   2635
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2636
C  SUBROUTINE GETROW.  LOADS ROW IROW OF SCALED AUGMENTED COEFFICIENT       2637
C      MATRIX INTO ARRAY A IN STORAGE INCREMENTS OF INC.                    2638
C  IF IUNIT .LT. 0, THEN THE MATRIX ELEMENTS ARE RECALCULATED               2639
C      EACH TIME.                                                           2640
C  IF IUNIT .GE. 0, THEN THE MATRIX ELEMENTS ARE CALCULATED                 2641
C      AND WRITTEN ON UNIT IUNIT (IF INIT=.TRUE.), OR READ FROM UNIT        2642
C      IUNIT (IF INIT=.FALSE.).                                             2643
C  THEREFORE, IF IUNIT .GE. 0, THEN GETROW MUST BE CALLED IN A              2644
C      SEQUENCE SUCH THAT IROW GOES FROM 1 TO NY.                           2645
C-----------------------------------------------------------------------    2646
C  CALLS SUBPROGRAMS - USERK, USERLF                                        2647
C  WHICH IN TURN CALL - ERRMES, USERTR                                      2648
C-----------------------------------------------------------------------    2649
      SUBROUTINE GETROW (IROW,A,INIT,INC,IUNIT,                             2650
     1  SQRTW,NY,NGL,NG,CQUAD,G,T,NLINF,Y,SSCALE)                           2651
      DOUBLE PRECISION A, SSCALE                                            2652
      LOGICAL INIT                                                          2653
      DIMENSION SQRTW(NY), A(1), CQUAD(NG), G(NG), T(NY), Y(NY),            2654
     1 SSCALE(NGL)                                                          2655
      SWT=SQRTW(IROW)                                                       2656
      JJ=1+INC*NGL                                                          2657
      IF (IUNIT.LT.0 .OR. INIT) GO TO 200                                   2658
C-----------------------------------------------------------------------    2659
C  READ IN ELEMENTS FROM UNIT IUNIT.                                        2660
C-----------------------------------------------------------------------    2661
      READ (IUNIT) (A(J),J=1,JJ,INC)                                        2662
      GO TO 300                                                             2663
C-----------------------------------------------------------------------    2664
C  CALCULATE MATRIX ELEMENTS.                                               2665
C-----------------------------------------------------------------------    2666
  200 K=1                                                                   2667
      DO 210 J=1,NG                                                         2668
        JJJ=J                                                               2669
        A(K)=SSCALE(J)*CQUAD(J)*SWT*USERK(IROW,T,JJJ,G)                     2670
        K=K+INC                                                             2671
  210 CONTINUE                                                              2672
      IF (NLINF .LE. 0) GO TO 230                                           2673
      IS=NG                                                                 2674
      DO 220 J=1,NLINF                                                      2675
        JJJ=J                                                               2676
        IS=IS+1                                                             2677
        A(K)=SSCALE(IS)*SWT*USERLF(IROW,JJJ,T,NY)                           2678
        K=K+INC                                                             2679
  220 CONTINUE                                                              2680
  230 A(K)=Y(IROW)*SWT                                                      2681
      IF (IUNIT .LT. 0) GO TO 800                                           2682
      WRITE (IUNIT) (A(J),J=1,JJ,INC)                                       2683
  300 IF (IROW .EQ. NY) REWIND IUNIT                                        2684
  800 RETURN                                                                2685
      END                                                                   2686
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2687
C  SUBROUTINE GETYLY.  PUTS RESIDUALS (USING SOLUTION IN SOL)               2688
C      IN YLYFIT(J), J=1,NY.                                                2689
C-----------------------------------------------------------------------    2690
C  CALLS SUBPROGRAMS - GETROW                                               2691
C  WHICH IN TURN CALL - USERK, USERLF, ERRMES, USERTR                       2692
C-----------------------------------------------------------------------    2693
      SUBROUTINE GETYLY (SOL,                                               2694
     1 CQUAD,G,IUNIT,IWT,MWORK,NG,NGL,NLINF,NY,SQRTW,SSCALE,T,WORK,Y,       2695
     2 YLYFIT)                                                              2696
      DOUBLE PRECISION DUM, SOL, WORK, SSCALE                               2697
      DIMENSION WORK(MWORK), SQRTW(NY), CQUAD(NG), G(NG), T(NY),            2698
     1 Y(NY), YLYFIT(NY), SOL(NGL), SSCALE(NGL)                             2699
      DO 110 IROW=1,NY                                                      2700
        IIROW=IROW                                                          2701
        CALL GETROW (IIROW,WORK,.FALSE.,1,IUNIT,                            2702
     1  SQRTW,NY,NGL,NG,CQUAD,G,T,NLINF,Y,SSCALE)                           2703
        DUM=WORK(NGL+1)                                                     2704
        DO 120 ICOL=1,NGL                                                   2705
          DUM=DUM-WORK(ICOL)*SOL(ICOL)                                      2706
  120   CONTINUE                                                            2707
        YLYFIT(IROW)=DUM                                                    2708
  110 CONTINUE                                                              2709
      RETURN                                                                2710
      END                                                                   2711
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2712
C     SUBROUTINE H12 (MODE,LPIVOT,L1,M,U,IUE,UP,C,ICE,ICV,NCV,RANGE)        2713
C     BASED ON C.L.LAWSON AND R.J.HANSON,                                   2714
C     'SOLVING LEAST SQUARES PROBLEMS', PRENTICE-HALL, 1974                 2715
C                                                                           2716
C     CONSTRUCTION AND/OR APPLICATION OF A SINGLE                           2717
C     HOUSEHOLDER TRANSFORMATION..     Q = I + U*(U**T)/B                   2718
C                                                                           2719
C     MODE    = 1 OR 2   TO SELECT ALGORITHM  H1  OR  H2 .                  2720
C     LPIVOT IS THE INDEX OF THE PIVOT ELEMENT.                             2721
C     L1,M   IF L1 .LE. M   THE TRANSFORMATION WILL BE CONSTRUCTED TO       2722
C            ZERO ELEMENTS INDEXED FROM L1 THROUGH M.   IF L1 GT. M         2723
C            THE SUBROUTINE DOES AN IDENTITY TRANSFORMATION.                2724
C     U(),IUE,UP    ON ENTRY TO H1 U() CONTAINS THE PIVOT VECTOR.           2725
C                   IUE IS THE STORAGE INCREMENT BETWEEN ELEMENTS.          2726
C                                       ON EXIT FROM H1 U() AND UP          2727
C                   CONTAIN QUANTITIES DEFINING THE VECTOR U OF THE         2728
C                   HOUSEHOLDER TRANSFORMATION.   ON ENTRY TO H2 U()        2729
C                   AND UP SHOULD CONTAIN QUANTITIES PREVIOUSLY COMPUTED    2730
C                   BY H1.  THESE WILL NOT BE MODIFIED BY H2.               2731
C     C()    ON ENTRY TO H1 OR H2 C() CONTAINS A MATRIX WHICH WILL BE       2732
C            REGARDED AS A SET OF VECTORS TO WHICH THE HOUSEHOLDER          2733
C            TRANSFORMATION IS TO BE APPLIED.  ON EXIT C() CONTAINS THE     2734
C            SET OF TRANSFORMED VECTORS.                                    2735
C     ICE    STORAGE INCREMENT BETWEEN ELEMENTS OF VECTORS IN C().          2736
C     ICV    STORAGE INCREMENT BETWEEN VECTORS IN C().                      2737
C     NCV    NUMBER OF VECTORS IN C() TO BE TRANSFORMED. IF NCV .LE. 0      2738
C            NO OPERATIONS WILL BE DONE ON C().                             2739
C  RANGE IS 2 OR 3 ORDERS OF MAGNITUDE SMALLER THAN BIG, WHERE BIG IS       2740
C      THE LARGEST NUMBER THAT DOES NOT OVERFLOW AND 1/BIG DOES NOT         2741
C      UNDERFLOW.  FOR THE DOUBLE PRECISION VERSION, BIG AND RANGE          2742
C      ARE IN DOUBLE PRECISION.  FOR THE SINGLE PRECISION VERSION,          2743
C      THEY ARE IN SINGLE PRECISION (AND THEREFORE RANGE=SRANGE).           2744
C                                                                           2745
      SUBROUTINE H12 (MODE,LPIVOT,L1,M,U,IUE,UP,C,ICE,ICV,NCV,RANGE)        2746
      DIMENSION U(IUE,1), C(1)                                              2747
      DOUBLE PRECISION SM,B                                                 2748
      DOUBLE PRECISION ABS, AMAX1, C, CL, CLINV, DOUBLE, ONE, RANGE,        2749
     1 RANGIN, SIGN, SM1, SQRT, U, UP                                       2750
      ABS(SM)=DABS(SM)                                                      2751
      AMAX1(SM,ONE)=DMAX1(SM,ONE)                                           2752
C     DOUBLE(ONE)=DBLE(ONE)!SP                                              2753
      DOUBLE(SM)=SM                                                         2754
      SQRT(SM)=DSQRT(SM)                                                    2755
      SIGN(SM,ONE)=DSIGN(SM,ONE)                                            2756
C     ONE=1.E0!SP                                                           2757
      ONE=1.D0                                                              2758
C                                                                           2759
      IF (0.GE.LPIVOT.OR.LPIVOT.GE.L1.OR.L1.GT.M) RETURN                    2760
      RANGIN=ONE/RANGE                                                      2761
      CL=ABS(U(1,LPIVOT))                                                   2762
      IF (MODE.EQ.2) GO TO 60                                               2763
C                            ****** CONSTRUCT THE TRANSFORMATION. ******    2764
          DO 10 J=L1,M                                                      2765
   10     CL=AMAX1(ABS(U(1,J)),CL)                                          2766
      IF (CL .LE. RANGIN) GO TO 130                                         2767
      CLINV=ONE/CL                                                          2768
      SM=(DOUBLE(U(1,LPIVOT))*CLINV)**2                                     2769
          DO 30 J=L1,M                                                      2770
   30     SM=SM+(DOUBLE(U(1,J))*CLINV)**2                                   2771
C                          CONVERT DOUBLE PREC. SM TO SNGL. PREC. SM1       2772
      SM1=SM                                                                2773
      CL=-SIGN(CL*SQRT(SM1),U(1,LPIVOT))                                    2774
      UP=U(1,LPIVOT)-CL                                                     2775
      U(1,LPIVOT)=CL                                                        2776
      GO TO 70                                                              2777
C            ****** APPLY THE TRANSFORMATION  I+U*(U**T)/B  TO C. ******    2778
C                                                                           2779
   60 IF (CL .LE. RANGIN) GO TO 130                                         2780
   70 IF (NCV.LE.0) RETURN                                                  2781
      B=DOUBLE(UP)*U(1,LPIVOT)                                              2782
C                       B  MUST BE NONPOSITIVE HERE.  IF B = 0., RETURN.    2783
C                                                                           2784
      IF (B .GE. -RANGIN) GO TO 130                                         2785
      B=ONE/B                                                               2786
      I2=1-ICV+ICE*(LPIVOT-1)                                               2787
      INCR=ICE*(L1-LPIVOT)                                                  2788
          DO 120 J=1,NCV                                                    2789
          I2=I2+ICV                                                         2790
          I3=I2+INCR                                                        2791
          I4=I3                                                             2792
          SM=C(I2)*DOUBLE(UP)                                               2793
              DO 90 I=L1,M                                                  2794
              SM=SM+C(I3)*DOUBLE(U(1,I))                                    2795
   90         I3=I3+ICE                                                     2796
          IF (SM) 100,120,100                                               2797
  100     SM=SM*B                                                           2798
          C(I2)=C(I2)+SM*DOUBLE(UP)                                         2799
              DO 110 I=L1,M                                                 2800
              C(I4)=C(I4)+SM*DOUBLE(U(1,I))                                 2801
  110         I4=I4+ICE                                                     2802
  120     CONTINUE                                                          2803
  130 RETURN                                                                2804
      END                                                                   2805
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2806
C  SUBROUTINE INIT.  INITIALIZES VARIABLES AT THE START OF A RUN.           2807
C-----------------------------------------------------------------------    2808
C  CALLS SUBPROGRAMS - DIFF, ERRMES                                         2809
C-----------------------------------------------------------------------    2810
      SUBROUTINE INIT                                                       2811
      DOUBLE PRECISION PRECIS, RANGE                                        2812
      DOUBLE PRECISION ONE, ZERO, PREMIN, DIFF,                             2813
     1 SMALL, SIZE, PTRY, DELTRY, AMAX1                                     2814
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  2815
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              2816
      LOGICAL DONE1                                                         2817
      DIMENSION IHOLER(6)                                                   2818
      COMMON /DBLOCK/ PRECIS, RANGE                                         2819
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         2820
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     2821
     2 SRANGE                                                               2822
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     2823
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       2824
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             2825
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              2826
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         2827
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            2828
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  2829
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            2830
     2 LUSER(30)                                                            2831
      DATA IHOLER/1HI, 1HN, 1HI, 1HT, 2*1H /                                2832
      AMAX1(PTRY,PRECIS)=DMAX1(PTRY,PRECIS)                                 2833
C     ONE=1.E0!SP                                                           2834
      ONE=1.D0                                                              2835
C     ZERO=0.E0!SP                                                          2836
      ZERO=0.D0                                                             2837
C     PREMIN=1.E-4!SP                                                       2838
      PREMIN=1.D-8                                                          2839
      FACT=RANGE**(-.025)                                                   2840
      SMALL=ONE/RANGE                                                       2841
      DONE1=.FALSE.                                                         2842
      SIZE=RANGE                                                            2843
      DO 110 J=1,80                                                         2844
        PTRY=PREMIN                                                         2845
        DO 120 K=1,150                                                      2846
          PTRY=.5*PTRY                                                      2847
          DELTRY=PTRY*SIZE                                                  2848
          IF (DELTRY .LT. SMALL) GO TO 140                                  2849
          IF (DIFF(SIZE+DELTRY,SIZE) .LE. ZERO) GO TO 130                   2850
  120   CONTINUE                                                            2851
  130   IF (DONE1) PRECIS=AMAX1(PTRY,PRECIS)                                2852
        IF (.NOT.DONE1) PRECIS=PTRY                                         2853
        DONE1=.TRUE.                                                        2854
        SIZE=FACT*SIZE                                                      2855
  110 CONTINUE                                                              2856
  140 PRECIS=20.*PRECIS                                                     2857
      IF (PRECIS .GT. PREMIN) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)            2858
      MIOERR=MAX0(2,MIOERR)                                                 2859
      EXMAX=ALOG(SRANGE)                                                    2860
      RETURN                                                                2861
      END                                                                   2862
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2863
C  SUBROUTINE INPUT.  READS INPUT DATA, WRITES IT OUT, AND CHECKS           2864
C      FOR INPUT ERRORS.                                                    2865
C-----------------------------------------------------------------------    2866
C  CALLS SUBPROGRAMS - STORIN, ERRMES, READYT, WRITIN                       2867
C  WHICH IN TURN CALL - USERIN, WRITYT                                      2868
C-----------------------------------------------------------------------    2869
      SUBROUTINE INPUT (EXACT,G,MA,MEQ,MG,MINEQ,MREG,MWORK,MY,SQRTW,T,Y)    2870
      DOUBLE PRECISION PRECIS, RANGE                                        2871
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  2872
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              2873
      LOGICAL LERR                                                          2874
      DIMENSION SQRTW(MY), T(MY), Y(MY), EXACT(MY), G(MG)                   2875
      DIMENSION LIN(6), LA(6,50), LA1(6,13), LA2(6,13), LA3(6,13),          2876
     1 LA4(6,11), IHOLER(6)                                                 2877
      COMMON /DBLOCK/ PRECIS, RANGE                                         2878
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         2879
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     2880
     2 SRANGE                                                               2881
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     2882
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       2883
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             2884
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              2885
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         2886
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            2887
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  2888
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            2889
     2 LUSER(30)                                                            2890
C-----------------------------------------------------------------------    2891
C  LA IS BEING BROKEN UP JUST TO KEEP THE NO. OF CONTINUATION               2892
C      CARDS IN THE DATA STATEMENTS SMALL.                                  2893
C-----------------------------------------------------------------------    2894
      EQUIVALENCE (LA(1,1),LA1(1,1)), (LA(1,14),LA2(1,1)),                  2895
     1 (LA(1,27),LA3(1,1)), (LA(1,40),LA4(1,1))                             2896
      DATA MLA/50/, IHOLER/1HI, 1HN, 1HP, 1HU, 1HT, 1H /                    2897
      DATA LA1/                                                             2898
     1 1HD, 1HF, 1HM, 1HI, 1HN, 1H ,                                        2899
     2 1HS, 1HR, 1HM, 1HI, 1HN, 1H ,   1HA, 1HL, 1HP, 1HS, 1HT, 1H ,        2900
     3 1HG, 1HM, 1HN, 1HM, 1HX, 1H ,   1HP, 1HL, 1HE, 1HV, 1HE, 1HL,        2901
     4 1HR, 1HS, 1HV, 1HM, 1HN, 1HX,   1HR, 1HU, 1HS, 1HE, 1HR, 1H ,        2902
     5 1HI, 1HG, 1HR, 1HI, 1HD, 1H ,   1HI, 1HQ, 1HU, 1HA, 1HD, 1H ,        2903
     6 1HI, 1HU, 1HN, 1HI, 1HT, 1H ,   1HI, 1HW, 1HT, 3*1H ,                2904
     7 1HL, 1HI, 1HN, 1HE, 1HP, 1HG,   1HM, 1HI, 1HO, 1HE, 1HR, 1HR/        2905
      DATA LA2/                                                             2906
     1 1HM, 1HP, 1HK, 1HM, 1HO, 1HM,   1HM, 1HQ, 1HP, 1HI, 1HT, 1HR,        2907
     2 1HN, 1HE, 1HQ, 3*1H ,           1HN, 1HE, 1HR, 1HF, 1HI, 1HT,        2908
     3 1HN, 1HG, 4*1H ,                1HN, 1HI, 1HN, 1HT, 1HT, 1H ,        2909
     4 1HN, 1HL, 1HI, 1HN, 1HF, 1H ,   1HN, 1HO, 1HR, 1HD, 1HE, 1HR,        2910
     5 1HI, 1HC, 1HR, 1HI, 1HT, 1H ,                                        2911
     6 1HI, 1HF, 1HO, 1HR, 1HM, 1HT,   1HI, 1HF, 1HO, 1HR, 1HM, 1HW,        2912
     7 1HI, 1HF, 1HO, 1HR, 1HM, 1HY,   1HI, 1HP, 1HL, 1HF, 1HI, 1HT/        2913
      DATA LA3/                                                             2914
     1 1HI, 1HP, 1HL, 1HR, 1HE, 1HS,   1HI, 1HP, 1HR, 1HI, 1HN, 1HT,        2915
     2 1HI, 1HU, 1HS, 1HE, 1HR, 1H ,   1HI, 1HU, 1HS, 1HR, 1HO, 1HU,        2916
     3 1HL, 1HS, 1HI, 1HG, 1HN, 1H ,   1HM, 1HO, 1HM, 1HN, 1HM, 1HX,        2917
     4 1HN, 1HE, 1HN, 1HD, 1HZ, 1H ,   1HN, 1HF, 1HL, 1HA, 1HT, 1H ,        2918
     5 1HN, 1HN, 1HS, 1HG, 1HN, 1H ,   1HN, 1HQ, 1HP, 1HR, 1HO, 1HG,        2919
     6 1HN, 1HS, 1HG, 1HN, 2*1H ,      1HD, 1HO, 1HC, 1HH, 1HO, 1HS,        2920
     7 1HD, 1HO, 1HM, 1HO, 1HM, 1H /                                        2921
      DATA LA4/                                                             2922
     1 1HD, 1HO, 1HU, 1HS, 1HI, 1HN,   1HD, 1HO, 1HU, 1HS, 1HN, 1HQ,        2923
     2 1HL, 1HA, 1HS, 1HT, 2*1H ,      1HN, 1HE, 1HW, 1HP, 1HG, 1H1,        2924
     3 1HN, 1HO, 1HN, 1HN, 1HE, 1HG,   1HO, 1HN, 1HL, 1HY, 1H1, 1H ,        2925
     4 1HP, 1HR, 1HW, 1HT, 2*1H ,      1HP, 1HR, 1HY, 3*1H ,                2926
     5 1HS, 1HI, 1HM, 1HU, 1HL, 1HA,   1HL, 1HU, 1HS, 1HE, 1HR, 1H ,        2927
     6 1HE, 1HN, 1HD, 3*1H /                                                2928
 5100 FORMAT (80A1)                                                         2929
      READ (NIN,5100) ITITLE                                                2930
 5999 FORMAT (1H1)                                                          2931
      IF (NEWPG1) WRITE (NOUT,5999)                                         2932
 5101 FORMAT (34H CONTIN - VERSION 2DP (MAR 1984) (,6A1,6H PACK),6X,80A1    2933
     1 //59H REFERENCES - S.W. PROVENCHER (1982) COMPUT. PHYS. COMMUN.,,    2934
     2 33H VOL. 27, PAGES 213-227, 229-242./30X,                            2935
     3 53H(1984) EMBL TECHNICAL REPORT DA07 (EUROPEAN MOLECULAR,            2936
     4 49H BIOLOGY LABORATORY, HEIDELBERG, F.R. OF GERMANY)                 2937
     5 ///20X,42HINPUT DATA FOR CHANGES TO COMMON VARIABLES)                2938
      WRITE (NOUT,5101) IAPACK, ITITLE                                      2939
      NIOERR=0                                                              2940
 5200 FORMAT (1X,6A1,I5,E15.6)                                              2941
  200 READ (NIN,5200) LIN,IIN,RIN                                           2942
 5210 FORMAT (/1X,6A1,I5,1PE15.5)                                           2943
      WRITE (NOUT,5210) LIN,IIN,RIN                                         2944
      DO 210 J=1,MLA                                                        2945
        DO 220 K=1,6                                                        2946
          IF (LIN(K) .NE. LA(K,J)) GO TO 210                                2947
  220   CONTINUE                                                            2948
        IF (J .EQ. MLA) GO TO 300                                           2949
        JJ=J                                                                2950
        CALL STORIN (JJ,NIOERR,LIN,IIN,RIN)                                 2951
        GO TO 200                                                           2952
  210 CONTINUE                                                              2953
      CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                                   2954
 5001 FORMAT (1H )                                                          2955
      WRITE (NOUT,5001)                                                     2956
      NIOERR=NIOERR+1                                                       2957
      IF (NIOERR .GE. MIOERR) STOP                                          2958
      GO TO 200                                                             2959
  300 CALL READYT (MY,NIOERR,SQRTW,T,Y)                                     2960
      CALL WRITIN (EXACT,G,LA,MG,MY,SQRTW,T,Y)                              2961
      NEWPG1=.TRUE.                                                         2962
C-----------------------------------------------------------------------    2963
C  CHECK COMMON VARIABLES FOR VIOLATIONS.                                   2964
C-----------------------------------------------------------------------    2965
      LERR=.FALSE.                                                          2966
      DO 410 K=1,2                                                          2967
        DO 420 J=1,2                                                        2968
          IF (K.EQ.2 .OR. (IWT.NE.1 .AND. IWT.NE.4)) LERR=LERR .OR.         2969
     1    PLEVEL(J,K).LT.0. .OR. PLEVEL(J,K).GT.1. .OR. ICRIT(K).LT.1       2970
     2     .OR. ICRIT(K).GT.2                                               2971
  420   CONTINUE                                                            2972
        IF (NQPROG(1) .GT. 0) LERR=LERR .OR. RSVMNX(K,1).LE.0.              2973
  410 CONTINUE                                                              2974
      LERR=LERR .OR. MIN0(IGRID,IQUAD,IWT,NG-1,NG+NLINF-NEQ).LT.1 .OR.      2975
     1 MIN0(NLINF,NEQ).LT.0 .OR.                                            2976
     2 MAX0(IGRID-3,IQUAD-3,IWT-5,NEQ-MEQ,NG+NLINF+2-MIN0(MG,MA),           2977
     3 NG+NLINF+1-MREG,NORDER-5,                                            2978
     4 MAX0(MG,NY)-MY,MAX0((MINEQ+2)*(MG+1)-4,MG*(MG-2),4*MG)-MWORK)        2979
     5 .GT. 0                                                               2980
      IF (.NOT.LERR) GO TO 500                                              2981
      CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                                   2982
 5420 FORMAT (5H MY =,I5,5X,4HMA =,I3,5X,4HMG =,I3,5X,6HMREG =,I3,5X,       2983
     1 7HMINEQ =,I3,5X,5HMEQ =,I3,5X,7HMWORK =,I5)                          2984
      WRITE (NOUT,5420) MY,MA,MG,MREG,MINEQ,MEQ,MWORK                       2985
      STOP                                                                  2986
  500 IF (NIOERR .NE. 0) STOP                                               2987
      RETURN                                                                2988
      END                                                                   2989
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    2990
      SUBROUTINE LDP (G,MDG,M,N,H,X,XNORM,W,INDEX,MODE,RANGE)               2991
C     BASED ON C.L.LAWSON AND R.J.HANSON,                                   2992
C     'SOLVING LEAST SQUARES PROBLEMS', PRENTICE-HALL, 1974                 2993
C                                                                           2994
C            **********  LEAST DISTANCE PROGRAMMING  **********             2995
C                                                                           2996
C  RANGE IS 2 OR 3 ORDERS OF MAGNITUDE SMALLER THAN BIG, WHERE BIG IS       2997
C      THE LARGEST NUMBER THAT DOES NOT OVERFLOW AND 1/BIG DOES NOT         2998
C      UNDERFLOW.  FOR THE DOUBLE PRECISION VERSION, BIG AND RANGE          2999
C      ARE IN DOUBLE PRECISION.  FOR THE SINGLE PRECISION VERSION,          3000
C      THEY ARE IN SINGLE PRECISION (AND THEREFORE RANGE=SRANGE).           3001
C-----------------------------------------------------------------------    3002
C  CALLS SUBPROGRAMS - DIFF, NNLS                                           3003
C  WHICH IN TURN CALL - H12, G1, G2                                         3004
C-----------------------------------------------------------------------    3005
      DOUBLE PRECISION DIFF, FAC, G, H, ONE, RANGE, RNORM,                  3006
     1 SQRT, W, X, XNORM, ZERO                                              3007
      INTEGER INDEX(M)                                                      3008
      DIMENSION G(MDG,N), H(M), X(1), W(1)                                  3009
      SQRT(FAC)=DSQRT(FAC)                                                  3010
C     ZERO=0.E0!SP                                                          3011
      ZERO=0.D0                                                             3012
C     ONE=1.E0!SP                                                           3013
      ONE=1.D0                                                              3014
      IF (N.LE.0) GO TO 120                                                 3015
          DO 10 J=1,N                                                       3016
   10     X(J)=ZERO                                                         3017
      XNORM=ZERO                                                            3018
      IF (M.LE.0) GO TO 110                                                 3019
C                                                                           3020
C     THE DECLARED DIMENSION OF W() MUST BE AT LEAST (N+1)*(M+2)+2*M.       3021
C                                                                           3022
C      FIRST (N+1)*M LOCS OF W()   =  MATRIX E FOR PROBLEM NNLS.            3023
C       NEXT     N+1 LOCS OF W()   =  VECTOR F FOR PROBLEM NNLS.            3024
C       NEXT     N+1 LOCS OF W()   =  VECTOR Z FOR PROBLEM NNLS.            3025
C       NEXT       M LOCS OF W()   =  VECTOR Y FOR PROBLEM NNLS.            3026
C       NEXT       M LOCS OF W()   =  VECTOR WDUAL FOR PROBLEM NNLS.        3027
C     COPY G**T INTO FIRST N ROWS AND M COLUMNS OF E.                       3028
C     COPY H**T INTO ROW N+1 OF E.                                          3029
C                                                                           3030
      IW=0                                                                  3031
          DO 30 J=1,M                                                       3032
              DO 20 I=1,N                                                   3033
              IW=IW+1                                                       3034
   20         W(IW)=G(J,I)                                                  3035
          IW=IW+1                                                           3036
   30     W(IW)=H(J)                                                        3037
      IF=IW+1                                                               3038
C                                STORE N ZEROS FOLLOWED BY A ONE INTO F.    3039
          DO 40 I=1,N                                                       3040
          IW=IW+1                                                           3041
   40     W(IW)=ZERO                                                        3042
      W(IW+1)=ONE                                                           3043
C                                                                           3044
      NP1=N+1                                                               3045
      IZ=IW+2                                                               3046
      IY=IZ+NP1                                                             3047
      IWDUAL=IY+M                                                           3048
C                                                                           3049
      CALL NNLS (W,NP1,NP1,M,W(IF),W(IY),RNORM,W(IWDUAL),W(IZ),INDEX,       3050
     *           MODE,RANGE)                                                3051
C                      USE THE FOLLOWING RETURN IF UNSUCCESSFUL IN NNLS.    3052
      IF (MODE.NE.1) RETURN                                                 3053
      IF (RNORM) 130,130,50                                                 3054
   50 FAC=ONE                                                               3055
      IW=IY-1                                                               3056
          DO 60 I=1,M                                                       3057
          IW=IW+1                                                           3058
C                               HERE WE ARE USING THE SOLUTION VECTOR Y.    3059
   60     FAC=FAC-H(I)*W(IW)                                                3060
C                                                                           3061
      IF (DIFF(ONE+FAC,ONE)) 130,130,70                                     3062
   70 FAC=ONE/FAC                                                           3063
          DO 90 J=1,N                                                       3064
          IW=IY-1                                                           3065
              DO 80 I=1,M                                                   3066
              IW=IW+1                                                       3067
C                               HERE WE ARE USING THE SOLUTION VECTOR Y.    3068
   80         X(J)=X(J)+G(I,J)*W(IW)                                        3069
   90     X(J)=X(J)*FAC                                                     3070
          DO 100 J=1,N                                                      3071
  100     XNORM=XNORM+X(J)**2                                               3072
      XNORM=SQRT(XNORM)                                                     3073
C                             SUCCESSFUL RETURN.                            3074
  110 MODE=1                                                                3075
      RETURN                                                                3076
C                             ERROR RETURN.       N .LE. 0.                 3077
  120 MODE=2                                                                3078
      RETURN                                                                3079
C                             RETURNING WITH CONSTRAINTS NOT COMPATIBLE.    3080
  130 MODE=4                                                                3081
      RETURN                                                                3082
      END                                                                   3083
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    3084
C  SUBROUTINE LDPETC.  EVALUATES CONSTRAINED REGULARIZED SOLUTION           3085
C      USING LEAST-DISTANCE PROGRAMMING.                                    3086
C  DOES STATISTICAL TESTS ON SOLUTION AND STORES IT IN SOLBES IF IT         3087
C      IS THE BEST SO FAR AND IF SSEARC=.TRUE..                             3088
C  PPLTPR=.TRUE. FOR COMPUTING COVARIANCE MATRIX FOR                        3089
C      PLOT OF SOLUTION ON PRINTER OR CALL TO USEROU.                       3090
C  PRINTS OUT MOMENTS IF DOMOM=PPLTPR=.TRUE..                               3091
C  NEWPAG=.TRUE. FOR STARTING A NEW PAGE.                                   3092
C  HEADNG=.TRUE. FOR PRINTING A HEADING FOR ONE-LINE SUMMARIES.             3093
C  PRLDP=.TRUE. FOR PRINTING OUT ONE-LINE SUMMARY OF ANALYSIS.              3094
C  IICRIT=ICRIT(ISTAGE) IN CALLING PROGRAM.                                 3095
C  NNNNEQ=CURRENT NO. OF INEQUALITY CONSTRAINTS.                            3096
C  ON RETURN, IERROR = 1 FOR NORMAL RETURN                                  3097
C                      3 FOR MAX. NO. OF ITERATIONS IN NNLS IN LDP          3098
C                      5 FOR ERROR RETURN FROM SVDRS2 IN CVNEQ.             3099
C-----------------------------------------------------------------------    3100
C  CALLS SUBPROGRAMS - LDP, ERRMES, CVNEQ, GETYLY, FISHNI, PLPRIN,          3101
C      USEROU, MOMENT                                                       3102
C  WHICH IN TURN CALL - NNLS, DIFF, H12, G1, G2, GETROW, USERK, USERLF,     3103
C      ELIMEQ, SVDRS2, LH1405, QRBD, BETAIN, GAMLN, MOMOUT, USERTR          3104
C-----------------------------------------------------------------------    3105
      SUBROUTINE LDPETC (ILEVEL,FINDVZ,NNNNEQ,SSEARC,IICRIT,DDOMOM,         3106
     1 PPLTPR,PRLDP,ALPHA,HEADNG,NEWPAG,ALPBES,VAR,                         3107
     2  A,AA,AINEQ,BTEST,CQUAD,DEGFRE,DEGFRZ,EXACT,G,IERROR,                3108
     3  ISTAGE,IWORK,LBIND,MA,MG,MINEQ,MREG,MWORK,MY,                       3109
     4  NGLE,NGLY,PREJ,REG,RHSNEQ,RS2MNX,S,SOLBES,                          3110
     5  SOLUTN,SQRTW,SSCALE,T,VALPCV,VALPHA,VARREG,VARZ,WORK,Y,YLYFIT)      3111
      DOUBLE PRECISION PRECIS, RANGE                                        3112
      DOUBLE PRECISION A, AA, AAMAX, AASCMX, ABS, AINEQ, ALPBES,            3113
     1 ALPHA, AMAX1, DUB, REG, RHSNEQ, S, SOLBES, SOLUTN, SQRT,             3114
     2 SSCALE, VALPCV, VALPHA, WORK, ZERO                                   3115
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  3116
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              3117
      LOGICAL FINDVZ, SSEARC, PPLTPR, LBIND, PRLDP, DDOMOM,                 3118
     1 NEWPAG, HEADNG, DOERR                                                3119
      DIMENSION SOLUTN(MG), VALPHA(MG), S(MG,3), A(MA,MG),                  3120
     1 AINEQ(MINEQ,MG), RHSNEQ(MINEQ), WORK(MWORK), IWORK(MA),              3121
     2 REG(MREG,MG), VALPCV(MG), G(MG), EXACT(MG),                          3122
     3 CQUAD(MG), SOLBES(MG), PREJ(2), SSCALE(MG),                          3123
     4 LBIND(MINEQ), AA(MG,MG), SQRTW(MY), T(MY), Y(MY), YLYFIT(MY)         3124
      DIMENSION IHOLER(6), ISTAR(2), RS2MNX(2)                              3125
      COMMON /DBLOCK/ PRECIS, RANGE                                         3126
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         3127
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     3128
     2 SRANGE                                                               3129
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     3130
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       3131
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             3132
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              3133
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         3134
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            3135
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  3136
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            3137
     2 LUSER(30)                                                            3138
      DATA IHOLER/1HL, 1HD, 1HP, 1HE, 1HT, 1HC/, ISTAR/1H , 1H*/            3139
      ABS(DUB)=DABS(DUB)                                                    3140
      SQRT(DUB)=DSQRT(DUB)                                                  3141
      AMAX1(DUB,AAMAX)=DMAX1(DUB,AAMAX)                                     3142
C     ZERO=0.E0!SP                                                          3143
      ZERO=0.D0                                                             3144
      DEGFRE=0.                                                             3145
      LSTAR=1                                                               3146
      VAR=SRANGE                                                            3147
      VARREG=SRANGE                                                         3148
      DO 105 J=1,NGL                                                        3149
        SOLUTN(J)=VALPHA(J)                                                 3150
        S(J,3)=ZERO                                                         3151
  105 CONTINUE                                                              3152
      IF (NNNNEQ .EQ. 0) GO TO 240                                          3153
C-----------------------------------------------------------------------    3154
C  PUT SOLUTION OF LEAST-DISTANCE PROGRAMMING PROBLEM IN S(J,3),            3155
C      J=1,NGLE.                                                            3156
C-----------------------------------------------------------------------    3157
      CALL LDP (A,MA,NNNNEQ,NGLE,RHSNEQ,S(1,3),DUB,WORK,IWORK,IERROR,       3158
     1 RANGE)                                                               3159
      GO TO (240,210,220,230),IERROR                                        3160
  210 CALL ERRMES (1,.TRUE.,IHOLER,NOUT)                                    3161
  220 DDUM=ALPHA/S(1,1)                                                     3162
 5220 FORMAT (41H0MAX. ITERATIONS IN NNLS FOR ALPHA/S(1) =,1PE9.2)          3163
      WRITE (NOUT,5220) DDUM                                                3164
      RETURN                                                                3165
  230 CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                                   3166
      RETURN                                                                3167
  240 VARREG=0.                                                             3168
      DO 250 ICOL=1,NGLE                                                    3169
C-----------------------------------------------------------------------    3170
C  EVALUATE ELEMENT ICOL OF S(TILDE)**(-1)*XI IN EQ. (5.29).                3171
C-----------------------------------------------------------------------    3172
        DUB=S(ICOL,2)*S(ICOL,3)                                             3173
C-----------------------------------------------------------------------    3174
C  COMPUTE PENALTY CONTRIBUTION OF REGULARIZOR TO OBJECTIVE FUNCTION.       3175
C-----------------------------------------------------------------------    3176
        VARREG=VARREG+(DUB+S(ICOL,2)**2*S(ICOL,1)*(A(ICOL,NGLP1)-           3177
     1  S(ICOL,1)*REG(ICOL,NGLP1)))**2                                      3178
        DO 255 IROW=1,NGL                                                   3179
          SOLUTN(IROW)=SOLUTN(IROW)+REG(IROW,ICOL)*DUB                      3180
  255   CONTINUE                                                            3181
  250 CONTINUE                                                              3182
      CALL CVNEQ (ALPHA,IERROR,NNNNEQ,SOLUTN,                               3183
     1 A,AA,AINEQ,DEGFRE,LBIND,MA,MG,MINEQ,MREG,MWORK,NGL,NGLE,             3184
     2 NGLP1,NGLY,NOUT,PPLTPR,RANGE,REG,S,VALPCV,WORK)                      3185
C-----------------------------------------------------------------------    3186
C  COMPUTE VAR (VARIANCE OF FIT).                                           3187
C-----------------------------------------------------------------------    3188
      CALL GETYLY (SOLUTN,                                                  3189
     1 CQUAD,G,IUNIT,IWT,MWORK,NG,NGL,NLINF,NY,SQRTW,SSCALE,T,WORK,Y,       3190
     2 YLYFIT)                                                              3191
      VAR=0.                                                                3192
      DO 310 J=1,NY                                                         3193
        VAR=VAR+YLYFIT(J)**2                                                3194
  310 CONTINUE                                                              3195
      VARREG=VARREG*ALPHA**2+VAR                                            3196
      IF (VARZ.LT.VAR .OR. .NOT.FINDVZ .OR. IERROR.NE.1) GO TO 320          3197
      LSTAR=2                                                               3198
      DEGFRZ=DEGFRE                                                         3199
      VARZ=VAR                                                              3200
  320 PREJ(1)=-1.                                                           3201
      PREJ(2)=-1.                                                           3202
      IF (VARZ.GE.SRANGE .OR. IERROR.NE.1) GO TO 325                        3203
      DDUM=AMAX1(ZERO,VAR/VARZ-1.-ZERO)                                     3204
      DDDUM=AMAX1(ZERO,FLOAT(NY)-DEGFRZ-ZERO)                               3205
      PREJ(1)=FISHNI(DDUM*DDDUM/DEGFRZ,DEGFRZ,DDDUM,NOUT)                   3206
      PREJ(2)=1.                                                            3207
      IF (DEGFRZ-DEGFRE .GT. .1) PREJ(2)=FISHNI(DDUM*DDDUM/                 3208
     1 (DEGFRZ-DEGFRE),DEGFRZ-DEGFRE,DDDUM,NOUT)                            3209
 5999 FORMAT (1H1)                                                          3210
  325 IF (NEWPAG) WRITE (NOUT,5999)                                         3211
      IF (.NOT.HEADNG) GO TO 326                                            3212
      IF (.NOT.NEWPAG .AND. ILEVEL.NE.2) WRITE (NOUT,5003)                  3213
 5003 FORMAT (//1H )                                                        3214
      IF (.NOT.NEWPAG .AND. ILEVEL.EQ.2) WRITE (NOUT,5001)                  3215
 5001 FORMAT (1H )                                                          3216
 5300 FORMAT (10X,80A1,10X,31HPRELIMINARY UNWEIGHTED ANALYSIS)              3217
      IF (ISTAGE .EQ. 1) WRITE (NOUT,5300) ITITLE                           3218
 5302 FORMAT (10X,80A1)                                                     3219
      IF (ISTAGE .EQ. 2) WRITE (NOUT,5302) ITITLE                           3220
 5310 FORMAT (/6X,5HALPHA,4X,10HALPHA/S(1),5X,                              3221
     1 10HOBJ. FCTN.,7X,8HVARIANCE,6X,9HSTD. DEV.,4X,11HDEG FREEDOM,4X,     3222
     2 15HPROB1 TO REJECT,4X,15HPROB2 TO REJECT)                            3223
      WRITE (NOUT,5310)                                                     3224
 5320 FORMAT (1X,A1,1PE9.2,E14.2,2E15.5,E15.3,0PF15.3,2F19.3)               3225
  326 DDUM=ALPHA/S(1,1)                                                     3226
      DDDUM=ALPHA                                                           3227
      STDDEV=SRANGE                                                         3228
      IF (FLOAT(NY) .GT. DEGFRE) DUB=VAR/(FLOAT(NY)-DEGFRE)                 3229
      IF (FLOAT(NY) .GT. DEGFRE) STDDEV=SQRT(DUB)                           3230
      IF (PRLDP) WRITE (NOUT,5320) ISTAR(LSTAR),DDDUM,DDUM,VARREG,VAR,      3231
     1 STDDEV,DEGFRE,PREJ                                                   3232
      IF (.NOT.PPLTPR) GO TO 390                                            3233
C-----------------------------------------------------------------------    3234
C  PUT UNSCALED SOLUTN INTO YLYFIT FOR PLOTTING AND MOMENTS                 3235
C-----------------------------------------------------------------------    3236
      DO 327 J=1,NGL                                                        3237
        YLYFIT(J)=SOLUTN(J)*SSCALE(J)                                       3238
  327 CONTINUE                                                              3239
      DOERR=STDDEV.LT.SRANGE .AND. DEGFRE.GT.0.                             3240
      NGLEY=MIN0(NGLE,NY)                                                   3241
      IF (.NOT.DOERR) GO TO 350                                             3242
C-----------------------------------------------------------------------    3243
C  PUT STANDARD ERRORS OF UNSCALED SOLUTION IN WORK.                        3244
C-----------------------------------------------------------------------    3245
      AASCMX=ZERO                                                           3246
      DO 330 J=1,NGL                                                        3247
        DUB=ZERO                                                            3248
        AAMAX=ZERO                                                          3249
        DO 335 K=1,NGLEY                                                    3250
          AAMAX=AMAX1(AAMAX,ABS(AA(J,K)))                                   3251
          DUB=DUB+AA(J,K)**2                                                3252
  335   CONTINUE                                                            3253
        WORK(J)=STDDEV*SQRT(DUB)*SSCALE(J)                                  3254
        AASCMX=AMAX1(AASCMX,AAMAX*SSCALE(J))                                3255
  330 CONTINUE                                                              3256
C-----------------------------------------------------------------------    3257
C  PUT SQRT(COVARIANCE MATRIX OF UNSCALED SOLUTION)/(AASCMX) IN AA,         3258
C      AND THEN SCALE STDDEV BY MULTIPLYING IT BY AASCMX.                   3259
C-----------------------------------------------------------------------    3260
      DO 340 J=1,NGL                                                        3261
        DUB=SSCALE(J)/AASCMX                                                3262
        DO 345 K=1,NGLEY                                                    3263
          AA(J,K)=AA(J,K)*DUB                                               3264
  345   CONTINUE                                                            3265
  340 CONTINUE                                                              3266
      STDDEV=STDDEV*AASCMX                                                  3267
  350 IF (IPRINT(ISTAGE) .GE. ILEVEL) CALL PLPRIN (G,YLYFIT,EXACT,NG,       3268
     1 ONLY1,NOUT,SRANGE,NLINF,NG,NGL,WORK,DOERR)                           3269
      IF (IUSROU(ISTAGE) .GE. ILEVEL) CALL USEROU (CQUAD,G,YLYFIT,          3270
     1 EXACT,AA,MG,STDDEV,DOERR,NGLEY)                                      3271
      IF (.NOT.DOMOM .OR. IPRINT(ISTAGE).LT.ILEVEL) GO TO 390               3272
      J=NGLEY**2+1                                                          3273
      L=MWORK-NGLEY                                                         3274
      K=L-NGLEY                                                             3275
      CALL MOMENT (G,YLYFIT,CQUAD,NG,                                       3276
     1 MOMNMX(1),MOMNMX(2),NOUT,AA,MG,STDDEV,DOERR,WORK,WORK(J),NGLEY,      3277
     2 MWORK,WORK(K),WORK(L),SRANGE,MPKMOM)                                 3278
  390 IF (.NOT.SSEARC .OR. VARZ.GE.SRANGE) GO TO 800                        3279
C-----------------------------------------------------------------------    3280
C  NORMALLY, RS2MNX(1) SHOULD BE SET IN FIRST SUCCESSFUL ANALYSIS,          3281
C      SINCE PREJ(1)=0 IN THAT CASE.  HOWEVER, IF DEGFRZ=NY, THEN           3282
C      PREJ(1)=1.  IN THIS CASE, RS2MNX(1) IS SET ANYWAY, AND RS2MNX(2)     3283
C      IS NOT SET.                                                          3284
C-----------------------------------------------------------------------    3285
      DDUM=ABS(PREJ(1)+ZERO)                                                3286
      IF (DDUM .LE. .01) RS2MNX(1)=ALPHA/(PRECIS*S(1,1))                    3287
      IF (PREJ(1).GE..995 .AND. RS2MNX(1).GT.0. .AND. RS2MNX(2).LE.0.)      3288
     1 RS2MNX(2)=ALPHA/S(1,1)                                               3289
      IF (RS2MNX(1) .LE. 0.) RS2MNX(1)=ALPHA/(PRECIS*S(1,1))                3290
      TEST=ABS(PREJ(IICRIT)+ZERO)-PLEVEL(IICRIT,ISTAGE)                     3291
C-----------------------------------------------------------------------    3292
C  SOLUTION IS TAKEN AS THE BEST SOLUTION SO FAR IF                         3293
C      PREJ.LT.PLEVEL OR IF (PREJ-PLEVEL) IS A MINIMUM SO FAR.              3294
C      HOWEVER, IF ALPHA.LT.ALPBES, THEN IT IS THE MINIMUM IN               3295
C      ABS(PREJ-PLEVEL) THAT IS SOUGHT.                                     3296
C-----------------------------------------------------------------------    3297
      DUB=TEST                                                              3298
      IF (ALPHA .LT. ALPBES) TEST=ABS(DUB)                                  3299
      IF (TEST .GE. BTEST) GO TO 800                                        3300
      BTEST=ABS(DUB)                                                        3301
      ALPBES=ALPHA                                                          3302
      DO 410 J=1,NGL                                                        3303
        SOLBES(J)=SOLUTN(J)                                                 3304
  410 CONTINUE                                                              3305
  800 RETURN                                                                3306
      END                                                                   3307
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    3308
C  SUBROUTINE LH1405.  PERFORMS LAWSON AND HANSON, P. 140, STEP 5.          3309
      SUBROUTINE LH1405 (F,M2,M1,E,ME,X)                                    3310
      DOUBLE PRECISION DUM, E, F, X                                         3311
      DIMENSION F(1), E(ME,M1), X(M1)                                       3312
      DO 110 I=1,M2                                                         3313
        DUM=F(I)                                                            3314
        DO 120 J=1,M1                                                       3315
          DUM=DUM-E(I,J)*X(J)                                               3316
  120   CONTINUE                                                            3317
        F(I)=DUM                                                            3318
  110 CONTINUE                                                              3319
      RETURN                                                                3320
      END                                                                   3321
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    3322
C  SUBROUTINE MOMENT.  COMPUTES MOMENTS OF Y(J), J=1,N ALONG X-AXIS.        3323
C  MOMENTS OF DEGREE IDEGMN THRU IDEGMX ARE COMPUTED.                       3324
C  COMPUTES MOMENTS OVER FULL RANGE J=1,N, AND ALSO OF THE FIRST MPKMOM     3325
C      PEAKS.  IF THERE ARE MORE THAN MPKMOM PEAKS, THEN PEAK MPKMOM        3326
C      AND ALL FOLLOWING PEAKS ARE CONSIDERED TOGETHER AS PEAK MPKMOM.      3327
C  CQUAD ARE THE QUADRATURE COEFFICIENTS FROM SETGRD.                       3328
C  RETURNS ARE MADE IF IDEGMX.LT.IDEGMN, OR IF X=0. IS TO BE                3329
C      RAISED TO A NEGATIVE POWER.                                          3330
C  ABS(Y(J)-Y(J+1)) IS CONSIDERED NEGLIGIBLE AND NOT USED TO                3331
C      DEFINE A PEAK IF IT IS LESS THAN RMIN*YMAX, WHERE YMAX IS THE        3332
C      MAXIMUM OF ABS(Y(J)) AND RMIN IS SET IN THE DATA STATEMENT BELOW.    3333
C-----------------------------------------------------------------------    3334
C  CALLS SUBPROGRAMS - ERRMES, MOMOUT                                       3335
C-----------------------------------------------------------------------    3336
      SUBROUTINE MOMENT (X,Y,CQUAD,N,IDEGMN,IDEGMX,NOUT,                    3337
     1 AA,MG,STDDEV,DOERR,W1,W2,NGLEY,MWORK,AMOM1,AMOM2,SRANGE,MPKMOM)      3338
      DOUBLE PRECISION AA, W1, W2, AMOM1, AMOM2                             3339
      LOGICAL DOERR                                                         3340
      DIMENSION X(N), Y(N), CQUAD(N), AA(MG,MG), W1(NGLEY,1), W2(1),        3341
     1 AMOM1(1), AMOM2(1)                                                   3342
      DIMENSION IHOLER(6)                                                   3343
      DATA RMIN/1.E-5/, IHOLER/1HM, 1HO, 1HM, 1HE, 1HN, 1HT/                3344
      NM=MIN0(MWORK/NGLEY-3,NGLEY,IDEGMX-IDEGMN+1)                          3345
      IDMAX=IDEGMN+NM-1                                                     3346
      IF (NM .LT. 1) RETURN                                                 3347
      IF (.NOT.DOERR) GO TO 105                                             3348
      IF (STDDEV .LT. 0.) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)                3349
C-----------------------------------------------------------------------    3350
C  TEMPORARILY SCALE THE X VALUES TO HELP AVOID OVERFLOW OR UNDERFLOW.      3351
C-----------------------------------------------------------------------    3352
  105 XMIN=SRANGE                                                           3353
      DO 110 J=1,N                                                          3354
        IF (ABS(X(J)) .GE. XMIN) GO TO 110                                  3355
        XMIN=ABS(X(J))                                                      3356
        L=J                                                                 3357
  110 CONTINUE                                                              3358
C-----------------------------------------------------------------------    3359
C  TAKE SECOND SMALLEST VALUE FOR MIN. IN CASE SMALLEST HAPPENS TO BE       3360
C      NEAR ZERO.                                                           3361
C-----------------------------------------------------------------------    3362
      IF (L .LT. N) XMIN=ABS(X(L+1))                                        3363
      IF (L .GT. 1) XMIN=AMIN1(XMIN,ABS(X(L-1)))                            3364
      IF (XMIN .LE. 0.) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                  3365
      L=1                                                                   3366
      IF (ABS(X(1)) .LT. ABS(X(N))) L=N                                     3367
      XMAX=ABS(X(L))                                                        3368
C-----------------------------------------------------------------------    3369
C  CENTER X ON LOG(X) AXIS.                                                 3370
C-----------------------------------------------------------------------    3371
      XSCALE=1./SQRT(XMIN*XMAX)                                             3372
C-----------------------------------------------------------------------    3373
C  GUARD AGAINST OVERFLOW OF MAXIMUM TERM.                                  3374
C-----------------------------------------------------------------------    3375
      IF (IDEGMN.EQ.0 .AND. NM.EQ.1) GO TO 120                              3376
      TMXLOG=(.5*ALOG(SRANGE)-ALOG(ABS(FLOAT(N)*CQUAD(L))*                  3377
     1 AMAX1(1.,ABS(Y(L)))))/                                               3378
     2 FLOAT(MAX0(IABS(IDEGMN),IABS(IDMAX)))-ALOG(ABS(X(L)))                3379
      IF (ALOG(XSCALE) .GT. TMXLOG) XSCALE=EXP(TMXLOG)                      3380
  120 XSCLOG=ALOG10(XSCALE)                                                 3381
      DO 130 J=1,N                                                          3382
        X(J)=X(J)*XSCALE                                                    3383
  130 CONTINUE                                                              3384
C-----------------------------------------------------------------------    3385
C  SET PEAK DETECTION THRESHOLD.                                            3386
C-----------------------------------------------------------------------    3387
      THRESH=0.                                                             3388
      DO 150 J=1,N                                                          3389
        THRESH=AMAX1(THRESH,ABS(Y(J)*CQUAD(J)))                             3390
  150 CONTINUE                                                              3391
      THRESH=RMIN*THRESH                                                    3392
C-----------------------------------------------------------------------    3393
C  START OF COMPUTATION OF MOMENTS.                                         3394
C-----------------------------------------------------------------------    3395
      DO 210 K=1,NM                                                         3396
        AMOM2(K)=0.                                                         3397
        IF (DOERR) W2(K)=0.                                                 3398
  210 CONTINUE                                                              3399
      JYOLD=1                                                               3400
      JY=0                                                                  3401
      DLAST=(Y(2)-Y(1))*(ABS(CQUAD(1))+ABS(CQUAD(2)))                       3402
      DO 220 JPEAK=1,N                                                      3403
        DO 230 K=1,NM                                                       3404
          AMOM1(K)=0.                                                       3405
          IF (.NOT.DOERR) GO TO 230                                         3406
          DO 240 J=1,NGLEY                                                  3407
            W1(J,K)=0.                                                      3408
  240     CONTINUE                                                          3409
  230   CONTINUE                                                            3410
  250   JY=JY+1                                                             3411
          IF (IDEGMN.GE.0 .OR. ABS(X(JY)).GT.0.) GO TO 255                  3412
          CALL ERRMES (3,.FALSE.,IHOLER,NOUT)                               3413
          RETURN                                                            3414
  255     TERM=CQUAD(JY)                                                    3415
          IF (IDEGMN .NE. 0) TERM=TERM*X(JY)**IDEGMN                        3416
          DO 260 JDEG=1,NM                                                  3417
            IF (.NOT.DOERR) GO TO 280                                       3418
            DO 270 J=1,NGLEY                                                3419
              W1(J,JDEG)=W1(J,JDEG)+TERM*AA(JY,J)                           3420
  270       CONTINUE                                                        3421
  280       DUM=TERM*Y(JY)                                                  3422
            AMOM1(JDEG)=AMOM1(JDEG)+DUM                                     3423
            AMOM2(JDEG)=AMOM2(JDEG)+DUM                                     3424
            IF (JDEG .LT. NM) TERM=TERM*X(JY)                               3425
  260     CONTINUE                                                          3426
          IF (JY .EQ. 1) GO TO 250                                          3427
          IF (JY .EQ. N) GO TO 300                                          3428
          DNEXT=(Y(JY+1)-Y(JY))*(ABS(CQUAD(JY))+ABS(CQUAD(JY+1)))           3429
          DUM=DLAST                                                         3430
          IF (ABS(DNEXT) .GT. THRESH) DLAST=DNEXT                           3431
          IF (DUM.GE.-THRESH .OR. DNEXT.LE.THRESH .OR. JPEAK.GE.MPKMOM)     3432
     1    GO TO 250                                                         3433
  300   DUM=X(JYOLD)/XSCALE                                                 3434
        DDUM=X(JY)/XSCALE                                                   3435
        WRITE (NOUT,5300) JPEAK,DUM,DDUM                                    3436
 5300   FORMAT (5H0PEAK,I2,10H GOES FROM,1PE11.3,3H TO,E11.3,               3437
     1   3X,1HJ,9X,9HMOMENT(J),8X,13HPERCENT ERROR,10X,11HM(J)/M(J-1),      3438
     2   3X,13HPERCENT ERROR,4X,1HJ)                                        3439
        CALL MOMOUT (.FALSE.,AMOM1,                                         3440
     1  DOERR,IDEGMN,NGLEY,NM,NOUT,STDDEV,W1,W2,XSCALE,XSCLOG)              3441
        JYOLD=JY+1                                                          3442
        IF (JY .LT. N) GO TO 220                                            3443
        IF (JPEAK .EQ. 1) GO TO 700                                         3444
        WRITE (NOUT,5310)                                                   3445
 5310   FORMAT (/16X,26HMOMENTS OF ENTIRE SOLUTION,                         3446
     1   3X,1HJ,9X,9HMOMENT(J),8X,13HPERCENT ERROR,10X,11HM(J)/M(J-1),      3447
     2   3X,13HPERCENT ERROR,4X,1HJ)                                        3448
        CALL MOMOUT (.TRUE.,AMOM2,                                          3449
     1  DOERR,IDEGMN,NGLEY,NM,NOUT,STDDEV,W1,W2,XSCALE,XSCLOG)              3450
        GO TO 700                                                           3451
  220 CONTINUE                                                              3452
C-----------------------------------------------------------------------    3453
C  RESTORE X TO THEIR UNSCALED VALUES.                                      3454
C-----------------------------------------------------------------------    3455
  700 DO 710 J=1,N                                                          3456
        X(J)=X(J)/XSCALE                                                    3457
  710 CONTINUE                                                              3458
      RETURN                                                                3459
      END                                                                   3460
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    3461
C  SUBROUTINE MOMOUT.  FOR OUTPUT OF MOMENTS.                               3462
C  FULLCV = .TRUE. IF MOMENTS FOR FULL CURVE ARE BEING OUTPUT.              3463
      SUBROUTINE MOMOUT (FULLCV,AMOM,                                       3464
     1  DOERR,IDEGMN,NGLEY,NM,NOUT,STDDEV,W1,W2,XSCALE,XSCLOG)              3465
      DOUBLE PRECISION AMOM, W1, W2                                         3466
      LOGICAL FULLCV, DOERR                                                 3467
      DIMENSION AMOM(1), W1(NGLEY,1), W2(1)                                 3468
      IDEG=IDEGMN-1                                                         3469
      DO 110 JDEG=1,NM                                                      3470
        IDEG=IDEG+1                                                         3471
        IF (JDEG .EQ. 1) GO TO 112                                          3472
        RMOM=0.                                                             3473
        SAMM1=AMOM(JDEG-1)                                                  3474
        IF (ABS(SAMM1) .GT. 0.) RMOM=AMOM(JDEG)/(AMOM(JDEG-1)*XSCALE)       3475
  112   SAM=AMOM(JDEG)                                                      3476
        IEXP=0                                                              3477
        AMANT=0.                                                            3478
        IF (ABS(SAM) .LE. 0.) GO TO 113                                     3479
        ANEWLG=ALOG10(ABS(SAM))-FLOAT(IDEG)*XSCLOG                          3480
        IEXP=INT(ANEWLG-.5+SIGN(.5,ANEWLG))                                 3481
        AMANT=SIGN(EXP(2.302585*(ANEWLG-FLOAT(IEXP))),SAM)                  3482
  113   PCNEW=0.                                                            3483
        PCRMOM=0.                                                           3484
        IF (.NOT.DOERR .OR. ABS(SAM).LE.0.) GO TO 130                       3485
        IF (.NOT.FULLCV) GO TO 115                                          3486
        ERRMOM=W2(JDEG)                                                     3487
        GO TO 125                                                           3488
  115   ERRMOM=0.                                                           3489
        DO 120 J=1,NGLEY                                                    3490
          ERRMOM=ERRMOM+W1(J,JDEG)**2                                       3491
  120   CONTINUE                                                            3492
        W2(JDEG)=W2(JDEG)+ERRMOM                                            3493
  125   PCNEW=100.*STDDEV*SQRT(ERRMOM)/ABS(SAM)                             3494
        IF (JDEG .GT. 1) PCRMOM=PCNEW+PCOLD                                 3495
  130   PCOLD=PCNEW                                                         3496
        IF (IDEG.NE.2 .OR. JDEG.LT.3) GO TO 140                             3497
        DUM1=AMOM(JDEG-1)                                                   3498
        IF (ABS(DUM1) .LE. 0.) GO TO 140                                    3499
        DUM=(AMOM(JDEG)/DUM1)*(AMOM(JDEG-2)/DUM1)-1.                        3500
        SDMEAN=SQRT(AMAX1(0.,DUM))                                          3501
 5125   FORMAT (15X,18H(STD. DEV.)/MEAN =,1PE9.1,I4,0PF10.4,                3502
     1  8H X (10**,I4,1H),1PE16.1,E21.4,E16.1,I5)                           3503
        WRITE (NOUT,5125) SDMEAN,IDEG,AMANT,IEXP,PCNEW,                     3504
     1  RMOM,PCRMOM,IDEG                                                    3505
        GO TO 110                                                           3506
  140   IF (JDEG .EQ. 1) WRITE (NOUT,5130) IDEG,AMANT,IEXP,PCNEW            3507
        IF (JDEG .GT. 1) WRITE (NOUT,5130) IDEG,AMANT,IEXP,PCNEW,           3508
     1  RMOM,PCRMOM,IDEG                                                    3509
 5130   FORMAT(1X,I45,F10.4,8H X (10**,I4,1H),1PE16.1,E21.4,E16.1,I5)       3510
  110 CONTINUE                                                              3511
      RETURN                                                                3512
      END                                                                   3513
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    3514
C     SUBROUTINE NNLS  (A,MDA,M,N,B,X,RNORM,W,ZZ,INDEX,MODE,RANGE)          3515
C     BASED ON C.L.LAWSON AND R.J.HANSON,                                   3516
C     'SOLVING LEAST SQUARES PROBLEMS', PRENTICE-HALL, 1974                 3517
C                                                                           3518
C         **********   NONNEGATIVE LEAST SQUARES   **********               3519
C                                                                           3520
C     GIVEN AN M BY N MATRIX, A, AND AN M-VECTOR, B,  COMPUTE AN            3521
C     N-VECTOR, X, WHICH SOLVES THE LEAST SQUARES PROBLEM                   3522
C                                                                           3523
C                      A * X = B  SUBJECT TO X .GE. 0                       3524
C                                                                           3525
C     A(),MDA,M,N     MDA IS THE FIRST DIMENSIONING PARAMETER FOR THE       3526
C                     ARRAY, A().   ON ENTRY A() CONTAINS THE M BY N        3527
C                     MATRIX, A.           ON EXIT A() CONTAINS             3528
C                     THE PRODUCT MATRIX, Q*A , WHERE Q IS AN               3529
C                     M BY M ORTHOGONAL MATRIX GENERATED IMPLICITLY BY      3530
C                     THIS SUBROUTINE.                                      3531
C     B()     ON ENTRY B() CONTAINS THE M-VECTOR, B.   ON EXIT B() CON-     3532
C             TAINS Q*B.                                                    3533
C     X()     ON ENTRY X() NEED NOT BE INITIALIZED.  ON EXIT X() WILL       3534
C             CONTAIN THE SOLUTION VECTOR.                                  3535
C     RNORM   ON EXIT RNORM CONTAINS THE EUCLIDEAN NORM OF THE              3536
C             RESIDUAL VECTOR.                                              3537
C     W()     AN N-ARRAY OF WORKING SPACE.  ON EXIT W() WILL CONTAIN        3538
C             THE DUAL SOLUTION VECTOR.   W WILL SATISFY W(I) = 0.          3539
C             FOR ALL I IN SET P  AND W(I) .LE. 0. FOR ALL I IN SET Z       3540
C     ZZ()     AN M-ARRAY OF WORKING SPACE.                                 3541
C     INDEX()     AN INTEGER WORKING ARRAY OF LENGTH AT LEAST N.            3542
C                 ON EXIT THE CONTENTS OF THIS ARRAY DEFINE THE SETS        3543
C                 P AND Z AS FOLLOWS..                                      3544
C                                                                           3545
C                 INDEX(1)   THRU INDEX(NSETP) = SET P.                     3546
C                 INDEX(IZ1) THRU INDEX(IZ2)   = SET Z.                     3547
C                 IZ1 = NSETP + 1 = NPP1                                    3548
C                 IZ2 = N                                                   3549
C     MODE    THIS IS A SUCCESS-FAILURE FLAG WITH THE FOLLOWING             3550
C             MEANINGS.                                                     3551
C             1     THE SOLUTION HAS BEEN COMPUTED SUCCESSFULLY.            3552
C             2     THE DIMENSIONS OF THE PROBLEM ARE BAD.                  3553
C                   EITHER M .LE. 0 OR N .LE. 0.                            3554
C             3    ITERATION COUNT EXCEEDED.  MORE THAN 3*N ITERATIONS.     3555
C                                                                           3556
C  RANGE IS 2 OR 3 ORDERS OF MAGNITUDE SMALLER THAN BIG, WHERE BIG IS       3557
C      THE LARGEST NUMBER THAT DOES NOT OVERFLOW AND 1/BIG DOES NOT         3558
C      UNDERFLOW.  FOR THE DOUBLE PRECISION VERSION, BIG AND RANGE          3559
C      ARE IN DOUBLE PRECISION.  FOR THE SINGLE PRECISION VERSION,          3560
C      THEY ARE IN SINGLE PRECISION (AND THEREFORE RANGE=SRANGE).           3561
C-----------------------------------------------------------------------    3562
C  CALLS SUBPROGRAMS - DIFF, H12, G1, G2                                    3563
C-----------------------------------------------------------------------    3564
      SUBROUTINE NNLS (A,MDA,M,N,B,X,RNORM,W,ZZ,INDEX,MODE,RANGE)           3565
      DOUBLE PRECISION A, ABS, ALPHA, ASAVE, B, CC, DIFF, DUMMY,            3566
     1 FACTOR, RANGE, RNORM, SM, SQRT, SS, T, TWO, UNORM, UP, W,            3567
     2 WMAX, X, ZERO, ZTEST, ZZ                                             3568
      DIMENSION A(MDA,N), B(1), X(1), W(1), ZZ(1)                           3569
      INTEGER INDEX(N)                                                      3570
      ABS(T)=DABS(T)                                                        3571
      SQRT(T)=DSQRT(T)                                                      3572
C     ZERO=0.E0!SP                                                          3573
      ZERO=0.D0                                                             3574
C     TWO=2.E0!SP                                                           3575
      TWO=2.D0                                                              3576
C     FACTOR=1.E-2!SP                                                       3577
      FACTOR=1.D-4                                                          3578
C                                                                           3579
      MODE=1                                                                3580
      IF (M.GT.0.AND.N.GT.0) GO TO 10                                       3581
      MODE=2                                                                3582
      RETURN                                                                3583
   10 ITER=0                                                                3584
      ITMAX=3*N                                                             3585
C                                                                           3586
C                    INITIALIZE THE ARRAYS INDEX() AND X().                 3587
C                                                                           3588
          DO 20 I=1,N                                                       3589
          X(I)=ZERO                                                         3590
   20     INDEX(I)=I                                                        3591
C                                                                           3592
      IZ2=N                                                                 3593
      IZ1=1                                                                 3594
      NSETP=0                                                               3595
      NPP1=1                                                                3596
C                             ******  MAIN LOOP BEGINS HERE  ******         3597
   30 CONTINUE                                                              3598
C                  QUIT IF ALL COEFFICIENTS ARE ALREADY IN THE SOLUTION.    3599
C                        OR IF M COLS OF A HAVE BEEN TRIANGULARIZED.        3600
C                                                                           3601
      IF (IZ1.GT.IZ2.OR.NSETP.GE.M) GO TO 350                               3602
C                                                                           3603
C         COMPUTE COMPONENTS OF THE DUAL (NEGATIVE GRADIENT) VECTOR W().    3604
C                                                                           3605
          DO 50 IZ=IZ1,IZ2                                                  3606
          J=INDEX(IZ)                                                       3607
          SM=ZERO                                                           3608
              DO 40 L=NPP1,M                                                3609
   40         SM=SM+A(L,J)*B(L)                                             3610
   50     W(J)=SM                                                           3611
C                                   FIND LARGEST POSITIVE W(J).             3612
   60 WMAX=ZERO                                                             3613
          DO 70 IZ=IZ1,IZ2                                                  3614
          J=INDEX(IZ)                                                       3615
          IF (W(J).LE.WMAX) GO TO 70                                        3616
          WMAX=W(J)                                                         3617
          IZMAX=IZ                                                          3618
   70     CONTINUE                                                          3619
C                                                                           3620
C             IF WMAX .LE. 0. GO TO TERMINATION.                            3621
C             THIS INDICATES SATISFACTION OF THE KUHN-TUCKER CONDITIONS.    3622
C                                                                           3623
      IF (WMAX) 350,350,80                                                  3624
   80 IZ=IZMAX                                                              3625
      J=INDEX(IZ)                                                           3626
C                                                                           3627
C     THE SIGN OF W(J) IS OK FOR J TO BE MOVED TO SET P.                    3628
C     BEGIN THE TRANSFORMATION AND CHECK NEW DIAGONAL ELEMENT TO AVOID      3629
C     NEAR LINEAR DEPENDENCE.                                               3630
C                                                                           3631
      ASAVE=A(NPP1,J)                                                       3632
      CALL H12 (1,NPP1,NPP1+1,M,A(1,J),1,UP,DUMMY,1,1,0,RANGE)              3633
      UNORM=ZERO                                                            3634
      IF (NSETP.EQ.0) GO TO 100                                             3635
          DO 90 L=1,NSETP                                                   3636
   90     UNORM=UNORM+A(L,J)**2                                             3637
      UNORM=SQRT(UNORM)                                                     3638
  100 IF (DIFF(UNORM+ABS(A(NPP1,J))*FACTOR,UNORM)) 130,130,110              3639
C                                                                           3640
C     COL J IS SUFFICIENTLY INDEPENDENT.  COPY B INTO ZZ, UPDATE ZZ AND     3641
C   = SOLVE FOR ZTEST ( = PROPOSED NEW VALUE FOR X(J) ).                    3642
C                                                                           3643
  110     DO 120 L=1,M                                                      3644
  120     ZZ(L)=B(L)                                                        3645
      CALL H12 (2,NPP1,NPP1+1,M,A(1,J),1,UP,ZZ,1,1,1,RANGE)                 3646
      ZTEST=ZZ(NPP1)/A(NPP1,J)                                              3647
C                                                                           3648
C                                     SEE IF ZTEST IS POSITIVE              3649
C     REJECT J AS A CANDIDATE TO BE MOVED FROM SET Z TO SET P.              3650
C     RESTORE A(NPP1,J), SET W(J)=0., AND LOOP BACK TO TEST DUAL            3651
C                                                                           3652
      IF (ZTEST) 130,130,140                                                3653
C                                                                           3654
C     COEFFS AGAIN.                                                         3655
C                                                                           3656
  130 A(NPP1,J)=ASAVE                                                       3657
      W(J)=ZERO                                                             3658
      GO TO 60                                                              3659
C                                                                           3660
C     THE INDEX  J=INDEX(IZ)  HAS BEEN SELECTED TO BE MOVED FROM            3661
C     SET Z TO SET P.    UPDATE B,  UPDATE INDICES,  APPLY HOUSEHOLDER      3662
C     TRANSFORMATIONS TO COLS IN NEW SET Z,  ZERO SUBDIAGONAL ELTS IN       3663
C     COL J,  SET W(J)=0.                                                   3664
C                                                                           3665
  140     DO 150 L=1,M                                                      3666
  150     B(L)=ZZ(L)                                                        3667
C                                                                           3668
      INDEX(IZ)=INDEX(IZ1)                                                  3669
      INDEX(IZ1)=J                                                          3670
      IZ1=IZ1+1                                                             3671
      NSETP=NPP1                                                            3672
      NPP1=NPP1+1                                                           3673
C                                                                           3674
      IF (IZ1.GT.IZ2) GO TO 170                                             3675
          DO 160 JZ=IZ1,IZ2                                                 3676
          JJ=INDEX(JZ)                                                      3677
  160     CALL H12 (2,NSETP,NPP1,M,A(1,J),1,UP,A(1,JJ),1,MDA,1,RANGE)       3678
  170 CONTINUE                                                              3679
C                                                                           3680
      IF (NSETP.EQ.M) GO TO 190                                             3681
          DO 180 L=NPP1,M                                                   3682
  180     A(L,J)=ZERO                                                       3683
  190 CONTINUE                                                              3684
C                                                                           3685
      W(J)=ZERO                                                             3686
C                                SOLVE THE TRIANGULAR SYSTEM.               3687
C                                STORE THE SOLUTION TEMPORARILY IN ZZ().    3688
      ASSIGN 200 TO NEXT                                                    3689
      GO TO 400                                                             3690
  200 CONTINUE                                                              3691
C                                                                           3692
C                       ******  SECONDARY LOOP BEGINS HERE ******           3693
C                                                                           3694
C                          ITERATION COUNTER.                               3695
C                                                                           3696
  210 ITER=ITER+1                                                           3697
      IF (ITER.LE.ITMAX) GO TO 220                                          3698
      MODE=3                                                                3699
      GO TO 350                                                             3700
  220 CONTINUE                                                              3701
C                                                                           3702
C                    SEE IF ALL NEW CONSTRAINED COEFFS ARE FEASIBLE.        3703
C                                  IF NOT COMPUTE ALPHA.                    3704
C                                                                           3705
      ALPHA=TWO                                                             3706
          DO 240 IP=1,NSETP                                                 3707
          L=INDEX(IP)                                                       3708
          IF (ZZ(IP)) 230,230,240                                           3709
C                                                                           3710
  230     T=-X(L)/(ZZ(IP)-X(L))                                             3711
          IF (ALPHA.LE.T) GO TO 240                                         3712
          ALPHA=T                                                           3713
          JJ=IP                                                             3714
  240     CONTINUE                                                          3715
C                                                                           3716
C          IF ALL NEW CONSTRAINED COEFFS ARE FEASIBLE THEN ALPHA WILL       3717
C          STILL = 2.    IF SO EXIT FROM SECONDARY LOOP TO MAIN LOOP.       3718
C                                                                           3719
      IF (ALPHA.EQ.TWO) GO TO 330                                           3720
C                                                                           3721
C          OTHERWISE USE ALPHA WHICH WILL BE BETWEEN 0. AND 1. TO           3722
C          INTERPOLATE BETWEEN THE OLD X AND THE NEW ZZ.                    3723
C                                                                           3724
          DO 250 IP=1,NSETP                                                 3725
          L=INDEX(IP)                                                       3726
  250     X(L)=X(L)+ALPHA*(ZZ(IP)-X(L))                                     3727
C                                                                           3728
C        MODIFY A AND B AND THE INDEX ARRAYS TO MOVE COEFFICIENT I          3729
C        FROM SET P TO SET Z.                                               3730
C                                                                           3731
      I=INDEX(JJ)                                                           3732
  260 X(I)=ZERO                                                             3733
C                                                                           3734
      IF (JJ.EQ.NSETP) GO TO 290                                            3735
      JJ=JJ+1                                                               3736
          DO 280 J=JJ,NSETP                                                 3737
          II=INDEX(J)                                                       3738
          INDEX(J-1)=II                                                     3739
          CALL G1 (A(J-1,II),A(J,II),CC,SS,A(J-1,II))                       3740
          A(J,II)=ZERO                                                      3741
              DO 270 L=1,N                                                  3742
              IF (L.NE.II) CALL G2 (CC,SS,A(J-1,L),A(J,L))                  3743
  270         CONTINUE                                                      3744
  280     CALL G2 (CC,SS,B(J-1),B(J))                                       3745
  290 NPP1=NSETP                                                            3746
      NSETP=NSETP-1                                                         3747
      IZ1=IZ1-1                                                             3748
      INDEX(IZ1)=I                                                          3749
C                                                                           3750
C        SEE IF THE REMAINING COEFFS IN SET P ARE FEASIBLE.  THEY SHOULD    3751
C        BE BECAUSE OF THE WAY ALPHA WAS DETERMINED.                        3752
C        IF ANY ARE INFEASIBLE IT IS DUE TO ROUND-OFF ERROR.  ANY           3753
C        THAT ARE NONPOSITIVE WILL BE SET TO ZERO                           3754
C        AND MOVED FROM SET P TO SET Z.                                     3755
C                                                                           3756
          DO 300 JJ=1,NSETP                                                 3757
          I=INDEX(JJ)                                                       3758
          IF (X(I)) 260,260,300                                             3759
  300     CONTINUE                                                          3760
C                                                                           3761
C         COPY B( ) INTO ZZ( ).  THEN SOLVE AGAIN AND LOOP BACK.            3762
C                                                                           3763
          DO 310 I=1,M                                                      3764
  310     ZZ(I)=B(I)                                                        3765
      ASSIGN 320 TO NEXT                                                    3766
      GO TO 400                                                             3767
  320 CONTINUE                                                              3768
      GO TO 210                                                             3769
C                      ******  END OF SECONDARY LOOP  ******                3770
C                                                                           3771
  330     DO 340 IP=1,NSETP                                                 3772
          I=INDEX(IP)                                                       3773
  340     X(I)=ZZ(IP)                                                       3774
C        ALL NEW COEFFS ARE POSITIVE.  LOOP BACK TO BEGINNING.              3775
      GO TO 30                                                              3776
C                                                                           3777
C                        ******  END OF MAIN LOOP  ******                   3778
C                                                                           3779
C                        COME TO HERE FOR TERMINATION.                      3780
C                     COMPUTE THE NORM OF THE FINAL RESIDUAL VECTOR.        3781
C                                                                           3782
  350 SM=ZERO                                                               3783
      IF (NPP1.GT.M) GO TO 370                                              3784
          DO 360 I=NPP1,M                                                   3785
  360     SM=SM+B(I)**2                                                     3786
      GO TO 390                                                             3787
  370     DO 380 J=1,N                                                      3788
  380     W(J)=ZERO                                                         3789
  390 RNORM=SQRT(SM)                                                        3790
      RETURN                                                                3791
C                                                                           3792
C     THE FOLLOWING BLOCK OF CODE IS USED AS AN INTERNAL SUBROUTINE         3793
C     TO SOLVE THE TRIANGULAR SYSTEM, PUTTING THE SOLUTION IN ZZ().         3794
C                                                                           3795
  400     DO 430 L=1,NSETP                                                  3796
          IP=NSETP+1-L                                                      3797
          IF (L.EQ.1) GO TO 420                                             3798
              DO 410 II=1,IP                                                3799
  410         ZZ(II)=ZZ(II)-A(II,JJ)*ZZ(IP+1)                               3800
  420     JJ=INDEX(IP)                                                      3801
  430     ZZ(IP)=ZZ(IP)/A(IP,JJ)                                            3802
      GO TO NEXT, (200,320)                                                 3803
      END                                                                   3804
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    3805
C  FUNCTION PGAUSS.  CALCULATES AREA UNDER NORMAL CURVE (CENTERED           3806
C      AT 0 WITH UNIT VARIANCE) FROM -(INFINITY) TO X USING                 3807
C      ABRAMOWITZ AND STEGUN, EQ. 26.2.18.                                  3808
C  ABS(ERROR) .LT. 2.5E-4                                                   3809
      FUNCTION PGAUSS (X)                                                   3810
      AX=ABS(X)                                                             3811
      PGAUSS=1.+AX*(.196854+AX*(.115194+AX*(3.44E-4+AX*.019527)))           3812
      PGAUSS=1./PGAUSS**2                                                   3813
      PGAUSS=.5*PGAUSS**2                                                   3814
      IF (X .GT. 0.) PGAUSS=1.-PGAUSS                                       3815
      RETURN                                                                3816
      END                                                                   3817
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    3818
C  SUBROUTINE PLPRIN.  PLOTS Y1 (AND Y2, IF ONLY1=.FALSE.) VS. X ON         3819
C      LINE PRINTER, AND PRINTS Y1 AND X.                                   3820
C  IF PLTERR=T, AN ERROR BAND IS ALSO PLOTTED USING THE ERRORS              3821
C      SUPPLIED IN YERR.                                                    3822
C  IF NLINF.GT.0, THEN THE NLINF LINEAR COEFFICIENTS Y(J), J=NG+1,MY1       3823
C      ARE PRINTED OUT.  IN THIS CASE MY1=NGL NORMALLY.                     3824
      SUBROUTINE PLPRIN (X,Y1,Y2,N,ONLY1,NOUT,SRANGE,NLINF,NG,MY1,          3825
     1 YERR, PLTERR)                                                        3826
      DOUBLE PRECISION YERR, DUB                                            3827
      LOGICAL ONLY1, PLTERR                                                 3828
      DIMENSION X(1), Y1(1), Y2(1), ICHAR(5), IH(109), YERR(1)              3829
      DATA ICHAR/1H , 1HX, 1HO, 1H*, 1H./                                   3830
C     SINGLE(DUB)=DUB!SP                                                    3831
      SINGLE(DUB)=SNGL(DUB)                                                 3832
      DUB=1.D0                                                              3833
      YMIN=SRANGE                                                           3834
      YMAX=-SRANGE                                                          3835
      DO 120 J=1,N                                                          3836
        YMIN=AMIN1(YMIN,Y1(J))                                              3837
        YMAX=AMAX1(YMAX,Y1(J))                                              3838
        IF (ONLY1) GO TO 120                                                3839
        YMIN=AMIN1(YMIN,Y2(J))                                              3840
        YMAX=AMAX1(YMAX,Y2(J))                                              3841
  120 CONTINUE                                                              3842
      DUM=YMAX-YMIN                                                         3843
      NCHAR=109                                                             3844
      IF (DUM .LE. FLOAT(NCHAR)/SRANGE) DUM=1.                              3845
      IF (PLTERR) GO TO 130                                                 3846
      WRITE (NOUT,5120)                                                     3847
 5120 FORMAT (/4X,8HORDINATE,2X,8HABSCISSA)                                 3848
      GO TO 140                                                             3849
  130 NCHAR=100                                                             3850
 5130 FORMAT (/4X,8HORDINATE,4X,5HERROR,2X,8HABSCISSA)                      3851
      WRITE (NOUT,5130)                                                     3852
  140 R=(FLOAT(NCHAR)-.001)/DUM                                             3853
      DO 150 J=1,N                                                          3854
        DO 155 L1=1,NCHAR                                                   3855
          IH(L1)=ICHAR(1)                                                   3856
  155   CONTINUE                                                            3857
        IF (.NOT.PLTERR) GO TO 158                                          3858
        LMIN=INT((AMAX1(YMIN,Y1(J)-ABS(SINGLE(YERR(J))))-YMIN)*R)+1         3859
        LMAX=INT((AMIN1(YMAX,Y1(J)+ABS(SINGLE(YERR(J))))-YMIN)*R)+1         3860
        IF (LMIN .GE. LMAX) GO TO 158                                       3861
        DO 156 L1=LMIN,LMAX                                                 3862
          IH(L1)=ICHAR(5)                                                   3863
  156   CONTINUE                                                            3864
  158   L1=INT((Y1(J)-YMIN)*R)+1                                            3865
        IH(L1)=ICHAR(2)                                                     3866
        IF (ONLY1) GO TO 160                                                3867
        L2=INT((Y2(J)-YMIN)*R)+1                                            3868
        IH(L2)=ICHAR(3)                                                     3869
        IF (L1 .EQ. L2) IH(L2)=ICHAR(4)                                     3870
  160   IF (.NOT.PLTERR) WRITE (NOUT,5160) Y1(J),X(J),IH                    3871
 5160   FORMAT (1X,1PE11.3,E10.2,109A1)                                     3872
        IF (PLTERR) WRITE (NOUT,5161) Y1(J),YERR(J),X(J),                   3873
     1  (IH(L1),L1=1,NCHAR)                                                 3874
C5161   FORMAT (1X,1PE11.3,E9.1,E10.2,100A1)!SP                             3875
 5161   FORMAT (1X,1PE11.3,D9.1,E10.2,100A1)                                3876
  150 CONTINUE                                                              3877
      IF (NLINF .LE. 0) GO TO 800                                           3878
      L2=NG+1                                                               3879
 5200 FORMAT (22H0LINEAR COEFFICIENTS =,1P8E13.4/(22X,8E13.4))              3880
      IF (.NOT.PLTERR) WRITE (NOUT,5200) (Y1(J),J=L2,MY1)                   3881
      IF (PLTERR) WRITE(NOUT,5201) (Y1(J),YERR(J),J=L2,MY1)                 3882
C5201 FORMAT (22H0LINEAR COEFFICIENTS =,!SP                                 3883
C    1 1PE13.4,3H +-,E9.1,E20.4,3H +-,E9.1,E20.4,3H +-,E9.1/!SP             3884
C    2 (22X,1PE13.4,3H +-,E9.1,E20.4,3H +-,E9.1,E20.4,3H +-,E9.1))!SP       3885
 5201 FORMAT (22H0LINEAR COEFFICIENTS =,                                    3886
     1 1PE13.4,3H +-,D9.1,E20.4,3H +-,D9.1,E20.4,3H +-,D9.1/                3887
     2 (22X,1PE13.4,3H +-,D9.1,E20.4,3H +-,D9.1,E20.4,3H +-,D9.1))          3888
  800 RETURN                                                                3889
      END                                                                   3890
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    3891
C  SUBROUTINE PLRES. PLOTS RESIDUALS ON PRINTER WITH X-AXIS HORIZONTAL.     3892
C-----------------------------------------------------------------------    3893
C  CALLS SUBPROGRAMS - ERRMES                                               3894
C-----------------------------------------------------------------------    3895
      SUBROUTINE PLRES (YLYFIT,NMAX,N,PRUNS,PUNCOR,RALPS1,NOUT,LINEPG,      3896
     1 ITITLE,CHOSEN)                                                       3897
      LOGICAL CHOSEN                                                        3898
      DIMENSION YLYFIT(NMAX), JCHAR(8), LINE(131), LABEL(6), BOUND(21),     3899
     1 LCHARJ(20), LINE1(20), IHOLER(6), ITITLE(80), PUNCOR(5)              3900
      DATA JCHAR/1H*, 1H-, 1HU, 1HL, 1H , 1H0, 1H-, 1H+/,                   3901
     1 IHOLER/1HP, 1HL, 1HR, 1HE, 1HS, 1H /, MPAGE/30/                      3902
      IF (LINEPG .GE. 17) GO TO 100                                         3903
      CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                                   3904
      RETURN                                                                3905
  100 MPLOT=(LINEPG-3)/16                                                   3906
      MLINE=MIN0(20,(LINEPG-3)/MPLOT-3)                                     3907
      RMIN=YLYFIT(1)                                                        3908
      RMAX=YLYFIT(1)                                                        3909
      DO 110 J=2,N                                                          3910
      RMIN=AMIN1(RMIN,YLYFIT(J))                                            3911
  110 RMAX=AMAX1(RMAX,YLYFIT(J))                                            3912
 5110 FORMAT (1H1,9X,80A1)                                                  3913
      IF (.NOT.CHOSEN) WRITE(NOUT,5110) ITITLE                              3914
 5200 FORMAT (/                                                             3915
     1 32H WEIGHTED RESIDUALS (ALPHA/S(1)=,                                 3916
     2 1PE9.2,8H) MAX=U=,E8.1,2X,6HMIN=L=,E8.1,1X,                          3917
     3 7H(PRUNS=,0PF7.4,9H) PUNCOR=,5F7.4)                                  3918
      WRITE (NOUT,5200) RALPS1,RMAX,RMIN,PRUNS,PUNCOR                       3919
      DELTA=(RMAX-RMIN)/FLOAT(MLINE-1)                                      3920
      IF (DELTA .GT. 0.) GO TO 115                                          3921
 5115 FORMAT (30H0RESIDUALS ALL EQUAL IN PLRES.,2X,50(2H**))                3922
      WRITE (NOUT,5115)                                                     3923
      RETURN                                                                3924
  115 IF (RMAX .LE. 0.) JAXIS=1                                             3925
      IF (RMIN .GE. 0.) JAXIS=MLINE                                         3926
      BOUND(1)=RMAX+.5*DELTA                                                3927
      K=MLINE+1                                                             3928
      DO 120 J=2,K                                                          3929
      BOUND(J)=BOUND(J-1)-DELTA                                             3930
  120 IF (BOUND(J)*BOUND(J-1) .LT. 0.) JAXIS=J-1                            3931
      LABEL(1)=-110                                                         3932
      DO 130 J=2,6                                                          3933
  130 LABEL(J)=LABEL(J-1)+20                                                3934
      K=MLINE-1                                                             3935
      DO 140 J=2,K                                                          3936
      LINE1(J)=JCHAR(5)                                                     3937
  140 LCHARJ(J)=5                                                           3938
      LINE1(1)=JCHAR(3)                                                     3939
      LINE1(JAXIS)=JCHAR(6)                                                 3940
      LINE1(MLINE)=JCHAR(4)                                                 3941
      LCHARJ(1)=7                                                           3942
      LCHARJ(JAXIS)=2                                                       3943
      LCHARJ(MLINE)=7                                                       3944
      NPOINT=0                                                              3945
      DO 200 NPAGE=1,MPAGE                                                  3946
      IF (NPAGE .GT. 1) WRITE(  NOUT,5999)                                  3947
 5999 FORMAT (1H1)                                                          3948
      DO 210 NPLOT=1,MPLOT                                                  3949
 5001 FORMAT (1H )                                                          3950
      WRITE (NOUT,5001)                                                     3951
      NST=NPOINT+1                                                          3952
      NEND=NPOINT+130                                                       3953
      NPOINT=NEND                                                           3954
      NLIM=MIN0(NEND,N)                                                     3955
      DO 220 NLINE=1,MLINE                                                  3956
      LCHAR=LCHARJ(NLINE)                                                   3957
      LINE(1)=LINE1(NLINE)                                                  3958
      BMAX=BOUND(NLINE)                                                     3959
      BMIN=BOUND(NLINE+1)                                                   3960
      K=1                                                                   3961
      DO 230 J=NST,NLIM                                                     3962
      K=K+1                                                                 3963
      LINE(K)=JCHAR(LCHAR)                                                  3964
      IF (YLYFIT(J).LT.BMAX .AND. YLYFIT(J).GE.BMIN) LINE(K)=JCHAR(1)       3965
  230 CONTINUE                                                              3966
      K=NLIM-NST+2                                                          3967
      IF (NLINE.NE.1 .AND. NLINE.NE.MLINE) GO TO 235                        3968
      DO 232 J=11,K,10                                                      3969
  232 IF (LINE(J) .NE. JCHAR(1)) LINE(J)=JCHAR(8)                           3970
 5230 FORMAT (1X,A1,130A1)                                                  3971
  235 WRITE (  NOUT,5230) (LINE(J),J=1,K)                                   3972
  220 CONTINUE                                                              3973
      DO 240 J=1,6                                                          3974
  240 LABEL(J)=LABEL(J)+130                                                 3975
 5240 FORMAT (3X,6(16X,I4)/)                                                3976
      WRITE (  NOUT,5240) LABEL                                             3977
      IF (NLIM .EQ. N) RETURN                                               3978
  210 CONTINUE                                                              3979
  200 CONTINUE                                                              3980
      CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                                   3981
      RETURN                                                                3982
      END                                                                   3983
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    3984
C     SUBROUTINE QRBD (IPASS,Q,E,NN,V,MDV,NRV,C,MDC,NCC,RANGE)              3985
C     BASED ON C.L.LAWSON AND R.J.HANSON,                                   3986
C     'SOLVING LEAST SQUARES PROBLEMS', PRENTICE-HALL, 1974                 3987
C          QR ALGORITHM FOR SINGULAR VALUES OF A BIDIAGONAL MATRIX.         3988
C                                                                           3989
C     THE BIDIAGONAL MATRIX                                                 3990
C                                                                           3991
C                       (Q1,E2,0...    )                                    3992
C                       (   Q2,E3,0... )                                    3993
C                D=     (       .      )                                    3994
C                       (         .   0)                                    3995
C                       (           .EN)                                    3996
C                       (          0,QN)                                    3997
C                                                                           3998
C                 IS PRE AND POST MULTIPLIED BY                             3999
C                 ELEMENTARY ROTATION MATRICES                              4000
C                 RI AND PI SO THAT                                         4001
C                                                                           4002
C                 RK...R1*D*P1**(T)...PK**(T) = DIAG(S1,...,SN)             4003
C                                                                           4004
C                 TO WITHIN WORKING ACCURACY.                               4005
C                                                                           4006
C  1. EI AND QI OCCUPY E(I) AND Q(I) AS INPUT.                              4007
C                                                                           4008
C  2. RM...R1*C REPLACES 'C' IN STORAGE AS OUTPUT.                          4009
C                                                                           4010
C  3. V*P1**(T)...PM**(T) REPLACES 'V' IN STORAGE AS OUTPUT.                4011
C                                                                           4012
C  4. SI OCCUPIES Q(I) AS OUTPUT.                                           4013
C                                                                           4014
C  5. THE SI'S ARE NONINCREASING AND NONNEGATIVE.                           4015
C                                                                           4016
C     THIS CODE IS BASED ON THE PAPER AND 'ALGOL' CODE..                    4017
C REF..                                                                     4018
C  1. REINSCH,C.H. AND GOLUB,G.H. 'SINGULAR VALUE DECOMPOSITION             4019
C     AND LEAST SQUARES SOLUTIONS' (NUMER. MATH.), VOL. 14,(1970).          4020
C                                                                           4021
C  RANGE IS 2 OR 3 ORDERS OF MAGNITUDE SMALLER THAN BIG, WHERE BIG IS       4022
C      THE LARGEST NUMBER THAT DOES NOT OVERFLOW AND 1/BIG DOES NOT         4023
C      UNDERFLOW.  FOR THE DOUBLE PRECISION VERSION, BIG AND RANGE          4024
C      ARE IN DOUBLE PRECISION.  FOR THE SINGLE PRECISION VERSION,          4025
C      THEY ARE IN SINGLE PRECISION (AND THEREFORE RANGE=SRANGE).           4026
C-----------------------------------------------------------------------    4027
C  CALLS SUBPROGRAMS - G1, G2, DIFF                                         4028
C-----------------------------------------------------------------------    4029
      SUBROUTINE QRBD (IPASS,Q,E,NN,V,MDV,NRV,C,MDC,NCC,RANGE)              4030
      DOUBLE PRECISION ABS, AMAX1, C, CS, DENOM, DIFF, DNORM, E, F,         4031
     A G, H, ONE, Q, RANGE,                                                 4032
     1 RNUMER, SN, SQRT, SQRTRG, T, TWO, V, X, Y, Z, ZERO                   4033
      LOGICAL WNTV, HAVERS, FAIL, LDUM                                      4034
      DIMENSION Q(NN),E(NN),V(MDV,1),C(MDC,1)                               4035
      AMAX1(ZERO,ONE)=DMAX1(ZERO,ONE)                                       4036
      ABS(ONE)=DABS(ONE)                                                    4037
      SQRT(ONE)=DSQRT(ONE)                                                  4038
C     ZERO=0.E0!SP                                                          4039
      ZERO=0.D0                                                             4040
C     ONE=1.E0!SP                                                           4041
      ONE=1.D0                                                              4042
C     TWO=2.E0!SP                                                           4043
      TWO=2.D0                                                              4044
      SQRTRG=SQRT(RANGE)                                                    4045
C                                                                           4046
      N=NN                                                                  4047
      IPASS=1                                                               4048
      IF (N.LE.0) RETURN                                                    4049
      N10=10*N                                                              4050
      WNTV=NRV.GT.0                                                         4051
      HAVERS=NCC.GT.0                                                       4052
      FAIL=.FALSE.                                                          4053
      NQRS=0                                                                4054
      E(1)=ZERO                                                             4055
      DNORM=ZERO                                                            4056
           DO 10 J=1,N                                                      4057
   10      DNORM=AMAX1(ABS(Q(J))+ABS(E(J)),DNORM)                           4058
           DO 200 KK=1,N                                                    4059
           K=N+1-KK                                                         4060
C                                                                           4061
C     TEST FOR SPLITTING OR RANK DEFICIENCIES..                             4062
C         FIRST MAKE TEST FOR LAST DIAGONAL TERM, Q(K), BEING SMALL.        4063
   20       IF(K.EQ.1) GO TO 50                                             4064
            IF(DIFF(DNORM+Q(K),DNORM)) 50,25,50                             4065
C                                                                           4066
C     SINCE Q(K) IS SMALL WE WILL MAKE A SPECIAL PASS TO                    4067
C     TRANSFORM E(K) TO ZERO.                                               4068
C                                                                           4069
   25      CS=ZERO                                                          4070
           SN=-ONE                                                          4071
                DO 40 II=2,K                                                4072
                I=K+1-II                                                    4073
                F=-SN*E(I+1)                                                4074
                E(I+1)=CS*E(I+1)                                            4075
                CALL G1 (Q(I),F,CS,SN,Q(I))                                 4076
C         TRANSFORMATION CONSTRUCTED TO ZERO POSITION (I,K).                4077
C                                                                           4078
                IF (.NOT.WNTV) GO TO 40                                     4079
                     DO 30 J=1,NRV                                          4080
   30                CALL G2 (CS,SN,V(J,I),V(J,K))                          4081
C              ACCUMULATE RT. TRANSFORMATIONS IN V.                         4082
C                                                                           4083
   40           CONTINUE                                                    4084
C                                                                           4085
C         THE MATRIX IS NOW BIDIAGONAL, AND OF LOWER ORDER                  4086
C         SINCE E(K) .EQ. ZERO..                                            4087
C                                                                           4088
   50           DO 60 LL=1,K                                                4089
                L=K+1-LL                                                    4090
                IF(DIFF(DNORM+E(L),DNORM)) 55,100,55                        4091
   55           IF(DIFF(DNORM+Q(L-1),DNORM)) 60,70,60                       4092
   60           CONTINUE                                                    4093
C     THIS LOOP CAN'T COMPLETE SINCE E(1) = ZERO.                           4094
C                                                                           4095
           GO TO 100                                                        4096
C                                                                           4097
C         CANCELLATION OF E(L), L.GT.1.                                     4098
   70      CS=ZERO                                                          4099
           SN=-ONE                                                          4100
                DO 90 I=L,K                                                 4101
                F=-SN*E(I)                                                  4102
                E(I)=CS*E(I)                                                4103
                IF(DIFF(DNORM+F,DNORM)) 75,100,75                           4104
   75           CALL G1 (Q(I),F,CS,SN,Q(I))                                 4105
                IF (.NOT.HAVERS) GO TO 90                                   4106
                     DO 80 J=1,NCC                                          4107
   80                CALL G2 (CS,SN,C(I,J),C(L-1,J))                        4108
   90           CONTINUE                                                    4109
C                                                                           4110
C         TEST FOR CONVERGENCE..                                            4111
  100      Z=Q(K)                                                           4112
           IF (L.EQ.K) GO TO 170                                            4113
C                                                                           4114
C         SHIFT FROM BOTTOM 2 BY 2 MINOR OF B**(T)*B.                       4115
           X=Q(L)                                                           4116
           Y=Q(K-1)                                                         4117
           G=E(K-1)                                                         4118
           H=E(K)                                                           4119
C-----------------------------------------------------------------------    4120
C  TO PREVENT ZERO-DIVIDE, TEST DENOMINATOR.                                4121
C-----------------------------------------------------------------------    4122
           RNUMER=(Y-Z)*(Y+Z)+(G-H)*(G+H)                                   4123
           DENOM=TWO*H*Y                                                    4124
           LDUM=ABS(DENOM) .LE. ZERO                                        4125
           IF (LDUM) F=.5*RANGE                                             4126
           IF (.NOT.LDUM) F=RNUMER/DENOM                                    4127
C-----------------------------------------------------------------------    4128
C  TO HELP PREVENT OVERFLOW, SET G=ABS(F) FOR VERY LARGE F.                 4129
C-----------------------------------------------------------------------    4130
           G=ABS(F)                                                         4131
           IF (G .LT. SQRTRG) G=SQRT(ONE+G**2)                              4132
           IF (F.LT.ZERO) GO TO 110                                         4133
           T=F+G                                                            4134
           GO TO 120                                                        4135
  110      T=F-G                                                            4136
  120      F=((X-Z)*(X+Z)+H*(Y/T-H))/X                                      4137
C                                                                           4138
C         NEXT QR SWEEP..                                                   4139
           CS=ONE                                                           4140
           SN=ONE                                                           4141
           LP1=L+1                                                          4142
                DO 160 I=LP1,K                                              4143
                G=E(I)                                                      4144
                Y=Q(I)                                                      4145
                H=SN*G                                                      4146
                G=CS*G                                                      4147
                CALL G1 (F,H,CS,SN,E(I-1))                                  4148
                F=X*CS+G*SN                                                 4149
                G=-X*SN+G*CS                                                4150
                H=Y*SN                                                      4151
                Y=Y*CS                                                      4152
                IF (.NOT.WNTV) GO TO 140                                    4153
C                                                                           4154
C              ACCUMULATE ROTATIONS (FROM THE RIGHT) IN 'V'                 4155
                     DO 130 J=1,NRV                                         4156
  130                CALL G2 (CS,SN,V(J,I-1),V(J,I))                        4157
  140           CALL G1 (F,H,CS,SN,Q(I-1))                                  4158
                F=CS*G+SN*Y                                                 4159
                X=-SN*G+CS*Y                                                4160
                IF (.NOT.HAVERS) GO TO 160                                  4161
                     DO 150 J=1,NCC                                         4162
  150                CALL G2 (CS,SN,C(I-1,J),C(I,J))                        4163
C              APPLY ROTATIONS FROM THE LEFT TO                             4164
C              RIGHT HAND SIDES IN 'C'..                                    4165
C                                                                           4166
  160           CONTINUE                                                    4167
           E(L)=ZERO                                                        4168
           E(K)=F                                                           4169
           Q(K)=X                                                           4170
           NQRS=NQRS+1                                                      4171
           IF (NQRS.LE.N10) GO TO 20                                        4172
C          RETURN TO 'TEST FOR SPLITTING'.                                  4173
C                                                                           4174
           FAIL=.TRUE.                                                      4175
C     ..                                                                    4176
C     CUTOFF FOR CONVERGENCE FAILURE. 'NQRS' WILL BE 2*N USUALLY.           4177
  170      IF (Z.GE.ZERO) GO TO 190                                         4178
           Q(K)=-Z                                                          4179
           IF (.NOT.WNTV) GO TO 190                                         4180
                DO 180 J=1,NRV                                              4181
  180           V(J,K)=-V(J,K)                                              4182
  190      CONTINUE                                                         4183
C         CONVERGENCE. Q(K) IS MADE NONNEGATIVE..                           4184
C                                                                           4185
  200      CONTINUE                                                         4186
      IF (N.EQ.1) RETURN                                                    4187
           DO 210 I=2,N                                                     4188
           IF (Q(I).GT.Q(I-1)) GO TO 220                                    4189
  210      CONTINUE                                                         4190
      IF (FAIL) IPASS=2                                                     4191
      RETURN                                                                4192
C     ..                                                                    4193
C     EVERY SINGULAR VALUE IS IN ORDER..                                    4194
  220      DO 270 I=2,N                                                     4195
           T=Q(I-1)                                                         4196
           K=I-1                                                            4197
                DO 230 J=I,N                                                4198
                IF (T.GE.Q(J)) GO TO 230                                    4199
                T=Q(J)                                                      4200
                K=J                                                         4201
  230           CONTINUE                                                    4202
           IF (K.EQ.I-1) GO TO 270                                          4203
           Q(K)=Q(I-1)                                                      4204
           Q(I-1)=T                                                         4205
           IF (.NOT.HAVERS) GO TO 250                                       4206
                DO 240 J=1,NCC                                              4207
                T=C(I-1,J)                                                  4208
                C(I-1,J)=C(K,J)                                             4209
  240           C(K,J)=T                                                    4210
  250      IF (.NOT.WNTV) GO TO 270                                         4211
                DO 260 J=1,NRV                                              4212
                T=V(J,I-1)                                                  4213
                V(J,I-1)=V(J,K)                                             4214
  260           V(J,K)=T                                                    4215
  270      CONTINUE                                                         4216
C         END OF ORDERING ALGORITHM.                                        4217
C                                                                           4218
      IF (FAIL) IPASS=2                                                     4219
      RETURN                                                                4220
      END                                                                   4221
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4222
C  FUNCTION RANDOM.  PRODUCES A PSEUDORANDOM REAL ON THE OPEN INTERVAL      4223
C      (0.,1.).                                                             4224
C  DIX (IN DOUBLE PRECISION) MUST BE INITIALIZED TO A WHOLE NUMBER          4225
C      BETWEEN 1.D0 AND 2147483646.D0 BEFORE THE FIRST CALL TO RANDOM       4226
C      AND NOT CHANGED BETWEEN SUCCESSIVE CALLS TO RANDOM.                  4227
C  BASED ON L. SCHRAGE, ACM TRANS. ON MATH. SOFTWARE 5, 132 (1979).         4228
C-----------------------------------------------------------------------    4229
      FUNCTION RANDOM(DIX)                                                  4230
C                                                                           4231
C  PORTABLE RANDOM NUMBER GENERATOR                                         4232
C   USING THE RECURSION                                                     4233
C    DIX = DIX*A MOD P                                                      4234
C                                                                           4235
      DOUBLE PRECISION A,P,DIX,B15,B16,XHI,XALO,LEFTLO,FHI,K                4236
C                                                                           4237
C  7**5, 2**15, 2**16, 2**31-1                                              4238
      DATA A/16807.D0/,B15/32768.D0/,B16/65536.D0/,P/2147483647.D0/         4239
C                                                                           4240
C  GET 15 HI ORDER BITS OF DIX                                              4241
      XHI = DIX / B16                                                       4242
      XHI = XHI - DMOD(XHI,1.D0)                                            4243
C  GET 16 LO BITS IF DIX AND FORM LO PRODUCT                                4244
      XALO=(DIX-XHI*B16)*A                                                  4245
C  GET 15 HI ORDER BITS OF LO PRODUCT                                       4246
      LEFTLO = XALO/B16                                                     4247
      LEFTLO = LEFTLO - DMOD(LEFTLO,1.D0)                                   4248
C  FORM THE 31 HIGHEST BITS OF FULL PRODUCT                                 4249
      FHI = XHI*A + LEFTLO                                                  4250
C  GET OVERFLO PAST 31ST BIT OF FULL PRODUCT                                4251
      K = FHI/B15                                                           4252
      K = K - DMOD(K,1.D0)                                                  4253
C  ASSEMBLE ALL THE PARTS AND PRESUBTRACT P                                 4254
C   THE PARENTHESES ARE ESSENTIAL                                           4255
      DIX = (((XALO-LEFTLO*B16) - P) + (FHI-K*B15)*B16) + K                 4256
C  ADD P BACK IN IF NECESSARY                                               4257
      IF (DIX .LT. 0.D0) DIX = DIX + P                                      4258
C  MULTIPLY BY 1/(2**31-1)                                                  4259
      RANDOM=DIX*4.656612875D-10                                            4260
      RETURN                                                                4261
      END                                                                   4262
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4263
C  SUBROUTINE READYT.  READS Y(J) (INPUT DATA), T(J) (INDEPENDENT           4264
C      VARIABLE), AND, IF IWT=4, LEAST SQUARES WEIGHTS, FOR J=1,NY.         4265
C  IF DOUSIN=.TRUE., THEN USERIN IS CALLED TO RECOMPUTE OR CHANGE           4266
C      INPUT DATA.                                                          4267
C-----------------------------------------------------------------------    4268
C  CALL SUBPROGRAMS - USERIN, ERRMES                                        4269
C-----------------------------------------------------------------------    4270
      SUBROUTINE READYT (MY,NIOERR,SQRTW,T,Y)                               4271
      DOUBLE PRECISION PRECIS, RANGE                                        4272
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  4273
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              4274
      DIMENSION SQRTW(MY), T(MY), Y(MY)                                     4275
      DIMENSION LIN(6), LA(6,2), IHOLER(6)                                  4276
      COMMON /DBLOCK/ PRECIS, RANGE                                         4277
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         4278
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     4279
     2 SRANGE                                                               4280
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     4281
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       4282
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             4283
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              4284
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         4285
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            4286
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  4287
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            4288
     2 LUSER(30)                                                            4289
      DATA IHOLER/1HR, 1HE, 1HA, 1HD, 1HY, 1HT/, LA/                        4290
     1 1HN, 1HS, 1HT, 1HE, 1HN, 1HD,   1HN, 1HY, 4*1H /                     4291
      IF (NINTT .LE. 0) GO TO 200                                           4292
C-----------------------------------------------------------------------    4293
C  COMPUTE T IN EQUAL INTERVALS.                                            4294
C-----------------------------------------------------------------------    4295
      NY=0                                                                  4296
      DO 110 J=1,NINTT                                                      4297
 5110 FORMAT (1X,6A1,I5,2E15.6)                                             4298
      READ (NIN,5110) LIN,NT,TSTART,TEND                                    4299
 5120 FORMAT (1X,6A1,I5,1P2E15.5)                                           4300
      WRITE (NOUT,5120) LIN,NT,TSTART,TEND                                  4301
      DO 120 K=1,6                                                          4302
        IF (LIN(K) .NE. LA(K,1)) GO TO 130                                  4303
  120 CONTINUE                                                              4304
      GO TO 140                                                             4305
  130 CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                                   4306
      GO TO 190                                                             4307
  140 IF (NT.GE.2 .AND. NT+NY.LE.MY) GO TO 150                              4308
      CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                                   4309
      GO TO 190                                                             4310
  150 DUM=(TEND-TSTART)/FLOAT(NT-1)                                         4311
      NY=NY+1                                                               4312
      T(NY)=TSTART                                                          4313
      DO 160 K=2,NT                                                         4314
      NY=NY+1                                                               4315
  160 T(NY)=T(NY-1)+DUM                                                     4316
      GO TO 110                                                             4317
  190 NIOERR=NIOERR+1                                                       4318
      IF (NIOERR .GE. MIOERR) STOP                                          4319
  110 CONTINUE                                                              4320
      GO TO 300                                                             4321
C-----------------------------------------------------------------------    4322
C  READ IN NY AND THEN T ARRAY.                                             4323
C-----------------------------------------------------------------------    4324
  200 READ (NIN,5110) LIN,NY                                                4325
      WRITE (NOUT,5110) LIN,NY                                              4326
      DO 210 K=1,6                                                          4327
        IF (LIN(K) .NE. LA(K,2)) GO TO 220                                  4328
  210 CONTINUE                                                              4329
      GO TO 230                                                             4330
  220 CALL ERRMES (3,.FALSE.,IHOLER,NOUT)                                   4331
      GO TO 235                                                             4332
  230 IF (NY .LE. MY) GO TO 240                                             4333
      CALL ERRMES (4,.FALSE.,IHOLER,NOUT)                                   4334
  235 NIOERR=NIOERR+1                                                       4335
      RETURN                                                                4336
  240 READ (NIN,IFORMT) (T(J),J=1,NY)                                       4337
C-----------------------------------------------------------------------    4338
C  READ IN Y ARRAY.                                                         4339
C-----------------------------------------------------------------------    4340
  300 IF (.NOT.SIMULA) READ (NIN,IFORMY) (Y(J),J=1,NY)                      4341
      IF (IWT .EQ. 4) GO TO 420                                             4342
C-----------------------------------------------------------------------    4343
C  INITIALIZE SQRTW (SQUARE ROOTS OF LEAST SQUARES WEIGHTS) TO UNITY.       4344
C-----------------------------------------------------------------------    4345
      DO 410 J=1,NY                                                         4346
      SQRTW(J)=1.                                                           4347
  410 CONTINUE                                                              4348
C-----------------------------------------------------------------------    4349
C  READ IN LEAST SQUARES WEIGHTS IF IWT=4.                                  4350
C-----------------------------------------------------------------------    4351
  420 IF (IWT .EQ. 4) READ (NIN,IFORMW) (SQRTW(J),J=1,NY)                   4352
C-----------------------------------------------------------------------    4353
C  CALL USERIN TO CHANGE OR RECOMPUTE INPUT DATA.                           4354
C-----------------------------------------------------------------------    4355
      IF (DOUSIN) CALL USERIN (T,Y,SQRTW,MY)                                4356
      DO 430 J=1,NY                                                         4357
      IF (SQRTW(J) .GE. 0.) GO TO 440                                       4358
      CALL ERRMES (5,.FALSE.,IHOLER,NOUT)                                   4359
 5440 FORMAT (1X,1P10E13.5)                                                 4360
      WRITE (NOUT,5440) (SQRTW(K),K=1,NY)                                   4361
      NIOERR=NIOERR+1                                                       4362
      GO TO 800                                                             4363
  440 SQRTW(J)=SQRT(SQRTW(J))                                               4364
  430 CONTINUE                                                              4365
  800 RETURN                                                                4366
      END                                                                   4367
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4368
C  SUBROUTINE RGAUSS.  AN EXACT METHOD FOR GENERATING X1 AND X2, TWO        4369
C      STANDARD (ZERO MEAN, UNIT VARIANCE) NORMAL RANDOM DEVIATES           4370
C      FROM 2 CALLS TO RANDOM, WHICH DELIVERS A UNIFORM RANDOM              4371
C      DEVIATE ON THE INTERVAL (0,1).  (SEE M. C. PIKE,                     4372
C      ALGORITHM 267 FROM CACM)                                             4373
C  TWOPI = 2.*PI.                                                           4374
C  DIX IS EXPLAINED IN THE COMMENTS IN RANDOM.                              4375
C  IF A VERY LARGE NUMBER OF DEVIATES ARE TO BE GENERATED,                  4376
C      FASTER ROUTINES EXIST.                                               4377
C-----------------------------------------------------------------------    4378
C  CALLS SUBPROGRAMS - RANDOM                                               4379
C-----------------------------------------------------------------------    4380
      SUBROUTINE RGAUSS (X1,X2,TWOPI,DIX)                                   4381
      DOUBLE PRECISION DIX                                                  4382
      X1=SQRT(-2.*ALOG(RANDOM(DIX)))                                        4383
      DUM=TWOPI*RANDOM(DIX)                                                 4384
      X2=X1*SIN(DUM)                                                        4385
      X1=X1*COS(DUM)                                                        4386
      RETURN                                                                4387
      END                                                                   4388
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4389
C  SUBROUTINE RUNRES.  COMPUTES PRUNS (RUNS TEST PROBABILITY) AND           4390
C      PUNCOR (AUTOCORRELATION TEST PROBABILITIES) FOR SOL(J), J=1,NY.      4391
C  IF ILEVEL.LE.IPLRES(ISTAGE), THEN RESIDUALS ARE PLOTTED.                 4392
C  IF ILEVEL.LE.IPLFIT(ISTAGE), THEN Y (DATA) AND                           4393
C      YFIT (FIT TO DATA) ARE PLOTTED.                                      4394
C  IF NEWPAG=.TRUE., THEN A NEW PAGE IS STARTED.                            4395
C  RALPS1 = ALPHA/(LARGEST SINGULAR VALUE).                                 4396
C-----------------------------------------------------------------------    4397
C  CALLS SUBPROGRAMS - GETPRU, PLRES, PLPRIN                                4398
C  WHICH IN TURN CALL - PGAUSS, GETYLY, GETROW, USERK, USERLF, ERRMES,      4399
C      BETAIN, FISHNI, GAMLN, USERTR                                        4400
C-----------------------------------------------------------------------    4401
      SUBROUTINE RUNRES (ILEVEL,SOL,NEWPAG,RALPS1,CHOSEN,                   4402
     1 CQUAD,G,IPLFIT,IPLRES,ISTAGE,ITITLE,IUNIT,IWT,LINEPG,MWORK,NG,       4403
     2 NGL,NLINF,NOUT,NY,SQRTW,SRANGE,SSCALE,T,WORK,Y,YLYFIT)               4404
      DOUBLE PRECISION  SOL, SSCALE, WORK                                   4405
      LOGICAL NEWPAG, CHOSEN                                                4406
      DIMENSION SOL(NGL), WORK(MWORK), SQRTW(NY), CQUAD(NG), G(NG),         4407
     1 SSCALE(NGL), T(NY), Y(NY), YLYFIT(NY), ITITLE(80), IPLFIT(2),        4408
     2 IPLRES(2)                                                            4409
      DIMENSION PUNCOR(5)                                                   4410
      CALL GETPRU (SOL,                                                     4411
     1 CQUAD,G,IUNIT,IWT,MWORK,NG,NGL,NLINF,NOUT,NY,PRUNS,PUNCOR,SQRTW,     4412
     2 SSCALE,T,WORK,Y,YLYFIT)                                              4413
      IF (ILEVEL .LE. IPLRES(ISTAGE)) GO TO 150                             4414
 5100 FORMAT (1H1,9X,80A1)                                                  4415
      IF (NEWPAG) WRITE (NOUT,5100) ITITLE                                  4416
 5110 FORMAT (18H0(FOR ALPHA/S(1) =,1PE9.2,                                 4417
     1 9H) PRUNS =,0PF7.4,9X,8HPUNCOR =,5F8.4)                              4418
      WRITE (NOUT,5110) RALPS1,PRUNS,PUNCOR                                 4419
      GO TO 200                                                             4420
C---MN 7x05 PRINTING OUT THE ACTUAL VALUES OF THE RESIDUALS.------------
  150 WRITE(NOUT,997)
  997 FORMAT (//9HRESIDUALS)
      DO 999 J=1,NY
  999 WRITE(NOUT,998) YLYFIT(J) 
  998 FORMAT(E12.5)
      CALL PLRES (YLYFIT,NY,NY,PRUNS,PUNCOR,RALPS1,NOUT,LINEPG,ITITLE,      4421
     1 CHOSEN)                                                              4422
  200 IF (ILEVEL .GT. IPLFIT(ISTAGE)) GO TO 800                             4423
C-----------------------------------------------------------------------    4424
C  TEMPORARILY PUT YFIT (FIT TO DATA) IN YLYFIT.                            4425
C  IF SQRTW(J).LE.0., THEN YFIT(J) IS ARBITRARILY SET TO Y(J).              4426
C-----------------------------------------------------------------------    4427
      DO 210 J=1,NY                                                         4428
        DUM=0.                                                              4429
        IF (SQRTW(J) .GT. 0.) DUM=YLYFIT(J)/SQRTW(J)                        4430
        YLYFIT(J)=Y(J)-DUM                                                  4431
  210 CONTINUE                                                              4432
 5210 FORMAT (//38H0PLOT OF DATA (O) AND FIT TO DATA (X).,                  4433
     1 34H  ORDINATES LISTED ARE FIT VALUES.)                               4434
      WRITE (NOUT,5210)                                                     4435
      CALL PLPRIN (T,YLYFIT,Y,NY,.FALSE.,NOUT,SRANGE,0,0,NY,WORK,           4436
     1 .FALSE.)                                                             4437
C-----------------------------------------------------------------------    4438
C  RESTORE YLYFIT.                                                          4439
C-----------------------------------------------------------------------    4440
      DO 220 J=1,NY                                                         4441
        YLYFIT(J)=(Y(J)-YLYFIT(J))*SQRTW(J)                                 4442
  220 CONTINUE                                                              4443
  800 RETURN                                                                4444
      END                                                                   4445
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4446
C  SUBROUTINE SEQACC.  SEQUENTIALLY ACCUMULATES N ROWS OF                   4447
C      COEFFICIENT MATRIX (1 ROW AT A TIME) AND PERFORMS                    4448
C      SEQUENTIAL HOUSEHOLDER TRANSFORMATIONS TO COMPRESS                   4449
C      PROBLEM INTO UPPER TRIANGLE OF AUGMENTED NGL-BY-(NGL+1) MATRIX A.    4450
C  IF NY.LE.NGL, THEN ROWS ARE JUST LOADED INTO A WITHOUT ANY               4451
C      TRIANGULARIZATION AND THE LAST (NGL-NY) ROWS ARE FILLED WITH         4452
C      ZEROES.                                                              4453
C-----------------------------------------------------------------------    4454
C  CALLS SUBPROGRAMS - H12, GETROW                                          4455
C  WHICH IN TURN CALL - USERK, ERRMES, USERLF, USERTR                       4456
C-----------------------------------------------------------------------    4457
      SUBROUTINE SEQACC (                                                   4458
     1 A,CQUAD,G,IUNIT,IWT,MA,MG,NG,NGL,NGLP1,NLINF,NY,                     4459
     2 RANGE,SQRTW,SSCALE,T,Y)                                              4460
      DOUBLE PRECISION A, RANGE, RHO, SSCALE, ZERO                          4461
      DIMENSION A(MA,MG), T(NY), Y(NY), SQRTW(NY), G(NG), CQUAD(NG),        4462
     1 SSCALE(MG)                                                           4463
C     ZERO=0.E0!SP                                                          4464
      ZERO=0.D0                                                             4465
      L=0                                                                   4466
      NGL=NG+NLINF                                                          4467
      NGLP1=NGL+1                                                           4468
      DO 200 IT=1,NY                                                        4469
        IP=L+1                                                              4470
        IIT=IT                                                              4471
        CALL GETROW (IIT,A(IP,1),.TRUE.,MA,IUNIT,                           4472
     1  SQRTW,NY,NGL,NG,CQUAD,G,T,NLINF,Y,SSCALE)                           4473
        IF (L.LE.0 .OR. NY.LE.NGL) GO TO 230                                4474
        J=MIN0(NGLP1,L)                                                     4475
        DO 220 I=1,J                                                        4476
          II=I                                                              4477
          CALL H12 (1,II,IP,IP,A(1,I),1,RHO,A(1,I+1),1,MA,NGLP1-I,RANGE)    4478
  220   CONTINUE                                                            4479
  230   L=MIN0(NGLP1,IP)                                                    4480
  200 CONTINUE                                                              4481
      IF (NY .LE. NGL) GO TO 350                                            4482
      DO 300 J=2,NGL                                                        4483
        L=J-1                                                               4484
        DO 310 K=1,L                                                        4485
          A(J,K)=ZERO                                                       4486
  310   CONTINUE                                                            4487
  300 CONTINUE                                                              4488
      GO TO 800                                                             4489
  350 L=NY+1                                                                4490
      DO 360 J=L,NGL                                                        4491
        DO 370 K=1,NGLP1                                                    4492
          A(J,K)=ZERO                                                       4493
  370   CONTINUE                                                            4494
  360 CONTINUE                                                              4495
  800 RETURN                                                                4496
      END                                                                   4497
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4498
C  SUBROUTINE SETGA1.  PUTS D*A1 (WHICH ARE STORED IN AINEQ AND REG)        4499
C      INTO THE FIRST NNINEQ ROWS AND NGLE COLUMNS OF A, WHERE              4500
C      A1 = K2*Z*H1**(-1)*W IN EQ. (5.18).                                  4501
      SUBROUTINE SETGA1 (NNINEQ,                                            4502
     1 A,AINEQ,MA,MG,MINEQ,MREG,NGL,NGLE,REG)                               4503
      DOUBLE PRECISION A, AINEQ, DUM, REG, ZERO                             4504
      DIMENSION AINEQ(MINEQ,MG), REG(MREG,MG), A(MA,MG)                     4505
C     ZERO=0.E0!SP                                                          4506
      ZERO=0.D0                                                             4507
      DO 120 IROW=1,NNINEQ                                                  4508
        DO 130 ICOL=1,NGLE                                                  4509
          DUM=ZERO                                                          4510
          DO 140 J=1,NGL                                                    4511
            DUM=DUM+AINEQ(IROW,J)*REG(J,ICOL)                               4512
  140     CONTINUE                                                          4513
          A(IROW,ICOL)=DUM                                                  4514
  130   CONTINUE                                                            4515
  120 CONTINUE                                                              4516
      RETURN                                                                4517
      END                                                                   4518
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4519
C  SUBROUTINE SETGRD.  CALCULATES G (QUADRATURE GRID) AND                   4520
C      CQUAD (WEIGHTS OF QUADRATURE FORMULA).                               4521
C  IGRID=1,2, OR 3 FOR G IN EQUAL INTERVALS, FOR G IN EQUAL INTERVALS       4522
C      OF A FUNCTION OF G (SAY H(G)), WHERE H IS DEFINED BY USER IN         4523
C      USERTR, OR FOR G AND CQUAD TO BE COMPUTED IN USERGR, RESPECTIVELY    4524
C  IQUAD=1,2, OR 3 FOR NO QUADRATURE (I.E., LINEAR EQUATIONS), FOR          4525
C      TRAPEZOIDAL RULE, OR FOR SIMPSON RULE, RESPECTIVELY.                 4526
C-----------------------------------------------------------------------    4527
C  CALLS SUBPROGRAMS - USERGR, ERRMES, USERTR, CQTRAP                       4528
C-----------------------------------------------------------------------    4529
      SUBROUTINE SETGRD (CQUAD,G,GMNMX,IGRID,IQUAD,MG,NG,NOUT)              4530
      DIMENSION G(MG), CQUAD(MG), GMNMX(2)                                  4531
      DIMENSION IHOLER(6)                                                   4532
      DATA IHOLER/1HS, 1HE, 1HT, 1HG, 1HR, 1HD/                             4533
      IF (IGRID .NE. 3) GO TO 200                                           4534
C-----------------------------------------------------------------------    4535
C  COMPUTE G AND CQUAD IN USER-SUPPLIED ROUTINE USERGR.                     4536
C-----------------------------------------------------------------------    4537
      CALL USERGR (G,CQUAD,MG)                                              4538
      GO TO 300                                                             4539
  200 IF (IGRID.EQ.2 .AND. AMIN1(GMNMX(1),GMNMX(2)).LE.0.) CALL             4540
     1 ERRMES (1,.FALSE.,IHOLER,NOUT)                                       4541
C-----------------------------------------------------------------------    4542
C  COMPUTE G WHEN IGRID=1 OR 2.                                             4543
C-----------------------------------------------------------------------    4544
      G(1)=GMNMX(1)                                                         4545
      DELTA=(USERTR(GMNMX(2),1)-USERTR(GMNMX(1),1))/FLOAT(NG-1)             4546
      DO 210 J=2,NG                                                         4547
        DUM=USERTR(G(J-1),1)+DELTA                                          4548
        G(J)=USERTR(DUM,2)                                                  4549
  210 CONTINUE                                                              4550
C-----------------------------------------------------------------------    4551
C  CHECK G FOR STRICT MONOTONICITY.                                         4552
C-----------------------------------------------------------------------    4553
  300 IF (NG .LE. 2) GO TO 350                                              4554
      DELOLD=G(2)-G(1)                                                      4555
      DO 310 J=3,NG                                                         4556
        DEL=G(J)-G(J-1)                                                     4557
        IF (DEL*DELOLD .GT. 0.) GO TO 315                                   4558
        CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                                 4559
 5310   FORMAT (1X,1P10E13.3)                                               4560
        WRITE (NOUT,5310) (G(K),K=1,NG)                                     4561
        STOP                                                                4562
  315   DELOLD=DEL                                                          4563
  310 CONTINUE                                                              4564
  350 IF (IGRID .EQ. 3) GO TO 800                                           4565
C-----------------------------------------------------------------------    4566
C  COMPUTE CQUAD.                                                           4567
C-----------------------------------------------------------------------    4568
      IF (IQUAD .NE. 1) GO TO 420                                           4569
C-----------------------------------------------------------------------    4570
C  NO QUADRATURE - LINEAR ALGEBRAIC EQUATIONS ARE BEING SOLVED.             4571
C-----------------------------------------------------------------------    4572
      DO 410 J=1,NG                                                         4573
        CQUAD(J)=1.                                                         4574
  410 CONTINUE                                                              4575
      GO TO 800                                                             4576
  420 IF (IQUAD .NE. 2) GO TO 450                                           4577
C-----------------------------------------------------------------------    4578
C  TRAPEZOIDAL RULE.                                                        4579
C-----------------------------------------------------------------------    4580
      CALL CQTRAP (G,CQUAD,NG)                                              4581
      GO TO 800                                                             4582
  450 IF (IQUAD .NE. 3) CALL ERRMES (3,.TRUE.,IHOLER,NOUT)                  4583
C-----------------------------------------------------------------------    4584
C  SIMPSON RULE (WITH LAST GRID PAIR DONE WITH TRAPEZOIDAL RULE IF          4585
C      NG IS EVEN).                                                         4586
C-----------------------------------------------------------------------    4587
      CQUAD(1)=DELTA/3.                                                     4588
      CQ2=2.*CQUAD(1)                                                       4589
      CQ4=CQ2+CQ2                                                           4590
      JJ=NG-1                                                               4591
      DO 460 J=2,JJ,2                                                       4592
        CQUAD(J)=CQ4                                                        4593
        CQUAD(J+1)=CQ2                                                      4594
  460 CONTINUE                                                              4595
      IF (MOD(NG,2) .EQ. 0) GO TO 470                                       4596
      CQUAD(NG)=CQUAD(1)                                                    4597
      GO TO 500                                                             4598
  470 CQUAD(NG)=1.5*CQUAD(1)                                                4599
      CQUAD(NG-1)=CQUAD(1)+CQUAD(NG)                                        4600
  500 IF (IGRID .NE. 2) GO TO 800                                           4601
C-----------------------------------------------------------------------    4602
C  CORRECT CQUAD FOR TRANSFORMATION IN USERTR WHEN IGRID=2.                 4603
C-----------------------------------------------------------------------    4604
      DO 510 J=1,NG                                                         4605
        CQUAD(J)=CQUAD(J)/USERTR(G(J),3)                                    4606
  510 CONTINUE                                                              4607
  800 RETURN                                                                4608
      END                                                                   4609
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4610
C  SUBROUTINE SETNNG.  CONSTRAINS THE SOLUTION TO BE NONNEGATIVE            4611
C      AT ALL NG GRID POINTS.  ADDS NG NONNEGATIVITY CONSTRAINTS            4612
C      ONTO THE LAST NG ROWS OF AINEQ.                                      4613
C-----------------------------------------------------------------------    4614
C  CALLS SUBPROGRAMS - ERRMES                                               4615
C-----------------------------------------------------------------------    4616
      SUBROUTINE SETNNG (AINEQ,MINEQ,NG,NGLP1,NINEQ,NOUT)                   4617
      DOUBLE PRECISION AINEQ, ONE, ZERO                                     4618
      DIMENSION AINEQ(MINEQ,NGLP1)                                          4619
      DIMENSION IHOLER(6)                                                   4620
      DATA IHOLER/1HS, 1HE, 1HT, 1HN, 1HN, 1HG/                             4621
      IF (NINEQ+NG .GT. MINEQ) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)           4622
C     ZERO=0.E0!SP                                                          4623
      ZERO=0.D0                                                             4624
C     ONE=1.E0!SP                                                           4625
      ONE=1.D0                                                              4626
      DO 210 J=1,NG                                                         4627
        NINEQ=NINEQ+1                                                       4628
        DO 220 K=1,NGLP1                                                    4629
          AINEQ(NINEQ,K)=ZERO                                               4630
  220   CONTINUE                                                            4631
        AINEQ(NINEQ,J)=ONE                                                  4632
  210 CONTINUE                                                              4633
      RETURN                                                                4634
      END                                                                   4635
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4636
C  SUBROUTINE SETREG.  SETS UP AUGMENTED SCALED REGULARIZOR MATRIX IN       4637
C      REG.                                                                 4638
C  NORDER = ORDER OF REGULARIZOR, WHEN 0.LE.NORDER.LE.5.  WHEN              4639
C      NORDER.LT.0, THEN THE REGULARIZOR IS SET IN THE                      4640
C      USER-SUPPLIED ROUTINE USERRG.                                        4641
C  NENDZ SPECIFIES THE EXTERNAL BOUNDARY CONDITIONS OF THE SOLUTION         4642
C      FOR COMPUTING THE REGULARIZOR.  NENDZ(J) = NO. OF GRID POINTS        4643
C      BELOW G(1) FOR WHICH THE SOLUTION IS ZERO (J=1), OR THE              4644
C      NO. OF GRID POINTS ABOVE G(NG) FOR WHICH THE SOLUTION                4645
C      IS ZERO (J=2).  (MUST HAVE NENDZ(J).LE.NORDER FOR J=1,2, WHEN        4646
C      NORDER.GE.0.)                                                        4647
C-----------------------------------------------------------------------    4648
C  CALLS SUBPROGRAMS - USERRG, ERRMES                                       4649
C-----------------------------------------------------------------------    4650
      SUBROUTINE SETREG (MG,MREG,NENDZ,NG,NGL,NGLE,NGLP1,NORDER,            4651
     1 NOUT,NREG,REG,SSCALE)                                                4652
      DOUBLE PRECISION ABS, REG, SSCALE, ZERO                               4653
      DIMENSION REG(MREG,MG), NENDZ(2), SSCALE(MG)                          4654
      DIMENSION DC(6,6), IHOLER(6)                                          4655
      DATA DC/1., 5*0.,   -1., 1., 4*0.,   1., -2., 1., 3*0.,               4656
     1 -1., 3., -3., 1., 2*0.,   1., -4., 6., -4., 1., 0.,                  4657
     2 -1., 5., -10., 10., -5., 1./,                                        4658
     3 IHOLER/1HS, 1HE, 1HT, 1HR, 1HE, 1HG/                                 4659
      ABS(ZERO)=DABS(ZERO)                                                  4660
C     ZERO=0.E0!SP                                                          4661
      ZERO=0.D0                                                             4662
      IF (NORDER .GE. 0) GO TO 200                                          4663
C-----------------------------------------------------------------------    4664
C  COMPUTE REGULARIZOR IN USER-SUPPLIED ROUTINE USERRG.                     4665
C-----------------------------------------------------------------------    4666
      CALL USERRG (REG,MREG,MG,NREG)                                        4667
      IF (NREG .LE. 0) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)                   4668
      GO TO 300                                                             4669
  200 IF (NORDER .GT. 5) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                 4670
      IF (MIN0(NENDZ(1),NENDZ(2)).LT.0 .OR.                                 4671
     1 MAX0(NENDZ(1),NENDZ(2)).GT.NORDER)                                   4672
     2 CALL ERRMES (3,.FALSE.,IHOLER,NOUT)                                  4673
      NENDZ(1)=MAX0(0,MIN0(NENDZ(1),NORDER))                                4674
      NENDZ(2)=MAX0(0,MIN0(NENDZ(2),NORDER))                                4675
      NREG=NG+NENDZ(1)+NENDZ(2)-NORDER                                      4676
      IF (MAX0(NREG,NGLE) .GE. MREG) CALL ERRMES (4,.TRUE.,IHOLER,NOUT)     4677
      NORDP1=NORDER+1                                                       4678
      DO 210 J=1,NREG                                                       4679
        DO 220 K=1,NGLP1                                                    4680
          REG(J,K)=ZERO                                                     4681
  220   CONTINUE                                                            4682
        L=J-NENDZ(1)-1                                                      4683
        DO 230 K=1,NORDP1                                                   4684
          L=L+1                                                             4685
          IF (L.GE.1 .AND. L.LE.NG) REG(J,L)=DC(K,NORDP1)                   4686
  230   CONTINUE                                                            4687
  210 CONTINUE                                                              4688
  300 IF (SSCALE(1) .LE. ZERO) GO TO 800                                    4689
C-----------------------------------------------------------------------    4690
C  SCALE REGULARIZOR WITH SSCALE AND DIVIDE ALL THE ELEMENTS OF THE         4691
C      REGULARIZOR AND ITS RIGHT-HAND SIDE BY THE AVERAGE ABSOLUTE          4692
C      ELEMENT SIZE.                                                        4693
C-----------------------------------------------------------------------    4694
      AVGREG=ZERO                                                           4695
      DO 310 ICOL=1,NGL                                                     4696
        DO 320 IROW=1,NREG                                                  4697
          REG(IROW,ICOL)=REG(IROW,ICOL)*SSCALE(ICOL)                        4698
          AVGREG=AVGREG+ABS(REG(IROW,ICOL))                                 4699
  320   CONTINUE                                                            4700
  310 CONTINUE                                                              4701
      IF (AVGREG .LE. ZERO) CALL ERRMES (5,.TRUE.,IHOLER,NOUT)              4702
      AVGREG=FLOAT(NGL*NREG)/AVGREG                                         4703
      DO 330 ICOL=1,NGLP1                                                   4704
        DO 340 IROW=1,NREG                                                  4705
          REG(IROW,ICOL)=REG(IROW,ICOL)*AVGREG                              4706
  340   CONTINUE                                                            4707
  330 CONTINUE                                                              4708
      DUM=AVGREG                                                            4709
 5340 FORMAT (25H0SCALE FACTOR FOR ALPHA =,1PE11.3)                         4710
      WRITE (NOUT,5340) DUM                                                 4711
      IF (NREG .GE. NGLE) GO TO 800                                         4712
C-----------------------------------------------------------------------    4713
C  AUGMENT REGULARIZOR WITH ZERO ROWS TO MAKE NREG=NGLE.                    4714
C-----------------------------------------------------------------------    4715
      L=NREG+1                                                              4716
      DO 350 ICOL=1,NGLP1                                                   4717
        DO 360 IROW=L,NGLE                                                  4718
          REG(IROW,ICOL)=ZERO                                               4719
  360   CONTINUE                                                            4720
  350 CONTINUE                                                              4721
      NREG=NGLE                                                             4722
  800 RETURN                                                                4723
      END                                                                   4724
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4725
C  SUBROUTINE SETSCA.  PUTS SCALE FACTORS FOR SOLUTION IN SSCALE AND        4726
C      SCALES INEQUALITY MATRIX AINEQ.  L1 LENGTHS ARE USED                 4727
C      THROUGHOUT TO AVOID UNDERFLOWS AND OVERFLOWS DUE TO SQUARING.        4728
C  OUTPUTS SCALE FACTORS AND MIN AND MAX ELEMENTS OF EACH COLUMN OF         4729
C      THE COEFFICIENT MATRIX A.                                            4730
C-----------------------------------------------------------------------    4731
C  CALLS SUBPROGRAMS - ERRMES, SETREG, GETROW                               4732
C  WHICH IN TURN CALL - USERRG, USERK, USERLF, USERTR                       4733
C-----------------------------------------------------------------------    4734
      SUBROUTINE SETSCA (AAMAX,AAMIN,TMAX,TMIN,                             4735
     1 AINEQ,CQUAD,G,ISTAGE,MG,MINEQ,MREG,MY,NGLE,REG,S,SQRTW,SSCALE,T,     4736
     2 Y)                                                                   4737
      DOUBLE PRECISION ABS, AINEQ, REG, S, SSCALE, ONE, ZERO, DUB,          4738
     1 AAMAX,AAMIN,TMAX,TMIN                                                4739
      DOUBLE PRECISION PRECIS, RANGE                                        4740
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  4741
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              4742
      DIMENSION AINEQ(MINEQ,MG), CQUAD(MG), G(MG), REG(MREG,MG),            4743
     1 S(MG,3), SQRTW(MY), SSCALE(MG), T(MY), Y(MY),                        4744
     2 AAMAX(1), AAMIN(1), TMAX(1), TMIN(1)                                 4745
      DIMENSION IHOLER(6)                                                   4746
      COMMON /DBLOCK/ PRECIS, RANGE                                         4747
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         4748
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     4749
     2 SRANGE                                                               4750
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     4751
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       4752
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             4753
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              4754
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         4755
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            4756
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  4757
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            4758
     2 LUSER(30)                                                            4759
      DATA IHOLER/1HS, 1HE, 1HT, 1HS, 1HC, 1HA/                             4760
      ABS(DUB)=DABS(DUB)                                                    4761
C     ZERO=0.E0!SP                                                          4762
      ZERO=0.D0                                                             4763
C     ONE=1.E0!SP                                                           4764
      ONE=1.D0                                                              4765
      IF (ISTAGE.EQ.1 .OR. IWT.EQ.1 .OR. IWT.EQ.4 .OR. NINEQ.LE.0) GO       4766
     1 TO 150                                                               4767
C-----------------------------------------------------------------------    4768
C  UNSCALE INEQUALITY CONSTRAINTS.                                          4769
C-----------------------------------------------------------------------    4770
      DO 110 ICOL=1,NGL                                                     4771
        IF (SSCALE(ICOL) .LE. ZERO) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)      4772
        DUB=ONE/SSCALE(ICOL)                                                4773
        DO 120 IROW=1,NINEQ                                                 4774
          AINEQ(IROW,ICOL)=AINEQ(IROW,ICOL)*DUB                             4775
  120   CONTINUE                                                            4776
  110 CONTINUE                                                              4777
C-----------------------------------------------------------------------    4778
C  INITIALIZE SCALE FACTOR, S(J,1), ETC.                                    4779
C-----------------------------------------------------------------------    4780
  150 SSCALE(1)=ZERO                                                        4781
      DO 160 J=1,NGL                                                        4782
        S(J,1)=ZERO                                                         4783
        AAMAX(J)=-RANGE                                                     4784
        AAMIN(J)=RANGE                                                      4785
  160 CONTINUE                                                              4786
C-----------------------------------------------------------------------    4787
C  PUT MAGNITUDES OF COLUMN VECTORS OF REGULARIZOR IN S(J,2),               4788
C      J=1,NGL.                                                             4789
C-----------------------------------------------------------------------    4790
      CALL SETREG (MG,MREG,NENDZ,NG,NGL,NGLE,NGLP1,NORDER,NOUT,NREG,        4791
     1 REG,SSCALE)                                                          4792
      DO 210 ICOL=1,NGL                                                     4793
        DUB=ZERO                                                            4794
        DO 220 IROW=1,NREG                                                  4795
          DUB=DUB+ABS(REG(IROW,ICOL))                                       4796
  220   CONTINUE                                                            4797
        S(ICOL,2)=DUB                                                       4798
        SSCALE(ICOL)=ONE                                                    4799
  210 CONTINUE                                                              4800
C-----------------------------------------------------------------------    4801
C  PUT MAGNITUDES OF COLUMN VECTORS OF COEFFICIENT MATRIX IN                4802
C      S(J,1), J=1,NGL.                                                     4803
C-----------------------------------------------------------------------    4804
      DO 230 IROW=1,NY                                                      4805
        J=IROW                                                              4806
        CALL GETROW (J,S(1,3),.TRUE.,1,IUNIT,                               4807
     1  SQRTW,NY,NGL,NG,CQUAD,G,T,NLINF,Y,SSCALE)                           4808
        DO 240 ICOL=1,NGL                                                   4809
          S(ICOL,1)=S(ICOL,1)+ABS(S(ICOL,3))                                4810
          IF (S(ICOL,3) .GE. AAMIN(ICOL)) GO TO 245                         4811
          AAMIN(ICOL)=S(ICOL,3)                                             4812
          TMIN(ICOL)=T(IROW)                                                4813
  245     IF (S(ICOL,3) .LE. AAMAX(ICOL)) GO TO 240                         4814
          AAMAX(ICOL)=S(ICOL,3)                                             4815
          TMAX(ICOL)=T(IROW)                                                4816
  240   CONTINUE                                                            4817
  230 CONTINUE                                                              4818
C-----------------------------------------------------------------------    4819
C  S(J,2)=0 MEANS THAT COLUMN J OF THE REGULARIZOR IS ZERO AND              4820
C      THEREFORE THAT THE J-TH VARIABLE IS NOT REGULARIZED.                 4821
C  SSCALE(J) OF REGULARIZED VARIABLES ARE INITIALLY SET TO NORMALIZE        4822
C      THEIR COLUMN VECTORS IN THE REGULARIZOR.                             4823
C  NACTRG = NO. OF REGULARIZED VARIABLES.                                   4824
C  AVCOER = AVERAGE (OVER REGULARIZED VARIABLES) OF THE LENGTHS OF THE      4825
C      COLUMN VECTORS IN THE SCALED COEFFICIENT MATRIX.                     4826
C-----------------------------------------------------------------------    4827
      NACTRG=0                                                              4828
      AVCOER=0.                                                             4829
      DO 260 J=1,NGL                                                        4830
        IF (S(J,2) .LE. ZERO) GO TO 260                                     4831
        NACTRG=NACTRG+1                                                     4832
        SSCALE(J)=ONE/S(J,2)                                                4833
        AVCOER=AVCOER+S(J,1)*SSCALE(J)                                      4834
  260 CONTINUE                                                              4835
      IF (NACTRG .LE. 0) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                 4836
      AVCOER=AVCOER/FLOAT(NACTRG)                                           4837
C-----------------------------------------------------------------------    4838
C  SSCALE(J) FOR THE REGULARIZED VARIABLES ARE FINALLY ALL DIVIDED BY       4839
C      AVCOER.  THIS MAKES THE AVERAGE LENGTH OF THEIR COLUMN VECTORS       4840
C      IN THE SCALED COEFFICIENT MATRIX 1.                                  4841
C  SSCALE(J) FOR THE NON-REGULARIZED VARIABLES IS SET TO NORMALIZE          4842
C      THEIR COLUMN VECTORS IN THE SCALED COEFFICIENT MATRIX.               4843
C-----------------------------------------------------------------------    4844
      DO 280 J=1,NGL                                                        4845
        IF (S(J,2) .LE. ZERO) GO TO 285                                     4846
        SSCALE(J)=SSCALE(J)/AVCOER                                          4847
        GO TO 280                                                           4848
  285   IF (S(J,1) .LE. ZERO) CALL ERRMES (3,.TRUE.,IHOLER,NOUT)            4849
        SSCALE(J)=ONE/S(J,1)                                                4850
  280 CONTINUE                                                              4851
C-----------------------------------------------------------------------    4852
C  SCALE INEQUALITY CONSTRAINTS AND NORMALIZE THEIR ROW VECTORS.            4853
C-----------------------------------------------------------------------    4854
      IF (NINEQ .LE. 0) GO TO 400                                           4855
      DO 310 ICOL=1,NGL                                                     4856
        DUB=SSCALE(ICOL)                                                    4857
        DO 320 IROW=1,NINEQ                                                 4858
          AINEQ(IROW,ICOL)=AINEQ(IROW,ICOL)*DUB                             4859
  320   CONTINUE                                                            4860
  310 CONTINUE                                                              4861
      DO 340 IROW=1,NINEQ                                                   4862
        DUB=ZERO                                                            4863
        DO 350 ICOL=1,NGL                                                   4864
          DUB=DUB+ABS(AINEQ(IROW,ICOL))                                     4865
  350   CONTINUE                                                            4866
        IF (DUB .LE. ZERO) CALL ERRMES (4,.TRUE.,IHOLER,NOUT)               4867
        DUB=ONE/DUB                                                         4868
        DO 360 ICOL=1,NGLP1                                                 4869
          AINEQ(IROW,ICOL)=AINEQ(IROW,ICOL)*DUB                             4870
  360   CONTINUE                                                            4871
  340 CONTINUE                                                              4872
 5400 FORMAT (//3X,10HGRID POINT,5X,15HMIN IN MATRIX A,4X,6HAT T =,5X,      4873
     1 15HMAX IN MATRIX A,4X,6HAT T =,5X,12HSCALE FACTOR)                   4874
  400 WRITE (NOUT,5400)                                                     4875
C5410 FORMAT (1X,1PE12.4,E20.4,E10.2,E20.4,E10.2,E17.3)!SP                  4876
 5410 FORMAT (1X,1PE12.4,D20.4,D10.2,D20.4,D10.2,D17.3)                     4877
      WRITE (NOUT,5410) (G(J),AAMIN(J),TMIN(J),AAMAX(J),TMAX(J),            4878
     1 SSCALE(J),J=1,NG)                                                    4879
      IF (NG .GE. NGL) GO TO 800                                            4880
C5420 FORMAT (13H  NLINF TERMS,1PE20.4,E10.2,E20.4,E10.2,E17.3)!SP          4881
 5420 FORMAT (13H  NLINF TERMS,1PD20.4,D10.2,D20.4,D10.2,D17.3)             4882
      K=NG+1                                                                4883
      WRITE (NOUT,5420) (AAMIN(J),TMIN(J),AAMAX(J),TMAX(J),                 4884
     1 SSCALE(J),J=K,NGL)                                                   4885
  800 RETURN                                                                4886
      END                                                                   4887
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4888
C  SUBROUTINE SETSGN.  SETS UP LLSIGN(J) (+ OR - THE SUBSCRIPT OF THE       4889
C      J-TH EXTREMA, WITH + IF A MAX. AND - IF A MIN.)                      4890
C      FROM THE INPUT VALUES OF LSIGN(J,INSGN),                             4891
C      FOR J=1,...,NSGN(INSGN)+1=NSGNI+1.                                   4892
C  FOR THE SPECIAL CASE OF A SINGLE PEAK AND LSIGN(2,INSGN) INPUT AS 0,     4893
C      THE MIN. OF LSIGN(1,INSGN)*SOLBES (WHERE SOLBES IS THE BEST          4894
C      SOLUTION FOUND SO FAR) IS USED.                                      4895
C  LLSIGN(1) = + OR -1.  LLSIGN(NSGN(INSGN)+1)=NG.                          4896
C-----------------------------------------------------------------------    4897
C  CALLS SUBPROGRAMS - ERRMES                                               4898
C-----------------------------------------------------------------------    4899
      SUBROUTINE SETSGN (INSGN,NSGNI,LSIGN,NOUT,LLSIGN,NG,SOLBES,           4900
     1 SRANGE)                                                              4901
      DOUBLE PRECISION SOLBES                                               4902
      DIMENSION LSIGN(4,INSGN), LLSIGN(5), SOLBES(NG)                       4903
      DIMENSION IHOLER(6)                                                   4904
      DATA IHOLER/1HS, 1HE, 1HT, 1HS, 1HG, 1HN/                             4905
      IF (NSGNI.LT.1 .OR. NSGNI.GT.4 .OR. IABS(LSIGN(1,INSGN)).NE.1)        4906
     1 CALL ERRMES (1,.TRUE.,IHOLER,NOUT)                                   4907
      LLSIGN(1)=LSIGN(1,INSGN)                                              4908
      LLSIGN(NSGNI+1)=NG                                                    4909
      IF (NSGNI .EQ. 1) GO TO 800                                           4910
      DO 110 ISGN=2,NSGNI                                                   4911
        LLSIGN(ISGN)=LSIGN(ISGN,INSGN)                                      4912
        IF (IABS(LLSIGN(ISGN)) .GT. IABS(LLSIGN(ISGN-1)) .AND.              4913
     1  IABS(LLSIGN(ISGN)) .LT. NG   .AND.                                  4914
     2  LLSIGN(ISGN)*LLSIGN(ISGN-1) .LT. 0) GO TO 110                       4915
        IF (ISGN.NE.2 .OR. NSGNI.NE.2) CALL ERRMES(2,.TRUE.,IHOLER,NOUT)    4916
C-----------------------------------------------------------------------    4917
C  SELECT STARTING SINGLE PEAK FROM THE ABSOLUTE MIN. OF                    4918
C      LLSIGN(1)*SOLBES.                                                    4919
C-----------------------------------------------------------------------    4920
        F=FLOAT(LLSIGN(1))                                                  4921
        PK=SRANGE                                                           4922
        DO 120 J=1,NG                                                       4923
          DUM=F*SOLBES(J)                                                   4924
          IF (DUM .GE. PK) GO TO 120                                        4925
          PK=DUM                                                            4926
          LLSIGN(2)=-ISIGN(J,LLSIGN(1))                                     4927
  120   CONTINUE                                                            4928
  110 CONTINUE                                                              4929
  800 RETURN                                                                4930
      END                                                                   4931
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4932
C  SUBROUTINE SETVAL.  PUTS S(TILDE)(J,J) (IN EQ. (5.22)) IN S(J,2).        4933
C  EVALUAUES VALPHA(J) AND VALPCV(J), J=1,NGL, WHERE                        4934
C      VALPCV = K1*X1+K2*Z*H1**(-1)*R1 AND                                  4935
C      VALPHA = K1*X1+K2*Z*H1**(-1)*(W*S(TILDE)**(-1)*GAMMA(TILDE)+R1)      4936
C      IN EQ. (5.29).                                                       4937
C  UPDATES A AND RHSNEQ (LEFT- AND RIGHT-HAND SIDES OF EQ. (5.28)).         4938
C  INIT SHOULD ONLY BE .TRUE. ON THE FIRST CALL TO SETVAL AFTER A           4939
C      CALL TO SETGA1.                                                      4940
      SUBROUTINE SETVAL (ALPHA,INIT,NNINEQ,                                 4941
     1  A,AINEQ,MA,MG,MINEQ,MREG,NGL,NGLE,REG,RHSNEQ,S,VALPCV,VALPHA,       4942
     2  VK1Y1)                                                              4943
      DOUBLE PRECISION A, AINEQ, ALPHA, ALPHA2, DDUM, DUM, FACT,            4944
     1 ONE, REG, RHSNEQ, S, SQRT, VALPCV, VALPHA, VK1Y1                     4945
      LOGICAL INIT                                                          4946
      DIMENSION S(MG,3), VK1Y1(MG), REG(MREG,MG), A(MA,MG),                 4947
     1 VALPHA(MG), VALPCV(MG), AINEQ(MINEQ,MG), RHSNEQ(MINEQ)               4948
      SQRT(DUM)=DSQRT(DUM)                                                  4949
C     ONE=1.E0!SP                                                           4950
      ONE=1.D0                                                              4951
      IF (.NOT.INIT) GO TO 108                                              4952
      DO 105 J=1,NGLE                                                       4953
        S(J,2)=ONE                                                          4954
  105 CONTINUE                                                              4955
  108 ALPHA2=ALPHA**2                                                       4956
      DO 110 J=1,NGL                                                        4957
        VALPCV(J)=VK1Y1(J)                                                  4958
        VALPHA(J)=VK1Y1(J)                                                  4959
  110 CONTINUE                                                              4960
      NGLP1=NGL+1                                                           4961
      DO 120 J=1,NGLE                                                       4962
        DDUM=ONE/(S(J,1)**2+ALPHA2)                                         4963
        DUM=(ALPHA2*REG(J,NGLP1)+S(J,1)*A(J,NGLP1))*DDUM                    4964
        DDUM=SQRT(DDUM)                                                     4965
        FACT=DDUM/S(J,2)                                                    4966
        S(J,2)=DDUM                                                         4967
        DO 125 K=1,NGL                                                      4968
          VALPHA(K)=VALPHA(K)+DUM*REG(K,J)                                  4969
  125   CONTINUE                                                            4970
        IF (NNINEQ .LE. 0) GO TO 120                                        4971
C-----------------------------------------------------------------------    4972
C  COMPUTE VALPCV FOR LATER USE IN CVNEQ.                                   4973
C-----------------------------------------------------------------------    4974
        DUM=REG(J,NGLP1)                                                    4975
        DO 130 K=1,NGL                                                      4976
          VALPCV(K)=VALPCV(K)+DUM*REG(K,J)                                  4977
  130   CONTINUE                                                            4978
C-----------------------------------------------------------------------    4979
C  UPDATE NNINEQ-BY-NGLE INEQUALITY MATRIX A.                               4980
C-----------------------------------------------------------------------    4981
        DO 140 K=1,NNINEQ                                                   4982
          A(K,J)=FACT*A(K,J)                                                4983
  140   CONTINUE                                                            4984
  120 CONTINUE                                                              4985
      IF (NNINEQ .LE. 0) GO TO 800                                          4986
C-----------------------------------------------------------------------    4987
C  PUT RIGHT-HAND-SIDE OF INEQUALITY IN RHSNEQ.                             4988
C-----------------------------------------------------------------------    4989
      DO 150 K=1,NNINEQ                                                     4990
        DUM=AINEQ(K,NGLP1)                                                  4991
        DO 160 J=1,NGL                                                      4992
          DUM=DUM-AINEQ(K,J)*VALPHA(J)                                      4993
  160   CONTINUE                                                            4994
        RHSNEQ(K)=DUM                                                       4995
  150 CONTINUE                                                              4996
  800 RETURN                                                                4997
      END                                                                   4998
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    4999
C  SUBROUTINE SETWT.  COMPUTES SQRTW (SQUARE-ROOTS OF THE LEAST             5000
C      SQUARES WEIGHTS) USING ABS(Y-YLYFIT)=YFIT (THE ABSOLUTE VALUE        5001
C      OF THE FIT TO THE DATA FROM A PRELIMINARY UNWEIGHTED SOLUTION).      5002
C  ONLY CALLED IF IWT=2,3, OR 5.                                            5003
C  IWT = 2 WHEN VARIANCE (Y) IS PROPORTIONAL TO ABS(Y) (AS WITH POISSON     5004
C      STATISTICS).                                                         5005
C  IWT = 3 WHEN VARIANCE (Y) IS PROPORTIONAL TO Y**2.                       5006
C  IWT = 5 WHEN SQRTW IS TO BE CALCULATED BY THE USER-SUPPLIED              5007
C      ROUTINE USERWT.                                                      5008
C-----------------------------------------------------------------------    5009
C  CALLS SUBPROGRAMS - GETYLY, USERWT, ERRMES                               5010
C  WHICH IN TURN CALL - GETROW, USERK, USERLF, USERTR                       5011
C-----------------------------------------------------------------------    5012
      SUBROUTINE SETWT (                                                    5013
     1 CQUAD,G,IUNIT,IWT,MWORK,MY,NERFIT,NG,NGL,NLINF,NOUT,NY,PRWT,         5014
     2 SOLBES,SQRTW,SRANGE,SSCALE,T,WORK,Y,YLYFIT)                          5015
      DOUBLE PRECISION SOLBES, SSCALE, WORK                                 5016
      LOGICAL PRWT                                                          5017
      DIMENSION SOLBES(NGL), WORK(MWORK), SQRTW(MY), CQUAD(NG),             5018
     1 G(NG), T(MY), Y(MY), YLYFIT(MY), SSCALE(NGL)                         5019
      DIMENSION IHOLER(6)                                                   5020
      DATA IHOLER/1HS, 1HE, 1HT, 1HW, 1HT, 1H /                             5021
      CALL GETYLY (SOLBES,                                                  5022
     1 CQUAD,G,IUNIT,IWT,MWORK,NG,NGL,NLINF,NY,SQRTW,SSCALE,T,WORK,Y,       5023
     2 YLYFIT)                                                              5024
      ERRFIT=0.                                                             5025
      IF (NERFIT.LE.0) GO TO 200                                            5026
C-----------------------------------------------------------------------    5027
C  COMPUTE ERRFIT (ROOT-MEAN-SQUARE DEVIATION IN THE REGION                 5028
C      AROUND THE MIN. VALUE OF YFIT).  ERRFIT WILL BE                      5029
C      ADDED TO YFIT IN ORDER TO PREVENT DISASTROUSLY LARGE WEIGHTS         5030
C      BEING CALCULATED FROM 1./YFIT, WHEN YFIT HAPPENS TO BE NEAR 0.       5031
C  NERFIT SHOULD TYPICALLY BE ABOUT 10.                                     5032
C-----------------------------------------------------------------------    5033
      ABSMIN=SRANGE                                                         5034
      DO 110 J=1,NY                                                         5035
        DUM=ABS(Y(J)-YLYFIT(J))                                             5036
        IF (DUM .GE. ABSMIN) GO TO 110                                      5037
        ABSMIN=DUM                                                          5038
        L=J                                                                 5039
  110 CONTINUE                                                              5040
      JMAX=MIN0(NY,L+NERFIT/2)                                              5041
      JMIN=MAX0(1,JMAX-NERFIT+1)                                            5042
      DUM=0.                                                                5043
      DO 120 J=JMIN,JMAX                                                    5044
        DUM=DUM+YLYFIT(J)**2                                                5045
  120 CONTINUE                                                              5046
      ERRFIT=SQRT(DUM/FLOAT(JMAX-JMIN+1))                                   5047
  200 IF (IWT .NE. 5) GO TO 250                                             5048
      CALL USERWT (Y,YLYFIT,MY,ERRFIT,SQRTW)                                5049
      GO TO 700                                                             5050
  250 IF (IWT.NE.2 .AND. IWT.NE.3) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)       5051
      DO 260 J=1,NY                                                         5052
        DUM=AMAX1(ABS(Y(J)-YLYFIT(J)),ERRFIT)                               5053
        IF (DUM .LE. 0.) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                 5054
        SQRTW(J)=1./DUM                                                     5055
        IF (IWT .EQ. 2) SQRTW(J)=SQRT(SQRTW(J))                             5056
  260 CONTINUE                                                              5057
 5260 FORMAT (//9H ERRFIT =,1PE9.2/30X,                                     5058
     A 37HSQUARE ROOTS OF LEAST SQUARES WEIGHTS/                            5059
     1 (1X,1P10E13.4))                                                      5060
  700 IF (PRWT) WRITE (NOUT,5260) ERRFIT,(SQRTW(J),J=1,NY)                  5061
      RETURN                                                                5062
      END                                                                   5063
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    5064
C  SUBPROGRAM STORIN. STORES INPUT DATA AFTER CHECKING THAT                 5065
C      ARRAY DIMENSIONS WILL NOT BE EXCEEDED.                               5066
C-----------------------------------------------------------------------    5067
C  CALLS SUBPROGRAMS - ERRMES                                               5068
C-----------------------------------------------------------------------    5069
      SUBROUTINE STORIN (JL,NIOERR,LIN,IIN,RIN)                             5070
      DOUBLE PRECISION PRECIS, RANGE                                        5071
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  5072
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              5073
      LOGICAL LEQUIV(11)                                                    5074
      DIMENSION LIN(6), IEQUIV(14), REQUIV(2), IHOLER(6)                    5075
      COMMON /DBLOCK/ PRECIS, RANGE                                         5076
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         5077
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     5078
     2 SRANGE                                                               5079
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     5080
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       5081
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             5082
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              5083
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         5084
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            5085
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  5086
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            5087
     2 LUSER(30)                                                            5088
      EQUIVALENCE (IGRID,IEQUIV(1)), (DOCHOS,LEQUIV(1)),                    5089
     1 (DFMIN,REQUIV(1))                                                    5090
      DATA IHOLER/1HS, 1HT, 1HO, 1HR, 1HI, 1HN/                             5091
      IFINT(RIN)=INT(RIN+SIGN(.5,RIN))                                      5092
      IF (JL .GT. 2) GO TO 200                                              5093
      REQUIV(JL)=RIN                                                        5094
      RETURN                                                                5095
  200 IF (JL .GT. 7) GO TO 300                                              5096
      JLL=JL-2                                                              5097
      GO TO (203,204,205,206,207),JLL                                       5098
  203 IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 805                                 5099
      ALPST(IIN)=RIN                                                        5100
      RETURN                                                                5101
  204 IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 805                                 5102
      GMNMX(IIN)=RIN                                                        5103
      RETURN                                                                5104
 5205 FORMAT (4F5.2)                                                        5105
  205 READ (NIN,5205) PLEVEL                                                5106
      WRITE (NOUT,5205) PLEVEL                                              5107
      RETURN                                                                5108
 5206 FORMAT (4E10.3)                                                       5109
  206 READ (NIN,5206) RSVMNX                                                5110
      WRITE (NOUT,5206) RSVMNX                                              5111
      RETURN                                                                5112
C***********************************************************************    5113
C  IF YOU CHANGE THE DIMENSION OF RUSER IN COMMON, THEN YOU MUST ALSO       5114
C      CHANGE 100 IN THE FOLLOWING STATEMENT TO THE NEW DIMENSION.          5115
C***********************************************************************    5116
  207 IF (IIN.LT.1 .OR. IIN.GT.551) GO TO 805                               5117
      RUSER(IIN)=RIN                                                        5118
      RETURN                                                                5119
  300 IF (JL .GT. 21) GO TO 400                                             5120
      IEQUIV(JL-7)=IFINT(RIN)                                               5121
      RETURN                                                                5122
  400 IF (JL .GT. 37) GO TO 500                                             5123
      JLL=JL-21                                                             5124
      GO TO (422,423,424,425,426,427,428,429,430,431,432,433,434,435,       5125
     1 436,437),JLL                                                         5126
  422 IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 805                                 5127
      ICRIT(IIN)=IFINT(RIN)                                                 5128
      RETURN                                                                5129
 5423 FORMAT (1X,70A1)                                                      5130
  423 READ (NIN,5423) IFORMT                                                5131
      WRITE (NOUT,5423) IFORMT                                              5132
      RETURN                                                                5133
  424 READ (NIN,5423) IFORMW                                                5134
      WRITE (NOUT,5423) IFORMW                                              5135
      RETURN                                                                5136
  425 READ (NIN,5423) IFORMY                                                5137
      WRITE (NOUT,5423) IFORMY                                              5138
      RETURN                                                                5139
  426 IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 805                                 5140
      IPLFIT(IIN)=IFINT(RIN)                                                5141
      RETURN                                                                5142
  427 IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 805                                 5143
      IPLRES(IIN)=IFINT(RIN)                                                5144
      RETURN                                                                5145
  428 IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 805                                 5146
      IPRINT(IIN)=IFINT(RIN)                                                5147
      RETURN                                                                5148
C***********************************************************************    5149
C  IF YOU CHANGE THE DIMENSION OF IUSER IN COMMON, THEN YOU MUST ALSO       5150
C      CHANGE 50 IN THE FOLLOWING STATEMENT TO THE NEW DIMENSION.           5151
C***********************************************************************    5152
  429 IF (IIN.LT.1 .OR. IIN.GT.50) GO TO 805                                5153
      IUSER(IIN)=IFINT(RIN)                                                 5154
      RETURN                                                                5155
  430 IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 805                                 5156
      IUSROU(IIN)=IFINT(RIN)                                                5157
      RETURN                                                                5158
 5431 FORMAT (16I5)                                                         5159
  431 READ (NIN,5431) LSIGN                                                 5160
      WRITE (NOUT,5431) LSIGN                                               5161
      RETURN                                                                5162
  432 IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 805                                 5163
      MOMNMX(IIN)=IFINT(RIN)                                                5164
      RETURN                                                                5165
  433 IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 805                                 5166
      NENDZ(IIN)=IFINT(RIN)                                                 5167
      RETURN                                                                5168
  434 READ (NIN,5431) NFLAT                                                 5169
      WRITE (NOUT,5431) NFLAT                                               5170
      RETURN                                                                5171
  435 IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 805                                 5172
      NNSGN(IIN)=IFINT(RIN)                                                 5173
      RETURN                                                                5174
  436 IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 805                                 5175
      NQPROG(IIN)=IFINT(RIN)                                                5176
      RETURN                                                                5177
  437 IF (IIN.LT.1 .OR. IIN.GT.4) GO TO 805                                 5178
      NSGN(IIN)=IFINT(RIN)                                                  5179
      RETURN                                                                5180
  500 L=IFINT(RIN)                                                          5181
      IF (IABS(L) .EQ. 1) GO TO 510                                         5182
      CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                                   5183
      GO TO 810                                                             5184
  510 IF (JL .GT. 48) GO TO 600                                             5185
      LEQUIV(JL-37)=L .EQ. 1                                                5186
      RETURN                                                                5187
C***********************************************************************    5188
C  IF YOU CHANGE THE DIMENSION OF LUSER IN COMMON, THEN YOU MUST ALSO       5189
C      CHANGE 30 IN THE FOLLOWING STATEMENT TO THE NEW DIMENSION.           5190
C***********************************************************************    5191
  600 IF (IIN.LT.1 .OR. IIN.GT.30) GO TO 805                                5192
      LUSER(IIN)=L .EQ. 1                                                   5193
      RETURN                                                                5194
  805 CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                                   5195
  810 NIOERR=NIOERR+1                                                       5196
      IF (NIOERR .GE. MIOERR) STOP                                          5197
 5002 FORMAT (/1H )                                                         5198
      WRITE (NOUT,5002)                                                     5199
      RETURN                                                                5200
      END                                                                   5201
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    5202
C     SUBROUTINE SVDRS2 (A,MDA,MM,NN,B,MDB,NB,S,IERROR,RANGE)               5203
C  BASED ON LAWSON-HANSON SVDRS EXCEPT THAT STOP IS REPLACED WITH           5204
C      IERROR=5 AND RETURN, AND IERROR=6 IF MIN0(MM,NN).LE.0.               5205
C  ON A NORMAL RETURN, IERROR=1.                                            5206
C  RANGE IS 2 OR 3 ORDERS OF MAGNITUDE SMALLER THAN BIG, WHERE BIG IS       5207
C      THE LARGEST NUMBER THAT DOES NOT OVERFLOW AND 1/BIG DOES NOT         5208
C      UNDERFLOW.  FOR THE DOUBLE PRECISION VERSION, BIG AND RANGE          5209
C      ARE IN DOUBLE PRECISION.  FOR THE SINGLE PRECISION VERSION,          5210
C      THEY ARE IN SINGLE PRECISION (AND THEREFORE RANGE=SRANGE).           5211
C-----------------------------------------------------------------------    5212
C  CALLS SUBPROGRAMS - H12, QRBD                                            5213
C  WHICH IN TURN CALL - G1, G2, DIFF                                        5214
C-----------------------------------------------------------------------    5215
C         SINGULAR VALUE DECOMPOSITION ALSO TREATING RIGHT SIDE VECTOR.     5216
C                                                                           5217
C     THE ARRAY S OCCUPIES 3*N CELLS.                                       5218
C     A OCCUPIES M*N CELLS                                                  5219
C     B OCCUPIES M*NB CELLS.                                                5220
C                                                                           5221
C     SPECIAL SINGULAR VALUE DECOMPOSITION SUBROUTINE.                      5222
C     WE HAVE THE M X N MATRIX A AND THE SYSTEM A*X=B TO SOLVE.             5223
C     EITHER M .GE. N  OR  M .LT. N IS PERMITTED.                           5224
C                  THE SINGULAR VALUE DECOMPOSITION                         5225
C     A = U*S*V**(T) IS MADE IN SUCH A WAY THAT ONE GETS                    5226
C       (1) THE MATRIX V IN THE FIRST N ROWS AND COLUMNS OF A.              5227
C       (2) THE DIAGONAL MATRIX OF ORDERED SINGULAR VALUES IN               5228
C       THE FIRST N CELLS OF THE ARRAY S(IP), IP .GE. 3*N.                  5229
C       (3) THE MATRIX PRODUCT U**(T)*B=G GETS PLACED BACK IN B.            5230
C       (4) THE USER MUST COMPLETE THE SOLUTION AND DO HIS OWN              5231
C       SINGULAR VALUE ANALYSIS.                                            5232
C     *******                                                               5233
C     GIVE SPECIAL                                                          5234
C     TREATMENT TO ROWS AND COLUMNS WHICH ARE ENTIRELY ZERO.  THIS          5235
C     CAUSES CERTAIN ZERO SING. VALS. TO APPEAR AS EXACT ZEROS RATHER       5236
C     THAN AS ABOUT ETA TIMES THE LARGEST SING. VAL.   IT SIMILARLY         5237
C     CLEANS UP THE ASSOCIATED COLUMNS OF U AND V.                          5238
C     METHOD..                                                              5239
C     1. EXCHANGE COLS OF A TO PACK NONZERO COLS TO THE LEFT.               5240
C        SET N = NO. OF NONZERO COLS.                                       5241
C        USE LOCATIONS A(1,NN),A(1,NN-1),...,A(1,N+1) TO RECORD THE         5242
C        COL PERMUTATIONS.                                                  5243
C     2. EXCHANGE ROWS OF A TO PACK NONZERO ROWS TO THE TOP.                5244
C        QUIT PACKING IF FIND N NONZERO ROWS.  MAKE SAME ROW EXCHANGES      5245
C        IN B.  SET M SO THAT ALL NONZERO ROWS OF THE PERMUTED A            5246
C        ARE IN FIRST M ROWS.  IF M .LE. N THEN ALL M ROWS ARE              5247
C        NONZERO.  IF M .GT. N THEN THE FIRST N ROWS ARE KNOWN              5248
C        TO BE NONZERO,AND ROWS N+1 THRU M MAY BE ZERO OR NONZERO.          5249
C     3. APPLY ORIGINAL ALGORITHM TO THE M BY N PROBLEM.                    5250
C     4. MOVE PERMUTATION RECORD FROM A(,) TO S(I),I=N+1,...,NN.            5251
C     5. BUILD V UP FROM  N BY N  TO NN BY NN BY PLACING ONES ON            5252
C        THE DIAGONAL AND ZEROS ELSEWHERE.  THIS IS ONLY PARTLY DONE        5253
C        EXPLICITLY.  IT IS COMPLETED DURING STEP 6.                        5254
C     6. EXCHANGE ROWS OF V TO COMPENSATE FOR COL EXCHANGES OF STEP 2.      5255
C     7. PLACE ZEROS IN  S(I),I=N+1,NN  TO REPRESENT ZERO SING VALS.        5256
C                                                                           5257
      SUBROUTINE SVDRS2 (A,MDA,MM,NN,B,MDB,NB,S,IERROR,RANGE)               5258
      DOUBLE PRECISION A, B, ONE, RANGE, S, T, ZERO                         5259
      DIMENSION A(MDA,1),B(MDB,1),S(NN,3)                                   5260
C     ZERO=0.E0!SP                                                          5261
      ZERO=0.D0                                                             5262
C     ONE=1.E0!SP                                                           5263
      ONE=1.D0                                                              5264
C                                                                           5265
C          BEGIN.. SPECIAL FOR ZERO ROWS AND COLS.                          5266
C                                                                           5267
C          PACK THE NONZERO COLS TO THE LEFT                                5268
C                                                                           5269
      N=NN                                                                  5270
      IERROR=6                                                              5271
      IF (N.LE.0.OR.MM.LE.0) RETURN                                         5272
      IERROR=1                                                              5273
      J=N                                                                   5274
   10 CONTINUE                                                              5275
         DO 20 I=1,MM                                                       5276
         IF (A(I,J)) 50,20,50                                               5277
   20    CONTINUE                                                           5278
C                                                                           5279
C        COL J  IS ZERO. EXCHANGE IT WITH COL N.                            5280
C                                                                           5281
      IF (J.EQ.N) GO TO 40                                                  5282
         DO 30 I=1,MM                                                       5283
   30    A(I,J)=A(I,N)                                                      5284
   40 CONTINUE                                                              5285
      A(1,N)=J                                                              5286
      N=N-1                                                                 5287
   50 CONTINUE                                                              5288
      J=J-1                                                                 5289
      IF (J.GE.1) GO TO 10                                                  5290
C          IF N=0 THEN A IS ENTIRELY ZERO AND SVD                           5291
C          COMPUTATION CAN BE SKIPPED                                       5292
      NS=0                                                                  5293
      IF (N.EQ.0) GO TO 240                                                 5294
C          PACK NONZERO ROWS TO THE TOP                                     5295
C          QUIT PACKING IF FIND N NONZERO ROWS                              5296
      I=1                                                                   5297
      M=MM                                                                  5298
   60 IF (I.GT.N.OR.I.GE.M) GO TO 150                                       5299
      IF (A(I,I)) 90,70,90                                                  5300
   70    DO 80 J=1,N                                                        5301
         IF (A(I,J)) 90,80,90                                               5302
   80    CONTINUE                                                           5303
      GO TO 100                                                             5304
   90 I=I+1                                                                 5305
      GO TO 60                                                              5306
C          ROW I IS ZERO                                                    5307
C          EXCHANGE ROWS I AND M                                            5308
  100 IF(NB.LE.0) GO TO 115                                                 5309
         DO 110 J=1,NB                                                      5310
         T=B(I,J)                                                           5311
         B(I,J)=B(M,J)                                                      5312
  110    B(M,J)=T                                                           5313
  115    DO 120 J=1,N                                                       5314
  120    A(I,J)=A(M,J)                                                      5315
      IF (M.GT.N) GO TO 140                                                 5316
         DO 130 J=1,N                                                       5317
  130    A(M,J)=ZERO                                                        5318
  140 CONTINUE                                                              5319
C          EXCHANGE IS FINISHED                                             5320
      M=M-1                                                                 5321
      GO TO 60                                                              5322
C                                                                           5323
  150 CONTINUE                                                              5324
C          END.. SPECIAL FOR ZERO ROWS AND COLUMNS                          5325
C          BEGIN.. SVD ALGORITHM                                            5326
C     METHOD..                                                              5327
C     (1)  REDUCE THE MATRIX TO UPPER BIDIAGONAL FORM WITH                  5328
C     HOUSEHOLDER TRANSFORMATIONS.                                          5329
C         H(N)...H(1)AQ(1)...Q(N-2) = (D**T,0)**T                           5330
C     WHERE D IS UPPER BIDIAGONAL.                                          5331
C                                                                           5332
C     (2)  APPLY H(N)...H(1) TO B. HERE H(N)...H(1)*B REPLACES B            5333
C     IN STORAGE.                                                           5334
C                                                                           5335
C     (3)  THE MATRIX PRODUCT W= Q(1)...Q(N-2) OVERWRITES THE FIRST         5336
C     N ROWS OF A IN STORAGE.                                               5337
C                                                                           5338
C     (4)  AN SVD FOR D IS COMPUTED.  HERE K ROTATIONS RI AND PI ARE        5339
C     COMPUTED SO THAT                                                      5340
C         RK...R1*D*P1**(T)...PK**(T) = DIAG(S1,...,SM)                     5341
C     TO WORKING ACCURACY.  THE SI ARE NONNEGATIVE AND NONINCREASING.       5342
C     HERE RK...R1*B OVERWRITES B IN STORAGE WHILE                          5343
C     A*P1**(T)...PK**(T)  OVERWRITES A IN STORAGE.                         5344
C                                                                           5345
C     (5)  IT FOLLOWS THAT,WITH THE PROPER DEFINITIONS,                     5346
C     U**(T)*B OVERWRITES B, WHILE V OVERWRITES THE FIRST N ROW AND         5347
C     COLUMNS OF A.                                                         5348
C                                                                           5349
      L=MIN0(M,N)                                                           5350
C        THE FOLLOWING LOOP REDUCES A TO UPPER BIDIAGONAL AND               5351
C        ALSO APPLIES THE PREMULTIPLYING TRANSFORMATIONS TO B.              5352
C                                                                           5353
         DO 170 J=1,L                                                       5354
         IF (J.GE.M) GO TO 160                                              5355
         JJ=J                                                               5356
         CALL H12 (1,JJ,J+1,M,A(1,J),1,T,A(1,J+1),1,MDA,N-J,RANGE)          5357
         CALL H12 (2,JJ,J+1,M,A(1,J),1,T,B,1,MDB,NB,RANGE)                  5358
  160    IF (J.GE.N-1) GO TO 170                                            5359
         CALL H12 (1,J+1,J+2,N,A(J,1),MDA,S(J,3),A(J+1,1),MDA,1,M-J,        5360
     1   RANGE)                                                             5361
  170    CONTINUE                                                           5362
C                                                                           5363
C     COPY THE BIDIAGONAL MATRIX INTO THE ARRAY S() FOR QRBD.               5364
C                                                                           5365
      IF (N.EQ.1) GO TO 190                                                 5366
         DO 180 J=2,N                                                       5367
         S(J,1)=A(J,J)                                                      5368
  180    S(J,2)=A(J-1,J)                                                    5369
  190 S(1,1)=A(1,1)                                                         5370
C                                                                           5371
      NS=N                                                                  5372
      IF (M.GE.N) GO TO 200                                                 5373
      NS=M+1                                                                5374
      S(NS,1)=ZERO                                                          5375
      S(NS,2)=A(M,M+1)                                                      5376
  200 CONTINUE                                                              5377
C                                                                           5378
C     CONSTRUCT THE EXPLICIT N BY N PRODUCT MATRIX, W=Q1*Q2*...*QL*I        5379
C     IN THE ARRAY A().                                                     5380
C                                                                           5381
         DO 230 K=1,N                                                       5382
         I=N+1-K                                                            5383
         IF(I.GT.MIN0(M,N-2)) GO TO 210                                     5384
         CALL H12 (2,I+1,I+2,N,A(I,1),MDA,S(I,3),A(1,I+1),1,MDA,N-I,        5385
     1   RANGE)                                                             5386
  210    DO 220 J=1,N                                                       5387
  220    A(I,J)=ZERO                                                        5388
  230    A(I,I)=ONE                                                         5389
C                                                                           5390
C         COMPUTE THE SVD OF THE BIDIAGONAL MATRIX                          5391
C                                                                           5392
      CALL QRBD (IPASS,S(1,1),S(1,2),NS,A,MDA,N,B,MDB,NB,RANGE)             5393
C                                                                           5394
      GO TO (240,310), IPASS                                                5395
  240 CONTINUE                                                              5396
      IF (NS.GE.N) GO TO 260                                                5397
      NSP1=NS+1                                                             5398
         DO 250 J=NSP1,N                                                    5399
  250    S(J,1)=ZERO                                                        5400
  260 CONTINUE                                                              5401
      IF (N.EQ.NN) RETURN                                                   5402
      NP1=N+1                                                               5403
C               MOVE RECORD OF PERMUTATIONS                                 5404
C               AND STORE ZEROS                                             5405
         DO 280 J=NP1,NN                                                    5406
         S(J,1)=A(1,J)                                                      5407
         DO 270 I=1,N                                                       5408
  270    A(I,J)=ZERO                                                        5409
  280    CONTINUE                                                           5410
C          PERMUTE ROWS AND SET ZERO SINGULAR VALUES.                       5411
         DO 300 K=NP1,NN                                                    5412
         I=S(K,1)                                                           5413
         S(K,1)=ZERO                                                        5414
         DO 290 J=1,NN                                                      5415
         A(K,J)=A(I,J)                                                      5416
  290    A(I,J)=ZERO                                                        5417
         A(I,K)=ONE                                                         5418
  300    CONTINUE                                                           5419
C          END.. SPECIAL FOR ZERO ROWS AND COLUMNS                          5420
      RETURN                                                                5421
  310 IERROR=5                                                              5422
      RETURN                                                                5423
      END                                                                   5424
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    5425
C  SUBROUTINE UPDDON.  FOR SOLUTION NO. NDONE, VARI IS STORED IN            5426
C      VDONE(NDONE).  HOWEVER, IF A MATCH BETWEEN SOLUTION NDONE AND A      5427
C      PREVIOUSLY STORED SOLUTION, SAY K, IS FOUND, THEN VARI IS SET        5428
C      TO VDONE(K), NDONE IS DECREMENTED, AND THE SOLUTION IS NOT           5429
C      STORED.                                                              5430
C  IABS(LSDONE(NDONE,J,K)) ARE THE MIN. (K=1) AND THE MAX. (K=2) GRID       5431
C      POINTS OF THE RANGE CONTAINING EXTREMUM J (THE EXTREMUM AT GRID      5432
C      POINT LLSIGN(J+1)) FOR WHICH THE MONOTONICITY CONSTRAINTS ARE        5433
C      BINDING.  THIS MEANS THAT THE SOLUTION IS EQUAL (I.E., FLAT)         5434
C      FOR ALL POINTS IN THIS RANGE.  IF THE EXTREMUM IS AT  AN END         5435
C      OF THE RANGE, THEN IN ORDER TO SAY A PRIORI THAT A NEW SET OF        5436
C      EXTREMA WILL GIVE THE IDENTICAL SOLUTION (AND THUS ALLOW THE         5437
C      ANALYSIS TO BE SKIPPED), ONE OF THE NEW EXTREMA MUST COINCIDE        5438
C      WITH THIS EXTREMUM.  THIS IS SIGNIFIED BY APPENDING A MINUS          5439
C      SIGN TO THE LSDONE(NDONE,J,K) THAT COINCIDES WITH THIS               5440
C      EXTREMUM.  (OTHERWISE, IT IS ONLY NECESSARY THAT THE NEW             5441
C      EXTREMUM LIE IN THE CLOSED INTERVAL                                  5442
C      (LSDONE(NDONE,J,1),LSDONE(NDONE,J,2)).)                              5443
      SUBROUTINE UPDDON (                                                   5444
     1 NSGNM1,LLSIGN,LSDONE,MDONE,NDONE,NNQUSR,LBIND,MINEQ,                 5445
     2 NG,VARI,VDONE)                                                       5446
      LOGICAL LBIND, STORE                                                  5447
      DIMENSION LLSIGN(5), LSDONE(MDONE,3,2), LBIND(MINEQ),                 5448
     1 VDONE(MDONE)                                                         5449
      STORE=NSGNM1 .GE. 1                                                   5450
      IF (.NOT.STORE) GO TO 700                                             5451
      VDONE(NDONE)=VARI                                                     5452
      DO 110 J=1,NSGNM1                                                     5453
        L=IABS(LLSIGN(J+1))                                                 5454
        KK=L-IABS(LLSIGN(J))-1                                              5455
        LL=L+NNQUSR+1                                                       5456
        IF (KK .EQ. 0) GO TO 130                                            5457
        DO 120 K=1,KK                                                       5458
          LL=LL-1                                                           5459
          IF (.NOT.LBIND(LL)) GO TO 130                                     5460
  120   CONTINUE                                                            5461
        LL=LL-1                                                             5462
  130   LSDONE(NDONE,J,1)=MIN0(LL-NNQUSR+1,L)                               5463
        KK=IABS(LLSIGN(J+2))-L-1                                            5464
        LL=NNQUSR-1+L                                                       5465
        IF (KK .EQ. 0) GO TO 150                                            5466
        DO 140 K=1,KK                                                       5467
          LL=LL+1                                                           5468
          IF (.NOT.LBIND(LL)) GO TO 150                                     5469
  140   CONTINUE                                                            5470
        LL=LL+1                                                             5471
  150   LSDONE(NDONE,J,2)=MAX0(LL-NNQUSR-1,L)                               5472
        IF (LSDONE(NDONE,J,1) .EQ. L) LSDONE(NDONE,J,1)=-L                  5473
        IF (LSDONE(NDONE,J,2) .EQ. L) LSDONE(NDONE,J,2)=-L                  5474
  110 CONTINUE                                                              5475
C-----------------------------------------------------------------------    5476
C  CHECK TO SEE IF THIS IS IDENTICAL TO A PREVIOUSLY STORED SOLUTION.       5477
C-----------------------------------------------------------------------    5478
      IF (NDONE .LE. 1) GO TO 700                                           5479
      KK=NDONE-1                                                            5480
      DO 210 K=1,KK                                                         5481
        DO 220 J=1,NSGNM1                                                   5482
          IF (IABS(LSDONE(NDONE,J,1)) .NE. IABS(LSDONE(K,J,1))  .OR.        5483
     1    IABS(LSDONE(NDONE,J,2)) .NE. IABS(LSDONE(K,J,2))) GO TO 210       5484
  220   CONTINUE                                                            5485
        VARI=VDONE(K)                                                       5486
        STORE=.FALSE.                                                       5487
        GO TO 700                                                           5488
  210 CONTINUE                                                              5489
  700 IF (.NOT.STORE) NDONE=NDONE-1                                         5490
      RETURN                                                                5491
      END                                                                   5492
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    5493
C  SUBROUTINE UPDLLS.  UPDATES LLSIGN (CURRENT POSITION OF EXTREMA),        5494
C      LLSTRY, AND VARTRY.  CHECKS LLSIGN FOR OVERLAP OF EXTREMA, AND,      5495
C      IF SO, BUMPS EXTREMA LYING TO THE RIGHT FARTHER TO THE RIGHT,        5496
C      IF POSSIBLE.                                                         5497
C  LLSTRY(J,L) = POSITION OF EXTREMA GIVING THE SOLUTION WITH THE           5498
C      LOWEST VARIANCE (VARTRY(L)) SO FAR AS THE POSITION OF                5499
C      EXTREMUM L IS VARIED.  LLSTRY IS ALWAYS POSITIVE.                    5500
C-----------------------------------------------------------------------    5501
C  CALLS SUBPROGRAMS - ERRMES                                               5502
C-----------------------------------------------------------------------    5503
      SUBROUTINE UPDLLS (NSGNI,JSTAGE,NOUT,VARTRY,VARI,LLSTRY,              5504
     1 LLSIGN,INC,DONE)                                                     5505
      LOGICAL DONE                                                          5506
      DIMENSION JSTAGE(NSGNI), VARTRY(NSGNI), LLSTRY(5,NSGNI),              5507
     1 LLSIGN(5), INC(NSGNI)                                                5508
      DIMENSION IHOLER(6)                                                   5509
      DATA IHOLER/1HU, 1HP, 1HD, 1HL, 1HL, 1HS/                             5510
      IF (NSGNI .LE. 1) GO TO 790                                           5511
      NSGNP1=NSGNI+1                                                        5512
      DONE=.FALSE.                                                          5513
      L=NSGNP1                                                              5514
      DO 200 JLL=2,NSGNI                                                    5515
C-----------------------------------------------------------------------    5516
C  L RUNS FROM NSGNI TO 2.                                                  5517
C-----------------------------------------------------------------------    5518
        L=L-1                                                               5519
C-----------------------------------------------------------------------    5520
C  JSTAGE(L) = 1,2, OR .GE.3 FOR INITIAL ANALYSIS, ANALYSIS WITH            5521
C      EXTREMUM L MOVING RIGHT AND LOOKING FOR AN INCREASE IN VARI,         5522
C      AND FOR THE FINAL STAGE WITH EXTREMUM L MOVING IN THE FINAL          5523
C      DIRECTION AFTER AN INCREASE IN VARI IN ONE DIRECTION HAS             5524
C      ALREADY BEEN FOUND, RESPECTIVELY.                                    5525
C-----------------------------------------------------------------------    5526
        JSTAGE(L)=JSTAGE(L)+1                                               5527
        IF (JSTAGE(L) .LT. 1) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)            5528
        IF (JSTAGE(L)-2) 300,400,500                                        5529
  300   IF (L .LT. NSGNI) GO TO 330                                         5530
C-----------------------------------------------------------------------    5531
C  INITIALIZE VARTRY AND LLSTRY WHEN JSTAGE(NSGNI)=1 (OR WHEN VARI          5532
C      HAS DECREASED).                                                      5533
C-----------------------------------------------------------------------    5534
        VARTRY(L)=VARI                                                      5535
        DO 310 J=1,NSGNP1                                                   5536
          LLSTRY(J,L)=IABS(LLSIGN(J))                                       5537
  310   CONTINUE                                                            5538
        GO TO 350                                                           5539
C-----------------------------------------------------------------------    5540
C  INITIALIZE VARTRY AND LLSTRY WHEN JSTAGE(L)=1 (OR WHEN VARI HAS          5541
C      DECREASED) AND WHEN L.LT.NSGNI                                       5542
C-----------------------------------------------------------------------    5543
  330   VARTRY(L)=VARTRY(L+1)                                               5544
        DO 340 J=1,NSGNP1                                                   5545
          LLSTRY(J,L)=LLSTRY(J,L+1)                                         5546
  340   CONTINUE                                                            5547
C-----------------------------------------------------------------------    5548
C  UPDATE LLSIGN, CHECK FOR OVERLAP OF EXTREMA, AND, IF SO, BUMP            5549
C      THE EXTREMA WITH HIGHER L TO THE RIGHT, IF POSSIBLE.                 5550
C-----------------------------------------------------------------------    5551
  350   DO 360 LL=2,NSGNI                                                   5552
          LLSIGN(LL)=ISIGN(LLSTRY(LL,L),LLSIGN(LL))                         5553
  360   CONTINUE                                                            5554
        LL=LLSTRY(L,L)+INC(L)                                               5555
        LLSIGN(L)=ISIGN(LL,LLSIGN(L))                                       5556
C                                                                           5557
C  IF NO OVERLAP, THEN RETURN.                                              5558
C                                                                           5559
        IF (LL.GT.LLSTRY(L-1,L) .AND. LL.LT.LLSTRY(L+1,L)) GO TO 800        5560
C                                                                           5561
C  IF OVERLAP IS TO THE LEFT OR IF L=NSGNI, THEN INCREMENT JSTAGE(L).       5562
C                                                                           5563
        IF (LL.LE.LLSTRY(L-1,L) .OR. L.GE.NSGNI) GO TO 370                  5564
C                                                                           5565
C  BUMP EXTREMUM L+1 TO THE RIGHT.                                          5566
C                                                                           5567
        LLSIGN(L+1)=ISIGN(LLSTRY(L+1,L)+1,LLSIGN(L+1))                      5568
        IF (IABS(LLSIGN(L+1)) .LT. LLSTRY(L+2,L)) GO TO 800                 5569
C                                                                           5570
C  IF THERE ARE NO MORE EXTREMA TO THE RIGHT THAT CAN BUMPED, THEN          5571
C      INCREMENT JSTAGE(L).                                                 5572
C                                                                           5573
        IF (L+1 .GE. NSGNI) GO TO 370                                       5574
C                                                                           5575
C  BUMP EXTREMUM L+2 TO THE RIGHT.                                          5576
C                                                                           5577
        LLSIGN(L+2)=ISIGN(LLSTRY(L+2,L)+1,LLSIGN(L+2))                      5578
        IF (IABS(LLSIGN(L+2)) .LT. LLSTRY(L+3,L)) GO TO 800                 5579
C                                                                           5580
C  OVERLAP COULD NOT BE REMEDIED BY BUMPING.  IF JSTAGE(L)=1, THEN          5581
C      SET IT TO 2.  OTHERWISE, INCREMENT JSTAGE(L-1).                      5582
C                                                                           5583
  370   IF (JSTAGE(L) .GT. 1) GO TO 510                                     5584
        JSTAGE(L)=2                                                         5585
        GO TO 420                                                           5586
C-----------------------------------------------------------------------    5587
C  TESTS WHEN JSTAGE(L)=2.                                                  5588
C-----------------------------------------------------------------------    5589
C  IF VARI HAS DECREASED, THEN CONTINUE IN THE SAME DIRECTION               5590
C      WITH JSTAGE(3)=3 THE NEXT ITERATION.                                 5591
C                                                                           5592
  400   IF (VARI-VARTRY(L)) 300,410,420                                     5593
C                                                                           5594
C  IF THE IDENTICAL VARIANCE HAS BEEN OBTAINED, THEN REPEAT                 5595
C      WITH JSTAGE(L)=2 AGAIN IN THE NEXT ITERATION.                        5596
C                                                                           5597
  410   JSTAGE(L)=1                                                         5598
        GO TO 300                                                           5599
C                                                                           5600
C  IF VARI HAS INCREASED, THEN TURN AROUND AND SEARCH TO THE LEFT           5601
C      WITH JSTAGE(L)=3 IN THE NEXT ITERATION.                              5602
C                                                                           5603
  420   INC(L)=-1                                                           5604
        GO TO 350                                                           5605
C-----------------------------------------------------------------------    5606
C  TESTS WHEN JSTAGE(L)=3.                                                  5607
C-----------------------------------------------------------------------    5608
C  IF VARI HAS NOT INCREASED, THEN CONTINUE WITH JSTAGE(L)=JSTAGE(L)+1      5609
C      IN THE NEXT ITERATION.                                               5610
C                                                                           5611
  500   IF (VARI .LE. VARTRY(L)) GO TO 300                                  5612
C                                                                           5613
C  IF VARI HAS INCREASED, THEN INCREMENT JSTAGE(L-1) AND ZERO               5614
C      JSTAGE(LL),LL=L,NSGNI.                                               5615
C                                                                           5616
  510   DO 520 LL=L,NSGNI                                                   5617
          JSTAGE(LL)=0                                                      5618
          INC(LL)=1                                                         5619
  520   CONTINUE                                                            5620
        VARI=VARTRY(L)                                                      5621
  200 CONTINUE                                                              5622
C-----------------------------------------------------------------------    5623
C  PEAK-CONSTRAINED ANALYSIS IS DONE.  LOCAL MIN. HAS BEEN FOUND.           5624
C-----------------------------------------------------------------------    5625
  790 DONE=.TRUE.                                                           5626
  800 RETURN                                                                5627
      END                                                                   5628
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    5629
C  SUBROUTINE UPDSGN.  UPDATES SIGNS OF A AND RHSNEQ (LEFT- AND             5630
C      RIGHT-HAND SIDES OF EQ. (5.28)), AS WELL AS IISIGN(J)                5631
C      (MONOTONICITY INDICATOR FOR J-TH GRID POINT) TO AGREE WITH           5632
C       LLLSGN IN ANPEAK.                                                   5633
C-----------------------------------------------------------------------    5634
C  CALLS SUBPROGRAMS - ERRMES                                               5635
C-----------------------------------------------------------------------    5636
      SUBROUTINE UPDSGN (NNSGNI,LLLSGN,                                     5637
     1 A,AINEQ,IISIGN,MA,MG,MINEQ,MREG,NGLE,NGLP1,NNINEQ,                   5638
     2 NNQUSR,NONNEG,NOUT,REG,RHSNEQ,S,VALPHA)                              5639
      DOUBLE PRECISION A, AINEQ, ONE, REG, RHSNEQ, S, VALPHA, ZERO          5640
      LOGICAL NONNEG                                                        5641
      DIMENSION LLLSGN(5), A(MA,MG), REG(MREG,MG), S(MG,3),                 5642
     1 RHSNEQ(MINEQ), VALPHA(MG), IISIGN(MG), AINEQ(MINEQ,MG)               5643
      DIMENSION IHOLER(6)                                                   5644
      DATA IHOLER/1HU, 1HP, 1HD, 1HS, 1HG, 1HN/                             5645
C     ZERO=0.E0!SP                                                          5646
      ZERO=0.D0                                                             5647
C     ONE=1.E0!SP                                                           5648
      ONE=1.D0                                                              5649
      IROW=NNQUSR                                                           5650
      IIROW=NNINEQ                                                          5651
      NNSGNP=NNSGNI+1                                                       5652
      DO 110 JS=1,NNSGNP                                                    5653
        LS=LLLSGN(JS)                                                       5654
        ICMIN=IABS(LS)                                                      5655
        IF (JS .LE. NNSGNI) GO TO 114                                       5656
        IF (NONNEG .AND. LLLSGN(NNSGNI).GT.0) GO TO 116                     5657
        GO TO 110                                                           5658
  114   ICMAX=IABS(LLLSGN(JS+1))-1                                          5659
        IF (ICMIN .GT. ICMAX) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)            5660
        IF (.NOT.NONNEG .OR. LS.GT.0) GO TO 130                             5661
C-----------------------------------------------------------------------    5662
C  CONSTRAIN MINIMA TO BE NONNEGATIVE.                                      5663
C-----------------------------------------------------------------------    5664
  116   IIROW=IIROW+1                                                       5665
        IF (IIROW .GT. MINEQ) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)            5666
        DO 120 IICOL=1,NGLE                                                 5667
          A(IIROW,IICOL)=REG(ICMIN,IICOL)*S(IICOL,2)                        5668
  120   CONTINUE                                                            5669
        RHSNEQ(IIROW)=-VALPHA(ICMIN)                                        5670
        DO 125 IICOL=1,NGLP1                                                5671
          AINEQ(IIROW,IICOL)=ZERO                                           5672
  125   CONTINUE                                                            5673
        AINEQ(IIROW,ICMIN)=ONE                                              5674
        IF (JS. GT. NNSGNI) GO TO 110                                       5675
C-----------------------------------------------------------------------    5676
C  UPDATE SIGNS OF INCREASING OR DECREASING CONSTRAINTS.                    5677
C-----------------------------------------------------------------------    5678
  130   DO 140 ICOL=ICMIN,ICMAX                                             5679
          IROW=IROW+1                                                       5680
          IF (IROW .GT. NNINEQ) CALL ERRMES (3,.TRUE.,IHOLER,NOUT)          5681
          IF (IISIGN(ICOL)*LS .GT. 0) GO TO 140                             5682
          IISIGN(ICOL)=-IISIGN(ICOL)                                        5683
          DO 145 IICOL=1,NGLE                                               5684
            A(IROW,IICOL)=-A(IROW,IICOL)                                    5685
  145     CONTINUE                                                          5686
          RHSNEQ(IROW)=-RHSNEQ(IROW)                                        5687
  140   CONTINUE                                                            5688
  110 CONTINUE                                                              5689
      RETURN                                                                5690
      END                                                                   5691
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    5692
C  SUBROUTINE WRITIN.  WRITES OUT FINAL VALUES OF COMMON                    5693
C      VARIABLES (AFTER CHANGES DUE TO INPUT DATA AND USERIN), AS WELL      5694
C      AS THE REST OF THE INPUT AND ANY SIMULATED DATA.                     5695
C-----------------------------------------------------------------------    5696
C  CALLS SUBPROGRAMS - WRITYT                                               5697
C-----------------------------------------------------------------------    5698
      SUBROUTINE WRITIN (EXACT,G,LA,MG,MY,SQRTW,T,Y)                        5699
      DOUBLE PRECISION PRECIS, RANGE                                        5700
      LOGICAL DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST, NEWPG1,                  5701
     1 NONNEG, ONLY1, PRWT, PRY, SIMULA, LUSER                              5702
      LOGICAL LEQUIV(11), LSPACE                                            5703
      DIMENSION EXACT(MY), SQRTW(MY), T(MY), Y(MY), LA(6,50)                5704
      DIMENSION IEQUIV(14), G(MG)                                           5705
      COMMON /DBLOCK/ PRECIS, RANGE                                         5706
      COMMON /SBLOCK/ DFMIN, SRMIN,                                         5707
     1 ALPST(2), EXMAX, GMNMX(2), PLEVEL(2,2), RSVMNX(2,2), RUSER(551),     5708
     2 SRANGE                                                               5709
      COMMON /IBLOCK/ IGRID, IQUAD, IUNIT, IWT, LINEPG,                     5710
     1 MIOERR, MPKMOM, MQPITR, NEQ, NERFIT, NG, NINTT, NLINF, NORDER,       5711
     2 IAPACK(6), ICRIT(2), IFORMT(70), IFORMW(70), IFORMY(70),             5712
     3 IPLFIT(2), IPLRES(2), IPRINT(2), ITITLE(80), IUSER(50),              5713
     4 IUSROU(2), LSIGN(4,4), MOMNMX(2), NENDZ(2), NFLAT(4,2), NGL,         5714
     5 NGLP1, NIN, NINEQ, NNSGN(2), NOUT, NQPROG(2), NSGN(4), NY            5715
      COMMON /LBLOCK/ DOCHOS, DOMOM, DOUSIN, DOUSNQ, LAST,                  5716
     1 NEWPG1, NONNEG, ONLY1, PRWT, PRY, SIMULA,                            5717
     2 LUSER(30)                                                            5718
      EQUIVALENCE (IGRID,IEQUIV(1)), (DOCHOS,LEQUIV(1))                     5719
      L=1                                                                   5720
      IF (IWT.EQ.1 .OR. IWT.EQ.4) L=2                                       5721
      LSPACE=MAX0(IUSROU(L),IPRINT(L)) .GE. 4                               5722
 5999 FORMAT (1H1)                                                          5723
      IF (LSPACE) WRITE(NOUT,5999)                                          5724
 5003 FORMAT (//1H )                                                        5725
      IF (.NOT.LSPACE) WRITE(NOUT,5003)                                     5726
 5100 FORMAT (40X,33HFINAL VALUES OF CONTROL VARIABLES)                     5727
      WRITE (NOUT,5100)                                                     5728
 5110 FORMAT (1X,6A1,2H =,1P10E12.5/(9X,10E12.5))                           5729
      WRITE (NOUT,5110) (LA(K,1),K=1,6),DFMIN                               5730
      WRITE (NOUT,5110) (LA(K,2),K=1,6),SRMIN                               5731
      WRITE (NOUT,5110) (LA(K,3),K=1,6),ALPST                               5732
      WRITE (NOUT,5110) (LA(K,4),K=1,6),GMNMX                               5733
      WRITE (NOUT,5110) (LA(K,5),K=1,6),PLEVEL                              5734
      WRITE (NOUT,5110) (LA(K,6),K=1,6),RSVMNX                              5735
      WRITE (NOUT,5110) (LA(K,7),K=1,6),RUSER                               5736
      JJ=7                                                                  5737
 5210 FORMAT (1X,6A1,2H =,10I12/(9X,10I12))                                 5738
      DO 210 J=1,14                                                         5739
        JJ=JJ+1                                                             5740
        WRITE (NOUT,5210) (LA(K,JJ),K=1,6),IEQUIV(J)                        5741
  210 CONTINUE                                                              5742
      WRITE (NOUT,5210) (LA(K,22),K=1,6),ICRIT                              5743
 5220 FORMAT (1X,6A1,3H = ,80A1)                                            5744
      WRITE (NOUT,5220) (LA(K,23),K=1,6),IFORMT                             5745
      WRITE (NOUT,5220) (LA(K,24),K=1,6),IFORMW                             5746
      WRITE (NOUT,5220) (LA(K,25),K=1,6),IFORMY                             5747
      WRITE (NOUT,5210) (LA(K,26),K=1,6),IPLFIT                             5748
      WRITE (NOUT,5210) (LA(K,27),K=1,6),IPLRES                             5749
      WRITE (NOUT,5210) (LA(K,28),K=1,6),IPRINT                             5750
      WRITE (NOUT,5210) (LA(K,29),K=1,6),IUSER                              5751
      WRITE (NOUT,5210) (LA(K,30),K=1,6),IUSROU                             5752
      WRITE (NOUT,5210) (LA(K,31),K=1,6),LSIGN                              5753
      WRITE (NOUT,5210) (LA(K,32),K=1,6),MOMNMX                             5754
      WRITE (NOUT,5210) (LA(K,33),K=1,6),NENDZ                              5755
      WRITE (NOUT,5210) (LA(K,34),K=1,6),NFLAT                              5756
      WRITE (NOUT,5210) (LA(K,35),K=1,6),NNSGN                              5757
      WRITE (NOUT,5210) (LA(K,36),K=1,6),NQPROG                             5758
      WRITE (NOUT,5210) (LA(K,37),K=1,6),NSGN                               5759
      JJ=37                                                                 5760
 5310 FORMAT (1X,6A1,2H =,10L12/(9X,10L12))                                 5761
      DO 310 J=1,11                                                         5762
        JJ=JJ+1                                                             5763
        WRITE (NOUT,5310) (LA(K,JJ),K=1,6),LEQUIV(J)                        5764
  310 CONTINUE                                                              5765
      WRITE (NOUT,5310) (LA(K,49),K=1,6),LUSER                              5766
      IF (.NOT.SIMULA .AND. NY.LE.MY) CALL WRITYT (EXACT,                   5767
     1 G,IPRINT,IUSROU,IWT,MG,NOUT,NY,PRY,SIMULA,SQRTW,T,Y)                 5768
C5320 FORMAT (9H0PRECIS =,1PE9.2,10X,8HSRANGE =,E9.2,!SP                    5769
C    1 5X,7HRANGE =,E9.2)!SP                                                5770
 5320 FORMAT (9H0PRECIS =,1PD9.2,10X,8HSRANGE =,E9.2,                       5771
     1 5X,7HRANGE =,D9.2)                                                   5772
      WRITE (NOUT,5320) PRECIS, SRANGE, RANGE                               5773
      RETURN                                                                5774
      END                                                                   5775
C++++++++++++++++ DOUBLE PRECISION VERSION 2DP (MAR 1984) ++++++++++++++    5776
C  SUBROUTINE WRITYT.  IF PRY=.TRUE., THEN WRITYT WRITES Y (INPUT DATA),    5777
C      T (INDEPENDENT                                                       5778
C      VARIABLE), AND, IF SIMULA=.TRUE., EXACT (EXACT SIMULATED DATA)       5779
C      AND (Y-EXACT), AND, IF IWT=4, SQRTW (SQUARE ROOTS OF WEIGHTS).       5780
      SUBROUTINE WRITYT (EXACT,G,IPRINT,IUSROU,IWT,MG,NOUT,NY,              5781
     1 PRY,SIMULA,SQRTW,T,Y)                                                5782
      LOGICAL PRY, SIMULA, LSPACE                                           5783
      DIMENSION EXACT(NY), SQRTW(NY), T(NY), Y(NY), G(MG),                  5784
     1 IPRINT(2), IUSROU(2)                                                 5785
      IF (.NOT.PRY) GO TO 700                                               5786
      L=1                                                                   5787
      IF (IWT.EQ.1 .OR. IWT.EQ.4) L=2                                       5788
      LSPACE=MAX0(IUSROU(L),IPRINT(L)) .GE. 4                               5789
 5999 FORMAT (1H1)                                                          5790
      IF (LSPACE) WRITE (NOUT,5999)                                         5791
 5003 FORMAT (//1X)                                                         5792
      IF (.NOT.LSPACE) WRITE(NOUT,5003)                                     5793
      IF (SIMULA) GO TO 200                                                 5794
 5110 FORMAT (5(12X,1HT,12X,1HY)/(2X,1PE11.3,E13.5,E13.3,E13.5,             5795
     1 E13.3,E13.5,E13.3,E13.5,E13.3,E13.5))                                5796
      IF (IWT .NE. 4) WRITE (NOUT,5110) (T(J),Y(J),J=1,NY)                  5797
 5120 FORMAT (3(17X,1HT,12X,1HY,8X,5HSQRTW)/(5X,1P3E13.5,5X,3E13.5,         5798
     1 5X,3E13.5))                                                          5799
      IF (IWT .EQ. 4) WRITE (NOUT,5120) (T(J),Y(J),SQRTW(J),J=1,NY)         5800
      GO TO 700                                                             5801
 5210 FORMAT (2(17X,1HT,12X,1HY,8X,5HEXACT,8X,5HERROR))                     5802
  200 IF (IWT .NE. 4) WRITE (NOUT,5210)                                     5803
 5211 FORMAT (2(12X,1HT,12X,1HY,8X,5HEXACT,8X,5HERROR,8X,                   5804
     1 5HSQRTW))                                                            5805
      IF (IWT .EQ. 4) WRITE (NOUT,5211)                                     5806
      DO 210 J=2,NY,2                                                       5807
        DUM=Y(J-1)-EXACT(J-1)                                               5808
        DDUM=Y(J)-EXACT(J)                                                  5809
 5220   FORMAT (5X,1P4E13.5,5X,4E13.5)                                      5810
        IF (IWT .NE. 4) WRITE (NOUT,5220) T(J-1),Y(J-1),EXACT(J-1),DUM,     5811
     1  T(J),Y(J),EXACT(J),DDUM                                             5812
 5221   FORMAT (2X,1PE11.3,4E13.5,E13.3,4E13.5)                             5813
        IF (IWT .EQ. 4) WRITE (NOUT,5221) T(J-1),Y(J-1),EXACT(J-1),DUM,     5814
     1  SQRTW(J-1),T(J),Y(J),EXACT(J),DDUM,SQRTW(J)                         5815
  210 CONTINUE                                                              5816
      IF (MOD(NY,2) .EQ. 0) GO TO 700                                       5817
      DUM=Y(NY)-EXACT(NY)                                                   5818
      IF (IWT .NE. 4) WRITE (NOUT,5220) T(NY),Y(NY),EXACT(NY),DUM           5819
      IF (IWT .EQ. 4) WRITE (NOUT,5221) T(NY),Y(NY),EXACT(NY),DUM,          5820
     1 SQRTW(NY)                                                            5821
  700 RETURN                                                                5822
      END                                                                   5823

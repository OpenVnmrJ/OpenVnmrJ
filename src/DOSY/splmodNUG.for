C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++MAIN0001
C     *** MINOR MODIFICATIONS FOR FITTING DIFFUSION DECAYS FROM NON-
C     ***    UNIFORM GRADIENTS
C     SPLMOD.  MAIN SUBPROGRAM.                                         MAIN0002
C     FOR THE ANALYSIS OF SUMS OF ONE-PARAMETER FUNCTIONS.              MAIN0003
C     APPROXIMATES OPTIONALLY THE MODEL WITH CUBIC B-SPLINES            MAIN0004
C     (SPLINE=.TRUE.) OR USES EXACT MODEL (SPLINE=.FALSE.).             MAIN0005
C                                                                       MAIN0006
C     Author: Robert H. Vogel
C
C     REFERENCES - R.H.VOGEL (1988) SPLMOD USERS MANUAL, EMBL TECHNICAL MAIN0007
C                            REPORT DA09, (EUROPEAN MOLECULAR BIOLOGY   MAIN0008
C                            LABORATORY, HEIDELBERG, F.R. OF GERMANY)   MAIN0009
C                  S.W.PROVENCHER, R.H.VOGEL (1983) IN PROGRESS IN      MAIN0010
C                            SCIENTIFIC COMPUTING, VOL. 2, PP. 304-319, MAIN0011
C                            P.DEUFLHARD AND E.HAIRER EDS. 
c                            (BIRKHAEUSER, BOSTON)
C                                                                       MAIN0013
c  Download the Users Manual and other essential documentation from
C      http://S-provencher.COM
c
C     DISTRIBUTION AND USE OF THIS PROGRAM IS UNRESTRICTED AS LONG AS   MAIN0014
C     NO CHARGE IS MADE AND PROPER REFERENCE GIVEN. HOWEVER, AS A NEW   MAIN0015
C     USER YOU SHOULD NOTIFY Stephen W. Provencher at:
C      sp@S-provencher.COM
c
C-----------------------------------------------------------------------MAIN0026
C     SUBPROGRAMS CALLED - ANALYZ, CHOTRD, ERRMES, INICAP, INIT,        MAIN0027
C                          INPUT,  SETWT,  USERSI, USERTR, WRITYT       MAIN0028
C     WHICH IN TURN CALL - BCOEFF, BETAIN, BSPLIN, CHOSOL, DIFF,        MAIN0029
C                          ERRMES, FISHNI, FULLSM, GAMLN,  GETPRU,      MAIN0030
C                          HESEXA, HESSEN, HESSPL, INTERP, INTERV,      MAIN0031
C                          LINEQS, LINPAR, LSTSQR, NOKSUB, NPASCL,      MAIN0032
C                          OUTALL, OUTCOR, OUTITR, OUTPAR, PGAUSS,      MAIN0033
C                          PIVOT,  PIVOT1, PLPRIN, PLRES,  RANDOM,      MAIN0034
C                          READYT, RGAUSS, STORIN, USEREX, USERFL,      MAIN0035
C                          USERFN, USERIN, USEROU, USERST, USERTR,      MAIN0036
C                          USERWT, VARIAN, WRITIN, WRITYT               MAIN0037
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               MAIN0038
C-----------------------------------------------------------------------MAIN0039
      DOUBLE PRECISION PRECIS, RANGE,                                   MAIN0040
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       MAIN0041
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           MAIN0042
     3                 TWOTHR, FORTHR, SIXTEL                           MAIN0043
      DOUBLE PRECISION EK, EPK, ETE, R, DELTAL, DELTAN, GTG, H,         MAIN0044
     1                 DELTAH, EKEK, ASAVE, ATOT, DTOLER, DX, BSPL0,    MAIN0045
     2                 BSPL1, BSPL2, CCAP, ECAP, YCAP, SGGCAP,          MAIN0046
     3                 SGYCAP, YW, BETA, ZGRID, D, U, WORK, YKN,        MAIN0047
     4                 SWKN                                             MAIN0048
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  MAIN0049
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    MAIN0050
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  MAIN0051
     3                 SPLINE, WRTBET, WRTCAP                           MAIN0052
      LOGICAL          PIVLIN, PIVLAM, STARTV                           MAIN0053
      DIMENSION        BOUNDL(2), BOUNDN(2), IHOLER(6)                  MAIN0054
C***********************************************************************MAIN0055
C  THE INSTRUCTIONS SET OFF BY ASTERISKS DESCRIBE ALL POSSIBLE CHANGES  MAIN0056
C  THAT YOU HAVE TO MAKE IN THIS SUBPROGRAM. (SEE ALSO THE CHANGES      MAIN0057
C  IN THE BLOCK DATA AND USER SUBPROGRAMS.) THESE CHANGES IN THE MAIN   MAIN0058
C  PROGRAM ARE ONLY NECESSARY IF YOU CHANGE                             MAIN0059
C  NYMAX,  NDMAX,  NONLMX,  NGAMMX,  NLINMX,  NLNDMX,  NZMAX, OR MTRYMX MAIN0060
C  IN THE DATA STATEMENTS BELOW. IF YOU DO, THEN THE                    MAIN0061
C  FOLLOWING DIMENSIONS MUST BE READJUSTED AS DESCRIBED BELOW -         MAIN0062
C                                                                       MAIN0063
      DIMENSION        Y(1024,3), T(1024,3), YLYFIT(1024,3),            MAIN0064
     1                 SQRTW(1024,3), RESPON(1024)                      MAIN0065
      DIMENSION        PLAM(5),   PLMTRY(5), EPK(5), DELTAN(5),         MAIN0066
     1                 NOKSET(5), NCOMBI(5),  DX(5),     IC(5)          MAIN0067
      DIMENSION        BSPL0(4,5), BSPL1(4,5), BSPL2(4,5)               MAIN0068
      DIMENSION        PLMSAV(5,10), DELTAH(5,3)                        MAIN0069
      DIMENSION        GTG(5,5)                                         MAIN0070
      DIMENSION        EK(9), DELTAL(9)                                 MAIN0071
      DIMENSION        PLIN(9,3), PLNTRY(9,3), PIVLIN(9,3), R(9,3)      MAIN0072
      DIMENSION        ETE(9,9)                                         MAIN0073
      DIMENSION        EKEK(9,9,3), ASAVE(9,9,3), H(9,9,3)              MAIN0074
      DIMENSION        NY(3), YW(3), ERRFIT(3)                          MAIN0075
      DIMENSION        PIVLAM(32), DTOLER(32), BUFF(32)                 MAIN0076
      DIMENSION        ATOT(32,32)                                      MAIN0077
      DIMENSION        ZGRID(52), D(52), U(52), WORK(52), BETA(52)      MAIN0078
      DIMENSION        CCAP(52,52,3), ECAP(52,4,3), YCAP(52,3),         MAIN0079
     1                 SGGCAP(4,4,3), SGYCAP(4,3)                       MAIN0080
      DIMENSION        STARTV(50)                                       MAIN0081
C                                                                       MAIN0082
C  THE ABOVE DIMENSION STATEMENTS MUST BE ADJUSTED AS FOLLOWS -         MAIN0083
C                                                                       MAIN0084
C     DIMENSION      Y(NYMAX,NDMAX),     T(NYMAX,NDMAX),                MAIN0085
C               YLYFIT(NYMAX,NDMAX), SQRTW(NYMAX,NDMAX),                MAIN0086
C                                                                       MAIN0087
C               RESPON(NYMAX)                                           MAIN0088
C                                                                       MAIN0089
C  YOU CAN SAVE STORAGE BY DIMENSIONING  RESPON  AS  RESPON(1)          MAIN0090
C  IF YOU NEVER WORK WITH CONVOLUTIONS (IUSER(9).NE.2)                  MAIN0091
C                                                                       MAIN0092
C     DIMENSION   PLAM(NONLMX), PLMTRY(NONLMX),    EPK(NONLMX),         MAIN0093
C               DELTAN(NONLMX), NOKSET(NONLMX), NCOMBI(NONLMX),         MAIN0094
C                   DX(NONLMX),     IC(NONLMX)                          MAIN0095
C     DIMENSION BSPL0(IB,NONLMX), BSPL1(IB,NONLMX), BSPL2(IB,NONLMX)    MAIN0096
C     DIMENSION PLMSAV(NONLMX,10), DELTAH(NONLMX,NDMAX)                 MAIN0097
C     DIMENSION GTG(NONLMX,NONLMX)                                      MAIN0098
C     DIMENSION EK(NLINMX), DELTAL(NLINMX)                              MAIN0099
C     DIMENSION   PLIN(NLINMX,NDMAX), PLNTRY(NLINMX,NDMAX),             MAIN0100
C               PIVLIN(NLINMX,NDMAX),      R(NLINMX,NDMAX)              MAIN0101
C     DIMENSION ETE(NLINMX,NLINMX)                                      MAIN0102
C     DIMENSION EKEK(NLINMX,NLINMX,NDMAX), ASAVE(NLINMX,NLINMX,NDMAX)   MAIN0103
C                  H(NLINMX,NLINMX,NDMAX)                               MAIN0104
C     DIMENSION NY(NDMAX), YW(NDMAX), ERRFIT(NDMAX)                     MAIN0105
C     DIMENSION PIVLAM(NLNDMX), DTOLER(NLNDMX), BUFF(NLNDMX)            MAIN0106
C     DIMENSION ATOT(NLNDMX,NLNDMX))                                    MAIN0107
C     DIMENSION ZGRID(NZMAX), D(NZMAX), U(NZMAX), WORK(NZMAX),          MAIN0108
C                BETA(NZMAX)                                            MAIN0109
C     DIMENSION CCAP(NZMAX,NZMAX,NDMAX), ECAP(NZMAX ,NGAMMX,NDMAX),     MAIN0110
C               YCAP(NZMAX,NDMAX),     SGGCAP(NGAMMX,NGAMMX,NDMAX),     MAIN0111
C               SGYCAP(NGAMMX,NDMAX)                                    MAIN0112
C     DIMENSION STARTV(MTRYMX)                                          MAIN0113
C                                                                       MAIN0114
C***********************************************************************MAIN0115
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        MAIN0116
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, MAIN0117
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 MAIN0118
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),MAIN0119
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     MAIN0120
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       MAIN0121
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       MAIN0122
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   MAIN0123
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    MAIN0124
     4                 MTRY(10,2), MXITER(2), NL(10),                   MAIN0125
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  MAIN0126
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, MAIN0127
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       MAIN0128
     8                 NIN, NOUT                                        MAIN0129
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          MAIN0130
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         MAIN0131
     2                 DOUSOU(2), LUSER(30),                            MAIN0132
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  MAIN0133
     4                 SPLINE, WRTBET, WRTCAP                           MAIN0134
C***********************************************************************MAIN0135
C  YOU CAN SAVE STORAGE BY MAKING THE INTEGERS IN THE FOLLOWING DATA    MAIN0136
C  STATEMENTS AS SMALL AS THE SIZE OF YOUR PROBLEM WILL ALLOW. (SEE     MAIN0137
C  SEC 3.5 IN THE USERS MANUAL FOR THE MINIMUM ALLOWABLE VALUES.)       MAIN0138
C                                                                       MAIN0139
      DATA             NYMAX /1024/, NDMAX /3/, NONLMX /5/, NGAMMX/4/   MAIN0140
C                                                                       MAIN0141
C  NLINMX = NONLMX + NGAMMX                                             MAIN0142
C  NLNDMX = NDMAX * NLINMX + NONLMX                                     MAIN0143
C                                                                       MAIN0144
      DATA             NLINMX/9/, NLNDMX /32/                           MAIN0145
C                                                                       MAIN0146
      DATA             NZMAX  /52/, MTRYMX/50/                          MAIN0147
C                                                                       MAIN0148
C  IF YOU CHANGE THE ABOVE DATA STATEMENTS, THE DIMENSION               MAIN0149
C  STATEMENTS ABOVE MUST BE READJUSTED, AS DESCRIBED ABOVE.             MAIN0150
C                                                                       MAIN0151
C  THIS IS THE END OF ALL POSSIBLE CHANGES THAT YOU MIGHT HAVE TO MAKE  MAIN0152
C  IN THE MAIN PROGRAM,                                                 MAIN0153
C  EXECPT THAT IF YOUR SYSTEM DOES NOT AUTOMATICALLY OPEN INPUT AND     MAIN0154
C  OUTPUT FILES FOR YOU, THEN YOU MIGHT HAVE TO OPEN THEM HERE AND      MAIN0155
C  GIVE THEM THE NUMBERS   NIN   (FOR THE INPUT) AND   NOUT   (FOR THE  MAIN0156
C  OUTPUT), WHERE   NIN   AND   NOUT   HAVE BEEN SET IN THE BLOCK DATA  MAIN0157
C  SUBPROGRAM.                                                          MAIN0158
C  IN ADDITION, IF YOU ARE GOING TO INPUT   IRWCAP.NE.0,   THEN YOU MAY MAIN0159
C  HAVE TO OPEN A TEMPORARY SCRATCH FILE NUMBERED   IUNIT.   DO THIS    MAIN0160
C  DIRECTLY AFTER STATEMENT 100. THIS IS NOT NECESSARY IF   IRWCAP   IS MAIN0161
C  ZERO OR IF YOUR SYSTEM OPENS FILES AUTOMATICALLY.                    MAIN0162
C                                                                       MAIN0163
C***********************************************************************MAIN0164
      DATA             IHOLER/1HM, 1HA, 1HI, 1HN, 1H , 1H /             MAIN0165
C-----------------------------------------------------------------------MAIN0166
C  OPEN FILES                                                           MAIN0167
C-----------------------------------------------------------------------MAIN0168
C                                                                       MAIN0169
C                                                                       MAIN0170
C-----------------------------------------------------------------------MAIN0171
C  INITIALIZE CERTAIN VARIABLES                                         MAIN0172
C-----------------------------------------------------------------------MAIN0173
      CALL INIT                                                         MAIN0174
C-----------------------------------------------------------------------MAIN0175
C  READ INPUT DATA                                                      MAIN0176
C-----------------------------------------------------------------------MAIN0177
 100  CALL INPUT (NYMAX,Y,T,YLYFIT,SQRTW,NY,NONLMX,NLINMX,NGAMMX,NZMAX, MAIN0178
     1            NDMAX,NLNDMX,MTRYMX,RESPON,BOUNDL,BOUNDN)             MAIN0179
C-----------------------------------------------------------------------MAIN0180
C  OPEN FILE FOR INPUT AND OUTPUT OF B-SPLINE COEFFICIENTS AND          MAIN0181
C  ARRAYS ...CAP IF ONE OF THE LOGICALS REDBET, REDCAP, WRTBET,         MAIN0182
C  OR WRTCAP IS .TRUE. (I.E. IRWCAP.NE.0)                               MAIN0183
C-----------------------------------------------------------------------MAIN0184
C     IF (IRWCAP .NE. 0) OPEN..........                                 MAIN0185
C                                                                       MAIN0186
C-----------------------------------------------------------------------MAIN0187
C  CALCULATE SIMULATED DATA                                             MAIN0188
C-----------------------------------------------------------------------MAIN0189
      IF (SIMULA) CALL USERSI (NYMAX,Y,T,YLYFIT,NY,RESPON)              MAIN0190
      IF (SIMULA .AND. PRY) CALL WRITYT (                               MAIN0191
     1    NYMAX,Y,T,YLYFIT,SQRTW,NY,ND,IWT,SIMULA,NOUT)                 MAIN0192
C-----------------------------------------------------------------------MAIN0193
C  INITIAL COMPUTATIONS.                                                MAIN0194
C-----------------------------------------------------------------------MAIN0195
      NGAMM1=NGAM-1                                                     MAIN0196
      NGAMP1=NGAM+1                                                     MAIN0197
      IF (NGAM .GT. 1) NPLINE=MIN0(8,NGAM)                              MAIN0198
      IWTIST=.TRUE.                                                     MAIN0199
      SPLINE=.FALSE.                                                    MAIN0200
      IWTSAV=IWT                                                        MAIN0201
      I=1                                                               MAIN0202
      IF (IWT.EQ.1 .OR. IWT.EQ.4) I=2                                   MAIN0203
      DO 120 J=1,10                                                     MAIN0204
         IF (.NOT. DOSPL(J,I)) GO TO 120                                MAIN0205
         SPLINE=.TRUE.                                                  MAIN0206
         GO TO 130                                                      MAIN0207
 120  CONTINUE                                                          MAIN0208
C-----------------------------------------------------------------------MAIN0209
C  COMPUTE ACTUAL BOUNDS FOR LINEAR AND NONLINEAR PARAMETERS.           MAIN0210
C-----------------------------------------------------------------------MAIN0211
 130  AYMAX=0.                                                          MAIN0212
      DO 134 N=1,ND                                                     MAIN0213
         MY=NY(N)                                                       MAIN0214
         DO 132 K=1,MY                                                  MAIN0215
 132     AYMAX=AMAX1(AYMAX,ABS(Y(K,N)))                                 MAIN0216
 134  CONTINUE                                                          MAIN0217
      PLMNMX(1)=PLMNMX(1)*AYMAX                                         MAIN0218
      PLMNMX(2)=PLMNMX(2)*AYMAX                                         MAIN0219
      T1       =SRANGE                                                  MAIN0220
      TN       =-SRANGE                                                 MAIN0221
      DO 140 N=1,ND                                                     MAIN0222
         MY  =NY(N)                                                     MAIN0223
         DDUM=-SRANGE                                                   MAIN0224
         DO 138 J=1,5                                                   MAIN0225
            DUM=SRANGE                                                  MAIN0226
            DO 136 K=1,MY                                               MAIN0227
               TKN=T(K,N)                                               MAIN0228
               IF (J .EQ. 1) TN=AMAX1(TN,TKN)                           MAIN0229
               IF (TKN.GT.DDUM .AND. TKN.LT.DUM) DUM=TKN                MAIN0230
 136        CONTINUE                                                    MAIN0231
            DDUM=DUM+1.E-5*ABS(DUM)                                     MAIN0232
            IF (J .EQ. 1) TT1=DUM                                       MAIN0233
 138     CONTINUE                                                       MAIN0234
         IF (T1 .LE. TT1) GO TO 140                                     MAIN0235
         T1=TT1                                                         MAIN0236
         T5=DUM                                                         MAIN0237
 140  CONTINUE                                                          MAIN0238
      DUM      =.25*(T5-T1)                                             MAIN0239
      DUM      =DUM-T1                                                  MAIN0240
      PNMNMX(1)=PNMNMX(1)/(TN+DUM)                                      MAIN0241
      PNMNMX(2)=PNMNMX(2)/(T1+DUM)                                      MAIN0242
      IF (NGAM .EQ. 0) WRITE (NOUT,1000)                                MAIN0243
     1                        PLMNMX(1),PLMNMX(2),PNMNMX(1),PNMNMX(2)   MAIN0244
      IF (NGAM .GT. 0) WRITE (NOUT,2000)                                MAIN0245
     1                        PLMNMX(1),PLMNMX(2),PNMNMX(1),PNMNMX(2)   MAIN0246
      IF (PLMNMX(1).GT.PLMNMX(2) .OR. PNMNMX(1).GT.PNMNMX(2))           MAIN0247
     1                        CALL ERRMES (1,.TRUE.,IHOLER,NOUT)        MAIN0248
C-----------------------------------------------------------------------MAIN0249
C  DO ALL EVALUATIONS ON   USERTR(PLAM)   AXIS                          MAIN0250
C-----------------------------------------------------------------------MAIN0251
      DUM      =PNMNMX(1)                                               MAIN0252
      PNMNMX(1)=USERTR(DUM,1)                                           MAIN0253
      DUM      =PNMNMX(2)                                               MAIN0254
      PNMNMX(2)=USERTR(DUM,1)                                           MAIN0255
      ZTOTAL   =PNMNMX(2)-PNMNMX(1)                                     MAIN0256
      IF (.NOT. SPLINE) GO TO 200                                       MAIN0257
C-----------------------------------------------------------------------MAIN0258
C  BUILD EQUIDISTANT INTERPOLATION GRID                                 MAIN0259
C-----------------------------------------------------------------------MAIN0260
         DZ      =ZTOTAL/FLOAT(NZ-3)                                    MAIN0261
         DZINV   =ONE/DZ                                                MAIN0262
         ZSTART  =PNMNMX(1)-DZ                                          MAIN0263
         ZGRID(1)=ZSTART                                                MAIN0264
         DO 150 J=2,NZ                                                  MAIN0265
 150     ZGRID(J)=ZGRID(J-1)+DZ                                         MAIN0266
C-----------------------------------------------------------------------MAIN0267
C  DECOMPOSE SPECIAL TRIDIAGONAL MATRIX FOR SOLVING A SYSTEM OF         MAIN0268
C  LINEAR EQUATIONS FOR B-SPLINES COEFFICIENTS.                         MAIN0269
C-----------------------------------------------------------------------MAIN0270
         CALL CHOTRD (NZMAX,D,U,NZ,ONE,TWO,FOUR)                        MAIN0271
         IF (REDBET .OR. REDCAP .OR. WRTBET .OR. WRTCAP) REWIND IUNIT   MAIN0272
 200  IF (IWT.EQ.1 .OR. IWT.EQ.4) GO TO 400                             MAIN0273
C-----------------------------------------------------------------------MAIN0274
C  INITIALIZE ACCUMULATION ARRAYS ...CAP                                MAIN0275
C-----------------------------------------------------------------------MAIN0276
         IF (SPLINE) CALL INICAP (                                      MAIN0277
     1       NYMAX,Y,T,SQRTW,NY,NZMAX,BETA,ZGRID,D,U,NGAMMX,CCAP,ECAP,  MAIN0278
     2       YCAP,SGGCAP,SGYCAP,YW,WORK,ZERO,DZ,RESPON)                 MAIN0279
         IF (WRTBET) REDBET=WRTBET                                      MAIN0280
         WRTBET=.FALSE.                                                 MAIN0281
         IF (WRTCAP) REDCAP=WRTCAP                                      MAIN0282
         WRTCAP=.FALSE.                                                 MAIN0283
         VARMIN=ZERO                                                    MAIN0284
         DO 220 N=1,ND                                                  MAIN0285
            MY=NY(N)                                                    MAIN0286
            DO 220 K=1,MY                                               MAIN0287
            YKN   =Y(K,N)                                               MAIN0288
            VARMIN=VARMIN+YKN*YKN                                       MAIN0289
 220     CONTINUE                                                       MAIN0290
         VARMIN=VARMIN*CONVRG*CONVRG                                    MAIN0291
C-----------------------------------------------------------------------MAIN0292
C  DO A COMPLTE PRELIMINARY UNWEIGHTED ANALYSIS TO GET A SMOOTH FIT     MAIN0293
C  TO THE DATA. THIS SMOOTH CURVE IS THEN USED TO CALCULATE THE         MAIN0294
C  WEIGHTS.                                                             MAIN0295
C-----------------------------------------------------------------------MAIN0296
         CALL ANALYZ (1,                                                MAIN0297
     1                NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,     MAIN0298
     2                PLNTRY,PLAM,PLMTRY,PLMSAV,PIVLIN,PIVLAM,EK,EPK,   MAIN0299
     3                ETE,R,DELTAL,DELTAN,GTG,H,DELTAH,EKEK,ASAVE,      MAIN0300
     4                NOKSET,NCOMBI,MTRYMX,STARTV,NLNDMX,ATOT,DTOLER,   MAIN0301
     5                BUFF,DX,BSPL0,BSPL1,BSPL2,IC,NZMAX,NGAMMX,CCAP,   MAIN0302
     6                ECAP,YCAP,SGGCAP,SGYCAP,YW,RESPON)                MAIN0303
         IF (IWT .EQ. 1) GO TO 420                                      MAIN0304
            IWTIST=.FALSE.                                              MAIN0305
C-----------------------------------------------------------------------MAIN0306
C  CALCULATE SQRTW (SQUARE ROOT OF LEAST SQUARES WEIGHTS)               MAIN0307
C-----------------------------------------------------------------------MAIN0308
            CALL SETWT (NYMAX,Y,T,YLYFIT,SQRTW,NY,ND,ERRFIT,NERFIT,IWT, MAIN0309
     1                  PRWT,SRANGE,NOUT,RESPON)                        MAIN0310
            IF (REDBET) REWIND IUNIT                                    MAIN0311
            SPLINE=.FALSE.                                              MAIN0312
            DO 300 J=1,10                                               MAIN0313
               IF (.NOT. DOSPL(J,2)) GO TO 300                          MAIN0314
               SPLINE=.TRUE.                                            MAIN0315
               GO TO 400                                                MAIN0316
 300        CONTINUE                                                    MAIN0317
 400     IF (IWT .EQ. 4) IWTIST=.FALSE.                                 MAIN0318
C-----------------------------------------------------------------------MAIN0319
C  INITIALIZE ACCUMULATION ARRAYS ...CAP  WITH WEIGHTS                  MAIN0320
C-----------------------------------------------------------------------MAIN0321
         IF (SPLINE) CALL INICAP (                                      MAIN0322
     1       NYMAX,Y,T,SQRTW,NY,NZMAX,BETA,ZGRID,D,U,NGAMMX,CCAP,ECAP,  MAIN0323
     2       YCAP,SGGCAP,SGYCAP,YW,WORK,ZERO,DZ,RESPON)                 MAIN0324
 420  VARMIN=ZERO                                                       MAIN0325
      DO 320 N=1,ND                                                     MAIN0326
         MY=NY(N)                                                       MAIN0327
         DO 320 K=1,MY                                                  MAIN0328
         YKN   =Y(K,N)                                                  MAIN0329
         SWKN  =SQRTW(K,N)                                              MAIN0330
         VARMIN=VARMIN+(YKN*SWKN)**2                                    MAIN0331
 320  CONTINUE                                                          MAIN0332
      VARMIN=VARMIN*CONVRG*CONVRG                                       MAIN0333
C-----------------------------------------------------------------------MAIN0334
C  DO FINAL WEIGHTED ANALYSIS                                           MAIN0335
C-----------------------------------------------------------------------MAIN0336
      CALL ANALYZ (2,                                                   MAIN0337
     1             NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,PLNTRY, MAIN0338
     2             PLAM,PLMTRY,PLMSAV,PIVLIN,PIVLAM,EK,EPK,ETE,R,DELTAL,MAIN0339
     3             DELTAN,GTG,H,DELTAH,EKEK,ASAVE,NOKSET,NCOMBI,MTRYMX, MAIN0340
     4             STARTV,NLNDMX,ATOT,DTOLER,BUFF,DX,BSPL0,BSPL1,BSPL2, MAIN0341
     5             IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,     MAIN0342
     6             RESPON)                                              MAIN0343
      IF (LAST) STOP                                                    MAIN0344
C-----------------------------------------------------------------------MAIN0345
C  RESTORE CHANGED CONTROL VARIABLES                                    MAIN0346
C-----------------------------------------------------------------------MAIN0347
      PLMNMX(1)=BOUNDL(1)                                               MAIN0348
      PLMNMX(2)=BOUNDL(2)                                               MAIN0349
      PNMNMX(1)=BOUNDN(1)                                               MAIN0350
      PNMNMX(2)=BOUNDN(2)                                               MAIN0351
      IWT      =IWTSAV                                                  MAIN0352
      GO TO 100                                                         MAIN0353
 1000 FORMAT(///4X,32HBOUNDS FOR    LINEAR PARAMETERS:,                 MAIN0354
     1          5X,1PE12.4,20H  .LE.  ALPHA   .LE.,1PE12.4,             MAIN0355
     2         /4X,32HBOUNDS FOR NONLINEAR PARAMETERS:,                 MAIN0356
     3          5X,1PE12.4,20H  .LE.  LAMBDA  .LE.,1PE12.4)             MAIN0357
 2000 FORMAT(///4X,32HBOUNDS FOR    LINEAR PARAMETERS:,                 MAIN0358
     1          5X,1PE12.4,26H  .LE.  ALPHA, GAMMA  .LE.,1PE12.4,       MAIN0359
     2         /4X,32HBOUNDS FOR NONLINEAR PARAMETERS:,                 MAIN0360
     3          5X,1PE12.4,26H  .LE.     LAMBDA     .LE.,1PE12.4)       MAIN0361
      END                                                               MAIN0362
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++BLCKD001
      BLOCK DATA                                                        BLCKD002
C-----------------------------------------------------------------------BLCKD003
C     INITIALIZES COMMON BLOCKS.                                        BLCKD004
C-----------------------------------------------------------------------BLCKD005
C     COMMON USED - DBLOCK, SBLOCK, IBLOCK, LBLOCK                      BLCKD006
C-----------------------------------------------------------------------BLCKD007
      DOUBLE PRECISION PRECIS, RANGE,                                   BLCKD008
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       BLCKD009
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           BLCKD010
     3                 TWOTHR, FORTHR, SIXTEL                           BLCKD011
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  BLCKD012
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    BLCKD013
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  BLCKD014
     3                 SPLINE, WRTBET, WRTCAP                           BLCKD015
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        BLCKD016
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, BLCKD017
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 BLCKD018
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),BLCKD019
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     BLCKD020
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       BLCKD021
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       BLCKD022
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   BLCKD023
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    BLCKD024
     4                 MTRY(10,2), MXITER(2), NL(10),                   BLCKD025
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  BLCKD026
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, BLCKD027
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       BLCKD028
     8                 NIN, NOUT                                        BLCKD029
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          BLCKD030
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         BLCKD031
     2                 DOUSOU(2), LUSER(30),                            BLCKD032
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  BLCKD033
     4                 SPLINE, WRTBET, WRTCAP                           BLCKD034
C***********************************************************************BLCKD035
C  YOU MUST SET THE FOLLOWING 4 VARIABLES TO VALUES APPROPRIATE FOR     BLCKD036
C  YOUR COMPUTER. (SEE USERS MANUAL)                                    BLCKD037
C                                                                       BLCKD038
C     DATA             RANGE/1.E35/, SRANGE/1.E35/, NIN/5/, NOUT/6/     BLCKD039
      DATA             RANGE/1.D35/, SRANGE/1.E35/, NIN/5/, NOUT/6/     BLCKD040
C***********************************************************************BLCKD041
C                                                                       BLCKD042
C                                                                       BLCKD043
C***********************************************************************BLCKD044
C  SPLMOD WILL USE THE VALUES OF THE CONTROL VARIABLES THAT ARE IN THE  BLCKD045
C  DATA STATEMENTS BELOW UNLESS YOU INPUT DIFFERENT VALUES.  TO SAVE    BLCKD046
C  INPUTTING THESE OFTEN, YOU CAN CHANGE THE VALUES BELOW TO THOSE      BLCKD047
C  THAT YOU WILL USUALLY USE.                                           BLCKD048
C                                                                       BLCKD049
C  THE FOLLOWING DATA STATEMENTS CONTAIN (IN ALPHABETICAL ORDER) THE    BLCKD050
C  REAL, INTEGER, AND LOGICAL CONTROL VARIABLES, IN THAT ORDER.         BLCKD051
C                                                                       BLCKD052
      DATA             CONVRG/5.E-5/, PLMNMX/-1.E5, 1.E5/,              BLCKD053
     1                 PNMNMX/.02, 2.08/, RUSER/100*0./                 BLCKD054
      DATA             IFORMT/1H(,1H5,1HE,1H1,1H5,1H.,1H6,1H),62*1H /,  BLCKD055
     1                 IFORMW/1H(,1H5,1HE,1H1,1H5,1H.,1H6,1H),62*1H /,  BLCKD056
     2                 IFORMY/1H(,1H5,1HE,1H1,1H5,1H.,1H6,1H),62*1H /,  BLCKD057
     3                 IPLFIT/2*0/, IPLRES/0, 1/, IPRINT/6*1/,          BLCKD058
     4                 IPRITR/0, 2/, IRWCAP/0/, IUNIT/0/, IUSER/50*0/,  BLCKD059
     5                 IWT/1/, LINEPG/60/, MCONV/3/, METHOD/2/,         BLCKD060
     6                 MIOERR/5/, MTRY/10, 9*20, 20, 9*50/,             BLCKD061
     7                 MXITER/20, 40/, NABORT/3/, ND/1/, NERFIT/10/,    BLCKD062
     8                 NGAM/0/, NL/1, 2, 3, 4, 5, 6, 7, 8, 9, 10/,      BLCKD063
     9                 NNL/1/, NZ/50/                                   BLCKD064
      DATA             DOADEX/20*.FALSE./, DOSPL/20*.TRUE./,            BLCKD065
     1                 DOSTRT/20*.FALSE./, DOUSIN/.FALSE./,             BLCKD066
     2                 DOUSOU/2*.FALSE./, LAST/.TRUE./,                 BLCKD067
     3                 LUSER/30*.FALSE./, PRWT/.TRUE./, PRY/.TRUE./,    BLCKD068
     4                 SAMET/.FALSE./, SIMULA/.FALSE./                  BLCKD069
C***********************************************************************BLCKD070
C     DATA             ZERO/0.E0/,  ONE/1.E0/,  TWO/2.E0/,              BLCKD071
C    1                 THREE/3.E0/, FOUR/4.E0/, SIX/6.E0/, TEN/1.E1/    BLCKD072
      DATA             ZERO/0.D0/,  ONE/1.D0/,  TWO/2.D0/,              BLCKD073
     1                 THREE/3.D0/, FOUR/4.D0/, SIX/6.D0/, TEN/1.D1/    BLCKD074
      DATA             IALPHA/1H , 1HA, 1HL, 1HP, 1HH, 1HA/,            BLCKD075
     1                 IGAMMA/1H , 1HG, 1HA, 1HM, 1HM, 1HA/,            BLCKD076
     2                 ILAMDA/1HL, 1HA, 1HM, 1HB, 1HD, 1HA/,            BLCKD077
     3                 ISTTXT/1H , 1HI, 1HT, 1HR, 1H , 1H , 1H , 1HV,   BLCKD078
     4                        1HA, 1HR, 1HI, 1HA, 1HN, 1HC, 1HE, 1H ,   BLCKD079
     5                        1H , 1H , 1HD, 1HA, 1HM, 1HP, 1HI, 1HN,   BLCKD080
     6                        1HG, 1H , 1HQ, 1H /,                      BLCKD081
     7                 INDEX/10*0/, IB/4/                               BLCKD082
      DATA             REDBET/.FALSE./, REDCAP/.FALSE./,                BLCKD083
     1                 WRTBET/.FALSE./, WRTCAP/.FALSE./                 BLCKD084
      END                                                               BLCKD085
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++USREX001
      REAL FUNCTION USEREX (NYMAX,T,NY,K,N,RESPON)                      USREX002
C-----------------------------------------------------------------------USREX003
C     USER SUPPLIED SUBROUTINE. (ONLY CALLED IF SIMULA=.TRUE.)          USREX004
C     TO EVALUATE THE NOISE-FREE VALUE OF THE SIMULATED DATA AT POINT   USREX005
C     T(K,N).                                                           USREX006
C     IN CASE OF CONVOLUTION, RESPON CONTAINS THE RESPONSE FUNCTION.    USREX007
C     BELOW IS ILLUSTRATED THE CASE OF THE DATA BEING COMPOSED          USREX008
C     OF A SUM OF IUSER(10) (.LE. 10) EXPONENTIALS PLUS A CONSTANT      USREX009
C     BACKGROUND, WHERE                                                 USREX010
C        DECAY CONSTANTS ARE STORED IN RUSER( 9+J), J=1,...,IUSER(10)   USREX011
C                    AND AMPLITUDES IN RUSER(19+J), J=1,...,IUSER(10)   USREX012
C                    AND BACKGROUND IN RUSER(30)                        USREX013
C     IT ALSO SHOWS HOW TO PREVENT UNDERFLOWS IN CALCULATING EXP(-X)    USREX014
C     WHEN X IS LARGE.                                                  USREX015
C-----------------------------------------------------------------------USREX016
C     SUBPROGRAMS CALLED - NONE                                         USREX017
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               USREX018
C-----------------------------------------------------------------------USREX019
      DOUBLE PRECISION PRECIS, RANGE,                                   USREX020
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       USREX021
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           USREX022
     3                 TWOTHR, FORTHR, SIXTEL                           USREX023
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  USREX024
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    USREX025
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USREX026
     3                 SPLINE, WRTBET, WRTCAP                           USREX027
      DIMENSION        T(NYMAX,1), NY(*), RESPON(*), IHOLER(6)          USREX028
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        USREX029
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, USREX030
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 USREX031
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),USREX032
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     USREX033
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       USREX034
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       USREX035
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   USREX036
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    USREX037
     4                 MTRY(10,2), MXITER(2), NL(10),                   USREX038
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  USREX039
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, USREX040
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       USREX041
     8                 NIN, NOUT                                        USREX042
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          USREX043
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         USREX044
     2                 DOUSOU(2), LUSER(30),                            USREX045
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USREX046
     4                 SPLINE, WRTBET, WRTCAP                           USREX047
      DATA             IHOLER/1HU, 1HS, 1HE, 1HR, 1HE, 1HX/             USREX048
C-----------------------------------------------------------------------USREX049
C  THE FOLLOWING STATEMENTS SHOULD BE REPLACED WITH THE ONES            USREX050
C  APPROPRIATE FOR YOUR SIMULATION.                                     USREX051
C-----------------------------------------------------------------------USREX052
      USEREX=RUSER(30)                                                  USREX053
      NLAM  =IUSER(10)                                                  USREX054
      DO 100 J=1,NLAM                                                   USREX055
         EZ   =RUSER(9+J)*T(K,N)                                        USREX056
         EXPEZ=0.                                                       USREX057
         IF (EZ .LT. EXMAX) EXPEZ=EXP(-EZ)                              USREX058
         USEREX=USEREX+RUSER(19+J)*EXPEZ                                USREX059
 100  CONTINUE                                                          USREX060
      RETURN                                                            USREX061
      END                                                               USREX062
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++USRFL001
      REAL FUNCTION USERFL (MU,NYMAX,T,NY,K,N,RESPON)                   USRFL002
C-----------------------------------------------------------------------USRFL003
C     USER SUPPLIED SUBROUTINE.                                         USRFL004
C     ONLY CALLED WHEN   NGAM>0.   EVALUATES  MU'TH  LINEAR FUNCTION AT USRFL005
C     T(K,N),  I.E. THE LINEAR TERMS IN THE RIGHT SUM OF EQ (3.1-1)     USRFL006
C     OR EQ (4.1.2-1) IN THE USERS MANUAL.                              USRFL007
C                                                                       USRFL008
C     IN CASE OF CONVOLUTION,   RESPON   CONTAINS THE RESPONSE FUNCTION.USRFL009
C                                                                       USRFL010
C     BELOW IS ILLUSTRATED THE CASE WHEN THE FIRST LINEAR FUNCTION IS A USRFL011
C     SIMPLE ADDITIVE CONSTANT, OFTEN CALLED "BASELINE" OR "BACKGROUND".USRFL012
C     THE OTHER TERMS WITH   MU > 1   ARE USED, WHEN ONE IS WORKING WITHUSRFL013
C     A CONVOLUTION AND/OR TO CORRECT FOR A SHIFT OF THE RESPONSE       USRFL014
C     FUNCTION  (SEE ALSO REF [6] IN THE USERS MANUAL).                 USRFL015
C                                                                       USRFL016
C     IN DETAIL THERE ARE THE FOLLOWING POSSIBILITIES:                  USRFL017
C                                                                       USRFL018
C     NGAM = 1  IF IUSER(1)=1 USERFN(1,...) ACCOUNTS FOR CONSTANT       USRFL019
C                                                           BACKGROUND  USRFL020
C               IF IUSER(1)=3 USERFN(1,...) ACCOUNTS FOR CONSTANT       USRFL021
C                                                           BACKGROUND  USRFL022
C     NGAM = 2  IF IUSER(1)=2 USERFN(1,...) ACCOUNTS FOR CONSTANT       USRFL023
C                                                         USERFN(2,...) USRFL024
C                                                                       USRFL025
C     IUSER(1)   AND   LUSER(1)   DEFINE MODEL SWITCHES IN SUBPROGRAM   USRFL026
C     USERFN,  SEE THERE FOR DETAILS.                                   USRFL027
C                                                                       USRFL028
C     THIS VERSION OF   USERFL   AND THE PROPER SETTINGS OF   IUSER(1), USRFL029
C     RUSER(1),   AND   LUSER(1)   ARE DESCRIBED IN DETAIL IN SEC 6.1   USRFL030
C     OF THE USERS MANUAL.                                              USRFL031
C-----------------------------------------------------------------------USRFL032
C     SUBPROGRAMS CALLED - ERRMES                                       USRFL033
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               USRFL034
C-----------------------------------------------------------------------USRFL035
      DOUBLE PRECISION PRECIS, RANGE,                                   USRFL036
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       USRFL037
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           USRFL038
     3                 TWOTHR, FORTHR, SIXTEL                           USRFL039
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  USRFL040
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    USRFL041
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRFL042
     3                 SPLINE, WRTBET, WRTCAP                           USRFL043
      DIMENSION        T(NYMAX,1), NY(*), RESPON(*), IHOLER(6)          USRFL044
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        USRFL045
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, USRFL046
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 USRFL047
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),USRFL048
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     USRFL049
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       USRFL050
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       USRFL051
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   USRFL052
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    USRFL053
     4                 MTRY(10,2), MXITER(2), NL(10),                   USRFL054
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  USRFL055
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, USRFL056
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       USRFL057
     8                 NIN, NOUT                                        USRFL058
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          USRFL059
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         USRFL060
     2                 DOUSOU(2), LUSER(30),                            USRFL061
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRFL062
     4                 SPLINE, WRTBET, WRTCAP                           USRFL063
      DATA             IHOLER/1HU, 1HS, 1HE, 1HR, 1HF, 1HL/             USRFL064
C-----------------------------------------------------------------------USRFL065
      IF (N .GT. ND)              CALL ERRMES (1,.TRUE.,IHOLER,NOUT)    USRFL066
      IF (K.LE.0 .OR. K.GT.NY(N)) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)    USRFL067
      IF (MU.LE.0 .OR. MU.GT.6)   CALL ERRMES (3,.TRUE.,IHOLER,NOUT)    USRFL068
C-----------------------------------------------------------------------USRFL069
      GO TO (100,200,300,400,500,600), MU                               USRFL070
C-----------------------------------------------------------------------USRFL071
C  THE FOLLOWING CONSTANT TERM ACCOUNTS FOR BACKGROUND                  USRFL072
C-----------------------------------------------------------------------USRFL073
 100  USERFL=1.                                                         USRFL074
      RETURN                                                            USRFL075
C-----------------------------------------------------------------------USRFL076
C  1ST CASE (NGAM=2 OR 3)                                               USRFL077
C-----------------------------------------------------------------------USRFL078
 200  IF (LUSER(1) .AND. IUSER(1).NE.2) GOTO 210                        USRFL079
      USERFL=0.                                                         USRFL080
      IF (NGAM.NE.2 .AND. NGAM.NE.3) CALL ERRMES (4,.TRUE.,IHOLER,NOUT) USRFL081
      USERFL=RESPON(K)                                                  USRFL082
      RETURN                                                            USRFL083
C-----------------------------------------------------------------------USRFL084
C  2ND CASE (NGAM=2 AND SHIFT CORRECTION)                               USRFL085
C-----------------------------------------------------------------------USRFL086
 210  USERFL=0.                                                         USRFL087
      IF (NGAM .NE. 2) CALL ERRMES (5,.TRUE.,IHOLER,NOUT)               USRFL088
      IF (K .EQ. NY(1)) GOTO 230                                        USRFL089
         IF (K .EQ. 1) GOTO 220                                         USRFL090
            KP1   =K+1                                                  USRFL091
            KM1   =K-1                                                  USRFL092
            USERFL=(RESPON(KP1)-RESPON(KM1))/(T(KP1,1)-T(KM1,1))        USRFL093
            GOTO 240                                                    USRFL094
 220     USERFL=(RESPON(2)-RESPON(1))/(T(2,1)-T(1,1))                   USRFL095
         GOTO 240                                                       USRFL096
 230  NY1   =NY(1)                                                      USRFL097
      NY1M1 =NY1-1                                                      USRFL098
      USERFL=(RESPON(NY1)-RESPON(NY1M1))/(T(NY1,1)-T(NY1M1,1))          USRFL099
 240  USERFL=RUSER(1)*RESPON(K)+USERFL                                  USRFL100
      RETURN                                                            USRFL101
C-----------------------------------------------------------------------USRFL102
C  NGAM=3                                                               USRFL103
C-----------------------------------------------------------------------USRFL104
 300  USERFL=0.                                                         USRFL105
      IF (NGAM.NE.3 .OR. LUSER(1)) CALL ERRMES (6,.TRUE.,IHOLER,NOUT)   USRFL106
      IF (K .EQ. NY(1)) GOTO 320                                        USRFL107
         IF (K .EQ. 1) GOTO 310                                         USRFL108
            KP1   =K+1                                                  USRFL109
            KM1   =K-1                                                  USRFL110
            USERFL=(RESPON(KP1)-RESPON(KM1))/(T(KP1,1)-T(KM1,1))        USRFL111
            RETURN                                                      USRFL112
 310     USERFL=(RESPON(2)-RESPON(1))/(T(2,1)-T(1,1))                   USRFL113
         RETURN                                                         USRFL114
 320  NY1   =NY(1)                                                      USRFL115
      NY1M1 =NY1-1                                                      USRFL116
      USERFL=(RESPON(NY1)-RESPON(NY1M1))/(T(NY1,1)-T(NY1M1,1))          USRFL117
      RETURN                                                            USRFL118
C-----------------------------------------------------------------------USRFL119
C  NGAM=4                                                               USRFL120
C-----------------------------------------------------------------------USRFL121
 400  USERFL=0.                                                         USRFL122
      RETURN                                                            USRFL123
C-----------------------------------------------------------------------USRFL124
C  NGAM=5                                                               USRFL125
C-----------------------------------------------------------------------USRFL126
 500  USERFL=0.                                                         USRFL127
      RETURN                                                            USRFL128
C-----------------------------------------------------------------------USRFL129
C  NGAM=6                                                               USRFL130
C-----------------------------------------------------------------------USRFL131
 600  USERFL=0.                                                         USRFL132
      RETURN                                                            USRFL133
      END                                                               USRFL134
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++USRFN001
      REAL FUNCTION USERFN (Z,J,NYMAX,T,NY,K,N,IDERIV,RESPON)           USRFN002
C *** MODIFIED TO USE EXPONENTIAL OF 4 TERM POWER SERIES WITH
C *** NO RECURSION AND NO OPTIONS  XS/GAM/LMN 18viii05
C *** CALCULATION OF DERIVATIVES CORRECTED GAM 3iii09
C-----------------------------------------------------------------------USRFN003
C     USER SUPPLIED SUBROUTINE.                                         USRFN004
C     EVALUATES THEORETICAL NONLINEAR MODEL FUNCTION OF EQ (3.1-1) OR   USRFN005
C     EQ (4.1.2-1) IN THE USERS MANUAL FOR A GIVEN PARAMETER            USRFN006
C     Z=ALOG(PLAM(J))   AND GIVEN T-VALUE   T(K,N)   (IDERIV=0), OR     USRFN007
C     ITS 1ST (IDERIV=1), OR 2ND (IDERIV=2) DERIVATIVE W.R.T. Z.        USRFN008
C                                                                       USRFN009
C     USES RECURSION FORMULAS IF POSSIBLE.                              USRFN010
C                                                                       USRFN011
C     IN CASE OF CONVOLUTION,   RESPON   CONTAINS RESPONSE FUNCTION.    USRFN012
C     WHEN USING RECURSION FORMULAS YOU WILL NEED AN ADDITIONAL COMMON  USRFN013
C     BLOCK TO STORE THE FUNCTION VALUE, ITS 1ST, AND 2ND DERIVATIVE,   USRFN014
C     AND OTHER QUANTITIES FROM PREVIOUS CALCULATIONS.                  USRFN015
C                                                                       USRFN016
C     BELOW ARE ILLUSTRATED 2 CASES OF RECURSION FORMULAS:              USRFN017
C          IF IUSER(1) = 1  USE RECURSIONS FOR PURE EXPONENTIALS        USRFN018
C          IF IUSER(1) = 2  USE RECURSIONS FOR CONVOLUTION              USRFN019
C          IF IUSER(1) = 3  USE RECURSIONS FOR SPECIAL CONVOLUTION AS   USRFN020
C                           DESCRIBED IN REF. [6] OF USERS MANUAL       USRFN021
C                                                                       USRFN022
C     THIS VERSION OF   USERFN   AND THE PROPER SETTINGS OF   IUSER(1), USRFN023
C     RUSER(1),   AND   LUSER(1)   ARE DESCRIBED IN DETAIL IN SEC 6.1   USRFN024
C     OF THE USERS MANUAL.                                              USRFN025
C-----------------------------------------------------------------------USRFN026
C     SUBPROGRAMS CALLED - ERRMES, USERTR                               USRFN027
C     WHICH IN TURN CALL - ERRMES                                       USRFN028
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK, RECURS       USRFN029
C-----------------------------------------------------------------------USRFN030
      DOUBLE PRECISION PRECIS, RANGE,                                   USRFN031
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       USRFN032
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           USRFN033
     3                 TWOTHR, FORTHR, SIXTEL                           USRFN034
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  USRFN035
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    USRFN036
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRFN037
     3                 SPLINE, WRTBET, WRTCAP                           USRFN038
      DIMENSION        T(NYMAX,1), NY(*), RESPON(*), IHOLER(6)          USRFN039
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        USRFN040
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, USRFN041
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 USRFN042
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),USRFN043
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     USRFN044
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       USRFN045
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       USRFN046
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   USRFN047
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    USRFN048
     4                 MTRY(10,2), MXITER(2), NL(10),                   USRFN049
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  USRFN050
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, USRFN051
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       USRFN052
     8                 NIN, NOUT                                        USRFN053
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          USRFN054
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         USRFN055
     2                 DOUSOU(2), LUSER(30),                            USRFN056
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRFN057
     4                 SPLINE, WRTBET, WRTCAP                           USRFN058
C***********************************************************************USRFN059
C  IF YOU CHANGE  NZMAX  OR  NONLMX  IN THE MAIN PROGRAM, THEN YOU MUST USRFN060
C  READJUST THE DIMENSIONS OF THE FOLLOWING ARRAYS AS DESCRIBED BELOW - USRFN061
C                                                                       USRFN062
      COMMON /RECURS/  DERIV0(52),  FOLD0(52),  PLAMDT(52),             USRFN063
     1                 DERIV1(5), DERIV2(5),                            USRFN064
     2                 FOLD1(5),  FOLD2(5),                             USRFN065
     3                 EZ, DT, DT2, DTOLD0, DTOLD1, DTOLD2, SAVD0, SAVD1USRFN066
C                                                                       USRFN067
C  THE DIMENSIONS OF THE ARRAYS ABOVE MUST BE ADJUSTED AS FOLLOWS -     USRFN068
C                                                                       USRFN069
C     COMMON /RECURS/  DERIV0(NZMAX),  FOLD0(NZMAX),  PLAMDT(NZMAX),    USRFN070
C    1                 DERIV1(NONLMX), DERIV2(NONLMX),                  USRFN071
C    2                 FOLD1(NONLMX),  FOLD2(NONLMX),                   USRFN072
C    3                 EZ, DT, DT2, DTOLD0, DTOLD1, DTOLD2, SAVD0, SAVD1USRFN073
C                                                                       USRFN074
C***********************************************************************USRFN075
      DATA             IHOLER/1HU, 1HS, 1HE, 1HR, 1HF, 1HN/             USRFN076
C-----------------------------------------------------------------------USRFN077
      IF (J .GT. NB)                 CALL ERRMES (1,.TRUE.,IHOLER,NOUT) USRFN078
      IF (N .GT. ND)                 CALL ERRMES (2,.TRUE.,IHOLER,NOUT) USRFN079
      IF (K.LE.0 .OR. K.GT.NY(N))    CALL ERRMES (3,.TRUE.,IHOLER,NOUT) USRFN080
      IF (IDERIV.LT.0 .OR. IDERIV.GT.2)                                 USRFN081
     1                               CALL ERRMES (4,.TRUE.,IHOLER,NOUT) USRFN082
C-----------------------------------------------------------------------USRFN083
C *** IGNORE ORIGINAL CODE, SUBSTITUTE SIMPLE EVALUATION
C *** GAM
C *** Try reducing number of multiplications
      PLAM=USERTR(Z,2)
      TKN=T(K,N)
      PLTK1=PLAM*TKN
      PLTK2=PLTK1*PLTK1
      PLTK3=PLTK2*PLTK1
      PLTK4=PLTK3*PLTK1
      C1=RUSER(1)
      C2=RUSER(2)
      C3=RUSER(3)
      C4=RUSER(4)
      FUNC=EXP(-(C1*PLTK1+C2*PLTK2+C3*PLTK3+C4*PLTK4))
      DFUNC=-PLAM*FUNC*(C1*TKN+2*C2*PLTK1*TKN+3*C3*PLTK2*TKN
     1 +4*C4*PLTK3*TKN)
      IF (IDERIV-1) 101,102,103
 101  USERFN=FUNC
      RETURN
 102  USERFN=DFUNC
      RETURN
 103  USERFN=-DFUNC*(C1*TKN+2*C2*PLTK1*TKN+3*C3*PLTK2*TKN
     1 +4*C4*PLTK3*TKN)
      USERFN=USERFN-FUNC*(2*C2*TKN*TKN+6*C3*PLTK1*TKN*TKN
     1 +12*C4*PLTK2*TKN*TKN)
      USERFN=USERFN*PLAM
      RETURN

C *** IGNORE ORIGINAL CODE, SUBSTITUTE SIMPLE EVALUATION

      IF (IUSER(1) .EQ. 3) GO TO 600                                    USRFN084
      IF (IUSER(1) .EQ. 2) GO TO 400                                    USRFN085
      IF (IUSER(1) .EQ. 1) GO TO 100                                    USRFN086
                                     CALL ERRMES (5,.TRUE.,IHOLER,NOUT) USRFN087
C-----------------------------------------------------------------------USRFN088
C  USE RECURSION FORMULAS FOR EXPONENTIALS ONLY                         USRFN089
C-----------------------------------------------------------------------USRFN090
 100  IF (IDERIV-1) 110,140,170                                         USRFN091
C-----------------------------------------------------------------------USRFN092
C  FUNCTION VALUE                                                       USRFN093
C-----------------------------------------------------------------------USRFN094
 110  IF (K .GT. 1) GO TO 120                                           USRFN095
         PLAM    =USERTR(Z,2)                                           USRFN096
         DTOLD0  =T(2,N)-T(1,N)                                         USRFN097
         EZ      =PLAM*DTOLD0                                           USRFN098
         FOLD0(J)=0.                                                    USRFN099
         IF (EZ .LT. EXMAX) FOLD0(J)=EXP(-EZ)                           USRFN100
         PLAMDT(J)=EZ                                                   USRFN101
         EZ       =PLAM*T(1,N)                                          USRFN102
         USERFN   =0.                                                   USRFN103
         IF (EZ .LT. EXMAX) USERFN=EXP(-EZ)                             USRFN104
         DERIV0(J)=USERFN                                               USRFN105
         RETURN                                                         USRFN106
 120  DT=T(K,N)-T(K-1,N)                                                USRFN107
      IF (DT .EQ. DTOLD0) GO TO 130                                     USRFN108
         PLAM    =USERTR(Z,2)                                           USRFN109
         EZ      =PLAM*DT                                               USRFN110
         FOLD0(J)=0.                                                    USRFN111
         IF (EZ .LT. EXMAX) FOLD0(J)=EXP(-EZ)                           USRFN112
         PLAMDT(J)=EZ                                                   USRFN113
         IF (J .EQ. NONL) DTOLD0=DT                                     USRFN114
 130  USERFN   =DERIV0(J)*FOLD0(J)                                      USRFN115
      SAVD0    =DERIV0(J)                                               USRFN116
      DERIV0(J)=USERFN                                                  USRFN117
      RETURN                                                            USRFN118
C-----------------------------------------------------------------------USRFN119
C  1ST DERIVATIVE                                                       USRFN120
C-----------------------------------------------------------------------USRFN121
 140  IF (K .GT. 1) GO TO 150                                           USRFN122
         DTOLD1   =DTOLD0                                               USRFN123
         USERFN   =-EZ*DERIV0(J)                                        USRFN124
         DERIV1(J)=USERFN                                               USRFN125
         FOLD1(J) =-PLAMDT(J)*FOLD0(J)                                  USRFN126
         RETURN                                                         USRFN127
 150  IF (DT .EQ. DTOLD1) GO TO 160                                     USRFN128
         FOLD1(J)=-PLAMDT(J)*FOLD0(J)                                   USRFN129
         IF (J .EQ. NONL) DTOLD1=DT                                     USRFN130
 160  USERFN   =DERIV1(J)*FOLD0(J)+SAVD0*FOLD1(J)                       USRFN131
      SAVD1    =DERIV1(J)                                               USRFN132
      DERIV1(J)=USERFN                                                  USRFN133
      RETURN                                                            USRFN134
C-----------------------------------------------------------------------USRFN135
C  2ND DERIVATIVE                                                       USRFN136
C-----------------------------------------------------------------------USRFN137
 170  IF (K .GT. 1) GO TO 180                                           USRFN138
         DTOLD2   =DTOLD1                                               USRFN139
         USERFN   =DERIV1(J)*(1.0-EZ)                                   USRFN140
         DERIV2(J)=USERFN                                               USRFN141
         FOLD2(J) =FOLD1(J)*(1.0-PLAMDT(J))                             USRFN142
         RETURN                                                         USRFN143
 180  IF (DT .EQ. DTOLD2) GO TO 190                                     USRFN144
         FOLD2(J)=FOLD1(J)*(1.0-PLAMDT(J))                              USRFN145
         IF (J .EQ. NONL) DTOLD2=DT                                     USRFN146
 190  USERFN   =DERIV2(J)*FOLD0(J)+2.0*SAVD1*FOLD1(J)+SAVD0*FOLD2(J)    USRFN147
      DERIV2(J)=USERFN                                                  USRFN148
      RETURN                                                            USRFN149
C-----------------------------------------------------------------------USRFN150
C  USE RECURSION FORMULAS FOR CONVOLUTIONS                              USRFN151
C-----------------------------------------------------------------------USRFN152
 400  IF (IDERIV-1) 410,450,500                                         USRFN153
C-----------------------------------------------------------------------USRFN154
C  FUNCTION VALUE                                                       USRFN155
C-----------------------------------------------------------------------USRFN156
 410  IF (K .GT. 2) GO TO 430                                           USRFN157
         IF (K .GT. 1) GO TO 420                                        USRFN158
            PLAM    =USERTR(Z,2)                                        USRFN159
            DTOLD0  =T(2,N)-T(1,N)                                      USRFN160
            EZ      =PLAM*DTOLD0                                        USRFN161
            FOLD0(J)=0.                                                 USRFN162
            IF (EZ .LT. EXMAX) FOLD0(J)=EXP(-EZ)                        USRFN163
            PLAMDT(J)=EZ                                                USRFN164
            USERFN   =RESPON(1)*DTOLD0/2.0                              USRFN165
            SAVD0    =USERFN                                            USRFN166
            DERIV0(J)=USERFN                                            USRFN167
            RETURN                                                      USRFN168
 420     USERFN   =DERIV0(J)*FOLD0(J)+RESPON(2)*DTOLD0/2.0              USRFN169
         DERIV0(J)=USERFN                                               USRFN170
         RETURN                                                         USRFN171
 430  DT =T(K,N)-T(K-1,N)                                               USRFN172
      DT2=DT/2.0                                                        USRFN173
      IF (DT .EQ. DTOLD0) GO TO 440                                     USRFN174
         PLAM    =USERTR(Z,2)                                           USRFN175
         EZ      =PLAM*DT                                               USRFN176
         FOLD0(J)=0.                                                    USRFN177
         IF (EZ .LT. EXMAX) FOLD0(J)=EXP(-EZ)                           USRFN178
         PLAMDT(J)=EZ                                                   USRFN179
         IF (J .EQ. NONL) DTOLD0=DT                                     USRFN180
 440  USERFN   =(DERIV0(J)+RESPON(K-1)*DT2)*FOLD0(J)+RESPON(K)*DT2      USRFN181
      SAVD0    =DERIV0(J)                                               USRFN182
      DERIV0(J)=USERFN                                                  USRFN183
      RETURN                                                            USRFN184
C-----------------------------------------------------------------------USRFN185
C  1ST DERIVATIVE                                                       USRFN186
C-----------------------------------------------------------------------USRFN187
 450  IF (K .GT. 2) GO TO 470                                           USRFN188
         IF (K .GT. 1) GO TO 460                                        USRFN189
            DTOLD1   =DTOLD0                                            USRFN190
            USERFN   =0.0                                               USRFN191
            DERIV1(J)=USERFN                                            USRFN192
            FOLD1(J) =-PLAMDT(J)*FOLD0(J)                               USRFN193
            RETURN                                                      USRFN194
 460     USERFN   =SAVD0*FOLD1(J)                                       USRFN195
         DERIV1(J)=USERFN                                               USRFN196
         RETURN                                                         USRFN197
 470  IF (DT .EQ. DTOLD1) GO TO 480                                     USRFN198
         FOLD1(J)=-PLAMDT(J)*FOLD0(J)                                   USRFN199
         IF (J .EQ. NONL) DTOLD1=DT                                     USRFN200
 480  USERFN   =DERIV1(J)*FOLD0(J)+(SAVD0+RESPON(K-1)*DT2)*FOLD1(J)     USRFN201
      SAVD1    =DERIV1(J)                                               USRFN202
      DERIV1(J)=USERFN                                                  USRFN203
      RETURN                                                            USRFN204
C-----------------------------------------------------------------------USRFN205
C  2ND DERIVATIVE                                                       USRFN206
C-----------------------------------------------------------------------USRFN207
 500  IF (K .GT. 2) GO TO 520                                           USRFN208
         IF (K .GT. 1) GO TO 510                                        USRFN209
            DTOLD2   =DTOLD1                                            USRFN210
            USERFN   =0.0                                               USRFN211
            DERIV2(J)=USERFN                                            USRFN212
            FOLD2(J) =FOLD1(J)*(1.0-PLAMDT(J))                          USRFN213
            RETURN                                                      USRFN214
 510     USERFN   =SAVD0*FOLD2(J)                                       USRFN215
         DERIV2(J)=USERFN                                               USRFN216
         RETURN                                                         USRFN217
 520  IF (DT .EQ. DTOLD2) GO TO 530                                     USRFN218
         FOLD2(J)=FOLD1(J)*(1.0-PLAMDT(J))                              USRFN219
         IF (J .EQ. NONL) DTOLD2=DT                                     USRFN220
 530  USERFN   =DERIV2(J)*FOLD0(J)+2.0*SAVD1*FOLD1(J)                   USRFN221
      USERFN   =USERFN+(SAVD0+RESPON(K-1)*DT2)*FOLD2(J)                 USRFN222
      DERIV2(J)=USERFN                                                  USRFN223
      RETURN                                                            USRFN224
C-----------------------------------------------------------------------USRFN225
C  USE RECURSION FORMULAS FOR SPECIAL CONVOLUTION (REFERENCE LIFETIME   USRFN226
C  KNOWN AND STORED IN RUSER(1), SEE ALSO REF [6] OF USERS MANUAL)      USRFN227
C-----------------------------------------------------------------------USRFN228
 600  IF (IDERIV-1) 620,700,800                                         USRFN229
C-----------------------------------------------------------------------USRFN230
C  FUNCTION VALUE                                                       USRFN231
C-----------------------------------------------------------------------USRFN232
 620  IF (K .GT. 2) GO TO 660                                           USRFN233
         IF (K .GT. 1) GO TO 640                                        USRFN234
            PLAM     =USERTR(Z,2)                                       USRFN235
            PLAMDT(J)=RUSER(1)-PLAM                                     USRFN236
            DT       =T(2,N)-T(1,N)                                     USRFN237
            DT2      =DT/2.                                             USRFN238
            EZ       =PLAM*DT                                           USRFN239
            FOLD0(J) =0.                                                USRFN240
            IF (EZ .LT. EXMAX) FOLD0(J)=EXP(-EZ)                        USRFN241
            USERFN   =PLAMDT(J)*RESPON(1)*DT2                           USRFN242
            DERIV0(J)=USERFN                                            USRFN243
            USERFN   =USERFN+RESPON(1)                                  USRFN244
            RETURN                                                      USRFN245
 640     USERFN   =DERIV0(J)*FOLD0(J)+PLAMDT(J)*RESPON(2)*DT2           USRFN246
         DERIV0(J)=USERFN                                               USRFN247
         USERFN   =USERFN+RESPON(2)                                     USRFN248
         RETURN                                                         USRFN249
 660  USERFN   =(DERIV0(J)+PLAMDT(J)*RESPON(K-1)*DT2)*FOLD0(J)+         USRFN250
     1                     PLAMDT(J)*RESPON(K)  *DT2                    USRFN251
      SAVD0    =DERIV0(J)                                               USRFN252
      DERIV0(J)=USERFN                                                  USRFN253
      USERFN   =USERFN+RESPON(K)                                        USRFN254
      RETURN                                                            USRFN255
C-----------------------------------------------------------------------USRFN256
C  1ST DERIVATIVE                                                       USRFN257
C-----------------------------------------------------------------------USRFN258
 700  IF (K .GT. 2) GO TO 740                                           USRFN259
         IF (K .GT. 1) GO TO 720                                        USRFN260
            USERFN   =-RESPON(1)*DT2                                    USRFN261
            DERIV1(J)=USERFN                                            USRFN262
            FOLD1(J) =-DT*FOLD0(J)                                      USRFN263
            GO TO 760                                                   USRFN264
 720     USERFN   =((PLAMDT(J)*FOLD1(J)-FOLD0(J))*RESPON(1)-            USRFN265
     1                                            RESPON(2))*DT2        USRFN266
         DERIV1(J)=USERFN                                               USRFN267
         GO TO 760                                                      USRFN268
 740  USERFN   =DERIV1(J)*FOLD0(J)+                                     USRFN269
     1                   (SAVD0+PLAMDT(J)*RESPON(K-1)*DT2)*FOLD1(J)     USRFN270
      USERFN   =USERFN-(RESPON(K-1)*FOLD0(J)+RESPON(K))*DT2             USRFN271
      DERIV1(J)=USERFN                                                  USRFN272
 760  USERFN   =(RUSER(1)-PLAMDT(J))*USERFN                             USRFN273
      RETURN                                                            USRFN274
C-----------------------------------------------------------------------USRFN275
C  2ND DERIVATIVE                                                       USRFN276
C-----------------------------------------------------------------------USRFN277
 800  CALL ERRMES (6,.TRUE.,IHOLER,NOUT)                                USRFN278
      RETURN                                                            USRFN279
      END                                                               USRFN280
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++USRIN001
      SUBROUTINE USERIN (NYMAX,Y,T,SQRTW,NY,RESPON)                     USRIN002
C-----------------------------------------------------------------------USRIN003
C     THIS IS A USER-SUPPLIED ROUTINE (ONLY CALLED WHEN DOUSIN=.TRUE.)  USRIN004
C     THAT IS CALLED RIGHT AFTER THE INITIALIZATION AND INPUT OF THE    USRIN005
C     COMMON VARIABLES, T, Y, AND THE LEAST-SQUARES WEIGHTS.            USRIN006
C     THEREFORE, IT CAN BE USED TO MODIFY ANY OF THESE.                 USRIN007
C     SEE THE USERS MANUAL (SEC 4.2) FOR THE POSITION IN THE INPUT      USRIN008
C     DATA DECK OF ANY INPUT FOR THIS USER SUBPROGRAM.                  USRIN009
C                                                                       USRIN010
C     BELOW IS ILLUSTRATED THE CASE OF A CONVOLUTION, WHERE             USRIN011
C     IUSER(1) > 1 HAS TO BE SET IN THE INPUT DECK, AND ND=1.           USRIN012
C     USERIN READS THE RESPONSE/REFERENCE FUNCTION (FOR DETAILS         USRIN013
C     SEE SEC 6.1 AND REF. [6] OF THE USERS MANUAL), STORES IT IN       USRIN014
C     ARRAY RESPON, WRITES IT OUT (IF PRY=.TRUE.), AND MODIFIES         USRIN015
C     THE BOUNDS FOR THE LINEAR PARAMETERS.                             USRIN016
C     USERIN HAS ALSO THE OPTION TO SUBTRACT BACKGROUND FROM THE        USRIN017
C     RESPONSE/REFERENCE FUNCTION.                                      USRIN018
C                                                                       USRIN019
C        RESPON    - REAL ARRAY OF DIMENSION AT LEAST NY.               USRIN020
C        LUSER(10) - LOGICAL, IF .TRUE. SUBTRACT BACKGROUND (STORED     USRIN021
C                    IN RUSER(10)) FROM ARRAY RESPON.                   USRIN022
C        RUSER(10) - REAL, CONTAINS BACKGROUND OF RESPON (ONLY          USRIN023
C                    NEEDED IF LUSER(10)=.TRUE.).                       USRIN024
C-----------------------------------------------------------------------USRIN025
C     SUBPROGRAMS CALLED - NONE                                         USRIN026
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               USRIN027
C-----------------------------------------------------------------------USRIN028
      DOUBLE PRECISION PRECIS, RANGE,                                   USRIN029
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       USRIN030
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           USRIN031
     3                 TWOTHR, FORTHR, SIXTEL                           USRIN032
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  USRIN033
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    USRIN034
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRIN035
     3                 SPLINE, WRTBET, WRTCAP                           USRIN036
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), SQRTW(NYMAX,1), NY(*),   USRIN037
     1                 RESPON(*), IHOLER(6)                             USRIN038
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        USRIN039
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, USRIN040
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 USRIN041
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),USRIN042
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     USRIN043
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       USRIN044
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       USRIN045
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   USRIN046
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    USRIN047
     4                 MTRY(10,2), MXITER(2), NL(10),                   USRIN048
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  USRIN049
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, USRIN050
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       USRIN051
     8                 NIN, NOUT                                        USRIN052
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          USRIN053
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         USRIN054
     2                 DOUSOU(2), LUSER(30),                            USRIN055
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRIN056
     4                 SPLINE, WRTBET, WRTCAP                           USRIN057
      DATA             IHOLER/1HU, 1HS, 1HE, 1HR, 1HI, 1HN/             USRIN058
C-----------------------------------------------------------------------USRIN059
C  READ IN RESPONSE/REFERENCE FUNCTION AND WRITE IT (IF PRY=.TRUE.)     USRIN060
C  (IUSER(1) > 1 (I.E. CONVOLUTION) HAS TO BE SET IN THE INPUT DECK).   USRIN061
C-----------------------------------------------------------------------USRIN062
      MY=NY(1)                                                          USRIN063
      READ (NIN,IFORMY) (RESPON(K),K=1,MY)                              USRIN064
      IF (.NOT.PRY) GO TO 100                                           USRIN065
         WRITE (NOUT,1000)                                              USRIN066
         WRITE (NOUT,1100) (RESPON(K),K=1,MY)                           USRIN067
C-----------------------------------------------------------------------USRIN068
C  IF LUSER(10)=.TRUE. SUBTRACT BACKGROUND STORED IN RUSER(10).         USRIN069
C-----------------------------------------------------------------------USRIN070
 100  IF (.NOT. LUSER(10)) GO TO 300                                    USRIN071
         DO 200 K=1,MY                                                  USRIN072
 200     RESPON(K)=RESPON(K)-RUSER(10)                                  USRIN073
C-----------------------------------------------------------------------USRIN074
C  RECOMPUTE PLMNMX(*), THE FACTORS TO SET BOUNDS FOR THE LINEAR        USRIN075
C  PARAMETERS (SEE USERS MANUAL SEC 3.4.3 FOR MORE INFORMATION).        USRIN076
C-----------------------------------------------------------------------USRIN077
 300  REFMAX=RESPON(1)*(T(2,1)-T(1,1))                                  USRIN078
      MYM1  =MY-1                                                       USRIN079
      DO 400 K=2,MYM1                                                   USRIN080
 400  REFMAX=AMAX1(REFMAX,RESPON(K)*.5*(T(K+1,1)-T(K-1,1)))             USRIN081
      REFMAX   =AMAX1(REFMAX,RESPON(MY)*(T(MY,1)-T(MYM1,1)))            USRIN082
      PLMNMX(1)=PLMNMX(1)/REFMAX                                        USRIN083
      PLMNMX(2)=PLMNMX(2)/REFMAX                                        USRIN084
      RETURN                                                            USRIN085
 1000 FORMAT(31H1 REFERENCE/RESPONSE - FUNCTION/)                       USRIN086
 1100 FORMAT(1X,1P10E13.4)                                              USRIN087
      END                                                               USRIN088
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++USROU001
      SUBROUTINE USEROU (IROUTE,NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,USROU002
     1                   PLIN,PLAM,PLMHLP,STDDEV,VAR,ISTAGE,RESPON)     USROU003
C-----------------------------------------------------------------------USROU004
C     USER SUPPLIED SUBROUTINE. (ONLY CALLED WHEN DOUSOU=.TRUE.)        USROU005
C     TO PRODUCE YOUR OWN EXTRA OUTPUT.                                 USROU006
C                                                                       USROU007
C     BELOW IS THE ILLUSTRATED THE CASE IROUTE = 5,                     USROU008
C     TO OUTPUT THE RECIPROCAL PLAM AND ITS STANDARD DEVIATION.         USROU009
C     ONLY DONE IF ISTAGE=2.                                            USROU010
C-----------------------------------------------------------------------USROU011
C                                                                       USROU012
C     DESCRIPTION OF PARAMETERS:                                        USROU013
C                                                                       USROU014
C           IROUTE - TELLS YOU FROM WHICH SUBROUTINE CALL WAS, SO YOU   USROU015
C                    HAVE AN IDEA WHAT IS COMPUTED ALREADY, AND WHAT CANUSROU016
C                    BE USED FOR DOING YOUR OWN OUTPUT.                 USROU017
C                    IROUTE = 1  FROM ANALYZ, TO OUTPUT YOUR OWN        USROU018
C                                STARTING VALUES FOR A COMPLETE ANALYSISUSROU019
C                    IROUTE = 2  FROM LSTSQR, AFTER A NOT NECESSARILY   USROU020
C                                SUCCESFULL ITERATION                   USROU021
C                    IROUTE = 3  FROM OUTALL, TO OUTPUT PARAMETERS      USROU022
C                                AND STD.DEV.                           USROU023
C                    IROUTE = 4  FROM OUTALL, TO OUTPUT RESIDUALS       USROU024
C                                AND FITTED DATA.                       USROU025
C                                E.G. CALL A PLOT ROUTINE.              USROU026
C                    IROUTE = 5  FROM OUTALL, TO OUTPUT BEST PARAMETERS USROU027
C                                AND STD.DEV.                           USROU028
C                    IROUTE = 6  FROM OUTALL, TO OUTPUT RESIDUALS AND   USROU029
C                                FITTED DATA FOR BEST SOLUTION          USROU030
C                                E.G. CALL A PLOT ROUTINE.              USROU031
C           NYMAX  - FIRST DIMENSION OF ARRAYS Y, T, YLYFIT, AND SQRTW, USROU032
C                    I.E. MAXIMUM NUMBER OF DATA POINTS                 USROU033
C                    (NYMAX = MAX(NY(I)),I=1,...,ND)                    USROU034
C           Y      - REAL ARRAY OF DIMENSION (NYMAX,ND), CONTAINS DATA  USROU035
C                    POINTS FOR ND DIFFERENT DATA SETS                  USROU036
C           T      - REAL ARRAY OF DIMENSION (NYMAX,ND), CONTAINS       USROU037
C                    ABSICCA-VALUES FOR ND DIFFERENT DATA SETS          USROU038
C           YLYFIT - REAL ARRAY OF DIMENSION (NYMAX,ND), MAY CONTAIN    USROU039
C                    RESIDUALS FOR ND DIFFERENT DATA SETS, IT DEPENDS   USROU040
C                    ON IROUTE AND IPRINT (IN COMMON)                   USROU041
C           SQRTW  - REAL ARRAY OF DIMENSION (NYMAX,ND), CONTAINS       USROU042
C                    WEIGHTS                                            USROU043
C           NY     - ARRAY OF DIMENSION ND, CONTAINS NUMBER OF DATA     USROU044
C                    POINTS FOR THE ND DIFFERENT DATA SETS              USROU045
C           NLINMX - FIRST DIMENSION OF ARRAY PLIN, I.E. MAXIMUM NUMBER USROU046
C                    OF LINEAR PARAMETERS                               USROU047
C           NONLMX - DIMENSION OF ARRAY PLAM, I.E. MAXIMUM NUMBER OF    USROU048
C                    NONLINEAR PARAMETERS                               USROU049
C           PLIN   - REAL ARRAY OF DIMENSION (NLINMX,ND), CONTAINS      USROU050
C                    LINEAR PARAMETERS FOR ND DIFFERENT DATA SETS       USROU051
C           PLAM   - REAL ARRAY OF DIMENSION NONLMX, CONTAINS NONLINEAR USROU052
C                    PARAMETERS                                         USROU053
C           PLMHLP - REAL ARRAY OF DIMENSION NONLMX, USED AS WORKING    USROU054
C                    SPACE                                              USROU055
C           STDDEV - REAL ARRAY OF DIMENSION NLNDNL, CONTAINS DIAGONAL  USROU056
C                    ELEMENTS OF INVERTED FULL LEAST SQUARES MATRIX,    USROU057
C                    FOR IROUTE = 3 OR 5 ONLY.                          USROU058
C           VAR    - REAL, CONTAINS SUM OF SQUARED WEIGHTED RESIDUALS,  USROU059
C                    FOR IROUTE > 1 ONLY.                               USROU060
C           ISTAGE - IF ISTAGE = 1, THEN YOU ARE IN THE PRELIMINARY     USROU061
C                                   ANALYSIS TO DETERMINE WEIGHTS       USROU062
C                    IF ISTAGE = 2, THEN YOU ARE IN THE FINAL ANALYSIS  USROU063
C           RESPON - REAL ARRAY, CONTAINS RESPONSE FUNCTION IN CASE OF  USROU064
C                    CONVOLUTION (IUSER(1)=2).                          USROU065
C                                                                       USROU066
C-----------------------------------------------------------------------USROU067
C     SUBPROGRAMS CALLED - ERRMES, USERTR                               USROU068
C     WHICH IN TURN CALL - ERRMES                                       USROU069
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               USROU070
C-----------------------------------------------------------------------USROU071
      DOUBLE PRECISION PRECIS, RANGE,                                   USROU072
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       USROU073
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           USROU074
     3                 TWOTHR, FORTHR, SIXTEL                           USROU075
      DOUBLE PRECISION VAR                                              USROU076
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  USROU077
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    USROU078
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USROU079
     3                 SPLINE, WRTBET, WRTCAP                           USROU080
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), YLYFIT(NYMAX,1),         USROU081
     1                 SQRTW(NYMAX,1), NY(*), PLIN(NLINMX,1),           USROU082
     2                 PLAM(NONLMX), PLMHLP(NONLMX), STDDEV(*),         USROU083
     3                 RESPON(*)                                        USROU084
      DIMENSION        IHOLER(6)                                        USROU085
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        USROU086
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, USROU087
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 USROU088
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),USROU089
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     USROU090
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       USROU091
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       USROU092
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   USROU093
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    USROU094
     4                 MTRY(10,2), MXITER(2), NL(10),                   USROU095
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  USROU096
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, USROU097
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       USROU098
     8                 NIN, NOUT                                        USROU099
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          USROU100
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         USROU101
     2                 DOUSOU(2), LUSER(30),                            USROU102
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USROU103
     4                 SPLINE, WRTBET, WRTCAP                           USROU104
      DATA             IHOLER/1HU, 1HS, 1HE, 1HR, 1HO, 1HU/             USROU105
C-----------------------------------------------------------------------USROU106
      IF (ISTAGE.LT.1.OR.ISTAGE.GT.2) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)USROU107
      IF (IROUTE.LT.1.OR.IROUTE.GT.6) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)USROU108
C-----------------------------------------------------------------------USROU109
      IF (ISTAGE .EQ. 1) RETURN                                         USROU110
      GO TO (100,200,300,400,500,600), IROUTE                           USROU111
C-----------------------------------------------------------------------USROU112
C  IROUTE = 1                                                           USROU113
C-----------------------------------------------------------------------USROU114
 100  CONTINUE                                                          USROU115
      RETURN                                                            USROU116
C-----------------------------------------------------------------------USROU117
C  IROUTE = 2                                                           USROU118
C-----------------------------------------------------------------------USROU119
 200  CONTINUE                                                          USROU120
      RETURN                                                            USROU121
C-----------------------------------------------------------------------USROU122
C  IROUTE = 3                                                           USROU123
C-----------------------------------------------------------------------USROU124
 300  CONTINUE                                                          USROU125
      RETURN                                                            USROU126
C-----------------------------------------------------------------------USROU127
C  IROUTE = 4                                                           USROU128
C-----------------------------------------------------------------------USROU129
 400  CONTINUE                                                          USROU130
      RETURN                                                            USROU131
C-----------------------------------------------------------------------USROU132
C  IROUTE = 5                                                           USROU133
C-----------------------------------------------------------------------USROU134
 500  CONTINUE                                                          USROU135
      IF (ISTAGE .EQ. 1) RETURN                                         USROU136
      WRITE (NOUT,5000)                                                 USROU137
      NLNDNL=NLIN*ND+NONL                                               USROU138
      STDVAR=VAR/FLOAT(NYSUM-NLNDNL)                                    USROU139
      STDVAR=SQRT(STDVAR)                                               USROU140
      WRITE (NOUT,5300)                                                 USROU141
      DO 520 J=1,NONL                                                   USROU142
         JD    =J+NLIN*ND                                               USROU143
         PLMINV=1./USERTR(PLAM(J),2)                                    USROU144
         ERRPLM=STDVAR*SQRT(STDDEV(JD))*ABS(PLMINV)                     USROU145
         PRCPLM=100.*ERRPLM/ABS(PLMINV)                                 USROU146
         WRITE (NOUT,5200) PLIN(J+NGAM,1),PLMINV,ERRPLM,PRCPLM          USROU147
 520  CONTINUE                                                          USROU148
      IF (ND.LE.1) RETURN                                               USROU149
      DO 530 K = 2,ND                                                   USROU150
         WRITE (NOUT,5300)                                              USROU151
         DO 531 J = 1,NONL                                              USROU152
            WRITE (NOUT,5200) PLIN(J+NGAM,K)                            USROU153
 531     CONTINUE                                                       USROU154
 530  CONTINUE                                                          USROU155
      RETURN                                                            USROU156
 5000 FORMAT(///15X,5HALPHA,10X,36H1/(LAMBDA)  +-  STD. ERROR   PERCENT)USROU157
 5200 FORMAT(8X,1PE12.4,E20.4,4H  +-,E12.4,0PF10.3)                     USROU158
 5300 FORMAT(1H )                                                       USROU159
C-----------------------------------------------------------------------USROU160
C  IROUTE = 6                                                           USROU161
C-----------------------------------------------------------------------USROU162
 600  CONTINUE                                                          USROU163
      RETURN                                                            USROU164
      END                                                               USROU165
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++USRSI001
      SUBROUTINE USERSI (NYMAX,Y,T,EXACT,NY,RESPON)                     USRSI002
C-----------------------------------------------------------------------USRSI003
C     USER SUPPLIED SUBROUTINE. (ONLY CALLED IF SIMULA=.TRUE.)          USRSI004
C     FOR CALCULATING EXACT (THE SIMULATED DATA BEFORE NOISE IS ADDED)  USRSI005
C     AND Y(K,N) (THE SIMULATED NOISY DATA) FOR K=1,..,NY(N), N=1,..,ND.USRSI006
C     IUSER(3) = STARTING INTEGER FOR RANDOM NUMBER GENERATOR RANDOM.   USRSI007
C     IUSER(3) AND RUSER(3) MUST BE SUPPLIED BY THE USER.               USRSI008
C     IUSER(3) MUST BE BETWEEN 1 AND 2147483646, IF IT IS NOT, THEN     USRSI009
C     IT IS SET TO THE DEFAULT VALUE OF 30171.                          USRSI010
C     USAGE OF RUSER(3) IS DESCRIBED BELOW.                             USRSI011
C     SEE THE USERS MANUAL (SEC 4.2) FOR THE POSITION IN THE INPUT      USRSI012
C     DATA DECK OF ANY INPUT FOR THIS USER SUBPROGRAM.                  USRSI013
C-----------------------------------------------------------------------USRSI014
C     SUBPROGRAMS CALLED - RGAUSS, USEREX                               USRSI015
C     WHICH IN TURN CALL - RANDOM                                       USRSI016
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               USRSI017
C-----------------------------------------------------------------------USRSI018
      DOUBLE PRECISION PRECIS, RANGE,                                   USRSI019
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       USRSI020
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           USRSI021
     3                 TWOTHR, FORTHR, SIXTEL                           USRSI022
      DOUBLE PRECISION DUB                                              USRSI023
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  USRSI024
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    USRSI025
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRSI026
     3                 SPLINE, WRTBET, WRTCAP                           USRSI027
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), EXACT(NYMAX,1),          USRSI028
     1                 NY(*), RN(2), RESPON(*), IHOLER(6)               USRSI029
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        USRSI030
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, USRSI031
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 USRSI032
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),USRSI033
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     USRSI034
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       USRSI035
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       USRSI036
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   USRSI037
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    USRSI038
     4                 MTRY(10,2), MXITER(2), NL(10),                   USRSI039
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  USRSI040
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, USRSI041
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       USRSI042
     8                 NIN, NOUT                                        USRSI043
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          USRSI044
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         USRSI045
     2                 DOUSOU(2), LUSER(30),                            USRSI046
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRSI047
     4                 SPLINE, WRTBET, WRTCAP                           USRSI048
      DATA             IHOLER/1HU, 1HS, 1HE, 1HR, 1HS, 1HI/             USRSI049
C-----------------------------------------------------------------------USRSI050
      TWOPI=6.2831853072D0                                              USRSI051
      DUB  =DBLE(FLOAT(IUSER(3)))                                       USRSI052
      IF (DUB.LT.1.D0 .OR. DUB.GT.2147483646.D0) DUB=30171.D0           USRSI053
      DO 200 N=1,ND                                                     USRSI054
         MY=NY(N)                                                       USRSI055
         NN=N                                                           USRSI056
         DO 100 K=1,MY                                                  USRSI057
            KK=K                                                        USRSI058
C-----------------------------------------------------------------------USRSI059
C  USEREX DELIVERES EXACT FUNCTION VALUE                                USRSI060
C-----------------------------------------------------------------------USRSI061
            EXACT(K,N)=USEREX(NYMAX,T,NY,KK,NN,RESPON)                  USRSI062
C-----------------------------------------------------------------------USRSI063
C  RGAUSS DELIVERS TWO NORMAL DEVIATES.  THEREFORE IT IS ONLY           USRSI064
C  CALLED FOR ODD K.                                                    USRSI065
C-----------------------------------------------------------------------USRSI066
            J=2-MOD(K,2)                                                USRSI067
            IF (J .EQ. 1) CALL RGAUSS (RN(1),RN(2),TWOPI,DUB)           USRSI068
C-----------------------------------------------------------------------USRSI069
C  THE FOLLOWING (COMMENT) STATEMENT IS APPROPRIATE WHEN THE EXPECTED   USRSI070
C  ERROR IS PROPORTIONAL TO SQRT(EXACT), AS IN POISSON PROCESSES.       USRSI071
C  FOR POISSON STATISTICS, RUSER(3) IS SIMPLY TO CORRECT FOR ANY        USRSI072
C  PREVIOUS SCALING IN EXACT,                                           USRSI073
C  I.E.,  RUSER(3)=SQRT(EXACT/(TOTAL COUNTS IN CHANNEL K)).             USRSI074
C  IF EXACT ALREADY IS IN COUNTS, THEN RUSER(3)=1.                      USRSI075
C-----------------------------------------------------------------------USRSI076
C           Y(K,N)=EXACT(K,N)+RUSER(3)*RN(J)*SQRT(EXACT(K,N))           USRSI077
C-----------------------------------------------------------------------USRSI078
C  THE FOLLOWING STATEMENT WOULD SIMULATE ZERO-MEAN GAUSSIAN NOISE      USRSI079
C  WITH STANDARD DEVIATION RUSER(3).                                    USRSI080
C-----------------------------------------------------------------------USRSI081
            Y(K,N)=EXACT(K,N)+RUSER(3)*RN(J)                            USRSI082
 100     CONTINUE                                                       USRSI083
 200  CONTINUE                                                          USRSI084
      RETURN                                                            USRSI085
      END                                                               USRSI086
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++USRST001
      SUBROUTINE USERST (ISTAGE,NYMAX,T,NY,NONLMX,PLAM)                 USRST002
C-----------------------------------------------------------------------USRST003
C     USER SUPPLIED SUBROUTINE.                                         USRST004
C     (ONLY CALLED WHEN DOSTRT(*,ISTAGE)=.TRUE.)                        USRST005
C     IF YOU WANT TO START FIRST TRY OF MTRY(*,ISTAGE) ANALYSES ASSUMINGUSRST006
C     NONL=NL(*) COMPONENTS WITH YOUR OWN STARTING VALUES FOR NONLINEAR USRST007
C     PARAMETERS PLAM.                                                  USRST008
C     BELOW IS ILLUSTRATED THE VERY SIMPLE CASE, THAT YOU HAVE STORED   USRST009
C     IN RUSER(50+I), I=1,...,NONL, THE STARTING VALUES FOR PLAM.       USRST010
C-----------------------------------------------------------------------USRST011
C                                                                       USRST012
C     DESCRIPTION OF PARAMETERS:                                        USRST013
C                                                                       USRST014
C           ISTAGE - IF ISTAGE = 1 YOU ARE IN PRELIMINARY ANALYSIS TO   USRST015
C                                  DETERMINE WEIGHTS                    USRST016
C                    IF ISTAGE = 2 YOU ARE IN FINAL ANALYSIS            USRST017
C           NONLMX - DIMENSION OF ARRAY PLAM, I.E. MAXIMUM NUMBER       USRST018
C                    OF NONLINEAR PARAMETERS                            USRST019
C           PLAM   - REAL ARRAY OF DIMENSION NONLMX, CONTAINS ON        USRST020
C                    OUTPUT STARTING VALUES FOR NONLINEAR PARAMETERS    USRST021
C                                                                       USRST022
C-----------------------------------------------------------------------USRST023
C     SUBPROGRAMS CALLED - ERRMES, USERTR                               USRST024
C     WHICH IN TURN CALL - ERRMES                                       USRST025
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               USRST026
C-----------------------------------------------------------------------USRST027
      DOUBLE PRECISION PRECIS, RANGE,                                   USRST028
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       USRST029
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           USRST030
     3                 TWOTHR, FORTHR, SIXTEL                           USRST031
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  USRST032
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    USRST033
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRST034
     3                 SPLINE, WRTBET, WRTCAP                           USRST035
      DIMENSION        T(NYMAX,1), NY(*), PLAM(NONLMX), IHOLER(6)       USRST036
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        USRST037
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, USRST038
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 USRST039
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),USRST040
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     USRST041
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       USRST042
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       USRST043
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   USRST044
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    USRST045
     4                 MTRY(10,2), MXITER(2), NL(10),                   USRST046
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  USRST047
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, USRST048
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       USRST049
     8                 NIN, NOUT                                        USRST050
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          USRST051
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         USRST052
     2                 DOUSOU(2), LUSER(30),                            USRST053
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRST054
     4                 SPLINE, WRTBET, WRTCAP                           USRST055
      DATA             IHOLER/1HU, 1HS, 1HE, 1HR, 1HS, 1HT/             USRST056
C-----------------------------------------------------------------------USRST057
      IF (ISTAGE.LT.1.OR.ISTAGE.GT.2) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)USRST058
      DO 100 I=1,NONL                                                   USRST059
         DUM    =RUSER(50+I)                                            USRST060
         DUM    =USERTR(DUM,1)                                          USRST061
         PLAM(I)=AMAX1(PNMNMX(1),AMIN1(DUM,PNMNMX(2)))                  USRST062
         IF (PLAM(I) .NE. DUM) CALL ERRMES (2,.FALSE.,IHOLER,NOUT)      USRST063
 100  CONTINUE                                                          USRST064
      RETURN                                                            USRST065
      END                                                               USRST066
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++USRTR001
      FUNCTION USERTR (X,IFUNCT)                                        USRTR002
C-----------------------------------------------------------------------USRTR003
C     USER SUPPLIED SUBROUTINE.                                         USRTR004
C     FOR COMPUTING THE INTERPOLATION-GRID TRANSFORMATION (CALL IT H)   USRTR005
C     H(G) WHEN IFUNCT=1, THE INVERSE TRANSFORMATION WHEN IFUNCT=2,     USRTR006
C     AND THE DERIVATIVE OF THE TRANSFORMATION WHEN IFUNCT=3.           USRTR007
C     BELOW IS ILLUSTRATED THE CASE H(G)=ALOG(G).  FOR ANOTHER H, YOU   USRTR008
C     CAN REPLACE THE STATEMENTS NUMBERED 110, 120, AND 130.            USRTR009
C     THESE ARE THE ONLY STATEMENTS THAT CAN BE REPLACED.  ALSO NOTE    USRTR010
C     THAT ONLY AN H THAT IS MONOTONIC IN THE RANGE OF INTERPOLATION    USRTR011
C     MAKES SENSE.                                                      USRTR012
C-----------------------------------------------------------------------USRTR013
C     SUBPROGRAMS CALLED - ERRMES                                       USRTR014
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               USRTR015
C-----------------------------------------------------------------------USRTR016
      DOUBLE PRECISION PRECIS, RANGE,                                   USRTR017
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       USRTR018
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           USRTR019
     3                 TWOTHR, FORTHR, SIXTEL                           USRTR020
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  USRTR021
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    USRTR022
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRTR023
     3                 SPLINE, WRTBET, WRTCAP                           USRTR024
      DIMENSION        IHOLER(6)                                        USRTR025
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        USRTR026
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, USRTR027
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 USRTR028
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),USRTR029
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     USRTR030
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       USRTR031
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       USRTR032
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   USRTR033
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    USRTR034
     4                 MTRY(10,2), MXITER(2), NL(10),                   USRTR035
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  USRTR036
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, USRTR037
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       USRTR038
     8                 NIN, NOUT                                        USRTR039
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          USRTR040
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         USRTR041
     2                 DOUSOU(2), LUSER(30),                            USRTR042
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRTR043
     4                 SPLINE, WRTBET, WRTCAP                           USRTR044
      DATA             IHOLER/1HU, 1HS, 1HE, 1HR, 1HT, 1HR/             USRTR045
C-----------------------------------------------------------------------USRTR046
      IF (IFUNCT.LT.1.OR.IFUNCT.GT.3) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)USRTR047
      GO TO (110,120,130),IFUNCT                                        USRTR048
C-----------------------------------------------------------------------USRTR049
C  COMPUTE TRANSFORMATION.                                              USRTR050
C-----------------------------------------------------------------------USRTR051
 110  IF (X .LE. 0.) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                 USRTR052
      USERTR=ALOG(X)                                                    USRTR053
      RETURN                                                            USRTR054
C-----------------------------------------------------------------------USRTR055
C  COMPUTE INVERSE TRANSFORMATION.                                      USRTR056
C-----------------------------------------------------------------------USRTR057
 120  USERTR=0.                                                         USRTR058
      IF (-X .GT. EXMAX) RETURN                                         USRTR059
      IF ( X .GT. EXMAX) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)             USRTR060
      USERTR=EXP(X)                                                     USRTR061
      RETURN                                                            USRTR062
C-----------------------------------------------------------------------USRTR063
C  COMPUTE DERIVATIVE OF TRANSFORMATION.                                USRTR064
C-----------------------------------------------------------------------USRTR065
 130  IF (X .EQ. 0.) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                 USRTR066
      USERTR=1./X                                                       USRTR067
      RETURN                                                            USRTR068
      END                                                               USRTR069
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++USRWT001
      SUBROUTINE USERWT (NYMAX,Y,T,YLYFIT,SQRTW,NY,ERRFIT,RESPON)       USRWT002
C-----------------------------------------------------------------------USRWT003
C     USER SUPPLIED SUBROUTINE. (ONLY NEEDED WHEN IWT=5)                USRWT004
C     FOR CALCULATING SQRTW (SQUARE ROOTS OF THE LEAST SQUARES WEIGHTS) USRWT005
C     FROM Y, YLYFIT, AND ERRFIT, AS EXPLAINED IN DETAIL IN SEC 4.1.3.4 USRWT006
C     OF THE USERS MANUAL.                                              USRWT007
C     SEE THE USERS MANUAL FOR THE POSITION IN THE INPUT DATA DECK OF   USRWT008
C     ANY INPUT FOR THIS USER SUBPROGRAM.                               USRWT009
C     BELOW IS ILLUSTRATED A CASE FROM PHOTON CORRELATION SPECTROSCOPY, USRWT010
C     WHERE THE VARIANCE OF Y IS PROPORTIONAL TO (Y**2+1)/(4*Y**2).     USRWT011
C-----------------------------------------------------------------------USRWT012
C     SUBPROGRAMS CALLED - NONE                                         USRWT013
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               USRWT014
C-----------------------------------------------------------------------USRWT015
      DOUBLE PRECISION PRECIS, RANGE,                                   USRWT016
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       USRWT017
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           USRWT018
     3                 TWOTHR, FORTHR, SIXTEL                           USRWT019
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  USRWT020
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    USRWT021
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRWT022
     3                 SPLINE, WRTBET, WRTCAP                           USRWT023
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), YLYFIT(NYMAX,1),         USRWT024
     1                 SQRTW(NYMAX,1), NY(*), ERRFIT(*), RESPON(*)      USRWT025
      DIMENSION        IHOLER(6)                                        USRWT026
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        USRWT027
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, USRWT028
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 USRWT029
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),USRWT030
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     USRWT031
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       USRWT032
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       USRWT033
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   USRWT034
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    USRWT035
     4                 MTRY(10,2), MXITER(2), NL(10),                   USRWT036
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  USRWT037
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, USRWT038
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       USRWT039
     8                 NIN, NOUT                                        USRWT040
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          USRWT041
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         USRWT042
     2                 DOUSOU(2), LUSER(30),                            USRWT043
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  USRWT044
     4                 SPLINE, WRTBET, WRTCAP                           USRWT045
      DATA             IHOLER/1HU, 1HS, 1HE, 1HR, 1HW, 1HT/             USRWT046
C-----------------------------------------------------------------------USRWT047
C  YOU MUST REPLACE THE FOLLOWING STATEMENTS WITH THOSE APPROPRIATE     USRWT048
C  FOR YOUR APPLICATION.                                                USRWT049
C-----------------------------------------------------------------------USRWT050
      DO 100 N=1,ND                                                     USRWT051
      MY=NY(N)                                                          USRWT052
      DO 100 K=1,MY                                                     USRWT053
         DUM       =AMAX1(ABS(Y(K,N)-YLYFIT(K,N)),ERRFIT(N))            USRWT054
         SQRTW(K,N)=2.*DUM/SQRT(DUM*DUM+1.)                             USRWT055
  100 CONTINUE                                                          USRWT056
      RETURN                                                            USRWT057
      END                                                               USRWT058
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++ANLYZ001
      SUBROUTINE ANALYZ (ISTAGE,NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,ANLYZ002
     1                   PLIN,PLNTRY,PLAM,PLMTRY,PLMSAV,PIVLIN,PIVLAM,  ANLYZ003
     2                   EK,EPK,ETE,R,DELTAL,DELTAN,GTG,H,DELTAH,EKEK,  ANLYZ004
     3                   ASAVE,NOKSET,NCOMBI,MTRYMX,STARTV,NLNDMX,ATOT, ANLYZ005
     4                   DTOLER,BUFF,DX,BSPL0,BSPL1,BSPL2,IC,NZMAX,     ANLYZ006
     5                   NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,RESPON) ANLYZ007
C-----------------------------------------------------------------------ANLYZ008
C  FIND STARTING VALUES FOR PLAM, PERFORMS LEAST SQUARES ANALYSIS, AND  ANLYZ009
C  OUTPUT FINAL RESULTS AS SPECIFIED THROUGH IPRINT, IPLFIT, AND IPLRES ANLYZ010
C-----------------------------------------------------------------------ANLYZ011
C     SUBPROGRAMS CALLED - ERRMES, FISHNI, LSTSQR, NOKSUB, NPASCL,      ANLYZ012
C                          OUTALL, USEROU, USERST, USERTR, VARIAN       ANLYZ013
C     WHICH IN TURN CALL - BETAIN, BSPLIN, ERRMES, FULLSM, GAMLN,       ANLYZ014
C                          GETPRU, HESEXA, HESSEN, HESSPL, INTERP,      ANLYZ015
C                          INTERV, LINEQS, LINPAR, NPASCL, OUTCOR,      ANLYZ016
C                          OUTITR, OUTPAR, PGAUSS, PLPRIN, PLRES,       ANLYZ017
C                          PIVOT,  PIVOT1, USERFL, USERFN, USEROU,      ANLYZ018
C                          USERTR, VARIAN                               ANLYZ019
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               ANLYZ020
C-----------------------------------------------------------------------ANLYZ021
      DOUBLE PRECISION PRECIS, RANGE,                                   ANLYZ022
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       ANLYZ023
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           ANLYZ024
     3                 TWOTHR, FORTHR, SIXTEL                           ANLYZ025
      DOUBLE PRECISION EK, EPK, ETE, R, DELTAL, DELTAN, GTG, H,         ANLYZ026
     1                 DELTAH, EKEK, ASAVE, ATOT, DTOLER, DX, BSPL0,    ANLYZ027
     2                 BSPL1, BSPL2, CCAP, ECAP, YCAP, SGGCAP,          ANLYZ028
     3                 SGYCAP, YW, VAR, VARBES                          ANLYZ029
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  ANLYZ030
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    ANLYZ031
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  ANLYZ032
     3                 SPLINE, WRTBET, WRTCAP                           ANLYZ033
      LOGICAL          PIVLIN, PIVLAM, STARTV, OWNST, MORE, BETTER,     ANLYZ034
     1                 PRBEST, PRHEAD, ERRONE, ADEXCT                   ANLYZ035
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), YLYFIT(NYMAX,1),         ANLYZ036
     1                 SQRTW(NYMAX,1), NY(*), RESPON(*), PLIN(NLINMX,1),ANLYZ037
     2                 PLNTRY(NLINMX,1), PLAM(NONLMX), PLMTRY(NONLMX),  ANLYZ038
     3                 PLMSAV(NONLMX,1), PIVLIN(NLINMX,1),              ANLYZ039
     4                 PIVLAM(NLNDMX), EK(NLINMX), EPK(NONLMX),         ANLYZ040
     5                 ETE(NLINMX,NLINMX), R(NLINMX,1), DELTAL(NLINMX), ANLYZ041
     6                 DELTAN(NONLMX), GTG(NONLMX,NONLMX)               ANLYZ042
      DIMENSION        H(NLINMX,NLINMX,1), DELTAH(NONLMX,1),            ANLYZ043
     1                 EKEK(NLINMX,NLINMX,1), ASAVE(NLINMX,NLINMX,1),   ANLYZ044
     2                 NOKSET(NONLMX), NCOMBI(NONLMX), STARTV(MTRYMX),  ANLYZ045
     3                 ATOT(NLNDMX,NLNDMX), DTOLER(NLNDMX), DX(NONLMX), ANLYZ046
     4                 BUFF(NLNDMX), BSPL0(4,NONLMX), BSPL1(4,NONLMX),  ANLYZ047
     5                 BSPL2(4,NONLMX), IC(NONLMX), CCAP(NZMAX,NZMAX,1),ANLYZ048
     6                 ECAP(NZMAX,NGAMMX,1), YCAP(NZMAX,1),             ANLYZ049
     7                 SGGCAP(NGAMMX,NGAMMX,1), SGYCAP(NGAMMX,1), YW(*) ANLYZ050
      DIMENSION        IHOLER(6)                                        ANLYZ051
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        ANLYZ052
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, ANLYZ053
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 ANLYZ054
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),ANLYZ055
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     ANLYZ056
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       ANLYZ057
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       ANLYZ058
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   ANLYZ059
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    ANLYZ060
     4                 MTRY(10,2), MXITER(2), NL(10),                   ANLYZ061
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  ANLYZ062
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, ANLYZ063
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       ANLYZ064
     8                 NIN, NOUT                                        ANLYZ065
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          ANLYZ066
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         ANLYZ067
     2                 DOUSOU(2), LUSER(30),                            ANLYZ068
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  ANLYZ069
     4                 SPLINE, WRTBET, WRTCAP                           ANLYZ070
      DATA             IHOLER/1HA, 1HN, 1HA, 1HL, 1HY, 1HZ/             ANLYZ071
C-----------------------------------------------------------------------ANLYZ072
C  INITIAL COMPUTATIONS                                                 ANLYZ073
C-----------------------------------------------------------------------ANLYZ074
      PRBEST=.FALSE.                                                    ANLYZ075
      PRHEAD=.FALSE.                                                    ANLYZ076
      DO 100 J=1,3                                                      ANLYZ077
         IF (IPRINT(J,ISTAGE).EQ.1 .OR. IPRINT(J,ISTAGE).EQ.3)          ANLYZ078
     1       PRBEST=.TRUE.                                              ANLYZ079
         IF (IPRINT(J,ISTAGE) .GE. 2) PRHEAD=.TRUE.                     ANLYZ080
 100  CONTINUE                                                          ANLYZ081
      IF (IPRITR(ISTAGE) .GE. 1) PRHEAD=.TRUE.                          ANLYZ082
      IAPFIT=IABS(IPLFIT(ISTAGE))                                       ANLYZ083
      IF (IAPFIT        .EQ.1 .OR. IAPFIT        .EQ.3) PRBEST=.TRUE.   ANLYZ084
      IF (IPLRES(ISTAGE).EQ.1 .OR. IPLRES(ISTAGE).EQ.3) PRBEST=.TRUE.   ANLYZ085
      IF (IAPFIT         .GE. 2) PRHEAD=.TRUE.                          ANLYZ086
      IF (IPLRES(ISTAGE) .GE. 2) PRHEAD=.TRUE.                          ANLYZ087
C-----------------------------------------------------------------------ANLYZ088
C  START OF NNL DIFFERENT ANALYSIS, ASSUMING NL(L) NONLINEAR            ANLYZ089
C  COMPONENTS, L=1,...,NNL                                              ANLYZ090
C-----------------------------------------------------------------------ANLYZ091
      DO 700 L=1,NNL                                                    ANLYZ092
         LOOP  =L                                                       ANLYZ093
         NONL  =NL(L)                                                   ANLYZ094
         NONLP1=NONL+1                                                  ANLYZ095
         NLIN  =NONL+NGAM                                               ANLYZ096
         NLND  =NLIN*ND                                                 ANLYZ097
         NLNDNL=NLND+NONL                                               ANLYZ098
         IF (NGAM .LE. 1) NPLINE=MIN0(4,NONL)                           ANLYZ099
         VARSAV(L)=RANGE                                                ANLYZ100
         SPLINE   =DOSPL(L,ISTAGE)                                      ANLYZ101
         ADEXCT   =DOADEX(L,ISTAGE) .AND. SPLINE                        ANLYZ102
         ERRONE   =.NOT.PRHEAD                                          ANLYZ103
C-----------------------------------------------------------------------ANLYZ104
C  COMPUTE MAXIMUM NUMBER OF TRIALS MMTRY, SO THAT                      ANLYZ105
C               MBOX                                                    ANLYZ106
C     MMTRY := (    )  .LE.  MTRY(L,ISTAGE)                             ANLYZ107
C               NONL                                                    ANLYZ108
C-----------------------------------------------------------------------ANLYZ109
         MMTRY =MTRY(L,ISTAGE)                                          ANLYZ110
         MBOX  =NONL                                                    ANLYZ111
         NOVERK=1                                                       ANLYZ112
 200     NTEMP =NOVERK                                                  ANLYZ113
         NOVERK=NPASCL(NTEMP,MBOX,NONL,1)                               ANLYZ114
         IF (NOVERK .EQ. MMTRY) GO TO 220                               ANLYZ115
            IF (NOVERK .LT. MMTRY) GO TO 200                            ANLYZ116
            MBOX =MBOX-1                                                ANLYZ117
            MMTRY=NTEMP                                                 ANLYZ118
 220     DBOX=ZTOTAL/FLOAT(MBOX)                                        ANLYZ119
         DO 240 J=1,MMTRY                                               ANLYZ120
 240     STARTV(J)=.FALSE.                                              ANLYZ121
         OWNST=DOSTRT(L,ISTAGE)                                         ANLYZ122
         MORE =.FALSE.                                                  ANLYZ123
         M    =0                                                        ANLYZ124
C-----------------------------------------------------------------------ANLYZ125
C  START OF MMTRY ANALYSIS, ASSUMING NONL:=NL(L) NONLINEAR COMPONENTS   ANLYZ126
C-----------------------------------------------------------------------ANLYZ127
         IF (ISTAGE.EQ.1 .AND. PRHEAD) WRITE (NOUT,1000) ITITLE,NONL    ANLYZ128
         IF (ISTAGE.EQ.2 .AND. PRHEAD) WRITE (NOUT,1500) ITITLE,NONL    ANLYZ129
 300     M=M+1                                                          ANLYZ130
         IF (M .GT. MMTRY) GO TO 500                                    ANLYZ131
C-----------------------------------------------------------------------ANLYZ132
C  LOOK FOR STARTING VALUES FOR PARAMETER PLAM                          ANLYZ133
C-----------------------------------------------------------------------ANLYZ134
         IF (.NOT. OWNST) GO TO 330                                     ANLYZ135
            OWNST=.FALSE.                                               ANLYZ136
            M    =M-1                                                   ANLYZ137
            CALL USERST (ISTAGE,NYMAX,T,NY,NONLMX,PLAM)                 ANLYZ138
            IF (DOUSOU(ISTAGE)) CALL USEROU (1,NYMAX,Y,T,YLYFIT,SQRTW,  ANLYZ139
     1         NY,NLINMX,NONLMX,PLIN,PLAM,PLMTRY,BUFF,VAR,ISTAGE,RESPON)ANLYZ140
C-----------------------------------------------------------------------ANLYZ141
C  CHECK IF USER'S PLAM(J), J=1,...,NONL, ARE FEASIBLE STARTING VALUES  ANLYZ142
C-----------------------------------------------------------------------ANLYZ143
            DO 310 J=1,NONL                                             ANLYZ144
            IF (PLAM(J).LT.PNMNMX(1).OR.PLAM(J).GT.PNMNMX(2)) GO TO 300 ANLYZ145
 310        CONTINUE                                                    ANLYZ146
            DO 320 J=1,NONL                                             ANLYZ147
 320        NCOMBI(J)=0                                                 ANLYZ148
            GO TO 350                                                   ANLYZ149
 330     CALL NOKSUB (MBOX,NONL,NOKSET,LTEMPL,MTEMPM,MORE)              ANLYZ150
         IF (STARTV(M)) GO TO 300                                       ANLYZ151
         DO 340 J=1,NONL                                                ANLYZ152
            I        =NOKSET(J)                                         ANLYZ153
            NCOMBI(J)=I                                                 ANLYZ154
            PLAM(J)  =(FLOAT(I)-.5)*DBOX+PNMNMX(1)                      ANLYZ155
 340     CONTINUE                                                       ANLYZ156
 350     IERROR=0                                                       ANLYZ157
         ALPLAM=.TRUE.                                                  ANLYZ158
C-----------------------------------------------------------------------ANLYZ159
C  SET FEASIBLE STARTING VALUES FOR PLIN                                ANLYZ160
C-----------------------------------------------------------------------ANLYZ161
         DO 360 J=1,NLIN                                                ANLYZ162
 360     PLIN(J,1)=AMIN1(PLMNMX(2),AMAX1(0.E0,PLMNMX(1)))               ANLYZ163
         IF (ND .EQ. 1) GO TO 400                                       ANLYZ164
            DO 370 N=2,ND                                               ANLYZ165
            DO 370 J=1,NLIN                                             ANLYZ166
 370        PLIN(J,N)=PLIN(J,1)                                         ANLYZ167
C-----------------------------------------------------------------------ANLYZ168
C  SOLVE NONLINEAR PROBLEM USING STEPWISE REGRESSION                    ANLYZ169
C-----------------------------------------------------------------------ANLYZ170
 400     CALL LSTSQR (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,     ANLYZ171
     1                PLNTRY,PLAM,PLMTRY,PIVLIN,PIVLAM,EK,EPK,ETE,EKEK, ANLYZ172
     2                R,DELTAL,DTOLER,DELTAN,GTG,H,DELTAH,ASAVE,NCOMBI, ANLYZ173
     3                MTRYMX,STARTV,DX,BSPL0,BSPL1,BSPL2,IC,NZMAX,      ANLYZ174
     4                NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,VAR,       ANLYZ175
     5                ISTAGE,IERROR,RESPON,BUFF)                        ANLYZ176
         IF (IERROR.EQ.0 .OR.                                           ANLYZ177
     1      (IPRITR(ISTAGE).GT.0 .AND. IERROR.NE.3)) GO TO 450          ANLYZ178
            IF (ISTAGE.EQ.1 .AND. ERRONE)                               ANLYZ179
     1          WRITE (NOUT,1000) ITITLE,NONL                           ANLYZ180
            IF (ISTAGE.EQ.2 .AND. ERRONE)                               ANLYZ181
     1          WRITE (NOUT,1500) ITITLE,NONL                           ANLYZ182
            ERRONE=.FALSE.                                              ANLYZ183
            IF (IERROR-2) 410,420,430                                   ANLYZ184
 410        CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                         ANLYZ185
            GO TO 450                                                   ANLYZ186
 420        CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                         ANLYZ187
            GO TO 450                                                   ANLYZ188
 430        IF (IERROR .EQ. 4) GO TO 440                                ANLYZ189
               IF (IPRITR(ISTAGE) .EQ. 0) CALL ERRMES (3,               ANLYZ190
     1            .FALSE.,IHOLER,NOUT)                                  ANLYZ191
               GO TO 300                                                ANLYZ192
 440        CALL ERRMES (4,.FALSE.,IHOLER,NOUT)                         ANLYZ193
 450     BETTER=VAR .LT. VARSAV(L)                                      ANLYZ194
C-----------------------------------------------------------------------ANLYZ195
C  OUTPUT INTERMEDIATE RESULTS AS SPECIFIED THROUGH IPRINT,             ANLYZ196
C  IPLRES, AND IPLFIT                                                   ANLYZ197
C-----------------------------------------------------------------------ANLYZ198
         CALL OUTALL (ISTAGE,NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,   ANLYZ199
     1                PLIN,PLNTRY,PLAM,PLMTRY,PLMSAV,PIVLIN,PIVLAM,EK,  ANLYZ200
     2                EPK,ETE,R,DELTAL,DELTAN,GTG,H,DELTAH,EKEK,ASAVE,  ANLYZ201
     3                NCOMBI,MTRYMX,STARTV,NLNDMX,ATOT,DTOLER,BUFF,DX,  ANLYZ202
     4                BSPL0,BSPL1,BSPL2,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP, ANLYZ203
     5                SGGCAP,SGYCAP,YW,BETTER,IERROR,LOOP,NLND,NLNDNL,  ANLYZ204
     6                VAR,.FALSE.,RESPON)                               ANLYZ205
         GO TO 300                                                      ANLYZ206
C-----------------------------------------------------------------------ANLYZ207
C  DO ADDITIONAL FIT USING EXACT FUNCTIONS, IF DOADEX(L,ISTAGE)=.TRUE.  ANLYZ208
C  USING PARAMETERS FOUND IN BEST FIT AS STARTING VALUES                ANLYZ209
C-----------------------------------------------------------------------ANLYZ210
 500     IF (.NOT. ADEXCT) GO TO 600                                    ANLYZ211
         IF (ISTAGE.EQ.1 .AND. ERRONE)                                  ANLYZ212
     1       WRITE (NOUT,1000) ITITLE,NONL                              ANLYZ213
         IF (ISTAGE.EQ.2 .AND. ERRONE)                                  ANLYZ214
     1       WRITE (NOUT,1500) ITITLE,NONL                              ANLYZ215
         WRITE (NOUT,2000)                                              ANLYZ216
         ADEXCT=.FALSE.                                                 ANLYZ217
         SPLINE=.FALSE.                                                 ANLYZ218
         DO 520 J=1,NONL                                                ANLYZ219
 520     PLAM(J)=PLMSAV(J,L)                                            ANLYZ220
         GO TO 350                                                      ANLYZ221
C-----------------------------------------------------------------------ANLYZ222
C  OUTPUT STANDARD-DEVIATION OF BEST FIT, ALL PARAMETERS TOGETHER WITH  ANLYZ223
C  STANDARD-DEVIATIONS, AND CORRELATION MATRIX, IF IPRINT(*,ISTAGE) = 1 ANLYZ224
C-----------------------------------------------------------------------ANLYZ225
 600     SPLINE=DOSPL(L,ISTAGE)                                         ANLYZ226
         IF (VARSAV(L) .LT. RANGE) GO TO 610                            ANLYZ227
            CALL ERRMES (5,.FALSE.,IHOLER,NOUT)                         ANLYZ228
            GO TO 700                                                   ANLYZ229
 610     IF (.NOT. PRBEST) GO TO 700                                    ANLYZ230
         IF (ISTAGE .EQ. 1) WRITE (NOUT,3000) ITITLE,NONL               ANLYZ231
         IF (ISTAGE .EQ. 2) WRITE (NOUT,3500) ITITLE,NONL               ANLYZ232
         DO 620 J=1,NONL                                                ANLYZ233
 620     PLAM(J)=PLMSAV(J,L)                                            ANLYZ234
         DO 630 N=1,ND                                                  ANLYZ235
         DO 630 J=1,NLIN                                                ANLYZ236
 630     PLIN(J,N)=AMIN1(PLMNMX(2),AMAX1(0.E0,PLMNMX(1)))               ANLYZ237
         ALPLAM=.FALSE.                                                 ANLYZ238
         Q     =0.                                                      ANLYZ239
         CALL VARIAN (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,     ANLYZ240
     1                PLNTRY,PLAM,PLMTRY,PIVLIN,EK,ETE,EKEK,R,DELTAL,   ANLYZ241
     2                DTOLER,DELTAN,ASAVE,.FALSE.,NCOMBI,MTRYMX,STARTV, ANLYZ242
     3                DX,BSPL0,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,   ANLYZ243
     4                SGYCAP,YW,Q,VAR,IERROR,.FALSE.,RESPON)            ANLYZ244
         DO 650 N=1,ND                                                  ANLYZ245
         DO 650 J=1,NLIN                                                ANLYZ246
 650     PLIN(J,N)=PLNTRY(J,N)                                          ANLYZ247
         CALL OUTALL (ISTAGE,NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,   ANLYZ248
     1                PLIN,PLNTRY,PLAM,PLMTRY,PLMSAV,PIVLIN,PIVLAM,EK,  ANLYZ249
     2                EPK,ETE,R,DELTAL,DELTAN,GTG,H,DELTAH,EKEK,ASAVE,  ANLYZ250
     3                NCOMBI,MTRYMX,STARTV,NLNDMX,ATOT,DTOLER,BUFF,DX,  ANLYZ251
     4                BSPL0,BSPL1,BSPL2,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP, ANLYZ252
     5                SGGCAP,SGYCAP,YW,BETTER,IERROR,LOOP,NLND,NLNDNL,  ANLYZ253
     6                VAR,.TRUE.,RESPON)                                ANLYZ254
 700  CONTINUE                                                          ANLYZ255
C-----------------------------------------------------------------------ANLYZ256
C  MAKE F-TEST                                                          ANLYZ257
C-----------------------------------------------------------------------ANLYZ258
      VARBES=RANGE                                                      ANLYZ259
      VARMIN=RANGE                                                      ANLYZ260
      NONL  =1                                                          ANLYZ261
      DO 720 L=1,NNL                                                    ANLYZ262
         INDEX(L)=1                                                     ANLYZ263
         DO 710 I=1,NNL                                                 ANLYZ264
            IF (NL(I) .GE. NL(L)) GO TO 710                             ANLYZ265
            INDEX(L)=INDEX(L)+1                                         ANLYZ266
 710     CONTINUE                                                       ANLYZ267
 720  CONTINUE                                                          ANLYZ268
      DO 750 L=1,NNL                                                    ANLYZ269
         NLAM=NL(INDEX(L))                                              ANLYZ270
         IF (VARBES .GE. RANGE) GO TO 730                               ANLYZ271
            IF (VARSAV(INDEX(L)) .GE. VARMIN) GO TO 750                 ANLYZ272
            DUM =VARBES/VARSAV(INDEX(L))-ONE                            ANLYZ273
            DGF1=FLOAT(NYSUM-ND*(NLAM+NGAM)-NLAM)                       ANLYZ274
            DGF2=FLOAT((ND+1)*(NLAM-NONL))                              ANLYZ275
            DUM =DGF1*DUM/DGF2                                          ANLYZ276
            IF (FISHNI(DUM,DGF2,DGF1,NOUT) .LE. .5) GO TO 740           ANLYZ277
 730     NONL  =NLAM                                                    ANLYZ278
         VARBES=VARSAV(INDEX(L))                                        ANLYZ279
C740     VARMIN=AMIN1(VARMIN,VARSAV(INDEX(L)))                          ANLYZ280
 740     VARMIN=DMIN1(VARMIN,VARSAV(INDEX(L)))                          ANLYZ281
 750  CONTINUE                                                          ANLYZ282
      IF (VARBES .LT. RANGE) GO TO 760                                  ANLYZ283
         CALL ERRMES (6,.FALSE.,IHOLER,NOUT)                            ANLYZ284
         IWT=1                                                          ANLYZ285
         RETURN                                                         ANLYZ286
 760  IF (ISTAGE.EQ.1 .OR. NNL.GT.1) WRITE (NOUT,1200)                  ANLYZ287
      IF (NNL .GT. 1) WRITE (NOUT,4000) NONL                            ANLYZ288
      DO 800 L=1,NNL                                                    ANLYZ289
         NLAM=NL(INDEX(L))                                              ANLYZ290
         IF (NLAM .NE. NONL) GO TO 770                                  ANLYZ291
            PNG(L)=0.                                                   ANLYZ292
            I     =L                                                    ANLYZ293
            GO TO 800                                                   ANLYZ294
 770     DGF1=FLOAT(NYSUM-(ND+1)*MAX0(NLAM,NONL)-ND*NGAM)               ANLYZ295
         DGF2=FLOAT((ND+1)*(NONL-NLAM))                                 ANLYZ296
         DUM =VARSAV(INDEX(L))/VARBES                                   ANLYZ297
         IF (DGF2 .LT. 0.) GO TO 780                                    ANLYZ298
            DUM   =DGF1*(DUM-1.)/DGF2                                   ANLYZ299
            PNG(L)=FISHNI(DUM,DGF2,DGF1,NOUT)                           ANLYZ300
            GO TO 800                                                   ANLYZ301
 780     IF (DUM .LT. 1.) GO TO 790                                     ANLYZ302
            PNG(L)=1.                                                   ANLYZ303
            GO TO 800                                                   ANLYZ304
 790     DUM   =DGF1*(1.-1./DUM)/DGF2                                   ANLYZ305
         PNG(L)=1.-FISHNI(DUM,-DGF2,DGF1,NOUT)                          ANLYZ306
 800  CONTINUE                                                          ANLYZ307
      IF (NNL .GT. 1) WRITE (NOUT,5000)                                 ANLYZ308
     1               (IGAMMA(1),NL(INDEX(L)),NONL,PNG(L),L=1,NNL)       ANLYZ309
      IF (ISTAGE .EQ. 2) RETURN                                         ANLYZ310
      SPLINE=DOSPL(I,ISTAGE)                                            ANLYZ311
      NONLP1=NONL+1                                                     ANLYZ312
C-----------------------------------------------------------------------ANLYZ313
C  EVALUATE PLIN FOR BEST PLAM TO GENERATE WEIGHTS                      ANLYZ314
C-----------------------------------------------------------------------ANLYZ315
      DO 820 J=1,NONL                                                   ANLYZ316
 820  PLAM(J)=PLMSAV(J,I)                                               ANLYZ317
      NLIN=NONL+NGAM                                                    ANLYZ318
      DO 830 N=1,ND                                                     ANLYZ319
      DO 830 J=1,NLIN                                                   ANLYZ320
 830  PLIN(J,N)=AMIN1(PLMNMX(2),AMAX1(0.E0,PLMNMX(1)))                  ANLYZ321
      ALPLAM=.FALSE.                                                    ANLYZ322
      Q     =0.                                                         ANLYZ323
      CALL VARIAN (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,PLNTRY, ANLYZ324
     1             PLAM,PLMTRY,PIVLIN,EK,ETE,EKEK,R,DELTAL,DTOLER,      ANLYZ325
     2             DELTAN,ASAVE,.FALSE.,NCOMBI,MTRYMX,STARTV,DX,BSPL0,  ANLYZ326
     3             IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,Q,   ANLYZ327
     4             VAR,IERROR,.FALSE.,RESPON)                           ANLYZ328
      DO 840 N=1,ND                                                     ANLYZ329
      DO 840 J=1,NLIN                                                   ANLYZ330
 840  PLIN(J,N)=PLNTRY(J,N)                                             ANLYZ331
      DO 850 J=1,NONL                                                   ANLYZ332
 850  PLMTRY(J)=USERTR(PLAM(J),2)                                       ANLYZ333
      WRITE (NOUT,6000)                                                 ANLYZ334
      IF (NGAM .GT. 0) GO TO 910                                        ANLYZ335
         WRITE (NOUT,7000)                                              ANLYZ336
         WRITE (NOUT,7200) (PLIN(J,1),PLMTRY(J),J=1,NONL)               ANLYZ337
         IF (ND .EQ. 1) GO TO 950                                       ANLYZ338
         DO 900 N=2,ND                                                  ANLYZ339
            WRITE (NOUT,6200)                                           ANLYZ340
            WRITE (NOUT,7500) (PLIN(J,N),J=1,NONL)                      ANLYZ341
 900     CONTINUE                                                       ANLYZ342
         GO TO 950                                                      ANLYZ343
 910  WRITE (NOUT,8000)                                                 ANLYZ344
      JEND=MIN0(NONL,NGAM)                                              ANLYZ345
      DO 940 N=1,ND                                                     ANLYZ346
         IF (N .EQ. 1) WRITE (NOUT,8200)                                ANLYZ347
     1                (PLIN(J,1),PLIN(J+NGAM,1),PLMTRY(J),J=1,JEND)     ANLYZ348
         IF (N .GT. 1) WRITE (NOUT,6200)                                ANLYZ349
         IF (N .GT. 1) WRITE (NOUT,8400)                                ANLYZ350
     1                       (PLIN(J,N),PLIN(J+NGAM,N),J=1,JEND)        ANLYZ351
         IF (NONL-NGAM) 920,940,930                                     ANLYZ352
 920        WRITE (NOUT,8600) (PLIN(J,N),J=NONLP1,NGAM)                 ANLYZ353
            GO TO 940                                                   ANLYZ354
 930     IF (N .EQ. 1) WRITE (NOUT,8800)                                ANLYZ355
     1                       (PLIN(J+NGAM,1),PLMTRY(J),J=NGAMP1,NONL)   ANLYZ356
         IF (N .GT. 1) WRITE (NOUT,9000) (PLIN(J+NGAM,N),J=NGAMP1,NONL) ANLYZ357
 940  CONTINUE                                                          ANLYZ358
 950  IF (.NOT. SPLINE) RETURN                                          ANLYZ359
      CALL VARIAN (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLNTRY,PLIN, ANLYZ360
     1             PLMTRY,PLAM,PIVLIN,EK,ETE,EKEK,R,DELTAL,DTOLER,      ANLYZ361
     2             DELTAN, ASAVE,.FALSE.,NCOMBI,MTRYMX,STARTV,DX,BSPL0, ANLYZ362
     3             IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,Q,   ANLYZ363
     4             VAR,IERROR,.TRUE.,RESPON)                            ANLYZ364
      RETURN                                                            ANLYZ365
 1000 FORMAT(1H1,1X,80A1,3X,22HPRELIMINARY UNWEIGHTED,I4,               ANLYZ366
     1                      21H  COMPONENTS ANALYSIS)                   ANLYZ367
 1200 FORMAT(1H1)                                                       ANLYZ368
 1500 FORMAT(1H1,1X,80A1,20X,5HFINAL,I4,21H  COMPONENTS ANALYSIS)       ANLYZ369
 2000 FORMAT(//23X,41H END OF FIT USING SPLINE APPROXIMATIONS, ,        ANLYZ370
     1             30HSTART OF FIT USING EXACT MODEL)                   ANLYZ371
 3000 FORMAT(1H1,1X,80A1,37X,13HBEST SOLUTION,/82X,14HIN PRELIMINARY,   ANLYZ372
     1                    11H UNWEIGHTED,I4,21H  COMPONENTS ANALYSIS)   ANLYZ373
 3500 FORMAT(1H1,1X,80A1,37X,13HBEST SOLUTION,/99X,8HIN FINAL,          ANLYZ374
     1                    I4,21H  COMPONENTS ANALYSIS)                  ANLYZ375
 4000 FORMAT(6X,27HFOUND BEST FIT TO DATA WITH,I5,3X,11HCOMPONENTS./    ANLYZ376
     1       6X,45HPROBABILITIES THAT OTHER SOLUTIONS ARE WORSE:/)      ANLYZ377
 5000 FORMAT(5(5X,A1,4HPNG(,I2,1H/,I2,3H) =,F6.3))                      ANLYZ378
 6000 FORMAT(////14X,35HPARAMETERS USED TO GENERATE WEIGHTS)            ANLYZ379
 6200 FORMAT(1H )                                                       ANLYZ380
 7000 FORMAT(//19X,5HALPHA,19X,6HLAMBDA/)                               ANLYZ381
 7200 FORMAT(1PE25.4,1PE25.4)                                           ANLYZ382
 7500 FORMAT(1PE25.4)                                                   ANLYZ383
 8000 FORMAT(//14X,5HGAMMA,15X,5HALPHA,15X,6HLAMBDA/)                   ANLYZ384
 8200 FORMAT(1PE20.4,1PE20.4,1PE20.4)                                   ANLYZ385
 8400 FORMAT(1PE20.4,1PE20.4)                                           ANLYZ386
 8600 FORMAT(1PE20.4)                                                   ANLYZ387
 8800 FORMAT(1PE40.4,1PE20.4)                                           ANLYZ388
 9000 FORMAT(1PE40.4)                                                   ANLYZ389
      END                                                               ANLYZ390
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++BCEFF001
      SUBROUTINE BCOEFF (NYMAX,T,NY,K,ISET,NZMAX,D,U,ZGRID,BETA,        BCEFF002
     1                   W,DZ,RESPON)                                   BCEFF003
C-----------------------------------------------------------------------BCEFF004
C     EVALUTES B-SPLINE COEFICIENTS BETA.                               BCEFF005
C-----------------------------------------------------------------------BCEFF006
C                                                                       BCEFF007
C     DESCRIPTION OF PARAMETERS:                                        BCEFF008
C                                                                       BCEFF009
C           NZMAX  - DIMENSION OF ARRAYS D, U, ZGRID, AND BETA AS       BCEFF010
C                    DEFINED IN MAIN PROGRAM                            BCEFF011
C           D      - REAL ARRAY OF DIMENSION NZMAX, CONTAINS NZ DIAGONALBCEFF012
C                    ELEMENTS OF DECOMPOSED MATRIX                      BCEFF013
C           U      - REAL ARRAY OF DIMENSION NZMAX, CONTAINS NZ-1 UPPER BCEFF014
C                    DIAGONAL ELEMENTS OF DECOMPOSED MATRIX             BCEFF015
C           ZGRID  - REAL ARRAY OF DIMENSION NZMAX, WITH EQUALLY SPACED BCEFF016
C                    KNOTS FOR INTERPOLATION                            BCEFF017
C           BETA   - REAL ARRAY OF DIMENSION NZMAX, CONTAINS ON OUTPUT  BCEFF018
C                    THE SOLUTION, I.E. THE B-SPLINE COEFFICIENTS.      BCEFF019
C           W      - REAL ARRAY OF DIMENSION AT LEAST NZMAX, USED AS    BCEFF020
C                    WORKING SPACE                                      BCEFF021
C           NZ     - ACTUAL DIMENSION OF D, U, ZGRID, BETA, AND W       BCEFF022
C           NB     - NUMBER OF B-SPLINE COEFFICIENTS                    BCEFF023
C           DZ     - DISTANCE BETWEEN POINTS ON INTERPOLATION GRID      BCEFF024
C                    ZGRID                                              BCEFF025
C                                                                       BCEFF026
C-----------------------------------------------------------------------BCEFF027
C     SUBPROGRAMS CALLED - CHOSOL, USERFN                               BCEFF028
C     WHICH IN TURN CALL - ERRMES, USERTR                               BCEFF029
C     COMMON USED        - IBLOCK                                       BCEFF030
C-----------------------------------------------------------------------BCEFF031
      DOUBLE PRECISION D, U, ZGRID, BETA, W, DZ, DF1, DFNZ              BCEFF032
      DIMENSION        T(NYMAX,1), NY(*), D(NZMAX), U(NZMAX),           BCEFF033
     1                 ZGRID(NZMAX), BETA(NZMAX), W(NZMAX), RESPON(*)   BCEFF034
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       BCEFF035
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       BCEFF036
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   BCEFF037
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    BCEFF038
     4                 MTRY(10,2), MXITER(2), NL(10),                   BCEFF039
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  BCEFF040
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, BCEFF041
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       BCEFF042
     8                 NIN, NOUT                                        BCEFF043
      DATA             TWO/2.E0/, THREE/3.E0/, SIX/6.E0/                BCEFF044
C-----------------------------------------------------------------------BCEFF045
C  SET UP RIGHT HAND SIDE                                               BCEFF046
C-----------------------------------------------------------------------BCEFF047
      NZM1    =NZ-1                                                     BCEFF048
      NONL    =NZ                                                       BCEFF049
      DUM     =ZGRID(1)                                                 BCEFF050
      BETA(1) =THREE * USERFN(DUM,1,NYMAX,T,NY,K,ISET,0,RESPON)         BCEFF051
      NONL    =2                                                        BCEFF052
      DF1     =DZ * USERFN(DUM,1,NYMAX,T,NY,K,ISET,1,RESPON)            BCEFF053
      NONL    =NZ                                                       BCEFF054
      DUM     =ZGRID(NZ)                                                BCEFF055
      BETA(NZ)=THREE * USERFN(DUM,2,NYMAX,T,NY,K,ISET,0,RESPON)         BCEFF056
      NONL    =2                                                        BCEFF057
      DFNZ    =DZ * USERFN(DUM,2,NYMAX,T,NY,K,ISET,1,RESPON)            BCEFF058
      NONL    =NZ                                                       BCEFF059
      BETA(1) =BETA(1) + DF1                                            BCEFF060
      DO 100 I=2,NZM1                                                   BCEFF061
         IP1    =I+1                                                    BCEFF062
         DUM    =ZGRID(I)                                               BCEFF063
         BETA(I)=SIX * USERFN(DUM,IP1,NYMAX,T,NY,K,ISET,0,RESPON)       BCEFF064
 100  CONTINUE                                                          BCEFF065
      BETA(NZ)=BETA(NZ) - DFNZ                                          BCEFF066
C-----------------------------------------------------------------------BCEFF067
C  SOLVE FOR B-SPLINE COEFFICIENTS BETA(I), I=2,...,NB-1                BCEFF068
C-----------------------------------------------------------------------BCEFF069
      CALL CHOSOL (NZMAX,D,U,BETA,W,NZ)                                 BCEFF070
C-----------------------------------------------------------------------BCEFF071
C  ADDITIONAL EVALUATIONS OF BETA(1) AND BETA(NB)                       BCEFF072
C-----------------------------------------------------------------------BCEFF073
      BETA(1) =BETA(3)   -TWO*DF1                                       BCEFF074
      BETA(NB)=BETA(NB-2)+TWO*DFNZ                                      BCEFF075
      RETURN                                                            BCEFF076
      END                                                               BCEFF077
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++BEAIN001
      FUNCTION BETAIN (X,A,B,NOUT)                                      BEAIN002
C-----------------------------------------------------------------------BEAIN003
C     APPROXIMATES THE INCOMPLETE BETA FUNCTION RATIO I(SUB X)(A,B)     BEAIN004
C     USING ABRAMOWITZ AND STEGUN, EQS. 26.5.5.                         BEAIN005
C     GOOD ABOUT 1 PART IN TOL (TOL SET IN DATA STATEMENT BELOW).       BEAIN006
C     FOR VERY LARGE ( > 1.E+4 ) A OR B, OF THE ORDER OF MAX(A,B)       BEAIN007
C     TERMS ARE NEEDED, AND AN ASYMPTOTIC FORMULA WOULD BE BETTER.      BEAIN008
C     TAKES AN ERROR EXIT IF A OR B .GE. 2.E+4.                         BEAIN009
C-----------------------------------------------------------------------BEAIN010
C     SUBPROGRAMS CALLED - ERRMES, GAMLN                                BEAIN011
C     WHICH IN TURN CALL - NONE                                         BEAIN012
C     COMMON USED        - NONE                                         BEAIN013
C-----------------------------------------------------------------------BEAIN014
      LOGICAL          SWAP                                             BEAIN015
      DIMENSION        IHOLER(6)                                        BEAIN016
      DATA             IHOLER/1HB, 1HE, 1HT, 1HA, 1HI, 1HN/, TOL/1.E-5/ BEAIN017
C-----------------------------------------------------------------------BEAIN018
      IF (X.LT.0. .OR. X.GT.1. .OR. AMIN1(A,B).LE.0. .OR.               BEAIN019
     1          AMAX1(A,B).GE.2.E+4) CALL ERRMES (1,.TRUE.,IHOLER,NOUT) BEAIN020
      BETAIN=X                                                          BEAIN021
      IF (X.LE.0. .OR. X.GE.1.) RETURN                                  BEAIN022
      SWAP=X .GT. .5                                                    BEAIN023
      IF (SWAP) GO TO 150                                               BEAIN024
         XX=X                                                           BEAIN025
         AA=A                                                           BEAIN026
         BB=B                                                           BEAIN027
         GO TO 200                                                      BEAIN028
C-----------------------------------------------------------------------BEAIN029
C  WHEN SWAP=.TRUE., I(SUB 1-X)(B,A)=1-I(SUB X)(A,B) IS EVALUATED       BEAIN030
C  FIRST.                                                               BEAIN031
C-----------------------------------------------------------------------BEAIN032
 150  XX=1.-X                                                           BEAIN033
      AA=B                                                              BEAIN034
      BB=A                                                              BEAIN035
 200  CX=1.-XX                                                          BEAIN036
      R =XX/CX                                                          BEAIN037
C-----------------------------------------------------------------------BEAIN038
C  TERM IMAX IS APPROXIMATELY THE MAXIMUM TERM IN THE SUM.              BEAIN039
C-----------------------------------------------------------------------BEAIN040
      IMAX  =MAX0(0,INT((R*BB-AA-1.)/(R+1.)))                           BEAIN041
      RI    =FLOAT(IMAX)                                                BEAIN042
      SUM   =0.                                                         BEAIN043
      TERMAX=(AA+RI)*ALOG(XX)+(BB-RI-1.)*ALOG(CX)+GAMLN(AA+BB)-         BEAIN044
     1                            GAMLN(AA+RI+1.)-GAMLN(BB-RI)          BEAIN045
      IF (TERMAX .LT. -50.) GO TO 700                                   BEAIN046
         TERMAX=EXP(TERMAX)                                             BEAIN047
         TERM  =TERMAX                                                  BEAIN048
         SUM   =TERM                                                    BEAIN049
C-----------------------------------------------------------------------BEAIN050
C  SUM TERMS FOR I=IMAX+1,IMAX+2,... UNTIL CONVERGENCE.                 BEAIN051
C-----------------------------------------------------------------------BEAIN052
         I1=IMAX+1                                                      BEAIN053
         DO 250 I=I1,20000                                              BEAIN054
            RI  =FLOAT(I)                                               BEAIN055
            TERM=TERM*R*(BB-RI)/(AA+RI)                                 BEAIN056
            SUM =SUM+TERM                                               BEAIN057
            IF (ABS(TERM) .LE. TOL*SUM) GO TO 300                       BEAIN058
 250     CONTINUE                                                       BEAIN059
         CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                             BEAIN060
 300     IF (IMAX .EQ. 0) GO TO 700                                     BEAIN061
C-----------------------------------------------------------------------BEAIN062
C  SUM TERMS FOR I=IMAX-1,IMAX-2,... UNTIL CONVERGENCE.                 BEAIN063
C-----------------------------------------------------------------------BEAIN064
         TERM=TERMAX                                                    BEAIN065
         RI  =FLOAT(IMAX)                                               BEAIN066
         DO 320 I=1,IMAX                                                BEAIN067
            TERM=TERM*(AA+RI)/(R*(BB-RI))                               BEAIN068
            SUM =SUM+TERM                                               BEAIN069
            IF (ABS(TERM) .LE. TOL*SUM) GO TO 700                       BEAIN070
            RI=RI-1.                                                    BEAIN071
 320     CONTINUE                                                       BEAIN072
 700  BETAIN=SUM                                                        BEAIN073
      IF (SWAP) BETAIN=1.-BETAIN                                        BEAIN074
      RETURN                                                            BEAIN075
      END                                                               BEAIN076
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++BSLIN001
      SUBROUTINE BSPLIN (BSPL,DX,IDERIV)                                BSLIN002
C-----------------------------------------------------------------------BSLIN003
C     EVALUATES 4 PARTS OF NORMALIZED CUBIC B-SPLINE (IDERIV=0) AND     BSLIN004
C     ITS 1ST AND 2ND DERIVATIVES (IDERIV=1,2).                         BSLIN005
C     UNIFORMLY SPACED KNOTS ASSUMED.                                   BSLIN006
C-----------------------------------------------------------------------BSLIN007
C                                                                       BSLIN008
C     DESCRIPTION OF PARAMETERS:                                        BSLIN009
C                                                                       BSLIN010
C           BSPL   - REAL ARRAY OF DIMENSION 4, CONTAINS ON OUTPUT      BSLIN011
C                    THE 4 PARTS OF A NORMALIZED CUBIC B-SPLINE, WHICH  BSLIN012
C                    IS DEFINED ON INTERVALL [-2,2]                     BSLIN013
C           DX     - NORMALIZED DISTANCE TO NEXT GRID POINT,            BSLIN014
C                    I.E.  0 < DX < 1                                   BSLIN015
C           IDERIV - FLAG IF B-SPLINES (IDERIV=0) OR DERIVATIVES        BSLIN016
C                    (IDERIV=1,2) ARE REQUIRED                          BSLIN017
C                                                                       BSLIN018
C-----------------------------------------------------------------------BSLIN019
C     SUBPROGRAMS CALLED - NONE                                         BSLIN020
C     COMMON USED        - DBLOCK, SBLOCK                               BSLIN021
C-----------------------------------------------------------------------BSLIN022
      DOUBLE PRECISION PRECIS, RANGE,                                   BSLIN023
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       BSLIN024
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           BSLIN025
     3                 TWOTHR, FORTHR, SIXTEL                           BSLIN026
      DOUBLE PRECISION BSPL, DX, X, DZINV2, ONEHLF, THRHLF              BSLIN027
      DIMENSION        BSPL(*)                                          BSLIN028
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        BSLIN029
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, BSLIN030
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 BSLIN031
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),BSLIN032
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     BSLIN033
C     DATA             ONEHLF/5.E-1/, THRHLF/1.5E0/                     BSLIN034
      DATA             ONEHLF/5.D-1/, THRHLF/1.5D0/                     BSLIN035
C-----------------------------------------------------------------------BSLIN036
      X=ONE+DX                                                          BSLIN037
      IF (IDERIV-1) 100,200,300                                         BSLIN038
C-----------------------------------------------------------------------BSLIN039
C  CALCULATE B-SPLINE                                                   BSLIN040
C-----------------------------------------------------------------------BSLIN041
 100  BSPL(1)=((-SIXTEL*X+ONE)*X  -TWO)*X+FORTHR                        BSLIN042
                                                 X=X-ONE                BSLIN043
      BSPL(2)=(  ONEHLF*X-ONE)*X**2      +TWOTHR                        BSLIN044
                                                 X=X-ONE                BSLIN045
      BSPL(3)=( -ONEHLF*X-ONE)*X**2      +TWOTHR                        BSLIN046
                                                 X=X-ONE                BSLIN047
      BSPL(4)=(( SIXTEL*X+ONE)*X  +TWO)*X+FORTHR                        BSLIN048
      RETURN                                                            BSLIN049
C-----------------------------------------------------------------------BSLIN050
C  1ST DERIVATIVE                                                       BSLIN051
C-----------------------------------------------------------------------BSLIN052
 200  BSPL(1)=(-ONEHLF*X+TWO)*X-TWO                                     BSLIN053
                                     X=X-ONE                            BSLIN054
      BSPL(2)=( THRHLF*X-TWO)*X                                         BSLIN055
                                     X=X-ONE                            BSLIN056
      BSPL(3)=(-THRHLF*X-TWO)*X                                         BSLIN057
                                     X=X-ONE                            BSLIN058
      BSPL(4)=( ONEHLF*X+TWO)*X+TWO                                     BSLIN059
C-----------------------------------------------------------------------BSLIN060
C  DO PROPER SCALING                                                    BSLIN061
C-----------------------------------------------------------------------BSLIN062
      DO 250 I=1,4                                                      BSLIN063
 250  BSPL(I)=DZINV*BSPL(I)                                             BSLIN064
      RETURN                                                            BSLIN065
C-----------------------------------------------------------------------BSLIN066
C  2ND DERIVATIVE                                                       BSLIN067
C-----------------------------------------------------------------------BSLIN068
 300  BSPL(1)=      -X+TWO                                              BSLIN069
                           X=X-ONE                                      BSLIN070
      BSPL(2)= THREE*X-TWO                                              BSLIN071
                           X=X-ONE                                      BSLIN072
      BSPL(3)=-THREE*X-TWO                                              BSLIN073
                           X=X-ONE                                      BSLIN074
      BSPL(4)=       X+TWO                                              BSLIN075
C-----------------------------------------------------------------------BSLIN076
C  SCALING                                                              BSLIN077
C-----------------------------------------------------------------------BSLIN078
      DZINV2=DZINV*DZINV                                                BSLIN079
      DO 350 I=1,4                                                      BSLIN080
 350  BSPL(I)=DZINV2*BSPL(I)                                            BSLIN081
      RETURN                                                            BSLIN082
      END                                                               BSLIN083
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++CHSOL001
      SUBROUTINE CHOSOL (NZMAX,D,U,BETA,W,NZ)                           CHSOL002
C-----------------------------------------------------------------------CHSOL003
C     EVALAUTES SOLUTION OF A SYSTEM OF LINEAR EQUATIONS, GIVEN THE     CHSOL004
C     THE DIAGONAL AND UPPER DIAGONAL ELEMENTS OF A TRIDIAGONAL MATRIX  CHSOL005
C     WHICH IS DECOMPOSED VIA THE SUBPROGRAM CHOTRD.                    CHSOL006
C-----------------------------------------------------------------------CHSOL007
C                                                                       CHSOL008
C     DESCRIPTION OF PARAMETERS:                                        CHSOL009
C                                                                       CHSOL010
C           NZMAX  - DIMENSION OF ARRAYS D, U, BETA, AND W AS           CHSOL011
C                    DEFINED IN MAIN PROGRAM                            CHSOL012
C           D      - REAL ARRAY OF DIMENSION NZMAX, CONTAINS ON INPUT   CHSOL013
C                    NZ DIAGONAL ELEMENTS OF DECOMPOSED MATRIX          CHSOL014
C           U      - REAL ARRAY OF DIMENSION NZMAX, CONTAINS ON INPUT   CHSOL015
C                    NZ-1 UPPER DIAGONAL ELEMENTS OF DECOMPOSED MATRIX  CHSOL016
C           BETA   - REAL ARRAY OF DIMENSION NZMAX, CONTAINS ON INPUT   CHSOL017
C                    THE RIGHT HAND SIDE VECTOR, AND ON OUTPUT          CHSOL018
C                    THE SOLUTION, I.E. THE B-SPLINE COEFFICIENTS       CHSOL019
C                    BETA(I), I=2,...,NB-1                              CHSOL020
C           W      - REAL ARRAY OF DIMENSION AT LEAST NZMAX, USED AS    CHSOL021
C                    WORKING SPACE                                      CHSOL022
C           NZ     - ACTUAL DIMENSION OF D, U, BETA, AND W              CHSOL023
C                                                                       CHSOL024
C-----------------------------------------------------------------------CHSOL025
C     SUBPROGRAMS CALLED - NONE                                         CHSOL026
C     COMMON USED        - NONE                                         CHSOL027
C-----------------------------------------------------------------------CHSOL028
      DOUBLE PRECISION D, U, BETA, W                                    CHSOL029
      DIMENSION        D(NZMAX), U(NZMAX), BETA(NZMAX), W(NZMAX)        CHSOL030
C-----------------------------------------------------------------------CHSOL031
C  FORWARD SOLUTION                                                     CHSOL032
C-----------------------------------------------------------------------CHSOL033
      NZM1=NZ-1                                                         CHSOL034
      W(1)=BETA(1)/D(1)                                                 CHSOL035
      DO 100 I=2,NZ                                                     CHSOL036
         I1  =I-1                                                       CHSOL037
         W(I)=(BETA(I)-U(I1)*W(I1))/D(I)                                CHSOL038
 100  CONTINUE                                                          CHSOL039
C-----------------------------------------------------------------------CHSOL040
C  BACKWARD SOLUTION                                                    CHSOL041
C-----------------------------------------------------------------------CHSOL042
      BETA(NZ+1)=W(NZ)/D(NZ)                                            CHSOL043
      DO 200 J=1,NZM1                                                   CHSOL044
         I        =NZ-J                                                 CHSOL045
         BETA(I+1)=(W(I)-U(I)*BETA(I+2))/D(I)                           CHSOL046
 200  CONTINUE                                                          CHSOL047
      RETURN                                                            CHSOL048
      END                                                               CHSOL049
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++CHTRD001
      SUBROUTINE CHOTRD (NZMAX,D,U,NZ,ONE,TWO,FOUR)                     CHTRD002
C-----------------------------------------------------------------------CHTRD003
C     DECOMPOSES THE FOLLOWING TRIDIAGONAL MATRIX                       CHTRD004
C                                                                       CHTRD005
C           ( 2  1                         )                            CHTRD006
C           ( 1  4  1                      )                            CHTRD007
C           (    1  4  1         0         )                            CHTRD008
C           (          .  .  .             )                            CHTRD009
C           (             .  .  .          )                            CHTRD010
C           (                .  .  .       )                            CHTRD011
C           (          0        1  4  1    )                            CHTRD012
C           (                      1  4  1 )                            CHTRD013
C           (                         1  2 )                            CHTRD014
C                                                                       CHTRD015
C     FOR EVALUATING OF B-SPLINE COEFFICIENTS.                          CHTRD016
C     METHOD USED: CHOLESKY FACTORIZATION.                              CHTRD017
C-----------------------------------------------------------------------CHTRD018
C                                                                       CHTRD019
C     DESCRIPTION OF PARAMETERS:                                        CHTRD020
C                                                                       CHTRD021
C           NZMAX  - DIMENSION OF ARRAYS D AND U, AS DEFINED IN         CHTRD022
C                    MAIN PROGRAM                                       CHTRD023
C           D      - REAL ARRAY OF DIMENSION NZMAX, CONTAINS ON OUTPUT  CHTRD024
C                    NZ DIAGONAL ELEMENTS OF DECOMPOSED MATRIX          CHTRD025
C           U      - REAL ARRAY OF DIMENSION NZMAX, CONTAINS ON OUTPUT  CHTRD026
C                    NZ-1 UPPER DIAGONAL ELEMENTS OF DECOMPOSED MATRIX  CHTRD027
C           NZ     - ACTUAL DIMENSION OF D AND U                        CHTRD028
C                                                                       CHTRD029
C-----------------------------------------------------------------------CHTRD030
C     SUBPROGRAMS CALLED - NONE                                         CHTRD031
C     COMMON USED        - NONE                                         CHTRD032
C-----------------------------------------------------------------------CHTRD033
      DOUBLE PRECISION D, U, ONE, TWO, FOUR, SQRT                       CHTRD034
      DIMENSION        D(NZMAX), U(NZMAX)                               CHTRD035
C-----------------------------------------------------------------------CHTRD036
      SQRT(ONE)=DSQRT(ONE)                                              CHTRD037
C-----------------------------------------------------------------------CHTRD038
      NZM1=NZ-1                                                         CHTRD039
      D(1)=SQRT(TWO)                                                    CHTRD040
      DO 100 I=2,NZM1                                                   CHTRD041
         IM1   =I-1                                                     CHTRD042
         U(IM1)=ONE/D(IM1)                                              CHTRD043
         D(I)  =SQRT(FOUR-U(IM1)**2)                                    CHTRD044
 100  CONTINUE                                                          CHTRD045
      U(NZM1)=ONE/D(NZM1)                                               CHTRD046
      D(NZ)  =SQRT(TWO-U(NZM1)**2)                                      CHTRD047
      RETURN                                                            CHTRD048
      END                                                               CHTRD049
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++DIFF0001
C     REAL FUNCTION DIFF (X,Y)                                          DIFF0002
      DOUBLE PRECISION FUNCTION DIFF (X,Y)                              DIFF0003
C-----------------------------------------------------------------------DIFF0004
C     BASED ON C.L.LAWSON AND R.J.HANSON,                               DIFF0005
C     'SOLVING LEAST SQUARES PROBLEMS', PRENTICE-HALL, 1974             DIFF0006
C-----------------------------------------------------------------------DIFF0007
C     SUBPROGRAMS CALLED - NONE                                         DIFF0008
C     COMMON USED        - NONE                                         DIFF0009
C-----------------------------------------------------------------------DIFF0010
      DOUBLE PRECISION X, Y                                             DIFF0011
      DIFF=X-Y                                                          DIFF0012
      RETURN                                                            DIFF0013
      END                                                               DIFF0014
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++ERMES001
      SUBROUTINE ERRMES (IERR,ABORT,IHOLER,NOUT)                        ERMES002
C-----------------------------------------------------------------------ERMES003
C     PRINTS OUT ERROR NUMBER IERR AND NAME OF SUBROUTINE WHERE         ERMES004
C     ERROR OCCURED.                                                    ERMES005
C-----------------------------------------------------------------------ERMES006
C                                                                       ERMES007
C     DESCRIPTION OF PARAMETERS:                                        ERMES008
C                                                                       ERMES009
C           IERR   - INTEGER, CONTAINS ERROR NUMBER AS DEFINED IN       ERMES010
C                    CALLING SUBROUTINE                                 ERMES011
C           ABORT  - LOGICAL, IF ABORT=.TRUE. STOP EXECUTION OTHERWISE  ERMES012
C                    PRINT ERROR MESSAGE ONLY                           ERMES013
C           IHOLER - INTEGER ARRAY OF DIMENSION 6, CONTAINS NAME OF     ERMES014
C                    CALLING SUBROUTINE                                 ERMES015
C           NOUT   - INTEGER, OUTPUT FILE NUMBER                        ERMES016
C                                                                       ERMES017
C-----------------------------------------------------------------------ERMES018
C     SUBPROGRAMS CALLED - NONE                                         ERMES019
C     COMMON USED        - NONE                                         ERMES020
C-----------------------------------------------------------------------ERMES021
      LOGICAL          ABORT                                            ERMES022
      DIMENSION        IHOLER(6)                                        ERMES023
C-----------------------------------------------------------------------ERMES024
      WRITE (NOUT,1000) IHOLER,IERR                                     ERMES025
      IF (ABORT) STOP                                                   ERMES026
      RETURN                                                            ERMES027
 1000 FORMAT(7H0ERROR ,6A1,I2,25H.  (CHECK USERS GUIDE.)  ,46(2H**))    ERMES028
      END                                                               ERMES029
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++FIHNI001
      FUNCTION FISHNI (F,DF1,DF2,NOUT)                                  FIHNI002
C-----------------------------------------------------------------------FIHNI003
C     CALCULATES FISHER F-DISTRIBUTION FOR ARGUMENT F WITH DF1 AND DF2  FIHNI004
C     (NOT NECESSARILY INTEGER) DEGREES OF FREEDOM. (SEE ABRAMOWITZ AND FIHNI005
C     STEGUN, EQS. 26.6.2 AND 26.5.2)                                   FIHNI006
C-----------------------------------------------------------------------FIHNI007
C     SUBPROGRAMS CALLED - ERRMES, BETAIN                               FIHNI008
C     WHICH IN TURN CALL - ERRMES, GAMLN                                FIHNI009
C     COMMON USED        - NONE                                         FIHNI010
C-----------------------------------------------------------------------FIHNI011
      DIMENSION        IHOLER(6)                                        FIHNI012
      DATA             IHOLER/1HF, 1HI, 1HS, 1HH, 1HN, 1HI/             FIHNI013
C-----------------------------------------------------------------------FIHNI014
      IF (AMIN1(DF1,DF2) .GT. 0.) GO TO 150                             FIHNI015
         CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                            FIHNI016
         FISHNI=1.                                                      FIHNI017
         RETURN                                                         FIHNI018
 150  HDF1  =.5*DF1                                                     FIHNI019
      HDF2  =.5*DF2                                                     FIHNI020
      DUM   =DF1*F                                                      FIHNI021
      FISHNI=BETAIN(DUM/(DF2+DUM),HDF1,HDF2,NOUT)                       FIHNI022
      RETURN                                                            FIHNI023
      END                                                               FIHNI024
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++FULSM001
      SUBROUTINE FULLSM (NYMAX,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,    FULSM002
     1                   PLAM,PIVLIN,EK,EPK,ETE,R,DELTAL,DELTAN,GTG,H,  FULSM003
     2                   DELTAH,EKEK,ASAVE,NLNDMX,ATOT,DTOLER,DX,BSPL0, FULSM004
     3                   BSPL1,BSPL2,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,    FULSM005
     4                   NLND,ND,NLIN,NONL,SPLINE,PRECIS,ZERO,TEN,      FULSM006
     5                   RESPON)                                        FULSM007
C-----------------------------------------------------------------------FULSM008
C     BUILD UP FULL LEAST SQUARES MATRIX ATOT(I,J), I,J=1,...,NLNDNL,   FULSM009
C     TO GET ERROR ESTIMATES OF PARAMETERS AND CORRELATION COEFFICIENTS.FULSM010
C-----------------------------------------------------------------------FULSM011
C     SUBPROGRAMS CALLED - HESEXA, HESSPL                               FULSM012
C     WHICH IN TURN CALL - BSPLIN, ERRMES, HESSEN, USERFL, USERFN,      FULSM013
C                          USERTR                                       FULSM014
C     COMMON USED        - NONE                                         FULSM015
C-----------------------------------------------------------------------FULSM016
      DOUBLE PRECISION EK, EPK, ETE, R, DELTAL, DELTAN, GTG, H,         FULSM017
     1                 DELTAH, EKEK, ASAVE, ATOT, DTOLER, DX, BSPL0,    FULSM018
     2                 BSPL1, BSPL2, CCAP, ECAP, YCAP, PRECIS, ZERO,    FULSM019
     3                 TEN                                              FULSM020
      LOGICAL          PIVLIN, SPLINE                                   FULSM021
      DIMENSION        T(NYMAX,1), YLYFIT(NYMAX,1), SQRTW(NYMAX,1),     FULSM022
     1                 NY(*), PLIN(NLINMX,1), PLAM(NONLMX),             FULSM023
     2                 PIVLIN(NLINMX,1), EK(NLINMX), EPK(NONLMX),       FULSM024
     3                 ETE(NLINMX,NLINMX), R(NLINMX,1), DELTAL(NLINMX), FULSM025
     4                 DELTAN(NONLMX), GTG(NONLMX,NONLMX),              FULSM026
     5                 H(NLINMX,NLINMX,1)                               FULSM027
      DIMENSION        DELTAH(NONLMX,1), EKEK(NLINMX,NLINMX,1),         FULSM028
     1                 ASAVE(NLINMX,NLINMX,1), ATOT(NLNDMX,NLNDMX),     FULSM029
     2                 DTOLER(NLNDMX), DX(NONLMX), BSPL0(4,NONLMX),     FULSM030
     3                 BSPL1(4,NONLMX), BSPL2(4,NONLMX), IC(NONLMX),    FULSM031
     4                 CCAP(NZMAX,NZMAX,1), ECAP(NZMAX,NGAMMX,1),       FULSM032
     5                 YCAP(NZMAX,1), RESPON(*)                         FULSM033
C-----------------------------------------------------------------------FULSM034
      DO 100 I=1,NLND                                                   FULSM035
      DO 100 J=I,NLND                                                   FULSM036
 100  ATOT(I,J)=ZERO                                                    FULSM037
      DO 150 I=1,NLIN                                                   FULSM038
      DO 150 J=I,NLIN                                                   FULSM039
 150  ATOT(I,J)=EKEK(I,J,1)                                             FULSM040
      IF (ND .LE. 1) GO TO 300                                          FULSM041
         DO 200 N=2,ND                                                  FULSM042
         JR=(N-1)*NLIN                                                  FULSM043
         DO 200 I=1,NLIN                                                FULSM044
         JC=JR                                                          FULSM045
         JR=JR+1                                                        FULSM046
         DO 200 J=I,NLIN                                                FULSM047
            JC=JC+1                                                     FULSM048
            ATOT(JR,JC)=EKEK(I,J,N)                                     FULSM049
 200     CONTINUE                                                       FULSM050
 300  IF (.NOT. SPLINE) CALL HESEXA (                                   FULSM051
     1            NYMAX,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,PLAM,EK,   FULSM052
     2            EPK,DELTAL,GTG,H,DELTAH,DELTAN,ETE,ASAVE,EKEK,        FULSM053
     3            DTOLER,R,PIVLIN,.TRUE.,ZERO,RESPON)                   FULSM054
      IF (SPLINE) CALL HESSPL (                                         FULSM055
     1            NLINMX,NONLMX,PLIN,GTG,H,DELTAH,DELTAN,ETE,ASAVE,     FULSM056
     2            EKEK,DTOLER,R,PIVLIN,BSPL0,BSPL1,BSPL2,DX,IC,NZMAX,   FULSM057
     3            NGAMMX,CCAP,ECAP,YCAP,.TRUE.,ZERO)                    FULSM058
      JR=NLND                                                           FULSM059
      DO 400 I=1,NONL                                                   FULSM060
      JC=JR                                                             FULSM061
      JR=JR+1                                                           FULSM062
      DO 400 J=I,NONL                                                   FULSM063
         JC=JC+1                                                        FULSM064
         ATOT(JR,JC)=GTG(I,J)                                           FULSM065
 400  CONTINUE                                                          FULSM066
      JC=NLND                                                           FULSM067
      DO 500 J=1,NONL                                                   FULSM068
      JC=JC+1                                                           FULSM069
      JR=0                                                              FULSM070
      DO 500 N=1,ND                                                     FULSM071
      DO 500 I=1,NLIN                                                   FULSM072
         JR         =JR+1                                               FULSM073
         ATOT(JR,JC)=H(I,J,N)                                           FULSM074
 500  CONTINUE                                                          FULSM075
C-----------------------------------------------------------------------FULSM076
C  SET TOLERANCE VALUES FOR PIVOT ELEMENTS                              FULSM077
C-----------------------------------------------------------------------FULSM078
      J=NLND                                                            FULSM079
      DO 600 I=1,NONL                                                   FULSM080
         J        =J+1                                                  FULSM081
         DTOLER(J)=PRECIS*DTOLER(I)                                     FULSM082
 600  CONTINUE                                                          FULSM083
      DO 650 I=1,NLND                                                   FULSM084
 650  DTOLER(I)=TEN*PRECIS*ATOT(I,I)                                    FULSM085
      RETURN                                                            FULSM086
      END                                                               FULSM087
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++GAMLN001
      FUNCTION GAMLN (XARG)                                             GAMLN002
C-----------------------------------------------------------------------GAMLN003
C     COMPUTES LN OF THE GAMMA FUNCTION FOR POSITIVE XARG USING A       GAMLN004
C     VERSION OF CACM ALGORITHM NO. 291.                                GAMLN005
C-----------------------------------------------------------------------GAMLN006
C     SUBPROGRAMS CALLED - NONE                                         GAMLN007
C     COMMON USED        - NONE                                         GAMLN008
C-----------------------------------------------------------------------GAMLN009
      GAMLN=0.                                                          GAMLN010
      IF (XARG .LE. 0.) RETURN                                          GAMLN011
      X=XARG                                                            GAMLN012
      P=1.                                                              GAMLN013
 110  IF (X .GE. 7.) GO TO 150                                          GAMLN014
         P=P*X                                                          GAMLN015
         X=X+1.                                                         GAMLN016
         GO TO 110                                                      GAMLN017
 150  IF (XARG .LT. 7.) GAMLN=-ALOG(P)                                  GAMLN018
      Z    =1./X**2                                                     GAMLN019
      GAMLN=GAMLN+(X-.5)*ALOG(X)-X+.9189385-(((Z/1680.-                 GAMLN020
     1             7.936508E-4)*Z+2.777778E-3)*Z-8.333333E-2)/X         GAMLN021
      RETURN                                                            GAMLN022
      END                                                               GAMLN023
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++GEPRU001
      FUNCTION GETPRU (NYMAX,YLYFIT,NY)                                 GEPRU002
C-----------------------------------------------------------------------GEPRU003
C     DOES APPROXIMATE RUNS TEST ON RESIDUALS (COMPUTED USING SOLUTION  GEPRU004
C     PLMSAV AND PLNSAV). GETPRU IS THE APPROXIMATE PROBABILTY THAT A   GEPRU005
C     RANDOMLY CHOSEN ORDER OF SIGNS WOULD HAVE NO MORE RUNS THAN THE   GEPRU006
C     RESIDUALS. APPROXIMATION IS ONLY GOOD IF THERE ARE AT LEAST OF    GEPRU007
C     THE ORDER 10 POSITIVE AND 10 NEGATIVE RESIDUALS. THERFORE IF      GEPRU008
C     THERE ARE NOT AT LEAST 10 OF EACH SIGN, GETPRU IS SET TO -1.      GEPRU009
C-----------------------------------------------------------------------GEPRU010
C     SUBPROGRAMS CALLED - PGAUSS                                       GEPRU011
C     COMMON USED        - NONE                                         GEPRU012
C-----------------------------------------------------------------------GEPRU013
      DIMENSION YLYFIT(*)                                               GEPRU014
C-----------------------------------------------------------------------GEPRU015
      NRUN  =0                                                          GEPRU016
      YLYOLD=-YLYFIT(1)                                                 GEPRU017
      DDUM  =0.                                                         GEPRU018
      DO 100 K=1,NY                                                     GEPRU019
         DUM =YLYFIT(K)                                                 GEPRU020
         DDUM=DDUM+SIGN(1.,DUM)                                         GEPRU021
         IF (DUM*YLYOLD .LT. 0.) NRUN=NRUN+1                            GEPRU022
         YLYOLD=DUM                                                     GEPRU023
 100  CONTINUE                                                          GEPRU024
      RN1   =.5*(FLOAT(NY)+DDUM)                                        GEPRU025
      RN2   =RN1-DDUM                                                   GEPRU026
      DUM   =2.*RN1*RN2                                                 GEPRU027
      RMU   =DUM/(RN1+RN2)+1.                                           GEPRU028
      SIG   =SQRT(DUM*(DUM-RN1-RN2)/(RN1+RN2-1.))/(RN1+RN2)             GEPRU029
      GETPRU=-1.                                                        GEPRU030
      IF (AMIN1(RN1,RN2) .GT. 9.5)                                      GEPRU031
     1                    GETPRU=PGAUSS((FLOAT(NRUN)-RMU+.5)/SIG)       GEPRU032
      RETURN                                                            GEPRU033
      END                                                               GEPRU034
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++HEEXA001
      SUBROUTINE HESEXA (NYMAX,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,    HEEXA002
     1                   PLAM,EK,EPK,EPPK,GTG,H,DELTAH,DELTAD,HELP,     HEEXA003
     2                   ASAVE,EPKEPK,DTOLER,R,PIVLIN,ONLYAT,ZERO,      HEEXA004
     3                   RESPON)                                        HEEXA005
C-----------------------------------------------------------------------HEEXA006
C     BUILDS UP HESSIAN MATRIX FOR NONLINEAR LEAST SQUARES IN           HEEXA007
C     4 DIFFERENT WAYS (METHOD=1,2,3,4). (SEE USERS MANUAL).            HEEXA008
C     USING EXACT FUNCTIONS.                                            HEEXA009
C-----------------------------------------------------------------------HEEXA010
C     SUBPROGRAMS CALLED - HESSEN, USERFL, USERFN                       HEEXA011
C     WHICH IN TURN CALL - ERRMES, USERTR                               HEEXA012
C     COMMON USED        - IBLOCK, LBLOCK                               HEEXA013
C-----------------------------------------------------------------------HEEXA014
      DOUBLE PRECISION EK, EPK, EPPK, GTG, H, DELTAH, DELTAD, HELP,     HEEXA015
     1                 ASAVE, EPKEPK, DTOLER, R, ZERO, DUB, SWK,        HEEXA016
     2                 SWK2, YLYK, AMAX1, ABS                           HEEXA017
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  HEEXA018
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    HEEXA019
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  HEEXA020
     3                 SPLINE, WRTBET, WRTCAP                           HEEXA021
      LOGICAL          PIVLIN, ONLYAT, ONLYM4                           HEEXA022
      DIMENSION        T(NYMAX,1), SQRTW(NYMAX,1), YLYFIT(NYMAX,1),     HEEXA023
     1                 PLIN(NLINMX,1), PLAM(NONLMX), EK(NLINMX), NY(*), HEEXA024
     2                 EPK(NONLMX), EPPK(NONLMX), GTG(NONLMX,NONLMX),   HEEXA025
     3                 H(NLINMX,NLINMX,1), DELTAH(NONLMX,1),            HEEXA026
     4                 DELTAD(NONLMX), HELP(NLINMX,NLINMX),             HEEXA027
     5                 ASAVE(NLINMX,NLINMX,1), EPKEPK(NLINMX,NLINMX,1), HEEXA028
     6                 DTOLER(NONLMX), R(NONLMX), PIVLIN(NLINMX,1),     HEEXA029
     7                 RESPON(*)                                        HEEXA030
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       HEEXA031
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       HEEXA032
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   HEEXA033
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    HEEXA034
     4                 MTRY(10,2), MXITER(2), NL(10),                   HEEXA035
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  HEEXA036
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, HEEXA037
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       HEEXA038
     8                 NIN, NOUT                                        HEEXA039
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          HEEXA040
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         HEEXA041
     2                 DOUSOU(2), LUSER(30),                            HEEXA042
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  HEEXA043
     4                 SPLINE, WRTBET, WRTCAP                           HEEXA044
C-----------------------------------------------------------------------HEEXA045
      AMAX1(DUB,ZERO)=DMAX1(DUB,ZERO)                                   HEEXA046
      ABS(DUB)       =DABS(DUB)                                         HEEXA047
C-----------------------------------------------------------------------HEEXA048
      DO 120 N=1,ND                                                     HEEXA049
      DO 120 J=1,NONL                                                   HEEXA050
         DO 100 I=1,NLIN                                                HEEXA051
 100     H(I,J,N)=ZERO                                                  HEEXA052
         DO 110 I=1,NONL                                                HEEXA053
 110     EPKEPK(I,J,N)=ZERO                                             HEEXA054
         DELTAH(J,N)=ZERO                                               HEEXA055
 120  CONTINUE                                                          HEEXA056
      DO 150 J=1,NONL                                                   HEEXA057
         DO 140 I=1,NONL                                                HEEXA058
 140     GTG(I,J)=ZERO                                                  HEEXA059
         R(J)     =ZERO                                                 HEEXA060
         DELTAD(J)=ZERO                                                 HEEXA061
         DTOLER(J)=ZERO                                                 HEEXA062
 150  CONTINUE                                                          HEEXA063
      GTG(NONLP1,NONLP1)=ZERO                                           HEEXA064
      ONLYM4=.NOT.ONLYAT .OR. (ONLYAT .AND. METHOD.EQ.4)                HEEXA065
C-----------------------------------------------------------------------HEEXA066
C  CASE  ND = 1                                                         HEEXA067
C-----------------------------------------------------------------------HEEXA068
      MY=NY(1)                                                          HEEXA069
      DO 300 K=1,MY                                                     HEEXA070
         KK  =K                                                         HEEXA071
         SWK =SQRTW(K,1)                                                HEEXA072
         SWK2=SWK*SWK                                                   HEEXA073
         YLYK=YLYFIT(K,1)                                               HEEXA074
         DO 200 J=1,NONL                                                HEEXA075
            JG    =J+NGAM                                               HEEXA076
            JJ    =J                                                    HEEXA077
            EK(JG)=USERFN(PLAM(J),JJ,NYMAX,T,NY,KK,1,0,RESPON)          HEEXA078
            EPK(J)=USERFN(PLAM(J),JJ,NYMAX,T,NY,KK,1,1,RESPON)          HEEXA079
            IF (METHOD .LT. 4) GO TO 200                                HEEXA080
            EPPK(J)=USERFN(PLAM(J),JJ,NYMAX,T,NY,KK,1,2,RESPON)         HEEXA081
 200     CONTINUE                                                       HEEXA082
         IF (NGAM .EQ. 0) GO TO 210                                     HEEXA083
            DO 205 MU=1,NGAM                                            HEEXA084
               I    =MU                                                 HEEXA085
               EK(I)=USERFL(I,NYMAX,T,NY,KK,1,RESPON)                   HEEXA086
 205        CONTINUE                                                    HEEXA087
 210     DO 240 J=1,NONL                                                HEEXA088
            DO 220 I=1,NONL                                             HEEXA089
 220        EPKEPK(J,I,1)=EPKEPK(J,I,1)+SWK2*EPK(J)*EPK(I)              HEEXA090
            DO 230 I=1,NLIN                                             HEEXA091
 230        H(I,J,1)=H(I,J,1)+SWK2*EK(I)*EPK(J)                         HEEXA092
            IF (ONLYM4) DELTAH(J,1)=DELTAH(J,1)+YLYK*SWK*EPK(J)         HEEXA093
            IF (METHOD .LT. 4) GO TO 240                                HEEXA094
            JG=J+NGAM                                                   HEEXA095
            DELTAD(J)=DELTAD(J)+YLYK*SWK*EPPK(J)*PLIN(JG,1)             HEEXA096
 240     CONTINUE                                                       HEEXA097
         IF (NSTORD) GO TO 300                                          HEEXA098
C-----------------------------------------------------------------------HEEXA099
C  CASE  ND > 1 .AND.  SAMET=.TRUE.                                     HEEXA100
C-----------------------------------------------------------------------HEEXA101
         DO 280 N=2,ND                                                  HEEXA102
            SWK =SQRTW(K,N)                                             HEEXA103
            SWK2=SWK*SWK                                                HEEXA104
            YLYK=YLYFIT(K,N)                                            HEEXA105
            DO 280 J=1,NONL                                             HEEXA106
            JG=J+NGAM                                                   HEEXA107
            IF (ONLYM4) DELTAH(J,N)=DELTAH(J,N)+YLYK*SWK*EPK(J)         HEEXA108
            IF (METHOD .EQ. 4) DELTAD(J)=DELTAD(J)+                     HEEXA109
     1                                   YLYK*SWK*EPPK(J)*PLIN(JG,N)    HEEXA110
            IF (IWTIST) GO TO 280                                       HEEXA111
            DO 250 I=1,NONL                                             HEEXA112
 250        EPKEPK(I,J,N)=EPKEPK(I,J,N)+SWK2*EPK(I)*EPK(J)              HEEXA113
            DO 260 I=1,NLIN                                             HEEXA114
 260        H(I,J,N)=H(I,J,N)+SWK2*EK(I)*EPK(J)                         HEEXA115
 280     CONTINUE                                                       HEEXA116
 300  CONTINUE                                                          HEEXA117
      IF (NSTORD .OR. .NOT.IWTIST) GO TO 400                            HEEXA118
C-----------------------------------------------------------------------HEEXA119
C  CASE  ND > 1 .AND.  SAMET=.TRUE.  AND  IWTIST=.TRUE.                 HEEXA120
C-----------------------------------------------------------------------HEEXA121
         DO 330 N=2,ND                                                  HEEXA122
         DO 330 J=1,NONL                                                HEEXA123
            DO 310 I=1,NONL                                             HEEXA124
 310        EPKEPK(I,J,N)=EPKEPK(I,J,1)                                 HEEXA125
            DO 320 I=1,NLIN                                             HEEXA126
 320        H(I,J,N)=H(I,J,1)                                           HEEXA127
 330     CONTINUE                                                       HEEXA128
 400  CALL HESSEN (NLINMX,NONLMX,PLIN,ASAVE,GTG,H,DELTAH,HELP,EPKEPK,   HEEXA129
     1             DTOLER,R,PIVLIN,ONLYAT,ZERO)                         HEEXA130
      IF (ND .EQ. 1) GO TO 900                                          HEEXA131
      DO 800 N=2,ND                                                     HEEXA132
         IF (SAMET) GO TO 700                                           HEEXA133
C-----------------------------------------------------------------------HEEXA134
C  CASE  ND > 1  .AND.  SAMET=.FALSE.                                   HEEXA135
C-----------------------------------------------------------------------HEEXA136
         NSET=N                                                         HEEXA137
         MY  =NY(N)                                                     HEEXA138
         DO 550 K=1,MY                                                  HEEXA139
            KK  =K                                                      HEEXA140
            SWK =SQRTW(K,N)                                             HEEXA141
            SWK2=SWK*SWK                                                HEEXA142
            YLYK=YLYFIT(K,N)                                            HEEXA143
            DO 500 J=1,NONL                                             HEEXA144
               JG    =J+NGAM                                            HEEXA145
               JJ    =J                                                 HEEXA146
               EK(JG)=USERFN(PLAM(J),JJ,NYMAX,T,NY,KK,NSET,0,RESPON)    HEEXA147
               EPK(J)=USERFN(PLAM(J),JJ,NYMAX,T,NY,KK,NSET,1,RESPON)    HEEXA148
               IF (METHOD .LT. 4) GO TO 500                             HEEXA149
               EPPK(J)=USERFN(PLAM(J),JJ,NYMAX,T,NY,KK,NSET,2,RESPON)   HEEXA150
 500        CONTINUE                                                    HEEXA151
            IF (NGAM .EQ. 0) GO TO 510                                  HEEXA152
               DO 505 MU=1,NGAM                                         HEEXA153
                  I    =MU                                              HEEXA154
                  EK(I)=USERFL(I,NYMAX,T,NY,KK,NSET,RESPON)             HEEXA155
 505           CONTINUE                                                 HEEXA156
 510        DO 540 J=1,NONL                                             HEEXA157
               DO 520 I=1,NONL                                          HEEXA158
 520           EPKEPK(I,J,N)=EPKEPK(I,J,N)+SWK2*EPK(I)*EPK(J)           HEEXA159
               IF (ONLYM4) DELTAH(J,N)=DELTAH(J,N)+YLYK*SWK*EPK(J)      HEEXA160
               DO 530 I=1,NLIN                                          HEEXA161
 530           H(I,J,N)=H(I,J,N)+SWK2*EK(I)*EPK(J)                      HEEXA162
               IF (METHOD .LT. 4) GO TO 540                             HEEXA163
               JG=J+NGAM                                                HEEXA164
               DELTAD(J)=DELTAD(J)+YLYK*SWK*EPPK(J)*PLIN(JG,N)          HEEXA165
 540        CONTINUE                                                    HEEXA166
 550     CONTINUE                                                       HEEXA167
 700     CALL HESSEN (NLINMX,NONLMX,PLIN(1,N),ASAVE(1,1,N),GTG,H(1,1,N),HEEXA168
     1                DELTAH(1,N),HELP,EPKEPK(1,1,N),DTOLER,R,          HEEXA169
     2                PIVLIN(1,N),ONLYAT,ZERO)                          HEEXA170
 800  CONTINUE                                                          HEEXA171
C-----------------------------------------------------------------------HEEXA172
C  ADD RIGHT HAND SIDE R,                                               HEEXA173
C  AND, IF METHOD=4, SUBTRACT SECOND DERIVATIVE TERM DELTAD             HEEXA174
C-----------------------------------------------------------------------HEEXA175
 900  IF (ONLYAT) GO TO 920                                             HEEXA176
         DO 910 J=1,NONL                                                HEEXA177
 910     GTG(J,NONLP1)=R(J)                                             HEEXA178
 920  IF (METHOD .LE. 3) RETURN                                         HEEXA179
      DO 930 J=1,NONL                                                   HEEXA180
         DUB      =ABS(DELTAD(J))                                       HEEXA181
         DTOLER(J)=AMAX1(DTOLER(J),DUB)                                 HEEXA182
         GTG(J,J) =GTG(J,J)-DELTAD(J)                                   HEEXA183
 930  CONTINUE                                                          HEEXA184
      RETURN                                                            HEEXA185
      END                                                               HEEXA186
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++HESEN001
      SUBROUTINE HESSEN (NLINMX,NONLMX,PLIN,ATA,GTG,H,DELTAH,HELP,      HESEN002
     1                   EPKEPK,DTOLER,R,PIVLIN,ONLYAT,ZERO)            HESEN003
C-----------------------------------------------------------------------HESEN004
C     BUILDS UP HESSIAN MATRIX FOR NONLINEAR LEAST SQUARES IN           HESEN005
C     4 DIFFERENT WAYS (METHOD=1,2,3,4).                                HESEN006
C-----------------------------------------------------------------------HESEN007
C     SUBPROGRAMS CALLED - NONE                                         HESEN008
C     COMMON USED        - IBLOCK                                       HESEN009
C-----------------------------------------------------------------------HESEN010
      DOUBLE PRECISION ATA, GTG, H, DELTAH, HELP, EPKEPK, DTOLER, R,    HESEN011
     1                 ZERO, DUB, AMAX1, ABS                            HESEN012
      LOGICAL          PIVLIN, ONLYAT                                   HESEN013
      DIMENSION        PLIN(*), ATA(NLINMX,1), GTG(NONLMX,NONLMX),      HESEN014
     1                 H(NLINMX,1), DELTAH(*), HELP(NLINMX,NLINMX),     HESEN015
     2                 EPKEPK(NLINMX,1), DTOLER(NONLMX), R(NONLMX),     HESEN016
     3                 PIVLIN(*)                                        HESEN017
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       HESEN018
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       HESEN019
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   HESEN020
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    HESEN021
     4                 MTRY(10,2), MXITER(2), NL(10),                   HESEN022
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  HESEN023
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, HESEN024
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       HESEN025
     8                 NIN, NOUT                                        HESEN026
C-----------------------------------------------------------------------HESEN027
      AMAX1(DUB,ZERO)=DMAX1(DUB,ZERO)                                   HESEN028
      ABS(DUB)       =DABS(DUB)                                         HESEN029
C-----------------------------------------------------------------------HESEN030
      IF (ONLYAT) GO TO 120                                             HESEN031
         DO 100 J=1,NONL                                                HESEN032
         DO 100 I=1,NLIN                                                HESEN033
 100     HELP(I,J)=ZERO                                                 HESEN034
 120  DO 220 J=1,NONL                                                   HESEN035
         JG=J+NGAM                                                      HESEN036
         DO 200 I=1,NONL                                                HESEN037
            IG      =I+NGAM                                             HESEN038
            GTG(J,I)=GTG(J,I)+EPKEPK(J,I)*PLIN(JG)*PLIN(IG)             HESEN039
            IF (I .EQ. J) DTOLER(J)=AMAX1(DTOLER(J),GTG(J,J))           HESEN040
 200     CONTINUE                                                       HESEN041
         DO 210 I=1,NLIN                                                HESEN042
 210     H(I,J)=H(I,J)*PLIN(JG)                                         HESEN043
         IF (.NOT. ONLYAT) R(J)=R(J)+DELTAH(J)*PLIN(JG)                 HESEN044
 220  CONTINUE                                                          HESEN045
      IF (ONLYAT .AND. METHOD.EQ.4) GO TO 350                           HESEN046
      IF (ONLYAT) RETURN                                                HESEN047
      DO 250 I=1,NLIN                                                   HESEN048
         IF (.NOT. PIVLIN(I)) GO TO 250                                 HESEN049
         DO 240 J=1,NONL                                                HESEN050
            DUB=ZERO                                                    HESEN051
            DO 230 L=1,NLIN                                             HESEN052
 230        IF (PIVLIN(L)) DUB=DUB+ATA(I,L)*H(L,J)                      HESEN053
            HELP(I,J)=DUB                                               HESEN054
 240     CONTINUE                                                       HESEN055
 250  CONTINUE                                                          HESEN056
      DO 280 I=1,NONL                                                   HESEN057
      DO 280 J=1,NONL                                                   HESEN058
         DUB=ZERO                                                       HESEN059
         DO 260 L=1,NLIN                                                HESEN060
 260     DUB=DUB+H(L,I)*HELP(L,J)                                       HESEN061
         IF (J .EQ. I) DTOLER(I)=AMAX1(DTOLER(I),DUB)                   HESEN062
         GTG(I,J)=GTG(I,J)-DUB                                          HESEN063
 280  CONTINUE                                                          HESEN064
      IF (METHOD .LE. 1) RETURN                                         HESEN065
      DO 300 J=1,NONL                                                   HESEN066
      DO 300 I=1,NLIN                                                   HESEN067
 300  HELP(I,J)=ZERO                                                    HESEN068
      DO 320 J=1,NONL                                                   HESEN069
         JG=J+NGAM                                                      HESEN070
         IF (.NOT. PIVLIN(JG)) GO TO 320                                HESEN071
         DO 310 I=1,NLIN                                                HESEN072
 310     IF (PIVLIN(I)) HELP(I,J)=ATA(I,JG)*DELTAH(J)                   HESEN073
 320  CONTINUE                                                          HESEN074
      DO 340 I=1,NONL                                                   HESEN075
         IG=I+NGAM                                                      HESEN076
         DO 330 J=1,NONL                                                HESEN077
            DUB     =DELTAH(I)*HELP(IG,J)                               HESEN078
            GTG(I,J)=GTG(I,J)+DUB                                       HESEN079
            IF (J .EQ. I) DTOLER(I)=AMAX1(DTOLER(I),DUB)                HESEN080
 330     CONTINUE                                                       HESEN081
 340  CONTINUE                                                          HESEN082
      IF (METHOD .LE. 2) RETURN                                         HESEN083
 350  DO 360 J=1,NONL                                                   HESEN084
         JG     =J+NGAM                                                 HESEN085
         H(JG,J)=H(JG,J)-DELTAH(J)                                      HESEN086
 360  CONTINUE                                                          HESEN087
      IF (ONLYAT) RETURN                                                HESEN088
      DO 390 I=1,NONL                                                   HESEN089
      DO 390 J=1,NONL                                                   HESEN090
         DUB=ZERO                                                       HESEN091
         DO 370 L=1,NLIN                                                HESEN092
 370     DUB=DUB+H(L,I)*HELP(L,J)                                       HESEN093
         GTG(I,J)=GTG(I,J)+DUB                                          HESEN094
         GTG(J,I)=GTG(J,I)+DUB                                          HESEN095
         IF (J .NE. I) GO TO 390                                        HESEN096
         DUB      =ABS(DUB)                                             HESEN097
         DTOLER(I)=AMAX1(DTOLER(I),DUB)                                 HESEN098
 390  CONTINUE                                                          HESEN099
      RETURN                                                            HESEN100
      END                                                               HESEN101
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++HESPL001
      SUBROUTINE HESSPL (NLINMX,NONLMX,PLIN,GTG,H,DELTAH,DELTAD,HELP,   HESPL002
     1                   ASAVE,EPKEPK,DTOLER,R,PIVLIN,BSPL0,BSPL1,BSPL2,HESPL003
     2                   DX,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,ONLYAT,ZERO) HESPL004
C-----------------------------------------------------------------------HESPL005
C     BUILDS UP HESSIAN MATRIX FOR NONLINEAR LEAST SQUARES IN           HESPL006
C     4 DIFFERENT WAYS (METHOD=1,2,3,4). (SEE USERS MANUAL).            HESPL007
C     USING SPLINE APPROXIMATIONS.                                      HESPL008
C-----------------------------------------------------------------------HESPL009
C     SUBPROGRAMS CALLED - BSPLIN, HESSEN                               HESPL010
C     COMMON USED        - IBLOCK, LBLOCK                               HESPL011
C-----------------------------------------------------------------------HESPL012
      DOUBLE PRECISION GTG, H, DELTAH, DELTAD, HELP, ASAVE, EPKEPK,     HESPL013
     1                 DTOLER, R, BSPL0, BSPL1, BSPL2, DX,              HESPL014
     2                 CCAP, ECAP, YCAP, ZERO,                          HESPL015
     3                 DUB, DDUB, SUM1, SUM2, AMAX1, ABS                HESPL016
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  HESPL017
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    HESPL018
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  HESPL019
     3                 SPLINE, WRTBET, WRTCAP                           HESPL020
      LOGICAL          PIVLIN, ONLYAT                                   HESPL021
      DIMENSION        PLIN(NLINMX,1), GTG(NONLMX,NONLMX),              HESPL022
     1                 H(NLINMX,NLINMX,1), DELTAH(NONLMX,1),            HESPL023
     2                 DELTAD(NONLMX), HELP(NLINMX,NLINMX),             HESPL024
     3                 ASAVE(NLINMX,NLINMX,1), EPKEPK(NLINMX,NLINMX,1), HESPL025
     4                 DTOLER(NONLMX), R(NONLMX), PIVLIN(NLINMX,1),     HESPL026
     5                 BSPL0(4,NONLMX), BSPL1(4,NONLMX), DX(NONLMX),    HESPL027
     6                 BSPL2(4,NONLMX), IC(NONLMX), CCAP(NZMAX,NZMAX,1),HESPL028
     7                 ECAP(NZMAX,NGAMMX,1), YCAP(NZMAX,1)              HESPL029
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       HESPL030
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       HESPL031
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   HESPL032
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    HESPL033
     4                 MTRY(10,2), MXITER(2), NL(10),                   HESPL034
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  HESPL035
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, HESPL036
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       HESPL037
     8                 NIN, NOUT                                        HESPL038
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          HESPL039
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         HESPL040
     2                 DOUSOU(2), LUSER(30),                            HESPL041
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  HESPL042
     4                 SPLINE, WRTBET, WRTCAP                           HESPL043
C-----------------------------------------------------------------------HESPL044
      AMAX1(DUB,DDUB)=DMAX1(DUB,DDUB)                                   HESPL045
      ABS(DUB)       =DABS(DUB)                                         HESPL046
C-----------------------------------------------------------------------HESPL047
      DO 150 J=1,NONL                                                   HESPL048
         DO 140 I=1,NONL                                                HESPL049
 140     GTG(I,J)=ZERO                                                  HESPL050
         R(J)     =ZERO                                                 HESPL051
         DTOLER(J)=ZERO                                                 HESPL052
 150  CONTINUE                                                          HESPL053
      GTG(NONLP1,NONLP1)=ZERO                                           HESPL054
C-----------------------------------------------------------------------HESPL055
C  COMPUTE 1ST, AND IF METHOD=4, 2ND DERIVATIVE OF B-SPLINES            HESPL056
C-----------------------------------------------------------------------HESPL057
      DO 170 J=1,NONL                                                   HESPL058
         CALL BSPLIN (BSPL1(1,J),DX(J),1)                               HESPL059
         IF (METHOD .EQ. 4) CALL BSPLIN (BSPL2(1,J),DX(J),2)            HESPL060
 170  CONTINUE                                                          HESPL061
C-----------------------------------------------------------------------HESPL062
C  CASE  ND = 1                                                         HESPL063
C-----------------------------------------------------------------------HESPL064
      DO 260 J=1,NONL                                                   HESPL065
         DO 220 I=1,NONL                                                HESPL066
            SUM1=ZERO                                                   HESPL067
            LL  =IC(J)-1                                                HESPL068
            DO 215 L=1,IB                                               HESPL069
               LL  =LL+1                                                HESPL070
               SUM2=ZERO                                                HESPL071
               MM  =IC(I)-1                                             HESPL072
               DO 200 M=1,IB                                            HESPL073
                  MM  =MM+1                                             HESPL074
                  SUM2=SUM2+BSPL1(M,I)*CCAP(LL,MM,1)                    HESPL075
 200           CONTINUE                                                 HESPL076
               SUM1=SUM1+BSPL1(L,J)*SUM2                                HESPL077
 215        CONTINUE                                                    HESPL078
            EPKEPK(J,I,1)=SUM1                                          HESPL079
 220     CONTINUE                                                       HESPL080
         DO 250 I=1,NLIN                                                HESPL081
            SUM1=ZERO                                                   HESPL082
            LL  =IC(J)-1                                                HESPL083
            DO 245 L=1,IB                                               HESPL084
               LL  =LL+1                                                HESPL085
               SUM2=ZERO                                                HESPL086
               IF (I .LE. NGAM) GO TO 235                               HESPL087
                  IG=I-NGAM                                             HESPL088
                  MM=IC(IG)-1                                           HESPL089
                  DO 230 M=1,IB                                         HESPL090
                     MM  =MM+1                                          HESPL091
                     SUM2=SUM2+BSPL0(M,IG)*CCAP(LL,MM,1)                HESPL092
 230              CONTINUE                                              HESPL093
                  GO TO 240                                             HESPL094
 235           SUM2=ECAP(LL,I,1)                                        HESPL095
 240           SUM1=SUM1+BSPL1(L,J)*SUM2                                HESPL096
 245        CONTINUE                                                    HESPL097
            H(I,J,1)=SUM1                                               HESPL098
 250     CONTINUE                                                       HESPL099
 260  CONTINUE                                                          HESPL100
      IF (ONLYAT .AND. METHOD.NE.4) GO TO 305                           HESPL101
      DO 300 J=1,NONL                                                   HESPL102
         SUM1=ZERO                                                      HESPL103
         SUM2=ZERO                                                      HESPL104
         LL  =IC(J)-1                                                   HESPL105
         DO 290 L=1,IB                                                  HESPL106
            LL =LL+1                                                    HESPL107
            DUB=YCAP(LL,1)                                              HESPL108
            IF (NGAM .EQ. 0) GO TO 275                                  HESPL109
               DO 270 MU=1,NGAM                                         HESPL110
 270           DUB=DUB-ECAP(LL,MU,1)*PLIN(MU,1)                         HESPL111
 275        DO 285 I=1,NONL                                             HESPL112
               IG  =I+NGAM                                              HESPL113
               DDUB=ZERO                                                HESPL114
               MM  =IC(I)-1                                             HESPL115
               DO 280 M=1,IB                                            HESPL116
                  MM  =MM+1                                             HESPL117
                  DDUB=DDUB+BSPL0(M,I)*CCAP(LL,MM,1)                    HESPL118
 280           CONTINUE                                                 HESPL119
               DUB=DUB-DDUB*PLIN(IG,1)                                  HESPL120
 285        CONTINUE                                                    HESPL121
            SUM1=SUM1+BSPL1(L,J)*DUB                                    HESPL122
            IF (METHOD .EQ. 4) SUM2=SUM2+BSPL2(L,J)*DUB                 HESPL123
 290     CONTINUE                                                       HESPL124
         DELTAH(J,1)=SUM1                                               HESPL125
         IF (METHOD .LT. 4) GO TO 300                                   HESPL126
         JG=J+NGAM                                                      HESPL127
         DELTAD(J)=SUM2*PLIN(JG,1)                                      HESPL128
 300  CONTINUE                                                          HESPL129
 305  IF (NSTORD .OR. .NOT.IWTIST) GO TO 400                            HESPL130
C-----------------------------------------------------------------------HESPL131
C  CASE  ND > 1  .AND.  SAMET=.TRUE.  .AND.  IWTIST                     HESPL132
C-----------------------------------------------------------------------HESPL133
         DO 330 N=2,ND                                                  HESPL134
         DO 330 J=1,NONL                                                HESPL135
            DO 310 I=1,NONL                                             HESPL136
 310        EPKEPK(I,J,N)=EPKEPK(I,J,1)                                 HESPL137
            DO 320 I=1,NLIN                                             HESPL138
 320        H(I,J,N)=H(I,J,1)                                           HESPL139
 330     CONTINUE                                                       HESPL140
 400  CALL HESSEN (NLINMX,NONLMX,PLIN,ASAVE,GTG,H,DELTAH,HELP,EPKEPK,   HESPL141
     1             DTOLER,R,PIVLIN,ONLYAT,ZERO)                         HESPL142
      IF (ND .EQ. 1) GO TO 900                                          HESPL143
      DO 800 N=2,ND                                                     HESPL144
         IF (SAMET .AND. IWTIST) GO TO 565                              HESPL145
C-----------------------------------------------------------------------HESPL146
C  CASE  ND > 1  .AND.  .NOT.IWTIST                                     HESPL147
C-----------------------------------------------------------------------HESPL148
         DO 560 J=1,NONL                                                HESPL149
            DO 520 I=1,NONL                                             HESPL150
               SUM1=ZERO                                                HESPL151
               LL  =IC(J)-1                                             HESPL152
               DO 515 L=1,IB                                            HESPL153
                  LL  =LL+1                                             HESPL154
                  SUM2=ZERO                                             HESPL155
                  MM  =IC(I)-1                                          HESPL156
                  DO 500 M=1,IB                                         HESPL157
                     MM  =MM+1                                          HESPL158
                     SUM2=SUM2+BSPL1(M,I)*CCAP(LL,MM,N)                 HESPL159
 500              CONTINUE                                              HESPL160
                  SUM1=SUM1+BSPL1(L,J)*SUM2                             HESPL161
 515           CONTINUE                                                 HESPL162
               EPKEPK(J,I,N)=SUM1                                       HESPL163
 520        CONTINUE                                                    HESPL164
            DO 550 I=1,NLIN                                             HESPL165
               SUM1=ZERO                                                HESPL166
               LL  =IC(J)-1                                             HESPL167
               DO 545 L=1,IB                                            HESPL168
                  LL  =LL+1                                             HESPL169
                  SUM2=ZERO                                             HESPL170
                  IF (I .LE. NGAM) GO TO 535                            HESPL171
                     IG=I-NGAM                                          HESPL172
                     MM=IC(IG)-1                                        HESPL173
                     DO 530 M=1,IB                                      HESPL174
                        MM  =MM+1                                       HESPL175
                        SUM2=SUM2+BSPL0(M,IG)*CCAP(LL,MM,N)             HESPL176
 530                 CONTINUE                                           HESPL177
                     GO TO 540                                          HESPL178
 535              SUM2=ECAP(LL,I,N)                                     HESPL179
 540              SUM1=SUM1+BSPL1(L,J)*SUM2                             HESPL180
 545           CONTINUE                                                 HESPL181
               H(I,J,N)=SUM1                                            HESPL182
 550        CONTINUE                                                    HESPL183
 560     CONTINUE                                                       HESPL184
 565     IF (ONLYAT .AND. METHOD.NE.4) GO TO 700                        HESPL185
         DO 600 J=1,NONL                                                HESPL186
            SUM1=ZERO                                                   HESPL187
            SUM2=ZERO                                                   HESPL188
            LL  =IC(J)-1                                                HESPL189
            DO 590 L=1,IB                                               HESPL190
               LL =LL+1                                                 HESPL191
               DUB=YCAP(LL,N)                                           HESPL192
               IF (NGAM .EQ. 0) GO TO 575                               HESPL193
                  DO 570 MU=1,NGAM                                      HESPL194
 570              DUB=DUB-ECAP(LL,MU,N)*PLIN(MU,N)                      HESPL195
 575           DO 585 I=1,NONL                                          HESPL196
                  IG  =I+NGAM                                           HESPL197
                  DDUB=ZERO                                             HESPL198
                  MM  =IC(I)-1                                          HESPL199
                  DO 580 M=1,IB                                         HESPL200
                     MM  =MM+1                                          HESPL201
                     DDUB=DDUB+BSPL0(M,I)*CCAP(LL,MM,N)                 HESPL202
 580              CONTINUE                                              HESPL203
                  DUB=DUB-DDUB*PLIN(IG,N)                               HESPL204
 585           CONTINUE                                                 HESPL205
               SUM1=SUM1+BSPL1(L,J)*DUB                                 HESPL206
               IF (METHOD .EQ. 4) SUM2=SUM2+BSPL2(L,J)*DUB              HESPL207
 590        CONTINUE                                                    HESPL208
            DELTAH(J,N)=SUM1                                            HESPL209
            IF (METHOD .LT. 4) GO TO 600                                HESPL210
            JG=J+NGAM                                                   HESPL211
            DELTAD(J)=DELTAD(J)+SUM2*PLIN(JG,N)                         HESPL212
 600     CONTINUE                                                       HESPL213
 700     CALL HESSEN (NLINMX,NONLMX,PLIN(1,N),ASAVE(1,1,N),GTG,H(1,1,N),HESPL214
     1                DELTAH(1,N),HELP,EPKEPK(1,1,N),DTOLER,R,          HESPL215
     2                PIVLIN(1,N),ONLYAT,ZERO)                          HESPL216
 800  CONTINUE                                                          HESPL217
C-----------------------------------------------------------------------HESPL218
C  ADD RIGHT HAND SIDE R,                                               HESPL219
C  AND, IF METHOD=4, SUBTRACT 2ND DERIVATIVE TERM DELTAD                HESPL220
C-----------------------------------------------------------------------HESPL221
 900  IF (ONLYAT) GO TO 920                                             HESPL222
         DO 910 J=1,NONL                                                HESPL223
 910     GTG(J,NONLP1)=R(J)                                             HESPL224
 920  IF (METHOD .LE. 3) RETURN                                         HESPL225
      DO 930 J=1,NONL                                                   HESPL226
         DUB      =ABS(DELTAD(J))                                       HESPL227
         DTOLER(J)=AMAX1(DTOLER(J),DUB)                                 HESPL228
         GTG(J,J) =GTG(J,J)-DELTAD(J)                                   HESPL229
 930  CONTINUE                                                          HESPL230
      RETURN                                                            HESPL231
      END                                                               HESPL232
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++INCAP001
      SUBROUTINE INICAP (NYMAX,Y,T,SQRTW,NY,NZMAX,BETA,ZGRID,D,U,NGAMMX,INCAP002
     1                   CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,WORK,ZERO,DZ,  INCAP003
     2                   RESPON)                                        INCAP004
C-----------------------------------------------------------------------INCAP005
C  EVALUATES ACCUMULATION ARRAYS ...CAP ONCE AND FOR ALL, SO MAKE       INCAP006
C  THE PROBLEM INDEPENDENT OF NUMBER OF DATA POINTS NY(N), N=1,..ND.    INCAP007
C  ONLY CALLED IF DOSPL=.TRUE., I.E. USE SPLINE MODEL.                  INCAP008
C-----------------------------------------------------------------------INCAP009
C     SUBPROGRAMS CALLED - BCOEFF, USERFL                               INCAP010
C     WHICH IN TURN CALL - CHOSOL, ERRMES, USERFN, USERTR               INCAP011
C     COMMON USED        - IBLOCK, LBLOCK                               INCAP012
C-----------------------------------------------------------------------INCAP013
      DOUBLE PRECISION BETA, ZGRID, D, U, CCAP, ECAP, YCAP, SGGCAP,     INCAP014
     1                 SGYCAP, YW, WORK, ZERO, DZ                       INCAP015
      DOUBLE PRECISION YK1, SWK1, YKN, SWKN                             INCAP016
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  INCAP017
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    INCAP018
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  INCAP019
     3                 SPLINE, WRTBET, WRTCAP                           INCAP020
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), SQRTW(NYMAX,1), NY(*),   INCAP021
     1                 BETA(NZMAX), ZGRID(NZMAX), D(NZMAX), U(NZMAX),   INCAP022
     2                 CCAP(NZMAX,NZMAX,1), ECAP(NZMAX,NGAMMX,1),       INCAP023
     3                 YCAP(NZMAX,1), SGGCAP(NGAMMX,NGAMMX,1),          INCAP024
     4                 SGYCAP(NGAMMX,1), YW(*), WORK(NZMAX), RESPON(*)  INCAP025
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       INCAP026
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       INCAP027
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   INCAP028
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    INCAP029
     4                 MTRY(10,2), MXITER(2), NL(10),                   INCAP030
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  INCAP031
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, INCAP032
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       INCAP033
     8                 NIN, NOUT                                        INCAP034
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          INCAP035
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         INCAP036
     2                 DOUSOU(2), LUSER(30),                            INCAP037
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  INCAP038
     4                 SPLINE, WRTBET, WRTCAP                           INCAP039
C-----------------------------------------------------------------------INCAP040
C  INITIALIZE ACCUMULATION ARRAYS ...CAP                                INCAP041
C  CASE  ND = 1                                                         INCAP042
C-----------------------------------------------------------------------INCAP043
      YW(1)=ZERO                                                        INCAP044
      DO 110 M=1,NB                                                     INCAP045
         YCAP(M,1)=ZERO                                                 INCAP046
         IF (REDCAP) GO TO 110                                          INCAP047
         DO 100 L=1,M                                                   INCAP048
 100     CCAP(L,M,1)=ZERO                                               INCAP049
 110  CONTINUE                                                          INCAP050
      IF (NGAM .EQ. 0) GO TO 150                                        INCAP051
         DO 140 NU=1,NGAM                                               INCAP052
            SGYCAP(NU,1)=ZERO                                           INCAP053
            IF (REDCAP) GO TO 140                                       INCAP054
            DO 120 L=1,NB                                               INCAP055
 120        ECAP(L,NU,1)=ZERO                                           INCAP056
            DO 130 MU=1,NU                                              INCAP057
 130        SGGCAP(MU,NU,1)=ZERO                                        INCAP058
 140     CONTINUE                                                       INCAP059
 150  IF (ND .EQ. 1) GO TO 220                                          INCAP060
C-----------------------------------------------------------------------INCAP061
C  CASE  ND > 1                                                         INCAP062
C-----------------------------------------------------------------------INCAP063
      DO 210 N=2,ND                                                     INCAP064
         YW(N)=ZERO                                                     INCAP065
         DO 170 M=1,NB                                                  INCAP066
            YCAP(M,N)=ZERO                                              INCAP067
            IF (REDCAP) GO TO 170                                       INCAP068
            DO 160 L=1,M                                                INCAP069
 160        CCAP(L,M,N)=ZERO                                            INCAP070
 170     CONTINUE                                                       INCAP071
         IF (NGAM .EQ. 0) GO TO 210                                     INCAP072
         DO 200 NU=1,NGAM                                               INCAP073
            SGYCAP(NU,N)=ZERO                                           INCAP074
            IF (REDCAP) GO TO 200                                       INCAP075
            DO 180 L=1,NB                                               INCAP076
 180        ECAP(L,NU,N)=ZERO                                           INCAP077
            DO 190 MU=1,NU                                              INCAP078
 190        SGGCAP(MU,NU,N)=ZERO                                        INCAP079
 200     CONTINUE                                                       INCAP080
 210  CONTINUE                                                          INCAP081
C-----------------------------------------------------------------------INCAP082
C  EVALUATE ACCUMULATION ARRAYS ...CAP ONCE AND FOR ALL                 INCAP083
C  CASE  ND = 1                                                         INCAP084
C-----------------------------------------------------------------------INCAP085
 220  MY=NY(1)                                                          INCAP086
      DO 400 K=1,MY                                                     INCAP087
         KK   =K                                                        INCAP088
         SWK1 =SQRTW(K,1)                                               INCAP089
         SWK1 =SWK1*SWK1                                                INCAP090
         YK1  =Y(K,1)                                                   INCAP091
         YW(1)=YW(1)+YK1*YK1*SWK1                                       INCAP092
C-----------------------------------------------------------------------INCAP093
C  EVALUATE B-SPLINE COEFFICIENTS BETA                                  INCAP094
C-----------------------------------------------------------------------INCAP095
         IF (.NOT. REDBET) CALL BCOEFF (                                INCAP096
     1        NYMAX,T,NY,KK,1,NZMAX,D,U,ZGRID,BETA,WORK,DZ,RESPON)      INCAP097
         IF (WRTBET) WRITE (IUNIT) (BETA(L),L=1,NB)                     INCAP098
         IF (REDBET)  READ (IUNIT) (BETA(L),L=1,NB)                     INCAP099
         IF (NGAM .EQ. 0) GO TO 240                                     INCAP100
C-----------------------------------------------------------------------INCAP101
C  COMPUTE USERFL(T(K,1),NU) FOR NU=1,...,NGAM                          INCAP102
C-----------------------------------------------------------------------INCAP103
            DO 230 NU=1,NGAM                                            INCAP104
               J       =NU                                              INCAP105
               WORK(NU)=USERFL(J,NYMAX,T,NY,KK,1,RESPON)                INCAP106
 230        CONTINUE                                                    INCAP107
C-----------------------------------------------------------------------INCAP108
C  NOW ACCUMULATE ARRAYS ...CAP                                         INCAP109
C-----------------------------------------------------------------------INCAP110
 240     DO 300 M=1,NB                                                  INCAP111
            YCAP(M,1)=YCAP(M,1)+BETA(M)*YK1*SWK1                        INCAP112
            IF (REDCAP) GO TO 260                                       INCAP113
               DO 250 L=1,M                                             INCAP114
 250           CCAP(L,M,1)=CCAP(L,M,1)+BETA(L)*BETA(M)*SWK1             INCAP115
 260        IF (NSTORD) GO TO 300                                       INCAP116
            DO 290 N=2,ND                                               INCAP117
               SWKN     =SQRTW(K,N)                                     INCAP118
               SWKN     =SWKN*SWKN                                      INCAP119
               YKN      =Y(K,N)                                         INCAP120
               YCAP(M,N)=YCAP(M,N)+BETA(M)*YKN*SWKN                     INCAP121
               IF (IWTIST .OR. REDCAP) GO TO 290                        INCAP122
               DO 270 L=1,M                                             INCAP123
 270           CCAP(L,M,N)=CCAP(L,M,N)+BETA(L)*BETA(M)*SWKN             INCAP124
 290        CONTINUE                                                    INCAP125
 300     CONTINUE                                                       INCAP126
         IF (NGAM .EQ. 0) GO TO 400                                     INCAP127
         DO 380 NU=1,NGAM                                               INCAP128
            SGYCAP(NU,1)=SGYCAP(NU,1)+WORK(NU)*YK1*SWK1                 INCAP129
            IF (REDCAP) GO TO 330                                       INCAP130
               DO 310 L=1,NB                                            INCAP131
 310           ECAP(L,NU,1)=ECAP(L,NU,1)+BETA(L)*WORK(NU)*SWK1          INCAP132
               DO 320 MU=1,NU                                           INCAP133
 320           SGGCAP(MU,NU,1)=SGGCAP(MU,NU,1)+WORK(MU)*WORK(NU)*SWK1   INCAP134
 330        IF (NSTORD) GO TO 380                                       INCAP135
            DO 360 N=2,ND                                               INCAP136
               SWKN=SQRTW(K,N)                                          INCAP137
               SWKN=SWKN*SWKN                                           INCAP138
               YKN =Y(K,N)                                              INCAP139
               SGYCAP(NU,N)=SGYCAP(NU,N)+WORK(NU)*YKN*SWKN              INCAP140
               IF (IWTIST .OR. REDCAP) GO TO 360                        INCAP141
               DO 340 L=1,NB                                            INCAP142
 340           ECAP(L,NU,N)=ECAP(L,NU,N)+BETA(L)*WORK(NU)*SWKN          INCAP143
               DO 350 MU=1,NU                                           INCAP144
 350           SGGCAP(MU,NU,N)=SGGCAP(MU,NU,N)+WORK(MU)*WORK(NU)*SWKN   INCAP145
 360        CONTINUE                                                    INCAP146
 380     CONTINUE                                                       INCAP147
 400  CONTINUE                                                          INCAP148
C-----------------------------------------------------------------------INCAP149
C  WRITE OR READ ARRAYS CCAP, ECAP, AND SGGCAP AS SPECIFIED THROUGH     INCAP150
C  WRTCAP AND REDCAP.                                                   INCAP151
C-----------------------------------------------------------------------INCAP152
      IF (.NOT. WRTCAP) GO TO 460                                       INCAP153
         DO 440 M=1,NB                                                  INCAP154
 440     WRITE (IUNIT) (CCAP(L,M,1),L=1,M)                              INCAP155
         IF (NGAM .EQ. 0) GO TO 460                                     INCAP156
         DO 450 NU=1,NGAM                                               INCAP157
            WRITE (IUNIT) (ECAP(L,NU,1),L=1,NB)                         INCAP158
            WRITE (IUNIT) (SGGCAP(MU,NU,1),MU=1,NU)                     INCAP159
 450     CONTINUE                                                       INCAP160
 460  IF (.NOT. REDCAP) GO TO 500                                       INCAP161
         DO 470 M=1,NB                                                  INCAP162
 470     READ (IUNIT) (CCAP(L,M,1),L=1,M)                               INCAP163
         IF (NGAM .EQ. 0) GO TO 500                                     INCAP164
         DO 480 NU=1,NGAM                                               INCAP165
            READ (IUNIT) (ECAP(L,NU,1),L=1,NB)                          INCAP166
            READ (IUNIT) (SGGCAP(MU,NU,1),MU=1,NU)                      INCAP167
 480     CONTINUE                                                       INCAP168
C-----------------------------------------------------------------------INCAP169
C  FILL UP LOWER TRIANGLE OF SYMMETRIC ARRAYS CCAP AND SGGCAP           INCAP170
C-----------------------------------------------------------------------INCAP171
 500  DO 520 L=2,NB                                                     INCAP172
         LM1=L-1                                                        INCAP173
         DO 510 M=1,LM1                                                 INCAP174
 510     CCAP(L,M,1)=CCAP(M,L,1)                                        INCAP175
 520  CONTINUE                                                          INCAP176
      IF (NGAM .LE. 1) GO TO 550                                        INCAP177
         DO 540 MU=2,NGAM                                               INCAP178
            MUM1=MU-1                                                   INCAP179
            DO 530 NU=1,MUM1                                            INCAP180
 530        SGGCAP(MU,NU,1)=SGGCAP(NU,MU,1)                             INCAP181
 540     CONTINUE                                                       INCAP182
 550  IF (ND .EQ. 1) RETURN                                             INCAP183
C-----------------------------------------------------------------------INCAP184
C  CASE  ND > 1                                                         INCAP185
C-----------------------------------------------------------------------INCAP186
      IF (.NOT.SAMET .OR. .NOT.IWTIST) GO TO 620                        INCAP187
         DO 610 N=2,ND                                                  INCAP188
            DO 570 M=1,NB                                               INCAP189
            DO 570 L=1,M                                                INCAP190
 570        CCAP(L,M,N)=CCAP(L,M,1)                                     INCAP191
            IF (NGAM .EQ. 0) GO TO 610                                  INCAP192
               DO 600 NU=1,NGAM                                         INCAP193
               DO 580 L=1,NB                                            INCAP194
 580           ECAP(L,NU,N)=ECAP(L,NU,1)                                INCAP195
               DO 590 MU=1,NU                                           INCAP196
 590           SGGCAP(MU,NU,N)=SGGCAP(MU,NU,1)                          INCAP197
 600        CONTINUE                                                    INCAP198
 610     CONTINUE                                                       INCAP199
 620  DO 900 N=2,ND                                                     INCAP200
         MY=NY(N)                                                       INCAP201
         NN=N                                                           INCAP202
         DO 700 K=1,MY                                                  INCAP203
            KK   =K                                                     INCAP204
            SWKN =SQRTW(K,N)                                            INCAP205
            SWKN =SWKN*SWKN                                             INCAP206
            YKN  =Y(K,N)                                                INCAP207
            YW(N)=YW(N)+YKN*YKN*SWKN                                    INCAP208
            IF (SAMET) GO TO 700                                        INCAP209
C-----------------------------------------------------------------------INCAP210
C  EVALUATE B-SPLINE COEFFICIENTS BETA                                  INCAP211
C-----------------------------------------------------------------------INCAP212
            IF (.NOT. REDBET) CALL BCOEFF (                             INCAP213
     1           NYMAX,T,NY,KK,NN,NZMAX,D,U,ZGRID,BETA,WORK,DZ,RESPON)  INCAP214
            IF (WRTBET) WRITE (IUNIT) (BETA(L),L=1,NB)                  INCAP215
            IF (REDBET) READ (IUNIT) (BETA(L),L=1,NB)                   INCAP216
            IF (NGAM .EQ. 0) GO TO 640                                  INCAP217
C-----------------------------------------------------------------------INCAP218
C  COMPUTE USERFL(T(K,N),NU) FOR NU=1,...,NGAM                          INCAP219
C-----------------------------------------------------------------------INCAP220
               DO 630 NU=1,NGAM                                         INCAP221
                  J       =NU                                           INCAP222
                  WORK(NU)=USERFL(J,NYMAX,T,NY,KK,NN,RESPON)            INCAP223
 630           CONTINUE                                                 INCAP224
C-----------------------------------------------------------------------INCAP225
C  NOW ACCUMULATE ARRAYS ...CAP                                         INCAP226
C-----------------------------------------------------------------------INCAP227
 640        DO 660 M=1,NB                                               INCAP228
               YCAP(M,N)=YCAP(M,N)+BETA(M)*YKN*SWKN                     INCAP229
               IF (REDCAP) GO TO 660                                    INCAP230
               DO 650 L=1,M                                             INCAP231
 650           CCAP(L,M,N)=CCAP(L,M,N)+BETA(L)*BETA(M)*SWKN             INCAP232
 660        CONTINUE                                                    INCAP233
            IF (NGAM .EQ. 0) GO TO 700                                  INCAP234
            DO 690 NU=1,NGAM                                            INCAP235
               SGYCAP(NU,N)=SGYCAP(NU,N)+WORK(NU)*YKN*SWKN              INCAP236
               IF (REDCAP) GO TO 690                                    INCAP237
               DO 670 L=1,NB                                            INCAP238
 670           ECAP(L,NU,N)=ECAP(L,NU,N)+BETA(L)*WORK(NU)*SWKN          INCAP239
               DO 680 MU=1,NU                                           INCAP240
 680           SGGCAP(MU,NU,N)=SGGCAP(MU,NU,N)+WORK(MU)*WORK(NU)*SWKN   INCAP241
 690        CONTINUE                                                    INCAP242
 700     CONTINUE                                                       INCAP243
C-----------------------------------------------------------------------INCAP244
C  WRITE OR READ ARRAYS CCAP, ECAP, AND SGGCAP AS SPECIFIED THROUGH     INCAP245
C  WRTCAP AND REDCAP.                                                   INCAP246
C-----------------------------------------------------------------------INCAP247
         IF (.NOT. WRTCAP) GO TO 820                                    INCAP248
            DO 800 M=1,NB                                               INCAP249
 800        WRITE (IUNIT) (CCAP(L,M,N),L=1,M)                           INCAP250
            IF (NGAM .EQ. 0) GO TO 820                                  INCAP251
            DO 810 NU=1,NGAM                                            INCAP252
               WRITE (IUNIT) (ECAP(L,NU,N),L=1,NB)                      INCAP253
               WRITE (IUNIT) (SGGCAP(MU,NU,N),MU=1,NU)                  INCAP254
 810        CONTINUE                                                    INCAP255
 820     IF (.NOT. REDCAP) GO TO 850                                    INCAP256
         DO 830 M=1,NB                                                  INCAP257
 830     READ (IUNIT) (CCAP(L,M,N),L=1,M)                               INCAP258
         IF (NGAM .EQ. 0) GO TO 850                                     INCAP259
         DO 840 NU=1,NGAM                                               INCAP260
            READ (IUNIT) (ECAP(L,NU,N),L=1,NB)                          INCAP261
            READ (IUNIT) (SGGCAP(MU,NU,N),MU=1,NU)                      INCAP262
 840     CONTINUE                                                       INCAP263
C-----------------------------------------------------------------------INCAP264
C  FILL UP LOWER TRIANGLE OF SYMMETRIC ARRAYS CCAP AND SGGCAP           INCAP265
C-----------------------------------------------------------------------INCAP266
 850     DO 870 L=2,NB                                                  INCAP267
            LM1=L-1                                                     INCAP268
            DO 860 M=1,LM1                                              INCAP269
 860        CCAP(L,M,N)=CCAP(M,L,N)                                     INCAP270
 870     CONTINUE                                                       INCAP271
         IF (NGAM .LE. 1) GO TO 900                                     INCAP272
         DO 890 MU=2,NGAM                                               INCAP273
            MUM1=MU-1                                                   INCAP274
            DO 880 NU=1,MUM1                                            INCAP275
 880        SGGCAP(MU,NU,N)=SGGCAP(NU,MU,N)                             INCAP276
 890     CONTINUE                                                       INCAP277
 900  CONTINUE                                                          INCAP278
      RETURN                                                            INCAP279
      END                                                               INCAP280
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++INIT0001
      SUBROUTINE INIT                                                   INIT0002
C-----------------------------------------------------------------------INIT0003
C     INITIALIZES VARIABLES AT THE START OF A RUN.                      INIT0004
C-----------------------------------------------------------------------INIT0005
C     SUBPROGRAMS CALLED - DIFF, ERRMES                                 INIT0006
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               INIT0007
C-----------------------------------------------------------------------INIT0008
      DOUBLE PRECISION PRECIS, RANGE,                                   INIT0009
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       INIT0010
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           INIT0011
     3                 TWOTHR, FORTHR, SIXTEL                           INIT0012
      DOUBLE PRECISION PREMIN, DIFF, SMALL, SIZE, PTRY, DELTRY,         INIT0013
     1                 AMAX1                                            INIT0014
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  INIT0015
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    INIT0016
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  INIT0017
     3                 SPLINE, WRTBET, WRTCAP                           INIT0018
      LOGICAL          DONE1                                            INIT0019
      DIMENSION        IHOLER(6)                                        INIT0020
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        INIT0021
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, INIT0022
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 INIT0023
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),INIT0024
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     INIT0025
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       INIT0026
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       INIT0027
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   INIT0028
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    INIT0029
     4                 MTRY(10,2), MXITER(2), NL(10),                   INIT0030
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  INIT0031
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, INIT0032
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       INIT0033
     8                 NIN, NOUT                                        INIT0034
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          INIT0035
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         INIT0036
     2                 DOUSOU(2), LUSER(30),                            INIT0037
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  INIT0038
     4                 SPLINE, WRTBET, WRTCAP                           INIT0039
      DATA             IHOLER/1HI, 1HN, 1HI, 1HT, 2*1H /                INIT0040
C-----------------------------------------------------------------------INIT0041
      AMAX1(ONE,TWO)=DMAX1(ONE,TWO)                                     INIT0042
C-----------------------------------------------------------------------INIT0043
      TWOTHR=TWO/THREE                                                  INIT0044
      FORTHR=FOUR/THREE                                                 INIT0045
      SIXTEL=ONE/SIX                                                    INIT0046
C     PREMIN=1.E-4                                                      INIT0047
      PREMIN=1.D-8                                                      INIT0048
      FACT  =RANGE**(-.025)                                             INIT0049
      SMALL =ONE/RANGE                                                  INIT0050
      DONE1 =.FALSE.                                                    INIT0051
      SIZE  =RANGE                                                      INIT0052
      DO 110 J=1,80                                                     INIT0053
         PTRY=PREMIN                                                    INIT0054
         DO 120 K=1,150                                                 INIT0055
            PTRY  =.5*PTRY                                              INIT0056
            DELTRY=PTRY*SIZE                                            INIT0057
            IF (DELTRY .LT. SMALL) GO TO 140                            INIT0058
            IF (DIFF(SIZE+DELTRY,SIZE) .LE. ZERO) GO TO 130             INIT0059
 120     CONTINUE                                                       INIT0060
 130     IF (DONE1) PRECIS=AMAX1(PTRY,PRECIS)                           INIT0061
         IF (.NOT.DONE1) PRECIS=PTRY                                    INIT0062
         DONE1=.TRUE.                                                   INIT0063
         SIZE =FACT*SIZE                                                INIT0064
 110  CONTINUE                                                          INIT0065
 140  PRECIS=20.*PRECIS                                                 INIT0066
      IF (PRECIS .GT. PREMIN) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)        INIT0067
      MIOERR=MAX0(2,MIOERR)                                             INIT0068
      EXMAX=ALOG(SRANGE)                                                INIT0069
      RETURN                                                            INIT0070
      END                                                               INIT0071
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++INPUT001
      SUBROUTINE INPUT (NYMAX,Y,T,EXACT,SQRTW,NY,NONLMX,NLINMX,NGAMMX,  INPUT002
     1                  NZMAX,NDMAX,NLNDMX,MTRYMX,RESPON,BOUNDL,BOUNDN) INPUT003
C-----------------------------------------------------------------------INPUT004
C     READS INPUT DATA, WRITES THEM OUT, AND CHECKS FOR INPUT ERRORS.   INPUT005
C-----------------------------------------------------------------------INPUT006
C     SUBPROGRAMS CALLED - ERRMES, READYT, STORIN, WRITIN               INPUT007
C     WHICH IN TURN CALL - ERRMES, PLPRIN, USERIN, WRITYT               INPUT008
C     COMMON USED        - SBLOCK, IBLOCK, LBLOCK                       INPUT009
C-----------------------------------------------------------------------INPUT010
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  INPUT011
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    INPUT012
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  INPUT013
     3                 SPLINE, WRTBET, WRTCAP                           INPUT014
      LOGICAL          LERR                                             INPUT015
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), SQRTW(NYMAX,1),          INPUT016
     1                 EXACT(NYMAX,1), NY(*), RESPON(*),                INPUT017
     2                 BOUNDL(2), BOUNDN(2)                             INPUT018
      DIMENSION        LIN(6), LA(6,40), LA1(6,14), LA2(6,13),          INPUT019
     1                 LA3(6,13), IHOLER(6)                             INPUT020
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),INPUT021
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     INPUT022
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       INPUT023
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       INPUT024
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   INPUT025
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    INPUT026
     4                 MTRY(10,2), MXITER(2), NL(10),                   INPUT027
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  INPUT028
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, INPUT029
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       INPUT030
     8                 NIN, NOUT                                        INPUT031
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          INPUT032
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         INPUT033
     2                 DOUSOU(2), LUSER(30),                            INPUT034
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  INPUT035
     4                 SPLINE, WRTBET, WRTCAP                           INPUT036
C-----------------------------------------------------------------------INPUT037
C  LA IS BEING BROKEN UP JUST TO KEEP THE NO. OF CONTINUATION CARDS     INPUT038
C  IN THE DATA STATEMENTS SMALL.                                        INPUT039
C-----------------------------------------------------------------------INPUT040
      EQUIVALENCE      (LA(1,1),LA1(1,1)), (LA(1,15),LA2(1,1)),         INPUT041
     1                 (LA(1,28),LA3(1,1))                              INPUT042
      DATA             IHOLER/1HI, 1HN, 1HP, 1HU, 1HT, 1H /,            INPUT043
     1                 MLA/40/                                          INPUT044
      DATA             LA1/                                             INPUT045
     1 1HC, 1HO, 1HN, 1HV, 1HR, 1HG,   1HP, 1HL, 1HM, 1HN, 1HM, 1HX,    INPUT046
     2 1HP, 1HN, 1HM, 1HN, 1HM, 1HX,   1HR, 1HU, 1HS, 1HE, 1HR, 1H ,    INPUT047
     3 1HI, 1HR, 1HW, 1HC, 1HA, 1HP,   1HI, 1HU, 1HN, 1HI, 1HT, 1H ,    INPUT048
     4 1HI, 1HW, 1HT,      3 *  1H ,   1HL, 1HI, 1HN, 1HE, 1HP, 1HG,    INPUT049
     5 1HM, 1HC, 1HO, 1HN, 1HV, 1H ,   1HM, 1HE, 1HT, 1HH, 1HO, 1HD,    INPUT050
     6 1HM, 1HI, 1HO, 1HE, 1HR, 1HR,   1HN, 1HA, 1HB, 1HO, 1HR, 1HT,    INPUT051
     7 1HN, 1HD,           4 *  1H ,   1HN, 1HE, 1HR, 1HF, 1HI, 1HT/    INPUT052
      DATA             LA2/                                             INPUT053
     1 1HN, 1HG, 1HA, 1HM, 2 *  1H ,   1HN, 1HN, 1HL,      3 *  1H ,    INPUT054
     2 1HN, 1HZ,           4 *  1H ,   1HI, 1HF, 1HO, 1HR, 1HM, 1HT,    INPUT055
     3 1HI, 1HF, 1HO, 1HR, 1HM, 1HW,   1HI, 1HF, 1HO, 1HR, 1HM, 1HY,    INPUT056
     4 1HI, 1HP, 1HL, 1HF, 1HI, 1HT,   1HI, 1HP, 1HL, 1HR, 1HE, 1HS,    INPUT057
     5 1HI, 1HP, 1HR, 1HI, 1HN, 1HT,   1HI, 1HP, 1HR, 1HI, 1HT, 1HR,    INPUT058
     6 1HI, 1HU, 1HS, 1HE, 1HR, 1H ,   1HM, 1HT, 1HR, 1HY, 2 *  1H ,    INPUT059
     7 1HM, 1HX, 1HI, 1HT, 1HE, 1HR/                                    INPUT060
      DATA             LA3/                                             INPUT061
     1 1HN, 1HL,           4 *  1H ,   1HD, 1HO, 1HU, 1HS, 1HI, 1HN,    INPUT062
     2 1HL, 1HA, 1HS, 1HT, 2 *  1H ,   1HP, 1HR, 1HW, 1HT, 2 *  1H ,    INPUT063
     3 1HP, 1HR, 1HY,      3 *  1H ,   1HS, 1HA, 1HM, 1HE, 1HT, 1H ,    INPUT064
     4 1HS, 1HI, 1HM, 1HU, 1HL, 1HA,   1HD, 1HO, 1HA, 1HD, 1HE, 1HX,    INPUT065
     5 1HD, 1HO, 1HS, 1HP, 1HL, 1H ,   1HD, 1HO, 1HS, 1HT, 1HR, 1HT,    INPUT066
     6 1HD, 1HO, 1HU, 1HS, 1HO, 1HU,   1HL, 1HU, 1HS, 1HE, 1HR, 1H ,    INPUT067
     7 1HE, 1HN, 1HD,      3 *  1H /                                    INPUT068
C-----------------------------------------------------------------------INPUT069
      READ (NIN,2000) ITITLE                                            INPUT070
      WRITE (NOUT,3000) ITITLE                                          INPUT071
      NIOERR=0                                                          INPUT072
 200  READ (NIN,4000) LIN,IIN,RIN                                       INPUT073
      WRITE (NOUT,5000) LIN,IIN,RIN                                     INPUT074
      DO 210 J=1,MLA                                                    INPUT075
         DO 220 K=1,6                                                   INPUT076
            IF (LIN(K) .NE. LA(K,J)) GO TO 210                          INPUT077
 220     CONTINUE                                                       INPUT078
         IF (J .EQ. MLA) GO TO 300                                      INPUT079
         JJ=J                                                           INPUT080
         CALL STORIN (JJ,NIOERR,LIN,IIN,RIN)                            INPUT081
         GO TO 200                                                      INPUT082
 210  CONTINUE                                                          INPUT083
      CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                               INPUT084
      WRITE (NOUT,1000)                                                 INPUT085
      NIOERR=NIOERR+1                                                   INPUT086
      IF (NIOERR .GE. MIOERR) STOP                                      INPUT087
      GO TO 200                                                         INPUT088
 300  BOUNDL(1)=PLMNMX(1)                                               INPUT089
      BOUNDL(2)=PLMNMX(2)                                               INPUT090
      BOUNDN(1)=PNMNMX(1)                                               INPUT091
      BOUNDN(2)=PNMNMX(2)                                               INPUT092
      CALL READYT (NYMAX,Y,T,SQRTW,NY,NIOERR,RESPON)                    INPUT093
      CALL WRITIN (NYMAX,Y,T,EXACT,SQRTW,NY,MLA,LA)                     INPUT094
      NB    =NZ+2                                                       INPUT095
      NSTORD=.NOT.SAMET .OR. ND.EQ.1                                    INPUT096
      REDBET=IRWCAP.EQ.2 .AND. (IWT.EQ.1 .OR. IWT.EQ.4)                 INPUT097
      REDCAP=REDBET                                                     INPUT098
      IF (REDBET) GO TO 350                                             INPUT099
      REDBET=IRWCAP.EQ.1 .AND. (IWT.EQ.2 .OR. IWT.EQ.3 .OR. IWT.EQ.5)   INPUT100
      REDCAP=IRWCAP.EQ.1 .AND. (IWT.EQ.1 .OR. IWT.EQ.4)                 INPUT101
 350  WRTBET=IRWCAP.EQ.-2 .AND. (IWT.EQ.1 .OR. IWT.EQ.4)                INPUT102
      WRTCAP=WRTBET                                                     INPUT103
      IF (WRTBET) GO TO 360                                             INPUT104
      WRTBET=IRWCAP.EQ.-1 .AND. (IWT.EQ.2 .OR. IWT.EQ.3 .OR. IWT.EQ.5)  INPUT105
      WRTCAP=IRWCAP.EQ.-1 .AND. (IWT.EQ.1 .OR. IWT.EQ.4)                INPUT106
C-----------------------------------------------------------------------INPUT107
C  CHECK COMMON VARIABLES FOR VIOLATIONS.                               INPUT108
C-----------------------------------------------------------------------INPUT109
 360  LERR=.FALSE.                                                      INPUT110
      IF (ND .LE. 0) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                 INPUT111
      MINNY=NY(1)                                                       INPUT112
      DO 400 N=1,ND                                                     INPUT113
 400  MINNY=MIN0(MINNY,NY(N))                                           INPUT114
      IF (NNL .LE. 0) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                INPUT115
      MINNL=NL(1)                                                       INPUT116
      MAXNL=NL(1)                                                       INPUT117
      DO 420 J=1,NNL                                                    INPUT118
         MINNL=MIN0(MINNL,NL(J))                                        INPUT119
         MAXNL=MAX0(MAXNL,NL(J))                                        INPUT120
 420  CONTINUE                                                          INPUT121
      MXNLNG=MAXNL+NGAM                                                 INPUT122
      LERR=LERR .OR. MIN0(IRWCAP+3,IWT,NGAM+1,NB-19,MINNL).LT.1         INPUT123
     1          .OR. MAX0(IRWCAP-2,IWT-5,NNL-10).GT.0                   INPUT124
      DO 450 J=1,2                                                      INPUT125
         IAPFIT=IABS(IPLFIT(J))                                         INPUT126
         LERR=LERR .OR. MIN0(IAPFIT,IPLRES(J),IPRITR(J)).LT.0           INPUT127
     1             .OR. MAX0(IAPFIT,IPLRES(J),IPRITR(J)).GT.3           INPUT128
         DO 430 I=1,3                                                   INPUT129
 430     LERR=LERR .OR. IPRINT(I,J).LT.1 .OR. IPRINT(I,J).GT.3          INPUT130
         DO 440 I=1,NNL                                                 INPUT131
 440     LERR=LERR .OR. MTRY(I,J).LT.1 .OR. MTRY(I,J).GT.MTRYMX         INPUT132
 450  CONTINUE                                                          INPUT133
      LERR=LERR .OR. ND.GT.NDMAX .OR. NGAM.GT.NGAMMX .OR. NB.GT.NZMAX   INPUT134
     1          .OR. MAXNL.GE.NONLMX .OR. MXNLNG.GE.NLINMX              INPUT135
     2          .OR. (MXNLNG*ND+MAXNL).GE.NLNDMX                        INPUT136
     3          .OR. (MXNLNG*ND+MAXNL).GE.NYSUM                         INPUT137
      LERR=LERR .OR. MIN0(MCONV,NABORT).GT.50 .OR. NERFIT.GT.MINNY      INPUT138
     1          .OR. NIOERR.NE.0                                        INPUT139
      IF (LERR) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)                      INPUT140
      RETURN                                                            INPUT141
 1000 FORMAT(1H )                                                       INPUT142
 2000 FORMAT(80A1)                                                      INPUT143
 3000 FORMAT(32H1SPLMOD - VERSION 3DP (JUN 1988),10X,80A1//             INPUT144
     1       49H REFERENCES - S.W.PROVENCHER AND R.H.VOGEL (1983),      INPUT145
     2       55H IN PROGRESS IN SCIENTIFIC COMPUTING, VOL. 2, PAGES 304,INPUT146
     3        5H-319,/49X,31H P.DEUFLHARD AND E.HAIRER EDS.,,           INPUT147
     4       22H (BIRKHAEUSER, BOSTON)/12X,11H- R.H.VOGEL,20X,          INPUT148
     5       49H(1988) SPLMOD USERS MANUAL, EMBL TECHNICAL REPORT,      INPUT149
     6        6H DA09,/49X,40H (EUROPEAN MOLECULAR BIOLOGY LABORATORY,, INPUT150
     7       29H HEIDELBERG, F.R. OF GERMANY)///40X,                    INPUT151
     8       42HINPUT DATA FOR CHANGES TO COMMON VARIABLES/)            INPUT152
 4000 FORMAT( 1X,6A1,I5,  E15.6)                                        INPUT153
 5000 FORMAT(/1X,6A1,I5,1PE15.6)                                        INPUT154
      END                                                               INPUT155
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++INERP001
      SUBROUTINE INTERP (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,  INERP002
     1                   PLNTRY,PLAM,PLMTRY,PIVLIN,EK,ETE,EKEK,R,DELTAL,INERP003
     2                   DTOLER,DELTAN,ASAVE,NCOMBI,MTRYMX,STARTV,DX,   INERP004
     3                   BSPL0,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,   INERP005
     4                   SGYCAP,YW,Q,QG,QMAX,VARG,NTRY,NOUT,VAROLD,NONL,INERP006
     5                   IPRITR,RESPON)                                 INERP007
C-----------------------------------------------------------------------INERP008
C     USE MODIFICATION-A OF G.E.BOX TO ESTIMATE BEST Q                  INERP009
C     (FRACTIONAL STEP SIZE)                                            INERP010
C-----------------------------------------------------------------------INERP011
C     SUBPROGRAMS CALLED - ERRMES, VARIAN                               INERP012
C     WHICH IN TURN CALL - BSPLIN, ERRMES, INTERV, LINEQS, LINPAR,      INERP013
C                          NPASCL, PIVOT,  PIVOT1, USERFL, USERFN,      INERP014
C                          USERTR                                       INERP015
C     COMMON USED        - NONE                                         INERP016
C-----------------------------------------------------------------------INERP017
      DOUBLE PRECISION EK, ETE, EKEK, R, DELTAL, DTOLER, DELTAN,        INERP018
     1                 ASAVE, DX, BSPL0, CCAP, ECAP, YCAP, SGGCAP,      INERP019
     2                 SGYCAP, YW, VARG, VAROLD, VT, DUB                INERP020
      LOGICAL          PIVLIN, STARTV                                   INERP021
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), YLYFIT(NYMAX,1),         INERP022
     1                 SQRTW(NYMAX,1), NY(*), RESPON(*), PLIN(NLINMX,1),INERP023
     2                 PLNTRY(NLINMX,1), PLAM(NONLMX), PLMTRY(NONLMX),  INERP024
     3                 PIVLIN(NLINMX,1), EK(NLINMX), ETE(NLINMX,NLINMX),INERP025
     4                 EKEK(NLINMX,NLINMX,1), R(NLINMX,1),              INERP026
     5                 DELTAL(NLINMX), DTOLER(NLINMX), DELTAN(NONLMX),  INERP027
     6                 ASAVE(NLINMX,NLINMX,1)                           INERP028
      DIMENSION        NCOMBI(NONLMX), STARTV(MTRYMX), DX(NONLMX),      INERP029
     1                 BSPL0(4,NONLMX), IC(NONLMX), CCAP(NZMAX,NZMAX,1),INERP030
     2                 ECAP(NZMAX,NGAMMX,1), YCAP(NZMAX,1),             INERP031
     3                 SGGCAP(NGAMMX,NGAMMX,1), SGYCAP(NGAMMX,1), YW(*) INERP032
      DIMENSION        VT(3), QT(3), IHOLER(6)                          INERP033
      DATA             QMIN/.001/                                       INERP034
      DATA             IHOLER/1HI, 1HN, 1HT, 1HE, 1HR, 1HP/             INERP035
C-----------------------------------------------------------------------INERP036
      ABS(DUB)=DABS(DUB)                                                INERP037
C-----------------------------------------------------------------------INERP038
      DDUM=0.                                                           INERP039
      DO 110 J=1,NONL                                                   INERP040
 110  DDUM=DDUM+DELTAN(J)*R(J,1)                                        INERP041
      IF (DDUM .GT. 0.) GO TO 120                                       INERP042
         IF (IPRITR .GT. 0) CALL ERRMES (1,.FALSE.,IHOLER,NOUT)         INERP043
         NTRY=2                                                         INERP044
         Q   =AMIN1(QMIN,QMAX)                                          INERP045
         RETURN                                                         INERP046
 120  NTRY=1                                                            INERP047
      CALL VARIAN (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,PLNTRY, INERP048
     1             PLAM,PLMTRY,PIVLIN,EK,ETE,EKEK,R,DELTAL,DTOLER,      INERP049
     2             DELTAN,ASAVE,.FALSE.,NCOMBI,MTRYMX,STARTV,DX,BSPL0,  INERP050
     3             IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,QG,  INERP051
     4             VARG,IERROR,.FALSE.,RESPON)                          INERP052
      DUM=VARG-VAROLD+2.*DDUM*QG                                        INERP053
      IF (DUM .LE. 0.) DUM=.25*DDUM*QG*QG                               INERP054
      Q=AMIN1(DDUM*QG*QG/DUM,4.,QMAX)                                   INERP055
      IF (VARG.LE.VAROLD .OR. Q.GE..125*QG) RETURN                      INERP056
C-----------------------------------------------------------------------INERP057
C  PUT QUADRATIC PARABOLA THROUGH ACTUAL VARIANCE SURFACE WHEN          INERP058
C  A STRONG NONLINEARITY IS INDICATED BY A SMALL Q                      INERP059
C-----------------------------------------------------------------------INERP060
      QT(1)=AMIN1(.125*QG,AMAX1(Q,.02))                                 INERP061
      IF (Q .GT. .125*QG) QT(1)=Q                                       INERP062
      CALL VARIAN (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,PLNTRY, INERP063
     1             PLAM,PLMTRY,PIVLIN,EK,ETE,EKEK,R,DELTAL,DTOLER,      INERP064
     2             DELTAN,ASAVE,.FALSE.,NCOMBI,MTRYMX,STARTV,DX,BSPL0,  INERP065
     3             IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,     INERP066
     4             QT(1),VT(1),IERROR,.FALSE.,RESPON)                   INERP067
      IF (VT(1) .GE. VAROLD) GO TO 200                                  INERP068
         QT(2)=4.*QT(1)                                                 INERP069
         IF (QT(1) .LE. .125*QG) GO TO 210                              INERP070
         QT(2)=0.                                                       INERP071
         VT(2)=VAROLD                                                   INERP072
         QT(3)=QG                                                       INERP073
         VT(3)=VARG                                                     INERP074
         GO TO 230                                                      INERP075
 200  QT(2)=.5*QT(1)                                                    INERP076
      QT(2)=AMIN1(QT(2),.05)                                            INERP077
 210  CALL VARIAN (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,PLNTRY, INERP078
     1             PLAM,PLMTRY,PIVLIN,EK,ETE,EKEK,R,DELTAL,DTOLER,      INERP079
     2             DELTAN,ASAVE,.FALSE.,NCOMBI,MTRYMX,STARTV,DX,BSPL0,  INERP080
     3             IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,     INERP081
     4             QT(2),VT(2),IERROR,.FALSE.,RESPON)                   INERP082
      IF (VT(1).GE.VAROLD .OR. VT(2).GE.VT(1)) GO TO 220                INERP083
         QT(3)=QG                                                       INERP084
         VT(3)=VARG                                                     INERP085
         GO TO 230                                                      INERP086
 220  QT(3)=0.                                                          INERP087
      VT(3)=VAROLD                                                      INERP088
C230  VARG=AMIN1(VT(1),VT(2),VARG)                                      INERP089
 230  VARG=DMIN1(VT(1),VT(2),VARG)                                      INERP090
      IF (VT(1) .LE. VARG) QG=QT(1)                                     INERP091
      IF (VT(2) .LE. VARG) QG=QT(2)                                     INERP092
      DUM =(VT(1)-VT(3))/(QT(1)-QT(3))                                  INERP093
      DDUM=(DUM-(VT(2)-VT(3))/(QT(2)-QT(3)))/(QT(1)-QT(2))              INERP094
      IF (DDUM .GT. 0.) GO TO 250                                       INERP095
 240     Q=AMIN1(Q,.125*QT(2),.01)                                      INERP096
         RETURN                                                         INERP097
 250  DUM=.5*((QT(1)+QT(3))-DUM/DDUM)                                   INERP098
      IF (DUM .LE. 0.) GO TO 240                                        INERP099
      Q=AMIN1(DUM,8.,QMAX)                                              INERP100
      RETURN                                                            INERP101
      END                                                               INERP102
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++INERV001
      SUBROUTINE INTERV (Z,DX,IBETA,DZ,ZSTART)                          INERV002
C-----------------------------------------------------------------------INERV003
C     EVAUALTES NORMALIZED ABSCISSA DX AND STARTING INDEX IBETA FOR     INERV004
C     B-SPLINE COEFFICIENTS FOR LATER USE IN BSPLIN.                    INERV005
C-----------------------------------------------------------------------INERV006
C                                                                       INERV007
C      DESCRIPTION OF PARAMETERS:                                       INERV008
C                                                                       INERV009
C           Z      - ACTUAL POINT ON ZGRID WHERE TO INTERPOLATE         INERV010
C           DX     - NORMALIZED DISTANCE BETWEEN GRID POINTS,           INERV011
C                    I.E.  0 < DX < 1                                   INERV012
C           IBETA  - STARTING INDEX FOR B-SPLINE COEFFICIENTS           INERV013
C           DZ     - DISTANCE BETWEEN KNOTS ON INTERPOLATION GRID,      INERV014
C                    I.E.  DZ=ZGRID(I+1)-ZGRID(I)                       INERV015
C                                                                       INERV016
C-----------------------------------------------------------------------INERV017
C     SUBPROGRAMS CALLED - NONE                                         INERV018
C     COMMON USED        - NONE                                         INERV019
C-----------------------------------------------------------------------INERV020
      DOUBLE PRECISION DX, FLOAT, DZ, ZSTART                            INERV021
C-----------------------------------------------------------------------INERV022
      INT(DZ) =IDINT(DZ)                                                INERV023
      FLOAT(I)=DFLOAT(I)                                                INERV024
C-----------------------------------------------------------------------INERV025
      DX   =Z-ZSTART                                                    INERV026
      IBETA=INT(DX/DZ)                                                  INERV027
      DX   =(DX-FLOAT(IBETA)*DZ)/DZ                                     INERV028
      IBETA=IBETA+1                                                     INERV029
      RETURN                                                            INERV030
      END                                                               INERV031
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++LIEQS001
      SUBROUTINE LINEQS (NYMAX,Y,T,SQRTW,NY,NLINMX,NONLMX,PLIN,PLAM,EK, LIEQS002
     1                   EKEK,R,BSPL0,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,   LIEQS003
     2                   SGGCAP,SGYCAP,ZERO,RESPON)                     LIEQS004
C-----------------------------------------------------------------------LIEQS005
C     BUILDS UPPER TRIANGLE OF NORMAL EQUATION MATRIX EKEK WITH RIGHT   LIEQS006
C     HAND SIDE R, FOR SOLVING LINEAR LEAST SQUARES PROBLEM.            LIEQS007
C-----------------------------------------------------------------------LIEQS008
C     SUBPROGRAMS CALLED - USERFL, USERFN                               LIEQS009
C     WHICH IN TURN CALL - ERRMES, USERTR                               LIEQS010
C     COMMON USED        - IBLOCK, LBLOCK                               LIEQS011
C-----------------------------------------------------------------------LIEQS012
      DOUBLE PRECISION EK, EKEK, R, BSPL0, CCAP, ECAP, YCAP, SGGCAP,    LIEQS013
     1                 SGYCAP, ZERO, SUM1, SUM2, SWK, SWK2, YLYFIT      LIEQS014
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  LIEQS015
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    LIEQS016
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  LIEQS017
     3                 SPLINE, WRTBET, WRTCAP                           LIEQS018
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), SQRTW(NYMAX,1), NY(*),   LIEQS019
     1                 PLIN(NLINMX,1), PLAM(NONLMX), EK(NLINMX),        LIEQS020
     2                 EKEK(NLINMX,NLINMX,1), R(NLINMX,1),              LIEQS021
     3                 BSPL0(4,NONLMX), IC(NONLMX), CCAP(NZMAX,NZMAX,1),LIEQS022
     4                 ECAP(NZMAX,NGAMMX,1), YCAP(NZMAX,1),             LIEQS023
     5                 SGGCAP(NGAMMX,NGAMMX,1), SGYCAP(NGAMMX,1),       LIEQS024
     6                 RESPON(*)                                        LIEQS025
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       LIEQS026
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       LIEQS027
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   LIEQS028
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    LIEQS029
     4                 MTRY(10,2), MXITER(2), NL(10),                   LIEQS030
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  LIEQS031
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, LIEQS032
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       LIEQS033
     8                 NIN, NOUT                                        LIEQS034
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          LIEQS035
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         LIEQS036
     2                 DOUSOU(2), LUSER(30),                            LIEQS037
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  LIEQS038
     4                 SPLINE, WRTBET, WRTCAP                           LIEQS039
C-----------------------------------------------------------------------LIEQS040
      IF (SPLINE) GO TO 400                                             LIEQS041
C-----------------------------------------------------------------------LIEQS042
C  USE EXACT FUNCTION EVALUATIONS                                       LIEQS043
C  CASE  ND = 1                                                         LIEQS044
C-----------------------------------------------------------------------LIEQS045
      DO 120 N=1,ND                                                     LIEQS046
      DO 120 I=1,NLIN                                                   LIEQS047
         DO 110 J=I,NLIN                                                LIEQS048
 110     EKEK(I,J,N)=ZERO                                               LIEQS049
         R(I,N)=ZERO                                                    LIEQS050
 120  CONTINUE                                                          LIEQS051
      MY=NY(1)                                                          LIEQS052
      DO 200 K=1,MY                                                     LIEQS053
         KK  =K                                                         LIEQS054
         SWK =SQRTW(K,1)                                                LIEQS055
         SWK2=SWK*SWK                                                   LIEQS056
         DO 140 I=1,NLIN                                                LIEQS057
            IF (I .LE. NGAM) GO TO 130                                  LIEQS058
               NU   =I-NGAM                                             LIEQS059
               EK(I)=USERFN(PLAM(NU),NU,NYMAX,T,NY,KK,1,0,RESPON)       LIEQS060
               GO TO 140                                                LIEQS061
 130        NU   =I                                                     LIEQS062
            EK(I)=USERFL(NU,NYMAX,T,NY,KK,1,RESPON)                     LIEQS063
 140     CONTINUE                                                       LIEQS064
         YHAT=ZERO                                                      LIEQS065
         DO 150 I=1,NLIN                                                LIEQS066
            DO 145 J=I,NLIN                                             LIEQS067
 145        EKEK(I,J,1)=EKEK(I,J,1)+SWK2*EK(I)*EK(J)                    LIEQS068
            YHAT=YHAT+EK(I)*PLIN(I,1)                                   LIEQS069
 150     CONTINUE                                                       LIEQS070
         YLYFIT=SWK*(Y(K,1)-YHAT)                                       LIEQS071
         DO 155 I=1,NLIN                                                LIEQS072
 155     R(I,1)=R(I,1)+YLYFIT*SWK*EK(I)                                 LIEQS073
         IF (NSTORD) GO TO 200                                          LIEQS074
C-----------------------------------------------------------------------LIEQS075
C  CASE  ND > 1  .AND.  SAMET=.TRUE.                                    LIEQS076
C-----------------------------------------------------------------------LIEQS077
         DO 180 N=2,ND                                                  LIEQS078
            SWK =SQRTW(K,N)                                             LIEQS079
            SWK2=SWK*SWK                                                LIEQS080
            YHAT=ZERO                                                   LIEQS081
            DO 160 I=1,NLIN                                             LIEQS082
 160        YHAT=YHAT+EK(I)*PLIN(I,N)                                   LIEQS083
            YLYFIT=SWK*(Y(K,N)-YHAT)                                    LIEQS084
            DO 170 I=1,NLIN                                             LIEQS085
               R(I,N)=R(I,N)+YLYFIT*SWK*EK(I)                           LIEQS086
               IF (IWTIST) GO TO 170                                    LIEQS087
               DO 165 J=I,NLIN                                          LIEQS088
 165           EKEK(I,J,N)=EKEK(I,J,N)+SWK2*EK(I)*EK(J)                 LIEQS089
 170        CONTINUE                                                    LIEQS090
 180     CONTINUE                                                       LIEQS091
 200  CONTINUE                                                          LIEQS092
      IF (NSTORD .OR. .NOT.IWTIST) GO TO 220                            LIEQS093
C-----------------------------------------------------------------------LIEQS094
C  CASE  ND > 1  .AND.  SAMET=.TRUE.  .AND.  IWTIST=.TRUE.              LIEQS095
C-----------------------------------------------------------------------LIEQS096
      DO 210 N=2,ND                                                     LIEQS097
      DO 210 I=1,NLIN                                                   LIEQS098
      DO 210 J=I,NLIN                                                   LIEQS099
 210  EKEK(I,J,N)=EKEK(I,J,1)                                           LIEQS100
 220  IF (ND.EQ.1 .OR. SAMET) RETURN                                    LIEQS101
C-----------------------------------------------------------------------LIEQS102
C  CASE  ND > 1  .AND.  SAMET=.FALSE.                                   LIEQS103
C-----------------------------------------------------------------------LIEQS104
      DO 350 N=2,ND                                                     LIEQS105
         NSET=N                                                         LIEQS106
         MY  =NY(N)                                                     LIEQS107
         DO 300 K=1,MY                                                  LIEQS108
            KK  =K                                                      LIEQS109
            SWK =SQRTW(K,N)                                             LIEQS110
            SWK2=SWK*SWK                                                LIEQS111
            DO 240 I=1,NLIN                                             LIEQS112
               IF (I .LE. NGAM) GO TO 230                               LIEQS113
                  NU   =I-NGAM                                          LIEQS114
                  EK(I)=USERFN(PLAM(NU),NU,NYMAX,T,NY,KK,NSET,0,RESPON) LIEQS115
               GO TO 240                                                LIEQS116
 230           NU   =I                                                  LIEQS117
               EK(I)=USERFL(NU,NYMAX,T,NY,KK,NSET,RESPON)               LIEQS118
 240        CONTINUE                                                    LIEQS119
            YHAT=ZERO                                                   LIEQS120
            DO 280 I=1,NLIN                                             LIEQS121
               DO 270 J=I,NLIN                                          LIEQS122
 270           EKEK(I,J,N)=EKEK(I,J,N)+SWK2*EK(I)*EK(J)                 LIEQS123
               YHAT=YHAT+EK(I)*PLIN(I,N)                                LIEQS124
 280        CONTINUE                                                    LIEQS125
            YLYFIT=SWK*(Y(K,N)-YHAT)                                    LIEQS126
            DO 290 I=1,NLIN                                             LIEQS127
 290        R(I,N)=R(I,N)+YLYFIT*SWK*EK(I)                              LIEQS128
 300     CONTINUE                                                       LIEQS129
 350  CONTINUE                                                          LIEQS130
      RETURN                                                            LIEQS131
C-----------------------------------------------------------------------LIEQS132
C  USE SPLINE APPROXIMATIONS                                            LIEQS133
C  CASE  ND = 1                                                         LIEQS134
C-----------------------------------------------------------------------LIEQS135
 400  IF (NGAM .EQ. 0) GO TO 450                                        LIEQS136
         DO 440 MU=1,NGAM                                               LIEQS137
            DO 410 NU=MU,NGAM                                           LIEQS138
 410        EKEK(MU,NU,1)=SGGCAP(MU,NU,1)                               LIEQS139
            DO 430 J=1,NONL                                             LIEQS140
               JG  =J+NGAM                                              LIEQS141
               SUM1=ZERO                                                LIEQS142
               LL  =IC(J)-1                                             LIEQS143
               DO 420 L=1,IB                                            LIEQS144
                  LL  =LL+1                                             LIEQS145
                  SUM1=SUM1+BSPL0(L,J)*ECAP(LL,MU,1)                    LIEQS146
 420           CONTINUE                                                 LIEQS147
               EKEK(MU,JG,1)=SUM1                                       LIEQS148
 430        CONTINUE                                                    LIEQS149
 440     CONTINUE                                                       LIEQS150
 450  DO 500 I=1,NONL                                                   LIEQS151
         IG=I+NGAM                                                      LIEQS152
         DO 490 J=I,NONL                                                LIEQS153
            JG  =J+NGAM                                                 LIEQS154
            SUM1=ZERO                                                   LIEQS155
            LL  =IC(I)-1                                                LIEQS156
            DO 480 L=1,IB                                               LIEQS157
               LL  =LL+1                                                LIEQS158
               SUM2=ZERO                                                LIEQS159
               MM  =IC(J)-1                                             LIEQS160
               DO 470 M=1,IB                                            LIEQS161
                  MM  =MM+1                                             LIEQS162
                  SUM2=SUM2+BSPL0(M,J)*CCAP(LL,MM,1)                    LIEQS163
 470           CONTINUE                                                 LIEQS164
               SUM1=SUM1+BSPL0(L,I)*SUM2                                LIEQS165
 480        CONTINUE                                                    LIEQS166
            EKEK(IG,JG,1)=SUM1                                          LIEQS167
 490     CONTINUE                                                       LIEQS168
 500  CONTINUE                                                          LIEQS169
      IF (NLIN .EQ. 1) GO TO 530                                        LIEQS170
         DO 520 I=2,NLIN                                                LIEQS171
            L=I-1                                                       LIEQS172
            DO 510 J=1,L                                                LIEQS173
 510        EKEK(I,J,1)=EKEK(J,I,1)                                     LIEQS174
 520     CONTINUE                                                       LIEQS175
 530  IF (NGAM .EQ. 0) GO TO 600                                        LIEQS176
         DO 590 MU=1,NGAM                                               LIEQS177
            R(MU,1)=SGYCAP(MU,1)                                        LIEQS178
            SUM1   =ZERO                                                LIEQS179
            DO 580 NU=1,NGAM                                            LIEQS180
 580        SUM1=SUM1+EKEK(NU,MU,1)*PLIN(NU,1)                          LIEQS181
            R(MU,1)=R(MU,1)-SUM1                                        LIEQS182
            SUM1   =ZERO                                                LIEQS183
            DO 585 J=1,NONL                                             LIEQS184
               JG  =J+NGAM                                              LIEQS185
               SUM1=SUM1+EKEK(MU,JG,1)*PLIN(JG,1)                       LIEQS186
 585        CONTINUE                                                    LIEQS187
            R(MU,1)=R(MU,1)-SUM1                                        LIEQS188
 590     CONTINUE                                                       LIEQS189
 600  DO 630 J=1,NONL                                                   LIEQS190
         JG  =J+NGAM                                                    LIEQS191
         SUM1=ZERO                                                      LIEQS192
         LL  =IC(J)-1                                                   LIEQS193
         DO 610 L=1,IB                                                  LIEQS194
            LL  =LL+1                                                   LIEQS195
            SUM1=SUM1+BSPL0(L,J)*YCAP(LL,1)                             LIEQS196
 610     CONTINUE                                                       LIEQS197
         IF (NGAM .EQ. 0) GO TO 620                                     LIEQS198
            DO 615 NU=1,NGAM                                            LIEQS199
 615        SUM1=SUM1-EKEK(NU,JG,1)*PLIN(NU,1)                          LIEQS200
 620     DO 625 I=1,NONL                                                LIEQS201
            IG  =I+NGAM                                                 LIEQS202
            SUM1=SUM1-EKEK(IG,JG,1)*PLIN(IG,1)                          LIEQS203
 625     CONTINUE                                                       LIEQS204
         R(JG,1)=SUM1                                                   LIEQS205
 630  CONTINUE                                                          LIEQS206
      IF (ND .EQ. 1) RETURN                                             LIEQS207
C-----------------------------------------------------------------------LIEQS208
C  CASE  ND > 1                                                         LIEQS209
C-----------------------------------------------------------------------LIEQS210
      DO 950 N=2,ND                                                     LIEQS211
         IF (.NOT.SAMET .OR. .NOT.IWTIST) GO TO 650                     LIEQS212
C-----------------------------------------------------------------------LIEQS213
C  CASE  ND > 1  .AND.  SAMET=.TRUE.  .AND.  IWTITS=.TRUE.              LIEQS214
C-----------------------------------------------------------------------LIEQS215
            DO 640 I=1,NLIN                                             LIEQS216
            DO 640 J=I,NLIN                                             LIEQS217
 640        EKEK(I,J,N)=EKEK(I,J,1)                                     LIEQS218
            GO TO 810                                                   LIEQS219
C-----------------------------------------------------------------------LIEQS220
C  CASE  ND > 1  .AND.  (SAMET=.FALSE.  .OR.  IWTITS=.FALSE.)           LIEQS221
C-----------------------------------------------------------------------LIEQS222
 650     IF (NGAM .EQ. 0) GO TO 750                                     LIEQS223
            DO 740 MU=1,NGAM                                            LIEQS224
               DO 710 NU=MU,NGAM                                        LIEQS225
 710           EKEK(MU,NU,N)=SGGCAP(MU,NU,N)                            LIEQS226
               DO 730 J=1,NONL                                          LIEQS227
                  JG  =J+NGAM                                           LIEQS228
                  SUM1=ZERO                                             LIEQS229
                  LL  =IC(J)-1                                          LIEQS230
                  DO 720 L=1,IB                                         LIEQS231
                     LL  =LL+1                                          LIEQS232
                     SUM1=SUM1+BSPL0(L,J)*ECAP(LL,MU,N)                 LIEQS233
 720              CONTINUE                                              LIEQS234
                  EKEK(MU,JG,N)=SUM1                                    LIEQS235
 730           CONTINUE                                                 LIEQS236
 740        CONTINUE                                                    LIEQS237
 750     DO 800 I=1,NONL                                                LIEQS238
            IG=I+NGAM                                                   LIEQS239
            DO 790 J=I,NONL                                             LIEQS240
               JG  =J+NGAM                                              LIEQS241
               SUM1=ZERO                                                LIEQS242
               LL  =IC(I)-1                                             LIEQS243
               DO 780 L=1,IB                                            LIEQS244
                  LL  =LL+1                                             LIEQS245
                  SUM2=ZERO                                             LIEQS246
                  MM  =IC(J)-1                                          LIEQS247
                  DO 770 M=1,IB                                         LIEQS248
                     MM  =MM+1                                          LIEQS249
                     SUM2=SUM2+BSPL0(M,J)*CCAP(LL,MM,N)                 LIEQS250
 770              CONTINUE                                              LIEQS251
                  SUM1=SUM1+BSPL0(L,I)*SUM2                             LIEQS252
 780           CONTINUE                                                 LIEQS253
               EKEK(IG,JG,N)=SUM1                                       LIEQS254
 790        CONTINUE                                                    LIEQS255
 800     CONTINUE                                                       LIEQS256
 810     IF (NLIN .EQ. 1) GO TO 830                                     LIEQS257
            DO 825 I=2,NLIN                                             LIEQS258
               L=I-1                                                    LIEQS259
               DO 820 J=1,L                                             LIEQS260
 820           EKEK(I,J,N)=EKEK(J,I,N)                                  LIEQS261
 825        CONTINUE                                                    LIEQS262
 830     IF (NGAM .EQ. 0) GO TO 900                                     LIEQS263
            DO 890 MU=1,NGAM                                            LIEQS264
               R(MU,N)=SGYCAP(MU,N)                                     LIEQS265
               SUM1   =ZERO                                             LIEQS266
               DO 880 NU=1,NGAM                                         LIEQS267
 880           SUM1=SUM1+EKEK(NU,MU,N)*PLIN(NU,N)                       LIEQS268
               R(MU,N)=R(MU,N)-SUM1                                     LIEQS269
               SUM1   =ZERO                                             LIEQS270
               DO 885 J=1,NONL                                          LIEQS271
                  JG  =J+NGAM                                           LIEQS272
                  SUM1=SUM1+EKEK(MU,JG,N)*PLIN(JG,N)                    LIEQS273
 885           CONTINUE                                                 LIEQS274
               R(MU,N)=R(MU,N)-SUM1                                     LIEQS275
 890        CONTINUE                                                    LIEQS276
 900     DO 930 J=1,NONL                                                LIEQS277
            JG  =J+NGAM                                                 LIEQS278
            SUM1=ZERO                                                   LIEQS279
            LL  =IC(J)-1                                                LIEQS280
            DO 910 L=1,IB                                               LIEQS281
               LL  =LL+1                                                LIEQS282
               SUM1=SUM1+BSPL0(L,J)*YCAP(LL,N)                          LIEQS283
 910        CONTINUE                                                    LIEQS284
            IF (NGAM .EQ. 0) GO TO 920                                  LIEQS285
               DO 915 NU=1,NGAM                                         LIEQS286
 915           SUM1=SUM1-EKEK(NU,JG,N)*PLIN(NU,N)                       LIEQS287
 920        DO 925 I=1,NONL                                             LIEQS288
               IG  =I+NGAM                                              LIEQS289
               SUM1=SUM1-EKEK(IG,JG,N)*PLIN(IG,N)                       LIEQS290
 925        CONTINUE                                                    LIEQS291
            R(JG,N)=SUM1                                                LIEQS292
 930     CONTINUE                                                       LIEQS293
 950  CONTINUE                                                          LIEQS294
      RETURN                                                            LIEQS295
      END                                                               LIEQS296
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++LIPAR001
      SUBROUTINE LINPAR (NYMAX,Y,T,SQRTW,NY,NLINMX,NONLMX,PLIN,PLNTRY,  LIPAR002
     1                   PLAM,PIVLIN,EK,ETE,EKEK,R,DELTAL,DTOLER,ASAVE, LIPAR003
     2                   SAVEA,DX,BSPL0,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP, LIPAR004
     3                   SGGCAP,SGYCAP,ND,NLIN,NONL,ALPLIN,SPLINE,      LIPAR005
     4                   IERROR,RESPON)                                 LIPAR006
C-----------------------------------------------------------------------LIPAR007
C     SOLVES LINEAR LEAST SQUARES PROBLEM. RETURNS OPTIMAL              LIPAR008
C     LINEAR PARAMETERS PLIN(I,N), I=1,...,NLIN , N=1,...,ND            LIPAR009
C     WITH RESPECT TO CONSTRAINTS PLMNMX(1) AND PLMNMX(2)               LIPAR010
C-----------------------------------------------------------------------LIPAR011
C     SUBPROGRAMS CALLED - BSPLIN, INTERV, LINEQS, PIVOT                LIPAR012
C     WHICH IN TURN CALL - ERRMES, PIVOT1, USERFL, USERFN, USERTR       LIPAR013
C     COMMON USED        - DBLOCK, SBLOCK                               LIPAR014
C-----------------------------------------------------------------------LIPAR015
      DOUBLE PRECISION PRECIS, RANGE,                                   LIPAR016
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       LIPAR017
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           LIPAR018
     3                 TWOTHR, FORTHR, SIXTEL                           LIPAR019
      DOUBLE PRECISION EK, ETE, EKEK, R, DELTAL, DTOLER, ASAVE, DX,     LIPAR020
     1                 BSPL0, CCAP, ECAP, YCAP, SGGCAP, SGYCAP,         LIPAR021
     2                 RNGINV, ABS                                      LIPAR022
      LOGICAL          PIVLIN, SAVEA, INVERT, ALPLIN, SPLINE            LIPAR023
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), SQRTW(NYMAX,1), NY(*),   LIPAR024
     1                 PLIN(NLINMX,1), PLNTRY(NLINMX,1), PLAM(NONLMX),  LIPAR025
     2                 PIVLIN(NLINMX,1), EK(NLINMX), ETE(NLINMX,NLINMX),LIPAR026
     3                 EKEK(NLINMX,NLINMX,1), R(NLINMX,1), RESPON(*),   LIPAR027
     4                 DELTAL(NLINMX), DTOLER(NLINMX),                  LIPAR028
     5                 ASAVE(NLINMX,NLINMX,1), DX(NONLMX),              LIPAR029
     6                 BSPL0(4,NONLMX), IC(NONLMX), CCAP(NZMAX,NZMAX,1),LIPAR030
     7                 ECAP(NZMAX,NGAMMX,1), YCAP(NZMAX,1),             LIPAR031
     8                 SGGCAP(NGAMMX,NGAMMX,1), SGYCAP(NGAMMX,1)        LIPAR032
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        LIPAR033
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, LIPAR034
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 LIPAR035
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),LIPAR036
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     LIPAR037
C-----------------------------------------------------------------------LIPAR038
      ABS(TEN)=DABS(TEN)                                                LIPAR039
C-----------------------------------------------------------------------LIPAR040
      RNGINV=ONE/RANGE                                                  LIPAR041
      NLINP1=NLIN+1                                                     LIPAR042
      IERROR=0                                                          LIPAR043
      IF (.NOT. SPLINE) GO TO 150                                       LIPAR044
C-----------------------------------------------------------------------LIPAR045
C  EVALUATE STARTING INDEX IC(J) FOR B-SPLINE COEFFICIENTS              LIPAR046
C-----------------------------------------------------------------------LIPAR047
         DO 100 J=1,NONL                                                LIPAR048
 100     CALL INTERV (PLAM(J),DX(J),IC(J),DZ,ZSTART)                    LIPAR049
C-----------------------------------------------------------------------LIPAR050
C  COMPUTE IB*NONL B-SPLINES (IB=4, I.E. CUBIC B-SPLINES)               LIPAR051
C-----------------------------------------------------------------------LIPAR052
         DO 120 J=1,NONL                                                LIPAR053
 120     CALL BSPLIN (BSPL0(1,J),DX(J),0)                               LIPAR054
C-----------------------------------------------------------------------LIPAR055
C  BUILD UP NORMAL EQUAITION MATRIX EKEK WITH RIGHT HAND SIDE R         LIPAR056
C-----------------------------------------------------------------------LIPAR057
 150  CALL LINEQS (NYMAX,Y,T,SQRTW,NY,NLINMX,NONLMX,PLIN,PLAM,EK,EKEK,  LIPAR058
     1             R,BSPL0,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,       LIPAR059
     2             SGYCAP,ZERO,RESPON)                                  LIPAR060
C-----------------------------------------------------------------------LIPAR061
C  CASE  ND = 1                                                         LIPAR062
C  BUILD UP AUGUMENTED NORMAL EQUAITION MATRIX ETE, AND SET TOLERANCE   LIPAR063
C  VALUES FOR DIAGONAL ELEMENTS OF ETE, TO DECIDE IF THEY COULD BE USED LIPAR064
C  AS PIVOTS OR NOT, AND CHECK IF RIGHT HAND SIDE IS NEARLY ZERO        LIPAR065
C-----------------------------------------------------------------------LIPAR066
      INVERT=.FALSE.                                                    LIPAR067
      DO 200 I=1,NLIN                                                   LIPAR068
         DO 190 J=I,NLIN                                                LIPAR069
 190     ETE(I,J)=EKEK(I,J,1)                                           LIPAR070
         DTOLER(I)=TEN*PRECIS*ETE(I,I)                                  LIPAR071
         ETE(I,NLINP1)=R(I,1)                                           LIPAR072
         IF (ABS(R(I,1)) .GT. RNGINV) GO TO 200                         LIPAR073
         INVERT=.TRUE.                                                  LIPAR074
 200  CONTINUE                                                          LIPAR075
      ETE(NLINP1,NLINP1)=ZERO                                           LIPAR076
C-----------------------------------------------------------------------LIPAR077
C  SOLVE FOR LINEAR PARAMETERS PLIN(I,1), I=1,...,NLIN                  LIPAR078
C-----------------------------------------------------------------------LIPAR079
      CALL PIVOT (ETE,NLINMX,NLIN,INVERT,PLMNMX(1),PLMNMX(2),PLIN(1,1), LIPAR080
     1           DTOLER,DELTAL,PIVLIN(1,1),ALPLIN,QG,QMAX,SRANGE,NERROR)LIPAR081
      IF (ALPLIN) GO TO 350                                             LIPAR082
         IERROR=4                                                       LIPAR083
         IF (NERROR .EQ. 0) GO TO 350                                   LIPAR084
         QG=0.                                                          LIPAR085
         GO TO 450                                                      LIPAR086
C-----------------------------------------------------------------------LIPAR087
C  SAVE INVERSE OF ETE IF SAVEA=.TRUE.                                  LIPAR088
C-----------------------------------------------------------------------LIPAR089
 350  IF (.NOT. SAVEA) GO TO 450                                        LIPAR090
         DO 400 I=1,NLIN                                                LIPAR091
         DO 400 J=1,NLIN                                                LIPAR092
 400     ASAVE(I,J,1)=ETE(I,J)                                          LIPAR093
C-----------------------------------------------------------------------LIPAR094
C  CORRECT LINEAR PARAMETERS PLIN                                       LIPAR095
C-----------------------------------------------------------------------LIPAR096
 450  IF (INVERT) QG=0.                                                 LIPAR097
      DO 500 I=1,NLIN                                                   LIPAR098
 500  PLNTRY(I,1)=PLIN(I,1)+QG*DELTAL(I)                                LIPAR099
      IF (ND .EQ. 1) RETURN                                             LIPAR100
C-----------------------------------------------------------------------LIPAR101
C  CASE  ND > 1                                                         LIPAR102
C-----------------------------------------------------------------------LIPAR103
      DO 800 N=2,ND                                                     LIPAR104
C-----------------------------------------------------------------------LIPAR105
C  BUILD UP AUGUMENTED NORMAL EQUAITION MATRIX ETE, AND SET TOLERANCE   LIPAR106
C  VALUES FOR DIAGONAL ELEMENTS OF ETE, TO DECIDE IF THEY COULD BE USED LIPAR107
C  AS PIVOTS OR NOT, AND CHECK IF RIGHT HAND SIDE IS NEARLY ZERO        LIPAR108
C-----------------------------------------------------------------------LIPAR109
         INVERT=.FALSE.                                                 LIPAR110
         DO 610 I=1,NLIN                                                LIPAR111
            DO 600 J=I,NLIN                                             LIPAR112
 600        ETE(I,J)=EKEK(I,J,N)                                        LIPAR113
            DTOLER(I)=TEN*PRECIS*ETE(I,I)                               LIPAR114
            ETE(I,NLINP1)=R(I,N)                                        LIPAR115
            IF (ABS(R(I,N)) .GT. RNGINV) GO TO 610                      LIPAR116
            INVERT=.TRUE.                                               LIPAR117
 610     CONTINUE                                                       LIPAR118
         ETE(NLINP1,NLINP1)=ZERO                                        LIPAR119
C-----------------------------------------------------------------------LIPAR120
C  SOLVE FOR LINEAR PARAMETERS PLIN(I,N), I=1,...,NLIN                  LIPAR121
C-----------------------------------------------------------------------LIPAR122
         CALL PIVOT (ETE,NLINMX,NLIN,INVERT,PLMNMX(1),PLMNMX(2),        LIPAR123
     1               PLIN(1,N),DTOLER,DELTAL,PIVLIN(1,N),ALPLIN,QG,     LIPAR124
     2               QMAX,SRANGE,NERROR)                                LIPAR125
         IF (ALPLIN) GO TO 680                                          LIPAR126
            IERROR=4                                                    LIPAR127
            IF (NERROR .EQ. 0) GO TO 680                                LIPAR128
            QG=0.                                                       LIPAR129
            GO TO 720                                                   LIPAR130
C-----------------------------------------------------------------------LIPAR131
C  SAVE INVERSE OF ETE IF SAVEA=.TRUE.                                  LIPAR132
C-----------------------------------------------------------------------LIPAR133
 680  IF (.NOT. SAVEA) GO TO 720                                        LIPAR134
         DO 700 I=1,NLIN                                                LIPAR135
         DO 700 J=1,NLIN                                                LIPAR136
 700     ASAVE(I,J,N)=ETE(I,J)                                          LIPAR137
C-----------------------------------------------------------------------LIPAR138
C  CORRECT LINEAR PARAMETERS PLIN                                       LIPAR139
C-----------------------------------------------------------------------LIPAR140
 720     IF (INVERT) QG=0.                                              LIPAR141
         DO 750 I=1,NLIN                                                LIPAR142
 750     PLNTRY(I,N)=PLIN(I,N)+QG*DELTAL(I)                             LIPAR143
 800  CONTINUE                                                          LIPAR144
      RETURN                                                            LIPAR145
      END                                                               LIPAR146
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++LSSQR001
      SUBROUTINE LSTSQR (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,  LSSQR002
     1                   PLNTRY,PLAM,PLMTRY,PIVLIN,PIVLAM,EK,EPK,ETE,   LSSQR003
     2                   EKEK,R,DELTAL,DTOLER,DELTAN,GTG,H,DELTAH,ASAVE,LSSQR004
     3                   NCOMBI,MTRYMX,STARTV,DX,BSPL0,BSPL1,BSPL2,IC,  LSSQR005
     4                   NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,  LSSQR006
     5                   VAR,ISTAGE,IERROR,RESPON,BUFF)                 LSSQR007
C-----------------------------------------------------------------------LSSQR008
C     SOLVES LEAST SQUARES PROBLEM (LINEAR AND NONLINEAR) USING         LSSQR009
C     METHOD OF STEPWISE REGRESSION.                                    LSSQR010
C-----------------------------------------------------------------------LSSQR011
C     SUBPROGRAMS CALLED - ERRMES, HESEXA, HESSPL, INTERP, OUTITR,      LSSQR012
C                          PIVOT,  USEROU, VARIAN                       LSSQR013
C     WHICH IN TURN CALL - BSPLIN, ERRMES, HESSEN, INTERV, LINEQS,      LSSQR014
C                          LINPAR, NPASCL, PIVOT,  PIVOT1, USERFL,      LSSQR015
C                          USERFN, USERTR, VARIAN                       LSSQR016
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               LSSQR017
C-----------------------------------------------------------------------LSSQR018
      DOUBLE PRECISION PRECIS, RANGE,                                   LSSQR019
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       LSSQR020
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           LSSQR021
     3                 TWOTHR, FORTHR, SIXTEL                           LSSQR022
      DOUBLE PRECISION EK, EPK, ETE, EKEK, R, DELTAL, DTOLER,           LSSQR023
     1                 DELTAN, GTG, H, DELTAH, ASAVE, DX, BSPL0,        LSSQR024
     2                 BSPL1, BSPL2, CCAP, ECAP, YCAP, SGGCAP,          LSSQR025
     3                 SGYCAP, YW, VAR, VARG, DUB, ABS, AMAX1           LSSQR026
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  LSSQR027
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    LSSQR028
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  LSSQR029
     3                 SPLINE, WRTBET, WRTCAP                           LSSQR030
      LOGICAL          PIVLIN, PIVLAM, STARTV                           LSSQR031
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), YLYFIT(NYMAX,1),         LSSQR032
     1                 SQRTW(NYMAX,1), NY(*), RESPON(*), PLIN(NLINMX,1),LSSQR033
     2                 PLNTRY(NLINMX,1), PLAM(NONLMX), PLMTRY(NONLMX),  LSSQR034
     3                 PIVLIN(NLINMX,1), PIVLAM(NONLMX), EK(NLINMX),    LSSQR035
     4                 EPK(NONLMX), ETE(NLINMX,NLINMX),                 LSSQR036
     5                 EKEK(NLINMX,NLINMX,1), R(NLINMX,1),              LSSQR037
     6                 DELTAL(NLINMX), DTOLER(NLINMX), DELTAN(NONLMX)   LSSQR038
      DIMENSION        GTG(NONLMX,NONLMX), H(NLINMX,NLINMX,1),          LSSQR039
     1                 DELTAH(NONLMX,1), ASAVE(NLINMX,NLINMX,1),        LSSQR040
     2                 NCOMBI(NONLMX), STARTV(MTRYMX), DX(NONLMX),      LSSQR041
     3                 BSPL0(4,NONLMX), BSPL1(4,NONLMX),                LSSQR042
     4                 BSPL2(4,NONLMX), IC(NONLMX),                     LSSQR043
     5                 CCAP(NZMAX,NZMAX,1), ECAP(NZMAX,NGAMMX,1),       LSSQR044
     6                 YCAP(NZMAX,1), SGGCAP(NGAMMX,NGAMMX,1),          LSSQR045
     7                 SGYCAP(NGAMMX,1), YW(*), BUFF(*)                 LSSQR046
      DIMENSION        IHOLER(6)                                        LSSQR047
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        LSSQR048
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, LSSQR049
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 LSSQR050
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),LSSQR051
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     LSSQR052
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       LSSQR053
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       LSSQR054
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   LSSQR055
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    LSSQR056
     4                 MTRY(10,2), MXITER(2), NL(10),                   LSSQR057
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  LSSQR058
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, LSSQR059
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       LSSQR060
     8                 NIN, NOUT                                        LSSQR061
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          LSSQR062
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         LSSQR063
     2                 DOUSOU(2), LUSER(30),                            LSSQR064
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  LSSQR065
     4                 SPLINE, WRTBET, WRTCAP                           LSSQR066
      DATA             IHOLER/1HL, 1HS, 1HT, 1HS, 1HQ, 1HR/             LSSQR067
C-----------------------------------------------------------------------LSSQR068
      ABS(DUB)       =DABS(DUB)                                         LSSQR069
      AMAX1(DUB,ZERO)=DMAX1(DUB,ZERO)                                   LSSQR070
C-----------------------------------------------------------------------LSSQR071
C  PRINT HEADLINE IF IPRITR(ISTAGE) > 0                                 LSSQR072
C-----------------------------------------------------------------------LSSQR073
      IF (IPRITR(ISTAGE) .LE. 0) GO TO 160                              LSSQR074
         IF (NGAMM1) 120,130,140                                        LSSQR075
C-----------------------------------------------------------------------LSSQR076
C  CASE  NGAM = 0                                                       LSSQR077
C-----------------------------------------------------------------------LSSQR078
 120     WRITE (NOUT,1000) (ISTTXT(I),I=1,28),                          LSSQR079
     1   ((IALPHA(I),I=1,6),J,(ILAMDA(I),I=1,6),J,J=1,NPLINE)           LSSQR080
         IF (NONL .GT. 4) WRITE (NOUT,1100)                             LSSQR081
     1   ((IALPHA(I),I=1,6),J,(ILAMDA(I),I=1,6),J,J=5,NONL)             LSSQR082
         GO TO 160                                                      LSSQR083
C-----------------------------------------------------------------------LSSQR084
C  CASE  NGAM = 1                                                       LSSQR085
C-----------------------------------------------------------------------LSSQR086
 130     WRITE (NOUT,2000) (ISTTXT(I),I=1,28),(IGAMMA(I),I=1,6),NGAM,   LSSQR087
     2   ((IALPHA(I),I=1,6),J,(ILAMDA(I),I=1,6),J,J=1,NPLINE)           LSSQR088
         IF (NONL .GT. 4) WRITE (NOUT,2100)                             LSSQR089
     1   ((IALPHA(I),I=1,6),J,(ILAMDA(I),I=1,6),J,J=5,NONL)             LSSQR090
         GO TO 160                                                      LSSQR091
C-----------------------------------------------------------------------LSSQR092
C  CASE  NGAM > 1                                                       LSSQR093
C-----------------------------------------------------------------------LSSQR094
 140     WRITE (NOUT,3000) (ISTTXT(I),I=1,28),                          LSSQR095
     1                    ((IGAMMA(I),I=1,6),J,J=1,NPLINE)              LSSQR096
         IF (NGAM .GT. 8) WRITE (NOUT,3100)                             LSSQR097
     1                    ((IGAMMA(I),I=1,6),J,J=9,NGAM)                LSSQR098
         WRITE (NOUT,3100)                                              LSSQR099
     1         ((IALPHA(I),I=1,6),J,(ILAMDA(I),I=1,6),J,J=1,NONL)       LSSQR100
C-----------------------------------------------------------------------LSSQR101
C  INITIALIZATION                                                       LSSQR102
C-----------------------------------------------------------------------LSSQR103
 160  DO 180 J=1,NONL                                                   LSSQR104
         PIVLAM(J)=.TRUE.                                               LSSQR105
         DELTAN(J)=ZERO                                                 LSSQR106
 180  CONTINUE                                                          LSSQR107
      NCONV =0                                                          LSSQR108
      NVARUP=0                                                          LSSQR109
      VAROLD=RANGE                                                      LSSQR110
      VARG  =RANGE                                                      LSSQR111
      ITER  =-1                                                         LSSQR112
      Q     =0.                                                         LSSQR113
      QG    =0.                                                         LSSQR114
C-----------------------------------------------------------------------LSSQR115
C  START OF MAIN LOOP FOR LEAST SQUARES FIT                             LSSQR116
C-----------------------------------------------------------------------LSSQR117
 200  NTRY=1                                                            LSSQR118
 300  ITER=ITER+1                                                       LSSQR119
C-----------------------------------------------------------------------LSSQR120
C  CALCULATE VARIANCE VAR FOR GIVEN PLAM                                LSSQR121
C-----------------------------------------------------------------------LSSQR122
 400  CALL VARIAN (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,PLNTRY, LSSQR123
     1             PLAM,PLMTRY,PIVLIN,EK,ETE,EKEK,R,DELTAL,DTOLER,      LSSQR124
     2             DELTAN,ASAVE,.TRUE.,NCOMBI,MTRYMX,STARTV,DX,BSPL0,IC,LSSQR125
     3             NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,Q,VAR,  LSSQR126
     4             IERROR,.FALSE.,RESPON)                               LSSQR127
      IF (VAR.LE.VARG .OR. NTRY.EQ.2) GO TO 420                         LSSQR128
         NTRY=2                                                         LSSQR129
         Q   =QG                                                        LSSQR130
         GO TO 400                                                      LSSQR131
 420  DUB=VAR-VAROLD                                                    LSSQR132
C-----------------------------------------------------------------------LSSQR133
C  SET NEW PARAMETERS PLAM AND PLIN                                     LSSQR134
C-----------------------------------------------------------------------LSSQR135
      DO 430 J=1,NONL                                                   LSSQR136
 430  PLAM(J)=PLMTRY(J)                                                 LSSQR137
      DO 440 N=1,ND                                                     LSSQR138
      DO 440 J=1,NLIN                                                   LSSQR139
 440  PLIN(J,N)=PLNTRY(J,N)                                             LSSQR140
C-----------------------------------------------------------------------LSSQR141
C  PRINT OUT INTERMEDIATE RESULTS IF IPRITR(ISTAGE) > 0                 LSSQR142
C                                                                       LSSQR143
C  IF  IPRITR(ISTAGE) = 1  PRINT LAST ITERATION ONLY                    LSSQR144
C  IF  IPRITR(ISTAGE) = 2  PRINT FIRST AND LAST ITERATION               LSSQR145
C  IF  IPRITR(ISTAGE) = 3  PRINT EACH ITERATION                         LSSQR146
C-----------------------------------------------------------------------LSSQR147
      IF (DOUSOU(ISTAGE)) CALL USEROU (2,NYMAX,Y,T,YLYFIT,SQRTW,NY,     LSSQR148
     1    NLINMX,NONLMX,PLIN,PLAM,PLMTRY,BUFF,VAR,ISTAGE,RESPON)        LSSQR149
      IF (IPRITR(ISTAGE) .LE. 1) GO TO 480                              LSSQR150
         IF (IPRITR(ISTAGE).EQ.2 .AND. ITER.GT.0) GO TO 480             LSSQR151
         CALL OUTITR (NLINMX,NONLMX,PLIN,PLAM,PLMTRY,EK,EPK,PIVLIN,     LSSQR152
     1                PIVLAM,ITER,VAR,Q,DUB,NTRY,ZERO)                  LSSQR153
C-----------------------------------------------------------------------LSSQR154
C  TEST FOR CONVERGENCE                                                 LSSQR155
C-----------------------------------------------------------------------LSSQR156
 480  IF (ABS(DUB) .GT. AMAX1(VARMIN,VAROLD*CONVRG)) GO TO 500          LSSQR157
         NCONV =NCONV+1                                                 LSSQR158
         IF (NCONV .LT. MCONV) GO TO 490                                LSSQR159
            IF (IPRITR(ISTAGE).EQ.1 .OR. IPRITR(ISTAGE).EQ.2)           LSSQR160
     1          CALL OUTITR (NLINMX,NONLMX,PLIN,PLAM,PLMTRY,EK,EPK,     LSSQR161
     2                       PIVLIN,PIVLAM,ITER,VAR,Q,DUB,NTRY,ZERO)    LSSQR162
            IF (.NOT.ALPLIN .OR. .NOT.ALPLAM) IERROR=4                  LSSQR163
            RETURN                                                      LSSQR164
 490     NVARUP=0                                                       LSSQR165
         GO TO 560                                                      LSSQR166
 500  NCONV=0                                                           LSSQR167
      IF (DUB .LE. ZERO) GO TO 520                                      LSSQR168
         NVARUP=NVARUP+1                                                LSSQR169
         IF (NVARUP.LT.NABORT .OR. ITER.LT.NONL) GO TO 540              LSSQR170
         IERROR=1                                                       LSSQR171
         IF (IPRITR(ISTAGE) .EQ. 0) RETURN                              LSSQR172
         IF (IPRITR(ISTAGE) .NE. 3) CALL OUTITR (NLINMX,NONLMX,PLIN,    LSSQR173
     1       PLAM,PLMTRY,EK,EPK,PIVLIN,PIVLAM,ITER,VAR,Q,DUB,NTRY,ZERO) LSSQR174
         CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                            LSSQR175
         RETURN                                                         LSSQR176
 520  NVARUP=0                                                          LSSQR177
 540  IF (ITER .LT. MXITER(ISTAGE)) GO TO 560                           LSSQR178
         IERROR=2                                                       LSSQR179
         IF (IPRITR(ISTAGE) .EQ. 0) RETURN                              LSSQR180
         IF (IPRITR(ISTAGE) .NE. 3) CALL OUTITR (NLINMX,NONLMX,PLIN,    LSSQR181
     1       PLAM,PLMTRY,EK,EPK,PIVLIN,PIVLAM,ITER,VAR,Q,DUB,NTRY,ZERO) LSSQR182
         CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                            LSSQR183
         RETURN                                                         LSSQR184
 560  VAROLD=VAR                                                        LSSQR185
C-----------------------------------------------------------------------LSSQR186
C  DO NONLINEAR LEAST SQUARES ANALYSIS USING STEPWISE REGRESSION TO     LSSQR187
C  FIND FRACTIONAL STEP FOR CORRECTION OF NONLINEAR PARAMETERS PLAM.    LSSQR188
C  BUILD UP HESSIAN MATRIX GTG FOR NONLINEAR LEAST SQUARES              LSSQR189
C-----------------------------------------------------------------------LSSQR190
      IF (.NOT. SPLINE) CALL HESEXA (                                   LSSQR191
     1    NYMAX,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,PLAM,EK,EPK,       LSSQR192
     2    DELTAL,GTG,H,DELTAH,DELTAN,ETE,ASAVE,EKEK,DTOLER,R,PIVLIN,    LSSQR193
     3    .FALSE.,ZERO,RESPON)                                          LSSQR194
      IF (SPLINE) CALL HESSPL (                                         LSSQR195
     1    NLINMX,NONLMX,PLIN,GTG,H,DELTAH,DELTAN,ETE,ASAVE,EKEK,DTOLER, LSSQR196
     2    R,PIVLIN,BSPL0,BSPL1,BSPL2,DX,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP, LSSQR197
     3    .FALSE.,ZERO)                                                 LSSQR198
C-----------------------------------------------------------------------LSSQR199
C  SET TOLERANCE VALUES DTOLER FOR PIVOTING                             LSSQR200
C-----------------------------------------------------------------------LSSQR201
      DO 640 J=1,NONL                                                   LSSQR202
 640  DTOLER(J)=PRECIS*DTOLER(J)                                        LSSQR203
C-----------------------------------------------------------------------LSSQR204
C  INVERT MATRIX GTG TO OBTAIN CORRECTION DELTAN FOR NONLINEAR          LSSQR205
C  PARAMETER PLAM                                                       LSSQR206
C-----------------------------------------------------------------------LSSQR207
      CALL PIVOT (GTG,NONLMX,NONL,.FALSE.,PNMNMX(1),PNMNMX(2),PLAM,     LSSQR208
     1            DTOLER,DELTAN,PIVLAM,ALPLAM,QG,QMAX,SRANGE,IERROR)    LSSQR209
      IF (IERROR .EQ. 0) GO TO 660                                      LSSQR210
         IERROR=3                                                       LSSQR211
         IF (IPRITR(ISTAGE) .GT. 0) CALL ERRMES (3,.FALSE.,IHOLER,NOUT) LSSQR212
         RETURN                                                         LSSQR213
C-----------------------------------------------------------------------LSSQR214
C  FIND OPTIMUM FRACTIONAL STEPSIZE Q FOR NONLINEAR PARAMETER PLAM      LSSQR215
C-----------------------------------------------------------------------LSSQR216
 660  CALL INTERP (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,PLNTRY, LSSQR217
     1             PLAM,PLMTRY,PIVLIN,EK,ETE,EKEK,R,DELTAL,DTOLER,      LSSQR218
     2             DELTAN,ASAVE,NCOMBI,MTRYMX,STARTV,DX,BSPL0,IC,NZMAX, LSSQR219
     3             NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,Q,QG,QMAX,    LSSQR220
     4             VARG,NTRY,NOUT,VAROLD,NONL,IPRITR(ISTAGE),RESPON)    LSSQR221
      IF (NTRY .EQ. 2) GO TO 300                                        LSSQR222
      GO TO 200                                                         LSSQR223
 1000 FORMAT(///28A1,1X,3(5X,6A1,I2,3X,6A1,I2),5X,6A1,I2,3X,6A1,I2)     LSSQR224
 1100 FORMAT(34X,6A1,I2,3X,6A1,I2,5X,6A1,I2,3X,6A1,I2,                  LSSQR225
     1        5X,6A1,I2,3X,6A1,I2,5X,6A1,I2,3X,6A1,I2)                  LSSQR226
 2000 FORMAT(/28A1,3X,6A1,I2,3(4X,6A1,I2,3X,6A1,I2),4X,6A1,I2,3X,6A1,I2)LSSQR227
 2100 FORMAT(43X,6A1,I2,3X,6A1,I2,4X,6A1,I2,3X,6A1,I2,                  LSSQR228
     1        4X,6A1,I2,3X,6A1,I2,4X,6A1,I2,3X,6A1,I2)                  LSSQR229
 3000 FORMAT(/28A1,2X,7(4X,6A1,I2),4X,6A1,I2)                           LSSQR230
 3100 FORMAT(34X,6A1,I2,4X,6A1,I2,4X,6A1,I2,4X,6A1,I2,                  LSSQR231
     1        4X,6A1,I2,4X,6A1,I2,4X,6A1,I2,4X,6A1,I2)                  LSSQR232
      END                                                               LSSQR233
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++NOSUB001
      SUBROUTINE NOKSUB (N,K,IPATER,L,M,MORE)                           NOSUB002
C-----------------------------------------------------------------------NOSUB003
C     EVALUATES NEXT K-SUBSET OF AN N-SET, IN LEXICOGRAPHIC ORDER.      NOSUB004
C     SUPPORTED BY A.NIJENHUIS, H.S.WILF: "COMBINATORIAL ALGORITMS",    NOSUB005
C                               ACADEMIC PRESS.                         NOSUB006
C-----------------------------------------------------------------------NOSUB007
C                                                                       NOSUB008
C     DESCRIPTION OF PARAMETERS:                                        NOSUB009
C                                                                       NOSUB010
C           N      - NUMBER OF ELEMENTS TO PICK OUT SUBSET              NOSUB011
C           K      - NUMBER OF ELEMENTS IN DESIRED SUBSET               NOSUB012
C           IPATER - INTEGER ARRAY OF DIMENSION AT LEAST K CONTAINS     NOSUB013
C                    ON OUTPUT THE SUBSET                               NOSUB014
C           L      - INTERNAL INTEGER, WILL BE USED ON SUCCESSIVE       NOSUB015
C                    CALLS TO NOKSUB                                    NOSUB016
C           M      - INTERNAL INTEGER, WILL BE USED ON SUCCESSIVE       NOSUB017
C                    CALLS TO NOKSUB                                    NOSUB018
C           MORE   - LOGICAL, IS TO BE SET .FALSE. BEFORE FIRST CALL,   NOSUB019
C                    WILL BE .TRUE. IF CURRENT OUTPUT IS NOT THE LAST   NOSUB020
C                    SUBSET, OTHERWISE .FALSE., I.E. TERMINATE CALLS.   NOSUB021
C                                                                       NOSUB022
C-----------------------------------------------------------------------NOSUB023
C     SUBPROGRAMS CALLED - NONE                                         NOSUB024
C     COMMON USED        - NONE                                         NOSUB025
C-----------------------------------------------------------------------NOSUB026
      LOGICAL          MORE                                             NOSUB027
      DIMENSION        IPATER(K)                                        NOSUB028
C-----------------------------------------------------------------------NOSUB029
      IF (MORE) GO TO 100                                               NOSUB030
         M=0                                                            NOSUB031
         L=K                                                            NOSUB032
         GO TO 200                                                      NOSUB033
 100  IF (M .LT. N-L) L=0                                               NOSUB034
      L=L+1                                                             NOSUB035
      M=IPATER(K+1-L)                                                   NOSUB036
 200  DO 300 J=1,L                                                      NOSUB037
 300  IPATER(K+J-L)=M+J                                                 NOSUB038
      MORE=IPATER(1) .NE. N-K+1                                         NOSUB039
      RETURN                                                            NOSUB040
      END                                                               NOSUB041
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++NPSCL001
      INTEGER FUNCTION NPASCL (NOVERK,N,K,IFLAG)                        NPSCL002
C-----------------------------------------------------------------------NPSCL003
C                                    N                                  NPSCL004
C     EVALUATES FOR GIVEN NOVERK = (   ) A NEW BINOM AS SPECIFIED       NPSCL005
C                                    K                                  NPSCL006
C     THROUGH IFLAG. (SEE COMMENT STATMENTS BELOW.)                     NPSCL007
C-----------------------------------------------------------------------NPSCL008
C     SUBPROGRAMS CALLED - NONE                                         NPSCL009
C     COMMON USED        - NONE                                         NPSCL010
C-----------------------------------------------------------------------NPSCL011
      DOUBLE PRECISION BINOM, DUM, DDUM, FLOAT                          NPSCL012
C-----------------------------------------------------------------------NPSCL013
      INT(DUM)=IDINT(DUM)                                               NPSCL014
      FLOAT(I)=DFLOAT(I)                                                NPSCL015
C-----------------------------------------------------------------------NPSCL016
      BINOM=FLOAT(NOVERK)                                               NPSCL017
      IF (IFLAG) 100,200,300                                            NPSCL018
C-----------------------------------------------------------------------NPSCL019
C                           N            N-1                            NPSCL020
C  CASE IFLAG = -1, I.E.  (   )  --->  (     )                          NPSCL021
C                           K            K-1                            NPSCL022
C-----------------------------------------------------------------------NPSCL023
 100  DUM   =FLOAT(K)                                                   NPSCL024
      DDUM  =FLOAT(N)                                                   NPSCL025
      N     =N-1                                                        NPSCL026
      K     =K-1                                                        NPSCL027
      DUM   =(DUM/DDUM)*BINOM+.5                                        NPSCL028
      NPASCL=INT(DUM)                                                   NPSCL029
      RETURN                                                            NPSCL030
C-----------------------------------------------------------------------NPSCL031
C                          N            N-1                             NPSCL032
C  CASE IFLAG = 0, I.E.  (   )  --->  (     )                           NPSCL033
C                          K             K                              NPSCL034
C-----------------------------------------------------------------------NPSCL035
 200  DUM   =FLOAT(N-K)                                                 NPSCL036
      DDUM  =FLOAT(N)                                                   NPSCL037
      N     =N-1                                                        NPSCL038
      DUM   =(DUM/DDUM)*BINOM+.5                                        NPSCL039
      NPASCL=INT(DUM)                                                   NPSCL040
      RETURN                                                            NPSCL041
C-----------------------------------------------------------------------NPSCL042
C                          N            N+1                             NPSCL043
C  CASE IFLAG = 1, I.E.  (   )  --->  (     )                           NPSCL044
C                          K             K                              NPSCL045
C-----------------------------------------------------------------------NPSCL046
 300  DUM   =FLOAT(N+1)                                                 NPSCL047
      DDUM  =FLOAT(N-K+1)                                               NPSCL048
      N     =N+1                                                        NPSCL049
      DUM   =(DUM/DDUM)*BINOM+.5                                        NPSCL050
      NPASCL=INT(DUM)                                                   NPSCL051
      RETURN                                                            NPSCL052
      END                                                               NPSCL053
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++OUALL001
      SUBROUTINE OUTALL (ISTAGE,NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,OUALL002
     1                   PLIN,PLNTRY,PLAM,PLMTRY,PLMSAV,PIVLIN,PIVLAM,  OUALL003
     2                   EK,EPK,ETE,R,DELTAL,DELTAN,GTG,H,DELTAH,EKEK,  OUALL004
     3                   ASAVE,NCOMBI,MTRYMX,STARTV,NLNDMX,ATOT,DTOLER, OUALL005
     4                   BUFF,DX,BSPL0,BSPL1,BSPL2,IC,NZMAX,NGAMMX,CCAP,OUALL006
     5                   ECAP,YCAP,SGGCAP,SGYCAP,YW,BETTER,IERROR,L,    OUALL007
     6                   NLND,NLNDNL,VAR,SOLBES,RESPON)                 OUALL008
C-----------------------------------------------------------------------OUALL009
C  OUTPUT STANDARD-DEVIATION OF FIT (STDFIT), ALL PARAMETERS TOGETHER   OUALL010
C  WITH STANDARD-DEVIATIONS, AND CORRELATION MATRIX, AS SPECIFIED       OUALL011
C  THROUGH IPRINT(*,ISTAGE)                                             OUALL012
C  OUTPUT RESIDUALS, AS SPECIFIED THROUGH IPLRES(ISTAGE)                OUALL013
C  OUTPUT PLOT OF FIT TO DATA, AS SPECIFIED THROUGH IPLFIT(ISTAGE)      OUALL014
C-----------------------------------------------------------------------OUALL015
C     SUBPROGRAMS CALLED - ERRMES, FULLSM, GETPRU, OUTCOR, OUTPAR,      OUALL016
C                          PIVOT,  PLPRIN, PLRES,  USEROU, VARIAN       OUALL017
C     WHICH IN TURN CALL - BSPLIN, ERRMES, HESEXA, HESSEN, HESSPL,      OUALL018
C                          INTERV, LINEQS, LINPAR, NPASCL, PGAUSS,      OUALL019
C                          PIVOT,  PIVOT1, USERFL, USERFN, USERTR       OUALL020
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               OUALL021
C-----------------------------------------------------------------------OUALL022
      DOUBLE PRECISION PRECIS, RANGE,                                   OUALL023
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       OUALL024
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           OUALL025
     3                 TWOTHR, FORTHR, SIXTEL                           OUALL026
      DOUBLE PRECISION EK, EPK, ETE, R, DELTAL, DELTAN, GTG, H,         OUALL027
     1                 DELTAH, EKEK, ASAVE, ATOT, DTOLER, DX, BSPL0,    OUALL028
     2                 BSPL1, BSPL2, CCAP, ECAP, YCAP, SGGCAP,          OUALL029
     3                 SGYCAP, YW, VAR                                  OUALL030
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  OUALL031
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    OUALL032
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  OUALL033
     3                 SPLINE, WRTBET, WRTCAP                           OUALL034
      LOGICAL          PIVLIN, PIVLAM, STARTV, BETTER, ALREDY, SOLBES   OUALL035
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), YLYFIT(NYMAX,1),         OUALL036
     1                 SQRTW(NYMAX,1), NY(*), RESPON(*), PLIN(NLINMX,1),OUALL037
     2                 PLNTRY(NLINMX,1), PLAM(NONLMX), PLMTRY(NONLMX),  OUALL038
     3                 PLMSAV(NONLMX,1), PIVLIN(NLINMX,1),              OUALL039
     4                 PIVLAM(NLNDMX), EK(NLINMX), EPK(NONLMX),         OUALL040
     5                 ETE(NLINMX,NLINMX), R(NLINMX,1), DELTAL(NLINMX), OUALL041
     6                 DELTAN(NONLMX), GTG(NONLMX,NONLMX)               OUALL042
      DIMENSION        H(NLINMX,NLINMX,1), DELTAH(NONLMX,1),            OUALL043
     1                 EKEK(NLINMX,NLINMX,1), ASAVE(NLINMX,NLINMX,1),   OUALL044
     2                 NCOMBI(NONLMX), STARTV(MTRYMX),                  OUALL045
     3                 ATOT(NLNDMX,NLNDMX), DTOLER(NLNDMX), DX(NONLMX), OUALL046
     4                 BUFF(NLNDMX), BSPL0(4,NONLMX), BSPL1(4,NONLMX),  OUALL047
     5                 BSPL2(4,NONLMX), IC(NONLMX), CCAP(NZMAX,NZMAX,1),OUALL048
     6                 ECAP(NZMAX,NGAMMX,1), YCAP(NZMAX,1),             OUALL049
     7                 SGGCAP(NGAMMX,NGAMMX,1), SGYCAP(NGAMMX,1), YW(*) OUALL050
      DIMENSION        IHOLER(6)                                        OUALL051
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        OUALL052
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, OUALL053
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 OUALL054
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),OUALL055
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     OUALL056
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       OUALL057
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       OUALL058
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   OUALL059
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    OUALL060
     4                 MTRY(10,2), MXITER(2), NL(10),                   OUALL061
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  OUALL062
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, OUALL063
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       OUALL064
     8                 NIN, NOUT                                        OUALL065
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          OUALL066
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         OUALL067
     2                 DOUSOU(2), LUSER(30),                            OUALL068
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  OUALL069
     4                 SPLINE, WRTBET, WRTCAP                           OUALL070
      DATA             IHOLER/1HO, 1HU, 1HT, 1HA, 1HL, 1HL/             OUALL071
C-----------------------------------------------------------------------OUALL072
C  OUTPUT STANDARD-DEVIATION OF FIT (STDFIT), AS SPECIFIED              OUALL073
C  THROUGH IPRINT(*,ISTAGE)                                             OUALL074
C-----------------------------------------------------------------------OUALL075
      STDFIT=VAR/FLOAT(NYSUM-NLNDNL)                                    OUALL076
      STDFIT=SQRT(STDFIT)                                               OUALL077
      IF (.NOT.SOLBES .AND. (IPRINT(1,ISTAGE).NE.1 .AND.                OUALL078
     1   (IPRINT(1,ISTAGE).NE.2 .OR.BETTER))) WRITE (NOUT,2000) STDFIT  OUALL079
      IF (SOLBES .AND. (IPRINT(1,ISTAGE).EQ.1 .OR.                      OUALL080
     1    IPRINT(1,ISTAGE).EQ.3)) WRITE (NOUT,2000) STDFIT              OUALL081
C-----------------------------------------------------------------------OUALL082
C  CHECK IF IT IS NECESSARY TO CALCULATE FULL NORMAL EQUATION MATRIX    OUALL083
C-----------------------------------------------------------------------OUALL084
      IF (SOLBES) GO TO 100                                             OUALL085
         IF (IPRINT(2,ISTAGE).EQ.3 .OR.                                 OUALL086
     1      (IPRINT(2,ISTAGE).EQ.2 .AND. BETTER)) GO TO 100             OUALL087
         IF (IPRINT(3,ISTAGE).EQ.1 .OR.                                 OUALL088
     1      (IPRINT(3,ISTAGE).EQ.2 .AND. .NOT.BETTER)) GO TO 500        OUALL089
C-----------------------------------------------------------------------OUALL090
C  BUILD UP FULL NORMAL EQUATION MATRIX ATOT(I,J), I,J=1,...,NLNDNL     OUALL091
C-----------------------------------------------------------------------OUALL092
 100  CALL FULLSM (NYMAX,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,PLAM,     OUALL093
     1             PIVLIN,EK,EPK,ETE,R,DELTAL,DELTAN,GTG,H,DELTAH,EKEK, OUALL094
     2             ASAVE,NLNDMX,ATOT,DTOLER,DX,BSPL0,BSPL1,BSPL2,IC,    OUALL095
     3             NZMAX,NGAMMX,CCAP,ECAP,YCAP,NLND,ND,NLIN,NONL,SPLINE,OUALL096
     4             PRECIS,ZERO,TEN,RESPON)                              OUALL097
C-----------------------------------------------------------------------OUALL098
C  INVERT FULL NORMAL EQUATION MATRIX ATOT(I,J), I,J=1,...,NLNDNL       OUALL099
C-----------------------------------------------------------------------OUALL100
      CALL PIVOT (ATOT,NLNDMX,NLNDNL,.TRUE.,PNMNMX(1),PNMNMX(2),PLAM,   OUALL101
     1            DTOLER,DELTAL,PIVLAM,ALPLAM,Q,DUM,SRANGE,IERROR)      OUALL102
      IF (ALPLAM) GO TO 300                                             OUALL103
         IF (IERROR .NE. 0) GO TO 200                                   OUALL104
            CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                         OUALL105
            GO TO 500                                                   OUALL106
 200     CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                            OUALL107
         RETURN                                                         OUALL108
C-----------------------------------------------------------------------OUALL109
C  OUTPUT ALL PARAMETERS TOGETHER WITH STANDARD-DEVIATIONS,             OUALL110
C  AND CORRELATION MATRIX, AS SPECIFIED THROUGH IPRINT(*,ISTAGE)        OUALL111
C-----------------------------------------------------------------------OUALL112
 300  DO 400 I=1,NLNDNL                                                 OUALL113
         IF (ATOT(I,I) .GT. ZERO) GO TO 350                             OUALL114
            CALL ERRMES (3,.FALSE.,IHOLER,NOUT)                         OUALL115
            GO TO 500                                                   OUALL116
 350     BUFF(I)=ATOT(I,I)                                              OUALL117
C        ATOT(I,I)=ONE/SQRT(ATOT(I,I))                                  OUALL118
         ATOT(I,I)=ONE/DSQRT(ATOT(I,I))                                 OUALL119
 400  CONTINUE                                                          OUALL120
C-----------------------------------------------------------------------OUALL121
C  OUTPUT PARAMETER + STANDARD DEVIATION + PERCENT ERROR                OUALL122
C-----------------------------------------------------------------------OUALL123
      IF (.NOT.SOLBES .AND. (IPRINT(2,ISTAGE).NE.1 .AND.                OUALL124
     1   (IPRINT(2,ISTAGE).NE.2 .OR. BETTER))) CALL OUTPAR (            OUALL125
     2    NLINMX,NONLMX,PLIN,PLAM,NLNDMX,ATOT,NLND,STDFIT)              OUALL126
      IF (SOLBES .AND. (IPRINT(2,ISTAGE).EQ.1 .OR.                      OUALL127
     1    IPRINT(2,ISTAGE).EQ.3)) CALL OUTPAR (                         OUALL128
     2    NLINMX,NONLMX,PLIN,PLAM,NLNDMX,ATOT,NLND,STDFIT)              OUALL129
      IROUTE=3                                                          OUALL130
      IF (SOLBES) IROUTE=5                                              OUALL131
      IF (DOUSOU(ISTAGE)) CALL USEROU (IROUTE,NYMAX,Y,T,YLYFIT,SQRTW,   OUALL132
     1    NY,NLINMX,NONLMX,PLIN,PLAM,PLMTRY,BUFF,VAR,ISTAGE,RESPON)     OUALL133
C-----------------------------------------------------------------------OUALL134
C  OUTPUT CORRELATION MATRIX                                            OUALL135
C-----------------------------------------------------------------------OUALL136
      IF (.NOT.SOLBES .AND. (IPRINT(3,ISTAGE).NE.1 .AND.                OUALL137
     1   (IPRINT(3,ISTAGE).NE.2 .OR. BETTER))) CALL OUTCOR (            OUALL138
     2    NLNDMX,ATOT,BUFF)                                             OUALL139
      IF (SOLBES .AND. (IPRINT(3,ISTAGE).EQ.1 .OR.                      OUALL140
     1    IPRINT(3,ISTAGE).EQ.3)) CALL OUTCOR (NLNDMX,ATOT,BUFF)        OUALL141
C-----------------------------------------------------------------------OUALL142
C  SAVE VAR IN VARSAV(L), PLAM IN PLMSAV, IF BETTER=.TRUE.              OUALL143
C-----------------------------------------------------------------------OUALL144
 500  IF (SOLBES .OR. .NOT.BETTER) GO TO 600                            OUALL145
         VARSAV(L)=VAR                                                  OUALL146
         DO 580 J=1,NONL                                                OUALL147
 580     PLMSAV(J,L)=PLAM(J)                                            OUALL148
C-----------------------------------------------------------------------OUALL149
C  OUTPUT RESIDUALS, AS SPECIFIED THROUGH IPLRES(ISTAGE)                OUALL150
C-----------------------------------------------------------------------OUALL151
 600  ALREDY=.FALSE.                                                    OUALL152
      IF (.NOT.SOLBES .AND. (IPLRES(ISTAGE).LE.1 .OR.                   OUALL153
     1   (IPLRES(ISTAGE).EQ.2 .AND. .NOT.BETTER))) GO TO 700            OUALL154
      IF (SOLBES .AND. IPLRES(ISTAGE).NE.1) GO TO 700                   OUALL155
         IF (SPLINE) CALL VARIAN (                                      OUALL156
     1       NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLNTRY,PLIN,PLMTRY,OUALL157
     2       PLAM,PIVLIN,EK,ETE,EKEK,R,DELTAL,DTOLER,DELTAN,ASAVE,      OUALL158
     3       .FALSE.,NCOMBI,MTRYMX,STARTV,DX,BSPL0,IC,NZMAX,NGAMMX,     OUALL159
     4       CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,Q,VAR,IERROR,.TRUE.,RESPON)OUALL160
         ALREDY=.TRUE.                                                  OUALL161
         DO 680 N=1,ND                                                  OUALL162
            J    =N                                                     OUALL163
            PRUNS=GETPRU(NYMAX,YLYFIT(1,N),NY(N))                       OUALL164
            CALL PLRES (NYMAX,YLYFIT(1,N),NY(N),PRUNS,NOUT,LINEPG,J)    OUALL165
 680     CONTINUE                                                       OUALL166
C-----------------------------------------------------------------------OUALL167
C  OUTPUT PLOT OF FIT TO DATA, AS SPECIFIED THROUGH IPLFIT(ISTAGE)      OUALL168
C-----------------------------------------------------------------------OUALL169
 700  IAPFIT=IABS(IPLFIT(ISTAGE))                                       OUALL170
      IF (.NOT.SOLBES .AND. (IAPFIT.LE.1 .OR.                           OUALL171
     1   (IAPFIT.EQ.2 .AND. .NOT.BETTER))) GO TO 800                    OUALL172
      IF (SOLBES .AND. IAPFIT.NE.1) GO TO 800                           OUALL173
         IF (SPLINE .AND. .NOT.ALREDY) CALL VARIAN (                    OUALL174
     1       NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLNTRY,PLIN,PLMTRY,OUALL175
     2       PLAM,PIVLIN,EK,ETE,EKEK,R,DELTAL,DTOLER,DELTAN,ASAVE,      OUALL176
     3       .FALSE.,NCOMBI,MTRYMX,STARTV,DX,BSPL0,IC,NZMAX,NGAMMX,     OUALL177
     4       CCAP,ECAP,YCAP,SGGCAP,SGYCAP,YW,Q,VAR,IERROR,.TRUE.,RESPON)OUALL178
         KSTART=0                                                       OUALL179
         KEND  =0                                                       OUALL180
         IF (IPLFIT(ISTAGE) .GT. 0) GO TO 810                           OUALL181
            KSTART=IUSER(19)                                            OUALL182
            KEND  =IUSER(20)                                            OUALL183
 810     WRITE (NOUT,1000)                                              OUALL184
         DO  780 N=1,ND                                                 OUALL185
            WRITE (NOUT,3000) N,NONL                                    OUALL186
            MY=NY(N)                                                    OUALL187
            DO 740 I=1,MY                                               OUALL188
 740        YLYFIT(I,N)=Y(I,N)-YLYFIT(I,N)/SQRTW(I,N)                   OUALL189
            CALL PLPRIN (NYMAX,T(1,N),YLYFIT(1,N),Y(1,N),MY,.FALSE.,    OUALL190
     1                   NOUT,SRANGE,KSTART,KEND)                       OUALL191
            DO 760 I=1,MY                                               OUALL192
 760        YLYFIT(I,N)=(Y(I,N)-YLYFIT(I,N))*SQRTW(I,N)                 OUALL193
 780     CONTINUE                                                       OUALL194
 800  IROUTE=4                                                          OUALL195
      IF (SOLBES) IROUTE=6                                              OUALL196
      IF (DOUSOU(ISTAGE)) CALL USEROU (IROUTE,NYMAX,Y,T,YLYFIT,SQRTW,   OUALL197
     1    NY,NLINMX,NONLMX,PLIN,PLAM,PLMTRY,BUFF,VAR,ISTAGE,RESPON)     OUALL198
      RETURN                                                            OUALL199
 1000 FORMAT(1H1)                                                       OUALL200
 2000 FORMAT(//37H STANDARD DEVIATION OF FIT = STDFIT =,1PE12.5)        OUALL201
 3000 FORMAT(//52H PLOT OF DATA (O)  AND FIT TO DATA (X)  FOR DATA SET, OUALL202
     1         I5,48X,8HASSUMING,I5,3X,10HCOMPONENTS                    OUALL203
     2        /33H ORDINATES LISTED ARE FIT VALUES.)                    OUALL204
      END                                                               OUALL205
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++OUCOR001
      SUBROUTINE OUTCOR (NLNDMX,ATOT,BUFF)                              OUCOR002
C-----------------------------------------------------------------------OUCOR003
C     OUTPUT CORRELATION MATRIX (I.E. INVERSE OF FULL LEAST             OUCOR004
C     SQUARES MATRIX)                                                   OUCOR005
C-----------------------------------------------------------------------OUCOR006
C     SUBPROGRAMS CALLED - NONE                                         OUCOR007
C     COMMON USED        - IBLOCK                                       OUCOR008
C-----------------------------------------------------------------------OUCOR009
      DOUBLE PRECISION ATOT, DUB                                        OUCOR010
      DIMENSION        ATOT(NLNDMX,NLNDMX), BUFF(NLNDMX)                OUCOR011
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       OUCOR012
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       OUCOR013
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   OUCOR014
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    OUCOR015
     4                 MTRY(10,2), MXITER(2), NL(10),                   OUCOR016
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  OUCOR017
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, OUCOR018
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       OUCOR019
     8                 NIN, NOUT                                        OUCOR020
C-----------------------------------------------------------------------OUCOR021
      WRITE (NOUT,1000)                                                 OUCOR022
      IF (NGAM .EQ. 0) WRITE (NOUT,2000)                                OUCOR023
     1                     (((IALPHA(I),I=2,4),J,J=1,NONL),N=1,ND),     OUCOR024
     2                      ((ILAMDA(I),I=1,3),J,J=1,NONL)              OUCOR025
      IF (NGAM .GT. 0) WRITE (NOUT,2000)                                OUCOR026
     1                     (((IGAMMA(I),I=2,4),J,J=1,NGAM),             OUCOR027
     2                      ((IALPHA(I),I=2,4),J,J=1,NONL),N=1,ND),     OUCOR028
     3                      ((ILAMDA(I),I=1,3),J,J=1,NONL)              OUCOR029
      JR=0                                                              OUCOR030
      DO 300 N=1,ND                                                     OUCOR031
      DO 300 L=1,2                                                      OUCOR032
         JSTOP=NGAM                                                     OUCOR033
         IF (L .EQ. 2) JSTOP=NONL                                       OUCOR034
         IF (JSTOP .EQ. 0) GO TO 300                                    OUCOR035
         DO 200 J=1,JSTOP                                               OUCOR036
            JEND=JR                                                     OUCOR037
            JR  =JR+1                                                   OUCOR038
            IF (JEND .EQ. 0) GO TO 150                                  OUCOR039
               DUB=ATOT(JR,JR)                                          OUCOR040
               DO 100 JC=1,JEND                                         OUCOR041
 100           BUFF(JC)=ATOT(JR,JC)*DUB*ATOT(JC,JC)                     OUCOR042
 150        BUFF(JR)=1.                                                 OUCOR043
            JEND    =MIN0(JR,15)                                        OUCOR044
            IF (L .EQ. 1) WRITE (NOUT,3000)                             OUCOR045
     1                          (IGAMMA(I),I=2,4),J,(BUFF(JC),JC=1,JEND)OUCOR046
            IF (L .EQ. 2) WRITE (NOUT,3000)                             OUCOR047
     1                          (IALPHA(I),I=2,4),J,(BUFF(JC),JC=1,JEND)OUCOR048
            IF (JR .GT. 15) WRITE (NOUT,4000) (BUFF(JC),JC=16,JR)       OUCOR049
 200     CONTINUE                                                       OUCOR050
 300  CONTINUE                                                          OUCOR051
      DO 500 J=1,NONL                                                   OUCOR052
         JEND=JR                                                        OUCOR053
         JR  =JR+1                                                      OUCOR054
         DUB =ATOT(JR,JR)                                               OUCOR055
         DO 400 JC=1,JEND                                               OUCOR056
 400     BUFF(JC)=ATOT(JR,JC)*DUB*ATOT(JC,JC)                           OUCOR057
         BUFF(JR)=1.                                                    OUCOR058
         JEND    =MIN0(JR,15)                                           OUCOR059
         WRITE (NOUT,3000) (ILAMDA(I),I=1,3),J,(BUFF(JC),JC=1,JEND)     OUCOR060
         IF (JR .GT. 15) WRITE (NOUT,4000) (BUFF(JC),JC=16,JR)          OUCOR061
 500  CONTINUE                                                          OUCOR062
      RETURN                                                            OUCOR063
 1000 FORMAT(//9X,24HCORRELATION COEFFICIENTS/)                         OUCOR064
 2000 FORMAT(9X,3A1,I2,3X,3A1,I2,3X,3A1,I2,3X,3A1,I2,3X,3A1,I2,         OUCOR065
     1       3X,3A1,I2,3X,3A1,I2,3X,3A1,I2,3X,3A1,I2,3X,3A1,I2,         OUCOR066
     2       3X,3A1,I2,3X,3A1,I2,3X,3A1,I2,3X,3A1,I2,3X,3A1,I2)         OUCOR067
 3000 FORMAT(1X,3A1,I2,15(F8.3))                                        OUCOR068
 4000 FORMAT(F14.3,F8.3,F8.3,F8.3,F8.3,F8.3,F8.3,F8.3,                  OUCOR069
     1             F8.3,F8.3,F8.3,F8.3,F8.3,F8.3,F8.3)                  OUCOR070
      END                                                               OUCOR071
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++OUITR001
      SUBROUTINE OUTITR (NLINMX,NONLMX,PLIN,PLAM,PLMHLP,EK,EPK,PIVLIN,  OUITR002
     1                   PIVLAM,ITER,VAR,Q,VARDIF,NTRY,ZERO)            OUITR003
C-----------------------------------------------------------------------OUITR004
C     OUTPUTS ONE ITERATION STEP.                                       OUITR005
C     IN DETAIL: ITR       - ITERATION COUNTER                          OUITR006
C                VARIANCE  - WEIGHTED SUM OF SQUARES                    OUITR007
C                DAMPING Q - DAMPING FACTOR USED TO MODIFY THE LENGTH   OUITR008
C                            OF A CORRECTION STEP IN NONLINEAR LEAST    OUITR009
C                            SQUARES                                    OUITR010
C                GAMMA     - PARAMETERS FOR BASELINE FUNCTIONS          OUITR011
C                ALPHA     - LINEAR PARAMETERS                          OUITR012
C                LAMBDA    - NONLINEAR PARAMETERS                       OUITR013
C-----------------------------------------------------------------------OUITR014
C     SUBPROGRAMS CALLED - USERTR                                       OUITR015
C     WHICH IN TURN CALL - ERRMES                                       OUITR016
C     COMMON USED        - IBLOCK                                       OUITR017
C-----------------------------------------------------------------------OUITR018
      DOUBLE PRECISION EK, EPK, VAR, VARDIF, ZERO, HOLRTH, STAR         OUITR019
      LOGICAL          PIVLIN, PIVLAM                                   OUITR020
      DIMENSION        PLIN(NLINMX,1), PLAM(NONLMX), PLMHLP(NONLMX),    OUITR021
     1                 EK(NLINMX), EPK(NONLMX), PIVLIN(NLINMX,1),       OUITR022
     2                 PIVLAM(NONLMX), HOLRTH(2)                        OUITR023
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       OUITR024
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       OUITR025
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   OUITR026
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    OUITR027
     4                 MTRY(10,2), MXITER(2), NL(10),                   OUITR028
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  OUITR029
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, OUITR030
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       OUITR031
     8                 NIN, NOUT                                        OUITR032
      DATA             HOLRTH/1H , 1H*/                                 OUITR033
C-----------------------------------------------------------------------OUITR034
C  PLMHLP(J)=USERTR(PLAM(J),2), J=1,...,NONL                            OUITR035
C-----------------------------------------------------------------------OUITR036
      STAR=HOLRTH(1)                                                    OUITR037
      IF (VARDIF .GT. ZERO) STAR=HOLRTH(2)                              OUITR038
      DO 100 J=1,NONL                                                   OUITR039
         EPK(J)=HOLRTH(1)                                               OUITR040
         IF (.NOT. PIVLAM(J)) EPK(J)=HOLRTH(2)                          OUITR041
         PLMHLP(J)=USERTR(PLAM(J),2)                                    OUITR042
 100  CONTINUE                                                          OUITR043
      IF (NGAMM1) 200,400,600                                           OUITR044
C-----------------------------------------------------------------------OUITR045
C  CASE  NGAM = 0                                                       OUITR046
C-----------------------------------------------------------------------OUITR047
 200     DO 210 I=1,NLIN                                                OUITR048
            EK(I)=HOLRTH(1)                                             OUITR049
            IF (.NOT. PIVLIN(I,1)) EK(I)=HOLRTH(2)                      OUITR050
 210     CONTINUE                                                       OUITR051
         WRITE (NOUT,2000) ITER,VAR,STAR,Q,HOLRTH(NTRY),                OUITR052
     1         (PLIN(I,1),EK(I),PLMHLP(I),EPK(I),I=1,NPLINE)            OUITR053
         IF (NONL .GT. 4) WRITE (NOUT,2001)                             OUITR054
     1         (PLIN(I,1),EK(I),PLMHLP(I),EPK(I),I=5,NONL)              OUITR055
         IF (ND .LE. 1) RETURN                                          OUITR056
         DO 250 N=2,ND                                                  OUITR057
            DO 230 I=1,NLIN                                             OUITR058
               EK(I)=HOLRTH(1)                                          OUITR059
               IF (.NOT. PIVLIN(I,N)) EK(I)=HOLRTH(2)                   OUITR060
 230        CONTINUE                                                    OUITR061
            WRITE (NOUT,2002) (PLIN(I,N),EK(I),I=1,NONL)                OUITR062
 250     CONTINUE                                                       OUITR063
         RETURN                                                         OUITR064
C-----------------------------------------------------------------------OUITR065
C  CASE  NGAM = 1                                                       OUITR066
C-----------------------------------------------------------------------OUITR067
 400     DO 410 I=1,NLIN                                                OUITR068
            EK(I)=HOLRTH(1)                                             OUITR069
            IF (.NOT. PIVLIN(I,1)) EK(I)=HOLRTH(2)                      OUITR070
 410     CONTINUE                                                       OUITR071
         WRITE (NOUT,4000) ITER,VAR,STAR,Q,HOLRTH(NTRY),PLIN(1,1),EK(1),OUITR072
     1         (PLIN(I+1,1),EK(I+1),PLMHLP(I),EPK(I),I=1,NPLINE)        OUITR073
         IF (NONL .GT. 4) WRITE (NOUT,4001)                             OUITR074
     1      (PLIN(I+1,1),EK(I+1),PLMHLP(I),EPK(I),I=5,NONL)             OUITR075
         IF (ND .LE. 1) RETURN                                          OUITR076
         DO 450 N=2,ND                                                  OUITR077
            DO 430 I=1,NLIN                                             OUITR078
               EK(I)=HOLRTH(1)                                          OUITR079
               IF (.NOT. PIVLIN(I,N)) EK(I)=HOLRTH(2)                   OUITR080
 430        CONTINUE                                                    OUITR081
            WRITE (NOUT,4002) PLIN(1,N),EK(1),                          OUITR082
     1                       (PLIN(I+1,N),EK(I+1),I=1,NPLINE)           OUITR083
            IF (NONL .GT. 4) WRITE (NOUT,4003)                          OUITR084
     1                      (PLIN(I,N),EK(I),I=6,NLIN)                  OUITR085
 450     CONTINUE                                                       OUITR086
         RETURN                                                         OUITR087
C-----------------------------------------------------------------------OUITR088
C  CASE  NGAM > 1                                                       OUITR089
C-----------------------------------------------------------------------OUITR090
 600  DO 610 I=1,NLIN                                                   OUITR091
         EK(I)=HOLRTH(1)                                                OUITR092
         IF (.NOT. PIVLIN(I,1)) EK(I)=HOLRTH(2)                         OUITR093
 610  CONTINUE                                                          OUITR094
      WRITE (NOUT,6000)                                                 OUITR095
     1      ITER,VAR,STAR,Q,HOLRTH(NTRY),(PLIN(I,1),EK(I),I=1,NPLINE)   OUITR096
      IF (NGAM .GT. 8) WRITE (NOUT,6001) (PLIN(I,1),EK(I),I=9,NGAM)     OUITR097
      WRITE (NOUT,6001)                                                 OUITR098
     1      (PLIN(NGAM+I,1),EK(NGAM+I),PLMHLP(I),EPK(I),I=1,NONL)       OUITR099
      IF (ND .LE. 1) RETURN                                             OUITR100
      DO 650 N=2,ND                                                     OUITR101
         DO 630 I=1,NLIN                                                OUITR102
            EK(I)=HOLRTH(1)                                             OUITR103
            IF (.NOT. PIVLIN(I,N)) EK(I)=HOLRTH(2)                      OUITR104
 630     CONTINUE                                                       OUITR105
         WRITE (NOUT,6001) (PLIN(I,N),EK(I),I=1,NPLINE)                 OUITR106
         IF (NGAM .GT. 8) WRITE (NOUT,6001) (PLIN(I,N),EK(I),I=9,NGAM)  OUITR107
         WRITE (NOUT,6002) (PLIN(I,N),EK(I),I=NGAMP1,NLIN)              OUITR108
 650  CONTINUE                                                          OUITR109
      RETURN                                                            OUITR110
C2000 FORMAT(1X,I3,1PE11.3,A1,E11.2,A1,1PE14.2,A1,1PE10.2,A1,           OUITR111
C    1           1PE12.2,A1,1PE10.2,A1,1PE12.2,A1,1PE10.2,A1,           OUITR112
C    2           1PE12.2,A1,1PE10.2,A1)                                 OUITR113
 2000 FORMAT(1X,I3,1PD11.3,A1,E11.2,A1,1PE14.2,A1,1PE10.2,A1,           OUITR114
     1           1PE12.2,A1,1PE10.2,A1,1PE12.2,A1,1PE10.2,A1,           OUITR115
     2           1PE12.2,A1,1PE10.2,A1)                                 OUITR116
 2001 FORMAT(30X,1PE12.2,A1,1PE10.2,A1,1PE12.2,A1,1PE10.2,A1,           OUITR117
     1           1PE12.2,A1,1PE10.2,A1,1PE12.2,A1,1PE10.2,A1)           OUITR118
 2002 FORMAT(30X,1PE12.2,A1,1PE23.2,A1,1PE23.2,A1,1PE23.2,A1)           OUITR119
C4000 FORMAT(1X,I3,1PE11.3,A1,E11.2,A1,1PE11.2,A1,                      OUITR120
C    1           1PE11.2,A1,1PE10.2,A1,1PE11.2,A1,1PE10.2,A1,           OUITR121
C    2           1PE11.2,A1,1PE10.2,A1,1PE11.2,A1,1PE10.2,A1)           OUITR122
 4000 FORMAT(1X,I3,1PD11.3,A1,E11.2,A1,1PE11.2,A1,                      OUITR123
     1           1PE11.2,A1,1PE10.2,A1,1PE11.2,A1,1PE10.2,A1,           OUITR124
     2           1PE11.2,A1,1PE10.2,A1,1PE11.2,A1,1PE10.2,A1)           OUITR125
 4001 FORMAT(40X,1PE11.2,A1,1PE10.2,A1,1PE11.2,A1,1PE10.2,A1,           OUITR126
     1           1PE11.2,A1,1PE10.2,A1,1PE11.2,A1,1PE10.2,A1)           OUITR127
 4002 FORMAT(28X,1PE11.2,A1,                                            OUITR128
     1           1PE11.2,A1,1PE22.2,A1,1PE22.2,A1,1PE22.2,A1)           OUITR129
 4003 FORMAT(40X,1PE11.2,A1,1PE22.2,A1,1PE22.2,A1,1PE22.2,A1)           OUITR130
C6000 FORMAT(1X,I3,1PE11.3,A1,E11.2,A1,1PE14.2,A1,7(1PE11.2,A1))        OUITR131
 6000 FORMAT(1X,I3,1PD11.3,A1,E11.2,A1,1PE14.2,A1,7(1PE11.2,A1))        OUITR132
 6001 FORMAT(31X,1PE11.2,A1,1PE11.2,A1,1PE11.2,A1,1PE11.2,A1,           OUITR133
     1           1PE11.2,A1,1PE11.2,A1,1PE11.2,A1,1PE11.2,A1)           OUITR134
 6002 FORMAT(31X,1PE11.2,A1,1PE23.2,A1,1PE23.2,A1,1PE23.2,A1)           OUITR135
      END                                                               OUITR136
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++OUPAR001
      SUBROUTINE OUTPAR (NLINMX,NONLMX,PLIN,PLAM,NLNDMX,ATOT,NLND,      OUPAR002
     1                   STDFIT)                                        OUPAR003
C-----------------------------------------------------------------------OUPAR004
C     OUTPUTS PARAMETER TOGETHER WITH STAND.ERROR, AND ERROR IN PERCENT OUPAR005
C-----------------------------------------------------------------------OUPAR006
C     SUBPROGRAMS CALLED - USERTR                                       OUPAR007
C     WHICH IN TURN CALL - ERRMES                                       OUPAR008
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK                       OUPAR009
C-----------------------------------------------------------------------OUPAR010
      DOUBLE PRECISION PRECIS, RANGE,                                   OUPAR011
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       OUPAR012
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           OUPAR013
     3                 TWOTHR, FORTHR, SIXTEL                           OUPAR014
      DOUBLE PRECISION ATOT                                             OUPAR015
      DIMENSION        PLIN(NLINMX,1), PLAM(NONLMX),                    OUPAR016
     1                 ATOT(NLNDMX,NLNDMX)                              OUPAR017
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        OUPAR018
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, OUPAR019
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 OUPAR020
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),OUPAR021
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     OUPAR022
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       OUPAR023
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       OUPAR024
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   OUPAR025
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    OUPAR026
     4                 MTRY(10,2), MXITER(2), NL(10),                   OUPAR027
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  OUPAR028
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, OUPAR029
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       OUPAR030
     8                 NIN, NOUT                                        OUPAR031
C-----------------------------------------------------------------------OUPAR032
      IF (NGAM .EQ. 0) GO TO 500                                        OUPAR033
         WRITE (NOUT,3000)                                              OUPAR034
         JEND=MIN0(NONL,NGAM)                                           OUPAR035
         DO 450 N=1,ND                                                  OUPAR036
            WRITE (NOUT,6000)                                           OUPAR037
            NDM1NL=(N-1)*NLIN                                           OUPAR038
            DO 150 J=1,JEND                                             OUPAR039
               JG   =J+NGAM                                             OUPAR040
               JN   =NDM1NL+J                                           OUPAR041
               JNG  =JN+NGAM                                            OUPAR042
               ERRG =STDFIT/ATOT(JN,JN)                                 OUPAR043
               PERCG=SRANGE                                             OUPAR044
               APLIN=ABS(PLIN(J,N))                                     OUPAR045
               IF (APLIN .GE. PRECIS) PERCG=100.*ERRG/APLIN             OUPAR046
               ERRA =STDFIT/ATOT(JNG,JNG)                               OUPAR047
               PERCA=SRANGE                                             OUPAR048
               APLIN=ABS(PLIN(JG,N))                                    OUPAR049
               IF (APLIN .GE. PRECIS) PERCA=100.*ERRA/APLIN             OUPAR050
               IF (N .GT. 1) GO TO 100                                  OUPAR051
                  DUM  =USERTR(PLAM(J),2)                               OUPAR052
                  JD   =J+NLND                                          OUPAR053
                  ERRL =STDFIT/(ATOT(JD,JD)*ABS(USERTR(DUM,3)))         OUPAR054
                  PERCL=100.*ERRL/DUM                                   OUPAR055
                  WRITE (NOUT,4000) PLIN(J,1),ERRG,PERCG,               OUPAR056
     1                              PLIN(JG,1),ERRA,PERCA,DUM,ERRL,PERCLOUPAR057
                  GO TO 150                                             OUPAR058
 100           WRITE (NOUT,4000) PLIN(J,N),ERRG,PERCG,                  OUPAR059
     1                           PLIN(JG,N),ERRA,PERCA                  OUPAR060
 150        CONTINUE                                                    OUPAR061
            IF (NONL-NGAM) 200,450,300                                  OUPAR062
 200           DO 250 J=NONLP1,NGAM                                     OUPAR063
                  JN   =NDM1NL+J                                        OUPAR064
                  ERRG =STDFIT/ATOT(JN,JN)                              OUPAR065
                  PERCG=SRANGE                                          OUPAR066
                  APLIN=ABS(PLIN(J,N))                                  OUPAR067
                  IF (APLIN .GE. PRECIS) PERCG=100.*ERRG/APLIN          OUPAR068
                  WRITE (NOUT,4000) PLIN(J,N),ERRG,PERCG                OUPAR069
 250           CONTINUE                                                 OUPAR070
               GO TO 450                                                OUPAR071
 300        DO 400 J=NGAMP1,NONL                                        OUPAR072
               JG   =J+NGAM                                             OUPAR073
               JNG  =NDM1NL+JG                                          OUPAR074
               ERRA =STDFIT/ATOT(JNG,JNG)                               OUPAR075
               PERCA=SRANGE                                             OUPAR076
               APLIN=ABS(PLIN(JG,N))                                    OUPAR077
               IF (APLIN .GE. PRECIS) PERCA=100.*ERRA/APLIN             OUPAR078
               IF (N .GT. 1) GO TO 350                                  OUPAR079
                  DUM  =USERTR(PLAM(J),2)                               OUPAR080
                  JD   =J+NLND                                          OUPAR081
                  ERRL =STDFIT/(ATOT(JD,JD)*ABS(USERTR(DUM,3)))         OUPAR082
                  PERCL=100.*ERRL/DUM                                   OUPAR083
                  WRITE (NOUT,5000) PLIN(JG,1),ERRA,PERCA,DUM,ERRL,PERCLOUPAR084
                  GO TO 400                                             OUPAR085
 350           WRITE (NOUT,5000) PLIN(JG,N),ERRA,PERCA                  OUPAR086
 400        CONTINUE                                                    OUPAR087
 450     CONTINUE                                                       OUPAR088
         RETURN                                                         OUPAR089
 500  WRITE (NOUT,1000)                                                 OUPAR090
      DO 700 N=1,ND                                                     OUPAR091
         WRITE (NOUT,6000)                                              OUPAR092
         NDM1NL=(N-1)*NLIN                                              OUPAR093
         DO 600 J=1,NONL                                                OUPAR094
            JN   =NDM1NL+J                                              OUPAR095
            ERRA =STDFIT/ATOT(JN,JN)                                    OUPAR096
            PERCA=SRANGE                                                OUPAR097
            APLIN=ABS(PLIN(J,N))                                        OUPAR098
            IF (APLIN .GE. PRECIS) PERCA=100.*ERRA/APLIN                OUPAR099
            IF (N .GT. 1) GO TO 550                                     OUPAR100
               DUM  =USERTR(PLAM(J),2)                                  OUPAR101
               JD   =J+NLND                                             OUPAR102
               ERRL =STDFIT/(ATOT(JD,JD)*ABS(USERTR(DUM,3)))            OUPAR103
               PERCL=100.*ERRL/DUM                                      OUPAR104
               WRITE (NOUT,2000) PLIN(J,1),ERRA,PERCA,DUM,ERRL,PERCL    OUPAR105
               GO TO 600                                                OUPAR106
 550        WRITE (NOUT,2000) PLIN(J,N),ERRA,PERCA                      OUPAR107
 600     CONTINUE                                                       OUPAR108
 700  CONTINUE                                                          OUPAR109
      RETURN                                                            OUPAR110
 1000 FORMAT(//15X,31HALPHA  +-  STD. ERROR   PERCENT,                  OUPAR111
     1         14X,32HLAMBDA  +-  STD. ERROR   PERCENT)                 OUPAR112
 2000 FORMAT(2(1PE20.4,4H  +-,E12.4,0PF10.3))                           OUPAR113
 3000 FORMAT(//7X,31HGAMMA  +-  STD. ERROR   PERCENT,                   OUPAR114
     1        11X,31HALPHA  +-  STD. ERROR   PERCENT,                   OUPAR115
     2        10X,32HLAMBDA  +-  STD. ERROR   PERCENT)                  OUPAR116
 4000 FORMAT(    3(1PE12.4,4H  +-,E12.4,0PF10.3,4X))                    OUPAR117
 5000 FORMAT(38X,2(1PE16.4,4H  +-,E12.4,0PF10.3))                       OUPAR118
 6000 FORMAT(1H )                                                       OUPAR119
      END                                                               OUPAR120
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++PGUSS001
      FUNCTION PGAUSS (X)                                               PGUSS002
C-----------------------------------------------------------------------PGUSS003
C     CALCULATES AREA UNDER NORMAL CURVE (CENTERED AT 0 WITH UNIT       PGUSS004
C     VARIANCE) FROM -(INFINITY) TO X USING ABRAMOWITZ AND STEGUN,      PGUSS005
C     EQ. 26.2.18.                                                      PGUSS006
C     ABS(ERROR) < 2.5E-4                                               PGUSS007
C-----------------------------------------------------------------------PGUSS008
C     SUBPROGRAMS CALLED - NONE                                         PGUSS009
C     COMMON USED        - NONE                                         PGUSS010
C-----------------------------------------------------------------------PGUSS011
      AX    =ABS(X)                                                     PGUSS012
      PGAUSS=1.+AX*(.196854+AX*(.115194+AX*(3.44E-4+AX*.019527)))       PGUSS013
      PGAUSS=1./PGAUSS**2                                               PGUSS014
      PGAUSS=.5*PGAUSS**2                                               PGUSS015
      IF (X .GT. 0.) PGAUSS=1.-PGAUSS                                   PGUSS016
      RETURN                                                            PGUSS017
      END                                                               PGUSS018
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++PIVOT001
      SUBROUTINE PIVOT (A,MDIMA,N,ONLYIN,PMIN,PMAX,P,TOLER,DELTAP,      PIVOT002
     1                  PIV,ALLPIV,QG,QMAX,SRANGE,IERROR)               PIVOT003
C-----------------------------------------------------------------------PIVOT004
C     DOES GAUSS-JORDAN PIVOTS ON ALL N DIAGONAL ELEMENTS OF AUGUMENTED PIVOT005
C     MATRIX A EXCEPT THOSE THAT HAVE A TOLERANCE LESS THAN TOLER OR    PIVOT006
C     THAT CAUSE A VIOLATION OF THE CONSTRAINTS PMIN .LE. P .LE. PMAX.  PIVOT007
C     ONLY NEEDS FULL UPPER TRIANGLE OF MATRIX ON INPUT, AND OVERWRITES PIVOT008
C     IT WITH INVERSE AND SOLUTION.                                     PIVOT009
C-----------------------------------------------------------------------PIVOT010
C                                                                       PIVOT011
C     DESCRIPTION OF PARAMETERS:                                        PIVOT012
C                                                                       PIVOT013
C           A      - REAL ARRAY OF DIMENSION (MDIMA X MDIMA) CONTAINS   PIVOT014
C                    AUGUMENTED MATRIX A WHICH IS TO INVERT             PIVOT015
C           MDIMA  - DIMENSION OF ARRAY A                               PIVOT016
C           N      - ACTUAL SIZE OF ARRAY A (AUGUMENTED SIZE EQUALS N+1)PIVOT017
C           ONLYIN - LOGICAL, HAS TO BE .TRUE. IF ONLY INVERSION OF A   PIVOT018
C                    IS REQUIRED                                        PIVOT019
C           PMIN   - REAL, CONTAINS LOWER BOUNDS FOR PARAMETER P        PIVOT020
C           PMAX   - REAL, CONTAINS UPPER BOUNDS FOR PARAMETER P        PIVOT021
C           P      - REAL ARRAY OF DIMENSION N, CONTAINS PARAMETER      PIVOT022
C                    VECTOR                                             PIVOT023
C           TOLER  - REAL ARRAY OF DIMENSION N, CONTAINS TOLERANCE      PIVOT024
C                    VALUES FOR DIAGONAL ELEMENTS OF A USED AS PIVOTS   PIVOT025
C           DELTAP - REAL ARRAY OF DIMENSION N, CONTAINS ON OUTPUT      PIVOT026
C                    CORRECTION FOR PARAMETER P                         PIVOT027
C           PIV    - LOGICAL ARRAY OF DIMENSION N, IS SET .TRUE. IF     PIVOT028
C                    DIAGONAL ELEMENT OF A WAS USED AS PIVOT,           PIVOT029
C                    OTHERWISE IS SET .FALSE.                           PIVOT030
C           ALLPIV - LOGICAL, IS .TRUE. IF PIV(J)=.TRUE. FOR ALL J      PIVOT031
C           QG     - MAXIMUM STEPLENGTH FOR CORRECTION DELTAP,          PIVOT032
C                    PREVENTS VIOLATING OF CONSTRAINTS (QG.LE.1.)       PIVOT033
C           QMAX   - MAXIMUM STEPLENGTH ALLOWED FOR CORRECTION          PIVOT034
C                    DELTAP, MAY BE > 1.                                PIVOT035
C           SRANGE - REAL, OF ORDER 2 SMALLER THAN BIGGEST NUMBER       PIVOT036
C           IERROR - ERROR FLAG, EQUALS 0 IF NO ERROR OCCURED,          PIVOT037
C                    AND 1 IF ALL ELEMENTS OF A ARE UNPIVOTED           PIVOT038
C                                                                       PIVOT039
C-----------------------------------------------------------------------PIVOT040
C     SUBPROGRAMS CALLED - PIVOT1                                       PIVOT041
C     COMMON USED        - NONE                                         PIVOT042
C-----------------------------------------------------------------------PIVOT043
      DOUBLE PRECISION A, TOLER, DELTAP, ZERO, ONE, DUB, DDUB           PIVOT044
      LOGICAL          ONLYIN, ALLPIV, PIV                              PIVOT045
      DIMENSION        A(MDIMA,MDIMA), P(1), TOLER(1), DELTAP(1), PIV(1)PIVOT046
C     DATA             ZERO/0.E0/, ONE/1.E0/                            PIVOT047
      DATA             ZERO/0.D0/, ONE/1.D0/                            PIVOT048
      DATA             TENFIV/1.E-5/                                    PIVOT049
C-----------------------------------------------------------------------PIVOT050
      IERROR=0                                                          PIVOT051
      QG    =1.                                                         PIVOT052
      QMAX  =999.                                                       PIVOT053
      NP    =N+1                                                        PIVOT054
      IF (ONLYIN) NP=N                                                  PIVOT055
      DO 100 J=1,N                                                      PIVOT056
 100  PIV(J)=.FALSE.                                                    PIVOT057
      IF (NP .EQ. 1) GO TO 150                                          PIVOT058
         DO 120 J=2,NP                                                  PIVOT059
            L=J-1                                                       PIVOT060
            DO 110 K=1,L                                                PIVOT061
 110        A(J,K)=A(K,J)                                               PIVOT062
 120     CONTINUE                                                       PIVOT063
C-----------------------------------------------------------------------PIVOT064
C  MAIN LOOP FOR COMPLETE PIVOTING                                      PIVOT065
C-----------------------------------------------------------------------PIVOT066
 150  DO 250 NPIV=1,N                                                   PIVOT067
C-----------------------------------------------------------------------PIVOT068
C  FIND NEXT PIVOT ELEMENT                                              PIVOT069
C-----------------------------------------------------------------------PIVOT070
         DUB=ZERO                                                       PIVOT071
         DO 200 J=1,N                                                   PIVOT072
            IF (PIV(J) .OR. A(J,J).LE.TOLER(J)) GO TO 200               PIVOT073
            DDUB=A(J,J)                                                 PIVOT074
            IF (.NOT. ONLYIN) DDUB=(A(J,NP)/DDUB)*A(J,NP)               PIVOT075
            IF (DDUB .LE. DUB) GO TO 200                                PIVOT076
            DUB=DDUB                                                    PIVOT077
            L  =J                                                       PIVOT078
 200     CONTINUE                                                       PIVOT079
         IF (DUB .LE. ZERO) GO TO 300                                   PIVOT080
         PIV(L)=.TRUE.                                                  PIVOT081
         CALL PIVOT1 (A,MDIMA,NP,L)                                     PIVOT082
 250  CONTINUE                                                          PIVOT083
      NPIV=N+1                                                          PIVOT084
 300  NPIV=NPIV-1                                                       PIVOT085
      IF (ONLYIN) GO TO 360                                             PIVOT086
C-----------------------------------------------------------------------PIVOT087
C  ENFORCE CONSTRAINTS BY MAKING QG .LT. 1. OR BY UNPIVOTING            PIVOT088
C-----------------------------------------------------------------------PIVOT089
      DO 340 J=1,N                                                      PIVOT090
         DUM=SRANGE                                                     PIVOT091
         DO 320 K=1,N                                                   PIVOT092
            IF (.NOT. PIV(K)) GO TO 320                                 PIVOT093
            RDELTA=ONE/A(K,NP)                                          PIVOT094
            DDUM  =AMAX1((PMIN-P(K))*RDELTA,(PMAX-P(K))*RDELTA)         PIVOT095
            IF (DDUM .GE. DUM) GO TO 320                                PIVOT096
            DUM=DDUM                                                    PIVOT097
            L  =K                                                       PIVOT098
 320     CONTINUE                                                       PIVOT099
         IF (DUM .GT. TENFIV) GO TO 360                                 PIVOT100
         NPIV  =NPIV-1                                                  PIVOT101
         PIV(L)=.FALSE.                                                 PIVOT102
         CALL PIVOT1 (A,MDIMA,NP,L)                                     PIVOT103
 340  CONTINUE                                                          PIVOT104
C-----------------------------------------------------------------------PIVOT105
C  IF ALL ELEMENTS ARE SOMEHOW UNPIVOTED, TAKE ERROR EXIT               PIVOT106
C-----------------------------------------------------------------------PIVOT107
 360  IF (NPIV .GT. 0) GO TO 380                                        PIVOT108
         IERROR=1                                                       PIVOT109
         ALLPIV=.FALSE.                                                 PIVOT110
         RETURN                                                         PIVOT111
 380  IF (ONLYIN) GO TO 400                                             PIVOT112
         QG  =AMIN1(DUM,1.)                                             PIVOT113
         QMAX=DUM                                                       PIVOT114
 400  ALLPIV=NPIV .EQ. N                                                PIVOT115
      IF (ONLYIN) RETURN                                                PIVOT116
      DO 450 J=1,N                                                      PIVOT117
         DELTAP(J)=ZERO                                                 PIVOT118
         IF (PIV(J)) DELTAP(J)=A(J,NP)                                  PIVOT119
 450  CONTINUE                                                          PIVOT120
      RETURN                                                            PIVOT121
      END                                                               PIVOT122
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++PIOT1001
      SUBROUTINE PIVOT1 (A,MDIMA,N,LPIV)                                PIOT1002
C-----------------------------------------------------------------------PIOT1003
C     DOES A GAUSS-JORDAN PIVOT ON DIAGONAL ELEMENT LPIV                PIOT1004
C-----------------------------------------------------------------------PIOT1005
C                                                                       PIOT1006
C     DESCRIPTION OF PARAMETERS:                                        PIOT1007
C                                                                       PIOT1008
C           A      - REAL ARRAY OF DIMENSION MDIMA X N, CONTAINS        PIOT1009
C                    MATRIX TO INVERT                                   PIOT1010
C           MDIMA  - DIMENSION OF ARRAY A AS DEFINED IN CALLING         PIOT1011
C                    PROGRAM                                            PIOT1012
C           N      - ACTUAL DIMENSION OF MATRIX A                       PIOT1013
C           LPIV   - INDEX OF DIAGONAL ELEMENT USED FOR PIVOTING        PIOT1014
C                                                                       PIOT1015
C-----------------------------------------------------------------------PIOT1016
C     SUBPROGRAMS CALLED - NONE                                         PIOT1017
C     COMMON USED        - NONE                                         PIOT1018
C-----------------------------------------------------------------------PIOT1019
      DOUBLE PRECISION A, DUB, DDUB, ONE                                PIOT1020
      DIMENSION        A(MDIMA,MDIMA)                                   PIOT1021
C     DATA             ONE/1.E0/                                        PIOT1022
      DATA             ONE/1.D0/                                        PIOT1023
C-----------------------------------------------------------------------PIOT1024
      DUB=ONE/A(LPIV,LPIV)                                              PIOT1025
      DO 200 J=1,N                                                      PIOT1026
         IF (J .EQ. LPIV) GO TO 200                                     PIOT1027
         DDUB=A(J,LPIV)*DUB                                             PIOT1028
         DO 100 K=1,N                                                   PIOT1029
            IF (K .NE. LPIV) A(J,K)=A(J,K)-A(LPIV,K)*DDUB               PIOT1030
 100     CONTINUE                                                       PIOT1031
 200  CONTINUE                                                          PIOT1032
      DO 300 J=1,N                                                      PIOT1033
         A(J,LPIV)=-A(J,LPIV)*DUB                                       PIOT1034
         A(LPIV,J)= A(LPIV,J)*DUB                                       PIOT1035
 300  CONTINUE                                                          PIOT1036
      A(LPIV,LPIV)=DUB                                                  PIOT1037
      RETURN                                                            PIOT1038
      END                                                               PIOT1039
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++PLRIN001
      SUBROUTINE PLPRIN (NYMAX,X,Y1,Y2,NY,ONLY1,NOUT,SRANGE,KSTART,KEND)PLRIN002
C-----------------------------------------------------------------------PLRIN003
C     PLOTS Y1(K) (AND Y2(K), IF ONLY1=.FALSE.) VS. X ON LINE PRINTER,  PLRIN004
C     AND PRINTS Y1(K) AND X(K) FOR K=KSTART,...,KEND.                  PLRIN005
C     IF KSTART.LE.0 OR KEND.LT.KSTART THEN KSTART=1 AND KEND=NY.       PLRIN006
C-----------------------------------------------------------------------PLRIN007
C     SUBPROGRAMS CALLED - NONE                                         PLRIN008
C     COMMON USED        - NONE                                         PLRIN009
C-----------------------------------------------------------------------PLRIN010
      LOGICAL          ONLY1                                            PLRIN011
      DIMENSION        X(*), Y1(*), Y2(*), ICHAR(4), IH(108)            PLRIN012
      DATA             ICHAR/1H , 1HX, 1HO, 1H*/                        PLRIN013
C-----------------------------------------------------------------------PLRIN014
      YMIN  = SRANGE                                                    PLRIN015
      YMAX  =-SRANGE                                                    PLRIN016
      JSTART=KSTART                                                     PLRIN017
      JEND  =KEND                                                       PLRIN018
      IF (JSTART.GT.0 .AND. JEND.GE.JSTART .AND. JEND.LE.NY) GOTO 100   PLRIN019
         JSTART=1                                                       PLRIN020
         JEND  =NY                                                      PLRIN021
 100  DO 150 J=JSTART,JEND                                              PLRIN022
         YMIN=AMIN1(YMIN,Y1(J))                                         PLRIN023
         YMAX=AMAX1(YMAX,Y1(J))                                         PLRIN024
         IF (ONLY1) GO TO 150                                           PLRIN025
         YMIN=AMIN1(YMIN,Y2(J))                                         PLRIN026
         YMAX=AMAX1(YMAX,Y2(J))                                         PLRIN027
 150  CONTINUE                                                          PLRIN028
      DUM=YMAX-YMIN                                                     PLRIN029
      IF (DUM .LE. 0.) DUM=1.                                           PLRIN030
      R=107.99/DUM                                                      PLRIN031
      WRITE (NOUT,1000)                                                 PLRIN032
      L2=1                                                              PLRIN033
      DO 300 J=JSTART,JEND                                              PLRIN034
         DO 200 L1=1,108                                                PLRIN035
 200     IH(L1)=ICHAR(1)                                                PLRIN036
         L1    =INT((Y1(J)-YMIN)*R)+1                                   PLRIN037
         IH(L1)=ICHAR(2)                                                PLRIN038
         IF (ONLY1) GO TO 250                                           PLRIN039
            L2    =INT((Y2(J)-YMIN)*R)+1                                PLRIN040
            IH(L2)=ICHAR(3)                                             PLRIN041
            IF (L1 .EQ. L2) IH(L2)=ICHAR(4)                             PLRIN042
 250     WRITE (NOUT,2000) Y1(J),X(J),IH                                PLRIN043
 300  CONTINUE                                                          PLRIN044
      RETURN                                                            PLRIN045
 1000 FORMAT(//4X,8HORDINATE,3X,8HABSCISSA)                             PLRIN046
 2000 FORMAT(1X,1P2E11.3,108A1)                                         PLRIN047
      END                                                               PLRIN048
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++PLRES001
      SUBROUTINE PLRES (NYMAX,YLYFIT,NY,PRUNS,NOUT,LINEPG,NDSET)        PLRES002
C-----------------------------------------------------------------------PLRES003
C     PLOTS RESIDUALS ON PRINTER WITH X-AXIS HORIZONTAL.                PLRES004
C-----------------------------------------------------------------------PLRES005
C     SUBPROGRAMS CALLED - ERRMES                                       PLRES006
C     COMMON USED        - NONE                                         PLRES007
C-----------------------------------------------------------------------PLRES008
      DIMENSION        YLYFIT(*),                                       PLRES009
     1                 JCHAR(8), LINE(131), LABEL(6), BOUND(21),        PLRES010
     2                 LCHARJ(20), LINE1(20), IHOLER(6)                 PLRES011
      DATA             JCHAR /1H*, 1H-, 1HU, 1HL, 1H , 1H0, 1H-, 1H+/,  PLRES012
     1                 IHOLER/1HP, 1HL, 1HR, 1HE, 1HS, 1H /, MPAGE/30/  PLRES013
C-----------------------------------------------------------------------PLRES014
      IF (LINEPG .GE. 17) GO TO 100                                     PLRES015
         CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                            PLRES016
         RETURN                                                         PLRES017
 100  MPLOT=(LINEPG-3)/16                                               PLRES018
      MLINE=MIN0(20,(LINEPG-3)/MPLOT-3)                                 PLRES019
      RMIN =YLYFIT(1)                                                   PLRES020
      RMAX =YLYFIT(1)                                                   PLRES021
      JAXIS=0                                                           PLRES022
      DO 150 J=2,NY                                                     PLRES023
         RMIN=AMIN1(RMIN,YLYFIT(J))                                     PLRES024
         RMAX=AMAX1(RMAX,YLYFIT(J))                                     PLRES025
 150  CONTINUE                                                          PLRES026
      WRITE (NOUT,1000) NDSET,RMAX,RMIN,PRUNS                           PLRES027
      IF (RMIN .NE. RMAX) GO TO 170                                     PLRES028
         WRITE (NOUT,1500)                                              PLRES029
         RETURN                                                         PLRES030
 170  DELTA   =(RMAX-RMIN)/FLOAT(MLINE-1)                               PLRES031
      BOUND(1)=RMAX+.5*DELTA                                            PLRES032
      K       =MLINE+1                                                  PLRES033
      DO 200 J=2,K                                                      PLRES034
         BOUND(J)=BOUND(J-1)-DELTA                                      PLRES035
         IF (BOUND(J)*BOUND(J-1) .LT. 0.) JAXIS=J-1                     PLRES036
 200  CONTINUE                                                          PLRES037
      IF (JAXIS.LE.0 .AND. RMAX.LT.0.) JAXIS=1                          PLRES038
      IF (JAXIS.LE.0 .AND. RMAX.GT.0.) JAXIS=MLINE                      PLRES039
      LABEL(1)=-110                                                     PLRES040
      DO 250 J=2,6                                                      PLRES041
 250  LABEL(J)=LABEL(J-1)+20                                            PLRES042
      K=MLINE-1                                                         PLRES043
      DO 300 J=2,K                                                      PLRES044
         LINE1(J) =JCHAR(5)                                             PLRES045
         LCHARJ(J)=5                                                    PLRES046
 300  CONTINUE                                                          PLRES047
      LINE1(1)     =JCHAR(3)                                            PLRES048
      LINE1(JAXIS) =JCHAR(6)                                            PLRES049
      LINE1(MLINE) =JCHAR(4)                                            PLRES050
      LCHARJ(1)    =7                                                   PLRES051
      LCHARJ(JAXIS)=2                                                   PLRES052
      LCHARJ(MLINE)=7                                                   PLRES053
      NPOINT       =0                                                   PLRES054
      DO 500 NPAGE=1,MPAGE                                              PLRES055
         IF (NPAGE .GT. 1) WRITE (NOUT,2000)                            PLRES056
         DO 450 NPLOT=1,MPLOT                                           PLRES057
            WRITE (NOUT,3000)                                           PLRES058
            NST   =NPOINT+1                                             PLRES059
            NEND  =NPOINT+130                                           PLRES060
            NPOINT=NEND                                                 PLRES061
            NLIM  =MIN0(NEND,NY)                                        PLRES062
            DO 400 NLINE=1,MLINE                                        PLRES063
               LCHAR  =LCHARJ(NLINE)                                    PLRES064
               LINE(1)=LINE1(NLINE)                                     PLRES065
               BMAX   =BOUND(NLINE)                                     PLRES066
               BMIN   =BOUND(NLINE+1)                                   PLRES067
               K      =1                                                PLRES068
               DO 350 J=NST,NLIM                                        PLRES069
                  K      =K+1                                           PLRES070
                  LINE(K)=JCHAR(LCHAR)                                  PLRES071
                  IF (YLYFIT(J).LT.BMAX .AND. YLYFIT(J).GE.BMIN)        PLRES072
     1                                          LINE(K)=JCHAR(1)        PLRES073
 350           CONTINUE                                                 PLRES074
               K=NLIM-NST+2                                             PLRES075
               IF (NLINE.NE.1 .AND. NLINE.NE.MLINE) GO TO 390           PLRES076
                  DO 380 J=11,K,10                                      PLRES077
 380              IF (LINE(J) .NE. JCHAR(1)) LINE(J)=JCHAR(8)           PLRES078
 390           WRITE (NOUT,4000) (LINE(J),J=1,K)                        PLRES079
 400        CONTINUE                                                    PLRES080
            DO 420 J=1,6                                                PLRES081
 420        LABEL(J)=LABEL(J)+130                                       PLRES082
            WRITE (NOUT,5000) LABEL                                     PLRES083
            IF (NLIM .EQ. NY) RETURN                                    PLRES084
 450     CONTINUE                                                       PLRES085
 500  CONTINUE                                                          PLRES086
      CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                               PLRES087
      RETURN                                                            PLRES088
 1000 FORMAT(1H1,//40H PLOT OF WEIGHTED RESIDUALS FOR DATA SET,I3,3X,   PLRES089
     1              9HMAX = U =,1PE8.1,5X,9HMIN = L =,1PE8.1,9X,        PLRES090
     2             19HRANDOM RUNS PROB. =,0PF7.4)                       PLRES091
 1500 FORMAT(//24H RESIDUALS ARE CONSTANT.,2X,50(2H**))                 PLRES092
 2000 FORMAT(1H1)                                                       PLRES093
 3000 FORMAT(1H )                                                       PLRES094
 4000 FORMAT(1X,A1,130A1)                                               PLRES095
 5000 FORMAT(3X,6(16X,I4)/)                                             PLRES096
      END                                                               PLRES097
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++RADOM001
      FUNCTION RANDOM (DIX)                                             RADOM002
C-----------------------------------------------------------------------RADOM003
C     PRODUCES A PSEUDORANDOM REAL ON THE OPEN INTERVAL (0.,1.).        RADOM004
C     DIX (IN DOUBLE PRECISION) MUST BE INITIALIZED TO A WHOLE NUMBER   RADOM005
C     BETWEEN 1.D0 AND 2147483646.D0 BEFORE FIRST CALL TO RANDOM        RADOM006
C     AND NOT CHANGED BETWEEN SUCCESSIVE CALLS TO RANDOM.               RADOM007
C     BASED ON L.SCHRAGE, ACM TRANS. ON MATH. SOFTWARE 5, 132 (1979).   RADOM008
C                                                                       RADOM009
C     PORTABLE RANDOM NUMBER GENERATOR USING THE RECURSION              RADOM010
C                       DIX = DIX * A MOD P                             RADOM011
C-----------------------------------------------------------------------RADOM012
C     SUBPROGRAMS CALLED - NONE                                         RADOM013
C     COMMON USED        - NONE                                         RADOM014
C-----------------------------------------------------------------------RADOM015
      DOUBLE PRECISION A, P, DIX, B15, B16, XHI, XALO, LEFTLO, FHI, K   RADOM016
C-----------------------------------------------------------------------RADOM017
C  7**5, 2**15, 2**16, 2**31-1                                          RADOM018
C-----------------------------------------------------------------------RADOM019
      DATA             A/16807.D0/, B15/32768.D0/, B16/65536.D0/,       RADOM020
     1                 P/2147483647.D0/                                 RADOM021
C-----------------------------------------------------------------------RADOM022
C  GET 15 HI ORDER BITS OF DIX                                          RADOM023
C-----------------------------------------------------------------------RADOM024
      XHI=DIX/B16                                                       RADOM025
      XHI=XHI-DMOD(XHI,1.D0)                                            RADOM026
C-----------------------------------------------------------------------RADOM027
C  GET 16 LO BITS IF DIX AND FORM LO PRODUCT                            RADOM028
C-----------------------------------------------------------------------RADOM029
      XALO=(DIX-XHI*B16)*A                                              RADOM030
C-----------------------------------------------------------------------RADOM031
C  GET 15 HI ORDER BITS OF LO PRODUCT                                   RADOM032
C-----------------------------------------------------------------------RADOM033
      LEFTLO=XALO/B16                                                   RADOM034
      LEFTLO=LEFTLO-DMOD(LEFTLO,1.D0)                                   RADOM035
C-----------------------------------------------------------------------RADOM036
C  FORM THE 31 HIGHEST BITS OF FULL PRODUCT                             RADOM037
C-----------------------------------------------------------------------RADOM038
      FHI=XHI*A+LEFTLO                                                  RADOM039
C-----------------------------------------------------------------------RADOM040
C  GET OVERFLO PAST 31ST BIT OF FULL PRODUCT                            RADOM041
C-----------------------------------------------------------------------RADOM042
      K=FHI/B15                                                         RADOM043
      K=K-DMOD(K,1.D0)                                                  RADOM044
C-----------------------------------------------------------------------RADOM045
C  ASSEMBLE ALL THE PARTS AND PRESUBTRACT P                             RADOM046
C  THE PARENTHESES ARE ESSENTIAL                                        RADOM047
C-----------------------------------------------------------------------RADOM048
      DIX=(((XALO-LEFTLO*B16)-P)+(FHI-K*B15)*B16)+K                     RADOM049
C-----------------------------------------------------------------------RADOM050
C  ADD P BACK IN IF NECESSARY                                           RADOM051
C-----------------------------------------------------------------------RADOM052
      IF (DIX .LT. 0.D0) DIX=DIX+P                                      RADOM053
C-----------------------------------------------------------------------RADOM054
C  MULTIPLY BY 1/(2**31-1)                                              RADOM055
C-----------------------------------------------------------------------RADOM056
      RANDOM=DIX*4.656612875D-10                                        RADOM057
      RETURN                                                            RADOM058
      END                                                               RADOM059
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++REDYT001
      SUBROUTINE READYT (NYMAX,Y,T,SQRTW,NY,NIOERR,RESPON)              REDYT002
C-----------------------------------------------------------------------REDYT003
C     READS Y(K,N) (INPUT DATA), T(K,N) (INDEPENDENT VARIABLE), AND,    REDYT004
C     IF IWT=4, LEAST SQUARES WEIGHTS, FOR K=1,..,NY(N), N=1,..,ND.     REDYT005
C     IF DOUSIN=.TRUE., THEN USERIN IS CALLED TO RECOMPUTE OR CHANGE    REDYT006
C     INPUT DATA.                                                       REDYT007
C-----------------------------------------------------------------------REDYT008
C     SUBPROGRAMS CALLED - ERRMES, USERIN                               REDYT009
C     COMMON USED        - IBLOCK, LBLOCK                               REDYT010
C-----------------------------------------------------------------------REDYT011
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  REDYT012
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    REDYT013
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  REDYT014
     3                 SPLINE, WRTBET, WRTCAP                           REDYT015
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), SQRTW(NYMAX,1),          REDYT016
     1                 NY(*), RESPON(*), LIN(6), LA(6,3), IHOLER(6)     REDYT017
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       REDYT018
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       REDYT019
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   REDYT020
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    REDYT021
     4                 MTRY(10,2), MXITER(2), NL(10),                   REDYT022
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  REDYT023
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, REDYT024
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       REDYT025
     8                 NIN, NOUT                                        REDYT026
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          REDYT027
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         REDYT028
     2                 DOUSOU(2), LUSER(30),                            REDYT029
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  REDYT030
     4                 SPLINE, WRTBET, WRTCAP                           REDYT031
      DATA             IHOLER/1HR, 1HE, 1HA, 1HD, 1HY, 1HT/,            REDYT032
     1                 LA/1HN, 1HS, 1HT, 1HE, 1HN, 1HD,                 REDYT033
     2                    1HN, 1HY,           4 *  1H ,                 REDYT034
     3                    1HN, 1HI, 1HN, 1HT, 1HT, 1H /                 REDYT035
C-----------------------------------------------------------------------REDYT036
      NYSUM=0                                                           REDYT037
      DO 500 N=1,ND                                                     REDYT038
         IF (N.EQ.1 .OR. .NOT.SAMET) GO TO 100                          REDYT039
            MY=NY(1)                                                    REDYT040
            GO TO 300                                                   REDYT041
 100     READ (NIN,1000) LIN,MY                                         REDYT042
         WRITE (NOUT,2000) LIN,MY                                       REDYT043
         I=2                                                            REDYT044
         IF (LIN(2) .EQ. LA(2,3)) I=3                                   REDYT045
         DO 110 K=1,6                                                   REDYT046
            IF (LIN(K) .EQ. LA(K,I)) GO TO 110                          REDYT047
            CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                         REDYT048
            NIOERR=NIOERR+1                                             REDYT049
            IF (NIOERR .GE. MIOERR) STOP                                REDYT050
 110     CONTINUE                                                       REDYT051
         IF (MY .LE. 0) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)              REDYT052
         IF (I .EQ. 2) GO TO 200                                        REDYT053
         NINTT=MY                                                       REDYT054
C-----------------------------------------------------------------------REDYT055
C  COMPUTE T IN EQUAL INTERVALS.                                        REDYT056
C-----------------------------------------------------------------------REDYT057
         MY=0                                                           REDYT058
         DO 190 J=1,NINTT                                               REDYT059
            READ (NIN,1000) LIN,NT,TSTART,TEND                          REDYT060
            WRITE (NOUT,2000) LIN,NT,TSTART,TEND                        REDYT061
            DO 120 K=1,6                                                REDYT062
               IF (LIN(K) .EQ. LA(K,1)) GO TO 120                       REDYT063
               CALL ERRMES (3,.FALSE.,IHOLER,NOUT)                      REDYT064
               GO TO 180                                                REDYT065
 120        CONTINUE                                                    REDYT066
            IF (NT.GE.2 .AND. NT+MY.LE.NYMAX) GO TO 150                 REDYT067
               CALL ERRMES (4,.FALSE.,IHOLER,NOUT)                      REDYT068
               GO TO 180                                                REDYT069
 150        DUM    =(TEND-TSTART)/FLOAT(NT-1)                           REDYT070
            MY     =MY+1                                                REDYT071
            T(MY,N)=TSTART                                              REDYT072
            DO 160 K=2,NT                                               REDYT073
               MY     =MY+1                                             REDYT074
               T(MY,N)=T(MY-1,N)+DUM                                    REDYT075
 160        CONTINUE                                                    REDYT076
            GO TO 190                                                   REDYT077
 180        NIOERR=NIOERR+1                                             REDYT078
            IF (NIOERR .GE. MIOERR) STOP                                REDYT079
 190     CONTINUE                                                       REDYT080
         GO TO 300                                                      REDYT081
C-----------------------------------------------------------------------REDYT082
C  CASE OF NON REGULAR INTERVALS IN T. READ IN T ARRAY.                 REDYT083
C-----------------------------------------------------------------------REDYT084
 200     IF (MY .LE. NYMAX) GO TO 240                                   REDYT085
            CALL ERRMES (5,.FALSE.,IHOLER,NOUT)                         REDYT086
            NIOERR=NIOERR+1                                             REDYT087
            RETURN                                                      REDYT088
 240     READ (NIN,IFORMT) (T(K,N),K=1,MY)                              REDYT089
C-----------------------------------------------------------------------REDYT090
C  READ IN Y ARRAY.                                                     REDYT091
C-----------------------------------------------------------------------REDYT092
 300     IF (.NOT. SIMULA) READ (NIN,IFORMY) (Y(K,N),K=1,MY)            REDYT093
         IF (IWT .EQ. 4) GO TO 420                                      REDYT094
C-----------------------------------------------------------------------REDYT095
C  INITIALIZE SQRTW (SQUARE ROOTS OF LEAST SQUARES WEIGHTS) TO UNITY.   REDYT096
C-----------------------------------------------------------------------REDYT097
            DO 410 K=1,MY                                               REDYT098
 410        SQRTW(K,N)=1.                                               REDYT099
C-----------------------------------------------------------------------REDYT100
C  READ IN LEAST SQUARES WEIGHTS IF IWT=4.                              REDYT101
C-----------------------------------------------------------------------REDYT102
 420     IF (IWT .EQ. 4) READ (NIN,IFORMW) (SQRTW(K,N),K=1,MY)          REDYT103
         NY(N)=MY                                                       REDYT104
         NYSUM=NYSUM+MY                                                 REDYT105
 500  CONTINUE                                                          REDYT106
C-----------------------------------------------------------------------REDYT107
C  COMPUTE T(K,N), N=2,...,ND IF SAMET=.TRUE.  .AND.  ND > 1            REDYT108
C-----------------------------------------------------------------------REDYT109
      IF (.NOT.SAMET .OR. ND.EQ.1) GO TO 550                            REDYT110
         DO 540 N=2,ND                                                  REDYT111
         DO 540 K=1,MY                                                  REDYT112
 540     T(K,N)=T(K,1)                                                  REDYT113
C-----------------------------------------------------------------------REDYT114
C  CALL USERIN TO CHANGE OR RECOMPUTE INPUT DATA.                       REDYT115
C-----------------------------------------------------------------------REDYT116
 550  IF (DOUSIN) CALL USERIN (NYMAX,Y,T,SQRTW,NY,RESPON)               REDYT117
      DO 700 N=1,ND                                                     REDYT118
         MY=NY(N)                                                       REDYT119
         DO 650 K=1,MY                                                  REDYT120
            IF (SQRTW(K,N) .GE. 0.) GO TO 600                           REDYT121
               CALL ERRMES (6,.FALSE.,IHOLER,NOUT)                      REDYT122
               WRITE (NOUT,3000) (SQRTW(J,N),J=1,MY)                    REDYT123
               NIOERR=NIOERR+1                                          REDYT124
               RETURN                                                   REDYT125
 600        SQRTW(K,N)=SQRT(SQRTW(K,N))                                 REDYT126
 650     CONTINUE                                                       REDYT127
 700  CONTINUE                                                          REDYT128
      RETURN                                                            REDYT129
 1000 FORMAT(1X,6A1,I5,  2E15.6)                                        REDYT130
 2000 FORMAT(1X,6A1,I5,1P2E15.5)                                        REDYT131
 3000 FORMAT(1X,1P10E13.5)                                              REDYT132
      END                                                               REDYT133
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++RGUSS001
      SUBROUTINE RGAUSS (X1,X2,TWOPI,DIX)                               RGUSS002
C-----------------------------------------------------------------------RGUSS003
C     AN EXACT METHOD FOR GENERATING X1 AND X2, TWO STANDARD            RGUSS004
C     (ZERO MEAN, UNIT VARIANCE) NORMAL RANDOM DEVIATES FROM 2 CALLS    RGUSS005
C     TO RANDOM, WHICH DELIVERS A UNIFORM RANDOM DEVIATE ON THE         RGUSS006
C     INTERVALL (0,1). (SEE M.C.PIKE, ALGORITHM 267 FROM CACM)          RGUSS007
C     TWOPI = 2. * PI                                                   RGUSS008
C     DIX IS EXPLAINED IN RANDOM                                        RGUSS009
C     IF A VERY LARGE NUMBER OF DEVIATES ARE TO BE GENERATED,           RGUSS010
C     FASTER ROUTINES EXIST.                                            RGUSS011
C-----------------------------------------------------------------------RGUSS012
C     SUBPROGRAMS CALLED - RANDOM                                       RGUSS013
C     COMMON USED        - NONE                                         RGUSS014
C-----------------------------------------------------------------------RGUSS015
      DOUBLE PRECISION DIX                                              RGUSS016
C-----------------------------------------------------------------------RGUSS017
      X1 =SQRT(-2.*ALOG(RANDOM(DIX)))                                   RGUSS018
      DUM=TWOPI*RANDOM(DIX)                                             RGUSS019
      X2 =X1*SIN(DUM)                                                   RGUSS020
      X1 =X1*COS(DUM)                                                   RGUSS021
      RETURN                                                            RGUSS022
      END                                                               RGUSS023
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++SETWT001
      SUBROUTINE SETWT (NYMAX,Y,T,YLYFIT,SQRTW,NY,ND,ERRFIT,NERFIT,     SETWT002
     1                  IWT,PRWT,SRANGE,NOUT,RESPON)                    SETWT003
C-----------------------------------------------------------------------SETWT004
C     COMPUTES SQRTW (SQUARE-ROOTS OF THE LEAST SQUARES WEIGHTS)        SETWT005
C     USING ABS(Y-YLYFIT)=YFIT (THE ABSOLUTE VALUE OF THE FIT TO THE    SETWT006
C     DATA FROM A PRELIMINARY UNWEIGHTED SOLUTION).                     SETWT007
C     ONLY CALLED IF IWT=2,3, OR 5.                                     SETWT008
C     IWT = 2 WHEN VARIANCE (Y) IS PROPORTIONAL TO ABS(Y) (AS WITH      SETWT009
C             POISSON STATISTICS).                                      SETWT010
C     IWT = 3 WHEN VARIANCE (Y) IS PROPORTIONAL TO Y**2.                SETWT011
C     IWT = 5 WHEN SQRTW IS TO BE CALCULATED BY THE USER-SUPPLIED       SETWT012
C             ROUTINE USERWT.                                           SETWT013
C-----------------------------------------------------------------------SETWT014
C     SUBPROGRAMS CALLED - ERRMES, USERWT                               SETWT015
C     COMMON USED        - NONE                                         SETWT016
C-----------------------------------------------------------------------SETWT017
      LOGICAL          PRWT                                             SETWT018
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), YLYFIT(NYMAX,1),         SETWT019
     1                 SQRTW(NYMAX,1), NY(*), ERRFIT(*), RESPON(*)      SETWT020
      DIMENSION        IHOLER(6)                                        SETWT021
      DATA             IHOLER/1HS, 1HE, 1HT, 1HW, 1HT, 1H /             SETWT022
C-----------------------------------------------------------------------SETWT023
      DO 100 N=1,ND                                                     SETWT024
 100  ERRFIT(N)=0.                                                      SETWT025
      IF (NERFIT .LE. 0) GO TO 200                                      SETWT026
C-----------------------------------------------------------------------SETWT027
C  COMPUTE ERRFIT (ROOT-MEAN-SQUARE DEVIATION IN THE REGION AROUND      SETWT028
C  THE MIN. VALUE OF YFIT).  ERRFIT WILL BE ADDED TO YFIT IN ORDER      SETWT029
C  TO PREVENT DISASTROUSLY LARGE WEIGHTS BEING CALCULATED FROM          SETWT030
C  1./YFIT, WHEN YFIT HAPPENS TO BE NEAR 0.                             SETWT031
C  NERFIT SHOULD TYPICALLY BE ABOUT 10.                                 SETWT032
C-----------------------------------------------------------------------SETWT033
      DO 150 N=1,ND                                                     SETWT034
         ABSMIN=SRANGE                                                  SETWT035
         MY=NY(N)                                                       SETWT036
         DO 110 K=1,MY                                                  SETWT037
            DUM=ABS(Y(K,N)-YLYFIT(K,N))                                 SETWT038
            IF (DUM .GE. ABSMIN) GO TO 110                              SETWT039
            ABSMIN=DUM                                                  SETWT040
            L     =K                                                    SETWT041
 110     CONTINUE                                                       SETWT042
         KMAX=MIN0(MY,L+NERFIT/2)                                       SETWT043
         KMIN=MAX0(1,KMAX-NERFIT+1)                                     SETWT044
         DUM=0.                                                         SETWT045
         DO 120 K=KMIN,KMAX                                             SETWT046
 120     DUM=DUM+YLYFIT(K,N)**2                                         SETWT047
         ERRFIT(N)=SQRT(DUM/FLOAT(KMAX-KMIN+1))                         SETWT048
 150  CONTINUE                                                          SETWT049
 200  IF (IWT .NE. 5) GO TO 250                                         SETWT050
         CALL USERWT (NYMAX,Y,T,YLYFIT,SQRTW,NY,ERRFIT,RESPON)          SETWT051
         GO TO 700                                                      SETWT052
 250  IF (IWT.NE.2 .AND. IWT.NE.3) CALL ERRMES (1,.TRUE.,IHOLER,NOUT)   SETWT053
      DO 300 N=1,ND                                                     SETWT054
      MY=NY(N)                                                          SETWT055
      DO 300 K=1,MY                                                     SETWT056
         DUM=AMAX1(ABS(Y(K,N)-YLYFIT(K,N)),ERRFIT(N))                   SETWT057
         IF (DUM .LE. 0.) CALL ERRMES (2,.TRUE.,IHOLER,NOUT)            SETWT058
         SQRTW(K,N)=1./DUM                                              SETWT059
         IF (IWT .EQ. 2) SQRTW(K,N)=SQRT(SQRTW(K,N))                    SETWT060
 300  CONTINUE                                                          SETWT061
 700  IF (.NOT. PRWT) RETURN                                            SETWT062
      DO 800 N=1,ND                                                     SETWT063
         MY=NY(N)                                                       SETWT064
         WRITE (NOUT,1000) N,ERRFIT(N),(SQRTW(K,N),K=1,MY)              SETWT065
 800  CONTINUE                                                          SETWT066
      RETURN                                                            SETWT067
 1000 FORMAT(//20H ERRFIT FOR DATA SET,I3,4H   =,1PE9.2//47X,           SETWT068
     1         37HSQUARE ROOTS OF LEAST SQUARES WEIGHTS//(1X,1P10E13.4))SETWT069
      END                                                               SETWT070
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++STRIN001
      SUBROUTINE STORIN (JL,NIOERR,LIN,IIN,RIN)                         STRIN002
C-----------------------------------------------------------------------STRIN003
C     STORES INPUT DATA AFTER CHECKING THAT ARRAY DIMENSIONS            STRIN004
C     WILL NOT BE EXCEEDED.                                             STRIN005
C-----------------------------------------------------------------------STRIN006
C     SUBPROGRAMS CALLED - ERRMES                                       STRIN007
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               STRIN008
C-----------------------------------------------------------------------STRIN009
      DOUBLE PRECISION PRECIS, RANGE,                                   STRIN010
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       STRIN011
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           STRIN012
     3                 TWOTHR, FORTHR, SIXTEL                           STRIN013
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  STRIN014
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    STRIN015
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  STRIN016
     3                 SPLINE, WRTBET, WRTCAP                           STRIN017
      LOGICAL          LEQUIV                                           STRIN018
      DIMENSION        LIN(6), IEQUIV(13), LEQUIV(6), IHOLER(6)         STRIN019
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        STRIN020
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, STRIN021
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 STRIN022
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),STRIN023
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     STRIN024
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       STRIN025
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       STRIN026
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   STRIN027
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    STRIN028
     4                 MTRY(10,2), MXITER(2), NL(10),                   STRIN029
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  STRIN030
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, STRIN031
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       STRIN032
     8                 NIN, NOUT                                        STRIN033
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          STRIN034
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         STRIN035
     2                 DOUSOU(2), LUSER(30),                            STRIN036
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  STRIN037
     4                 SPLINE, WRTBET, WRTCAP                           STRIN038
      EQUIVALENCE      (IRWCAP,IEQUIV(1)), (DOUSIN,LEQUIV(1))           STRIN039
      DATA             IHOLER/1HS, 1HT, 1HO, 1HR, 1HI, 1HN/             STRIN040
C-----------------------------------------------------------------------STRIN041
      IFINT(RIN)=INT(RIN+SIGN(.5,RIN))                                  STRIN042
C-----------------------------------------------------------------------STRIN043
      IF (JL .GT. 1) GO TO 200                                          STRIN044
         CONVRG=RIN                                                     STRIN045
         RETURN                                                         STRIN046
 200  IF (JL .GT. 4) GO TO 300                                          STRIN047
         IF (JL-3) 210,220,230                                          STRIN048
 210     PLMNMX(IIN)=RIN                                                STRIN049
         RETURN                                                         STRIN050
 220     PNMNMX(IIN)=RIN                                                STRIN051
         RETURN                                                         STRIN052
C***********************************************************************STRIN053
C  IF YOU CHANGE THE DIMENSION OF RUSER IN COMMON, THEN YOU MUST ALSO   STRIN054
C  CHANGE 100 IN THE FOLLOWING STATEMENT TO THE NEW DIMENSION.          STRIN055
C***********************************************************************STRIN056
 230     IF (IIN.LT.1 .OR. IIN.GT.100) GO TO 900                        STRIN057
         RUSER(IIN)=RIN                                                 STRIN058
         RETURN                                                         STRIN059
 300  IF (JL .GT. 17) GO TO 400                                         STRIN060
         IEQUIV(JL-4)=IFINT(RIN)                                        STRIN061
         RETURN                                                         STRIN062
 400  IF (JL .GT. 28) GO TO 600                                         STRIN063
         JLL=JL-17                                                      STRIN064
         GO TO (490,500,510,520,530,540,550,560,570,580,590),JLL        STRIN065
 490     READ (NIN,1000) IFORMT                                         STRIN066
         WRITE (NOUT,1000) IFORMT                                       STRIN067
         RETURN                                                         STRIN068
 500     READ (NIN,1000) IFORMW                                         STRIN069
         WRITE (NOUT,1000) IFORMW                                       STRIN070
         RETURN                                                         STRIN071
 510     READ (NIN,1000) IFORMY                                         STRIN072
         WRITE (NOUT,1000) IFORMY                                       STRIN073
         RETURN                                                         STRIN074
 520     IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 900                          STRIN075
         IPLFIT(IIN)=IFINT(RIN)                                         STRIN076
         RETURN                                                         STRIN077
 530     IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 900                          STRIN078
         IPLRES(IIN)=IFINT(RIN)                                         STRIN079
         RETURN                                                         STRIN080
 540     READ (NIN,2000) IPRINT                                         STRIN081
         WRITE (NOUT,2000) IPRINT                                       STRIN082
         RETURN                                                         STRIN083
 550     IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 900                          STRIN084
         IPRITR(IIN)=IFINT(RIN)                                         STRIN085
         RETURN                                                         STRIN086
C***********************************************************************STRIN087
C  IF YOU CHANGE THE DIMENSION OF IUSER IN COMMON, THEN YOU MUST ALSO   STRIN088
C  CHANGE 50 IN THE FOLLOWING STATEMENT TO THE NEW DIMENSION.           STRIN089
C***********************************************************************STRIN090
 560     IF (IIN.LT.1 .OR. IIN.GT.50) GO TO 900                         STRIN091
         IUSER(IIN)=IFINT(RIN)                                          STRIN092
         RETURN                                                         STRIN093
 570     READ (NIN,2000) MTRY                                           STRIN094
         WRITE (NOUT,2000) MTRY                                         STRIN095
         RETURN                                                         STRIN096
 580     IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 900                          STRIN097
         MXITER(IIN)=IFINT(RIN)                                         STRIN098
         RETURN                                                         STRIN099
 590     IF (IIN.LT.1 .OR. IIN.GT.10) GO TO 900                         STRIN100
         NL(IIN)=IFINT(RIN)                                             STRIN101
         RETURN                                                         STRIN102
 600  L=IFINT(RIN)                                                      STRIN103
      IF (JL .GT. 34) GO TO 700                                         STRIN104
         IF (IABS(L) .EQ. 1) GO TO 610                                  STRIN105
            CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                         STRIN106
            GO TO 910                                                   STRIN107
 610     LEQUIV(JL-28)=L .EQ. 1                                         STRIN108
         RETURN                                                         STRIN109
 700  JLL=JL-34                                                         STRIN110
      GO TO (760,770,780,790,800),JLL                                   STRIN111
 760  READ (NIN,3000) DOADEX                                            STRIN112
      WRITE (NOUT,3000) DOADEX                                          STRIN113
      RETURN                                                            STRIN114
 770  READ (NIN,3000) DOSPL                                             STRIN115
      WRITE (NOUT,3000) DOSPL                                           STRIN116
      RETURN                                                            STRIN117
 780  READ (NIN,3000) DOSTRT                                            STRIN118
      WRITE (NOUT,3000) DOSTRT                                          STRIN119
      RETURN                                                            STRIN120
 790  IF (IIN.LT.1 .OR. IIN.GT.2) GO TO 900                             STRIN121
         IF (IABS(L) .EQ. 1) GO TO 795                                  STRIN122
            CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                         STRIN123
            GO TO 910                                                   STRIN124
 795     DOUSOU(IIN)=L .EQ. 1                                           STRIN125
         RETURN                                                         STRIN126
C***********************************************************************STRIN127
C  IF YOU CHANGE THE DIMENSION OF LUSER IN COMMON, THEN YOU MUST ALSO   STRIN128
C  CHANGE 30 IN THE FOLLOWING STATEMENT TO THE NEW DIMENSION.           STRIN129
C***********************************************************************STRIN130
 800  IF (IIN.LT.1 .OR. IIN.GT.30) GO TO 900                            STRIN131
         IF (IABS(L) .EQ. 1) GO TO 805                                  STRIN132
            CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                         STRIN133
            GO TO 910                                                   STRIN134
 805     LUSER(IIN)=L .EQ. 1                                            STRIN135
         RETURN                                                         STRIN136
 900  CALL ERRMES (2,.FALSE.,IHOLER,NOUT)                               STRIN137
 910  NIOERR=NIOERR+1                                                   STRIN138
      IF (NIOERR .GE. MIOERR) STOP                                      STRIN139
      RETURN                                                            STRIN140
 1000 FORMAT(1X,70A1)                                                   STRIN141
 2000 FORMAT(20I4)                                                      STRIN142
 3000 FORMAT(20L1)                                                      STRIN143
      END                                                               STRIN144
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++VAIAN001
      SUBROUTINE VARIAN (NYMAX,Y,T,YLYFIT,SQRTW,NY,NLINMX,NONLMX,PLIN,  VAIAN002
     1                   PLNTRY,PLAM,PLMTRY,PIVLIN,EK,ETE,EKEK,R,DELTAL,VAIAN003
     2                   DTOLER,DELTAN,ASAVE,SAVEA,NCOMBI,MTRYMX,STARTV,VAIAN004
     3                   DX,BSPL0,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,VAIAN005
     4                   SGYCAP,YW,Q,VAR,IERROR,ONLRES,RESPON)          VAIAN006
C-----------------------------------------------------------------------VAIAN007
C     EVALUATES FOR GIVEN PARAMETER PLMTRY OPTIMAL LINEAR PARAMETERS    VAIAN008
C     PLNTRY AND VARIANCE VAR.                                          VAIAN009
C-----------------------------------------------------------------------VAIAN010
C     SUBPROGRAMS CALLED - ERRMES, LINPAR, NPASCL, USERFL, USERFN       VAIAN011
C     WHICH IN TURN CALL - BSPLIN, ERRMES, INTERV, LINEQS, PIVOT,       VAIAN012
C                          PIVOT1, USERFL, USERFN, USERTR               VAIAN013
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               VAIAN014
C-----------------------------------------------------------------------VAIAN015
      DOUBLE PRECISION PRECIS, RANGE,                                   VAIAN016
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       VAIAN017
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           VAIAN018
     3                 TWOTHR, FORTHR, SIXTEL                           VAIAN019
      DOUBLE PRECISION EK, ETE, EKEK, R, DELTAL, DTOLER, DELTAN,        VAIAN020
     1                 ASAVE, DX, BSPL0, CCAP, ECAP, YCAP, SGGCAP,      VAIAN021
     2                 SGYCAP, YW, VAR, SUM1, SUM2, SUM3, YK            VAIAN022
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  VAIAN023
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    VAIAN024
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  VAIAN025
     3                 SPLINE, WRTBET, WRTCAP                           VAIAN026
      LOGICAL          PIVLIN, SAVEA, STARTV, LFIRST, SAMBOX, ONLRES    VAIAN027
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), YLYFIT(NYMAX,1),         VAIAN028
     1                 SQRTW(NYMAX,1), NY(*), RESPON(*), PLIN(NLINMX,1),VAIAN029
     2                 PLNTRY(NLINMX,1), PLAM(NONLMX), PLMTRY(NONLMX),  VAIAN030
     3                 PIVLIN(NLINMX,1), EK(NLINMX), ETE(NLINMX,NLINMX),VAIAN031
     4                 EKEK(NLINMX,NLINMX,1), R(NLINMX,1),              VAIAN032
     5                 DELTAL(NLINMX), DTOLER(NLINMX), DELTAN(NONLMX),  VAIAN033
     6                 ASAVE(NLINMX,NLINMX,1)                           VAIAN034
      DIMENSION        NCOMBI(NONLMX), STARTV(MTRYMX), DX(NONLMX),      VAIAN035
     1                 BSPL0(4,NONLMX), IC(NONLMX), CCAP(NZMAX,NZMAX,1),VAIAN036
     2                 ECAP(NZMAX,NGAMMX,1), YCAP(NZMAX,1),             VAIAN037
     3                 SGGCAP(NGAMMX,NGAMMX,1), SGYCAP(NGAMMX,1), YW(*) VAIAN038
      DIMENSION        IHOLER(6)                                        VAIAN039
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        VAIAN040
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, VAIAN041
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 VAIAN042
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),VAIAN043
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     VAIAN044
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       VAIAN045
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       VAIAN046
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   VAIAN047
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    VAIAN048
     4                 MTRY(10,2), MXITER(2), NL(10),                   VAIAN049
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  VAIAN050
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, VAIAN051
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       VAIAN052
     8                 NIN, NOUT                                        VAIAN053
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          VAIAN054
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         VAIAN055
     2                 DOUSOU(2), LUSER(30),                            VAIAN056
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  VAIAN057
     4                 SPLINE, WRTBET, WRTCAP                           VAIAN058
      DATA             IHOLER/1HV, 1HA, 1HR, 1HI, 1HA, 1HN/             VAIAN059
C-----------------------------------------------------------------------VAIAN060
      IF (ONLRES) GO TO 105                                             VAIAN061
C-----------------------------------------------------------------------VAIAN062
C  SET NEW NONLINEAR PARAMETER PLMTRY                                   VAIAN063
C-----------------------------------------------------------------------VAIAN064
      DO 100 J=1,NONL                                                   VAIAN065
 100  PLMTRY(J)=PLAM(J)+Q*DELTAN(J)                                     VAIAN066
C-----------------------------------------------------------------------VAIAN067
C  EVALUATE FOR GIVEN NONLINEAR PLMTRY OPTIMAL LINEAR PARAMETERS PLNTRY VAIAN068
C-----------------------------------------------------------------------VAIAN069
      CALL LINPAR (NYMAX,Y,T,SQRTW,NY,NLINMX,NONLMX,PLIN,PLNTRY,PLMTRY, VAIAN070
     1             PIVLIN,EK,ETE,EKEK,R,DELTAL,DTOLER,ASAVE,SAVEA,DX,   VAIAN071
     2             BSPL0,IC,NZMAX,NGAMMX,CCAP,ECAP,YCAP,SGGCAP,SGYCAP,  VAIAN072
     3             ND,NLIN,NONL,ALPLIN,SPLINE,IERROR,RESPON)            VAIAN073
      ALPLIN=IERROR .EQ. 0                                              VAIAN074
      IF (SPLINE) GO TO 400                                             VAIAN075
C-----------------------------------------------------------------------VAIAN076
C  USE EXACT FUNCTIONS                                                  VAIAN077
C  CASE  ND = 1                                                         VAIAN078
C-----------------------------------------------------------------------VAIAN079
 105  MY=NY(1)                                                          VAIAN080
      DO 200 K=1,MY                                                     VAIAN081
         KK=K                                                           VAIAN082
         YK=Y(K,1)                                                      VAIAN083
         DO 120 I=1,NLIN                                                VAIAN084
            IF (I .LE. NGAM) GO TO 110                                  VAIAN085
               NU   =I-NGAM                                             VAIAN086
               EK(I)=USERFN(PLMTRY(NU),NU,NYMAX,T,NY,KK,1,0,RESPON)     VAIAN087
               GO TO 120                                                VAIAN088
 110        NU   =I                                                     VAIAN089
            EK(I)=USERFL(NU,NYMAX,T,NY,KK,1,RESPON)                     VAIAN090
 120     CONTINUE                                                       VAIAN091
         VAR=ZERO                                                       VAIAN092
         DO 130 I=1,NLIN                                                VAIAN093
 130     VAR=VAR+EK(I)*PLNTRY(I,1)                                      VAIAN094
         YLYFIT(K,1)=(YK-VAR)*SQRTW(K,1)                                VAIAN095
         IF (NSTORD) GO TO 200                                          VAIAN096
         DO 160 N=2,ND                                                  VAIAN097
            YK =Y(K,N)                                                  VAIAN098
            VAR=ZERO                                                    VAIAN099
            DO 140 I=1,NLIN                                             VAIAN100
 140        VAR=VAR+EK(I)*PLNTRY(I,N)                                   VAIAN101
            YLYFIT(K,N)=(YK-VAR)*SQRTW(K,N)                             VAIAN102
 160     CONTINUE                                                       VAIAN103
 200  CONTINUE                                                          VAIAN104
C-----------------------------------------------------------------------VAIAN105
C  CASE  ND > 1                                                         VAIAN106
C-----------------------------------------------------------------------VAIAN107
      IF (SAMET .OR. ND.EQ.1) GO TO 360                                 VAIAN108
         DO 350 N=2,ND                                                  VAIAN109
            NSET=N                                                      VAIAN110
            MY  =NY(N)                                                  VAIAN111
            DO 330 K=1,MY                                               VAIAN112
               KK=K                                                     VAIAN113
               YK=Y(K,N)                                                VAIAN114
               DO 310 I=1,NLIN                                          VAIAN115
                  IF (I .LE. NGAM) GO TO 300                            VAIAN116
                     NU   =I-NGAM                                       VAIAN117
                     EK(I)=USERFN(PLMTRY(NU),NU,NYMAX,T,NY,             VAIAN118
     1                            KK,NSET,0,RESPON)                     VAIAN119
                     GO TO 310                                          VAIAN120
 300              NU   =I                                               VAIAN121
                  EK(I)=USERFL(NU,NYMAX,T,NY,KK,NSET,RESPON)            VAIAN122
 310           CONTINUE                                                 VAIAN123
               VAR=ZERO                                                 VAIAN124
               DO 320 I=1,NLIN                                          VAIAN125
 320           VAR=VAR+EK(I)*PLNTRY(I,N)                                VAIAN126
               YLYFIT(K,N)=(YK-VAR)*SQRTW(K,N)                          VAIAN127
 330        CONTINUE                                                    VAIAN128
 350     CONTINUE                                                       VAIAN129
 360  IF (ONLRES) RETURN                                                VAIAN130
      VAR=ZERO                                                          VAIAN131
C-----------------------------------------------------------------------VAIAN132
C  CASE  ND = 1                                                         VAIAN133
C-----------------------------------------------------------------------VAIAN134
      MY=NY(1)                                                          VAIAN135
      DO 370 K=1,MY                                                     VAIAN136
         YK =YLYFIT(K,1)                                                VAIAN137
         VAR=VAR+YK*YK                                                  VAIAN138
 370  CONTINUE                                                          VAIAN139
      IF (ND .EQ. 1) GO TO 900                                          VAIAN140
C-----------------------------------------------------------------------VAIAN141
C  CASE  ND > 1                                                         VAIAN142
C-----------------------------------------------------------------------VAIAN143
      DO 390 N=2,ND                                                     VAIAN144
         MY=NY(N)                                                       VAIAN145
         DO 390 K=1,MY                                                  VAIAN146
         YK =YLYFIT(K,N)                                                VAIAN147
         VAR=VAR+YK*YK                                                  VAIAN148
 390  CONTINUE                                                          VAIAN149
      GO TO 900                                                         VAIAN150
C-----------------------------------------------------------------------VAIAN151
C  USE SPLINE APPROXIMATIONS                                            VAIAN152
C  CASE  ND = 1                                                         VAIAN153
C-----------------------------------------------------------------------VAIAN154
 400  VAR =YW(1)                                                        VAIAN155
      SUM1=ZERO                                                         VAIAN156
      DO 620 J=1,NONL                                                   VAIAN157
         JG  =J+NGAM                                                    VAIAN158
         SUM2=ZERO                                                      VAIAN159
         DO 600 I=1,NONL                                                VAIAN160
            IG  =I+NGAM                                                 VAIAN161
            SUM2=SUM2+EKEK(IG,JG,1)*PLNTRY(IG,1)                        VAIAN162
 600     CONTINUE                                                       VAIAN163
         SUM3=ZERO                                                      VAIAN164
         LL  =IC(J)-1                                                   VAIAN165
         DO 610 L=1,IB                                                  VAIAN166
            LL  =LL+1                                                   VAIAN167
            SUM3=SUM3+BSPL0(L,J)*YCAP(LL,1)                             VAIAN168
 610     CONTINUE                                                       VAIAN169
         SUM1=SUM1+(SUM2-TWO*SUM3)*PLNTRY(JG,1)                         VAIAN170
 620  CONTINUE                                                          VAIAN171
      VAR =VAR+SUM1                                                     VAIAN172
      IF (NGAM .EQ. 0) GO TO 660                                        VAIAN173
         SUM1=ZERO                                                      VAIAN174
         DO 650 MU=1,NGAM                                               VAIAN175
            SUM2=ZERO                                                   VAIAN176
            DO 630 NU=1,NGAM                                            VAIAN177
 630        SUM2=SUM2+EKEK(MU,NU,1)*PLNTRY(NU,1)                        VAIAN178
            SUM3=ZERO                                                   VAIAN179
            DO 640 J=1,NONL                                             VAIAN180
               JG  =J+NGAM                                              VAIAN181
               SUM3=SUM3+EKEK(MU,JG,1)*PLNTRY(JG,1)                     VAIAN182
 640        CONTINUE                                                    VAIAN183
            SUM1=SUM1+(SUM2+TWO*(SUM3-SGYCAP(MU,1)))*PLNTRY(MU,1)       VAIAN184
 650     CONTINUE                                                       VAIAN185
         VAR=VAR+SUM1                                                   VAIAN186
 660  IF (ND .EQ. 1) GO TO 850                                          VAIAN187
C-----------------------------------------------------------------------VAIAN188
C  CASE  ND > 1                                                         VAIAN189
C-----------------------------------------------------------------------VAIAN190
      DO 800 N=2,ND                                                     VAIAN191
         VAR =VAR+YW(N)                                                 VAIAN192
         SUM1=ZERO                                                      VAIAN193
         DO 720 J=1,NONL                                                VAIAN194
            JG  =J+NGAM                                                 VAIAN195
            SUM2=ZERO                                                   VAIAN196
            DO 700 I=1,NONL                                             VAIAN197
               IG  =I+NGAM                                              VAIAN198
               SUM2=SUM2+EKEK(IG,JG,N)*PLNTRY(IG,N)                     VAIAN199
 700        CONTINUE                                                    VAIAN200
            SUM3=ZERO                                                   VAIAN201
            LL  =IC(J)-1                                                VAIAN202
            DO 710 L=1,IB                                               VAIAN203
               LL  =LL+1                                                VAIAN204
               SUM3=SUM3+BSPL0(L,J)*YCAP(LL,N)                          VAIAN205
 710        CONTINUE                                                    VAIAN206
            SUM1=SUM1+(SUM2-TWO*SUM3)*PLNTRY(JG,N)                      VAIAN207
 720     CONTINUE                                                       VAIAN208
         VAR =VAR+SUM1                                                  VAIAN209
         IF (NGAM .EQ. 0) GO TO 800                                     VAIAN210
         SUM1=ZERO                                                      VAIAN211
         DO 750 MU=1,NGAM                                               VAIAN212
            SUM2=ZERO                                                   VAIAN213
            DO 730 NU=1,NGAM                                            VAIAN214
 730        SUM2=SUM2+EKEK(MU,NU,N)*PLNTRY(NU,N)                        VAIAN215
            SUM3=ZERO                                                   VAIAN216
            DO 740 J=1,NONL                                             VAIAN217
               JG  =J+NGAM                                              VAIAN218
               SUM3=SUM3+EKEK(MU,JG,N)*PLNTRY(JG,N)                     VAIAN219
 740        CONTINUE                                                    VAIAN220
            SUM1=SUM1+(SUM2+TWO*(SUM3-SGYCAP(MU,N)))*PLNTRY(MU,N)       VAIAN221
 750     CONTINUE                                                       VAIAN222
         VAR=VAR+SUM1                                                   VAIAN223
 800  CONTINUE                                                          VAIAN224
C-----------------------------------------------------------------------VAIAN225
C  CHECK THE RARE CASE OF VAR BEING NEGATIVE DUE TO ROUNDING ERRORS.    VAIAN226
C-----------------------------------------------------------------------VAIAN227
 850  IF (VAR .GT. ZERO) GO TO 900                                      VAIAN228
         VAR=SQRT(SRANGE)                                               VAIAN229
         CALL ERRMES (1,.FALSE.,IHOLER,NOUT)                            VAIAN230
C-----------------------------------------------------------------------VAIAN231
C  SAVE COMBINATION OF PLMTRY-VALUES IN LOGICAL ARRAY STARTV(LOC),      VAIAN232
C  LOC=1,...,MMTRY, FOR LATER USE IN ANALYZ.                            VAIAN233
C-----------------------------------------------------------------------VAIAN234
 900  IF (.NOT. ALPLAM) RETURN                                          VAIAN235
      SAMBOX=.TRUE.                                                     VAIAN236
      DO 930 J=1,NONL                                                   VAIAN237
         DUM =(PLMTRY(J)-PNMNMX(1))/DBOX                                VAIAN238
         IPOS=INT(DUM)+1                                                VAIAN239
         IPOS=MIN0(IPOS,MBOX)                                           VAIAN240
         IF (IPOS .EQ. NCOMBI(J)) GO TO 910                             VAIAN241
            SAMBOX   =.FALSE.                                           VAIAN242
            NCOMBI(J)=IPOS                                              VAIAN243
 910     IF (J .EQ. 1) GO TO 930                                        VAIAN244
         IEND=J-1                                                       VAIAN245
         DO 920 I=1,IEND                                                VAIAN246
            IF (IPOS .NE. NCOMBI(I)) GO TO 920                          VAIAN247
            RETURN                                                      VAIAN248
 920     CONTINUE                                                       VAIAN249
 930  CONTINUE                                                          VAIAN250
      IF (SAMBOX) RETURN                                                VAIAN251
C-----------------------------------------------------------------------VAIAN252
C  NOW LOOK FOR LOCATION LOC IN ARRAY STARTV                            VAIAN253
C-----------------------------------------------------------------------VAIAN254
      NN    =MBOX                                                       VAIAN255
      KK    =NONL                                                       VAIAN256
      NTEMP =MMTRY                                                      VAIAN257
      LFIRST=.TRUE.                                                     VAIAN258
      LOC   =1                                                          VAIAN259
      ITRAP =0                                                          VAIAN260
      DO 980 J=1,MBOX                                                   VAIAN261
         DO 950 I=1,NONL                                                VAIAN262
            IF (NCOMBI(I) .EQ. J) GO TO 960                             VAIAN263
 950     CONTINUE                                                       VAIAN264
C-----------------------------------------------------------------------VAIAN265
C  POSITION NOT OCCUPIED                                                VAIAN266
C-----------------------------------------------------------------------VAIAN267
         IF (LFIRST)       NTEMP=NPASCL (NTEMP,NN,KK,-1)                VAIAN268
         IF (.NOT. LFIRST) NTEMP=NPASCL (NTEMP,NN,KK, 0)                VAIAN269
         LFIRST=.FALSE.                                                 VAIAN270
         LOC   =LOC+NTEMP                                               VAIAN271
         GO TO 980                                                      VAIAN272
C-----------------------------------------------------------------------VAIAN273
C  POSITION OCCUPIED                                                    VAIAN274
C-----------------------------------------------------------------------VAIAN275
 960     ITRAP=ITRAP+1                                                  VAIAN276
         IF (ITRAP .EQ. NONL) GO TO 990                                 VAIAN277
         NTEMP=NPASCL (NTEMP,NN,KK,-1)                                  VAIAN278
 980  CONTINUE                                                          VAIAN279
 990  IF (LOC .GT. MMTRY) RETURN                                        VAIAN280
      STARTV(LOC)=.TRUE.                                                VAIAN281
      RETURN                                                            VAIAN282
      END                                                               VAIAN283
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++WRTIN001
      SUBROUTINE WRITIN (NYMAX,Y,T,EXACT,SQRTW,NY,MLA,LA)               WRTIN002
C-----------------------------------------------------------------------WRTIN003
C     WRITES OUT FINAL VALUES OF COMMON VARIABLES (AFTER CHANGES DUE    WRTIN004
C     TO INPUT DATA AND USERIN), AS WELL AS THE REST OF THE INPUT AND   WRTIN005
C     ANY SIMULATED DATA.                                               WRTIN006
C-----------------------------------------------------------------------WRTIN007
C     SUBPROGRAMS CALLED - WRITYT                                       WRTIN008
C     COMMON USED        - DBLOCK, SBLOCK, IBLOCK, LBLOCK               WRTIN009
C-----------------------------------------------------------------------WRTIN010
      DOUBLE PRECISION PRECIS, RANGE,                                   WRTIN011
     1                 DZ, DZINV, VARMIN, VAROLD, VARSAV, ZSTART,       WRTIN012
     2                 ZERO, ONE, TWO, THREE, FOUR, SIX, TEN,           WRTIN013
     3                 TWOTHR, FORTHR, SIXTEL                           WRTIN014
      LOGICAL          DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA, DOADEX,  WRTIN015
     1                 DOSPL, DOSTRT, DOUSOU, LUSER,                    WRTIN016
     2                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  WRTIN017
     3                 SPLINE, WRTBET, WRTCAP                           WRTIN018
      LOGICAL          LEQUIV                                           WRTIN019
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), SQRTW(NYMAX,1),          WRTIN020
     1                 EXACT(NYMAX,1), NY(*), LA(6,MLA), IEQUIV(13),    WRTIN021
     2                 LEQUIV(6)                                        WRTIN022
      COMMON /DBLOCK/  PRECIS, RANGE, DZ, DZINV, VARMIN, VAROLD,        WRTIN023
     1                 VARSAV(10), ZSTART, ZERO, ONE, TWO, THREE, FOUR, WRTIN024
     2                 SIX, TEN, TWOTHR, FORTHR, SIXTEL                 WRTIN025
      COMMON /SBLOCK/  SRANGE, CONVRG, PLMNMX(2), PNMNMX(2), RUSER(100),WRTIN026
     1                 DBOX, EXMAX, PNG(10), ZTOTAL                     WRTIN027
      COMMON /IBLOCK/  IRWCAP, IUNIT, IWT, LINEPG, MCONV, METHOD,       WRTIN028
     1                 MIOERR, NABORT, ND, NERFIT, NGAM, NNL, NZ,       WRTIN029
     2                 IFORMT(70), IFORMW(70), IFORMY(70), IPLFIT(2),   WRTIN030
     3                 IPLRES(2), IPRINT(3,2), IPRITR(2), IUSER(50),    WRTIN031
     4                 MTRY(10,2), MXITER(2), NL(10),                   WRTIN032
     5                 IALPHA(6), IGAMMA(6), ILAMDA(6), IB, INDEX(10),  WRTIN033
     6                 ISTTXT(28), ITITLE(80), MBOX, MMTRY, NB, NGAMM1, WRTIN034
     7                 NGAMP1, NLIN, NONL, NONLP1, NPLINE, NYSUM,       WRTIN035
     8                 NIN, NOUT                                        WRTIN036
      COMMON /LBLOCK/  DOUSIN, LAST, PRWT, PRY, SAMET, SIMULA,          WRTIN037
     1                 DOADEX(10,2), DOSPL(10,2), DOSTRT(10,2),         WRTIN038
     2                 DOUSOU(2), LUSER(30),                            WRTIN039
     3                 ALPLAM, ALPLIN, IWTIST, NSTORD, REDBET, REDCAP,  WRTIN040
     4                 SPLINE, WRTBET, WRTCAP                           WRTIN041
      EQUIVALENCE      (IRWCAP,IEQUIV(1)), (DOUSIN,LEQUIV(1))           WRTIN042
C-----------------------------------------------------------------------WRTIN043
      WRITE (NOUT,1000)                                                 WRTIN044
      WRITE (NOUT,2000) (LA(K,1),K=1,6),CONVRG                          WRTIN045
      WRITE (NOUT,2000) (LA(K,2),K=1,6),PLMNMX                          WRTIN046
      WRITE (NOUT,2000) (LA(K,3),K=1,6),PNMNMX                          WRTIN047
      WRITE (NOUT,2000) (LA(K,4),K=1,6),RUSER                           WRTIN048
      JJ=4                                                              WRTIN049
      DO 100 J=1,13                                                     WRTIN050
         JJ=JJ+1                                                        WRTIN051
         WRITE (NOUT,3000) (LA(K,JJ),K=1,6),IEQUIV(J)                   WRTIN052
 100  CONTINUE                                                          WRTIN053
      WRITE (NOUT,4000) (LA(K,18),K=1,6),IFORMT                         WRTIN054
      WRITE (NOUT,4000) (LA(K,19),K=1,6),IFORMW                         WRTIN055
      WRITE (NOUT,4000) (LA(K,20),K=1,6),IFORMY                         WRTIN056
      WRITE (NOUT,3000) (LA(K,21),K=1,6),IPLFIT                         WRTIN057
      WRITE (NOUT,3000) (LA(K,22),K=1,6),IPLRES                         WRTIN058
      WRITE (NOUT,3000) (LA(K,23),K=1,6),IPRINT                         WRTIN059
      WRITE (NOUT,3000) (LA(K,24),K=1,6),IPRITR                         WRTIN060
      WRITE (NOUT,3000) (LA(K,25),K=1,6),IUSER                          WRTIN061
      WRITE (NOUT,3000) (LA(K,26),K=1,6),MTRY                           WRTIN062
      WRITE (NOUT,3000) (LA(K,27),K=1,6),MXITER                         WRTIN063
      WRITE (NOUT,3500) (LA(K,28),K=1,6),NL                             WRTIN064
      JJ=28                                                             WRTIN065
      DO 200 J=1,6                                                      WRTIN066
         JJ=JJ+1                                                        WRTIN067
         WRITE (NOUT,5000) (LA(K,JJ),K=1,6),LEQUIV(J)                   WRTIN068
 200  CONTINUE                                                          WRTIN069
      WRITE (NOUT,5000) (LA(K,35),K=1,6),DOADEX                         WRTIN070
      WRITE (NOUT,5000) (LA(K,36),K=1,6),DOSPL                          WRTIN071
      WRITE (NOUT,5000) (LA(K,37),K=1,6),DOSTRT                         WRTIN072
      WRITE (NOUT,5000) (LA(K,38),K=1,6),DOUSOU                         WRTIN073
      WRITE (NOUT,5000) (LA(K,39),K=1,6),LUSER                          WRTIN074
      IF (.NOT.SIMULA .AND. PRY) CALL WRITYT (                          WRTIN075
     1     NYMAX,Y,T,EXACT,SQRTW,NY,ND,IWT,SIMULA,NOUT)                 WRTIN076
      WRITE (NOUT,6000) PRECIS, SRANGE, RANGE                           WRTIN077
      RETURN                                                            WRTIN078
 1000 FORMAT(1H1,40X,33HFINAL VALUES OF CONTROL VARIABLES)              WRTIN079
 2000 FORMAT(1X,6A1,2H =,1P10E12.5/(9X,10E12.5))                        WRTIN080
 3000 FORMAT(1X,6A1,2H =,10I12/(9X,10I12))                              WRTIN081
 3500 FORMAT(1X,6A1,2H =,10I12)                                         WRTIN082
 4000 FORMAT(1X,6A1,3H = ,80A1)                                         WRTIN083
 5000 FORMAT(1X,6A1,2H =,10L12/(9X,10L12))                              WRTIN084
C6000 FORMAT(12H0   PRECIS =,1PE9.2,10X,8HSRANGE =,E9.2,                WRTIN085
C    1       5X,7HRANGE =,E9.2)                                         WRTIN086
 6000 FORMAT(12H0   PRECIS =,1PD9.2,10X,8HSRANGE =,E9.2,                WRTIN087
     1       5X,7HRANGE =,D9.2)                                         WRTIN088
      END                                                               WRTIN089
C++++++++++++++++ DOUBLE PRECISION VERSION 3DP (JUN 1988) ++++++++++++++WRTYT001
      SUBROUTINE WRITYT (NYMAX,Y,T,EXACT,SQRTW,NY,ND,IWT,SIMULA,NOUT)   WRTYT002
C-----------------------------------------------------------------------WRTYT003
C     IF PRY=.TRUE., THEN WRITYT WRITES                                 WRTYT004
C                    Y (INPUT DATA),                                    WRTYT005
C                    T (INDEPENDENT VARIABLE),                          WRTYT006
C     AND, IF SIMULA=.TRUE.,                                            WRTYT007
C                    EXACT (EXACT SIMULATED DATA)                       WRTYT008
C     AND            (Y-EXACT),                                         WRTYT009
C     AND, IF IWT=4, SQRTW (SQUARE ROOTS OF WEIGHTS).                   WRTYT010
C-----------------------------------------------------------------------WRTYT011
C     SUBPROGRAMS CALLED - NONE                                         WRTYT012
C     COMMON USED        - NONE                                         WRTYT013
C-----------------------------------------------------------------------WRTYT014
      LOGICAL          SIMULA                                           WRTYT015
      DIMENSION        Y(NYMAX,1), T(NYMAX,1), SQRTW(NYMAX,1),          WRTYT016
     1                 EXACT(NYMAX,1), NY(*)                            WRTYT017
C-----------------------------------------------------------------------WRTYT018
      WRITE (NOUT,1000)                                                 WRTYT019
      IF (SIMULA) GO TO 200                                             WRTYT020
         DO 100 N=1,ND                                                  WRTYT021
            MY=NY(N)                                                    WRTYT022
            WRITE (NOUT,1500) N                                         WRTYT023
            IF (IWT .NE. 4) WRITE (NOUT,2000) (T(K,N),Y(K,N),K=1,MY)    WRTYT024
            IF (IWT .EQ. 4) WRITE (NOUT,3000)                           WRTYT025
     1                     (T(K,N),Y(K,N),SQRTW(K,N),K=1,MY)            WRTYT026
 100     CONTINUE                                                       WRTYT027
         RETURN                                                         WRTYT028
 200  DO 500 N=1,ND                                                     WRTYT029
         MY=NY(N)                                                       WRTYT030
         WRITE (NOUT,1500) N                                            WRTYT031
         IF (IWT .NE. 4) WRITE (NOUT,4000)                              WRTYT032
         IF (IWT .EQ. 4) WRITE (NOUT,5000)                              WRTYT033
         DO 300 K=2,MY,2                                                WRTYT034
            DUM =Y(K-1,N)-EXACT(K-1,N)                                  WRTYT035
            DDUM=Y(K,N)  -EXACT(K,N)                                    WRTYT036
            IF (IWT .NE. 4) WRITE (NOUT,6000)                           WRTYT037
     1          T(K-1,N),Y(K-1,N),EXACT(K-1,N),DUM,                     WRTYT038
     2          T(K,N),  Y(K,N),  EXACT(K,N),  DDUM                     WRTYT039
            IF (IWT .EQ. 4) WRITE (NOUT,7000)                           WRTYT040
     1          T(K-1,N),Y(K-1,N),EXACT(K-1,N),DUM, SQRTW(K-1,N),       WRTYT041
     2          T(K,N),  Y(K,N),  EXACT(K,N),  DDUM,SQRTW(K,N)          WRTYT042
 300     CONTINUE                                                       WRTYT043
         IF (MOD(MY,2) .EQ. 0) GO TO 500                                WRTYT044
         DUM=Y(MY,N)-EXACT(MY,N)                                        WRTYT045
         IF (IWT .NE. 4) WRITE (NOUT,6000)                              WRTYT046
     1       T(MY,N),Y(MY,N),EXACT(MY,N),DUM                            WRTYT047
         IF (IWT .EQ. 4) WRITE (NOUT,7000)                              WRTYT048
     1       T(MY,N),Y(MY,N),EXACT(MY,N),DUM,SQRTW(MY,N)                WRTYT049
 500  CONTINUE                                                          WRTYT050
      RETURN                                                            WRTYT051
 1000 FORMAT(1H1)                                                       WRTYT052
 1500 FORMAT(//4X,8HDATA SET,I5/)                                       WRTYT053
 2000 FORMAT(5(12X,1HT,12X,1HY)/(2X,1PE11.3,E13.5,E13.3,E13.5,          WRTYT054
     1                    E13.3,E13.5,E13.3,E13.5,E13.3,E13.5))         WRTYT055
 3000 FORMAT(3(17X,1HT,12X,1HY,8X,5HSQRTW)/(5X,1P3E13.5,5X,3E13.5,      WRTYT056
     1                                                  5X,3E13.5))     WRTYT057
 4000 FORMAT(2(17X,1HT,12X,1HY,8X,5HEXACT,8X,5HERROR))                  WRTYT058
 5000 FORMAT(2(12X,1HT,12X,1HY,8X,5HEXACT,8X,5HERROR,8X,5HSQRTW))       WRTYT059
 6000 FORMAT(5X,1P4E13.5,5X,4E13.5)                                     WRTYT060
 7000 FORMAT(2X,1PE11.3,4E13.5,E13.3,4E13.5)                            WRTYT061
      END                                                               WRTYT062

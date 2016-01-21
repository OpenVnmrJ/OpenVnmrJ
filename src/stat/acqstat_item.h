/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**
#define dispExp   	0
#define dispProg  	1
**/

/*
 * DO NOT CHANGE ANY OF THESE TITLE DEFINES, OR
 * YOU WILL BREAK OLD ACQSTAT PROPERTIES FILES.
 * Add any new titles to the end of the title list and redefine LASTTITLE.
 */
#define StatusTitle	1
#define PendTitle	2
#define UserTitle	3
#define ExpTitle	4
#define ArrayTitle	5
#define CT_Title	6
#define DecTitle	7
#define SampleTitle	8
#define LockTitle	9
#define LKTitle 	10

#define CompTitle	11
#define RemainTitle	12
#define DataTitle 	13
#define SpinTitle	14
#define SPNTitle 	15
#define SPN2Title 	16
#define VT_Title 	17
#define VTTitle 	18
#define VT2Title 	19
#define AIRTitle 	20

#define RF10s1AvgTitle 	21
#define RF10s1LimTitle 	22
#define RF5m1AvgTitle 	23
#define RF5m1LimTitle 	24
#define RF10s2AvgTitle 	25
#define RF10s2LimTitle 	26
#define RF5m2AvgTitle 	27
#define RF5m2LimTitle 	28

#define RF10s3AvgTitle 	29
#define RF10s3LimTitle 	30
#define RF5m3AvgTitle 	31
#define RF5m3LimTitle 	32
#define RF10s4AvgTitle 	33
#define RF10s4LimTitle 	34
#define RF5m4AvgTitle 	35
#define RF5m4LimTitle 	36
#define RFMonTitle	37
#define ProbeID1Title   38
#define MASLimitTitle   39
#define MASSpanTitle    40
#define MASAdjTitle     41
#define MASMaxTitle     42
#define MASActiveSPTitle 43
#define MASProfileTitle 44
#define GradCoilIDTitle 45
#define SampleZoneTitle 46
#define SampleRackTitle 47
#define FTSTitle        48

#define FIRSTTITLE      StatusTitle
#define LASTTITLE       FTSTitle      

#define StatusVal 	(LASTTITLE + StatusTitle)
#define PendVal 	(LASTTITLE + PendTitle)
#define UserVal 	(LASTTITLE + UserTitle)
#define ExpVal 		(LASTTITLE + ExpTitle)
#define ArrayVal 	(LASTTITLE + ArrayTitle)
#define CT_Val 		(LASTTITLE + CT_Title)
#define DecVal 		(LASTTITLE + DecTitle)
#define SampleVal 	(LASTTITLE + SampleTitle)
#define LockVal 	(LASTTITLE + LockTitle)
#define SpinVal 	(LASTTITLE + SpinTitle)
#define VT_Val 		(LASTTITLE + VT_Title)

#define CompVal 	(LASTTITLE + CompTitle)
#define RemainVal 	(LASTTITLE + RemainTitle)
#define DataVal 	(LASTTITLE + DataTitle)
#define LKVal 		(LASTTITLE + LKTitle)
#define SPNVal 		(LASTTITLE + SPNTitle)
#define SPNVal2 	(LASTTITLE + SPN2Title)
#define VTVal 		(LASTTITLE + VTTitle)
#define VTVal2 		(LASTTITLE + VT2Title)
#define AIRVal 		(LASTTITLE + AIRTitle)

#define RF10s1AvgVal 	(LASTTITLE + RF10s1AvgTitle)
#define RF10s1LimVal 	(LASTTITLE + RF10s1LimTitle)
#define RF5m1AvgVal 	(LASTTITLE + RF5m1AvgTitle)
#define RF5m1LimVal 	(LASTTITLE + RF5m1LimTitle)
#define RF10s2AvgVal 	(LASTTITLE + RF10s2AvgTitle)
#define RF10s2LimVal 	(LASTTITLE + RF10s2LimTitle)
#define RF5m2AvgVal 	(LASTTITLE + RF5m2AvgTitle)
#define RF5m2LimVal 	(LASTTITLE + RF5m2LimTitle)

#define RF10s3AvgVal 	(LASTTITLE + RF10s3AvgTitle)
#define RF10s3LimVal 	(LASTTITLE + RF10s3LimTitle)
#define RF5m3AvgVal 	(LASTTITLE + RF5m3AvgTitle)
#define RF5m3LimVal 	(LASTTITLE + RF5m3LimTitle)
#define RF10s4AvgVal 	(LASTTITLE + RF10s4AvgTitle)
#define RF10s4LimVal 	(LASTTITLE + RF10s4LimTitle)
#define RF5m4AvgVal 	(LASTTITLE + RF5m4AvgTitle)
#define RF5m4LimVal 	(LASTTITLE + RF5m4LimTitle)
#define RFMonVal	(LASTTITLE + RFMonTitle)

#define ProbeID1	(LASTTITLE + ProbeID1Title)
#define MASSpeedLimit	(LASTTITLE + MASLimitTitle)
#define MASBearSpan	(LASTTITLE + MASSpanTitle)
#define MASBearAdj	(LASTTITLE + MASAdjTitle)
#define MASBearMax	(LASTTITLE + MASMaxTitle)
#define MASActiveSetPoint  (LASTTITLE + MASActiveSPTitle)
#define MASProfileSetting  (LASTTITLE + MASProfileTitle)
#define GradCoilID	(LASTTITLE + GradCoilIDTitle)

#define SampleZoneVal   (LASTTITLE + SampleZoneTitle) 
#define SampleRackVal	(LASTTITLE + SampleRackTitle)
#define FTSVal 		(LASTTITLE + FTSTitle)

#define LASTITEM        FTSVal

#define dispStatus 	(LASTITEM+1)
#define dispIndex  	(dispStatus+1)
#define dispSpec   	(dispIndex+1)
#define lockGain   	(dispSpec+1)
#define lockPower   	(lockGain+1)
#define lockPhase   	(lockPower+1)
#define shimSet   	(lockPhase+1)

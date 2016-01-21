/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <string.h>

#include "Console.h"
#include "cpsg.h"
#include "PSGFileHeader.h"
#include "ACode32.h"
#include "acqparms.h"
#include "MasterController.h"
#include "Bridge.h"

extern int bgflag;
extern int ptsval[];
extern char gradtype[];
extern int dps_flag;
extern int tuneflag;
extern int safetyoff;
extern int rtonce;

extern "C" int getIlFlag();

#ifndef MAXSTR
#define MAXSTR 256
#endif

Console *P2TheConsole = NULL;
char RollCallString[100];

// local utility functions

inline DDRController *ddrchannel(int i)
{
   return (DDRController *) (P2TheConsole->DDRUserTable[i]);
}

// wip replace obsStr..
char Console::obs[16][8] = { "obs","tof","tpwr","tpwrf","xm","xmm","xmf","xseq","xhomo","sfrq","xres","tpwrt","tpwrm","tn"};

Console::Console(const char *nm)
{
  build(nm);
  P2TheConsole=this;
  theRFGroup = NULL;
  RFShared = 0;
  RFSharedDecoupling = 0;
  consoleHBmask = 0;
}


void Console::build(const char *nm)
{
  double numrfchan, numddrchan, numgradchan;
  double pfgConfig;
  double rfpwr;
  double tmpVal, tmpVal2;
  char tmpStr[MAXSTR];
  int rcvrActive;
  int firstActiveRcvrFlag = 0;
  if (getparmd("numrfch","real",GLOBAL,&numrfchan,1) == 1)
  {
    abort_message("configuration parameter numrfchan not found\n");
  }

  consoleHBmask = 0;
/*
   System 		consoleSubType	consoleRFInterface  consoleIdNumber
   VNMRS, 400MR		v		1			0(VNMRS), 1(400MR)
   DD2			s		1			2
   400MR DD2		s		1			3
   ProPulse		s		2			3
   ProPulse INNOVA	s		3			3
*/
  
  consoleRFInterface = 1;

  consoleCryoUsage = 0;
  getparmnwarn();
  if (getparm("probestyle","string",GLOBAL,tmpStr,MAXSTR) == 0)
  {
     if (strcmp(tmpStr, "cold") == 0)
        consoleCryoUsage = 1;
  }

  if (P_getreal(CURRENT,"consoleIdNumber", &tmpVal ,1) < 0)
  {
     consoleSubType = 'v';            // VNMRS, 400MR consoles
  }
  else
  {
     if (tmpVal >= 2)
     {
        consoleSubType = 's';         // DD2 console
        if (P_getreal(GLOBAL,"rfinterface", &tmpVal2 ,1) < 0) 
        {
          consoleRFInterface = 1;
        }
        else
        {
          if ((int)(tmpVal2+0.49) == 2) 
          {
            if ((int)numrfchan == 2)
              consoleRFInterface = 2;  // ProPulse RF Interface
            else if ((int)numrfchan == 3)
              consoleRFInterface = 3;  // ProPulse INNOVA
          }
          else
            consoleRFInterface = 1;  // DD2 RF Interface
        }
     }
     else
     {
        consoleSubType = 'v';         // VNMRS, 400MR consoles
        consoleRFInterface = 1;
     }
  }

  // check console & RF Interface configuration
  if ((consoleSubType == 's') && (consoleRFInterface == 1))
  {
     if (bgflag) printf("Console system configured as DD2\n");
  }
  else if ((consoleSubType == 's') && (consoleRFInterface == 2))
  {
     if (consoleCryoUsage == 1)
        abort_message("cryo probe usage not supported on ProPulse system. abort!\n");
     if (bgflag) printf("Console system configured as ProPulse\n");
  }
  else if ((consoleSubType == 's') && (consoleRFInterface == 3))
  {
     if (bgflag) 
     {
        if (consoleCryoUsage == 1)
           {printf("Console system configured as ProPulse INNOVA with Cryo\n"); }
        else
           {printf("Console system configured as ProPulse INNOVA\n"); }
     }
  }
  else if ((consoleSubType == 'v') && (consoleRFInterface == 1))
  {
     if (bgflag) printf("Console system configured as VNMRS/400MR\n");
  }
  else
  {  
     abort_message("invalid console type configured. abort!\n");
  }


  if (getparm("gradtype","string",GLOBAL,gradtype,8) == 1)
  {
     abort_message("configuration parameter gradtype not found\n");
  }
  if ( ((gradtype[0] != 'n') && (gradtype[0] != 'a')) ||
       ((gradtype[1] != 'n') && (gradtype[1] != 'a')) ||
       ((gradtype[2] != 'n') && (gradtype[2] != 'a') && (gradtype[2] != 'h')) )
    numgradchan=1.0;
  else
    numgradchan=0.0;

  if ( P_getreal(GLOBAL,"pfg1board",&pfgConfig,1) == 0)
  {
      if (pfgConfig > 1.5)
      {
         numgradchan=2.0;
      }
  }

  channelBitsConfigured[0] = 0;
  channelBitsConfigured[1] = 0;
  channelBitsActive[0] = 0;
  channelBitsActive[1] = 0;

  if (getparmd("numrcvrs","real",GLOBAL,&numddrchan,1) == 1)
  {
    numddrchan = 1 ;
  }
  numRFChan  = (int)numrfchan;
  numDDRChan = (int)numddrchan;
  numGRDChan = (int)numgradchan;

  int i;
  for (i=1; i<=numrfchan; i++)
    channelBitsConfigured[0] = (channelBitsConfigured[0] | (1<<i));

  for (i=1; i<=numddrchan; i++)
    channelBitsConfigured[1] = (channelBitsConfigured[1] | (1<<i));

  if (numgradchan >= 1.0)
    channelBitsConfigured[0] = (channelBitsConfigured[0] | (1<<9));
  
  if (bgflag)
    fprintf(stdout,"Configuration: numrfch=%d  numrcvrs=%d  numgradchannel=%d\n",numRFChan,numDDRChan,numGRDChan);

  char tbuff[128];
  for (i=0; i < MAXCHAN; i++)
  {
       PhysicalTable[i] = NULL;
       RFUserTable[i]   = NULL;
       DDRUserTable[i]  = NULL;
  }
  PFGUserTable  = NULL;
  GRADUserTable = NULL;

  AcodeManager *pAcodeMgr   = AcodeManager::makeInstance(2+numRFChan+numDDRChan+numGRDChan);
  (void) pAcodeMgr;

  /* This next line is needed to make LINUX work.  If not present, calls to virtual functions
   * fail.  The first failure is in the call to SystemSync() in broadcastSystemSync()
   */
   /* and also need for Solaris it would appear.  GMB. 7/18/05 */
  Controller *dummy = new Controller("dummy",0);
  (void) dummy;

  nValid = 0;
  PhysicalTable[nValid++] = (Controller *) new MasterController("master1",0);
  PhysicalTable[nValid++] = (Controller *) new LockController("lock1",1);

  double alvl, pcal, ptc, pop;
  RFController* rfc ;
  for (i = 0; i < (int)numrfchan; i++)
  {
    sprintf(tbuff,"rf%d",i+1);
    rfc = new RFController(tbuff,ptsval[i]);
    sprintf(tmpStr,"maxattench%d",i+1);
    PhysicalTable[nValid++] = (Controller *)   rfc;
    RFUserTable[i]     = (RFController *) rfc;
    if ( P_getreal(GLOBAL,tmpStr,&rfpwr,1) == 0)
      rfc->setMaxUserLevel(rfpwr);
//  power additions  rf1pop, rf1elimit, rf1pcal etc.
    sprintf(tmpStr,"%spop",tbuff);
    getRealSetDefault(GLOBAL, tmpStr, &pop,3.0);
    sprintf(tmpStr,"%selimit",tbuff);
    getRealSetDefault(GLOBAL, tmpStr, &alvl,15.0);
    sprintf(tmpStr,"%spcal",tbuff);
    getRealSetDefault(GLOBAL, tmpStr, &pcal, 2.0);
    sprintf(tmpStr,"%sptc",tbuff);
    getRealSetDefault(GLOBAL, tmpStr, &ptc, 10.0);
    // set the no safe request 
    if (safetyoff) pop = 0.0; //
    if (i > 5) pop = 0.0;  //  disable checking on groups...
    rfc->setPowerAlarm((int) pop,pcal,alvl,ptc);
  }

  getStringSetDefault(CURRENT,"hfmode",tmpStr,"i");
  if ( (tmpStr[0] == 'c') || (tmpStr[0] == 'i') )
     consoleHFmode = tmpStr[0];
  else if (tmpStr[0] == '\0')
     consoleHFmode = 'i' ;
  else
     abort_message("invalid value specified for hfmode paramter (should be 'c' or 'i' or ''). abort!\n");

  if ( (consoleHFmode == 'c') && (consoleCryoUsage == 1) )
     abort_message("H/F combined mode and cryo probe usage are incompatible. Set hfmode & probetype correctly. abort!\n");

  DDRController* ddrc;
  numActiveRcvrs = 0;
  for (i = 0; i < (int)numddrchan; i++)
    {
      sprintf(tbuff,"ddr%d",i+1);
      rcvrActive = isRcvrActive(i);
      if (rcvrActive)
      {
         numActiveRcvrs++;
         if (firstActiveRcvrFlag == 0)
         {
             firstActiveRcvrIndex = i;
             firstActiveRcvrFlag = 1;
         }
      }
      if (bgflag)
      {
         fprintf(stdout,"Console:build: ddr%d , active: %d \n", i+1, rcvrActive);
      }
      ddrc = new DDRController(tbuff,rcvrActive,i+1);
      PhysicalTable[nValid++] = (Controller *)    ddrc;
      DDRUserTable[i]              = (DDRController *) ddrc;
    }

 if (numgradchan > 0)
 {
  if ( (gradtype[2]=='p')||(gradtype[2]=='q')||(gradtype[2]=='t')||     \
       (gradtype[2]=='u')||(gradtype[2]=='c')||(gradtype[2]=='d') || (gradtype[2] =='l'))
  {
     PFGController *pfgc      = new PFGController("pfg1", gradtype);
     PhysicalTable[nValid++]  = (Controller *) pfgc;
     PFGUserTable             =  pfgc;
     if (numGRDChan == 2)
	PhysicalTable[nValid++] = (Controller *) new GradientController("grad1",0);
  }
  else if ((gradtype[2]=='w') || (gradtype[2]=='r'))
  {
     GradientController *grdc = new GradientController("grad1",0);
     PhysicalTable[nValid++]  = (Controller *) grdc;
     GRADUserTable            = grdc;
     if (numGRDChan == 2)
     {
        PFGController *pfgc      = new PFGController("pfg1","ppp");
        PhysicalTable[nValid++]  = (Controller *) pfgc;
        PFGUserTable             =  pfgc;
     }
  }
  else
  {
     abort_message("invalid character in gradtype parameter\n");
  }
 }

 if (consoleSubType == 's')
 {
     MinTimeEventTicks=2L;
 }
 else
 {
    MinTimeEventTicks=4L;
 }

  nActive = nValid;

  strncpy(configName,nm,400);

  setLogicalParamNames();

  /* create InitAcqObject object for Low Core & initacqparms */
  initAcqObject = new InitAcqObject("InitAcqObject");

  /* zero various tickers, interlocks */
  loop_Ticker      = 0;
  nowait_loop_Ticker=0;
  ifz_True_Ticker  = 0;
  ifz_False_Ticker = 0;

  nestedLoopDepth  = 0;
  progDecInterLock = 0;
  parallelIndex = 0;
  parallelInfo = 0;
  parallelKzDuration = 0.0;
  parallelController = NULL;
  for (i = 0; i < MAXCHAN; i++)
  {
     parallelTicks[i] = 0;
     parallelSyncPos[i] = 0;
  }

  /* debug output */
  /* describe2((char *)("Console Build\0")); */
}


void Console::getChannelBits(int *channelbits)
{
  int i;
  RFController *rfCh;
  for (i=1; i<=numRFChan; i++)
  {
    rfCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(i);
    if ( (rfCh != NULL) && (rfCh->getChannelActive() == 1) )
      channelBitsActive[0] = (channelBitsActive[0] | (1<<i));
  }

  DDRController *ddrCh;
  for (i=1; i<=numDDRChan; i++)
  {
    ddrCh = (DDRController *) (P2TheConsole->DDRUserTable[i]);
    if ( (ddrCh != NULL) && (ddrCh->getChannelActive() == 1) )
       channelBitsActive[1] = (channelBitsActive[1] | (1<<i));
  }

  if (PFGUserTable != NULL)
  {
     char estring[42];
     P_getstring(GLOBAL,"pfgon",estring,1,40);
     if (tolower(estring[0] == 'y'))
     {
       if (PFGUserTable->getChannelActive() == 1)
          channelBitsActive[0] = (channelBitsActive[0] | (1<<11));
     }
     if (tolower(estring[1] == 'y'))
     {
       if (PFGUserTable->getChannelActive() == 1)
          channelBitsActive[0] = (channelBitsActive[0] | (1<<10));
     }
     if (tolower(estring[2] == 'y'))
     {
       if (PFGUserTable->getChannelActive() == 1)
          channelBitsActive[0] = (channelBitsActive[0] | (1<<9));
     }
  }

  /* Information about configured & active channels */
  channelbits[0] = channelBitsConfigured[0];
  channelbits[1] = channelBitsConfigured[1];
  channelbits[2] = channelBitsActive[0];
  channelbits[3] = channelBitsActive[1];
 /* printf("channel bits CCAA: %d  %d  %d  %d\n",channelbits[0],channelbits[1],channelbits[2],channelbits[3]); */
}


void Console::setLogicalParamNames()
{
  strcpy(obsStr[0],"obs");
  strcpy(obsStr[1],"tof");
  strcpy(obsStr[2],"tpwr");
  strcpy(obsStr[3],"tpwrf");
  strcpy(obsStr[4],"xm");
  strcpy(obsStr[5],"xmm");
  strcpy(obsStr[6],"xmf");
  strcpy(obsStr[7],"xseq");
  strcpy(obsStr[8],"xhomo");
  strcpy(obsStr[9],"sfrq");
  strcpy(obsStr[10],"xres");
  strcpy(obsStr[11],"tpwrt");
  strcpy(obsStr[12],"tpwrm");
  strcpy(obsStr[13],"tn");

  strcpy(decStr[0],"dec");
  strcpy(decStr[1],"dof");
  strcpy(decStr[2],"dpwr");
  strcpy(decStr[3],"dpwrf");
  strcpy(decStr[4],"dm");
  strcpy(decStr[5],"dmm");
  strcpy(decStr[6],"dmf");
  strcpy(decStr[7],"dseq");
  strcpy(decStr[8],"homo");
  strcpy(decStr[9],"dfrq");
  strcpy(decStr[10],"dres");
  strcpy(decStr[11],"dpwrt");
  strcpy(decStr[12],"dpwrm");
  strcpy(decStr[13],"dn");

  strcpy(dec2Str[0],"dec2");
  strcpy(dec2Str[1],"dof2");
  strcpy(dec2Str[2],"dpwr2");
  strcpy(dec2Str[3],"dpwrf2");
  strcpy(dec2Str[4],"dm2");
  strcpy(dec2Str[5],"dmm2");
  strcpy(dec2Str[6],"dmf2");
  strcpy(dec2Str[7],"dseq2");
  strcpy(dec2Str[8],"homo2");
  strcpy(dec2Str[9],"dfrq2");
  strcpy(dec2Str[10],"dres2");
  strcpy(dec2Str[11],"dpwrt2");
  strcpy(dec2Str[12],"dpwrm2");
  strcpy(dec2Str[13],"dn2");

  strcpy(dec3Str[0],"dec3");
  strcpy(dec3Str[1],"dof3");
  strcpy(dec3Str[2],"dpwr3");
  strcpy(dec3Str[3],"dpwrf3");
  strcpy(dec3Str[4],"dm3");
  strcpy(dec3Str[5],"dmm3");
  strcpy(dec3Str[6],"dmf3");
  strcpy(dec3Str[7],"dseq3");
  strcpy(dec3Str[8],"homo3");
  strcpy(dec3Str[9],"dfrq3");
  strcpy(dec3Str[10],"dres3");
  strcpy(dec3Str[11],"dpwrt3");
  strcpy(dec3Str[12],"dpwrm3");
  strcpy(dec3Str[13],"dn3");

  strcpy(dec4Str[0],"dec4");
  strcpy(dec4Str[1],"dof4");
  strcpy(dec4Str[2],"dpwr4");
  strcpy(dec4Str[3],"dpwrf4");
  strcpy(dec4Str[4],"dm4");
  strcpy(dec4Str[5],"dmm4");
  strcpy(dec4Str[6],"dmf4");
  strcpy(dec4Str[7],"dseq4");
  strcpy(dec4Str[8],"homo4");
  strcpy(dec4Str[9],"dfrq4");
  strcpy(dec4Str[10],"dres4");
  strcpy(dec4Str[11],"dpwrt4");
  strcpy(dec4Str[12],"dpwrm4");
  strcpy(dec4Str[13],"dn4");
}


/*
  Here the Console would map the Controllers
  looking up the Vnmr Configuration file, i.e.,
  The default mapping for high band observe would be
  pulse sequence        Controller ID
  	obs		rf1
	dec		rf2
	dec2		rf3
	dec3		rf4
  The default mapping for low band observe would be
  pulse sequence        Controller ID
  	obs		rf2
	dec		rf1
	dec2		rf3
	dec3		rf4
  Given the high band channel one/low band channel 2 standard.
  a new parameter probeConnect is used to assist the
  configuration = "X1,X2,X3,X4,X5,X6"  (to numrfchannels)
  compare tn to probeConnect when matched set that to rf? if
  preAmpConfig shows a pre amp (of correct band) on that channel, ok
  same with dn etc.

  These would be overridden by the Vnmr parameter
  rfchannel.
  rfchannel works like
  tnchannel dnchannel dn2channel etc.

  probeConnect associates via nucleus..
  'NUC1 NUC2 NUC3 NUC4'
  rf1 connects to NUC1 etc.
  example
  'H1 C13 P31'  rf1->H1 rf2->C13 rf3=P31
   tn=H1 rf1 observe, tn = C13 rf2 observe etc.
*/

/* returns a zero if not found else position of match */
/* matches X in probeConnect to first non-match */
/* extension if b2 = '' index = 9 */
/* extension if X does not match H or F */
int Console::posIndex(char *b1,char *b2)
{
  char *p1,*p2;
  int j,flag,k;
  if (strlen(b2) < 1) return(9);
  k = (strcmp(b2,"H1") != 0) && (strcmp(b2,"F19") != 0);
  p1 = strstr(b1,b2);
  if (p1 == NULL) {
    p1 = strstr(b1,"X");
    if (k && (Xunused) && (p1 != NULL)) // exclude hiband
      Xunused = 0;  // used now - p1 is ok..
    else
      return(0); 
  }
  j = 1;
  flag = 1;  // prevent paired spaces paired comma prevent channel use.
  for (p2 = b1; p2 < p1; p2++)
  {
    if (*p2 == ' ')
    {
      if (flag ==1)
      {
         j++;
         flag = 0;
      }
      else
        flag = 1;
    }
    else
      flag = 1;
    if (*p2 == ',')
       j++;
  }
  return(j);
}

int Console::next_free()
{
  int i,j;
  i = 1;
  j=0;
  while ((i < 7) && (j==0))
	{
	  if (!(usedMask & (1<< i))) j = i;
          i++;
        }
  if (i == 7) abort_message("probe connect mapping failed");
  // allocate the mask
  usedMask |= 1 << j;
  return(j);
}
// allocate "" channels..
int Console::clean(int numRFChan)
{
  // OBSch not a problem..
  if (DECch == 9)
      DECch = next_free();
  if (numRFChan < 3)
    return(0);
  if (DEC2ch == 9)
      DEC2ch = next_free();
  if (numRFChan < 4)
      return(0);
  if (DEC3ch == 9)
      DEC3ch = next_free();
   if (numRFChan < 5)
      return(0);
   if (DEC4ch == 9)
      DEC4ch = next_free();
   if (numRFChan < 6)
    return(0);
  return(-1);
}
//
//  map the channels with probeConnect
//  return codes 
//  0 mapped ok
//  -1 probeConnect failed to map
//  1 no probeConnect
int Console::RFConfigMap()
{
  int index,trialMask;
  int i;
  int hbuserMask,hbchanCount,xx;
  char probeConnect[MAXSTR],preAmpConfig[MAXSTR],tmpstr[MAXSTR];
  const char *use[]={"tn","dn","dn2","dn3","dn4"};
  // note hbusermask is right shifted against userMask
  index = 0;
  hbuserMask = 0;
  hbchanCount = 0;
  Xunused = 1; 
  usedMask=0;
  trialMask=0;
  if(P_getstring(GLOBAL,"probeConnect",probeConnect,1,MAXSTR) < 0)
     return(1);  //
  if (strlen(probeConnect) < 3) return(1);
  if(P_getstring(GLOBAL,"preAmpConfig",preAmpConfig,1,MAXSTR) < 0)
     strcpy(preAmpConfig,"HLXXX"); // system default
  // makes the readily assigned associations
  for (i = 0; i < numRFChan; i++)  // actually iterates tn,dn,dn2...
  {
      if (P_getstring(CURRENT,use[i],tmpstr,1,MAXSTR))
        strcpy(tmpstr,"");
      index = posIndex(probeConnect,tmpstr);
      if ((!strcmp(tmpstr,"H1") || !strcmp("F19",tmpstr)))
      {
         hbuserMask |= 1 << i;  // note if hband channel in functional
         hbchanCount++;
         if (index == 0) index = 2;  // Force combo mode! If there's a conflict, error exit.
      }
      switch (i) {
        case 0:  OBSch  = index;
           if (!strcmp(tmpstr,"lk"))
                return(-3);   // was index = 2 conflicts with tn==lk
           break;
        case 1:  DECch  = index; break;
        case 2:  DEC2ch = index; break;
        case 3:  DEC3ch = index; break;
        case 4:  DEC4ch = index; break;
        default:  return(-9); /* not supported past dn4 */
        }
        if ((index > 0) && (index < 9))
              trialMask = 1 << (index);
        else
              trialMask = 0;
        if (trialMask & usedMask)
           return(-4);
        usedMask |= trialMask; // Mask is physical channel #
  }
  // end readily assigned associations
  // shared mode logic works ONLY IF either chan1 or chan2 is unassigned
  // bit 1 or 2 in usedMask and the other is high band
  // two hibands - observe is 2 and high band then 
  if ((hbchanCount == 2) && (OBSch==2) && (hbuserMask & 1))
  {
      // force to channel 1 combo mode must use 1.
      OBSch = 1;
      xx = hbuserMask & 0x1e;

      switch (xx) {
         case 2:  DECch = 2; break;
         case 4:  DEC2ch = 2; break;
         case 8:  DEC3ch = 2;   break;
         case 16:  DEC4ch = 2; break;
         default:
          return(-9);
      }
  }
  // find unused channels ...
  clean(numRFChan);
  if (preAmpConfig[OBSch-1] == 'X')
     text_message("no preamplifier configured for %d channel\n",OBSch);

  if ((numRFChan == 3) && ((OBSch + DECch + DEC2ch) != 6)) return(-1); // fail
     if ((numRFChan == 4) && ((OBSch + DECch + DEC2ch + DEC3ch ) != 10)) return(-1); // fail
     if ((numRFChan == 5) && ((OBSch + DECch + DEC2ch + DEC3ch + DEC4ch) != 15)) return(-1); // fail

     return(0);
}

//
//
//
void Console::setRcvrsConfigMap()
{
  char rcvrsString[MAXSTR], rcvrsTypeStr[MAXSTR], tempCh[MAXSTR];

  getStringSetDefault(CURRENT,"rcvrs",rcvrsString,"ynnnn");
//  strcpy(trInterconnect.rcvrS,rcvrStr);
  int charnum = countSpecificChars(rcvrsString, 'y');
  if (charnum == 0)
    abort_message("error in receiver configuration variable rcvrs. abort!\n");

  getStringSetDefault(GLOBAL,"rcvrstype",rcvrsTypeStr,"s");
  rcvrsType = rcvrsTypeStr[0];
  receiverConfig = SINGRCVR_SINGACQ;
  //// enable volumercv for 1 receiver imaging.
  //   bug fix for p and multi receivers.
  //   'p' and 1 DDR turns on volume/local feature for 1 receiver
  if (rcvrsType == 'p')
  {
     if (numDDRChan > 1)
	  receiverConfig = MULTIMGRCVR_SINGACQ;
     else
     {
          receiverConfig = MULTIMGRCVR_MULTACQ;
          rcvrsType = 's'; 
     }
  }

  if (numDDRChan >= 2)
  { //  ynnn or nynn - one y only
    if (rcvrsType == 'm')
    {
      if (charnum == 1)
        receiverConfig = MULTNUCRCVR_SINGACQ;
      else
        receiverConfig = MULTNUCRCVR_MULTACQ;
    }
    else // it is not multi-nuclear multi-receiver - it is phased array for imaging.
    {
       if (charnum == 1)
         receiverConfig = MULTIMGRCVR_SINGACQ;
       else
         receiverConfig = MULTIMGRCVR_MULTACQ;;
    }
  }

  // cout << "receiverConfig = " << receiverConfig << "  rfChannelStr[0] = " << rfChannelStr[0] << endl;

  tempCh[0]=rfChannelStr[0]; tempCh[1]='\0';
  if ( (receiverConfig == MULTNUCRCVR_SINGACQ)  && (atoi(tempCh) == RCVR2_RF_LO_CHANNEL) )
  {
     strcpy(rcvrsString,"ny");
     P_setstring(CURRENT, "rcvrs", rcvrsString, 1);
     // cout << "setRvcrsConfigMap(): setting rcvrs to ny\n" << endl;
  }
  // cout << "receiverConfig = " << receiverConfig << "  rcvrsString = " << rcvrsString << endl;
  setactivercvrs(rcvrsString);
}



int Console::countSpecificChars(char *string, char character)
{
  int len=0, count=0, i;
  len = strlen(string);

  for(i=0; i<len; i++)
    if (string[i] == character) count++;
  return count;
}



int Console::getRcvrsConfigMap()
{
  return receiverConfig;
}


void Console::setChannelMapping()
{
  int ans,j,k,tnlkflag,isGrouped,jj;
  char tmp[MAXSTR],tnStr[MAXSTR];
  double bandsw,sfrq,dfrq;
  DDRController *ddrCh, *ddrCh1, *ddrCh2;
  MasterController *master = (MasterController *) getControllerByID("master1");
  bandsw = whatamphibandmin(0, 0.0);
  sfrq = getval("sfrq");
  dfrq = getval("dfrq");
  turnOffRFDuplicates();
  P_getstring(CURRENT,"tn",tnStr,1,MAXSTR);
  if (strcmp(tnStr,"lk"))
    tnlkflag = 0;
  else
    tnlkflag = 1;

  ans = RFConfigMap();
  if ( (ans < 0) && (tnlkflag == 0) && ! tuneflag )
     warn_message("probeConnect failed to map. Using system default...");
  if (ans == 0)
  {
     //fprintf(stdout,"rf chan equivalent => %1d%1d%1d%1d%1d\n",OBSch,DECch,DEC2ch,DEC3ch,DEC4ch);
     sprintf(rfChannelStr,"%1d%1d%1d%1d%1d",OBSch,DECch,DEC2ch,DEC3ch,DEC4ch);
  }
  else
  {
    /* brute force rfchannels */

    if (P_getstring(CURRENT,"rfchannel",rfChannelStr,1,(numRFChan+1)))
       strcpy(rfChannelStr,"");
    if (strcmp(rfChannelStr, "") == 0)
    {
      strcpy(rfChannelStr,"123456789");
      OBSch = 1; DECch =2;  DEC2ch = 3; DEC3ch = 4; DEC4ch = 5;
      // fix the low band observe and tn = lk case..
      if ((sfrq < bandsw) && (numRFChan >1))
      {
	    strcpy(rfChannelStr,"213456789");
            OBSch = 2; DECch =1;
      }
    }
    else
      if (ans == -3)
       text_error("probeConnect: using rfchannel = %s",rfChannelStr);
  }


  setRcvrsConfigMap();

  //  rf grouped pulse support.
  if((P_getstring(GLOBAL,"rfGroupMap",tmp,1,MAXSTR) < 0) || (strlen(tmp) < 2))
    {
      strcpy(tmp,"000000000000000000000000000000000000000000");
  }
  else
  {
       theRFGroup = new RFChannelGroup(tmp);
       //cout << "instantiate a Group " << tmp << "is the parameter" << endl;
   }

    // initialize the RF_LO_source for each DDR
    for (int j=0; j<numDDRChan; j++)
    {
      ddrCh = ddrchannel(j);
      ddrCh->set_RF_LO_source(OBSch);
    }

	// if a member of a group'd pulse use the 1st element value.
    for (j=0; j < numRFChan; j++)
    {
      k = rfChannelStr[j] - '0';
    }

    double amphbminfreq = whatamphibandmin(0,0.0);

    for (j=0; j < numRFChan; j++) // walks down the string!
    {
        jj = j;
        k = rfChannelStr[j] - '0';   // this is an OFF by 1
        if ((k < 1) || (k > 9))
        {
           abort_message("invalid rfchannel string. abort!\n");
        }
        if (k > numRFChan)
	  abort_message("channel %d set for observe - it is not present",k);
        ((RFController *)RFUserTable[k-1])->setObserve(0);

        // RFCHANNEL GROUP SUPPORT

        isGrouped = tmp[k-1] - '0';
        if ((isGrouped != 0) && (isGrouped != (k)))
        {
          if (isGrouped == OBSch)   jj = OBSch  - 1;
          if (isGrouped == DECch)   jj = DECch  - 1;
          if (isGrouped == DEC2ch)  jj = DEC2ch - 1;
          if (isGrouped == DEC3ch)  jj = DEC3ch - 1;
          if (isGrouped == DEC4ch)  jj = DEC4ch - 1;
        }
        else
        {
          isGrouped = 0;
          //cout <<"channel " << k << "is NOT in a group "  << endl;
        }

        ddrCh1=ddrchannel(0);
	// if a member of a group'd pulse use the 1st element value.
        // for j .. i.e. get base parameters.

        switch (jj)
	{
	  case 0:
            // first position is OBSch - subtract for RFUserTableIndex
            if (!isGrouped)
              OBSch = k;
	    ((RFController *)RFUserTable[k-1])->setLogicalNames(k, obsStr);
            ((RFController *)RFUserTable[k-1])->setObserve(1);
            if ( (((RFController *)RFUserTable[k-1])->getBaseFrequency()) > amphbminfreq)
               consoleHBmask |= (1<<0);

            break;

          case 1:
            // second position is DECch
            if (!isGrouped)
              DECch = k;
	    ((RFController *)RFUserTable[k-1])->setLogicalNames(k, decStr);
            if ( (((RFController *)RFUserTable[k-1])->getBaseFrequency()) > amphbminfreq)
               consoleHBmask |= (1<<1);

            break;

         case 2:
            // third position is DEC2ch
            if (!isGrouped)
              DEC2ch = k;
	    ((RFController *)RFUserTable[k-1])->setLogicalNames(k, dec2Str);
            if ( (((RFController *)RFUserTable[k-1])->getBaseFrequency()) > amphbminfreq)
               consoleHBmask |= (1<<2);

            break;

         case 3:
            // fourth position is DEC3ch
            if (!isGrouped)
              DEC3ch = k;
	    ((RFController *)RFUserTable[k-1])->setLogicalNames(k, dec3Str);
            if ( (((RFController *)RFUserTable[k-1])->getBaseFrequency()) > amphbminfreq)
               consoleHBmask |= (1<<3);

	    break;

	 case 4:
            if (!isGrouped)
              DEC4ch = k;
            ((RFController *)RFUserTable[k-1])->setLogicalNames(k, dec4Str);
            if ( (((RFController *)RFUserTable[k-1])->getBaseFrequency()) > amphbminfreq)
               consoleHBmask |= (1<<4);
	    break;
	  }
      }

   // this lifts the dec is ch3
   if ((numActiveRcvrs >= 2) && (getRcvrsType() == 'm'))
   {
         // 2 is RFCH 3
         ((RFController *)RFUserTable[2])->setObserve(1);
         ddrCh2=ddrchannel(1);
         ddrCh2->set_RF_LO_source(3);
       // by custom this is dn but this is not enforced...
   }

    /* send OBSch to master to id lo switch, select T/R 2 places!, preamp, set hi/low */
    k = 0;
    if (RFUserTable[OBSch-1]->getBaseFrequency() > 405.0)
      k  = 1; // selects hi band
    if ((numRFChan >= 2) && (RFUserTable[DECch-1]->getBaseFrequency() > 405.0))
      k |= 2; // selects hi band for DECch
    if ((numRFChan >= 3) && (RFUserTable[DEC2ch-1]->getBaseFrequency() > 405.0))
      k |= 4; // selects hi band for DEC2ch
    if ((numRFChan >= 4) && (RFUserTable[DEC3ch-1]->getBaseFrequency() > 405.0))
      k |= 8; // selects hi band for DEC3ch
    // the following works for upto 2 receivers (HX/HF)
    double w = whatamphibandmin(0,0.0);
    // prevent core dump on numRFChan == 1 !
    //  if rf1 and rf2 are both HIGH BAND - flip on sharing
    //  since rf1 is ALWAYS high band,  test rf2 only..
    if ((numRFChan > 1) && (RFUserTable[1]->getBaseFrequency() > w))
    {
      RFShared = 2; /* 4 is also possible */
      RFSharedDecoupling = 0;  /* default */
      RFUserTable[1]->rfSharingEnabled=1;
    }
    master->setConsoleMap(receiverConfig, OBSch, DECch, k, tnlkflag);

    // get nucleus names into an array to set rfchnuclei parameter
    char nucleiNameStr[MAXSTR], nucname[MAXSTR];
    strcpy(nucleiNameStr,"'");
    ((RFController *)RFUserTable[0])->getNucleusName(nucname);
    if (strcmp(nucname,"") == 0)
       strcpy(nucname,"-");
    strcat(nucleiNameStr, nucname);
    for (int i=1; i<numRFChan; i++)
    {
      strcat(nucleiNameStr, " ");
      ((RFController *)RFUserTable[i])->getNucleusName(nucname);
      if (strcmp(nucname,"") == 0)
         strcpy(nucname,"-");
      strcat(nucleiNameStr, nucname);
    }
    strcat(nucleiNameStr,"'");
    if (P_getstring(CURRENT, "rfchnuclei", nucname, 1, 255) >= 0)
    {
       putCmd("rfchnuclei = %s",nucleiNameStr);
    }


    RollCallString[0] = '\0';
    for (k=0; k < nValid; k++)
    {
      strcpy(tmp, PhysicalTable[k]->getName());
      // don't add master1 or lock1 to the list.
      if ((strcmp(tmp,"master1")!=0) && (strcmp(tmp,"lock1")!=0))
      {
        strcat(RollCallString,tmp);
        strcat(RollCallString," ");
      }
    }

    RollCallString[strlen(RollCallString)]='\0'; // kill last blank
}


int Console::getIndexToRFChannel(char* name)
{
  char rfc;
  rfc=name[2];
  double numrfchan;
  if (getparmd("numrfch","real",GLOBAL,&numrfchan,1))
    numrfchan=0;
  for(int i=0;i<numrfchan;i++)
  {
    if (rfc == rfChannelStr[i]) return (i+1);
  }
  return(0);
}


int Console::getFirstActiveRcvr()
{
  return firstActiveRcvrIndex;
}

int Console::getNumActiveRcvrs()
{
   return numActiveRcvrs;
}


void Console::setNumActiveRcvrs(int numactive)
{
   numActiveRcvrs = numactive;
}


//  base class is virtual for the controllers...
void Console::initializeExpStates(int setupflag)
{
   int ilflag;
  // setupflag is 0 = GO    & 1 = SU

  // is there enough Recvproc Buffering for FID transfers ?

   if(!chk4EnoughHostBufferMemory())
       return;

   // if not setup then obtain the il flag
   if (setupflag != 1)
   {
       /* il flag */
       ilflag = getIlFlag();    // 1 = true, 0 = false 
   }
   else
      ilflag = -99;

  AcodeManager *pAcodeMgr   = AcodeManager::getInstance();
  pAcodeMgr->startSubSection(ACODEHEADER);

  /* force all controllers in the initializeExpStates to be Sync
   * as is done for the pulse sequnece
   */
  broadcastSystemSync(320  /* ticks */, 0 /* never a prepScan */);  /*  4 usec, a grad requirement */

  F_initval(0.0, rtonce);  // call all valid controllers 
  for (int i = 0; i < nValid; i++)
  {
    PhysicalTable[i] -> initializeExpStates(setupflag);
    
    /* initialize rtonce rtvar  this happens in the INIT stage and not the PS stage */

    if (setupflag)
    {
      // need to send END_PARSE   with setupflag=su argument
      int codebuffer[2];
      codebuffer[0] = setupflag;
      codebuffer[1] = -999;
      PhysicalTable[i]->outputACode(END_PARSE,1,codebuffer);
    }
    else
    {
      int codebuffer[2];
      codebuffer[0] = ilflag;
      codebuffer[1] = -999;
      // need to send Iterleave mode  1 argument
      PhysicalTable[i]->outputACode(IL_MODE,1,codebuffer);

      // need to send NEXTCODESET with setupflag=go argument
      codebuffer[0] = -999;
      codebuffer[1] = -999;
      PhysicalTable[i]->outputACode(NEXTCODESET,0,codebuffer);
    }
  }

  pAcodeMgr->endSubSection();
}



void Console::describe(char *pntr)
{
  int i;
  fprintf(stdout,"Channel status %s\n",pntr);
  for (i = 0; i < nValid; i++)
  {
    if (PhysicalTable[i] == NULL)
       fprintf(stdout, "Controller PhysicalTable[%d] is null\n",i);
    else
       PhysicalTable[i]->describe("Controller status");
  }
  if (initAcqObject != NULL) initAcqObject->describe(pntr);
}

void Console::describe2(char *pntr)
{
  int i;
  fprintf(stdout,"System Configuration\n");

  fprintf(stdout, "consoleSubType = %c\n", consoleSubType);
  fprintf(stdout, "consoleRFInterface = %d\n", consoleRFInterface);
  fprintf(stdout, "consoleHFmode = %c\n", consoleHFmode);
  fprintf(stdout, "consoleCryoUsage = %d\n", consoleCryoUsage);
  fprintf(stdout, "numrfch = %d\n",numRFChan);
  fprintf(stdout, "HB Mask = %d\n",consoleHBmask);

  for (i = 0; i < nValid; i++)
  {
    if (PhysicalTable[i] == NULL)
       fprintf(stdout, "Controller PhysicalTable[%d] is null\n",i);
    else
       PhysicalTable[i]->describe("Controller status");
  }
  if (initAcqObject != NULL) initAcqObject->describe(pntr);
}



char Console::getConsoleSubType()
{
  return consoleSubType;
}


int Console::getConsoleRFInterface()
{
  return consoleRFInterface;
}

char Console::getConsoleHFMode()
{
  return consoleHFmode;
}

int Console::getConsoleCryoUsage()
{
  return consoleCryoUsage;
}

int Console::getNumberControllers()
{
  return(nValid);
}

int Console::addController(Controller *mine)
{
   int i;
   // should check conflicting definitions...
   //
   for (i=0; i < MAXCHAN; i++)
   {
      if (PhysicalTable[i] == 0)
      {
        PhysicalTable[i] = mine;
        nValid++;
        nActive++;
        return(0);
      }
   }
   abort_message("configuration error. out of channels. abort!\n");
   return(0);
}


RFController * Console::getRFControllerByLogicalIndex(int k)
{
  char emessage[MAXSTR];
  if (RFUserTable[k-1] != NULL)
     return(RFUserTable[k-1]);
  else
  {
      sprintf(emessage,"no controller mapped at RF index %d. abort!\n",k);
      abort_message(emessage);
  }
  return(0);
}

Controller *Console::getDDRControllerByIndex(int k)
{
  if (DDRUserTable[k-1] == 0)
    {
      abort_message("no controller mapped at this DDR index %d. abort!\n", k );
    }
    return(DDRUserTable[k-1]);
}

//
//
//
Controller *Console::getFirstActiveDDRController()
{
  DDRController *ddrCh = NULL;

  for(int i=0; i<numDDRChan; i++)
  {
    ddrCh = (DDRController *)DDRUserTable[i];

    if ( ! ddrCh->ddrActive4Exp ) continue;

    if ( (i == 0) && (getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) )
    {
      if (OBSch == RCVR2_RF_LO_CHANNEL)
        ddrCh = (DDRController *)DDRUserTable[1];
      else
        ddrCh = (DDRController *)DDRUserTable[0];
    }

    if ( ! ddrCh->ddrActive4Exp ) continue;
    if (ddrCh != NULL)
      return ddrCh;
  }
  return ddrCh;
}

//
//  The calling function is responsible for actions
//
Controller *Console::getControllerByID(const char *key)
{
  int i;
  for (i=0; i < nValid; i++)
  {
      if (PhysicalTable[i]->matchID(key))
	  return(PhysicalTable[i]);
  }
  text_message("no matching channel for %s\n",key);
  return(0);
}

Controller *Console::getGRADController()
{
   return (Controller *)GRADUserTable;
}

Controller *Console::getPFGController()
{
   return (Controller *)PFGUserTable;
}


GradientBase * Console::getConfiguredGradient()
{
   GradientBase *gC = NULL;
   char gradconfig = 'a';

   int len = strlen(gradtype);

   switch (len)
   {
      case 3:
           gradconfig = gradtype[0];
           if ( (gradconfig == 'n') || (gradconfig == 'a') )
           {
              gradconfig = gradtype[1];
              if ( (gradconfig == 'n') || (gradconfig == 'a') )
                 gradconfig = gradtype[2];
           }
           break;

      default:
           abort_message("parameter gradtype not configured correctly. abort!\n");
           break;
   }

   switch (gradconfig)
   {
      case 'a':
      case 'h':
            gC = NULL;
            break;

      case 'l':       // L620 Performa I successor
      case 'c':       // Performa IV
      case 'd':       // Performa IV + WFG
      case 'p':       // Performa II/III
      case 'q':       // Performa II/III + WFG
      case 't':       // Performa XYZ
      case 'u':       // Performa XYZ    + WFG

            gC = (GradientBase *) P2TheConsole->getControllerByID("pfg1");
            if (gC == NULL)
               abort_message("unable to find PFG Controller. configuration error. abort!\n");
            break;


      case 'w':        // Imaging Gradient + WFG
      case 'r':        // Imaging Gradient + WFG + CRB

            gC = (GradientBase *) P2TheConsole->getControllerByID("grad1");
            if (gC == NULL)
               abort_message("unable to find Gradient Controller. configuration error. abort!\n");
            break;

      default:
            abort_message("invalid configuration in Gradient or PFG for oblique_gradient. abort!\n");
   }
   return gC;
}


long long Console::getMinTimeEventTicks()
{
   return MinTimeEventTicks;
}


InitAcqObject * Console::getInitAcqObject()
{
   return initAcqObject;
}


void Console::newEvent()
{
  int i;
  Controller *tmp;
  for (i=0; i<nValid; i++)
    {
      tmp = PhysicalTable[i];
      if (tmp->isActive())
	tmp->setSync();
    }
}

void Console::update4Sync(long long tickCount)
{
  int i;
  Controller *p2Ch;

  for (i=0; i < nValid; i++)
  {
      p2Ch = PhysicalTable[i];
      p2Ch->update4Sync(tickCount);
  }

}

void Console::update4SyncNoteDelay(long long tickCount)
{
  int i;
  Controller *p2Ch;

  for (i=0; i < nValid; i++)
  {
      p2Ch = PhysicalTable[i];
      p2Ch->noteDelay(tickCount);
  }

}

//  NYI >>> update
void Console::flushAllEvents()
{
  int i;
  for (i=0; i < nValid; i++)
    PhysicalTable[i]->flushDload();
}



// broadcast sync to all Controllers with FIFOs

void Console::broadcastSystemSync(int many,int prepflag)
{
  for (int i=0; i < nValid; i++)
    PhysicalTable[i]->SystemSync(many,prepflag);
}


// broadcast codes to all controllers with FIFOs

void Console::broadcastCodes(int code, int many, int *stream)
{
  for (int i = 0; i < nValid; i++)
     PhysicalTable[i]->outputACode(code, many, stream);
}

//
//
void Console::resetSync()
{
  for (int i=0; i < nValid; i++)
  {
      if (!PhysicalTable[i]->isOff())
        {
          PhysicalTable[i]->resetSync();
        }
  }
}

double Console::duration()
{
  long long reference;
  reference = PhysicalTable[0]->getBigTicker();
  return( (double) reference * 12.5e-9 );
}

int Console::verifySync(const char *lbl)
{
  int i;
  long long reference, flag;
  long long tmp=0L;
  reference = PhysicalTable[0]->getBigTicker();

  double duration = reference * 12.5e-9;
#ifndef __INTERIX
  if (bgflag) { cout << "\nConsole Verify Sync: Master (ticks) = " << reference << "   Duration = " << duration << " sec \n";}
#endif

  double numscans; long long totalTicks;
  getparmd("nt","real",CURRENT,&numscans,1);
  duration   = duration * numscans;
  totalTicks = (long long) (reference * numscans);
#ifndef __INTERIX
  if (bgflag) { cout << "  nt = "<< numscans << "    Total Ticks = " << setw(12) << totalTicks << "    Total Duration = " << duration << " sec \n"; }
#endif

  flag = 0;
  for (i=1; i < nValid; i++)
  {
      if (!PhysicalTable[i]->isOff())
      {
	  tmp = PhysicalTable[i]->getBigTicker();
          if (reference != tmp)
          {
	    flag = 1;
#ifndef __INTERIX
            if (bgflag) { cout << lbl << "  timing error " << reference-tmp << endl;}
#endif
          }
      }
  }

 if (flag == 0)
 {
   if (bgflag)
   {
     cout.setf(ios::hex);
#ifndef __INTERIX
     cout << "<<<<< " << lbl << "  VERIFY SYNC TICK COUNT OK  " << tmp << " >>>>>\n" << endl;
#endif
   }
 }
 else
 {
   if (bgflag)
   {
     cout.setf(ios::hex);
     cout << "<<<<< " << lbl << "  TICK COUNT ERROR !!" << " >>>>>\n" << endl;
   }
   abort_message("timing synchronization tick count mismatch. abort!\n");
 }

 return(flag);
}


void Console::printSyncStatus()
{
  char id[128];
  long long tks;

  cout << "\nConsole Timing Synchronization Status \nController     Accumulated Ticks\n";

  for (int i=0; i < nValid; i++)
    {
      if ( !PhysicalTable[i]->isOff() )
        {
          strcpy(id,PhysicalTable[i]->getName());
          tks = PhysicalTable[i]->getBigTicker();
#ifndef __INTERIX
          cout << setiosflags(ios::left) << setw(10) <<  id << setiosflags(ios::right) << setw(12) << tks << endl;
#endif
        }
    }
}

void Console::setAcodeBuffers()
{
  for (int i=0; i<nValid; i++)
  {
    PhysicalTable[i]->setAcodeBuffer();
  }
}



void Console::setWaveformBuffers()
{
  for(int i=0;i<numRFChan;i++)
  {
    ((RFController *)RFUserTable[i])->setWaveformBuffer();
  }

  if (PFGUserTable != NULL)
    PFGUserTable->setWaveformBuffer();

  if (GRADUserTable != NULL)
    GRADUserTable->setWaveformBuffer();

  Controller *master1 = getControllerByID("master1");
  if (master1 == NULL)
          printf("Console::setWaveformBuffers: master1 is NULL\n");
  master1->setWaveformBuffer();

  for (int j=0; j<numDDRChan; j++)
  {
     ddrchannel(j)->setWaveformBuffer();
  }
}



void Console::flushAllControllerDload()
{
  for (int i=0; i < nValid; i++)
  {
      PhysicalTable[i]->flushDload();
  }
}


void Console::closeWaveformFiles()
{
  for(int i=0;i<numRFChan;i++)
  {
    ((RFController *)RFUserTable[i])->writePatternAlias();
    ((RFController *)RFUserTable[i])->closeWaveformFile(0);
  }

  if (PFGUserTable != NULL)
    PFGUserTable->closeWaveformFile(0);

  if (GRADUserTable != NULL)
    GRADUserTable->closeWaveformFile(0);

  Controller *master1 = getControllerByID("master1");
  if (master1 != NULL)
    master1->closeWaveformFile(0);

  for (int j=0; j<numDDRChan; j++)
  {
     if (ddrchannel(j) != NULL)
       ddrchannel(j)->closeWaveformFile(0);
  }
}

void Console::closeAllFiles(int num)
{
  int i;
  for (i=0; i < nValid; i++)
    {
      if (!PhysicalTable[i]->isOff())
	PhysicalTable[i]->closeAcodeFile(0);
    }
  initAcqObject->closeAcodeFile(0);
}
//
//  full procedure should be:
//  each channel should perform set up tasks
//  pad to 4 usec
//  could do all channels - get maximum and pad to that
//  4 usec is probably a good start.
//  All channels must track the time if they are synchronoous.
//
void Console::postInitScanCodes()
{
  long long ticks,initD;
  int i;
  initD = 324; //
  for (i=0; i < nValid; i++)
    {
      if (!PhysicalTable[i]->isOff())
	{
	   ticks = PhysicalTable[i]->initializeIncrementStates(initD);  // accomodate gain settings
           if ((i == 0) &&  (ticks > 324)) initD = ticks;  // only master..
        }
    }
}

int  Console::syncCompVar(int moduloticks, int myTicks)
{
   long long tick1;

   tick1 = PhysicalTable[0]->getBigTicker();
   tick1 = moduloticks - (tick1 % moduloticks);
   tick1 += myTicks;  /* make or break the modulo */
   if (tick1 < P2TheConsole->getMinTimeEventTicks()) tick1 += moduloticks;  /* guard against min time */
   newEvent();
   PhysicalTable[0]->setActive();
   PhysicalTable[0]->setTickDelay(tick1);
   update4Sync(tick1);
   return((int) tick1);
}


void Console::startTicker(int type)
{
  switch (type)
  {
    case LOOP_TICKER:
      // capture the current BigTicker
      nestedLoopDepth++;
      loop_Ticker = PhysicalTable[0]->getBigTicker();
      break;

    case RL_LOOP_TICKER:
      // capture the current BigTicker
      nestedLoopDepth++;
      if (parallelIndex)
         loop_Ticker = parallelController->getBigTicker();
      else
         loop_Ticker = PhysicalTable[0]->getBigTicker();
      break;

    case IFZ_TRUE_TICKER:
    case IFZ_FALSE_TICKER:
      if (progDecInterLock) abort_message("obs/decprgon followed by ifzero not implemented. abort!\n");
      break;

    case NOWAIT_LOOP_TICKER:
      // capture the current BigTicker
      nowait_loop_Ticker = PhysicalTable[0]->getBigTicker();
      break;

    default:
      text_message("psg error: invalid Console ticker request\n");
      break;
  }
}


long long Console::stopTicker(int type)
{
  switch (type)
  {
    case LOOP_TICKER:
      loop_Ticker  = PhysicalTable[0]->getBigTicker() - loop_Ticker;

      // on all RFController objects, check if prg dec on
      for(int i=0;i<numRFChan;i++)
      {
        ((RFController *)RFUserTable[i])->loopAction();
      }

      nestedLoopDepth--;
      return(loop_Ticker);
      break;

    case RL_LOOP_TICKER:
      if (parallelIndex)
         loop_Ticker = parallelController->getBigTicker() - loop_Ticker;
      else
         loop_Ticker = PhysicalTable[0]->getBigTicker() - loop_Ticker;

      // on all RFController objects, check if prg dec on
      if ( ! parallelIndex)
         for(int i=0;i<numRFChan;i++)
         {
           ((RFController *)RFUserTable[i])->loopAction();
         }

      nestedLoopDepth--;
      return(loop_Ticker);
      break;

    case NOWAIT_LOOP_TICKER:
      // return the current value
      nowait_loop_Ticker = (PhysicalTable[0]->getBigTicker()) - nowait_loop_Ticker;
      return(nowait_loop_Ticker);
      break;

    case IFZ_TRUE_TICKER:
    case IFZ_FALSE_TICKER:
      break;

    default:
      text_message("psg error: invalid Console ticker request\n");
      return(0);
      break;
  }
  return(0);
}



long long Console::getTicker(int type)
{
  switch (type)
  {
    case LOOP_TICKER:
      return(loop_Ticker);
      break;

    case NOWAIT_LOOP_TICKER:
      return(nowait_loop_Ticker);
      break;

    case IFZ_TRUE_TICKER:
    case IFZ_FALSE_TICKER:
      return(0);
      break;

    default:
      text_message("psg error: invalid Console ticker request\n");
      return(0);
      break;
  }
}


int Console::getNestedLoopDepth()
{
  return nestedLoopDepth;
}


void Console::setProgDecInterLock()
{
  progDecInterLock++;
}


void Console::clearProgDecInterLock()
{
  progDecInterLock--;
  if (progDecInterLock < 0)
  {
    abort_message("psg error: invalid value for programmable decoupling interlock. abort!\n");
  }
}


int  Console::getProgDecInterLock()
{
   return progDecInterLock;
}

void Console::setAuxHomoDecTicker(long long ticks)
{
  for (int i=0; i < nValid; i++)
  {
      if (PhysicalTable[i]->isRF())
      {
        ((RFController *)PhysicalTable[i])->setAuxHomoDecTicker(ticks);
      }
  }
}

int Console::turnOffRFDuplicates()
{
   char baseStr[MAXSTR],tmpstr[MAXSTR];
   const char *parN[] = {"dn","dn2","dn3","dn4","dn5"};
   int i;
   if ((numRFChan < 2) || (numRFChan > 5)) return(1);
   strcpy(baseStr,"");  // matches are errors
   P_getstring(CURRENT,"tn",tmpstr,1,MAXSTR);
   if (strcmp(tmpstr,"none"))
     strcpy(baseStr,tmpstr);
   for (i = 0; i < numRFChan-1; i++)
   {
     getStringSetDefault(CURRENT,parN[i],tmpstr,"none");
     if ((strlen(tmpstr)) && (strcmp(tmpstr,"none")))
     {
       strcat(baseStr," ");
       if (strstr(baseStr,tmpstr) != 0)
       {
         if ( ! dps_flag)
            text_message("%s duplicates others: %s set to none",parN[i],parN[i]);
         P_setstring(CURRENT, parN[i], "none", 1);
       }
       else  // no duplicate -
         strcat(baseStr,tmpstr);
     }
   }
   return(2);
}


char Console::getRcvrsType()
{
   return rcvrsType;
}


void Console::checkForErrors()
{
  if ((consoleSubType == 's') && (consoleRFInterface == 3))
  {
     if ( (consoleHFmode != 'c') && ((consoleHBmask==3)||(consoleHBmask==5)||(consoleHBmask==6)) )
     {
        int numactive = 0;
        RFController *rfCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(1);
        numactive += rfCh->getChannelActive();
        rfCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(2);
        numactive += rfCh->getChannelActive();
        rfCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(3);
        if (rfCh != NULL)
           numactive += rfCh->getChannelActive();
        if (numactive >= 2)
           text_message("advisory: hfmode should be set to 'c', when channels 1 & 3 are both highband");
     }

     if ( (consoleHFmode == 'c') && ((consoleHBmask==1)||(consoleHBmask==2)||(consoleHBmask==4)) )
     {
        text_message("advisory: verify that hfmode is set in the same mode as how the probe HB channel is tuned\n");
     }
  }
}


/*
 * obtain the system DRAM size
 *
 */
void Console::getSystemRamSize()
{
    int numPages, pageSize;
    long long totalRAM;
    /* Note: Multiplying sysconf(_SC_PHYS_PAGES)  or
     * sysconf(_SC_AVPHYS_PAGES) by sysconf(_SC_PAGESIZE) to deter-
     * mine memory amount in bytes can exceed  the  maximum  values
     * representable in a int or unsigned int.
     */
#if defined( __INTERIX) || defined(MACOS)
// TODO: find method to set numPages in SFU (_SC_PHYS_PAGES is undefined)
     numPages = 1;
#else
     numPages = sysconf(_SC_PHYS_PAGES);
#endif
     pageSize = sysconf(_SC_PAGESIZE);
     totalRAM = (long long) numPages * (long long) pageSize;
     HostSystemMemory = totalRAM;
     return;
}

/*
 * check on number of active receivers and data size to be sure
 * the Host buffering can handle it
 *
 */
int Console::chk4EnoughHostBufferMemory()
{
    char cbuff[MAXSTR];
    long long alottedMemory, FidSize_NF_Corrected,RequiredMemBytes;
    char errstr[256];
    unsigned int fidSizeBytes;
    int numActiveDDRs;
    unsigned int nfadj,numDataPts,calc_nbufs;
    int nfmod=0,nf=1;

#ifdef MACOS
    return(1);
#endif
    getSystemRamSize(); // initialize the amount of system ram availble

    numDataPts = (unsigned int) np;

    // fprintf(stdout,">>>>> np: %d fidptsize: %d\n",np,fidptsize);
//HostSystemMemory=100000000;
    alottedMemory = HostSystemMemory / 4LL;

    numActiveDDRs = getNumActiveRcvrs();
    // fprintf(stdout,">>>>> alottedMemory: %llu, numActiveRcvrs: %d\n",alottedMemory,numActiveDDRs);

    // printf("alottedMemory=%llu \n",alottedMemory);

    int fidptsize = 4; // default double precision dp='y'
    cbuff[0]='y';
    getstr("dp",cbuff);    // data type flag

    if(cbuff[0]=='n')
        fidptsize = 2;

    if (var_active("nf",CURRENT)==1) {
        int tmp = (int) getval("nf");
        if (tmp < 1.0)
            nf = 1;
        else
            nf=(int)tmp;
    }

    if (var_active("nfmod",CURRENT)==1)
        nfmod = (int)getval("nfmod");
    if(nfmod<=0 || nfmod>nf)
        nfmod=nf;
    if (nf/nfmod != (double)nf/(double)nfmod)
        abort_message("nf not an integral multiple of nfmod, run prep to adjust nfmod");

    nfadj = nf / nfmod;
    fidSizeBytes = numDataPts * fidptsize * nf;
    FidSize_NF_Corrected = (long long) fidSizeBytes / (long long) nfadj;
    calc_nbufs = (unsigned int) ((alottedMemory / (long long) numActiveDDRs) / FidSize_NF_Corrected);
    RequiredMemBytes = 2 /* minbuffer required */ * ((long long) numActiveDDRs) *  FidSize_NF_Corrected;
    /* fprintf(stdout,">>>>> nf: %d nfmod: %d, fidsize: %lu, FidSize_NF_Corrected: %llu, calc bufs: %d\n",
      nf,nfmod,fidSizeBytes,FidSize_NF_Corrected,calc_nbufs); */
    if (calc_nbufs < 2)
    {
      sprintf(errstr,"Data Size %llu MB with %d active receivers, has exceeded Host buffer requirement (%llu MB) by %llu MB, abort.\n",
        FidSize_NF_Corrected / 1048576, numActiveDDRs, RequiredMemBytes/1048576, (RequiredMemBytes - alottedMemory)/1048576 );
      abort_message(errstr);
      return 0;
    }
    return 1;
}


void Console::showPowerIntegral()
{
  int numcntrlrs = nValid;

  cout << endl<<"Power Info:"<<endl;
  for (int i=0; i < numcntrlrs; i++)
  {
    Controller *contr = PhysicalTable[i];
    contr->showPowerIntegral();
  }
}


void Console::showEventPowerIntegral(const char* comment)
{
  int numcntrlrs = nValid;

  for (int i=0; i < numcntrlrs; i++)
  {
    Controller *contr = PhysicalTable[i];
    contr->showEventPowerIntegral(comment);
  }
}


void Console::suspendPowerCalculation()
{
  int numcntrlrs = nValid;

  for (int i=0; i < numcntrlrs; i++)
  {
    Controller *contr = PhysicalTable[i];
    contr->suspendPowerCalculation();
  }
}


void Console::enablePowerCalculation()
{
  int numcntrlrs = nValid;

  for (int i=0; i < numcntrlrs; i++)
  {
    Controller *contr = PhysicalTable[i];
    contr->enablePowerCalculation();
  }
}


void Console::eventStartAction()
{
  int numcntrlrs = nValid;

  for (int i=0; i < numcntrlrs; i++)
  {
    Controller *contr = PhysicalTable[i];
    contr->eventStartAction();
  }
}

void Console::setParallelKzDuration(double duration)
{
   parallelKzDuration = duration;
}

double Console::getParallelKzDuration()
{
   return(parallelKzDuration);
}

void Console::setParallelInfo(int info)
{
   parallelInfo = info;
}

int Console::getParallelInfo()
{
   return(parallelInfo);
}

void Console::parallelEvents(const char *chanType, int chanIndex)
{

   if ( (chanIndex < 0) || (chanIndex > nValid) )
   {
      abort_message("parallelEvents on non-existent channel index %d",
                    chanIndex);
   }
   parallelController = NULL;
   if (parallelIndex == 0)
   {
      parallelInfo = 0;
      parallelKzDuration = 0.0;
      for (int i=0; i < nValid; i++)
      {
         parallelTicks[i] = 0;
         parallelSyncPos[i] = 0;
      }
   }
   parallelIndex = chanIndex;
   if ( ! strcmp(chanType,"rf") )
      parallelController = getRFControllerByLogicalIndex(chanIndex);
   else if ( ! strcmp(chanType,"rcvr") )
      parallelController = getDDRControllerByIndex(chanIndex);
   else if ( ! strcmp(chanType,"grad") )
   {
      parallelController = getPFGController();
      if (parallelController == NULL)
         parallelController = getGRADController();
   }
   if (parallelController == NULL)
   {
      abort_message("parallelEvents on non-existent %s channel %d",
                    chanType, chanIndex);
   }
}

double Console::parallelEnd()
{
  Controller *p2Ch;
  long long max;
  long long min;
  long long minTicks = getMinTimeEventTicks();

  parallelIndex = 0;
  newEvent();
  max = 0;
  for (int i=0; i < nValid; i++)
  {
     if (parallelTicks[i] > max)
        max = parallelTicks[i];
  }
  min = 0;
  for (int i=0; i < nValid; i++)
  {
     if (max != parallelTicks[i])
        if ( (min == 0) || (max - parallelTicks[i] < min) )
           min = max - parallelTicks[i];
  }
  if ( (min > 0) && (min < minTicks) )
  {
     max += minTicks;
  }
  for (int i=0; i < nValid; i++)
  {
     p2Ch = PhysicalTable[i];
     if (max - parallelTicks[i] > 0)
     {
         if ( parallelSyncPos[i] )
         {
            p2Ch->update4ParallelSync(max - parallelTicks[i],
                                      parallelSyncPos[i]);
         }
         else
         {
            p2Ch->update4Sync(max - parallelTicks[i]);
         }
     }
  }
  return( (double) max * 12.5e-9 );
}

int Console::isParallel()
{
   return(parallelIndex);
}

Controller *Console::getParallelController()
{
   return(parallelController);
}

void Console::parallelElapsed(const char *name, long long ticks)
{
   Controller *p2Ch;
   for (int i=0; i < nValid; i++)
   {
      p2Ch = PhysicalTable[i];
      if (p2Ch->matchID(name))
      {
         parallelTicks[i] += ticks;
         break;
      }
   }
}

void Console::parallelSync()
{
   int buffer[4];
   Controller *p2Ch;
   if ( ! parallelIndex)
      abort_message("parallelsync called outside parallel sections.");
   buffer[0] = 0;
   buffer[1] = 0;
   parallelController->outputACode(BIGDELAY,2,buffer);
   for (int i=0; i < nValid; i++)
   {
      p2Ch = PhysicalTable[i];
      if (p2Ch->matchID(parallelController->getName()))
      {
         parallelSyncPos[i] =  parallelController->getAcodePosition() - 2;
         break;
      }
   }
   
}

void Console::parallelShow()
{
   fprintf(stderr,"Parallel Status ********* \n");
   fprintf(stderr,"Events %s in parallel mode\n",
           (parallelIndex == 0) ? "are not" : "are" );
   fprintf(stderr,"parallelIndex = %d \n", parallelIndex );
   for (int i=0; i < nValid; i++)
   {
      fprintf(stderr,"parallelTicks[%d] = %lld synced= %d (%s)\n",
              i, parallelTicks[i],parallelSyncPos[i],
              PhysicalTable[i]->getName() );
   }
   fprintf(stderr,"Parallel Status ********* \n");
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <string.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include "Console.h"
#include "PFGController.h"
#include "GradientController.h"
#include "ACode32.h"
#include "FFKEYS.h"
#include "Bridge.h"
#include "GradientBridge.h"
#include "cpsg.h"

extern "C" {

#include "safestring.h"

}

extern unsigned int ix;
extern int bgflag;
extern "C" char gradtype[];
extern int specialGradtype;
extern double gxmax;
extern double gymax;
extern double gzmax;
extern double gmax;
extern double gtotlimit;
extern double gxlimit;
extern double gylimit;
extern double gzlimit;
extern double gradstepsz;
extern double gradalt;
extern int ct;
extern int gxFlip;
extern int gyFlip;
extern int gzFlip;
extern int readuserbyte;

extern "C" void ifmod2zero(int rtvar);
extern "C" void elsenz(int rtvar);
extern "C" void endif(int rtvar);

#define MAX_GRAD_AMP  32767.0
#define MAXSTR 256
#define START   0
#define END     1


int grad_flag = FALSE;

//
//
//
void pfgenable(char *key)
{
  PFGController *tmp = (PFGController *) P2TheConsole->getControllerByID("pfg1");
  if (tmp == 0)
    exit(-1);
  tmp->setEnable(key);
}

//
//
//
int getorientation(char *c1,char *c2,char *c3,char *orientname)
{
   char orientstring[MAXSTR];
   int i;
   getstr(orientname,orientstring);
   if (orientstring[0] == '\0')
   {
      abort_message("can't find variable in tree\n");
   }
   else if (bgflag)
     fprintf(stderr,"orientname = %s\n",orientname);
   for (i=0;i<3;i++)
   {
     switch(orientstring[i])
     {
       case 'X': case 'x':
       case 'Y': case 'y':
       case 'Z': case 'z':
       case 'R': case 'r':
       case 'N': case 'n':
       break;
       default: return(-1);
     }
   }
   *c1 = orientstring[0];
   *c2 = orientstring[1];
   *c3 = orientstring[2];
   return(0);
}

//
//
//
void grad_limit_checks(double xgrad,double ygrad,double zgrad,char *routinename)
{
  double precision_limit;

   if (bgflag)
     cout << "grad_limit_checks: xgrad=" << xgrad << " ygrad=" << ygrad << " zgrad=" << zgrad << endl;

    precision_limit = 0.5/MAX_GRAD_AMP;
    xgrad = fabs(xgrad);
    ygrad = fabs(ygrad);
    zgrad = fabs(zgrad);
    if (xgrad > (gxlimit+(gxlimit*precision_limit)))
    {
        abort_message("%s: X gradient: %7.3f exceeds safety limit: %7.3f.\n",
                                                routinename,xgrad,gxlimit);
    }
    if (ygrad > (gylimit+(gylimit*precision_limit)))
    {
        abort_message("%s: Y gradient: %7.3f exceeds safety limit: %7.3f.\n",
                                                routinename,ygrad,gylimit);
    }
    if (zgrad > (gzlimit+(gzlimit*precision_limit)))
    {
        abort_message("%s: Z gradient: %7.3f exceeds safety limit: %7.3f.\n",
                                                routinename,zgrad,gzlimit);
    }

}

//
//
//
void set_rotation_matrix(double ang1, double ang2, double ang3)
{
    if (gradtype[0] != gradtype[1])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");
    if (gradtype[0] != gradtype[2])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");

    GradientBase  *gC = P2TheConsole->getConfiguredGradient();

    gC->set_rotation_matrix(ang1,ang2,ang3);
}

//
//
//
int create_rot_angle_list(char *nm, double* angle_set, int num_sets)
{
   if (gradtype[0] != gradtype[1])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");
   if (gradtype[0] != gradtype[2])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");

   int listId = -1;
   GradientBase  *gC = P2TheConsole->getConfiguredGradient();
   listId = gC->create_rotation_list(nm, angle_set, num_sets);

   if (listId > 0)
     return listId;
   else
     text_error("error in creating rotation list on %s . abort!\n", gC->getName());
   return(listId);
}

//
//
//
void  rot_angle_list(int listId, char mode, int vindex)
{
   if (gradtype[0] != gradtype[1])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");
   if (gradtype[0] != gradtype[2])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");
   if ( (listId < 1) || (listId >= 0x7FFFFF) )
       abort_message("invalid coordinate rotation list id specified. abort!\n");
   if (! validrtvar(vindex))
       abort_message("invalid real time variable specified in rot_angle_list command. abort!\n");

   int buffer[2];
   GradientBase  *gC = P2TheConsole->getConfiguredGradient();

   buffer[0] = listId;
   buffer[1] = vindex;
   gC->outputACode(SETVGRDROTATION,2,buffer);
}

//
//
//
int create_angle_list(char *nm, double* angle_set, int num_sets)
{
   if (gradtype[0] != gradtype[1])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");
   if (gradtype[0] != gradtype[2])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");

   int listId = -1;
   GradientBase  *gC = P2TheConsole->getConfiguredGradient();
   listId = gC->create_angle_list(nm, angle_set, num_sets);

   if (listId > 0)
     return listId;
   else
     text_error("error in creating rotation list on %s . abort!\n", gC->getName());
   return(listId);
}

//
//
//
void set_angle_list(int angleListId, char *logAxis, char mode, int rtvar)
{
   if (gradtype[0] != gradtype[1])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");
   if (gradtype[0] != gradtype[2])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");

   GradientBase  *gC = P2TheConsole->getConfiguredGradient();
   gC->set_angle_list(angleListId, logAxis, mode, rtvar);
}

//
//
//
void exe_grad_rotation()
{
   int buffer[3];

   GradientBase  *gC = P2TheConsole->getConfiguredGradient();

   buffer[0] = gxFlip;
   buffer[1] = gyFlip;
   buffer[2] = gzFlip;

   gC->outputACode(EXEVGRDROTATION,3,buffer);
}

//
//
//
void phase_encode3_gradient(double stat1, double stat2, double stat3,              \
     double step1, double step2, double step3,                                     \
     int vmult1, int vmult2, int vmult3, double lim1, double lim2, double lim3)
{
    int peR_step, peP_step, peS_step, peR_lim, peP_lim, peS_lim;

    if (gradtype[0] != gradtype[1])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");
    if (gradtype[0] != gradtype[2])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");


    GradientBase *gC = P2TheConsole->getConfiguredGradient();

    if (ix == 1) gC->errorCheck(0,stat1,stat2,stat3,step1,step2,step3);

    if (gC->isNoWaitMode())
       abort_message("gradient event requested too soon after previous one in NOWAIT mode. abort!\n");


   /* set the GRADSCALE here, for GRADIENT it is all 0x4000; for Triax PFG they may be different */

   gC->setGradScale("X",gC->GXSCALE);
   gC->setGradScale("Y",gC->GYSCALE);
   gC->setGradScale("Z",gC->GZSCALE);


    int buffer[4];

       if (step1 != 0.0)
       {
           if (! validrtvar(vmult1))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 1;                        // destination  RO
           buffer[1] = (int)(step1*(gC->GMAX_TO_DAC));
           buffer[2] = vmult1;
           buffer[3] = (int) (lim1+0.5);
           gC->outputACode(SETPEVALUE,4,buffer);
           peR_step  = buffer[1];
           peR_lim   = buffer[3];
       }
       else
       {
           peR_step  = 0;
           peR_lim   = 0;
       }

       if (step2 != 0.0)
       {
           if (! validrtvar(vmult2))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 2;                        // destination  PE
           buffer[1] = (int)(step2*(gC->GMAX_TO_DAC));
           buffer[2] = vmult2;
           buffer[3] = (int) (lim2+0.5);
           gC->outputACode(SETPEVALUE,4,buffer);
           peP_step  = buffer[1];
           peP_lim   = buffer[3];
       }
       else
       {
           peP_step  = 0;
           peP_lim   = 0;
       }

       if (step3 != 0.0)
       {
           if (! validrtvar(vmult3))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 3;                       // destination  SS
           buffer[1] = (int)(step3*(gC->GMAX_TO_DAC));
           buffer[2] = vmult3;
           buffer[3] = (int) (lim3+0.5);
           gC->outputACode(SETPEVALUE,4,buffer);
           peS_step  = buffer[1];
           peS_lim   = buffer[3];
       }
       else
       {
           peS_step  = 0;
           peS_lim   = 0;
       }

   /* figure out which logical gradients are active? */
   int tempType=0;


    /* ACode
       OBLPEGRD  (4 args)
       Type Info
       stat1
       stat2
       stat3
     */

   buffer[0] = tempType;
   buffer[1] = (int)(stat1*gC->GMAX_TO_DAC);
   buffer[2] = (int)(stat2*gC->GMAX_TO_DAC);
   buffer[3] = (int)(stat3*gC->GMAX_TO_DAC);

   gC->oblpegrad_limit_check(buffer[1],buffer[2],buffer[3],  \
         peR_step,peP_step,peS_step, peR_lim,peP_lim,peS_lim);

   gC->outputACode(OBLPEGRD, 4, buffer);

   if (bgflag)
   {
      printf("OBLPEGRD\n");
      for(int i=0; i<4; i++)
         printf("%d  ",buffer[i]);
      printf("\n");
   }

  grad_flag = TRUE;
  return;
}

//
//
//
void phase_encode3_oblshapedgradient(char *pat1, char *pat2, char *pat3, double width, \
                                     double stat1, double stat2, double stat3,         \
                                     double step1, double step2, double step3,         \
                                     int vmult1, int vmult2, int vmult3,               \
                                     double lim1, double lim2, double lim3,            \
                                     int loops, int wait4me, int tag)
{

    if (gradtype[0] != gradtype[1])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");
    if (gradtype[0] != gradtype[2])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");

    int divs1=0, divs2=0, divs3=0, duration=0;
    long long ticks;
    int peR_step,peP_step,peS_step, peR_lim,peP_lim,peS_lim;

    GradientBase *gC = P2TheConsole->getConfiguredGradient();

    if (ix == 1) gC->errorCheck(0,stat1,stat2,stat3,step1,step2,step3);

    P2TheConsole->eventStartAction();

    if (gC->isNoWaitMode())
       abort_message("gradient event requested too soon after previous one in NOWAIT mode. abort!\n");

    cPatternEntry *tmp1=NULL, *tmp2=NULL, *tmp3=NULL;


   /* set the GRADSCALE here, for GRADIENT it is all 0x4000; for Triax PFG they may be different */

   gC->setGradScale("X",gC->GXSCALE);
   gC->setGradScale("Y",gC->GYSCALE);
   gC->setGradScale("Z",gC->GZSCALE);

    int buffer[9];


       if (step1 != 0.0)
       {
           if (! validrtvar(vmult1))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 1;                        // destination  RO
           buffer[1] = (int)(step1*(gC->GMAX_TO_DAC));
           buffer[2] = vmult1;
           buffer[3] = (int) (lim1+0.5);
           gC->outputACode(SETPEVALUE,4,buffer);
           peR_step  = buffer[1];
           peR_lim   = buffer[3];
       }
       else
       {
           peR_step  = 0;
           peR_lim   = 0;
       }

       if (step2 != 0.0)
       {
           if (! validrtvar(vmult2))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 2;                        // destination  PE
           buffer[1] = (int)(step2*(gC->GMAX_TO_DAC));
           buffer[2] = vmult2;
           buffer[3] = (int) (lim2+0.5);
           gC->outputACode(SETPEVALUE,4,buffer);
           peP_step  = buffer[1];
           peP_lim   = buffer[3];
       }
       else
       {
           peP_step  = 0;
           peP_lim   = 0;
       }

       if (step3 != 0.0)
       {
           if (! validrtvar(vmult3))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 3;                       // destination  SS
           buffer[1] = (int)(step3*(gC->GMAX_TO_DAC));
           buffer[2] = vmult3;
           buffer[3] = (int) (lim3+0.5);
           gC->outputACode(SETPEVALUE,4,buffer);
           peS_step  = buffer[1];
           peS_lim   = buffer[3];
       }
       else
       {
           peS_step  = 0;
           peS_lim   = 0;
       }

   /* figure out which logical gradients are active? */
   int tempType=0;

   char gradPwrId[512], swidth[256];
   OSTRCPY( gradPwrId, sizeof(gradPwrId), "OblPEShp ");

   // if shape name blank, that logical axis has no gradient

   if (strcmp(pat1,"") != 0)
   {
     tempType |= 0x1;
     OSTRCAT( gradPwrId, sizeof(gradPwrId), pat1);
     OSTRCAT( gradPwrId, sizeof(gradPwrId), " ");
   }

   if (strcmp(pat2,"") != 0)
   {
     tempType |= 0x2;
     OSTRCAT( gradPwrId, sizeof(gradPwrId), pat2);
     OSTRCAT( gradPwrId, sizeof(gradPwrId), " ");
   }

   if (strcmp(pat3,"") != 0)
   {
     tempType |= 0x4;
     OSTRCAT( gradPwrId, sizeof(gradPwrId), pat3);
     OSTRCAT( gradPwrId, sizeof(gradPwrId), " ");
   }
   OSPRINTF( swidth, sizeof(swidth), "%5.4f", width);
   OSTRCAT( gradPwrId, sizeof(gradPwrId), swidth);

    P2TheConsole->newEvent();
    gC->clearSmallTicker();
    if (wait4me)
      gC->setActive();
    else
      gC->setNOWAITMode();

    // axisCode=7 for oblique shaped gradient


    // Gradient Logical Axis READOUT (R)
    if (tempType & 0x1)
    {
      // OblShpGrd pattern has only 16 bits amp info; no KEYS or latch bit
      tmp1 = gC->resolveOblShpGrdPattern(pat1,7,"oblpeshapedgradient",1);
      divs1 = tmp1->getNumberStates();
      duration = gC->calcTicksPerState(width,divs1,4,0.1,"PE Shaped Gradient",0);
      if (duration < 320)   // 4 us is lower limit.
         abort_message("PFG Pattern duration too short. abort!\n");
    }


    if (tempType & 0x2)
    {
      // OblShpGrd pattern has only 16 bits amp info; no KEYS or latch bit
      tmp2 = gC->resolveOblShpGrdPattern(pat2,7,"oblpeshapedgradient",1);
      divs2 = tmp2->getNumberStates();
      if ( (divs1 != 0) && (divs1 != divs2) )
         abort_message("phase_encode3_oblshapedgradient: number of steps in waveforms must be the same. abort!\n");

      duration = gC->calcTicksPerState(width,divs2,4,0.1,"PE Shaped Gradient",0);
      if (duration < 320)   // 4 us is lower limit.
         abort_message("PFG Pattern duration too short. abort!\n");
      divs1 = divs2;
    }

    if (tempType & 0x4)
    {
      // OblShpGrd pattern has only 16 bits amp info; no KEYS or latch bit
      tmp3 = gC->resolveOblShpGrdPattern(pat3,7,"oblpeshapedgradient",1);
      divs3 = tmp3->getNumberStates();
      if ( (divs1 != 0) && (divs1 != divs3) )
         abort_message("phase_encode3_oblshapedgradient: number of steps in waveforms must be the same. abort!\n");

      duration = gC->calcTicksPerState(width,divs3,4,0.1,"PE Shaped Gradient",0);
      if (duration < 320)   // 4 us is lower limit.
         abort_message("PFG Pattern duration too short. abort!\n");
      divs1 = divs3;
    }


    /* ACode
       OBLPESHAPEDGRD  (9 args)
       Type Info
       patID 1
       pat1D 2
       patID 3
       stat1
       stat2
       stat3
       Duration Count
       Loops
     */

   buffer[0] = tempType;

   if (tempType & 0x1)
      buffer[1] = tmp1->getReferenceID();
   else
      buffer[1] = 0;

   if (tempType & 0x2)
      buffer[2] = tmp2->getReferenceID();
   else
      buffer[2] = 0;

   if (tempType & 0x4)
      buffer[3] = tmp3->getReferenceID();
   else
      buffer[3] = 0;

   buffer[4] = (int)(stat1*gC->GMAX_TO_DAC);
   buffer[5] = (int)(stat2*gC->GMAX_TO_DAC);
   buffer[6] = (int)(stat3*gC->GMAX_TO_DAC);

   buffer[7] = duration;                  // DURATION COUNT
   buffer[8] = 1;                         // LOOPS

   gC->oblpegrad_limit_check(buffer[4],buffer[5],buffer[6],  \
         peR_step,peP_step,peS_step, peR_lim,peP_lim,peS_lim);

   gC->computeOBLPESHPGradPower(START,tmp1,tmp2,tmp3,tempType,buffer[4],buffer[5],buffer[6], \
                             peR_step,peP_step,peS_step,vmult1,vmult2,vmult3,                \
                             peR_lim,peP_lim,peS_lim, 1);

   gC->outputACode(OBLPESHAPEDGRD, 9, buffer);

   if (bgflag)
   {
      printf("OBLPESHAPEDGRD\n");
      for(int i=0; i<9; i++)
         printf("%d  ",buffer[i]);
      printf("\n");
   }

   gC->noteDelay(duration*divs1);
   ticks = gC->getSmallTicker();
   if (wait4me)
      P2TheConsole->update4Sync(ticks);
   else
      gC->incr_NOWAIT_eventTicker(ticks);

   gC->computeOBLPESHPGradPower(END,tmp1,tmp2,tmp3,tempType,buffer[4],buffer[5],buffer[6], \
                             peR_step, peP_step, peS_step, peR_lim,peP_lim, peS_lim,       \
                             peR_lim, peP_lim, peS_lim, 1);

  int gradenergyaction = (int)(getvalnwarn("gradenergyaction")+0.49);
  if ( (gradenergyaction & 0x2) ) P2TheConsole->showEventPowerIntegral(gradPwrId);

  grad_flag = TRUE;
  return;
}


//
//
//
void d_phase_encode3_oblshapedgradient(char *pat1, char *pat2, char *pat3, char *pat4, char *pat5, char *pat6,          \
                                     double stat1, double stat2, double stat3, double stat4, double stat5, double stat6,     \
                                     double step1, double step2, double step3, double step4, double step5, double step6,     \
                                     int vmult1, int vmult2, int vmult3, int vmult4, int vmult5, int vmult6,                 \
                                     double lim1, double lim2, double lim3, double lim4, double lim5, double lim6,            \
                                     double width, int loops, int wait4me, int tag)
{

    if (gradtype[0] != gradtype[1])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");
    if (gradtype[0] != gradtype[2])
       abort_message("error in gradient or pfg configuration parameter gradtype. abort!\n");

    int divs1=0, divs2=0, divs3=0, duration=0;
    int divs4=0, divs5=0, divs6=0;
    long long ticks;
    int peR_step,peP_step,peS_step, peR_lim,peP_lim,peS_lim;
    int d_peR_step,d_peP_step,d_peS_step, d_peR_lim,d_peP_lim,d_peS_lim;

    GradientBase *gC = P2TheConsole->getConfiguredGradient();

    if (ix == 1) gC->errorCheck(0,stat1,stat2,stat3,step1,step2,step3);
    if (ix == 1) gC->errorCheck(0,stat4,stat5,stat6,step4,step5,step6);

    if (gC->isNoWaitMode())
       abort_message("gradient event requested too soon after previous one in NOWAIT mode. abort!\n");

    cPatternEntry *tmp1 = NULL, *tmp2 = NULL, *tmp3 = NULL;
    cPatternEntry *tmp4 = NULL, *tmp5 = NULL, *tmp6 = NULL;


   /* set the GRADSCALE here, for GRADIENT it is all 0x4000; for Triax PFG they may be different */

   gC->setGradScale("X",gC->GXSCALE);
   gC->setGradScale("Y",gC->GYSCALE);
   gC->setGradScale("Z",gC->GZSCALE);

    int buffer[20];

    // text_message("%f %f %f %f %f %f\n",step1,step2,step3,step4,step5,step6);

       if (step1 != 0.0)
       {
           if (! validrtvar(vmult1))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 1;                        // destination  RO
           buffer[1] = (int)(step1*(gC->GMAX_TO_DAC));
           buffer[2] = vmult1;
           buffer[3] = (int) (lim1+0.5);
	   // text_message("%d %d %d %d\n",buffer[0],buffer[1],buffer[2],buffer[3]);
           gC->outputACode(SETPEVALUE,4,buffer);
           peR_step  = buffer[1];
           peR_lim   = buffer[3];
       }
       else
       {
           peR_step  = 0;
           peR_lim   = 0;
       }

       if (step2 != 0.0)
       {
           if (! validrtvar(vmult2))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 2;                        // destination  PE
           buffer[1] = (int)(step2*(gC->GMAX_TO_DAC));
           buffer[2] = vmult2;
           buffer[3] = (int) (lim2+0.5);
	   // text_message("%d %d %d %d\n",buffer[0],buffer[1],buffer[2],buffer[3]);
           gC->outputACode(SETPEVALUE,4,buffer);
           peP_step  = buffer[1];
           peP_lim   = buffer[3];
       }
       else
       {
           peP_step  = 0;
           peP_lim   = 0;
       }

       if (step3 != 0.0)
       {
           if (! validrtvar(vmult3))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 3;                       // destination  SS
           buffer[1] = (int)(step3*(gC->GMAX_TO_DAC));
           buffer[2] = vmult3;
           buffer[3] = (int) (lim3+0.5);
	   // text_message("%d %d %d %d\n",buffer[0],buffer[1],buffer[2],buffer[3]);
           gC->outputACode(SETPEVALUE,4,buffer);
           peS_step  = buffer[1];
           peS_lim   = buffer[3];
       }
       else
       {
           peS_step  = 0;
           peS_lim   = 0;
       }

       if (step4 != 0.0)
       {
           if (! validrtvar(vmult4))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 4;                        // destination  d_RO
           buffer[1] = (int)(step4*(gC->GMAX_TO_DAC));
           buffer[2] = vmult4;
           buffer[3] = (int) (lim4+0.5);
	   // text_message("%d %d %d %d\n",buffer[0],buffer[1],buffer[2],buffer[3]);
           gC->outputACode(SETPEVALUE,4,buffer);
           d_peR_step  = buffer[1];
           d_peR_lim   = buffer[3];
       }
       else
       {
           d_peR_step  = 0;
           d_peR_lim   = 0;
       }

       if (step5 != 0.0)
       {
           if (! validrtvar(vmult5))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 5;                        // destination  d_PE
           buffer[1] = (int)(step5*(gC->GMAX_TO_DAC));
           buffer[2] = vmult5;
           buffer[3] = (int) (lim5+0.5);
	   // text_message("%d %d %d %d\n",buffer[0],buffer[1],buffer[2],buffer[3]);
           gC->outputACode(SETPEVALUE,4,buffer);
           d_peP_step  = buffer[1];
           d_peP_lim   = buffer[3];
       }
       else
       {
           d_peP_step  = 0;
           d_peP_lim   = 0;
       }

       if (step6 != 0.0)
       {
           if (! validrtvar(vmult6))
              abort_message("invalid real time variable specified for phase encode multiplier. abort!\n");
           // send SETPEVALUE
           buffer[0] = 6;                       // destination  SS
           buffer[1] = (int)(step6*(gC->GMAX_TO_DAC));
           buffer[2] = vmult6;
           buffer[3] = (int) (lim6+0.5);
	   // text_message("%d %d %d %d\n",buffer[0],buffer[1],buffer[2],buffer[3]);
           gC->outputACode(SETPEVALUE,4,buffer);
           d_peS_step  = buffer[1];
           d_peS_lim   = buffer[3];
       }
       else
       {
           d_peS_step  = 0;
           d_peS_lim   = 0;
       }

   /* figure out which logical gradients are active? */
   int tempType=0;

   // if shape name blank, that logical axis has no gradient

   if (strcmp(pat1,"") != 0)
     tempType |= 0x1;

   if (strcmp(pat2,"") != 0)
     tempType |= 0x2;

   if (strcmp(pat3,"") != 0)
     tempType |= 0x4;

   if (strcmp(pat4,"") != 0)
     tempType |= 0x8;

   if (strcmp(pat5,"") != 0)
     tempType |= 0x10;

   if (strcmp(pat6,"") != 0)
     tempType |= 0x20;


    P2TheConsole->newEvent();
    gC->clearSmallTicker();
    if (wait4me)
      gC->setActive();
    else
      gC->setNOWAITMode();

    // axisCode=7 for oblique shaped gradient


    // Gradient Logical Axis READOUT (R)
    if (tempType & 0x1)
    {
      // OblShpGrd pattern has only 16 bits amp info; no KEYS or latch bit
      tmp1 = gC->resolveOblShpGrdPattern(pat1,7,"oblpeshapedgradient",1);
      divs1 = tmp1->getNumberStates();
      duration = gC->calcTicksPerState(width,divs1,4,0.1,"PE Shaped Gradient",0);
      if (duration < 320)   // 4 us is lower limit.
         abort_message("PFG Pattern duration too short. abort!\n");
    }


    if (tempType & 0x2)
    {
      // OblShpGrd pattern has only 16 bits amp info; no KEYS or latch bit
      tmp2 = gC->resolveOblShpGrdPattern(pat2,7,"oblpeshapedgradient",1);
      divs2 = tmp2->getNumberStates();
      if ( (divs1 != 0) && (divs1 != divs2) )
         abort_message("phase_encode3_oblshapedgradient: number of steps in waveforms must be the same. abort!\n");

      duration = gC->calcTicksPerState(width,divs2,4,0.1,"PE Shaped Gradient",0);
      if (duration < 320)   // 4 us is lower limit.
         abort_message("PFG Pattern duration too short. abort!\n");
      divs1 = divs2;
    }

    if (tempType & 0x4)
    {
      // OblShpGrd pattern has only 16 bits amp info; no KEYS or latch bit
      tmp3 = gC->resolveOblShpGrdPattern(pat3,7,"oblpeshapedgradient",1);
      divs3 = tmp3->getNumberStates();
      if ( (divs1 != 0) && (divs1 != divs3) )
         abort_message("phase_encode3_oblshapedgradient: number of steps in waveforms must be the same. abort!\n");

      duration = gC->calcTicksPerState(width,divs3,4,0.1,"PE Shaped Gradient",0);
      if (duration < 320)   // 4 us is lower limit.
         abort_message("PFG Pattern duration too short. abort!\n");
      divs1 = divs3;
    }

    // Gradient Logical Axis d_READOUT (R)
    if (tempType & 0x8)
    {
      // OblShpGrd pattern has only 16 bits amp info; no KEYS or latch bit
      tmp4 = gC->resolveOblShpGrdPattern(pat4,7,"oblpeshapedgradient",1);
      divs4 = tmp4->getNumberStates();
      duration = gC->calcTicksPerState(width,divs4,4,0.1,"PE Shaped Gradient",0);
      if (duration < 320)   // 4 us is lower limit.
         abort_message("PFG Pattern duration too short. abort!\n");
    }

    // Gradient Logical Axis d_PHASE (P)
    if (tempType & 0x10)
    {
      // OblShpGrd pattern has only 16 bits amp info; no KEYS or latch bit
      tmp5 = gC->resolveOblShpGrdPattern(pat5,7,"oblpeshapedgradient",1);
      divs5 = tmp5->getNumberStates();
      duration = gC->calcTicksPerState(width,divs5,4,0.1,"PE Shaped Gradient",0);
      if (duration < 320)   // 4 us is lower limit.
         abort_message("PFG Pattern duration too short. abort!\n");
    }

    // Gradient Logical Axis d_SLICE (S)
    if (tempType & 0x20)
    {
      // OblShpGrd pattern has only 16 bits amp info; no KEYS or latch bit
      tmp6 = gC->resolveOblShpGrdPattern(pat6,7,"oblpeshapedgradient",1);
      divs6 = tmp6->getNumberStates();
      duration = gC->calcTicksPerState(width,divs6,4,0.1,"PE Shaped Gradient",0);
      if (duration < 320)   // 4 us is lower limit.
         abort_message("PFG Pattern duration too short. abort!\n");
    }

    /* ACode
       OBLPESHAPEDGRD  (15 args)
       Type Info
       patID 1
       pat1D 2
       patID 3
       stat1
       stat2
       stat3
       Duration Count
       Loops
       patID 4
       pat1D 5
       patID 6
       stat4
       stat5
       stat6
     */

   buffer[0] = tempType;

   if (tempType & 0x1)
      buffer[1] = tmp1->getReferenceID();
   else
      buffer[1] = 0;

   if (tempType & 0x2)
      buffer[2] = tmp2->getReferenceID();
   else
      buffer[2] = 0;

   if (tempType & 0x4)
      buffer[3] = tmp3->getReferenceID();
   else
      buffer[3] = 0;

   buffer[4] = (int)(stat1*gC->GMAX_TO_DAC);
   buffer[5] = (int)(stat2*gC->GMAX_TO_DAC);
   buffer[6] = (int)(stat3*gC->GMAX_TO_DAC);

   buffer[7] = duration;                  // DURATION COUNT
   buffer[8] = 1;                         // LOOPS

   if (tempType & 0x8)
      buffer[9] = tmp4->getReferenceID();
   else
      buffer[9] = 0;

   if (tempType & 0x10)
      buffer[10] = tmp5->getReferenceID();
   else
      buffer[10] = 0;

   if (tempType & 0x20)
      buffer[11] = tmp6->getReferenceID();
   else
      buffer[11] = 0;

   buffer[12] = (int)(stat4*gC->GMAX_TO_DAC);
   buffer[13] = (int)(stat5*gC->GMAX_TO_DAC);
   buffer[14] = (int)(stat6*gC->GMAX_TO_DAC);

   gC->oblpegrad_limit_check(buffer[4],buffer[5],buffer[6],peR_step,peP_step,peS_step, peR_lim,peP_lim,peS_lim);
//   gC->oblpegrad_limit_check(buffer[12],buffer[13],buffer[14],d_peR_step,d_peP_step,d_peS_step, d_peR_lim,d_peP_lim,d_peS_lim);

   gC->outputACode(OBLPESHAPEDGRD, 15, buffer);

   if (bgflag)
   {
      printf("OBLPESHAPEDGRD\n");
      for(int i=0; i<15; i++)
         printf("%d  ",buffer[i]);
      printf("\n");
   }

   gC->noteDelay(duration*divs1);
   ticks = gC->getSmallTicker();
   if (wait4me)
      P2TheConsole->update4Sync(ticks);
   else
      gC->incr_NOWAIT_eventTicker(ticks);

  grad_flag = TRUE;
  return;
}




void phase_encode3_gradpulse(double stat1, double stat2, double stat3,              \
     double duration, double step1, double step2, double step3,                     \
     int vmult1, int vmult2, int vmult3, double lim1, double lim2, double lim3)
{

     if (duration >= 2.40e-6)
     {
       phase_encode3_gradient(stat1,stat2,stat3,step1,step2,step3,vmult1,vmult2,vmult3,lim1,lim2,lim3);
       delay(duration);
       zero_all_gradients();
     }
}


//
//
//
void settmpgradtype(char *tmpgradname)
{
   double pfgBoard;
   char tmpGradtype[MAXSTR];

   if (P_getstring(CURRENT, tmpgradname, tmpGradtype, 1, MAXSTR) == 0)
   {
      if (strlen(tmpGradtype) >= 3)
      {
         P_setstring(GLOBAL,"gradtype",tmpGradtype,1);

//       see checkGradtype() in psgmain.cpp
         specialGradtype = 0;
         if ( P_getreal(GLOBAL,"pfg1board",&pfgBoard,1) == 0)
         {
            if (pfgBoard > 0.5)
            {
               OSTRCPY( tmpGradtype, sizeof(tmpGradtype), "   ");
               P_getstring(GLOBAL,"gradtype",tmpGradtype,1,MAXSTR-1);
               if (tmpGradtype[2] == 'a')
               {
                  specialGradtype = 'a';
               }
               else if (tmpGradtype[2] == 'h')
               {
                  specialGradtype = 'h';
               }
               if (specialGradtype)
               {
                  tmpGradtype[2] = 'p';
                  P_setstring(GLOBAL,"gradtype",tmpGradtype,1);
               }
            }
         }
      }
   }
}


//
//
//
void rgradientCodes(char axis, double value)
{
  PFGController *tmpPFG;
  GradientController *tmpGrad;
  MasterController *tmpMaster;
  char lowercase;
  int index,extra;

  index = ((int)(axis | 0x20)) - 'x';
  if (specialGradtype && (index == 2) )
  {
     lowercase = specialGradtype;
  }
  else
  {
     lowercase = gradtype[index] | 0x20;
  }
  if (axis == 'b')
  {
    if (gradtype[2] == 'r')
      lowercase = 'r';
    else
      return;
  }
  switch (lowercase)
  {
  case 'a':
       P2TheConsole->newEvent();
       tmpMaster=(MasterController*)P2TheConsole->getControllerByID("master1");
       tmpMaster->setActive();
       // track the duration using the small ticker...
       tmpMaster->clearSmallTicker();
       tmpMaster->noteDelay(2000);   // 25 usec is hardcoded in several places
       tmpMaster->shimGradient(axis,value);
       extra = tmpMaster->getSmallTicker();
       P2TheConsole->update4Sync(extra);
       break;
  case 'h':
       tmpMaster=(MasterController*)P2TheConsole->getControllerByID("master1");
       tmpMaster->homospoilGradient(axis,value);
       break;
       // only a few of these are supported...
  case 'c':	// Performa IV
  case 'l':	// Performa I
  case 'p':	// Performa II/III
  case 'q':	// Performa II/III + WFG
  case 'd':	// Performa IV     + WFG
  case 't':	// Performa XYZ
  case 'u':	// Performa XYZ    + WFG
       tmpPFG = (PFGController *) P2TheConsole->getControllerByID("pfg1");
       if (tmpPFG->isNoWaitMode())
          abort_message("gradient event requested too soon after previous one in NOWAIT mode. abort!\n");

       tmpPFG->setGradScale(&axis, 0x4000);  // set GRADSCALE to max value
       tmpPFG->setGrad(&axis,value);
       if (fabs(value) > 0.0)
          tmpPFG->setChannelActive(1);
       break;
  case 'r':	// Imaging case  + CRB
  case 'w':	// imaging case..
       tmpGrad = (GradientController *)P2TheConsole->getControllerByID("grad1");
       if (tmpGrad->isNoWaitMode())
          abort_message("gradient event requested too soon after previous one in NOWAIT mode. abort!\n");

       tmpGrad->setGrad(&axis,value);
       if (fabs(value) > 0.0)
          tmpGrad->setChannelActive(1);
       break;
  case 'n':
  default:
       cout << "Parameter gradtype = '" << gradtype << "', run config to change" << endl;
       break;
  }

  grad_flag = TRUE;
  return;
}

void rgradient(char axis, double value)
{
   if (value == 0.0)
      rgradientCodes(axis, value);
   else if (gradalt == 1.0)
      rgradientCodes(axis, value);
   else
   {
      ifmod2zero(ct);
         rgradientCodes(axis, value);
      elsenz(ct);
         rgradientCodes(axis, gradalt * value);
      endif(ct);
   }
}

void zero_all_gradients()
{
   GradientBase *gC = P2TheConsole->getConfiguredGradient();
   if ( (gC != NULL) && gC->isNoWaitMode())
      abort_message("gradient event requested too soon after previous one in NOWAIT mode. abort!\n");

   if ( (gradtype[0] | 0x20) != 'n') rgradient('x',0.0);
   if ( (gradtype[1] | 0x20) != 'n') rgradient('y',0.0);
   if ( (gradtype[2] | 0x20) != 'n') rgradient('z',0.0);
   if (!strcmp(gradtype,"rrr") || strcmp(gradtype,"RRR")) rgradient('b',0.0);
   delay(0.0000040); // NECESSARY TO PREVENT GRADIENT OVERRUN
}

void vgradient(char axis, double value, int rtvar)
{
   PFGController *tmp;
   tmp = (PFGController *) P2TheConsole->getControllerByID("pfg1");
   tmp->setVGrad(&axis,value*(tmp->GMAX_TO_DAC),rtvar);
   grad_flag = TRUE;
   return;
}

/*  functions that enable gradient amps,
    etc are in the controller object.
*/

/* a single axis shaped gradient - no other gradient activity
   allowed within that event.  Best for single axis only
   configurations.
*/

//
//
//
void shapedgradient(char *name,double width,double amp,char which,
     int loops,int wait4me)
{
   cPatternEntry *tmp;
   int axisCode = 0;
   long long extra;
   double value;
   char buffer[4];
   int divs, duration;

   buffer[0] = which;
   buffer[1] = '\0';
   value = width;
   switch (buffer[0])
   {
     case 'x': case 'X': axisCode = 1; break;
     case 'y': case 'Y': axisCode = 2; break;
     case 'z': case 'Z': axisCode = 3; break;
     default:
       abort_message("invalid gradient axis in shapedgradient. abort!\n");
   }

   GradientBase *gC = P2TheConsole->getConfiguredGradient();

   if (gC->isNoWaitMode())
      abort_message("gradient event requested too soon after previous one in NOWAIT mode. abort!\n");

   tmp = gC->resolveOblShpGrdPattern(name,axisCode,"shapedgradient",1);

   P2TheConsole->newEvent();
   gC->clearSmallTicker();
   if (wait4me)
     gC->setActive();
   else
     gC->setNOWAITMode();

   divs = tmp->getNumberStates();
   duration = gC->calcTicksPerState(value,divs,4,0.1,"Shaped Gradient",0);
   if (duration < 320)   // 4 us is lower limit.
      abort_message("gradient duration too short for shapedgradient. abort!\n");


   // This should work for all imaging, micro imaging & Z axis and Triax PFG
   gC->setGradScale("X",0x4000);
   gC->setGradScale("Y",0x4000);
   gC->setGradScale("Z",0x4000);

   int fifobuffer[5];
   fifobuffer[0] = tmp->getReferenceID();

   if (axisCode == 1)
   {
       fifobuffer[1] = XGRADKEY;
   }
   else if (axisCode == 2)
   {
       fifobuffer[1] = YGRADKEY;
   }
   else
   {
       fifobuffer[1] = ZGRADKEY;
   }

   int daclimit = gC->getDACLimit();

   if (daclimit == 0x7ff)
   {
     if ( (amp > 2047.5) || (amp < -2047.5 ) )
       abort_message("gradient amplitude value %g is outside valid dac range in shapedgradient. abort!\n", amp);
     amp *= 16.0;
   }

   int dacvalue = ((int) floor(amp));

   if (dacvalue < -32767)
   {
     warn_message("advisory: gradient amplitude value %d outside valid dac range clipped to -32767\n",dacvalue);
     dacvalue = -32767;
   }
   else if (dacvalue > 32767)
   {
     warn_message("advisory: gradient amplitude value +%d outside valid dac range clipped to +32767\n",dacvalue);
     dacvalue = 32767;
   }

   fifobuffer[2] = dacvalue;
   fifobuffer[3] = duration;
   fifobuffer[4] = 1;           // Loops
   gC->outputACode(SHAPEDGRD, 5, fifobuffer);

   gC->noteDelay(duration*divs);            // maybe plus 1..
   extra = gC->getSmallTicker();
   if (wait4me)
      P2TheConsole->update4Sync(extra);
   else
      gC->incr_NOWAIT_eventTicker(extra);

   grad_flag = TRUE;
   return;
}

//
//
//
void oblique_gradpulse(double level1,double level2,double level3,double ang1,double ang2,double ang3,double width)
{

}

/* this routine puts out acodes from initializepState()   */
/* no time  synchronization done                            */
void putEccAmpsNTimes(int n, int amps[], long long times[])
{
   int *p2times,swap,i;
   GradientController *tmp;
// cout << "putEccs: n=" << n << endl;
   tmp = (GradientController *) P2TheConsole->getControllerByID("grad1");
   tmp->outputACode(ECC_AMPS, n, amps);
   // 64 bit swap for LINUX
#ifdef LINUX
   p2times = (int *) times;
   for (i=0; i < n; i++)
   {
      swap = *p2times;
      *p2times = *(p2times+1);
      *(p2times+1) = swap;
      p2times += 2;
   }
#endif
   tmp->outputACode(ECC_TIMES, 2*n, (int *)times);
}

/* this routine puts out acodes from initializeExpState()   */
/* no time  synchronization done                            */
void putSdacScaleNLimits(int n, int scaleNlimits[])
{
   GradientController *tmp;
//  cout << "putSdacs: n=" << n << endl;
   tmp = (GradientController *) P2TheConsole->getControllerByID("grad1");
   tmp->outputACode(SDAC_VALUES, n, scaleNlimits);
}

void putDutyLimits(int n,int dutyLimits[])
{
   MasterController *tmp;
//  cout << "putDutyCyle: n=" << n << endl;
   tmp = (MasterController *) P2TheConsole->getControllerByID("master1");
   tmp->outputACode(DUTYCYCLE_VALUES, n, dutyLimits);
}

void setMRIUserGates(int rtvar)
{
int  codes[3];
GradientController *tmp;
   tmp = (GradientController *) P2TheConsole->getControllerByID("grad1");
   codes[0] = rtvar;
   tmp->outputACode(MRIUSERGATES, 1, codes);
}

// experimental..
CEXTERN int grad_advance(double dur)
{
   if(dur<=0)
       return(1);
   long long count;
//if(readuserbyte==1){ //SAS if using fast RUB you need to do this
  if(1==1){
   P2TheConsole->newEvent();
   MasterController *tmp2 = (MasterController *) P2TheConsole->getControllerByID("master1");
   tmp2->setActive();
   tmp2->setNoDelay(0.0002);
   count = tmp2->getSmallTicker();
   P2TheConsole->update4Sync(count); // delays everyone else
}
//   cout << "gradient advance of dur " << dur << endl;
   GradientController *tmp = (GradientController *) P2TheConsole->getControllerByID("grad1");
   P2TheConsole->newEvent();
   tmp->setActive();
   tmp->setNoDelay(dur);
   count = tmp->getSmallTicker();
//if(readuserbyte==1){ //SAS if using fast RUB you need to do this
  if(1==1){
   MasterController *tmp2 = (MasterController *) P2TheConsole->getControllerByID("master1");
   tmp2->setActive();
   tmp2->setNoDelay(dur);
   count = tmp2->getSmallTicker();
}
   P2TheConsole->update4Sync(count); // delays everyone else
   return(0);
}


void getgradpowerintegral(double *powerarray)
{
	GradientBase *gC = P2TheConsole->getConfiguredGradient();
	gC->getPowerIntegral(powerarray);
}

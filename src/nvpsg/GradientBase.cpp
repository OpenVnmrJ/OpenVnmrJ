/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* GradientBase.cpp */

#include <math.h>
#include <string.h>
#include "GradientBase.h"
#include "FFKEYS.h"
#include "ACode32.h"
#include "cpsg.h"
#include "acqparms.h"

extern "C" {

#include "safestring.h"

}

//
#define WriteWord( x ) pAcodeBuf->putCode( x )
#define putPattern( x ) pWaveformBuf->putCode( x )
#define MAXSTR 256
#define START   0
#define END     1


extern int bgflag;
extern int dps_flag;
extern double dps_fval[];
extern int gxFlip;
extern int gyFlip;
extern int gzFlip;


//
// because the scaling is done before rotation, the 
// scale registers are not utilized with patterns etc.
// the application software is oblique ...
//
int GradientBase::userToScale(double xx)
{
  printf("called GradientBase::user2Scale\n");
  return(0);
}

int GradientBase::ampTo16Bits(double xx)
{
  printf("called GradientBase::ampTo16Bits\n");
  return(0);
}
 
void GradientBase::setGates(int GatePattern)
{
  printf("called GradientBase::setGates\n");
}

void GradientBase::setGrad(const char *which, double value)
{
  printf("called GradientBase::setGrad\n");
}

void GradientBase::setGradScale(const char *which, double value)
{
  printf("called GradientBase::setGradScale\n");
}

void GradientBase::setVGrad(char *which, double step, int rtvar)
{
  printf("called GradientBase::setVGrad\n");
}

int GradientBase::errorCheck(int checkType, double g1, double g2, double g3, double s1, double s2, double s3)
{
  printf("called GradientBase::errorCheck\n");
  return (1);
}

void GradientBase::setRotArrayElement(int index1, int index2, double value)
{
  printf("called GradientBase::setRotArrayElement\n");
}
  
  
cPatternEntry *GradientBase::resolveGrad1Pattern(char *nm, int flag, char *emsg, int action)
{
  printf("called GradientBase::resolveGrad1Pattern\n");
  return(NULL);
}

cPatternEntry *GradientBase::resolveOblShpGrdPattern(char *nm, int flag, const char *emsg, int action)
{
  printf("called GradientBase::resolveOblShpGrdPattern\n");
  return(NULL);
}

void GradientBase::setEnable(char *cmd)
{
  printf("called GradientBase::setEnable\n");
}


int GradientBase::initializeExpStates(int setupflag)
{
  printf("called GradientBase::initializeExpStates\n");
  return 0;
}
//
//
//
int GradientBase::initializeIncrementStates(int num)
{ 
  printf("called GradientBase::initializeIncrementStates\n");
  return(0);
}
//
//
//
int GradientBase::getDACLimit()
{
  printf("called GradientBase::getDACLimit\n");
  return(0);
}
//
//
//
void GradientBase::set_rotation_matrix(double ang1, double ang2, double ang3)
{
   double m11,m12,m13,m21,m22,m23,m31,m32,m33;

   calc_obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);

   //printf("set_rotation_matrix:\n ang1=%g  ang2=%g  ang3=%g\n",ang1,ang2,ang3);
   //printf("m11=%g m12=%g m13=%g m23=%g m22=%g m23=%g m31=%g m23=%g m33=%g\n",m11,m12,m13,m21,m22,m23,m31,m32,m33);
   // gxFlip, gyFlip & gzFlip is used to negate the gradients depending on the wiring diffrences

   int buffer[9];
   buffer[0] = gxFlip * (int)(m11*65536.0); buffer[1] = gxFlip * (int)(m12*65536.0); buffer[2] = gxFlip * (int)(m13*65536.0);
   buffer[3] = gyFlip * (int)(m21*65536.0); buffer[4] = gyFlip * (int)(m22*65536.0); buffer[5] = gyFlip * (int)(m23*65536.0);
   buffer[6] = gzFlip * (int)(m31*65536.0); buffer[7] = gzFlip * (int)(m32*65536.0); buffer[8] = gzFlip * (int)(m33*65536.0);

   outputACode(SETGRDROTATION, 9, buffer);

   /* set rotm_ values for oblpegrad limit checks */
   rotm_11 = buffer[0]; rotm_12 = buffer[1]; rotm_13 = buffer[2];
   rotm_21 = buffer[3]; rotm_22 = buffer[4]; rotm_23 = buffer[5];
   rotm_31 = buffer[6]; rotm_32 = buffer[7]; rotm_33 = buffer[8];
}

//
//
//
void GradientBase::calc_obl_matrix(double ang1,double ang2,double ang3,double *tm11,double *tm12,double *tm13,   \
                   double *tm21,double *tm22,double *tm23,double *tm31,double *tm32,double *tm33)
{
    double D_R;
    double sinang1,cosang1,sinang2,cosang2,sinang3,cosang3;
    double m11,m12,m13,m21,m22,m23,m31,m32,m33;
    double im11,im12,im13,im21,im22,im23,im31,im32,im33;
    double tol = 1.0e-14;

    /* Convert the input to the basic mag_log matrix */
    D_R = M_PI / 180;

    cosang1 = cos(D_R*ang1);
    sinang1 = sin(D_R*ang1);
       
    cosang2 = cos(D_R*ang2);
    sinang2 = sin(D_R*ang2);
       
    cosang3 = cos(D_R*ang3);
    sinang3 = sin(D_R*ang3);

    m11 = (sinang2*cosang1 - cosang2*cosang3*sinang1);
    m12 = (-1.0*sinang2*sinang1 - cosang2*cosang3*cosang1);
    m13 = (sinang3*cosang2);

    m21 = (-1.0*cosang2*cosang1 - sinang2*cosang3*sinang1);
    m22 = (cosang2*sinang1 - sinang2*cosang3*cosang1);
    m23 = (sinang3*sinang2);

    m31 = (sinang1*sinang3);
    m32 = (cosang1*sinang3);
    m33 = (cosang3);

    if (fabs(m11) < tol) m11 = 0;
    if (fabs(m12) < tol) m12 = 0;
    if (fabs(m13) < tol) m13 = 0;
    if (fabs(m21) < tol) m21 = 0;
    if (fabs(m22) < tol) m22 = 0;
    if (fabs(m23) < tol) m23 = 0;
    if (fabs(m31) < tol) m31 = 0;
    if (fabs(m32) < tol) m32 = 0;
    if (fabs(m33) < tol) m33 = 0;

    /* Generate the transform matrix for mag_log ******************/

    /*HEAD SUPINE*/
        im11 = m11;       im12 = m12;       im13 = m13;
        im21 = m21;       im22 = m22;       im23 = m23;
        im31 = m31;       im32 = m32;       im33 = m33;

    /*Transpose intermediate matrix and return***********/
    *tm11 = im11;     *tm21 = im12;     *tm31 = im13;
    *tm12 = im21;     *tm22 = im22;     *tm32 = im23;
    *tm13 = im31;     *tm23 = im32;     *tm33 = im33;
}

//
//
//
int GradientBase::create_rotation_list(char *nm, double* angle_set, int num_sets)
{
   double m11,m12,m13,m21,m22,m23,m31,m32,m33;
   double ang1,ang2,ang3, *pAngle_Set;
   int listId = -1, nang = 3;

   if (num_sets <= 0)
      abort_message("invalid number of coordinate rotation angle sets in create_rotation_list command. abort!\n");
   if (num_sets > 262144)   /* 64K rotation angle sets */
      abort_message("number of gradient rotation angle sets in create_rotation_list command too large. abort!\n");
  
   char listName[MAXSTR];
   OSTRCPY( listName, sizeof(listName), nm);
   OSTRCAT( listName, sizeof(listName), "_vrot");

   cPatternEntry *tmp = find(listName, 0);
   if (tmp !=  NULL)
   {
     listId = tmp->getReferenceID();
     if ( (listId >= 1) && (listId < 0x7FFFFF) )
       return listId;
     else
       abort_message("unable to determine the coordinate rotation list id. abort!\n");
   }

   int buf_size = num_sets*9;    /* 3x3 rot elements per rotation angle set */

   int *pBuffer = new int[buf_size];
   if (pBuffer == NULL)
      abort_message("unable to allocate memory for rotation list array. abort!\n");
   int ind = 0;

   pAngle_Set = angle_set;

   for (int i=0; i< num_sets; i++)
   {
      /* ang1 = angle_set[i][0]; ang2 = angle_set[i][1]; ang3 = angle_set[i][2]; */

      ang1 = *(pAngle_Set+(i*nang)+0);
      ang2 = *(pAngle_Set+(i*nang)+1);
      ang3 = *(pAngle_Set+(i*nang)+2);

      calc_obl_matrix(ang1,ang2,ang3,&m11,&m12,&m13,&m21,&m22,&m23,&m31,&m32,&m33);

     *(pBuffer+ind++) = gxFlip * (int)(m11*65536.0);
     *(pBuffer+ind++) = gxFlip * (int)(m12*65536.0);
     *(pBuffer+ind++) = gxFlip * (int)(m13*65536.0);

     *(pBuffer+ind++) = gyFlip * (int)(m21*65536.0);
     *(pBuffer+ind++) = gyFlip * (int)(m22*65536.0);
     *(pBuffer+ind++) = gyFlip * (int)(m23*65536.0);

     *(pBuffer+ind++) = gzFlip * (int)(m31*65536.0);
     *(pBuffer+ind++) = gzFlip * (int)(m32*65536.0);
     *(pBuffer+ind++) = gzFlip * (int)(m33*65536.0);
   }

   if (ind != buf_size)
      abort_message("error in creating rotation list. abort!\n");

   /* writing the list of rotation matrix elements as a waveform and adding to pattern file */

   pWaveformBuf->startSubSection(PATTERNHEADER);
   pWaveformBuf->putCodes(pBuffer, ind);
   tmp = new cPatternEntry(listName,0,num_sets,(num_sets*9));
   if (tmp != NULL)
   {
      addPattern(tmp);
      sendPattern(tmp);
      listId = tmp->getReferenceID();
      if ( (listId >= 1) && (listId < 0x7FFFFF) )
        return listId;
      else
        abort_message("unable to determine the coordinate rotation list id. abort!\n");
   }
   else
      abort_message("error in creating rotation list. abort!\n");
   return(listId);
}

//
//
//
void GradientBase::set_angle_list(int angleList, char *angleName, char mode, int rtvar)
{
   /* check for correct mode */
   if (mode != 'c')
     abort_message("invalid mode %c specified in set_angle_list command. abort!\n",mode);

   /* is rtvar legal ? */
   if (! validrtvar(rtvar))
     abort_message("invalid real time variable specified in set_angle_list command. abort!\n");

   /* is angleList defined? */
   cPatternEntry *tmp = findByID(angleList);
   if (tmp == NULL)
     abort_message("invalid list id specified for angle list in set_angle_list command. abort!\n");
  
   int buffer[3];
   buffer[0] = angleList;
   
   if (strcmp(angleName, "psi") == 0)
      buffer[1] = 0;
   else if (strcmp(angleName, "phi") == 0)
      buffer[1] = 1;
   else if (strcmp(angleName, "theta") == 0)
      buffer[1] = 2;
   else
      abort_message("invalid gradient rotation angle %s specified in set_angle_list command. abort!\n",angleName);
  
   buffer[2] = rtvar;

   outputACode(SETVGRDANGLIST, 3, buffer);
}

//
//
//
int GradientBase::create_angle_list(char *name, double *angle_set, int num_sets)
{
   double ang, *pAngle_Set;
   int listId = -1;

   if (num_sets <= 0)
      abort_message("invalid number of coordinate rotation angle sets in create_rotation_list command. abort!\n");
   if (num_sets > 262144)   /* 64K rotation angle sets */
      abort_message("number of gradient rotation angle sets in create_rotation_list command too large. abort!\n");
 
   char listName[MAXSTR];
   /* CHECK the size of name string */
   OSTRCPY( listName, sizeof(listName), name);
   OSTRCAT( listName, sizeof(listName), "_vangle");

   cPatternEntry *tmp = find(listName, 0);
   if (tmp !=  NULL)
   {
     listId = tmp->getReferenceID();
     if ( (listId >= 1) && (listId < 0x7FFFFF) )
       return listId;
     else
       abort_message("unable to determine the coordinate rotation list id. abort!\n");
   }

   int *pBuffer = new int[num_sets];
   if (pBuffer == NULL)
      abort_message("unable to allocate memory for rotation list array. abort!\n");

   int ind = 0;
   pAngle_Set = angle_set;

   for (int i=0; i< num_sets; i++)
   {
      ang = *(pAngle_Set + i);
      /* CHECK the range of ang to be in 0-360 */
      *(pBuffer+ind++) = (int)(ang*100.0+0.49);  /* angles are kept with 0.01 deg resolution */
   }

   if (ind != num_sets)
      abort_message("error in creating rotation list. abort!\n");

   /* write the list of angles as a waveform and add it to pattern file */

   pWaveformBuf->startSubSection(PATTERNHEADER);
   pWaveformBuf->putCodes(pBuffer, ind);
   tmp = new cPatternEntry(listName,0,num_sets,num_sets);
   if (tmp != NULL)
   {
      addPattern(tmp);
      sendPattern(tmp);
      listId = tmp->getReferenceID();
      if ( (listId >= 1) && (listId < 0x7FFFFF) )
        return listId;
      else
        abort_message("unable to determine the coordinate rotation list id. abort!\n");
   }
   else
      abort_message("error in creating rotation list. abort!\n");
}

//
//
//
void GradientBase::calcGradientRotation(int G_r, int G_p, int G_s, \
                          int peR_step, int peP_step, int peS_step,\
                          int peR_mult, int peP_mult, int peS_mult,\
                          int Gr_rms, int Gp_rms, int Gs_rms, int *G_array)
{
    // do the phase encode & oblique rotation

    int tempGx1=0;  int tempGy1=0;  int tempGz1=0;
    int tempGx2=0;  int tempGy2=0;  int tempGz2=0;
    int tempGx3=0;  int tempGy3=0;  int tempGz3=0;
    int srotm_11=0; int srotm_12=0; int srotm_13=0;
    int srotm_21=0; int srotm_22=0; int srotm_23=0;
    int srotm_31=0; int srotm_32=0; int srotm_33=0;
    int statGx, statGy, statGz;

    if (bgflag)
    {
       printf("GradientBase::calcGradientRotation:\n");
       printf("G_r = %d  G_p = %d  G_s = %d\n",G_r, G_p, G_s);
       printf("peR_step=%d  peP_step=%d  peS_step=%d\n",peR_step,peP_step,peS_step);
    }

    srotm_11 = (rotm_11*Gr_rms)>>16;
    srotm_21 = (rotm_21*Gr_rms)>>16;
    srotm_31 = (rotm_31*Gr_rms)>>16;

    srotm_12 = (rotm_12*Gp_rms)>>16;
    srotm_22 = (rotm_22*Gp_rms)>>16;
    srotm_32 = (rotm_32*Gp_rms)>>16;

    srotm_13 = (rotm_13*Gs_rms)>>16;
    srotm_23 = (rotm_23*Gs_rms)>>16;
    srotm_33 = (rotm_33*Gs_rms)>>16;

    /* Do the rotation of the static gradient part */

    statGx  = srotm_11*G_r + srotm_12*G_p + srotm_13*G_s ;
    statGy  = srotm_21*G_r + srotm_22*G_p + srotm_23*G_s ;
    statGz  = srotm_31*G_r + srotm_32*G_p + srotm_33*G_s ;

    if (dps_flag)
    {
        printf("dps_flag part executed\n");
        dps_fval[0] = statGx;
        dps_fval[1] = statGy;
        dps_fval[2] = statGz;
        return;
    }

    statGx >>= 15;  statGy >>= 15;  statGz >>= 15;

    if (bgflag)
      {
        if ( (statGx < -32767) || (statGx > 32767) )
            warn_message("advisory: X gradient value %d is outside +/-32767 range. power calculations may be incorrect\n",statGx);

          if ( (statGy < -32767) || (statGy > 32767) )
              warn_message("advisory: Y gradient value %d is outside +/-32767 range. power calculations may be incorrect\n",statGy);

          if ( (statGz < -32767) || (statGz > 32767) )
            warn_message("advisory: Z gradient value %d is outside +/-32767 range. power calculations may be incorrect\n",statGz);
      }
    /* First: Gradient RO */

    if (peR_step != 0)
    {
       tempGx1 = ((srotm_11*peR_step +0x3fff)>>15)*peR_mult;
       tempGy1 = ((srotm_21*peR_step +0x3fff)>>15)*peR_mult;
       tempGz1 = ((srotm_31*peR_step +0x3fff)>>15)*peR_mult;
    }

    /* Second: Gradient PE */

    if (peP_step != 0)
    {
       tempGx2 = ((srotm_12*peP_step +0x3fff)>>15)*peP_mult;
       tempGy2 = ((srotm_22*peP_step +0x3fff)>>15)*peP_mult;
       tempGz2 = ((srotm_32*peP_step +0x3fff)>>15)*peP_mult;
    }

    /* Third: Gradient SS */

    if (peS_step != 0)
    {
       tempGx3 = ((srotm_13*peS_step +0x3fff)>>15)*peS_mult;
       tempGy3 = ((srotm_23*peS_step +0x3fff)>>15)*peS_mult;
       tempGz3 = ((srotm_33*peS_step +0x3fff)>>15)*peS_mult;
    }

    G_array[0] = tempGx1 + tempGx2 + tempGx3 + statGx;
    G_array[1] = tempGy1 + tempGy2 + tempGy3 + statGy;
    G_array[2] = tempGz1 + tempGz2 + tempGz3 + statGz;
}
//
//
//
void GradientBase::oblpegrad_limit_check(int G_r, int G_p, int G_s, \
                          int peR_step, int peP_step, int peS_step, \
                                      int lim1, int lim2, int lim3)
{

    int G_array[3], Gx, Gy, Gz;

    calcGradientRotation(G_r, G_p, G_s, peR_step, peP_step, peS_step, lim1, lim2, lim3, \
                         32767, 32767, 32767, &(G_array[0]));

    Gx = G_array[0]; Gy = G_array[1]; Gz = G_array[2];
    
    if ( (Gx < -32767) || (Gx > 32767) )
        warn_message("X gradient value %d may be outside +/-32767 range\n",Gx);

    if ( (Gy < -32767) || (Gy > 32767) )
        warn_message("Y gradient value %d may be outside +/-32767 range\n",Gy);

    if ( (Gz < -32767) || (Gz > 32767) )
        warn_message("Z gradient value %d may be outside +/-32767 range\n",Gz);
}
//
//
//
int GradientBase::estimatePELoopCount(int vmult, double numpe)
{
   if ( (vmult == 0) || (vmult == zero) ) return 0;
   else if (vmult == one) return 1;
   else if (vmult == two) return 2;
   else if (vmult == three) return 3;
   else if ( (vmult >= v1) || (vmult <= VVARMAX) )
   {
      int value = getloopcount_for_index(vmult);
      if (value > 0) return value;
      else
        return (int)(numpe+0.49);
   }
   else
      return (int)(numpe+0.49);
}
//
//
//
void GradientBase::computeOBLPESHPGradPower(int when, cPatternEntry *Gr_wfg, cPatternEntry *Gp_wfg, cPatternEntry *Gs_wfg, \
                                        int whichones, int Gr, int Gp, int Gs, int Gr_step, int Gp_step, int Gs_step, \
                                        int vmult1, int vmult2, int vmult3,                                           \
                                        int Gr_lim, int Gp_lim, int Gs_lim, int GradMaskBit)
{
   int Gr_val, Gp_val, Gs_val, XYZgrad[3];
   int Xval=0, Yval=0, Zval=0;
   Gr_val=0; Gp_val=0; Gs_val=0;
   XYZgrad[0]=0; XYZgrad[1]=0; XYZgrad[2]=0;

   if (when == START)
   {
     if (whichones & 0x1)
       Gr_val = (int)(Gr_wfg->getPowerFraction());
     if (whichones & 0x2)
       Gp_val = (int)(Gp_wfg->getPowerFraction());
     if (whichones & 0x4)
       Gs_val = (int)(Gs_wfg->getPowerFraction());

     // determine the pe dimension loop count

     int Gr_mult = estimatePELoopCount(vmult1, Gr_lim);
     int Gp_mult = estimatePELoopCount(vmult2, Gp_lim);
     int Gs_mult = estimatePELoopCount(vmult3, Gs_lim);

     // Find average values for the PE gradients
     // Average value is estimated as the maximum PE amplitude divided by 3
     // Maximum PE value is step multiplied by nv/2
     int Gr_mavg = Gr_step * Gr_mult/(2 * 3);
     int Gp_mavg = Gp_step * Gp_mult/(2 * 3);
     int Gs_mavg = Gs_step * Gs_mult/(2 * 3);

//     printf("Gr=%d  Gr_mavg=%d\n",Gr,Gr_mavg);
//     printf("Gp=%d  Gp_mavg=%d\n",Gp,Gp_mavg);
//     printf("Gs=%d  Gs_mavg=%d\n",Gs,Gs_mavg);

     // PE contribution: now rotate the static and PE part
     calcGradientRotation((Gr+Gr_mavg),(Gp+Gp_mavg),(Gs+Gs_mavg), \
                         0,0,0, 0,0,0,             \
                         Gr_val,Gp_val,Gs_val, XYZgrad);

     Xval = XYZgrad[0];
     Yval = XYZgrad[1];
     Zval = XYZgrad[2];
   }
   else if (when == END)
   {
     if (whichones & 0x1)
       Gr_val = (int)(Gr_wfg->getLastElement());
     if (whichones & 0x2)
       Gp_val = (int)(Gp_wfg->getLastElement());
     if (whichones & 0x4)
       Gs_val = (int)(Gs_wfg->getLastElement());
     // rotate only the static part
     calcGradientRotation(Gr,Gp,Gs,0,0,0,0,0,0, \
                        Gr_val,Gp_val,Gs_val, XYZgrad);
     Xval = XYZgrad[0];
     Yval = XYZgrad[1];
     Zval = XYZgrad[2];
   }
   else
     abort_message("error in computing gradient power");

   long long now = getBigTicker();
   Xpower.eventFinePowerChange(Xval/32767.0,now);
   Ypower.eventFinePowerChange(Yval/32767.0,now);
   Zpower.eventFinePowerChange(Zval/32767.0,now);
}
//
//
//
void GradientBase::powerMonLoopStartAction()
{
  double x;
  long long now = getBigTicker();
  x = now * 0.0000000125L;
  Xpower.calculateEnergyPerInc(now);
  Ypower.calculateEnergyPerInc(now);
  Zpower.calculateEnergyPerInc(now);
  Xpower.saveStartOfLoop(getLoopLevel(),x);
  Ypower.saveStartOfLoop(getLoopLevel(),x);
  Zpower.saveStartOfLoop(getLoopLevel(),x);
}
//
//
//
void GradientBase::powerMonLoopEndAction()
{
  double x,cnt;
  int lvl;
  long long now = getBigTicker();
  cnt = (double) getcurrentloopcount();
  lvl = getLoopLevel();
  x = now * 0.0000000125L;
  Xpower.calculateEnergyPerInc(now);
  Ypower.calculateEnergyPerInc(now);
  Zpower.calculateEnergyPerInc(now);
  Xpower.computeEndOfLoop(lvl,cnt,x);
  Ypower.computeEndOfLoop(lvl,cnt,x);
  Zpower.computeEndOfLoop(lvl,cnt,x);
}
//
//
//
void GradientBase::PowerActionsAtStartOfScan()
{
  double x;
  long long now = getBigTicker();
  x = now * 0.0000000125L;
  Xpower.saveStartOfScan(x);
  Ypower.saveStartOfScan(x);
  Zpower.saveStartOfScan(x);
}
//
//
//
void GradientBase::computePowerAtEndOfScan()
{
  double x;
  long long now = getBigTicker();
//  double ntval = getval("nt");
  x = now * 0.0000000125L;
  Xpower.calculateEnergyPerInc(now);
  Ypower.calculateEnergyPerInc(now);
  Zpower.calculateEnergyPerInc(now);
/* following lines commented out to eliminate multiplication of energy by nt. gradient duty cycle isn't affected by averaging
   so don't want to have to take nt into account in those computations */
  //Xpower.computeEndOfScan(ntval,x);
  //Ypower.computeEndOfScan(ntval,x);
  //Zpower.computeEndOfScan(ntval,x);
}
//
//
//
void GradientBase::showPowerIntegral()
{
  double xpower, xfraction, ypower, yfraction, zpower, zfraction;
  long long now = getBigTicker();
  double ntval = getval("nt");
  
  xpower = Xpower.getPowerIntegral();
  xfraction = xpower*100.0/(now*12.5e-9*ntval);
  ypower = Ypower.getPowerIntegral();
  yfraction = ypower*100.0/(now*12.5e-9*ntval);
  zpower = Zpower.getPowerIntegral();
  zfraction = zpower*100.0/(now*12.5e-9*ntval);
 
  printf("%6s: X axis Energy=%10.4g\n",getName(),xpower);
  printf("%6s: Y axis Energy=%10.4g\n",getName(),ypower);
  printf("%6s: Z axis Energy=%10.4g\n",getName(),zpower);
}
//
//
//
void GradientBase::resetPowerEventStart()
{
   Xpower.initialize(getBigTicker());
   Ypower.initialize(getBigTicker());
   Zpower.initialize(getBigTicker());
}
//
//
//
void GradientBase::showEventPowerIntegral(const char *comment)
{
  printf("Power info: %s %5s Energy    X=%8.4g  Y=%8.4g  Z=%8.4g\n",comment,getName(),Xpower.getEventPowerIntegral(), Ypower.getEventPowerIntegral(), Zpower.getEventPowerIntegral());
}
//
//
//
void GradientBase::eventStartAction()
{
  Xpower.eventStartAction();
  Ypower.eventStartAction();
  Zpower.eventStartAction();
}
//
//
//
void GradientBase::getPowerIntegral(double *powerarray)
{
}

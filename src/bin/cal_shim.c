/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <string.h>

void getcal(char *calfile, float *scal)

{
  FILE *cf;
  int i,j;

  if ((cf = fopen(calfile,"r")) == NULL)
  {
    fprintf(stderr,"error: cannot open %s\n",calfile);
    return;
  }
  else
  {
    fprintf(stderr,"reading calibrations from %s\n",calfile);
    for (i = 0; i<=maxshim; i++)
    {
      for (j=0; j<maxshim; j++)
      {
        fscanf(cf,"%e",&scal[maxshim*i+j]);
#ifdef DEBUG
        fprintf(stderr," %d %d = %+.3e  ",i,j,scal[maxshim*i+j]);
#endif
      }
    fscanf(cf,"\n");
    printf("\n");
    }
  }
  fclose(cf);
}

void cal_shim(char *pos[3], poly_coeff a1,poly_coeff a2,shims *sval,float *scal)
  {
    float a,ia,x0,y0,z0;
    int i;
    FILE *fp;
    char logfile[255];
 

  strcpy(logfile,getenv("vnmruser"));
  strcat(logfile,"/shims/fastmap.log");
    fp = fopen(logfile,"a");
/*cal_shim*/
    sval[0].coeff = k_x(a1);   sval[0].dcur = (int)(-sval[0].coeff/scal[0]);
    sval[1].coeff = k_y(a1);   sval[1].dcur = (int)(-sval[1].coeff/scal[maxshim+1]);
    sval[2].coeff = k_z(a1);   sval[2].dcur = -(int)(sval[2].coeff/scal[2*maxshim+2]);
    sval[3].coeff = k_zx(a2);  sval[3].dcur = (int)(-sval[3].coeff/scal[3*maxshim+3]);
    sval[4].coeff = k_zy(a2);  sval[4].dcur = (int)(-sval[4].coeff/scal[4*maxshim+4]);
    sval[5].coeff = k_z2(a2);  sval[5].dcur = (int)(-sval[5].coeff/scal[5*maxshim+5]);
    sval[6].coeff = k_xy(a2);  sval[6].dcur = -(int)(sval[6].coeff/scal[6*maxshim+6]);
    if (fabs(scal[7*maxshim+7])>1.0E-06) sval[7].coeff = k_x2y2(a2); else  sval[7].coeff = 0.0; sval[7].dcur = (int)(-sval[7].coeff/scal[7*maxshim+7]);
    fprintf(stderr,"kx = %8.3f ky = %8.3f kz = %8.3f\n",
            sval[0].coeff,sval[1].coeff,sval[2].coeff);
    fprintf(fp,"kx = %8.3f ky = %8.3f kz = %8.3f\n",
            sval[0].coeff,sval[1].coeff,sval[2].coeff);
#ifdef DEBUG
    fprintf(stderr,"Ix = %8d Iy = %8d Iz = %8d\n",
            sval[0].dcur,sval[1].dcur,sval[2].dcur);
#endif
    fprintf(stderr,"kzx = %+8.3f kzy = %+8.3f kz2 = %+8.3f \nkxy = %+8.3f kx2y2 = %+8.3f\n",
            sval[3].coeff,sval[4].coeff,sval[5].coeff,sval[6].coeff,sval[7].coeff);    
    fprintf(fp,"kzx = %+8.3f kzy = %+8.3f kz2 = %+8.3f kxy = %+8.3f kx2y2 = %+8.3f\n",
            sval[3].coeff,sval[4].coeff,sval[5].coeff,sval[6].coeff,sval[7].coeff);    
#ifdef DEBUG
    fprintf(stderr,"Izx = %8d Izy = %8d Iz2 = %8d Ixy = %8d Ix2y2 = %8d\n",
            sval[3].dcur,sval[4].dcur,sval[5].dcur,sval[6].dcur,sval[7].dcur);
#endif
    x0=(float)atof(pos[0]);
    y0=(float)atof(pos[1]);
    z0=(float)atof(pos[2]);
    fprintf(stderr,"Pos. in cm (+ gdt DAC's):\n (x = %+6.2f, y = %+6.2f, z = %+6.2f) \n",x0,y0,z0);
    fprintf(fp,"Pos. in cm (+ gdt DAC's):\n (x = %+6.2f, y = %+6.2f, z = %+6.2f) \n",x0,y0,z0);
    a = sval[0].coeff+sval[3].coeff*z0+sval[6].coeff*2.0*y0   
      +sval[7].coeff*2.0*x0-sval[5].coeff*x0;                
    fprintf(stderr,"1st order corrections :\n");
    fprintf(stderr," kx-->%+8.3f ",a);
    fprintf(fp,"1st order corrections :\n");
    fprintf(fp," kx-->%+8.3f ",a);
    for (i=3; i<maxshim; i++)
    {
      a-=sval[i].coeff*scal[i*maxshim]/scal[i*maxshim+i];
    }
    fprintf(stderr,"-->%+8.3f\n",a);
    fprintf(fp,"-->%+8.3f\n",a);
    a=a/scal[0];
    ia=a/fabs(a);
    sval[0].dcur = -(int)((fabs(a)+0.5)*ia);

#ifdef DEBUG
    fprintf(stderr,"Ix = %8d\n",sval[0].dcur);
#endif
    a = sval[1].coeff+sval[4].coeff*z0+sval[6].coeff*2.0*x0     
      -sval[7].coeff*2*y0-sval[5].coeff*y0;                     
    fprintf(stderr," ky-->%+8.3f ",a);
    fprintf(fp," ky-->%+8.3f ",a);
    for (i=3; i<maxshim; i++)
    {
      a-=sval[i].coeff*scal[i*maxshim+1]/scal[i*maxshim+i];
    }
    fprintf(stderr,"-->%8.3f\n",a);
    fprintf(fp,"-->%8.3f\n",a);
    a=a/scal[maxshim+1];
    ia=a/fabs(a);
    sval[1].dcur = -(int)((fabs(a)+0.5)*ia);

#ifdef DEBUG
    fprintf(stderr,"Iy = %8d\n",sval[1].dcur);
#endif
    a = sval[2].coeff+sval[3].coeff*x0+sval[4].coeff*y0+sval[5].coeff*2*z0;     
    fprintf(stderr," kz-->%+8.3f ",a);
    fprintf(fp," kz-->%+8.3f ",a);
    for (i=3; i<maxshim; i++)
    {
      a-=sval[i].coeff*scal[i*maxshim+2]/scal[i*maxshim+i];
    }
    fprintf(stderr,"-->%8.3f\n",a);
    fprintf(fp,"-->%8.3f\n",a);
    a=a/scal[2*maxshim+2];
    ia=a/fabs(a);
    sval[2].dcur = -(int)((fabs(a)+0.5)*ia);

#ifdef DEBUG
    fprintf(stderr,"Iz = %8d\n",sval[2].dcur);
#endif
    a = 0.0;
    for (i=0; i<maxshim; i++)
    {
      a += sval[i].dcur*scal[maxshim*8+i];
    }
    fprintf(stderr,"\nresto/tof changes due to shim impurities (z2) = %8.1f\n",a);
    a -= sval[0].coeff*x0;
    a -= sval[1].coeff*y0;
    a -= sval[2].coeff*z0;
    a -= sval[3].coeff*x0*z0;
    a -= sval[4].coeff*y0*z0;
    a -= sval[5].coeff*z0*z0;
    a += sval[5].coeff*x0*x0*0.5;
    a += sval[5].coeff*y0*y0*0.5;
    a -= sval[6].coeff*2.0*x0*y0;
    a -= sval[7].coeff*x0*x0;
    a += sval[7].coeff*y0*y0;
    fprintf(stderr,"resto/tof changes = %8.1f\n",a);
    printf("%8.1f",a);
    for (i = 0; i<maxshim; i++)
    {
    printf("%8d",sval[i].dcur);
    } 
    fprintf(stderr,"tof changes by %+8.1f\n",a);
    fprintf(fp,"tof changes by %+8.1f\n",a);
    fclose(fp);
  } /*cal_shim*/


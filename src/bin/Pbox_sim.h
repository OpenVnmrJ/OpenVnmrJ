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
/* Pbox_sim.h - Pbox simulator */

void bloch(bl)
Blodata *bl;
{
  FILE *fil;
  char ch, name[MAXSTR];
  int i, j, Xm, Ym, Zm, iflg=0, nt=0;
  double *csp, *snp;
  double axis, mx, my, mz, rx1, rx2, p1, frq, dumm, scl = 32767.0;
  double teta, snt, cst, st2, ct2, Beff, csf, snf, step;
  double ca2, sa2, cas, ctsf, sfst, stct, scc, csf1, caf, scc1, sas;
  double xx, xy, xz, yx, yy, yz, zx, zy, zz;
  double rd = M_PI / 180.0, pi2 = 2.0 * M_PI, mxl = 0.0;
  int start, finsh;		/* time */

  if (bl->nstp < 1)
    bl->nstp = (int) (6.0 * bl->pw * bl->sw);
  i=2; while(i < bl->nstp) i*=2; bl->nstp = i; 

  if (bl->sign != 0)		/* check the number of steps for acquire */
  {
    (void) strcpy(name, shapes);
    (void) strcat(name, FN_SIM);
    if ((fil = fopen(name, "r")) == NULL)
    {
      printf("Pbox: cannot open \"%s\" file.\n", name);
      exit(1);
    }

    if (fscanf(fil, "%c", &ch) && ch == '#')
    {
      fscanf(fil, "%d %d", &nt, &j);
    }
    else
    {
      printf("Header missing in pbox.sim. Simulation aborted.\n\n"); 
      exit(1);
    }

    if (j != bl->nstp)
    {
      printf("\n %d %d - Unequal simulation data sizes ! \n", j, bl->nstp);
      printf("      Simulation aborted.\n\n");
      exit(1);
    }
    fclose(fil);
  }

  dumm = bl->st * M_PI / 2.0;
  mz = cos(dumm);
  mx = sin(dumm);
  p1 = bl->pw / bl->np;
  frq = M_PI * bl->sw;
  if (bl->reps > 1)
    printf("  sw=%.1f Hz (%d*%d matrix)\n", bl->sw, bl->nstp, bl->np);
  bl->B1max *= pi2;
  step = pi2 * bl->sw / (bl->nstp - 1);
  
  if (bl->T1 > 0.1e-12)
    rx1 = p1 / bl->T1;
  else
    rx1 = 0.0;
  if (bl->T2 > 0.1e-12)
    rx2 = p1 / bl->T2;
  else
    rx2 = 0.0;
  if ((rx1 > 0.0) && (rx2 < rx1))
    rx2 = rx1;
  if (bl->reps > 1)
  {
    if (rx2 > 0.0)
    {
      if (rx1 > 0.0)
        printf("  T1=%.7f sec, ", p1/rx1);
      printf("T2=%.7f sec\n", p1/rx2);
    }
  }

  csp = arry(bl->np);
  snp = arry(bl->np);
  bl->Mx = arry(bl->nstp);
  bl->My = arry(bl->nstp);
  bl->Mz = arry(bl->nstp);

  for (j = 0; j < bl->nstp; j++)
  {
    bl->Mx[j] = mx;
    bl->My[j] = 0.0;
    bl->Mz[j] = mz;
  }
  for (i = 0; i < bl->np; i++)
  {
    Pha[i] = Pha[i] * rd;
    mxl = MAX(mxl, Amp[i]);
    csp[i] = cos(Pha[i]);
    snp[i] = sin(Pha[i]);
  }
  for (i = 0; i < bl->np; i++)
    Amp[i] = bl->B1max * Amp[i] / mxl; 

  start = time(NULL);		/* start the simulation */
  for (j = 0; j < bl->nstp; j++)
  {
    for (i = 0; i < bl->np; i++)
    {
      teta = atan2(Amp[i], frq);
      Beff = sqrt(Amp[i] * Amp[i] + frq * frq);

      snt = sin(teta);
      cst = cos(teta);
      ct2 = cst * cst;
      st2 = snt * snt;
      snf = sin(Beff * p1);
      csf = cos(Beff * p1);
      ctsf = cst * snf;
      sfst = snf * snt;
      ca2 = csp[i] * csp[i];
      sa2 = snp[i] * snp[i];
      cas = csp[i] * snp[i];
      scc = st2 + ct2 * csf;
      stct = snt * cst;
      csf1 = 1 - csf;
      caf = cas * (csf - scc);
      scc1 = stct * csf1;
      sas = snp[i] * sfst;

      xx = ca2 * csf + sa2 * scc;
      xy = caf - ctsf;
      xz = csp[i] * sfst - snp[i] * scc1;
      yx = ctsf + caf;
      yy = sa2 * csf + ca2 * scc;
      yz = sas + csp[i] * scc1;
      zx = csp[i] * sfst + snp[i] * scc1;
      zy = csp[i] * scc1 - sas;
      zz = ct2 + st2 * csf;
      
      mx = bl->Mx[j]*xx + bl->My[j]*xy + bl->Mz[j]*xz - bl->Mx[j]*rx2; 
      my = bl->Mx[j]*yx + bl->My[j]*yy + bl->Mz[j]*yz - bl->My[j]*rx2; 
      mz = -bl->Mx[j]*zx + bl->My[j]*zy + bl->Mz[j]*zz - bl->Mz[j]*rx1 + rx1;

      bl->Mx[j] = mx;
      bl->My[j] = my;
      bl->Mz[j] = mz;
    }
    frq -= step;
    finsh = time(NULL);
    if (((finsh - start) > 10) && (bl->reps > 1) && (iflg == 0))
    {
      dumm = (finsh - start) * (((double) bl->nstp / j) - 1.0);
      if (dumm > 10)
      {
	printf("  Simulation will take another %.0f sec ...\n", dumm);
	iflg = 1;
	if (dumm > bl->time)
	{
	  printf("     which exceeds the time limit of %d sec\n", bl->time);
	  printf("\n  Simulation aborted...\n\n");
	  return;
	}
      }
    }
  }
  dumm = bl->sw;
  if (bl->sw > 1500.0) dumm = 0.001 * bl->sw;

  (void) strcpy(name, shapes);
  (void) strcat(name, FN_SIM);

  if (bl->sign != 0)
  {
    if ((fil = fopen(name, "r")) == NULL)
    {
      printf("Pbox: cannot open \"%s\" file.\n", name);
      exit(1);
    }
    skipcomms(fil);
    if (bl->sign > 0)
    {
      for (i = 0; i < bl->nstp; i++)
      {
	fscanf(fil, "%lf %lf %lf %lf\n", &axis, &mx, &my, &mz);
	bl->Mx[i] += mx;
	bl->My[i] += my;
	bl->Mz[i] += mz;
      }
    }
    else if (bl->sign < 0)
    {
      for (i = 0; i < bl->nstp; i++)
      {
	fscanf(fil, "%lf %lf %lf %lf\n", &axis, &mx, &my, &mz);
	bl->Mx[i] = mx - bl->Mx[i];
	bl->My[i] = my - bl->My[i];
	bl->Mz[i] = mz - bl->Mz[i];
      }
    }
    fclose(fil);
  }

  scl /= (nt+1);
  Xm = (int) (scl * maxval(bl->nstp, bl->Mx));
  Ym = (int) (scl * maxval(bl->nstp, bl->My));
  Zm = (int) (scl * maxval(bl->nstp, bl->Mz));

  if ((fil = fopen(name, "w")) == NULL)
  {
    printf("Pbox: cannot open \"%s\" file \n", name);
    exit(1);
  }

  if (bl->reps > 2)
  {
    if (bl->sign == 0)
      printf("\nSaving simulation to :\n \"%s\", \n", name);
    else 
      printf("\nAcquiring simulation data in :\n \"%s\", \n", name);
  }

  fprintf(fil, "# %d %d %d %d %d\n", nt+1, bl->nstp, Xm, Ym, Zm);
  fprintf(fil, "#    scale         Mx         My         Mz\n"); 
  j = bl->nstp - 1;
  for (i = 0; i < bl->nstp; i++)
  {
    axis = dumm * (0.5 - (double) i / j);
    fprintf(fil, " %12.8f %10.6f %10.6f %10.6f\n", axis, bl->Mx[i], 
                   bl->My[i], bl->Mz[i]);
  }
  fclose(fil);					/* close the pbox.sim file */

  (void) strcpy(name, shapes);
  (void) strcat(name, FN_XY);
  if ((fil = fopen(name, "w")) == NULL)
  {
    printf("Pbox: cannot open \"%s\" file \n", name);
    exit(1);
  }
  if (bl->reps > 2) 
      printf(" \"%s\" \n", name);

  for (i = 0; i < bl->nstp; i++)
    fprintf(fil, " %10d %10d\n", (int) (scl * bl->Mx[i]), (int) (scl * bl->My[i]));
  fclose(fil);					/* close the pbox.xy file */

  (void) strcpy(name, shapes);
  (void) strcat(name, FN_Z);
  if ((fil = fopen(name, "w")) == NULL)
  {
    printf("Pbox: cannot open \"%s\" file \n", name);
    exit(1);
  }
  if (bl->reps > 2)
    printf(" \"%s\" \n", name);

  for (i = 0; i < bl->nstp; i++)
    fprintf(fil, " %10d %10d\n", (int) (scl * bl->Mz[i]), 0);
  fclose(fil);					/* close the pbox.mz file */

  if (bl->reps > 1) printf("\n");

}


void bloch_rd(bl)		/* Bloch simulation with Radiation Damping */
Blodata *bl;
{
  FILE *fil;
  char ch, name[MAXSTR];
  int i, j, Xm, Ym, Zm, iflg=0, nt=0;
  double *csp, *snp, csr, snr;
  double axis, mx, my, mz, rx1, rx2, p1, frq, dumm, scl = 32767.0;
  double teta, snt, cst, st2, ct2, Beff, csf, snf, step;
  double ca2, sa2, cas, ctsf, sfst, stct, scc, csf1, caf, scc1, sas;
  double xx, xy, xz, yx, yy, yz, zx, zy, zz, rdc;
  double rd = M_PI / 180.0, pi2 = 2.0 * M_PI, mxl = 0.0;
  int start, finsh;		/* time */ 

  if (bl->nstp < 1)
    bl->nstp = (int) (6.0 * bl->pw * bl->sw);
  i=2; while(i < bl->nstp) i*=2; bl->nstp = i; 

  if (bl->sign != 0)		/* check the number of steps for acquire */ 
  {
    (void) strcpy(name, shapes);
    (void) strcat(name, FN_SIM);
    if ((fil = fopen(name, "r")) == NULL)
    {
      printf("Pbox: cannot open \"%s\" file.\n", name);
      exit(1);
    }

    if (fscanf(fil, "%c", &ch) && ch == '#')
    {
      fscanf(fil, "%d %d", &nt, &j);
    }
    else
    {
      printf("Header missing in pbox.sim. Simulation aborted.\n\n"); 
      exit(1);
    }

    if (j != bl->nstp)
    {
      printf("\n %d %d - Unequal simulation data sizes ! \n", j, bl->nstp);
      printf("      Simulation aborted.\n\n");
      exit(1);
    }
    fclose(fil);
  }

  dumm = bl->st * M_PI / 2.0;
  mz = cos(dumm);
  mx = sin(dumm);
  p1 = bl->pw / bl->np;
  frq = M_PI * bl->sw;
  if (bl->reps > 1)
    printf("  sw=%.1f Hz (%d*%d matrix)\n", bl->sw, bl->nstp, bl->np);
  bl->B1max *= pi2;
  rdc = bl->rdc * pi2;
  if (bl->reps > 1)
    printf("  Radiation damping constant = %.2f Hz\n", bl->rdc);

  step = pi2 * bl->sw / (bl->nstp - 1);
  if (bl->T1 > 0.1e-12)
    rx1 = p1 / bl->T1;
  else
    rx1 = 0.0;
  if (bl->T2 > 0.1e-12)
    rx2 = p1 / bl->T2;
  else
    rx2 = 0.0;
  if ((rx1 > 0.0) && (rx2 < rx1))
    rx2 = rx1;
  if (bl->reps > 1)
  {
    if (rx2 > 0.0)
    {
      if (rx1 > 0.0)
        printf("  T1=%.7f sec, ", p1/rx1);
      printf("T2=%.7f sec\n", p1/rx2);
    }
  }

  csp = arry(bl->np);
  snp = arry(bl->np);
  bl->Mx = arry(bl->nstp);
  bl->My = arry(bl->nstp);
  bl->Mz = arry(bl->nstp);

  for (j = 0; j < bl->nstp; j++)
  {
    bl->Mx[j] = mx;
    bl->My[j] = 0.0;
    bl->Mz[j] = mz;
  }
  for (i = 0; i < bl->np; i++)
  {
    Pha[i] = Pha[i] * rd;
    mxl = MAX(mxl, Amp[i]);
  }
  for (i = 0; i < bl->np; i++)
  {
    Amp[i] = bl->B1max * Amp[i] / mxl; 
    csp[i] = Amp[i] * cos(Pha[i]);
    snp[i] = Amp[i] * sin(Pha[i]);
  }

  start = time(NULL);		/* start the simulation */
  for (j = 0; j < bl->nstp; j++)
  {
    for (i = 0; i < bl->np; i++)
    {
      mx = csp[i] - rdc*bl->Mx[j]; 
      my = snp[i] - rdc*bl->My[j];
      mz = sqrt(mx*mx + my*my);
      csr=mx/mz;  snr=my/mz;

      teta = atan2(mz, frq);
      Beff = sqrt(mz * mz + frq * frq);

      snt = sin(teta);
      cst = cos(teta);
      ct2 = cst * cst;
      st2 = snt * snt;
      snf = sin(Beff * p1);
      csf = cos(Beff * p1);
      ctsf = cst * snf;
      sfst = snf * snt;
      ca2 = csr * csr;
      sa2 = snr * snr;
      cas = csr * snr;
      scc = st2 + ct2 * csf;
      stct = snt * cst;
      csf1 = 1 - csf;
      caf = cas * (csf - scc);
      scc1 = stct * csf1;
      sas = snr * sfst;

      xx = ca2 * csf + sa2 * scc;
      xy = caf - ctsf;
      xz = csr * sfst - snr * scc1;
      yx = ctsf + caf;
      yy = sa2 * csf + ca2 * scc;
      yz = sas + csr * scc1;
      zx = csr * sfst + snr * scc1;
      zy = csr * scc1 - sas;
      zz = ct2 + st2 * csf;
      
      mx = bl->Mx[j]*xx + bl->My[j]*xy + bl->Mz[j]*xz - bl->Mx[j]*rx2; 
      my = bl->Mx[j]*yx + bl->My[j]*yy + bl->Mz[j]*yz - bl->My[j]*rx2; 
      mz = -bl->Mx[j]*zx + bl->My[j]*zy + bl->Mz[j]*zz - bl->Mz[j]*rx1 + rx1;

      bl->Mx[j] = mx;
      bl->My[j] = my;
      bl->Mz[j] = mz;
    }
    frq -= step;
    finsh = time(NULL);
    if (((finsh - start) > 10) && (bl->reps > 1) && (iflg == 0))
    {
      dumm = (finsh - start) * (((double) bl->nstp / j) - 1.0);
      if (dumm > 10)
      {
	printf("  Simulation will take another %.0f sec ...\n", dumm);
	iflg = 1;
	if (dumm > bl->time)
	{
	  printf("     which exceeds the time limit of %d sec\n", bl->time);
	  printf("\n  Simulation aborted...\n\n");
	  return;
	}
      }
    }
  }
  dumm = bl->sw;
  if (bl->sw > 1500.0) dumm = 0.001 * bl->sw;

  (void) strcpy(name, shapes);
  (void) strcat(name, FN_SIM);

  if (bl->sign != 0)
  {
    if ((fil = fopen(name, "r")) == NULL)
    {
      printf("Pbox: cannot open \"%s\" file.\n", name);
      exit(1);
    }
    skipcomms(fil);
    if (bl->sign > 0)
    {
      for (i = 0; i < bl->nstp; i++)
      {
	fscanf(fil, "%lf %lf %lf %lf\n", &axis, &mx, &my, &mz);
	bl->Mx[i] += mx;
	bl->My[i] += my;
	bl->Mz[i] += mz;
      }
    }
    else if (bl->sign < 0)
    {
      for (i = 0; i < bl->nstp; i++)
      {
	fscanf(fil, "%lf %lf %lf %lf\n", &axis, &mx, &my, &mz);
	bl->Mx[i] = mx - bl->Mx[i];
	bl->My[i] = my - bl->My[i];
	bl->Mz[i] = mz - bl->Mz[i];
      }
    }
    fclose(fil);
  }

  scl /= (nt+1);
  Xm = (int) (scl * maxval(bl->nstp, bl->Mx));
  Ym = (int) (scl * maxval(bl->nstp, bl->My));
  Zm = (int) (scl * maxval(bl->nstp, bl->Mz));

  if ((fil = fopen(name, "w")) == NULL)
  {
    printf("Pbox: cannot open \"%s\" file \n", name);
    exit(1);
  }

  if (bl->reps > 2)
  {
    if (bl->sign == 0)
      printf("\nSaving simulation to :\n \"%s\", \n", name);
    else 
      printf("\nAcquiring simulation data in :\n \"%s\", \n", name);
  }

  fprintf(fil, "# %d %d %d %d %d\n", nt+1, bl->nstp, Xm, Ym, Zm);
  fprintf(fil, "#    scale         Mx         My         Mz\n"); 
  j = bl->nstp - 1;
  for (i = 0; i < bl->nstp; i++)
  {
    axis = dumm * (0.5 - (double) i / j);
    fprintf(fil, " %12.8f %10.6f %10.6f %10.6f\n", axis, bl->Mx[i], 
                   bl->My[i], bl->Mz[i]);
  }
  fclose(fil);					/* close the pbox.sim file */

  (void) strcpy(name, shapes);
  (void) strcat(name, FN_XY);
  if ((fil = fopen(name, "w")) == NULL)
  {
    printf("Pbox: cannot open \"%s\" file \n", name);
    exit(1);
  }
  if (bl->reps > 2) 
      printf(" \"%s\" \n", name);

  for (i = 0; i < bl->nstp; i++)
    fprintf(fil, " %10d %10d\n", (int) (scl * bl->Mx[i]), (int) (scl * bl->My[i]));
  fclose(fil);					/* close the pbox.xy file */

  (void) strcpy(name, shapes);
  (void) strcat(name, FN_Z);
  if ((fil = fopen(name, "w")) == NULL)
  {
    printf("Pbox: cannot open \"%s\" file \n", name);
    exit(1);
  }
  if (bl->reps > 2)
    printf(" \"%s\" \n", name);

  for (i = 0; i < bl->nstp; i++)
    fprintf(fil, " %10d %10d\n", (int) (scl * bl->Mz[i]), 0);
  fclose(fil);					/* close the pbox.mz file */

  if (bl->reps > 1) printf("\n");

}


void    blqtph(bl)		/* Bloch trajectories */
Blodata *bl;
{
  FILE   *fil;
  char    name[MAXSTR];
  int     m;
  double  p1, teta, Beff, snf, csf, snt, cst, st2, ct2, ctsf, stsf;
  double  scc, ca2, sa2, caf, scc1, sas, cas, sp1, cp1, csp, snp;
  double  xx, xy, xz, yx, yy, yz, zx, zy, zz, mx, my, mz;
  double  f1, f2, b1max, scl = 32767.0, rd = M_PI / 180.0, mxl = 0.0;
  double  rx1 = 0.0, rx2 = 0.0;

  bl->Mx = arry(bl->np+2);
  bl->My = arry(bl->np+2);
  bl->Mz = arry(bl->np+2);   
  
  p1 = bl->st * M_PI / 2.0;  
  bl->Mx[0]=sin(p1); bl->My[0]=0.0; bl->Mz[0]=cos(p1);  /* status */
  p1 = bl->pw/bl->np; 
  if (bl->T2 > bl->T1) 
    bl->T2 = bl->T1;
  if (bl->T1 > 0.1e-12) 
    rx1 = p1/bl->T1;
  if (bl->T2 > 0.1e-12) 
    rx2 = p1/bl->T2;
  f1 = bl->ofs*M_PI*2.0;
  f2 = f1*f1;
  b1max = bl->B1max*M_PI*2.0;
  for (m = 0; m < bl->np; m++)
    mxl = MAX(mxl, Amp[m]);
  if (mxl == 0.0) mxl = 1.0;

  for(m=0; m<bl->np; m++)           
  {
    xx = b1max*Amp[m]/mxl;
    teta=atan2(xx, f1);
    Beff=sqrt(xx*xx + f2); 
    xx = rd*Pha[m];                        
    csp=cos(xx); snp=sin(xx);

snf=sin(Beff*p1);     csf=cos(Beff*p1); ca2=csp*csp;    sa2=snp*snp;
snt=sin(teta);        cst=cos(teta);    st2=snt*snt;    ct2=cst*cst;
ctsf=cst*snf;         stsf=snt*snf;     sas=snp*stsf;   cas=csp*stsf;
scc1=snt*cst*(1-csf); scc=st2+ct2*csf;  sp1=snp*scc1;   cp1=csp*scc1;    
caf=csp*snp*(csf-scc);        

    xx=ca2*csf+sa2*scc;     xy=caf-ctsf;            xz=cas-sp1;
    yx=caf+ctsf;            yy=sa2*csf+ca2*scc;     yz=cp1+sas;
    zx=cas+sp1;             zy=cp1-sas;             zz=ct2+st2*csf;

    mx = bl->Mx[m]*xx + bl->My[m]*xy + bl->Mz[m]*xz - bl->Mx[m]*rx2;
    my = bl->Mx[m]*yx + bl->My[m]*yy + bl->Mz[m]*yz - bl->My[m]*rx2;
    mz =-bl->Mx[m]*zx + bl->My[m]*zy + bl->Mz[m]*zz + (1.0 - bl->Mz[m])*rx1;

    bl->Mx[m+1]=mx; bl->My[m+1]=my; bl->Mz[m+1]=mz; 
  }

  (void) strcpy(name, shapes);
  (void) strcat(name, FN_SIM);
  if ((fil = fopen(name, "w")) == NULL)
  {
    printf("\nPbox: cannot open \"%s\" file \n", name);
    exit(1);
  }
  if (bl->reps > 2)
    printf("\nSaving simulation to :\n \"%s\", ", name);

  fprintf(fil, "#    scale         Mx         My         Mz\n"); 
  for (m = 0; m < bl->np+1; m++)
    fprintf(fil, " %12d %10.6f %10.6f %10.6f\n", m, bl->Mx[m], bl->My[m], bl->Mz[m]); 
  fclose(fil);					/* close the pbox.sim file */

  (void) strcpy(name, shapes);
  (void) strcat(name, FN_XY);
  if ((fil = fopen(name, "w")) == NULL)
  {
    printf("\nPbox: cannot open \"%s\" file \n", name);
    exit(1);
  }
  if (bl->reps > 2) 
      printf(" \"%s\" \n", name);

  for (m = 0; m < bl->np+1; m++)
    fprintf(fil, " %10d %10d\n", (int) (scl * bl->Mx[m]), (int) (scl * bl->My[m]));
  fclose(fil);	 				/* close the pbox.xy file */
}


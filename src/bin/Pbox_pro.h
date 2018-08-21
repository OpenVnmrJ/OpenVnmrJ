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
/* Pbox_pro.h - Pandora's Box Procedures */

void setpbox(itnf)
int itnf;
{
  int   i;
  FILE *fil;

/* ------------------------- Set Globals ---------------------------------- */
  setglo(); sethdr(); sethdrstr(); 
  Usetglo();
  (void) strcat(strcpy(ifn, SYSSHAPELIB), FN_GLO);	/* first read vnmr */
  if ((fil = fopen(ifn, "r")) != NULL)
  {
    getglo(fil); 
    gethdr(fil);
    fclose(fil); 
  }  
  if(home[0] == '\0')
    (void) strcpy(home, (char *) getenv("HOME"));       /* then read local */
  (void) strcat(strcpy(ifn, home), FN_GLO);
  if ((fil = fopen(ifn, "r")) != NULL)			/* load globals */
  {
    getglo(fil);
    gethdr(fil);
    fclose(fil);
  }
  setwavelib(); 
  (void) strcat(strcpy(shapes, home), g.shdir);
  (void) strcat(strcpy(waves, home), g.wvdir);

  i = remove(strcat(strcpy(e_str, shapes), FN_GR));
  i = remove(strcat(strcpy(e_str, shapes), FN_CAL));
  i = remove(strcat(strcpy(e_str, shapes), FN_FID));
  i = remove(strcat(strcpy(e_str, shapes), FN_XY));
  i = remove(strcat(strcpy(e_str, shapes), FN_Z));

  if(itnf)
  {
    Wv = (Wave *) calloc(itnf, sizeof(Wave));
    Sh = (Shape *) calloc(itnf, sizeof(Shape)); 
    for (i=0; i<itnf; i++) Sh[i].set = 0;
  }
}

int loadshape(id)
int id;
{
  int j;

  if (Sh[id].set) return 1;
  else resetshape(&Sh[id]); 

  if ((Wv[id].sh[0] == 's') && (strcmp(Wv[id].sh, "slurp1") == 0 ||
         strcmp(Wv[id].sh, "slurp2") == 0))
  {								/* slurps */
    fixdir(Wv[id].dir);
    strcat(Wv[id].dir,"slurp/"); 
    Wv[id].sh[2] = slrp(Wv[id].pw, h.T2); 
    Wv[id].sh[0] = 's', Wv[id].sh[1] = Wv[id].sh[5];
    Wv[id].sh[3] = '\0';
  } 

  for (j = 0; j < h.itnf; j++)
  {
    if((j!=id) && (Sh[j].set) && (strcmp(Wv[id].sh, Wv[j].sh) == 0))
    { 
      Sh[id] = Sh[j];
      j=h.itnf;
    }
  }

  if (!Sh[id].set)
  {
    if (j = is_reserved(Wv[id].sh))   /* set NULL and RDC shapes */
    {
      Sh[id].set = j; 
      if(j == 2)
      {
        Sh[id].dnp = 1;
        Sh[id].pwb = 1.0;
        strcpy(Sh[id].sh, "null");
      }
    }
    else
    {
      Sh[id].set = getshape(Wv[id].dir, Wv[id].sh, &Sh[id], h.reps);
      if (!Sh[id].set) return 0; 
      if ((Wv[id].dash >= 0.0) && (isname(&Sh[id].dash))) 
      {
        setdash(&Sh[id], Wv[id].dash);
        if (strcmp(Wv[id].sh, "wurst") == 0) 
          Sh[id].pwb = Wv[id].dash * 2.0; 
      }
    }
  }
      
  return 1;
}

/* -------------------------- Read Shapes from Wavelib ----------------------- */

int setshapes()
{
  int i, j;
  char str1[MAXSTR];
      
  h.itns = 0; h.itnw = 0; h.itnd = 0; str1[0]='\0';  
  h.imax = 0; h.maxtof = 0.0; 
  if ((h.itnf < 2) && (h.bscor[0] != 'n'))
    h.bscor[0] = 'n', h.bscor[1] = '\0';

  for(i = 0; i < h.itnf; i++)				/* set shape params */
  { 
    if ((h.type[0] == 'n') || (h.type[0] == '\0'))
    {
      if (Sh[i].dres > 0.001) set_type('d');
      else if (Sh[i].pwb > 0.01) set_type('r');
      else if (Sh[i].set > 0) set_type('g');            /* if not RDC */
    }
    if (h.tfl < 1)
    {
      Wv[i].of -= h.offset;   
      tm = fabs(Wv[i].of);
      if ((tm > h.maxtof) && (Wv[i].oflg != 'T') && (Sh[i].oflg != 'T'))
      {
        h.maxtof = tm;
        h.imax = i;
      }
    }
    else if (h.bscor[0] != 'n')
      h.bscor[0] = 'n', h.bscor[1] = '\0';

    if ((h.tfl == 1) && ((Wv[i].pw < 0.000001) || (Wv[i].pw > 1.0)))
      Wv[i].pw = g.gpw;
    else if ((h.tfl == 2) && (Wv[i].pw < 0.000001))
      Wv[i].pw = g.pw;
    else if ((fabs(Wv[i].bw) < 0.000001) && (fabs(Wv[i].pw) < 0.000001))
      Wv[i].pw = g.pw;
    else
      Wv[i].pwb = Wv[i].bw*Wv[i].pw; 

    if (h.sfl > 0)				/* supercycles */
    {
      Wv[i].sfl = 1;
      strcpy(Wv[i].su, h.su);
    }
    else if ((h.sfl < 0) && (Wv[i].sfl<1))	/* default */
      Wv[i].sfl = -1;
    else if ((h.sfl == 0) && (Wv[i].sfl<1))	/* no sucyc */
      Wv[i].sfl = 0;

    if (Wv[i].sfl < 0)
    { 
      if (!isname(&Sh[i].su)) 
        Wv[i].sfl = 0; 				/* if default = n */
      else
        { strcpy(Wv[i].su, Sh[i].su); Wv[i].sfl = 1; }
    }

    if (Wv[i].sfl)					
    {
      if (h.itns==0) strcpy(str1, Wv[i].su); 
      else strcat(strcat(str1, ","), Wv[i].su);
      h.itns++;
    }
    if (isname(&Sh[i].df)) h.itnd++;
    if (isname(&Sh[i].wf)) h.itnw++;
  }

  if ((h.itns = nsuc(str1)) > 0)	/* setup supercycles */
  {
    su = (Shape *) calloc(h.itns, sizeof(Shape)); 
    for(i=0; i<h.itns; i++) 
    {
      resetshape(&su[i]); cutstr(su[i].sh, str1);   
      e_str[0]='\0';
      su[i].set = getshape(e_str, su[i].sh, &su[i], h.reps); 
      if (!su[i].set) return 0; 
    }  
  }
  if (h.itnw)
  {
    wf = (Windw *) calloc(h.itnw, sizeof(Windw)); 
    for(i=0, j=0; i<h.itnw; i++) 
    {
      if (isname(&Sh[i].wf))
      {
        resetwindow(&wf[j]);
        e_str[0]='\0';
        if (getwindow(e_str, Sh[i].wf, &wf[j], h.reps) == 0) return 0;
        j++;
      }
    }
  }
  if (h.itnd)
  {
    df = (Windw *) calloc(h.itnd, sizeof(Windw)); 
    for(i=0, j=0; i<h.itnd; i++) 
    {
      if (isname(&Sh[i].df))
      {
        resetwindow(&df[j]);
        strcpy(e_str, "filters/");
        if (getwindow(e_str, Sh[i].df, &df[j], h.reps) == 0) return 0; 
        j++;
      }
    }
  } 
  return 1;
}

int resetpars(fln, inm)
int inm;
char fln[MAXSTR];
{
  FILE *fil;
  int i, j;
  char str[MAXSTR];


  if ((fil = fopen(fln, "r")) == NULL)   
  { 
    err(FN_INP); 
    return 0; 
  }  

  while (findch(fil, '$') > 0)
  {
    fscanf(fil, "%s", str);
    j = ftell(fil);
    if (isdigit(str[0])) 
      i = stoi(str);  
    else 
    {
      fclose(fil); 
      pxerr("Pbox: input file error. Usage : $ wavenumber");  
      return 0;
    }
    if ((i > 0) && (--i < h.itnf))
    {
      sprintf(Sh[i].modpar, "# $%d \n", i+1);
      modpars(fil, &Sh[i], j); 
    }
    fseek(fil, j, 0);
  }
  fclose(fil);
  return 1;
}


int closepbox()
{
  FILE *fil, *fil2;
  double **Sam, **Sph, **Wam, **Wph, **Wbs;
  char str1[MAXSTR], str2[MAXSTR];
  static char fnm[MAXSTR], fcl[MAXSTR];
  int i, j, k, n, m, nn, itr, nitr, inp, gate, minnumb, npmax;
  double mindelta, maxdelta, dumm, attf, pls, plu, sum, mxl;
  double B1max, b1max, qq, dj, itrerr, pw0, edB;
  void   blqtph();

/* ------------------------------- Reset parameters ------------------------- */

  str1[0]='\0'; str2[0]='\0'; fnm[0]='\0'; fcl[0]='\0';
  i=0, j=0, k=0, n=0, m=0, nn=0, itr=0, nitr=0, inp=0, gate=0, minnumb=0;
  mindelta=0.0, maxdelta=0.0, dumm=0.0, attf=0.0, pls=0.0; 
  plu=0.0, sum=0.0, mxl=0.0, Pfact=0.0, B1rms=0.0, MaxAmp=0.0, MaxIncr=0.0;
  B1max=0.0, b1max=0.0, RmsPwr=0, qq=0.0, dj=0.0, itrerr=0.0, pw0=0.0;


/* -------------------------- set up NULL pulses -----------------------------*/

  if ((h.itnf == 1) && (Sh[0].set == 2))                 
  {
    h.attn.flg = -1;   
    h.attn.u = 'i';     
    h.attn.mgn = 0.0;
    Sh[0].set = 1;
  }
    
/* ------------------------------- Find Time Base --------------------------- */

  if (h.pw90 > 0.01) h.Bref = 250.0 / h.pw90;
  if (h.tfl > 0)					/* GRD and wf files */
  {
    for (i = 0; i < h.itnf; i++)			/* set time constants */
    {
      if (fabs(Sh[i].fla) < 0.001) Sh[i].fla = 100.0; 
      if (fabs(Wv[i].fla) < 0.001) Wv[i].fla = Sh[i].fla;  
      if (Wv[i].pw < 0.0) Wv[i].pw = -Wv[i].pw; 
      if (Sh[i].duty < 1.0) Wv[i].pw *= Sh[i].duty;
      Sh[i].lft = 1.0 - 2.0*Sh[i].lft;
      Sh[i].rgt *= (1.0 + Sh[i].lft);
    }
    if (h.tfl == 1)					/* wf files */
      MaxAmp = g.maxgr;
    else					            /* wf files */
    {
      MaxAmp = g.maxgr = 1.0;
      h.coms[0] = 'n', h.coms[1] = '\0';
      g.cmpr = 0;
      g.minpwg = g.minpw;
      if (strcmp(h.name, "Pbox")==0)
        strcat(strcpy(h.fname, shapes),"pbox_wt");
    }      
  }
  else							/* .RF and .DEC files */
  {
    MaxAmp = g.maxamp;
    for (i = 0; i < h.itnf; i++)			/* set time constants */
    {
      if (Sh[i].set > 0)				/* exclude RDC shapes */
      {
        if (fabs(Wv[i].fla) < 0.001) 
          Wv[i].fla = Sh[i].fla, Wv[i].oflg = Sh[i].oflg; 
        if ((fabs(Wv[i].pwb) > 0) && (Sh[i].ffl)) Sh[i].pwb = Wv[i].pwb; 
        if (strcmp(Sh[i].fm, "fslg") == 0) 
        {
          Sh[i].pwb = 2.0*cos(rd*Sh[i].fla); 
          if (Wv[i].bw < 0.0) 
          {
            Wv[i].bw = -Wv[i].bw;
            Sh[i].pwb = -Sh[i].pwb;
          }
        }
        if (Sh[i].pwb == 0.0)
        {
          pxerr("pw*bw not known");  
          return 0;
        }
        if (Wv[i].pw < 0.0) Wv[i].pw = -Wv[i].pw;  
        if (Wv[i].bw < 0.0) Wv[i].bw = -Wv[i].bw; 
        tm = g.minpw*0.000001; 
        if (Wv[i].bw < tm)
          Wv[i].bw = fabs(Sh[i].pwb / Wv[i].pw);
        if ((Wv[i].pw < tm) || (!Sh[i].ffl))          /*  give bw preference */
          Wv[i].pw = fabs(Sh[i].pwb / Wv[i].bw);

/*
        if (strcmp(Sh[i].fm, "fslg") == 0) 
          Wv[i].of = Wv[i].bw;
*/
        if (Sh[i].duty < 1.0) 
        {
          Wv[i].pw *= Sh[i].duty;
          Wv[i].pwb *= Sh[i].duty;
          Sh[i].pwb *= Sh[i].duty;
        }
        Sh[i].lft = 1.0 - 2.0*Sh[i].lft;
        Sh[i].rgt *= (1.0 + Sh[i].lft);
      }
    }
    if ((h.dcyc > 0.0001) && (h.dcyc < 0.999) && (h.sw > 0.1))
      h.stepsize = 1000000.0/h.sw; 
    if (h.np)				/* re-set the "local" dres */
    { 
      for (i = 0; i < h.itnf; i++)
      {
        if (Sh[i].sqw)
        {
          k = npsqw(&Sh[i]); 
	  Sh[i].dres *= (double) k / h.np; 
        }
      }
    }
    if (h.tfl < 0)	 			/* set the "global" dres */
    {
      if (h.dres < g.drmin)
      { 
        tm = 10000.0;
        for (i = 0; i < h.itnf; i++) 
        {
          if (Sh[i].dres >= g.drmin)		 /* find min non-0 dres */
          tm = MIN(tm, Sh[i].dres);
        }
        if (tm < 10000.0)
          h.dres = pboxRound(tm, g.drmin);
        else 
          h.dres = g.dres;
      }
      else if (h.np < 1)
      {
        for (i = 0; i < h.itnf; i++) 
          Sh[i].dres = h.dres;		 
      } 
    }
    if (h.dres > 180.0) 
      pxout("Warning : dres exceeds 180 deg\n", 1); 
  }

  for (i = 0; i < h.itnf; i++)
  {
    Wv[i].d1 += Wv[i].pw * Sh[i].d1;
    Wv[i].d2 += Wv[i].pw * Sh[i].d2; 
  }
/* ------------------------------ Start attn loop --------------------------- */

  if ((h.attn.flg > 0) || (h.attn.u == 'd')) itr = g.maxitr; /* # of iterations */
  else itr = 1;
  nitr = 0; attf = 1.0; inp = h.np;  
  do					                  /* attn loop */
  {
    h.pw = 0.0;					        /* Positioning */
    for (i = 0; i < h.itnf; i++)
    {  
      if (Sh[i].set > 0)                            /* skip RDC shapes */
      {
        if ((Wv[i].apflg) && (i > 0)) Wv[i].d0 = Wv[i-1].pwt; 
        if (strcmp(Sh[i].fm, "fslg") == 0) 
          Wv[i].pw *= attf;  
        else if ((Wv[i].oflg == 't') || (Wv[i].oflg == 'T'))
          Wv[i].of /= attf;
        else
          Wv[i].pw *= attf;
        if ((Sh[i].adb > 0) && (Sh[i].ffl))     /* keep the bw for adb shapes */
        {
          Sh[i].pwb *= attf; 
          Sh[i].pwsw *= attf; 
          Wv[i].pwb *= attf; 
        }
        Wv[i].pwt = Wv[i].pw + Wv[i].d0 + Wv[i].d1 + Wv[i].d2;  
        h.pw = MAX(h.pw, Wv[i].pwt);
        if ((Sh[i].stch > 0.0) && (strcmp(Sh[i].fm, "n") != 0)) 
          itr = 1;	             /* attn disabled for stretched FM pulses */
      }
    }

    if(h.stepsize >= g.minpw) 
      plu = h.stepsize; 
    else       				         /* find the minimum stepsize */
    {
      if(h.pw > 0.0) 
        plu = 1000000.0*h.pw; 
      else 
        plu = 5000.0;
        
      minnumb = 1; h.np = inp; 
      for (i = 0; i < h.itnf; i++)  			/* find minnumb */
      {     
        nn = 0;     
        if (Sh[i].set == 1)                             /* skip RDC shapes */
        { 
          if (h.np < 1) 
          {
            if (Sh[i].dnp > 0) h.np = Sh[i].dnp;
            else h.np = g.defnp; 
          } 
          if (Sh[i].adb > 0.001)
            tm = (fabs(Wv[i].of) + fabs(0.5 * Wv[i].bw)) / fabs(h.maxincr);
          else
            tm = fabs(Wv[i].of / h.maxincr);
          if (Wv[i].oflg == 'T') tm = 0.0;
          nn = (int) (0.01 + pboxRound((Wv[i].pwt * 360.0 * tm), 1.0)); 
          if (Sh[i].sqw)
          {
            if (Sh[i].dres > 0.001)	
	      h.np = 0;
            if (i == 0)
	      minnumb = 1;
            k = npsqw(&Sh[i]); 
	    nn = MAX(minnumb, nn);
            if (nn > k)
            {
	      Sh[i].dres *= (double) k / nn;
              nn = npsqw(&Sh[i]); 
            }
            else nn = k;
          }
          else
            nn += minnmb(&Sh[i], h.maxincr); 
/*          nn = MAX(nn, Sh[i].dnp);     */
        }
        else if (Sh[i].set == 2)                       /* NULL shapes */
          nn = 1;
        minnumb = MAX(minnumb, nn);
        if (nn > 0) 
        {  
          tm = 1000000.0*Wv[i].pwt/((double) nn);  
          plu = MIN(plu, tm);  
        }
      }
    }

    nn = (int) (h.pw * 1000000.0 / plu);
    if (h.np > nn)                          /* override if steps > minnumb */
      plu = (h.pw * 1000000.0 / h.np);       

    plu = pboxRound(plu, g.tmres);
    if ((h.tfl > 0) && (plu < g.minpwg))
    {
      plu = g.minpwg;
      sprintf(e_str, " Step size reset to minimum step duration (%5.3f us)\n", 
              g.minpwg);
      pxout(e_str, 1);
    }
    else if (plu < g.minpw)
    {
      plu = g.minpw;
      sprintf(e_str, " Step size reset to minimum step duration (%5.3f us)\n", 
              g.minpw);
      pxout(e_str, 1);
    }

    h.np = (int) (h.pw * 1000000.0 / plu); 
    h.dnp = (double) h.np;
    pls = plu / 1000000.0;
    if ((h.stepsize >= g.minpw) || (h.stepsize > 0.001))
    {
      h.stepsize = plu;
      sprintf(h.stepsize_,"%.2f", plu); 
    }    
    
    nn = 0;
    for (i = 0; i < h.itnf; i++)
    {
      if (Sh[i].set > 0)                           /* skip RDC shapes */
      {
        if (Sh[i].sqw)
        {
          Wv[i].np = adjsqw(&Sh[i]); 
          h.np = 0;
        }
        else
          Wv[i].np = pboxRound(Wv[i].pw / pls, 1.0); 
        if (strcmp(Sh[i].fm, "fslg") == 0)
        {
          Wv[i].np = 2*((Wv[i].np+1)/2);
/*        tm = 0.5*Sh[i].pwb;
          tm = pboxRound(360.0*Sh[i].pwb/(double) Wv[i].np, 1.0); * phi *
          tm = pboxRound((double) Wv[i].np*tm/360.0, 1.0);        * Beff *
          Wv[i].np = pboxRound(180.0*Sh[i].pwb/tm, 2.0); 
FSLG -  one of the versions disabled here. EK.
*/
        }
        if (Wv[i].d0 > pls)
        { 
          Wv[i].apflg = pboxRound(Wv[i].d0 / pls, 1.0);
          h.posflg = 0;
        }
        if (Wv[i].d1 > pls)
        { 
          Wv[i].n1 = pboxRound(Wv[i].d1 / pls, 1.0);
          h.posflg = 0;
        }
        if (Wv[i].d2 > pls) 
        { 
          Wv[i].n2 = pboxRound(Wv[i].d2 / pls, 1.0);
          h.posflg = 0;
        }
        Wv[i].npt = Wv[i].np + Wv[i].n1 + Wv[i].n2;
        nn = MAX(nn, Wv[i].npt + Wv[i].apflg);
      }
    }
    h.np = MAX(h.np, nn); 
    h.dnp = (double) h.np; 
    h.pw = h.dnp * pls;

    mindelta = pi2, maxdelta = 0.0;
    for (i = 0; i < h.itnf; i++)		  /* ph incr & bw calc */
    {
      if (Sh[i].set > 0)                           /* skip RDC shapes */
      {
/*      
        if (strcmp(Sh[i].fm, "fslg") == 0) 	* special for FSLG-dec *
          Wv[i].of = 0.0;
*/          
        if (Wv[i].stflg == 0) 
        {
          Wv[i].st = Sh[i].st;  /* reset status */
          Wv[i].trev += (int) (Wv[i].st + 0.5);
          Wv[i].trev = abs(Wv[i].trev%2);             /* trev 0 or 1 */
        }

        if (Wv[i].php < 0.0) 
        {
          j = (int) (Wv[i].st + 0.5);                   /* set phase pivot */
          j = abs(j%2);
          Wv[i].php = (double) j; 
          j = (int) (0.5 + Sh[i].fla/90.0);
          j = abs((j - 2)%4);
          if (j == 0) Wv[i].php *= 0.5;  
        }
        if (Wv[i].php > 1.0) Wv[i].php = 0.0;
        else if (Wv[i].php < 0.0) Wv[i].php = 0.0;

        Wv[i].incr = pi2 * Wv[i].of * pls;
        if (Wv[i].oflg == 'T') Wv[i].incr = 0.0;
        Wv[i].pw = (double) Wv[i].np * pls;
        Wv[i].bw = fabs(Sh[i].pwb / Wv[i].pw);
        maxdelta = MAX(maxdelta, fabs(Wv[i].incr));
        mindelta = MIN(mindelta, fabs(Wv[i].incr));
        Wv[i].ph0 = 0.5 * Wv[i].incr + rd*Wv[i].ph; /* phase ph0 */
        if (h.posflg > 0)	  /* positioning */
        {
          if (!Wv[i].sfl)
          {
            Wv[i].d0 = (double) (h.np - Wv[i].np) * pls * (1.0 - Wv[i].php);
            Wv[i].n1 = pboxRound(Wv[i].d0 / pls, 1.0);
            Wv[i].n2 = h.np - Wv[i].np - Wv[i].n1;
            Wv[i].npt = h.np;
          }
          Wv[i].ph0 -= ((double) Wv[i].npt) * Wv[i].incr * (1.0 - Wv[i].php);
        } 
        else if (h.posflg < 0)  /* not active */
          Wv[i].ph0 -= ((double) h.np) * Wv[i].incr * (1.0 - Wv[i].php);
        else
          Wv[i].ph0 -= (((double) Wv[i].np)*(1.0-Wv[i].php) + (double) Wv[i].n1)*Wv[i].incr;
      }
    }
 
    if(!nitr)
    {
      h.npst = 0;
      for (i = 0; i < h.itnf; i++)
      {
        if (Sh[i].set > 0)                           /* skip RDC shapes */
        {
          Wv[i].nps = 1;
          if (Wv[i].sfl > 0)
          {
            strcpy(str2, Wv[i].su);
            while (cutstr(str1, str2))
            {
              for (k=0; k< h.itns; k++)
              {  
                if (strcmp(str1, su[k].sh) == 0)
                {
                  Wv[i].nps *= su[k].dnp; 
                  break;
                }
              }
            }
          }
          Wv[i].npst = Wv[i].apflg + Wv[i].nps * (Wv[i].npt);
          h.npst = MAX(h.npst, Wv[i].npst);
        }
      }
    }
    else
    {
      h.npst = 0;
      for (i = 0; i < h.itnf; i++)
      {
        if (Sh[i].set > 0)                           /* skip RDC shapes */
        {
          Wv[i].npst = Wv[i].apflg + Wv[i].nps * (Wv[i].npt);
          h.npst = MAX(h.npst, Wv[i].npst);
        }
      }
    }

    if (h.npst > g.maxst)
    {
      printf("hnpst = %d\n", h.npst);
      sprintf(e_str, "total number of steps exceeds %d !!", g.maxst); 
      pxerr(e_str);
    }

    if ((h.itns) && (!nitr))				/* setup supercycles */
    {
      Sam = (double **) calloc(h.itns, sizeof(double *));
      Sph = (double **) calloc(h.itns, sizeof(double *));
      for (i = 0; i < h.itns; i++)
      {
        Sam[i] = arry(su[i].dnp); 
        Sph[i] = arry(su[i].dnp);
        getsu(&su[i], &Sam[i], &Sph[i]); 
      }
    }
  
    if (h.bscor[0] != 'n')		/* allocate memory for BS compensation */
    {
      Wam = (double **) calloc(h.itnf, sizeof(double *));
      Wph = (double **) calloc(h.itnf, sizeof(double *));
      Wbs = (double **) calloc(h.itnf, sizeof(double *));
      for (i = 0; i < h.itnf; i++)
      {
        Wam[i] = arry(h.npst+200); 
        Wph[i] = arry(h.npst+200);
        Wbs[i] = arry(h.npst+200);
      }
    }

/* ------------------------ Start Shaping --------------------------- */ 

  npmax = h.npst+200;
  Re = arry(npmax);
  Im = arry(npmax);
  
  npmax += (h.pad1 + h.pad2);
  Amp = arry(npmax); 
  Pha = arry(npmax);

  h.itnw = 0, h.itnd = 0;
  							/* shaping */
  for (i = 0; i < h.itnf; i++)
  {
    for (j = 0; j < npmax; j++)
      Amp[j] = 0.0, Pha[j] = 0.0;

    if (Sh[i].set == 1)                                 /* skip RDC shapes */
    {
      if (Sh[i].stch > 0.001) 
        Sh[i].np = Wv[i].np / (1.0 + Sh[i].stch);
      else 
        Sh[i].np = Wv[i].np;				/* default np !!! */

      if (strcmp(Sh[i].fm, "ca") == 0) 
        Sh[i].np += 2; 

      qq = Sh[i].pb1;	    
      j = ampmod(&Sh[i]); 				/* AM */ 
      if (j<0) return 0;
      imod += j;

      if (isname(&Sh[i].wf))
      {
        if(wmult(&Sh[i], &wf[h.itnw]) == 0) return 0;	/* windowing */
        h.itnw++;
      }
      if (Wv[i].fla < 0.0)                              /* neg fla */
      {
        for (j = 0; j < Sh[i].np; j++)
          Amp[j] = -Amp[j];
      }
      if ((Sh[i].ffl) && (frqmod(&Sh[i])==0))		/* FM & PM */
        return 0; 
      if (Sh[i].stch > 0.001) 			        /* stretch */
      {
        stretch(&Sh[i], Wv[i].np - Sh[i].np);	
        Sh[i].np = Wv[i].np;	
      }
      if (Sh[i].srev != 0.0) 			        /* sweep rev */
        swrev(Sh[i].srev, Sh[i].np);
      if ((Sh[i].max < 1.0) && (Sh[i].max > 0.0)) 	/* clip ampl */
        clip(Sh[i].np, Sh[i].max);	
      if (Sh[i].co > 0) 				/* re or im */
      {
        Sh[i].pb1 = 0.0;				
        for (j = 0; j < Sh[i].np; j++) 
        {
          Amp[j] *= cos(Pha[j]);
          Pha[j] = 0.0;
        }
      }
      else if (Sh[i].co < 0) 
      {
        Sh[i].pb1 = 0.0;
        for (j = 0; j < Sh[i].np; j++) 
        {
          Amp[j] *= sin(Pha[j]);
          Pha[j] = 0.0;
        }
      }
      if (Sh[i].dc) 
        rmdc(Sh[i].np);				/* remove dc */
      if (isname(&Sh[i].df))
      {
        df[h.itnd].np = Sh[i].np;
        if(dfil(&df[h.itnd])==0) return 0; 	/* digital filtering */
        h.itnd++;
      }
      mxl = maxval(Sh[i].np, Amp);			/* scaling */
      for (j = 0; j < Sh[i].np; j++)
        Amp[j] /= mxl;

      if (h.tfl > 0)
      {
        for (j = 0; j < Sh[i].np; j++)
	  Amp[j] *= g.maxgr * fabs(Wv[i].fla) / 100.0;
      }
      else
      {
        /* if ((Sh[i].pb1 < 0.001) && */
        if ((Wv[i].oflg == 't') || (Wv[i].oflg == 'T'))    /* off-resonance SL */
        {
          Sh[i].pb1 = fabs(Wv[i].pw*Wv[i].of*tan(rd*Wv[i].fla));
          if (strcmp(Sh[i].fm, "fslg") == 0) 	/* special for FSLG-dec */
            Sh[i].pb1 = fabs(Wv[i].pw*Wv[i].bw*tan(rd*Wv[i].fla));
        }
        else
        {
          if (qq > 0.001) 
            Sh[i].pb1 = qq; 
          else if ((Sh[i].pb1 < 0.001) && (h.cal))         /* pulse area theorem */
          {
            Sh[i].pb1 = calib(Sh[i].of, Sh[i].np);
            Sh[i].pb1 *= (fabs(Wv[i].fla) / 360.0);
          }
          else if ((Wv[i].fla > 0.01) && (Sh[i].fla > 0.01))
            Sh[i].pb1 *= Wv[i].fla / Sh[i].fla;
        }
        if (Sh[i].pb1 < 0.001)
        {
          sprintf(e_str, "pw*b1 = %8.5f\n", Sh[i].pb1);
	  sprintf(ifn, "Problems calibrating pulse # %d, %s\n", i+1, Wv[i].sh);
          pxout(strcat(e_str, ifn), -1);
	  h.cal = 0;
        }
        else qq = Sh[i].pb1 / Wv[i].pw;
        if (h.cal) 
        { 
          for (j = 0; j < Sh[i].np; j++) 
            Amp[j] *= qq; 
        }
      }

      if(Wv[i].trev > 0)			        	/* time rev */
      {
        tmrev(&Amp, Sh[i].np);
        tmrev(&Pha, Sh[i].np); 
      }

      if(Wv[i].n1 > 0)					/* positioning */
      {
        for (j = Wv[i].np - 1, k = Wv[i].np + Wv[i].n1 - 1; j >= 0; j--, k--)
        {
          Amp[k] = Amp[j];
          Pha[k] = Pha[j];
          Amp[j] = 0.0, Pha[j] = 0.0;
        }
      }
      
      if (Wv[i].sfl > 0)				/* supercycling */
      {
        strcpy(str2, Wv[i].su);
        while (cutstr(str1, str2))
        {
          for (k=0; k< h.itns; k++)
          {  
            if (strcmp(str1, su[k].sh) == 0)
            {
              if ((Wv[i].oflg == 'r') && (su[k].dnp > 1))  /* add plane rotation angle */
                Sph[k][0] += (0.5*Wv[i].fla*rd); 
              for (m = 0, j = 0; m < su[k].dnp; m++)
              {
                for (n = 0; n < Wv[i].npt; n++, j++)
                  Pha[j] = Pha[n], Amp[j] = Amp[n]; 
              }
              for (m = 0, j = 0; m < su[k].dnp; m++)
              {
                for (n = 0; n < Wv[i].npt; n++, j++)
                {
                  Pha[j] += Sph[k][m]; 
                  Amp[j] *= Sam[k][m];
                }
              }
              Wv[i].npt *= su[k].dnp;
              break;
            }
          }
        }
      }
      h.np = MAX(h.np, Wv[i].npt + Wv[i].apflg);

      if((Sh[i].wr+Wv[i].wrp) > 0.0) 
      {
        if (!(wrap((Sh[i].wr+Wv[i].wrp), Wv[i].npt)))
        {
          sprintf(e_str, "Warning ! Wave #%d wrap-around ignored\n", i);  /* wrap */
          pxout(e_str, -1);
        }
      }

      if (Wv[i].apflg > 0)		                  /* appending */
      {
        for (j = Wv[i].npt - 1, k = Wv[i].npt + Wv[i].apflg - 1; j >= 0; j--, k--)
        {
          Amp[k] = Amp[j];
          Pha[k] = Pha[j];
          Amp[j] = 0.0, Pha[j] = 0.0;
        }
        Wv[i].npt += Wv[i].apflg;
      }

      if (h.bscor[0] != 'n')				  /* BS compensation */
      {
        for (j = 0; j < Wv[i].npt; j++)       /* freq shift and ph0 */
        {    
          Pha[j] += Wv[i].ph0 + (double) j * Wv[i].incr;
          Wph[i][j] = Pha[j];
          Wam[i][j] = Amp[j]; 
          Wbs[i][j] = 0.0;  /* amp in Hz */
        }
      }					  
      else
      {				  
        for (j = 0; j < Wv[i].npt; j++)       /* freq shift, ph0 and mixing */
        {
          Pha[j] += Wv[i].ph0 + (double) j * Wv[i].incr;
          Re[j] += Amp[j] * cos(Pha[j]);
          Im[j] += Amp[j] * sin(Pha[j]);
        }
      }
    }
    else if (Sh[i].set == 2)
    {
      Sh[i].np = Wv[i].np;			/* overwrite default np !!! */
      if (Wv[i].apflg > 0) 		   	/* appending */
        Wv[i].npt += Wv[i].apflg;
    }
  }

  if (h.bscor[0] != 'n')		/* mixing and BS compensation */
  {
    for (i = 0; i < h.itnf; i++)
    {
      if (Sh[i].set == 1)                           /* skip NULL & RDC shapes */
      {
        for (j = 1; j < Wv[i].npt; j++)       
          Wbs[i][j] = (Wph[i][j]-Wph[i][j-1])/(pi2*pls);   
      }					  
    }
    for (i = 0; i < h.itnf; i++)
    {
      if (Sh[i].set == 1)                           /* skip NULL & RDC shapes */
      {
        for (j = 1; j < Wv[i].npt; j++)
        {       
          sum = 0.0;    /* average frequency */
          for (k = 0; k < h.itnf; k++)
          {
            if (i!= k)
            {
              tm = Wbs[i][j] - Wbs[k][j];
              mxl = sqrt(tm*tm + Wam[k][j]*Wam[k][j]);
              if (tm > 0.0) 
                sum += (mxl-tm);
              else
                sum -= (mxl+tm);               
            }
          }
          Pha[j] = sum*pi2*pls; 
        }
        Pha[0] = 0.0; 
        for (j = 1; j < Wv[i].npt; j++)
          Pha[j] += Pha[j-1];
        k = (int) ((1.0-Wv[i].php)*((double) (h.np-1)));  
        for (j = 0; j < Wv[i].npt; j++)      /* BS phase correction for status */
          Wph[i][j] += (Pha[j] - Pha[k]); 
      }
    }
    
    for (i = 0; i < h.itnf; i++)
    {
      if (Sh[i].set == 1)                           /* skip NULL & RDC shapes */
      {
        for (j = 0; j < Wv[i].npt; j++) 
        {      
          Re[j] += Wam[i][j] * cos(Wph[i][j]);
          Im[j] += Wam[i][j] * sin(Wph[i][j]);
        }
      }	
    }				  					  
  }

  if (h.npst != h.np)
  { 
    printf("\nWarning : Supercycle failure, %d steps expected\n\n", h.npst);
    printf("nps = %d, np = %d\n", h.npst, h.np);
  }
  
/* --------------------------- Shaping Finished ---------------------------- */

  mxl = 0.0;
  if (h.tfl < 1)
  {
    for (j = 0; j < h.np; j++)				/* find Amp and Pha */
    {
      if ((Re[j] != 0.0) || (Im[j] != 0.0))
      {
	  Amp[j] = sqrt(Re[j] * Re[j] + Im[j] * Im[j]);   
	  Pha[j] = atan2(Im[j], Re[j]);	        
	  mxl = MAX(mxl, Amp[j]);                       /* mxl is used below ! */
      }
      else
      {
        Amp[j] = Re[j];
        Pha[j] = Im[j];
      }
      Pha[j] *= dg;                               /* convert Pha to degrees */
    }

    if (h.cal) b1max = 0.001 * mxl;			/* B1max in kHz */
    B1max = b1max; 

    if ((h.attn.flg > 0) || (h.attn.u == 'd'))	/* attenuation */    
    {
      dumm = 0.0; 
      if ((h.attn.u == 'd') || (h.attn.u == 'H')) 
      {
        if (h.pw90 < 0.01)
        {
          pxerr("RF calibration data are missing !");
          return 0;
        }
        if (h.attn.flg == 0)          /* ??? */
        {
          h.attn.mgn = DCB(b1max, h.Bref); 
          h.attn.mgn = pboxRound(h.attn.mgn, 1.0) + h.rfpwr;
        }
        dumm = ATTN((double) (h.rfpwr - h.attn.mgn));
        dumm = h.Bref / dumm; 
      }
      else dumm = h.attn.mgn; 
      if (dumm > 0.01) attf = b1max / dumm; 
      else itr = 1;
      if (h.reps > 2)
        printf("  B1max = %f, target = %f, error = %f\n", b1max, dumm, b1max - dumm); 
      if (fabs(1.0 - attf) < (g.maxdev / 100.0)) itr = 1;
    }
  }

  itr--; nitr++;
  if ((itr) && (h.reps > 2)) 
  {
    sprintf(e_str, "  Itteration : #%d\n", nitr);
    pxout(e_str, 0);
  }
} while (itr);

  if(h.wrp > 0.0) 
  {
    if (!(wrap(h.wrp, h.np)))
    {
      sprintf(e_str, "Warning ! Shape wrap-around ignored\n");  /* wrap */
      pxout(e_str, -1);
    }
  }

  MaxIncr=dg*maxdelta;
/* ----------------------- Report Parameters ---------------------------- */

  if (h.reps > 2)
  {
    if (h.tfl > 0)
    {
      printf("   sh       pw        ofs   st  ph  su  lev(%s) ", "%");
      if (Wv[i].php > 0.0) printf("trev   d1   d2   d0   wrp  php\n");
      else if (Wv[i].wrp > 0.0) printf("trev   d1   d2   d0   wrp\n");
      else if ((Wv[i].d0 - Wv[i].d1) > 0.0) printf("trev  d1  d2  d0\n");
      else if (Wv[i].d2 > 0.0) printf("trev  d1  d2\n");
      else if (Wv[i].d1 > 0.0) printf("trev  d1\n");
      else printf("trev\n");
      for (i = 0; i < h.itnf; i++)
      {
	printf("  %s", Wv[i].sh);
	if (Wv[i].dash >= 0.0)
	  printf("%d", (int) -Wv[i].dash);
	printf(" %10.7f %6.1f", Wv[i].pw, Wv[i].of); 
	printf(" %5.1f %2.0f %s %8.1f", Wv[i].st, Wv[i].ph, Wv[i].su, Wv[i].fla);
        if (Wv[i].php > 0.0) printf(" %d %.7f %.7f %.7f %.4f %.4f\n", 
          Wv[i].trev, Wv[i].d1, Wv[i].d2, (Wv[i].d0 - Wv[i].d1), Wv[i].wrp, Wv[i].php);
        else if (Wv[i].wrp > 0.0) printf(" %d %.7f %.7f %.7f %.4f\n", 
          Wv[i].trev, Wv[i].d1, Wv[i].d2, (Wv[i].d0 - Wv[i].d1), Wv[i].wrp);
        else if ((Wv[i].d0 - Wv[i].d1) > 0.0) printf(" %d %.7f %.7f %.7f\n", 
          Wv[i].trev, Wv[i].d1, Wv[i].d2, Wv[i].d0 - Wv[i].d1);
        else if (Wv[i].d2 > 0.0)
	  printf(" %d %.7f %.7f\n", Wv[i].trev, Wv[i].d1, Wv[i].d2);
        else if (Wv[i].d1 > 0.0)
	  printf(" %d %.7f\n", Wv[i].trev, Wv[i].d1);
        else
	  printf(" %d\n", Wv[i].trev);
      }
    }
    else
    {
      printf("   sh      bw      pw        ofs   st  ph  su  fla ");
      if (Wv[i].php > 1.0e-8) printf("trev   d1   d2   d0   wrp  php\n");
      else if (Wv[i].wrp > 1.0e-8) printf("trev   d1   d2   d0   wrp\n");
      else if ((Wv[i].d0 - Wv[i].d1) > 1.0e-8) printf("trev  d1  d2  d0\n");
      else if (Wv[i].d2 > 1.0e-8) printf("trev  d1  d2\n");
      else if (Wv[i].d1 > 1.0e-8) printf("trev  d1\n");
      else printf("trev\n");
      for (i = 0; i < h.itnf; i++)
      {
	printf(" %s", Wv[i].sh);
	if (Wv[i].dash >= 0.0)
	  printf("%d", (int) -Wv[i].dash);
	printf("%8.2f %8.6f %6.1f", Wv[i].bw, Wv[i].pw, Wv[i].of);
	printf(" %5.1f %2.0f  %s %5.1f", Wv[i].st, Wv[i].ph, Wv[i].su, Wv[i].fla);
        if (Wv[i].php > 0.0) printf(" %d %.7f %.7f %.7f %.4f %.4f\n", Wv[i].trev, 
          Wv[i].d1, Wv[i].d2, (Wv[i].d0 - Wv[i].d1), Wv[i].wrp, Wv[i].php);
        else if (Wv[i].wrp > 0.0) printf(" %d %.7f %.7f %.7f %.4f\n", 
          Wv[i].trev, Wv[i].d1, Wv[i].d2, (Wv[i].d0 - Wv[i].d1), Wv[i].wrp);
        else if ((Wv[i].d0 - Wv[i].d1) > 0.0) printf(" %d %.7f %.7f %.7f\n", 
          Wv[i].trev, Wv[i].d1, Wv[i].d2, Wv[i].d0 - Wv[i].d1);
        else if (Wv[i].d2 > 0.0)
	  printf(" %d %.7f %.7f\n", Wv[i].trev, Wv[i].d1, Wv[i].d2);
        else if (Wv[i].d1 > 0.0)
	  printf(" %d %.7f\n", Wv[i].trev, Wv[i].d1);
        else
	  printf(" %d\n", Wv[i].trev);
      }
    }
  }
  if (h.reps == 4)
  {
    h.itnw = 0, h.itnd = 0;
    for (i = 0; i < h.itnf; i++)
    {
      printf("\nWaveform # %d, %s :\n", i+1, Wv[i].sh);
      j = strlen(Wv[i].sh);
      printf("================"); 
      for(k=0; k<j; k++) printf("="); printf("\n");
      reportwave(&Wv[i], h.reps);
      printf("\n");
      if(Sh[i].set == 1)
        printcoms(Wv[i].dir, Wv[i].sh, 'h');
      else
        printf("\n# reserved shape name : %s, (id = %d)\n", Wv[i].sh, Sh[i].set);
      reportshape(&Sh[i]);
    }
    for (i = 0; i < h.itns; i++)
    {
      printf("\nSupercycle # %d, %s :\n", i+1, su[i].sh);
      printf("---------------------");
      reportshape(&su[i]);
    }
    for (i = 0; i < h.itnw; i++)
    {
      printf("\nWindow Function :\n");
      printf("-----------------\n");
      reportwindow(&wf[i]);
    }
    for (i = 0; i < h.itnd; i++)
    {
      printf("\nDigital Filter :\n");
      printf("----------------\n");
      reportwindow(&df[i]);
    }
    printf("\n");
  }
  if (h.reps > 2)
  {
    printf("\n  # of steps=%d, min # of steps=%d\n", h.np, minnumb);
    printf("  pulse width %.4f ms", h.pw * 1000.0);
    printf(", length of a single step %.2f us\n", plu);
    if (maxdelta != 0.0)
    {
      if (mindelta != maxdelta)
	printf("  ph incr: min=%.2f, max=%.2f deg\n",
		dg * mindelta, dg * maxdelta);
      else
	printf("  maximum phase increment %.2f deg\n", dg * maxdelta);
    }
    if (h.coms[0] == 'n')
      printf("  - comments in pulse file suppressed - \n");
  }
  if ((h.reps > 2) && (h.sfl > 0))
    printf("  %s supercycling superimposed \n", h.su);

/* ----------------------- Reports Finished ---------------------------- */

  itrerr = 100.0 * fabs(1.0 - attf);

  bl.sign = 0;					/* Set Bloch simulator */
  bl.time = g.simtime; 
  if ((h.bls[0] == 'n') || (h.bls[1] == 'f'))
    bl.time = -1.0;
  else if (h.bls[0] == 'a')
  {
    bl.time = 24 * 3600;
    bl.sign = 1;
  }
  else if (h.bls[0] == 's')
  {
    bl.time = 24 * 3600;
    bl.sign = -1;
  }
  else if ((isdigit(h.bls[0])) && ((bl.time = stoi(h.bls)) < 1))
    bl.time = 24 * 3600;

  if (h.tfl < 1)
  {
    bl.B1max = b1max*1000.0;
    bl.np = h.np;
    bl.reps = h.reps;
    bl.sw = h.sw;
    if (bl.sw < 0.1)
      bl.sw = 6.0 * Wv[h.imax].bw + 2.0 * h.maxtof;
    bl.T1 = h.T1;
    bl.T2 = h.T2;
    bl.pw = (double) h.np*pls;
    bl.nstp = (int) (6.0 * bl.pw * bl.sw);
    if (bl.nstp < g.minstps) bl.nstp = g.minstps;
   
    i=0; 
    while(i < h.itnf)
    {
      if (Sh[i].set > 0) 
      {
        bl.st = Wv[i].st;
        i = h.itnf;
      }
      i++;
    }

    tm = g.minpw*0.000001; 
    for (i=0; i< h.itnf; i++)              /* Radiation Damping Compensation */
    {
      if (Sh[i].set == -1) 
      {
        if (Wv[i].bw < 0.0)
          Wv[i].bw = -Wv[i].bw;
        if ((Wv[i].bw < tm) && (Wv[i].pw > tm))
          Wv[i].bw = (1.0 / Wv[i].pw);
        mxl = 0.0;
        bl.ofs = Wv[i].of; 
        bl.st = Wv[i].st;        
        blqtph(&bl); 
        dumm = 0.5*Wv[i].bw;  /* R in Hz, or 0.5*Wv[i].bw/1.6768, if linewidth */
        if ((h.rdc_[0] == 'y') && (bl.time > 0))     
        {
          h.calflg = 1;
          for (j = 0; j < h.np; j++)				
          {
            Re[j] -= (dumm*(bl.Mx[j] + bl.Mx[j+1]));  /* calculate the average */
            Im[j] -= (dumm*(bl.My[j] + bl.My[j+1]));
          }
        }
        else
        {
          for (j = 0; j < h.np; j++)				
          {
            Re[j] += (dumm*(bl.Mx[j] + bl.Mx[j+1]));  /* calculate the average */
            Im[j] += (dumm*(bl.My[j] + bl.My[j+1]));
          }
        }
      }
    }   
    if (mxl < 0.001)
    {
      bl.ofs = 0.0;     
      for (j = 0; j < h.np; j++)	                /* reset Amp and Pha */
      {
        if ((Re[j] != 0.0) || (Im[j] != 0.0))
        {
	    Amp[j] = sqrt(Re[j] * Re[j] + Im[j] * Im[j]);
	    Pha[j] = atan2(Im[j], Re[j]);	                
	    mxl = MAX(mxl, Amp[j]);
        }
        else
        {
          Amp[j] = Re[j];
          Pha[j] = Im[j];
        }
        Pha[j] *= dg;                                /* convert Pha to degrees */
      }
      if (h.cal) b1max = 0.001 * mxl;			       /* B1max in kHz */
      B1max = b1max; 
    }                             /* Finished - Radiation Damping Compensation */
  }

  if (h.rdc_[0] == 'y')
  {
    if (bl.time > 0)
      h.calflg = 1;
    else
      pxerr("\nrdc parameter is only active if bsim = y\n\n");
  }      
  if (h.rdc < 0.0)
    h.rdc = -h.rdc;
  if ((h.rdc < 1.0) && (h.rdc > (g.minpw*0.000001)))
    h.rdc = 1.0 / h.rdc;
  if ((h.rdc > 0.5) && (bl.time <= 0) && (h.reps > 1))
    printf("\nPbox warning : rdc parameter is only active if bsim = y\n\n");
  bl.rdc = h.rdc;
      
  if(h.pad1 > 0)                                               /* zero padding */
  {
    for (j = h.np-1; j > -1; j--)
    {
      Amp[j+h.pad1] = Amp[j];
      Pha[j+h.pad1] = Pha[j];
    }
    for (j = 0; j < h.pad1; j++)
    {
      Amp[j] = 0.0;
      Pha[j] = 0.0;
    } 
    h.np += h.pad1;   
  }
  if(h.pad2 > 0)
  {
    for (j = h.np; j < h.np+h.pad2; j++)
    {
      Amp[j] = 0.0;
      Pha[j] = 0.0;
    } 
    h.np += h.pad2;   
  }

  h.pw = (double) h.np*pls;
  bl.np = h.np;
  bl.pw = h.pw;

  if (h.tfl < 0)
  {
    h.dres = pboxRound(h.dres, g.drmin); 
    if (h.dres < g.drmin)
      h.dres = g.drmin;
    h.dmf = h.dres / (pls * 90.0); 
  }
  else if (h.tfl > 0)                   /* integrate .GRD shapes */
  {
    strcpy(h.bls, "off");		/* Bloch Sim. disabled for *.GRD files */
    sum = 0.0;
    for (j = 0; j < h.np; j++)
      sum += (fabs(Amp[j]) / MaxAmp);   /* area factor. Do we need an integral too ? */
    h.dmf = sum / h.np;
  }
  
  if (h.tfl < 1)
  {
    Pfact = 0.0;
    if (mxl > 0.0)
    {
      for (j = 0; j < h.np; j++)				/* power factor */
      {
        dumm = Amp[j] / mxl;
        Pfact += dumm * dumm;
      }
      Pfact /= (double) h.np;
    }
    else
      Pfact = 0.0;
    if ((h.mod[0] == 'c') || (h.mod[0] == 's')) /* Mercury (DANTE) implementation */
    {
      h.dcyc -= (g.ipdel/plu);	     /* ipdel - interpulse delay, in usec */
      if (h.mod[0] == 'c') 
      {
        edB = 1.0 - 1.0 / exp(0.05*M_LN10);    /* extra dB */
        h.dcyc -= 0.5*(1.0 + edB);
      }
      if((h.dcyc*plu) < g.minpw)
      {
        printf("Low duty cycle : %.6f ! Aborting ... \n", h.dcyc);
        exit(0);
      }
      h.pwrf = 0;
    }
    
    B1rms = b1max*sqrt(Pfact)/sqrt(h.dcyc);

    if ((h.reps > 2) && (h.tfl < 1))
      printf("  power factor %6.4f, B1max: %.4f kHz\n", Pfact, b1max);

    if (h.cal)
    {
      if ((h.pw90 > 0.001) && (b1max > 0.0))
      {							/* scaling */
        dumm = DCB(b1max, (h.dcyc * h.Bref));
        if (h.attn.u == 'd')
          h.pwr = h.rfpwr + pboxRound(dumm, 1.0);
        else if (fabs(dumm) > 0.001)
          h.pwr = (int) (h.rfpwr + dumm + 1.0);
        else h.pwr = h.rfpwr;
	  if (h.pwr < g.minpwr) 
	    h.pwr = g.minpwr; 
	  else if ((h.attn.u == 'E') || (h.attn.u == 'I'))
	    h.pwr = h.attn.mgn;
        dumm = ATTN((double) (h.rfpwr - h.pwr));
        B1max = h.dcyc * h.Bref / dumm;
        if (h.attn.u != 'd')
	    MaxAmp *= b1max / B1max;
        if ((h.attn.u == 'e') || (h.attn.u == 'E') || (h.attn.u == 'h') || 
            (h.attn.u == 'k') || (h.attn.u == 'H'))
        {
          h.pwrf = (int) pboxRound(4.0*MaxAmp, g.amres) - 1;
          MaxAmp = g.maxamp;
        }
        if (Pfact < 0.999)
        {
          dumm = DCB(B1rms, h.Bref);
          if (fabs(dumm) > 0.001) 
            RmsPwr = (int) (h.rfpwr + dumm + 1.0);
          else RmsPwr = (int) h.rfpwr;
        }
        else 
          RmsPwr=(int) h.pwr;
      }
      else
        h.pwrf = -1;
 
      if ((h.attn.u == 'h') || (h.attn.u == 'k') || (h.attn.u == 'H'))
      {
        for (j = 0; j < h.np; j++)
        {
	    Pha[j] = pboxRound(Pha[j], g.phres);
	    Amp[j] = pboxRound(Amp[j], g.amres);
        }
      }
      else if (mxl > 0.0)
      {
        for (j = 0; j < h.np; j++)
        {
	  Pha[j] = pboxRound(Pha[j], g.phres);
	  Amp[j] = pboxRound((MaxAmp * Amp[j] / mxl), g.amres);
        }
      }
    }
  }
  else
  {
    MaxAmp = maxval(h.np, Re);
    if ((MaxAmp > g.maxgr) || (h.attn.u == 'e') || (h.attn.u == 'E'))
    {
      if (MaxAmp != 0.0)
        tm = g.maxgr/MaxAmp;
      for (j = 0; j < h.np; j++) Re[j] *= tm;
    }
    if(h.tfl != 2)
    {
      for (j = 0; j < h.np; j++)
        Amp[j] = pboxRound(Re[j], g.amres);
    }
    MaxAmp = maxval(h.np, Amp);
  }

  if ((h.attn.u != 'e') && (h.attn.u != 'E') && (h.attn.u != 'h') && 
      (h.attn.u != 'k') && (h.attn.u != 'H') && (h.tfl < 1))
    MaxAmp = pboxRound(MaxAmp, g.amres);

  if (h.itns) 
    free(Sam), free(Sph);

  for (j = 0; j < h.np; j++)
    Pha[j] = scale(Pha[j]);
    
  (void) set_Stype(); h.stepsize = plu;  
  if (!h.calflg)
  { 						/* Open the file for output */
    if (h.tfl < 0)
      (void) strcat(h.fname, ".DEC");
    else if (h.tfl == 1)
      (void) strcat(h.fname, ".GRD");
    else if (h.tfl == 0)
      (void) strcat(h.fname, ".RF");

    if ((fil = fopen(h.fname, "w")) == NULL)
    {
      flerr(h.fname);
      return 0;
    }

    if (h.reps > 1)
    {
      sprintf(e_str, "Saving waveform to :\n \"%s\"... ", h.fname);
      pxout(e_str, -1);
    }
    if (h.coms[0] != 'n')
    {
      sscanf(REV, "%s %s\n", e_str, e_str);
      if (h.tfl < 1)
      {
        fprintf(fil, "# Pbox %9.2f %3d %4d %4.1f %5.0f %9.4f %.5f %5d\n", 1000000.0*h.pw, 
                        h.pwr, h.pwrf, h.dres, h.dmf, h.dcyc, B1max / h.dcyc, Stype);
        fprintf(fil, "# %s  pw (us)  pwr  pwrf  dres  dmf  dcyc   B1max (kHz)  Stype\n",
                        e_str);
        if (h.reps == 1) 
          printf("Pbox %9.2f %3d %4d %4.1f %5.0f %9.4f %.5f %5d\n", 1000000.0*h.pw, 
                        h.pwr, h.pwrf, h.dres, h.dmf, h.dcyc, B1max / h.dcyc, Stype);
      }
      else if (h.tfl > 0)
      {
        fprintf(fil, "# Pbox %9.6f  %5.0f   %9.6f  0 0 0  %d\n", h.pw, MaxAmp, h.dmf, Stype);
        fprintf(fil, "# %s  pw (s)   maxampl   area fact 0 0 0 Stype\n", e_str);
        if (h.reps == 1) 
          printf("Pbox %9.6f  %5.0f   %9.6f  0 0 0  %d\n", h.pw, MaxAmp, h.dmf, Stype);
      }
      fprintf(fil, "# ---------------------------------------------------------------\n");
      fprintf(fil, "# Pxsim %.7f %.4f %.3f %.1f %.7f %.7f %.2f\n",
                      bl.pw, bl.B1max, bl.st, bl.sw, bl.T1, bl.T2, bl.rdc);
      fprintf(fil, "# data: pw (s) B1max (Hz) status sw (Hz) T1 (s)   T2 (s) RDC (Hz)\n");
      fprintf(fil, "# ---------------------------------------------------------------\n");
      writeheader(fil, '#');
      for (i=0; i<h.itnf; i++)
      {
        if(strlen(Sh[i].modpar))
          fprintf(fil, "%s", Sh[i].modpar);
      }
  
      fprintf(fil, "###\n");
      if ((h.coms[0] == 'i') && (h.tfl < 1))
      {
        fprintf(fil, "# ********************* Imaging Header *******************\n");
        for (i=0; i< h.itnf; i++)
        {
          if (Sh[i].set > 0) 
          {
            printcoms(Wv[0].dir, Wv[0].sh, 't');
            fprintf(fil, "%s", Wv[0].dir);
            fprintf(fil, "# VERSION       8.2\n"); 
            fprintf(fil, "# TYPE          %s\n", h.ptype);
            if (Sh[0].ffl)
              fprintf(fil, "# MODULATION    adiabatic\n"); 
            else
              fprintf(fil, "# MODULATION    amplitude\n"); 
            if ((fabs(Wv[0].fla) > 170.0) && (fabs(Wv[0].fla) < 190.0))
            {
              fprintf(fil, "# EXCITEWIDTH   %6.3f\n", -1.0);
              fprintf(fil, "# INVERTWIDTH   %6.3f\n", Sh[0].pwb);
            }
            else
            {
              fprintf(fil, "# EXCITEWIDTH   %6.3f\n", Sh[0].pwb);
              fprintf(fil, "# INVERTWIDTH   %6.3f\n", -1.0);
            }
            if (Sh[0].ffl)
              fprintf(fil, "# INTEGRAL      %6.4f\n", -1.0);
            else
              fprintf(fil, "# INTEGRAL      %6.4f\n", Sh[0].fla / (360.0 * Sh[0].pb1));
          }
        }
        fprintf(fil, "# ********************************************************\n");
      }
      fprintf(fil, "# ------------------- Shape data ------------------\n");
      if(g.Console == 'm') fprintf(fil, "# Console    = Mercury\n");
      if (h.tfl < 1)
      {
        fprintf(fil, "# pwtot      = %.7f (total pw, s)\n", h.pw);
        if (h.pw90 > 0.001)
        {
	  fprintf(fil, "# power      = %d (power level, dB)\n", h.pwr);
          fprintf(fil, "# pwrf       = %d (fine power)\n", h.pwrf);
        }
        fprintf(fil, "# B1max      = %.4f (B1max, kHz)\n", B1max / h.dcyc);
	  if ((b1max - B1rms) > 0.0001)
        fprintf(fil, "# B1rms      = %.4f (B1[RMS], kHz)\n", B1rms);
        if (h.dcyc < 0.999)
	  fprintf(fil, "# duty cycle = %5.3f\n", h.dcyc);
        if (h.tfl < 0)
        {
	  fprintf(fil, "# dres       = %.1f deg\n", h.dres);
	  fprintf(fil, "# dmf        = %.0f\n", h.dmf);
        }
        fprintf(fil, "# nptot      = %d (total number of steps)\n", h.np);
        fprintf(fil, "# stepsize   = %.2f (length of a single step, us)\n", plu);
        fprintf(fil, "# maxamp     = %.1f (max amplitude)\n", MaxAmp);
      }
      if (h.tfl > 0)
      {
        fprintf(fil, "# nptot       = %d (total number of steps)\n", h.np);
        fprintf(fil, "# stepsize    = %.2f (length of a single step, us)\n", plu);
        fprintf(fil, "# maxamp      = %.1f (max amplitude)\n", MaxAmp);
        fprintf(fil, "# area factor = %.4f\n", h.dmf);
        if (h.sfl > 0)
          fprintf(fil, "# supercycle  = %s\n", h.su);
	fprintf(fil, "# ------------------------------------------------- \n");
        fprintf(fil, "#   amplitude   duration \n");
        fprintf(fil, "# ------------------------------------------------- \n");
      }
      else
      {
        if (mindelta != maxdelta)
	  fprintf(fil,
		"# minimum phase increment = %.2f deg\n", dg * mindelta);
        if (maxdelta != 0)
	  fprintf(fil,
		"# maximum phase increment = %.2f deg\n", dg * maxdelta);
        if (h.sfl > 0)
	  fprintf(fil, "# supercycling = %s\n", h.su);
        fprintf(fil, "# ------------------------------------------------- \n");
        if (h.tfl < 0)
	  fprintf(fil, "#      length       phase   amplitude   gate \n");
        else
	  fprintf(fil, "#      phase      amplitude   length     gate \n");
        fprintf(fil, "# ------------------------------------------------- \n");
      }
    }

    if(h.attn.flg < 0) 
      g.cmpr = 0;                                /* do not compress the shape */
    else if((h.tfl == 0) && (MaxAmp < 1024.0) &&      
      ((h.attn.u == 'i') || (h.attn.u == 'I')))  /* add the anti-rescaling line */
      fprintf(fil, "%12.2f %12.2f %8.1f %8d\n", 0.0, 1023.0, 0.0, 0);

    if (h.tfl < 0)
      dumm = h.dres;
    else
      dumm = 1.0;

    mxl = 360.0;
    if(((h.attn.u == 'h') || (h.attn.u == 'k') || (h.attn.u == 'H')) && (h.tfl < 1))
    {
      for (i=0, j=0; j < h.np; i++, j++)
      {
        sum = dumm;
        if (g.cmpr)	/* compress */
        {
          if (h.tfl < 0)
          {
	    while ((Amp[j] == Amp[j + 1]) && (Pha[j] == Pha[j + 1]) &&
	           (sum < 360.0) && (j + 1 < h.np))
	    {
	      sum += dumm;
	      j++;
	    }
          }
          else
          {
	    while ((Amp[j] == Amp[j + 1]) && (Pha[j] == Pha[j + 1]) &&
	           (sum < 255.0) && (j + 1 < h.np))
	    {
	      sum += dumm;
	      j++;
	    }
          }
        }
        if (h.tfl < 0)
          fprintf(fil, "%12.1f %12.3f %8.3f\n", sum, Pha[j], fabs(Amp[j]));
        else
          fprintf(fil, "%12.3f %12.3f %8.1f\n", Pha[j], fabs(Amp[j]), sum);
      }
    }
    else
    {
      for (i=0, j=0; j < h.np; i++, j++)
      {
        sum = dumm;
        if (g.cmpr)	/* compress */
        {
          if (h.tfl < 0)
          {
	      while ((Amp[j] == Amp[j + 1]) && (Pha[j] == Pha[j + 1]) &&
	             (sum < 360.0) && (j + 1 < h.np))
	      {
	        sum += dumm;
	        j++;
	      }
          }
          else
          {
	      while ((Amp[j] == Amp[j + 1]) && (Pha[j] == Pha[j + 1]) &&
	             (sum < 255.0) && (j + 1 < h.np))
	      {
	        sum += dumm;
	        j++;
	      }
          }
        }
        
        if (Amp[j] < 0.5) 
          gate = 0;
        else
          gate = 1;

        if (Amp[j] < 1.0) 
          mxl = 0.0;
        else
          mxl = Amp[j]-1.0;

        if (h.attn.flg < 0)          /* NULL pulses */
        {
          gate = 0; 
          if (h.attn.u == 'i')
            Amp[j] = 0.0, mxl = 0.0; 
        }
        
        if (h.tfl < 0)
          fprintf(fil, "%12.1f %12.3f %8.3f %8d\n", sum, Pha[j], mxl, gate);
        else if (h.tfl == 1)
          fprintf(fil, "%12.1f %8.1f\n", Amp[j], sum);
        else if (h.tfl == 2)
          fprintf(fil, "%16.8f\n", Amp[j]);
        else
          fprintf(fil, "%12.3f %12.3f %8.1f %8d\n", Pha[j], mxl, sum, gate);
      }
/*
      if ((h.tfl > 0) && (Amp[h.np-1] > 0.0))                   * disabled in 6.1d *
        fprintf(fil, "%12.1f %8.1f\n", 0.0, 1.0);		* turn off PFG *
*/
      if((h.tfl < 0) && (MaxAmp < 1024.0) && (h.attn.flg > -1) &&  
        ((h.attn.u == 'i') || (h.attn.u == 'I')))  /* .DEC anti-rescaling line */
        fprintf(fil, "%12.1f %12.3f %8.3f %8d\n", 0.0, 0.0, 1023.0, 0);
    }
    fclose(fil);					/* close the OUTfile */
  }							/* end if "cal" */
  (void) strcpy(fcl, shapes);
  (void) strcat(fcl, FN_CAL);		/* calibration file */
  if((fil=fopen(fcl, "w"))==NULL)
  {
    flerr(fcl);
    return 0;
  }
  fprintf(fil, "  %s\n", h.name);
  if (h.tfl > 0)
    fprintf(fil, "  %.6f\n  %.1f\n  %.4f\n", h.pw, MaxAmp, h.dmf);
  else if (h.pw90 > 0.001)
  {
    fprintf(fil, "  %.1f\n  %d\n  %d\n  %.1f\n  %.0f\n", 
                    h.pw * 1000000.0, h.pwr, h.pwrf, h.dres, h.dmf);
  }
  else if (h.pw > 0.0000001)
  {
    fprintf(fil, "  %.1f\n  0\n  -1\n  %.1f\n  %.0f\n", 
                    h.pw * 1000000.0, h.dres, h.dmf);
  }
  else
    fprintf(fil, "  0.0\n  0\n  -1\n  0.0\n  0\n");
  fclose(fil);
  
  if (!h.calflg)
  {
    (void) strcpy(fnm, shapes);
    (void) strcat(fnm, FN_FID);
    if((fil=fopen(fnm, "w"))==NULL)			/* fid file */
    {
      flerr(fnm);
      return 0;
    }
    if (h.reps > 2) printf("\n \"%s\" ... ", fnm);
    if (g.gr)						/* xvgr */
    {
      (void) strcpy(fnm, shapes);
      (void) strcat(fnm, FN_GR);
      if((fil2=fopen(fnm, "w"))==NULL)
      {
        flerr(fnm);
        return 0;
      }
      if (h.reps > 2) printf("\n \"%s\" ... ", fnm);
    }
    if (h.tfl == 2)
    {
      for (j = 0; j < h.np; j++)
      {
        qq = Amp[j] * Cs[j];				/* real part */
        mxl = Amp[j] * Sn[j];				/* imag part */
        fprintf(fil, "%12.0f %12.0f\n", 1024.0*qq, 1024.0*mxl);
        if (g.gr) fprintf(fil2, "%7d %12.1f %12.1f\n", j, qq, mxl);
      }
    }
    else
    {    
      for (j = 0; j < h.np; j++)
      {
        qq = Amp[j] * Cs[j];				/* real part */
        mxl = Amp[j] * Sn[j];				/* imag part */
        fprintf(fil, "%12.0f %12.0f\n", qq, mxl);
        if (g.gr) fprintf(fil2, "%7d %12.1f %12.1f\n", j, qq, mxl);
      }
    }
    fclose(fil);
    if (g.gr) fclose(fil2);

    if (h.reps > 1) 
    {
      i = h.np/10;
      if (((h.np%10) == 1) && (((h.np/10)%10) != 1))
        sprintf(e_str, "\n\n  %d Step generated...  ", h.np);
      else
        sprintf(e_str, "\n\n  %d Steps generated...  ", h.np);      
      pxout(e_str, -1);

      if (h.tfl < 1)
      {
        if (h.pw90 > 0.0)
        {
          sprintf(e_str, "\n  RF calibration data:  ");
          sprintf(ifn, "ref_pwr=%d, ref_pw90=%.2f\n", h.rfpwr, h.pw90);
          pxout(strcat(e_str, ifn), -1);
        }
        if ((nitr > 1) && (itrerr > 0.1))
        {
            sprintf(e_str, "    %.2f percent attenuation error\n", itrerr);
            pxout(e_str, -1);
        }
      }
    }
  }

  if (h.tfl < 1)
  {
    if (nitr > 1)
    {
      for(i = 0; i < h.itnf; i++)				/* report offsets */
      { 
        if(Wv[i].oflg == 'T') 
        {
          sprintf(e_str, "\n>>> Set offset to %.2f Hz <<<", Wv[i].of);
          pxout(e_str, -1);
        }
      }
    }
    if (b1max < 0.00001) h.cal = 0;
    if (h.cal)
    {
      if ((h.reps > 1) && (h.tfl < 0))
      {
	sprintf(e_str, "\n  modulation cycle: %.4f ms (%.2f Hz)\n",
	       h.pw * 1000.0, 1.0 / h.pw);
        pxout(e_str, -1);
	if ((h.dcyc < 1.0) && (h.mod[0] != 'c') && (h.mod[0] != 's'))
        {
	  sprintf(e_str, "\n>>> For a duty cycle of %.4f ", h.dcyc);
	  sprintf(ifn, "set dmf=%.0f, dres=%.1f <<<\n", h.dmf, h.dres);
          strcat(e_str, ifn);
        }
	else
	  sprintf(e_str, "\n>>> Set dmf=%.0f, dres=%.1f <<<\n", h.dmf, h.dres);
        pxout(e_str, -1);
	if (h.pw90 > 0.001)
	{
        if ((h.mod[0] == 'c') || (h.mod[0] == 's'))  /* idc - internal duty cycle */
          sprintf(e_str, ">>> Set power to %d dB <<<\n", h.pwr);
        else if ((h.attn.u == 'e') || (h.attn.u == 'E') || (h.attn.u == 'h') || 
                 (h.attn.u == 'k') || (h.attn.u == 'H'))
	    sprintf(e_str, ">>> Set power to %d dB (fine power %d) <<<\n", 
                  h.pwr, h.pwrf);
        else
	  sprintf(e_str, ">>> Set power to %d dB <<<\n", h.pwr);
        pxout(e_str, -1);
	  if (imod)
        {
	    sprintf(e_str, "Equivalent to %d dB const ampl decoupling\n", RmsPwr);
          pxout(e_str, -1);
        }
	  pxout("\n", -1);
      }
	else
	{
	  sprintf(e_str, ">>> Adjust B1max to %.4f kHz <<<\n", B1max / h.dcyc);
	  mxl = h.dcyc / B1max;
	  sprintf(ifn, "(pw360/180/90: %.3f / %.3f / %.3f ms)\n",
		  mxl, mxl/2.0, mxl/4.0);
          pxout(strcat(e_str, ifn), -1);
	  if (Pfact > 0.999)
          {
	    sprintf(e_str, "Equivalent to %.4f kHz const ampl decoupling\n", B1rms);
            pxout(e_str, -1);
          }
	  pxout("\n", -1);
	}
      }
      else if (h.reps > 1)
      {
	if (h.pw90 > 0.001)
	{
	  sprintf(e_str, "\n>>> Set pulse width to %.4f ms <<<", h.pw*1000.0);
          if ((h.attn.u == 'e') || (h.attn.u == 'E') || (h.attn.u == 'h') || 
              (h.attn.u == 'k') || (h.attn.u == 'H'))
	    sprintf(ifn, "\n>>> Set pulse power to %d dB (fine power %d) <<<\n\n", 
			  h.pwr, h.pwrf);
          else
	    sprintf(ifn, "\n>>> Set pulse power to %d dB <<<\n\n", h.pwr);
          pxout(strcat(e_str, ifn), -1);
	}
	else
	{
	  sprintf(e_str, "\n>>> Set pulse width to %9.4f ms  <<<\n", h.pw * 1000.0);
	  sprintf(ifn, ">>> Adjust B1max to %12.4f kHz <<<\n", B1max / h.dcyc);
	  mxl = 1.0 / B1max;
          strcat(e_str, ifn);
	  sprintf(ifn, "(pw360/180/90: %.3f / %.3f / %.3f ms)\n\n",
		mxl, mxl/2.0, mxl/4.0);
          pxout(strcat(e_str, ifn), -1);
	}
      }
      if (h.pwr > g.maxpwr)
      {
        sprintf(e_str, "\nWarning :\nPower exceeds the maximum power level of %d dB !!!\n\n", g.maxpwr);
        pxout(e_str, -1);
      }
    }
  }
  else
  {
    if (h.reps > 1)
    {
      sprintf(e_str, "\n   Area factor : %.4f\n", h.dmf);
      pxout(e_str, -1);
    }
  }
  return 1;
}

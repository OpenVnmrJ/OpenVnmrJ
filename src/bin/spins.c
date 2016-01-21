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
/************************************************/
/* spins	-	spin simulation program	*/
/************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>

#define MAXPATHL 128
#define DIAG1	1.0e-5	/* diagonalization parameter */
#define DIAG2	4.0e-5
#define CONVERT	0.01	/* convergence test parameter */

#define MAXSPINS 8	/* maximum number of non equivalent spins */
#define MSUB	80 	/* maximum number of submatrices */
#define MSUBSB  64	/* maximum number of subsubmatrices */
#define MEN	512	/* maximum number of energy levels */
#define MENSUB  256	/* maximum number of energy levels in submatrix */
#define MESBSB  90	/* maximum number of energy level in subsubmatrix */
#define MPAR    (MAXSPINS + (MAXSPINS-1)*(MAXSPINS-1)/2)
                        /* maximum of parameters iterated */
#define MTRCAL 16384	/* maximum number of transitions in (amin,fr1,fr2) */
#define MTROBS  201	/* maximum number of assined lines + 1 */
#define MPARTO	  8	/* number of parameters equal because of symmetry */

struct tmatrix
  { float m[MESBSB][MESBSB];
  };

struct tclines
  { int lct;
    float fc[MTRCAL];
    float sc[MTRCAL];
  };

float corr[MPAR],bv[MPAR];
float a[MAXSPINS][MAXSPINS];	/* coupling constants */
float w[MAXSPINS];		/* chemical shifts */
short ns2[MSUB+1][MAXSPINS];	/* 2x spin value array */
short nq2[MEN+1][MAXSPINS];	/* 2x spin value array */
short nucl[MAXSPINS];		/* # of spins in spin set */
short iso[MAXSPINS];		/* isotope type */
char  nucname[MAXSPINS];	/* name of each nucleus */
short nijs[MSUB+1];
short ll[MSUBSB];		/* # of first energy level in a subsubmatrix */
short no[MSUBSB];		/* # of energy levels in a subsubmatrix */
short nss[MSUB];
float va[MESBSB][MESBSB];
float x[MESBSB][MESBSB];
short iq[MAXSPINS];
short il[MTROBS];			/* line number of assigned transition */
float e[MEN];
short ia[MPAR+1][MPARTO];
short ib[MPAR+1][MPARTO];
float f[MTROBS];
int ntype;			/* # of sets of magnetically equivalent spins */
int ntypem;			
int matd;			/* # of energy levels */
int nsbsb;			/* # of subsubmatrices */
float dev;
int nos;			/* number of paired parameter sets */
int ni;
struct tmatrix vb[MSUBSB];
char spinsystemstring[80];
float er1,er2;			/* old and new rms error */
float frgp;			/* frequency grouping of transitions */
int nl;				/* number of assigned lines */
float amin,fr1,fr2;		/* minimum intensity, frequency limits */

int js,irow;			/* globals for hamilt() */
int jj;				/* globals for hamilt() and iterat() */
int na,lul,kcurr,jjs,moa,iter;	/* globals for iterat() */
int nconv,do_continue,listiterations;	/* globals for iterat() */
float dcv[MPAR];		/* globals for iterat() */

int listtransitions,listenergy;

short imrow[MSUB];
short nt2[MENSUB];
short lin[MTRCAL],nkal[MTRCAL],nkbl[MTRCAL];
struct tclines clines;

float d[MPAR][MPAR],y[MPAR][MPAR];
char  path[MAXPATHL];

/***********/
printparams()
/***********/
{ int i,j;
  printf("Spin system %s\n",spinsystemstring);
  for (i=0; i<ntype; i++)
    printf("%c %16.2f\n",nucname[i],w[i]);
  for (i=0; i<ntype-1; i++)
    for (j=i+1; j<ntype; j++)
      printf("J%c%c %16.4f\n",nucname[i],nucname[j],a[i][j]);
}

/***********/
saveparams()
/***********/
{ int i,j;
  FILE *fopen();
  FILE *outputfile;
  char  filename[MAXPATHL];

  strcpy(filename,path);
  strcat(filename,"spini.outpar");
  if (outputfile=fopen(filename,"w"))
    {  
      for (i=0; i<ntype; i++)
        fprintf(outputfile,"%c   %14.4f\n",nucname[i],w[i]);
      for (i=0; i<ntype-1; i++)
        for (j=i+1; j<ntype; j++)
          fprintf(outputfile,"J%c%c %14.4f\n",nucname[i],nucname[j],a[i][j]);
      fclose(outputfile);
    }
  else
    { printf("ERROR: cannot open file %s\n",filename);
      return 1;
    }
  return 0;
}

/************/
writestatus(s)
/************/
int s;
{ FILE *fopen();
  FILE *outputfile;
  char  filename[MAXPATHL];

  strcpy(filename,path);
  strcat(filename,"spins.stat");
  if (outputfile=fopen(filename,"w+"))
    { fprintf(outputfile,"%d\n",s);
      fprintf(outputfile,"%d\n",nconv);
      fprintf(outputfile,"%d\n",iter);
      fprintf(outputfile,"%g\n",er2);
      fclose(outputfile);
    }
}

/**********/
init_spins()
/**********/
{ register int i,j;
  int lasti,r;
  FILE *fopen();
  FILE *inputfile;
  char  filename[MAXPATHL];

  for (i=0; i<MAXSPINS; i++)
    { a[i][i] = 0.0;
      nucl[i] = 0;
      iso[i]  = 0;
      for (j=0; j<MAXSPINS; j++)
        a[i][j] = 0.0;
    }
  strcpy(filename,path);
  strcat(filename,"spins.inpar");
  if (inputfile=fopen(filename,"r"))
    { /* first read the spin system string */
      if (fscanf(inputfile,"%f,%f,%f\n",&amin,&fr1,&fr2)!=3)
        { printf("ERROR: cannot read 'amin,fr1,fr2' from %s\n",filename);
          fclose(inputfile);
          return 1;
        }
      if (fscanf(inputfile,"%s\n",spinsystemstring)!=1)
        { printf("ERROR: cannot read spin system type from %s\n",filename);
          fclose(inputfile);
          return 1;
        }
      /* now interpret the spin system string */
      i = 0;
      ntype = 0;
      while (spinsystemstring[i])
        { /* capitalize, if necessary */
          if ((spinsystemstring[i]>='a')&&(spinsystemstring[i]<='z'))
            spinsystemstring[i] -= 32;
          if ((spinsystemstring[i]>='A')&&(spinsystemstring[i]<='Z'))
            { nucname[ntype] = spinsystemstring[i];
              if (nucname[ntype]=='J')
                { printf("ERROR: J is not a legal nucleus\n");
                  return 1;
                }
              if (ntype>0)
                { /* set isotope type */
                  if (spinsystemstring[i]-spinsystemstring[lasti]==1)
                    iso[ntype] = iso[ntype-1];
                  else
                    iso[ntype] = iso[ntype-1] + 1;
                  lasti = i;
                }
              else
                { iso[ntype] = 0;
                  lasti = i;
                }
              i++;
              if ((spinsystemstring[i]>='1')&&(spinsystemstring[i]<='9'))
                { nucl[ntype] = spinsystemstring[i] - '0';
                  i++;
                }
              else
                nucl[ntype] = 1;
              ntype++;
              if (ntype>MAXSPINS)
                { printf("ERROR: too many non equivalent spins\n");
                  fclose(inputfile);
                  return 1;
                }
            }
          else
            { printf("ERROR: illegal syntax in spin system type\n");
              fclose(inputfile);
              return 1;
            }
        }
      if (ntype<2)
        { printf("ERROR: minimum of two not equivalent spins required\n");
          fclose(inputfile);
          return 1;
        }
      /* now read the chemical shifts */
      for (i=0; i<ntype; i++)
        { if (fscanf(inputfile,"%e\n",&w[i])!=1)
            { printf("ERROR: chemical shift w[%d] input error\n",i);
              fclose(inputfile);
              return 1;
            }
        }
      /* now read the coupling constants */
      for (i=0; i<ntype-1; i++)
        { for (j=i+1; j<ntype-1; j++)
            { if (fscanf(inputfile,"%e,",&a[i][j])!=1)
                { printf("ERROR: coupling constant a[%d][%d] input error\n",
                    i,j);
                  fclose(inputfile);
                  return 1;
                }
              if (iso[i]==iso[j]) a[j][i] = a[i][j];
            }
          j = ntype - 1;
          if (fscanf(inputfile,"%e\n",&a[i][j])!=1)
            { printf("ERROR: coupling constant a[%d][%d] input error\n",i,j);
              fclose(inputfile);
              return 1;
            }
          if (iso[i]==iso[j]) a[j][i] = a[i][j];
        }
      printparams();
      nos = 1;
      fclose(inputfile);
    }
  else
    { printf("ERROR: cannot open file %s\n",filename);
      return 1;
    }
  if (ni>0)
    { r = setiteration1();
      if (r)
        return r;
      r = setiteration2();
      if (r)
        return r;
    }
  return 0;
}

/**********/
print_iaib()
/**********/
{ int i,j;
  printf("matrix ia:\n");
  for (i=0; i<ntype*ntype-1; i++)
    { printf("[%2d]:  ",i);
      for (j=0; j<MPARTO; j++)
        printf("%10d",ia[i][j]);
      printf("\n");
    }
  printf("matrix ib:\n");
  for (i=0; i<ntype*ntype-1; i++)
    { printf("[%2d]:  ",i);
      for (j=0; j<MPARTO; j++)
        printf("%10d",ib[i][j]);
      printf("\n");
    }
}

/*************/
setiteration2()
/*************/
{ FILE *fopen();
  FILE *inputfile;
  char  filename[MAXPATHL];
  register int k,j,is;
  int cntr,lineno,ilsm,note;
  float linefreq,temp;

  strcpy(filename,path);
  strcat(filename,"spini.indata");
  if (inputfile=fopen(filename,"r"))
    { /* get transitions */
      cntr = -1;
      while (1)
        { if (fscanf(inputfile,"%d,%f\n",&lineno,&linefreq)!=2)
            { /* end of file */
              nl = cntr+1;
              /* place transitions i order of line number */
              for (j=0; j<nl-1; j++)
                { ilsm = il[j];
                  note = -1;
                  for (k=j+1; k<nl; k++)
                    if (il[k]<ilsm)
                      { ilsm = il[k];
                        note = k;
                      }
                  if (note != -1)
                    { temp = f[j];
                      f[j] = f[note];
                      f[note] = temp;
                      is = il[j];
                      il[j] = ilsm;
                      il[note] = is;
                    }
                }
              return 0;
            }
          else
            { cntr++;
              if (cntr>=MTROBS-1)
                { printf("ERROR: too many assigned lines, maximum=%d\n",
                    MTROBS-1);
                  return 1;
                }
              il[cntr] = lineno-1;
              f[cntr]  = linefreq;
            }
        }
    }
  else
    { printf("ERROR: cannot open file %s\n",filename);
      return 1;
    }
}

/*************/
setiteration1()
/*************/
{ int i,j,k,l,cntr,ntypej,m;
  struct tparamdata
    { int param_no;
      int kmax;
    } pt[MPAR];
  FILE *fopen();
  FILE *inputfile;
  char  filename[MAXPATHL];
  char nextname[80],dummy[80];

  for (i=0; i<MPAR; i++)
    for (j=0; j<MPARTO; j++)
      { ia[i][j] = -1;
        ib[i][j] = -1;
      }
  strcpy(filename,path);
  strcat(filename,"spini.inpar");
  if (inputfile=fopen(filename,"r"))
    { cntr = 0;
      do
        { if (fscanf(inputfile,"%s %s",nextname,dummy)!=2)
            { /* print_iaib(); */
              fclose(inputfile);
              nos = cntr;
              return 0;		/* no more strings in file */
            }
          if (strlen(nextname)<1)
            { printf("ERROR: illegal syntax in file %s\n",filename);
              fclose(inputfile);
              return 1;
            }
          for (j=0; j<strlen(nextname); j++)
            if ((nextname[j]>='a') && (nextname[j]<='z'))
              nextname[j] -= 32;
          if (nextname[0]=='J')
            { /* coupling constant */
              if (strlen(nextname)<3)
                { printf("ERROR: illegal coupling constant syntax in file %s\n",
                    filename);
                  fclose(inputfile);
                  return 1;
                }
              j = 0;
              while ((nucname[j]!=nextname[1]) && (j<ntype))
                 j++;
              k = 0;
              while ((nucname[k]!=nextname[2]) && (k<ntype))
                 k++;
              ntypej = ntype + j + ntype * k;
              if ((nucname[j]==nextname[1])&&(nucname[k]==nextname[2]))
                { if (nextname[3]==0)
                    { pt[ntypej].kmax = 1;
                      pt[ntypej].param_no = cntr;
                      cntr++;
                      ia[pt[ntypej].param_no][pt[ntypej].kmax-1] = j;
                      ib[pt[ntypej].param_no][pt[ntypej].kmax-1] = k;
                    }
                  else if (nextname[3]=='=')
                    { if ((strlen(nextname)!=7)||(nextname[4]!='J'))
                        { printf("ERROR: illegal syntax in file %s\n",filename);
                          fclose(inputfile);
                          return 1;
                        }
                      l = 0;
                      while ((nucname[l]!=nextname[5]) && (l<ntype))
                        l++;
                      m = 0;
                      while ((nucname[m]!=nextname[6]) && (m<ntype))
                        m++;
                      if ((nucname[l]==nextname[5])&&(nucname[m]==nextname[6]))
                        { l = ntype + l + ntype * m;
                          if (l>ntypej)
                            { printf("ERROR: assignment to non iterative parameter\n");
                              fclose(inputfile);
                              return 1;
                            }
                          pt[l].kmax++;
                          if (pt[l].kmax>MPARTO)
                            { printf("ERROR: too many equal parameters\n");
                              fclose(inputfile);
                              return 1;
                            }
                          ia[pt[l].param_no][pt[l].kmax-1] = j;
                          ib[pt[l].param_no][pt[l].kmax-1] = k;
                        }
                      else
                        {printf("ERROR: assignment to unknown nucleus %c in file %s\n",
                            nextname[2],filename);
                          fclose(inputfile);
                          return 1;
                        }
                    }
                  else
                    { printf("ERROR: illegal syntax in file %s\n",filename);
                      fclose(inputfile);
                      return 1;
                    }
                }
              else
                { printf("ERROR: unknown nucleus %c in file %s\n",
                    nextname[0],filename);
                  fclose(inputfile);
                  return 1;
                }
            }
          else
            { /* chemical shift */
              j = 0;
              while ((nucname[j]!=nextname[0]) && (j<ntype))
                 j++;
              if (nucname[j]==nextname[0])
                { if (nextname[1]==0)
                    { pt[j].kmax = 1;
                      pt[j].param_no = cntr;
                      cntr++;
                      ia[pt[j].param_no][pt[j].kmax-1] = j;
                    }
                  else if (nextname[1]=='=')
                    { if (strlen(nextname)!=3)
                        { printf("ERROR: illegal syntax in file %s\n",filename);
                          fclose(inputfile);
                          return 1;
                        }
                      l = 0;
                      while ((nucname[l]!=nextname[2]) && (l<ntype))
                        l++;
                      if (nucname[l]==nextname[2])
                        { if (l>j)
                            { printf("ERROR: assignment to non iterative parameter\n");
                              fclose(inputfile);
                              return 1;
                            }
                          pt[l].kmax++;
                          if (pt[l].kmax>MPARTO)
                            { printf("ERROR: too many equal parameters\n");
                              fclose(inputfile);
                              return 1;
                            }
                          ia[pt[l].param_no][pt[l].kmax-1] = j;
                        }
                      else
                        {printf("ERROR: assignment to unknown nucleus %c in file %s\n",
                            nextname[2],filename);
                          fclose(inputfile);
                          return 1;
                        }
                    }
                  else
                    { printf("ERROR: illegal syntax in file %s\n",filename);
                      fclose(inputfile);
                      return 1;
                    }
                }
              else
                { printf("ERROR: unknown nucleus %c in file %s\n",
                    nextname[0],filename);
                  fclose(inputfile);
                  return 1;
                }
            }
        }
        while (1);
    }
  else
    { printf("ERROR: cannot open file %s\n",filename);
      return 1;
    }
}

/******/
invert()
/******/
{ int lz[MPAR];
  register int i,j,k,l,n;
  int lp,m;
  register float yy,ww;
  n = nos;
  for (j=0; j<n; j++)
    lz[j] = j;
  for (i=0; i<n; i++)
    { k = i;
      yy = va[i][i];
      l = i-1;
      lp = i+1;
      if (n>lp)
        for (j=lp; j<n; j++)
          { ww = va[i][j];
            if (fabs(ww)>fabs(yy))
              { k = j;
                yy = ww;
              }
          }
      for (j=0; j<n; j++)
        { dcv[j] = va[j][k];
          va[j][k] = va[j][i];
          va[j][i] = - dcv[j]/yy;
          va[i][j] = va[i][j]/yy;
          corr[j] = va[i][j];
        }
      va[i][i] = 1.0/yy;
      j = lz[i];
      lz[i] = lz[k];
      lz[k] = j;
      for (k=0; k<n; k++)
        { if (i!=k)
            for (j=0; j<n; j++)
              if (i!=j)
                va[k][j] -= corr[j]*dcv[k];
        }
    }
  for (i=0; i<n; i++)
    if (i!=lz[i])
      for (j=i+1; j<n; j++)
        if (i==lz[j])
          { m = lz[i];
            lz[i] = lz[j];
            lz[j] = m;
            for (l=0; l<n; l++)
              { dcv[l] = va[i][l];
                va[i][l] = va[j][l];
                va[j][l] = dcv[l];
              }
          }
}

/******/
hamilt()
/******/
/* sets up the shape of the hamiltonian and evaluates the matrix relating */
/* its diagonal elements to the chemical shifts.			  */
{ register int k,kk,j;
  int jm,ido,idom,jjm,jjs,jsm,i,flag,jx;
  /* generation of submatrices */
  /* ns2[j][i] is twice the total spin of the ith complex particle in the   */
  /* jth submatrix.							  */
  for (k=0; k<ntype; k++)
    ns2[0][k] = nucl[k];
  j = 0;
  kk = ntype - 1;
  do
    { j++;
      if (j>=MSUB) 
        { return 4;
        }
      for (k=0; k<ntype; k++)
        ns2[j][k] = ns2[j-1][k];
      kk = ntype-1;
      ns2[j][kk] -= 2;
      if (ns2[j][kk]<0)
        { do
            { ns2[j][kk] = ns2[0][kk];
              kk--;
              ns2[j][kk] -= 2;
            }
            while ((ns2[j][kk]<0) && (kk!=0));
        }
    }
    while (ns2[j][0]>=0);
  irow = j;
  /* irow is the number of submatrices */
  /* generation of basic functions for each submatrix */
  /* nq2[j][k] is twice the iz value of the kth complex particle */
  /* in the jth basic function of the whole matrix */
  js = 0;
  jm = 0;
  jj = -1;
  do
    { jj++;
      for (k=0; k<ntype; k++)
        nq2[jm][k] = ns2[jj][k];
      j = jm;
      kk = ntype - 1;
      while (kk>=0)
        { j++;
          for (k=0; k<ntype; k++)
            { nq2[j][k] = nq2[j-1][k];
            }
          kk = ntype - 1;
          nq2[j][kk] -= 2;
          if ((nq2[j][kk] + ns2[jj][kk]) < 0)
            { do
                { nq2[j][kk] = nq2[jm][kk];
                  kk--;
                  nq2[j][kk] -= 2;
                }
                while ((nq2[j][kk]+ns2[jj][kk]<0) && (kk!=0));
            }
          if (nq2[j][0]+ns2[jj][0]<0) kk = -1;
        }
      imrow[jj] = j - jm;
      if (imrow[jj]>MENSUB)
        return 1;
      /* imrow[jj] is the number of basic functions in the jjth submatrix */
      /* ordering of the basic functions by total iz value */
      /* nt2[jjm] is twice the sum of the iz values of the jjmth basis */
      /* function of the current submatrix */
      ido = imrow[jj];
      if (ido==1) 
        idom = 1;
      else
        idom = ido - 1;
      for (jjm=0; jjm<ido; jjm++)
        { j = jjm + jm;
          nt2[jjm] = 0;
          for (i=0; i<ntype; i++)
            nt2[jjm] = nt2[jjm] + nq2[j][i];
        }
      if (ido!=1)
        do
          { flag = 1;
            for (jjm=0; jjm<idom; jjm++)
              { j = jjm + jm;
                if (nt2[jjm]<nt2[jjm+1])
                  { flag = 0;
                    jx = nt2[jjm];
                    nt2[jjm] = nt2[jjm+1];
                    nt2[jjm+1] = jx;
                    for (i=0; i<ntype; i++)
                      { jx = nq2[j][i];
                        nq2[j][i] = nq2[j+1][i];
                        nq2[j+1][i] = jx;
                      }
                  }
              }
          }
          while (!flag);
      /* identification of submatrices */
      /* nijs[jj] is the number (within the whole matrix) of the first */
      /*   subsubmatrix in this submatrix */
      /* jjs numbers the subsubmatrices of this submatrix */
      /* jsm numbers the basic functions within the submatrix */
      /* no[is] is the size of the jth subsubmatrix of the whole problem */
      /* ll[is] is the number (within the whole matrix) of the first */
      /*   function in the jsth subsubmatrix ( of the whole problem) */
      /* nss[jj] is the number of the subsubmatrices in the jjth submatrix */
      jsm = 0;
      nijs[jj] = js;
      jjs = 0;
      ll[js] = jm;
      if (ido!=1)
        { for (jjm=0; jjm<idom; jjm++)
            { if (nt2[jjm]!=nt2[jjm+1])
                { no[js] = jsm+1;
                  if (jsm>MESBSB)
                    return 2;
                  jsm = -1;
                  jjs++;
                  js++;
                  ll[js] = jm+1;
                }
              jsm++;
              jm++;
            }
          no[js] = 1;
          nss[jj] = jjs+1;
          jm++;
          js++;
          if (js>MSUBSB)
            return 5;
        }
    }
    while (jj<irow-1);
    /* matd is the dimension of the matrix of the whole problem */
    matd = jm;
    if (matd>MEN)
      return 3;
    /* nsnsn is the number of subsubmatrices in the whole problem */
    nijs[irow] = js;
    nsbsb = js;
  return 0;
}

/*************/
hamilt_error(r)
/*************/
int r;
{ switch (r)
    { case 1: printf("ERROR:Submatrix no %d has %d functions, max=%d\n",
                jj,imrow[jj],MENSUB);
              break;
      case 2: printf("ERROR:Subsubmatrix no %d has %d functions, max=%d\n",
                js,no[js],MESBSB);
              break;
      case 3: printf("ERROR:The number of functions is %d, max=%d\n",
                matd,MEN);
              break;
      case 4: printf("ERROR:The number of submatrices is too big\n");
              break;
      case 5: printf("ERROR:The number of subsubmatrices is too large\n");
              break;
      default:printf("ERROR:Error condition in calculation of hamiltonian\n");
              break;
    }
}

/******/
iterat()
/******/
{ int i,j,jl,kl,k,m,n,z,ndiff,ip,kntrl,ja1,ja2,jx1,jx2,r;

  na = 0;			/* counts the subsubmatrices */
  dev = 0.0;
  for (j=0; j<nos; j++)
    bv[j] = 0.0;
  if (ni>0)
    { for (i=0; i<MPAR; i++)
        for (j=0; j<MPAR; j++)
          va[i][j] = 0.0;
    }
  lul = -1;			/* counts the transition numbers */
  kcurr = 0;			/* counts the observed transitions */
  for (jj=0; jj<irow; jj++)	/* counts the submatrices */
    { jjs = 0;			/* counts subsubmatrices in current submat */
      /* this is the start of the loop of the submatric */
      do
        { moa = no[na];		/* size of the current subsubmatrix */
          /* compute elements of the hamiltonian matrix for each submatrix */
          for (j=0; j<moa; j++)
            { for (k=j; k<moa; k++)
                { jl = ll[na] + j;
                  kl = ll[na] + k;
                  if (j==k)
                    { /* diagnonal matrix element */
                      x[j][j] = 0;
                      for (m=0; m<ntype; m++)
                        x[j][j] += 0.5 * nq2[jl][m] * w[m];
                      for (m=0; m<ntypem; m++)
                        for (n=m+1; n<ntype; n++)
                          x[j][j] += 0.25 * nq2[jl][m] * nq2[jl][n] * a[m][n];
                    }
                  else
                    { /* off diagonal matrix element */
                      x[j][k] = 0.0;
                      ndiff = 0;
                      for (i=0; i<ntype; i++)
                        if (nq2[jl][i]!=nq2[kl][i])
                          { iq[ndiff] = i;
                            ndiff++;
                          }
                      if (ndiff==2)
                        { i = iq[0];
                          ip = iq[1];
                          if (a[ip][i]!=0.0)
                            { if ((nq2[jl][i]-nq2[kl][i]) *
                                          (nq2[jl][ip]-nq2[kl][ip]) == -4)
                                { z = (ns2[jj][i] * (ns2[jj][i] + 2)
                                         - nq2[jl][i] * nq2[kl][i])
                                       * (ns2[jj][ip] * (ns2[jj][ip] + 2)
                                         - nq2[jl][ip] * nq2[kl][ip]);
                                  x[j][k] = 0.125 * a[ip][i] * sqrt((double)z);
                                }
                            }
                        }
                      x[k][j] = x[j][k];
                    }
                }
            }
          if (iter==0)
            kntrl = 0;
          else
            { /* rough diagonalization */
              for (ja1=0; ja1<moa; ja1++)
                for (ja2=0; ja2<moa; ja2++)
                  { y[ja1][ja2] = 0.0;
                    for (jx1=0; jx1<moa; jx1++)
                      y[ja1][ja2] += x[ja1][jx1] * vb[na].m[jx1][ja2];
                  }
              for (jx1=0; jx1<moa; jx1++)
                for (jx2=0; jx2<moa; jx2++)
                  { x[jx1][jx2] = 0.0;
                    for (ja1=0; ja1<moa; ja1++)
                      x[jx1][jx2] += vb[na].m[ja1][jx1] * y[ja1][jx2];
                  }
              kntrl = 1;
            }
          diagonalizematrix(moa,kntrl);
          /* store energies and eigenvectors */
          for (j=0; j<moa; j++)
            { jl = j + ll[na];
              e[jl] = x[j][j];
            }
          if (ni!=0) 
            { r = square();
              if (r)
                return r;
            }
          na++;
        }
        while (na<nijs[jj+1]);
    }
  return 0;
}

/*****/
setup()
/*****/
{ int i,k,ns,nsa,nsb,ias,ibs;
  nconv = 0;
  ntypem = ntype - 1;
  iter = 0;
  er1  = 1e10;
  er2  = 0;
  do
    { if (!do_continue)
        { if (iterat())
            return 1;
          na--;
          if (ni<=0)
            { printenergylevels();
              return 0;
            }
          er2 = sqrt(er2/nl);
          if (iter==ni)
            nconv = 1;
          else if (iter>=3)
            { if (er2==0)
                nconv = 2;
              else if ((er1-er2)/er1<=CONVERT)
                nconv = 2;
              if (er2>er1)
                nconv = 3;
            }
          er1 = er2;
          if ((nconv!=0) || (listiterations && (iter>0)))
            { if (nconv>0)
                printf("Final parameters\n");
              printf("Iteration number %4d\n",iter);
              printparams();
              saveparams();
            }
           printf("rms frequency error %16.4f\n",er2);
           if (nconv==2) 
             printf("Iteration has converged\n");
           printf("\n");
        }
       else
        { printf("ERROR:continue not implemented in setup\n");
          return 1;
        }
       if (nconv==0)
         invert();
       else
         return 0;

       for (nsa=0; nsa<nos; nsa++)
         { corr[nsa] = 0;
           for (nsb=0; nsb<nos; nsb++)
             { corr[nsa] += va[nsa][nsb] * bv[nsb];
             }
         }
       for (ns=0; ns<nos; ns++)
         for (k=0; k<MPARTO; k++)
           { ias = ia[ns][k];
             if (ias!=-1)
               { ibs = ib[ns][k];
                 /* needs additions for continue */
                 if (ibs!=-1)            
                   { a[ias][ibs] += corr[ns];
                     if (iso[ias]==iso[ibs])
                       a[ibs][ias] = a[ias][ibs];
                   }
                 else
                   { w[ias] += corr[ns];
                   }
               }
           }
       do_continue = 0;
       iter++;
    }
    while (iter<=ni);
  return 0;
}

/************************/
diagonalizematrix(n,kntrl)
/************************/
int n,kntrl;
/* n=size of current subsubmatrix */
{ register int i,j,k,l;
  int nsqp,iter,nm1,jm1,ip1;
  register float bigx,bigr,ar,b,f1;
  float del,rot,ts,r,d;
  int flag;
  flag = 0;
  nsqp = 3 * n * n / 2;
  iter = 0;
  nm1 = n-1;
  if (nm1>=0)
    { if (nm1==0)
        { vb[na].m[0][0] = 1.0;
        }
      else
        { if (kntrl==0)
            { for (i=0; i<n; i++)
                for (j=0; j<n; j++)
                  vb[na].m[i][j] = 0.0;
              for (k=0; k<n; k++)
                vb[na].m[k][k] = 1.0;
            }
          do
            { iter++;
              /* find the largest off diagonal element */
              bigx = 0.0;
              for (j=1; j<n; j++)
                { jm1 = j;
                  for (i=0; i<jm1; i++)
                    if (bigx<fabs(x[i][j]))
                      { bigx = fabs(x[i][j]);
                        k = i;
                        l = j;
                      }
                }
              /* test for diagonalization complete */
              if (bigx<=DIAG1)
                { bigr = DIAG2;
                  for (i=0; i<nm1; i++)
                    { ip1 = i + 1;
                      for (j=ip1; j<n; j++)
                        { if (x[i][j]!=0.0)
                            { del = x[i][i] - x[j][j];
                              if (del!=0.0)
                                { rot = fabs(x[i][j]/del);
                                  if (rot>bigr)
                                    { bigr = rot;
                                      k = i;
                                      l = j;
                                    }
                                }
                            }
                       }
                   }
                }
              else
                bigr = DIAG2 + 1.0;
              if (bigr>DIAG2)
                { ts = x[k][l]*x[k][l];
                  del = x[k][k] - x[l][l];
                  r = sqrt(fabs(del*del+4.0*ts));
                  ar = sqrt(fabs((r+del)/(2.0*r)));
                  if (ar<0.707)
                    { b = -ar;
                      ar = sqrt(1.0-b*b);
                    }
                  else
                    b = -sqrt(1.0-ar*ar);
                  if (del/x[k][l]<0.0) 
                    b = -b;
                  /* enter rotation in eigenvector matrix */
                  /* and do orthogonal rotation of matrix */
                  for (j=0; j<n; j++)
                    { f1 = vb[na].m[j][k];
                      vb[na].m[j][k] = ar * f1 - b * vb[na].m[j][l];
                      vb[na].m[j][l] = b * f1 + ar * vb[na].m[j][l];
                      if (k!=j)
                        { if (l!=j)
                            { x[k][j] = ar * x[j][k] - b * x[j][l];
                              x[l][j] = b * x[j][k] + ar * x[j][l];
                              x[j][k] = x[k][j];
                              x[j][l] = x[l][j];
                            }
                        }
                    }
                  d = x[k][k] + x[l][l];
                  x[k][k] = ar*ar*x[k][k] + b*b*x[l][l] - 2.0*ar*b*x[k][l];
                  x[l][l] = d - x[k][k];
                  x[l][k] = 0.0;
                  x[k][l] = 0.0;
                }
              else
                flag = 1;
              }
            while ((iter<nsqp) && (flag==0));
          if (iter>=nsqp)
            printf("ERROR:inclomplete diagonalization\n");
        }
    }
}

/******/
square()
/******/
{ register int i,j,k,ka;
  int ias,ibs,iz,kl,kal,ja,jb,jal,jbl,jap,nps,ns;
  int idx,ilk,lll,moas,mobs,nas,nbs;
  register float b;
  int done;
  if (jjs!=0)
    { moas = no[na-1];
      for (j=0; j<moas; j++)
        for (ns=0; ns<nos; ns++)
          y[j][ns] = d[j][ns];
    }
  for (j=0; j<moa; j++)
    for (ns=0; ns<nos; ns++)
      { d[j][ns] = 0;
        nps = -1;
        do
          { nps++;
            ias = ia[ns][nps];
            if (ias!=-1)
              { ibs = ib[ns][nps];
                if (ibs<=0)
                  for (k=0; k<moa; k++)
                    { kl = k + ll[na];
                      d[j][ns] += 0.5 * vb[na].m[k][j] * vb[na].m[k][j]
                                                       * nq2[kl][ias];
                    }
                else
                  { for (k=0; k<moa; k++)
                      { kl = k + ll[na];
                        d[j][ns] += 0.25 * vb[na].m[k][j] * vb[na].m[k][j]
                                         * nq2[kl][ias] * nq2[kl][ibs];
                      }
                    /* contributions from off diagonal elements */
                    if ((iso[ias]==iso[ibs]) && (moa>1))
                      for (ja=0; ja<moa-1; ja++)
                        { jap = ja + 1;
                          jal = ja + ll[na];
                          for (ka=jap; ka<moa; ka++)
                            { kal = ka + ll[na];
                              done = 0;
                              i = -1;
                              do
                                { i++;
                                  if ((i!=ias) && (i!=ibs))
                                    done = (nq2[jal][i] != nq2[kal][i]);
                                }
                                while ((!done) && (i!=ntype-1) &&
                                       (i!=ias) && (i!=ibs));
                              if ((!done) && ((nq2[jal][ias]-nq2[kal][ias])
                                  * (nq2[jal][ibs]-nq2[kal][ibs]) == -4))
                                { iz = (ns2[jj][ias] * (ns2[jj][ias]+2)
                                       - (nq2[jal][ias] * nq2[kal][ias]))
                                       * (ns2[jj][ibs] * (ns2[jj][ibs]+2)
                                       - (nq2[jal][ibs] * nq2[kal][ibs]));
                                  d[j][ns] += 0.25 * vb[na].m[ja][j] 
                                       * vb[na].m[ka][j] * sqrt((double)iz);
                                }   
                            }
                        }
                  }
              }
          }
          while ((ias!=-1) && (nps!=MPARTO-1));
      }
    if (jjs!=0)
      { nas = na-1;
        nbs = na;
        mobs= moa;
        k   = kcurr;
        lll = lul+1;
        lul = lul + moas*mobs;
        /* lll,lul delimit the line numbers of transitions between */
        /* the current pairs of submatrices. */
        for (k=lll; k<=lul; k++)
          { while ((il[kcurr]!=k) && (kcurr<nl-1) && (il[kcurr]<k))
              kcurr++;
            idx = k-lll;
            ja = idx / mobs;
            jb = idx - ja*mobs;
            jal = ja + ll[nas];
            jbl = jb + ll[nbs];
            for (ns=0; ns<nos; ns++)
              dcv[ns] = y[ja][ns] - d[jb][ns];
            if (il[kcurr]==k)
              b = f[kcurr] + e[jbl] - e[jal];
            else
              b = 0; /* line not assigned */
            /* printf("k=%d, kcurr=%d, b=%g\n",k,kcurr,b); */
            for (i=0; i<nos; i++)
              { for (j=0; j<nos; j++)
                  { va[i][j] += dcv[i] * dcv[j];
                    va[j][i] = va[i][j];
                  }
                bv[i] += dcv[i] * b;
                /* printf("%12g  ",bv[i]); */
              }
            /* printf("\n"); */
            dev += b*b;
            er2 = dev;
          }
      }
  jjs++;
  return 0;
}

/*****************/
printenergylevels()
/*****************/
{ int i,jj,na;
  float zit[MAXSPINS];
  if (!listenergy)
    return 0;
  na = 0;
  ll[nsbsb] = matd;
  printf("Energy levels\n");
  for (jj=0; jj<irow; jj++)
    { for (i=0; i<ntype; i++)
        zit[i] = 0.5 * ns2[jj][i];
      printf("Submatrix number %d\n",jj+1);
      if (jj==0)
        printf("Total spin quantum numbers\n");
      for (i=0; i<ntype; i++)
        printf("%8.1f",zit[i]);
      printf("\n\n");
      if (jj==0)
        printf("Level number    energy\n");
      do
        { for (i=ll[na]; i<ll[na+1]; i++)
            printf("%4d  %12.3f\n",i+1,e[i]);
          printf("\n");
          na++;
        }
        while (na<nijs[jj+1]);
    }
}

/************/
double fact(i)
/************/
int i;
{ register int j,x;
  x = 1;
  for (j=1; j<=i; j++)
    x *= j;
  return (double)x;
}

/******/
transt()
/******/
{ float fr,fsm,omega,s,z,zn,temp;
  int i,iarg1,j,k,l,line,lm,lint,note,curssb;
  int na,nb,moa,mob,ja,jb,jj,kal,kbl,ka,kb,kd,npa,npb,inn,ndiff;
  int lb1,lb2;
  int oldjj = 0;
  char ch;
  FILE *fopen();
  FILE *outputfile;
  char  filename[MAXPATHL];

  if (listtransitions)
    printf("line # transitions    frequency     intensity\n");
  if (ni<0) 
    ni = 0;
  lm = 0;		/* counter of observed assigned transitions */
  line = -1;		/* counts theoretically possible transitions */
  clines.lct = -1;	/* counts printed transitions */
  na = -2;		/* counter for subsubmatrices */
  for (jj=0; jj<irow; jj++)
    if (nss[jj]!=1)
      { omega = 1.0;
        for (i=0; i<ntype; i++)
          { iarg1 = (nucl[i] - ns2[jj][i]) / 2;
            zn = ns2[jj][i] + 1;
            if (iarg1>0)
              { omega = (omega * zn * fact(nucl[i])) /
                (fact(iarg1) * fact((nucl[i]+ns2[jj][i])/2+1));
              }
          }
        curssb = 0;
        /* beginning of loop of submatrices */
        na++;
        while (na<nijs[jj+1]-2)
          { 
	    na++;
            nb = na + 1;
            moa = no[na];
            mob = no[nb];
            for (jb=0; jb<mob; jb++)
              { npb = jb + ll[nb];
                for (ja=0; ja<moa; ja++)
                  { npa = ja + ll[na];
                    /* calculate transformation matrix */
                    ndiff = -1;
                    iq[0] = -1;
                    for (i=0; i<ntype; i++)
                      { if (nq2[npb][i]!=nq2[npa][i])
                          { ndiff++;
                            if (nq2[npa][i]==nq2[npb][i]+2)
                              iq[ndiff] = i;
                          }
                      }
                    z = 0.0;
                    if ((ndiff==0)&&(iq[0]!=-1))
                      { i = iq[0];
                        z = (ns2[jj][i]+nq2[npa][i])*(ns2[jj][i]-nq2[npb][i]);
                        z = 0.5 * sqrt(z);
                      }
                    x[ja][jb] = z;
                  }
              }
            for (ja=0; ja<moa; ja++)
              for (jb=0; jb<mob; jb++)
                { z = 0.0;
                  for (inn=0; inn<moa; inn++)
                    z += vb[curssb].m[inn][ja] * x[inn][jb];
                  va[ja][jb] = z;
                }
            curssb = nb;           
	    /* printf("na=%d, jj=%d, oldjj=%d, nss[jj]=%d, nijs[jj+1]-1=%d\n",
		na,jj,oldjj,nss[jj],nijs[jj+1]-1); */
            for (ka=0; ka<moa; ka++)
              { for (kb=0; kb<mob; kb++)
                  { line++;
                    s = 0.0;
                    for (kd=0; kd<mob; kd++)
                      s += va[ka][kd] * vb[curssb].m[kd][kb];
                    s = omega * s * s;
                    kal = ka + ll[na];
                    kbl = kb + ll[nb];
                    fr = e[kal] - e[kbl];
            /*  printf("line=%d, %d %d,na=%d %d %d, e[kal]=%g, e[kbl]=%g\n",
			line,kbl,kal,na,ka,kb,e[kal],e[kbl]); */
                    lb1 = (s>=amin) && (fr>=fr1) && (fr2>=fr);
                    if (ni>0)
                      lb2 = (line==il[lm]);
                    else
                      lb2 = 0;
                    if (lb1 || lb2)
                      { if (listtransitions) 
                          { if (lb2) ch = '*';
                            else ch = ' ';
                            printf("%4d%c%4d  %4d  %12.3f  %12.3f\n",
                                                   line,ch,kal+1,kbl+1,fr,s);
                          }
	   /*	printf("lb1=%d lb2=%d lm=%d line=%d il[lm]=%d lct=%d\n",
	 	lb1,lb2,lm,line,il[lm],clines.lct); */
                        if (lb2) lm++;
                        if (clines.lct<MTRCAL-1)
                          clines.lct++;
                        if (clines.lct<MTRCAL)
                          { lin[clines.lct]  = line;
                            nkal[clines.lct] = kal;
                            nkbl[clines.lct] = kbl;
                            clines.fc[clines.lct]   = fr;
                            clines.sc[clines.lct]   = s;
                          }
                      } /* if (lb1 || lb2) */
                  } /* for kb */
              } /* for ka */
	    /* oldjj=jj; */
          } /* while */
      } /* for jj */
    clines.lct++;
    
    /* order lines in storage by frequency */
    if (clines.lct<MTRCAL-1)
      for (j=0; j<clines.lct-1; j++)
        { note = -1;
          fsm = clines.fc[j];
          for (k=j+1; k<clines.lct; k++)
            { if (fsm<clines.fc[k])
                { fsm = clines.fc[k];
                  note = k;
                }
            }
          if (note != -1)
            { temp = clines.fc[j];
              clines.fc[j] = clines.fc[note];
              clines.fc[note] = temp;
              temp = clines.sc[j];
              clines.sc[j] = clines.sc[note];
              clines.sc[note] = temp;
              lint = lin[j];
              lin[j] = lin[note];
              lin[note] = lint;
              lint = nkal[j];
              nkal[j] = nkal[note];
              nkal[note] = lint;
              lint = nkbl[j];
              nkbl[j] = nkbl[note];
              nkbl[note] = lint;
            }
        }
    else
      { printf("Calculated transitions = %d, maximum allowed = %d\n",
          clines.lct,MTRCAL);
        printf("ERROR:Increase amin to reduce transition count.\n");
        return 1;
      }
    printf("line # transitions    frequency     intensity");
    if (ni>0)
      printf("   observed     difference  ");
    printf("\n");
    strcpy(filename,path);
    strcat(filename,"spins.outdata");
    outputfile=fopen(filename,"w+");
    for (l=0; l<clines.lct; l++)
      { if ((l!=0) && (frgp!=0.0))
          if (clines.fc[l-1]-clines.fc[l]>frgp)
             printf("\n");
        ch = ' ';
        if (ni>0)
          { k = -1;
            do
              k++;
              while(((lin[l]) != il[k]) && (k<nl-1));
            if ((lin[l]) == il[k]) 
              ch = '*'; 
          }
        printf("%4d%c%4d  %4d  %12.3f  %12.3f",
          lin[l]+1,ch,nkal[l]+1,nkbl[l]+1,clines.fc[l],clines.sc[l]);
        if ((ni>0) && (ch=='*'))
          printf("%12.3f  %12.3f",f[k],f[k]-clines.fc[l]);
        printf("\n");
        if (outputfile)
          { fprintf(outputfile,"%g,%g;L%d%c\n",
              clines.fc[l],clines.sc[l],lin[l]+1,ch);
          }
      }
  if (outputfile)
    fclose(outputfile);
  return 0;
}

/***********/
printhamilt()
/***********/
{ int i,j;
  printf("nucl\n");
  for (i=0; i<ntype; i++)
    printf("%5d",nucl[i]);
  printf("\n ns2\n");
  for (i=0; i<irow; i++)
    for (j=0; j<ntype; j++)
      printf("%5d",ns2[i][j]);
  printf("\nq2\n");
  for (i=0; i<matd; i++)
    { for (j=0; j<ntype; j++)
        printf("%5d",nq2[i][j]);
      printf("\n");
    }
  printf("matd = %d\n",matd);
  printf("    imrow    nijs+1    nss\n");
  for (i=0; i<irow; i++)
    printf("%8d%8d%8d\n",imrow[i],nijs[i]+1,nss[i]);
  printf("\n");
  printf("    no       ll\n");
  for (i=0; i<nsbsb; i++)
   printf("%8d%8d\n",no[i],ll[i]);
  printf("\n");
}

/*************/
main(argc,argv)
/*************/
int argc;
char *argv[];
{ int r;
  int i,do_iterate;
  ni = do_iterate = do_continue = 0;
  listtransitions = 0;
  listiterations  = 0;
  listenergy = 0;
  frgp = 0;
  path[0] = 0;
  if (argc>1)
    { for (i=1; i<argc; i++)
        { if (strcmp(argv[i],"iterate")==0)
            do_iterate = 1;
          else if (strcmp(argv[i],"continue")==0)
            do_continue = 1;
          else if (strcmp(argv[i],"transitions")==0)
            listtransitions = 1;
          else if ((strcmp(argv[i],"iterations")==0) ||
                   (strcmp(argv[i],"iteration")==0))
            listiterations = 1;
          else if (strcmp(argv[i],"energy")==0)
            listenergy = 1;
          else if (argv[i][0] == '/')
          {
            strcpy(path,argv[i]);
            strcat(path,"/");
          }
          else if ((argv[i][0]>='0') && (argv[i][0]<='9'))
            { if (sscanf(argv[i],"%d",&ni) != 1)
                { printf("ERROR: integer argument syntax error\n");
                  writestatus(1);
                  return 1;
                }
              if ((ni<1) || (ni>9999))
                { printf("ERROR: number of iteration out of limits\n");
                  writestatus(1);
                  return 1;
                }
            }
#ifdef VMS
          else if (argv[i][ strlen( argv[i] ) - 1 ] == ']' ||
                   argv[i][ strlen( argv[i] ) - 1 ] == ':')
            strcpy(path,argv[i]);
#endif
        }
    }
  if (do_iterate || do_continue)
    { if (ni==0)
        ni = 32;
    }
  else
    { if (ni!=0)
        do_iterate = 1;
    }
  if (do_continue)
    { printf("ERROR: continue not yet implemented\n");
      writestatus(1);
      return 1;
    }
  if (init_spins())
    { writestatus(1);
      return 1;
    }
  r = hamilt();
  if (r)
    { hamilt_error(r);
      writestatus(1);
      return 1;
    }
  /* printhamilt(); */		/* debug only */
  if (setup())
    { writestatus(1);
      return 1;
    }
  if (transt())
    { writestatus(1);
      return 1;
    }
  writestatus(0);
  return 0;
}


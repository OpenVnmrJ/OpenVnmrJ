/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* vnmrio.h - VNMR input and output functions */


#ifndef __VNMRIO_H
#define __VNMRIO_H
#include <strings.h>      
#include <sys/utsname.h>
#ifdef LINUX
#include <sys/sysinfo.h>
#elif MACOS
#include <stdint.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#endif
#include <sys/stat.h>
#include <stdio.h>

#define VNMRIO  1

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#ifndef MAX
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#endif

#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif

#ifndef MAXSTR
#define MAXSTR 1024
#endif

#ifndef DBUG
#define DBUG 1
#endif

static int  pc_bytes = 0;

/* ------------- Varian NMR data types ------------ */

typedef struct                   /* single precision data */
{
  short   re, im;
} SPFID;

typedef struct                   /* double precision data */
{
  int    re, im;
} DPFID;

typedef struct                   /* floating point data */
{
  float   re, im;
} FPFID;

typedef struct                   /* double precision floating point data */
{
  double   re, im;
} COMPLX;

typedef union 
{
  SPFID         sp;
  FPFID         fp;
  DPFID         dp; 
  COMPLX        dd; 
} DATA;

typedef struct 
{
  int    nblocks, 		/* # of blocks in file, ni */
         ntraces, 		/* # of traces per block, usually 1 */
         np, 			/* # of points per trace */
         ebytes, 		/* # of bytes per point, 2 - int, 4 - int */
         tbytes, 		/* # of bytes per trace = np*ebytes */
         bbytes;		/* # of bytes per block = ntraces*tbytes +
                                     nbheaders*sizeof(block header = 28)   */
  short  vers_id, 		/* software version & file id */
         status;		/* status of whole file */
  int   nbheaders;	        /* # of block headers */
} FILEHEADER;

typedef struct 
{
  short  scale, 		/* scaling factor */
         status, 		/* status of data in block */ 
         iblock, 		/* block index, ix */ 
         mode; 		        /* mode of data in block */
  int    ct;	 		/* ct counter value for fid */
  float  lp, 	 		/* left phase in f1 */
         rp, 	 		/* right phase in f1 */
         lvl,	 		/* level drift correction */ 
         tlt;	 		/* tilt drift correction */
} BLOCKHEADER;

typedef struct                   /* procpar data */
{
  double   lb, sb, sbs, gf, gfs, fpmult;
  double   lb1, sb1, sbs1, gf1, gfs1, fpmult1;
  double   lb2, sb2, sbs2, gf2, gfs2, fpmult2;
  double   lb3, sb3, sbs3, gf3, gfs3, fpmult3;
  double   at, at1, at2, at3, sw, sw1, sw2, sw3;
  int      np, ni, ni2, ni3, nimax, ni2max, ni3max, fn, fn1, fn2, fn3, ct;
} PROCPAR;


typedef struct                   /* frequencies */
{
  double   tof, dof, dof2, dof3;
  double   sfrq, dfrq, dfrq2, dfrq3, lockfreq, lockfreq_;
  double   reffrq, reffrq1, reffrq2, reffrq3;
  double   refpos, refpos1, refpos2, refpos3;
  double   rfl, rfl1, rfl2, rfl3, rfp, rfp1, rfp2, rfp3;
} SFRQ;


void vn_error(char *err_msg)
{
  printf("%s",err_msg);
  printf(" Aborting...\n");

  exit(1);
}


void vn_abort(src, err_msg)
FILE  *src;
char  *err_msg;
{
  printf("%s",err_msg);
  printf(" Aborting...\n");
  if(src)
    fclose(src);
  exit(1);
}


void mk_dir(xname)
char  *xname;
{
  mkdir(xname, 0777);
  return;
}

int check_expdir_fname(xname)
char *xname;
{
  int   i, num;
  char  e, x, p, homedir[512];
  
  i=sscanf(xname,"%c%c%c%d", &e, &x, &p, &num);      
  if((i == 4) && (e == 'e') && (x == 'x') && (p == 'p'))
  { 
    strcpy(homedir, (char *) getenv("HOME"));
    strcat(homedir, "/vnmrsys/");
    strcat(homedir, xname);    
    strcpy(xname, homedir);
  }
  else i=0;
  
  return i;
}

  
void check_phasefile_fname(xname)
char *xname;
{  
  if(check_expdir_fname(xname))  
    strcat(xname, "/datdir/phasefile");
  
  return;
}

short fid_ext(xname)
char *xname;
{
  int   i, j;

  i=0;
  j = strlen(xname);
  if(j > 4)
  {
    if(xname[j-4] == '.') i++;
    if(xname[j-3] == 'f') i++;
    if(xname[j-2] == 'i') i++;
    if(xname[j-1] == 'd') i++;
  }  
  if(i == 4) return 1; 
  else return 0; 
}
  
void add_fid_ext(xname)
char *xname;
{
  if(!fid_ext(xname)) 
    strcat(xname,".fid");

  return; 
}
  
int check_acqfil(xname)
char *xname;
{
  int    i, j;
  char  *s;

  (int) check_expdir_fname(xname); 

  if((s = strrchr(xname, '/')) == NULL) 
    return 0;
  else if(strcmp(s, "/acqfil") == 0) 
    return 1;
  else 
  {
    j = strlen(s);  
    if((j>4) && (s[1]=='e') && (s[2]=='x') && (s[3]=='p')) 
    {
      s+=4; i=0;
      i=sscanf(s, "%d", &j); 
      if(i>0)
      {
        strcat(xname, "/acqfil");
        return 1;
      }
    }
  }

  return 0;
}
  
void check_fid_dir(xname)
char *xname;
{
  
  if(check_acqfil(xname) == 0)
    (void) add_fid_ext(xname);  
  return;
}  

void check_fid_fname(xname)
char *xname;
{
  int   i, j, k;
  
  i=0; k=0;
  j = strlen(xname);  
  if(j > 4)
  {
    if(xname[j-4] == '/') k=1;
    else if(xname[j-4] == '.') k=2;
    if(xname[j-3] == 'f') i++;
    if(xname[j-2] == 'i') i++;
    if(xname[j-1] == 'd') i++;
  }
  
  if((i == 3) && (k > 0)) 
  {
    if(k == 2)
      strcat(xname, "/fid");
    return; 
  } 
  else 
  {
    check_fid_dir(xname);
    strcat(xname, "/fid");
  }

  return;
}


void check_fname(xname)
char *xname;
{
  if(fid_ext(xname)) return;  
  else (int) check_expdir_fname(xname);
  
  return;
}


void add_xt(xname, xt)
char *xname, *xt;
{
  char       *sx; 

  sx = strrchr(xname, '.'); *sx = '\0';  
  strcat(xname, xt);
  strcat(xname, ".fid");
  
  return;                   
}

static void copytext(char *fromPath, char *toPath)
{
  FILE *infile,*outfile;
  register int   ch;

  if ( (infile=fopen(fromPath,"r")) )
  {
    if ( (outfile=fopen(toPath,"w")) )
    {
      while ((ch=getc(infile)) != EOF) putc(ch,outfile);
      fclose(outfile);
    }
    fclose(infile);
  }
}

void copy_par(char *fromDir, char *toDir)
{
  char  fromFile[MAXSTR], toFile[MAXSTR];
  
  (void) check_fname(fromDir);
  (void) check_fname(toDir);
  
  sprintf(fromFile,"%s/text",fromDir);
  sprintf(toFile,"%s/text",toDir);
  copytext(fromFile, toFile);

  sprintf(fromFile,"%s/procpar",fromDir);
  sprintf(toFile,"%s/procpar",toDir);
  copytext(fromFile, toFile);

  sprintf(fromFile,"%s/log",fromDir);
  sprintf(toFile,"%s/log",toDir);
  if(access(fromFile, 0) != 0)
    sprintf(fromFile,"%s/acqfil/log",fromDir);
  copytext(fromFile, toFile);
                                           
  if(fid_ext(fromDir))
    sprintf(fromFile, "%s/sampling.sch", fromDir);   
  else
    sprintf(fromFile, "%s/acqfil/sampling.sch", fromDir);
  if(access(fromFile, 0) == 0) /* copy the sampling schedule if it exists */
  {
    sprintf(toFile, "%s/sampling.sch", toDir);   
    copytext(fromFile, toFile);
  }    
  return;
}

short fid_exists(xname, rep)
char  *xname;
short  rep;
{
  char  fnm[MAXSTR];
  
  sprintf(fnm, "%s/fid", xname);            
  if(access(fnm, 0) == 0)  
  {
    if(rep) printf("file %s exists. \n", xname);
    return 1;
  }
  else return 0;
} 

short copy_fid(old, new, rep)  /* copy .fid to .fid */
char *old, *new;
short  rep;
{
  char  cmd[MAXSTR];
  
  if(fid_exists(new, rep)) return 0;

  sprintf(cmd, "cp -r %s %s", old, new);   
  if(rep) printf("%s\n", cmd);
  system(cmd);

  return 1;  
}

void delete_fid(fname)
char *fname;
{
  char cmd[MAXSTR];
  
  sprintf(cmd, "rm -r %s\n", fname);
  system(cmd);
  
  return;
}

  
FILE *open_file(fname, mode)
char *fname, *mode;
{
  FILE *src;
    
  if((strcmp(mode,"r") == 0) && (access(fname, 0) != 0))
  {
    printf("\n open_file: cannot access %s\n", fname);
    return(NULL);
  }

  if ((src = fopen(fname, mode)) == NULL)       /* open 2D source file */
  {
    printf("\n open_file: cannot open %s \n", fname);
    return(NULL);
  }

  return src;
}


FILE *open_fid(xname, mode)
char *xname, *mode;
{
  char fname[MAXSTR];
  
  strcpy(fname, xname); 
  (void) check_fid_fname(fname);  
  
  return open_file(fname, mode);
}

FILE *open_data(xname, mode)
char *xname, *mode;
{
  char fname[MAXSTR];
  
  strcpy(fname, xname);
  (int) check_expdir_fname(fname);    

  return open_file(strcat(fname, "/datdir/data"), mode);
}


FILE *open_procpar(xfname, mode)
char *xfname, *mode;
{
  char fname[MAXSTR];
  
  strcpy(fname, xfname);
  (void) check_fname(fname);    
    
  return open_file(strcat(fname, "/procpar"), mode);
}


FILE *open_curpar(xfname, mode)
char *xfname, *mode;
{
  char fname[MAXSTR];
  
  strcpy(fname, xfname);
  (void) check_fname(fname);    
  
  return open_file(strcat(fname, "/curpar"), mode);
}


BLOCKHEADER **calloc_BH2d(nbheaders, nblocks)
int   nbheaders, nblocks;
{
  BLOCKHEADER **xarr;
  int  i;
  
  xarr = (BLOCKHEADER **) calloc(nbheaders, sizeof(BLOCKHEADER *));
  if (!xarr) 
  {
    printf("\nvnmrio: memory allocation failure. \n");
    exit(0);
  }
  else
  { 
    for(i=0; i<nbheaders; i++)
    {
      xarr[i] = (BLOCKHEADER *) calloc(nblocks, sizeof(BLOCKHEADER));
      if (!xarr[i])
      { 
        printf("\nvnmrio: memory allocation failure. \n");
        exit(0);
      }
    }
  }  
  
  return xarr;
}


void reset_C1d(xfid, xnp)
COMPLX  *xfid[];
int     xnp;
{
  int  i;
  
  for(i=0; i<xnp; i++)
  {
    (*xfid)[i].re = 0.0;
    (*xfid)[i].im = 0.0;
  }
  
  return;
}


void reset_C2d(xfid, xni, xnp)
int       xni, xnp;
COMPLX  **xfid[xni][xnp];
{
  int  i, j;
  
  for(i=0; i<xni; i++)
  {
    for(j=0; j<xnp; j++)
    {
      (**xfid)[i][j].re = 0.0;
      (**xfid)[i][j].im = 0.0;
    }
  }
  
  return;
}


void reset_C3d(xfid, xni2, xni, xnp)
int       xni2, xni, xnp;
COMPLX  ***xfid[xni2][xni][xnp];
{
  int  i, j, k;
  
  for(i=0; i<xni2; i++)
  {
    for(j=0; j<xni; j++)
    {
      for(k=0; k<xnp; k++)
      {
        (***xfid)[i][j][k].re = 0.0;
        (***xfid)[i][j][k].im = 0.0;
      }
    }
  }
  
  return;
}


void copy_C1d(new, orig, xnp)
COMPLX  *new[], *orig[];
int     xnp;
{
  int  i;   
  
  for(i=0; i<xnp; i++)
  {
    (*new)[i].re = (*orig)[i].re;  
    (*new)[i].im = (*orig)[i].im;
  }
  
  return;
}


void copy_C2d(xfid1, xfid2, xni, xnp)
int       xni, xnp;
COMPLX  **xfid1[xni][xnp], **xfid2;
{
  int  i, j;
  
  for(i=0; i<xni; i++)
  {
    for(j=0; j<xnp; j++)
    {
      (**xfid1)[i][j].re = xfid2[i][j].re;
      (**xfid1)[i][j].im = xfid2[i][j].im;
    }
  }
  
  return;
}


void copy_C3d(xfid1, xfid2, xni2, xni, xnp)
int        xni2, xni, xnp;
COMPLX  ***xfid1[xni2][xni][xnp], ***xfid2;
{
  int  i, j, k;
  
  for(i=0; i<xni2; i++)
  {
    for(j=0; j<xni; j++)
    {
      for(k=0; k<xnp; k++)
      {
        (***xfid1)[i][j][k].re = xfid2[i][j][k].re;
        (***xfid1)[i][j][k].im = xfid2[i][j][k].im;
      }
    }
  }
 
  return;
}


COMPLX **calloc_C2d(nblocks, npoints)
int nblocks, npoints;
{
  COMPLX **xarr;
  int  i;
  
  xarr = (COMPLX **) calloc(nblocks, sizeof(COMPLX *));
  if (!xarr) 
  {
    printf("\nvnmrio: memory allocation failure. \n");
    exit(0);
  }
  else
  { 
    for(i=0; i<nblocks; i++)
    {
      xarr[i] = (COMPLX *) calloc(npoints, sizeof(COMPLX));
      if (!xarr[i])
      { 
        printf("\nvnmrio: memory allocation failure. \n");
        exit(0);
      }
    }
  }  
  
  return xarr;
}

COMPLX *calloc_C1d(npoints)
int npoints;
{
  COMPLX *xarr;
  
  xarr = (COMPLX *) calloc(npoints, sizeof(COMPLX));
  if (!xarr)
  { 
    printf("\nvnmrio: memory allocation failure. \n");
    exit(0);
  }
  
  return xarr;
}

COMPLX ***calloc_C3d(ntr, nblocks, npoints)
int ntr, nblocks, npoints;
{
  COMPLX ***xarr;
  int  i, j;

  xarr = (COMPLX ***) calloc(ntr, sizeof(COMPLX **));
  if (!xarr) vn_error("calloc_C3d: memory allocation failure. ");
  else
  { 
    for(i=0; i<ntr; i++)
    {
      xarr[i] = (COMPLX **) calloc(nblocks, sizeof(COMPLX *));
      if (!xarr[i]) vn_error("calloc_C3d: memory allocation failure. ");
      else
      {
        for(j=0; j<nblocks; j++)
        {
          xarr[i][j] = (COMPLX *) calloc(npoints, sizeof(COMPLX));
          if (!xarr[i][j]) vn_error("calloc_C3d: memory allocation failure. ");
        }
      }
    }
  }  
  
  return xarr;
}


int *calloc_i1d(npoints)
int npoints;
{
  int *xarr;
  
  xarr = (int *) calloc(npoints, sizeof(int));
  if (!xarr)
  {
    printf("\nvnmrio: memory allocation failure.");
    exit(0);
  }
  
  return xarr;
}


double *calloc_1d(npoints)
int npoints;
{
  double *xarr;
  
  xarr = (double *) calloc(npoints, sizeof(double));
  if (!xarr)
  {
    printf("\nvnmrio: memory allocation failure.");
    exit(0);
  }
  
  return xarr;
}

double **calloc_2d(nblocks, npoints)
int nblocks, npoints;
{
  double **xarr;
  int  i;
  
  xarr = (double **) calloc(nblocks, sizeof(double *));
  if (!xarr) 
  {
    printf("\nvnmrio: calloc_2d memory allocation failure.");
    exit(0);
  }
  else
  { 
    for(i=0; i<nblocks; i++)
    {
      xarr[i] = (double *) calloc(npoints, sizeof(double));
      if (!xarr[i])
      {
        printf("\nvnmrio: memory allocation failure.");
        exit(0);
      }
    }
  }  
  
  return xarr;
}


float **calloc_f2d(nblocks, npoints)
int nblocks, npoints;
{
  float **xarr;
  int  i;
  
  xarr = (float **) calloc(nblocks, sizeof(float *));
  if (!xarr) 
  {
    printf("\nvnmrio: calloc_2d memory allocation failure.");
    exit(0);
  }
  else
  { 
    for(i=0; i<nblocks; i++)
    {
      xarr[i] = (float *) calloc(npoints, sizeof(float));
      if (!xarr[i])
      {
        printf("\nvnmrio: memory allocation failure.");
        exit(0);
      }
    }
  }  
  
  return xarr;
}


void show_bits(st)
short  st;
{
  int  nbits = 16;    /* number of bits displayed */
  int  i=1, j=1;

  j= 1 << nbits;
  while(i<j)
  {
    if (st&i) printf("1 ");
    else printf("0 ");
    i = i << 1;    
  }
  printf("\n");
  return;  
}


void check_sys_arch()
{
  struct utsname *s_uname;
                                     
  s_uname = (struct utsname *) malloc(sizeof(struct utsname));

  if((uname(s_uname) >= 0) && (!strncasecmp(s_uname->machine, "sun", 3)))         
    pc_bytes = 0;                                     
  else pc_bytes = 1;
  if(pc_bytes)
      printf("Little endean\n");
  else
      printf("Big endean\n");

}

unsigned long available_RAM()
{
#ifdef LINUX
  struct sysinfo sys_info;

  sysinfo(&sys_info);
  return sys_info.mem_unit*sys_info.totalram;
#else
  return 2000000000L;
#endif
}


void swapbytes(void *data, int size)
{
  unsigned char *c, cs;
  int i, j;
  
  c = (unsigned char *) data;
  
  j = size - 1; size /= 2;
  for (i=0; i<size; i++, j--)
    cs = c[i], c[i] = c[j], c[j] = cs;
}


void print_fheader(msg, xfhd)
char       *msg;
FILEHEADER *xfhd;
{
  printf("\n%s \n", msg);
  printf(" status  = %8d,    nbheaders       = %8d\n", xfhd->status, xfhd->nbheaders);
  printf(" nblocks = %8d     bytes per block = %8d\n", xfhd->nblocks, xfhd->bbytes);
  printf(" ntraces = %8d     bytes per trace = %8d\n", xfhd->ntraces, xfhd->tbytes);
  printf(" npoints = %8d     bytes per point = %8d\n", xfhd->np, xfhd->ebytes);
  printf(" vers_id = %8d \n", xfhd->vers_id);

  return;
}


void set_fhd(xfhd, np, ni)  /* np and ni is real np (not complex) */
FILEHEADER *xfhd;
int        np, ni;
{
  if((xfhd->status & 8) || (xfhd->status & 4))
    xfhd->ebytes = 4;
  else
    xfhd->ebytes = 2; 
  
  if(np > 0) xfhd->np = np; 
  if(ni > 0) xfhd->nblocks = ni; 
  xfhd->tbytes = xfhd->np*xfhd->ebytes;
  xfhd->bbytes = xfhd->ntraces*xfhd->tbytes + xfhd->nbheaders * sizeof(BLOCKHEADER);
}


int get_bswp_fheader(src, xfhd)      /* includes byte swap */
FILE        *src;
FILEHEADER  *xfhd;
{
  int      i, ii;
  short   jj;
  
  i = 0;
  if(fread(&ii, sizeof(int), 1, src) == 1) 
  { swapbytes(&ii, sizeof(int));  xfhd->nblocks = ii; i++;  }  
  if(fread(&ii, sizeof(int), 1, src) == 1) 
  { swapbytes(&ii, sizeof(int));  xfhd->ntraces = ii; i++;  }
  if(fread(&ii, sizeof(int), 1, src) == 1)  
  { swapbytes(&ii, sizeof(int));  xfhd->np = ii; i++;       } 
  if(fread(&ii, sizeof(int), 1, src) == 1)  
  { swapbytes(&ii, sizeof(int));  xfhd->ebytes = ii; i++;   }
  if(fread(&ii, sizeof(int), 1, src) == 1)  
  { swapbytes(&ii, sizeof(int));  xfhd->tbytes = ii; i++;   } 
  if(fread(&ii, sizeof(int), 1, src) == 1)  
  { swapbytes(&ii, sizeof(int));  xfhd->bbytes = ii; i++;   }  
  if(fread(&jj, sizeof(short), 1, src) == 1) 
  { swapbytes(&jj, sizeof(short)); xfhd->vers_id = jj; i++; }  
  if(fread(&jj, sizeof(short), 1, src) == 1) 
  { swapbytes(&jj, sizeof(short)); xfhd->status = jj; i++;  } 
  if(fread(&ii, sizeof(int), 1, src) == 1)  
  { swapbytes(&ii, sizeof(int)); xfhd->nbheaders = ii; i++; } 
  
  if(i < 9) return 0; 
  else      return 1;
}


int get_fheader(src, xfhd, rep)
FILE       *src;
FILEHEADER *xfhd;
short       rep;
{
  int     i;
  short  st;
  
  fseek(src, 0, 0);
  
  i = 0;
  
  if(!pc_bytes)
    i = fread(xfhd, sizeof(FILEHEADER), 1, src);
  else
    i = get_bswp_fheader(src, xfhd);
    
  if(i == 0) 
  {
    printf("\nfailed to read fid file header\n");
    return 0;
  }

  if((xfhd->status & 1)==0)
  {
    printf("\n get_fheader: Data file appears to be empty \n");
    print_fheader("READ DATA FILE HEADER", xfhd);
  }
  else if(xfhd->status < 0) 
    i=0;
  else if((i) && (rep))
  {
    print_fheader("READ DATA FILE HEADER", xfhd);
    if(rep > 1)
    {    
      st = xfhd->status;  
      if(st & 64) st=st^64, st=st^16;            /* allign with vnmr */
      printf("status bits: ");
      show_bits(st);
    }
  }
  return i;
}

int fwrite_fh(xfhd, size, np, src)
FILEHEADER *xfhd;
int         size, np;
FILE       *src;
{
  int      i, ii;
  short   jj;
  
  fseek(src, 0, 0);

  if(!pc_bytes) return fwrite(xfhd, size, np, src);
  else
  {  
    i = 0; 
    ii = xfhd->nblocks; swapbytes(&ii, sizeof(int)); 
    if(fwrite(&ii, sizeof(int), 1, src) == 1) i++;
    ii = xfhd->ntraces; swapbytes(&ii, sizeof(int)); 
    if(fwrite(&ii, sizeof(int), 1, src) == 1) i++;
    ii = xfhd->np; swapbytes(&ii, sizeof(int)); 
    if(fwrite(&ii, sizeof(int), 1, src) == 1) i++;
    ii = xfhd->ebytes; swapbytes(&ii, sizeof(int)); 
    if(fwrite(&ii, sizeof(int), 1, src) == 1) i++;
    ii = xfhd->tbytes; swapbytes(&ii, sizeof(int)); 
    if(fwrite(&ii, sizeof(int), 1, src) == 1) i++;
    ii = xfhd->bbytes; swapbytes(&ii, sizeof(int)); 
    if(fwrite(&ii, sizeof(int), 1, src) == 1) i++;
    jj = xfhd->vers_id; swapbytes(&jj, sizeof(short)); 
    if(fwrite(&jj, sizeof(short), 1, src) == 1) i++;
    jj = xfhd->status; swapbytes(&jj, sizeof(short)); 
    if(fwrite(&jj, sizeof(short), 1, src) == 1) i++;
    ii = xfhd->nbheaders; swapbytes(&ii, sizeof(int)); 
    if(fwrite(&ii, sizeof(int), 1, src) == 1) i++;
     
    if(i < 9) return 0; 
    else      return 1;  
  }
}


int put_fheader(src, xfhd, rep)
FILE       *src;
FILEHEADER *xfhd;
short       rep;
{
  int    i, j;
  short  st;

  if(rep)
  {
    print_fheader("WRITE DATA FILE HEADER", xfhd);
    if(rep > 1)
    {    
      st = xfhd->status;  
      if(st & 64) st=st^64, st=st^16;            /* allign with vnmr */
      printf("status bits: ");
      show_bits(st);
    }
  }

  i=0; j=xfhd->status;   
  if((j&1)==0)
  {
    printf("\n put_fheader: Data file appears to be empty \n");
    return(0);
  }
  else   
    i = fwrite_fh(xfhd, sizeof(FILEHEADER), 1, src);
  
  if(i == 0) 
    printf("\nfailed to write fid file header\n");
  
  return i;
}

void print_bheader(msg, sbhd)
char        *msg;
BLOCKHEADER *sbhd;
{
  short       st;
  
  st = sbhd->status;   
  if(st & 64) st=st^64, st=st^16;            /* allign with Vnmr */
  printf("\n%s \n", msg);
  printf(" status  = %8d,    scale = %8d\n", st, sbhd->scale);
  printf(" iblock  = %8d,    mode  = %8d\n", sbhd->iblock, sbhd->mode);
  printf(" lp      = %8.2f,    rp    = %8.2f\n", sbhd->lp, sbhd->rp);
  printf(" lvl     = %8.2f,    tlt   = %8.2f\n", sbhd->lvl, sbhd->tlt);
  printf(" ct      = %8d \n", sbhd->ct);
    
  return;
}

void reset_bheader(sbhd, ix)
BLOCKHEADER *sbhd;
short ix;
{  
  sbhd->iblock = ix; 
  sbhd->lp=0.0;  sbhd->rp=0.0;
  sbhd->lvl=0.0; sbhd->tlt=0.0;
    
  return;
}


int get_bswp_bheader(src, sbhd)      /* includes byte swap */
FILE        *src;
BLOCKHEADER *sbhd;
{
  int      i, ii;
  short    jj;
  float    ff;
  
  i = 0;
  if(fread(&jj, sizeof(short), 1, src) == 1) 
  { swapbytes(&jj, sizeof(short)); sbhd->scale = jj; i++;   }  
  if(fread(&jj, sizeof(short), 1, src) == 1) 
  { swapbytes(&jj, sizeof(short)); sbhd->status = jj; i++;  } 
  if(fread(&jj, sizeof(short), 1, src) == 1) 
  { swapbytes(&jj, sizeof(short)); sbhd->iblock = jj; i++;  } 
  if(fread(&jj, sizeof(short), 1, src) == 1) 
  { swapbytes(&jj, sizeof(short)); sbhd->mode = jj; i++;    }   
  if(fread(&ii, sizeof(int), 1, src) == 1) 
  { swapbytes(&ii, sizeof(int));  sbhd->ct = ii; i++;       } 
  if(fread(&ff, sizeof(float), 1, src) == 1) 
  { swapbytes(&ff, sizeof(float)); sbhd->lp = ff; i++;      } 
  if(fread(&ff, sizeof(float), 1, src) == 1) 
  { swapbytes(&ff, sizeof(float)); sbhd->rp = ff; i++;      } 
  if(fread(&ff, sizeof(float), 1, src) == 1) 
  { swapbytes(&ff, sizeof(float)); sbhd->lvl = ff; i++;     } 
  if(fread(&ff, sizeof(float), 1, src) == 1) 
  { swapbytes(&ff, sizeof(float)); sbhd->tlt = ff; i++;     } 
     
  if(i < 9) return 0; 
  else      return 1;
}


int get_bheader(FILE *src, BLOCKHEADER *sbhd, short rep)
{
  int i;  
  
  i = 0; 
  if(!pc_bytes)
    i = fread(sbhd, sizeof(BLOCKHEADER), 1, src);
  else
    i = get_bswp_bheader(src, sbhd); 
  
  if((i) && (rep))
    print_bheader("READ DATA BLOCK HEADER", sbhd);

  return i;
}

int fwrite_bh(sbhd, size, np, src)
BLOCKHEADER  *sbhd;
int size, np;
FILE *src;
{
  int      i, ii;
  short    jj;
  float    ff;
  

  if(!pc_bytes) return fwrite(sbhd, size, np, src);
  else
  {  
    i = 0;
    jj = sbhd->scale; swapbytes(&jj, sizeof(short));
    if(fwrite(&jj, sizeof(short), 1, src) == 1) i++;
    jj = sbhd->status; swapbytes(&jj, sizeof(short));
    if(fwrite(&jj, sizeof(short), 1, src) == 1) i++;
    jj = sbhd->iblock; swapbytes(&jj, sizeof(short));
    if(fwrite(&jj, sizeof(short), 1, src) == 1) i++;
    jj = sbhd->mode; swapbytes(&jj, sizeof(short));
    if(fwrite(&jj, sizeof(short), 1, src) == 1) i++;
    ii = sbhd->ct; swapbytes(&ii, sizeof(int));
    if(fwrite(&ii, sizeof(int), 1, src) == 1) i++;
    ff = sbhd->lp; swapbytes(&ff, sizeof(float));
    if(fwrite(&ff, sizeof(float), 1, src) == 1) i++;
    ff = sbhd->rp; swapbytes(&ff, sizeof(float));
    if(fwrite(&ff, sizeof(float), 1, src) == 1) i++;
    ff = sbhd->lvl; swapbytes(&ff, sizeof(float));
    if(fwrite(&ff, sizeof(float), 1, src) == 1) i++;
    ff = sbhd->tlt; swapbytes(&ff, sizeof(float));
    if(fwrite(&ff, sizeof(float), 1, src) == 1) i++;
     
    if(i < 9) return 0; 
    else      return 1;
  }
}

int put_bheader(src, sbhd, rep)
FILE        *src;
BLOCKHEADER *sbhd;
short        rep;
{
  int i;
  
  i = 0;
  i = fwrite_bh(sbhd, sizeof(BLOCKHEADER), 1, src);

  if((i) && (rep))
    print_bheader("WRITE DATA BLOCK HEADER", sbhd);

  return i;
}

void ddff(xmsg, xfhd, xfid)
char       *xmsg;
FILEHEADER *xfhd;
COMPLX     *xfid;
{
  int i, nx;
  
  nx = 18;
  if(xfhd->np < 36) nx = xfhd->np/2;
  
  (void) print_fheader(xmsg, xfhd); 
  for(i=0; i<nx; i++)
  {
    if(i%3 == 0) printf("%4d", i*6+1);
    printf("%12.4f %12.4f ", xfid[i].re, xfid[i].im);
    if(i%3 == 2) printf("\n");
  } 
  
  return;
}


int wtrace(xdir)
char  *xdir;
{
  FILE *src; 
  int   k, getval_s();
  char  fname[MAXSTR], trace[4];
  
  k=0;
  sprintf(fname, "%s/curpar", xdir);  
  if((src = open_fid(fname, "r")) == NULL)   /* open 2D source file */
  {
    sprintf(fname, "%s/procpar", xdir);
    if((src = open_fid(fname, "r")) == NULL)
      return 0;
  }  
    
  if(getval_s(src, "trace", trace))
  {
    if(trace[1] == '1') 
      k = 1;
    else if(trace[1] == '2')
      k = 2;
    if(DBUG) printf("wtrace: trace = %s\n", trace);  
  }
    
  fclose(src);
  
  return k;
}

int fread_tr(float *ffid[], int size, int np, FILE *src)
{
  int   i;
  float ff;
  
  if(!pc_bytes) return fread(ffid[0], size, np, src);
  else
  {
    for(i=0; i<np; i++) 
    {
      if((fread(&ff, size, 1, src)) < 1) break;
      swapbytes(&ff, size);
      (*ffid)[i] = ff;
    }
  } 
  
  return i;
}


double **get_phasefile2(src, nix, npx, rep)   /* get the phasefile type data */
FILE    *src;
int     *nix, *npx, rep;
{
  FILEHEADER  fhd;
  BLOCKHEADER bhd1, bhd2;
  int         i, j, k, m;
  float      *fdat;
  double     **ddat;
      
  fseek(src, 0, 0); 
  if(get_fheader(src, &fhd, rep) == 0)        /* read source file header */
    vn_abort(src, "\nfailed to get phasefile header \n");

  fdat = (float *) calloc(fhd.np, sizeof(float));
  ddat = calloc_2d(fhd.nblocks*fhd.ntraces, fhd.np);  
  m=0; j=0;
  if(DBUG) printf("reading part #1 \n");  
  for (i = 0; i < fhd.nblocks; i++)
  {
    get_bheader(src, &bhd1, rep);   
    if (fhd.nbheaders > 1)
      get_bheader(src, &bhd2, rep);
    for (j = 0; j < fhd.ntraces; j++) 
    {   
      if(fread_tr(&fdat[0], sizeof(float), fhd.np, src))  
      {
        for(k=0; k<fhd.np; k++)
          ddat[m][k] = (double) fdat[k];
        m++;
      } 
      else
        break;
    }
    if(j < fhd.ntraces)
      break; 
  } 
  k = i*j; 
  m = fhd.nblocks * fhd.ntraces;
  
  if(DBUG) printf("st=%d, st2=%d, k=%d, m=%d\n", bhd1.status, bhd2.status, k, m);

/* This condition seems to be inapplicable. This is where it goes wrong */
/* only the first part of the phasefile seems to be readable as documented */
/* Try using:  trace = 'f1' full f flush - in case the first part empty */

  if((bhd1.status == 0) && (bhd2.status == 0) && (k==m))  /* if trace = 'f2' */
  {
    m = 0; 
    if(DBUG) printf("reading part #2\n");
    for (i = 0; i < fhd.nblocks; i++)
    {
      get_bheader(src, &bhd1, rep); 
      if (fhd.nbheaders > 1)
        get_bheader(src, &bhd2, rep);        
      for (j = 0; j < fhd.ntraces; j++) 
      {   
        if(fread_tr(&fdat[0], sizeof(float), fhd.np, src))  
        {
          for(k=0; k<fhd.np; k++)
            ddat[m][k] = (double) fdat[k];
          m++;
        } 
        else
          break;
      } 
      if(j < fhd.ntraces)
        break; 
    }
    k = i*j; 
    m = fhd.nblocks * fhd.ntraces;
  }  
 
  if(k != m)
  {
    if (feof(src))  /* end of file */
      printf("\nEnd of file (block %d, trace %d). Done.\n", i, j);
    else
      printf("\nError while reading data (block %d, trace %d). Aborting.\n", i, j);           
  }     
  fclose(src);
  
  *npx = fhd.np;
  *nix = i*j;
    
  return ddat;
}  


double **get_phasefile(src, nix, npx, rep)  /* reads only one part of phasefile */
FILE    *src;
int     *nix, *npx, rep;
{
  FILEHEADER  fhd;
  BLOCKHEADER bhd1, bhd2;
  int         i, j, k, m;
  float      *fdat;
  double     **ddat;
      
  fseek(src, 0, 0);
  if(get_fheader(src, &fhd, rep) == 0)         /* read source file header */
    vn_abort(src, "\nfailed to get phasefile header \n");

  fdat = (float *) calloc(fhd.np, sizeof(float));
  ddat = calloc_2d(fhd.nblocks*fhd.ntraces, fhd.np);  
  m = 0; j=0;
  for (i=0; i<fhd.nblocks; i++)
  {
    get_bheader(src, &bhd1, rep);
    if (fhd.nbheaders > 1)
      get_bheader(src, &bhd2, rep);
    for (j=0; j<fhd.ntraces; j++) 
    {   
      if(fread_tr(&fdat[0], sizeof(float), fhd.np, src))  
      {
        for(k=0; k<fhd.np; k++)
          ddat[m][k] = (double) fdat[k];
        m++;
      } 
      else
        break;
    }
    if(j < fhd.ntraces)
      break; 
  } 
  k = i*j; 
  m = fhd.nblocks * fhd.ntraces;
  
  if(DBUG) printf("st=%d, st2=%d\n", bhd1.status, bhd2.status);
 
  if(k != m)
  {
    if (feof(src))  /* end of file */
      printf("\nEnd of file (block %d, trace %d). Done.\n", i, j);
    else
      printf("\nError while reading data (block %d, trace %d). Aborting.\n", i, j);           
  }     
  fclose(src);
  
  *npx = fhd.np;
  *nix = i*j;
    
  return ddat;
}  


double *get_trace(src, jtr, fx, npx)
FILE       *src;
int        *npx, jtr, fx;
{
  int       j, fn, fn1;
  double  **phf, *trx;
  
  phf = get_phasefile2(src, &fn, &fn1, 0);
  trx = NULL;

  j=0;   
  if(fx==1) 
  {
    if(jtr>fn) fn1=0;
    trx = calloc_1d(fn1);
    for (j=0; j<fn1; j++) 
      trx[j] = phf[jtr][j];
    *npx = fn1;
  }
  else if(fx==2)
  {
    if(jtr>fn1) fn=0;
    trx = calloc_1d(fn);
    for (j=0; j<fn; j++) 
      trx[j] = phf[j][jtr];
    *npx = fn;
  }
  free(phf);
    
  return trx;
}

double **get_rgn(src, fx, i0, ii, j0, jj)  /* i0=f1, j0=f2 */
FILE       *src;
int        fx, i0, ii, j0, jj;
{
  int       i, j, k1, k2, fn, fn1;
  double  **phf, **trx;
  
  fn=0; fn1=0; trx=NULL;
  phf = get_phasefile2(src, &fn, &fn1, 0);
  
  j=0;   
  if(fx==2) 
  {  
    trx = calloc_2d(ii, jj);
    for(i=0, k1=j0; i<jj; i++, k1++)
    {
      for (j=0, k2=i0; j<ii; j++, k2++) 
        trx[j][i] = phf[k1][k2];
    }
  }
  else if(fx==1)
  {  
    trx = calloc_2d(jj, ii);
    for(i=0, k1=j0; i<jj; i++, k1++)
    {
      for (j=0, k2=i0; j<ii; j++, k2++) 
        trx[i][j] = phf[k2][k1];
    }
  }
  free(phf);
    
  return trx;
}

double get_pt2d(src, i0, j0)  /* i0=f1, j0=f2 */
FILE       *src;
int        i0, j0;
{
  int       fn, fn1;
  double  **phf;
  
  fn=0; fn1=0;
  phf = get_phasefile2(src, &fn, &fn1, 0);
      
  return phf[i0][j0];
}


int get_bswp_fid(FILE *src, FILEHEADER xfhd, COMPLX *dfid[])
{
  int          i, j, np, ii;
  short        jj;
  float        ff;
  char         dp;
  COMPLX       ctm;    
  
  if(xfhd.status & 8)
    dp = 'f';
  else if(xfhd.status & 4)
    dp = 'y';
  else
    dp = 'n'; 

  i=0; j=0;
  np = xfhd.np/2;   
  switch(dp)
  {
    case 'y' :                      /* read dp-FID */
            
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          fread(&ii, sizeof(int), 1, src); swapbytes(&ii, sizeof(int)); ctm.re = (double) ii;
          fread(&ii, sizeof(int), 1, src); swapbytes(&ii, sizeof(int)); ctm.im = (double) ii;
          (*dfid)[j] = ctm;
        }
      }
      break;
      
    case 'f' :                      /* read fp-FID */  
          
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          fread(&ff, sizeof(float), 1, src); swapbytes(&ff, sizeof(float)); ctm.re = (double) ff;
          fread(&ff, sizeof(float), 1, src); swapbytes(&ff, sizeof(float)); ctm.im = (double) ff;
          (*dfid)[j] = ctm;  
        }
      }
      break;
      

    case 'n' :                      /* read sp-FID */         
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          fread(&jj, sizeof(short), 1, src); swapbytes(&jj, sizeof(short)); ctm.re = (double) jj;
          fread(&jj, sizeof(short), 1, src); swapbytes(&jj, sizeof(short)); ctm.im = (double) jj;
          (*dfid)[j] = ctm;
        }
      }
      break;
  }

  return i*j;
}  


int get_fid(FILE *src, FILEHEADER xfhd, COMPLX *dfid[])
{
  int          i, j, np;
  char         dp;
  DATA         fid;
  COMPLX       ctm;    

  if(pc_bytes) return get_bswp_fid(src, xfhd, dfid);
  
  if(xfhd.status & 8)
    dp = 'f';
  else if(xfhd.status & 4)
    dp = 'y';
  else
    dp = 'n'; 

  i=0; j=0;
  np = xfhd.np/2;
  switch(dp)
  {
    case 'y' :                      /* read dp-FID */
            
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          fread(&fid.dp, sizeof(DPFID), 1, src);
          ctm.re = (double) fid.dp.re;
          ctm.im = (double) fid.dp.im;
          (*dfid)[j] = ctm;
        }
      }
      break;
      
    case 'f' :                      /* read fp-FID */  
          
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          fread(&fid.fp, sizeof(FPFID), 1, src);  	  
          ctm.re = (double) fid.fp.re;
          ctm.im = (double) fid.fp.im; 
          (*dfid)[j] = ctm;
        }
      }
      break;
      

    case 'n' :                      /* read sp-FID */         
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          fread(&fid.sp, sizeof(SPFID), 1, src);
          ctm.re = (double) fid.sp.re;
          ctm.im = (double) fid.sp.im;
          (*dfid)[j] = ctm;
        }
      }
      break;
  }
  
  return i*j;
}  


int get_fids(src, xfhd, xbhd, xfid, nx, rep)   /* read nx fids from the data file */
FILE         *src;
FILEHEADER    xfhd;
int           nx;
short         rep;
BLOCKHEADER **xbhd[xfhd.nbheaders][nx];
COMPLX      **xfid[nx][xfhd.np/2];                         
{
  int          i, k;             
  
  for(k=0; k<nx; k++)
  {
    for(i=0; i<xfhd.nbheaders; i++)
    {
      if(!get_bheader(src, &(**xbhd)[i][k], rep)) break; 
    } 
    if((i<xfhd.nbheaders) || (get_fid(src, xfhd, &(**xfid)[k]) == 0)) break; 
  }
  
  return k;   /* return the number of successfully read fids */
}  


COMPLX *read_1dfid(dir, xfhd, xbhd1, xbhd2, rep)   /* read 1d fid from file */
char        *dir;
FILEHEADER  *xfhd;
BLOCKHEADER *xbhd1, *xbhd2;
int          rep;
{
  FILE     *src;  
  char      fname[MAXSTR];
  int       j;
  COMPLX   *dfid;

  sprintf(fname, "%s", dir); 
  if((src = open_fid(fname, "r")) == NULL)
    exit(0);

  if(get_fheader(src, xfhd, rep) == 0)        /* read source file header */
    vn_abort(src, "\nfailed to get fid file header \n");

  dfid = calloc_C1d(xfhd->np/2);  
   
  get_bheader(src, xbhd1, rep);   /* return the first block headers */
  if (xfhd->nbheaders > 1)
    get_bheader(src, xbhd2, rep);      
  j = get_fid(src, *xfhd, &dfid);       /* get the first fid here */
  
  fclose(src);  
      
  return dfid;
}  


COMPLX **read_data(dir, xfhd, xbhd1, xbhd2, rep)   /* read traces from datadir/phasefile */
char        *dir;
FILEHEADER  *xfhd;
BLOCKHEADER *xbhd1, *xbhd2;
int          rep;
{
  FILE     *src;  
  char      fname[MAXSTR];
  int       i, j;
  COMPLX  **dfid;
  BLOCKHEADER *bhd1, *bhd2;

  sprintf(fname, "%s", dir);  
  if((src = open_data(fname, "r")) == NULL)
    exit(0);

  if(get_fheader(src, xfhd, rep) == 0)        /* read source file header */
    vn_abort(src, "\nfailed to get fid file header \n");

  dfid = calloc_C2d(xfhd->nblocks*xfhd->ntraces, xfhd->np);  
   
  get_bheader(src, xbhd1, rep);   /* return the first block headers */
  if (xfhd->nbheaders > 1)
    get_bheader(src, xbhd2, rep);      
  j = get_fid(src, *xfhd, &dfid[0]);       /* get the first fid here */
  
  for (i = 1; i < xfhd->nblocks; i++)
  {
    get_bheader(src, &bhd1, rep);   
    if (xfhd->nbheaders > 1)
      get_bheader(src, &bhd2, rep);      
    j = get_fid(src, *xfhd, &dfid[i]);
  }        
  fclose(src);  
      
  return dfid;
}  


COMPLX *get_FID(src, xfhd, ok)   /* read nx fids from the data file */
FILE         *src;
FILEHEADER    xfhd;
int          *ok;
{
  int      i, np;
  COMPLX  *dfid;
  
  np = xfhd.np/2;
  dfid = calloc_C1d(np);
    
  i = get_fid(src, xfhd, &dfid);
  
  *ok = i;
  if(i<np) *ok = 0; 
   
  return dfid;   
}  


int get_F1plane3d(src, xfhd, xbhd, xfid1, xfid2, nx, iph, rep)   /* read nx fids from the data file */
FILE         *src;
FILEHEADER    xfhd;
BLOCKHEADER  *xbhd;
int           nx, iph;
COMPLX      **xfid1, **xfid2;                            
short         rep;
{
  int          i, ok;
  BLOCKHEADER  bhd;  
  
  ok = 0; i = 0; 
  
  if(iph == 0)   /* array='phase' */
  {
    for(i=0; i<nx; i++)
    {
      if(!get_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) get_bheader(src, &bhd, rep);  
      xfid1[i] = get_FID(src, xfhd, &ok); if(!ok) break;

      if(!get_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) get_bheader(src, &bhd, rep);  
      xfid2[i] = get_FID(src, xfhd, &ok); if(!ok) break;
    } 
  }
  else if(iph == 12)   /* array='phase,phase2' */
  {
    for(i=0; i<nx; i++)
    {
      if(!get_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) get_bheader(src, &bhd, rep);        
      xfid1[2*i] = get_FID(src, xfhd, &ok); if(!ok) break; 

      if(!get_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) get_bheader(src, &bhd, rep);  
      xfid2[2*i] = get_FID(src, xfhd, &ok); if(!ok) break;
      
      if(!get_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) get_bheader(src, &bhd, rep);  
      xfid1[2*i+1] = get_FID(src, xfhd, &ok); if(!ok) break;
      
      if(!get_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) get_bheader(src, &bhd, rep);  
      xfid2[2*i+1] = get_FID(src, xfhd, &ok); if(!ok) break; 
    }
  }
  else if(iph == 21)  /* array = 'phase2,phase' */
  {
    for(i=0; i<nx; i++)
    {
      if(!get_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) get_bheader(src, &bhd, rep);  
      xfid1[2*i] = get_FID(src, xfhd, &ok); if(!ok) break;

      if(!get_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) get_bheader(src, &bhd, rep);  
      xfid1[2*i+1] = get_FID(src, xfhd, &ok); if(!ok) break;
      
      if(!get_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) get_bheader(src, &bhd, rep);  
      xfid2[2*i] = get_FID(src, xfhd, &ok); if(!ok) break;
      
      if(!get_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) get_bheader(src, &bhd, rep);  
      xfid2[2*i+1] = get_FID(src, xfhd, &ok); if(!ok) break;
    } 
  }
  
  return i;   
}  


COMPLX **read_2dfid(dir, xfhd, xbhd1, xbhd2, rep)   /* read 2d fid from file */
char        *dir;
FILEHEADER  *xfhd;
BLOCKHEADER *xbhd1, *xbhd2;
int          rep;
{
  FILE     *src;  
  char      fname[MAXSTR];
  int       i, j;
  COMPLX  **dfid;
  FILEHEADER   fhd;
  BLOCKHEADER  bhd1, bhd2;

  i=0;
  sprintf(fname, "%s", dir); 
  if((src = open_fid(fname, "r")) == NULL)
    exit(0);

  if(get_fheader(src, &fhd, rep) == 0)        /* read source file header */
    vn_abort(src, "\nfailed to get fid file header \n");

 *xfhd = fhd;
  dfid = calloc_C2d(fhd.nblocks*fhd.ntraces, fhd.np);   
   
  get_bheader(src, xbhd1, rep);   /* return the first block headers */ 
  if (fhd.nbheaders > 1)
    get_bheader(src, xbhd2, rep);       
  j = get_fid(src, fhd, &dfid[0]);       /* get the first fid here */
   
  for (i=1; i<fhd.nblocks; i++)
  {
    get_bheader(src, &bhd1, rep);   
    if (fhd.nbheaders > 1)
      get_bheader(src, &bhd2, rep);      
    j = get_fid(src, fhd, &dfid[i]); 
  }        
  fclose(src);  
      
  return dfid;
}  


COMPLX *read_1d(dir, xfhd, xbhd1, xbhd2, rep)   /* read 1d spectrum from datadir/phasefile */
char        *dir;
FILEHEADER  *xfhd;
BLOCKHEADER *xbhd1, *xbhd2;
int          rep;
{
  FILE     *src;  
  char      fname[MAXSTR];
  int       j;
  COMPLX   *dfid;

  sprintf(fname, "%s", dir);  
  if((src = open_data(fname, "r")) == NULL)
    exit(0);

  if(get_fheader(src, xfhd, rep) == 0)        /* read source file header */
    vn_abort(src, "\nfailed to get fid file header \n");

  dfid = calloc_C1d(xfhd->np/2);  
   
  get_bheader(src, xbhd1, rep);   /* return the first block headers */
  if (xfhd->nbheaders > 1)
    get_bheader(src, xbhd2, rep);      
  j = get_fid(src, *xfhd, &dfid);               /* get the data here */
  
  fclose(src);  
      
  return dfid;
}  


int put_bswp_fid(src, xfhd, dfid)
FILE       *src;
FILEHEADER  xfhd;
COMPLX      dfid[];
{
  int    i, j, np, ii;
  short  jj;
  float  ff;
  char   dp;
  
  if(xfhd.status & 8)
    dp = 'f';
  else if(xfhd.status & 4)
    dp = 'y';
  else
    dp = 'n';

  i=0; j=0;
  np = xfhd.np/2;
  switch(dp)
  {
    case 'y' :                      /* read dp-FID */
            
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          ii = (int) dfid[j].re; swapbytes(&ii, sizeof(int)); fwrite(&ii, sizeof(int), 1, src);
          ii = (int) dfid[j].im; swapbytes(&ii, sizeof(int)); fwrite(&ii, sizeof(int), 1, src);
        }
      }
      break;
      
    case 'f' :                      /* read fp-FID */  
          
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          ff = (float) dfid[j].re; swapbytes(&ff, sizeof(float)); fwrite(&ff, sizeof(float), 1, src);
          ff = (float) dfid[j].im; swapbytes(&ff, sizeof(float)); fwrite(&ff, sizeof(float), 1, src);
        }
      }
      break;
      

    case 'n' :                      /* read sp-FID */         
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          jj = (short) dfid[j].re; swapbytes(&jj, sizeof(short)); fwrite(&jj, sizeof(short), 1, src);
          jj = (short) dfid[j].im; swapbytes(&jj, sizeof(short)); fwrite(&jj, sizeof(short), 1, src);
        }
      }
      break;
  }
  
  return i*j;
}  


int put_fid(src, xfhd, dfid)
FILE       *src;
FILEHEADER  xfhd;
COMPLX     *dfid;
{
  int          i, j, np;
  char         dp;
  DATA         fid;

  if(pc_bytes) return put_bswp_fid(src, xfhd, dfid);
  
  if(xfhd.status & 8)
    dp = 'f';
  else if(xfhd.status & 4)
    dp = 'y';
  else
    dp = 'n';
  i=0; j=0;
  np = xfhd.np/2;
  switch(dp)
  {
    case 'y' :                      /* read dp-FID */
            
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          fid.dp.re = (int) dfid[j].re;
          fid.dp.im = (int) dfid[j].im;
          fwrite(&fid.dp, sizeof(DPFID), 1, src);
        }
      }
      break;
      
    case 'f' :                      /* read fp-FID */  
          
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          fid.fp.re = (float) dfid[j].re;
          fid.fp.im = (float) dfid[j].im;
          fwrite(&fid.fp, sizeof(FPFID), 1, src);
        }
      }
      break;
      

    case 'n' :                      /* read sp-FID */         
      for(i=0; i<xfhd.ntraces; i++)
      {
        for (j=0; j <np; j++) 
        {
          fid.sp.re = (short) dfid[j].re;
          fid.sp.im = (short) dfid[j].im;
          fwrite(&fid.sp, sizeof(SPFID), 1, src);
        }
      }
      break;
  }
  
  return i*j;
}  

int put_fids(src, xfhd, xbhd, xfid, nx, rep)   /* read nx fids from the data file */
FILE         *src;
FILEHEADER    xfhd;
COMPLX      **xfid;
int           nx;
short         rep;
BLOCKHEADER **xbhd[xfhd.nbheaders][nx];
{
  int          i, k, ii;

  ii = xfhd.ntraces*xfhd.np;  

  for(k=0; k<nx; k++)
  {
    for(i=0; i<xfhd.nbheaders; i++)
    {
      if(!put_bheader(src, &(**xbhd)[i][k], rep)) break;
    }
    if((i<xfhd.nbheaders) || (put_fid(src, xfhd, xfid[k]) == 0)) break;
  }
  
  return k;
}  


void write_2dfid(dir, xfhd, xbhd, xbhd2, dfid, rep)  /* write 2d fid */
char       *dir;
FILEHEADER  xfhd;
BLOCKHEADER xbhd, xbhd2;
COMPLX    **dfid;
int         rep;
{
  FILE   *trg;
  char    fname[MAXSTR];
  int     i, j;
  
  sprintf(fname, "%s", dir); 
  trg = open_fid(fname, "w");
      
  if(put_fheader(trg,&xfhd, rep) == 0)        /* read source file header */
    vn_abort(trg, "\nfailed to write fid file header \n");
    
  xbhd.iblock = 1; xbhd2.iblock = 1;   
  for (i = 0; i < xfhd.nblocks; i++, xbhd.iblock++, xbhd2.iblock=xbhd.iblock)                          
  {  
    put_bheader(trg,&xbhd, rep);   
    if (xfhd.nbheaders > 1)
      put_bheader(trg,&xbhd2, rep);      
    j = put_fid(trg, xfhd, dfid[i]); 
  }  
  fclose(trg);

  return;
}


int put_F1plane3d(src, xfhd, xbhd, xfid1, xfid2, nx, iph, rep)   /* read nx fids from the data file */
FILE         *src;
FILEHEADER    xfhd;
BLOCKHEADER  *xbhd;
int           nx, iph;
COMPLX      **xfid1, **xfid2;                            
short         rep;
{
  int          i;
  
  i=0;
  if(iph == 0)   /* array='phase' */
  {
    for(i=0; i<nx; i++)
    {
      if(!put_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) put_bheader(src, xbhd, rep);  
      if(!put_fid(src, xfhd, xfid1[i])) break; xbhd->iblock++;

      if(!put_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) put_bheader(src, xbhd, rep);  
      if(!put_fid(src, xfhd, xfid2[i])) break; xbhd->iblock++;
    } 
  }
  else if(iph == 12)   /* array='phase,phase2' */
  {
    for(i=0; i<nx; i++)
    {
      if(!put_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) put_bheader(src, xbhd, rep);  
      if(!put_fid(src, xfhd, xfid1[2*i])) break; xbhd->iblock++;

      if(!put_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) put_bheader(src, xbhd, rep);  
      if(!put_fid(src, xfhd, xfid2[2*i])) break; xbhd->iblock++;

      if(!put_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) put_bheader(src, xbhd, rep);  
      if(!put_fid(src, xfhd, xfid1[2*i+1])) break; xbhd->iblock++;

      if(!put_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) put_bheader(src, xbhd, rep);  
      if(!put_fid(src, xfhd, xfid2[2*i+1])) break; xbhd->iblock++;
    }
  }
  else if(iph == 21)  /* array = 'phase2,phase' */
  {
    for(i=0; i<nx; i++)
    {
      if(!put_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) put_bheader(src, xbhd, rep);  
      if(!put_fid(src, xfhd, xfid1[2*i])) break; xbhd->iblock++;

      if(!put_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) put_bheader(src, xbhd, rep);  
      if(!put_fid(src, xfhd, xfid1[2*i+1])) break; xbhd->iblock++;
      
      if(!put_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) put_bheader(src, xbhd, rep);  
      if(!put_fid(src, xfhd, xfid2[2*i])) break; xbhd->iblock++;
      
      if(!put_bheader(src, xbhd, 0)) break; 
      if(xfhd.nbheaders>1) put_bheader(src, xbhd, rep);  
      if(!put_fid(src, xfhd, xfid2[2*i+1])) break; xbhd->iblock++;
    } 
  }
  
  return i;   
}  


int cutstr2(str1, str2, ch)
char *str1, *str2, ch;
{
  int i, j, k;

  if ((j = strlen(str2)) == 0) return 0;
  i = j - 1;
  while((i > 0) && (str2[i] != ch)) i--;
  if (i == 0) return 0;
  strcpy(str1, str2); 
  str1[i] = '\0';
  for (k = 0, i++; i < j; i++, k++) { str2[k] = str2[i]; }
  str2[k] = '\0'; 
  return k;
}

int findpar(fnm, str, val)  /* find parameter in xx.inp file */
FILE *fnm;
char *str, *val;
{
  char chr[MAXSTR];

  fseek(fnm, 0, 0); 
  while (fscanf(fnm, "%s", chr) != EOF)
  {
    while ((chr[0] == '#') || (chr[0] == '('))
    {
      fgets(chr, MAXSTR, fnm); fscanf(fnm, "%s", chr);
    }
    if (chr[0] == str[0])
    {
      if (strcmp(chr,str) == 0)
      {
        fscanf(fnm, "%s", chr); 
        if (chr[0] == '=') 
        {
          fscanf(fnm, "%s", val); 
          return 1;
        }
      } 
      else if ((cutstr2(val, chr, '=')) && (strcmp(val, str) == 0))
      {
        strcpy(val, chr); 
        return 1;
      } 
    }
  } 
  return (0);
}



int getprm(fnm, prm, pval)   /* returns parameter sub-type or array size  */
FILE   *fnm;                 /* 0 - not active, -1 not found */
char   *prm, *pval;
{
  int     i=-1, ptyp=0, k=0;
  char    pstr[MAXSTR], pnam[MAXSTR], s[MAXSTR], *ch;
  
  fseek(fnm, 0, 0); 
  while (fgets(pstr, MAXSTR, fnm))		/* find parameter */
  {    
    if((pstr[0] == prm[0]) && (pstr[1] == prm[1]) && 
    (sscanf(pstr, "%s %d %s %s %s %s %s %s %s %d", 
     pnam, &ptyp, s, s, s, s, s, s, s, &k)==10) && 
    (strcmp(pnam,prm) == 0))
    {
      fgets(pstr, MAXSTR, fnm); 
      if(sscanf(pstr, "%d %s\n", &i, pval) < 2)
      {
        printf("getprm: getval for %s failed\n", prm);
        return 0;
      }
      else if(i>1)  /* return the array size */
        printf("Warning: parameter %s is arrayed\n", prm);
      else if(k<1)
        i=0;
      else
      {
        i=ptyp;     /* otherwise return the parameter type */
        if(ptyp == 2)   
        {          
          ch = strrchr(pstr, '"'); ch++; *ch = '\0';
          ch = strchr(pstr, '"'); strcpy(pval, ch);          
        }
      }
      if(DBUG==1) printf("getprm: %s = %s (%d)\n", prm, pval, i);
      break;
    }
  }   
  
  return i;
}

int getstat(fnm, prm)
FILE   *fnm;
char   *prm;
{
    int     i=-1, ptyp=0, k=0;
    char    pstr[MAXSTR], pnam[MAXSTR], s[MAXSTR];
    char    pval[MAXSTR];
    fseek(fnm, 0, 0);
    while (fgets(pstr, MAXSTR, fnm))      /* find parameter */
    {
      if((pstr[0] == prm[0]) && (pstr[1] == prm[1]) &&
      (sscanf(pstr, "%s %d %s %s %s %s %s %s %s %d",
       pnam, &ptyp, s, s, s, s, s, s, s, &k)==10) &&
      (strcmp(pnam,prm) == 0))
      {
        fgets(pstr, MAXSTR, fnm);
        if(sscanf(pstr, "%d %s\n", &i, pval) < 2)
        {
          printf("getprm: getval for %s failed\n", prm);
          i=0;
        }
        else if(k<1){
          i=k;
        }
        if(DBUG==1) printf("getstat: %s = %s (%d)\n", prm, pval, i);
        break;
      }
    }
    return i;

}

int getval(fnm, prm, dval)
FILE   *fnm;
char   *prm;
double *dval;
{
  int     ptyp=0;
  char    pval[MAXSTR];
  double  val=0.0;

  ptyp = getprm(fnm, prm, pval);  
  
  if(((ptyp == 1) || (ptyp == 3) || (ptyp == 5)  || (ptyp == 6)  || 
  (ptyp == 7)) && (sscanf(pval, "%lf", &val) > 0))
    *dval = val;
  else if(ptyp < 1)
    return ptyp;
  else
    return 0;

  return 1;
}

int getprm_d(dir, prm, dval)
char   *dir, *prm;
double *dval;
{
  int      i;
  FILE	  *fil;
  char     fnm[MAXSTR];

  strcat(strcpy(fnm, dir), "/procpar");

  if((fil=fopen(fnm,"r"))==NULL) 
  { 
    printf("fopen %s failed\n", fnm); 
    return 0; 
  }
  
  i = getval(fil, prm, dval);
  fclose(fil);
  
  return i;
}

int getval_i(fnm, prm, ival)
FILE   *fnm;
char   *prm;
int    *ival;
{
  int     ptyp=0, val=0;
  char    pval[MAXSTR];

  ptyp = getprm(fnm, prm, pval);  
  
  if((ptyp == 7) && (sscanf(pval, "%d", &val) > 0))
    *ival = val;
  else if((ptyp == 1) && (sscanf(pval, "%d", &val) > 0))
    *ival = val, printf("   !!! Warning: unexpected type for %s : %d\n", prm, ptyp);
  else if(ptyp < 1)
    return ptyp;
  else  
    return 0;

  return 1;
}

int getval_s(fnm, prm, sval)
FILE   *fnm;
char   *prm, *sval;
{
  int     ptyp=0;
  char    pval[MAXSTR], *ch;

  ptyp = getprm(fnm, prm, pval);  
        
  if((ptyp == 2) || (ptyp == 4))
  {
    ch = strrchr(pval, '"'); *ch = '\0';
    ch = strchr(pval, '"'); ch++; 
    strcpy(sval, ch);             
    return strlen(sval);
  }  
  else if(ptyp < 1)
    return ptyp;
  else  
    return 0;
}

int get_np(dir, sw)   /* get the basic acquisition parameters */
char   *dir;
double *sw;
{
  FILE  *src; 
  int    np;
  double a;
  
  if((src = open_procpar(dir, "r")) == NULL) exit(0);
  
  np=0; a=0.0;          
  if(getval_i(src, "np", &np) < 1)   printf("get_nps: np not found. ");  
  if(getval(src, "sw", &a) < 1)   printf("get_nps: sw not found. ");  
   
  *sw = a;    
  fclose(src);
     
  return np;
}

void get_ni(dir, npx, nix)   /* get the basic acquisition parameters */
char   *dir;
int    *npx, *nix;
{
  FILE  *src; 
  int    ni, np;
  
  if((src = open_procpar(dir, "r")) == NULL) exit(0);
  
  np=0; ni=0;          
  if(getval_i(src, "np", &np) < 1)   printf("get_nps: np not found. ");  
  if(getval_i(src, "ni", &ni) < 1)   printf("get_nps: ni not found. "); 
      
  fclose(src);
   
  *npx = np; *nix = ni; 
  
  return;
}


int get_dr1(dir, sw1)   /* get ni and sw1, assuming complex np */
char   *dir;
double *sw1;
{
  FILE  *src; 
  int    ni;
  double a;
  
  if((src = open_procpar(dir, "r")) == NULL) exit(0);
  
  ni=0; a=0.0;          
  if(getval_i(src, "ni", &ni) < 1)   printf("get_nps: ni not found. ");  
  if(getval(src, "sw1", &a) < 1)   printf("get_nps: sw not found. ");  
   
  *sw1 = a;    
  fclose(src);
     
  return ni;
}


int get_ni2(dir, npx, nix, ni2x)   /* get the basic acquisition parameters */
char   *dir;
int    *npx, *nix, *ni2x;
{
  FILE  *src; 
  char   array[MAXSTR];
  int    ni2, ni, np, phase;
  
  if((src = open_procpar(dir, "r")) == NULL) exit(0);
  
  np=0; ni=0; ni2=0;         
  if(getval_i(src, "np", &np) < 1)   printf("get_nps: np not found. ");  
  if(getval_i(src, "ni", &ni) < 1)   printf("get_nps: ni not found. "); 
  if(getval_i(src, "ni2", &ni2) < 1) printf("get_nps: ni2 not found. "); 

  phase=0;
  if(getval_s(src, "array", array) > 0)
  {
    if(strcmp(array, "phase,phase2") == 0)
      phase=1;
    else if(strcmp(array, "phase2,phase") == 0)
      phase=0;
    else
      vn_abort(src, "Unexpected array. ");
  }
  else vn_abort(src, "Problem retrieving array."); 
      
  fclose(src);
   
  *npx = np; *nix = ni; *ni2x = ni2;
  
  return phase;
}


int get_ni3(dir, npx, nix, ni2x, ni3x, array)   /* get the basic acquisition parameters */
char   *dir, *array;
int    *npx, *nix, *ni2x, *ni3x;
{
  FILE  *src; 
  int    ni3, ni2, ni, np, dim;
  
  if((src = open_procpar(dir, "r")) == NULL) exit(0);
  
  np=0; ni=0; ni2=0; ni3=0;        

  if (getval_s(src, "array", array) < 1)
  {
    printf("getval_s: array parameter not found. "); 
    return 0;  
  }
  
  if(getval_i(src, "np", &np) < 1)   
  {
    printf("getval_i: np not found. "); 
    return 0;
  }
   
  if((getval_i(src, "acqdim", &dim) < 1) || (dim < 1)) return 0;
  if(getval_i(src, "ni", &ni) < 1) ni = 0;  
  if(getval_i(src, "ni2", &ni2) < 1) ni2 = 0; 
  if(getval_i(src, "ni3", &ni3) < 1) ni3 = 0; 
      
  fclose(src);
   
  *npx = np; *nix = ni; *ni2x = ni2, *ni3x = ni3;
  
  return dim;
}



int turn_off(dir, prm)   /* returns: 0 - not set, 1 set OK */
char   *dir, *prm;
{
  FILE	  *fnm, *ftmp;
  int     ptyp=0, j=0, k=0, ns=1, ok=0;
  char    pstr[MAXSTR], pnam[MAXSTR], s[MAXSTR];

  strcat(strcpy(pstr, dir), "/procpar");
  strcat(strcpy(pnam, dir), "/tmp");

  rename(pstr, pnam);

  if((fnm=fopen(pstr,"w"))==NULL) 
  { 
    printf("fopen %s failed\n", pstr); 
    exit(1); 
  }
  if((ftmp=fopen(pnam,"r"))==NULL) 
  { 
    printf("fopen %s failed\n", pnam); 
    exit(1); 
  }

  ns=1; ok=0;
  while (fgets(pstr, 512, ftmp))		/* fix parameters */
  {    
    if((ns) && (pstr[0] == prm[0]) && (pstr[1] == prm[1]) && 
       (sscanf(pstr, "%s %d %s %s %s %s %s %s %d %s", 
        pnam, &ptyp, s, s, s, s, s, s, &k, s)==10) && 
       (strcmp(pnam,prm) == 0))
    {
      j = strlen(pstr);
      if(j>10) 
      {
        j=j-2;
        while(j>2)
        {
          if((pstr[j] == '6') && (pstr[j+1] == '4'))
          {
            pstr[j-2]='0';
            j=0;
          }
          j--;
        }
        ns = 0;
        ok = 1;
      } 
    }
    fputs(pstr, fnm);
  }   
  fclose(fnm);
  fclose(ftmp);

  strcat(strcpy(pstr, dir), "/tmp");
  remove(pstr);
  
  return ok;
}


int setprm(dir, prm, pval)   /* returns: 0 - not set, 1 set OK */
char   *dir, *prm, *pval;
{
  FILE	  *fnm, *ftmp;
  int     i=-1, ptyp=0, k=0, ns=1, ok=1;
  char    pstr[MAXSTR], pnam[MAXSTR], s[MAXSTR];

  strcat(strcpy(pstr, dir), "/procpar");
  strcat(strcpy(pnam, dir), "/tmp");

  rename(pstr, pnam);

  if((fnm=fopen(pstr,"w"))==NULL) 
  { 
    printf("fopen %s failed\n", pstr); 
    exit(1); 
  }
  if((ftmp=fopen(pnam,"r"))==NULL) 
  { 
    printf("fopen %s failed\n", pnam); 
    exit(1); 
  }

  ns=1; ok=1;
  while (fgets(pstr, 512, ftmp))		/* fix parameters */
  {
    fputs(pstr, fnm);
    if((ns) && (pstr[0] == prm[0]) && (pstr[1] == prm[1]) && 
       (sscanf(pstr, "%s %d %s %s %s %s %s %s %s %d", 
        pnam, &ptyp, s, s, s, s, s, s, s, &k)==10) && 
       (strcmp(pnam,prm) == 0))
    {
      fgets(pstr, MAXSTR, ftmp); 
      if(sscanf(pstr, "%d %s\n", &i, s) < 2)
      {
        printf("setpar: setpar for %s failed\n", prm);
        fputs(pstr, fnm);
        ok = 0; 
      }
      else if(i>1)
      {
        printf("Warning: parameter %s is arrayed - unable to change \n", prm);
        fputs(pstr, fnm);
        ok = 0; 
      }
      else
        fprintf(fnm, "%d %s\n", i, pval);      
      ns = 0; 
    }
  }   
  fclose(fnm);
  fclose(ftmp);

  strcat(strcpy(pstr, dir), "/tmp");
  remove(pstr);
  
  return ok;
}

int setprm_d(dir, prm, dval)   /* returns: 0 - not set, 1 set OK */
char    *dir, *prm;
double   dval;
{
  char pval[128];
  
  sprintf(pval, "%.7f", dval);
  return setprm(dir, prm, pval);   
}

int setprm_i(dir, prm, ival)   /* returns: 0 - not set, 1 set OK */
char  *dir, *prm;
int    ival;
{
  char pval[128];
  
  sprintf(pval, "%d", ival);
  return setprm(dir, prm, pval);   
}

int unarray_procpar(dir, prm)   /* returns: 0 - not set, 1 set OK */
char   *dir, *prm;
{
  FILE	  *fnm, *ftmp;
  int     ax, i=-1, j=0;
  char    str[MAXSTR], str2[MAXSTR], *c1, *c2;

  strcat(strcpy(str, dir), "/procpar");
  strcat(strcpy(str2, dir), "/tmp");

  rename(str, str2); 

  if((fnm=fopen(str,"w"))==NULL) 
  { 
    printf("fopen %s failed\n", str); 
    exit(1); 
  }
  if((ftmp=fopen(str2,"r"))==NULL) 
  { 
    printf("fopen %s failed\n", str2); 
    exit(1); 
  }

  if((ax = getprm(ftmp, prm, str)) == 0) { fclose(fnm); fclose(ftmp); return 0; }

  fseek(ftmp, 0, 0); 
  while (fgets(str, 512, ftmp))		/* fix parameters */
  {
    fputs(str, fnm);
    if((str[0] == prm[0]) && sscanf(str, "%s\n", str2) && (strcmp(str2, prm) == 0)) /* unarray prm */
    {
      fgets(str, 512, ftmp);
      sscanf(str, "%d %s\n", &i, str2);           
      fprintf(fnm, "1 %s\n", str2);   
    }
    else switch (str[0])
    {   
      case 'a' :
        sscanf(str, "%s\n", str2);
        if ((str2[1] == 'r') && (str2[2] == 'r'))
        {
          if (strcmp(str2, "arraydim") == 0)		          /* arraydim */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %d\n", &i, &j);           
            fprintf(fnm, "%d %d\n", i, j/ax);   
          }
          else if (strcmp(str2, "arrayelemts") == 0)           /* arrayelemts */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %d\n", &i, &j);
            fprintf(fnm, "%d %d\n", i, j-1);    
          }
          else if (strcmp(str2, "array") == 0)		        /* array */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %s\n", &i, str2); 
            if((c1 = strstr(str2, prm)) != NULL)
            {
              c2 = c1 + strlen(prm);
              if (*c2 == ',') c2++; else c1--;
              *c1 = '\0'; strcat(str2, c2);   
            }
            fprintf(fnm, "1 %s \n", str2);           
          }
        }
        else if((str2[1] == 'c') && (str2[2] == 'q'))
        {
          if (strcmp(str2, "acqcycles") == 0)        /* acqcycles */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %d\n", &i, &j);
            fprintf(fnm, "%d %d\n", i, j/ax);
          }
        }
      break; 
      case 'c' :
        sscanf(str, "%s\n", str2);
        if (strcmp(str2, "celem") == 0)                        /* celem - completed increments */
        {
          fgets(str, 512, ftmp);
          sscanf(str, "%d %d\n", &i, &j);             
          fprintf(fnm, "%d %d\n", i, j/ax);
        }
      break;      
    }
  }
  fclose(fnm);
  fclose(ftmp);

  strcat(strcpy(str, dir), "/tmp"); 
  remove(str);
  
  return ax;
}


int setpar_d(dir, prm, dval)
char   *dir, *prm;
double dval;
{
  int   i=0;
  char  pval[MAXSTR];
  
  sprintf(pval, "%.9f", dval);  
  i = setprm(dir, prm, pval);
  
  return i;
}

int setpar_i(dir, prm, ival)
char   *dir, *prm;
int    ival;
{
  int   i=0;
  char  pval[MAXSTR];
  
  sprintf(pval, "%d", ival);  
  i = setprm(dir, prm, pval);
  
  return i;
}


int setpar_s(dir, prm, sval)
char   *dir, *prm, *sval;
{
  int   i=0;
  char  pval[MAXSTR];
  
  sprintf(pval, "\"%s\" ", sval);  
  i = setprm(dir, prm, pval);
  
  return i;
}


int num_active_rcvrs(dir)
char  *dir;
{
  FILE  *sfile;
  int    i, j, nrec=0;
  char   rcvrs[MAXSTR];
     

  if ((sfile = open_file(strcat(strcpy(rcvrs, dir), "/procpar"), "r")) == NULL)     
    exit(1);
    
  j = getval_s(sfile, "rcvrs", rcvrs);  
    
  for(i=0, nrec=0; i<j; i++)    /* # of active receivers */
  {
    if(rcvrs[i] == 'y') nrec++;
  }
  fclose(sfile);

  return nrec;
}      


void fix_rcvrs(dir, rcvx, rep)  /* np, ni and ni2 as in vnmr */
char  *dir;
int   rcvx;
short  rep;
{
  FILE	  *fnm, *ftmp;
  int     i, j, k;
  double  dm, tof, dofx, sfrq, dfrqx;
  char    str[512], str1[512], str2[512], rcvrs[32], 
          tn[8], dn[8], sdn[8], sdof[8], sdfrq[8], rcvn[32];

  if(rcvx < 2) return;
  
  strcat(strcpy(str, dir), "/procpar");
  strcat(strcpy(str2, dir), "/tmp");

  rename(str, str2); 

  if((fnm=fopen(str,"w"))==NULL) 
  { 
    printf("fopen %s failed\n", str); 
    exit(1); 
  }  
  if((ftmp=fopen(str2,"r"))==NULL) 
  { 
    printf("fopen %s failed\n", str2); 
    exit(1); 
  }  
  if(rep) printf("fixing rcvrs...\n");  

  j = getval_s(ftmp, "rcvrs", rcvrs);  
  if((j >= rcvx) && (rcvrs[rcvx-1] == 'y'))
  {
    strcpy(rcvn, rcvrs);
    for(i=0; i<j; i++) { if(rcvn[i] == 'y') rcvn[i] = 'n'; }
               
    k = rcvx-2;
    if(k) sprintf(sdn, "dn%d", k);
    else  strcpy(sdn, "dn");
    if(k) sprintf(sdof, "dof%d", k);
    else  strcpy(sdof, "dof");
    if(k) sprintf(sdfrq, "dfrq%d", k);
    else  strcpy(sdfrq, "dfrq");
    
    if(!getval_s(ftmp, "tn", tn)  || !getval_s(ftmp, sdn, dn) || 
       !getval(ftmp, "tof", &tof) || !getval(ftmp, sdof, &dofx) || 
       !getval(ftmp, "sfrq", &sfrq) || !getval(ftmp, sdfrq, &dfrqx)) 
      printf("\nfix_rcvrs: failed to retrieve parameters\n\n");
    else
    {
      fseek(ftmp, 0, 0); 
      while (fgets(str, 512, ftmp))		/* fix parameters */
      {
        fputs(str, fnm);
        switch (str[0])
        {           
          case 'd' :
            sscanf(str, "%s\n", str2);
            if (strcmp(str2, sdfrq) == 0)		/* dfrq */
            {
              fgets(str, 512, ftmp);  
              sscanf(str, "%d %lf\n", &i, &dm);
              fprintf(fnm, "%d %.7f\n", i, sfrq); 
            }
            else if (strcmp(str2, sdn) == 0)             /* dn */
            {
              fgets(str, 512, ftmp);  
              sscanf(str, "%d %s\n", &i, str1);
              fprintf(fnm, "%d \"%s\" \n", i, tn);
            }
            else if (strcmp(str2, sdof) == 0)            /* dof */
            {
              fgets(str, 512, ftmp);
              sscanf(str, "%d %lf\n", &i, &dm);
              fprintf(fnm, "%d %.2f\n", i, tof);
            }
          break;    
          case 'r' :
            sscanf(str, "%s\n", str2);
            if (strcmp(str2, "reffrq") == 0) 	      /* reffrq */
            {
              fgets(str, 512, ftmp);  
              sscanf(str, "%d %lf\n", &i, &dm);
              fprintf(fnm, "%d %.7f\n", i, dfrqx); 
            }
            else if (strcmp(str2, "rcvrs") == 0)       /* rcvrs */
            {
              fgets(str, 512, ftmp);  
              sscanf(str, "%d %s\n", &i, str1);
              fprintf(fnm, "%d \"%s\" \n", i, rcvn); 
            }
          break;       
          case 's' :
            sscanf(str, "%s\n", str2);
            if (strcmp(str2, "sfrq") == 0)		/* sfrq */
            {
              fgets(str, 512, ftmp);  
              sscanf(str, "%d %lf\n", &i, &dm);
              fprintf(fnm, "%d %.7f\n", i, dfrqx); 
            }
          break;
          case 't' :
            sscanf(str, "%s\n", str2);
            if (strcmp(str2, "tn") == 0)             /* tn */
            {
              fgets(str, 512, ftmp);  
              sscanf(str, "%d %s\n", &i, str1);
              fprintf(fnm, "%d \"%s\" \n", i, dn);  
            }
            else if (strcmp(str2, "tof") == 0)            /* tof */
            {
              fgets(str, 512, ftmp);
              sscanf(str, "%d %lf\n", &i, &dm);
              fprintf(fnm, "%d %.2f\n", i, dofx);   
            }
          break;    
        }
      }
    }
  }    
  fclose(fnm);
  fclose(ftmp);

  strcat(strcpy(str2, dir), "/tmp");
  remove(str2);

}


void fix_np(dir, npx, nix, ni2x, rep)  /* np, ni and ni2 as in vnmr */
char *dir;
int npx, nix, ni2x;
short  rep;
{
  FILE	  *fnm, *ftmp;
  int     i, j, k, fni, rto, np_orig, ni_orig, ni2_orig, ph, ph2;
  double  dm;
  char    str[512], str2[512], rcvrs[32];

  fni=0; rto=0; ph = 1; ph2 = 1;
  
  strcat(strcpy(str, dir), "/procpar");
  strcat(strcpy(str2, dir), "/tmp");

  rename(str, str2); 

  if((fnm=fopen(str,"w"))==NULL) 
  { 
    printf("fopen %s failed\n", str); 
    exit(1); 
  }  
  if((ftmp=fopen(str2,"r"))==NULL) 
  { 
    printf("fopen %s failed\n", str2); 
    exit(1); 
  }  
  if (rep) printf("fixing procpar: %d, %d, %d \n", npx, nix, ni2x);  

  np_orig = 0;
  if((npx) && (getval_i(ftmp, "np", &np_orig) == 0))
  {
    printf("fix_np: failed to get the original np. Aborting.\n");
    fclose(fnm); exit(0);
  }
  if((npx > -1) && (getval_i(ftmp, "fn", &j)))
    fni = j;
  if((nix > -1) && (getval_i(ftmp, "ni", &ni_orig) == 0))
    ni_orig = 0;
  if((ni2x > -1) && (getval_i(ftmp, "ni2", &ni2_orig) == 0))
    ni2_orig = 0;  
    
  j = getval_s(ftmp, "rcvrs", rcvrs);    
  for(i=0, rto=0; i<j; i++)
  {
    if(rcvrs[i] == 'y') rto++;
  }
  if(rto < 1) rto = 1;
      
  j = getval_s(ftmp, "array", str);
  if((strcmp(str, "phase,phase2") == 0) || (strcmp(str, "phase2,phase") == 0)) 
  {
    if((ni_orig > 1) && ((nix == 0) || (nix == 1))) ph = 2;  
    if((ni2_orig > 1) && ((ni2x == 0) || (ni2x == 1))) ph2 = 2;  
  }

  fseek(ftmp, 0, 0); 
  while (fgets(str, 512, ftmp))		/* fix parameters */
  {
    fputs(str, fnm);
    switch (str[0])
    {   
      case 'n' :
        sscanf(str, "%s\n", str2);
        if ((npx > -1) && (str2[1] == 'p') && (str2[2] == '\0'))	/* np */
        {
          fgets(str, 512, ftmp);  
          sscanf(str, "%d %d\n", &i, &j);
          fprintf(fnm, "%d %d\n", i, npx); 
        }
        else if ((nix > -1) && (str2[1] == 'i') && (str2[2] == '\0'))   /* ni */
        {
          fgets(str, 512, ftmp);  
          sscanf(str, "%d %d\n", &i, &j);
          fprintf(fnm, "%d %d\n", i, nix);
        }
        else if ((ni2x > -1) && (str2[1] == 'i') && (str2[2] == '2') && (str2[3] == '\0'))   /* ni2 */
        {
          fgets(str, 512, ftmp);
          sscanf(str, "%d %d\n", &i, &j);
          fprintf(fnm, "%d %d\n", i, ni2x);  
        }
      break;    
      case 'c' :
        sscanf(str, "%s\n", str2);
        if (strcmp(str2, "celem") == 0)                        /* celem - completed increments */
        {
          fgets(str, 512, ftmp);
          sscanf(str, "%d %d\n", &i, &j);             
          j /= rto;  
          dm = (double) j; 
          if(nix > -1)
          {
            if(nix > 1) dm *= (double) nix;
            if(ni_orig > 1) dm /= (double) ni_orig;
            if(ph>1) dm /= ph; 
          }
          if(ni2x > -1)
          {         
            if(ni2x > 1) dm *= (double) ni2x;
            if(ni2_orig > 0) dm /= ni2_orig;
            if(ph2>1) dm /= ph2; 
          }         
          j = (int) (0.5 + dm);                 
          fprintf(fnm, "%d %d\n", i, j);
        }
      break;
      case 'a' :
        sscanf(str, "%s\n", str2);
        if ((str2[1] == 'r') && (str2[2] == 'r'))
        {
          if (strcmp(str2, "array") == 0)		        /* array */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %s\n", &i, str2); 
            if (ph == 2) fprintf(fnm, "%d \"phase2\" \n", i);  
            else if (ph2 == 2) fprintf(fnm, "%d \"phase\" \n", i);  
            else fprintf(fnm, "%d %s \n", i, str2);           
          }
          else if (strcmp(str2, "arraydim") == 0)		/* arraydim */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %d\n", &i, &j);
            j /= rto;   
            dm = (double) j; 
            if(nix > -1)
            {
              if(nix > 1) dm *= (double) nix;     
              if(ni_orig > 1) dm /= (double) ni_orig;    
              if(ph>1) dm /= ph;                        
            }     
            if(ni2x > -1)
            {
              if(ni2x > 1) dm *= (double) ni2x;    
              if(ni2_orig > 0) dm /= ni2_orig;    
              if(ph2>1) dm /= ph2;             
            }
            j = (int) (0.5 + dm);       
            fprintf(fnm, "%d %d\n", i, j);
          }
          else if (strcmp(str2, "arrayelemts") == 0)     /* arrayelemts */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %d\n", &i, &j);
            if(((nix == 0) || (nix == 1)) && (ni_orig > 1)) j--;
            if(((ni2x == 0) || (ni2x == 1)) && (ni2_orig > 1)) j--;
            if((nix > -1) && (ph > 1)) j--;
            if((ni2x > -1) && (ph2 > 1)) j--;
            fprintf(fnm, "%d %d\n", i, j);
          }
        }
        else if((str2[1] == 'c') && (str2[2] == 'q'))
        {
          if (strcmp(str2, "acqcycles") == 0)        /* acqcycles */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %d\n", &i, &j);
            dm = (double) j; 
            if(nix > -1)
            {
              if(nix > 1) dm *= (double) nix;
              if(ni_orig > 1) dm /= (double) ni_orig;
              if(ph>1) dm /= ph;                    
            }
            if(ni2x > -1)
            {
              if(ni2x > 1) dm *= (double) ni2x;
              if(ni2_orig > 0) dm /= ni2_orig;
              if(ph2>1) dm /= ph2; 
            }
            j = (int) (0.5 + dm);        
            fprintf(fnm, "%d %d\n", i, j);
          }
          else if (strcmp(str2, "acqdim") == 0)           /* acqdim */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %d\n", &i, &j);
            if(((nix == 0) || (nix == 1)) && (ni_orig > 1)) j--;
            if(((ni2x == 0) || (ni2x == 1)) && (ni2_orig > 1)) j--;
            fprintf(fnm, "%d %d\n", i, j);
          }          
        }
        else if ((str2[1] == 't') && (str2[2] == '\0'))		/* at */
        {
          fgets(str, 512, ftmp);
          sscanf(str, "%d %lf\n", &i, &dm);
          if((npx > 1) && (np_orig > 0))
            dm *= (double) npx/np_orig;
          fprintf(fnm, "%d %.6f\n", i, dm);
        }
      break;
      case 'p' :
        sscanf(str, "%s\n", str2);
        if (strcmp(str2, "phase") == 0)		/* phase */
        {
          fgets(str, 512, ftmp);  
          sscanf(str, "%d %d %d", &i, &j, &k); 
          if ((nix > -1) && (ph > 1)) fprintf(fnm, "1 1\n"); 
          else fprintf(fnm, "%s", str); 
        }
        else if (strcmp(str2, "phase2") == 0)		/* phase2 */
        {
          fgets(str, 512, ftmp);  
          sscanf(str, "%d %d %d", &i, &j, &k); 
          if ((ni2x > -1) && (ph2 > 1)) fprintf(fnm, "1 1\n"); 
          else fprintf(fnm, "%s", str); 
        }
      break;          
      case 'f' :
        sscanf(str, "%s\n", str2);
        if ((fni) && (npx > -1) && (str2[1] == 'n') && (str2[2] == '\0'))	/* fn */
        {
          fgets(str, 512, ftmp);  
          sscanf(str, "%d %d", &i, &j);
          fprintf(fnm, "%d %d\n", i, fni); 
        }
      break;          
    }
  }
  fclose(fnm);
  fclose(ftmp);

  strcat(strcpy(str2, dir), "/tmp");
  remove(str2);
}


int pow2(jnp)
int jnp;
{
  int i;

  i=1;  
  while(i<jnp) i=i<<1; 
  
  return i;
}

void reset_procpar(pp)
PROCPAR  *pp;
{
  pp->lb=0.0;  pp->sb=0.0;  pp->sbs=0.0;  pp->gf=0.0;  pp->gfs=0.0;  pp->fpmult=1.0; 
  pp->lb1=0.0; pp->sb1=0.0; pp->sbs1=0.0; pp->gf1=0.0; pp->gfs1=0.0; pp->fpmult1=1.0;
  pp->lb2=0.0; pp->sb2=0.0; pp->sbs2=0.0; pp->gf2=0.0; pp->gfs2=0.0; pp->fpmult2=1.0;
  pp->lb3=0.0; pp->sb3=0.0; pp->sbs3=0.0; pp->gf3=0.0; pp->gfs3=0.0; pp->fpmult3=1.0;
  pp->at=0.0;  pp->at1=0.0; pp->at2=0.0;  pp->at3=0.0; 
  pp->sw=0.0;  pp->sw1=0.0; pp->sw2=0.0;  pp->sw3=0.0;
  pp->np=0; pp->ni=0;  pp->ni2=0; pp->ni3=0; pp->nimax=0; pp->ni2max=0; pp->ni3max=0;
  pp->fn=0; pp->fn1=0; pp->fn2=0; pp->fn3=0; pp->ct=0;
}


void reset_sfrq(xsfrq)
SFRQ  *xsfrq;
{
  xsfrq->tof=0.0;    xsfrq->dof=0.0;     xsfrq->dof2=0.0;    xsfrq->dof3=0.0;
  xsfrq->sfrq=0.0;   xsfrq->dfrq=0.0;    xsfrq->dfrq2=0.0;   xsfrq->dfrq3=0.0; 
  xsfrq->reffrq=0.0; xsfrq->reffrq1=0.0; xsfrq->reffrq2=0.0; xsfrq->reffrq3=0.0;
  xsfrq->refpos=0.0; xsfrq->refpos1=0.0; xsfrq->refpos2=0.0; xsfrq->refpos3=0.0;
  xsfrq->lockfreq=0.0; xsfrq->lockfreq_=0.0;
  xsfrq->rfl=0.0;    xsfrq->rfl1=0.0;     xsfrq->rfl2=0.0;    xsfrq->rfl3=0.0;
  xsfrq->rfp=0.0;    xsfrq->rfp1=0.0;     xsfrq->rfp2=0.0;    xsfrq->rfp3=0.0;
}


int is_par(parstr, pname, tp)
char  *parstr, *pname;
int   *tp;
{
  char  prm[512], s[128];
  int   i=0, j=0, k=0;
  
  if(sscanf(parstr, "%s %d %s %s %s %s %s %s %s %d", 
            prm, &j, s, s, s, s, s, s, s, &k)==10) 
  {
    if(strcmp(prm, pname) == 0) { *tp=j; i=1; }
    if(k<1) *tp=0; 
  }
  else
    i=0;
    
  return i;
}


SFRQ get_sfrq(xfnm, rep)
char *xfnm;
short  rep;
{
  FILE    *sfile;
  char     fnm[MAXSTR], str[MAXSTR], pstr[MAXSTR];
  SFRQ     xsfrq;
  int      i, ptyp;

  sprintf(fnm, "%s", xfnm);
  (int) check_expdir_fname(fnm);
  strcat(fnm, "/procpar");
  
  if ((sfile = open_file(fnm, "r")) == NULL)     /* open 2D source file */
    exit(1);

  (void) reset_sfrq(&xsfrq);

  while (fgets(str, 512, sfile))		/* get frequencies */
  {
    switch (str[0])
    {   
      case 'd' :
        if((str[1] == 'o') && (str[2] == 'f'))
        {
          if(is_par(str, "dof", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
          {
            fgets(pstr, MAXSTR, sfile);   
            if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.dof) < 2)
              printf("getprm: getval for dof failed\n");
            else if(DBUG==1) 
              printf("get_sfrq: dof = %.2f\n", xsfrq.dof);        
          }
          else if(is_par(str, "dof2", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
          {
            fgets(pstr, MAXSTR, sfile);   
            if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.dof2) < 2)
              printf("getprm: getval for dof2 failed\n");
            else if(DBUG==1) 
              printf("get_sfrq: dof2 = %.2f\n", xsfrq.dof2);        
          }
          else if(is_par(str, "dof3", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
          {
            fgets(pstr, MAXSTR, sfile);   
            if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.dof3) < 2)
              printf("getprm: getval for dof3 failed\n");
            else if(DBUG==1) 
              printf("get_sfrq: dof3 = %.2f\n", xsfrq.dof3);        
          }
        }
        else if((str[1] == 'f') && (str[2] == 'r'))
        {
          if(is_par(str, "dfrq", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
          {
            fgets(pstr, MAXSTR, sfile);   
            if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.dfrq) < 2)
              printf("getprm: getval for dfrq failed\n");
            else if(DBUG==1) 
              printf("get_sfrq: dfrq = %.7f\n", xsfrq.dfrq);        
          }
          else if(is_par(str, "dfrq2", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
          {
            fgets(pstr, MAXSTR, sfile);   
            if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.dfrq2) < 2)
              printf("getprm: getval for dfrq2 failed\n");
            else if(DBUG==1) 
              printf("get_sfrq: dfrq2 = %.7f\n", xsfrq.dfrq2);        
          }
          else if(is_par(str, "dfrq3", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
          {
            fgets(pstr, MAXSTR, sfile);   
            if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.dfrq3) < 2)
              printf("getprm: getval for dfrq3 failed\n");
            else if(DBUG==1) 
              printf("get_sfrq: dfrq3 = %.7f\n", xsfrq.dfrq3);        
          }
        }
      break;
      
      case 'l' :
        if((str[1] == 'o') && (str[2] == 'c')) 
        {
          if(is_par(str, "lockfreq", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
          {
            fgets(pstr, MAXSTR, sfile);   
            if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.lockfreq) < 2)
              printf("getprm: getval for lockfreq failed\n");
            else if(DBUG==1) 
              printf("get_sfrq: lockfreq = %.7f\n", xsfrq.lockfreq);        
          }
          else if(is_par(str, "lockfreq_", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
          {
            fgets(pstr, MAXSTR, sfile);   
            if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.lockfreq_) < 2)
              printf("getprm: getval for lockfreq_ failed\n");
            else if(DBUG==1) 
              printf("get_sfrq: lockfreq_ = %.7f\n", xsfrq.lockfreq_);        
          }
       }
      break;

      case 'r' :
        if((str[1] == 'e') && (str[2] == 'f')) 
        {
          if(str[3] == 'f')
          {
            if(is_par(str, "reffrq", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
            {
              fgets(pstr, MAXSTR, sfile);   
              if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.reffrq) < 2)
                printf("getprm: getval for reffrq failed\n");
              else if(DBUG==1) 
                printf("get_sfrq: reffrq = %.7f\n", xsfrq.reffrq);        
            }
            else if(is_par(str, "reffrq1", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
            {
              fgets(pstr, MAXSTR, sfile);   
              if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.reffrq1) < 2)
                printf("getprm: getval for reffrq1 failed\n");
              else if(DBUG==1) 
                printf("get_sfrq: reffrq1 = %.7f\n", xsfrq.reffrq1);        
            }
            else if(is_par(str, "reffrq2", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
            {
              fgets(pstr, MAXSTR, sfile);   
              if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.reffrq2) < 2)
                printf("getprm: getval for reffrq2 failed\n");
              else if(DBUG==1) 
                printf("get_sfrq: reffrq2 = %.7f\n", xsfrq.reffrq2);        
            }
            else if(is_par(str, "reffrq3", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
            {
              fgets(pstr, MAXSTR, sfile);   
              if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.reffrq3) < 2)
                printf("getprm: getval for reffrq3 failed\n");
              else if(DBUG==1) 
                printf("get_sfrq: reffrq3 = %.7f\n", xsfrq.reffrq3);        
            }
          }
          else if(str[3] == 'p')
          {          
            if(is_par(str, "refpos", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
            {
              fgets(pstr, MAXSTR, sfile);   
              if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.refpos) < 2)
                printf("getprm: getval for refpos failed\n");
              else if(DBUG==1) 
                printf("get_sfrq: refpos = %.2f\n", xsfrq.refpos);        
            }
            else if(is_par(str, "refpos1", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
            {
              fgets(pstr, MAXSTR, sfile);   
              if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.refpos1) < 2)
                printf("getprm: getval for refpos1 failed\n");
              else if(DBUG==1) 
                printf("get_sfrq: refpos1 = %.2f\n", xsfrq.refpos1);        
            }
            else if(is_par(str, "refpos2", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
            {
              fgets(pstr, MAXSTR, sfile);   
              if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.refpos2) < 2)
                printf("getprm: getval for refpos2 failed\n");
              else if(DBUG==1) 
                printf("get_sfrq: refpos2 = %.2f\n", xsfrq.refpos2);        
            }
            else if(is_par(str, "refpos3", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
            {
              fgets(pstr, MAXSTR, sfile);   
              if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.refpos3) < 2)
                printf("getprm: getval for refpos3 failed\n");
              else if(DBUG==1) 
                printf("get_sfrq: refpos3 = %.2f\n", xsfrq.refpos3);        
            }
          }
        }
      break;
      
      case 's' :
        if((str[1] == 'f') && (str[2] == 'r')) 
        {
          if(is_par(str, "sfrq", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
          {
            fgets(pstr, MAXSTR, sfile);   
            if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.sfrq) < 2)
              printf("getprm: getval for sfrq failed\n");
            else if(DBUG==1) 
              printf("get_sfrq: sfrq = %.7f\n", xsfrq.sfrq);        
          }
        }
      break;
      
      case 't' :
        if((str[1] == 'o') && (str[2] == 'f')) 
        {
          if(is_par(str, "tof", &ptyp) && ((ptyp == 1) || (ptyp == 5)))
          {
            fgets(pstr, MAXSTR, sfile);   
            if(sscanf(pstr, "%d %lf\n", &i, &xsfrq.tof) < 2)
              printf("getprm: getval for tof failed\n");
            else if(DBUG==1) 
              printf("get_sfrq: tof = %.2f\n", xsfrq.tof);        
          }
        }
      break;
    }
  }
  fclose(sfile);
          
  return xsfrq;
}      
        

PROCPAR get_procpar(xfnm)
char *xfnm;
{
  FILE    *sfile;
  char     fnm[MAXSTR], str[MAXSTR], pstr[MAXSTR];
  PROCPAR  pp;
  int      i, ptyp;
  
  sprintf(fnm, "%s", xfnm);
  (int) check_expdir_fname(fnm);
  strcat(fnm, "/procpar");
  
  if ((sfile = open_file(fnm, "r")) == NULL)     /* open 2D source file */
    exit(1);

  (void) reset_procpar(&pp);
  
  while (fgets(str, 512, sfile))		/* get processing parameters */
  {
    switch (str[0])
    {   
      case 'a' :
        if(is_par(str, "at", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.at) < 2)
          {
            printf("getprm: getval for at failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: at = %.6f\n", pp.at);        
        }
      break;

      case 'c' :
        if(is_par(str, "ct", &ptyp) && (ptyp == 7))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %d\n", &i, &pp.ct) < 2)
          {
            printf("getprm: getval for ct failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: ct = %d\n", pp.ct);        
        }
      break;

      case 'l' :
        if(is_par(str, "lb", &ptyp) && ((ptyp == 1) ||   
          (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.lb) < 2)
          {
            printf("getprm: getval for lb failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: lb = %.6f\n", pp.lb);        
        }
        else if(is_par(str, "lb1", &ptyp) && ((ptyp == 1) ||   
          (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.lb1) < 2)
          {
            printf("getprm: getval for lb1 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: lb1 = %.6f\n", pp.lb1);        
        }
        else if(is_par(str, "lb2", &ptyp) && ((ptyp == 1) ||   
          (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.lb2) < 2)
          {
            printf("getprm: getval for lb2 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: lb2 = %.6f\n", pp.lb2);        
        }
      break;
      
      case 'n' :
        if(is_par(str, "np", &ptyp) && (ptyp == 7))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %d\n", &i, &pp.np) < 2)
          {
            printf("getprm: getval for np failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: np = %d\n", pp.np);        
        }
        else if(is_par(str, "ni", &ptyp) && (ptyp == 7))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %d\n", &i, &pp.ni) < 2)
          {
            printf("getprm: getval for ni failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: ni = %d\n", pp.ni);        
        }
        else if(is_par(str, "ni2", &ptyp) && (ptyp == 7))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %d\n", &i, &pp.ni2) < 2)
          {
            printf("getprm: getval for ni2 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: ni2 = %d\n", pp.ni2);        
        }
      break;    
      
      case 'f' :
        if(is_par(str, "fn", &ptyp) && (ptyp == 7))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %d\n", &i, &pp.fn) < 2)
          {
            printf("getprm: getval for fn failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: fn = %d\n", pp.fn);        
        }
        else if(is_par(str, "fn1", &ptyp) && (ptyp == 7))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %d\n", &i, &pp.fn1) < 2)
          {
            printf("getprm: getval for fn1 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: fn1 = %d\n", pp.fn1);        
        }
        else if(is_par(str, "fn2", &ptyp) && (ptyp == 7))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %d\n", &i, &pp.fn2) < 2)
          {
            printf("getprm: getval for fn2 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: fn2 = %d\n", pp.fn2);        
        }
        else if(is_par(str, "fpmult", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.fpmult) < 2)
          {
            printf("getprm: getval for fpmult failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: fpmult = %.4f\n", pp.fpmult);        
        }
        else if(is_par(str, "fpmult1", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.fpmult1) < 2)
          {
            printf("getprm: getval for fpmult1 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: fpmult1 = %.4f\n", pp.fpmult1);        
        }
        else if(is_par(str, "fpmult2", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.fpmult2) < 2)
          {
            printf("getprm: getval for fpmult2 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: fpmult2 = %.4f\n", pp.fpmult2);        
        }
      break;

      case 'g' :
        if(is_par(str, "gf", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.gf) < 2)
          {
            printf("getprm: getval for gf failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: gf = %.6f\n", pp.gf);        
        }
        else if(is_par(str, "gfs", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.gfs) < 2)
          {
            printf("getprm: getval for gfs failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: gfs = %.6f\n", pp.gfs);        
        }
        else if(is_par(str, "gf1", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.gf1) < 2)
          {
            printf("getprm: getval for gf1 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: gf1 = %.6f\n", pp.gf1);        
        }
        else if(is_par(str, "gfs1", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.gfs1) < 2)
          {
            printf("getprm: getval for gfs1 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: gfs1 = %.6f\n", pp.gfs1);        
        }
        else if(is_par(str, "gf2", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.gf2) < 2)
          {
            printf("getprm: getval for gf2 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: gf2 = %.6f\n", pp.gf2);        
        }
        else if(is_par(str, "gfs2", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.gfs2) < 2)
          {
            printf("getprm: getval for gfs2 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: gfs2 = %.6f\n", pp.gfs2);        
        }
      break;          
      
      case 's' :
        if(is_par(str, "sw", &ptyp) && ((ptyp == 1) ||   
          (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.sw) < 2)
          {
            printf("getprm: getval for sw failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: sw = %.2f\n", pp.sw);        
        }
        else if(is_par(str, "sw1", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.sw1) < 2)
          {
            printf("getprm: getval for sw1 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: sw1 = %.2f\n", pp.sw1);        
        }
        else if(is_par(str, "sw2", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.sw2) < 2)
          {
            printf("getprm: getval for sw2 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: sw2 = %.2f\n", pp.sw2);        
        }
        else if(is_par(str, "sb", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.sb) < 2)
          {
            printf("getprm: getval for sb failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: sb = %.6f\n", pp.sb);        
        }
        else if(is_par(str, "sbs", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.sbs) < 2)
          {
            printf("getprm: getval for sbs failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: sbs = %.6f\n", pp.sbs);        
        }
        else if(is_par(str, "sb1", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.sb1) < 2)
          {
            printf("getprm: getval for sb1 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: sb1 = %.6f\n", pp.sb1);        
        }
        else if(is_par(str, "sbs1", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.sbs1) < 2)
          {
            printf("getprm: getval for sbs1 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: sbs1 = %.6f\n", pp.sbs1);        
        }
        else if(is_par(str, "sb2", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.sb2) < 2)
          {
            printf("getprm: getval for sb2 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: sb2 = %.6f\n", pp.sb2);        
        }
        else if(is_par(str, "sbs2", &ptyp) && ((ptyp == 1) ||   
               (ptyp == 3) || (ptyp == 5) || (ptyp == 6) || (ptyp == 7)))
        {
          fgets(pstr, MAXSTR, sfile);   
          if(sscanf(pstr, "%d %lf\n", &i, &pp.sbs2) < 2)
          {
            printf("getprm: getval for sbs2 failed\n");
            return pp;
          }
          if(DBUG==1) printf("get_procpar: sbs2 = %.6f\n", pp.sbs2);        
        }
      break;          
    }
  }
  fclose(sfile);
          
  if(pp.sw1 > 0.01) 
    pp.at1 = pp.ni/pp.sw1;
  if(pp.sw2 > 0.01)
    pp.at2 = pp.ni2/pp.sw2;

  return pp;
}

 
COMPLX **SE_proc(dfid, np, jx, nx)   /* nx is block size, jx is SE size */
COMPLX  **dfid;
int       np, jx, nx;
{
  int        i, j, k, i0, i1, i2;
  COMPLX  **df;

  if(nx<jx) 
  {
    printf("se_proc ignored: block size < SE size !\n");
    return dfid;
  }

  df = calloc_C2d((int) nx, (int) np);   
  
  i0=jx/2;                     
  for(k=i0; k<nx; k+=jx)                                                   
  {   
    for(i=0; i<i0; i++)                                                    
    { 
      i1 = k+i; i2 = i1-i0;   
      for(j=0; j<np; j++)
      {
        df[i2][j].re = dfid[i2][j].re - dfid[i1][j].re;   /* Re = re1 - re2 */  
        df[i2][j].im = dfid[i2][j].im - dfid[i1][j].im;     
        
        df[i1][j].re = dfid[i2][j].im + dfid[i1][j].im;   /* Im = im1 + im2 */                              
        df[i1][j].im = -dfid[i2][j].re - dfid[i1][j].re;                          
      } 
    }
  }

  return df;
}


void set_np(dir, new_np, rep)  /* change np */
char  *dir;
int    new_np;
short  rep;
{
  FILE	  *fnm, *ftmp;
  int     i, j, fni, np_orig;
  double  dm;
  char    str[512], str2[512];

    
  if(rep) printf("changing np to: %d \n", new_np);  
  if(new_np < 2) vn_error("set_np: new_np < 2"); 
  
  strcat(strcpy(str, dir), "/procpar");
  strcat(strcpy(str2, dir), "/tmp");

  np_orig = 0; fni = pow2(new_np); 
  if((fnm=open_file(str,"r"))==NULL) exit(0);
  if((getval_i(fnm, "np", &np_orig) == 0) || (np_orig < 2))
    vn_abort(fnm, "fix_np: failed to get the original np. ");
  if((getval_i(fnm, "fn", &j)) && (j > fni)) fni = j;
  fclose(fnm);  
  if(np_orig == new_np) return;
  
  rename(str, str2); 

  if((fnm=open_file(str,"w"))==NULL) exit(0);
  if((ftmp=open_file(str2,"r"))==NULL) exit(0); 
    
  fseek(ftmp, 0, 0); 
  while (fgets(str, 512, ftmp))		/* fix parameters */
  {
    fputs(str, fnm);
    switch (str[0])
    {   
      case 'n' :
        sscanf(str, "%s\n", str2);
        if((str2[1] == 'p') && (str2[2] == '\0'))	       /* np */
        {
          fgets(str, 512, ftmp);  
          sscanf(str, "%d %d\n", &i, &j);
          fprintf(fnm, "%d %d\n", i, new_np); 
        }
      break;    
      case 'a' :
        sscanf(str, "%s\n", str2);
        if ((str2[1] == 't') && (str2[2] == '\0'))	      /* at */
        {
          fgets(str, 512, ftmp);
          sscanf(str, "%d %lf\n", &i, &dm);
          dm *= (double) new_np/np_orig;
          fprintf(fnm, "%d %.6f\n", i, dm);
        }
      break;
      case 'f' :
        sscanf(str, "%s\n", str2);
        if ((str2[1] == 'n') && (str2[2] == '\0'))	     /* fn */
        {
          fgets(str, 512, ftmp);  
          sscanf(str, "%d %d", &i, &j);
          fprintf(fnm, "%d %d\n", i, fni); 
        }
      break;          
    }
  }
  fclose(fnm);
  fclose(ftmp);

  strcat(strcpy(str2, dir), "/tmp");
  remove(str2);
}



void set_ni3(dir, xni, xni2, xni3, ncyc, rep)  /* set ni, ni2 or ni3 to new values */
char *dir;
int    xni, xni2, xni3;
int   ncyc;
short  rep;
{
  FILE	  *fnm, *ftmp;
  int     i, j, k, celem;
  char    str[512], str2[512];
  
  i = xni; j = xni2; k = xni3; celem = 1;
  if(i<2) i=1;
  if(j<2) j=1;
  if(k<2) k=1;
  celem *= (i*j*k)*ncyc;
  

  strcat(strcpy(str, dir), "/procpar");
  strcat(strcpy(str2, dir), "/tmp");

  rename(str, str2); 

  if((fnm=open_file(str,"w"))==NULL) exit(0);
  if((ftmp=open_file(str2,"r"))==NULL)  exit(0);
      
  fseek(ftmp, 0, 0); 
  while (fgets(str, 512, ftmp))		/* fix parameters */
  {
    fputs(str, fnm);
    switch (str[0])
    {   
      case 'n' :
        sscanf(str, "%s\n", str2);
        if((strcmp(str2, "ni") == 0) && (xni > -1))
        {
          fgets(str, 512, ftmp);  
          sscanf(str, "%d %d\n", &i, &j);
          fprintf(fnm, "%d %d\n", i, xni); 
        }
        else if((strcmp(str2, "ni2") == 0) && (xni2 > -1))
        {
          fgets(str, 512, ftmp);  
          sscanf(str, "%d %d\n", &i, &j);
          fprintf(fnm, "%d %d\n", i, xni2);  
        }
        else if((strcmp(str2, "ni3") == 0) && (xni3 > -1))
        {
          fgets(str, 512, ftmp);  
          sscanf(str, "%d %d\n", &i, &j);
          fprintf(fnm, "%d %d\n", i, xni3);
        }
      break;    
      case 'c' :
        sscanf(str, "%s\n", str2);
        if (strcmp(str2, "celem") == 0)              /* celem - completed increments */
        {
          fgets(str, 512, ftmp);
          sscanf(str, "%d %d\n", &i, &j);  
          fprintf(fnm, "%d %d\n", i, celem);
        }
      break;
      case 'a' :
        sscanf(str, "%s\n", str2);
        if((str2[1] == 'r') && (str2[2] == 'r') && (strcmp(str2, "arraydim") == 0))       /* arraydim */
        {
          fgets(str, 512, ftmp);
          sscanf(str, "%d %d\n", &i, &j);           
          fprintf(fnm, "%d %d\n", i, celem);
        }
        else if((str2[1] == 'c') && (str2[2] == 'q') && (strcmp(str2, "acqcycles") == 0)) /* acqcycles */
        {
          fgets(str, 512, ftmp);
          sscanf(str, "%d %d\n", &i, &j);
          fprintf(fnm, "%d %d\n", i, celem);
        }
      break;
    }
  }
  fclose(fnm);
  fclose(ftmp);

  strcat(strcpy(str2, dir), "/tmp");
  remove(str2);
}


void  deactivate_par(xstr)
char  *xstr;
{
  int   i = strlen(xstr);
  
  if((xstr[i-5] == '1') && (xstr[i-3] == '6') && (xstr[i-2] == '4')) xstr[i-5] = '0';
   
  return;  
}  

void reset_pars(dir, sw, sw1, sw2)  /* reset 4D parameters */
char *dir;
double sw, sw1, sw2;
{
  FILE	  *fnm, *ftmp;
  int     i;
  double  tm;
  char    str[512], str2[512];
  
  strcat(strcpy(str, dir), "/procpar");
  strcat(strcpy(str2, dir), "/tmp");

  rename(str, str2); 

  if((fnm=fopen(str,"w"))==NULL) 
    vn_error("failed to open procpar. "); 
  if((ftmp=fopen(str2,"r"))==NULL) 
    vn_error("failed to open tmp file. "); 

  fseek(ftmp, 0, 0); 
  while (fgets(str, 512, ftmp))		/* fix parameters */
  {
    switch(str[0])
    {   
      case 'f' :
        if(str[1] == 'n') 	     /* fn, fn1, fn2, fn3 */
        {
          sscanf(str, "%s", str2);
          if((str2[2] == '\0') || ((str2[3] == '\0') && ((str2[2] == '1') || (str2[2] == '2') || (str2[2] == '3'))))
            deactivate_par(str);
        }
        else if(str[2] == 'c') 	     /* f1coef, f2coef, f3coef */
        {
          sscanf(str, "%s", str2);
          if((strcmp(str2, "f1coef") == 0) || (strcmp(str2, "f2coef") == 0) || (strcmp(str2, "f3coef") == 0))
          {
            fputs(str, fnm); fgets(str, 512, ftmp);            
            if(sscanf(str, "%d %s", &i, str2) == 2)
              sprintf(str, "%d \"\" \n", i); 
          }
        }
      break;
                
      case 'l' :   	        /* lp, lp1, lp2, lp3 */      
        if(str[1] == 'p')
        {
          sscanf(str, "%s", str2);
          if((str2[2] == '\0') || ((str2[3] == '\0') && ((str2[2] == '1') || (str2[2] == '2') || (str2[2] == '3')))) 
          {
            fputs(str, fnm); fgets(str, 512, ftmp);             
            if(sscanf(str, "%d %lf", &i, &tm) == 2)
              sprintf(str, "%d 0.0\n", i); 
          }
        }  
      break;          
                
      case 'p' :   	        /* proc, proc1, proc2, proc3 */      
        if(str[1] == 'r') 
        {
          sscanf(str, "%s", str2);
          if((strcmp(str2, "proc1") == 0) || (strcmp(str2, "proc2") == 0) || (strcmp(str2, "proc3") == 0))
          {
            fputs(str, fnm); fgets(str, 512, ftmp);            
            if(sscanf(str, "%d %s", &i, str2) == 2)
              sprintf(str, "%d \"\" \n", i); 
          }          
        }  
      break;          
                
      case 'r' :   	        /* rp, rp1, rp2, rp3 */      
        if(str[1] == 'p')
        {
          sscanf(str, "%s", str2);
          if((str2[2] == '\0') || ((str2[3] == '\0') && ((str2[2] == '1') || (str2[2] == '2') || (str2[2] == '3')))) 
          {
            fputs(str, fnm); fgets(str, 512, ftmp);             
            if(sscanf(str, "%d %lf", &i, &tm) == 2)
              sprintf(str, "%d 0.0\n", i); 
          }
        }  
      break;          
                
      case 's' :        
        if(str[1] == 'w') 	        /* sw, sw1, sw2 */
        {
          sscanf(str, "%s", str2);
          if((str2[2] == '\0') && (sw > 0.1))
          {
            fputs(str, fnm); fgets(str, 512, ftmp);             
            if(sscanf(str, "%d %lf", &i, &tm) == 2)
              sprintf(str, "%d %.2f\n", i, sw); 
          }
          else if(str2[3] == '\0') 
          {
            if((str2[2] == '1') && (sw1 > 0.1))
            {
              fputs(str, fnm); fgets(str, 512, ftmp);             
              if(sscanf(str, "%d %lf", &i, &tm) == 2)
                sprintf(str, "%d %.2f\n", i, sw1); 
            }
            else if((str2[2] == '2') && (sw2 > 0.1))
            {
              fputs(str, fnm); fgets(str, 512, ftmp);             
              if(sscanf(str, "%d %lf", &i, &tm) == 2)
                sprintf(str, "%d %.2f\n", i, sw2); 
            }
          }
        }  
        else                           /* ssfilter */ 
        {
          sscanf(str, "%s\n", str2);
          if(strcmp(str2, "ssfilter") == 0)
            deactivate_par(str);
        }
      break;          
    }
    fputs(str, fnm);
  }
  fclose(fnm);
  fclose(ftmp);

  strcat(strcpy(str2, dir), "/tmp");
  remove(str2);
}


int  add_phase3d(dir)
char *dir;
{  
  FILE	  *fnm, *ftmp;
  int     i, j, ph;
  char    str[512], str2[512];
  
  strcat(strcpy(str, dir), "/procpar");
  strcat(strcpy(str2, dir), "/tmp");

  rename(str, str2); 

  if((fnm=fopen(str,"w"))==NULL) 
  { 
    printf("fopen %s failed\n", str); 
    exit(1); 
  }  
  if((ftmp=fopen(str2,"r"))==NULL) 
  { 
    printf("fopen %s failed\n", str2); 
    exit(1); 
  }  
       
  j = getval_s(ftmp, "array", str);
  if((strcmp(str, "phase,phase2") == 0) || (strcmp(str, "phase2,phase") == 0)) return 1;
  else if (strcmp(str, "phase") == 0) ph=2;
  else if (strcmp(str, "phase2") == 0) ph=1;
  else 
  {
    printf("unexpected array = %s\n", str);
    return 0;
  }
  
  fseek(ftmp, 0, 0); 
  while (fgets(str, 512, ftmp))		 /* add phase related parameters */
  {
    fputs(str, fnm);
    switch (str[0])
    {   
      case 'c' :
        sscanf(str, "%s\n", str2);
        if (strcmp(str2, "celem") == 0)  /* celem - completed increments */
        {
          fgets(str, 512, ftmp);
          sscanf(str, "%d %d\n", &i, &j);             
          fprintf(fnm, "%d %d\n", i, 2*j);
        }
      break;
      case 'a' :
        sscanf(str, "%s\n", str2);
        if ((str2[1] == 'r') && (str2[2] == 'r'))
        {
          if (strcmp(str2, "array") == 0)		       /* array */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %s\n", &i, str2); 
            fprintf(fnm, "%d \"phase,phase2\" \n", i);  
          }
          else if (strcmp(str2, "arraydim") == 0)           /* arraydim */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %d\n", &i, &j);           
            fprintf(fnm, "%d %d\n", i, 2*j);
          }
          else if (strcmp(str2, "arrayelemts") == 0)     /* arrayelemts */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %d\n", &i, &j);
            fprintf(fnm, "%d %d\n", i, 2*j);
          }
        }
        else if((str2[1] == 'c') && (str2[2] == 'q'))
        {
          if (strcmp(str2, "acqcycles") == 0)              /* acqcycles */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %d\n", &i, &j);
            fprintf(fnm, "%d %d\n", i, 2*j);
          }
          else if (strcmp(str2, "acqdim") == 0)               /* acqdim */
          {
            fgets(str, 512, ftmp);
            sscanf(str, "%d %d\n", &i, &j);
            fprintf(fnm, "%d %d\n", i, j+1);
          }          
        }
      break;
      case 'p' :
        sscanf(str, "%s\n", str2);
        if (strcmp(str2, "phase") == 0)		              /* phase */
        {
          fgets(str, 512, ftmp);  
          fprintf(fnm, "2 1 2\n"); 
        }
        else if (strcmp(str2, "phase2") == 0)		     /* phase2 */
        {
          fgets(str, 512, ftmp);  
          fprintf(fnm, "2 1 2\n"); 
        }
      break;          
    }
  }
  fclose(fnm);
  fclose(ftmp);

  strcat(strcpy(str2, dir), "/tmp");
  remove(str2);

  return 1;
}

#endif



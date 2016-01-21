/* Xrecon.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Xrecon.c: External reconstruction                                         */
/*                                                                           */
/* Copyright (C) 2012 Paul Kinchesh                                          */
/*               2012 Martyn Klassen                                         */
/*               2012 Margaret Kritzer                                       */
/*                                                                           */
/* Xrecon is free software: you can redistribute it and/or modify            */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* Xrecon is distributed in the hope that it will be useful,                 */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with Xrecon. If not, see <http://www.gnu.org/licenses/>.            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/**/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Usage:   Xrecon [-v] VnmrJ.fid(s)                                         */
/*            Options:                                                       */
/*              [-v] flags Xrecon that it has been called from within VnmrJ  */
/*                   'VnmrJ.fid' should then be set to curexp                */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/**/


/*---------------------*/
/*---- Define data ----*/
/*---------------------*/
struct data d0;


/*----------------------------------*/
/*---- Define input filename(s) ----*/
/*----------------------------------*/
struct file f0;


/*-------------------------*/
/*---- Include globals ----*/
/*-------------------------*/
#define LOCAL /* EXTERN globals will not be 'extern' */
#include "Xrecon.h"


static struct sigaction act;

static void sighandler(int signum, siginfo_t *info, void *ptr) {
   //  printf("Received signal %d\n", signum);
   //  printf("Signal originates from process %lu\n",
     //    (unsigned long)info->si_pid);
     interupt=TRUE;
}


int main(int argc,char *argv[])
{
  int i;
  FILE *fpid;
  char pidfilename[MAXPATHLEN];
  char *hn;

  hn = getenv("HOME");
  strcpy(pidfilename, hn);
  strcat(pidfilename,"/xrecon_pid_file");

  pid_t processid;
  fpid=fopen(pidfilename,"w");
  processid=getpid();
  fprintf(fpid,"%d\n",processid);
  fclose(fpid);

  interupt=FALSE;

  memset(&act, 0, sizeof(act));
  act.sa_sigaction = sighandler;
  act.sa_flags = SA_SIGINFO;

  sigaction(SIGUSR1,&act, NULL);

  /* Null the data structure */
  nulldata(&d0);

  /* Get input options and directories */
  getoptions(&f0,argc,argv);

  /* Loop over input '.fid' directories */
  for (i=0;i<f0.nfiles;i++) {

    /* Interupt/cancel from VnmrJ */
    if (interupt) exit(1);

    /* Get pars from f.procpar[i] */
    getpars(f0.procpar[i],&d0);

    /* Open data file f.fid[i] and get file header */
    opendata(f0.fid[i],&d0);

    /* Set data structure members from procpar values */
    setdatapars(&d0);

    /* Check apptype to determine recon type */

    if (spar(&d0,"apptype","im1Dglobal")) recon1D(&d0);

    if (spar(&d0,"apptype","im1D")) recon1D(&d0);

    else if (spar(&d0,"apptype","im2D")) recon2D(&d0);

    else if (spar(&d0,"apptype","im2Dfse")) recon2D(&d0);

    else if (spar(&d0,"apptype","im2Depi")) reconEPI(&d0);

    else if (spar(&d0,"apptype","im3D")) recon3D(&d0);

    else if (spar(&d0,"apptype","im3Dfse")) recon3D(&d0);

    else if (spar(&d0,"apptype","im2Dcsi")) reconCSI2D(&d0);

    else if (spar(&d0,"apptype","im3Dcsi")) reconCSI3D(&d0);

    /* Close data file f.fid[i] */
    closedata(&d0);

  }

  unlink(pidfilename);

  exit(0);

}

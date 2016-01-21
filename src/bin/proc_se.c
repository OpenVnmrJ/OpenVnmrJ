/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* proc_se.c - recombine nD SE spectra, revision 4. 
 Allows to choose the dimension, simplified syntax */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "vnmrio.h"
#include "ddft.h"
static int c[8]={1,0,-1,0,0,-1,0,-1}; // default coefficients

int main(argc, argv)
    int argc;char *argv[]; {
    FILE *src, *trg;
    FILEHEADER fhd;
    BLOCKHEADER **bhd;
    COMPLX **dfid;
    char fnm[MAXSTR];
    int i, k, np, nb, nbh, nx=1, nc=8;
    void proc_se(), reset_pars_se();
    //f1coef = '1 0 -1 0   0 -1 0 -1'
    fnm[0]=0;
    if ((argc < 2) || (!fid_ext(argv[1])))
        vn_error("\n Usage :  proc_se fname.fid <SE loop size>. ");
    if (argc > 2){
        if(sscanf(argv[2], "%d", &i)>=1)
            nx=i;
        if(argc>3) // note: if f1coef contains < 8 terms excess values will not be changed
            nc=sscanf(argv[3], "%d %d %d %d %d %d %d %d",c,c+1,c+2,c+3,c+4,c+5,c+6,c+7);
    }

    printf("\n proc_se nx:%d nc:%d c:",nx,nc);
    for(i=0;i<8;i++)
        printf("%d ",c[i]);
    printf("\n");

    (void) check_sys_arch();

    strcat(strcpy(fnm, argv[1]), ".orig");
    if (copy_fid(argv[1], fnm, 1) == 0)
        exit(0);
    (void) reset_pars_se(argv[1]);

    strcat(fnm, "/fid");

    if ((src = open_fid(fnm, "r")) == NULL )
        exit(0); /* read source file header */
    if ((trg = open_fid(argv[1], "w")) == NULL )
        exit(0);

    if (!get_fheader(src, &fhd, 1))
        vn_abort(src, "\nfailed to read fid file header.\n");
    if (!put_fheader(trg, &fhd, 1))
        vn_abort(trg, "\nfailed to write fid file header.\n");

    np = fhd.np;
    nb = fhd.nblocks;
    nbh = fhd.nbheaders;

    dfid = calloc_C2d(nx, np / 2);
    bhd = calloc_BH2d(nbh, nx);

    k = nb / nx;
    for (i = 0; i < k; i++) {
        if (get_fids(src, fhd, &bhd, &dfid, nx, 0) < nx)
            break;
        (void) proc_se(&dfid, np / 2, nx, nx);
        if (put_fids(trg, fhd, &bhd, dfid, nx, 0) < nx)
            break;
        printf("blocks processed\r%-4d ", (i + 1) * nx);
    }
    printf("blocks processed");

    fclose(src);
    fclose(trg);

    printf("\nDone. \n");
    exit(1);
}

void reset_pars_se(dir)
    /* reset fncoef-s */
    char *dir; {
    FILE *fnm, *ftmp;
    int i;
    char str[512], str2[512];

    strcat(strcpy(str, dir), "/procpar");
    strcat(strcpy(str2, dir), "/tmp");

    rename(str, str2);

    if ((fnm = fopen(str, "w")) == NULL )
        vn_error("failed to open procpar. ");
    if ((ftmp = fopen(str2, "r")) == NULL )
        vn_error("failed to open tmp file. ");

    fseek(ftmp, 0, 0);
    while (fgets(str, 512, ftmp))		    // fix parameters
    {
        if ((str[0] == 'f') && (str[2] == 'c'))  // f1coef, f2coef, f3coef
                {
            sscanf(str, "%s", str2);
            if ((strcmp(str2, "f1coef") == 0) || (strcmp(str2, "f2coef") == 0)
                    || (strcmp(str2, "f3coef") == 0)) {
                fputs(str, fnm);
                fgets(str, 512, ftmp);
                if (sscanf(str, "%d %s", &i, str2) == 2)
                    sprintf(str, "%d \"\" \n", i);
            }
        }
        fputs(str, fnm);
    }
    fclose(fnm);
    fclose(ftmp);

    strcat(strcpy(str2, dir), "/tmp");
    remove(str2);
}

void proc_se(dfid, np, jx, nx)
    /* nx is block size, jx is SE size */
    int np, jx, nx;COMPLX **dfid[nx][np]; {
    int i, j, k, i0, i1, i2;
    double tm, re, im, R1,R2,I1,I2;

    if (nx < jx) {
        printf("se_proc ignored: block size < SE size !\n");
        return;
    }

#define TEST
    i0 = jx / 2;
    for (k = i0; k < nx; k += jx) {
        for (i = 0; i < i0; i++) {
            i1 = k + i;
            i2 = i1 - i0;
            for (j = 0; j < np; j++) {
#ifndef TEST
                //'1 0 -1 0 0 -1 0 -1'
                re = (**dfid)[i2][j].re; im = (**dfid)[i2][j].im;
                (**dfid)[i2][j].re -= (**dfid)[i1][j].re; // Re = re1 - re2
                (**dfid)[i2][j].im -= (**dfid)[i1][j].im;
                tm = (**dfid)[i1][j].re;
                (**dfid)[i1][j].re = (im + (**dfid)[i1][j].im);// Im = im1 + im2
                (**dfid)[i1][j].im =-(re + tm);
#else
                //'1 0 1 0 0 -1 0 1'
                R1=(**dfid)[i2][j].re;
                I1=(**dfid)[i2][j].im;
                R2=(**dfid)[i1][j].re;
                I2=(**dfid)[i1][j].im;
                (**dfid)[i2][j].re=c[0]*R1+c[2]*R2;
                (**dfid)[i2][j].im=c[0]*I1+c[2]*I2;

                (**dfid)[i1][j].re=-(c[5]*I1+c[7]*I2); // why is this one inverted ?
                (**dfid)[i1][j].im= (c[5]*R1+c[7]*R2);
#endif
            }
        }
    }

    return;
}


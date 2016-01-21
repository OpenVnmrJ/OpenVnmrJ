/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* sch2fid.c - convert sampling schedule into fid format. Eriks Kupce, Jan 2011 
 for 3D check, f1coef and f2coef !!!
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "vnmrio.h"
#include "random.h"

#define FNSC "/sched.fid"  // to be stored in the current experiment 
#define FN3D "/vnmr/biopack/fidlib/HNCO_3D.fid"  
#define FN2D "/vnmr/fidlib/fid2d.fid"
#define FA    1.0       // amplitude of the data points defined here
int main(argc, argv)
    int argc;char *argv[]; {
    FILE *src, *trg;
    FILEHEADER fhd;
    BLOCKHEADER bhd;
    COMPLX *dfid;
    char fnm[MAXSTR], fnm2[MAXSTR];
    int i, j, k, ii, np, nb, nsp, ix;
    int ni=0, ni2=0, ni3=0, nimax=0, ni2max=0, ni3max=0, xdim=0;
    int *sch, nix[6]={0};
    float f1d, f1z;
    void usage(), fix_pars();

    ni = 0;
    ni2 = 0;
    ni3 = 0;
    nimax = 0;
    ni2max = 0;
    ni3max = 0;
    xdim = 0;
    nsp = 0;
    f1z = 0.0;
    f1d = FA;

    printf("\n sch2fid : \n");

    if ((argc < 4) || (argc > 8))
        usage();
    sprintf(fnm, "%s", argv[1]);
    i = 2;
    while (argc > i) {
        if (sscanf(argv[i], "%d", &nix[i - 2]) < 1)
            usage();
        i++;
    }
    ni = nix[0];
    if (ni < 2)
        ni = 1, nimax = 0;
    else
        nimax = nix[1];
    ni2 = nix[2];
    if (ni2 < 2)
        ni2 = 1, ni2max = 0;
    else
        ni2max = nix[3];
    ni3 = nix[4];
    if (ni3 < 2)
        ni3 = 1, ni3max = 0;
    else
        ni3max = nix[5];

    xdim = 0;
    nb = 1;
    nsp = ni * ni2 * ni3;
    if (nimax >= ni)
        nb *= ni, xdim++;
    else
        nimax = 0;
    if (ni2max >= ni2)
        nb *= ni2, xdim++;
    else
        ni2max = 0;
    if (ni3max >= ni3)
        nb *= ni3, xdim++;
    else
        ni3max = 0;
    if (!xdim)
        vn_error("sch2fid: No sparse dimensions specified.");

    nix[0] = 1;
    nix[1] = 0;
    nix[2] = 0;
    if ((xdim > 1) && (nimax)) {
        nix[1] = nimax;
        if ((xdim > 2) && (ni2max))
            nix[2] = nimax * ni2max;
    } else if ((xdim > 1) && (ni2max))
        nix[1] = ni2max;
    sch = get_sch(fnm, &i, nix, xdim, nb);
    if ((sch == NULL )|| (i == 0) || (nsp !=i ))
        vn_error("sch2fid: failed to read the sampling schedule");

    if (not_sequential(sch, nsp, xdim)) {
        if (sort_me_out(&sch, nsp) == NULL )
            vn_abort("sch2fid: failed to process the sampling schedule. ");
        printf("Randomized schedule.\n");
    } else
        printf("Sequential schedule.\n");

    (void) check_sys_arch();

    if (xdim > 2)
        sprintf(fnm2, "%s", FN3D);
    else
        sprintf(fnm2, "%s", FN2D);
    if (!fid_exists(fnm2, 0))
        vn_error("sch2fid: nD source file not found. ");
    if ((src = open_fid(fnm2, "r")) == NULL )
        exit(0);   // read FID-file headers
    if (!get_fheader(src, &fhd, 1))
        vn_abort(src, "\nsch2fid: failed to get FID file header \n");
    if (!get_bheader(src, &bhd, 0))
        vn_abort(src, "\nsch2fid: failed to get FID block header \n");
    fclose(src);

    strcat(fnm, "/sampling_sch.fid");
    mk_dir(fnm);
    copy_par(fnm2, fnm);

    k = -1;
    np = nimax;
    if (xdim == 1) {
        nb = 128;
        ii = 64;
        j = nb;
        if (ni2max > 1)
            np = ni2max;
        if (ni3max > 1)
            np = ni3max;
    } else if (xdim == 2) {
        nb = ni2max;
        if (ni3max > 1) {
            nb = ni3max;
            if (ni2max > 1)
                np = ni2max;
        }
        j = nb;
    } else {
        nb = ni2max * ni3max;
        j = ni2max;
        k = ni3max;
    }
    fix_pars(fnm, 2 * np, j, k, 1);
    (void) set_fhd(&fhd, np * 2, nb);
    dfid = calloc_C1d(np);

    printf("ni:%d nimax:%d xdim:%d\n",ni,nimax,xdim);

    for (i = 0; i < np; i++)
        dfid[i].re = 0.0, dfid[i].im = 0.0;

    if ((trg = open_fid(fnm, "w")) == NULL )
        exit(0);
    if (!put_fheader(trg, &fhd, 1))
        vn_abort(trg, "\nsch2fid: failed to write FID file header \n");

    ix = 0;

    for (i = 0, j = 0; i < nb; i++) {
        bhd.iblock = i + 1;
        if (!put_bheader(trg, &bhd, 0))
            vn_abort(trg, "\nsch2fid: failed to write FID block header \n");
        if (!((xdim == 1) && (i != ii))) {
            for (k = 0; k < np; k++, j++) {
                if ((ix < nsp) && (j == sch[ix])){
                    dfid[k].re = FA, ix++;
                }
            }
        }
        if (!put_fid(trg, fhd, dfid))
            break;
        for (k = 0; k < np; k++)
            dfid[k].re = 0.0;

        // printf("blocks written\r%-4d ", i+1);
    }

//  printf("blocks written.\n");  
    fclose(trg);

    printf("\nDone. \n");

    return 1;
}

void usage() {
    vn_error("Usage: sch2fid curexp ni nimax <ni2 ni2max>, <ni3 ni3max> \n");
}

void fix_pars(dir, npx, nix, ni2x, rep)
    // fix ncyc and phases as for abs val spectra
    char *dir;                               // np, ni and ni2 as in vnmr
    int npx, nix, ni2x;short rep; {
    FILE *src, *trg;
    int i, j, np;
    double dm;
    char str[512], str2[512];

    strcat(strcpy(str, dir), "/procpar");
    strcat(strcpy(str2, dir), "/tmp");

    rename(str, str2);

    if ((trg = fopen(str, "w")) == NULL ) {
        printf("fopen %s failed\n", str);
        exit(1);
    }
    if ((src = fopen(str2, "r")) == NULL ) {
        printf("fopen %s failed\n", str2);
        exit(1);
    }
    if (rep)
        printf("fixing procpar: %d, %d, %d \n", npx, nix, ni2x);

    np = 0;
    if ((npx) && (getval_i(src, "np", &np) == 0))
        vn_abort(trg, "fix_pars: failed to get the original np. \n");

    fseek(src, 0, 0);
    while (fgets(str, 512, src)) /* fix parameters */
    {
        fputs(str, trg);
        switch (str[0]) {
        case 'n':
            sscanf(str, "%s\n", str2);
            if ((npx > -1) && (str2[1] == 'p') && (str2[2] == '\0')) /* np */
            {
                fgets(str, 512, src);
                if(sscanf(str, "%d %d\n", &i, &j)==2)
                    fprintf(trg, "%d %d\n", i, npx);
                else
                    printf("sscanf failed for np\n");
            } else if ((nix > -1) && (str2[1] == 'i') && (str2[2] == '\0')) /* ni */
            {
                fgets(str, 512, src);
                if(sscanf(str, "%d %d\n", &i, &j)==2)
                    fprintf(trg, "%d %d\n", i, nix);
                else
                    printf("sscanf failed for ni\n");
            } else if ((ni2x > -1) && (str2[1] == 'i') && (str2[2] == '2')
                    && (str2[3] == '\0')) /* ni2 */
                    {
                fgets(str, 512, src);
                sscanf(str, "%d %d\n", &i, &j);
                fprintf(trg, "%d %d\n", i, ni2x);
            }
            break;
        case 'c':
            sscanf(str, "%s\n", str2);
            if (strcmp(str2, "celem") == 0) /* celem - completed increments */
            {
                fgets(str, 512, src);
                sscanf(str, "%d %d\n", &i, &j);
                j = nix;
                if (ni2x > 1)
                    j *= ni2x;
                fprintf(trg, "%d %d\n", i, j);
            }
            break;
        case 'a':
            sscanf(str, "%s\n", str2);
            if ((str2[1] == 'r') && (str2[2] == 'r')) {
                if (strcmp(str2, "array") == 0) /* array */
                {
                    fgets(str, 512, src);
                    sscanf(str, "%d %s\n", &i, str2);
                    fprintf(trg, "%d \"\" \n", i);
                } else if (strcmp(str2, "arraydim") == 0) /* arraydim */
                {
                    fgets(str, 512, src);
                    sscanf(str, "%d %d\n", &i, &j);
                    j = nix;
                    if (ni2x > 1)
                        j *= ni2x;
                    fprintf(trg, "%d %d\n", i, j);
                } else if (strcmp(str2, "arrayelemts") == 0) /* arrayelemts */
                {
                    fgets(str, 512, src);
                    sscanf(str, "%d %d\n", &i, &j);
                    j = 1;
                    if (ni2x > 1)
                        j = 2;
                    fprintf(trg, "%d %d\n", 1, j);
                }
            } else if ((str2[1] == 'c') && (str2[2] == 'q')) {
                if (strcmp(str2, "acqcycles") == 0) /* acqcycles */
                {
                    fgets(str, 512, src);
                    sscanf(str, "%d %d\n", &i, &j);
                    j = nix;
                    if (ni2x > 1)
                        j *= ni2x;
                    fprintf(trg, "%d %d\n", i, j);
                } /* acqdim unchanged */
            } else if ((str2[1] == 't') && (str2[2] == '\0')) /* at */
            {
                fgets(str, 512, src);
                sscanf(str, "%d %lf\n", &i, &dm);
                if ((npx > 1) && (np > 0))
                    dm *= (double) npx / np;
                fprintf(trg, "%d %.6f\n", i, dm);
            }
            break;
        case 'p':
            sscanf(str, "%s\n", str2);
            if (strcmp(str2, "phase") == 0) /* phase set to av */
            {
                fgets(str, 512, src);
                fprintf(trg, "1 1\n");
            } else if (strcmp(str2, "phase2") == 0) /* phase2 set to av */
            {
                fgets(str, 512, src);
                fprintf(trg, "1 1\n");
            }
            break;
        }
    }
    fclose(trg);
    fclose(src);

    strcat(strcpy(str2, dir), "/tmp");
    remove(str2);
}


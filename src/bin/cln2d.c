/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* cln2d.c - authentic CLEAN 2d spectrum. Cleans 2d spectrum given a 
 sampling schedule sch1d.sch and a sparsely sampled data set xxx.fid
 added scaling and frequency inversion. Expects inflated data.
 Expected processing flow: rsw > proc_se > inflateND > clnND
 */

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "vnmrio.h"
#include "ddft.h"
#include "cln2d.h"
#include <stdio.h>

int main(argc, argv)
    int argc;char *argv[]; {
    char fnm[MAXSTR], fnm2[MAXSTR], str[MAXSTR];
    COMPLX **dfid;
    FILEHEADER fhd;
    BLOCKHEADER bhd1, bhd2;
    FILE *src;
    int *sch, i, j, np, ni, ncyc, nsp, not_pow2;
    short sft, ift, reps, dnse;
    double cf, thr;

    printf("cln2d: \n");

    cf = 2.5;
    reps = 0;
    ift = 1;
    sft = 0;
    dnse = 0;
    not_pow2 = 1;           // set defaults

    if (argc < 3)
        vn_error("\n Usage :  cln2d inp.fid out.fid <noise level> <-f (no ift)> <-v (verbose)> \n");
    i = 3;
    while (argc > i) {
        if ((argv[i][0] == '-') && (argv[i][1] == 'f'))
            ift = 0;                 // ift is disabled
        else if ((argv[i][0] == '-') && (argv[i][1] == 'd'))
            dnse = 1;
        else if ((argv[i][0] == '-') && (argv[i][1] == 'v'))
            reps = 1;
        else if ((sscanf(argv[i], "%lf", &thr) > 0) && (thr > 0.5))
            cf = thr;
        i++;
    }

    (void) check_sys_arch();

    strcpy(fnm, argv[1]);
    strcpy(fnm2, argv[2]);
    (void) add_fid_ext(fnm);
    (void) add_fid_ext(fnm2);

    sch = get_sched(fnm, &nsp, 0);
    if (reps)
        printf("number of sampling points: %d\n", nsp);
    dfid = read_2dfid_zf(fnm, &fhd, &bhd1, &bhd2, 0, 1, &not_pow2, reps);

    src = open_procpar(fnm, "r");
    getval_s(src, "array", str);

    ncyc=1;
    if(strcmp(str, "phase") == 0 || strcmp(str, "phase2")==0 || strcmp(str, "phase3")==0)
        ncyc=2;

    ni = fhd.nblocks / ncyc;
    np = fhd.np / 2;
    if (nsp > ni)
        vn_error("\ncln2d: Error - data not inflated! ");
    if (sft)
        ftrs_f2(&dfid, fhd, sch, nsp, reps);  // time domain data - sft disabled
    else                                 // use rsw with no iFT in F2 - default
    {
        scale2dC(&dfid, ncyc * ni, np, (double) np);
        invert_frq_2dC(&dfid, ncyc * ni, np);
    }
    for (j = 0; j < np; j++) {
        if (reps)
            printf("cleaning trace %4d...", j);
        if(ncyc==2)
            i = clean_1d(&dfid, cf, sch, nsp, fhd.nblocks/2, np, j, dnse, reps);
        else
            i = clean_1d_cmplx(&dfid, cf, sch, nsp, fhd.nblocks, np, j, dnse, reps);

        if (reps)
            printf("# iterations %4d\n", i);
        else
            printf("F2 traces processed\r%-4d", j + 1);
    }
    printf("\n ");

    printf("cln2d: Inverse FT ... \n");          // if ift

    j = ncyc * ni;
    for (i = 0; i < j; i++)                          // inverse FT(F2) all fids
        cft1dy(&dfid[i], np, 'x');                 // np  must be pow2

    printf(" cln2d: Saving data in %s...\n", fnm2);
    mk_dir(fnm2);
    copy_par(fnm, fnm2);
    if (reps)
        printf("\nsaving processed data as %s\n", fnm2);
    write_2d(fnm, fnm2, dfid, reps);                       // write 2d data

    if (not_pow2)
        (void) fix_np(fnm2, fhd.np, -1, -1, 0);

    printf("\ncln2d done. \n");
    exit(1);
}


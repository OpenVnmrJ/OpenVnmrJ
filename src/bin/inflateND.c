/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* inflateND.c - inflate randomly sampled data sets. Eriks Jan 2011 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "vnmrio.h"
#include "random.h"

int main(argc, argv)
    int argc;char *argv[]; {
    FILE *src, *trg;
    FILEHEADER fhd;
    BLOCKHEADER bhd;
    char fnm[MAXSTR], fnm2[MAXSTR], str[MAXSTR];
    int i, j, k, kk, nb, ns, nd, nsp, nsq, ncyc, ib, ix, db, zb, fb;
    int deflate = 0;
    int ni, ni2, ni3, nimax, ni2max, ni3max, xdim, celem;
    int *sch, *seq, nix[4];
    float f1d, **ffid, dummy;
    double tm;
    double csd = 0.0;
    int debug_file_io = 0;

    ni = 0;
    ni2 = 0;
    ni3 = 0;
    nimax = 0;
    ni2max = 0;
    ni3max = 0;
    xdim = 0;
    nsp = 0;

    printf("\n inflateND start: \n");

    if ((argc < 3) || (!fid_ext(argv[2])))
        vn_error("\n Usage :  inflateND fname.fid new.fid");

    sprintf(fnm, "%s", argv[1]);
    sprintf(fnm2, "%s", argv[2]);
    if (fid_exists(fnm2))
        exit(0);
    if (!fid_exists(fnm))
        vn_error("inflateND: file not found. ");

    (void) check_sys_arch();

    src = open_procpar(fnm, "r");
    nd = 1;
    i=getstat(src, "phase");
    j=getstat(src, "phase2");
    k = getstat(src, "phase3");
    ncyc = 1;
    if (i > 1)
        ncyc *= 2;
    if (j > 1)
        ncyc *= 2;
    if (k > 1)
        ncyc *= 2;

    printf("phase:%d phase2:%d phase3:%d\n",i,j,k);

    if (getval_i(src, "ni", &i) > 0) {
        if (getval_i(src, "nimax", &nimax)>0)
            ni=i;
        else if (getval(src, "CSdensity", &csd) > 0) {
            nimax = i;
            ni = i * csd / 100.0;
        }
        if(nimax){
            nd *= nimax;
            printf("ni:%d nimax:%d \n",ni,nimax);
        }
    }
    if (getval_i(src, "ni2", &i) > 0) {
        if (getval_i(src, "ni2max", &ni2max)>0)
            ni2=i;
        else if (getval(src, "CSdensity2", &csd) > 0) {
            ni2max = i;
            ni2 = i * csd / 100.0;
        }
        if(ni2max){
            nd *= nimax;
            printf("ni2:%d ni2max:%d\n",ni2,ni2max);
        }
    }
    if (getval_i(src, "ni3", &i) > 0) {
        if (getval_i(src, "ni3max", &ni3max)>0)
            ni3=i;
        else if (getval(src, "CSdensity3", &csd) > 0) {
            ni3max = i;
            ni3 = i * csd / 100.0;
        }
        if(ni3max){
            nd *= ni3max;
            printf("ni3:%d ni3max:%d\n",ni3,ni3max);
        }
    }

    getval(src, "celem", &tm);
    celem = (int) (tm + 0.5);

    fclose(src);

    j = 0;
    if ((nimax >= ni) && (ni > 1))
        j++;
    else
        nimax = 0;
    if ((ni2max >= ni2) && (ni2 > 1))
        j++;
    else
        ni2max = 0;
    if ((ni3max >= ni3) && (ni3 > 1))
        j++;
    else
        ni3max = 0;

    if (nimax)
        nimax = pow2(nimax); // make sure nimax-s are pow 2
    if (ni2max)
        ni2max = pow2(ni2max);
    if (ni3max)
        ni3max = pow2(ni3max);

    xdim = 0;
    if (nimax)
        xdim++;
    if (ni2max)
        xdim++;
    if (ni3max)
        xdim++;
    nix[0] = 1;
    nix[1] = 0;
    nix[2] = 0;
    if ((j > 1) && (nimax)) {
        nix[1] = nimax;
        if ((j > 2) && (ni2max))
            nix[2] = nimax * ni2max;
    } else if ((j > 1) && (ni2max))
        nix[1] = ni2max;

    if (ni < 2)
        ni = 1;
    if (ni2 < 2)
        ni2 = 1;
    if (ni3 < 2)
        ni3 = 1;
    nb = ni * ni2 * ni3; /* number of increments */

    if (!xdim)
        vn_error("inflateND: No sparse dimensions found.");

    sch = get_sch(fnm, &nsp, nix, xdim, nb);
    if ((sch == NULL )|| (nsp == 0))vn_error("inflateND: failed to read the sampling schedule");

    celem /= ncyc;
    kk = 0;
    if (celem < nsp)
        nsp = celem, kk = 1;

    nsq = 0;
    seq = NULL;
    (void) copy_fid(fnm, fnm2);
    if (not_sequential(sch, nsp, xdim)) {
        if ((seq = sort_me_out(&sch, nsp)) == NULL )
            vn_abort("inflateND: failed to process the sampling schedule. ");
        else
            save_rsch(fnm2, sch, nsp, ni, ni2, ni3, nimax, ni2max, ni3max);
        nsq = 1;
        printf("Randomized schedule.\n");
    } else {
        if (kk) // truncate schedule for truncated data sets
        {
            sprintf(str, "%s/sampling.sch", fnm);
            if ((src = fopen(str, "r")) == NULL )
                exit(0);
            sprintf(str, "%s/sampling.sch", fnm2);
            if ((trg = fopen(str, "w")) == NULL )
                exit(0);
            i = 0;
            while ((fgets(str, 512, src)) && (i < nsp))
                fputs(str, trg), i++;
            fclose(src);
            fclose(trg);
        }
        printf("Sequential schedule.\n");
    }
    i = -1;
    j = -1;
    k = -1;
    if (nimax)
        i = nimax;
    if (ni2max)
        j = ni2max;
    if (ni3max)
        k = ni3max;
    set_ni3(fnm2, i, j, k, ncyc, 1);

    if ((src = open_fid(fnm, "r")) == NULL )
        exit(0); // read FID-file headers
    if ((trg = open_fid(fnm2, "w")) == NULL )
        exit(0);
    if (!get_fheader(src, &fhd, 1))
        vn_abort(src, "\ninflateND: failed to get FID file header \n");


    fb = fhd.nblocks;
    ns = nb;
    nb = 1; // calculate the new nb
    if (nimax)
        nb *= nimax;
    else
        nb *= ni;
    if (ni2max)
        nb *= ni2max;
    else
        nb *= ni2;
    if (ni3max)
        nb *= ni3max;
    else
        nb *= ni3;

    printf("nb:%d ns:%d nd:%d ncyc:%d deflate:%d\n", nb, ns,
            nd, ncyc, fhd.nblocks == (nd * ncyc));

    if (fhd.nblocks == (nd * ncyc))
        deflate = 1;
    else if (fhd.nblocks == (ns * ncyc))
        deflate = 0;
    else
        vn_abort(src, "\ninflateND: invalid number of blocks (%d) in input data. expecting %d (sparse) or %d (full)\n"
                ,fhd.nblocks,nd * ncyc,ns * ncyc);
    db = zb = 0;

    (void) set_fhd(&fhd, fhd.np, ncyc * nb);
    if (!put_fheader(trg, &fhd, 1))
        vn_abort(trg, "\ninflateND: failed to write FID file header \n");

    if (nsq) {
        kk = ncyc * nb;
        ffid = calloc_f2d(kk, fhd.np);
        for (i = 0; i < kk; i++) // read all data
                {
            if (!get_bheader(src, &bhd, debug_file_io))
                break;
            for (j = 0; j < fhd.np; j++) {
                if (!fread(&ffid[i][j], sizeof(float), 1, src))
                    break;
            }
        }
        fclose(src);

        ib = 1;
        ix = 0;
        for (i = 0; i < nb; i++) {
            if ((ix < nsp) && (i == sch[ix])) {
                for (k = 0; k < ncyc; k++) {
                    bhd.iblock = ib;
                    ib++;
                    kk = seq[ix] * ncyc + k;
                    if (!put_bheader(trg, &bhd, debug_file_io))
                        break;
                    for (j = 0; j < fhd.np; j++) {
                        if (!fwrite(&ffid[kk][j], sizeof(float), 1, trg))
                            break;
                    }
                }
                ix++;
                db++;
            } else {
                f1d = 0.0;
                for (k = 0; k < ncyc; k++) {
                    bhd.iblock = ib;
                    ib++;
                    zb++;
                    if (!put_bheader(trg, &bhd, debug_file_io))
                        break;
                    for (j = 0; j < fhd.np; j++) {
                        if (!fwrite(&f1d, sizeof(float), 1, trg))
                            break;
                    }
                }
            }
        }
        //printf("blocks inflated to %d.\n", nb * ncyc);
        fclose(trg);
    } else {
        ib = 1;
        ix = 0;
        for (i = 0; i < nb; i++) {
            if ((ix < nsp) && (i == sch[ix])) {
                for (j = 0; j < ncyc; j++) {
                    if (deflate) {
                        if (i < nd && !get_bheader(src, &bhd, debug_file_io))
                            break;
                    } else if (!get_bheader(src, &bhd, debug_file_io))
                        break;

                    bhd.iblock = ib;
                    ib++;
                    db++;
                    if (!put_bheader(trg, &bhd, debug_file_io))
                        break;
                    for (k = 0; k < fhd.np; k++) {
                        f1d = 0.0;
                        if (deflate) {
                            if (i < nd && !fread(&f1d, sizeof(float), 1, src))
                                break;
                        } else if (!fread(&f1d, sizeof(float), 1, src))
                            break;
                        if (!fwrite(&f1d, sizeof(float), 1, trg))
                            break;
                    }
                }
                ix++;
            } else {
                f1d = 0.0;
                for (j = 0; j < ncyc; j++) {
                    if (i < nd && deflate
                            && !get_bheader(src, &bhd, debug_file_io))
                        break;
                    bhd.iblock = ib;
                    zb++;
                    ib++;
                    if (!put_bheader(trg, &bhd, debug_file_io))
                        break;
                    for (k = 0; k < fhd.np; k++) {
                        if (i < nd && deflate
                                && !fread(&dummy, sizeof(float), 1, src))
                            break;
                        if (!fwrite(&f1d, sizeof(float), 1, trg))
                            break;
                    }
                }
            }
        }
        fclose(src);
        fclose(trg);
    }
    if (deflate)
        printf("%d blocks deflated to %d.\n", fb, db);
    else
        printf("%d blocks inflated to %d.\n", fb, ib - 1);
    fprintf(stderr, "\nDone. \n");

    return 1;
}


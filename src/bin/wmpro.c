/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* wmpro.c - create window profiles. Eriks Kupce, 5th Aug 2012 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "vnmrio.h"
#include "ddft.h"

typedef struct /* procpar data */
{
    double lb, sb, sbs, gf, gfs;
    double at, sw;
    int np;
} WMPAR;

int main(argc, argv)
    int argc;char *argv[]; {
    FILE *trg;
    WMPAR get_wmpar(), wmpar;
    char fnm[MAXSTR];
    void usage();
    int i, dom;
    double *wpro;

    dom = 1;
    if (argc < 2)
        usage();
    if ((argc > 2) && (((sscanf(argv[2], "%d", &dom)) < 1) || (dom > 3)))
        usage();
    //printf("argv[1]=%s dom=%d\n",argv[1],dom);
    wmpar.np=0;
    wmpar = get_wmpar(argv[1], dom);
    if (wmpar.np < 2)
        vn_error("wmpro: window parameters not set.");


    wpro = (double *) calloc(wmpar.np, sizeof(double));
    for (i = 0; i < wmpar.np; i++)
        wpro[i] = 1.0;

    if (fabs(wmpar.lb) > 0.00001)
        (void) lbmult(wmpar.np, wmpar.np, wmpar.at, wmpar.lb, &wpro);
    if (fabs(wmpar.sb) > 0.00001)
        (void) sbmult(wmpar.np, wmpar.np, wmpar.at, wmpar.sb, wmpar.sbs, &wpro);
    if (fabs(wmpar.gf) > 0.00001)
        (void) gfmult(wmpar.np, wmpar.np, wmpar.at, wmpar.gf, wmpar.gfs, &wpro);

    strcpy(fnm, (char *) getenv("HOME"));
    strcat(fnm, "/vnmrsys/shapelib/pbox_wt");
    trg = open_file(fnm, "w");
    for (i = 0; i < wmpar.np; i++)
        fprintf(trg, "%12.8f\n", wpro[i]);
    fclose(trg);

    printf("\nDone.\n");
    return (0);
}

void usage() {
    printf("\n Usage : wmpro curexp domain (domain = 1, 2 or 3) \n");
    exit(1);
}

WMPAR get_wmpar(dir, dom)
    char *dir;int dom; {
    FILE *src;
    WMPAR wm;
    char par[32];
    int i=0, j;

    if ((src = open_curpar(dir, "r")) == NULL ) {
        if ((src = open_procpar(dir, "r")) == NULL )
            vn_abort(src, "Cannot open the parameter file. ");
    }

    i = 0;
    sprintf(par, "lb%d", dom);
    if (getval(src, par, &wm.lb) > 0)
        i++;
    sprintf(par, "gf%d", dom);
    if (getval(src, par, &wm.gf) > 0)
        i++;
    sprintf(par, "gfs%d", dom);
    if (getval(src, par, &wm.gfs) > 0)
        i++;
    sprintf(par, "sb%d", dom);
    if (getval(src, par, &wm.sb) > 0)
        i++;
    sprintf(par, "sbs%d", dom);
    if (getval(src, par, &wm.sbs) > 0)
        i++;
    if (dom > 1)
        sprintf(par, "ni%d", dom);
    else
        sprintf(par, "ni");
    if (!getval_i(src, par, &wm.np) > 0)
        i = 0;
    if (dom > 1)
        sprintf(par, "ni%dmax", dom);
    else
        sprintf(par, "nimax");
    if ((i) && (getval_i(src, par, &j) > 0) && (j > wm.np))
        wm.np = j;
    sprintf(par, "sw%d", dom);
    if ((i) && (getval(src, par, &wm.sw) < 1))
        vn_error("get_wmpar: failed to get window parameters. ");
   else if (i)
        wm.at = wm.np / wm.sw;
    fclose(src);

    if (i < 1)
        wm.np = 0;

    return wm;
}

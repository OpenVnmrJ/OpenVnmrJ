// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/*  aptunenv - probe tuning sequence for Vnmrs */

#include <standard.h>

void pulsesequence()
{
    double tunesw;              /* Sweep width (Hz) */
    double freq;                /* Center freq (MHz) */
    double fstart;              /* Sweep start freq (MHz) */
    double fend;                /* Sweep end freq (MHz) */
    int nfreqs;                 /* Number of frequencies */
    int np2;                    /* Number of data points (np/2) */
    int chan;                   /* RF xmit channel */
    double attn;                /* RF power */
    double acqOffset;           /* 0.0: sample at start of freq step */
                                /* 0.5: sample at middle of freq step */
                                /* 1.0: sample at end of freq step */
    double offset_sec;
    int trig;                   /* If true, put scope trig on user line 1 */
    char strbuf[MAXSTR];

    /* Set RF channel and center frequency */
    if (find("tchan") == -1) {
        chan = 1;
    } else {
        chan = (int) getval("tchan");
        if (chan < 1) {
            chan = 1;
        } else if (chan > 5) {
            chan = 5;
        }
    }

    /* Set sweep limits */
    freq = sfrq;
    tunesw = getval("tunesw");
    if (tunesw < 0.0) {
        tunesw = 10e6;
    }
    fstart = freq - (tunesw/2) * 1e-6;
    fend = freq + (tunesw/2) * 1.0e-6;

    /* Set number of frequency steps */
    nfreqs = np2 = (int)getval("np") / 2;  /* Default one point per frequency */
    if (find("tunfreqs") != -1) {
        nfreqs = (int)getval("tunfreqs");
        if (np2 % nfreqs != 0) {
            printf("Warning: np/2 (%d) not divisible by tunfreqs (%d)\n",
                   np2, nfreqs);
        }
    }

    /* Set acquisition timing */
    acqOffset = 0.5;            /* Default to middle of freq step */
    if (find("tuacqoffset") != -1) {
        acqOffset = getval("tuacqoffset");
    }

    /* Set RF power */
    if (find("tupwr") == -1) {   /* Optional parameter "tupwr" sets tune pwr */
        attn = 10.0;
    } else {
        attn = getval("tupwr");
        if (attn > 20.0) {
            attn = 20.0;
        }
    }

    /* Whether we want a scope trigger (is tutrig='y') */
    trig = 0;
    if (find("tutrig") != -1) {
        getstr("tutrig", strbuf);
        if (*strbuf == 'y') {
            trig = 1;
        }
    }

    /* The pulse sequence */
    status(A);
    hsdelay(d1);
    set4Tune(chan,getval("gain")); 
    assign(zero,oph);
    genPower(attn,chan);
    delay(0.0001);
    /*printf("f0=%f, f1=%f, nfreqs=%d, center=%f, span=%f, step=%f\n",
           fstart, fend, nfreqs,
           (fend + fstart) / 2, (fend - fstart),
           (fend - fstart) / (nfreqs - 1)); */ /*DBG*/
    if (trig) {
        sp1on();
    }
    offset_sec = (acqOffset / sw) * np2 / nfreqs;
    SweepNOffsetAcquire(fstart, fend, nfreqs, chan, offset_sec); 
    if (trig) {
        sp1off();
    }
}

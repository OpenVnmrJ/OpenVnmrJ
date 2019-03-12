// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* 
 */

#include <standard.h>

#include "acodes.h"

pre_fidsequence()
{
    int chan;
    int attn;

    chan = 1;

    if (find("tupwr") == -1){   /* Optional parameter "tupwr" sets tune pwr */
        attn = 60;
    }else{
        attn = (int)getval("tupwr");
    }

    /* Start tune mode */
    putcode(TUNE_START);
    putcode(chan);
    putcode(attn);
    delay(0.01);
}

void pulsesequence()
{
    int i;
    double df;
    double f;
    double *freqs;
    double fstart;
    double tunesw;
    int ppfreq;                 /* Samples per frequency */
    int nfreqs;
    int chan;
    int attn;

    chan = 1;

    if (find("tupwr") == -1){   /* Optional parameter "tupwr" sets tune pwr */
        attn = 60;
    }else{
        attn = (int)getval("tupwr");
    }


    tunesw = getval( "tunesw" );
    if (tunesw < 0.0)
      tunesw = 1.0e7;

    if (find("tuppf") == -1) {  /* Optional "tuppf" sets Points Per Frequency */
        ppfreq = 2;
    } else {
        ppfreq = getval( "tuppf" );
    }
        

    /*
     * Start with a sweep of tunesw, centered on sfrq.
     * sfrq is in units of MHz; freqs[] array must be in MHz.
     * All other frequencies are in Hz.
     */

    fstart = sfrq * 1e6 - tunesw / 2;

    nfreqs = np / (2 * ppfreq);
    freqs = (double *)malloc(sizeof(double) * nfreqs);
    df = tunesw / (nfreqs - 1);

    for (i=0, f=fstart; i<nfreqs; i++, f+=df){
        freqs[i] = f * 1e-6;
    }
    create_freq_list(freqs, nfreqs, TODEV, 0);
    free(freqs);

    /* Begin pulse sequence */

    initval((double)nfreqs, v1);

    /* Start tune mode */
    /*
    putcode(TUNE_START);
    putcode(chan);
    putcode(attn);
    delay(0.01);
    */

    rcvron();
    delay(0.02);
    loop(v1, v2);{
        vfreq(0, v2);
        /* delay(0.0007); */
	delay(0.0005);
        /* Note: too short delay in acq gives "pts acquired 0 less than np" */
        acquire((double)(2 * ppfreq), 0.0005);
        /*acquire((double)(2 * ppfreq), 0.003);*/
    }endloop(v2);

    /* BEWARE: adding 1 ms delay here can delay sending data back by 2-3 sec!
    /*delay(0.001);*/
}

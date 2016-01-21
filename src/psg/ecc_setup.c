/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>

#include "vnmrsys.h"
#ifdef VNMR
#include "acodes.h"
#else
#ifdef NVPSG
#include "ACode32.h"
#else
#include "acodes.h"
#endif
#endif
#include "group.h"
#include "ecc.h"
#ifndef VNMR
#include "abort.h"
#endif
#include "pvars.h"
extern void vnmremsg(const char *paramstring);
extern void putDutyLimits(int n, int values[]);
extern float checkEccExcursions(Ecc ecc_matrix[N1][N2]);
extern int option_check(const char *option);

#define ECCBUGBIT 1

#ifdef PSG_LC
#include "acqparms.h"
#endif

double get_quantized_sdac_value();
void putSdacScaleNLimits(int n,int *values);
#ifdef NVPSG
void putEccAmpsNTimes(int n,int *amps,long long *times);
#endif
extern int bugmask;

#define DECCLIB "imaging/decclib" /* DECC directory, relative to "/vnmr" */

/* typedef struct{
 *    char label[16];
 *    float max_val;
 *    float max_ival;
 *    float scale_ival;
 *    int funcadr;
 *    float values[4];
 * } Sdac;
 */


#ifdef NVPSG
Sdac sdac_values[N_SDAC_VALS] = {
    {"eccscale", 0.1, 32767.0, 32768.0, 0},
    {"shimscale", 0.1, 32767.0, 32768.0, 0},
    {"totalscale", 1.0, 32767.0, 32768.0, 0},
    {"slewlimit", 0.0035, 32767.0, 32768.0, 0},
    {"dutylimit", 1.0, 32767.0, 32768.0, 0}
};

struct _coilLimits {
    double xrms,  yrms,  zrms;
    double current;
    double flowrate;
    int    tune;
    double xduty, yduty, zduty;
};

struct _coilLimits coilLimits;



#else
Sdac sdac_values[N_SDAC_VALS] = {
    {"eccscale", 0.1, 255.0, 256.0, 0x02},
    {"shimscale", 0.1, 255.0, 256.0, 0x12},
    {"totalscale", 1.0, 16383.0, 16384.0, 0x03},
    {"slewlimit", 0.0035, 255.0, 256.0, 0x04},
    {"dutylimit", 1.0, 255.0, 256.0, 0x24}
};
#endif

typedef struct{
    int src;
    int dst;
} Intpair;

#define DECC_LOOPTIME (4.0e-6)   /* Time between ECC updates by DSP */

/* Defines unique to non-NVPSG */

/* These AP defines are also in oopc.h, but other stuff there messes us up */
#ifndef APSELECT
#define APSELECT 0xA000
#define APWRITE 0xB000
#endif
/* Stuff about the DECC hardware */
#define DECC 0x968      /* Base AP address of DECC board */
#define DECCWRITE (APWRITE | (DECC & 0xf00))
#define DSPADDR 0x87ffc0   /* Address inside DSP to load into */
#define DSPBUFLEN 64    /* Number of words to load into DSP */

#define POLARITY_ADR 0x0f
#define SDAC_DATAADR 0x960
#define SDAC_ADRADR (SDAC_DATAADR+2)

/* end of non-NVPSG defines */

int quoteToMainFlag=0;
/*
 * Return the number of terms for the axis pair (srcaxis, dstaxis).
 */
int
nterms(int srcaxis, int dstaxis)
{
    int rtn;

    if (srcaxis == dstaxis){
        if (srcaxis == B0_AXIS)
            rtn= 4;
        else
            rtn = 6;
    }else if (dstaxis == B0_AXIS) {
        rtn = 4;
    }else{
        rtn = 3;
    }
    /*fprintf(stderr,"nterms(%d,%d)=%d\n", srcaxis, dstaxis, rtn);     */
    return rtn;
}

#ifndef VNMR
static int     deccFileRead=0; // the values have been read in and checked.
/*
 * Fill in the default values for an ecc matrix.
 */
static void
init_ecc_matrix(Ecc ecc_matrix[N1][N2])
{
    int i;
    int j;
    int k;

    for (i=0; i<N1; i++){
        for (j=0; j<N2; j++){
            ecc_matrix[i][j].nterms = nterms(i, j);
            for (k=0; k<ecc_matrix[i][j].nterms; k++){
                ecc_matrix[i][j].tau[k] = 1.0;
                ecc_matrix[i][j].amp[k] = 0.0;
            }
        }
    }
}

static int
deccWarnings()
{
    int rtn = FALSE;
    char buf[2];
    if (P_getstring(GLOBAL, "deccwarnings", buf, 1, sizeof(buf))
        || (buf[0] != 'n' && buf[0] != 'N'))
    {
        rtn = TRUE;
    }
#ifdef ECC_INCLUDE_MAIN
    return 1;
#else
    return rtn;
#endif
}

/*
 * Given the name of an input file, fill a matrix of Ecc structures
 * with the ECC values contained in the file.
 *
 * Format of input file is:
 * # Comment if line starts with "#" or """
 * # termtype  tau1 amp1   tau2 amp2 ...
 * xx    .025  -.03  *.011 .0137 .00278 .00741
 *
 * Termtype xy is ECC for y-axis due to x input
 * Possibilities are: xx xy xz xb0 yx yy yz yb0 zx zy zz zb0
 * where "b0" is the correction sent to the b0 coil.
 * Types are not case sensitive.
 * A "*" prefix to the tau value (as in tau2, above) means to leave
 * out that term from the ECC correction (tau2 and amp2 are both set
 * to zero).
 */
static int
get_decc_from_file(char *filepath, Ecc ecc_matrix[N1][N2])
{
    int i;
    int src;         /* Input axis number */
    int dst;         /* Output axis number */
    int nt;       /* Number of time constants of this type */
    int rtn=TRUE;
    char buf[512];
    char msg[512];
    FILE *fd = NULL;
    int disable;
    struct stat statbuf;

    /* Init matrix values and open input file */
    init_ecc_matrix(ecc_matrix);

    if (strstr(filepath, "/.None")) {
        return(FALSE);
    }

    if (stat(filepath, &statbuf) == -1
        || ! (statbuf.st_mode & S_IFREG)
        || ! (fd = fopen(filepath, "r") ))
    {
        text_error("Cannot find \"%s\". Check the sysgcoil parameter", filepath);
        psg_abort(1);
    }

    if (bugmask & ECCBUGBIT){
        printf("ECC: Amp correction = (srcscale/dstscale) / eccscale\n");
    }
    /* Parse input file */
    while (fgets(buf, sizeof(buf), fd)){
        char *tok;
        tok = strtok(buf, " \t\n");
        if (tok && *tok != '#' && *tok != '"'){
            if (strcasecmp(tok,"xx") == 0){
                src = 0; dst = 0;
            }else if (strcasecmp(tok,"xy") == 0){
                src = 0; dst = 1;
            }else if (strcasecmp(tok,"xz") == 0){
                src = 0; dst = 2;
            }else if (strcasecmp(tok,"xb0") == 0){
                src = 0; dst = 3;
            }else if (strcasecmp(tok,"yx") == 0){
                src = 1; dst = 0;
            }else if (strcasecmp(tok,"yy") == 0){
                src = 1; dst = 1;
            }else if (strcasecmp(tok,"yz") == 0){
                src = 1; dst = 2;
            }else if (strcasecmp(tok,"yb0") == 0){
                src = 1; dst = 3;
            }else if (strcasecmp(tok,"zx") == 0){
                src = 2; dst = 0;
            }else if (strcasecmp(tok,"zy") == 0){
                src = 2; dst = 1;
            }else if (strcasecmp(tok,"zz") == 0){
                src = 2; dst = 2;
            }else if (strcasecmp(tok,"zb0") == 0){
                src = 2; dst = 3;
#ifdef NVPSG
            }else if (strcasecmp(tok,"b0b0") == 0){
                src = 3; dst = 3;
#endif
            }else{
                continue;   /* Skip this line */
            }
            nt = ecc_matrix[src][dst].nterms;
            for (i=0; i<nt && (tok=strtok(NULL," \t\n")); i++) {
                float amp;
                double ampcorr;
                double eccgain;
                double srcgain;
                double dstgain;
                disable = FALSE;
                if (*tok == '*') {
                    disable = TRUE;
                    ecc_matrix[src][dst].tau[i] = 0.0;
                } else if (sscanf(tok,"%g",&ecc_matrix[src][dst].tau[i])!=1)
                {
                    sprintf(msg,
                            "Bad tau in DECC file \"%.300s\": \"%.99s\"",
                            filepath, tok);
                    vnmremsg(msg);
                    rtn=FALSE;
                    break;
                }
                if (bugmask & ECCBUGBIT){
                    printf("ECC: tau[%d][%d][%d]=%g, ",
                           src, dst, i, ecc_matrix[src][dst].tau[i]);
                }
                tok = strtok(NULL," \t\n");
                if (!tok) {
                    sprintf(msg,
                            "Missing amplitude in DECC file \"%.300s\"",
                            filepath);
                    vnmremsg(msg);
                    rtn = FALSE;
                    break;
                }
                if (disable){
                    amp = 0.0;
                } else if (sscanf(tok,"%g",&amp) != 1)
                {
                    sprintf(msg,
                            "Bad amp in DECC file \"%.300s\": \"%.99s\"",
                            filepath, tok);
                    vnmremsg(msg);
                    rtn = FALSE;
                    break;
                }
                ampcorr = amp;
                /* Correct amplitude for different gains on SDAC */
                eccgain = get_quantized_sdac_value(ECCSCALE, dst);
                srcgain = get_quantized_sdac_value(TOTALSCALE, src);
                if (dst != B0_AXIS)
                {
#ifndef NVPSG
                    if (dst == B0_AXIS) srcgain = fabs(srcgain);
#endif
                    dstgain = get_quantized_sdac_value(TOTALSCALE, dst);
                    if (dstgain * eccgain != 0) {
                        ampcorr = amp * srcgain / (dstgain * eccgain);
                    }
                    if (bugmask & ECCBUGBIT){
                        printf("amp=%g * (%g / %g) / %g = %g\n",
                               amp, srcgain, dstgain, eccgain, ampcorr);
                    }
                }
#ifdef NVPSG
                else
                {
                    ampcorr = amp * srcgain / eccgain;
                }
#endif
                ecc_matrix[src][dst].amp[i] = ampcorr;
            }
        }
    }
    /* Check that the ECC values are within range */
    if (rtn && deccWarnings() && checkEccExcursions(ecc_matrix) >= 1)  {
        vnmremsg("ECC correction may be too large for DECC Board");
    }

    fclose(fd);
    return rtn;
}

#ifdef AIX
static double scalbn()
{
   return(1.0);
}
#endif

#ifndef NVPSG
/*
 * Convert a floating point number in native format to TMS32032
 * floating point single precision (32 bit) format.  Note that the
 * TMS floating point value is returned as an unsigned integer.
 */
static unsigned int
float2tms32(float x)
{
    unsigned int zero = 0x80000000; /* Zero value is special case */
    int nfracbits = 23;    /* Not including hidden bit / sign bit */
    int signbit = 1 << nfracbits;
    int fracmask = ~((~0)<<nfracbits);
    int iexp;
    int sign;
    int ifrac;
    unsigned int rtn;

    if (x == 0){
        rtn = zero;
    }else{
        iexp = ilogb(x);  /* Binary exponent if 1 <= |fraction| < 2 */
        ifrac = (int)scalbn(x, nfracbits-iexp); /* Frac part as integer */
        if (x<0 && (ifrac & signbit)){
            /* Force top bit of negative fraction to be 0 */
            ifrac <<= 1;
            iexp--;
        }
        sign = x<0 ? signbit : 0;
        rtn = (iexp << (nfracbits+1)) | sign | (ifrac & fracmask);
    }
    return rtn;
}

/*
 * Take ECC values (for one output axis) from an ECC matrix, and form
 * a list of 32 bit words for the DSP bootloader.
 * Buffer organization:
 * +------------------------------------------------+
 * |        |            |                |         |
 * |        |      downloaded data        |         |
 * | header |            |                | trailer |
 * |        | amplitudes | time constants |         |
 * |        |            |                |         |
 * +------------------------------------------------+
 * Returns a pointer to the list, and sets "nwords" to the length of
 * the list.
 * NB: The caller must free the memory for the list.
 */
static unsigned int *
make_tms_ecc_download(int axis,  /* Chip # (0-2) */
            Ecc ecc_matrix[N1][N2],
            int *nwords)
{
    unsigned int *rtn;
    unsigned int *pui;
    int i, j;
    int src, dst;
    int nt;       /* # terms of this type */
    int nw;       /* Cumulative number of words in block */
    float decay;
    float tau;

    /* The order ECC terms are loaded in each chip */
    Intpair order[N1][N2] = { {{0,0}, {1,0}, {2,0}, {0,3}}
              ,{{1,1}, {2,1}, {0,1}, {1,3}}
              ,{{2,2}, {0,2}, {1,2}, {2,3}}
#ifdef NVPSG
              ,{{3,3}, {3,3}, {3,3}, {3,3}}
#endif
                            };
    unsigned int hdr[] = {0x8,   /* Byte-wide physical data */
           0xe8,  /* 3 global control words */
           0xf10f8,
           0x310f8,
           DSPBUFLEN, /* Nbr of words to download */
           DSPADDR, /* DSP address of download */
           0x0};  /* Block control word */
    unsigned int tail[] = {0x0};

    rtn = (unsigned int *)malloc(sizeof(hdr)
             + DSPBUFLEN * sizeof(int)
             + sizeof(tail));
    if (!rtn){
        vnmremsg("Out of memory in make_tms_ecc_download()");
        return NULL;
    }

    /* Load header */
    for (i=0; i<sizeof(hdr)/sizeof(int); i++){
        rtn[i] = hdr[i];
    }
    pui = &rtn[i];      /* Next location to load */

    /* Load body of block; load amplitude and tau sections in parallel */
    for (i=nw=0; i<N2; i++) {
        src = order[axis][i].src;  /* Source axis */
        dst = order[axis][i].dst;  /* Dest axis */
        nt = ecc_matrix[src][dst].nterms;
        nw += nt * 2;
        if (nw > DSPBUFLEN){
            /* Block size too small to hold all the terms */
            fprintf(stderr,
                    "make_tms_ecc_download(): i=%d, nw=%d, DSPBUFLEN=%d\n",
                    i, nw, DSPBUFLEN);
            vnmremsg("Internal error in make_tms_ecc_download()");
            free(rtn);
            rtn = NULL;
            break;
        }
        for (j=0; j<nt; j++){
            tau = ecc_matrix[src][dst].tau[j];
            if (tau){
                decay = exp(-DECC_LOOPTIME / tau);
            }else{
                decay = 0;
            }
            pui[j] = float2tms32(ecc_matrix[src][dst].amp[j]);
            pui[j + DSPBUFLEN/2] = float2tms32(decay);
            /*fprintf(stderr,"*tau[%d][%d][%d]=%g (%g), amp=%g\n",
              src, dst, j,
              ecc_matrix[src][dst].tau[j], decay,
              ecc_matrix[src][dst].amp[j]);     */
        }
        pui += nt;     /* Next location to load */
    }

    /* Pad block if necessary; pad amplitude and tau sections in parallel */
    for (i=0 ; nw < DSPBUFLEN; i++, nw += 2){
        /*if (!i) {fprintf(stderr,"Pad: ");}     */
        /*fprintf(stderr,"%d ", i+1);     */
        pui[i] = float2tms32(0.0);
        pui[i + DSPBUFLEN/2] = float2tms32(0.0);
    }
    /*fprintf(stderr,"\n");     */
    pui += i + DSPBUFLEN/2;   /* Move pointer to end of tau section */

    /* Load trailer */
    for (i=0; rtn && i<sizeof(tail)/sizeof(int); i++){
        pui[i] = tail[i];
    }

    *nwords = (sizeof(hdr) / sizeof(hdr[0])
               + DSPBUFLEN
               + sizeof(tail) / sizeof(tail[0]));

    return rtn;
}

/*
 * Takes three DSP download blocks (one for each chip), and forms an APBOUT
 * string that loads all three chips.
 * Returns a pointer to the APBOUT string.  (The length of the string
 * is encoded into the string.)
 * NB: The caller must free the memory for the string.
 */
static codeint *
make_apbout_from_decc_dspcode(unsigned int *dspcode[], /* 3 DSP code lists */
               int nwords) /* # words to each register */
{
    int i, j;
    int outlen;         /* Number of codeints to output */
    int headlen;
    int taillen;
    int regorder[] = {3, 2, 1};  /* First download to first reg # in list */
    codeint head[] = {APBOUT,
                      0,                   /* Reserve for count-1 */
                      APSELECT | (DECC+0), /* Register 0 */
                      DECCWRITE | 0x00,    /* Reset FIFOs */
                      DECCWRITE | 0x40};   /* Enable FIFOs */
    codeint tail[] = {APSELECT | (DECC+0), /* Register 0 */
                      DECCWRITE | 0x47,    /* Interrupt all 3 DSPs */
                      DECCWRITE | 0x40};   /* DSPs to normal mode */
    codeint *pc;
    codeint *rtn;

    /* Allocate memory */
    headlen = sizeof(head) / sizeof(head[0]);
    taillen = sizeof(tail) / sizeof(tail[0]);
    outlen = headlen + N1 + N1 * nwords * 4 + taillen;
    rtn = (codeint *)malloc(outlen * sizeof(codeint));
    if (!rtn){
        vnmremsg("Out of memory in make_apbout_from_decc_dspcode()");
        return NULL;
    }

    /* Write header */
    pc = rtn;
    head[1] = outlen - 3;
    for (i=0; i<headlen; i++){
        *pc++ = head[i];
    }

    /* Write to the FIFOs. Words go in bytewise, little-endian */
    for (i=0; i<N1; i++){
        *pc++ = APSELECT | (DECC + regorder[i]);
        for (j=0; j<nwords; j++){
            *pc++ = DECCWRITE | (dspcode[i][j] & 0xff);
            *pc++ = DECCWRITE | ((dspcode[i][j] >> 8) & 0xff);
            *pc++ = DECCWRITE | ((dspcode[i][j] >> 16) & 0xff);
            *pc++ = DECCWRITE | ((dspcode[i][j] >> 24) & 0xff);
        }
    }

    /* Write tail */
    for (i=0; i<taillen; i++){
        *pc++ = tail[i];
    }

    return rtn;
}

/*
 * Given a file containing ECC values, construct an APBOUT string
 * that loads those values into the DECC board.
 */
static codeint *
make_apbout_from_decc_file(char *filename, Ecc ecc_matrix[N1][N2])
{
    int i;
    int err = FALSE;
    unsigned int *dspcode[N1];
    int nwords[N1];
    codeint *apb_string = NULL;

    /* Construct TMS32032 download blocks */
    for (i=0; i<N1; i++){
        dspcode[i] = make_tms_ecc_download(i, ecc_matrix, &nwords[i]);
    }
    if (!nwords[0] || nwords[0] != nwords[1] || nwords[1] != nwords[2]){
        fprintf(stderr,"make_apbout_from_decc_file(): nwords=%d, %d, %d\n",
                nwords[0], nwords[1], nwords[2]);
        vnmremsg("Internal error in make_apbout_from_decc_file()");
        err = TRUE;
    }

    /* Make up an APBOUT string */
    if (!err){
        apb_string = make_apbout_from_decc_dspcode(dspcode, nwords[0]);
    }

    /* Free memory */
    for (i=0; i<N1; i++){
        if (dspcode[i]){
            free(dspcode[i]);
        }
    }

    return apb_string;
}
#endif

#define INT64BIT 0x3FFFFFFFFFFFFFFFLL
#define INT32BIT 0x7FFFFFFF
#define INT16BIT 0x7FFF

#ifdef NVPSG
static void make_ecc_for_grad_cntl(Ecc ecc_matrix[N1][N2])
{
    Intpair   order[N1][N2] = {{{0,0}, {1,0}, {2,0}, {0,3}},
                               {{1,1}, {0,1}, {2,1}, {1,3}},
                               {{2,2}, {0,2}, {1,2}, {2,3}},
                               {{3,3}, {3,3}, {3,3}, {3,3}}
    };
    double    tmp1, tmpDouble;
    int       amps[60];
    int       i,j,k,n,nt,src,dst;
    long long times[60];

    n = 0;  // counter through amps and times array

    for (i=0; i<N1; i++)
    {   for (j=0; j<N2; j++)
        {
            if ( (i==3) && (j!=3) ) continue;
            src = order[i][j].src;
            dst = order[i][j].dst;
            nt = ecc_matrix[src][dst].nterms;
            for (k=0; k<nt; k++)
            {
                tmp1 = tmpDouble = ecc_matrix[src][dst].tau[k];
                if (tmpDouble>0.0){
                    tmpDouble = exp(-DECC_LOOPTIME / tmpDouble);
                }else{
                    tmpDouble = 0.0;
                }
                times[n] =
                    (long long)((long double)tmpDouble * (long double)INT64BIT);
                /* we scale all ampliftudes to +/-8192 at Phil's request.
                 * this keeps all within the FPGA in bound
                 */
                amps[n]  = (int)(ecc_matrix[src][dst].amp[k]*16383.0);
                n++;

            }
        }
    }
    putEccAmpsNTimes(n,amps,times);
}
#endif

#ifndef NVPSG
/*
 * Load an APBOUT string into the Acode stream
 */
static void
send_apbout_string(codeint *apb)
{
    int len;
    codeint *apend;

    if (apb){
        len = apb[1] + 1; /* Count word is (length - 1) */
        apend = apb + len + 2;  /* Include 2 hdr words (APBOUT + count) */
        while (apb < apend){
            /*fprintf(stderr,"0x%04x\n", (int)*apb & 0xffff);     */
            putcode(*apb++);
        }
        curfifocount += len;
    }
}
#endif

#ifdef NVPSG
static void
get_dutycycle_limits()
{
    char    tmp[MAXPATH];
    char    coilname[80];
    char   *k;
    FILE   *fd = NULL;
    struct _coilLimits *c;


    c = &(coilLimits);  // just so the code reads easier
    c->xduty = c->yduty = c->zduty = c->current  = 0.0;
    c->xrms  = c->yrms  = c->zrms  = c->flowrate = 0.0;
    c->tune  = 0;

    if (P_getstring(GLOBAL,"sysgcoil", coilname, 1, sizeof(coilname)) )
    {
        text_error("Cannot find the parameter sysgcoil");
        psg_abort(1);
    }
    if (strcmp(coilname,"None") == 0)  return;
    sprintf(tmp,"%s/imaging/gradtables/%s", systemdir,coilname);
    if (bugmask & ECCBUGBIT)
        printf("ECC: file=%s\n", tmp);

    fd = fopen(tmp, "r");
    if (fd == NULL)
    {
        text_error("cannot open %s=n", tmp);
        psg_abort(1);
    }

    while ( (k = fgets(tmp, sizeof(tmp), fd)) )
    {
        char *tok;
        tok = strtok(tmp, " \t\n");
        if (tok && *tok != '#')
        {
            if ( strncmp(tok,"RMS",  3) == 0)
            {  tok=strtok(NULL," \t\n");
                c->xrms = c->yrms = c->zrms = atof(tok);
            }
            if ( strncmp(tok,"XRMS", 4) == 0)
            {  tok=strtok(NULL," \t\n");
                c->xrms = atof(tok);
            }
            if ( strncmp(tok,"YRMS", 4) == 0)
            {  tok=strtok(NULL," \t\n");
                c->yrms = atof(tok);
            }
            if ( strncmp(tok,"ZRMS", 4) == 0)
            {  tok=strtok(NULL," \t\n");
                c->zrms = atof(tok);
            }
            if ( strncmp(tok,"current", 7) == 0)
            {  tok=strtok(NULL," \t\n");
                c->current = atof(tok);
            }
            if ( strncmp(tok,"flow", 4) == 0)
            {  tok=strtok(NULL," \t\n");
                c->flowrate = atof(tok);
            }
            if ( strncmp(tok,"tune", 4) == 0)
            {  tok=strtok(NULL," \t\n");
                c->tune = atof(tok);
            }
        }
    }
    if (c->current > 0.5)
    {
        c->xduty = c->xrms * c->xrms / (c->current * c->current);
        c->yduty = c->yrms * c->yrms / (c->current * c->current);
        c->zduty = c->zrms * c->zrms / (c->current * c->current);
    }
}

static void compare_dutycycle_limits()
{
    Sdac *pd;
    int  panic;

    if (option_check("danger") ) return;
    pd = sdac_values;
    panic = 0;
    if (coilLimits.xduty < pd[DUTYLIMIT].values[X1_AXIS])  panic=1;
    if (coilLimits.zduty < pd[DUTYLIMIT].values[Y1_AXIS])  panic=1;
    if (coilLimits.zduty < pd[DUTYLIMIT].values[Z1_AXIS])  panic=1;
    if (panic)
    {
        text_error("Dutycycle limits exceed gradient coil specifications");
        psg_abort(3);
    }
}
#endif

static int
get_ecc_filename(char *fname, int len)
{
    char *pc;
    char pfx[] = ".";
    int n;

    sprintf(fname,"%s/%s/%s", systemdir, DECCLIB, pfx);
    n = strlen(fname);
    pc = fname + n;
    len -= n;

    quoteToMainFlag=0;
    if (P_getstring(GLOBAL,"sysgcoil", pc, 1, len)
        || strlen(pc) == 0)
    {
        strcpy(pc, "main");
        quoteToMainFlag=1;
    }
    if (bugmask & ECCBUGBIT){
        printf("ECC: basename=%s, filepath=%s\n", pc, fname);
    }
    if ( strcmp(pc,"None")  == 0)
        return(FALSE);
    else
        return(TRUE);
}

static void
get_one_sdac_setting_from_file(FILE *fd, char *label, float values[4])
{
    char buf[512];
    int i;
    float x;
    // printf("label='%s'\n",label);
    rewind(fd);
    while (fgets(buf, sizeof(buf), fd)){
        if (strncasecmp(buf, label, strlen(label)) == 0){
            /* This line has the data */
            if ( (strcmp(label,"slewlimit") == 0) ||
                 (strcmp(label,"eccscale") == 0) )
            {
                sscanf(buf,"%*s %f %f %f %f", &values[0], &values[1],
                       &values[2], &values[3]);
                if (values[3] < 1e-6)
                {  abort_message("B0 limit or scale value is zero. Run 'decctool' as VnmrJ Admin to correct\n");
                }
                if (bugmask & ECCBUGBIT){
                    printf("%s %f %f %f %f\n", label, values[0], values[1],
                           values[2], values[3]);
                }
            }
            else
            {
                sscanf(buf,"%*s %f %f %f", &values[0], &values[1],
                       &values[2]);
                if (bugmask & ECCBUGBIT){
                    printf("%s %f %f %f\n", label, values[0], values[1],
                           values[2]);
                }
            }
            break;
        }
    }
    if (strcmp(label,"dutylimit") == 0) values[3] = 1.0;
    if (strcmp(label,"shimscale") == 0) {
        for (i=0; i<3; i++) {
            x=values[i];
            if ( (fabs(x-0.01)>1e-5) && (fabs(x-0.02)>1e-5) &&
                 (fabs(x-0.05)>1e-5) && (fabs(x-0.10)>1e-5) ) {
                abort_message("shimscale value is invalid. Run 'decctool' as VnmrJ Admin to correct\n");
            }
        }
    }
}

#ifndef NVPSG
static void
set_sdac_reg(int type, int index, int val)
{
    int addr;

    addr = sdac_values[type].funcadr + 4 * index;
    apset(addr, SDAC_ADRADR);
    apset(val, SDAC_DATAADR);
    apset(0, SDAC_ADRADR);
    if (bugmask & ECCBUGBIT){
        fprintf(stderr,"ECC: %s[%d]=%g: addr=0x%x, val=%d=0x%x\n",
                sdac_values[type].label, index, sdac_values[type].values[index],
                addr, val, val);
    }
}

static void
sdac_to_ap()
{
    int i,j;
    int polarity;
    Sdac *pd;
    for (i=0, pd=sdac_values; i<N_SDAC_VALS; pd++, i++){
        /* Set values over APbus */
        if (i == TOTALSCALE){
            /* Set appropriate polarity bits depending on signs */
            polarity = ((pd->values[2] >= 0)
                        | ((pd->values[1] >= 0) << 1)
                        | ((pd->values[0] >= 0) << 2));
            apset(POLARITY_ADR, SDAC_ADRADR);
            apset(polarity, SDAC_DATAADR);
            apset(0, SDAC_ADRADR);
            /*fprintf(stderr,"Polarity = 0x%x\n", polarity);     */
        }
        for (j=0; j<3; j++){
            set_sdac_reg(i, j, get_sdac_download_val(i, j));
        }
    }
}
#endif

/* fill in the Sdac sdac_value structure with sdac numbers */
static int
get_all_sdac_from_file(char *filename)
{
    int i, j;
    int rtn = TRUE;
    FILE *fd = NULL;
    Sdac *pd;
    struct stat statbuf;

    if (strstr(filename, "/.None")) {
        rtn = FALSE;
    } else if (stat(filename, &statbuf) == -1
               || ! (statbuf.st_mode & S_IFREG)
               || ! (fd = fopen(filename, "r") ))
    {
        text_error("Cannot find \"%s\". Check the sysgcoil parameter", filename);
        psg_abort(1);
    }


    if (!rtn){
        /* If no file, just turn off gain for safety */
        for (j=0, pd=sdac_values; j<3; j++){
            pd[TOTALSCALE].values[j] = 0;
        }
    }else
    {
        for (i=0, pd=sdac_values; i<N_SDAC_VALS; pd++, i++){
            for (j=0; j<4; j++){
                if (i == SLEWLIMIT) {
                    pd->values[j] = pd->max_val;
                }else{
                    pd->values[j] = 0;
                }
            }
            get_one_sdac_setting_from_file(fd, pd->label, pd->values);
        }
        fclose(fd);
    }

    return rtn;
}

#ifdef NVPSG

extern int gxFlip;  /* in psgmain.cpp */
extern int gyFlip;
extern int gzFlip;

int scale2bit(float value)
{
    value *= 100.0;
    if (value>7.5) value=3;
    else if (value>3.5) value=2;
    else if (value>1.5) value=1;
    else value=0;
    return(value);

}

static void sdac_to_grad_cntl()
{
    Sdac *pd;
    int   i,j;
    int   scaleBits;
    int   acodes[30];

    /*   for (i=0, pd=sdac_values; i<N_SDAC_VALS; pd++, i++){
     *            printf("%s: %f %f %f %f\n", pd->label, pd->values[0],
     *                        pd->values[1],pd->values[2],pd->values[3]);
     *        }
     */
    pd = sdac_values; // x,y,x=0.1,0.2, b0=10,20,50,100%
    scaleBits =   scale2bit(pd[ECCSCALE].values[X1_AXIS] );
    scaleBits +=  scale2bit(pd[ECCSCALE].values[Y1_AXIS] ) << 2;
    scaleBits +=  scale2bit(pd[ECCSCALE].values[Z1_AXIS] ) << 4;
    scaleBits +=  scale2bit(pd[ECCSCALE].values[B0_AXIS]/10.0 ) << 6;

    scaleBits += ( scale2bit)(pd[SHIMSCALE].values[X1_AXIS] ) << 8;
    scaleBits += ( scale2bit)(pd[SHIMSCALE].values[Y1_AXIS] ) << 10;
    scaleBits += ( scale2bit)(pd[SHIMSCALE].values[Z1_AXIS] ) << 12;

    i=0;
    acodes[i++]=8;
    acodes[i++] = scaleBits;
    /* corrected */
    /* sign flip handled in gradient Controller */
    j = (int)( pd[TOTALSCALE].values[X1_AXIS] * 65535);
    if (j < 0)
    {  gxFlip = -1;  j = -j;  }
    acodes[i++] = j;
    j = (int)( pd[TOTALSCALE].values[Y1_AXIS] * 65535);
    if (j < 0)
    {  gyFlip = -1;  j = -j;  }
    acodes[i++] = j;
    j = (int)( pd[TOTALSCALE].values[Z1_AXIS] * 65535);
    if (j < 0)
    {  gzFlip = -1;  j = -j;  }
    acodes[i++] = j;

    /* max rise in DAC units in 4 usec */
    acodes[i++] = (int)(4.0e-06 * INT16BIT / pd[SLEWLIMIT].values[X1_AXIS]) + 1;
    acodes[i++] = (int)(4.0e-06 * INT16BIT / pd[SLEWLIMIT].values[Y1_AXIS]) + 1;
    acodes[i++] = (int)(4.0e-06 * INT16BIT / pd[SLEWLIMIT].values[Z1_AXIS]) + 1;
    acodes[i++] = (int)(4.0e-06 * INT16BIT / pd[SLEWLIMIT].values[B0_AXIS]) + 1;


    putSdacScaleNLimits(9, acodes);
}

static void sdac_to_master_cntl()
{
    Sdac *pd;
    int   i;
    int   acodes[30];
    i=0;
    acodes[i++]=6;
    /* Duty Cycle */
    pd = sdac_values;
    acodes[i++] = (int)( pd[DUTYLIMIT].values[X1_AXIS] * INT16BIT );
    acodes[i++] = (int)( pd[DUTYLIMIT].values[Y1_AXIS] * INT16BIT );
    acodes[i++] = (int)( pd[DUTYLIMIT].values[Z1_AXIS] * INT16BIT );
    acodes[i++] = (int)( coilLimits.current + 0.5);   // straight from gradtable
    acodes[i++] = (int)( coilLimits.flowrate * 10);
    acodes[i++] = (int)( coilLimits.tune + 0.5);

    putDutyLimits(i, acodes);

}
#endif

/*
 * Load the ECC values in the master ECC file into the DECC board
 * by putting the values directly into the Acode stream.
 * If there is no master file, the overall gains on the SDAC board
 * are set to zero.
 * Returns TRUE on success, FALSE on failure (probably because of
 * no master file).
 *
 * For Nirvana the ECC and most of the SDAC values go to the Gradient
 * Controller. The DutyCycle values go to the Industry Pack II in the
 * ISI/ILI module
 * This means that both the master1 and grad1 access this routine for
 * their respective values.
 */

#ifdef NVPSG
extern int checkflag;
int download_master_decc_values(const char *cntlr)
#else
    int download_master_decc_values()
#endif
{
    char     filename[MAXPATH+1];
    int      rtn = TRUE;
    Ecc      ecc_matrix[N1][N2];
    struct stat statbuf;

#ifdef NVPSG
    if (checkflag)
    {
       char stmp[MAXSTR];

       stmp[0] = '\0';
       P_getstring(GLOBAL,"system",stmp,1,MAXSTR-1);
       if ( ! strcmp(stmp,"datastation") )
          return(rtn);
    }
#endif
    rtn = get_ecc_filename((char *)&filename, sizeof(filename));
    if (rtn)
    { if (stat(filename, &statbuf) == -1 || ! (statbuf.st_mode & S_IFREG) )
        {
            if (quoteToMainFlag==1)
                return(rtn);
            else
            {
                text_error("Cannot find \"%s\". Check the sysgcoil parameter", filename);
                psg_abort(1);
            }
        }
    }
    if ( deccFileRead == 0 )
    {
        /* Get the  SDAC values */
        rtn = get_all_sdac_from_file(filename);
        deccFileRead++;
#ifdef NVPSG
        if (rtn)
        {
            get_dutycycle_limits();
            compare_dutycycle_limits();
        }
#endif
    }
    /* convert for download if rtn is TRUE */
    if ( (deccFileRead>0) && rtn)
    {
#ifdef NVPSG
        if (strcmp(cntlr,"master1") == 0)
            sdac_to_master_cntl();   // master part
        else
            sdac_to_grad_cntl();     // gradient part
#else
        sdac_to_ap();
#endif
    }

    if ( (deccFileRead<2) && rtn)
    {
        /* Read the ECC values from master file */
        rtn = get_decc_from_file(filename, ecc_matrix);
        deccFileRead++;
        /* convert for download if rtn is TRUE */
        if (rtn)
        {
#ifdef NVPSG
            make_ecc_for_grad_cntl(ecc_matrix);
#else
            codeint *apbout;
            /* Set DECC board */
            apbout = make_apbout_from_decc_file(filename, ecc_matrix);
            send_apbout_string(apbout);
            if (apbout) {
                free(apbout);
                rtn = TRUE;
            }
#endif
        }
    }

    /* printf("download_master_decc_values(): Done\n");     */
    return rtn;
}

#ifdef ECC_INCLUDE_MAIN
/* this will need ecc_worstCase.c */
char systemdir[128];
int curfifocount;
int bugmask=1;

vnmremsg(strStr)
     char *strStr;
{
    printf("%s\n",strStr);
}

apset()
{
}

putcode()
{
}

P_getstring(int tree, char *name, char *buf, int index, int len)
{
    strcpy(buf,"Performa_1");
}

void putSdacScaleNLimits(int n,int *values)
{
    int i;
    printf("SDAC: n=%d\n",n);
    for (i=0; i<n; i++) ;
    //     printf("values[%2d]=%d\n",i,values[i]);
}

#ifdef NVPSG
void putEccAmpsNTimes(int n,int *amps,long long *times)
{
    int i;
    printf("ECC: n=%d\n",n);
    for (i=0; i<n; i++) ;
    //     printf("time[%2d]=%lld  amps[%2d]=%d\n",i,times[i],i,amps[i]);
}
#endif

main()
{
    strcpy( systemdir, "/vnmr");
    download_master_decc_values();
}
#endif
#endif

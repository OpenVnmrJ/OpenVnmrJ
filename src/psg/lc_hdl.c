/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#ifndef PSG_LC
#define PSG_LC
#endif
#include "lc.h"
#include "acodes.h"
#include "lc_index.h"


#define         AUDIO_CHAN_SELECT_MASK  0x000f
#define         OBS1_CHAN               0x0000
#define         OBS2_CHAN               0x0001
#define         LOCK_CHAN               0x0002
#define         TEST_CHAN               0x0003
/* STM APbus Control register defines from hardware.h in vwacq */
#define STM_AP_SINGLE_PRECISION  0x10   /* bit 4 - 1=16 bit, 0=32 bit */
#define STM_AP_ENABLE_ADC1	     0x20   /* bit 5 - Enable STM */
#define STM_AP_ENABLE_ADC2	     0x40   /* bit 6 - Enable STM */
#define STM_AP_ENABLE_STM	     0x80   /* bit 7 - Enable STM */
/* ADC APbus Control register defines from hardware.h in vwacq */
#define ADC_AP_ENABLE_RCV1_OVERLD 0x00010000 
#define ADC_AP_ENABLE_RCV2_OVERLD 0x00020000
#define ADC_AP_ENABLE_ADC_OVERLD  0x00040000
#define ADC_AP_ENABLE_CTC	  0x00100000  /* enable CTC & Fake CTC */
#define ADC_AP_ENABLE_APTO_DSP	  0x00200000  /* enable Apbus access to DSP regs */
#define ADC_AP_CHANSELECT_POS          24

unsigned long initial_adc();
unsigned long initial_dtm();

autodata    *Aauto;     /* pointer to automation structure in Acode set */
Acqparams   *Alc;	/* pointer to low core structure in Acode set */
codeint     *Aacode; 	/* pointer into the Acode array, also Start Address */
codeint     *lc_stadr;  /* Low Core Start Address */

Acqparams   Alc_addr;	/* pointer to low core structure in Acode set */

#define ACQSHORT codechar
#define ACQWORD codeint
#define BASE_ADD ((ACQWORD *) &(Alc_addr.acqnp))
#define VAR_ADD(var) ((ACQWORD *) &(Alc_addr.var))
#define ACQVARADDR(var) (ACQWORD) ((VAR_ADD(var) - BASE_ADD)/sizeof(ACQSHORT))
#define LACQVARADDR(var) (ACQWORD) ((VAR_ADD(var) - BASE_ADD)/sizeof(ACQSHORT) + 1)

/* --- Pulse Seq. globals --- */
#ifdef NO_ACQ
codeint npr_ptr;
codeint ntrt, ct, ctss, fidctr, HSlines_ptr, oph, bsval, bsctr;
codeint ssval, ssctr, nsp_ptr, sratert, rttmp, spare1rt, id2, id3, id4;
codeint zero, one, two, three;
codeint v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14;
codeint tablert, tpwrrt, dhprt, tphsrt, dphsrt, dlvlrt;
#else
/* codeint npr_ptr     = (ACQWORD)  ACQVARADDR(acqnp); */
codeint npr_ptr     = (ACQWORD)   0;
codeint ntrt        = (ACQWORD) LACQVARADDR(acqnt);
codeint ct          = (ACQWORD) LACQVARADDR(acqct);
codeint ctss        = (ACQWORD) LACQVARADDR(acqrtvptr);
codeint fidctr      = (ACQWORD) LACQVARADDR(acqelemid);
codeint HSlines_ptr = (ACQWORD) LACQVARADDR(acqsqui);
codeint oph         = (ACQWORD)  ACQVARADDR(acqoph);
codeint bsval       = (ACQWORD)  ACQVARADDR(acqbs);
codeint bsctr       = (ACQWORD)  ACQVARADDR(acqbsct);
codeint ssval       = (ACQWORD)  ACQVARADDR(acqss);
codeint ssctr       = (ACQWORD)  ACQVARADDR(acqssct);
codeint nsp_ptr     = (ACQWORD)  ACQVARADDR(acqocsr);
codeint	sratert     = (ACQWORD)  ACQVARADDR(acqsrate);
codeint rttmp       = (ACQWORD)  ACQVARADDR(acqrttmp);
codeint spare1rt    = (ACQWORD)  ACQVARADDR(acqspare1);
codeint id2         = (ACQWORD)  ACQVARADDR(acqid2);
codeint id3         = (ACQWORD)  ACQVARADDR(acqid3);
codeint id4         = (ACQWORD)  ACQVARADDR(acqid4);
codeint zero        = (ACQWORD)  ACQVARADDR(acqzero);
codeint one         = (ACQWORD)  ACQVARADDR(acqone);
codeint two         = (ACQWORD)  ACQVARADDR(acqtwo);
codeint three       = (ACQWORD)  ACQVARADDR(acqthree);
codeint v1          = (ACQWORD)  ACQVARADDR(acqv1);
codeint v2          = (ACQWORD)  ACQVARADDR(acqv2);
codeint v3          = (ACQWORD)  ACQVARADDR(acqv3);
codeint v4          = (ACQWORD)  ACQVARADDR(acqv4);
codeint v5          = (ACQWORD)  ACQVARADDR(acqv5);
codeint v6          = (ACQWORD)  ACQVARADDR(acqv6);
codeint v7          = (ACQWORD)  ACQVARADDR(acqv7);
codeint v8          = (ACQWORD)  ACQVARADDR(acqv8);
codeint v9          = (ACQWORD)  ACQVARADDR(acqv9);
codeint v10         = (ACQWORD)  ACQVARADDR(acqv10);
codeint v11         = (ACQWORD)  ACQVARADDR(acqv11);
codeint v12         = (ACQWORD)  ACQVARADDR(acqv12);
codeint v13         = (ACQWORD)  ACQVARADDR(acqv13);
codeint v14         = (ACQWORD)  ACQVARADDR(acqv14);
codeint tablert     = (ACQWORD)  ACQVARADDR(acqtablert);
codeint tpwrrt      = (ACQWORD)  ACQVARADDR(acqtpwrr);
codeint dhprt       = (ACQWORD)  ACQVARADDR(acqdpwrr);
codeint tphsrt      = (ACQWORD)  ACQVARADDR(acqtphsr);
codeint dphsrt      = (ACQWORD)  ACQVARADDR(acqdphsr);
codeint dlvlrt      = (ACQWORD)  ACQVARADDR(acqdlvlr);
#endif

#define MAXSTR 256

double relaxdelay;
double dtmmaxsum;

extern int bgflag;
extern int newacq;
extern int acqiflag;
extern int ra_flag;
extern char il[MAXSTR];	/* interleaved acquisition parameter, 'y','n' */


#define LOCKPOWER_I NNOISE
#define LOCKPHASE_I NACQXX
#define LOCKGAIN_I NEXACQT
#define LOCKZ0_I   XSAPBIO
#define RTINIT LOADF

long rt_tab[RT_TAB_SIZE];
int  rtinit_count;
codeint rt_alc_tab[RT_TAB_SIZE];
codeint dpfrt = DPFINDEX;
codeint arraydimrt = ARRAYDIMINDEX;
codeint relaxdelayrt = RLXDELAYINDEX;
codeint nfidbuf = NFIDBUFINDEX;
codeint rtcpflag = CPFINDEX;
codeint rtrecgain = RECGAININDEX;
codeint npnoise = NPNOISEINDEX;
codeint dtmcntrl = DTMCTRLINDEX;
codeint activercvrs = ACTIVERCVRINDEX;
codeint gindex = GTBLINDEX;
codeint adccntrl = ADCCTRLINDEX;
codeint scanflag = SCANFLAGINDEX;
codeint ilflagrt = ILFLAGRTINDEX;
codeint ilssval = ILSSINDEX;
codeint ilctss = ILCTSSINDEX;
codeint tmprt = TMPRTINDEX;
codeint acqiflagrt = ACQIFLAGRT;
codeint maxsum = MAXSUMINDEX;
codeint strt = STARTTINDEX;
codeint incrdelay = INCDELAYINDEX;
codeint endincrdelay = ENDINCDELAYINDEX;
codeint initflagrt = INITFLAGINDEX;
codeint v15 = V15INDEX;
codeint v16 = V16INDEX;
codeint v17 = V17INDEX;
codeint v18 = V18INDEX;
codeint v19 = V19INDEX;
codeint v20 = V20INDEX;
codeint v21 = V21INDEX;
codeint v22 = V22INDEX;
codeint v23 = V23INDEX;
codeint v24 = V24INDEX;
codeint clrbsflag = CLRBSFLAGINDEX;

get_rt_tab_elems()
{
   return(RT_TAB_SIZE);
}

static make_rt_table()
{
   int i;

   for (i=2; i<RT_TAB_SIZE; i++)
      rt_tab[i] = 0;
   rt_tab[0]   = LASTINDEX;
   rt_tab[1]   = RTVAR_BUFFER;

   rt_alc_tab[NPINDEX] = npr_ptr; npr_ptr     = NPINDEX;
   rt_alc_tab[NTINDEX] = ntrt; ntrt        = NTINDEX;
   rt_alc_tab[CTINDEX] = ct; ct          = CTINDEX;
   rt_alc_tab[CTSSINDEX] = ctss; ctss        = CTSSINDEX;
   rt_alc_tab[FIDINDEX] = fidctr; fidctr      = FIDINDEX;
   rt_alc_tab[HSINDEX] = HSlines_ptr; HSlines_ptr = HSINDEX;
   rt_alc_tab[OPHINDEX] = oph; oph         = OPHINDEX;
   rt_alc_tab[BSINDEX] = bsval; bsval       = BSINDEX;
   rt_alc_tab[BSCTINDEX] = bsctr; bsctr       = BSCTINDEX;
   rt_alc_tab[SSINDEX] = ssval; ssval       = SSINDEX;
   rt_alc_tab[SSCTINDEX] = ssctr; ssctr       = SSCTINDEX;
   rt_alc_tab[NSPINDEX] = nsp_ptr; nsp_ptr     = NSPINDEX;
   rt_alc_tab[SRATEINDEX] = sratert; sratert     = SRATEINDEX;
   rt_alc_tab[RTTMPINDEX] = rttmp; rttmp       = RTTMPINDEX;
   rt_alc_tab[SPARE1INDEX] = spare1rt; spare1rt    = SPARE1INDEX;
   rt_alc_tab[ID2INDEX] = id2; id2         = ID2INDEX;
   rt_alc_tab[ID3INDEX] = id3; id3         = ID3INDEX;
   rt_alc_tab[ID4INDEX] = id4; id4         = ID4INDEX;
   rt_alc_tab[ZEROINDEX] = zero; zero        = ZEROINDEX;
   rt_alc_tab[ONEINDEX] = one; one         = ONEINDEX;
   rt_alc_tab[TWOINDEX] = two; two         = TWOINDEX;
   rt_alc_tab[THREEINDEX] = three; three       = THREEINDEX;
   rt_alc_tab[V1INDEX] = v1; v1          = V1INDEX;
   rt_alc_tab[V2INDEX] = v2; v2          = V2INDEX;
   rt_alc_tab[V3INDEX] = v3; v3          = V3INDEX;
   rt_alc_tab[V4INDEX] = v4; v4          = V4INDEX;
   rt_alc_tab[V5INDEX] = v5; v5          = V5INDEX;
   rt_alc_tab[V6INDEX] = v6; v6          = V6INDEX;
   rt_alc_tab[V7INDEX] = v7; v7          = V7INDEX;
   rt_alc_tab[V8INDEX] = v8; v8          = V8INDEX;
   rt_alc_tab[V9INDEX] = v9; v9          = V9INDEX;
   rt_alc_tab[V10INDEX] = v10; v10         = V10INDEX;
   rt_alc_tab[V11INDEX] = v11; v11         = V11INDEX;
   rt_alc_tab[V12INDEX] = v12; v12         = V12INDEX;
   rt_alc_tab[V13INDEX] = v13; v13         = V13INDEX;
   rt_alc_tab[V14INDEX] = v14; v14         = V14INDEX;
   rt_alc_tab[RTTABINDEX] = tablert; tablert     = RTTABINDEX;
   rt_alc_tab[TPWRINDEX] = tpwrrt; tpwrrt      = TPWRINDEX;
   rt_alc_tab[DPWRINDEX] = dhprt; dhprt       = DPWRINDEX;
   rt_alc_tab[TPHSINDEX] = tphsrt; tphsrt      = TPHSINDEX;
   rt_alc_tab[DPHSINDEX] = dphsrt; dphsrt      = DPHSINDEX;
   rt_alc_tab[CLRBSFLAGINDEX] = clrbsflag; clrbsflag = CLRBSFLAGINDEX;
}

codeint *
init_acodes(Codes)
Acqparams *Codes;
{   
    /* Set up Acode pointers */
    Alc = (Acqparams *) Codes;	/* start of low core */
    lc_stadr = (codeint *) Codes;
    Aauto = (autodata *) (Alc + 1) ;/* start of auto struc */
    Aacode = (codeint *) (Aauto + 1);
    rtinit_count = 0;

    if (bgflag)
    {	fprintf(stderr,"Code address:  0x%lx \n",Codes);
     	fprintf(stderr,"Aacode address:  0x%lx \n",Aacode);
    }
    if (newacq)
       make_rt_table();
    return(Aacode);
}

set_lacqvar(index,val)
codeint index;
int val;
{
   codeint *ptr;
   codelong *ptr2;

   
   if (newacq)
   {
      rt_tab[index+TABOFFSET] = val;
      index = rt_alc_tab[index];
   }
   ptr = (codeint *)Alc;
   ptr = ptr + (index-1);
   ptr2 = (codelong *) ptr;
   *ptr2 = (codelong) val;
   if (bgflag)
   {
      fprintf(stderr,"index= %d val= %d \n",index,val);
      fprintf(stderr,"val = %d at address:  0x%lx \n",*ptr2, ptr2);
   }
}

set_acqvar(index,val)
codeint index;
int val;
{
   codeint *ptr;

   if (newacq)
   {
      rt_tab[index+TABOFFSET] = val;
      init_acqvar(index,val);
      index = rt_alc_tab[index];
   }
   ptr = (codeint *)Alc;
   ptr = ptr + index;
   *ptr = (codeint) val;
   if (bgflag)
   {
      fprintf(stderr,"index= %d val= %d \n",index,val);
      fprintf(stderr,"val = %d at address:  0x%lx \n",*ptr, ptr);
   }
}

init_acqvar(index,val)
codeint index;
int val;
{
   codeint *ptr;

   if (newacq)
   {
      putcode(RTINIT);
      putLongCode(val);
      putcode(index);
      rtinit_count++;
   }
}

init_acqvartab(index,val)
codeint index;
int val;
{
   codeint *ptr;

   if (newacq)
   {
      rt_tab[index+TABOFFSET] = val;
   }
}

get_acqvar(index)
codeint index;
{
   if (newacq)
   {
      return( rt_tab[index+TABOFFSET] );
   }
   else
   {
      codeint *ptr;
      codeint val;

      ptr = (codeint *)Alc;
      ptr = ptr + index;
      val = *ptr;
      return((int) val);
   }
}

std_lc_init()
{
    int i,words;
    codeint *lc_struc;

    words = sizeof(Acqparams) / sizeof(codeint);
    lc_struc = lc_stadr;
    for (i=0;i < words;i++)
	lc_struc[i] = 0;		/* zero Low Core parameters */

    Alc->acqstatus = 85; 		/* status    loc 19*/
    Alc->acqcheck = 64; 		/* check     loc 26*/
    Alc->acqoph = 0; 		/* oph       loc 27*/
   /* user variables*/
    Alc->acqid2 = (codeint) 0;
    Alc->acqid3 = (codeint) 0;
    Alc->acqid4 = (codeint) 0;
    Alc->acqzero = (codeint) 0;
    Alc->acqone = (codeint) 1;
    Alc->acqtwo = (codeint) 2;
    Alc->acqthree = (codeint) 3;
  /*  atten & phaseshift parameters used in su to initialize 500RF hardware.*/
    Alc->acqtpwrr = (codeint) 0;
    Alc->acqdpwrr = (codeint) 0;
    Alc->acqtphsr = (codeint) 0;
    Alc->acqdphsr = (codeint) 0;
    Alc->acqdlvlr = (codeint) 63;

    /* --- autostructure offset (words) into code section --- */
    Alc->acqo2auto = sizeof(Acqparams) / sizeof(codeint);
    if (bgflag)
	fprintf(stderr,"autostructure: offset: %d \n",Alc->acqo2auto);


    /* code offset*/
    Alc->acqcodeb = Alc->acqcodep = Aacode - (codeint *) Alc;
    if (bgflag)
    {
	fprintf(stderr,"acode offset addr = %lx, acode offset = %d \n",
		&(Alc->acqcodeb),Alc->acqcodeb);
        fprintf(stderr,"finish np: %ld, nt: %ld\n",Alc->acqnp,Alc->acqnt);
	fprintf(stderr,"npr_ptr offset: %d \n",npr_ptr);
        fprintf(stderr,"acodeb adr: %lx,acodep adr: %lx\n",
	      &(Alc->acqcodeb),&(Alc->acqcodep));
        fprintf(stderr,"np adr: %lx, nt adr: %lx\n",&(Alc->acqnp),&(Alc->acqnt));
        fprintf(stderr,"np: %ld, nt: %ld\n",Alc->acqnp,Alc->acqnt);
	fprintf(stderr,"ct offset: %d \n",ct);
	fprintf(stderr,"oph offset: %d \n",oph);
	fprintf(stderr,"bsctr offset: %d \n",bsctr);
        fprintf(stderr,"acqrtvptr set to : %d \n",Alc->acqrtvptr);
	fprintf(stderr,"squi_ptr offset: %d \n",HSlines_ptr);
	fprintf(stderr,"zero offset: %d \n",zero);
    }
    if (newacq)
    {
       rt_tab[oph+TABOFFSET] = 0;
       rt_tab[id2+TABOFFSET] = 0;
       rt_tab[id3+TABOFFSET] = 0;
       rt_tab[id4+TABOFFSET] = 0;
       rt_tab[zero+TABOFFSET] = 0;
       rt_tab[one+TABOFFSET] = 1;
       rt_tab[two+TABOFFSET] = 2;
       rt_tab[three+TABOFFSET] = 3;
       rt_tab[tpwrrt+TABOFFSET] = 0;
       rt_tab[dhprt+TABOFFSET] = 0;
       rt_tab[tphsrt+TABOFFSET] = 0;
       rt_tab[dphsrt+TABOFFSET] = 0;
       rt_tab[relaxdelayrt+TABOFFSET] = 0;	/* 0 millisecs */
       rt_tab[npnoise+TABOFFSET] = 256;
       for (i=0; i<MAX_STM_OBJECTS; i++)
	   rt_tab[dtmcntrl+TABOFFSET+i] = 0;
       rt_tab[activercvrs+TABOFFSET] = 1;
       rt_tab[acqiflagrt+TABOFFSET] = 0;
       rt_tab[initflagrt+TABOFFSET] = 0;
       rt_tab[ssval+TABOFFSET] = 0;
       rt_tab[strt+TABOFFSET] = 0;
       rt_tab[clrbsflag+TABOFFSET] = 0;

    }

}

custom_lc_init(val1,val2,val3,val4,val5,val6,val7,val8,val9,val10,val11,val12,val13,val14,val15,val16,val17,val18)
codeint val1;
codeulong val2;
codeint val3;
codeint val4;
codelong val5;
codelong val6;
codeint val7;
codeint val8;
codeint val9;
codeint val10;
codeint val11;
codeint val12;
codeint val13;
codeulong val14;
codeulong val15;
codelong val16;
codeint val17;
codeint val18;
{
    int i;
    int adcchan;

    Alc->acqidver =  (codeint) val1;
    Alc->acqelemid = (codeulong) val2;
    Alc->acqctctr =  (codeint) val3;
    Alc->acqdsize =  (codeint) val4;
    Alc->acqnp =     (codelong) val5;
    Alc->acqnt =     (codelong) val6;
    Alc->acqdpf =    (codeint) val7;
    Alc->acqbs =     (codeint) val8;
    Alc->acqbsct =   (codeint) val9;
    Alc->acqss =     (codeint) val10;
    Alc->acqasize =  (codeint) val11;
    Alc->acqcpf =    (codeint) val12;
    Alc->acqmaxconst = (codeint) val13;
    Alc->acqct =  (codelong) 0L;	/* U+ RA, ct had better be Zero */
    if (newacq)
    {
       Alc->acqct =  (codelong) val16;  /* moved here since U+ RA can't handle this */
       if (val2 > 0)
          rt_tab[fidctr+TABOFFSET] = val2;
       else 
          rt_tab[fidctr+TABOFFSET] = val2;
       rt_tab[npr_ptr+TABOFFSET] = val5;
       rt_tab[ntrt+TABOFFSET] = val6;
       rt_tab[dpfrt+TABOFFSET] = val7;
       rt_tab[bsval+TABOFFSET] = val8;
       rt_tab[bsctr+TABOFFSET] = val9;
       /* rt_tab[ssval+TABOFFSET] = val10; */
       rt_tab[CPFINDEX+TABOFFSET] = val12;
       rt_tab[MXSCLINDEX+TABOFFSET] = val13;
       rt_tab[arraydimrt+TABOFFSET] = val14;
       rt_tab[relaxdelayrt+TABOFFSET] = val15;	/* 0 millisecs */
       adcchan = val18 ? LOCK_CHAN : OBS1_CHAN;
       for (i=0; i<MAX_ADC_OBJECTS; i++)
	   rt_tab[adccntrl+TABOFFSET+i] = initial_adc(adcchan);
       for (i=0; i<MAX_STM_OBJECTS; i++)
	   rt_tab[dtmcntrl+TABOFFSET+i] = initial_dtm(val7);
       rt_tab[activercvrs+TABOFFSET] = parmToRcvrMask("rcvrs");
       rt_tab[ct+TABOFFSET] = val16;
       rt_tab[clrbsflag+TABOFFSET] = val17;


       /* calculate stm buffers from nt, bs, and arraydim, add noise 	*/
       /* buffer after calculation					*/
       if ((val8 > 0) && ((val6/val8) >= 1))
       {
	  if ( (val6%val8) > 0)
          	rt_tab[nfidbuf+TABOFFSET] = (((val6/val8)+1)*val14) + 1;
	  else
          	rt_tab[nfidbuf+TABOFFSET] = ((val6/val8)*val14) + 1;
       }
       else if (val14 > 1)
          rt_tab[nfidbuf+TABOFFSET] = val14 + 1;	/* add noise */
       else
          rt_tab[nfidbuf+TABOFFSET] = 1 + 1;	/* add noise */

       if ( acqiflag && (rt_tab[nfidbuf+TABOFFSET] < 3) )
          rt_tab[nfidbuf+TABOFFSET] = 3;

       /* For acqi set bsval to at least 1, n_hkeep() expects this	*/
       /* in convert.c							*/
       if (acqiflag && (rt_tab[bsval+TABOFFSET] < 1))
	  rt_tab[bsval+TABOFFSET] = 1;

       /* Init Interleaving, check for arraydim > 1 */
       if ( getIlFlag() && (val14 > 1))
	   rt_tab[ilflagrt+TABOFFSET] = 1;
       else
	   rt_tab[ilflagrt+TABOFFSET] = 0;

       /* Init maxsum  */
       rt_tab[maxsum+TABOFFSET] = (long)dtmmaxsum;	

    }
}

send_auto_pars()
{
    if (newacq)
    {
       putcode(LOCKPOWER_I);
       putcode(1);
       putcode(Aauto->lockpower);
       putcode(LOCKPHASE_I);
       putcode(1);
       putcode(Aauto->lockphase);
       putcode(LOCKGAIN_I);
       putcode(1);
       putcode(Aauto->lockgain);
       putcode(LOCKZ0_I);
       putcode(2);
       putcode(Aauto->control_mask >> 8);
       putcode(Aauto->z0);
    }
}

init_auto_pars(var1,var2,var3,var4,var5,var6,var7,var8)
codeint var1,var2,var3,var4,var5,var6,var7,var8;
{
    Aauto->lockpower = var1;
    Aauto->lockgain = var2;
    Aauto->lockphase = var3;
    Aauto->z0 = var4;
    Aauto->control_mask = var5;
    Aauto->when_mask = var6;
    Aauto->recgain = var7;
    Aauto->sample_mask = var8;
    if (newacq)
    {
      rt_tab[rtrecgain+TABOFFSET] = var7;
    }

    if (bgflag)
    {
       fprintf(stderr,"lockpower: %d \n",Aauto->lockpower);
       fprintf(stderr,"lockgain: %d \n",Aauto->lockgain);
       fprintf(stderr,"lockphase: %d \n",Aauto->lockphase);
       fprintf(stderr,"z0: %d \n",Aauto->z0);
       fprintf(stderr,"recgain: %d \n", (int) Aauto->recgain);
       fprintf(stderr,"sampleloc: %d \n", (int) Aauto->sample_mask);
    }
}

new_lcinit_arrayvars()
{
    extern int  gainactive;

    init_acqvar(fidctr,(long)Alc->acqelemid);
    init_acqvar(ct,(long)Alc->acqct);
    init_acqvar(ssval,(long)Alc->acqssct);
    init_acqvar(ctss,(long)Alc->acqrtvptr);
    init_acqvar(id2,(long)Alc->acqid2);
    init_acqvar(id3,(long)Alc->acqid3);
    init_acqvar(id4,(long)Alc->acqid4);
    if (!acqiflag)
    {
	init_acqvar(ntrt,(long)Alc->acqnt);
    }
    /* if gain is set to a value RTINIT it, if set to 'n' don'y
       so that prior FID will use the autogain value */
    if (gainactive && !acqiflag)
    {
      init_acqvar(rtrecgain,(long)Aauto->recgain);
    }

    if (!ra_flag && !getIlFlag() )
       init_acqvar(bsctr,(long)Alc->acqbsct);

}

set_nfidbuf()
{
	int ntval, nbsval, ndim, ntmax;

       ntval = get_acqvar(ntrt);
       nbsval = get_acqvar(bsval);
       ndim = get_acqvar(arraydimrt);
       /* calculate stm buffers from nt, bs, and arraydim, add noise 	*/
       /* buffer after calculation					*/
       
       ntmax = getmaxval("nt");
       if (ntmax > ntval)
	  ntval = ntmax;
       if ((nbsval > 0) && ((ntval/nbsval) >= 1))
       {
	  if ( (ntval%nbsval) > 0)
          	rt_tab[nfidbuf+TABOFFSET] = (((ntval/nbsval)+1)*ndim) + 1;
	  else
          	rt_tab[nfidbuf+TABOFFSET] = ((ntval/nbsval)*ndim) + 1;
       }
       else if (ndim > 1)
          rt_tab[nfidbuf+TABOFFSET] = ndim + 1;	/* add noise */
       else
          rt_tab[nfidbuf+TABOFFSET] = 1 + 1;	/* add noise */

       if ( acqiflag && (rt_tab[nfidbuf+TABOFFSET] < 3) )
          rt_tab[nfidbuf+TABOFFSET] = 3;

}

set_lockpower( val )
codeint val;
{
   Aauto->lockpower = val;
   if (newacq)
   {
      putcode(LOCKPOWER_I);
      putcode(1);
      putcode(val);
   }
}

set_lockphase( val )
codeint val;
{
   Aauto->lockphase = val;
   if (newacq)
   {
      putcode(LOCKPHASE_I);
      putcode(1);
      putcode(val);
   }
}

set_lockgain( val )
codeint val;
{
   Aauto->lockgain = val;
   if (newacq)
   {
      putcode(LOCKGAIN_I);
      putcode(1);
      putcode(val);
   }
}

set_recgain( val )
codeint val;
{
   Aauto->recgain = val;
}

/*----------------------------------------------------------------------*/
/* initial_adc								*/
/*	argument chan takes values 0-3.					*/
/*		0 - 1st receiver channel.				*/
/*		1 - 2nd receiver channel.				*/
/*		2 - lock channel.					*/
/*		3 - test channel.					*/
/*----------------------------------------------------------------------*/
unsigned long 
initial_adc(chan)
int chan;
{
 unsigned long adccontrol;
   adccontrol = (AUDIO_CHAN_SELECT_MASK & chan) << ADC_AP_CHANSELECT_POS;
   if (chan == 0)
	adccontrol = adccontrol | ADC_AP_ENABLE_RCV1_OVERLD;
   if (chan == 1)
	adccontrol = adccontrol | ADC_AP_ENABLE_RCV2_OVERLD;
   adccontrol = adccontrol | ADC_AP_ENABLE_ADC_OVERLD;
   adccontrol = adccontrol | ADC_AP_ENABLE_CTC;

   return(adccontrol);
}

unsigned long 
initial_dtm(precision)
codeint precision;
{
 unsigned long dtmcontrol;
   /* Enable adc and stm */
   dtmcontrol =  STM_AP_ENABLE_ADC1 | STM_AP_ENABLE_STM;

   /* set precision if single precision (2 bytes) */
   if (precision == 2)
   	dtmcontrol = dtmcontrol | STM_AP_SINGLE_PRECISION;

   return(dtmcontrol);
}

write_rtvar_acqi_file()
{
FILE	*fopen();
char    filename[MAXSTR];
char   *getenv();
FILE	*rtacqifile = 0;

   strcpy(filename,getenv("vnmrsystem") );
   strcat(filename,"/acqqueue/rtvars.IPA");
   rtacqifile=fopen(filename,"w");

   if (rtacqifile == 0)
   {
	text_error("Unable to write rtvar acqi file.");
	return;
   }

   write_rtvar(rtacqifile,"np",NPINDEX);
   write_rtvar(rtacqifile,"nt",NTINDEX);
   write_rtvar(rtacqifile,"ct",CTINDEX);
   write_rtvar(rtacqifile,"ctss",CTSSINDEX);
   write_rtvar(rtacqifile,"fidctr",FIDINDEX);
   write_rtvar(rtacqifile,"oph",OPHINDEX);
   write_rtvar(rtacqifile,"bsval",BSINDEX);
   write_rtvar(rtacqifile,"ssval",SSINDEX);
   write_rtvar(rtacqifile,"id2",ID2INDEX);
   write_rtvar(rtacqifile,"id3",ID3INDEX);
   write_rtvar(rtacqifile,"id4",ID4INDEX);
   write_rtvar(rtacqifile,"v1",V1INDEX);
   write_rtvar(rtacqifile,"v2",V2INDEX);
   write_rtvar(rtacqifile,"v3",V3INDEX);
   write_rtvar(rtacqifile,"v4",V4INDEX);
   write_rtvar(rtacqifile,"v5",V5INDEX);
   write_rtvar(rtacqifile,"v6",V6INDEX);
   write_rtvar(rtacqifile,"v7",V7INDEX);
   write_rtvar(rtacqifile,"v8",V8INDEX);
   write_rtvar(rtacqifile,"v9",V9INDEX);
   write_rtvar(rtacqifile,"v10",V10INDEX);
   write_rtvar(rtacqifile,"v11",V11INDEX);
   write_rtvar(rtacqifile,"v12",V12INDEX);
   write_rtvar(rtacqifile,"v13",V13INDEX);
   write_rtvar(rtacqifile,"v14",V14INDEX);
   write_rtvar(rtacqifile,"tablert",RTTABINDEX);
   write_rtvar(rtacqifile,"arraydimrt",ARRAYDIMINDEX);
   write_rtvar(rtacqifile,"relaxdelayrt",RLXDELAYINDEX);
   write_rtvar(rtacqifile,"rtcpflag",CPFINDEX);
   write_rtvar(rtacqifile,"rtrecgain",RECGAININDEX);
   write_rtvar(rtacqifile,"dtmcntrl",DTMCTRLINDEX);
   write_rtvar(rtacqifile,"adccntrl",ADCCTRLINDEX);
   write_rtvar(rtacqifile,"acqiflagrt",ACQIFLAGRT);

   if (rtacqifile) fclose(rtacqifile);
}

write_rtvar(fileid,rtname,rtindex)
FILE	*fileid;
char    *rtname;
int	rtindex;
{
   fprintf(fileid,"%-20s %10d %5d\n",rtname,rt_tab[rtindex+TABOFFSET],rtindex);
}

void setVs4dps(int start)
{
   v1 = start; v2 = start+1;  v3 = start+2; v4 = start+3; v5 = start+4;
   v6 = start+5; v7 = start+6;  v8 = start+7; v9 = start+8; v10 = start+9;
   v11 = start+10; v12 = start+11;  v13 = start+12; v14 = start+13;
   v15 = start+14; v16 = start+15;  v17 = start+16; v18 = start+17;
   v19 = start+18; v20 = start+19;  v21 = start+20; v22 = start+21;
   v23 = start+22; v24 = start+23;
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
#include <stdio.h>
#include <sys/types.h>
#include "lc_gem.h"
#include "acodes.h"
#include "lc_index.h"
#include "group.h"
#include "shrexpinfo.h"
#include "variables.h"
#include "abort.h"


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
#define BASE_ADD ((ACQWORD *) &(Alc_addr.np))
#define VAR_ADD(var) ((ACQWORD *) &(Alc_addr.var))
#define ACQVARADDR(var) (ACQWORD) ((VAR_ADD(var) - BASE_ADD)/sizeof(ACQSHORT))
#define LACQVARADDR(var) (ACQWORD) ((VAR_ADD(var) - BASE_ADD)/sizeof(ACQSHORT) + 1)

/* #define ACQVARADDR(var)  (short) (((int)&(Alc_addr.var) - (int)&(Alc_addr.np))/sizeof(char))
/* #define LACQVARADDR(var) (short) (((int)&(Alc_addr.var) - (int)&(Alc_addr.np))/sizeof(char) + 1)
/* */

/* --- Pulse Seq. globals --- */
#ifdef NO_ACQ
codeint npr_ptr;
codeint ntrt, ct, ctss, fidctr, HSlines_ptr, oph, bsval, bsctr;
codeint ssval, ssctr, nsp_ptr, sratert, rttmp, spare1rt, id2, id3, id4;
codeint zero, one, two, three;
codeint v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14;
codeint tablert, tpwrrt, dhprt, tphsrt, dphsrt, dlvlrt;
#else
/* codeint npr_ptr     = (ACQWORD)  ACQVARADDR(np); */
codeint npr_ptr     = (ACQWORD)   0;
codeint ntrt        = (ACQWORD) LACQVARADDR(nt);
codeint ct          = (ACQWORD) LACQVARADDR(ct);
codeint ctss        = (ACQWORD) LACQVARADDR(rtvptr);
codeint fidctr      = (ACQWORD) LACQVARADDR(elemid);
codeint HSlines_ptr = (ACQWORD) LACQVARADDR(squi);
codeint oph         = (ACQWORD)  ACQVARADDR(oph);
codeint bsval       = (ACQWORD)  ACQVARADDR(bs);
codeint bsctr       = (ACQWORD)  ACQVARADDR(bsct);
codeint ssval       = (ACQWORD)  ACQVARADDR(ss);
codeint ssctr       = (ACQWORD)  ACQVARADDR(ssct);
codeint nsp_ptr     = (ACQWORD)  ACQVARADDR(ocsr);
codeint	sratert     = (ACQWORD)  ACQVARADDR(srate);
codeint rttmp       = (ACQWORD)  ACQVARADDR(rttmp);
codeint spare1rt    = (ACQWORD)  ACQVARADDR(spare1);
codeint id2         = (ACQWORD)  ACQVARADDR(id2);
/* codeint id3         = (ACQWORD)  ACQVARADDR(id3); */
/* codeint id4         = (ACQWORD)  ACQVARADDR(id4); */
codeint zero        = (ACQWORD)  ACQVARADDR(zero);
codeint one         = (ACQWORD)  ACQVARADDR(one);
codeint two         = (ACQWORD)  ACQVARADDR(two);
codeint three       = (ACQWORD)  ACQVARADDR(three);
codeint v1          = (ACQWORD)  ACQVARADDR(v1);
codeint v2          = (ACQWORD)  ACQVARADDR(v2);
codeint v3          = (ACQWORD)  ACQVARADDR(v3);
codeint v4          = (ACQWORD)  ACQVARADDR(v4);
codeint v5          = (ACQWORD)  ACQVARADDR(v5);
codeint v6          = (ACQWORD)  ACQVARADDR(v6);
codeint v7          = (ACQWORD)  ACQVARADDR(v7);
codeint v8          = (ACQWORD)  ACQVARADDR(v8);
codeint v9          = (ACQWORD)  ACQVARADDR(v9);
codeint v10         = (ACQWORD)  ACQVARADDR(v10);
codeint v11         = (ACQWORD)  ACQVARADDR(v11);
codeint v12         = (ACQWORD)  ACQVARADDR(v12);
codeint v13         = (ACQWORD)  ACQVARADDR(v13);
codeint v14         = (ACQWORD)  ACQVARADDR(v14);
codeint tablert     = (ACQWORD)  ACQVARADDR(tablert);
/* codeint tpwrrt      = (ACQWORD)  ACQVARADDR(tpwrr); */
codeint dhprt       = (ACQWORD)  ACQVARADDR(dpwrr);
codeint tphsrt      = (ACQWORD)  ACQVARADDR(tphsr);
codeint dphsrt      = (ACQWORD)  ACQVARADDR(dphsr);
codeint dlvlrt      = (ACQWORD)  ACQVARADDR(dlvlr);
#endif

#define MAXSTR 256

double relaxdelay;
double dtmmaxsum=0x7F;

extern int bgflag;
extern int newacq;
extern int acqiflag;
extern int fidscanflag;
extern int ra_flag;
extern SHR_EXP_STRUCT ExpInfo;
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
/*   rt_alc_tab[ID3INDEX] = id3; id3         = ID3INDEX;
/*   rt_alc_tab[ID4INDEX] = id4; id4         = ID4INDEX; NOMERCURY */
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
/*    rt_alc_tab[TPWRINDEX] = tpwrrt; tpwrrt      = TPWRINDEX; NOMERCURY */
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
    if (newacq)
    {
       rt_tab[oph+TABOFFSET] = 0;
       rt_tab[id2+TABOFFSET] = 0;
/*       rt_tab[id3+TABOFFSET] = 0;
/*       rt_tab[id4+TABOFFSET] = 0; NOMERCURY */
       rt_tab[zero+TABOFFSET] = 0;
       rt_tab[one+TABOFFSET] = 1;
       rt_tab[two+TABOFFSET] = 2;
       rt_tab[three+TABOFFSET] = 3;
/*       rt_tab[tpwrrt+TABOFFSET] = 0; NOMERCURY */
       rt_tab[dhprt+TABOFFSET] = 0;
       rt_tab[tphsrt+TABOFFSET] = 0;
       rt_tab[dphsrt+TABOFFSET] = 0;
       rt_tab[relaxdelayrt+TABOFFSET] = 0;	/* 0 millisecs */
       rt_tab[npnoise+TABOFFSET] = 256;
       rt_tab[dtmcntrl+TABOFFSET] = 0;
       rt_tab[acqiflagrt+TABOFFSET] = 0;
       rt_tab[initflagrt+TABOFFSET] = 0;
       rt_tab[ssval+TABOFFSET] = 0;
       rt_tab[strt+TABOFFSET] = 0;
       rt_tab[clrbsflag+TABOFFSET] = 0;

    }

}

custom_lc_init(val1,val2,val3,val4,val5,val6,val7,val8,val9,val10,val11,val12,val13,val14,val15,val16,val17)
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
{
    Alc->id =  (char) (val1>>8);
    Alc->versn =  (char) (val1&0xff);
    Alc->elemid = (codeulong) val2;
    Alc->ctctr =  (codeint) val3;
    Alc->dsize =  (codeint) val4;
    Alc->np =     (codelong) val5;
    Alc->nt =     (codelong) val6;
    Alc->dpf =    (codeint) val7;
    Alc->bs =     (codeint) val8;
    Alc->bsct =   (codeint) val9;
    Alc->ss =     (codeint) val10;
    Alc->asize =  (codeint) val11;
    Alc->cpf =    (codeint) val12;
    Alc->maxconst = (codeint) val13;
    Alc->ct =  (codelong) 0L;	/* U+ RA, ct had better be Zero */
    if (newacq)
    {
       Alc->ct =  (codelong) val16;  /* moved here since U+ RA can't handle this */
       if (val2 > 0)
          rt_tab[fidctr+TABOFFSET] = val2;
       else 
          rt_tab[fidctr+TABOFFSET] = val2;
       rt_tab[npr_ptr+TABOFFSET] = val5;
       rt_tab[ntrt+TABOFFSET] = val6;
       rt_tab[dpfrt+TABOFFSET] = val7;
       rt_tab[bsval+TABOFFSET] = val8;
       rt_tab[bsctr+TABOFFSET] = val9;
/*       rt_tab[ssval+TABOFFSET] = val10; */
       rt_tab[CPFINDEX+TABOFFSET] = val12;
       rt_tab[MXSCLINDEX+TABOFFSET] = val13;
       rt_tab[arraydimrt+TABOFFSET] = val14;
       rt_tab[relaxdelayrt+TABOFFSET] = (fidscanflag) ? -1 : val15; /* 0 msec */
       rt_tab[adccntrl+TABOFFSET] = initial_adc(OBS1_CHAN);
       rt_tab[dtmcntrl+TABOFFSET] = initial_dtm(val7);
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
       if ( ExpInfo.IlFlag && (val14 > 1))
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
/*       putcode(LOCKPHASE_I);
/*       putcode(1);
/*       putcode(Aauto->lockphase); */
       putcode(LOCKGAIN_I);
       putcode(1);
       putcode(Aauto->lockgain);
       putcode(LOCKZ0_I);
       putcode(2);
       putcode(Aauto->control_mask >> 8);
       putcode(Aauto->coil_val[1]);
    }
}

init_auto_pars(var1,var2,var3,var4,var5,var6,var7,var8)
codeint var1,var2,var3,var4,var5,var6,var7,var8;
{
    Aauto->lockpower = var1;
    Aauto->lockgain = var2;
    Aauto->lockphase = var3;
    Aauto->coil_val[1] = var4;
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
       fprintf(stderr,"z0: %d \n",Aauto->coil_val[1]);
       fprintf(stderr,"recgain: %d \n", (int) Aauto->recgain);
       fprintf(stderr,"sampleloc: %d \n", (int) Aauto->sample_mask);
    }
}

new_lcinit_arrayvars()
{
    extern int  again;
    extern double nt;

    init_acqvar(fidctr,(long)Alc->elemid);
    init_acqvar(ct,(long)Alc->ct);
    init_acqvar(ssval,(long)Alc->ssct);
    init_acqvar(ctss,(long)Alc->rtvptr);
    init_acqvar(id2,(long)Alc->id2);
/*    init_acqvar(id3,(long)Alc->id3);
/*    init_acqvar(id4,(long)Alc->id4); NOMERCURY */
    if (!acqiflag)
    {
	init_acqvar(ntrt,(long)Alc->nt);
    }
    /* if gain is set to a value RTINIT it, if set to 'n' don't
       so that prior FID will use the autogain value */
    if (!again && !acqiflag)
    {
      init_acqvar(rtrecgain,(long)Aauto->recgain);
    }

    if (!ra_flag && !ExpInfo.IlFlag )
       init_acqvar(bsctr,(long)Alc->bsct);

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
/*  unsigned long dtmcontrol;
/*    /* Enable adc and stm */
/*    dtmcontrol =  STM_AP_ENABLE_ADC1 | STM_AP_ENABLE_STM;
/* 
/*    /* set precision if single precision (2 bytes) */
/*    if (precision == 2)
/*    	dtmcontrol = dtmcontrol | STM_AP_SINGLE_PRECISION;
/*  NOMERCURY */
   return( 0 );
}

/*-----------------------------------------------------------------
|       getmaxval()/1
|       Gets the maximum value of an arrayed or list real parameter. 
+------------------------------------------------------------------*/
getmaxval( parname )
char *parname;
{
    int      size,r,i,tmpval,maxval;
    double   dval;
    vInfo    varinfo;

    if (r = P_getVarInfo(CURRENT, parname, &varinfo)) {
        abort_message("getmaxval: could not find the parameter \"%s\"\n",parname);
    }
    if ((int)varinfo.basicType != 1) {
        abort_message("getmaxval: \"%s\" is not an array of reals.\n",parname);
    }

    size = (int)varinfo.size;
    maxval = 0;
    for (i=0; i<size; i++) {
        if ( P_getreal(CURRENT,parname,&dval,i+1) ) {
	    abort_message("getmaxval: problem getting array element %d.\n",i+1);
	}
	tmpval = (int)(dval+0.5);
	if (tmpval > maxval)	maxval = tmpval;
    }
    return(maxval);

}

void setVs4dps(int start)
{
   v1 = start; v2 = start+1;  v3 = start+2; v4 = start+3; v5 = start+4;
   v6 = start+5; v7 = start+6;  v8 = start+7; v9 = start+8; v10 = start+9;
   v11 = start+10; v12 = start+11;  v13 = start+12; v14 = start+13;
}

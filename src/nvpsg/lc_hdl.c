/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* lc_hdl.c */

#include <stdio.h>
#include "lc.h"
#include "ACode32.h"


extern double sign_add(double, double);
extern void broadcastCodes(int, int, int*);
extern int getIlFlag();


Acqparams   *Alc = 0;	/* pointer to low core structure in Acode set */
autodata    *Aauto;     /* pointer to automation structure in Acode set */
int     *Aacode; 	/* pointer into the Acode array, also Start Address */
int     *lc_stadr;  /* Low Core Start Address */


#define ACQWORD int
#define BASE_ADD ((ACQWORD *) &(Alc->np))
#define VAR_ADD(var) ((ACQWORD *) &(Alc->var))
#define ACQVARADDR(var) (ACQWORD) ((VAR_ADD(var) - BASE_ADD)) 

/* --- Pulse Seq. globals --- */
int npr_ptr;
int ntrt, ct, ctss, fidctr, HSlines_ptr, oph, bsval, bsctr;
int ssval, ssctr, nsp_ptr, sratert, rttmp, spare1rt, id2, id3, id4;
int zero, one, two, three;
int v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14;
int v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28;
int v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42;
int rtonce, vslice_ctr, vslices, virblock, vnirpulses;
int vir, virslice_ctr, vnir, vnir_ctr, vtest;
int vtabskip, vtabskip2;
int tablert , tpwrrt, dhprt, tphsrt, dphsrt, dlvlrt;
int res_hdec_cntr;
int res_hdec_lcnt;
int res_1_internal;
int res_2_internal;
int res_3_internal;
int res_4_internal;
int res_5_internal;
int res_6_internal;
int res_7_internal;
int res_8_internal;
int ilflagrt;
int ilssval;
int ilctss;
int strt;
int initflagrt;

#define MAXSTR 256

double relaxdelay;

extern int bgflag;
extern int acqiflag;
extern int ra_flag;
extern char il[MAXSTR];	/* interleaved acquisition parameter, 'y','n' */

int  rtinit_count;  /* referenced in arrayfuncs - otherwise unused */
/* these are probably left over - we may need them they are harmlessly init'd */
int vvar_init_flag[256];
int lc_end      = sizeof(Acqparams) / sizeof(int);
int VVARMAX     = sizeof(Acqparams) / sizeof(int);

static void initVvars()
{
   npr_ptr     = (ACQWORD)   0;
   ntrt        = (ACQWORD)  ACQVARADDR(nt);
   ct          = (ACQWORD)  ACQVARADDR(ct);
   ctss        = (ACQWORD)  ACQVARADDR(rtvptr);
   oph         = (ACQWORD)  ACQVARADDR(oph);
   bsval       = (ACQWORD)  ACQVARADDR(bs);
   bsctr       = (ACQWORD)  ACQVARADDR(bsct);
   ssval       = (ACQWORD)  ACQVARADDR(ss);
   ssctr       = (ACQWORD)  ACQVARADDR(ssct);

   rttmp       = (ACQWORD)  ACQVARADDR(rttmp);
   spare1rt    = (ACQWORD)  ACQVARADDR(spare1);
   id2         = (ACQWORD)  ACQVARADDR(id2);
   id3         = (ACQWORD)  ACQVARADDR(id3);
   id4         = (ACQWORD)  ACQVARADDR(id4);
   zero        = (ACQWORD)  ACQVARADDR(zero);
   one         = (ACQWORD)  ACQVARADDR(one);
   two         = (ACQWORD)  ACQVARADDR(two);
   three       = (ACQWORD)  ACQVARADDR(three);

   v1          = (ACQWORD)  ACQVARADDR(v1);
   v2          = (ACQWORD)  ACQVARADDR(v2);
   v3          = (ACQWORD)  ACQVARADDR(v3);
   v4          = (ACQWORD)  ACQVARADDR(v4);
   v5          = (ACQWORD)  ACQVARADDR(v5);
   v6          = (ACQWORD)  ACQVARADDR(v6);
   v7          = (ACQWORD)  ACQVARADDR(v7);
   v8          = (ACQWORD)  ACQVARADDR(v8);
   v9          = (ACQWORD)  ACQVARADDR(v9);
   v10         = (ACQWORD)  ACQVARADDR(v10);

   v11         = (ACQWORD)  ACQVARADDR(v11);
   v12         = (ACQWORD)  ACQVARADDR(v12);
   v13         = (ACQWORD)  ACQVARADDR(v13);
   v14         = (ACQWORD)  ACQVARADDR(v14);
   v15         = (ACQWORD)  ACQVARADDR(v15);
   v16         = (ACQWORD)  ACQVARADDR(v16);
   v17         = (ACQWORD)  ACQVARADDR(v17);
   v18         = (ACQWORD)  ACQVARADDR(v18);
   v19         = (ACQWORD)  ACQVARADDR(v19);

   v20         = (ACQWORD)  ACQVARADDR(v20);
   v21         = (ACQWORD)  ACQVARADDR(v21);
   v22         = (ACQWORD)  ACQVARADDR(v22);
   v23         = (ACQWORD)  ACQVARADDR(v23);
   v24         = (ACQWORD)  ACQVARADDR(v24);
   v25         = (ACQWORD)  ACQVARADDR(v25);
   v26         = (ACQWORD)  ACQVARADDR(v26);
   v27         = (ACQWORD)  ACQVARADDR(v27);
   v28         = (ACQWORD)  ACQVARADDR(v28);
   v29         = (ACQWORD)  ACQVARADDR(v29);

   v30         = (ACQWORD)  ACQVARADDR(v30);
   v31         = (ACQWORD)  ACQVARADDR(v31);
   v32         = (ACQWORD)  ACQVARADDR(v32);
       v33     = (ACQWORD)  ACQVARADDR(v33);
       v34     = (ACQWORD)  ACQVARADDR(v34);
       v35     = (ACQWORD)  ACQVARADDR(v35);
       v36     = (ACQWORD)  ACQVARADDR(v36);
       v37     = (ACQWORD)  ACQVARADDR(v37);
       v38     = (ACQWORD)  ACQVARADDR(v38);
       v39     = (ACQWORD)  ACQVARADDR(v39);
       v40     = (ACQWORD)  ACQVARADDR(v40);
       v41     = (ACQWORD)  ACQVARADDR(v41);
       v42     = (ACQWORD)  ACQVARADDR(v42);
       res_hdec_cntr         = (ACQWORD)  ACQVARADDR(res_hdec_cntr);
       res_hdec_lcnt         = (ACQWORD)  ACQVARADDR(res_hdec_lcnt);
       res_1_internal        = (ACQWORD)  ACQVARADDR(res_1_internal); 
       res_2_internal        = (ACQWORD)  ACQVARADDR(res_2_internal); 
       rtonce                = (ACQWORD)  ACQVARADDR(rtonce);
       res_4_internal        = (ACQWORD)  ACQVARADDR(res_4_internal);
       res_5_internal        = (ACQWORD)  ACQVARADDR(res_5_internal);
       res_6_internal        = (ACQWORD)  ACQVARADDR(res_6_internal);
       res_7_internal        = (ACQWORD)  ACQVARADDR(res_7_internal);
       res_8_internal        = (ACQWORD)  ACQVARADDR(res_8_internal);
/* 9 reserved rtvars for imaging IR module */
       vslice_ctr            = (ACQWORD)  ACQVARADDR(vslice_ctr);   /* imaging IR module */
       vslices               = (ACQWORD)  ACQVARADDR(vslices);      /* imaging IR module */
       virblock              = (ACQWORD)  ACQVARADDR(virblock);     /* imaging IR module */
       vnirpulses            = (ACQWORD)  ACQVARADDR(vnirpulses);   /* imaging IR module */
       vir                   = (ACQWORD)  ACQVARADDR(vir);          /* imaging IR module */
       virslice_ctr          = (ACQWORD)  ACQVARADDR(virslice_ctr); /* imaging IR module */
       vnir                  = (ACQWORD)  ACQVARADDR(vnir);         /* imaging IR module */
       vnir_ctr              = (ACQWORD)  ACQVARADDR(vnir_ctr);     /* imaging IR module */
       vtest                 = (ACQWORD)  ACQVARADDR(vtest);        /* imaging IR module */
       vtabskip              = (ACQWORD)  ACQVARADDR(vtabskip);     /* imaging peloop var */
       vtabskip2             = (ACQWORD)  ACQVARADDR(vtabskip2);    /* imaging peloop2 var */
   tablert     = (ACQWORD)  ACQVARADDR(tablert);
   ilflagrt = (ACQWORD)  ACQVARADDR(spare1);
   ilssval =  (ACQWORD)  ACQVARADDR(spare1);
   ilctss =   (ACQWORD)  ACQVARADDR(spare1);
   strt =     (ACQWORD)  ACQVARADDR(spare1);
   initflagrt =  (ACQWORD)  ACQVARADDR(spare1);
}


int *init_acodes(Acqparams *Codes)
{   
    /* Set up Acode pointers */
    Alc = (Acqparams *) Codes;	/* start of low core */
    lc_stadr = (int *) Codes;
    Aauto = (autodata *) (Alc + 1) ;/* start of auto struc */
    Aacode = (int *) (Aauto + 1);
    initVvars();
    rtinit_count = 0;
    return(Aacode);
}
/****************************************************
force RT INIT 
always do initval not just first/block size 
used for PSG internals specifically loop initialiers
*****************************************************/
void F_initval(double value, int index)
{
   int code[2];
   int *ptr;
   /* clip */ 
   if (value > 2147483647.0)
     value = 2147483647.0;
   if (value < -2147483648.0)
     value = -2147483648.0;
   code[0] = (int) (value + 0.5);
   code[1] = index;
   rtinit_count++;  
   broadcastCodes(FRTINIT,2,code);

/* write into LC, so that value is available at runtime */
   ptr = (int *)Alc;
   ptr = ptr + index;
   *ptr = (int) (value + 0.5);
   vvar_init_flag[index] = 1;
   if (bgflag)
      fprintf(stderr,"index= %d value= %g \n",index,value);
}
   
void init_acqvar(int index,int val)
{
   int codearray[2];

   if ((index >= 0) && (index < lc_end))
   {
      codearray[0]=val; codearray[1]=index;
      broadcastCodes(RTINIT,2,codearray);
      rtinit_count++;
   }
   else
   {
      if (bgflag)
         fprintf(stderr,"RTINIT called with unsupported index %d\n",index);
   }
}

/* causes an RTINIT, records in Alc - */
void set_acqvar(int index,int val)
{
   int *ptr;
   init_acqvar(index,val);
   ptr = (int *)Alc;
   ptr = ptr + index;
   *ptr = (int) val;
   vvar_init_flag[index] = 1;
   if (bgflag)
   {
      fprintf(stderr,"index= %d val= %d \n",index,val);
   }
}

int get_acqvar(int index)
{
   int *ptr;
   if (vvar_init_flag[index] == 1)
   {
     ptr = (int *)Alc;
     ptr = ptr + index;
     return (*ptr);
   }
   else
     return -1;
}

void initialize_vvar_init_flag()
{
  int i;
  for (i=0; i<256; i++)
    vvar_init_flag[i] = 0;
}

void std_lc_init()
{
    int i,words;
    int *lc_struc;

    words = sizeof(Acqparams) / sizeof(int);

    lc_struc = lc_stadr;

    for (i=0;i < words;i++)
	lc_struc[i] = 0;		/* zero Low Core parameters */


    Alc->oph = 0; 		/* oph       loc 27*/
    /* user variables*/
    Alc->id2 = (int) 0;
    Alc->id3 = (int) 0;
    Alc->id4 = (int) 0;
    Alc->zero = (int) 0;
    Alc->one = (int) 1;
    Alc->two = (int) 2;
    Alc->three = (int) 3;

    /* --- autostructure offset (words) into code section --- */
    Alc->o2auto = sizeof(Acqparams) / sizeof(int);
    if (bgflag)
	fprintf(stderr,"autostructure: offset: %d \n",Alc->o2auto);

    initialize_vvar_init_flag();   /* initialize the v-variable initialization flags */

    /* code offset*/
    /* Alc->codeb = Alc->codep = Aacode - (int *) Alc; */
    if (bgflag)
    {
        fprintf(stderr,"finish np: %d, nt: %d\n",Alc->np,Alc->nt);
	fprintf(stderr,"npr_ptr offset: %d \n",npr_ptr);
        fprintf(stderr,"np: %d, nt: %d\n",Alc->np,Alc->nt);
	fprintf(stderr,"ct offset: %d \n",ct);
	fprintf(stderr,"oph offset: %d \n",oph);
	fprintf(stderr,"bsctr offset: %d \n",bsctr);
        fprintf(stderr,"rtvptr set to : %d \n",Alc->rtvptr);
	fprintf(stderr,"zero offset: %d \n",zero);
    }
}

void custom_lc_init(int val1, int val2, int val3, int val4,
                    int val5, int val6, int val7, int val8,
                    int val9, int val10, int val11, int val12,
                    int val13, int val14, int val15, int val16,
                    int val17, int val18)
{
    Alc->idver =  (int) val1;
    Alc->np =     (int) val5;
    Alc->nt =     (int) val6;
    Alc->dpf =    (int) val7;
    Alc->bs =     (int) val8;
    Alc->bsct =   (int) val9;
    Alc->ss =     (int) val10;
    Alc->cpf =    (int) val12;
    Alc->cfct = 0;
    Alc->il_incr = 0;
    Alc->il_incrBsCnt = 0;  
    Alc->ra_fidnum = 0;
    Alc->ra_ctnum = 0;   
    Alc->ct =  (int) val16;

}

/*-------------------------------------------------------------------
|
|       G_initval()/2
|       initialize a real time variable v1-v14, (contain within the Acq code)
|       The double real is rounded up and made integer.
|       initval() moved to macros.h for typeless arguments.
|
+------------------------------------------------------------------*/
/* value           value to set variable to */
/* index           offset into acq code of variable */
void G_initval(double value, int index)
{
   value = (value > 0.0) ? (value + 0.5) : (value - 0.5); /* round up value */
   if (bgflag)
       fprintf(stderr,"initval(): value: %8.1lf , index: %d \n",value,index);
   /* --- force value to be in limits of integer value --- */
        if (value > 2147483647.0)
           value = 2147483647.0;
        else
           if (value < -2147483648.0)
                value = -2147483648.0;

   set_acqvar(index,(int) sign_add(value,0.0005) );
}


/* these are unused - we may need them*/ 
void init_auto_pars(int var1,int var2,int var3,int var4,int var5,
                    int var6,int var7,int var8)
{
    Aauto->lockpower = 0x21839;
    Aauto->lockgain =  0x21832;
    Aauto->lockphase = 0x21833;
    Aauto->z0 =        0x21839;
    Aauto->control_mask = 0x21839;
    Aauto->when_mask = 0x21839;
    Aauto->recgain = 0x21839;
    Aauto->sample_mask = 0x21839;
}

void new_lcinit_arrayvars()
{
    /*    init_acqvar(fidctr,(int)Alc->elemid); */

    init_acqvar(ctss,(int)Alc->rtvptr);

    if (!ra_flag && !getIlFlag() )
       init_acqvar(bsctr,(int)Alc->bsct);
}


void setVs4dps(int start)
{
   v1 = start;   v2 = start+1;  v3 = start+2; v4 = start+3; v5 = start+4;
   v6 = start+5; v7 = start+6;  v8 = start+7; v9 = start+8; v10 = start+9;
   v11 = start+10; v12 = start+11;  v13 = start+12; v14 = start+13;
   v15 = start+14; v16 = start+15;  v17 = start+16; v18 = start+17;
   v19 = start+18; v20 = start+19;  v21 = start+20; v22 = start+21;
   v23 = start+22; v24 = start+23;  v25 = start+24; v26 = start+25;
   v27 = start+26; v28 = start+27;  v29 = start+28; v30 = start+29;
   v31 = start+30; v32 = start+31;  v33 = start+32; v34 = start+33;
   v35 = start+34; v36 = start+35;  v37 = start+36; v38 = start+37;
   v39 = start+38; v40 = start+39;  v41 = start+40; v42 = start+41;
   res_hdec_cntr = v42 + 1;
   res_hdec_lcnt = v42 + 2;
   res_1_internal = v42 + 3;
   res_2_internal = v42 + 4;
   rtonce = v42 + 5;
   res_4_internal = v42 + 6;
   res_5_internal = v42 + 7;
   res_6_internal = v42 + 8;
   res_7_internal = v42 + 9;
   res_8_internal = v42 + 10;
   vslice_ctr = start+52; vslices = start+53; virblock = start+54;
   vnirpulses = start+55; vir = start+56; virslice_ctr = start+57;
   vnir = start+58; vnir_ctr = start+59; vtest = start+60;
   vtabskip = start+61; vtabskip2 = start+62;
}


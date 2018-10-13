#ifndef WDUMBOWSH_H
#define WDUMBOWSH_H

//WPMSEQ -            a container for shapes in the loop.
//	              The shapes can be defined as SHAPE or MPSEQ
//getwdumbogen1 -     get function for wDUMBO computed from form coefficients,
//		      uses MPSEQ to store shape
//getwdumbogen2 -     the same as above, but stores DUMBO in SHAPE
//_wdumbo1 -          runs wDUMBO, created in getwdumbogen1, can be XmX
//_wdumbo2 - 	      runs wDUMBO, created in getwdumbogen2, can be XmX
//getwpmlgxmx1 -      get function for wPMLGxmx
//_wpmlg1 -           runs wPMLGxmx, created in getwpmlgxmx1
//getwsamn1 - 	      get function for wSAMn, stores shape in MPSEQ
//_wsamn1 -	      underscore function for wSAMn



//structure for windowed pulse sequence which use shapes in loops

typedef struct {
   union {
	MPSEQ mpseq;        //mpseq container
	SHAPE shape;        //shape container
   } wvsh; 		    //only one type of shape can be stored
   double   npa;            //200 ns steps during each acquire()
   double   npa1;           //200 ns steps for extra acquire()
   double   npa2;           //200 ns steps for extra acquire()
   double   r1;             //prepulse delay
   double   r2;             //postpulse delay
   double   r3;             //rof3 or 2.0us by default
   double   tau;            //small-tau, (i.e.) {pw + delay}
   double   rtau;           //tau obtained from 1/srate
   double   dtau;           //dummy acquisition delay
   double   dtaua;          //acquisiton delay
   double   t1;             //extra delay t1
   double   t2;             //extra delay t2
   double   cycles;         //number of MP cycles in a main loop
   double   cycles1;        //number of MP cycles in a nested loop
   int   vcycles;           //RT v-pointer for the number of cycles
   int   vcount;            //RT v-pointer for the cycles loop counter
   int   vphase;            //RT v-pointer for the total phase
   int   istep;             //RT v-pointer for index of MP phase cycle
   int   vstep;             //RT v-pointer for the MP phase cycle
   int   tstep;             //table-pointer of the MP phase cycle
   int	 ofph;		    //phase increment to correct the offset
   int   va;                //RT v-pointer a
   int   vb;                //RT v-pointer b
   int   vc;                //RT v-pointer c
   int   vd;                //RT v-pointer d   
   int   ta;                //RT table-pointer a
   int   tb;                //RT table-pointer b
   int   tc;                //RT table-pointer c
   int   td;                //RT table-pointer d
   double apdelay;    
   double strtdelay;   
   double offstdelay;
} WMPSEQ;




WMPSEQ getwdumbogen1(char *seqName, char *coeffName)

//  DUMBO-1 experimental description A.Lesage et al., JMR, 163 (2003) 105
//  eDumbo method of optimisation G. de Paepe, Chem.Phys.Lett., 376 (2003) 259
//  Fourier coefficients from http://www.ens-lyon.fr/CHEMIE/Fr/Groupes/NMR/Pages/library.html
//  Z-Rotation Supercycle: M.Leskes, Chem. Phys. Lett., 466 (2008) 95

{
   WMPSEQ mp;
   mp.wvsh.mpseq.nelem=1; //waveform should have a single PMLG element
   mp.wvsh.mpseq=getdumbogen(seqName, coeffName,0,0,0,0,1);

   //printf("dumbo dur: %f\n",mp.wvsh.mpseq.t);
   char *var;
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("getwdumbogen() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.wvsh.mpseq.ch,sizeof(mp.wvsh.mpseq.ch));

// aXsuffix

   var = getname0("a",mp.wvsh.mpseq.seqName,"");
   mp.wvsh.mpseq.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.wvsh.mpseq.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.wvsh.mpseq.seqName,"");
   mp.r1 = getval(var); 

// r2Xsuffix

   var = getname0("r2",mp.wvsh.mpseq.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.wvsh.mpseq.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.wvsh.mpseq.seqName,"");
   mp.tau = getval(var);
   
// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.wvsh.mpseq.t - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwdumbogen Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }


// Set the number of DUMBO Cycles and create V-vars for the main loop

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) 2.0*((int) (np/(mp.tau*4.0*sw)));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

//Calculate phase increments
   double obsstep=360.0/(8192*PSD);
   mp.va = setvvarname();
   mp.vb = setvvarname();
   mp.vc = setvvarname();

//mp.va stores the accumulated phase for each PMLG block
   initval(0.0, mp.va);
//mp.vb stores the 180.0 shift needed for xmx

   int supercyc;
   var = getname0("sc",coeffName,"");
   supercyc=getval(var);

   if (supercyc)
	initval(roundoff(180.0/obsstep,1),mp.vb);
   else
	initval(0.0,mp.vb);
//mp.vc stores 360 degrees for removing the complete phase
   initval(roundoff(360.0/obsstep,1),mp.vc);
   

   double phase_increment=roundphase(mp.wvsh.mpseq.of[0]*(mp.tau)*360.0,obsstep);
   int ph_inc_int=roundoff(phase_increment/obsstep,1);

//mp.ofph stores the phase accumulation due to offset
   mp.ofph = setvvarname();
   initval(ph_inc_int,mp.ofph);

// Set mp.va and initialize 2048 steps (90 degrees)

//   mp.va = setvvarname();
//   initval(PSD*2048.0, mp.va);

   return mp;
}

WMPSEQ getwdumbogen2(char *seqName, char *coeffName)

//  DUMBO-1 experimental description A.Lesage et al., JMR, 163 (2003) 105
//  eDumbo method of optimisation G. de Paepe, Chem.Phys.Lett., 376 (2003) 259
//  Fourier coefficients from http://www.ens-lyon.fr/CHEMIE/Fr/Groupes/NMR/Pages/library.html
//  Z-Rotation Supercycle: M.Leskes, Chem. Phys. Lett., 466 (2008) 95

{
   WMPSEQ mp;
   mp.wvsh.shape.pars.nelem=1; //waveform should have a single DUMBO element
   mp.wvsh.shape=getdumbogenshp(seqName, coeffName, 0.0,0.0,0,1);

   //printf("dumbo dur: %f\n",mp.wvsh.mpseq.t);
   char *var;
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("getwdumbogen() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.wvsh.shape.pars.ch,sizeof(mp.wvsh.shape.pars.ch));

// aXsuffix

   var = getname0("a",mp.wvsh.shape.pars.seqName,"");
   mp.wvsh.shape.pars.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.wvsh.shape.pars.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.wvsh.shape.pars.seqName,"");
   mp.r1 = getval(var); 

// r2Xsuffix

   var = getname0("r2",mp.wvsh.shape.pars.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.wvsh.shape.pars.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.wvsh.shape.pars.seqName,"");
   mp.tau = getval(var);
   
// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.wvsh.shape.pars.t - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwdumbogen Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }


// Set the number of DUMBO Cycles and create V-vars for the main loop

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) 2.0*((int) (np/(mp.tau*4.0*sw)));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

//Calculate phase increments
   double obsstep=360.0/(8192*PSD);
   mp.va = setvvarname();
   mp.vb = setvvarname();
   mp.vc = setvvarname();

//mp.va stores the accumulated phase for each PMLG block
   initval(0.0, mp.va);
//mp.vb stores the 180.0 shift needed for xmx

   int supercyc;
   var = getname0("sc",coeffName,"");
   supercyc=getval(var);

   if (supercyc)
	initval(roundoff(180.0/obsstep,1),mp.vb);
   else
	initval(0.0,mp.vb);
//mp.vc stores 360 degrees for removing the complete phase
   initval(roundoff(360.0/obsstep,1),mp.vc);
   

   double phase_increment=roundphase(mp.wvsh.shape.pars.of*(mp.tau)*360.0,obsstep);
   int ph_inc_int=roundoff(phase_increment/obsstep,1);

//mp.ofph stores the phase accumulation due to offset
   mp.ofph = setvvarname();
   initval(ph_inc_int,mp.ofph);

// Set mp.va and initialize 2048 steps (90 degrees)

//   mp.va = setvvarname();
//   initval(PSD*2048.0, mp.va);

   return mp;
}


//------------------------------------------------------
// Windowed DUMBO
//------------------------------------------------------

void _wdumbo1(WMPSEQ mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.wvsh.mpseq.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.wvsh.mpseq.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.wvsh.mpseq.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.wvsh.mpseq.ch,"dec3")) chnl = 4;
   else {
      printf("_wdumbo() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   double obsstep = 360.0/(PSD*8192);
   gen_RFpwrf(mp.wvsh.mpseq.a,chnl);
   gen_RFunblank(chnl);
   loop(mp.vcycles,mp.vcount);
      gen_RFblank(chnl);
      rcvron();
      delay(mp.dtaua);
      acquire(mp.npa,2.0e-7);
      rcvroff();
      gen_RFunblank(chnl);
      delay(mp.r1);
      xmtrphase(mp.va);
      _mpseq(mp.wvsh.mpseq,phase);
//      gen_RFfphase(zero,chnl);
//      obsprgon(mp.wvsh.mpseq.pattern,mp.wvsh.mpseq.n90*12.5e-9,mp.wvsh.mpseq.pw[0]/(mp.wvsh.mpseq.n90*12.5e-9)*90.0);
//      delay(mp.wvsh.mpseq.t);
//      obsprgoff();
      delay(mp.r2);
      //remove complete 360 degrees contributions
      modn(mp.va,mp.vc,mp.va);
//add accumulation due to offset
      add(mp.va,mp.ofph,mp.va);
//add xmx shift
      add(mp.va,mp.vb,mp.va);
   endloop(mp.vcount);
   gen_RFunblank(chnl);
}

//------------------------------------------------------
// Windowed DUMBO
//------------------------------------------------------

void _wdumbo2(WMPSEQ mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.wvsh.shape.pars.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.wvsh.shape.pars.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.wvsh.shape.pars.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.wvsh.shape.pars.ch,"dec3")) chnl = 4;
   else {
      printf("_wdumbo() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   double obsstep = 360.0/(PSD*8192);
   gen_RFpwrf(mp.wvsh.shape.pars.a,chnl);
   gen_RFunblank(chnl);
   loop(mp.vcycles,mp.vcount);
      gen_RFblank(chnl);
      rcvron();
      delay(mp.dtaua);
      acquire(mp.npa,2.0e-7);
      rcvroff();
      gen_RFunblank(chnl);
      delay(mp.r1);
      xmtrphase(mp.va);
      _shape(mp.wvsh.shape,phase);
//      gen_RFfphase(zero,chnl);
//      obsprgon(mp.wvsh.mpseq.pattern,mp.wvsh.mpseq.n90*12.5e-9,mp.wvsh.mpseq.pw[0]/(mp.wvsh.mpseq.n90*12.5e-9)*90.0);
//      delay(mp.wvsh.mpseq.t);
//      obsprgoff();
      delay(mp.r2);
      //remove complete 360 degrees contributions
      modn(mp.va,mp.vc,mp.va);
//add accumulation due to offset
      add(mp.va,mp.ofph,mp.va);
//add xmx shift
      add(mp.va,mp.vb,mp.va);
   endloop(mp.vcount);
   gen_RFunblank(chnl);
}

//   -----------------------------
//   Windowed PMLGxmx
//   -----------------------------

WMPSEQ getwpmlgxmx1(char *seqName)
{
   WMPSEQ mp;
   mp.wvsh.mpseq.nelem=1; //waveform should have a single PMLG element
   mp.wvsh.mpseq=getpmlg(seqName,0,0,0,0,1);

   char *var;
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("getwpmlg() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.wvsh.mpseq.seqName,seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.wvsh.mpseq.ch,sizeof(mp.wvsh.mpseq.ch));

// aXsuffix

   var = getname0("a",mp.wvsh.mpseq.seqName,"");
   mp.wvsh.mpseq.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.wvsh.mpseq.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.wvsh.mpseq.seqName,"");
   mp.r1 = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.wvsh.mpseq.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.wvsh.mpseq.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.wvsh.mpseq.seqName,"");
   mp.tau = getval(var);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.wvsh.mpseq.t - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwpmlg() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of WPMLG Cycles and create V-vars for the main loop. 

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(mp.tau*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

//Calculate phase increments
   double obsstep=360.0/(8192*PSD);
   mp.va = setvvarname();
   mp.vb = setvvarname();
   mp.vc = setvvarname();

//mp.va stores the accumulated phase for each PMLG block
   initval(0.0, mp.va);
//mp.vb stores the 180.0 shift needed for xmx
   initval(roundoff(180.0/obsstep,1),mp.vb);
//mp.vc stores 360 degrees for removing the complete phase
   initval(roundoff(360.0/obsstep,1),mp.vc);
   

   double phase_increment=roundphase(mp.wvsh.mpseq.of[0]*(mp.tau)*360.0,obsstep);
   int ph_inc_int=roundoff(phase_increment/obsstep,1);

//mp.ofph stores the phase accumulation due to offset
   mp.ofph = setvvarname();
   initval(ph_inc_int,mp.ofph);

   return mp;
}

void _wpmlg1(WMPSEQ mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.wvsh.mpseq.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.wvsh.mpseq.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.wvsh.mpseq.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.wvsh.mpseq.ch,"dec3")) chnl = 4;
   else {
      printf("_wpmlg() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   double obsstep = 360.0/(PSD*8192);
   obsstepsize(obsstep);
   gen_RFpwrf(mp.wvsh.mpseq.a,chnl);
   gen_RFunblank(chnl);
   loop(mp.vcycles,mp.vcount);
      gen_RFblank(chnl);
      rcvron();
      delay(mp.dtaua);
      acquire(mp.npa,2.0e-7);
      rcvroff();
      gen_RFunblank(chnl);
      delay(mp.r1);
      xmtrphase(mp.va);
      _mpseq(mp.wvsh.mpseq,phase);
      delay(mp.r2);
//remove complete 360 degrees contributions
      modn(mp.va,mp.vc,mp.va);
//add accumulation due to offset
      add(mp.va,mp.ofph,mp.va);
//add xmx shift
      add(mp.va,mp.vb,mp.va);
   endloop(mp.vcount);
   gen_RFunblank(chnl);
}

WMPSEQ getwsamn1(char *seqName)
{
   WMPSEQ mp;
   mp.wvsh.mpseq=getsamn(seqName,0,0,0,0,1);
   char *var;
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("getwsamn() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.wvsh.mpseq.seqName,seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.wvsh.mpseq.ch,sizeof(mp.wvsh.mpseq.ch));

// aXsuffix

//   var = getname0("a",mp.wvsh.mpseq.seqName,"");
//   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.wvsh.mpseq.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.wvsh.mpseq.seqName,"");
   mp.r1 = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.wvsh.mpseq.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.wvsh.mpseq.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.wvsh.mpseq.seqName,"");
   mp.tau = getval(var);

// qXsuffix

   var = getname0("q",mp.wvsh.mpseq.seqName,"");
   int rfcycles = getval(var);

// pwXsuffix

   var = getname0("pw",mp.wvsh.mpseq.seqName,"");
   double taur = getval(var);

//   taur = roundoff(taur,rfcycles*300.0e-9);
//   int totalsteps = (int) roundoff(taur/300.0e-9,1);
//   mp.pw = 300.0e-9; 

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.wvsh.mpseq.t - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwsamn() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }


// Set the Number of SAM Rotor Cycles and Create V-vars for the Main Loop. 

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();
   double cycles = (double) (int) (np/(mp.tau*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

   return mp;
}

void _wsamn1(WMPSEQ mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.wvsh.mpseq.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.wvsh.mpseq.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.wvsh.mpseq.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.wvsh.mpseq.ch,"dec3")) chnl = 4;
   else {
      printf("_wsam() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   double obsstep = 360.0/(PSD*8192);
   gen_RFpwrf(mp.wvsh.mpseq.a,chnl);
   gen_RFunblank(chnl);
   loop(mp.vcycles,mp.vcount);
      gen_RFblank(chnl);
      rcvron();
      delay(mp.dtaua);
      acquire(mp.npa,2.0e-7);
      rcvroff();
      gen_RFunblank(chnl);
      delay(mp.r1);
      gen_RFfphase(zero,chnl);
      _mpseq(mp.wvsh.mpseq,phase);
//      obsprgon(mp.wvsh.mpseq.pattern,mp.wvsh.mpseq.n90*12.5e-9,mp.wvsh.mpseq.pw[0]/(mp.wvsh.mpseq.n90*12.5e-9)*90.0);
//      delay(mp.wvsh.mpseq.t);
//      obsprgoff();
   endloop(mp.vcount);
   gen_RFunblank(chnl);
}

#endif


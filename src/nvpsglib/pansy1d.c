// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */

/*  pansy1d - one-pulse sequence  for sequential multi receive 
    parameters: rcvrs = 'yy'
                mrmode = 'p' parallel acquisition of H-1 and C-13, no decoupling;
		         's' sequential acquisition of H-1 and C-13, no decoupling;
			 'd' sequential acquisition of H-1 and C-13 with H-1 decoupling;
			      for H-1 decoupling use dpwr, dseq, dres and dmf parameters,
			      which is convenient, but not strictly correct;		        
*/

#include <standard.h>

static shape H1dec;

static int ph1[4] = {0, 2, 1, 3};
static int ph2[4] = {0, 2, 1, 3};

pulsesequence()
{
  char rcvrsflag[MAXSTR],
       mrmode[MAXSTR];

  double compH, Hdpwr, pw90,
         pwx = getval("pwx"),
         pwxlvl = getval("pwxlvl");
	  
  getstr("rcvrs",rcvrsflag);
  getstr("mrmode",mrmode);
  Hdpwr = 0.0; compH = 1.0;   /* initialize the parameters */

  if (strcmp(rcvrsflag,"yy"))
    printf("rcvrs parameter should be set to 'yy'\n");

  /* check decoupling modes */

  if ((dm[A] == 'y') || (dm[B] == 'y') || (dm[C] == 'y') || (dm[D] == 'y') || (homo[0] == 'y'))
  {
    printf("dm should be set to 'nnnn' and homo should set to 'n'");
    psg_abort(1);
  }
  
  if(mrmode[A] == 'd')  /* make the decoupling waveform */
  {
    Hdpwr = getval("Hdpwr");
    compH = getval("compH");
    pw90  = getval("pw90");
    if(FIRST_FID) 
      H1dec = pbox_mix("pansyW16", "WALTZ16", Hdpwr, pw90*compH, tpwr);
  }
  else if(mrmode[A] != 's') 
    mrmode[A] = 'p';           /* default is 'p' */

  
  status(A);
  
  delay(1.0e-4);
  decpower(pwxlvl);
  obspower(tpwr);    
  delay(d1);
  
  status(B);

  if(mrmode[A] != 'p')   /* sequential acquisition */
  {    
    rgpulse(pw, oph, rof1, rof2);
     
    setactivercvrs("yn");
    startacq(alfa);
    acquire(np,1.0/sw);
    endacq();             
    delay(1.0e-6);

    if(mrmode[A] == 'd')
    {
      obsunblank();
      pbox_xmtron(&H1dec);
    }   
      delay(d2);    
      decrgpulse(pwx, oph, rof1, rof2);

      status(C);
      
      setactivercvrs("ny");
      startacq(alfa);
      acquire(np,1.0/sw);
      endacq();
      
    if(mrmode[A] == 'd')
    {
      pbox_xmtroff();    
      obsblank();
    }
  }
  else 
  {    
    delay(d2);
    simpulse(pw, pwx, oph, oph, rof1, rof2);
 
    status(C);
  }
}

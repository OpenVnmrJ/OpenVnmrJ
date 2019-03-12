// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/************************************************************************/
/* docycle        Decoupler offset cycling for improved noe difference	*/
/*                experiment or multiple peak saturation		*/
/* Reference:     M. Kinns and J.K.M. Sanders, J. Magn. Reson. 		*/
/*                56:518 (1984)						*/
/* Parameters:								*/
/* f1...f9        individual decoupling frequencies (set unused		*/
/*                frequyencies. to 0)					*/
/* tau            time per individual decoupling (#cycles=d1/tau*n)	*/
/* dofoff         position of decoupler during d1 of control experiment	*/
/* doq            posistion of decoupler during acquisition		*/
/* ctrl           y to do control experiment, n for decoupling cycling	*/
/************************************************************************/
#include <standard.h>

#define CURRENT         1

void pulsesequence()
{
int	i, n ;
char	fname[5];
char	ctrl [8];
double  dofoff, tau, cycles;
double  fn [10],fabs();

   n = 0;
   fname[0] = 'f';
   fname[2] = '\000';
   do
   {  fname[1] = n + '0';
      fn[n]    = getval(fname);
      n=n+1;
   }
   while ( (n<9) && (fabs(fn[n-1])>0.005) );
   n--;
   tau = getval("tau");
   if (tau==0.0)
      cycles=1.0;
   else 
      cycles=(double)((int)(d1/(tau*(double)n)));
   tau=d1/(cycles*(double)n); 
   initval(cycles,v4);
      
   dofoff = getval("dofoff");
   if ( P_getstring(CURRENT,"ctrl",ctrl,1,7) < 0)
      printf("noedif: Cannot find 'ctrl'\n");
   status(A);
   if (ctrl[0]=='y')
      {		/* control experiment, decoupler position=dofoff */
      if (!cpflag)  initval(0.0,v1);
      else          assign(oph,v1);
      offset(dofoff,DODEV);
      delay(d1);
      }
   else
      {		/* decoupler cycling experiment */
      hlv(ct,v1);
      assign(v1,oph);
      mod2(ct,v2);
      ifzero(v2);
         add(oph,two,oph);
         mod4(oph,oph);
         loop(v4,v3);
         for (i=0; i<n; i++)
         {
            offset(dofoff,DODEV);
            delay(tau);
         }
         endloop(v3);
      elsenz(v2);
         mod4(oph,oph);
         loop(v4,v3);
         for (i=0; i<n; i++)
         {
            offset(fn[i],DODEV);
            delay(tau);
         }
         endloop(v3);
      endif(v2);
   }
   status(B);
   offset(dof,DODEV);   /* normal 'dof' is decoupler during acquisition */
   delay(d2);
   status(C);
   rgpulse(pw,v1,rof1,rof2);
}


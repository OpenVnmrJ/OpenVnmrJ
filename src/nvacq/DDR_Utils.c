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

//=========================================================================
// FILE: DDR_Utils.c
//=========================================================================

#include "DDR_Utils.h"

//###################  405 utilities #############################

#ifndef C67

//=========================================================================
// markTime: set system clock time marker
//=========================================================================
void markTime(long long *dat)
{ 
  union ltw TEMP;
  int top,bot;
  vxTimeBaseGet(&top,&bot);
  TEMP.idur[0] = top;
  TEMP.idur[1] = bot;
  *dat = (TEMP.ldur);
}

//=========================================================================
// diffTime: return delta-time (microseconds)
//=========================================================================
double diffTime(long long m2,long long m1)
{
    return ((double)(m2-m1))/333.3 ;
}

#endif

//=========================================================================
// acqtm: return calculated acquisition time
//=========================================================================
double calc_acqtm(double SR, int NP, 
		double SW1, double OS1, double XS1,
		double SW2, double OS2, double XS2
		)
{
    int NX1,NY1,M1,N1,M2,N2;    
    if(OS1==0 || SW1==0){
        NX1=NP;
	}
	else{	
		M1=(int)(SR/SW1);
		N1=(int)(OS1*M1+0.5);
		N1=(N1%2==0)?N1+1:N1;		
		if(XS1>=1 || XS1<=-1)		
 		    XS1=(int)(0.5*N1-XS1);
 		else
 		    XS1=(int)(N1*(0.5-XS1));
 		XS1=XS1<0?0:XS1;		
		if(SW2 && OS2){
 			M2=(int)(SW1/SW2);
			N2=(int)(OS2*(int)M2+0.5);
			N2=(N2%2==0)?N2+1:N2;			
			if(XS2>=1 || XS2<=-1)		
 				XS2=(int)(0.5*N2-XS2);
 			else
 		   	 	XS2=(int)(N2*(0.5-XS2));
 			XS2=XS2<0?0:XS2;
 			NY1=(int)((NP-1)*(int)M2+N2-XS2);
			NX1=(int)((NY1-1)*(int)M1+N1-XS1);	
		}
		else{
			NX1=(NP-1)*M1+N1-XS1;
		}
	} 
	return NX1/SR;
}

//=========================================================================
// acqtm: return calculated sweep width
//=========================================================================
double calc_sw(double SR, double SW1, double SW2)
{
 	return SW2?SW2:(SW1?SW1:SR);
}

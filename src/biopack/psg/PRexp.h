/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* PRexp.h - include file for PR experiments */
/*
    in a 4D experiment 
    pra > 0, prb > 0  F1, F2, F3 ~ d2 (ni), inactive: d3, d4 (ni2, ni3)
    pra = 0, prb > 0  F1, F3 ~ d2, F2 ~ d3 (ni2), inactive : d4 (ni3)
    pra = 90, prb > 0 F2, F3 ~ d2, F1 ~ d3 (ni2), inactive : d4 (ni3)
    pra > 0, prb = 0  F1, F2 ~ d2, F3 ~ d3 (ni2), inactive : d4 (ni3)
    prb = 90 F4 ~ d2 (ni), inactive : d3 (ni2), d4 (ni3)
    pra = 0, prb = 0 F1 ~ d2 (ni), F2 ~ d3 (ni2), F3 ~ d4 (ni3), all active 
*/

#define SWAP(a,b) tm=(a); (a)=(b); (b)=tm

int         PRexp;
int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
            t3_counter,  	 	        /* used for states tppi in t3 */
            t4_counter,  	 	        /* used for states tppi in t4 */
            t5_counter,  	 	        /* used for states tppi in t5 */
            t6_counter,  	 	        /* used for states tppi in t6 */
            t7_counter;  	 	        /* used for states tppi in t7 */

double  tau1, tau2, tau3, tau4, tau5, tau6, tau7;
double  swF1, swF2, swF3, swF4, swF5, swF6, swF7;
int     niF1, niF2, niF3, niF4, niF5, niF6, niF7;
int     phaseF1, phaseF2, phaseF3, phaseF4, phaseF5, phaseF6, phaseF7; 
static double   d2_init=0.0, d3_init=0.0, d4_init=0.0;


void proj_angle_error(msg)
char *msg;
{
  printf("PR error: illegal projection angle: %s \n", msg);
  printf("          The acceptable range is 0 to 90 degrees\n");
  psg_abort(1);
}

void PRsetup_error(msg)
char *msg;
{
  printf("PR setup error: %s \n", msg);
  psg_abort(1);
}

void hyperD()
{
  int     PRdim;
  int     ni = (int) (0.5 + getval("ni")), 
          ni2 = (int) (0.5 + getval("ni2")), 
	  ni3 = 0;
  double  sw1 = getval("sw1"),
	  sw2 = getval("sw2"),
	  sw3 = 1.0;
  double  pra, prb, prc, prd, pre, prf, rd, 
          csa, csb, csc, csd, cse, csf, sna, snb, snc, snd, sne, snf;
    
  PRexp = 0; PRdim = 0;
  rd = M_PI/180.0; 

  tau1 = d2;  tau2 = d3;  tau3 = 0.0; tau4 = 0.0; tau5 = 0.0; tau6 = 0.0; tau7 = 0.0;  
  csa = 1.0;  csb = 1.0;  csc = 1.0;  csd = 1.0;  cse = 1.0;  csf = 1.0;
  sna = 0.0;  snb = 0.0;  snc = 0.0;  snd = 0.0;  sne = 0.0;  snf = 0.0; 
    
  if( ix == 1) { d2_init = d2; d3_init = d3; }
     
  t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
  t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );  
  t3_counter = 0; t4_counter=0; t5_counter=0; t6_counter=0; t7_counter=0;

  if(find("ni3") > 0) 
  { ni3 = (int) (0.5 + getval("ni3")); tau3 = d4; if( ix == 1) d4_init = d4; }
  niF1 = ni; niF2 = ni2; niF3 = ni3; niF4 = 0; niF5 = 0; niF6 = 0; niF7 = 0;
  if(find("sw3") > 0) 
  { sw3 = (int) (0.5 + getval("sw3")); t3_counter = (int) ( (d4-d4_init)*sw3 + 0.5 ); }
  swF1 = sw1; swF2 = sw2; swF3 = sw3; swF4 = 1.0; swF5 = 1.0; swF6 = 1.0; swF7 = 1.0;  
  
  phaseF1=phase1; phaseF2=phase2; phaseF3=phase3; 
  phaseF4=0; phaseF5=0; phaseF6=0; phaseF7=0; 
 
        
  if(((ni > 1) || (ni2 > 1) || (ni3 > 1)) && (find("pra") > 0)) 
  { 
    pra = getval("pra");               
    if((pra < 0.0) || (pra > 90.0)) 
      proj_angle_error("pra");
    PRexp = 1; 
    if(pra > 0.0)
    {
      csa = cos(pra*rd); sna = sin(pra*rd);
      if(pra == 90.0) /* swap F1 and F2 */
      {
	SWAP(tau2, tau1); SWAP(phaseF2, phaseF1); SWAP(niF2, niF1);
	SWAP(swF2, swF1); SWAP(t2_counter, t1_counter);
        if(FIRST_FID) printf("F1 and F2 swapped\n"); 
      } 
      else
      {
        tau1 = d2*csa; swF1 = sw1/csa; t2_counter = 0; 
	tau2 = d2*sna; swF2 = sw1/sna; niF2 = niF1; PRdim++;
      }
    }    
    
    if(find("prb") > 0)   /* params: d4, tau3, sw3, swF3, phaseF3, ni3, niF3, t3_counter */
    {  
      prb = getval("prb");
      if((prb < 0.0) || (prb > 90.0)) 
        proj_angle_error("prb");
      PRexp++;
      phaseF3 = (int) (0.5 + getval("phase3"));             
      if(prb > 0.0)   
      {	  
        csb = cos(prb*rd); snb = sin(prb*rd);
        if(prb == 90.0)                                             /* swap F1 and F3 */ 
	{
	  if(PRdim) PRsetup_error("Set pra = 0 or 90");
	  SWAP(tau3, tau1); SWAP(phaseF3, phaseF1); SWAP(niF3, niF1);
	  SWAP(swF3, swF1); SWAP(t3_counter, t1_counter);	    
	  if(FIRST_FID) printf("F1 and F3 swapped\n"); 
	}
        else
        {
	  t3_counter = 0;
	  if (pra > 0.0)  { tau2 *= csb; swF2 /= csb; }  /* if pra = 0  tau2 = d3 */
	  if (pra < 90.0) { tau1 *= csb; swF1 /= csb; }  /* if pra = 90 tau1 = d3 */
	  tau3 = d2*snb; swF3 = sw1/snb; niF3 = niF1; 	    
	  PRdim ++;	  	   
	}        
      } 
          
      if(find("prc") > 0)     /* 5D */
      { 
        prc = getval("prc"); 
        if((prc < 0.0) || (prc > 90.0)) 
          proj_angle_error("prc");
        PRexp++;
        phaseF4 = (int) (0.5 + getval("phase4"));   
        if(prc > 0.0)  
        {
          csc = cos(prc*rd); snc = sin(prc*rd);
          if(prc == 90.0)                                             /* swap F1 and F4 */ 
	  {
	    if(PRdim) PRsetup_error("Set pra and prb to 0 or 90");
	    SWAP(tau4, tau1); SWAP(phaseF4, phaseF1); SWAP(niF4, niF1);
	    SWAP(swF4, swF1); SWAP(t4_counter, t1_counter);	      
	    if(FIRST_FID) printf("F1 and F4 swapped\n"); 
	  }
	  else
	  {
	    if(prb > 0.0) { tau3 *= csc; swF3 /= csc; }  	      
	    if(prb < 90.0) 
	    {
	      if(pra > 0.0)  { tau2 *= csc; swF2 /= csc; }
              if(pra < 90.0) { tau1 *= csc; swF1 /= csc; }
	    }	       	      
	    tau4 = d2*snc; swF4 = sw1/snc; niF4 = niF1;  
	    PRdim ++;	  
	  }	    
        } 
	
        if(find("prd") > 0)     /* 6D */ 
        { 
          prd = getval("prd"); 
          if((prd < 0.0) || (prd > 90.0)) 
            proj_angle_error("prd");
          PRexp++;
          phaseF5 = (int) (0.5 + getval("phase5")); 
	  
          if(prd > 0.0)  
          {
            csd = cos(prd*rd); snd = sin(prd*rd);
            if(prd == 90.0)                                             /* swap F1 and F5 */ 
	    {
	      if(PRdim) PRsetup_error("Set pra, prb and prc to 0 or 90");
	      SWAP(tau5, tau1); SWAP(phaseF5, phaseF1); SWAP(niF5, niF1);
	      SWAP(swF5, swF1); SWAP(t5_counter, t1_counter);	      
	      if(FIRST_FID) printf("F1 and F5 swapped\n"); 
	    }
	    else
	    {
	      if(prc > 0.0) { tau4 *= csd; swF4 /= csd; }  	      
	      if(prc < 90.0) 
	      {
	        if(prb > 0.0) { tau3 *= csd; swF3 /= csd; }  	      
	        if(prb < 90.0) 
	        {
	          if(pra > 0.0)  { tau2 *= csd; swF2 /= csd; }
                  if(pra < 90.0) { tau1 *= csd; swF1 /= csd; }
	        }
	      }	       	      
	      tau5 = d2*snd; swF5 = sw1/snd; niF5 = niF1;  
	      PRdim ++;	  
	    }	    
          } 
          if(find("pre") > 0)   /* 7D */
          { 
            pre = getval("pre"); 
            if((pre < 0.0) || (pre > 90.0)) 
              proj_angle_error("pre");
            PRexp++;	      
            phaseF6 = (int) (0.5 + getval("phase6"));   

            if(pre > 0.0)  
            {
              cse = cos(pre*rd); sne = sin(pre*rd);
              if(pre == 90.0)                                             /* swap F1 and F6 */ 
	      {
	        if(PRdim) PRsetup_error("Set pra, prb, prc and prd to 0 or 90");
	        SWAP(tau6, tau1); SWAP(phaseF6, phaseF1); SWAP(niF6, niF1);
	        SWAP(swF6, swF1); SWAP(t6_counter, t1_counter);	      
	        if(FIRST_FID) printf("F1 and F6 swapped\n"); 
	      }
	      else
	      {
	        if(prd > 0.0) { tau5 *= cse; swF5 /= cse; }  	      
	        if(prd < 90.0) 
	        {
	          if(prc > 0.0) { tau4 *= cse; swF4 /= cse; }  	      
	          if(prc < 90.0) 
	          {
	            if(prb > 0.0) { tau3 *= cse; swF3 /= cse; }  	      
	            if(prb < 90.0) 
	            {
	              if(pra > 0.0)  { tau2 *= cse; swF2 /= cse; }
                      if(pra < 90.0) { tau1 *= cse; swF1 /= cse; }
	            }
	          }
	        }	       	      
	        tau6 = d2*sne; swF6 = sw1/sne; niF6 = niF1;  
	        PRdim ++;	  
	      }	    
            } 	      
            if(find("prf") > 0)   /* 8D */ 
            { 
              prf = getval("prf"); 
              if((prf < 0.0) || (prf > 90.0)) 
                proj_angle_error("prf");
              PRexp++;
              phaseF7 = (int) (0.5 + getval("phase7"));
	      
              if(prf > 0.0)  
              {
                csf = cos(prf*rd); snf = sin(prf*rd);
                if(prf == 90.0)                                             /* swap F1 and F7 */ 
	        {
	          if(PRdim) PRsetup_error("Set pra, prb, prc, prd and pre to 0 or 90");
	          SWAP(tau7, tau1); SWAP(phaseF7, phaseF1); SWAP(niF7, niF1);
	          SWAP(swF7, swF1); SWAP(t7_counter, t1_counter);	      
	          if(FIRST_FID) printf("F1 and F7 swapped\n"); 
	        }
	        else
	        {
	          if(pre > 0.0) { tau6 *= csf; swF6 /= csf; }  	      
	          if(pre < 90.0) 
	          {
	            if(prd > 0.0) { tau5 *= csf; swF5 /= csf; }  	      
	            if(prd < 90.0) 
	            {
	              if(prc > 0.0) { tau4 *= csf; swF4 /= csf; }  	      
	              if(prc < 90.0) 
	              {
	                if(prb > 0.0) { tau3 *= csf; swF3 /= csf; }  	      
	                if(prb < 90.0) 
		        {
	                  if(pra > 0.0)  { tau2 *= csf; swF2 /= csf; }
                          if(pra < 90.0) { tau1 *= csf; swF1 /= csf; }
	                }
	              }
	            }
	          }	       	      
	          tau7 = d2*snf; swF7 = sw1/snf; niF7 = niF1;  
	          PRdim ++;
		}	  
	      }	    
            } 	      	      	           
          }
        }
      }
    }
  }
}  

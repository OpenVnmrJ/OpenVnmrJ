/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*NOTE TO PROGRAMMER

This is a template. There may be an error or two. You are responsible to
create 3 parameter arrays settable by the GUI for on/off and tpwr and tpwrf and 
loading these values into the c-arrays:

my_on_off
my_tpwr
my_tpwrf

Watch out for the indexing in the calls to modifyRFGroup since the 
channel index starts at one but I address my arrays at 0 and the 
VNMRJ may want to address parameter arrays from 1 or whatever. Anyway
you have to get that straight.

The model for this is the fidelity.c sequence in /vnmr/psglib.
There are parts to fidelity that I don't quite understand and the
whole fidelity package has a few problems. My suggestion is for you to build
the GUI around the transmit capability and then after that just pull
in the parts of fidelity that have to do with matlab calls.

*/



#include <standard.h>
#include "sgl.c"

void pulsesequence()
{
	/* Internal variable declarations *********************/
	char txphase[MAXSTR];
	char rxphase[MAXSTR];
        char my_p1pat[MAXSTR];
        char my_p4pat[MAXSTR];
        char my_p2pat[MAXSTR];
        char my_p3pat[MAXSTR];
        char blankmode[MAXSTR]={0};
        char tmpstr[MAXSTR];
	double  postDelay;
	double rfDuration;
	double acqt;
	int i,ret=-1;
	static int phs1[4] = {0,2,1,3}; /* from T1meas.c */
	
	int my_pulseID;
	double my_offsets[64];
	double my_tpwr[64];
	double my_tpwrf[64];
	int my_on_off[64];
	double mytmp;
	int my_numrfch;
	int my_retval __attribute__((unused));


	

	for(i=0;i<64;i++) my_offsets[i] = 0.0;

	/*************************************************/                                     
	/*  Initialize paramter **************************/
	i                = 0;
	postDelay        = 0.5;
	acqt             = 0.0;
//	getstr("rxphase",rxphase);
//	getstr("txphase",txphase); 
        getstr("p1pat",my_p1pat);
        getstr("p2pat",my_p2pat); 
        getstr("p3pat",my_p3pat); 
        getstr("p4pat",my_p4pat);  
         

	ret = P_getstring(GLOBAL,"blankmode",blankmode,1,MAXSTR);

	if ( P_getreal(CURRENT,"ampargs",&mytmp,2) < 0 )
		mytmp = 0.0;

    if ( P_getreal(GLOBAL, "numrfch", &mytmp, 1) < 0 )
    abort_message("Cannot proceed without global numrfch\n");   
    my_numrfch = (int)(mytmp+0.05);
    if(my_numrfch<2) abort_message("this is a single transmit system and you don't need this pulse sequence\n");   
    if(my_numrfch>16) abort_message("numrfch > 16 not supported \n");   

    printf("mytmp=%f\n",mytmp);
     

 // if((P_getstring(GLOBAL,"rfGroupMap",tmpstr,1,MAXSTR) < 0) || (strlen(tmp) < 2))
 //   {
   // abort_message("global rfGroupMap doesn't exist or is too short\n");   
 // }
 // else // rfGroupMap is set to all 1's for the same number as numrfch so we control everything here in the sequence
  //{
     strncpy(tmpstr,"111111111111111111111111111111111111111111111111111111111111111111",(size_t)my_numrfch);
     tmpstr[my_numrfch]=0;
     P_setstring(CURRENT, "rfGroupMap", tmpstr, 1);
  // }

    //getparm("blankmode","string",GLOBAL,blankmode,MAXSTR);
	postDelay = tr - at;

   //printf("blankmode=%s\n",blankmode);
                        
	/*************************************************/
	/* check phase setting ***************************/
//	if ( (txphase[0] =='n')   && (rxphase[0] =='n') )
//	{
//		abort_message("ERROR - Select at least one phase [Tx or Rx]\n");   
//	}

	/**************************************************/
	/* check pulse width  *****************************/
	rfDuration = shapelistpw(p1pat, pw);     /* assign exitation pulse  duration */
        printf("p1pat: %s\n",my_p1pat);                  
        printf("duration: %e\n",rfDuration);                  
	acqt = rfDuration + rof1 - alfa;
	if (FP_GT(acqt, at))
	{
		abort_message("Pulse duration too long. max [%.3f]    ms\n",(at-rof1+alfa)*1000.0);   
	}
    if(ret==0 && blankmode[0]=='u')
    	obsunblank();
	delay(postDelay);
    
	settable(t1,4,phs1); /*from T1meas.c */
	getelem(t1,ct,v11);  /*from T1meas.c */
	setreceiver(t1);  

        printf("mytmp=%d\n",my_numrfch);                  

	my_pulseID = initRFGroupPulse(rfDuration, my_p1pat, 's',  0.0, 0.0, v11, &my_offsets[0], my_numrfch-1);
        printf("pulse id = %d\n",my_pulseID);
         //modifyRFGroupName(my_pulseID,2,my_p2pat);
         //modifyRFGroupName(my_pulseID,3,my_p3pat);
         // modifyRFGroupName(my_pulseID,4,my_p4pat);

        
	for(i=0;i<64;i++) my_tpwr[i]=0.0;
	for(i=0;i<64;i++) my_tpwrf[i]=0.0;
	for(i=0;i<64;i++) my_on_off[i]=0;

//SAS turn everything off
	for(i=0;i<my_numrfch-1;i++) 

        {
        modifyRFGroupOnOff(my_pulseID, i+1,0);
        printf("here is i=%d\n",i);
        }
	for(i=0;i<my_numrfch-1;i++) modifyRFGroupPwrf(my_pulseID, i+1, 0.0);
	for(i=0;i<my_numrfch-1;i++) modifyRFGroupPwrC(my_pulseID, i+1, 0.0);

//SAS get the parameter arrays into the local arrays

     my_retval=(int)getarray("my_tpwr",my_tpwr);

/*
	printf("my tpwr0 was: %e\n",my_tpwr[0]);
	printf("my tpwr1 was: %e\n",my_tpwr[1]);
	printf("my tpwr2 was: %e\n",my_tpwr[2]);
	printf("my tpwr3 was: %e\n",my_tpwr[3]);
	printf("my tpwr4 was: %e\n",my_tpwr[4]);
	printf("my tpwr5 was: %e\n",my_tpwr[5]);
	printf("my tpwr6 was: %e\n",my_tpwr[6]);
	printf("my tpwr7 was: %e\n",my_tpwr[7]);
*/

     my_retval=(int)getarray("my_on_off",my_tpwrf); //dump the array of reals into a real array

	for(i=0;i<my_numrfch;i++) my_on_off[i] = (int)my_tpwrf[i]; //convert to integer

/*
	printf("my on_off_0 was: %d\n",my_on_off[0]);
	printf("my on_off_1 was: %d\n",my_on_off[1]);
	printf("my on_off_2 was: %d\n",my_on_off[2]);
	printf("my on_off_3 was: %d\n",my_on_off[3]);
	printf("my on_off_4 was: %d\n",my_on_off[4]);
	printf("my on_off_5 was: %d\n",my_on_off[5]);
	printf("my on_off_6 was: %d\n",my_on_off[6]);
	printf("my on_off_7 was: %d\n",my_on_off[7]);
*/
	for(i=0;i<64;i++) my_tpwrf[i]=2047.0;

//SAS set everything to array values

	for(i=0;i<my_numrfch;i++) modifyRFGroupOnOff(my_pulseID, i+1,my_on_off[i]);
	for(i=0;i<my_numrfch;i++) modifyRFGroupPwrf(my_pulseID, i+1, my_tpwrf[i]);
	for(i=0;i<my_numrfch;i++) modifyRFGroupPwrC(my_pulseID, i+1, my_tpwr[i]);


	/*==============================================*/
	/*  START LOOPBACK PULSE SEQUENCE               */
	/*==============================================*/
	status(A);
	obsoffset(resto);

	/* TTL trigger to scope sequence ****************************/       
	sp1on();             

	/* Relaxation delay ***********************************/       
    xgate(ticks);

	/* RF pulse *******************************************/ 
	GroupPulse(my_pulseID, rof1, rof2,  v11, 0);

	endacq();
	sp1off();
    if(ret==0 && blankmode[0]=='u')
 		obsunblank();
}

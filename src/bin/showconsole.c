/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "config.h"

#define  AP_ADDR_UNKNOWN  -1

#define BRDTYPESIZE 5
typedef struct __brdtype_ {
		     int code;
		     char *codestr;
                  } BrdType;

static BrdType boardType[BRDTYPESIZE] = { 
				{ 2101 , "DTM 16 MB" },
				{ 2201, "DTM 64 MB" },
				{ 2301, "DTM 4 MB" },
				{ 5001, "NMR-DATA ADC DUAL Solids 5MHz" },
				{ 5101, "NMR-DATA ADC 1MHz" } 
			      };

static char   consoleDataFile[ 1024 ] = "";
struct _hw_config     hw_config;

static void
locateConfFile( char *confFile )
{
        strcpy( confFile, getenv("vnmrsystem") );
        if (strlen( confFile ) < (size_t) 1)
          strcpy( confFile, "/vnmr" );
        strcat( confFile, "/acqqueue/acq.conf" );
}

static int
readConsoleData(int argc, char *argv[])
{
   int  fd, ival;
 
   if (argc > 1)
      strcpy( consoleDataFile, argv[1] );
   else
      locateConfFile( &consoleDataFile[ 0 ] );
 
   /*  printf("config_file = %s\n", &consoleDataFile[ 0 ]); */

   fd = open( &consoleDataFile[ 0 ], O_RDONLY );
   if (fd < 0)
   {
      printf("Cannot open %s\n", &consoleDataFile[ 0 ]);
      close( fd );
      return( -1 );
   } 
   ival = read( fd, &hw_config, sizeof( hw_config ) );
   close( fd );
 
    /* printf( "read %d, expecting %d\n", ival, sizeof( hw_config ) ); */
 
   if (ival != sizeof( hw_config ))
   {
      printf("Error reading %s\n", &consoleDataFile[ 0 ]);
      return( -1 );
   }
   else
   {
      return( 0 );
   }
}          

static void
show_h1freq()
{
   int  value;
   
   value = (hw_config.H1freq >> 4) * 100;
/*printf("value= %d\n", value);*/
   printf("System H1 frequency : ");
   if( value == 700 )
      printf(" 750 MHz \n");
   else
      printf(" %d MHz\n", value);
}

static void
show_ADC()
{
   static int size;
   static int speed;

   size = (hw_config.ADCsize & 0xFF);
   speed = ((hw_config.ADCsize >> 8)* 100) ;

   printf("ADC :  ");
   if( hw_config.ADCsize == 0x1FF )
      printf("NOT Present\n");
   else
      printf("Present, %d-bit @ %d kHz\n", size, speed);
}

static void
show_VT()
{
   printf("VT  :  ");
   if(hw_config.VTpresent == 1)
      printf("Present\n");
   else
      printf("NOT Present\n");
}

static void
show_DTMs()
{
int i,j,  stmType;
   if (hw_config.STM_present)
   {
      for (i=0; i<hw_config.STM_present; i++)
      {  stmType = hw_config.STM_Type[i];
	 if (stmType > 0)
         {  for (j=0; j<5; j++)
            {  if (boardType[j].code == stmType) {
                  printf("DTM%d : %s\n",i,boardType[j].codestr);
		  break;
	       }
            }
	    printf("DTM%d : UNKNOWN (%d)\n",i,stmType);
         }
      }
   }
   else
      printf("DTM : None Present\n");
}

static void
show_DTM()
{
   printf("DTM :  ");
   if(hw_config.STM_present == 1)
      printf("Present\n");
   else
      printf("NOT Present\n");
}

static void
show_FIFO()
{
   printf("FIFO : ");
   if( hw_config.fifolpsize == 2048 )
      printf("Present\n");
   else
      printf("NOT Present\n");
}

           /* The routine below is not valid */
           /* need to study HS&R board to get more detail about PTS */
static void
show_PTS()
{
   int  i;

   printf("PTS :  ");
   if( hw_config.PTSes_present != (-1) )
   {
      /* printf("Present ==> "); */
      for( i=0; i<6; i++ )
      {
         if(hw_config.sram_val[ i ] != 511)
            printf("  %d ", hw_config.sram_val[ i ]);
      }
      printf("\n");
   }
   else
      printf("NOT Present\n");
}
 
static void
show_AMT(i)
int i;
{
   int tmp_apbyte;

   switch(i)
   {
      case 1:
         tmp_apbyte = hw_config.AMT1_conf;
         break;
      case 2:
         tmp_apbyte = hw_config.AMT2_conf;
         break;
      case 3:
         tmp_apbyte = hw_config.AMT3_conf;
         break;
    }
   /*printf("0x%05x ",tmp_apbyte);*/
   printf("Amplifier %d:  ",i);
   if (tmp_apbyte == AP_ADDR_UNKNOWN)
   {
      printf(">>> NO CONTROLLER <<< \n");
   }
   else
   {  tmp_apbyte &= 0xff;
      if      (tmp_apbyte==0x0) printf("AMT 3900-11  Lo-Lo band\n");
      else if (tmp_apbyte==0x1) printf("AMT 3900-12  Hi-Lo band\n");
      else if (tmp_apbyte==0x4) printf("AMT 3900-1S  Lo band\n");
      else if (tmp_apbyte==0x8) printf("AMT 3900-1   Lo band\n");
      else if (tmp_apbyte==0xb) printf("AMT 3900-1S4 HI-Lo band\n");
      else if (tmp_apbyte==0xc) printf("AMT 3900-15  Hi-Lo band(100W)\n");
      else if (tmp_apbyte==0xf) printf("None\n");
      else                      printf("Unknown\n");
   }
}

static void
show_ATTNs()
{
   int  i, tmp_apbyte;
   
   tmp_apbyte = hw_config.attn1_present;
   /*printf("0x%05x ",tmp_apbyte);*/
   printf("Attenuator(s) present are :");
   if (tmp_apbyte == AP_ADDR_UNKNOWN)
      printf(">>> NO CONTROLLER <<< \n");
   else
   {  for (i=0; i<4; i++)
      {  if (tmp_apbyte & (1<<i) )
            printf(" %d ",i+1);
      }  
      printf("\n");
   }
}

static void
show_RXcntrl()
{
   printf("Receiver controller : ");
  /* printf("0x%05x :      ",hw_config.rcvr_stat[0]);*/
   if( hw_config.rcvr_stat[0] == AP_ADDR_UNKNOWN )
      printf("NOT present\n");
   else
      printf("Present\n");
}

static void
show_XMTRs()
{
   printf("Transmitters: ");
   if ( (hw_config.xmtr_present & 0xf) == 0)
      printf(" >>> NO TRANSMITTERS <<< \n");
   else
      if (hw_config.xmtr_present & 0x1)
      {  printf("High band: ");
         if (hw_config.xmtr_stat[0]&0x10)
         { printf("79 dB ");
           if (!(hw_config.xmtr_stat[0] & 0x80))
               printf(" WFG  ");
            else
               printf(" No-WFG  ");
         }
         else
            printf(" 63 dB  ");
       }
       
       if (hw_config.xmtr_present & 0x2)
	  printf ("Low band: 63 dB");
       printf("\n");

}

static void
show_TXcntrl()
{ 
   int  i;   
   printf("TX_controller(s) present are : ");
   if ( (hw_config.xmtr_present & 0xf) == 0)
      printf(" >>> NO CONTROLLER <<< \n");
   else
   {  for (i=0; i<6; i++)
      {  if (hw_config.xmtr_present & (1<<i) )
            printf(" %d ",i+1);
      }  
      printf("\n");
   }

   /* Check Transmitters and RF wfgs */

   for (i=0; i<6; i++)
   {  if (hw_config.xmtr_present & (1<<i) )
      {  
         if ((hw_config.xmtr_stat[i]&0xF0) == 0xA0)
            printf(" RF TX_board %d : 2H Decoupler\n",i+1);
         else
         {  printf("  RF TX_board  %d : ", i+1);
            if (!(hw_config.xmtr_stat[i] & 0x2))
               printf(" Present \n");
            else  printf(" NOT present \n");
   
            printf("  WFG board    %d : ", i+1);
            if (!(hw_config.xmtr_stat[i] & 0x8))
               printf(" Present \n");
            else  printf(" N/A \n");
         }
      }
   }
}

static void
show_LKfreq()
{
   float x;

   x = (float) ((hw_config.sram_val[ LF_OFFSET ]<<16) +
                    ((int)hw_config.sram_val[ LF_OFFSET + 1 ] & 0xffff));
   printf("Lock frequency : %f MHz\n", x/1.0e6);
}

static void
show_IFfreq()
{
   float iffreq;
   if (hw_config.sram_val[ IF_OFFSET ] != 0x1ff)
   {  iffreq = (float) (hw_config.sram_val[ IF_OFFSET ]) /10;
      printf("IF frequency : %f MHz\n", iffreq);
   }
   else
      printf("IF frequency : UNKNOWN\n");
}

static void
show_Mleg_ID()
{
   printf("Magnet Leg Driver Id :   %d \n", hw_config.mleg_conf);
}

static void
show_SHIM()
{
   printf("Shim P.S. ");
   switch (hw_config.shimtype)
   { case  1:
        printf(":  Agilent 13 shims\n");
        break;
     case  2:
        printf(":  Oxford 18 shims\n");
        break;
     case  3:
        printf(":  Agilent 23 shims\n");
        break;
     case  4:
        printf(":  Agilent 28 shims\n");
        break;
     case  5:
        printf(":  Ultra Shims\n");
        break;
     case  6:
        printf(":  Agilent 18 shims\n");
        break;
     case  7:
        printf(":  Agilent 21 shims\n");
        break;
     case  8:
        printf(":  Oxford 15 shims\n");
        break;
     case  9:
        printf(":  Agilent 40 shims\n");
        break;
     case 10:
        printf(":  Agilent 14 shims\n");
        break;
     case 11:
        printf(":  Whole Body Shims\n");
        break;
     case 12:
        printf(":  Agilent 26 shims\n");
        break;
     case 13:
        printf(":  Agilent 29 shims\n");
        break;
     case 14:
        printf(":  Agilent 35 shims\n");
        break;
     case 15:
        printf(":  Agilent 15 shims\n");
        break;
     case 16:
        printf(":  Ultra 18 shims\n");
        break;
     case 17:
        printf(":  Agilent 27 shims\n");
        break;
     case 18:
        printf(":  Agilent Combo 13 shims\n");
        break;
     case 19:
        printf(":  Agilent Thin 28 shims (51mm)\n");
        break;
     case 20:
        printf(":  Agilent 32 shims\n");
        break;
     case 21:
        printf(":  Agilent 24 shims\n");
        break;
     case 22:
        printf(":  Oxford 28 shims\n");
        break;
     case 23:
        printf(":  Agilent Thin 28 shims (54mm)\n");
        break;
     case 24:
        printf(":  PremiumCompact+ 27 shims\n");
        break;
     case 25:
        printf(":  PremiumCompact+ 28 shims\n");
        break;
     case 26:
        printf(":  Agilent 32 shims (28 ch)\n");
        break;
     case 27:
        printf(":  Agilent 40 shims (28 ch)\n");
        break;
     default:
        /*printf("(0x%05x):  UNKNOWN or TIMEOUT <<<<<<<<<<\n",hw_config.shimtype);*/
        printf(":  UNKNOWN or TIMEOUT <<<<<<<<<<\n");
        break;
   }

}

static char *show_AMP(int show)
{
   static char type[50]; 
   static char val[3];

   switch( hw_config.sram_val[2] )
   {
      case 11:
         strcpy(type,"4-Nucleus (35W/35W)");
         strcpy(val,"aa");
         break;
      case 22:
         strcpy(type,"Broadband (75W/125W)");
         strcpy(val,"bb");
         break;
      case 33:
         strcpy(type,"CP/MAS (100W/300W)");
         strcpy(val,"cc");
         break;
      default:
         strcpy(type,"4-Nucleus (35W/35W)");
         strcpy(val,"aa");
         break;
       
   }
   if (show)
      printf("Amplifier: %s\n", type);
   return(val);
}

static void
show_PTS_value ()
{
   static char type[50]; 

   switch( hw_config.sram_val[0] )
   {
      case 1:
         strcpy(type,"4 Nuc");
         break;
      case 2:
         strcpy(type,"Broadband");
         break;
      default:
         strcpy(type,"Unknown");
         break;
       
   }
   printf("Console_type: %s \nMax DEC Value: %d \n", type, hw_config.sram_val[1]);
}

static void
show_GRADient()
{
   static char grad_axis[4] = { 'X', 'Y', 'Z', 'R'};
   static int  i;
 
   printf("Gradients:\n");
   for (i=0; i<4; i++)
   {
      printf("  %10c :  ",grad_axis[i]);
      switch( hw_config.gradient[i] )
      {
         case 'c':	
            printf("Performa IV \n");
            break;
         case 'd':	
            printf("Performa IV + WFG \n");
            break;
         case 'h':	
            printf("Homospoil \n");
            break;
         case 'l':	
            printf("Performa I \n");
            break;
         case 'p':	
            printf("Performa II/III \n");
            break;
         case 'q':	
            printf("Performa II/III + WFG\n");
            break;
         case 'r':	
            printf("Coordinate Rotator\n");
            break;
         case 't':	
            printf("Performa XYZ \n");
            break;
         case 'u':	
            printf("Performa XYZ + WFG \n");
            break;
         case 'w':	
            printf("WFG+GCU  \n");
            break;
         default : 
            printf("N/A\n");
            break;
      }
   }
}

static void
show_SYSTEM()
{
   printf("System : %d \n", hw_config.system);
   printf("System Version : %d \n", hw_config.SystemVer);
   printf("Interpreter Version : %d \n", hw_config.InterpVer);
}

static void
show_homodec()  /*MERCURY only*/
{
   printf("HOMO-DEC  :  ");
   if(hw_config.homodec == 1)
      printf("Present\n");
   else
      printf("NOT Present\n");
}


int main(int argc, char *argv[])
{

   if (readConsoleData(argc,argv) == -1)
      exit(1);
/* added the ability to return values to a macro by doing
/*   shell('/vnmr/bin/showconsole #'):$value
/* this way we have access to values not in global
*/
   if ((argc > 2) && strcmp(argv[2],"mercury") && strcmp(argv[2],"inova") && strcmp(argv[2],"vnmrs") )
   {  switch(argv[2][0])
      {   case '1':
		printf("%f",(float)hw_config.AMT1_conf);
		break;
      }
   }
   else
   {   if( hw_config.system == 7 )  /*MERCURY*/
       {
         if ( (argc == 3) && !strcmp(argv[2],"mercury") )
         {
           printf("valid 1\n");
           printf("rftype %s\n", (hw_config.sram_val[0] == 1) ? "ee" : "fe");
           printf("h1freq %d\n", (hw_config.H1freq >> 4) * 100);
           printf("vttype %d\n", (hw_config.VTpresent == 1) ? 2 : 0);
           printf("amptype %s\n", show_AMP(0));
           printf("shimset %d\n", hw_config.shimtype);
           printf("gradtype %c\n", hw_config.gradient[2]);
           printf("lockfreq %g\n",
                      (hw_config.sram_val[6]*65536.0 +
                      (hw_config.sram_val[7] & 0xffff) ) / 1e6 );
           printf("decmax %d\n", hw_config.sram_val[1]);
         }
         else
         {
           printf("\n");
           show_h1freq();
           show_ADC();
           show_VT();
           show_DTM();
           show_FIFO();
           show_XMTRs();
           show_AMP(1);
           show_LKfreq();
           show_SHIM();
           show_GRADient();
           show_SYSTEM();
           show_homodec();
           show_PTS_value();
         }
       } 
       else
       {
         if ( (argc == 3) && ( !(strcmp(argv[2],"inova") && strcmp(argv[2],"vnmrs")) ) )
         {
           int i, numxmtr, stmType, ampType;
           int gr;
           printf("valid 1\n");
           printf("h1freq %d\n", (hw_config.H1freq >> 4) * 100);
           printf("vttype %d\n", (hw_config.VTpresent == 1) ? 2 : 0);
           gr = hw_config.gradient[0];
           if ( (gr == 'c') || (gr == 'd') || (gr == 'h') ||
                (gr == 'l') || (gr == 'p') || (gr == 'q') ||
                (gr == 'r') || (gr == 't') || (gr == 'u') ||
                (gr == 'w') )
              printf("x_gradtype %c\n", hw_config.gradient[0]);
           else
              printf("x_gradtype n\n");
           gr = hw_config.gradient[1];
           if ( (gr == 'c') || (gr == 'd') || (gr == 'h') ||
                (gr == 'l') || (gr == 'p') || (gr == 'q') ||
                (gr == 'r') || (gr == 't') || (gr == 'u') ||
                (gr == 'w') )
              printf("y_gradtype %c\n", hw_config.gradient[1]);
           else
              printf("y_gradtype n\n");
           gr = hw_config.gradient[2];
           if ( (gr == 'c') || (gr == 'd') || (gr == 'h') ||
                (gr == 'l') || (gr == 'p') || (gr == 'q') ||
                (gr == 'r') || (gr == 't') || (gr == 'u') ||
                (gr == 'w') )
              printf("z_gradtype %c\n", hw_config.gradient[2]);
           else
              printf("z_gradtype n\n");
           numxmtr = 0;
           for (i=0; i<6; i++)
           {
              if (hw_config.xmtr_present & (1<<i) )
                 numxmtr++;
           }
           printf("numrfch %d\n", numxmtr);

           printf("shimset %d\n", hw_config.shimtype);
           printf("numrcvrs %d\n", hw_config.STM_present);
           stmType = hw_config.STM_Type[0];
           if (stmType == 5001)
           {
              printf("swmax 5e6\n");
           }
           else if (stmType == 5101)
           {
              printf("swmax 1e6\n");
           }
           else
           {
              printf("swmax 500000\n");
           }
           if (stmType == 2201)
              printf("stmmemsize 64\n");
           else if (stmType == 2301)
              printf("stmmemsize 4\n");
           else
              printf("stmmemsize 16\n");

           printf("iffreq %g\n", (float)hw_config.sram_val[IF_OFFSET]/10.0);
           printf("lockfreq %g\n",
                      (hw_config.sram_val[6]*65536.0 +
                      (hw_config.sram_val[7] & 0xffff) ) / 1e6 );
           for (i=0; i<numxmtr; i++)
           {
              if (hw_config.xmtr_present & (1<<i) )
              {
                 if ((hw_config.xmtr_stat[i]&0xF0) == 0xA0)
                 {
	            printf("rfchtype_%d H2\n", i+1);
	            printf("ptsval_%d 0\n",i+1);
                 }
                 else if ( ((1 << i) & hw_config.PTSes_present) == 0 )
                 {
	            printf("rfchtype_%d H1\n", i+1);
	            printf("ptsval_%d 0\n",i+1);
                 }
                 else
                 {
	            printf("rfchtype_%d direct\n", i+1);
	            printf("ptsval_%d %d\n",
                      i+1,hw_config.sram_val[PTS_OFFSET+i]);
                 }

                 if (!(hw_config.xmtr_stat[i] & 0x8))
                    printf("rfwg_%d y\n", i+1);
                 else
                    printf("rfwg_%d n\n", i+1);
                 if (i < 2)
                    printf("amptype_%d a\n", i+1);
                 else
                 {
		    ampType = hw_config.AMT2_conf;
                    if (ampType == 1 || ampType == 11 || ampType == 12)
                       printf("amptype_%d a\n", i+1);
		     else if (ampType == 0)
                       printf("amptype_%d l\n", i+1);
		     else if (ampType == 4 || ampType == 8)
			if (i == 2)
                          printf("amptype_%d l\n", i+1);
			else
                          printf("amptype_%d s\n", i+1);
                  }
              }
           }
         }
         else
         {
           printf("\n");
           show_h1freq();
           show_ADC();
           show_VT();
           show_DTMs();
           show_FIFO();
           show_PTS();
           show_AMT(1);
           show_AMT(2);
           show_AMT(3);
           show_ATTNs();
           show_RXcntrl();
           show_TXcntrl();
           show_LKfreq();
	   show_IFfreq();
           show_Mleg_ID();
           show_SHIM();
           show_GRADient();
           show_SYSTEM();
          }
       }
   }
   exit(0);
}

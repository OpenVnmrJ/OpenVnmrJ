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
#include <math.h>
#include <string.h>
#include <ctype.h>

/* 
   eccsend is designed to take a text file which 
   specifies ecc parameters 
   24 numbers need to be sent per channel 
   the file eccsend uses is keyed as follows 
---cooked format-----------
   channel    item  value	

   channels are 'x','X','y','Y','z','Z' 
   keys are case insensitive!
   items are values for 			key		range of values
			gain			gain		0-100.0
			duty cycle max		dutycl		0-100.0
   			slew rate		slew		0-100.0
                        diag			diag		-100 to 100

   			TC1			tc1
			TC2			tc2
			TC3			tc3
			TC4			tc4
			TC5			tc5

			AMP1			amp1		0-100.0
			AMP2			amp2		0-100.0
			AMP3			amp3		0-100.0
			AMP4			amp4		0-100.0
			AMP5			amp5		0-100.0

---raw format --------------
   channel is the same but keys change to reflect direct values for settings
                        item 			key		value range
			gain			_gain		0-0x0fff
			duty cycle max		_dutycl		0-0x0fff
   			slew rate		_slew		0-0x0fff
                        diag			_diag		0-0x0fff

   			TC1 fine		_tc1f		0-0x0fff
   			TC1 coarse		_tc1c		0-0x0fff
   			TC2 fine		_tc2f		0-0x0fff
   			TC2 coarse		_tc2c		0-0x0fff
   			TC3 fine		_tc3f		0-0x0fff
   			TC3 coarse		_tc3c		0-0x0fff
   			TC4 fine		_tc4f		0-0x0fff
   			TC4 coarse		_tc4c		0-0x0fff
   			TC5 fine		_tc5f		0-0x0fff
   			TC5 coarse		_tc5c		0-0x0fff

   			AMP1 fine		_amp1f		0-0x0fff
   			AMP1 coarse		_amp1c		0-0x0fff
   			AMP2 fine		_amp2f		0-0x0fff
   			AMP2 coarse		_amp2c		0-0x0fff
   			AMP3 fine		_amp3f		0-0x0fff
   			AMP3 coarse		_amp3c		0-0x0fff
   			AMP4 fine		_amp4f		0-0x0fff
   			AMP4 coarse		_amp4c		0-0x0fff
   			AMP5 fine		_amp5f		0-0x0fff
   			AMP5 coarse		_amp5c		0-0x0fff

One safety interlock control - all channels - for now
			safety 			safety		0-0x0ff
			
                         
---general reference----------
comments are " 
blank lines or nonsense lines are ignored
not all settings must be calculated !

eccsend infile [outfile] [flags]
if no outfile => goes to /vnmr/acqqueue/eddy.out
                                          if -N flag it prints binary data stdout
if -C check values and print errors to stderr only
if -R process and make a text out of infile.raw of raw values
if -S suppress all errors

OMASK    
bit 		use
0		print errors
1		binary out (default or specified)
2		print values to text
3		print raw replica
4		print cooked replica

default  0x3

*/
#define	ERRP	1
#define BINOUT  2
#define TOUT	4
#define RAWOUT  8
#define COOKED  16
#define TESTMASK(X)	if ((omask & X) != 0)

char archf='I';

char *strpos();
int  omask;

/*
    0 exit everything ok
   -1 not called correctly
   -2 infile not found
   -3 outfile cannot be made
   -4 raw file cannot be made
   -5 cooked file cannot be made
*/

main(argc,argv)
int argc;
char *argv[];
{ 
   char tmp[64],infile[64],outfile[64];
   int fcnt;
   char *getenv();
   fcnt = 0;
   omask = 3;
   strcpy(outfile,getenv("vnmrsystem"));
   strcat(outfile,"/acqqueue/eddy.out");
   if (argc < 2) 
   {
     send_help();
     exit(-1);
   }
   argc -= 1;
   while (argc-- > 0)
   {
     argv++;
     strcpy(tmp,*argv);
     if (tmp[0] == '-')
     {
        set_flag(tmp);
     }
     else
     {
       if (fcnt == 0)
         strcpy(infile,tmp);
       if (fcnt == 1)
         strcpy(outfile,tmp);
       fcnt++;
     }
   }
   /* argument parsing done - now doit */
   if (fcnt == 0)
   {
      TESTMASK(ERRP)
	fprintf(stderr,"no file specified");
      exit(-1);
   }
   do_it(infile,outfile);
   exit(0);
}

send_help()
{
    printf("eccsend [flags] infile [outfile]\n");  
    printf("flags are: -P print data to stdout\n");
    printf("           -C check file no binary sent\n");
    printf("           -S suppress all error output\n");
    /*
    printf("           -R make <infile.raw> from data\n");
    printf("           -B make <infile.cooked> from data\n");
    */
}

set_flag(tmp)
char *tmp;
{
   switch (tmp[1])
   {
     case 'P':   omask |= 0x05; break;
     case 'C':   omask &= 0x0D; break;
     case 'R':   omask |= 0x08; break;
     case 'B':	 omask |= 0x10; break;
     case 'S':   omask &= 0x0A; break;
     case 'I': case 'i': archf = 'I'; break;
     case 'U': case 'u': archf = 'U'; break;
   }
}

/*
if -N flag it prints binary data stdout
if -C check values and print errors to stderr only
if -R process and make a text out of infile.raw of raw values
if -S suppress all errors
bit 		use
0		print errors
1		binary out (default or specified)
2		print values to text
3		print raw replica
4		print cooked replica

default  0x3

*/

FILE *ofd;

do_it(inf,of)
char *inf,*of;
{
   FILE *fd;
   int i,iw;
   char ilb[80],*ss,*tt;
   fd = fopen(inf,"r");
   if (fd == NULL)
   { 
     fprintf(stderr,"could not find input file %s\n",inf);
     exit(-2);
   }
   ofd = NULL;
   TESTMASK(BINOUT)
   {
     ofd = fopen(of,"w"); 
     if (ofd == NULL)
     { 
       fprintf(stderr,"could not open output file %s\n",of);
       exit(-3);
     }
   }
   while (getline(fd,ilb,sizeof(ilb)) != EOF)
   {
     ss = ilb;
     do 
     {
        tt = strchr(ss,',');
        if (tt != NULL)
	   *tt = '\0';
        parse_string(ss);  /* we made a substring */ 
	ss = tt + 1;
     }
     while (tt != NULL);
   }
   TESTMASK(BINOUT)
   {
     i = 0xf0e1d2c3;
     if (iw = fwrite(&i,sizeof(i),1,ofd) != 1)
     {
	fprintf(stderr,"binary write failed!\n");
	exit(-3);
     }
     fclose(ofd);
     /* group */
   }
}

#define NKEYS 42

int base = 0;
char cbase = ' ';

static struct key
{
   char *keyw;
   int  addr;
}
keytab[NKEYS] =
{   
    "_gain",22,"_dutycl",23,"_slew", 0,"_diag",1,
    "_tc1c", 2,"_tc1f", 3,"_amp1c", 4,"_amp1f", 5,
    "_tc2c", 6,"_tc2f", 7,"_amp2c", 8,"_amp2f", 9,
    "_tc3c",10,"_tc3f",11,"_amp3c",12,"_amp3f",13,
    "_tc4c",14,"_tc4f",15,"_amp4c",16,"_amp4f",17,
    "_tc5c",18,"_tc5f",19,"_amp5c",20,"_amp5f",21,
    "gain", 22,"dutycl",23,"slew",0,"diag",1,
    "tc1", 2,"amp1", 4,
    "tc2", 6,"amp2", 8,
    "tc3",10,"amp3",12,
    "tc4",14,"amp4",16,
    "tc5",18,"amp5",20,
    "x:",0x0940,"y:",0x0948,"z:",0x0950,"safety",0x0958
};

struct tc_ref
{  
   char *usrkey,*internalkey;
   float kmax,kmin;
}
time_data[15] =
{
    "tc_x_1", "xtc1",  200.0, 22.22,
    "tc_x_2", "xtc2",   39.0,  4.33,
    "tc_x_3", "xtc3",   10.0,  1.11,
    "tc_x_4", "xtc4",    2.0,  0.222,
    "tc_x_5", "xtc5",    1.0,  0.111,
    "tc_y_1", "ytc1",  200.0, 22.22, 
    "tc_y_2", "ytc2",   39.0,  4.33,
    "tc_y_3", "ytc3",   10.0,  1.11,
    "tc_y_4", "ytc4",    2.0,  0.222,
    "tc_y_5", "ytc5",    1.0,  0.111,
    "tc_z_1", "ztc1",  200.0, 22.22,  
    "tc_z_2", "ztc2",   39.0,  4.33,
    "tc_z_3", "ztc3",   10.0,  1.11,
    "tc_z_4", "ztc4",    2.0,  0.222,
    "tc_z_5", "ztc5",    1.0,  0.111
};

parse_string(xx)
char *xx;
{   
   int j,k,ll;
   char *front,*ff;
   float y;
   double dlb;
   k = 0;
   while ((k < NKEYS) && ((front = strpos(xx,keytab[k].keyw)) == NULL)) 
      k++;
   if (k == NKEYS)
   {
     j = 0;
     while ((j < 15) && ((front = strpos(xx,time_data[j].usrkey)) == NULL))
       j++;
     if (j < 15)
     {
      ff = front + strlen(time_data[j].usrkey);
      k = sscanf(ff,"%F",&dlb);
      if (k > 0)
      {
	 time_data[j].kmax = dlb;
	 /* PFG PFG PFG */
	 time_data[j].kmin = dlb/9.0;
      }
      return;
     }
     TESTMASK(TOUT)
       printf("no match -- ignore \n");
     return;
   }
   TESTMASK(TOUT)
   {
     printf("------------------------------\n");
     printf("match  %s\n%s %x\n",xx,keytab[k].keyw,keytab[k].addr);
   }
   ff = front + strlen(keytab[k].keyw);
   if (k < 24) 
   {
      sscanf(ff,"%x",&ll);
      TESTMASK(TOUT)
        printf("raw mode addr = %x %x value %x\n",base,keytab[k].addr,ll);
      send_apb(base,keytab[k].addr,ll);
   }
   else
   {
     if (k < 38)
     {
       sscanf(ff,"%f",&y);
       TESTMASK(TOUT)
         printf("cooked mode addr =  %x  %x %f\n",base,keytab[k].addr,y);
       calc_value(base,keytab[k],y,k);
     }
     else
     {
       if (k == NKEYS-1)
       {
         if (strpos(ff,"on") == NULL)
             set_safety(keytab[k].addr,0x087);
         else
             set_safety(keytab[k].addr,0x0A7);
       }
       else
       {
         base = keytab[k].addr;
         TESTMASK(TOUT)
           printf("base = 0x%4x\n",base);
         cbase = front[0];
       }
     }
  }
  TESTMASK(TOUT)
    printf("------------------------------\n");
}

calc_value(base,ktb,y,k)
int base,k;
double y;
struct key ktb;
{
   int kappa;
   switch (k)
   {
     case 24: 
     case 25: 
     case 26:  if ((y > 100.0) || (y < 0.0))
	       {
		  TESTMASK(ERRP)
		    fprintf(stderr,"error %c %s value out of range at %f\n",cbase,ktb.keyw,y);
		  return;
               }
	       kappa = (int) floor(y *40.95 + 0.5);
	       send_apb(base,ktb.addr,kappa);
	       break;

     case 27:  if ((y > 100.0) || (y < -100.0))
	       {
		  TESTMASK(ERRP)
		  fprintf(stderr,"error %c %s value out of range at %f\n",cbase,ktb.keyw,y);
		  return;
               }
	       kappa = (int) floor((y + 100.0)*20.47+0.5);
	       send_apb(base,ktb.addr,kappa);
	       break;
     case 29:
     case 31:
     case 33:
     case 35:
     case 37:  if ((y > 100.0) || (y < 0.0))
	       {
		  TESTMASK(ERRP)
		  fprintf(stderr,"error %c %s value out of range at %f\n",cbase,ktb.keyw,y);
		  return;
               }
	       calc_amp(base,ktb.addr,y);
	       break; /* unfinished */
     case 28:
     case 30:
     case 32:
     case 34:
     case 36:  calc_tc(base,ktb,y);
	       break;
     default:  TESTMASK(ERRP)
		  printf("default case\n");
     }
}

calc_tc(bs,ktb,y)
int bs;
struct key ktb;
double y;
{
   int i,k;
   char dblook[16];
   double yy,zz;
   dblook[0] = cbase;
   dblook[1] = '\0';
   strcat(dblook,ktb.keyw);
   i = 0;
   while ((i < 15) && (strcmp(dblook,time_data[i].internalkey) != 0))
      i++;
   if (i > 14)
   {
      TESTMASK(ERRP)
        fprintf(stderr,"look up error help\n");
      return;
   }
   if ((y > time_data[i].kmax) || (y < time_data[i].kmin))
   {
     TESTMASK(ERRP)
     {
       fprintf(stderr,"error %c %s value out of range at %f\n",cbase,ktb.keyw,y);
       fprintf(stderr,"range is [%5.1f,%5.1f]\n",time_data[i].kmin,time_data[i].kmax);
     }
     return;
   }
   /* now calculate */
   /* calculate the quantity (tcb/O - 1.0)/G */
   /* G = 8.0 */
   zz = (time_data[i].kmax/y - 1.0)/8.0;
   /* SAVE ZZ */
   yy = (zz - 0.008078)* 4095.0;
   yy += 0.5;
   if (yy >= 4095.0)
      k = 4095;
   else if (yy < 0)
      k = 0;
   else
      k = (int) yy;
   send_apb(base,ktb.addr,k);
   yy = 253542.0*(zz - ((double) k) /4096.0);
   yy += 0.5;
   if (yy >= 4095.0)
     k = 4095;
   else if (yy < 0)
     k = 0;
   else
      k = (int) yy;
   send_apb(base,ktb.addr+1,k);
}


calc_amp(base,off,y)
int base,off;
float y;
{
     /* equation
     O = SF*( crs/4096 + fin/253542)
     O/SF =  crs/4096 + fin/253542
     crs = 4096 * (O/SF - fin/253542)
     sf = 98.41  O max = 100.0 %
     */
     int k;
     double xx,yy;
     /* now calculate */
     /* should check status here */
     xx = y;
     xx /= 98.41; 
     /* convert to fraction scale 
	xx is now target - no reuse */
     yy = 4095.0*(xx - 0.008078);
     yy += 0.5;
     if (yy >= 4095.0)
       k = 4095;
     else if (yy < 0)
       k = 0;
     else
        k = (int) yy;
     send_apb(base,off,k);
     /* now do fine */
     yy = 253542.0*(xx - ((double) k ) /4096.0);
     yy += 0.5;
     if (yy >= 4095.0)
       k = 4095;
     else if (yy < 0)
       k = 0;
     else
        k = (int) yy;
     send_apb(base,off+1,k);
}

set_safety(where,what)
int where,what;
{
   short obuff[4],msk,iw;
   msk = where & 0x0f00;
   obuff[0] = 2;
   obuff[1] = 0x3001;
   obuff[2] = 0xA000 | where;
   obuff[3] = 0xB000 | msk | what;
   TESTMASK(BINOUT)
   {
      if (iw = fwrite(obuff,sizeof(obuff),1,ofd) != 1)
      {
	fprintf(stderr,"binary write failed!\n");
	exit(-3);
      }
   }
}

send_apb(base,reg,data12)
int base, reg, data12;
{
  if (archf != 'U')
   INsend_apb(base,reg,data12);
  else
   UPsend_apb(base,reg,data12);
}

/*  INOVA ONLY GCU */
INsend_apb(base,reg,data12)
int base, reg, data12;
{  
   short obuff[10],msk,iw;
   int i;
   msk = base & 0x0f00;
   obuff[0] = 8;
   obuff[1] = 0x3001;   /* is actually skipped */
   obuff[2] = 0xA003 | base;
   obuff[3] = 0xB000 | msk ;   /* a zero to base + 3 */
   obuff[4] = 0xA000 | base;   /* register goes to base addr */
   obuff[5] = 0xB000 | msk | (reg & 0x0ff);
   obuff[6] = 0x9000 | msk | (data12 & 0x0ff);
   obuff[7] = 0x9000 | msk | ((data12 >> 8) & 0x00f);
   obuff[8] = 0xA000 | base;
   obuff[9] = 0xB0ff | msk ;   /* select a non-existant chip */
   TESTMASK(TOUT)  
   {
   for (i = 0; i < 10; i++ ) 
      printf("%x    ",((int) obuff[i]) & 0x0ffff);
   printf("\n");
   }
   TESTMASK(BINOUT)
   {
      if (iw = fwrite(obuff,sizeof(obuff),1,ofd) != 1)
      {
	fprintf(stderr,"binary write failed!\n");
	exit(-3);
      }
   }
}
UPsend_apb(base,reg,data12)
int base, reg, data12;
{  
   short obuff[7],msk,iw;
   int i;
   msk = base & 0x0f00;
   obuff[0] = 5;
   obuff[1] = 0x3001;
   obuff[2] = 0xA000 | base;
   obuff[3] = 0xB000 | msk | (reg & 0x0ff);
   obuff[4] = 0x9000 | msk | (data12 & 0x0ff);
   obuff[5] = 0x9000 | msk | ((data12 >> 8) & 0x00f);
   obuff[6] = 0x9000 | msk;
   TESTMASK(TOUT)
   {
   for (i = 0; i < 7; i++ ) 
      printf("%x    ",((int) obuff[i]) & 0x0ffff);
   printf("\n");
   }
   TESTMASK(BINOUT)
   {
      if (iw = fwrite(obuff,sizeof(obuff),1,ofd) != 1)
      {
	fprintf(stderr,"binary write failed!\n");
	exit(-3);
      }
   }
}

/* 
phil's utilities 
*/

int getline(fd,targ,mxline)
FILE *fd;
char *targ;
int mxline;
{   
   char uu,*tt;
   int cnt,tmp;
   tt = targ;
   cnt = 0;
   do
   {  
    tmp = getc(fd);
    uu = (char) tmp;
    if (isupper(uu))
      uu = tolower(uu);
    if (cnt < mxline)
      *tt++ = uu;    /* read but discard chars after mxline */
    cnt++;
   }
   while ((tmp != EOF) && (uu != '\n'));
   if (cnt >= mxline)
      targ[mxline -1] = '\0';
   else 
      *--tt = '\0';	/* ensure a null terminated list */
   if (tmp == EOF)
     return(EOF);
   else
     return(cnt);
}

char *strpos(target,search)
char *target,*search;
{
   char *base,*pntr,*pntr1,*pntr2;
   base = target;
   if (target == NULL) return(NULL);
   if (search == NULL) return(NULL);
   while ((pntr = strchr(base,*search)) != 0) 
   { /* first char matches */
     pntr1 = pntr;
     pntr2 = search;
     while ((*pntr2 != '\0') && (*pntr1 != '\0') && (*pntr1++ == *pntr2))
      pntr2++;
     if (*pntr2 == '\0') return pntr;
     base = pntr + 1;
   } 
   return NULL;
}


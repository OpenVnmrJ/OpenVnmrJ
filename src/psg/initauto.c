/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#ifdef __INTERIX
#include <arpa/inet.h>
#endif
#include <netinet/in.h>
#include "acodes.h"
#include "rfconst.h"
#ifdef MERCURY
#include "acqparms2.h"
#else
#include "acqparms.h"
#endif
#include "group.h"
#include "shims.h"
#include "abort.h"
#include "cps.h"

#ifdef MERCURY
extern char interLock[];
extern int  whenshim;
#endif

extern double getGain();
extern void init_auto_pars(int var1,int var2,int var3,
         int var4,int var5,int var6,int var7,int var8);
extern int getFirstActiveRcvr();

/*-----------------------------------------------------------------
|
|	initautostruct()
|	initialize automation structure which is appended to each
|	Acode section.
|
|				Author Greg Brissey 6/30/86
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/29/89   Greg B.    1. Use new global parameters to initial lc & 
|			    calc.  acode offsets 
|
+---------------------------------------------------------------*/

#ifdef DEBUG
extern int      bgflag;

#define DPRINT0(str) \
        if (bgflag) fprintf(stderr,str)
#define DPRINT1(str, arg1) \
        if (bgflag) fprintf(stderr,str,arg1)
#define DPRINT2(str, arg1, arg2) \
        if (bgflag) fprintf(stderr,str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
        if (bgflag) fprintf(stderr,str,arg1,arg2,arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4) \
        if (bgflag) fprintf(stderr,str,arg1,arg2,arg3,arg4)
#else
#define DPRINT0(str)
#define DPRINT1(str, arg2)
#define DPRINT2(str, arg1, arg2)
#define DPRINT3(str, arg1, arg2, arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4)
#endif

#define DONE_SH     0
#define LOCK_SH     1
#define FID_SH      2
#define SEL_COIL    3
#define DELAY       4
#define SPIN_ON     5
#define SPIN_OFF    6
#define MAX_TIME    7
#define QUICK_SH    8

extern double getval();
extern double sign_add();

static int com_len;
static codeint com_str[64];
static codeint quick_str[64];
static int quick_len;

#ifdef DEBUG
/*
 *  aut_parse and aut_parse_string model the interpretation of a
 *  shim method by autshm.  It is included here as an aid to debugging.
 */
int aut_parse(char *pp, int *inc)
{
   int  flag;
   int  tmp;
   int  delaytime;
   int  maxtime;
   long active;
   char criteria,f_crit;

   flag = 0;
   *inc = 0;
   switch (*pp++)
   {
      case LOCK_SH:		/* shim on the lock signal */
	 DPRINT0("Lock Shim Selected\n");
	 break;
      case FID_SH:		/* shim on the fid */
         {
            int fid_start = (int) *pp++;
            int fid_end = (int) *pp++;
            *inc += 2;
	    DPRINT2("Fid Shim Selected with limits %d and %d\n",
                     fid_start,fid_end);
         }
	 break;
      case QUICK_SH:		/* shim on the lock signal */
	 DPRINT0("Quick Shim Selected\n");
	 break;
      case SEL_COIL:		/* shim coils to shim and then do simplex */
         tmp = *pp++;
	 active =  (long) ((*pp++ << 8) & 0xff00);
	 active |= (long) ((*pp++)       & 0x00ff);
         active = (active << 16);
	 active |= (long) ((*pp++ << 8)  & 0xff00);
	 active |= (long) ((*pp++)       & 0x00ff);
	 criteria = *pp++;	/* for method shim */
	 f_crit = *pp++;	/* for method shim */
         *inc += 7;
	 flag |= 1;
	 DPRINT2("Select coils (new = %d) 0x%x and ", tmp, active);
	 DPRINT2("criteria %c and %c\n", criteria,f_crit);
	 break;
      case DELAY:
         delaytime =  (int) ((*pp++ << 8) & 0xff00);
         delaytime |= (int) ((*pp++)      & 0x00ff);
         *inc += 2;
	 DPRINT1("delay time is %d hundredths of a sec\n", delaytime);
	 break;
      case SPIN_ON:
	 DPRINT0("Spin on\n");
	 flag |= 2;
	 break;
      case SPIN_OFF:
	 DPRINT0("Spin off\n");
	 flag |= 4;
	 break;
      case MAX_TIME:		/* Max time for shimming */
	 maxtime =  (int) ((*pp++ << 8) & 0xff00);
	 maxtime |= (int) ((*pp++)      & 0x00ff);
         *inc += 2;
	 DPRINT1("Maximum Shimming Time is %d sec\n", maxtime);
	 break;

   }
   *inc += 1;
   return(flag);
}

static void
aut_parse_string()
{
   int temp, incr;
   char *pp;

   pp = (char *) &com_str[0];
   temp = com_len * sizeof(codeint);
   DPRINT1("Start parse_string %d chars\n",temp);
   /* for debugging, only looking at first 64 instructions */
   while (temp > 0 ) 
   {
      incr = (int) *pp++;
      DPRINT1("%d ",incr);
      temp--;
   }
   DPRINT0("\n\n");
   pp = (char *) &com_str[0];
   
   while (*pp != DONE_SH) 
   {				/* while commands left to parse */

      temp = aut_parse(pp,&incr);
      DPRINT2("-----   Action is %d incr is %d\n", temp, incr);
      pp += incr;
   }				/* end of while # 1 */
   DPRINT0("Done parse_string\n");
   if (quick_len)
   {
      pp = (char *) &quick_str[0];
      temp = quick_len * 2;
      DPRINT1("Start parse_string %d chars for quick shim\n",temp);
      /* for debugging, only looking at first 64 instructions */
      while (temp > 0 ) 
      {
         incr = (int) *pp++;
         DPRINT1("%d ",incr);
         temp--;
      }
      DPRINT0("\n\n");
      pp = (char *) &quick_str[0];
   
      while (*pp != DONE_SH) 
      {				/* while commands left to parse */

         temp = aut_parse(pp,&incr);
         DPRINT2("-----   Action is %d incr is %d\n", temp, incr);
         pp += incr;
      }				/* end of while # 1 */
      DPRINT0("Done parse_string for quick shim\n");
   }
}

#endif

static int
parse_int(int *val, char *exc)
/* returns number of characters in val +1 if ok, sets val */
{
   int             tt;

   char            tmp[80];
 
   tt = 0;
   while (((*exc >= '0') && (*exc <= '9')) || (*exc == '-'))
      tmp[tt++] = *exc++;
   if (tt == 0)
      return (-1);
   else
   {   
      tmp[tt] = '\0';           /* close off tmp string */
      *val = atoi(tmp);
      tt++;
      return (tt);
   }
}

/* returns number of characters in val. if ok, sets val */
/* decimal        mode = 1 decimal, mode = 0 hexidecimal */
static int
parse_lng(unsigned int *val, char *exc, int decimal)
{
   int             tt;
   int             sign,
                   digit;
   long            value;
 
   sign = 1;
   value = 0L;
   tt = 0;
   if (*exc == '-')
   {
      sign = -1;
      tt++;
      exc++;
   }
   if (decimal)
   {
      while ((*exc >= '0') && (*exc <= '9'))
      {
         value = value * 10L + (*exc++ - '0');
         tt++;
      }
   }   
   else
   {
      while (TRUE)
      {

         if ((*exc - '0') > 9)
         {
            if ((*exc >= 'a') && (*exc <= 'f'))
            {
               digit = (*exc++ - 'a') + 10;
               tt++;
            }
            else
            {
               if ((*exc >= 'A') && (*exc <= 'F'))
               {
                  digit = (*exc++ - 'A') + 10;
                  tt++;
               }
               else
                  break;        /* not hex char so done */
            }
         }
         else
         {
            if (*exc >= 0)
            {
               digit = *exc++ - '0';
               tt++;
            }
            else
               break;           /* char < 0 so done */
         }
         value = value * 16L + (long) digit;
      }
   }   
   if (tt == 0)

      return (0);
   else
   {
      *val = value * (long) sign;
      return (tt);
   }   
}
 
static void checkactive(unsigned int *mask)
{
   int index;
   const char *sh_name;
/*
 *  NOTE:  this method of passing shim masks only works for long ints
 *         and is therefore restricted to 32 dacs.
 */

   for (index= Z0 + 1; index < 32; index++)
   {
      if ((sh_name = get_shimname(index)) == NULL)
         *mask &= ~(1 << index);
   }
   *mask &= ~3;  /* turn off Z0 */
}

/*
      27  25        20        15        10         5         0   (decmal)
 
       1 1 1 1 1 1 1 1 1 1 1 1
       B A 9 8 7 6 5 4 3 2 1 0 F E D C B A 9 8 7 6 5 4 3 2 1 0   COIL # (hex)
       ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
       Z X Z Y Y X Y X X X Y X - - - - - - - Z Z Z Z Z Z Z Z -
       X Z X Z 3 3 Z 2 Y Z                   5 4 3 2 2 1 1 0
       Y 2 2 2       |                             c   c
           |         Y
           Y         2
           2
 
      |_______|_______|_______|_______|_______|_______|_______|
 
*/
static int
selectcoil(char *py, unsigned int *mask, int shimset)
{
   register char   tt,
                   ty;
   int             kk;
   unsigned int    active;
   int             mStyle = 0;
 
   kk = 0;
   tt = *py++;
   ty = *py++;
   switch (tt)
   {
   case 'z':
   case 'Z':
      switch (ty)
      {
      case '1':
         active = 0x0cL;
         break;                 /* Z1 and Z1c */
      case '2':
         active = 0x30L;
         break;                 /* Z2  and Z2c */
      case '3':
         active = 0x40L;
         break;                 /* Z3 */
      case '4':
         active = 0x80L;
         break;                 /* Z4 */
      case '5':
         active = 0x100L;
         break;                 /* Z5 */
      case 'a':
      case 'A':
         active = 0x1fcL;
         break;                 /* Z1c,2c,3,4,5 */
      case 'b':
      case 'B':
         active = 0xbcL;
         break;                 /* Z1c,2c,4 */
      case 'e':
      case 'E':
         active = 0xb0L;
         break;                 /* Z2c,4 */
      case 'o':
      case 'O':
         active = 0x14cL;
         break;                 /* Z1c,3,5 */
      case 'q':
      case 'Q':
         active = 0x3cL;
         break;                 /* Z1c,2c */
      case 't':
      case 'T':
         active = 0x7cL;
         break;                 /* Z1c,2c,3 */
      case 'c':
      case 'C':
         active = 0x13cL;
         break;                 /* Z1c,2c,5 */
      case 'm':
      case 'M':
         mStyle = 1;
         kk = parse_lng(&active, py, 0); /* You,choose */
         break;
      default:
         active = 0L;
      }
      break;
   case 't':
   case 'T':
      switch (ty)
      {
      case 'x':
      case 'X':
         active = 0x10004L;
         break;                 /* Z1f,X */
 
      case 'y':
      case 'Y':
         active = 0x20004L;
         break;                 /* Z1f,Y */
 
      case '1':
         active = 0x30004L;
         break;                 /* Z1f,XZ */

      case '2':
         active = 0x3b0004L;
         break;                 /* Z1f,X,Y,XY, */
         /* X2-Y2,YZ */
 
      case 'z':
      case 'Z':
         active = 0x270004L;
         break;                 /* Z1f,X,Y,XZ, */
         /* YZ */
 
      case 'a':
      case 'A':
         active = 0x53f0004L;
         break;                 /* Z1f,X,Y,XZ, */
         /* XY,X2-Y2, */
         /* YZ,YZ2,XZ2 */
 
      case 't':
      case 'T':
         active = 0x3f0004L;
         break;                 /* Z1f,X,Y,XZ, */
         /* XY,X2-Y2,YZ */
 
      case '3':
         active = 0x5030004L;
         break;                 /* Z1f,X,Y, */
         /* YZ2,XZ2 */
 
      case '4':
         active = 0x4450004L;
         break;                 /* Z1f,X,XZ, */
         /* XZ2,X3 */
 
      case '5':
         active = 0x1A20004L;

         break;                 /* Z1f,Y,YZ, */
         /* YZ2,Y3 */
 
      case '6':
         active = 0xA180004L;
         break;                 /* Z1f,XY, */
         /* X2-Y2, */
         /* ZXY,Z(X2-Y2) */
 
      case '7':
         active = 0xfff0004L;
         break;                 /* Z1f,X,Y,XZ, */
         /* XY,X2-Y2,YZ, */
         /* X3,Y3,YZ2, */
         /* Z(X2-Y2), */
         /* XZ2,ZXY */
 
      case 'm':
      case 'M':
         mStyle = 1;
         kk = parse_lng(&active, py, 0); /* You,choose */
         break;
      default:
         active = 0L;
      }
      break;
   default:
      active = 0L;
   }
   if (!mStyle && (shimset == 5))
      active &= 0xffffffd7;
   *mask = active;
   checkactive(mask);
   return(kk + 2);
}
 
static char
meth_set_crit(char cc)               /* for method shimming ( passes ascii ) */
{
   char            tmp;
 
   switch (cc)
   {
   case 'B':
   case 'b':
      tmp = 'b';
      break;   
   case 'L':
   case 'l':
      tmp = 'l';
      break;   
   case 'M':
   case 'm':
      tmp = 'm';
      break;
   case 'T':
   case 't':
      tmp = 't';
      break;
   case 'E':
   case 'e':
      tmp = 'e';
      break;
   default:
      tmp = 'l';
   }
   return (tmp);
}

void
translate_string(int shimset)
{
   char comstring[MAXSTR];	/* shimming background command string */
   char *pp, *out_str;
   int fid_start = 0;
   int fid_end = 100;
   unsigned int mask;
   char s_crit = 'm';
   char f_crit = 'm';
   int kk,tmp;
   char *com_ptr;
   char *q_ptr;

   getstr("com$string",comstring);
   out_str = com_ptr = (char *) &com_str[0];
   pp = comstring;
   q_ptr = 0;
   quick_len = 0;
   DPRINT0("start translate_string(): \n");
   DPRINT1("val = ' %s ' \n",pp);

   while (*pp != '\0')
   {
      switch (*pp++)
      {
         case 'L':
         case 'l':   *out_str++ = LOCK_SH;
                     DPRINT0("set lock shim(): \n");
                     break;
         case 'F':
         case 'f':   *out_str++ = FID_SH;
                     DPRINT0("set fid shim ");
                     if (*pp == ':')
                     {
                        pp++;
                        kk = parse_int(&tmp, pp);   /* get fid start limit */
                        if (kk > 0 && tmp >= 0 && tmp < 100)
                        {
                           fid_start = tmp;
                           pp += kk - 1;
                           if (*pp == ',')
                           {
                               pp++;
                               kk = parse_int(&tmp, pp);    /* get fid stop limit */
                               if (kk > 0 && tmp > fid_start && tmp <= 100)
                               {
                                  fid_end = tmp;
                                  pp += kk - 1;
                               }
                           }    
                        }
                     }
                     DPRINT2("with limits %d and %d\n",fid_start,fid_end);
                     *out_str++ = (char) fid_start;
                     *out_str++ = (char) fid_end;
                     break;
         case 'Q':
         case 'q':
                     q_ptr = out_str;
                     quick_len = 0;
                     *out_str++ = QUICK_SH;
                     DPRINT0("set quick shim(): \n");
                     break;
         case 'S':
         case 's':
                     DPRINT0("select coil");
                     *out_str++ = SEL_COIL;
                     kk = selectcoil(pp,&mask,shimset);
                     pp += kk;
                     *out_str++ = (char) (mask == 0);
                     *out_str++ = (char) ((mask & 0xff000000) >> 24);
                     *out_str++ = (char) ((mask & 0x00ff0000) >> 16);
                     *out_str++ = (char) ((mask & 0x0000ff00) >> 8);
                     *out_str++ = (char) (mask & 0x000000ff);
                     DPRINT1(" mask 0x%x and criteria",mask);
                     if (*pp == ':')
                     {
                        pp++;
                        if ((*pp == 'c') || (*pp == 'C'))
                        {
                           pp++;
                           s_crit = (char) meth_set_crit(*pp++);
                           f_crit = (char) meth_set_crit(*pp++);
                        }
                     }
                     DPRINT2(" %c and %c\n",s_crit,f_crit);
                     *out_str++ = (char) s_crit;
                     *out_str++ = (char) f_crit;
                     break;
         case 'D':
         case 'd':  
                     DPRINT0("set delay");
                     if (((kk = parse_int(&tmp, pp)) > 0) && (tmp > 0) && (tmp < 2000))
                     {
                        DPRINT1(" of %d hundredths of a sec.",tmp);
                        *out_str++ = DELAY;
                        *out_str++ = (char) ((tmp & 0xff00) >> 8);
                        *out_str++ = (char) (tmp & 0x00ff);
                        pp += kk - 1;
                     }
                     DPRINT0("\n");
                     break;
         case 'R':
         case 'r':
                     switch (*pp++)
                     {
                        case 'y':
                        case 'Y':
                                   DPRINT0("set spin on: \n");
                                   *out_str++ = SPIN_ON;
                                   break;  
                        case 'n':
                        case 'N':
                                   DPRINT0("set spin off: \n");
                                   *out_str++ = SPIN_OFF;
                                   break;  
                     }
                     break;
         case 'T':         /* Max time for shimming */
         case 't':
                     DPRINT0("set max time");
                     /* time > 0 & < 2 hours */
                     if ((kk = parse_int(&tmp, pp)) > 0 && tmp >= 0 && tmp <= 7200)
                     {
                        DPRINT1(" of %d sec.",tmp);
                        *out_str++ = MAX_TIME;
                        *out_str++ = (char) ((tmp & 0xff00) >> 8);
                        *out_str++ = (char) (tmp & 0x00ff);
                        pp += kk - 1;
                     }
                     DPRINT0("\n");
                     break;
         case ';':
                     DPRINT0("finish method\n");
                     *out_str++ = DONE_SH;
                     if (q_ptr && !quick_len)
                     {
                        quick_len = out_str - q_ptr;
                     }
                     break;
          default:
                     DPRINT1("unrecognized command ' %s '\n",pp);
      }
      if (*pp == ',')
	 pp++;
   }
   *out_str++ = DONE_SH;
   com_len = out_str - com_ptr;
   if (q_ptr && !quick_len)
   {
      quick_len = out_str - q_ptr;
   }
   if (com_len == 1)
      com_len = 0;
   else
      com_len = (com_len + sizeof(codeint) - 1 ) / sizeof(codeint);
   *out_str++ = DONE_SH;
   if (quick_len <= 1)
   {
      quick_len = 0;
   }
   else
   {
      char *tmpPtr;
      int i;

      tmpPtr = (char *) &quick_str[0];
      for (i=0; i < quick_len; i++)
         *tmpPtr++ = *q_ptr++;
      *tmpPtr++ = DONE_SH;
      *tmpPtr++ = DONE_SH;
      
      quick_len = (quick_len +  sizeof(codeint) - 1 ) / sizeof(codeint);
   }
   DPRINT1("end translate_string() %d chars: \n",com_len * sizeof(codeint));
#ifdef DEBUG
   aut_parse_string();
#endif
}

#ifdef NVPSG
void putmethod(int mode, int buffer[])
{
   int temp;

   buffer[0] = whenshim;
   buffer[1] = mode;
   buffer[2] = com_len;
   for (temp = 0; temp < com_len; temp++)
      buffer[temp+3] = htonl(com_str[temp]);
}
#else
void putmethod(int mode)
{
   int temp;

   putcode(SHIMA);		/* Acode for auto shimming */
   putcode( (codeint) mode);		/* Acode for auto shimming */
   if (quick_len && (ix > 1) )
   {
      putcode( (codeint) quick_len);
      temp = 0;
      for (temp=0; temp < quick_len; temp++)
         putcode( (codeint) htons(quick_str[temp]));
   }
   else
   {
      putcode( (codeint) com_len);
      temp = 0;
      for (temp=0; temp < com_len; temp++)
         putcode( (codeint) htons(com_str[temp]));
   }
}
#endif

void initautostruct()
{
    double dgain;
    double lockpower;
    double lockgain;
    double lockphase;
    double z0;
    int mask;
    int shimset;

    if (getparm("lockpower","real",GLOBAL,&lockpower,1))
	psg_abort(1);
    if (lockpower < 0.5)
    {
       interLock[0] = 'n'; /* No lock interlock if lockpower = 0 */
    }
    if (getparm("lockgain","real",GLOBAL,&lockgain,1))
	psg_abort(1);
    if (getparm("lockphase","real",GLOBAL,&lockphase,1))
	psg_abort(1);
    if (getparm("z0","real",GLOBAL,&z0,1))
	psg_abort(1);
    shimset = mapShimset(init_shimnames(GLOBAL));
    mask = shimset << 8;
    mask += 6;
    translate_string(shimset);
    /* This sets gain bits in 1st rcvr and preamp attenuation bits.
     * If 1st rcvr is inactive, we don't care about its gain, but
     * we still need to set the preamp attn to an appropriate value.
     */
    dgain = getGain(getFirstActiveRcvr());
    init_auto_pars(
                             (codeint) sign_add(lockpower,0.005),
                             (codeint) sign_add(lockgain,0.005),
                             (codeint) sign_add(lockphase,0.005),
                             (codeint) sign_add(z0,0.005),
                             (codeint) mask,
                             (codeint) whenshim,
                             (codeint) sign_add(dgain, 0.0005),
                             (codeint) loc
       );

}

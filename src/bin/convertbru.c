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
/*************************************************************************
*									 *
*  convertbru.c:							 *
*									 *
*  This is a UNIX program to convert Bruker data files to a form that	 *
*  the VNMR program "sread" can understand. "Sread" reads the data	 *
*  into the current experiment.						 *
* 									 *
*  Bruker data is stored on tape in blocks of 532 24-bit words. The	 *
*  first block contains parameters. The data is placed in a unix file	 *
*  with the suffix '.bru' by "btape" without being converted. This	 *
*  program reads some of the parameters and writes them out to "stext."	 *
*  It then converts the data from 24 bit integers to 32 bit integers.	 *
*  The data is read in blocks of four and written out in blocks of 4. 	 *
*									 *
*  Every 2nd and 3rd data point is negated.  The data can then be	 *
*  transformed by the Varian FT followed by POSTPROC.  If every 3rd and	 *
*  4th data point is negated and zeros inserted after all data points,	 *
*  the Varian FT will produce a spectrum with a mirror image without	 *
*  using POSTPROC.							 *
* 									 *
*************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#ifdef VMS
#include stat
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

#define TRUE		(0==0)
#define FALSE		!TRUE
#define OK		FALSE
#define ERROR		TRUE

#ifdef VMS
#define OK_EXIT		TRUE
#define BAD_EXIT	FALSE
#else
#define OK_EXIT		FALSE
#define BAD_EXIT	TRUE
#endif

#define MAXPATHL	128
#define MAXNUMDELAYS	10
#define MAXNUMPULSES	10
#define MAXNUMPOWERS	10
#define MINFN		128
#define HIGHPOWER	6208
#define LOWPOWER	4120
#define MAXNI		16384
#define MINFREQ		10.0
#define MAXFREQ		640.0
#define MIN_DR		4
#define MAX_DR		24

#define FN_OFFSET	0		/* offset into fourier number        */
#define CT_OFFSET	2		/* offset into completed transients  */
#define NP_OFFSET	3		/* offset into number of data points */
#define DW_OFFSET	4		/* offset into dwell time	     */
#define FB_OFFSET	5 		/* offset into filter bandwidth	     */
#define SS_OFFSET	12 		/* offset into steady-state count    */
#define SW_OFFSET	13		/* offset into F2 spectral width     */
#define AQ_OFFSET	15		/* offset into AQ_mode               */
#define NI_OFFSET	16		/* offset into number of increments  */
#define TOF_OFFSET	17		/* offset into xmtr offset (O1)	     */
#define DOF_OFFSET	19		/* offset into dec offset (O2)	     */
#define RD_OFFSET	27		/* offset into relaxation delay	     */
#define PW_OFFSET	28		/* offset into main pulse width	     */
#define DE_OFFSET	29		/* offset into dead time	     */
#define NT_OFFSET	30		/* offset into number of transients  */
#define DR_OFFSET	31		/* offset into digitizer resolution  */
#define SATDLY_OFFSET	34		/* offset into saturation delay      */
#define SFRQ_OFFSET	37		/* offset into xmtr frequency	     */
#define DECPWR_OFFSET	41		/* offset into dec power level	     */
#define TEMP_OFFSET	45		/* offset into temperature	     */
#define RG_OFFSET	48		/* offset into receiver gain	     */
#define DW1_OFFSET	54		/* offset into t1 dwell time	     */
#define SF0_OFFSET	58		/* offset into spec frequency        */
#define DFRQ_OFFSET	60		/* offset into dec frequency	     */
#define DELAY_OFFSET	69		/* offset into delays D0-D9	     */
#define POWER_OFFSET	79		/* offset into powers S0-S9	     */
#define PULSE_OFFSET	89		/* offset into pulses P0-P9	     */
#define SPIN_OFFSET	100		/* offset into spin word	     */
#define DATE_OFFSET	105		/* offset into date      	     */
#define TIME_OFFSET	106		/* offset into time      	     */

#define AM_SIZE		 3		/* no. of bytes in AM "int" */
#define X32_SIZE	 4		/* no. of bytes in X32 "int" */
#define MSB_FIRST	 0		/* most-significant byte first */
#define LSB_FIRST	 1		/* least-significant byte first */

#define HDR_SIZE	216		/* number of AM_SIZE or X32_SIZE words
					   in header */
#define HDR_NO_SHIFT	 0		/* FN_OFFSET starts at byte 0 of hdr*/
#define HDR_SHIFT	 40		/* FN_OFFSET starts at byte 40 of hdr*/
#define EXTRA_NP_OFFSET -35		/* if HDR_SHIFT, then NP also is at
					   byte 5 (=HDR_SHIFT+EXTRA_NP_OFFSET)*/
#define TPPI		 1
#define ABS		 2
#define FILE_INFO_UNSET -1

#define AM_SPEC		 0
#define AMX_SPEC	 1
#define AMX_HIGHPOWER	 44

#define HDR_ONLY	 1
#define DATA_ONLY	 2
#define HDR_AND_DATA	 3

#define OUT_BLOCK_SIZE	 256		/* block size of output buffer */
#define SUN_INT_SIZE	 4		/* no. of bytes in SUN int */

#define SIMULTANEOUS_DATA	1	/* values of AQ_mod for simultaneously
					   sampled data */
#define SEQUENTIAL_DATA		2	/* values of AQ_mod for sequentially
					   sampled data */
#define MAX_PAR_LEN	 20
#define MAX_BRU_PAR	 10

typedef struct {
	char in_file[MAXPATHL], out_dir[MAXPATHL];
	int word_size,		/* AM_SIZE or X32_SIZE */
	    byte_order,		/* MSB_FIRST or LSB_FIRST */
	    hdr_shift,		/* HDR_SHIFT or HDR_NO_SHIFT */
	    hdr_blocks,		/* number of header blocks (1 or 2) */
	    file_type,		/* HDR_ONLY, DATA_ONLY, or HDR_AND_DATA */
	    skip_words,	/* if DATA_ONLY, # of words to skip at start of file */
	    data_format,	/* TPPI or ABS */
	    overwrite_flag,	/* TRUE to overwrite existing file */
	    spectrometer,	/* type of spectrometer (AM_SPEC or AMX_SPEC) */
	    twod,		/* TRUE if 2d data set */
	    aqmod;		/* = 2 if simultaneously acquired data
				   = 1 if sequentially acquired data */
	double dec_frq;		/* decoupler freq. */
	} file_info_struct;

typedef struct {
   int		fn,
		fn1,
		np,
		nt,
		ct,
		ni,
		ss,
		spin,
		date,
		time,
		dpwr,
		rg,
		s[MAXNUMPOWERS];
   double	sw1,
		sw,
		pw,
		pw90,
		temp,
		dw,
		de,
		fb,
		rd,
		tof,
		dof,
		sfrq,
		dfrq,
		dfrqval,
		in,
		at,
		satdly,
		delay[MAXNUMDELAYS],
		pulse[MAXNUMPULSES];
    char	tn[MAX_PAR_LEN],
		dn[MAX_PAR_LEN],
		seqfil[MAX_PAR_LEN];
    int		num_bruker_par, num_skip_par;
    char	bruker_par_list[MAX_BRU_PAR][MAX_PAR_LEN];
    char	skip_par_list[MAX_BRU_PAR][MAX_PAR_LEN];
   } data_struct;


#ifdef DEBUG
int	debug = TRUE;
#else
int	debug = FALSE;
#endif

extern double	atof();


/*---------------------------------------
|					|
|		main()/2		|
|					|
+--------------------------------------*/
int  main(argc, argv)
int	argc;
char	*argv[];
{
   file_info_struct *file_info;
   char buf[100010];
   FILE *fp;
   int i;
   data_struct *data;

   file_info = (file_info_struct *)malloc(sizeof(file_info_struct));
   init_file_info(file_info);
   if (parse_args(argc,argv,file_info))
     return(BAD_EXIT);
   if ((file_info->file_type != DATA_ONLY))  {
     if (file_info->spectrometer == AM_SPEC)  {
       if (get_file_format(file_info))  {
	 rmdir(file_info->out_dir);
         return(BAD_EXIT);
	 }
       printf("AM data.\n");
       printf("Word size = %d bytes, Byte order = %s first, ",
		file_info->word_size,(file_info->byte_order == MSB_FIRST) ?
		"MSB" : "LSB");
       if (file_info->file_type == DATA_ONLY)
         printf("Header size = %d words\n",(file_info->skip_words > 0) ?
		file_info->skip_words : 0);
       else
         printf("Header size = %d words\n",
		file_info->hdr_blocks*(HDR_SIZE+file_info->hdr_shift));
       }
     else
       printf("AMX data.\n");
     if ((data = (data_struct *)malloc(sizeof(data_struct))) == NULL)  {
       printf("convertbru: Error - could not malloc space for data\n");
       rmdir(file_info->out_dir);
       return(BAD_EXIT);
       }
     if (make_stext(file_info,data))  {
       rmdir(file_info->out_dir);
       return(BAD_EXIT);
       }
     }
   if (file_info->file_type != HDR_ONLY)  {
     if (convertfile(file_info,data))  {
       rmdir(file_info->out_dir);
       return(BAD_EXIT);
       }
     }
   printf("Conversion Complete.\n");
   printf("\n");

   return( OK_EXIT );
}

init_file_info(file_info)
file_info_struct *file_info;
{
    file_info->in_file[0] = '\0';
    file_info->out_dir[0] = '\0';
    file_info->word_size = FILE_INFO_UNSET;
    file_info->byte_order = FILE_INFO_UNSET;
    file_info->hdr_shift = FILE_INFO_UNSET;
    file_info->hdr_blocks = FILE_INFO_UNSET;
    file_info->file_type = FILE_INFO_UNSET;
    file_info->skip_words = FILE_INFO_UNSET;
    file_info->data_format = FILE_INFO_UNSET;
    file_info->overwrite_flag = FILE_INFO_UNSET;
    file_info->dec_frq = FILE_INFO_UNSET;
    file_info->spectrometer = FILE_INFO_UNSET;
    file_info->twod = FILE_INFO_UNSET;
    file_info->aqmod = FILE_INFO_UNSET;
}

/*---------------------------------------
|					|
|	     convert_real()		|
|					|
+--------------------------------------*/
double convert_real(buf, pos, file_info)
char *buf;
int	pos;
file_info_struct *file_info;
{
    short	rrc[6];
    int		exp,
		expflag = 0,
		nsign = 0,
		i;
    double real;

/* rrc needs data in order |MSB2|xx|LSB2|MSB1|xx|LSB1| */
    if ((file_info->word_size==AM_SIZE)&&(file_info->byte_order==MSB_FIRST))  {
	/* data stored |MSB2|xx|LSB2|MSB1|xx|LSB1| */
      for (i=0;i<6;i++)  {
	rrc[i] = buf[pos*file_info->word_size + i];
	if (rrc[i] < 0)
	  rrc[i] += 256;
	}
      }
    else if ((file_info->word_size==AM_SIZE)&&
	     (file_info->byte_order==LSB_FIRST))  {
	/* data stored |LSB2|xx|MSB2|LSB1|xx|MSB1| */
      for (i=0;i<6;i++)  {
	rrc[i] = buf[pos*file_info->word_size +
		file_info->word_size-i-1+(i>2?2*file_info->word_size:0)];
	if (rrc[i] < 0)
	  rrc[i] += 256;
	}
      }
    else if ((file_info->word_size==X32_SIZE)&&
	     (file_info->byte_order==MSB_FIRST)) {
	/* data stored |00|MSB2|xx|LSB2|00|MSB1|xx|LSB1| */
      for (i=0;i<6;i++)  {
	rrc[i] = buf[pos*file_info->word_size + i + (i>2?2:1)];
	if (rrc[i] < 0)
	  rrc[i] += 256;
	}
      }
    else if ((file_info->word_size==X32_SIZE)&&
	     (file_info->byte_order==LSB_FIRST)) {
	/* data stored |LSB2|xx|MSB2|00|LSB1|xx|MSB1|00| */
      for (i=0;i<6;i++)  {
	rrc[i] = buf[pos*file_info->word_size +
		file_info->word_size-i-2+(i>2?2*file_info->word_size-1:0)];
	if (rrc[i] < 0)
	  rrc[i] += 256;
	}
      }
    else  {
      printf("convert_real: Error - data format not valid.");
      return(0);
      }

   nsign = (rrc[0] > 127);
   expflag = (rrc[3] > 127);

/********************
*  DEBUG statement  *
********************/

   if (debug)
   {
      for (i = 0; i < 6; i++)
         printf(" %d ", rrc[i]);
      printf("\n");
   }

   exp = (rrc[4] >> 5) + (rrc[3] << 3);
   if (expflag)
      exp = -(~exp) - 2049;
   exp -= 11;

   if (nsign)
   {
      for (i = 0; i < 6; i++)
         rrc[i] = 255 - rrc[i];
   }

   real = (double) (rrc[0] << 4) + (double) (rrc[1] >> 4)
	   + ((double) (rrc[1] % 16))/16.0 + ((double) (rrc[2]))/4096.0
	   + ((double) (rrc[4] % 32))/(4096.0*32.0)
  	   + ((double) rrc[5])/33554432.0;

   if (nsign)
      real *= -1;
   if (exp > 0)
   {
      for (i = 0; i < exp; i++)
         real *= 2.0;
   }
   else if (exp < 0)
   {
      for (i = 0; i < -exp; i++)
         real /= 2.0;
   }

/********************
*  DEBUG statement  *
********************/

   if (debug)
      printf(" real = %f, exp = %d\n", real, exp);

   return(real);
}

/*--------------------------------------
|					|
|	      convert_int()		|
|					|
+--------------------------------------*/
int  convert_int(buf, pos, file_info)
char *buf;
int	pos;
file_info_struct *file_info;
{
    u_char temp[SUN_INT_SIZE];
    int i;

/* sun int must be in order |sign_extend|MSB|xx|LSB| */
    *((int *)temp) = 0;
    if ((file_info->word_size==AM_SIZE)&&(file_info->byte_order==MSB_FIRST))  {
	/* data stored |MSB|xx|LSB| */
      for (i=0;i<file_info->word_size;i++)  {
	temp[i+1] = buf[pos*file_info->word_size + i];
	}
      }
    else if ((file_info->word_size==AM_SIZE)&&
	     (file_info->byte_order==LSB_FIRST))  {
	/* data stored |LSB|xx|MSB| */
      for (i=0;i<file_info->word_size;i++)  {
	temp[SUN_INT_SIZE-i-1] = buf[pos*file_info->word_size + i];
	}
      }
    else if ((file_info->word_size==X32_SIZE)&&
	     (file_info->byte_order==MSB_FIRST)) {
	/* data stored |00|MSB|xx|LSB| */
      for (i=1;i<file_info->word_size;i++)  {
	temp[i] = buf[pos*file_info->word_size + i];
	}
      }
    else if ((file_info->word_size==X32_SIZE)&&
	     (file_info->byte_order==LSB_FIRST)) {
	/* data stored |LSB|xx|MSB|00| */
      for (i=0;i<file_info->word_size-1;i++)  {
	temp[SUN_INT_SIZE-i-1] = buf[pos*file_info->word_size + i];
	}
      }
    else  {
      printf("convert_int: Error - data format not valid.");
      return(0);
      }

    /* sign extend if necessary */
    if (temp[1] & 0x80)
      temp[0] = (u_char)0xFF;

    return(*((int *)temp));
}


/*---------------------------------------
|					|
|		convert_power()		|
|					|
+--------------------------------------*/
int convert_power(buf, pos, file_info, pwrlevel)
char *buf;
int	pos;
file_info_struct *file_info;
int *pwrlevel;
{
   *pwrlevel = convert_int(buf, pos, file_info);
   if (*pwrlevel >= HIGHPOWER)
   {
      *pwrlevel -= HIGHPOWER;
      pwrlevel++;
      *pwrlevel = TRUE;
   }
   else
   {
      *pwrlevel -= LOWPOWER;
      pwrlevel++;
      *pwrlevel = FALSE;
   }
}


/*---------------------------------------
|					|
|		convert_delay()		|
|					|
+--------------------------------------*/
double convert_delay(buf, pos, file_info)
char *buf;
int	pos;
file_info_struct *file_info;
{
   int	i, intv;
   double delay;

   delay = 1e-7;
   intv = convert_int(buf, pos, file_info);
   for (i = 0; i < (intv % 8); i++)
      delay *= 10.0;

   delay *= (double) (intv/8);
   return(delay);
}


/*---------------------------------------
|					|
|		find_string()		|
|					|
| find string str in file fp.		|
| brute-force, slow algorithm, but	|
| since parameter files are small,	|
| we can live with it.			|
+--------------------------------------*/
find_string(fp,str)
FILE *fp;
char *str;
{
    int i, len, temp, found;
    char buf[MAXPATHL], tmpstr[MAXPATHL];

    len = strlen(str);
    if (len >= MAXPATHL)  {
      fprintf(stderr,"find_string: string too long\n");
      return(FALSE);
      }
    for (i=0;i<len;i++)
      tmpstr[i] = tolower(str[i]);
    tmpstr[len] = '\0';
    i=0;
    buf[i++] = tolower(getc(fp));
    while (i < len)  {
      buf[i++] = tolower(getc(fp));
      }
    buf[i] = '\0';
    found = (strcmp(buf,tmpstr) == 0);
    for (i=1;i<len;i++)		buf[i-1] = buf[i];
    while ((!found) && ((buf[len-1] = tolower(getc(fp))) != EOF))  {
      found = (strcmp(buf,tmpstr) == 0);
      for (i=1;i<len;i++)	buf[i-1] = buf[i];
      }
    if (found)  {
      return(TRUE);
      }
    else
      return(FALSE);
}


/*---------------------------------------
|					|
|		read_amx_int()		|
|					|
+--------------------------------------*/
int read_amx_int(fp, str)
FILE *fp;
char *str;
{
    int temp;
    char ch;

    fseek(fp,0,0);	/* go to beginning of file */
    if (find_string(fp,str))  {
      fscanf(fp,"%d",&temp);
      return(temp);
      }
    else
      return(0.0);
}


/*---------------------------------------
|					|
|		read_amx_real()		|
|					|
+--------------------------------------*/
double read_amx_real(fp, str)
FILE *fp;
char *str;
{
    double temp;
    char ch;

    fseek(fp,0,0);	/* go to beginning of file */
    if (find_string(fp,str))  {
      fscanf(fp,"%lf",&temp);
      return(temp);
      }
    else
      return((double)0.0);
}

/*---------------------------------------
|					|
|		read_amx_string()	|
|					|
+--------------------------------------*/
char *read_amx_string(fp, str)
FILE *fp;
char *str;
{
    int i, done;
    double temp;
    char ch;
    char tmp_str[MAX_PAR_LEN], *tmp_ptr;

    fseek(fp,0,0);	/* go to beginning of file */
    if (find_string(fp,str))  {
      while (((ch = getc(fp)) != '<') && (ch != EOF)) {}
      fscanf(fp,"%s",tmp_str);
      i = strlen(tmp_str)-1;
      while ((i >= 0) && (tmp_str[i--] != '>')) {}
      i++;
      if (tmp_str[i]  == '>') tmp_str[i] = '\0';
      tmp_ptr = (char *)malloc(sizeof(char)*strlen(tmp_str)+1);
      strcpy(tmp_ptr,tmp_str);
      i=0;  done = FALSE;
      while ((i<strlen(tmp_ptr)) && (!done))  {
	if (tmp_ptr[i] < 32)  {
	  done = TRUE;
	  tmp_ptr[i] = '\0';
	  }
	i++;
	}
      return(tmp_ptr);
      }
    else
      return((char *)NULL);
}

/*---------------------------------------
|					|
|		read_amx_delay()	|
|					|
+--------------------------------------*/
double read_amx_delay(fp, str)
FILE *fp;
char *str;
{
   double delay;
   int i, d_num, temp;
   char ch, line[MAXPATHL];

   fseek(fp,0,0);	/* go to beginning of file */
   if (((str[1] == 'D') || (str[1] == 'P')) && (!isdigit(str[2])))  {
     find_string(fp,str);
     fscanf(fp,"%lf",&delay);
     return(delay);
     }
   else if ((str[1] == 'D') && (isdigit(str[2])))  {
     find_string(fp,"$D=");
     fgets(line,MAXPATHL-1,fp);
     }
   else if ((str[1] == 'P') && (isdigit(str[2])))  {
     find_string(fp,"$P=");
     fgets(line,MAXPATHL-1,fp);
     }
   else  {
     fprintf(stderr,"read_amx_delay: unknown delay \"%s\"\n",str);
     return(0.0);
     }
   line[0] = str[2];
   line[1] = '\0';
   sscanf(line,"%d",&d_num);
   for (i=0;i<=d_num;i++)
     fscanf(fp,"%lf",&delay);
   return(delay);
}

/*---------------------------------------
|					|
|		read_amx_power()	|
|					|
+--------------------------------------*/
int read_amx_power(fp, str, pwrlevel)
FILE *fp;
char *str;
int *pwrlevel;
{
   int i, p_num, temp;
   char ch, line[MAXPATHL];

   fseek(fp,0,0);	/* go to beginning of file */
   if ((str[1] == 'S') && (isdigit(str[2])))  {
     find_string(fp,"$S=");
     fgets(line,MAXPATHL-1,fp);
     line[0] = str[2];
     line[1] = '\0';
     sscanf(line,"%d",&p_num);
     for (i=0;i<=p_num;i++)
       fscanf(fp,"%d",pwrlevel);
     if (*pwrlevel < AMX_HIGHPOWER) {
       pwrlevel++;
       *pwrlevel = TRUE;
       }
     else  {
       *pwrlevel -= AMX_HIGHPOWER;
       pwrlevel++;
       *pwrlevel = FALSE;
       }
     }
   else if ((str[1] != 'S') || (!isdigit(str[2])))  {
     find_string(fp,str);
     fgets(line,MAXPATHL-1,fp);
     if (strchr(line,'('))
       fscanf(fp,"%d",pwrlevel);
     else
       sscanf(line,"%d",pwrlevel);
     if (*pwrlevel < AMX_HIGHPOWER) {
       pwrlevel++;
       *pwrlevel = TRUE;
       }
     else  {
       *pwrlevel -= AMX_HIGHPOWER;
       pwrlevel++;
       *pwrlevel = FALSE;
       }
     }
}



/*----------------------------------------------+
|						|
|		get_file_format()		|
|						|
|	Need to set file_info->			|
|				word_size	|
|				byte_order	|
|				hdr_shift	|
|				hdr_blocks	|
|						|
+----------------------------------------------*/
/* kind of a messy subroutine.  Need to set the above 4 variable, but
   most checks must be done on 2 or more at the same time, so there are
   a lot of possibilities to check.  */
/* Checks each of the 4 possibilities :
     |00|MSB|xx|LSB| 			(X32_SIZE, MSB_FIRST)
     |LSB|xx|MSB|00| or |LSB|xx|MSB|FF| (X32_SIZE, LSB_FIRST)
     |MSB|xx|LSB|			(AM_SIZE, MSB_FIRST)
     |LSB|xx|MSB| 			(AM_SIZE, LSB_FIRST)
   To do this, we check :
	1) if first or last of every 4 byte word is 0x00 or 0xFF.
	2) if DR makes sense with the chosen byte order (6 < DR < 12)
	3) if the file size is correct, given the values of NP and NI */
int get_file_format(file_info)
file_info_struct *file_info;
{
    FILE *fp;
    u_char buf[(HDR_SIZE+HDR_SHIFT)*X32_SIZE];
    int format_found, hdr_found, i, ival, np, ni, read_size, dr;
    struct stat	unix_fab; /* unix_fab.st_size is file size */
    double sfrq,dfrq;

    if (!(fp = fopen(file_info->in_file,"r")))  {
      printf("get_file_format: Error - file \"%s\" not found\n",
		file_info->in_file);
      return(ERROR);
      }

    /* get file size for upcoming checks */
    ival = stat(file_info->in_file, &unix_fab);
    if (ival != 0)  {
      printf("get_file_format: Error obtaining file \"%s\" size\n",
		file_info->in_file);
      fclose(fp);
      return(ERROR);
      }

    if ((file_info->word_size == AM_SIZE) ||
	(unix_fab.st_size < X32_SIZE*(HDR_SIZE+HDR_SHIFT)))
      read_size = AM_SIZE;
    else
      read_size = X32_SIZE;
    if (fread(buf,sizeof(u_char),(HDR_SIZE+HDR_SHIFT)*read_size,fp)!=
		(HDR_SIZE+HDR_SHIFT)*read_size)  {
      printf("get_file_format: Error reading file \"%s\" header \n",
		file_info->in_file);
      fclose(fp);
      return(ERROR);
      }

    format_found = FALSE;
    hdr_found = FALSE;

    /* check if user specified word_size and byte_order */
    if ((file_info->word_size != FILE_INFO_UNSET) && 
        (file_info->byte_order != FILE_INFO_UNSET))
      format_found = TRUE;

  /*******************  Start checks for X32_SIZE data  **********************/
  if (!format_found)  {
    /* check for X32 format with |00|MSB|xx|LSB| */
    i = 0;
    while ((i < HDR_SIZE)&&(buf[X32_SIZE*i]==(u_char)0x00))  {
      i++;
      }
    if (i == HDR_SIZE)  {
      file_info->word_size = X32_SIZE;
      file_info->byte_order = MSB_FIRST;
      format_found = TRUE;
      }
    /* check for X32 format with |LSB|xx|MSB|00| or |LSB|xx|MSB|FF| */
    i = 0;
    while ((i < HDR_SIZE) && ((buf[X32_SIZE*(i+1)-1]==(u_char)0x00)||
			      (buf[X32_SIZE*(i+1)-1]==(u_char)0xFF))) {
      i++;
      }
    if (i == HDR_SIZE)  {
      file_info->word_size = X32_SIZE;
      file_info->byte_order = LSB_FIRST;
      format_found = TRUE;
      }
    }
  /* checks for hdr_shift.  DR could be in two different places in the header */
    if (format_found)  {
      dr = convert_int(buf,DR_OFFSET, file_info);
      if ((dr < MAX_DR) && (dr > MIN_DR))  {
	file_info->hdr_shift = HDR_NO_SHIFT;	/* non-shifted header */
	file_info->hdr_blocks = 1;		/* assume 1 header block */
	hdr_found = TRUE;
	}
      if (!hdr_found)  {
        dr = convert_int(buf,DR_OFFSET+HDR_SHIFT, file_info);
        if ((dr < MAX_DR) && (dr > MIN_DR))  {
	  file_info->hdr_shift = HDR_SHIFT;	/* shifted header */
	  file_info->hdr_blocks = 1;		/* assume 1 header block */
	  hdr_found = TRUE;
	  }
	}
      }
    if (format_found && !hdr_found)  {
      printf("get_file_format: Error - file has unknown header format\n");
      fclose(fp);
      return(ERROR);
      }
/*******************  End checks for X32_SIZE data  ************************/

/*******************  Start checks for AM_SIZE data  ************************/
    if (!format_found)  {
      /* try AM_SIZE data format */
      file_info->word_size = AM_SIZE;
      /* try MSB_FIRST */
      file_info->byte_order = MSB_FIRST;
      dr = convert_int(buf,DR_OFFSET, file_info);
      if ((dr < MAX_DR) && (dr > MIN_DR))  {
	file_info->hdr_shift = HDR_NO_SHIFT;	/* non-shifted header */
	file_info->hdr_blocks = 1;		/* assume 1 header block */
        format_found = TRUE;
        hdr_found = TRUE;
	}
      if (!format_found)  {
	dr = convert_int(buf,DR_OFFSET+HDR_SHIFT,file_info);
        if ((dr < MAX_DR) && (dr > MIN_DR))  {
	  file_info->hdr_shift = HDR_SHIFT;	/* shifted header */
	  file_info->hdr_blocks = 1;		/* assume 1 header block */
          format_found = TRUE;
          hdr_found = TRUE;
	  }
	}
      if (!format_found)  {
        /* try LSB_FIRST */
        file_info->byte_order = LSB_FIRST;
        dr = convert_int(buf,DR_OFFSET, file_info);
        if ((dr < MAX_DR) && (dr > MIN_DR))  {
	  file_info->hdr_shift = HDR_NO_SHIFT;	/* non-shifted header */
	  file_info->hdr_blocks = 1;		/* assume 1 header block */
          format_found = TRUE;
          hdr_found = TRUE;
	  }
        if (!format_found)  {
          dr = convert_int(buf,DR_OFFSET + HDR_SHIFT, file_info);
          if ((dr < MAX_DR) && (dr > MIN_DR))  {
	    file_info->hdr_shift = HDR_SHIFT;	/* shifted header */
	    file_info->hdr_blocks = 1;		/* 1 header block */
            format_found = TRUE;
            hdr_found = TRUE;
	    }
	  }
	}
      }
/*******************  End checks for AM_SIZE data  ************************/

    if (!format_found || !hdr_found)  {
      printf("get_file_format: Error - file has unknown header format\n");
      printf("Possibly header has been stripped - use \"-s\", \"-o\", and \"-t\" options\n");
      fclose(fp);
      return(ERROR);
      }
    else  {
      /* determine if there are two headers present (may not always work) */
      if (file_info->file_type == HDR_AND_DATA)  {
        np = convert_int(buf,NP_OFFSET+file_info->hdr_shift, file_info);
	if (unix_fab.st_size == 
		((np+2*(HDR_SIZE+file_info->hdr_shift))*file_info->word_size)) 
	  file_info->hdr_blocks = 2;
        ni = convert_int(buf,NI_OFFSET+file_info->hdr_shift, file_info);
	if (unix_fab.st_size == 
	      ((ni*np+2*(HDR_SIZE+file_info->hdr_shift))*file_info->word_size)) 
	  file_info->hdr_blocks = 2;
	}
      sfrq = convert_real(buf,SFRQ_OFFSET+file_info->hdr_shift,file_info);
      if ((sfrq < MINFREQ) || (sfrq > MAXFREQ))  {
	printf("get_file_format: Error - SFRQ out of range\n");
	fclose(fp);
	return(ERROR);
	}
      fclose(fp);
      return(OK);
      }
}

/*---------------------------------------
|					|
|	      parse_args()/3		|
|					|
+--------------------------------------*/
int  parse_args(argc,argv,file_info)
int argc;
char *argv[];
file_info_struct *file_info;
{
    int i, j, itmp, n;
    float ftmp;
    struct stat	unix_fab; /* unix_fab.st_size is file size */
    char str[MAXPATHL], *char_ptr;

    if (argc < 2)  {
      printf("parse_args: Error - no file name specified\n");
      return(ERROR);
      }

    /* deal with file name argument */
    strcpy(file_info->in_file,argv[1]);
    n = strlen(file_info->in_file);
    /* delete trailing "/", if present */
    if (n > 0)  {
      if (file_info->in_file[n-1] == '/')  {
	file_info->in_file[n-1] = '\0';
	n--;
	}
      }
    strcpy(str,file_info->in_file);
    /* if filename has ".bru" extension, remove it */
    if (n > 4) {
      if (((file_info->in_file[n-4] == '.')&&(file_info->in_file[n-3] == 'b') &&
          (file_info->in_file[n-2] == 'r')&&(file_info->in_file[n-1] == 'u')) ||
         ((file_info->in_file[n-4] == '.')&&(file_info->in_file[n-3] == 'B') &&
          (file_info->in_file[n-2] == 'R')&&(file_info->in_file[n-1] == 'U'))) {
        str[n-4]='\0';
        }
      }
    /* make output file go to cwd */
    getcwd(file_info->out_dir, sizeof(file_info->out_dir));
    char_ptr = strrchr(str,'/');
    if (char_ptr == NULL)
      char_ptr = str;
    else
      char_ptr++;
    strcat(file_info->out_dir,"/");
    strcat(file_info->out_dir,char_ptr);
    strcat(file_info->out_dir, ".cv");
    if (stat(file_info->in_file,&unix_fab))  {
      printf("parse_args: Error - file \"%s\" not found\n",file_info->in_file);
      return(ERROR);
      }

    for (i=2;i<argc;i++)  {
      if (strncmp(argv[i],"-f",2) == 0)  {
	file_info->overwrite_flag = TRUE;
	}
      else if (strncmp(argv[i],"-d",2) == 0)  {	/* decoupler frequency */
	if (sscanf(argv[i]+2,"%f",&ftmp) != 1)  {
	  printf("parse_args: Error - illegal argument \"%s\"\n",argv[i]);
	  return(ERROR);
	  }
	file_info->dec_frq = ftmp;
	}
      else if (strncmp(argv[i],"-b",2) == 0)  {	/* spectrometer type */
	strcpy(str,argv[i]+2);
	if ((strcmp(str,"amx") == 0) || (strcmp(str,"AMX") == 0))
	  file_info->spectrometer = AMX_SPEC;
	else if ((strcmp(str,"am") == 0) || (strcmp(str,"AM") == 0))
	  file_info->spectrometer = AM_SPEC;
	else  {
	  printf("parse_args: Error - illegal argument \"%s\"\n",argv[i]);
	  return(ERROR);
	  }
	}
      else if (strncmp(argv[i],"-s",2) == 0)  {	/* word_size */
	if (sscanf(argv[i]+2,"%d",&itmp) != 1)  {
	  printf("parse_args: Error - illegal argument \"%s\"\n",argv[i]);
	  return(ERROR);
	  }
	if ((itmp < AM_SIZE) || (itmp > X32_SIZE)) {
	  printf("parse_args: Error - illegal value for word size : \"%d\"\n",itmp);
	  return(ERROR);
	  }
	file_info->word_size = itmp;
	}
      else if (strncmp(argv[i],"-o",2) == 0)  {	/* byte_order */
	strcpy(str,argv[i]+2);
	if ((strcmp(str,"msb") == 0) || (strcmp(str,"MSB") == 0))
	  file_info->byte_order = MSB_FIRST;
	else if ((strcmp(str,"lsb") == 0) || (strcmp(str,"LSB") == 0))
	  file_info->byte_order = LSB_FIRST;
	else  {
	  printf("parse_args: Error - illegal argument \"%s\"\n",argv[i]);
	  return(ERROR);
	  }
	}
      else if (strncmp(argv[i],"-p",2) == 0)  {	/* skip_words */
	if (sscanf(argv[i]+2,"%d",&itmp) != 1)  {
	  printf("parse_args: Error - illegal argument \"%s\"\n",argv[i]);
	  return(ERROR);
	  }
	file_info->skip_words = itmp;
	}
      else if (strncmp(argv[i],"-t",2) == 0)  {	/* file_type */
	strcpy(str,argv[i]+2);
	if ((strcmp(str,"hdr") == 0) || (strcmp(str,"HDR") == 0))
	  file_info->file_type = HDR_ONLY;
	else if ((strcmp(str,"data") == 0) || (strcmp(str,"DATA") == 0))
	  file_info->file_type = DATA_ONLY;
	else if ((strcmp(str,"all") == 0) || (strcmp(str,"ALL") == 0))
	  file_info->file_type = HDR_AND_DATA;
	else  {
	  printf("parse_args: Error - illegal argument \"%s\"\n",argv[i]);
	  return(ERROR);
	  }
	}
      else if (strncmp(argv[i],"-c",2) == 0)  {	/* create output directory */
	strcpy(file_info->out_dir,argv[i]+2);
        n = strlen(file_info->out_dir);
        if (n > 0)  {
          if (file_info->out_dir[n-1] == '/')
	    file_info->out_dir[n-1] = '\0';
	  }
	strcat(file_info->out_dir,".cv");
	}
      else  {
	printf("parse_args: Error - illegal argument \"%s\"\n",argv[i]);
	return(ERROR);
	}
      }

    if (file_info->spectrometer == FILE_INFO_UNSET)  {
      if (S_ISDIR(unix_fab.st_mode))
	file_info->spectrometer = AMX_SPEC;
      else
	file_info->spectrometer = AM_SPEC;
      }
    /* make sure word size is implicit for AMX data */
    if (file_info->spectrometer == AMX_SPEC)  {
      if (file_info->byte_order != FILE_INFO_UNSET)  {
	if (file_info->word_size == FILE_INFO_UNSET)  {
	  file_info->word_size = X32_SIZE;
	  }
	else if (file_info->word_size == AM_SIZE)  {
	  printf("parse_args: Warning - AMX data - word size being set to 4\n");
	  file_info->word_size = X32_SIZE;
	  }
	}
      }
    /* check if all args needed were specified */
    if (file_info->file_type==DATA_ONLY)  {
      if (file_info->spectrometer == AM_SPEC)  {
        if ((file_info->word_size == FILE_INFO_UNSET) ||
          (file_info->byte_order == FILE_INFO_UNSET))  {
	  printf("parse_args: Error - need to specify \"-s\" and \"-o\" arguments with \"-tdata\"\n");
	  return(ERROR);
	  }
	}
      else  {
        if (file_info->byte_order == FILE_INFO_UNSET)  {
	  printf("parse_args: Error - need to specify \"-o\" arguments with \"-tdata\"\n");
	  return(ERROR);
	  }
	}
      }
    if ((file_info->skip_words != FILE_INFO_UNSET) &&
	(file_info->file_type != DATA_ONLY))  {
	printf("parse_args: Error - must specify \"-tdata\" with \"-p%d\" argument\n",file_info->skip_words);
	return(ERROR);
	}
    if ((file_info->word_size == FILE_INFO_UNSET) &&
	 (file_info->byte_order != FILE_INFO_UNSET))  {
        printf("parse_args: Warning - No \"-s\" argument; \"-o\" argument ignored\n");
	file_info->byte_order = FILE_INFO_UNSET;
	}
    if ((file_info->word_size != FILE_INFO_UNSET) &&
	 (file_info->byte_order == FILE_INFO_UNSET))  {
        printf("parse_args: Warning - No \"-o\" argument; \"-s\" argument ignored\n");
	file_info->word_size = FILE_INFO_UNSET;
	}

    /* set up defaults */
    if (file_info->file_type == FILE_INFO_UNSET)
      file_info->file_type = HDR_AND_DATA;
    if (file_info->overwrite_flag == FILE_INFO_UNSET)
      file_info->overwrite_flag = FALSE;
    if (file_info->data_format == FILE_INFO_UNSET)
      file_info->data_format = TPPI;
    if (file_info->spectrometer == AMX_SPEC)  {
      if (file_info->word_size == FILE_INFO_UNSET)
	file_info->word_size = X32_SIZE;
      if (file_info->byte_order == FILE_INFO_UNSET)
	file_info->byte_order = LSB_FIRST;
      if (file_info->skip_words == FILE_INFO_UNSET)
	file_info->skip_words = 0;
      }

/**********************************************
*  Create directory for converted Bruker FID  *
*  and parameter data:  SREAD format          *
**********************************************/

    if (mkdir(file_info->out_dir, 0777)) {
      if (errno == EEXIST) {
        if (!file_info->overwrite_flag) {
          printf("File '%s' already exists -- use \"-f\" option to overwrite\n", file_info->out_dir);
          return(ERROR);
          }
        }
      else {
        printf("Cannot create file %s\n", file_info->out_dir);
	return(ERROR);
        }
      }
    return(OK);
}


/*---------------------------------------
|					|
|	      write_header()		|
|					|
+--------------------------------------*/
int  write_header(file_info)
file_info_struct *file_info;
{
    FILE *fp;
    u_char buf[(HDR_SIZE+HDR_SHIFT)*X32_SIZE];
    u_char buf2[(HDR_SIZE+HDR_SHIFT)*X32_SIZE];
    char fname[MAXPATHL];
    int i,dpwr[2];
    
    if (!(fp = fopen(file_info->in_file,"r")))  {
      printf("write_header: Error - file \"%s\" not opened\n",
			file_info->in_file);
      return(ERROR);
      }
    if (fread(buf,sizeof(u_char),(HDR_SIZE+HDR_SHIFT)*file_info->word_size-
		file_info->hdr_shift*file_info->word_size,fp)!=
		(HDR_SIZE+HDR_SHIFT)*file_info->word_size-
		file_info->hdr_shift*file_info->word_size){
      printf("write_header: Error reading file \"%s\" header \n",
		file_info->in_file);
      fclose(fp);
      return(ERROR);
      }
    if (fread(buf2,sizeof(u_char),(HDR_SIZE+HDR_SHIFT)*file_info->word_size-
		file_info->hdr_shift*file_info->word_size,fp)!=
		(HDR_SIZE+HDR_SHIFT)*file_info->word_size-
		file_info->hdr_shift*file_info->word_size){
      printf("write_header: Error reading file \"%s\" header \n",
		file_info->in_file);
      fclose(fp);
      return(ERROR);
      }
    fclose(fp);
    strcpy(fname,file_info->out_dir);
    strcat(fname,".par");
    if (!(fp = fopen(fname,"w")))  {
      printf("write_header: Error - file \"%s\" not created\n",fname);
      return(ERROR);
      }
    fprintf(fp,"FN = %d\n",
		convert_int(buf,FN_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"NT = %d\n",
		convert_int(buf,NT_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"NP = %d\n",
		convert_int(buf,NP_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"DW = %d\n",
		convert_int(buf,DW_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"FB = %d\n",
		convert_int(buf,FB_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"SS = %d\n",
		convert_int(buf,SS_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"SW = %20.10f\n",
		convert_real(buf,SW_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"NI = %d\n",
		convert_int(buf,NI_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"TOF = %20.10f\n",
		convert_real(buf,TOF_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"DOF = %20.10f\n",
		convert_real(buf,DOF_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"RD = %20.10f\n",
		convert_delay(buf,RD_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"PW = %20.10f\n",
		convert_delay(buf,PW_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"DE = %20.10f\n",
		convert_delay(buf,DE_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"SFRQ = %20.10f\n",
		convert_real(buf,SFRQ_OFFSET+file_info->hdr_shift,file_info));
    convert_power(buf,DECPWR_OFFSET+file_info->hdr_shift,file_info,dpwr);
    fprintf(fp,"DECPWR = %s%d\n",dpwr[1]?"hi":"lo",dpwr[0]);
    fprintf(fp,"TEMP = %d\n",
		convert_int(buf,TEMP_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"DW1 = %d\n",
		convert_int(buf,DW1_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"SF0 = %20.10f\n",
		convert_real(buf,SF0_OFFSET+file_info->hdr_shift,file_info));
    fprintf(fp,"DFRQ = %20.10f\n",
		convert_real(buf,DFRQ_OFFSET+file_info->hdr_shift,file_info));
    for (i=0;i<(HDR_SIZE+HDR_SHIFT);i++)
      fprintf(fp,"%d: %d, %20.10f, %20.10f\n",i,
convert_int(buf,i,file_info),
convert_delay(buf,i,file_info),
convert_real(buf,i,file_info));
    for (i=0;i<(HDR_SIZE+HDR_SHIFT);i++)
      fprintf(fp,"%d: %d, %20.10g, %20.10g\n",i,
convert_int(buf2,i,file_info),
convert_delay(buf2,i,file_info),
convert_real(buf2,i,file_info));
    fclose(fp);
    return(OK);
}


/*---------------------------------------
|					|
|		make_stext()		|
|					|
+--------------------------------------*/
int  make_stext(file_info,data)
file_info_struct *file_info;
data_struct *data;
{

    if (file_info->spectrometer == AMX_SPEC)  {
      if (read_amx_header(file_info,data))
        return(ERROR);
      write_amx_stext(file_info,data);
      }
    else  {
      if (read_am_header(file_info,data))
        return(ERROR);
      write_stext(file_info,data);
      }
    return(OK);
}

typedef struct {
	char str[9];
	int num_bytes,
	    class,
	    other;
	} param_struct;

#define INTEGER_MASK	0x100
#define FLOAT_MASK	0x300
#define DOUBLE_MASK	0x400
#define CHAR_MASK	0x800

swap_bytes(a)
int *a;
{
    char temp, *tempptr;

    tempptr = (char *)a;
    temp = tempptr[0];
    tempptr[0] = tempptr[3];
    tempptr[3] = temp;
    temp = tempptr[1];
    tempptr[1] = tempptr[2];
    tempptr[2] = temp;
}

swap_words(a)
double *a;
{
    int temp, *tempptr;

    tempptr = (int *)a;
    temp = tempptr[0];
    tempptr[0] = tempptr[1];
    tempptr[1] = temp;
}

amx_bin_to_ascii_header(ifp, ofp)
FILE *ifp, *ofp;
{
    param_struct params;
    char c_data[9], *char_ptr, str[133];
    int done, i_data, dummy, num_p, i;
    float f_data;
    double d_data;

    done = FALSE;
    while (!done)  {
      fread(params.str,sizeof(char),8,ifp);
      params.str[8] = '\0';
      if (strcmp(params.str,"ZZ00") == 0)
	done = TRUE;
      fread(&params.num_bytes,sizeof(int),1,ifp);
      swap_bytes(&params.num_bytes);
      fread(&params.class,sizeof(int),1,ifp);
      fread(&params.other,sizeof(int),1,ifp);
      /* must check float before int because float is 0x300 and int is 0x100 */
      if ((params.class & FLOAT_MASK) == FLOAT_MASK)  {
	fread(&f_data,sizeof(float),1,ifp);
	swap_bytes(&f_data);
	fread(&dummy,sizeof(int),1,ifp);
	if (params.num_bytes <= 8)  {
	  if ((fabs(f_data) < 1.0) && (fabs(f_data) > 0.0))
	    fprintf(ofp,"##$%s= %.7g\n",params.str,f_data);
	  else
	    fprintf(ofp,"##$%s= %g\n",params.str,f_data);
	  }
	}
      else if ((params.class & INTEGER_MASK) == INTEGER_MASK)  {
	fread(&i_data,sizeof(int),1,ifp);
	swap_bytes(&i_data);
	fread(&dummy,sizeof(int),1,ifp);
	if ((strcmp(params.str,"A000")!=0)&&(strcmp(params.str,"ZZ00")!=0))  {
	  if (params.num_bytes <= 8)
	    fprintf(ofp,"##$%s= %d\n",params.str,i_data);
	  }
	}
      else if ((params.class & DOUBLE_MASK) == DOUBLE_MASK)  {
	fread(&d_data,sizeof(double),1,ifp);
	swap_bytes(&d_data);
	char_ptr = (char *)&d_data;
	char_ptr += 4;
	swap_bytes(char_ptr);
	swap_words(&d_data);
	if (params.num_bytes <= 8)
	  fprintf(ofp,"##$%s= %.7f\n",params.str,d_data);
	}
      else if ((params.class & CHAR_MASK) == CHAR_MASK)  {
	fread(c_data,sizeof(char),8,ifp);
	c_data[8] = '\0';
	if ((params.num_bytes <= 8) && (strcmp(c_data,"ZZ00") != 0))
	  fprintf(ofp,"##$%s= %s\n",params.str,c_data);
	}
      }
    done = FALSE;
    while (!done)  {
      fread(params.str,sizeof(char),8,ifp);
      params.str[8] = '\0';
      if (strcmp(params.str,"ENDE") == 0)
	done = TRUE;
      fread(&params.num_bytes,sizeof(int),1,ifp);
      swap_bytes(&params.num_bytes);
      fread(&params.class,sizeof(int),1,ifp);
      fread(&params.other,sizeof(int),1,ifp);
      /* must check float before int because float is 0x300 and int is 0x100 */
      if ((params.class & FLOAT_MASK) == FLOAT_MASK)  {
	num_p = params.num_bytes/sizeof(float);
	fprintf(ofp,"##$%s= (0..%d)\n",params.str,num_p-1);
	for (i=0;i<num_p;i++)  {
	  fread(&f_data,sizeof(float),1,ifp);
	  swap_bytes(&f_data);
	  if ((fabs((double)f_data) < 1.0) && (fabs((double)f_data) > 0.0))
	    fprintf(ofp," %.7g",f_data);
	  else
	    fprintf(ofp," %g",f_data);
	  if (((i+1) % 8) == 0)
	    fprintf(ofp,"\n");
	  }
	if (((num_p) % 8) != 0)
	  fprintf(ofp,"\n");
	}
      else if ((params.class & INTEGER_MASK) == INTEGER_MASK)  {
	num_p = params.num_bytes/sizeof(int);
	fprintf(ofp,"##$%s= (0..%d)\n",params.str,num_p-1);
	for (i=0;i<num_p;i++)  {
	  fread(&i_data,sizeof(float),1,ifp);
	  swap_bytes(&i_data);
	  fprintf(ofp," %d",i_data);
	  if (((i+1) % 8) == 0)
	    fprintf(ofp,"\n");
	  }
	if (((num_p) % 8) != 0)
	  fprintf(ofp,"\n");
	}
      else if ((params.class & DOUBLE_MASK) == DOUBLE_MASK)  {
	num_p = params.num_bytes/sizeof(double);
	fprintf(ofp,"##$%s= (0..%d)\n",params.str,num_p-1);
	for (i=0;i<num_p;i++)  {
	  fread(&d_data,sizeof(double),1,ifp);
	  swap_bytes(&d_data);
	  char_ptr = (char *)&d_data;
	  char_ptr += 4;
	  swap_bytes(char_ptr);
	  swap_words(&d_data);
	  fprintf(ofp," %.7f",d_data);
	  if (((i+1) % 8) == 0)
	    fprintf(ofp,"\n");
	  }
	if (((num_p) % 8) != 0)
	  fprintf(ofp,"\n");
	}
      else if ((params.class & CHAR_MASK) == CHAR_MASK)  {
	num_p = params.num_bytes/sizeof(char);
	fread(str,sizeof(char),num_p,ifp);
	str[num_p] = '\0';
	fprintf(ofp,"##$%s= <%s>\n",params.str,str);
	}
      }
}


/*---------------------------------------
|					|
|		read_acqu()		|
|					|
+--------------------------------------*/
read_acqu(fp,data,file_info)
FILE *fp;
data_struct *data;
file_info_struct *file_info;
{
	/* number of transients:	NS */
   data->nt = read_amx_int(fp,"$NS=");
}

/*---------------------------------------
|					|
|		read_acqus()		|
|					|
+--------------------------------------*/
read_acqus(fp,data,file_info)
FILE *fp;
data_struct *data;
file_info_struct *file_info;
{
   char *temp_str;

   data->num_bruker_par = 0;
   data->num_skip_par = 0;
   strcpy(data->skip_par_list[data->num_skip_par++], "date");
   strcpy(data->bruker_par_list[data->num_bruker_par++], "dp");
   strcpy(data->bruker_par_list[data->num_bruker_par++], "in");
   strcpy(data->bruker_par_list[data->num_bruker_par++], "sp");
/********************************************************
*  Determine how data was acquired.  AQ_mod = 2 means	*
*   sequentially acquired data, and AQ_mod = 1 means	*
*   simultaneously acquired data			*
********************************************************/
   file_info->aqmod = read_amx_int(fp,"$AQ_mod=");

/**********************************************
*  Read in basic NMR acquisition parameters.  *
**********************************************/

	/* number of completed transients:	NS */
   data->ct = read_amx_int(fp,"$NS=");
	/* number of data points:	TD */
   data->np = read_amx_int(fp,"$TD=");
	/* sweep width:			SW */
   data->sw = read_amx_real(fp,"$SW_h=");
   strcpy(data->bruker_par_list[data->num_bruker_par++], "sw");
	/* filter bandwidth (1.1*SW/2)	FB */
   data->fb = 100*((int)(((1.1*data->sw/2.0)/100.0)+0.5));
	/* steady-state transients:	DS */
   data->ss = read_amx_int(fp,"$DS=");
	/* relaxation delay:		D1 */
   data->rd = read_amx_real(fp,"$RD=");
   if (data->rd == 0.0)  {
     data->rd = read_amx_delay(fp,"$D1=");
     strcpy(data->bruker_par_list[data->num_bruker_par++], "rd");
     }
	/* ringdown delay:		DE */
   data->de = read_amx_real(fp,"$DE=");
	/* pulse width:			PW */
   data->pw = read_amx_real(fp,"$PW=");
   if (data->pw == 0.0)
     data->pw = read_amx_delay(fp,"$P0");
   strcpy(data->bruker_par_list[data->num_bruker_par++], "pw");
	/* pulse width:			P1 */
   data->pw90 = read_amx_delay(fp,"$P1");
	/* temperature:			TE */
   data->temp = read_amx_real(fp,"$TE=") - 273;
	/* dwell time in us		DW */
   data->dw = (1.0/data->sw)*1.0e6;
   if (file_info->aqmod == SEQUENTIAL_DATA)
     data->dw /= 2.0;
	/* observe synthesizer frequency;	SFRQ */
   data->sfrq = read_amx_real(fp,"$SFO1=");
	/* decoupler synthesizer frequency;	DFRQ */
   data->dfrq = read_amx_real(fp,"$SFO2=");
   if ((data->dfrq < MINFREQ) && (file_info->dec_frq == FILE_INFO_UNSET)) {
     file_info->dec_frq = data->sfrq;
     printf("DFRQ being set to SFRQ\n");
     }
   else if (file_info->dec_frq == FILE_INFO_UNSET) {
     file_info->dec_frq = data->dfrq;
     }
   data->dfrq = file_info->dec_frq;

	/* observe offset:		O1 */
   data->tof = read_amx_real(fp,"$O1=");
	/* decoupler offset:		O2 */
   data->dof = read_amx_real(fp,"$O2=");
   data->spin = read_amx_real(fp,"$RO=");
   data->date = read_amx_int(fp,"$DATE=");
   data->rg = read_amx_int(fp,"$RG=");
   temp_str = read_amx_string(fp,"$NUCLEUS=");
   strcpy(data->tn,temp_str);
   temp_str = read_amx_string(fp,"$DECNUC=");
   strcpy(data->dn ,temp_str);
   temp_str  = read_amx_string(fp,"$PULPROG=");
   strcpy(data->seqfil,temp_str);
   data->at = data->dw*data->np/1.0e+6;
}

/*---------------------------------------
|					|
|		read_acqu2s()		|
|					|
+--------------------------------------*/
read_acqu2s(fp,data,file_info)
FILE *fp;
data_struct *data;
file_info_struct *file_info;
{
	/* number of t1 increments:	NE */
     data->ni = read_amx_int(fp,"$TD=");
     data->sw1 = read_amx_real(fp,"$SW_h=");
     data->in = 1.0/data->sw1;
}

/*---------------------------------------
|					|
|		read_amx_header()	|
|					|
+--------------------------------------*/
int  read_amx_header(file_info,data)
file_info_struct *file_info;
data_struct *data;
{
   FILE		*f1,*f2,*f3;
   int		done, lb_count, ch_count, bin_file;
   char		ch,filename[MAXPATHL];


    bin_file = FALSE;
    strcpy(filename,file_info->in_file);
    strcat(filename,"/acqu");
    if (!(f1 = fopen(filename,"r")))  {
      printf("read_amx_header: Error - file \"%s\" not found\n",
		filename);
      return(ERROR);
      }
    /* try to determine if file is ascii or binary by checking for
	# characters, which appear 2/line in ascii version of file */
    done = FALSE;
    lb_count = 0;
    ch_count = 0;
    while (((ch = getc(f1)) != EOF) && (!done))  {
      if (ch == '#')
	lb_count++;
      ch_count++;
      if (ch_count >= 256)
	 done = TRUE;
      }
    fseek(f1,0,0);		/* back to beginning of file */
    if (lb_count < 10)  {	/* binary file */
      bin_file = TRUE;
      strcpy(filename,file_info->out_dir);
      strcat(filename,"/acqu");
      if (!(f2 = fopen(filename,"w")))  {
	printf("read_amx_header: Error - file \"%s\" not found\n",
		filename);
	return(ERROR);
	}
      amx_bin_to_ascii_header(f1,f2);
      fclose(f1);   fclose(f2);
      if (!(f1 = fopen(filename,"r")))  {
	printf("read_amx_header: Error - file \"%s\" not found\n",
		filename);
	return(ERROR);
	}
      }
    /* at this point the ascii version of the parameter file is open and
	has the file pointer at the beginning of the file */
    read_acqu(f1,data,file_info);
    fclose(f1);

    strcpy(filename,file_info->in_file);
    strcat(filename,"/acqus");
    if (!(f1 = fopen(filename,"r")))  {
      printf("read_amx_header: Error - file \"%s\" not found\n",
		filename);
      return(ERROR);
      }
    if (bin_file)  {	/* binary file */
      bin_file = TRUE;
      strcpy(filename,file_info->out_dir);
      strcat(filename,"/acqus");
      if (!(f2 = fopen(filename,"w")))  {
	printf("read_amx_header: Error - file \"%s\" not found\n",
		filename);
	return(ERROR);
	}
      amx_bin_to_ascii_header(f1,f2);
      fclose(f1);   fclose(f2);
      if (!(f1 = fopen(filename,"r")))  {
	printf("read_amx_header: Error - file \"%s\" not found\n",
		filename);
	return(ERROR);
	}
      }
    /* at this point the ascii version of the parameter file is open and
	has the file pointer at the beginning of the file */
    read_acqus(f1,data,file_info);
    fclose(f1);

/******************************************************
*  Read in 2D parameters if this is a 2D experiment.  *
******************************************************/
   strcpy(filename,file_info->in_file);
   strcat(filename,"/acqu2s");
   if (!(f2 = fopen(filename,"r")))  {
     data->ni = 1;
     data->sw1 = 0.0;
     data->in = 0.0;
     file_info->twod = FALSE;
     }
   else  {
     if (bin_file)  {
	strcpy(filename,file_info->out_dir);
	strcat(filename,"/acqu2s");
	if (!(f3 = fopen(filename,"w")))  {
	  printf("read_amx_header: Error - file \"%s\" not found\n",
		filename);
	  return(ERROR);
	  }
	amx_bin_to_ascii_header(f2,f3);
	fclose(f2);   fclose(f3);
	if (!(f2 = fopen(filename,"r")))  {
	  printf("read_amx_header: Error - file \"%s\" not found\n",
		filename);
	  return(ERROR);
	  }
	}
     read_acqu2s(f2,data,file_info);
     fclose(f2);
     file_info->twod = TRUE;
     }

/**********************************
*  Calculate minimum FN and FN1.  *
**********************************/

   data->fn = MINFN;
   while (data->fn < data->np)
      data->fn *= 2; 

   if (data->ni > 1)
   {
      data->fn1 = MINFN;
      while (data->fn1 < data->ni)
         data->fn1 *= 2;
   }
   return(OK);
}

/*---------------------------------------
|					|
|	is_varian_param()		|
|					|
+--------------------------------------*/
int is_varian_param(str,data)
char *str;
data_struct *data;
{
    int i;

    for (i=0;i<data->num_bruker_par;i++)
      if (strcmp(data->bruker_par_list[i],str) == 0)
	return(TRUE);
    return(FALSE);
}

/*---------------------------------------
|					|
|	is_skip_param()			|
|					|
+--------------------------------------*/
int is_skip_param(str,data)
char *str;
data_struct *data;
{
    int i;

    for (i=0;i<data->num_skip_par;i++)  {
      if (strcmp(data->skip_par_list[i],str) == 0)
	return(TRUE);
      }
    return(FALSE);
}

#define MAX_LINE 133
/*---------------------------------------
|					|
|	convert_all_params()		|
|					|
+--------------------------------------*/
convert_all_params(f1,f2,file_info,data)
FILE *f1,*f2;
file_info_struct *file_info;
data_struct *data;
{
    int i, beg, end, done;
    char line[MAX_LINE+1], *ptr;
    char param_name[MAX_PAR_LEN],param_string[MAX_LINE];
    double double_num;
    int is_varian_param(), is_skip_param();

  while (fgets(line,MAX_LINE,f1) != NULL)  {
    ptr = line;
    while (*ptr == '#')  {ptr++;}
    if (*ptr == '$')  {
      param_name[0] = '\0';
      ptr++;
      i = 0;
      while (isalnum(*ptr) || (*ptr == '_'))  {
	param_name[i] = tolower(*ptr);
	ptr++; i++;
	}
      param_name[i] = '\0';
      if (is_varian_param(param_name,data))
	strcat(param_name,"b");
      if ((i>0) && !is_skip_param(param_name,data))  {
        while (*ptr != '=')  ptr++;
        ptr++;
        while (*ptr == ' ')  ptr++;
        if (*ptr == '<')  {
	  ptr++;
	  i = 0;
          while (*ptr != '>') {
            if (*ptr == '\0')
            {
               fgets(line,MAX_LINE,f1);
               ptr = line;
            }
            if (i < MAX_LINE)
	       param_string[i] = *ptr;
	    ptr++; i++;
	    }
	  param_string[i] = '\0';
	  /* get rid of bruker strings with control characters */
	  i = 0;  done = FALSE;
	  while ((i<strlen(param_string)) && (!done))  {
	    if (param_string[i] < 32)  {
	      param_string[i] = '\0';
	      done = TRUE;
	      }
	    i++;
	    }
	  fprintf(f2,"%s\"18Y\" = %s\n",param_name,param_string);
	  }
        else if (*ptr == '(')  {
	  if ((strncmp(param_name,"routwd1",7)==0)||
		(strncmp(param_name,"routwd2",7)==0))  {
	    for (i=3;i<7;i++)
	      param_name[i-2] = param_name[i];
	    param_name[5] = '\0';
	    }
	  ptr++;
	  sscanf(ptr,"%d",&beg);
	  while (*ptr != '.')  ptr++;
	  while (*ptr == '.')  ptr++;
	  sscanf(ptr,"%d",&end);
	  for (i=beg;i<=end;i++)  {
	    fscanf(f1,"%lf",&double_num);
	    sprintf(param_string,"%s%d",param_name,i);
            if (!is_varian_param(param_string,data))
              fprintf(f2,"%s  = %g\n",param_string,double_num);
	    }
	  fgets(line,MAX_LINE,f1);
	  }
        else  {
	  sscanf(ptr,"%lf",&double_num);
	  fprintf(f2,"%-9s = %g\n",param_name,double_num);
	  }
        }
      }
    }
}

/*---------------------------------------
|					|
|		write_amx_stext()	|
|					|
+--------------------------------------*/
write_amx_stext(file_info,data)
file_info_struct *file_info;
data_struct *data;
{
   char outname[MAXPATHL], filename[MAXPATHL];
   FILE *f1, *f2;
   int i;
   struct tm *time_struct;
   char timebuf[MAX_LINE+1], datebuf[MAX_LINE+1], line[MAX_LINE+1];

/*******************************************
*  Write out parameter data to text file.  *
*******************************************/
   strcpy(outname, file_info->out_dir);
#ifdef VMS
   strcat(outname, "stext");
#else
   strcat(outname, "/stext");
#endif
   if ((f2 = fopen(outname, "w")) == 0)
   {
      printf("Could not open %s\n", outname);
      return(ERROR);
   }

   if ( (file_info->twod) && (data->ni > 1) )
     fprintf(f2, "datatype  = 3\n");
   else
     fprintf(f2, "datatype  = 1\n");
   fprintf(f2, "system    = bruker\n");
   if ( (file_info->twod) && (data->ni > 1) )
   {
     fprintf(f2, "dim1      = 2\n");
     fprintf(f2, "dim2      = %d\n", data->ni/2);
   }
   else
   {
     fprintf(f2, "dim1      = 1\n");
     fprintf(f2, "dim2      = 1\n");
   }
   fprintf(f2, "dim3      = 1\n");
   fprintf(f2, "np        = %d\n", data->np);
   fprintf(f2, "sw        = %.7f\n", data->sw);
   fprintf(f2, "sfrq      = %.7f\n", data->sfrq);
   if ( (file_info->twod) && (data->ni > 1) )
     fprintf(f2, "sw1       = %.7f \n", data->sw1);
   fprintf(f2, "dfrq      = %.7f\n", data->dfrq);
   fprintf(f2, "bytes     = 4\n");
   fprintf(f2, "order     = 1\n");
   if (file_info->aqmod == SEQUENTIAL_DATA)
     fprintf(f2, "acqtyp    = 1\n");
   else
     fprintf(f2, "acqtyp    = 0\n");
   if ( (file_info->twod) && (data->ni > 1) )
     fprintf(f2, "abstype   = 1\n");

   fprintf(f2, "#TEXT:\n");
   fprintf(f2, "    Bruker Data\n");
/* read any text from "info" file and put it into stext */
   strcpy(filename,file_info->in_file);
   strcat(filename,"/info");
   if (f1 = fopen(filename,"r"))  {
     while (fgets(line,MAX_LINE,f1))
       fputs(line,f2);
     fclose(f1);
     }
   fprintf(f2, "#PARAMETERS:\n");
   fprintf(f2, "seqfil    = %s\n",data->seqfil);
   fprintf(f2, "pslabel   = %s\n",data->seqfil);
   time_struct = localtime((long*)&(data->date));
   strftime(datebuf,MAX_LINE,"%m%d%y",time_struct);
   strftime(timebuf,MAX_LINE,"%R:%S",time_struct);
   fprintf(f2, "date      = %s\n",datebuf);
   fprintf(f2, "time\"18Y\" = %s\n",timebuf);
   fprintf(f2, "file      = %s\n", file_info->out_dir);
   if (file_info->aqmod == SEQUENTIAL_DATA)
     fprintf(f2, "proc      = rft\n");
   else
     fprintf(f2, "proc      = ft\n");

   if ( (file_info->twod) && (data->ni > 1) )
   {
      fprintf(f2, "ni        = %d\n", data->ni/2);
      fprintf(f2, "sw1       = %.7f\n", data->sw1);
   }

   fprintf(f2, "tn        = %s\n",data->tn);
   fprintf(f2, "dn        = %s\n",data->dn);
   fprintf(f2, "pw        = %g\n", data->pw);
   fprintf(f2, "pw90      = %g\n", data->pw90);
   fprintf(f2, "rd        = %g\n", data->rd);
   fprintf(f2, "nt        = %d\n", data->nt);
   fprintf(f2, "ct        = %d\n", data->ct);
   fprintf(f2, "ss        = %d\n", data->ss);
   fprintf(f2, "tof       = %f\n", data->tof);
   fprintf(f2, "dof       = %f\n", data->dof);
   fprintf(f2, "fb        = %g\n", data->fb);
   fprintf(f2, "at        = %.7f\n", data->at);
   fprintf(f2, "dw        = %.7g\n", data->dw);
   fprintf(f2, "de        = %g\n", data->de);
   fprintf(f2, "dp        = y\n");
   fprintf(f2, "temp      = %g\n", data->temp);
   fprintf(f2, "spin      = %d\n",data->spin);
   fprintf(f2, "gain      = %d\n", data->rg);
   fprintf(f2, "fn        = n\n");
   fprintf(f2, "rp        = 0.0\n");
   fprintf(f2, "lp        = 0.0\n");
   fprintf(f2, "rfl       = 0.0\n");
   fprintf(f2, "rfp       = 0.0\n");
   fprintf(f2, "lb        = n\n");
   fprintf(f2, "gf        = n\n");
   fprintf(f2, "sb        = n\n");
   fprintf(f2, "awc       = n\n");
   fprintf(f2, "wp        = %.7f\n", data->sw);
   fprintf(f2, "sp        = %f\n", 0.0);

   if ( (file_info->twod) && (data->ni > 1) )
   {
      fprintf(f2, "proc1     = rft\n");
      fprintf(f2, "fn1       = n\n");
      fprintf(f2, "rp1       = 0.0\n");
      fprintf(f2, "lp1       = 0.0\n");
      fprintf(f2, "rfl1      = 0.0\n");
      fprintf(f2, "rfp1      = 0.0\n");
      fprintf(f2, "lb1       = n\n");
      fprintf(f2, "gf1       = n\n");
      fprintf(f2, "sb1       = n\n");
      fprintf(f2, "awc1      = n\n");
      fprintf(f2, "wp1       = %.7f\n", data->sw1);
      fprintf(f2, "sp1       = 0.0\n");
      fprintf(f2, "arraydim  = %d\n",data->ni);
      fprintf(f2, "arraye    = 2\n");
      fprintf(f2, "phase\"array1\"=\n");
      fprintf(f2, "phase        = 1\n");
      fprintf(f2, "phase        = 4\n");
   }

/*******************************************
*  Convert all other parameters in acqus.  *
*******************************************/
   strcpy(filename,file_info->out_dir);
   strcat(filename,"/acqus");
   if (!(f1 = fopen(filename,"r")))  {
     strcpy(filename,file_info->in_file);
     strcat(filename,"/acqus");
     if (!(f1 = fopen(filename,"r")))  {

       printf("write_amx_stext: Error - file \"%s\" not found\n",
		filename);
       return(ERROR);
       }
     }
   convert_all_params(f1,f2,file_info,data);

   /* don't know why sread requires this next part, but it does... */
   fprintf(f2, "#ACQDAT:\n");
   for (i = 0; i < data->ni; i++)
        fprintf(f2, "    %d    =    0  \n", data->nt);

   fclose(f2);
   return(OK);
}

/*---------------------------------------
|					|
|		read_am_header()	|
|					|
+--------------------------------------*/
int  read_am_header(file_info,data)
file_info_struct *file_info;
data_struct *data;
{
   char		dirname[MAXPATHL],
		buf[(HDR_SIZE+HDR_SHIFT)*X32_SIZE];
   FILE		*f1;
   int		i, dummy;
   char		*char_ptr;
   struct stat	unix_fab; /* unix_fab.st_size is file size */


    if (!(f1 = fopen(file_info->in_file,"r")))  {
      printf("read_am_header: Error - file \"%s\" not found\n",
		file_info->in_file);
      return(ERROR);
      }

    if (fread(buf,sizeof(u_char),(HDR_SIZE+file_info->hdr_shift)*
		file_info->word_size,f1)!=(HDR_SIZE+file_info->hdr_shift)*
		file_info->word_size) {
      printf("read_am_header: Error reading file \"%s\" header \n",
		file_info->in_file);
      fclose(f1);
      return(ERROR);
      }
    if (stat(file_info->in_file,&unix_fab))  {
      printf("read_am_header: Error - file \"%s\" not found\n",
		file_info->in_file);
      return(ERROR);
      }

   strcpy(dirname, file_info->out_dir);

/**********************************************
*  Read in basic NMR acquisition parameters.  *
**********************************************/
	/* number of transients:	   */
   data->ct = convert_int(buf,CT_OFFSET+file_info->hdr_shift,file_info);
	/* number of transients:	NS */
   data->nt = convert_int(buf,NT_OFFSET+file_info->hdr_shift,file_info);
	/* number of data points:	TD */
   data->np = convert_int(buf,NP_OFFSET+file_info->hdr_shift,file_info);
   if (unix_fab.st_size < (int) ((file_info->hdr_blocks*(HDR_SIZE +
		file_info->hdr_shift) + data->np)*file_info->word_size))
     data->np = (unix_fab.st_size-file_info->hdr_blocks*(HDR_SIZE
			+ file_info->hdr_shift)*file_info->word_size)/
			file_info->word_size;
	/* dwell time in us		DW */
   data->dw = convert_int(buf,DW_OFFSET+file_info->hdr_shift,file_info);
   data->at = data->dw*data->np/1.0e+6;
	/* filter bandwidth            	FB */
   data->fb = convert_int(buf,FB_OFFSET+file_info->hdr_shift,file_info);
	/* steady-state transients:	DS */
   data->ss = convert_int(buf,SS_OFFSET+file_info->hdr_shift,file_info);
	/* number of t1 increments:	NE */
   data->ni = convert_int(buf,NI_OFFSET+file_info->hdr_shift,file_info);
	/* relaxation delay:		RD */
   data->rd = convert_delay(buf,RD_OFFSET+file_info->hdr_shift,file_info);
   if (data->rd == 0)	/* if rd == 0 set rd to d1 */
     data->rd = convert_delay(buf,DELAY_OFFSET+1+file_info->hdr_shift,
			file_info);
	/* pulse width:			PW */
   data->pw = 1e+6*convert_delay(buf,PW_OFFSET+file_info->hdr_shift,file_info);
   if (data->pw == 0.0)
     data->pw=1e+6*convert_delay(buf,PULSE_OFFSET+file_info->hdr_shift,
		file_info);
	/* temperature:			TE */
   data->temp = (double)convert_int(buf,TEMP_OFFSET+file_info->hdr_shift,file_info) - 273;
	/* sweep width:			SW */
   data->sw = convert_real(buf,SW_OFFSET+file_info->hdr_shift,file_info);
	/* observe offset:		O1 */
   data->tof = convert_real(buf, TOF_OFFSET+file_info->hdr_shift,file_info);
	/* decoupler offset:		O2 */
   data->dof = convert_real(buf, DOF_OFFSET+file_info->hdr_shift, file_info);
	/* observe synthesizer frequency;	SFRQ */
   /* it can also be calculated from the sum of pos(58) and pos(119) */
   data->sfrq = convert_real(buf, SFRQ_OFFSET+file_info->hdr_shift, file_info);
	/* decoupler synthesizer frequency;	DFRQ */
   data->dfrq = convert_real(buf,DFRQ_OFFSET+file_info->hdr_shift,file_info);
   if ((data->dfrq < MINFREQ) && (file_info->dec_frq == FILE_INFO_UNSET)) {
     file_info->dec_frq = data->sfrq;
     printf("DFRQ being set to SFRQ\n");
     }
   else if (file_info->dec_frq == FILE_INFO_UNSET) {
     file_info->dec_frq = data->dfrq;
     }
   data->dfrq = file_info->dec_frq;

	/* receiver dead time:		DE */
   data->de = convert_delay(buf, DE_OFFSET+file_info->hdr_shift, file_info);
   data->de *= 1e+6;

	/* time				 */
   data->time = convert_int(buf, TIME_OFFSET+file_info->hdr_shift,file_info);
	/* date				 */
   data->date = convert_int(buf, DATE_OFFSET+file_info->hdr_shift,file_info);
	/* spin				 */
   data->spin = convert_int(buf, SPIN_OFFSET+file_info->hdr_shift,file_info);
	/* satdly			 */
   data->satdly=convert_delay(buf, SATDLY_OFFSET+file_info->hdr_shift,file_info);
	/* receiver gain		 */
   data->rg = convert_int(buf, RG_OFFSET+file_info->hdr_shift,file_info);
/******************************************************
*  Read in 2D parameters if this is a 2D experiment.  *
******************************************************/

    /* check to see that the file is large enough to contain "ni" FIDs */
   if ((data->ni > 1) && (unix_fab.st_size >= file_info->word_size*
		(data->ni*data->np+file_info->hdr_blocks*
		(HDR_SIZE+file_info->hdr_shift))))
   {
		/* delta_t1 per increment */
      data->in = convert_delay(buf, DW1_OFFSET+file_info->hdr_shift, file_info);
      if (data->in == 0.0)
      {
         data->sw1 = 0.0;			/* safety value for SW1	  */
      }
      else
      {
         data->sw1 = 1.0/(2.0*data->in); 	/* F1 spectral width      */
      }
      file_info->twod = TRUE;
   }
   else
   {
      data->ni = 1;
      file_info->twod = FALSE;
   }

/**************************
*  Read in power levels.  *
**************************/

   data->dpwr = convert_int(buf,DECPWR_OFFSET+file_info->hdr_shift,file_info);
   if (data->dpwr >= HIGHPOWER)
   {
      data->dpwr -= (HIGHPOWER - AMX_HIGHPOWER);
   }
   else
   {
      data->dpwr -= (LOWPOWER - AMX_HIGHPOWER);
   }
   for (i = 0; i < MAXNUMPOWERS; i++)  {
      data->s[i] = convert_int(buf,POWER_OFFSET+file_info->hdr_shift + i,
			file_info);
      if (data->s[i] >= HIGHPOWER)
      {
         data->s[i] -= (HIGHPOWER - AMX_HIGHPOWER);
      }
      else
      {
         data->s[i] -= (LOWPOWER - AMX_HIGHPOWER);
      }
     }

/*****************************************
*  Read in all 10 delays and 10 pulses.  *
*****************************************/

   for (i = 0; i < MAXNUMDELAYS; i++)
      data->delay[i] = convert_delay(buf, DELAY_OFFSET+file_info->hdr_shift + i,
			file_info);

   for (i = 0; i < MAXNUMPULSES; i++)
   {
      data->pulse[i] = 1e+6*convert_delay(buf, 
		PULSE_OFFSET+file_info->hdr_shift + i, file_info);
   }

   if (debug)
      printf("NT = %d, NI = %d, NP = %d\n", data->nt, data->ni, data->np);

/**********************************
*  Calculate minimum FN and FN1.  *
**********************************/

   data->fn = MINFN;
   while (data->fn < data->np)
      data->fn *= 2; 

   if (data->ni > 1)
   {
      data->fn1 = MINFN;
      while (data->fn1 < data->ni)
         data->fn1 *= 2;
   }
   fclose(f1);
   return(OK);
}

/*---------------------------------------
|					|
|		write_stext()		|
|					|
+--------------------------------------*/
write_stext(file_info,data)
file_info_struct *file_info;
data_struct *data;
{
   char outname[MAXPATHL];
   FILE *f2;
   int i, temp;

/*******************************************
*  Write out parameter data to text file.  *
*******************************************/
   strcpy(outname, file_info->out_dir);
#ifdef VMS
   strcat(outname, "stext");
#else
   strcat(outname, "/stext");
#endif
   if ((f2 = fopen(outname, "w")) == 0)
   {
      printf("Could not open %s\n", outname);
      return(ERROR);
   }

   if ( (file_info->twod) && (data->ni > 1) )
     fprintf(f2, "datatype  = 3\n");
   else
     fprintf(f2, "datatype  = 1\n");
   fprintf(f2, "system    = bruker\n");
   if ( (file_info->twod) && (data->ni > 1) )
   {
     fprintf(f2, "dim1      = 2  \n");
     fprintf(f2, "dim2      = %d  \n", data->ni/2);
   }
   else   {
     fprintf(f2, "dim1      = 1  \n");
     fprintf(f2, "dim2      = 1  \n");
     }
   fprintf(f2, "dim3      = 1  \n");
   fprintf(f2, "np        = %d \n", data->np);
   fprintf(f2, "sw        = %.7f \n", data->sw);
   fprintf(f2, "sfrq      = %.7f \n", data->sfrq+(data->tof/1.0e+6));
   if ( (file_info->twod) && (data->ni > 1) )
     fprintf(f2, "sw1       = %.7f \n", data->sw1);
   fprintf(f2, "dfrq      = %.7f \n", data->dfrq+(data->dof/1.0e+6));
   fprintf(f2, "bytes     = 4  \n");
   fprintf(f2, "order     = 1  \n");
   if (file_info->aqmod == SEQUENTIAL_DATA)
     fprintf(f2, "acqtyp    = 1  \n");
   else
     fprintf(f2, "acqtyp    = 0  \n");
   if ( (file_info->twod) && (data->ni > 1) )
      fprintf(f2, "abstype   = 1  \n");

   fprintf(f2, "#TEXT:\n");
   fprintf(f2, "    Bruker Data \n");

   fprintf(f2, "#PARAMETERS:\n");
   fprintf(f2, "seqfil    = bruker\n");
   fprintf(f2, "pslabel   = bruker\n");
   fprintf(f2, "date      = ");
   temp = (data->date&0xFF00)>>8;
   if (temp < 16)
     fprintf(f2,"0");
   fprintf(f2, "%x",temp);
   temp = data->date>>16;
   if (temp < 16)
     fprintf(f2,"0");
   fprintf(f2, "%x",temp);
   temp = data->date&0xFF;
   if (temp < 16)
     fprintf(f2,"0");
   fprintf(f2, "%x\n",temp);
   fprintf(f2, "time\"18Y\" = ");
   temp = (data->time&0xFF0000)>>16;
   if (temp < 16)
     fprintf(f2,"0");
   fprintf(f2, "%x:",temp);
   temp = (data->time&0xFF00)>>8;
   if (temp < 16)
     fprintf(f2,"0");
   fprintf(f2, "%x:",temp);
   temp = data->time&0xFF;
   if (temp < 16)
     fprintf(f2,"0");
   fprintf(f2, "%x\n",temp);
   fprintf(f2, "file      = %s\n", file_info->out_dir);
   fprintf(f2, "proc      = rft\n");
   

   if ( (file_info->twod) && (data->ni > 1) )
   {
      fprintf(f2, "ni        = %d\n", data->ni/2);
      fprintf(f2, "sw1       = %.7f\n", data->sw1);
   }

   fprintf(f2, "tn        = UNKNOWN\n");
   fprintf(f2, "dn        = UNKNOWN\n");
   fprintf(f2, "pw        = %g\n", data->pw);
   fprintf(f2, "pw90      = %g\n", data->pulse[1]);
   fprintf(f2, "rd        = %g\n", data->rd);
   fprintf(f2, "nt        = %d\n", data->nt);
   fprintf(f2, "ct        = %d\n", data->ct);
   fprintf(f2, "ss        = %d\n", data->ss);
   fprintf(f2, "tof       = %f\n", data->tof);
   fprintf(f2, "dof       = %f\n", data->dof);
   fprintf(f2, "fw        = %g\n",data->fb);
   fprintf(f2, "fb        = %g\n",(double)100.0*((int)(((1.1*data->sw/2.0)/100)+0.5)));
   fprintf(f2, "at        = %.7f\n", data->at);
   fprintf(f2, "dw        = %.7g\n", data->dw);
   fprintf(f2, "de        = %g\n", data->de);
   fprintf(f2, "dp        = y\n");
   fprintf(f2, "dpwr      = %d\n",data->dpwr);
   fprintf(f2, "temp      = %g\n", data->temp);
   fprintf(f2, "rg        = %d\n", data->rg);
   fprintf(f2, "gain      = %d\n", data->rg);
   fprintf(f2, "fn        = n\n");
   fprintf(f2, "rp        = 0.0\n");
   fprintf(f2, "lp        = 0.0\n");
   fprintf(f2, "rfl       = 0.0\n");
   fprintf(f2, "rfp       = 0.0\n");
   fprintf(f2, "lb        = n\n");
   fprintf(f2, "gf        = n\n");
   fprintf(f2, "sb        = n\n");
   fprintf(f2, "awc       = n\n");
   fprintf(f2, "wp        = %.7f\n", data->sw);
   fprintf(f2, "sp        = %f\n", 0.0);

   if ( (file_info->twod) && (data->ni > 1) )
   {
      fprintf(f2, "proc1     = rft\n");
      fprintf(f2, "fn1       = n\n");
      fprintf(f2, "lp1       = 0.0\n");
      fprintf(f2, "rp1       = 0.0\n");
      fprintf(f2, "rfl1      = 0.0\n");
      fprintf(f2, "rfp1      = 0.0\n");
      fprintf(f2, "lb1       = n\n");
      fprintf(f2, "gf1       = n\n");
      fprintf(f2, "sb1       = n\n");
      fprintf(f2, "awc1      = n\n");
      fprintf(f2, "wp1       = %.7f\n", data->sw1);
      fprintf(f2, "sp1       = 0.0\n");
      fprintf(f2, "id        = %.7f\n",data->in);
      fprintf(f2, "arraydim  = %d\n",data->ni);
      fprintf(f2, "arraye    = 2\n");
      fprintf(f2, "phase\"array1\"=\n");
      fprintf(f2, "phase        = 1\n");
      fprintf(f2, "phase        = 4\n");
   }

   fprintf(f2, "spin      = %d\n",data->spin&0x3F);
   fprintf(f2, "satdly    = %g\n",data->satdly);

   fprintf(f2, "sfo1      = %g\n",data->sfrq);
   fprintf(f2, "sfo2      = %g\n",data->dfrq);
   fprintf(f2, "o1        = %g\n",data->tof);
   fprintf(f2, "o2        = %g\n",data->dof);
   fprintf(f2, "fw        = %g\n",data->fb);
   fprintf(f2, "td        = %d\n",data->np);
   fprintf(f2, "ns        = %d\n",data->nt);
   fprintf(f2, "ds        = %d\n",data->ss);
   fprintf(f2, "te        = %g\n",data->temp+273);
   fprintf(f2, "ro        = %d\n",data->spin&0x3F);

   for (i = 0; i < MAXNUMDELAYS; i++)
      fprintf(f2, "d%c        = %g\n", '0' + i, data->delay[i]);
   for (i = 0; i < MAXNUMPULSES; i++)
      fprintf(f2, "p%c        = %g\n", '0' + i, data->pulse[i]);

   for (i = 0; i < MAXNUMPOWERS; i++)
   {
         fprintf(f2, "s%c        = %d\n", '0' + i, data->s[i]);
   }

   fprintf(f2, "#ACQDAT:\n");
   for (i = 0; i < data->ni; i++)
        fprintf(f2, "    %d    =    0\n", data->nt);

   fclose(f2);
   return(OK);
}

/*---------------------------------------
|                                       |
|            convertfile()/1            |
|                                       |
+--------------------------------------*/
int  convertfile(file_info, data)
file_info_struct *file_info;
data_struct *data;
{
   char outname[MAXPATHL], data_file[MAXPATHL],
	*buf;
   int  outbuffer[OUT_BLOCK_SIZE],
        cntr,
        i,
	ibuf,
        ix,
        done,
	totpts,
	factor;
   FILE *f1,
        *f2;
 
   strcpy(outname, file_info->out_dir);
#ifdef VMS
   strcat(outname, "sdata");
#else
   strcat(outname, "/sdata");
#endif

   strcpy(data_file,file_info->in_file);
   if (file_info->spectrometer == AMX_SPEC)  {
     if (file_info->twod)
       strcat(data_file,"/ser");
     else
       strcat(data_file,"/fid");
     }
 
   if (!(f1 = fopen(data_file,"r")))  {
      printf("convertfile: Error - file \"%s\" not found\n",
		data_file);
      return(ERROR);
      }
 
   if ((f2 = fopen(outname, "w+")) == 0)
   {
      printf("Cannot not open %s\n", outname);
      fclose(f1);
      return(ERROR);
   }
 
 if (file_info->spectrometer == AM_SPEC)  {
   if (file_info->file_type == HDR_AND_DATA)  {
     if ((buf = (char *)malloc(sizeof(char)*(HDR_SIZE+file_info->hdr_shift)*
					file_info->word_size)) == NULL)  {
       printf("convertfile: Error allocating memory for input buffer\n");
       fclose(f1);
       return(ERROR);
       }
     if (fread(buf,sizeof(char),(HDR_SIZE+file_info->hdr_shift)*
		file_info->word_size,f1)!=
                (HDR_SIZE+file_info->hdr_shift)*file_info->word_size)  {
       printf("convertfile: Error reading file \"%s\" header \n",
             file_info->in_file);
       fclose(f1);
       return(ERROR);
       }
	/* read second header block, if present */
      
     if (file_info->hdr_blocks == 2)  {
       if (fread(buf,sizeof(u_char),(HDR_SIZE+file_info->hdr_shift)*
		file_info->word_size,f1)!= 
                (HDR_SIZE+file_info->hdr_shift)*file_info->word_size)  {
         printf("convertfile: Error reading file \"%s\" header \n",
             file_info->in_file);
         fclose(f1);
         return(ERROR);
         }
       }
     free(buf);
     }
   else if (file_info->skip_words > 0)  {
     if ((buf = (char *)malloc(sizeof(char)*file_info->skip_words*
					file_info->word_size)) == NULL)  {
       printf("convertfile: Error allocating memory for input buffer\n");
       fclose(f1);
       return(ERROR);
       }
     if (fread(buf,sizeof(u_char),file_info->skip_words*file_info->word_size,
		f1)!= file_info->skip_words*file_info->word_size)  {
       printf("convertfile: Error skipping words in \"%s\" header \n",
             file_info->in_file);
       fclose(f1);
       return(ERROR);
       }
     free(buf);
     }
   }
 else if (file_info->skip_words > 0)  {
   if ((buf = (char *)malloc(sizeof(char)*file_info->skip_words*
				file_info->word_size)) == NULL)  {
     printf("convertfile: Error allocating memory for input buffer\n");
     fclose(f1);
     return(ERROR);
     }
   if (fread(buf,sizeof(u_char),file_info->skip_words*file_info->word_size,
		f1)!= file_info->skip_words*file_info->word_size)  {
     printf("convertfile: Error skipping words in \"%s\" header \n",
             file_info->in_file);
     fclose(f1);
     return(ERROR);
     }
   free(buf);
   }

   cntr = 0;
   totpts = 0;
   done = feof(f1);
 
   factor = 1;
   while ((!done) && (cntr < OUT_BLOCK_SIZE))
   {
      if (fread(&ibuf,file_info->word_size,1,f1) != 1)
	done = TRUE;
      if (!done)  {
	if (file_info->spectrometer == AM_SPEC)
	  ix = convert_int((char *)(&ibuf), 0, file_info );
	else  {
	  ix = ibuf;
	  if (file_info->byte_order == LSB_FIRST)
	    swap_bytes(&ix);
	  }
	outbuffer[cntr] = (factor > 0) ? ix : -ix;
	cntr++;
	totpts++;
	if (((totpts+data->np) % (2*data->np)) == 0)
	  factor = -factor;
	done = feof(f1);
	/* if header was present, check if all data has been processed */
	if (file_info->file_type == HDR_AND_DATA)
	  if (totpts == data->ni*data->np)
	    done = TRUE;
	}
      if ((cntr == OUT_BLOCK_SIZE) || done) {
	if ((file_info->spectrometer == AM_SPEC)||(file_info->aqmod ==
			SEQUENTIAL_DATA))
          for (i = 0; i < cntr; i = i + 4) {
            putw(outbuffer[i], f2);
            putw(-outbuffer[i + 1], f2);
            putw(-outbuffer[i + 2], f2);
            putw(outbuffer[i + 3], f2);
	    }
	else
          for (i = 0; i < cntr; i = i + 4) {
            putw(outbuffer[i], f2);
            putw(-outbuffer[i + 1], f2);
            putw(outbuffer[i + 2], f2);
            putw(-outbuffer[i + 3], f2);
	    }
        cntr = 0;
        }
     }
 
   fclose(f1);
   fclose(f2);
 
   return(OK);
}

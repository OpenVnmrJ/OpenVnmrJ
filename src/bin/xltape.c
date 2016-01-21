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
/*-----------------------------------------------------------------------
|    xltape 
|	- unix program to convert files to the VXR-4000 9-track tape format. 
|    complements VXR-5000 program 'tape' which reads tapes in this format.
|    use the file name in the argument as a file name, append a header,
|    and make new file: filename.xl
|    'Tapedata' does this.
|
|    for downloading text use alias t_xltape.
|    first use unix programs expand to remove tabs.
|    then  use unix program tr a-z A-Z if capital letters are
|       desired in downloaded source. 
|    t_xltape sets 8th bit of characters as required by VXR-4000.
|    'Tapetext' does all this.
|
|    for directories made by 'swrite', 'To_VXR4000' does everything.
|    these directories contain a file called 'stext' and one called 'sdata'.
|    the VXR-4000 generally cannot use files containg mixed data and text.
|
|    macro for doing tape operations ('Totape')
#
foreach i ($argv)
echo $i out
dd obs=10240 if=$i of=/dev/rmt12
end
|
|     makefile for xltape
|
xltape: xltape.o
	    cc -fsingle -o xltape xltape.o -ll -lm
xltape.o       : xltape.c
	cc -fsingle -c xltape.c
|
+----------------------------------------------------------------------*/
/*  from ultrix xltape.c 6.1   3/9/87  */

#include <stdio.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <strings.h>
#include <ctype.h>
#include <sys/time.h>

#define PMODE 0644;


char buff[512];
int spare;
struct timeval tp; struct timezone tzp;
struct tm *ralph;
int textflag;

main(argc,argv)
int argc; char *argv[];
{
  int ii,jj,length;
  char xl_key[80],name[80];
  time_it();
  if (strcmp(argv[0],"t_xltape") == 0) textflag = 1; else textflag = 0;
  for (ii = 1; ii < argc; ii++) {
     length = filelength(argv[ii]);
     if (length > 0) {
     tail(argv[ii],name); /* have tail end of name */
     xlname(name,xl_key);
     jj = length % 512;
     length /= 512;
     if (jj > 0) { spare = 512 - jj; length += 2;}
     else { length += 1; spare = 0; }
     makeheader(xl_key,length);
     makenewfile(argv[ii],name);
     } else printf("%s not found \n",argv[ii]);
}}


filelength(key) char *key; 
{ struct stat stbuf;
  if (stat(key,&stbuf) < 0) return(0); else
  return(stbuf.st_size);
  }

char tag1[20] = "VARIAN VXR/VIS 7";
char tag2[20] = "XL/VXR/VIS   END";
char tag3[20] = "downld";

makeheader(key,len) char *key; int len;
{ char *gg,*hh,place[8]; int i,j; 
  short kk,ll,*kf;
  for (i = 0; i < 512; i++) buff[i] = ' ';
  i = 0;
  gg = key;
  hh = buff + 40;
  while ((*gg != '\0') && (i < 6)) {*hh++ = *gg; buff[i++] = *gg++; }
  gg = buff+20; 
  for (i = 0; i < 16; i++) *gg++ = tag1[i];
  gg = buff+38;
  if (textflag == 1) 
  { *gg++ = 0x02; *gg = 0x0;} else { *gg++ = 0x3; *gg = 0xd; }
  gg = buff+36;
  kf = (short *) (buff + 36);
  *kf = (ralph->tm_mon+1 + 12*(ralph->tm_year-83))*100+ralph->tm_mday;
  /* swab(gg,gg,2); */
  gg = buff+46;
  for (i = 0; i < 6; i++) *gg++ = tag3[i];
  gg = buff+496; 
  for (i = 0; i < 16; i++) *gg++ = tag2[i];
  /* hard code blocksize */
  gg = buff + 16;
  *gg++ = 0x28; 
  *gg++ = 0x0;
  /* swab filelength */
  kk = (short) len;
  ll = (kk >> 8) & 0xff;
  kk %= 256;
  gg = buff + 18;
  *gg++ = (char) ll;
  *gg = (char) kk;
  kf = (short *) (buff + 52);
  *kf++ = ralph->tm_mon+1;
  *kf++ = ralph->tm_mday;
  *kf++ = ralph->tm_year;
  *kf++ = ralph->tm_hour*60+ralph->tm_min;
  gg = buff +52;
  /* swab(gg,gg,8); */
  gg = buff + 60;
  *gg++ = 0x0;
  *gg++ = 0x1;
  }

/*
   512 byte header (one sector) header contains:
   0  file name string          :text16;
   16 blocksize (default=10240) :integer;
   18 sectors(filefcb.i4+1)     :integer;
   20 'Varian XR/VIS   ',       :text16;  VXR
   36 filfcb.i2                 :integer;
   38 filfcb.i1                 :text2;
   40 filename                  :name;
   46 user                      :name;
   52 date                      :trn;
   58 time(minutes)             :integer;
   60 tapefile no (first=1)     :integer;
   62 blanks to 496             :array[1..434] of char;
   496 'XL/XR/VIS    end'       :text16;
*/  

makenewfile(src,key) 
char *src,*key; 
{
  int fin,num_in,fout,num_out,i;
  char outn[20],blnk;
  strcpy(outn,key);
  strcat(outn,".xl");
  if (textflag == 1) blnk = 0x20; else blnk = 0x0;
  fin = open(src,0644);
  if (fin > 0) {
  fout = creat(outn,0644);
  num_out = write(fout,buff,512);
  if (num_out != 512) printf("ekk\n");
  while ((num_in = read(fin,buff,512)) > 0) {
  if (num_in < 512) { 
  if (textflag == 1) buff[num_in++] = 0x19;
  for (i = num_in; i < 512; i++) buff[i] = blnk; }
  /* if (textflag == 0) swab(buff,buff,512); on vax only */
  if (textflag) set8(buff,512);
  num_out = write(fout,buff,512);
  }
  } else printf("%s not found\n",src);
  }

set8(in,numb)
char *in; int numb;
{int i; for (i = 0; i < numb; i++) *in++ |= 0x80; in++; }

tail(inarg,outarg) /* gets local file name */
char *inarg,*outarg;
{
    char *dd,*gg,tmp;
    gg = outarg;
    dd = inarg;
    while ( *dd != '\0') {
    tmp = *dd++;
    if (tmp == '/') gg = outarg;
    else { *gg++ = tmp; } }
    *gg = '\0';
    }

xlname(inarg,outarg)
char *inarg,*outarg;
{
    char *dd,*gg,tmp; int jj;
    gg = outarg;
    dd = inarg;
    jj = 1;
    while ((tmp = *dd++) != '\0') {
    if (islower(tmp)) tmp -= 040;
    *gg++ = tmp; jj++;}
    while (jj++ < 7) { *gg++ = ' ';}}


time_it()
{  
   gettimeofday(&tp,&tzp);
   ralph = localtime(&tp.tv_sec);
   }


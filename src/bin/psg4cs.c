/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* psg4cs.c - check the pulse sequence for CompSens.h include file and modify, if requested.
   Agilent Technologies. Eriks Kupce, Jan 2011 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "vnmrio.h"

int main (argc, argv)
int argc;
char *argv[];
{
  FILE        *src, *trg;
  char        *s, opt, fnm[MAXSTR], fnm2[MAXSTR], str[MAXSTR], s1[MAXSTR], s2[MAXSTR];
  int         i, j, k, ok, k1, k2, k3, ipos;
  void        find_nis();

  opt = 'm'; ok = 0;

  printf("\n psg4cs : \n");
  
  if((argc < 2) || (argc > 3))
    vn_error("\n Usage :  psg4cs fname.c <-u>");

  if(argc > 2)
  {
    if((argv[2][0] == '-') && ((argv[2][1] == 'm') || (argv[2][1] == 'u'))) 
      opt = argv[2][1];
    else
      vn_error("\n Usage :  psg4cs fname.c <-u>");
  }

  sprintf(fnm, "%s", argv[1]); /* check for extension */   
  i = strlen(fnm);
  if(fnm[i-2] != '.')
  {
    printf(" psg4cs : adding .c extension...\n"); 
    strcat(fnm, ".c");
  }

  if((src = open_file(fnm, "r")) == NULL)
    exit(0);
    
  i=0; ipos=0;
  while(fgets(str, MAXSTR, src)) 
  { 
    j = sscanf(str, "%s %s", s1, s2); 
    if(j>0) 
    {
      if((s1[0] == 'p') && (strcmp(s1, "pulsesequence()") == 0))
        break;
      else if(s1[0] == '#') 
      {
        if(strcmp(s1, "#endif") == 0) ipos = i;
        else if(j>1) 
        {    
          if(strcmp(s1, "#include") == 0)
          {
            if(strcmp(s2, "<standard.h>") == 0)      ipos = i;
            else if(strcmp(s2, "<Pbox_psg.h>") == 0) ipos = i;
            else if(strcmp(s2, "<CompSens.h>") == 0) ok = 1;
            else if(s2[0] == '"')
            {
              s = strchr(s2, '"');
              s++; k=strlen(s); s[k-1]='\0';
              if(strcmp(s, "Pbox_psg.h") == 0) ipos = i;
              if(strcmp(s,"CompSens.h") == 0) ok = 1;
            }
          }      
        }
      }
    }
    i++;
  }

  if((opt == 'm') && (ok==0))
  {
    k1=0; k2=0; k3=0;
    strcpy(fnm2, fnm);   
    strcat(fnm2, ".tmpcs");
    if((trg = open_file(fnm2, "w")) == NULL)
      vn_abort(src, "psg4cs: unable to save the modified file. "); 
    (void) find_nis(src, &k1, &k2, &k3); 

    i=0; ok=0; fseek(src,0,0);
    while(fgets(str, MAXSTR, src)) 
    {
      if(i == ipos) 
      {
        fputs(str, trg);
        fprintf(trg, "#include <CompSens.h>   /* includes the Compressive Sensing module */ \n");
      }
      else if((i > ipos) && (ok < 1))
      {
        j = sscanf(str, "%s %s", s1, s2); 
        if(j>0) 
        {
          if((s1[0] == 's') || (s1[0] == 'g'))
          {
            if(s1[6] == '(') s1[6] = '\0';
            if((strcmp(s1, "getstr") == 0) || (strcmp(s1, "status") == 0))
            {
              fprintf(trg, "\n           (void) set_RS(0);   /* set up random sampling */ \n");
              if(k1 || k2 || k3)
              {
                fprintf(trg, "           if(SPARSE[A] == 'y') \n");
                fprintf(trg, "           { \n"); if(k1)
                fprintf(trg, "             if(nimax > ni) ni = nimax; \n"); if(k2)
                fprintf(trg, "             if(ni2max > ni2) ni2 = ni2max; \n"); if(k3)
                fprintf(trg, "             if(ni3max > ni3) ni3 = ni3max; \n"); 
                fprintf(trg, "           } \n");               
              }
              fprintf(trg, " \n");
              ok = 1;
            }
          }
        }
        fputs(str, trg);
      }
      else fputs(str, trg);
      i++;
    }
    fclose(trg); fclose(src);
    sprintf(str, "mv %s %s\n", fnm2, fnm); 
    system(str);
  }
  else if((opt == 'u') && (ok == 1))  /* remove the modifications */
  {
    strcpy(fnm2, fnm);   
    strcat(fnm2, ".tmpcs");
    if((trg = open_file(fnm2, "w")) == NULL)
      vn_abort(src, "psg4cs: unable to save the modified file. ");
    fseek(src,0,0);

    ipos=0;
    while(fgets(str, MAXSTR, src)) 
    { 
      j = sscanf(str, "%s %s", s1, s2); 
      i=0;
      if(j>1)
      {
        if((ipos<1) && (s1[0] == '#') && (strcmp(s1, "#include") == 0))
        {    

          if(strcmp(s2, "<CompSens.h>") == 0) i=1, ipos=1;
          else if(s2[0] == '"')
          {
            s = strchr(s2, '"');
            s++; k=strlen(s); s[k-1]='\0';
            if(strcmp(s,"CompSens.h") == 0) i=1, ipos=1;
          }
        }
        else if((ipos==1) && (strcmp(s1,"(void)")==0) && (s2[0]=='s') && (s2[6]=='('))
        { 
          s2[6] = '\0'; 
          if(strcmp(s2, "set_RS") == 0) 
          {
            i=1; ipos=2;       
            if((fgets(str, MAXSTR, src)) && (strchr(str, 'S')))   /* if SPARSE */
            {                                       
              if((fgets(str, MAXSTR, src)) && (strchr(str, '{'))) 
              {
                while((!strchr(str, '}')) && (fgets(str, MAXSTR, src))) j++;
              }
            }
          }
        }
      }
      if(i==0) fputs(str, trg);
    }
    fclose(trg); fclose(src);
    sprintf(str, "mv %s %s\n", fnm2, fnm); 
    system(str);
  }
  else fclose(src);

  printf("\n psg4cs for %s done. \n", fnm);

  return 1;
}

void  find_nis(src, ni, ni2, ni3)
FILE   *src;
int    *ni, *ni2, *ni3;
{
  int   i, j, k1, k2, k3;
  char  str[MAXSTR], s1[MAXSTR], *s, ch;

  fseek(src, 0,0); 
  while(fgets(str, MAXSTR, src)) 
  { 
    j = sscanf(str, "%s", s1); 
    if((j>0) && (s1[0] == 'p') && (strcmp(s1, "pulsesequence()") == 0))
      break;
  }
  k1=0; k2=0; k3=0; i=0; j=0;

  while(fgets(str, MAXSTR, src)) 
  { 
    s = strchr(str, '/');
    if((s != NULL) && ((s[1] == '*') || (s[1] == '/'))) s[0] = '\0';   /* exclude the comments */
    s = strchr(str, 'n'); 
    if(s!=NULL) 
    {
      if(s[1] == 'i') 
      { 
        if((k2==0) && (s[2] == '2')) ch = s[3];
        else if((k3==0) && (s[2] == '3')) ch = s[3];
        else if(k1==0) ch = s[2];
        else ch = '#';

        i = isalpha(ch);        
        if((i == 0) && (ch != '_'))
        {
          if((k2 == 0) && (s[2] == '2')) k2++; 
          else if((k3 == 0) && (s[2] == '3')) k3++; 
          else if(k1 == 0) k1++; 
        }
      }
    }
  }

  *ni = k1; *ni2 = k2; *ni3 = k3;

  return;
}


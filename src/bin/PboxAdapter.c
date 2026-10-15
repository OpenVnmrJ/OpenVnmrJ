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
/* Adapter between Pbox and SpinCAD */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MIN_ATTEN -16
#define MAX_ATTEN 63
#define EOS       '\0'     /* End of string */

int main(int argc, char *argv[])
{
char sdir[256], shape[64], outfile[128], spw[64], sbw[64]; 
char sflip[64], addopt[512];
double off, calpwr, calpw;

 int CalcPwFlag=0, CalcBwFlag=0, DefFlipFlag=0;
 double pw, pwr, fpwr, spwbw1, spwbw2;
 char ch, fn[512], fntemp[512], str[512];
 FILE *inpf, *pboxinp;
 int i, j, done, sysret;

 if (argc < 11)
 {
   printf("Usage: PboxAdapter shape outfile pw bw flip off calpwr calpw dir addopt [dec]\n");
   exit(1);
 }

 (void) snprintf(shape, sizeof(shape), "%s", argv[1]);
 (void) snprintf(outfile, sizeof(outfile), "%s", argv[2]);
 (void) snprintf(spw, sizeof(spw), "%s", argv[3]);
 (void) snprintf(sbw, sizeof(sbw), "%s", argv[4]);
 (void) snprintf(sflip, sizeof(sflip), "%s", argv[5]);
 (void) snprintf(sdir, sizeof(sdir), "%s", argv[9]);
 (void) snprintf(addopt, sizeof(addopt), "%s", argv[10]);


 off=atof(argv[6]);
 calpwr=atof(argv[7]);
 calpw=atof(argv[8]);
 calpw=calpw*1.0e6;


 if (!strcmp(spw, "Calculate") || !strcmp(spw,"calculate") ) CalcPwFlag=1;
 if (!strcmp(sbw, "Calculate") || !strcmp(sbw,"calculate") ) CalcBwFlag=1;
 if (CalcPwFlag && CalcBwFlag)
     {printf("PboxError pw and/or bw not set correctly"); exit(1);}
 if ( !CalcPwFlag) {spwbw1 = atof(spw);}
 if ( !CalcBwFlag) {spwbw2 = atof(sbw);}


/*
 if ( !strcmp(sflip, "Default") || !strcmp(sflip,"default") ) { DefFlipFlag=1;}
 else { sscanf(sflip,"%lf",&flip); 
        if (flip < 0.0)
            {printf("PboxError flip angle should be set to default"); exit(1);}
      }
*/

  DefFlipFlag=1;              /* force default flip for now */


/* set the name for output shape file */
if ( !strcmp(outfile,"none") || !strcmp(outfile,"None")  \
       || !strcmp(outfile,"auto") || !strcmp(outfile,"Auto") ) {
     { int _tn = snprintf(fn, sizeof(fn), "sc_%s",shape); (void) _tn; } strcpy(fntemp,fn);
     if (! CalcPwFlag)  { int _tn = snprintf(fn, sizeof(fn), "%s_%7.5f",fntemp,spwbw1); (void) _tn; strcpy(fntemp,fn); }
     if (! CalcBwFlag)  { int _tn = snprintf(fn, sizeof(fn), "%s_%d",fntemp,(int)spwbw2); (void) _tn; strcpy(fntemp,fn); }
     if ( fabs(off) > 0.0 ) { int _tn = snprintf(fn, sizeof(fn), "%s_%d",fntemp,(int)off); (void) _tn; strcpy(fntemp,fn); }
     if (! DefFlipFlag) { int _tn = snprintf(fn, sizeof(fn), "%s_%s",fntemp,sflip); (void) _tn; }
     }
else {
      (void) snprintf(fn, sizeof(fn), "%s", outfile);
     }
    { int _tn = snprintf(str, sizeof(str), "rm -f %s%s", sdir,fn); (void) _tn; }
    if (system(str) != 0) {
      printf("PboxError unable to remove old shape file"); exit(1);
    }

 
if ( ( !CalcPwFlag ) && (spwbw1 <= 0.0) ) {
    printf("Pbox hard 0.0 0.0 0.0"); exit(0);
}
   
  
snprintf(fntemp, sizeof(fntemp), "%sPbox.inp", sdir);
if ((pboxinp = fopen(fntemp,"w")) != NULL) {
   fprintf(pboxinp, "# Pbox input file created by autoshapepulse\n");

   if ( !CalcPwFlag && !CalcBwFlag ) {
      fprintf(pboxinp,"\n{ %s %10.1f/%9.8f %15.2f }", shape,spwbw2,spwbw1,off); }
 
   else if ( !CalcPwFlag ) {
      fprintf(pboxinp,"\n{ %s %14.8f %15.2f }", shape,spwbw1,off); }

   else if ( !CalcBwFlag ) {
      fprintf(pboxinp,"\n{ %s %10.1f %15.2f }", shape,spwbw2,off);}

   else {printf("PboxError pw and/or bw not set correctly"); exit(1);}

   fprintf(pboxinp, "\nname = %s", fn);
   fprintf(pboxinp, "\nref_pwr = %4.1f    ref_pw90 = %5.2f", calpwr, calpw);

   if (strcmp(addopt,"None") && strcmp(addopt,"none")) {

      i=0; j=0; done=0; fntemp[0]='\0'; 
      while ( ! done ) {
        ch=addopt[i];
        switch (ch) {
          case ';' :
                    fntemp[j]='\0';
                    fprintf(pboxinp, "\n%s", fntemp);
                    j=0; fntemp[0]='\0';
                    break;
          case '~' :
                    fntemp[j]=' '; j++;
                    break;
          case EOS :
                    fntemp[j]='\0';
                    fprintf(pboxinp, "\n%s", fntemp);
                    j=0; fntemp[0]='\0'; done=1;
                    break;
          default  :
                    fntemp[j]=ch; j++;
                    break;
        }
        i++;
      }
   }

   fclose(pboxinp);
}
else {printf("PboxError in writing Pbox.inp file"); exit(1); }



 snprintf(fntemp, sizeof(fntemp), "Pbox > %sPbox.tmpout", sdir);
 sysret=system(fntemp);
 if (sysret != 0) {printf("PboxError in execution! check input values"); exit(1); }

				 
if (argc > 11) 			  /* check to see if it is a decoupling shape */
   {
 { int _tn = snprintf(fntemp, sizeof(fntemp), "%s%s.DEC",sdir,fn); (void) _tn; }
 if ((inpf = fopen(fntemp,"r")) == NULL) 
      {printf("PboxError unable to read shape output file= %s", fntemp);exit(1);}
 else 
      { 
        i = fscanf(inpf,"%c %511s %lf %lf %lf\n", &ch, str, &pw, &pwr, &fpwr);
        fclose(inpf);
        if (! ((i == 5) && (ch == '#') && (!strcmp(str,"Pbox"))) )
          {printf("PboxError invalid values in shape output file=%s", fntemp); exit(1);}
        if ((pwr > MAX_ATTEN) || (pwr < MIN_ATTEN)) {
            printf("PboxError calculated power out of range! possibly incorrect reference values");
            exit(1);
        }
        printf("Pbox %s %14.2f %f %f", fn, pw, pwr, fpwr); exit(0);
      }
   }
else
   {
 { int _tn = snprintf(fntemp, sizeof(fntemp), "%s%s.RF",sdir,fn); (void) _tn; }
 if ((inpf = fopen(fntemp,"r")) == NULL) 
      {printf("PboxError unable to read shape output file= %s", fntemp);exit(1);}
 else 
      { 
        i = fscanf(inpf,"%c %511s %lf %lf %lf\n", &ch, str, &pw, &pwr, &fpwr);
        fclose(inpf);
        if (! ((i == 5) && (ch == '#') && (!strcmp(str,"Pbox"))) )
          {printf("PboxError invalid values in shape output file=%s", fntemp); exit(1);}
        if ((pwr > MAX_ATTEN) || (pwr < MIN_ATTEN)) {
            printf("PboxError calculated power out of range! possibly incorrect reference values");
            exit(1);
        }
        printf("Pbox %s %14.2f %f %f", fn, pw, pwr, fpwr); exit(0);
      }
   }

   return(EXIT_SUCCESS);
}
/* End of file */

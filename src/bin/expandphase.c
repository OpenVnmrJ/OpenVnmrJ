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
/*

   char *ExpandPhase(InputString,OutputString)
   char *InputString, *OutputString;
     returns OutputString
         or  NULL if error


   Expand Phase String
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define NUMBER     '0'		/* indicates the Token is a number */
#define OPENPARAN  '1'
#define CLOSEPARAN '2'
#define TILDA      '3'
#define INVALID    '9'		/* indicates invalid characters in phase string */
#define EOS        '\0'		/* indicates end of string */
#define MAXDIGITS  16		/* 15=max no. of significant digits in a phase value; value should be 1 more */
#define MAXFSTACKSIZE 4097	/* 4096=maximum depth of phase stack; value should be 1 more */
#define MAXPSTACKSIZE 33	/* 32=maximum depth of nested paranthesis; value should be 1 more */


static int StackOpenParan[MAXPSTACKSIZE-1], StackCloseParan[MAXPSTACKSIZE-1];
static char *StackOut[MAXFSTACKSIZE-1];
static int PointOutStack = 0;
static int PointOpenParan = 0;
static int PointCloseParan = 0;
static int isp = 0;
static int error =0;
static char errMsge[128];

int getToken(char *token, char *inputstring, int maxdigits);
int pushf(char *number);
int pushOpen(int number);
int pushClose(int number);
int popOpen();
int popClose();
int ReplicateSingle(int stackpointer, int times);
int ReplicateRange(int OpenIndex, int CloseIndex, int times);


int ExpandPhase(InputString,OutputString)
char *InputString, *OutputString;
{
  int TokenType, LastTokenType, PrevLastTokenType;
  char Token[MAXDIGITS];

  int i, TildaFlag=0;

  PrevLastTokenType=LastTokenType=INVALID;

  while((TokenType=getToken(Token, InputString, MAXDIGITS)) != EOS)

    
      switch(TokenType)
        {
          case NUMBER:

                  if (LastTokenType == TILDA)
                    {
                      TildaFlag=0;
                      if (PrevLastTokenType == CLOSEPARAN)
                       {             /* replicate the range on numbers on OutStack*/
                         ReplicateRange(popOpen(), popClose(), atoi(Token));
                       }
                      else if (PrevLastTokenType == NUMBER)
                       {            /* replicate only the top item on OutStack */
                         ReplicateSingle(PointOutStack-1, atoi(Token));
                       }
                      else 
                       {
                         error= -1;
                         strcpy(errMsge,"^ not preceeded by a number nor a )");
                         return(error);
                         break;
                       }
                    }
                  else
                    {
                       pushf(Token);
                    }
 
                  
                  PrevLastTokenType=LastTokenType;
                  LastTokenType=NUMBER;
                  if (error) { return(error);}
                  break;
   
          case OPENPARAN:

                  if (TildaFlag)
                  {
                     strcpy(errMsge,"^ followed by a (");
                     error= -2;
                  }
                  pushOpen(PointOutStack);

                  PrevLastTokenType=LastTokenType;
                  LastTokenType=OPENPARAN;
                  if (error) { return(error);}
                  break;


          case CLOSEPARAN:
 
                  if (TildaFlag)
                  {
                     strcpy(errMsge,"^ followed by a )");
                     error= -3;
                  }
                  pushClose((PointOutStack-1));

                  PrevLastTokenType=LastTokenType;
                  LastTokenType=CLOSEPARAN;
                  if (error) { return(error);}
                  break;


          case TILDA:
            
                  if (TildaFlag)
                  {
                     strcpy(errMsge,"^ followed by a ^");
                     error= -4;
                  }
                  PrevLastTokenType=LastTokenType;
                  LastTokenType=TILDA;
                  TildaFlag=1;
                  if (error) { return(error);}
                  break;
         


          case INVALID:

          default:
                  error= -5;
                  sprintf(errMsge,"invalid element %s",Token);
                  return(error);
                  break;

        }

        if (TildaFlag)
        {
           strcpy(errMsge,"extra ^");
           error= -6;
        }

        if (PointOpenParan != PointCloseParan)
          {
            error= -7;
            strcpy(errMsge,"mismatched parentheses");
            return (error);
          }

        if (PointOutStack <= 0) {
           strcpy(OutputString, "");
           return(0); 
          }


         if ( !error) {
           strcpy(OutputString, "");
           for(i=0; i<PointOutStack; i++)
             {
               if (strlen(OutputString) == 0)
                  strcpy(OutputString,StackOut[i]);
               else
               {
                  strcat(OutputString," ");
                  strcat(OutputString,StackOut[i]);
               }
             }
           return(PointOutStack); }
         else { return(error); }
}


/* Function: getToken       */

int getToken(char *token, char *inputstring, int maxdigits)
{
  int i, c;

  while ((c=inputstring[isp]) == ' ' || c == '\t' || c == '\n' || c == ',')
     isp++;

  if (c == '(') { isp++; return (OPENPARAN); }
 
  if (c == ')') { isp++; return (CLOSEPARAN);}

  if (c == '^') { isp++; return (TILDA);}

  if (c == '\0') { isp++; return (EOS);}

  if ((c != '.') && ((c < '0') || (c > '9')))
  { isp++;
fprintf(stderr,"invalid character '%c'\n",c);
    return (INVALID);
  }

  token[0]=c;
  isp++;
  for (i=1; (c=inputstring[isp]) >= '0' && c <= '9'; isp++, i++)
      if (i < maxdigits) token[i]=c;

  if (c == '.') 
    { 
       if (i < maxdigits) token[i]=c; 
       for (i++,isp++; (((c=inputstring[isp]) >= '0') && c <= '9'); isp++,i++)
         if  (i < maxdigits) token[i]=c;
    }
       
     
  if (i < maxdigits) 
    {
      token[i]='\0';
      return (NUMBER);
    }
  else
   {
     token[i]='\0';
fprintf(stderr,"too many digits (%d) in token %s\n",i,token);
     return (INVALID);
   }

}   
  


/* Function: pushf */

int pushf(char *number)
{
   char *ptr;
   if (PointOutStack >= MAXFSTACKSIZE)
      {
        strcpy(errMsge,"Error in Phase Cycle Expansion");
        error= -8;
        return(0);
      }
  
   ptr = (char *) malloc(strlen(number)+1);
   strcpy(ptr,number);
   StackOut[PointOutStack]=ptr;
   PointOutStack++;
   return(1);
}



/* Function: pushOpen  */

int pushOpen(int number)
{
  if (PointOpenParan >= MAXPSTACKSIZE)
    {
      strcpy(errMsge,"Error in Phase Cycle Expansion");
      error= -9;
      return(0);
    }

  StackOpenParan[PointOpenParan]=number;
  PointOpenParan++;
  return(1);
}



/* Function: popOpen   */

int popOpen()
{
  int number;


  if (PointOpenParan <= 0)
    {
      strcpy(errMsge,"Error in Phase Cycle Expansion");
      error=1 -10;
      return(-1);
    }

  number=StackOpenParan[PointOpenParan-1];
  PointOpenParan--;

  return(number);

} 




/* Function: pushClose  */

int pushClose(int number)
{
  if (PointCloseParan >= MAXPSTACKSIZE)
    { 
      strcpy(errMsge,"Error in Phase Cycle Expansion");
      error= -11; 
      return(0);
    }

  StackCloseParan[PointCloseParan]=number;
  PointCloseParan++;
  return(1);
}



/* Function: popClose   */

int popClose()
{
  int number;


  if (PointCloseParan <= 0)
    {
      strcpy(errMsge,"Error in Phase Cycle Expansion");
      error= -12;
      return(-1);
    }

  number=StackCloseParan[PointCloseParan-1];
  PointCloseParan--;

  return(number);

} 


/* Function: ReplicateSingle  */
/*  stackpointer;     points to the stack position to be replicated
 *                    is 1 less than the current PointOutStack
 *  times;            No of times -replication must be >= 1 
 */
int ReplicateSingle(int stackpointer, int times)
{
  int  i;
  char *stackitem;

  if ( (!(times >= 1)) || (times >= MAXFSTACKSIZE) )
     {
       strcpy(errMsge,"Phase repeat index is < 1  or  too large");
       error= -13;
       return(0);
     }

  if (stackpointer >= MAXFSTACKSIZE)
      {
         strcpy(errMsge,"Phase expression is too long");
         error= -14;
         return(0);
      }
  
  if ((MAXFSTACKSIZE-stackpointer) <= times)
     {
         strcpy(errMsge,"Phase expression is too long");
         error= -15;
        return(0);
     }


  stackitem=StackOut[stackpointer];
  for(i=1; i <= (times-1); i++)
      { 
         char *ptr;
         ptr = malloc(strlen(stackitem)+1);
         strcpy(ptr,stackitem);
         StackOut[stackpointer+i]=ptr;
      }
  PointOutStack=PointOutStack+(times-1);
  if (PointOutStack >= MAXFSTACKSIZE)
    {
         strcpy(errMsge,"Phase expression is too long");
         error= -16;
         return(0);
    }


  return(1);

}




/* Function: ReplicateRange  */
/* No of times -replication must be >= 1         */
int ReplicateRange(int OpenIndex, int CloseIndex, int times)
{
  int  i, j, sp, numitems, len;
  char *stackitem, *ptr;
   
  if (CloseIndex < OpenIndex)
    {
       strcpy(errMsge,"Error in Phase Cycle Expansion");
       error= -17;
       return(0);
    }

  if (CloseIndex == OpenIndex)   /* This is really for ReplicateSingle  */
    {

      if ( (i=ReplicateSingle(PointOutStack-1, times)) )
         { return(1); }
      else
         { error=1;return(0); }

    }

  if ( (!(times >= 1)) || (times*((CloseIndex-OpenIndex)+1) >= MAXFSTACKSIZE) )
     {
       strcpy(errMsge,"Phase repeat index is < 1  or  too large");
       error= -17;
       return(0);
     }

  if (PointOutStack >= MAXFSTACKSIZE)
      {
         strcpy(errMsge,"Phase expression is too long");
         error= -18;
         return(0);
      }
 
  if ((MAXFSTACKSIZE-PointOutStack) <= (times*((CloseIndex-OpenIndex)+1)) )
     {
        strcpy(errMsge,"Phase expression is too long");
        error= -19;
        return(0);
     }


  sp=PointOutStack;
  numitems=(CloseIndex-OpenIndex)+1;
  for (j=0; j < numitems; j++)
   {

        stackitem=StackOut[OpenIndex+j];
        len = strlen(stackitem) + 1;
        for(i=0; i <= (times-2); i++)
           {
             ptr = malloc(len);
             strcpy(ptr,stackitem);
             StackOut[sp+(i*numitems)+j]=ptr;
           }


   }

  PointOutStack=PointOutStack+(numitems*(times-1));
  if (PointOutStack >= MAXFSTACKSIZE)
    {
         strcpy(errMsge,"Phase expression is too long");
         error= -20;
         return(0);
    }


  return(1);

}



/*  END   */

int main(int argc, char *argv[])
{
   char res[8192];
   int num;
   if (argc != 2)
   {
      fprintf(stdout,"-1");
      fprintf(stdout,"");
   }
   else
   {
      num = ExpandPhase(argv[1],res);
      fprintf(stdout,"%d\n",num);
      if (num < 0)
      {
         fprintf(stdout,"%s\n",errMsge);
      }
      else
      {
         fprintf(stdout,"%s\n",res);
      }
   }
   exit(0);
}

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
/******************************************************************
   
   psfilter - input lpr filter to make Postscript Text from 
	      straight text and print a small gaudy header

	Author: Phil Hornung

handles:
	embedded control characters 
	binary input stream 
	form feeds

******************************************************************/
#include <stdio.h>
#include <ctype.h>

#define MAXLINE		(256)
#define MAXLINE1	(255)
#define MAXNUMCNTRL	(16)
#define TRUE		(1)
#define FALSE		(0)

char inbuffer[MAXLINE], psline1[MAXLINE];
char host[64],user[64];
int firstlineflag, chars_per_line, lines_per_page;
int non_print_count;

main(argc,argv)
int argc;
char *argv[];
{
   char h1,h2;
   int  tmp,tag;

   non_print_count = 0;
   chars_per_line = 80;
   lines_per_page = 66;
   tag = 0;
   strcpy(user,"");
   strcpy(host,"");
   while (--argc > 0)
   {
     argv++;
     sscanf(*argv,"-w%d",&chars_per_line);
     sscanf(*argv,"-l%d",&lines_per_page);
     if ((**argv != '-') && (tag == 0))
     {
       strcpy(user,*argv);
       tag = 1;
     }
     if ((**argv != '-') && (tag != 0))
     {
       strcpy(host,*argv);
     }
   }
   h1 = getchar();
   if (h1 == EOF) 
     exit(-1);
   h2 = getchar();
   if (h2 == EOF) 
     exit(-1);
   if ((h1 == '%') && (h2 == '!')) 
   /* is it a postscript file? */
   {  /* if so, re-emit it */
      putchar(h1);
      putchar(h2);
      while ((tmp = getchar()) != EOF)
        putchar(tmp);
      /* putchar(EOF); */
   }
   else
   {
      firstlineflag = TRUE;
      inbuffer[0] = h1;
      inbuffer[1] = h2;
      convert2ps();
   }
   exit(0);
}


/*****************************************************************/
/* get_buffer returns end of buffer pointer 			 */
/* end of buffer is a line feed or return or EOF or MAXLINE	 */
/* flag is set if non-alpha except tabs 			 */
/* linefeed or return is replaced by \0 			 */
/*****************************************************************/
get_buffer()
{
  char *px,cval;
  int count,done,flag;
  flag = 0;
  done = FALSE;
  if (firstlineflag == TRUE)
  {
    px = inbuffer + 2;
    count = 2;
    /* if the first char was a new line, ignore it */
    firstlineflag = FALSE;
    if (inbuffer[1] == '\n') 
    {
       inbuffer[1] = '\0';
       return(0);
    }
  }
  else
  {  
    px = inbuffer;
    count = 0;
  }
  do
  {
    *px = getchar();
    count++;
    if (*px == EOF)
    {
      done = TRUE;
      *px = '\0';
      flag |= 1;
    }
    if ((*px == '\n') || (*px == '\r'))
    {
      done = TRUE;
      *px = '\0';
    }
    if (*px == '\f')
    {
      /* typically ^L linefeed eat lf in this case */
      /* but don't count as a control */
      flag |= 8;
      *px=' ';
    }
    if (count == MAXLINE1)
    {
      done = TRUE;
      *(px+1) = '\0';
      flag |= 2;
    }
    if ((!done) && (!isprint(*px)) && (*px != '\t'))
    {
      flag |= 4;
      cval = *px;
      *px++ = '^';
      *px = cval+0x40;
      count++;
      non_print_count++;
    }
    px++;
  }
  while (!done);
  return(flag);
}

convert2ps()
{
   char *p1,*ps,*ob,*obm;
   int char_count,page_count,line_count, flag;
   line_count = 1;
   page_count = 1;
   send_preamble();
   do
   {
      flag = get_buffer();
      ps = inbuffer;
      p1 = ps;
      /******************************************/
      /* how long is the input 			*/
      /******************************************/
      do
      {
        if (((flag & 1) != 1) && (line_count == 1))
	  new_page(page_count);
	/********/
	if ((flag & 8) && (!(flag&1)))
	{
	  line_count = lines_per_page+10; 
	  /* line_count triggers new_page next loop */
	  break; /* discard line */
	}
	/********/
	/* copy input into new buffer expanding tabs */
	/* detect over chars_per_line split at white space  */
	/* print the line - initialize any remainder */ 
	ob = psline1;
	char_count = 0;
	while ((char_count < chars_per_line) && (*p1 != '\0'))
	{
	  if (*p1 == '\t')	/* expand the tab */
	  {
	    /* TAB HANDLER */
	    do
	    {
	      *ob++ = ' ';
	      char_count++;
            }
	    while (((char_count % 8) != 0) && (char_count < chars_per_line));
          }
	  else
	  {
	    if ((*p1 == '(') || (*p1 == ')') || (*p1 == '\\'))
	      *ob++ = '\\';  /* quote it but don't count quote*/
	    *ob++ = *p1;
	    char_count++;
	  }
	  p1++;
	}
	if (char_count == chars_per_line)
	{
	   /* back down string for break point */  
	   /* break at ' ' or back at chars_per_line */
	   obm = ob;
	   while ((*ob != ' ') && (char_count > chars_per_line-20))
	   {
	      ob--;
	      char_count--;
	   }
	   /* find a good place? */
	   if (char_count == chars_per_line - 20 )/* no good place */
	   { 		
	      *(obm+1) = '\0';   
	      /* p1 is ok */
	   }
	   else
	   {
	      /* a good place */
	      *(ob+1) = '\0';
	      while (*p1 != ' ')
		p1--;
              p1++; /* next char please */
	   }
	}
	else 
	  *ob='\0';

        printf("(%s) show nl\n",psline1);
        line_count++;
        if ( line_count > lines_per_page ) /* a new page */
        {
	  send_page(page_count);
	  line_count = 1;
	  page_count++;
        }
	if (non_print_count > MAXNUMCNTRL)
	{
	  /* exit with control char flags */
	  flag = 5;
	  break;
	}
     }
     while (*p1 != '\0');
   }
   while ((flag & 1) != 1);
   if (non_print_count > MAXNUMCNTRL)
   {
    printf("288 0 translate 90 rotate \n");
    printf("0 0 mv /Courier-Bold  findfont 24 scalefont setfont\n");
    printf("(Vnmrprint:psfilter Cannot print binary file ) show \n");
    printf("grestore\n");
    return;
   }
   if (line_count != 1)
     send_page(page_count);
   send_postamble(page_count);
}

send_preamble()
{
    char t1[40],t2[40];
    printf("%%!\n%%PS-\n");
    printf("%%%%Title:Printing with Vnmr\n");
    printf("%%%%Creator:Varian Vnmr\n");
    printf("%%%%Pages:(atend)\n");
    printf("%%%%EndComments\n");
    printf("%%%%EndProlog\n");
    printf("%%lines_per_page=%d\n",lines_per_page);
    printf("%%chars_per_line=%d\n",chars_per_line);
    printf("gsave\n");
    printf("/mv { moveto } def\n");
    printf("/ls { lineto stroke } def\n");
    printf("/nl { /ypos ypos 10 sub def 0 ypos mv } def\n");
    printf("/Courier findfont 10 scalefont setfont\n");
    /* Courier is mono-spaced - easiest to start */
    printf("/obox { tgx tgy moveto 0 15 rlineto bxl 0 rlineto 0 -15 rlineto closepath gsave\n");
    printf("0.95 setgray fill grestore 0 setgray tgx tgy moveto 4 4 rmoveto stroke } def\n");
    printf("/ostr {tgx tgy moveto 4 4 rmoveto show } def \n");
    /*************************************************************/
    /*************************************************************/
    printf("/tgx 0 def /tgy 666 def /bxl 0 def \n");
    printf("/htag { (Host: %s) dup stringwidth pop 8 add /bxl exch def /tgx 0 def obox \n",host);
    printf("ostr } def\n");
    printf("/utag { (User: %s) dup stringwidth pop 8 add /bxl exch def 480 bxl sub /tgx exch def obox ostr } def\n",user);
    strcpy(t1," ");
    strcpy(t2," ");
    if (strlen(user) > 1)
      strcpy(t2,"utag");
    if (strlen(host) > 1)
      strcpy(t1,"htag");
    printf("/newpage {72 72 translate %s %s /ypos 648 def 0 ypos mv} def\n",t1,t2);
    printf("/bottompage { 216 -36 mv (page ) show show showpage } def\n");
}

new_page(pn)
int pn;
{
    printf("%%%%Page:%d  %d\nnewpage\n",pn,pn);
}   

send_page(pn)
int pn;
{
    printf("(%d) bottompage\n",pn);
}

send_postamble(pgs)
int pgs;
{
    printf("grestore\n");
    printf("%%%%Trailer\n");
    printf("%%%%Pages:%d\n",pgs);
}

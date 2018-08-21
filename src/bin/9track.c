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
/************************************************/
/*						*/
/* tape	- XL/VXR style tape handler		*/
/*						*/
/* tape cat		tape directory		*/
/* tape read name(s)	read file(s)		*/
/* tape rewind		rewind tape		*/
/* tape quit		unmount tape		*/
/*						*/
/************************************************/

/* In 'tape read name(s)' one or several tape files can be read into  */
/* the current directory. Wild cards can be used in the file name, if */
/* it is put in quotes */

#include <stdio.h>
#include <strings.h>
#include <sys/file.h>

#define BLOCKSIZE 10240

struct ntapeheader {
    char filename[16];
    short blocksize;
    short sectors;
    char label[16];
    short size;
    short type;
    char shortname[6];
    char user[6];
    short date[3];
    short time[1];
    short filenumber;
    char filler[BLOCKSIZE];
  };

static int organize_dir;

/***********/
ntape_input_error()
/***********/
{
    printf( "You have selected the 9-track tape interface\n\n" );
    printf( "Options include:\n" );
    printf( "    cat                    display list of files on the tape\n" );
    printf( "    read <file names>      extract named files from the tape\n" );
    printf( "    rewind                 rewind the tape\n" );
    printf( "    quit                   release the tape device\n\n" );
    printf( "You can supply a wildcard by using the '*' character.  Don't\n" );
    printf( "forget to used double quotes to prevent intervention by the\n" );
    printf( "UNIX shell.\n\n" );
    printf( "Files are always created in your current working directory.\n" );
    printf( "If a file already exists, it will NOT be overwritten\n\n" );
}

verify_label( lptr )
char *lptr;
{
	static char	vlabel[] = "VARIAN ";
	int		i;

	for (i = 0; i < 6; i++)
	 if ( *(lptr+i) != vlabel[ i ] ) return( 1 );

	return( 0 );
}

/********/
ntape_cat()
/********/
{
  struct ntapeheader head;
  int i,index,r;

/* first mount the tape */

  if (mounttape( O_RDONLY )) return 1;
  printf("\n         	TAPE CATALOGUE\n\n");
  rewind();
  index = 1;
  while (r=readtape(&head,BLOCKSIZE)) {
      if (r<512) return 1;
      if (verify_label( &head.label[ 0 ] )) {
	if (index == 1) printf( "tape:  not a VXR tape\n" );
	return( 1 );
      }
      printf("%3d   ",head.filenumber);
      i=6;
      while (i--)
        if ((head.shortname[i]>='A')&&(head.shortname[i]<='Z'))
          head.shortname[i] += 32;
      for (i=0; i<6; i++) printf("%c",head.shortname[i]);
      printf("  '");
      i=16;
      while (i--)
        if ((head.filename[i]>='A')&&(head.filename[i]<='Z'))
          head.filename[i] += 32;
      for (i=0; i<16; i++) printf("%c",head.filename[i]);
      printf("' %5d  ",head.sectors-1);
      for (i=0; i<6; i++) printf("%c",head.user[i]);
      printf("  %2d-%2d-%2d  ",head.date[0],head.date[1],head.date[2]);
      printf("\n");
      if (tapeskip(1,1)) return 1;
      index++;
  }
  rewind();
}

/*********/
tape_quit()
/*********/
{
  /* first mount the tape */
  if (mounttape( O_RDONLY )) return 1;
  /* dismount tape */
  dismount();
}

/***********/
tape_rewind()
/***********/
{
  /* first mount the tape */
  if (mounttape( O_RDONLY )) return 1;
  /* rewind the tape */
  rewind();
}


/**************************/
ntape_load(numfiles,namelist)
/**************************/
int numfiles; char *namelist[];
{
  struct ntapeheader head;
  char *buffer;
  int match,i,r,still_to_write,l;
  int fl;
  char fname[20],ext[10];

  buffer = (char *)(&head);
/* first mount the tape */
  if (mounttape( O_RDONLY )) return 1;
  rewind();

  while (r=readtape(buffer,BLOCKSIZE)) {
      if (r<512) return 1;
      l = 5;
      while ((head.shortname[l]==' ') && (l>0)) l--;
      for (i=0; i<=l; i++) {
	  fname[i] = head.shortname[i];
          if ((fname[i]>='A')&&(fname[i]<='Z'))
          fname[i] += 32;
      }
      fname[l+1] = '\0';
      match = 0;
      for (i=0; i<numfiles; i++)
        if (nscmp(namelist[i],fname)) match = 1;
      if (match) {
	  make_ext(ext, (head.type >> 8) & 0x7f, head.size);
	  strcat(fname,ext);
	  if (access(fname,0) == 0) {
	      printf("file %s exists, not overwritten\n",fname);
	      if(tapeskip(1,1)) return 1;
	      continue;
	  }
	  printf("reading file '%s'\n",fname);
          if ((fl=open(fname,O_WRONLY | O_TRUNC | O_CREAT,0660))<0) {
	      perror("tape, open output file");
              return 1;
          }
          still_to_write = (head.sectors-1) * 512;
          if (still_to_write>BLOCKSIZE - 512) {
	      if (write(fl,buffer+512,BLOCKSIZE-512)<BLOCKSIZE-512) {
		  perror("tape, first block write");
                  close(fl);
                  return 1;
              }
              still_to_write -= BLOCKSIZE - 512;
              while (still_to_write>0) {
		  if (r=readtape(buffer,BLOCKSIZE)) {
		      if (still_to_write>BLOCKSIZE) {
			if (write(fl,buffer,BLOCKSIZE) <BLOCKSIZE) {
			      perror("tape, additional block writes");
                              close(fl);
                              return 1;
                        }
                      }
                      else {
			    if (write(fl,buffer,still_to_write)
                                                       <still_to_write) {
			      perror("tape, last block write");
                              close(fl);
                              return 1;
                        }
                      }
                  }
                  else {
		      close(fl);
                      return 1;
                  }
                  still_to_write -= BLOCKSIZE;
            }
          }
          else {
            if (write(fl,buffer+512,still_to_write)<still_to_write) {
		perror("tape");
                close(fl);
                return 1;
	    }
          }
          close(fl);
          if (!organize_dir) return 0;
        }
      else {
	  printf("skipping file '%s'\n",fname);
      }
      if (tapeskip(1,1)) return 1;
  }

  rewind();
}

/*************/
ntape_main(argc,argv)
/*************/
int argc; char *argv[];
{ 
  if (argc<1) {
    ntape_input_error();
    return 1;
  }
  if (strcmp(argv[0],"cat")==0) {
    ntape_cat();
  }
  else if (strcmp(argv[0],"read")==0) {
    if (argc<2) {
      ntape_input_error();
      return 1;
    }
    organize_dir = 131071;
    if (argc == 2) 
     if ( (rindex( argv[ 1 ], '*' ) == NULL) &&
	  (rindex( argv[ 1 ], '%' ) == NULL) &&
	  (rindex( argv[ 1 ], '#' ) == NULL)
	) organize_dir = 0;
    ntape_load(argc-1,argv+1);
  }
  else if (strcmp(argv[0],"quit")==0) {
    tape_quit();
  }
  else if (strcmp(argv[0],"rewind")==0) {
    tape_rewind();
  }
  else {
    ntape_input_error();
    return 1;
  }
}

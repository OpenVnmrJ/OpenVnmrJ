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
#include <string.h>
#include <strings.h>

#define  BUFSIZE 1024
#define  TRUE 1
#define  FALSE 0


int getUser(char *line, char *username, char *password)
{
   int bUser = FALSE;
   char *strToken;

   /* if the name whose password should be changed, 
      is the same as the user in this line, then return true */
   if ((strToken = strtok(line, " \t")) != NULL)
   {
      if (strcmp(username, strToken) == 0)
	bUser = TRUE;
   }
   
   return bUser;
}

int main(int argc, char *argv[])
{
   char line[124];
   int bUser = FALSE;
   FILE *input;
   FILE *output;
   int nLength = 0;
   int index = 0;
   char *username;
   char *password;
   char data[256][124];
   char data2[124];
   int bWrite = FALSE;
   int i = 0;
  
   if (argc < 4)
   {
     /*fprintf(stderr, "Error: %s must be called with three args\n", argv[0]);*/
      return(0);
   }

   if ((input=fopen(argv[1], "r")) == NULL)
   {
      fprintf(stderr, "Error: Unable to open file %s\n", argv[1]);
      return 1;
   }

   username = argv[2];
   password = argv[3];

   index = 0;
   while (fgets(line, BUFSIZE,input) != NULL)
   {
      /* copy each line from the file to the data buffer */
      strcpy(data[index++], line);
      /* see if the name is in the current line */
      bUser = getUser(line, username, password);
      /* the current password is set for the user */
      if (bUser == TRUE)
      {
	 strcat(line, "\t");
         strcat(line, password);
         strcat(line, "\n");
         strcpy(data[index-1], line);
	 bWrite = TRUE;
      }
   }

   /* if the user is not in the file, then this is a new user,
      add it to the file */
   if (bWrite == FALSE)
   {
      char line[124];
      strcat(line, username);
      strcat(line, "\t");
      strcat(line, password);
      strcat(line, "\n");
      strcpy(data[index++], line);
   }
   fclose(input);

   if ((output=fopen(argv[1], "w")) == NULL)
   {
      fprintf(stdout, "Unable to open file %s\n", argv[1]);
      return 1;
   }

   for (i = 0; i < index; i++)
   {
     fprintf(output, "%s", data[i]);
   }
   fclose(output);

   return 0;
}

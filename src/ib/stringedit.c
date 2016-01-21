/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
static char *Sid(){
    return "@(#)stringedit.c 18.1 03/21/08 (c)1991 SISCO";
}

#include <stdio.h>
#include <string.h>
#include "stringedit.h"


TextEdit::TextEdit() {
  curpos = 0;
  text = new char[1];
  text[0] = 0;
}

TextEdit::~TextEdit() {
  delete text;
}

void TextEdit::debugprint(char* txt) {
  printf("curpos = %d, text = ", curpos);
  for (int i = 0 ; i < 32 ; i++) {
    if (curpos == i) printf(" || ");
    if ((int)txt[i]) {
      printf("%3d ", (int)txt[i]);
    } else {
      printf("<$$>");
    }
  }
  printf("\n");
}

int TextEdit::SetCurPos(int newpos) {
  int len = strlen(text);
  if (newpos < 0 || newpos > len) return -1;
  curpos = newpos;
  return curpos;
}

char* TextEdit::SetText(char* newtext) {
  int len = strlen(newtext);
  
  delete text;
  text = new char[len+1];
  strcpy(text, newtext);
  return(text);
}

int TextEdit::handle(char c) {
  int length = strlen(text);

  switch (c) {
    
  case DEL:
  case BS:
    
    if (curpos > 0) {
      DeleteChar();
    } else {
      return(kill_previous_newline);
    }
    return(no_op);


  case CONTROL_D:
    
    if (curpos < length && length > 0) {
      curpos++;
      DeleteChar();
    } else {
      return(kill_newline);
    }
    return(no_op);

  
  case BACKWARD_CHAR:
    
    if (curpos > 0) {
      curpos--;
    } else {
      curpos = 0;
      return(previous_line);
    }
    return(no_op);


  case FORWARD_CHAR:
    
    if (curpos < length) {
      curpos++;
    } else {
      curpos = length;
      return(next_line);
    }
    return(no_op);


  case NEXT_LINE:
  case PREVIOUS_LINE:
  case CARRIAGE_RETURN:
    return(next_line);

  case LINE_FEED:
    return(newline);

  case CONTROL_W:
  case CONTROL_U:
    
    SetText("");
    SetCurPos(0);
    return(no_op);


  case CONTROL_E:
    
    curpos = length;
    return(no_op);


  case CONTROL_A:
    
    curpos = 0;
    return(no_op);

  case CONTROL_C:
  case CONTROL_G:
  case CONTROL_I:
  case CONTROL_K:
  case CONTROL_L:
  case CONTROL_O:
  case CONTROL_Q:
  case CONTROL_R:
  case CONTROL_S:
  case CONTROL_T:
  case CONTROL_V:
  case CONTROL_X:
  case CONTROL_Y:
  case CONTROL_Z:
    return(no_op);

  
  default:
    InsertToString(c);
    return(no_op);

  }

}

char* TextEdit::DeleteChar() {
  if (curpos <= 0) return text;

  char* buf = new char[strlen(text)];

  if (curpos > 1) {
    // Copy all but the last character before the cursor into the array 'buf'
    strncpy(buf, text, curpos-1);
    
    if (text[curpos]) {
      sprintf(&buf[curpos-1], "%s", &text[curpos]);
    } else {
      buf[curpos-1] = 0;
    }
    
  } else {
    // Cursor is positioned immediately after first character
    if (text[curpos]) {
      
      // There is at least one character after the cursor postion
      sprintf(buf, "%s", &text[curpos]);
      
    } else {
      // There are no characters after the cursor position so
      // just erase the entire text string.
      buf[0] = 0;
    }
  }

  curpos--;
  delete text;
  text = buf;
  return(text);
}

char* TextEdit::InsertToString(char c) {

  char* buf = new char[strlen(text)+2];

  if (curpos > 0) {
    // There is at least one character in string and cursor is not at beginning
    // So copy all characters up to curpos into the 'buf' string
    strncpy(buf, text, curpos);

    // If there is any text following the cursor position, then
    // copy it onto the end of the 'buf' string
    if (text[curpos]) {
      sprintf(&buf[curpos], "%c%s", c, &text[curpos]);

    } else {
      sprintf(&buf[curpos], "%c", c);
    }
  } else {
    // The cursor is at the beginning of the string. Just concatenate the
    // new character with the text after the cursor (if any)
    if (text[curpos]) {
      sprintf(buf, "%c%s", c, &text[curpos]);

    } else {
      // This string is just empty so just stuff it with the new charcter
      buf[0] = c;  buf[1] = 0;
    }
  }

  delete text;
  curpos++;
  text = buf;
  return text;
}
     

     
char* TextEdit::GetPrintText(char cursor)
{
    char* buf = new char[strlen(text)+2];
    
    if (cursor == 0){
	strcpy(buf, text);
    }else{
	if (curpos > 0) {
	    // There is at least one character in string and
	    //  the cursor is not at beginning,
	    //  so copy all characters up to curpos into the 'buf' string.
	    strncpy(buf, text, curpos);
	    
	    // If there is any text following the cursor position, then
	    // copy it onto the end of the 'buf' string
	    if (text[curpos]) {
		sprintf(&buf[curpos], "%c%s", cursor, &text[curpos]);
		
	    } else {
		sprintf(&buf[curpos], "%c", cursor);
	    }
	} else {
	    // The cursor is at the beginning of the string.
	    // Just concatenate the
	    // new character with the text after the cursor (if any)
	    if (text[curpos]) {
		sprintf(buf, "%c%s", cursor, &text[curpos]);
		
	    } else {
		// This string is empty. Just stuff in the cursor character.
		buf[0] = cursor;  buf[1] = 0;
	    }
	}
    }
    
    return buf;
}

int testmain() {

  char c;
  TextEdit t;
  char* buf;

  while ((c = getchar()) != '.') {
    if (c == EOF) return(0);
    t.handle(c);
    printf("\n== %s ==\n", buf = t.GetPrintText());
    delete buf;
  }
  return(0);
}

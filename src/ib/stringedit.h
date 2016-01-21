/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef STRINGEDIT_H
#define STRINGEDIT_H


class TextEdit {


 public:
  TextEdit();
  ~TextEdit();
  int handle(char c);
  char* DeleteChar();
  char* InsertToString(char c);
  int ShowString(float xpos, float ypos);
  char* SetText(char*);
  char* GetText() {return text;}
  int SetCurPos(int);
  int GetCurPos() {return curpos;}
  char* GetPrintText(char cursor='|');
  void debugprint(char*);

  char *text;
  
 protected:
  int curpos;
};


#define CONTROL_SPACE    '\000'
#define CONTROL_A        '\001'
#define CONTROL_B        '\002'
#define CONTROL_C        '\003'
#define CONTROL_D        '\004'
#define CONTROL_E        '\005'
#define CONTROL_F        '\006'
#define CONTROL_G        '\007'
#define CONTROL_H        '\b'
#define CONTROL_I        '\011'
#define CONTROL_J        '\012'
#define CONTROL_K        '\013'
#define CONTROL_L        '\f'
#define CONTROL_M        '\015'
#define CONTROL_N        '\016'
#define CONTROL_O        '\017'
#define CONTROL_P        '\020'
#define CONTROL_Q        '\021'
#define CONTROL_R        '\022'
#define CONTROL_S        '\023'
#define CONTROL_T        '\024'
#define CONTROL_U        '\025'
#define CONTROL_V        '\026'
#define CONTROL_W        '\027'
#define CONTROL_X        '\030'
#define CONTROL_Y        '\031'
#define CONTROL_Z        '\032'
#define BS               '\010'
#define DEL              '\177'
#define FORWARD_CHAR     '\006'
#define BACKWARD_CHAR    '\002'
#define NEXT_LINE        '\016'
#define PREVIOUS_LINE    '\020'
#define CARRIAGE_RETURN  '\015'
#define LINE_FEED        '\012'
#define H_TAB            '\011'
#define ESCAPE           '\033'


typedef enum {
  no_op,
  modified_no_op,
  newline,
  kill_line,
  kill_newline,
  kill_previous_newline,
  split_line,
  kill_region,
  yank_kill_buffer,
  set_mark,
  undo,
  refresh,
  next_line,
  previous_line,
  next_page,
  previous_page,
  top_of_file,
  end_of_file
} editcode;

#endif

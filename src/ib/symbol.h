/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

/*

How to use the DDL library


The DDL parser has been designed as a stand-alone library package
to read and manipulate the headers of SISCO data files.  In order
to use the package, you must include the header file <ddllib.h>
in your code and and link the code with the library libddl.a.



     
The following is a list of all the functions that are available to
end user applications to read and manipulate data file headers.
Please refer to the file "ddl-parser.h" for a description of the
class hierarchies created by the library.


  ==========

  DDLSymbolTable(char* name, char *magic_string = "#!/usr/local/fdf/startup");
  DDLSymbolTable(ifstream& infile,
  		 char *magic_string = "#!/usr/local/fdf/startup");



The DDL functionality is implemented by the DDLSymbolTable class.
This class is responsible for opening, reading and parsing a data file
written in the standard SISCO format.  Upon successful parsing of the
data header file, this object will contain a dictionary of symbol
names and their associated values defined by the DDL header file.

The first form of the constructor accepts a string containing the
unopened data file to be parsed and a string which contains the
correct matching magic "number", This is the magic string designating
the SISCO data format.  To ignore the magic string and read files with
any magic string, the user should supply an empty string "".  The
second form of the object constructor accepts a reference to an
ifstream object which describes the file to be read.  Note that in the
first form of the constructor, an ifstream object will be created and
stored as a public member of the DDLSymbolTable object.

If the file is successfully opened and parsed, this procedure returns
a symbol table containing all the information that has been parsed
from the header file.  The symbol table also contains an element which
is an ifstream object that manipulates the same data file.  This
ifstream object can then be used to read any additional data following
the header (it is identical to the ifstream infile parameter in the
second calling instance shown above).



  ==========



The following is the public interface to the DDLSymbolTable object:     */

     
#include <stdlib.h>
#ifdef LINUX
#include "generic.h"
#include <iostream>
#include <strstream>
#include <fstream>
using namespace std;
#else
#include <generic.h>
#include <iostream.h>
#include <strstream.h>
#include <fstream.h>
#endif
#include <netdb.h>
#include "ddlnode2.h"

typedef union {
    uint32_t my_int;
    uint8_t  my_bytes[4];
} endian_tester;

class DDLSymbolTable : public DDLNode
{

 public:
  ifstream *ddlin;
  ofstream *st_debug;

  int GetValue(char *name, int& value);
  int GetValue(char *name, double& value);
  int GetValue(char *name, char*& value);

  int GetValue(char *name, int& value, int index);
  int GetValue(char *name, double& value, int index);
  int GetValue(char *name, char*& value, int index);
  ArrayData *GetArray(char *name);

  int SetValue(char *name, double value);
  int SetValue(char *name, double value, int index);
  int SetValue(char *name, char* value);
  int SetValue(char *name, char* value, int index);

  DDLNode *CreateArray(char *name);
  DDLNode *AppendElement(char *name, double value);
  DDLNode *AppendElement(char *name, char *value);
  DDLNode *AppendElement(char *name, ArrayData *value);

  int MallocReadData();
  int MallocData() { return MallocReadData(); };
  void SetData(char* _data, int _data_length);
  char *GetData() { return data; };
  int DataLength() { return data_length; };
  int HeaderLength() { return header_length; };
  int FileLength() { return file_length; };
  char *GetSrcid() { return srcid; };
  void SetSrcid(char* newid);

  int IsDefined(char *name);
  int NotDefined(char *name);
  void PrintSymbols(ostream& os = cout);
  void PrintSymbols(ostrstream& os);
  void PrintSymbolsAll(ostream& os = cout);
  void SaveSymbolsAndData(ostream& os = cout);
  void SaveSymbolsAndData(char* filename);

  DDLNode *Lookup(char*);
  int      LengthOf(char*);
  DDLNode *Install(char*, int);
  DDLNode *Install(char*, int, double);
  DDLNode *Install(char*, int, char*);

  DDLSymbolTable(char* name);
  DDLSymbolTable();

  virtual DDLNode* Clone(int dataflag=TRUE);

  ~DDLSymbolTable() {
    if (data) delete[] data;
  };
  
protected:
  char *data;
  int header_length;  // number of bytes in the header
  int data_length;    // total number of bytes in the infile
  int file_length;    // total number of bytes in the data file
  char *srcid;        // string which identifies the source of the data
                      // (usually just the name of the DDL file
};

extern class DDLSymbolTable *ParseDDLFile(char *infile);
extern DDLNode *NullDDL;

#define MAGIC_STRING "#!/usr/local/fdf/startup"

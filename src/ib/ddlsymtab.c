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
*/

#include <stdio.h>
#include <stdlib.h>
#ifdef LINUX
#include <iostream>
#include <fstream>
#include <strstream>
#else
#include <iostream.h>
#include <fstream.h>
#include <strstream.h>
#endif
#include <string.h>
#include <math.h>
/* #include <sysent.h> */
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <time.h>
#include "ddllib.h"
#include "ddl-tab.h"
#include "crc.h"

#define ByteSwap5(x) ByteSwap((unsigned char *) &x,sizeof(x))
void ByteSwap(unsigned char * b, int n);
int BigEndian = 1;

DefineListClass(DDLNode);

#ifdef OCDEBUG
#define debug cout
#else
#define debug name2(/,/)
#endif


extern DDLSymbolTable *symlist;
extern unsigned short updcrcr(unsigned short crc, char *data, int length);

DDLNode *NullDDL = new DDLNode("NULL", BUILTIN);

ArrayData *NullArray = new ArrayData;

int alignment_size = 8;


DDLSymbolTable::DDLSymbolTable(char* filename) : DDLNode(filename, VAR)
{
  ddlin = new ifstream;
  
  data = 0;
  srcid = 0;

  // Redirect all debugging information to /dev/null
  st_debug = new ofstream("/dev/null", ios::out);

  if (strcmp(filename, "-") == 0) {
    ddlin->open("/dev/tty", ios::in);
  } else {
    ddlin->open(filename, ios::in);
  }
#ifndef OCDEBUG
  if ( ddlin->bad() ){
    cerr << "Can't open " << filename << endl;
  } else {
    SetSrcid(filename);
  }
#else 
    SetSrcid(filename);
#endif
}

DDLSymbolTable::DDLSymbolTable() : DDLNode("", VAR)
{
  ddlin = NULL;
  data = 0;
  srcid = 0;
  SetSrcid("");
  // Redirect all debugging information to /dev/null
  // If we open this, it never gets closed--so forget it.
  //st_debug = new ofstream("/dev/null", ios::out);
}

DDLNode* DDLSymbolTable::Clone(int dataflag)
{
    DDLSymbolTable* tmp = new DDLSymbolTable;
    
    char *pfx = "Clone(\"";
    char *sfx = "\")";
    char *newid = new char[strlen(pfx) + strlen(sfx) + strlen(srcid) + 1];
    strcpy(newid, pfx);
    strcat(newid, srcid);
    strcat(newid, sfx);
    tmp->SetSrcid(newid);
    delete [] newid;

    if (dataflag && data && data_length > 0) {
	tmp->SetData(data, data_length);
    }
    tmp->header_length = 0;
    return tmp;
}

void DDLSymbolTable::SetData(char* _data, int _data_length)
{
  if (_data_length <= 0) return;
  
  if (data) delete [] data;
  
  data = new char[_data_length];
  
  memcpy(data, _data, _data_length);
  data_length = _data_length;
}

void DDLSymbolTable::SetSrcid(char* newid)
{
  if (srcid) {
    free(srcid);
  }
  srcid = strdup(newid);
}

//
//  Returns TRUE if data successfully read, otherwise FALSE.
//
int DDLSymbolTable::MallocReadData()
{
  struct stat buf;
  ifstream ddl_in;

  file_length = data_length = header_length = 0;

  if (stat(srcid, &buf) == 0) { // successful
    ddl_in.open(srcid, ios::in);
    file_length = (int) buf.st_size;
    //debug << "length of " << srcid << " = " << file_length << "\n";

    char c;
    header_length = 0;
    while ((c = ddl_in.get()) != EOF) {
      if (c == 0) break;
      header_length++;
    }
    if (data) delete [] data;
    data = 0;
    
    if (c != EOF) {
      
      header_length++;
      data_length = file_length - header_length;
      //debug << "length of header " << srcid << " = " << header_length << "\n";
      //debug << "length of data " << srcid << " = " << data_length << "\n";


      if (data_length > 0) {
	data = new char[data_length];
	//debug << "MallocData() : malloc(" << data_length << ") = ";
	//debug << (int) data << "\n";
	
	if (data == 0) {
	  cerr << "Error: can't allocate memory for " << srcid << endl;
	  return FALSE;
	}

	ddl_in.read(data, data_length);

// do byteswap if necessary

            int bigendian_hdr = 1;
            GetValue("bigendian", bigendian_hdr);
                                                                                
            endian_tester et;
            et.my_int = 0x0a0b0c0d;
            BigEndian = (et.my_bytes[0] == 0x0a) ? 1:0;
                                                                                
            if(bigendian_hdr != BigEndian) {

	      char* storage ;
              GetValue("storage", storage ) ;

	      int bits=0;
              GetValue("bits", bits);

              int num = data_length;
              int cnt;

	      if( strcmp( storage, "float" ) == 0) { 
                num = data_length / sizeof(float);
                float *lptr = (float *) data;
                for (cnt = 0; cnt < num; cnt++)
                {
                  ByteSwap5(*lptr);
                  lptr++;
                }
	      }
	      if( ( strcmp( storage, "long" ) == 0 ) 
		|| (( strcmp( storage, "integer" ) == 0 ) && bits == 32) ){
                num = data_length / sizeof(long);
                long *lptr = (long *) data;
                for (cnt = 0; cnt < num; cnt++)
                {
                  ByteSwap5(*lptr);
                  lptr++;
		}
	      }
	      else if ( strcmp( storage, "short" ) == 0) {
                num = data_length / sizeof(short);
                short *lptr = (short *) data;
                for (cnt = 0; cnt < num; cnt++)
                {
                  ByteSwap5(*lptr);
                  lptr++;
		}
	      }

	    }

	if (ddl_in.gcount() == 0) {
	  cerr << "Error: can't read data from " << srcid << "\n";
	  return FALSE;
	}
      } else {
	cerr << "Warning: entire file is DDL header!" << "\n";
      }
    } else {	// (c == EOF)
      cerr << "Warning: no NULL at end of DDL header!" << "\n";
      return FALSE;
    }
  }
  return TRUE;
}



DDLNodeLink& DDLNodeLink::Print() {
  printf("object[%d]\n", item);
  return *this;
}

void CheckDelete(DDLNode *n)
{
  printf("deleting %x\n", n);
}

void DDLSymbolTable::SaveSymbolsAndData(ostream& os)
{
  ostrstream temp;   /* temporary stream to hold header as it's being build */
  
  /* First write out the magic string */
  temp << MAGIC_STRING << "\n";

  /* Generate the creator string */
  long clock = 0;      // date in seconds
  char *tdate;         // pointer to the time
  char *tlogin;        // pointer to login name
  char thost[MAXHOSTNAMELEN+2];  // hostname buffer   

  clock = time(NULL);
  SetValue("creation_time", clock);
  SetValue("bigendian", BigEndian);
  
  if ((tdate = ctime(&clock)) != NULL) tdate[strlen(tdate)-1] = 0;
  
  tlogin = (char *)cuserid(NULL);
  if (tlogin == NULL) tlogin = "unknown_user";
  SetValue("user", tlogin);

  /*gethostname(thost, MAXHOSTNAMELEN+1);*/
  struct utsname names;
  uname(&names);
  strncpy(thost, names.nodename, MAXHOSTNAMELEN);

  if (thost[0] == 0)  {
    sprintf(thost, "unknown_host");
  }
  SetValue("hostname", thost);
                                             
  temp << "/* Created by: " << tlogin << "@" << thost
    << " on " << tdate << " */" << "\n\n" ;
  
  /* Now recompute the checksum for the data */
  tcrc csum = addbfcrc(data, data_length);

  SetValue("checksum", csum);
  
  /* Now print all the symbols in the table */
  PrintSymbols(temp);

  /* Now capture the header from the temporary stream into a string pointer */
  temp << (char) 0;
  char *header = temp.str();

  /* Copy the header to the real output stream */
  os << header;
  
  /* Compute the length of the header */
  int temp_header_length = strlen(header);

  /* Compute the padding required to align the data */
  /* Recall that the header is following the two terminating characters */
  int padding = (temp_header_length + 3) % alignment_size;
  padding = (alignment_size - padding) % alignment_size;

  os << "\f\n" ;		/* Stops a "more" listing */
  for (int i = padding; i > 0; i--) {
    os << '\n';
  }
  /* Now add the header termination character */
  os << (char)0 ;
  
  /* Finally, write out the data */
  os.write(data, data_length);
}

void DDLSymbolTable::SaveSymbolsAndData(char* filename)
{
  ofstream fout;

  fout.open(filename);
  if (!fout) return;
  SaveSymbolsAndData(fout);
  fout.close();
}

void DDLSymbolTable::PrintSymbols(ostrstream& os)
{
  DDLNodeIterator st(this);
  DDLNode *sp;

  os << "/* Symbol Table */" << "\n";
  while (sp = ++st) {
    if (sp->symtype != UNDEFINED &&
	sp->symtype != BUILTIN &&
	sp->symtype != TYPEDEF &&
	sp->symtype != STRUCT &&
	sp->symtype != DDL_FLOAT &&
	sp->symtype != VOID &&
	sp->symtype != DDL_CHAR &&
	sp->symtype != DDL_INT)
    {
	DDLNode *sr = NULL;
	char *typestr = "void ";
	char *arraystr = "";
	DDLNode *sq = sp->val;
	if (sq){
	    if (sq->IsType(ARRAY_DATA)){
		arraystr = "[]";
		if (sr=sq->Top()){
		    if (sr->IsType(STRING_DATA)){
			typestr = "char *";
		    }else if (sr->IsType(REAL_DATA)){
			typestr = "float ";
		    }
		}
	    }else if (sq->IsType(STRING_DATA)){
		typestr = "char *";
	    }else if (sq->IsType(REAL_DATA)){
		typestr = "float ";
	    }
	}
	os << typestr << sp->GetName() << arraystr << " = ";
	*sp >> os << ";" << "\n";
    }
  }
  os << "\n";
}

void DDLSymbolTable::PrintSymbols(ostream& os)
{
  DDLNodeIterator st(this);
  DDLNode *sp;

  os << "\n";
  
  if (srcid) {
    os << '/' << '*' << "Symbol Table for " <<  srcid  << " */" << "\n";
  } else {
    os << "/* Symbol Table */" << "\n";
  }
  while (sp = ++st) {
    if (sp->symtype != UNDEFINED &&
	sp->symtype != BUILTIN &&
	sp->symtype != TYPEDEF &&
	sp->symtype != STRUCT &&
	sp->symtype != DDL_FLOAT &&
	sp->symtype != VOID &&
	sp->symtype != DDL_CHAR &&
	sp->symtype != DDL_INT)
    {
	DDLNode *sr = NULL;
	char *typestr = "void ";
	char *arraystr = "";
	DDLNode *sq = sp->val;
	if (sq){
	    if (sq->IsType(ARRAY_DATA)){
		arraystr = "[]";
		if (sr=sq->Top()){
		    if (sr->IsType(STRING_DATA)){
			typestr = "char *";
		    }else if (sr->IsType(REAL_DATA)){
			typestr = "float ";
		    }
		}
	    }else if (sq->IsType(STRING_DATA)){
		typestr = "char *";
	    }else if (sq->IsType(REAL_DATA)){
		typestr = "float ";
	    }
	}
	os << typestr << sp->GetName() << arraystr << " = ";
	*sp >> os << ";" << "\n";
    }
  }
  os << "\n";
}

void DDLSymbolTable::PrintSymbolsAll(ostream& os)
{
  DDLNodeIterator st(this);
  DDLNode *sp;

  os << "/* Symbol Table: */" << "\n";
  while (sp = ++st) {
    os << sp->GetName() << " = ";
    if (sp->symtype != UNDEFINED) {
      *sp >> os << "\n";
    } else {
      os << "UNDEFINED" << "\n";
    }
  }
  os << "\n";
}


/*ostream& DataValue::operator>>(ostream& os)
{
  return os << "DataValue!" << "\n";
}*/


DDLNode* DDLSymbolTable::Install(char *s, int t)
{
  DDLNode *sp = new DDLNode(s, t);

  sp->val = 0;
  Push(sp);
  return sp;
}

DDLNode* DDLSymbolTable::Install(char *s, int t, double d)
{
  DDLNode *sp = Install(s, t);
  sp->val = new RealData(d);
  return sp;
}

DDLNode* DDLSymbolTable::Install(char *s, int t, char* d)
{
  DDLNode *sp = Install(s, t);
  if (d == NULL) d = "";
  sp->val = new StringData(d);
  return sp;
}

DDLNode* DDLSymbolTable::Lookup(char *nme)
{
  DDLNodeIterator st(this);

  DDLNode* sp;

  while (sp = ++st) {
    if (strcmp(sp->GetName(), nme) == 0) break;
  }
  if (sp) {
    return sp;
  } else {
    return NullDDL;
  }
}


DDLNode* DDLSymbolTable::CreateArray(char *nme)
{
  DDLNode *arr = Lookup(nme);

  if (arr == NullDDL) {
    /* Array does not exist yet, so create a new one */
    arr = Install(nme, VAR);
    ArrayData *ad = new ArrayData();
    TypelessAsgn(arr, ad);
  } else {
    /* Array already exists, empty it */
      DDLNode *s = arr->GetArray();
      DDLNode *sq = arr->val;
      while (sq && sq->IsType(ARRAY_DATA) && s->LengthOf()){
	  sq->Pop();
      }
  }

  return arr;
}


DDLNode* DDLSymbolTable::AppendElement(char *nme, ArrayData *value)
{
  DDLNode *sp = Lookup(nme);

  if (sp == NullDDL) {
    /* No such array, first create one */
    sp = CreateArray(nme);
  }

  if (sp->val) {
    sp->val->Push(value);
  }
  return sp;
    
}

DDLNode* DDLSymbolTable::AppendElement(char *nme, double value)
{
  DDLNode *sp = Lookup(nme);

  if (sp == NullDDL) {
    /* No such array, first create one */
    sp = CreateArray(nme);
  }

  RealData *rd = new RealData(value);
  if (sp->val) {
    sp->val->Push(rd);
  }
  return sp;
    
}


DDLNode* DDLSymbolTable::AppendElement(char *nme, char *value)
{
  DDLNode *sp = Lookup(nme);

  if (sp == NullDDL) {
    /* No such array, first create one */
    sp = CreateArray(nme);
  }
         
  if (value == NULL) value = "";
  StringData *rd = new StringData(value);
  if (sp->val) {
    sp->val->Push(rd);
  }
  return sp;
    
}

int DDLSymbolTable::GetValue(char *nme, char*& value)
{
  DDLNode *valu = Lookup(nme);

  if (valu && (valu->val) && valu->val->IsType(STRING_DATA)) {
    if (valu != NullDDL) {
      value = (char*) (*valu);
      return TRUE;
    }
  }
  return FALSE;
}

int DDLSymbolTable::GetValue(char *nme, double& value)
{
  DDLNode *valu = Lookup(nme);

  if (valu && (valu->val) && valu->val->IsType(REAL_DATA)) {
    if (valu != NullDDL) {
      value = (double) (*valu);
      return TRUE;
    }
  }
  return FALSE;
}


int DDLSymbolTable::GetValue(char *nme, int& value)
{
  DDLNode *valu = Lookup(nme);

  if (valu && (valu->val) && valu->val->IsType(REAL_DATA)) {
    if (valu != NullDDL) {
      value = (int) (double) (*valu);
      return TRUE;
    }
  }
  return FALSE;
}
  

int DDLSymbolTable::SetValue(char *nme, double _value)
{
  DDLNode *valu = Lookup(nme);

  if (valu == NullDDL) {
    Install(nme, VAR, _value);
  } else if (valu && (valu->val) && valu->val->IsType(REAL_DATA)) {
    ((RealData*) valu->val)->value = _value;
  } else {
    return FALSE;
  }
  return TRUE;
}

int DDLSymbolTable::SetValue(char *nme, char* value)
{
  DDLNode *valu = Lookup(nme);

  if (valu == NullDDL) {
    Install(nme, VAR, value);
  } else if (valu && (valu->val) && valu->val->IsType(STRING_DATA)) {
      ((StringData*) valu->val)->SetString(value);
  } else {
    return FALSE;
  }
  return TRUE;
}

int DDLSymbolTable::GetValue(char *nme, int& value, int idx)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0) return FALSE;
  
  DDLNode *s = p->GetArray();
  if (s == 0) return FALSE;
  
  DDLNode *q = s->Index(idx);
  if (q == 0) return FALSE;
  
  value = (int)((RealData*) q)->value;
  return TRUE;
}

  
int DDLSymbolTable::GetValue(char *nme, double& value, int idx)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0) return FALSE;
  
  DDLNode *s = p->GetArray();
  if (s == 0) return FALSE;
  
  DDLNode *q = s->Index(idx);
  if (q == 0) return FALSE;
  
  value = (double) *(s->Index(idx));
  return TRUE;
}

int DDLSymbolTable::SetValue(char *nme, double _value, int idx)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0 || p == NullDDL) return FALSE;
  
  DDLNode *s = p->GetArray();
  if (s == 0 || p == NullDDL || !p->val) return FALSE;
  
  RealData *rd;
  while (s->LengthOf() < idx+1){
      rd = new RealData(0.0);
      p->val->Push(rd);
  }

  DDLNode *q = s->Index(idx);
  if (q == 0 || p == NullDDL) return FALSE;
  ((RealData*) q)->value = _value;
  return TRUE;
}

int DDLSymbolTable::SetValue(char *nme, char* value, int idx)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0 || p == NullDDL) return FALSE;
  
  DDLNode *s = p->GetArray();
  if (s == 0 || p == NullDDL || !p->val) return FALSE;

  StringData *rd;
  while (s->LengthOf() < idx+1){
      rd = new StringData("");
      p->val->Push(rd);
  }

  DDLNode *q = s->Index(idx);
  if (q == 0 || p == NullDDL) return FALSE;
  
  if ( ((StringData*) q)->val) {
    delete (((StringData*) q)->val);
  }
  if (value == NULL) value = "";
  ((StringData*) q)->SetString(value);
  return TRUE;
}

int DDLSymbolTable::LengthOf(char *nme)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0) return 0;
  
  DDLNode *s = p->GetArray();
  if (s) {
    return s->LengthOf();
  } else {
    return 0;
  }
}
  
int DDLSymbolTable::GetValue(char *nme, char*& value, int idx)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0) return FALSE;
  
  DDLNode *s = p->GetArray();
  if (s == 0) return FALSE;
  
  DDLNode *q = s->Index(idx);
  if (q == 0) return FALSE;
  
  value = (char*) *(s->Index(idx));
  return TRUE;
}

ArrayData* DDLSymbolTable::GetArray(char *nme)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0) return NullArray;
  
  DDLNode *s = p->GetArray();
  if (s == 0) {
    return NullArray;
  } else {
    return (ArrayData*) s;
  }
}

int DDLSymbolTable::IsDefined(char *nme)
{
  if (Lookup(nme) != NullDDL) {
    return TRUE;
  } else {
    return FALSE;
  }
}

int DDLSymbolTable::NotDefined(char *nme)
{
  if (IsDefined(nme)) {
    return FALSE;
  } else {
    return TRUE;
  }
}

void ByteSwap(unsigned char * b, int n)
{
   register int i = 0;
   register int j = n-1;
   register unsigned char c;
   while (i<j)
   {
#ifdef LINUX
      std::swap(b[i], b[j]);
#else
      c = b[i];
      b[i] = b[j];
      b[j] = c;
#endif
      i++, j--;
   }
}


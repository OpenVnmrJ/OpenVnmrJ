/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* #include <sysent.h> */
/*
#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#include <sstream>
#include <string.h>
#include <math.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "ddllib.h"
#include "ddl-tab.h"
*/
//#include "crc.h"

#include <string>
using namespace std;
using std::string;

#include <sys/stat.h>
#include <netdb.h>		// for MAXHOSTNAMELEN
#include <sys/utsname.h>
#include <algorithm> //required for std::swap
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "vnmrsys.h"
#ifdef LINUX
#ifndef  MAXHOSTNAMELEN
#define  MAXHOSTNAMELEN MAXPATH
#endif
#endif

#include "ddlNode.h"
#include "ddlGrammar.h"
#include "ddlSymbol.h"
#include "ddlParser.h"		// for TypelessAsgn()

#ifdef OCDEBUG
#define debug cout
#else 
#define debug name2(/,/)
#endif 

#define ByteSwap5(x) ByteSwap((unsigned char *) &x,sizeof(x))
void ByteSwap(unsigned char * b, int n);

extern char UserName[];
extern char HostName[];

extern DDLSymbolTable *symlist;
//extern unsigned short updcrcr(unsigned short crc, char *data, int length);

DDLNode *NullDDL = new DDLNode("NULL", BUILTIN);

ArrayData *NullArray = new ArrayData;

int alignment_size = 8;


DDLSymbolTable::DDLSymbolTable(const char* filename) : DDLNode(filename, VAR)
{

  copyNumber = 0;

  ddlin = new ifstream;
  
  data = 0;
  srcid = 0;

  // Redirect all debugging information to /dev/null
  //st_debug = new ofstream("/dev/null", ios::out);
  st_debug = NULL;

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
  copyNumber = 0;
  ddlin = NULL;
  data = 0;
  srcid = 0;
  SetSrcid("");
  // Redirect all debugging information to /dev/null
  //st_debug = new ofstream("/dev/null", ios::out);
  st_debug = NULL;
}

DDLSymbolTable::~DDLSymbolTable() {
	if(data)
		delete[] data;
    if (ddlin) {
        ddlin->close();
        delete ddlin;
    }
    if (st_debug) {
        st_debug->close();
        delete st_debug;
    }
    if (srcid) {
        free(srcid);
    }
}

DDLNode* DDLSymbolTable::Clone(bool dataflag)
{
    DDLSymbolTable* tmp = new DDLSymbolTable;
    
    const char *pfx = "Clone(\"";
    const char *sfx = "\")";
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

void DDLSymbolTable::SetData(float* _data, int _data_length)
{
  if (_data_length <= 0) return;
  
  if (data) 
	  delete [] data;
  
  data = new float[_data_length / sizeof(float)];
  
  memcpy(data, _data, _data_length);
  data_length = _data_length;
}

void DDLSymbolTable::SetSrcid(const char* newid)
{
  if (srcid) {
    free(srcid);
  }
  srcid = strdup(newid);
}

//
//  invalidate mapped data
//
void DDLSymbolTable::ClrData() {
	data=NULL;
	data_length=0;
}

//
//  Returns true if data successfully read, otherwise false.
//
char *DDLSymbolTable::MallocReadData(char *mapfile, MFILE_ID *mobj) {
	struct stat buf;
	ifstream ddl_in;
	static char msg[1024]; // This can get returned to caller
	msg[0]=0;
	
	if(mobj!=NULL)
		*mobj=NULL;

	file_length = data_length = header_length = 0;

	if (stat(srcid, &buf) != 0) {
		sprintf(msg, "File not found: %.900s", srcid);
		return msg;
	}
	ddl_in.open(srcid, ios::in);
	file_length = (int) buf.st_size;
	//debug << "length of " << srcid << " = " << file_length << "\n";

	char c;
	header_length = 0;
	while ((c = ddl_in.get()) != EOF&& c != 0) {
		header_length++;
	}

	int rank = 0;
	int bits = 0;
	unsigned long buflen=0;
	GetValue("rank", rank);
	GetValue("bits", bits);
	buflen = bits / 8; // Bytes per word
	for (int i = 0; i < rank; ++i) {
		int dim = 0;
		GetValue("matrix", dim, i);
		buflen *= dim;
	}

	if (data)
		delete [] data;
	data = 0;

	if (c == EOF) {
		sprintf(msg, "No NULL at end of DDL header: %.900s", srcid);
		return msg;
	} else {
		header_length++;
		data_length = file_length - header_length;

		if (data_length <= 0) {
			sprintf(msg, "Entire file is DDL header: %.900s", srcid);
			return msg;
		} else if (data_length != buflen) {
			sprintf(msg, "Specified image bytes=%d, data size=%d: %s", (int) buflen,
					data_length, srcid);
			return msg;
		} else if (mapfile != NULL) {			
			MFILE_ID mapid=mOpen(mapfile,data_length, O_RDWR | O_CREAT);			
			if(mapid==NULL){
				sprintf(msg, "Could not mmap file: %.900s", mapfile);
				return msg;				
			}
			if(mobj!=NULL)
				*mobj=mapid;
			data=(float*)mapid->offsetAddr;
		}
		else
			data = new float[data_length / sizeof(float)];
		if (data == 0) {
			sprintf(msg, "Cannot allocate memory for %.900s", srcid);
			return msg;
		}
		ddl_in.read((char *)data, data_length);

		int bigendian_hdr = 1;
		GetValue("bigendian", bigendian_hdr);

		if (bigendian_hdr != BigEndian) {
			int num = data_length / sizeof(float);
			long *lptr = (long *) data;
			int cnt;
			for (cnt = 0; cnt < num; cnt++) {
				ByteSwap5(*lptr);
				lptr++;
			}
		}
		if (ddl_in.gcount() == 0) {
			sprintf(msg, "Cannot read data from %.900s", srcid);
			return msg;
		}
	}
	return msg;
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
  ostringstream temp;   /* temporary stream to hold header as it's being build */
  
  /* First write out the magic string */
  temp << MAGIC_STRING << "\n";

  /* Generate the creator string */
  time_t clock = 0;      // date in seconds
  char *tdate;         // pointer to the time
  char datetim[26];

  clock = time(NULL);
  SetValue("creation_time", clock);
  
  if ((tdate = ctime(&clock)) != NULL) {
     strcpy(datetim,tdate);
     datetim[24] = '\0';
   } else {
     strcpy(datetim,"");
   }

  SetValue("user", UserName);
  SetValue("hostname", HostName);
  SetValue("bigendian", BigEndian);
                                             
  temp << "/* Created by: " << UserName << "@" << HostName
    << " on " << datetim << " */" << "\n\n" ;
  
  /* Now recompute the checksum for the data */
  //tcrc csum = addbfcrc(data, data_length);

  //SetValue("checksum", csum);
  
  /* Now print all the symbols in the table */
  PrintSymbols(temp);

  /* Now capture the header from the temporary stream into a string pointer */
  string header = temp.str();

  /* Copy the header to the real output stream */
  os << header;
  
  /* Compute the length of the header */
  int temp_header_length = strlen(header.c_str());

  /* Compute the padding required to align the data */
  /* We need to add at least one character -- the terminating null */
  int padding = 0;
  int extra = (temp_header_length + 1) % alignment_size;
  if (extra != 0) {
      padding = alignment_size - extra;
  }
  for (int i = 0; i < padding; i++) {
    os << '\n';
  }
  /* Now add the header termination character */
  os << (char)0 ;
 
  /* Finally, write out the data */
  os.write((char *)data, data_length);
}

void DDLSymbolTable::SaveSymbolsAndData(const char* filename)
{
  ofstream fout;

  fout.open(filename);
  if (!fout) return;
  SaveSymbolsAndData(fout);
  fout.close();
}

void DDLSymbolTable::PrintSymbols(ostringstream& os)
{
  DDLNodeIterator st(this);
  DDLNode *sp;

  os << "/* Symbol Table */" << "\n";
  while ( (sp = ++st) ) {
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
	string typestr = "void ";
	string arraystr = "";
	DDLNode *sq = sp->val;
	if (sq){
	    if (sq->IsType(ARRAY_DATA)){
		arraystr = "[]";
		if ( (sr=sq->Top()) ) {
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
  os << "\f";		/* Stops a "more" listing */
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
  while ( (sp = ++st) ) {
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
	string typestr = "void ";
	string arraystr = "";
	DDLNode *sq = sp->val;
	if (sq){
	    if (sq->IsType(ARRAY_DATA)){
		arraystr = "[]";
		if ( (sr=sq->Top()) ) {
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
  os << "\f";		/* Stops a "more" listing */
}

void DDLSymbolTable::PrintSymbolsAll(ostream& os)
{
  DDLNodeIterator st(this);
  DDLNode *sp;

  os << "/* Symbol Table: */" << "\n";
  while ( (sp = ++st) ) {
    os << sp->GetName() << " = ";
    if (sp->symtype != UNDEFINED) {
      *sp >> os << "\n";
    } else {
      os << "UNDEFINED" << "\n";
    }
  }
  os << "\f";		/* Stops a "more" listing */
}


/*ostream& DataValue::operator>>(ostream& os)
{
  return os << "DataValue!" << "\n";
}*/


DDLNode* DDLSymbolTable::Install(const char *s, int t)
{
  DDLNode *sp = new DDLNode(s, t);

  sp->val = 0;
  Push(sp);
  return sp;
}

DDLNode* DDLSymbolTable::Install(const char *s, int t, double d)
{
  DDLNode *sp = Install(s, t);
  sp->val = new RealData(d);
  return sp;
}

DDLNode* DDLSymbolTable::Install(const char *s, int t, const char* d)
{
  DDLNode *sp = Install(s, t);
  if (d == NULL) d = (char *)"";
  sp->val = new StringData(d);
  return sp;
}

DDLNode* DDLSymbolTable::Lookup(const char *nme)
{
  DDLNodeIterator st(this);

  DDLNode* sp;

  while ( (sp = ++st) ) {
    if (strcmp(sp->GetName(), nme) == 0) break;
  }
  if (sp) {
    return sp;
  } else {
    return NullDDL;
  }
}


DDLNode* DDLSymbolTable::CreateArray(const char *nme)
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


DDLNode* DDLSymbolTable::AppendElement(const char *nme, ArrayData *value)
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

DDLNode* DDLSymbolTable::AppendElement(const char *nme, double value)
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


DDLNode* DDLSymbolTable::AppendElement(const char *nme, const char *value)
{
  DDLNode *sp = Lookup(nme);

  if (sp == NullDDL) {
    /* No such array, first create one */
    sp = CreateArray(nme);
  }
         
  if (value == NULL) value = (char *)"";
  StringData *rd = new StringData(value);
  if (sp->val) {
    sp->val->Push(rd);
  }
  return sp;
    
}

bool
DDLSymbolTable::GetValue(const char *nme, char*& value)
{
  DDLNode *valu = Lookup(nme);

  if (valu && (valu->val) && valu->val->IsType(STRING_DATA)) {
    if (valu != NullDDL) {
      value = (char*) (*valu);
      return true;
    }
  }
  return false;
}

bool
DDLSymbolTable::GetValue(const char *nme, double& value)
{
  DDLNode *valu = Lookup(nme);

  if (valu && (valu->val) && valu->val->IsType(REAL_DATA)) {
    if (valu != NullDDL) {
      value = (double) (*valu);
      return true;
    }
  }
  return false;
}


bool
DDLSymbolTable::GetValue(const char *nme, int& value)
{
  DDLNode *valu = Lookup(nme);

  if (valu && (valu->val) && valu->val->IsType(REAL_DATA)) {
    if (valu != NullDDL) {
      value = (int) (double) (*valu);
      return true;
    }
  }
  return false;
}
  

bool
DDLSymbolTable::SetValue(const char *nme, double _value)
{
  DDLNode *valu = Lookup(nme);

  if (valu == NullDDL) {
    Install(nme, VAR, _value);
  } else if (valu && (valu->val) && valu->val->IsType(REAL_DATA)) {
    ((RealData*) valu->val)->value = _value;
  } else {
    return false;
  }
  return true;
}

bool
DDLSymbolTable::SetValue(const char *nme, const char* value)
{
  DDLNode *valu = Lookup(nme);

  if (valu == NullDDL) {
    Install(nme, VAR, value);
  } else if (valu && (valu->val) && valu->val->IsType(STRING_DATA)) {
      ((StringData*) valu->val)->SetString(value);
  } else {
    return false;
  }
  return true;
}

bool
DDLSymbolTable::GetValue(const char *nme, int& value, int idx)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0) return false;
  
  DDLNode *s = p->GetArray();
  if (s == 0) return false;
  
  DDLNode *q = s->Index(idx);
  if (q == 0) return false;
  
  value = (int)((RealData*) q)->value;
  return true;
}

  
bool
DDLSymbolTable::GetValue(const char *nme, double& value, int idx)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0) return false;
  
  DDLNode *s = p->GetArray();
  if (s == 0) return false;
  
  DDLNode *q = s->Index(idx);
  if (q == 0) return false;
  
  value = (double) *(s->Index(idx));
  return true;
}

bool
DDLSymbolTable::SetValue(const char *nme, double _value, int idx)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0 || p == NullDDL) return false;
  
  DDLNode *s = p->GetArray();
  if (s == 0 || p == NullDDL || !p->val) return false;
  
  RealData *rd;
  while (s->LengthOf() < idx+1){
      rd = new RealData(0.0);
      p->val->Push(rd);
  }

  DDLNode *q = s->Index(idx);
  if (q == 0 || p == NullDDL) return false;
  ((RealData*) q)->value = _value;
  return true;
}

bool
DDLSymbolTable::SetValue(const char *nme, const char* value, int idx)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0 || p == NullDDL) return false;
  
  DDLNode *s = p->GetArray();
  if (s == 0 || p == NullDDL || !p->val) return false;

  StringData *rd;
  while (s->LengthOf() < idx+1){
      rd = new StringData("");
      p->val->Push(rd);
  }

  DDLNode *q = s->Index(idx);
  if (q == 0 || p == NullDDL) return false;
  
  if ( ((StringData*) q)->val) {
    delete (((StringData*) q)->val);
  }
  if (value == NULL) value = (char *)"";
  ((StringData*) q)->SetString(value);
  return true;
}

int DDLSymbolTable::LengthOf(const char *nme)
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
  
bool
DDLSymbolTable::GetValue(const char *nme, char*& value, int idx)
{
  DDLNode *p = (Lookup(nme));
  if (p == 0) return false;
  
  DDLNode *s = p->GetArray();
  if (s == 0) return false;
  
  DDLNode *q = s->Index(idx);
  if (q == 0) return false;
  
  value = (char*) *(s->Index(idx));
  return true;
}

ArrayData* DDLSymbolTable::GetArray(const char *nme)
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

bool
DDLSymbolTable::IsDefined(const char *nme)
{
  if (Lookup(nme) != NullDDL) {
    return true;
  } else {
    return false;
  }
}

bool
DDLSymbolTable::NotDefined(const char *nme)
{
  if (IsDefined(nme)) {
    return false;
  } else {
    return true;
  }
}

void ByteSwap(unsigned char * b, int n)
{
   register int i = 0;
   register int j = n-1;
   while (i<j)
   {
      std::swap(b[i], b[j]);
      i++, j--;
   }
}

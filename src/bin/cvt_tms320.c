/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Preliminary version of a program to convert DSP .out file, as written out
    by the TMS320 program lnk30 on an Intel-based Personal Computer.  Becuase
    the Personal Computer orders bytes in the opposite manner from Motorola/
    SPARC, it is necessary to convert 16-bit and 32-bit numbers.  The contents
    of the sections, the real stuff here, are all 32-bit numbers.

    The output is an image of the program with "holes" filled with TMS320 NOPs.
    It is written to standard output, so redirect standard out.

    This program is subject to change without consultation or notice.		*/


/*  11/1995   For some reason it is required to subtract 0x20000 from the
              virtual address in the section header.  Beware the virtual
              addresses are expressed in units of longwords; it is thus
              required to multiply the address by 4 to obtain the correct
              address in VME address space.  This subtract is done BEFORE
              the required multiplication; you could also multiply the
              virtual address by 4 and then subtract 0x80000.		*/

/*  12/1995   Extended to parse arguments in a format similar to the C compiler
              Thus:  "cvt_tms320 dsp.out" yields dsp.ram as output and
              "cvt_tms320 -o dsp2.ram dsp.out" yields dsp2.ram as output.	*/


#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <sys/mman.h>

#define  TMS320_MAGIC	0x093
#define  TMS320_NOP	0x0c800000


static char	inputFile[ 122 ];
static char	outputFile[ 122 ];
static int	swap = 0;


typedef struct _tms320Header {
	unsigned short	magicNumber;
	unsigned short	numSectHeaders;
	long int	timeStamp;
	long int	symTableOffset;
	long int	numEntrySymTable;
	unsigned short	optHeaderSize;
	unsigned short	headerFlags;
} tms320Header;

typedef struct _optTms320Header {
	short		magicNumber;
	short		versionNumber;
	long		sizeExecuteCode;
	long		sizeInitdData;
	long		sizeUninitdData;
	long		startAddress;
	long		entryPoint;
	long		startInitdData;
} optTms320Header;

typedef struct _tms320SectHeader {
	char		name[ 8 ];
	long		physAddr;
	long		virtualAddr;
	long		size;
	long		offset2data;
	long		offset2reloc;
	long		offset2linenum;
	unsigned short	numReloc;
	unsigned short	numLineNum;
	unsigned short	flags;
	char		reserved;
	unsigned char	memPageNum;
} tms320SectHeader;


char *
fillMemoryWithPattern( char *memAddr, int memSize, char *pattern, int patternSize )
{
	int	iter;
	short	spattern, *sAddr;
	long	lpattern, *lAddr;

	if (patternSize < 1)
	  return( NULL );
	if (patternSize > memSize)	/* Prevent writing past the end of memory */
	  patternSize = memSize;

	if (patternSize == sizeof( char )) {
		memset( memAddr, *pattern, memSize );
	}
	else if (patternSize == sizeof( short )) {
		spattern = *((short *) pattern);
		sAddr = (short *) memAddr;

		for (iter = 0; iter < memSize / sizeof (short); iter++)
		  *sAddr++ = spattern;
	}
	else if (patternSize == sizeof( long )) {
		lpattern = *((long *) pattern);
		lAddr = (long *) memAddr;

		for (iter = 0; iter < memSize / sizeof (long); iter++)
		  *lAddr++ = lpattern;
	}
	else {
		for (iter = 0; iter < memSize / patternSize; iter++) {
			memcpy( memAddr, pattern, patternSize );
			memAddr += patternSize;
		}
	}
}

cvt16( short sval )
{
	char	tchar;
	union {
		char	c[ 2 ];
		short	sval;
	} retval;

	retval.sval = sval;
	tchar = retval.c[ 0 ];
	retval.c[ 0 ] = retval.c[ 1 ];
	retval.c[ 1 ] = tchar;

	return( retval.sval );
}

cvt32( long lval )
{
	char	tchar;
	union {
		char	c[ 4 ];
		long	lval;
	} retval;

	retval.lval = lval;
	tchar = retval.c[ 0 ];
	retval.c[ 0 ] = retval.c[ 3 ];
	retval.c[ 3 ] = tchar;
	tchar = retval.c[ 1 ];
	retval.c[ 1 ] = retval.c[ 2 ];
	retval.c[ 2 ] = tchar;

	return( retval.lval );
}

void
cvtTms320Header( tms320Header *tHeader )
{
	tHeader->magicNumber      = cvt16( tHeader->magicNumber );
	tHeader->numSectHeaders   = cvt16( tHeader->numSectHeaders );
	tHeader->timeStamp        = cvt32( tHeader->timeStamp );
	tHeader->symTableOffset   = cvt32( tHeader->symTableOffset );
	tHeader->numEntrySymTable = cvt32( tHeader->numEntrySymTable );
	tHeader->optHeaderSize    = cvt16( tHeader->optHeaderSize );
	tHeader->headerFlags      = cvt16( tHeader->headerFlags );
}

void
cvtOptHeader( optTms320Header *tHeader )
{
	tHeader->magicNumber     = cvt16( tHeader->magicNumber );
	tHeader->versionNumber   = cvt16( tHeader->versionNumber );
	tHeader->sizeExecuteCode = cvt32( tHeader->sizeExecuteCode );
	tHeader->sizeInitdData   = cvt32( tHeader->sizeInitdData );
	tHeader->sizeUninitdData = cvt32( tHeader->sizeUninitdData );
	tHeader->startAddress    = cvt32( tHeader->startAddress );
	tHeader->entryPoint      = cvt32( tHeader->entryPoint );
	tHeader->startInitdData  = cvt32( tHeader->startInitdData );
}

void
cvtSectHeader( tms320SectHeader *tHeader )
{
	tHeader->physAddr       = cvt32( tHeader->physAddr );
	tHeader->virtualAddr    = cvt32( tHeader->virtualAddr );
	tHeader->size           = cvt32( tHeader->size );
	tHeader->offset2data    = cvt32( tHeader->offset2data );
	tHeader->offset2reloc   = cvt32( tHeader->offset2reloc );
	tHeader->offset2linenum = cvt32( tHeader->offset2linenum );
	tHeader->numReloc       = cvt16( tHeader->numReloc );
	tHeader->numLineNum     = cvt16( tHeader->numLineNum );
	tHeader->flags          = cvt16( tHeader->flags );
}

int
accessFileMmapRead( char *fileName, char **mmap_addr )
{
	int	 ival, tmpfd, len;
	char	*tmp_mmap_addr;

	tmpfd = open( fileName, 0 );
	if (tmpfd < 0) {
		fprintf( stderr, "can't access %s\n", fileName );
		return( -1 );
	}

	len = lseek( tmpfd, 0, SEEK_END );
	ival = lseek( tmpfd, 0, SEEK_SET );

	tmp_mmap_addr = mmap( NULL, len, PROT_READ, MAP_SHARED, tmpfd, 0 );
	if (tmp_mmap_addr == (char *) -1) {
		perror( "mmap failure" );
		return( -1 );
	}

	*mmap_addr = tmp_mmap_addr;
	return( 0 );
}

int
findMinMaxAddr( tms320SectHeader *sectAddr, int numSections, char **minAddr, char **maxAddr )
{
	int		 iter;
	int		 addrAssigned;
	char		*tmpMinAddr, *tmpMaxAddr;
	tms320SectHeader sectHeader;

	addrAssigned = 0;

	for (iter = 0; iter < numSections; iter++) {
		memcpy( (char *) &sectHeader, sectAddr, sizeof( sectHeader ) );
		sectAddr++;
		if (swap)
		   cvtSectHeader( &sectHeader );

		if (sectHeader.size < 1 || sectHeader.offset2data < 1)
		  continue;

		if (sectHeader.virtualAddr < 0x021000) {
			fprintf( stderr,
		   "error: section missing offset of 0x21000\n"
			);
			exit( 1 );
		}
		sectHeader.virtualAddr -= 0x021000;
		sectHeader.virtualAddr *= 4;
		sectHeader.size *= 4;

		if (addrAssigned == 0) {
			tmpMinAddr = (char *) sectHeader.virtualAddr;
			tmpMaxAddr = (char *) (sectHeader.virtualAddr + sectHeader.size);
			addrAssigned = 131071;
		}
		else {
			if (tmpMinAddr > (char *) sectHeader.virtualAddr)
			  tmpMinAddr = (char * ) sectHeader.virtualAddr;
			if (tmpMaxAddr < (char *) (sectHeader.virtualAddr + sectHeader.size))
			  tmpMaxAddr = (char * ) (sectHeader.virtualAddr + sectHeader.size);
		}
	}

	if (addrAssigned) {
		*minAddr = tmpMinAddr;
		*maxAddr = tmpMaxAddr;
	}

	if (addrAssigned)
	  return( 0 );
	else
	  return( -1 );
}

fillMemoryFromSections(
	char *promSpaceAddr,
	char *mmap_addr,
	tms320SectHeader *sectAddr,
	int numSections
)
{
	int		 iter, iter_2;
	long		*laddr;
	tms320SectHeader sectHeader;

        for (iter = 0; iter < numSections; iter++) {
		memcpy( (char *) &sectHeader, sectAddr, sizeof( sectHeader ) );
		sectAddr++;
		if (swap)
		   cvtSectHeader( &sectHeader );

		if (sectHeader.size < 1 || sectHeader.offset2data < 1)
		  continue;

		if (sectHeader.virtualAddr < 0x021000) {
			fprintf( stderr,
		   "error: section missing offset of 0x21000\n"
			);
			exit( 1 );
		}
		sectHeader.virtualAddr -= 0x021000;
		sectHeader.virtualAddr *= 4;
		sectHeader.size *= 4;

		/*fprintf( stderr, "Section %d, name %.8s\n", iter, sectHeader.name );
		fprintf( stderr, "writing %d chars from file offset %d to PROM offset 0x%x\n",
			 sectHeader.size, sectHeader.offset2data, sectHeader.virtualAddr );*/

		memcpy( promSpaceAddr + sectHeader.virtualAddr, 
			mmap_addr + sectHeader.offset2data,
			sectHeader.size
		);

/*  Convert the section contents from Motorola/SPARC to Intel format  */

		laddr = (long *) (promSpaceAddr + sectHeader.virtualAddr);
		for (iter_2 = 0; iter_2 < sectHeader.size / sizeof( long ); iter_2++) {
			   *laddr = cvt32( *laddr );
			laddr++;
		}
	}
}

int
process_args( argc, argv )
int argc;
char *argv[];
{
	char	*tptr;
	int	 iter;

	if (argc < 2)
	  return( -1 );

	inputFile[ 0 ] = '\0';
	outputFile[ 0 ] = '\0';

	iter = 0;
	while (iter < argc) {
		if (strcmp( "-swap", argv[ iter ] ) == 0) {
		   fprintf( stdout, "%s: Intel PC swap option invoked.\n", argv[ 0 ] );
		   swap = 1;
		}
                else if (strcmp( "-o", argv[ iter ] ) == 0) {
			iter++;
			if (strlen( argv[ iter ] ) > sizeof( outputFile ) - 1) {
				fprintf( stderr,
					"%s: error, output file name is too long\n", argv[ 0 ]
				);
				return( -1 );
			}
			else
			  strcpy( &outputFile[ 0 ], argv[ iter ] );
		}
		else {
			if (strlen( argv[ iter ] ) > sizeof( inputFile ) - 1) {
				fprintf( stderr,
					"%s: error, input file name is too long\n", argv[ 0 ]
				);
				return( -1 );
			}
			else
			  strcpy( &inputFile[ 0 ], argv[ iter ] );
		}

		iter++;
	}

	if (strlen( &outputFile[ 0 ] ) < (unsigned int) 1) {
		if (strlen( &inputFile[ 0 ] ) < (unsigned int) 1) {
			fprintf( stderr, "%s: internal programming error\n", argv[ 0 ] );
			return( -1 );
		}

		strcpy( &outputFile[ 0 ], &inputFile[ 0 ] );
		tptr = strchr( &outputFile[ 0 ], '.' );
		if (tptr == NULL)
		  strcat( &outputFile[ 0 ], ".ram" );
		else
		  strcpy( tptr, ".ram" );
	}

	return( 0 );
}

main( argc, argv )
int argc;
char *argv[];
{
	int		 iter, ival, len, outfd;
	int		 promSize;
	long		 tms320_nop;
	char		*mmap_addr, *sectAddr, *minAddrProm, *maxAddrProm, *promSpaceAddr;
	tms320Header	 basicHeader;
	optTms320Header	 optHeader;

	if (process_args( argc, argv ) < 0)
	  exit( 1 );

	ival = accessFileMmapRead( &inputFile[ 0 ], &mmap_addr );
	if (ival < 0) {
		exit( 1 );
	}
	outfd = open( &outputFile[ 0 ], O_WRONLY | O_CREAT, 0644 );
	if (outfd < 1) {
		fprintf( stderr, "%s: can't access %s\n", argv[ 0 ], &outputFile[ 0 ] );
		exit( 1 );
	}

	memcpy( (char *) &basicHeader, mmap_addr, sizeof( basicHeader ) );
        if (swap)
	  cvtTms320Header( &basicHeader );

	if (basicHeader.magicNumber != TMS320_MAGIC) {
		fprintf( stderr, "input file is not a TMS320 executable\n" );
		exit( 1 );
	}
	else if (basicHeader.optHeaderSize != 0 &&
	         basicHeader.optHeaderSize != sizeof( optHeader )) {
		fprintf( stderr, "file header's optional header size of %d not recognized\n",
			 basicHeader.optHeaderSize );
		exit( 1 );
	}
	else if (basicHeader.optHeaderSize != 0) {
		memcpy( (char *) &optHeader,
				  mmap_addr + sizeof( basicHeader ),
			  sizeof( optHeader )
		);
        	if (swap)
		   cvtOptHeader( &optHeader );

		sectAddr = mmap_addr + sizeof( basicHeader ) + sizeof( optHeader );
	}
	else
	  sectAddr = mmap_addr + sizeof( basicHeader );

	ival = findMinMaxAddr(
		(tms320SectHeader *) sectAddr,
		(int) basicHeader.numSectHeaders,
		      &minAddrProm, &maxAddrProm
	);
	if (ival < 0) {
		fprintf( stderr, "input file has no sections with data or text\n" );
		exit( 1 );
	}


	fprintf( stderr, "PROM addresses run from 0x%x to 0x%x\n", minAddrProm, maxAddrProm );

	promSize = (int) maxAddrProm;
	promSpaceAddr = malloc( promSize );
	if (promSpaceAddr == (char *) NULL) {
		fprintf( stderr, "Cannot allocate %d chars of memory\n", promSize );
		exit( 1 );
	}

	tms320_nop = TMS320_NOP;
	fillMemoryWithPattern( promSpaceAddr, promSize, (char *) &tms320_nop, sizeof( tms320_nop ) );

	fillMemoryFromSections( promSpaceAddr, mmap_addr,
		(tms320SectHeader *) sectAddr,
		(int) basicHeader.numSectHeaders
	);

	write( outfd, promSpaceAddr, promSize );
	exit( 0 );
}

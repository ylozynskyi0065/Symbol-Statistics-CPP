#pragma once
#include <stdio.h>

#define SYMBOL_BITCOUNT					25							//max 31 bits
#define SYMBOL_COUNT_MAX				(1<<SYMBOL_BITCOUNT)		//0,1...

#define BITREADER_FILE_SEGMENT_BUFFER_SIZE		128*1024*1024	//size given in bytes. Must be bigger then 4 and preferably = 2^x + 4

struct bitreader
{
	FILE			*inf;
	FILE			*outf;
	unsigned char	*cbuffer;
	int				readcount;				// last file read gave us X bytes
	int				TotalReadCount;
	int				processed_count;		// total byte count read
	__int64			have_symbols;
	int				FileSize;
	__int64			*symbol_counts;
	__int64			*symbol_counts2;
	__int64			*symbol_counts_encode;
	int				SymbolBitCount;
	int				SymbolCount;
	int				SymbolCounterSize;
	unsigned char	*cbitmask;
	unsigned char	*cbitmask2;
	unsigned char	*cbitmask3;
	__int64			BitmaskSize;
	__int64			BitmaskSize2;
	__int64			BitmaskSize3;
	__int64			BitMaskIndex;
	__int64			BitMaskIndex2;
	__int64			BitMaskIndex3;
	int				ReadStartSkipp;
};

int CountBitsRequiredStore(unsigned __int64 val);
unsigned __int64 GetSymbolAtLoc(unsigned char *cbuffer, unsigned __int64 CurLoc, int BitCount);
void SetSymbolAtLoc(unsigned char *cbuffer, unsigned __int64 CurLoc, int BitCount, unsigned __int64 ValNew);
int GetFileSize(FILE *fp);
void bitreader_open(bitreader &br, const char *infilename, int SymbolBitCount);
void bitreader_close(bitreader &br);
__int64 Count1s(unsigned __int64 Symbol);
int GetBitFromBuff(unsigned char* buff, int BitIndex);
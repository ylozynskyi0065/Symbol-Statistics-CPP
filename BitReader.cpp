#include "BitReader.h"
#include <windows.h>
#include <string.h>

int CountBitsRequiredStore(unsigned __int64 val)
{
	if (val == 0)
		return 1;
	val -= 1;
	int MSBAt = 0;
	while (val)
	{
		MSBAt++;
		val >>= 1;
	}
	if (MSBAt == 0)
		MSBAt = 1;
	return MSBAt;
}

//max 56 bits
unsigned __int64 GetSymbolAtLoc(unsigned char *cbuffer, unsigned __int64 CurLoc, int BitCount)
{
	unsigned __int64 ByteIndex = CurLoc / 8;
	unsigned __int64 BitIndex = CurLoc % 8;
	unsigned __int64 InterestMask = ((unsigned __int64)1 << BitCount) - 1;
	unsigned __int64 Value = *(unsigned __int64*)&cbuffer[ByteIndex];
	Value >>= BitIndex;
	Value &= InterestMask;
	return Value;
}

//max 56 bits
void SetSymbolAtLoc(unsigned char *cbuffer, unsigned __int64 CurLoc, int BitCount, unsigned __int64 ValNew)
{
	unsigned __int64 ByteIndex = CurLoc / 8;
	unsigned __int64 BitIndex = CurLoc % 8;
	unsigned __int64 ValNewLarge = ValNew;
	unsigned __int64 InterestMask = ((unsigned __int64)1 << BitCount) - 1;
	InterestMask <<= BitIndex;
	ValNewLarge <<= BitIndex;
	ValNewLarge &= InterestMask;
	unsigned __int64 ValueOld = *(unsigned __int64*)&cbuffer[ByteIndex];
	ValueOld = ValueOld & (~InterestMask);
	ValNewLarge |= ValueOld;
	*(unsigned __int64*)&cbuffer[ByteIndex] = ValNewLarge;
}

int GetFileSize(FILE *fp)
{
	if (fp == NULL)
		return 0;
	fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return sz;
}

void bitreader_open(bitreader &br, const char *infilename, int SymbolBitCount)
{
	// open file
	br.inf = fopen(infilename, "rb");
	//alloc buffer
	br.FileSize = GetFileSize(br.inf);
	br.cbuffer = (unsigned char*)malloc(BITREADER_FILE_SEGMENT_BUFFER_SIZE + 8);
	// fill buffer initially
	br.readcount = (unsigned int)fread(br.cbuffer, 1, BITREADER_FILE_SEGMENT_BUFFER_SIZE, br.inf);
	// make sure all buffer is initialized
//	for(int i=br.readcount;i<BITREADER_FILE_SEGMENT_BUFFER_SIZE;i++)
//		br.cbuffer[i] = 0;
	// no blocks processed yet 
	br.processed_count = 0;
	//alloc counter buffer
	br.SymbolBitCount = SymbolBitCount;
	br.SymbolCount = 1 << br.SymbolBitCount;
	br.SymbolCounterSize = sizeof(__int64) * (br.SymbolCount + 1);
	br.symbol_counts = (__int64*)malloc(br.SymbolCounterSize);
	br.symbol_counts2 = (__int64*)malloc(br.SymbolCounterSize);
	br.symbol_counts_encode = (__int64*)malloc(br.SymbolCounterSize);
	memset(br.symbol_counts, 0, br.SymbolCounterSize);
	memset(br.symbol_counts_encode, 0, br.SymbolCounterSize);
	br.have_symbols = 0;
	br.TotalReadCount = br.readcount;
	br.cbitmask = br.cbitmask2 = br.cbitmask3 = 0;
	br.BitMaskIndex = br.BitMaskIndex2 = br.BitMaskIndex3 = 0;
	br.BitmaskSize = br.BitmaskSize2 = br.BitmaskSize3 = 0;
	br.outf = fopen("out.dat", "wb");
	br.ReadStartSkipp = 0;
}

void bitreader_close(bitreader &br)
{
	// close file
	if (br.inf)
	{
		fclose(br.inf);
		br.inf = NULL;
	}
	if (br.outf)
	{
		fclose(br.outf);
		br.outf = NULL;
	}
	if (br.cbuffer)
	{
		free(br.cbuffer);
		br.cbuffer = NULL;
	}
	if (br.symbol_counts)
	{
		free(br.symbol_counts);
		br.symbol_counts = NULL;
	}
	if (br.symbol_counts2)
	{
		free(br.symbol_counts2);
		br.symbol_counts2 = NULL;
	}
	if (br.symbol_counts_encode)
	{
		free(br.symbol_counts_encode);
		br.symbol_counts_encode = NULL;
	}
	if (br.cbitmask)
	{
		free(br.cbitmask);
		br.cbitmask = NULL;
	}
}

__int64 Count1s(unsigned __int64 Symbol)
{
	int Count = 0;
	for (unsigned __int64 i = 0; i < 64; i++)
		if (((unsigned __int64)1 << i) & Symbol)
			Count++;
	return Count;
}

//did not test it yet
int GetBitFromBuff(unsigned char* buff, int BitIndex)
{
	unsigned char* buff2 = buff + BitIndex / 8;
	int BitIndex2 = BitIndex % 8;
	int ret = buff2[0];
	ret = ret >> BitIndex2;
	return ret & 1;
}
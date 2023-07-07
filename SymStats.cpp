#include "SymStats.h"
#include "BitReader.h"

void bitreader_stat_symbols(bitreader& br, int ResetCounters, int allbytes, int ForcedBitcount = 0, int SkipPrintf = 0, int MaskAlsoFlags = 1);

void PrintStatisticsf(const char* FileName, int SymbolBitcount)
{
	//not checked if indeed working
	bitreader br;
	bitreader_open(br, FileName, SymbolBitcount);
	bitreader_stat_symbols(br, 1, 0, SymbolBitcount);
}

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <math.h>
#include <assert.h>
#include "BitReader.h"
#include "HuffmanTree.h"
//#include "AriEnc.h"

/******************************************
/ count all possible simbols with len 4-8
/ symbol must have highest bit 1 and can have any length
******************************************/

#ifdef min
	#undef min
#endif
#define min(a,b) (a<b?a:b)

float BestPCT = 99999.0f;
int *Costs;

//#define SEGMENT_BITCOUNT			1024*8
#define SEGMENT_BITCOUNT			2*8

int GetOutArchivedSize( bitreader &br )
{
	if( br.cbitmask != NULL && br.BitMaskIndex > 0 )
		fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	fwrite( br.cbuffer, 1, br.readcount, br.outf );
	fclose( br.outf );

	int ret = system( "\"c:/Program Files/WinRAR/rar.exe\" a out.rar out.dat" );

	FILE *tf = fopen( "out.rar","rb");
	int Size = GetFileSize( tf );
	fclose( tf );

	br.outf = fopen("out.dat","wb");

	return Size;
}

void PrintStatistics( bitreader &br )
{
	printf("\t Total count practical %d = %d bytes.\n", (int)br.have_symbols, (int)br.have_symbols * br.SymbolBitCount / 8 );
	printf("\n");

	int	MissingSymbolCount = 0;
	int	UniqueSymbolCount = 0;
	for(int i=0;i<br.SymbolCount;i++)
		if( br.symbol_counts_encode[ i ] <= 0 )
			MissingSymbolCount++;
		else
			UniqueSymbolCount++;
	assert( UniqueSymbolCount + MissingSymbolCount == br.SymbolCount );
	printf("\t Theoretical unique symbol count %d \n" ,br.SymbolCount );
	printf("\t Practical unique symbol count   %d \n" , UniqueSymbolCount );
	printf("\t Perfect distribution %d \n" , br.have_symbols / UniqueSymbolCount );
	printf("\n");

	int QuickRecombineSymbolCount = 0;
	for(int i=0;i<br.SymbolCount;i++)
		QuickRecombineSymbolCount += br.symbol_counts_encode[ i ];
	printf("Recombine sum bytes %d = %d MB\n", QuickRecombineSymbolCount * br.SymbolBitCount / 8, QuickRecombineSymbolCount * br.SymbolBitCount / 8 / 1024 / 1024 );
	printf("\n");

	float CompressionPCT = 66;
	if( UniqueSymbolCount < ( 1 << 16 ) )
	{
		int StampTreeStart = GetTickCount();
		HuffmanTree HTree;
		HTree.BuildTree( br.symbol_counts_encode, br.SymbolCount );

		HTree.SearchSetSymbolCost( Costs, br.SymbolCount );

		//get best case cost
		__int64 HuffmanBitsCompression = 0;
		for(int i=0;i<br.SymbolCount;i++)
			if( br.symbol_counts_encode[ i ] > 0 )
				HuffmanBitsCompression += ( br.symbol_counts_encode[ i ] * Costs[ i ] + br.SymbolBitCount + 4 * 8 ) / 8;

		printf( "Huffman new size : %d Bytes %d kBytes %d Mbytes\n", (int)HuffmanBitsCompression, (int)(HuffmanBitsCompression/1024), (int)(HuffmanBitsCompression/1024/1024) );
		CompressionPCT = (float)( HuffmanBitsCompression ) / br.FileSize;
		printf("\t As PCT %f \n" , CompressionPCT );

		printf( "Tree build time %d seconds \n", ( GetTickCount() - StampTreeStart ) / 1000 );
	}
	if( BestPCT > CompressionPCT && CompressionPCT != 0.0f )
	{
		printf( "New record : %f\n",CompressionPCT);
		BestPCT = CompressionPCT;
	}

	//find and report min / max
#define REPORT_BEST_COUNT	16
	if( CompressionPCT < 1.5f )
	{
		int Max[REPORT_BEST_COUNT];
		for(int i=0;i<REPORT_BEST_COUNT;i++)
			Max[i] = 0;
		for(int i=0;i<br.SymbolCount;i++)
		{
			int MinPosInMax = 0;
			int MinValInMax = Max[0];
			for(int j=0;j<REPORT_BEST_COUNT;j++)
				if( Max[j] < MinValInMax )
				{
					MinValInMax = Max[j];
					MinPosInMax = j;
				}
			if( br.symbol_counts_encode[ i ] > MinValInMax )
				Max[ MinPosInMax ] = br.symbol_counts_encode[ i ];
		}
		int	ReportAboveCount = Max[0];
		for(int j=0;j<REPORT_BEST_COUNT;j++)
			if( Max[j] < ReportAboveCount )
				ReportAboveCount = Max[j];

		for(int i=0;i<br.SymbolCount;i++)
			if( br.symbol_counts_encode[ i ] >= ReportAboveCount )
				printf("\t\t%03d : %lld - cost %d\n", i, br.symbol_counts_encode[ i ], Costs[ i ] );

		printf("Input symbol counts : \n");
		for(int i=0;i<br.SymbolCount;i++)
			if( br.symbol_counts[ i ] >= ReportAboveCount )
				printf("\t\t%03d : %lld\n", i, br.symbol_counts[ i ] );
		printf("\n");
	}

	__int64 Count0sNew = 0,Count1sNew = 0;
	__int64 Count0sOld = 0,Count1sOld = 0;
	for(int i=0;i<br.SymbolCount;i++)
	{
		int ones = Count1s( i );
		int zeros = br.SymbolBitCount - ones;
		Count1sNew += br.symbol_counts_encode[ i ] * ones;
		Count0sNew += br.symbol_counts_encode[ i ] * zeros;

		Count1sOld += br.symbol_counts[ i ] * ones;
		Count0sOld += br.symbol_counts[ i ] * zeros;
	}
	printf("Number of 1 / 0 symbols, old / new \n");
	printf("\t\t%lld %lld %f %f\n", Count1sOld, Count0sOld, float(Count1sOld)/float(Count0sOld), float(Count0sOld)/float(Count1sOld) );
	printf("\t\t%lld %lld %f %f\n", Count1sNew, Count0sNew, float(Count1sNew)/float(Count0sNew), float(Count0sNew)/float(Count1sNew) );
	printf("\t\t%f %f \n", float(Count1sOld)/float(Count1sNew), float(Count0sOld)/float(Count0sNew) );

	printf("\n");
	printf("\n");
	printf("\n");
}

void PrintAsBinary( int val, int ForcedBitcount )
{
	for(int j=ForcedBitcount-1;j>=0;j--)
		printf( "%d", (val & ( 1<<j ) ) != 0 );
}

void bitreader_stat_symbols(bitreader &br, int ResetCounters, int allbytes, int ForcedBitcount = 0, int SkipPrintf = 0, int MaskAlsoFlags = 1 )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	if( ForcedBitcount == 0 )
		ForcedBitcount = br.SymbolBitCount;

	int can_process_bytes = br.readcount;
	if( allbytes )
		can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = 0;
	if( MaskAlsoFlags & 1 )
	{
		while( BitsRead < can_process_bytes * 8 )
		{
			int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, ForcedBitcount );
			BitsRead += ForcedBitcount;
			br.symbol_counts[ Symbol ]++;
		}
	}
	if( ( MaskAlsoFlags & 2 ) && br.cbitmask )
	{
		unsigned __int64 BitsRead = 0;
		while( BitsRead < br.BitMaskIndex )
		{
			int Symbol = GetSymbolAtLoc( br.cbitmask, BitsRead, ForcedBitcount );
			BitsRead += ForcedBitcount;
			br.symbol_counts[ Symbol ]++;
		}
	}
	if( SkipPrintf == 0 )
	{
		unsigned __int64 Sum = 0;
		if( ForcedBitcount <= 7 )
			for(int i=0;i<(1<<ForcedBitcount);i++)
			{
				printf("count ");
				PrintAsBinary( i, ForcedBitcount );
	//			printf("   : %lld Score Good %lld Score Bad %lld \n", br.symbol_counts[ i ], br.symbol_counts[ i ] * Count1s( i ), br.symbol_counts[ i ] * ( ForcedBitcount - Count1s( i ) ) );
				printf("   : %lld\n", br.symbol_counts[ i ]);
				Sum += br.symbol_counts[ i ];
			}
		printf("Sum        : %lld \n", Sum );
		if( ForcedBitcount == 1 )
		{
			printf("pct 0 / 1 : %f %f \n", (float)br.symbol_counts[ 0 ]/(float)br.symbol_counts[ 1 ], (float)br.symbol_counts[ 1 ]/(float)br.symbol_counts[ 0 ] );
			printf("pct 0 / 1 : %f %f \n", (float)br.symbol_counts[ 0 ]/(float)(br.symbol_counts[ 0 ] + br.symbol_counts[ 1 ] ), (float)br.symbol_counts[ 1 ]/(float)(br.symbol_counts[ 0 ] + br.symbol_counts[ 1 ] ) );
		}
	}
}

void AddSymbolToMemmory( int *memory, int *SymbolCountMemoryBased, int &MemoryWriteInd, int Symbol, int IsInit, int WindowSlots )
{
	//memorize what we read
	MemoryWriteInd = MemoryWriteInd + 1;
	if( MemoryWriteInd == WindowSlots )
		MemoryWriteInd = 0;

	//remove oldest symbol from counter
	int MemoryWriteIndOldest = MemoryWriteInd;
	if( IsInit == 0 )
	{
		assert( SymbolCountMemoryBased[ memory[ MemoryWriteIndOldest ] ] > 0 );
		SymbolCountMemoryBased[ memory[ MemoryWriteIndOldest ] ]--;
	}

	memory[ MemoryWriteInd ] = Symbol;

	//update our symbol appearance chances
	SymbolCountMemoryBased[ memory[ MemoryWriteInd ] ]++;
}

//DecissionCoeff -> larger then 1 means we need to be more convinced the next value is what we want
void bitreader_convert_next_block_memory(bitreader &br, int ResetCounters, int WindowSlots, int Count0, int Count1, float DecissionCoeff )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	//the bigger the memory the better it will sort appearance chances. 0 will have biggest appear chance
	// 5x symbol count already seems to provide a decent result
	int	*memory = (int	*)malloc( sizeof( int ) * WindowSlots + 65535 );
	memset( memory, 0, sizeof( int ) * WindowSlots + 65535 );
	int MemoryWriteInd = 0;
	int *SymbolCountMemoryBased = (int	*)malloc( sizeof( int ) * br.SymbolCount );
	memset( SymbolCountMemoryBased, 0, sizeof( int ) * br.SymbolCount );

	int can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64	BitsRead = 0;
	int					GuessesGood = 0;
	int					GuessesBad = 0;
	int					FlipNextN = 0;
	float				SymbolRatioFull10 = (float)(Count1) / (float)(Count1 + Count0); //if this is larger then 0.5 it means there are more 1's then 0s
	float				SymbolRatioFull01 = (float)(Count0) / (float)(Count1 + Count0); //if this is larger then 0.5 it means there are more 1's then 0s
	while( BitsRead < can_process_bytes * 8 )
	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, br.SymbolBitCount );
		int		NewSymbol = Symbol;

		if( br.have_symbols >= WindowSlots && FlipNextN == 0 )
		{
			int	Count0Memory = SymbolCountMemoryBased[ 0 ];
			int	Count1Memory = SymbolCountMemoryBased[ 1 ];

			//what are the ods that next symbol will be 0 ?
			float	SymbolRatioNow10 = (float)(Count1Memory) / (float)(WindowSlots); //when this is smaller then 0.5, there is a chance next symbol is 1
			float	SymbolRatioNow01 = (float)(Count0Memory) / (float)(WindowSlots); //when this is smaller then 0.5, there is a chance next symbol is 1
//			if( SymbolRatioNow10 > SymbolRatioFull10 * DecissionCoeff )
//			if( SymbolRatioNow01 < SymbolRatioFull01 * DecissionCoeff )
			if( Count1Memory - Count0Memory > 64 )
				FlipNextN = WindowSlots;
		}
		if( FlipNextN > 0 )
		{
			if( NewSymbol == 0 )
				GuessesGood++;
			else
				GuessesBad++;
			NewSymbol = 1 - NewSymbol;
			FlipNextN--;
		}

		AddSymbolToMemmory( memory, SymbolCountMemoryBased, MemoryWriteInd, Symbol, br.have_symbols < WindowSlots, WindowSlots );

		br.have_symbols++;
		BitsRead += br.SymbolBitCount;
		br.symbol_counts_encode[ NewSymbol ]++;
	}
	br.processed_count += can_process_bytes;

	printf("Good Guesses made   : %d \n", GuessesGood );
	printf("Bad Guesses made    : %d \n", GuessesBad );
	printf("Total Guesses made  : %d \n", GuessesBad + GuessesGood );
	printf("Total symbols have  : %d \n", br.have_symbols );
	printf("Failure strength %f %f \n", (float)(GuessesGood) / (float)(GuessesBad), (float)(GuessesBad) / (float)(GuessesGood) );
	printf("new count 0 : %d \n", br.symbol_counts_encode[ 0 ] );
	printf("new count 1 : %d \n", br.symbol_counts_encode[ 1 ] );
	printf("sum old : %lld \n", br.symbol_counts[ 0 ] + br.symbol_counts[ 1 ] );
	printf("sum new : %lld \n", br.symbol_counts_encode[ 0 ] + br.symbol_counts_encode[ 1 ] );
	free( memory );
	free( SymbolCountMemoryBased );
}

//DecissionCoeff -> larger then 1 means we need to be more convinced the next value is what we want
void bitreader_convert_next_block_flip_if_avg(bitreader &br, int ResetCounters, int Count0, int Count1, float DecissionCoeff )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	int can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64	BitsRead = 0;
	int					GuessesGood = 0;
	int					GuessesBad = 0;
	int					StartedToggle = 0;
	int					GuessesGoodSinceToggle = 0;
	int					GuessesBadSinceToggle = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, br.SymbolBitCount );
		int		NewSymbol = Symbol;

//		if( ( (float)(br.symbol_counts[ 1 ]) / (float)(br.symbol_counts[ 0 ]) ) * 0.997f > (float)Count1 / (float)Count0 )
		if( br.symbol_counts[ 1 ] * Count0 <= Count1 * br.symbol_counts[ 0 ] 
			|| StartedToggle == 1
			) //if the 1/0 ratio avg total is smaller then 1/0 avg so far ( we have more ones, then we expect to recover )
		{
			if( StartedToggle == 0 )
			{
				GuessesGoodSinceToggle = 0;
				GuessesBadSinceToggle = 0;
				StartedToggle = 1;
			}
			else if( GuessesGoodSinceToggle > GuessesBadSinceToggle )
				StartedToggle = 0;

			if( Symbol == 0 )
			{
				GuessesGood++;
				GuessesGoodSinceToggle++;
			}
			else
			{
				GuessesBad++;
				GuessesBadSinceToggle++;
			}

			NewSymbol = 1 - NewSymbol;
		}

		br.have_symbols++;
		BitsRead += br.SymbolBitCount;
		br.symbol_counts[ Symbol ]++;
		br.symbol_counts_encode[ NewSymbol ]++;
	}
	br.processed_count += can_process_bytes;

	printf("Good Guesses made   : %d \n", GuessesGood );
	printf("Bad Guesses made    : %d \n", GuessesBad );
	printf("Total Guesses made  : %d \n", GuessesBad + GuessesGood );
	printf("Total symbols have  : %d \n", br.have_symbols );
	printf("Failure strength %f %f \n", (float)(GuessesGood) / (float)(GuessesBad), (float)(GuessesBad) / (float)(GuessesGood) );
	printf("new count 0 : %d \n", br.symbol_counts_encode[ 0 ] );
	printf("new count 1 : %d \n", br.symbol_counts_encode[ 1 ] );
	printf("sum old : %lld \n", br.symbol_counts[ 0 ] + br.symbol_counts[ 1 ] );
	printf("sum new : %lld \n", br.symbol_counts_encode[ 0 ] + br.symbol_counts_encode[ 1 ] );
}

//DecissionCoeff -> larger then 1 means we need to be more convinced the next value is what we want
void bitreader_convert_next_block_memory_zone(bitreader &br, int ResetCounters, int WindowSlots, int SymbolAdvantageRequired )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	//the bigger the memory the better it will sort appearance chances. 0 will have biggest appear chance
	// 5x symbol count already seems to provide a decent result
	int	*memory = (int	*)malloc( sizeof( int ) * WindowSlots + 65535 );
	memset( memory, 0, sizeof( int ) * WindowSlots + 65535 );
	int MemoryWriteInd = 0;
	int *SymbolCountMemoryBased = (int	*)malloc( sizeof( int ) * br.SymbolCount );
	memset( SymbolCountMemoryBased, 0, sizeof( int ) * br.SymbolCount );

	int can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64	BitsRead = 0;
	int ZoneBitGains = 0;
	int ZoneCount = 0;
	unsigned __int64 LastZoneLoc = 0;
	unsigned __int64 MaxZoneDist = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, br.SymbolBitCount );
		int		NewSymbol = Symbol;

		if( br.have_symbols >= WindowSlots )
		{
			int	Count0Memory = SymbolCountMemoryBased[ 0 ];
			int	Count1Memory = SymbolCountMemoryBased[ 1 ];

			//a zone that could be flipped detected
			if( Count0Memory - Count1Memory > SymbolAdvantageRequired && BitsRead - LastZoneLoc > WindowSlots )
			{
				unsigned __int64 ZoneDist = BitsRead - LastZoneLoc;
				if( ZoneDist > MaxZoneDist )
					MaxZoneDist = ZoneDist;
				LastZoneLoc = BitsRead;
				ZoneCount++;
				ZoneBitGains += Count0Memory - Count1Memory;
//				printf("Decent zone detected %d gain\n",WindowSlots - Count1Memory - 24);
			}
		}

		AddSymbolToMemmory( memory, SymbolCountMemoryBased, MemoryWriteInd, Symbol, br.have_symbols < WindowSlots, WindowSlots );

		br.have_symbols++;
		BitsRead += br.SymbolBitCount;
		br.symbol_counts_encode[ NewSymbol ]++;
	}
	br.processed_count += can_process_bytes;

	printf("zones found         : %d \n", ZoneCount );
	printf("zone bit unbalances : %d \n", ZoneBitGains );
	printf("biggest dist        : %lld \n", MaxZoneDist );
	printf("dist cost           : %d \n", CountBitsRequiredStore( (int)MaxZoneDist ) * ZoneCount );
	printf("new count 0 : %d \n", br.symbol_counts[ 0 ] - ZoneBitGains );
	printf("new count 1 : %d \n", br.symbol_counts[ 1 ] + ZoneBitGains );
	printf("symbol PCT : %f \n", (float)(br.symbol_counts[ 0 ] - ZoneBitGains)/(float)(br.symbol_counts[ 1 ] + ZoneBitGains) );
	free( memory );
	free( SymbolCountMemoryBased );
}

//if there are more symbols 1 then 0 then flip bits
//create a mask for the xors
void bitreader_convert_next_block_xormask(bitreader &br, int ResetCounters, int SymbolBitCount, int ForcedStep, int RequiredOnes )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	if( br.cbitmask == 0 || br.BitmaskSize < br.BitMaskIndex + br.readcount * 8 / SymbolBitCount * (  SymbolBitCount / ForcedStep ) )
	{
		int RequiredSize = 0;
		int TSymbolBitCount = SymbolBitCount;
		while( TSymbolBitCount > 2 )
		{
			int SymbolCount = br.readcount * 8 / TSymbolBitCount;
			RequiredSize += SymbolCount + 1;
			TSymbolBitCount /= 2;
		}
		RequiredSize *= 4;
		if( RequiredSize < br.BitMaskIndex + br.readcount * 8 / SymbolBitCount * (  SymbolBitCount / ForcedStep ) )
			RequiredSize = br.BitMaskIndex + br.readcount * 8 / SymbolBitCount * (  SymbolBitCount / ForcedStep );
		br.BitmaskSize = RequiredSize;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, RequiredSize / 8  ); 
	}

	int can_process_bytes = br.readcount - br.processed_count;
	unsigned __int64	BitsRead = br.ReadStartSkipp;
	unsigned __int64	SkippsMade = 0;
	unsigned __int64	FlipsMade = 0;
	unsigned __int64	SumOldC0 = 0, SumOldC1 = 0, SumNewC0 = 0, SumNewC1 = 0;
	int SymbolReadCount = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		unsigned __int64	Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount );
		unsigned __int64	NewSymbol = Symbol;
		int					Count1 = Count1s( Symbol );
		int					Count0 = SymbolBitCount - Count1;
		SumOldC0 += Count0;
		SumOldC1 += Count1;
//		if( Count1 > RequiredOnes )
		if( Count0 > RequiredOnes )
		{
			NewSymbol = ( ~NewSymbol ) & ( ( ( unsigned __int64 )1 << SymbolBitCount ) - 1 );
			SetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount, NewSymbol );

			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 1 );
			FlipsMade++;
			SumNewC0 += Count1;
			SumNewC1 += Count0;
		}
		else
		{
			SkippsMade++;
			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 0 );
			SumNewC0 += Count0;
			SumNewC1 += Count1;
		}
		br.BitMaskIndex += 1;

		br.have_symbols++;
		BitsRead += ForcedStep;
		SymbolReadCount++;
	}
	br.processed_count += can_process_bytes;
	printf("Symbol Flips Made       : %lld\n", FlipsMade );
	printf("Symbol Skipps Made      : %lld\n", SkippsMade );
	printf("Symbol 0/1 ratio before : %f\n", (float)((double)SumOldC0/(double)SumOldC1) );
	printf("Symbol 0/1 ratio new    : %f\n", (float)((double)SumNewC0/(double)SumNewC1) );
	printf("Symbol 1-0 diff before  : %lld\n", SumOldC1 - SumOldC0 );
	printf("Symbol 1-0 diff new     : %lld\n", SumNewC1 - SumNewC0 );
	printf("Bitmask Cost	        : %d\n", SymbolReadCount );
	printf("Symbol Flip / Skipp PCT : %f\n", (float)FlipsMade / (float)SkippsMade );
	printf("Bitmask Len Now         : %lld B %lld KB %lld MB\n", br.BitMaskIndex / 8, br.BitMaskIndex / 8 / 1024, br.BitMaskIndex / 8 / 1024 / 1024 );
}

void bitreader_convert_next_block_xormask_anylen(bitreader &br, int ResetCounters, int SymbolBitCount, int ForcedStep, int RequiredOnes, int WriteBack )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	if( br.cbitmask == 0 || br.BitmaskSize < br.BitMaskIndex + br.readcount * 8 / SymbolBitCount * (  SymbolBitCount / ForcedStep ) )
	{
		int RequiredSize = 0;
		int TSymbolBitCount = SymbolBitCount;
		while( TSymbolBitCount > 2 )
		{
			int SymbolCount = br.readcount * 8 / TSymbolBitCount;
			RequiredSize += SymbolCount + 1;
			TSymbolBitCount /= 2;
		}
		RequiredSize *= 4;
		if( RequiredSize < br.BitMaskIndex + br.readcount * 8 / SymbolBitCount * (  SymbolBitCount / ForcedStep ) )
			RequiredSize = br.BitMaskIndex + br.readcount * 8 / SymbolBitCount * (  SymbolBitCount / ForcedStep );
		br.BitmaskSize = RequiredSize;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, RequiredSize / 8  ); 
	}

	int can_process_bytes = br.readcount - br.processed_count;
	unsigned __int64	BitsRead = br.ReadStartSkipp;
	unsigned __int64	SkippsMade = 0;
	unsigned __int64	FlipsMade = 0;
	unsigned __int64	SumOldC0 = 0, SumOldC1 = 0, SumNewC0 = 0, SumNewC1 = 0;
	int SymbolReadCount = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int					Count1 = 0;
		for( int ti = 0; ti < SymbolBitCount; ti++ )
		{
			unsigned __int64	Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead + ti, 1 );
			if( Symbol == 1 )
				Count1++;
		}

		int					Count0 = SymbolBitCount - Count1;
		SumOldC0 += Count0;
		SumOldC1 += Count1;
//		if( Count1 > RequiredOnes )
		if( Count0 > RequiredOnes )
		{
			if( WriteBack )
			{
				for( int ti = 0; ti < SymbolBitCount; ti++ )
				{
					unsigned __int64 Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead + ti, 1 );
					unsigned __int64 NewSymbol = ( ~Symbol ) & 1;
					SetSymbolAtLoc( br.cbuffer, BitsRead + ti, 1, NewSymbol );
				}

//				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 1 );
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 0 );
			}
			FlipsMade++;
			SumNewC0 += Count1;
			SumNewC1 += Count0;
		}
		else
		{
			SkippsMade++;
			if( WriteBack )
//				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 0 );
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 1 );
			SumNewC0 += Count0;
			SumNewC1 += Count1;
		}
		if( WriteBack )
			br.BitMaskIndex += 1;

		br.have_symbols++;
		BitsRead += ForcedStep;
		SymbolReadCount++;
	}
	br.processed_count += can_process_bytes;
	printf("Symbol Flips Made       : %lld\n", FlipsMade );
	printf("Symbol Skipps Made      : %lld\n", SkippsMade );
	printf("Symbol 0/1 ratio before : %f\n", (float)((double)SumOldC0/(double)SumOldC1) );
	printf("Symbol 0/1 ratio new    : %f\n", (float)((double)SumNewC0/(double)SumNewC1) );
	printf("Symbol 1-0 diff before  : %lld\n", SumOldC1 - SumOldC0 );
	printf("Symbol 1-0 diff new     : %lld\n", SumNewC1 - SumNewC0 );
	printf("Bitmask Cost	        : %d\n", SymbolReadCount );
	printf("gain/cost			    : %f\n", (float)((double)((SumNewC1 - SumNewC0)-(SumOldC1 - SumOldC0))/(double)SymbolReadCount) );
	printf("Symbol Flip / Skipp PCT : %f\n", (float)FlipsMade / (float)SkippsMade );
	printf("Bitmask Len Now         : %lld B %lld KB %lld MB as PCT %f\n", br.BitMaskIndex / 8, br.BitMaskIndex / 8 / 1024, br.BitMaskIndex / 8 / 1024 / 1024, ( br.BitMaskIndex + 7 ) / 8 * 100 / (float)br.readcount );
	printf("\n");
}

void bitreader_swapsymbol(bitreader &br, int SymbolBitCount, unsigned __int64 Sym1, unsigned __int64 Sym2 )
{
	int can_process_bytes = br.readcount;
	unsigned __int64	BitsRead = br.ReadStartSkipp;
	while( BitsRead < can_process_bytes * 8 )
	{
		unsigned __int64	Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount );
		if( Symbol == Sym1 )
			SetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount, Sym2 );
		else if( Symbol == Sym2 )
			SetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount, Sym1 );
		BitsRead += SymbolBitCount;
	}
	printf("Swapped symbol %lld with %lld\n\n",Sym1,Sym2);
}

void bitreader_convert_next_block_xormask_specific(bitreader &br, int ResetCounters, int SymbolBitCount, int ForcedStep, int RequiredOnes )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	if( br.cbitmask == 0 || br.BitmaskSize < br.BitMaskIndex + br.readcount * 8 / SymbolBitCount * (  SymbolBitCount / ForcedStep ) )
	{
		int RequiredSize = 0;
		int TSymbolBitCount = SymbolBitCount;
		while( TSymbolBitCount > 2 )
		{
			int SymbolCount = br.readcount * 8 / TSymbolBitCount;
			RequiredSize += SymbolCount + 1;
			TSymbolBitCount /= 2;
		}
		RequiredSize *= 4;
		if( RequiredSize < br.BitMaskIndex + br.readcount * 8 / SymbolBitCount * (  SymbolBitCount / ForcedStep ) )
			RequiredSize = br.BitMaskIndex + br.readcount * 8 / SymbolBitCount * (  SymbolBitCount / ForcedStep );
		br.BitmaskSize = RequiredSize;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, RequiredSize / 8  ); 
	}

	int can_process_bytes = br.readcount - br.processed_count;
	unsigned __int64	BitsRead = br.ReadStartSkipp;
	unsigned __int64	SkippsMade = 0;
	unsigned __int64	FlipsMade = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount );
		int		NewSymbol = Symbol;
		int		Count1 = Count1s( Symbol );
		int		Count0 = SymbolBitCount - Count1;
		if( Count1 == RequiredOnes || Count0 == RequiredOnes )
		{
			if( Count1 == RequiredOnes )
			{
				NewSymbol = ( ~NewSymbol ) & ( ( 1 << SymbolBitCount ) - 1 );
				SetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount, NewSymbol );

				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 1 );
				FlipsMade++;
			}
			else
			{
				SkippsMade++;
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 0 );
			}
			br.BitMaskIndex += 1;
		}

		BitsRead += ForcedStep;
	}
	br.processed_count += can_process_bytes;
	printf("Symbol Flips Made       : %lld\n", FlipsMade );
	printf("Symbol Skipps Made      : %lld\n", SkippsMade );
	printf("Symbol Flip / Skipp PCT : %f\n", (float)FlipsMade / (float)SkippsMade );
	printf("Bitmask Len Now         : %lld B %lld KB %lld MB\n", br.BitMaskIndex / 8, br.BitMaskIndex / 8 / 1024, br.BitMaskIndex / 8 / 1024 / 1024 );
}

void bitreader_convert_next_block_flip_if_avg_( bitreader &br )
{
	bitreader_stat_symbols( br, 1, 1 );

	int RequiredGain = 16;
//				for( ; RequiredGain <= 16; RequiredGain += 1 )
	{
		int StatisticsLength = 1024;
//					for( ; StatisticsLength <= 255; )
		{
//						printf("Using memory length %d and required gain %d\n", StatisticsLength, RequiredGain );
//						bitreader_convert_next_block_memory( br, 0, StatisticsLength, br.symbol_counts[0], br.symbol_counts[1], 0.8f );
			bitreader_convert_next_block_flip_if_avg( br, 1, br.symbol_counts[0], br.symbol_counts[1], 0.8f );
//						bitreader_convert_next_block_memory_zone( br, 0, StatisticsLength, RequiredGain );
			StatisticsLength++;
/*						if( StatisticsLength <= 255 )
			{
				br.processed_count = 0;
				memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
				br.have_symbols = 0;
			}/**/
		}
	}
}

//DecissionCoeff -> larger then 1 means we need to be more convinced the next value is what we want
void bitreader_convert_next_block_flip_if_avg_memory(bitreader &br, int ResetCounters, int WindowSlots, int NeedAdvantage, int WaitForGain = 1, int TimoutSearch = 20, int WriteBack = 0 )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	//the bigger the memory the better it will sort appearance chances. 0 will have biggest appear chance
	// 5x symbol count already seems to provide a decent result
	int	*memory = (int	*)malloc( sizeof( int ) * WindowSlots + 65535 );
	memset( memory, 0, sizeof( int ) * WindowSlots + 65535 );
	int MemoryWriteInd = 0;
	int *SymbolCountMemoryBased = (int	*)malloc( sizeof( int ) * br.SymbolCount );
	memset( SymbolCountMemoryBased, 0, sizeof( int ) * br.SymbolCount );

	int can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64	BitsRead = 0;
	int					GuessesGood = 0;
	int					GuessesBad = 0;
	int					StartedToggle = 0;
	int					GuessesGoodSinceToggle = 0;
	int					GuessesBadSinceToggle = 0;
	int					PauseUntilDipp = 0;
	int					StartedFlipAt = 0;
	int					FlipCounterCur = 0;
	int					ZonesSearched = 0;
	int					BiggestValidFlipZone = 0;
	int					SumValidFlipZone = 0, CountValidFlipZone = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, br.SymbolBitCount );
		int		NewSymbol = Symbol;

		int	Count0Memory = SymbolCountMemoryBased[ 0 ];
		int	Count1Memory = SymbolCountMemoryBased[ 1 ];

		if( Count1Memory < Count0Memory && StartedToggle == 0 )
		{
			PauseUntilDipp = 0;
			ZonesSearched++;
		}

//		if( ( (float)(br.symbol_counts[ 1 ]) / (float)(br.symbol_counts[ 0 ]) ) * 0.997f > (float)Count1 / (float)Count0 )
		if( ( Count1Memory > Count0Memory + NeedAdvantage && PauseUntilDipp == 0 )
			|| StartedToggle == 1
			) //if the 1/0 ratio avg total is smaller then 1/0 avg so far ( we have more ones, then we expect to recover )
		{
			if( StartedToggle == 0 )
			{
				GuessesGoodSinceToggle = 0;
				GuessesBadSinceToggle = 0;
				StartedToggle = 1;
//				printf("Start flipping zone at dist %d, pos %lld\n", (int)( BitsRead - StartedFlipAt ), BitsRead );
				StartedFlipAt = BitsRead;
			}
			else if( GuessesGoodSinceToggle > ( GuessesBadSinceToggle + WaitForGain ) || FlipCounterCur > TimoutSearch )
			{
				if( FlipCounterCur < TimoutSearch )
				{
					if( BiggestValidFlipZone < BitsRead - StartedFlipAt )
						BiggestValidFlipZone = BitsRead - StartedFlipAt;
					SumValidFlipZone += BitsRead - StartedFlipAt;
					CountValidFlipZone++;
				}
				//on timeout revert this change
				else
				{
					GuessesGood -= GuessesGoodSinceToggle;
					GuessesBad -= GuessesBadSinceToggle;
					for( int StepBackPos = StartedFlipAt; StepBackPos < BitsRead; StepBackPos++ )
					{
						int	Symbol = GetSymbolAtLoc( br.cbuffer, StepBackPos, br.SymbolBitCount );
						if( WriteBack )
							Symbol = 1 - Symbol;
						int NewSymbol = 1 - Symbol;
						br.symbol_counts_encode[ NewSymbol ]--;
						br.symbol_counts_encode[ Symbol ]++;
						if( WriteBack )
							SetSymbolAtLoc( br.cbuffer, StepBackPos, br.SymbolBitCount, Symbol );
					}

				}
				StartedToggle = 0;
				PauseUntilDipp = 1;
//				printf("End flipping zone size %d. Flips made %d. Score %d\n", (int)( BitsRead - StartedFlipAt ), FlipCounterCur, GuessesGoodSinceToggle - GuessesBadSinceToggle );
				StartedFlipAt = BitsRead;
			}
		
			if( StartedToggle == 1 )
			{
//				if( FlipCounterCur < 100 )	printf("flip %d\n",FlipCounterCur);
				FlipCounterCur++;

				if( Symbol == 0 )
				{
					GuessesGood++;
					GuessesGoodSinceToggle++;
				}
				else
				{
					GuessesBad++;
					GuessesBadSinceToggle++;
				}
	
				NewSymbol = 1 - NewSymbol;
			}
		}
		else
			FlipCounterCur = 0;

		AddSymbolToMemmory( memory, SymbolCountMemoryBased, MemoryWriteInd, Symbol, br.have_symbols < WindowSlots, WindowSlots );

		if( WriteBack )
			SetSymbolAtLoc( br.cbuffer, BitsRead, br.SymbolBitCount, NewSymbol );

		br.have_symbols++;
		BitsRead += br.SymbolBitCount;
		br.symbol_counts[ Symbol ]++;
		br.symbol_counts_encode[ NewSymbol ]++;
	}
	br.processed_count += can_process_bytes;
	printf("ZonesSearched       : %d \n", ZonesSearched );
	printf("Good Guesses made   : %d \n", GuessesGood );
	printf("Bad Guesses made    : %d \n", GuessesBad );
	printf("Biggest good zone size  : %d \n", BiggestValidFlipZone );
	if( CountValidFlipZone > 0 )
		printf("Avg good zone size  : %d \n", SumValidFlipZone / CountValidFlipZone );
	printf("Effective gain      : %d as pct impact %f %%\n", ( GuessesGood - GuessesBad ), (float)( GuessesGood - GuessesBad ) * 100 / (float)( can_process_bytes * 8 ) );
	printf("Total Guesses made  : %d \n", GuessesBad + GuessesGood );
	printf("Total symbols have  : %d \n", br.have_symbols );
	printf("Failure strength %f %f \n", (float)(GuessesGood) / (float)(GuessesBad), (float)(GuessesBad) / (float)(GuessesGood) );
	printf("new count 0 : %d \n", br.symbol_counts_encode[ 0 ] );
	printf("new count 1 : %d \n", br.symbol_counts_encode[ 1 ] );
	printf("sum old : %lld \n", br.symbol_counts[ 0 ] + br.symbol_counts[ 1 ] );
	printf("sum new : %lld \n", br.symbol_counts_encode[ 0 ] + br.symbol_counts_encode[ 1 ] );
	free( memory );
	free( SymbolCountMemoryBased );
}

int EffectiveGain;
int MaskCost;

//DecissionCoeff -> larger then 1 means we need to be more convinced the next value is what we want
void bitreader_convert_next_block_flip_if_avg_memory2(bitreader &br, int ResetCounters, int WindowSlots, int NeedAdvantage, int WaitForGain = 1, int TimoutSearch = 20, int WriteBack = 0 )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}


	if( br.cbitmask == 0 || br.BitmaskSize < br.BitMaskIndex + br.readcount )
	{
		br.BitmaskSize = br.BitmaskSize + br.readcount;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize  ); 
	}

	//the bigger the memory the better it will sort appearance chances. 0 will have biggest appear chance
	// 5x symbol count already seems to provide a decent result
	int	*memory = (int	*)malloc( sizeof( int ) * WindowSlots + 65535 );
	memset( memory, 0, sizeof( int ) * WindowSlots + 65535 );
	int MemoryWriteInd = 0;
	int *SymbolCountMemoryBased = (int	*)malloc( sizeof( int ) * br.SymbolCount );
	memset( SymbolCountMemoryBased, 0, sizeof( int ) * br.SymbolCount );

	int can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64	BitsRead = 0;
	int					GuessesGood = 0;
	int					GuessesBad = 0;
//	int					StartedToggle = 0;
	int					GuessesGoodSinceToggle = 0;
	int					GuessesBadSinceToggle = 0;
//	int					PauseUntilDipp = 0;
//	int					StartedFlipAt = 0;
//	int					FlipCounterCur = 0;
	int					ZonesSearched = 0;
//	int					BiggestValidFlipZone = 0;
	int					SumValidFlipZone = 0, CountValidFlipZone = 0;
	int					FlipNextN = 0;
	int					FakeDetects = 0;
	int					PauseAfterTry = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, br.SymbolBitCount );
		int		NewSymbol = Symbol;

		int	Count0Memory = SymbolCountMemoryBased[ 0 ];
		int	Count1Memory = SymbolCountMemoryBased[ 1 ];

		if( PauseAfterTry > 0 )
			PauseAfterTry--;

		if( FlipNextN == 0 && PauseAfterTry == 0 && ( Count1Memory > Count0Memory + NeedAdvantage )	) //if the 1/0 ratio avg total is smaller then 1/0 avg so far ( we have more ones, then we expect to recover )
		{
			ZonesSearched++;
			//check to see if we can flip next region
			GuessesGoodSinceToggle = 0;
			GuessesBadSinceToggle = 0;
			int FutureBitsRead = BitsRead;
			for( ; FutureBitsRead < BitsRead + TimoutSearch; FutureBitsRead++ )
			{
				int	Symbol = GetSymbolAtLoc( br.cbuffer, FutureBitsRead, br.SymbolBitCount );
				if( Symbol == 0 )
				{
					GuessesGoodSinceToggle++;
					if( GuessesGoodSinceToggle - GuessesBadSinceToggle > WaitForGain )
					{
						FutureBitsRead++;
						break;
					}
				}
				else
					GuessesBadSinceToggle++;
			}

			if( FutureBitsRead < BitsRead + TimoutSearch )
			{
				FlipNextN = FutureBitsRead - BitsRead;
				assert( FlipNextN > 0 );
				SumValidFlipZone += FutureBitsRead - BitsRead;
				CountValidFlipZone++;
				GuessesGood += GuessesGoodSinceToggle;
				GuessesBad += GuessesBadSinceToggle;
			}
			else
			{
				PauseAfterTry = WindowSlots;
				FakeDetects++;
			}

			if( WriteBack )
			{
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, FlipNextN!=0 );
				br.BitMaskIndex++;
			}

		}

		if( FlipNextN > 0 )
		{
			NewSymbol = 1 - NewSymbol;
			FlipNextN--;
		}

		AddSymbolToMemmory( memory, SymbolCountMemoryBased, MemoryWriteInd, Symbol, br.have_symbols < WindowSlots, WindowSlots );

		if( WriteBack )
			SetSymbolAtLoc( br.cbuffer, BitsRead, br.SymbolBitCount, NewSymbol );

		br.have_symbols++;
		BitsRead += br.SymbolBitCount;
		br.symbol_counts[ Symbol ]++;
		br.symbol_counts_encode[ NewSymbol ]++;
	}
	br.processed_count += can_process_bytes;
	printf("Testing windowlength=%d, advantage=%d, gain=%d, timeout=%d\n",WindowSlots,NeedAdvantage,WaitForGain,TimoutSearch );
	printf("ZonesSearched       : %d \n", ZonesSearched );
	printf("Good Guesses made   : %d \n", GuessesGood );
	printf("Bad Guesses made    : %d \n", GuessesBad );
	printf("Fake decodes	    : %d. We need to waste %d bits to get %d gains \n", FakeDetects, ZonesSearched, GuessesGood - GuessesBad );
//	printf("Biggest good zone size  : %d \n", BiggestValidFlipZone );
	if( CountValidFlipZone > 0 )
		printf("Avg good zone size  : %d \n", SumValidFlipZone / CountValidFlipZone );
	printf("Effective gain      : %d as pct impact %f %%\n", ( GuessesGood - GuessesBad ), (float)( GuessesGood - GuessesBad ) * 100 / (float)( can_process_bytes * 8 ) );
	printf("Total Guesses made  : %d \n", GuessesBad + GuessesGood );
	printf("Total symbols have  : %d \n", br.have_symbols );
	printf("Failure strength %f %f \n", (float)(GuessesGood) / (float)(GuessesBad), (float)(GuessesBad) / (float)(GuessesGood) );
	printf("old count 0 : %d \n", br.symbol_counts[ 0 ] );
	printf("old count 1 : %d \n", br.symbol_counts[ 1 ] );
	printf("new count 0 : %d \n", br.symbol_counts_encode[ 0 ] );
	printf("new count 1 : %d \n", br.symbol_counts_encode[ 1 ] );
	printf("sum old : %lld \n", br.symbol_counts[ 0 ] + br.symbol_counts[ 1 ] );
	printf("sum new : %lld \n", br.symbol_counts_encode[ 0 ] + br.symbol_counts_encode[ 1 ] );
	printf("\n");
	free( memory );
	free( SymbolCountMemoryBased );

	EffectiveGain = GuessesGood - GuessesBad;
	MaskCost = FakeDetects;
}

void bitreader_convert_next_block_flip_if_avg_memory_GetNextBestCombo( bitreader &br, int &RetWindow, int &RetAdvantage, int &RetGain, int &RetTimout )
{
	float BestScore = 0;
	for( int windowlength = 8; windowlength>=5; windowlength-- )
		for( int advantage = 0; advantage<windowlength/3; advantage++ ) //the larger the advantage, the less cases we will inspect
			for( int gain = 1; gain<4; gain += 1 )	//the larger the gain the higher the chance we will timout
				for( int timeout = 20; timeout<60; timeout += 20 )	//the larger the timeout the more both good / bad inspects we will make. Considering there are more "1" then "0", the larger the timout the more bad you make
				{
//					printf("Testing windowlength=%d, advantage=%d, gain=%d, timeout=%d\n",windowlength,advantage,gain,timeout );
					br.processed_count = 0;
					br.have_symbols = 0;
					memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
					bitreader_convert_next_block_flip_if_avg_memory2( br, 1, windowlength, advantage, gain, timeout, 0 );
//					float ResultScore = ( (float)EffectiveGain / (float)MaskCost ) * EffectiveGain; // favors quite a lot close to parity good flips / mask size
					float ResultScore = ( (float)EffectiveGain / (float)MaskCost ); //favors few but strong gains
//					float ResultScore = ( (float)EffectiveGain - (float)MaskCost * 2 ); //favors small loss and lots of zone searches
					printf("Score of this run : %f\n",ResultScore);
					if( ResultScore > BestScore )
					{
						printf("!!New record score %f for %d %d %d %d\n",ResultScore, windowlength, advantage, gain, timeout );
						BestScore = ResultScore;
						RetWindow = windowlength;
						RetAdvantage = advantage;
						RetGain = gain;
						RetTimout = timeout;
					}
				}/**/
}

//!! this will only work if there are no exceptions ( timeouts )
// if there are timouts then we need to add an extra bit for every case
void bitreader_convert_next_block_flip_if_avg_memory2_( bitreader &br )
{
	int WLen,Adv,Gain,TO;
	for( int Loop = 0; Loop < 5; Loop++ )
	{
		bitreader_convert_next_block_flip_if_avg_memory_GetNextBestCombo( br,WLen,Adv,Gain,TO );

		printf("!!Processing selected combination\n");
		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, WLen, Adv, Gain, TO, 1 );
	}
	fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	fwrite( br.cbuffer, 1, br.readcount, br.outf );
}

//!! this will only work if there are no exceptions ( timeouts )
// if there are timouts then we need to add an extra bit for every case
void bitreader_convert_next_block_flip_if_avg_memory_( bitreader &br )
{
//	bitreader_stat_symbols( br, 1, 1 );

/*	for( int windowlength = 8; windowlength<32; windowlength += 4 )
		for( int advantage = 4; advantage<windowlength-4; advantage += 2 ) //the larger the advantage, the less cases we will inspect
			for( int gain = 2; gain<advantage; gain += 1 )	//the larger the gain the higher the chance we will timout
				for( int timeout = 20; timeout<80; timeout += 20 )	//the larger the timeout the more both good / bad inspects we will make. Considering there are more "1" then "0", the larger the timout the more bad you make
				{
					printf("Testing windowlength=%d, advantage=%d, gain=%d, timeout=%d\n",windowlength,advantage,gain,timeout );
					br.processed_count = 0;
					br.have_symbols = 0;
					memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
					bitreader_convert_next_block_flip_if_avg_memory( br, 1, windowlength, advantage, gain, timeout, 0 );
				}/**/

			br.processed_count = 0;
			br.have_symbols = 0;
			memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
			//Effective gain      : 14832 as pct impact 0.541429 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 16, 8, 4, 40, 0 );
			//Effective gain      : 44906 as pct impact 1.639254 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 16, 4, 8, 80, 0 );
			//Effective gain      : 28172 as pct impact 1.028394 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 16, 4, 8, 40, 0 );
			//Effective gain      : 52524 as pct impact 1.917343 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 16, 4, 4, 40, 0 );
			//Effective gain      : 31440 as pct impact 1.147690 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 8, 4, 4, 20, 0 );
			//Effective gain      : 39760 as pct impact 1.451404 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 8, 4, 2, 20, 0 );
			//Effective gain      : 80295 as pct impact 2.931099 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 8, 2, 2, 20, 0 );
			//Effective gain      : 59738 as pct impact 2.180684 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 8, 2, 2, 10, 0 );
			//Effective gain      : 85340 as pct impact 3.115263 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 8, 2, 2, 30, 0 );
			//Effective gain      : 86595 as pct impact 3.161075 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 8, 2, 2, 40, 0 );
			//Effective gain      : 63186 as pct impact 2.306550 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 6, 2, 2, 10, 0 );
			//Effective gain      : 89858 as pct impact 3.280188 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 6, 2, 2, 40, 0 );
			//Effective gain      : 88707 as pct impact 3.238172 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 6, 2, 2, 30, 0 );
			//Effective gain      : 87780 as pct impact 3.204333 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 4, 2, 2, 40, 0 );
			//Effective gain      : 44945 as pct impact 1.640678 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 16, 4, 2, 20, 0 );
			//Effective gain      : 44079 as pct impact 1.609066 %
//			bitreader_convert_next_block_flip_if_avg_memory( br, 1, 32, 4, 4, 40, 0 );
/*	{
		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		//Effective gain      : 89858 as pct impact 3.280188 %
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, 6, 2, 2, 40, 1 );

		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		//Effective gain      : 59864 as pct impact 2.185283 %
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, 4, 2, 1, 20, 1 );

		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		//Effective gain      : 75092 as pct impact 2.741168 %
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, 3, 2, 1, 20, 1 );

		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		//Effective gain      : 41188 as pct impact 1.503532 %
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, 3, 2, 1, 10, 1 );

		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		//Effective gain      : 68532 as pct impact 2.501701 %
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, 2, 1, 1, 20, 1 );

		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		//Effective gain      : 32614 as pct impact 1.190546 %
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, 2, 1, 1, 10, 1 );

		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		//Effective gain      : 12584 as pct impact 0.459368 %
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, 2, 0, 1, 20, 1 );

		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		//Effective gain      : 14858 as pct impact 0.542378 %
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, 4, 0, 0, 20, 1 );

		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		//Effective gain      : 12792 as pct impact 0.466961 %
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, 6, 0, 0, 20, 1 );

		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		//Effective gain      : 9562 as pct impact 0.349052 %
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, 8, 0, 0, 20, 1 );

		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		//Effective gain      : 6446 as pct impact 0.235306 %
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, 10, 0, 0, 20, 1 );
		fwrite( br.cbuffer, 1, br.readcount, br.outf );
	}*/
	{
		int WLen,Adv,Gain,TO;
		bitreader_convert_next_block_flip_if_avg_memory_GetNextBestCombo( br,WLen,Adv,Gain,TO );

		br.processed_count = 0;
		br.have_symbols = 0;
		memset( br.symbol_counts_encode, 0, br.SymbolCounterSize );
		bitreader_convert_next_block_flip_if_avg_memory2( br, 1, WLen, Adv, Gain, TO, 1 );
	}
}

void bitreader_SearchBestSymbolSwap( bitreader &br, int SymbolBitCount, int &BestGain, int &BestSym1, int &BestSym2 )
{
	bitreader_stat_symbols( br, 1, 1, SymbolBitCount, 1 );
	__int64 *BestOfLen = (__int64 *)malloc( (SymbolBitCount+1) * sizeof( __int64 ) );
	__int64 *BestSym= (__int64 *)malloc( (SymbolBitCount+1) * sizeof( __int64 ) );
	memset( BestOfLen, 0, (SymbolBitCount+1) * sizeof( __int64 ) );
	memset( BestSym, 0, (SymbolBitCount+1) * sizeof( __int64 ) );
	for(int SearchingFor=0;SearchingFor<(1<<SymbolBitCount);SearchingFor++)
	{
		int Count1Needle = Count1s( SearchingFor );
		if( BestOfLen[ Count1Needle ] < br.symbol_counts[ SearchingFor ] )
		{
			BestOfLen[ Count1Needle ] = br.symbol_counts[ SearchingFor ];
			BestSym[ Count1Needle ] = SearchingFor;
		}
	}
	//at this point, for a good compression input BestOfLen[i] == BestOfLen[j]
	BestGain = 0;
	for(int HighBitCount=SymbolBitCount;HighBitCount>0;HighBitCount--)
		for(int LowBitCount=HighBitCount-1;LowBitCount>=0;LowBitCount--)
		{
			int Gain = ( HighBitCount - LowBitCount ) * ( BestOfLen[ LowBitCount ] - BestOfLen[ HighBitCount ] );
			if( Gain > BestGain )
			{
				BestSym1 = BestSym[LowBitCount];
				BestSym2 = BestSym[HighBitCount];
				BestGain = Gain;
			}
		}
	free( BestOfLen );
	free( BestSym );
}

void bitreader_SearchBestSymbolMultilenSwap( bitreader &br, int &BestLen, int &BestGain, int &BestSym1, int &BestSym2 )
{
	int g,s1,s2;
	BestGain = 0;
	for( int i=2;i<=6;i++)
	{
		bitreader_SearchBestSymbolSwap( br, i, g, s1, s2 );
		if( g > BestGain )
		{
			BestGain = g;
			BestSym1 = s1;
			BestSym2 = s2;
			BestLen = i;
		}
	}
}

void bitreader_SearchBestSymbolSwapBiggestLossExcept( bitreader &br, int SymbolBitCount, int &BestGain, int &BestSym1, int &BestSym2 )
{
	bitreader_stat_symbols( br, 1, 1, SymbolBitCount, 1 );
	__int64 *BestOfLen = (__int64 *)malloc( (SymbolBitCount+1) * sizeof( __int64 ) );
	__int64 *BestSym= (__int64 *)malloc( (SymbolBitCount+1) * sizeof( __int64 ) );
	memset( BestOfLen, 0, (SymbolBitCount+1) * sizeof( __int64 ) );
	memset( BestSym, 0, (SymbolBitCount+1) * sizeof( __int64 ) );
	for(int SearchingFor=0;SearchingFor<(1<<SymbolBitCount);SearchingFor++)
	{
		int Count1Needle = Count1s( SearchingFor );
		if( BestOfLen[ Count1Needle ] < br.symbol_counts[ SearchingFor ] )
		{
			BestOfLen[ Count1Needle ] = br.symbol_counts[ SearchingFor ];
			BestSym[ Count1Needle ] = SearchingFor;
		}
	}
	//at this point, for a good compression input BestOfLen[i] == BestOfLen[j]
	BestGain = 0;
	for(int HighBitCount=SymbolBitCount-1;HighBitCount>0;HighBitCount--)
		for(int LowBitCount=HighBitCount-1;LowBitCount>=0;LowBitCount--)
		{
			int Gain = ( HighBitCount - LowBitCount ) * ( BestOfLen[ LowBitCount ] - BestOfLen[ HighBitCount ] );
			if( Gain < BestGain )
			{
				BestSym1 = BestSym[LowBitCount];
				BestSym2 = BestSym[HighBitCount];
				BestGain = Gain;
			}
		}
	free( BestOfLen );
	free( BestSym );
}

void bitreader_SearchBestSymbolMultilenSwapBiggestLoss( bitreader &br, int &BestLen, int &BestGain, int &BestSym1, int &BestSym2 )
{
	int g,s1,s2;
	BestGain = 0;
	for( int i=2;i<=6;i++)
	{
		bitreader_SearchBestSymbolSwapBiggestLossExcept( br, i, g, s1, s2 );
		if( g < BestGain )
		{
			BestGain = g;
			BestSym1 = s1;
			BestSym2 = s2;
			BestLen = i;
		}
	}
}

void bitreader_biary_checkpredictable( bitreader &br, int writeback )
{
	bitreader_stat_symbols( br, 1, 1, 1, 0 );

	unsigned __int64	Count0sTotal = br.symbol_counts[0];
	unsigned __int64	Count1sTotal = br.symbol_counts[1];
	unsigned __int64	Count0sCur = 0;
	unsigned __int64	Count1sCur = 0;
	unsigned __int64	Count0sNew = 0;
	unsigned __int64	Count1sNew = 0;
	unsigned __int64	GuessedGood = 0, GuessedBad = 0;
	unsigned __int64	Guessed1 = 0, Guessed0 = 0;
	unsigned __int64	TurningPoints = 0;
/*	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, 0, 1 );
		if( Symbol == 1 )
			Count1sCur++;
		else
			Count0sCur++;
	}/**/
	int can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64	BitsRead = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, 1 );
		int		MySymbolGuess;

/*		{
			double OldChance1 = (double)Count1sTotal * 100 / ( Count0sTotal + Count1sTotal );
			double OldChance0 = (double)Count0sTotal * 100 / ( Count0sTotal + Count1sTotal );
			double NewChance1 = (double)Count1sCur * 100 / ( Count0sCur + Count1sCur );
			double NewChance0 = (double)Count0sCur * 100 / ( Count0sCur + Count1sCur );

			if( OldChance1 > 50 )
			{
				if( NewChance1 > OldChance1 )
					MySymbolGuess = 0;
				else
					MySymbolGuess = 1;

				if( MySymbolGuess == 1 )
					Guessed1++;
				else
					Guessed0++;

				if( Symbol == MySymbolGuess )
					GuessedGood++;
				else
					GuessedBad++;

				if( GuessedBad > GuessedGood )
					GuessedGood = GuessedGood;
			}
		}/**/

		{
//			if( Count1sCur / Count0sCur > Count1sTotal / Count0sTotal ) //right now there are more 1s then at the end, next one is probably 0
			if( Count1sCur * Count0sTotal > Count1sTotal * Count0sCur )
				MySymbolGuess = 0;
			else
				MySymbolGuess = 1;
		}/**/

		if( Count1sCur * Count0sTotal / 100 == Count1sTotal * Count0sCur / 100 )
		{
			TurningPoints++;
			printf("Turning Point at   : %lld \n", BitsRead );
		}

		int		GuessedSymbol;
		if( Symbol == MySymbolGuess )
			GuessedSymbol = 1;
		else
			GuessedSymbol = 0;

		if( Symbol == 1 )
			Count1sCur++;
		else
			Count0sCur++;

		if( GuessedSymbol == 1 )
			Count1sNew++;
		else
			Count0sNew++;

		if( writeback )
			SetSymbolAtLoc( br.cbuffer, BitsRead, 1, GuessedSymbol );

		BitsRead += 1;
	}
	br.processed_count = br.readcount;
	printf("For a well compressed file, we should guess 50%% of the time \n" );
	printf("Count old 0   : %lld \n", Count0sTotal );
	printf("Count old 1   : %lld \n", Count1sTotal );
	printf("Count new 0   : %lld \n", Count0sNew );
	printf("Count new 1   : %lld \n", Count1sNew );
	printf("Turning Points   : %lld \n", TurningPoints );
	printf("2 Count new 0   : %lld \n", GuessedBad );
	printf("2 Count new 1   : %lld \n", GuessedGood );
	printf("predicted 0 \ 1  : %f %f\n", float( Guessed0 ) / float( Guessed0 + Guessed1 ), float( Guessed1 ) / float( Guessed0 + Guessed1 ) );
	printf("\n");
}

void bitreader_convert_next_block_xormask_( bitreader &br )
{
	int Gain,Sym1,Sym2,BestLen;
//	int CurBitcount = CountBitsRequiredStore( br.readcount ) / 2;	//every symbol to appear at least twice to make sure we can flip half of the symbols
	int CurBitcount;
	//??largest size so that it's enough to parse the input only once with gain
	//smallest possible size so that gain is around 2 ( if gain is less then 2 then it's better for mask to simply extract 1 bit from symbols )
	// flip / skip should be close/above 1
	// this will eliminate symbol 0 ( of LenX ) from the dictionary for the cost of (filesize*8 / LenX)/8. Ex for Lenx=2 will create for input 11011000 = mask 0001 + 110110
	// this will eliminate symbol 0 ( of LenX ) from the dictionary for the cost of (filesize*8 / LenX)/8. Ex for Lenx=2 will create for input 11011000 = mask 0001 + 11011110
	CurBitcount = 40;	
	// 26 - 6 increased size from 69 to 77
	if( CurBitcount > br.SymbolBitCount )
	{
		printf("!Changing expected bitcount from %d to %d\n",CurBitcount,br.SymbolBitCount);
//		CurBitcount = br.SymbolBitCount;
	}
	bitreader_stat_symbols( br, 1, 1, 1 );

	// ex : 19, 9
	br.BitMaskIndex = 0;
//	while( CurBitcount >= 16 )
	for(int i=0;i<1;i++)
	{
		printf("Flipping indexes with bitlen %d\n", CurBitcount );

		br.ReadStartSkipp = 0;
		br.processed_count = 0;
		br.have_symbols = 0;
//		bitreader_convert_next_block_xormask( br, 0, CurBitcount, CurBitcount, CurBitcount / 2 );
		bitreader_convert_next_block_xormask_anylen( br, 0, CurBitcount, CurBitcount, CurBitcount / 2, 1 );
/*		{
			br.processed_count = 0;
			bitreader_stat_symbols( br, 1, 1, 2 );
			printf("\n");
			bitreader_biary_checkpredictable( br, 0 );
		}/**/
		{
			bitreader_SearchBestSymbolMultilenSwapBiggestLoss( br, BestLen, Gain, Sym1, Sym2 );
			printf("Swapping symbol len %d sym1 %d sym2 %d gain %d\n", BestLen, Sym1, Sym2, Gain  );
			bitreader_swapsymbol( br, BestLen, Sym1, Sym2 );

			CurBitcount *= 2;
			br.ReadStartSkipp = 0;
			br.processed_count = 0;
			br.have_symbols = 0;
			bitreader_convert_next_block_xormask_anylen( br, 0, CurBitcount, CurBitcount, CurBitcount / 2, 1 );
		}
		{
//			printf("\n");
//			bitreader_stat_symbols( br, 1, 1, 3 );
//			bitreader_swapsymbol( br, 3, 0, 3 );
//			printf("\n");
//			bitreader_stat_symbols( br, 1, 1, 3 );
//			CurBitcount *= 2;

//			bitreader_SearchBestSymbolMultilenSwap( br, BestLen, Gain, Sym1, Sym2 );
//			printf("Swapping symbol len %d sym1 %d sym2 %d gain %d\n", BestLen, Sym1, Sym2, Gain  );
//			bitreader_swapsymbol( br, BestLen, Sym1, Sym2 );

			bitreader_SearchBestSymbolMultilenSwapBiggestLoss( br, BestLen, Gain, Sym1, Sym2 );
			printf("Swapping symbol len %d sym1 %d sym2 %d gain %d\n", BestLen, Sym1, Sym2, Gain  );
			bitreader_swapsymbol( br, BestLen, Sym1, Sym2 );

			br.ReadStartSkipp = 0;
			br.processed_count = 0;
			br.have_symbols = 0;
			bitreader_convert_next_block_xormask_anylen( br, 0, CurBitcount, CurBitcount, CurBitcount / 2, 1 );
		}
		{
//			printf("\n");
//			bitreader_stat_symbols( br, 1, 1, 3 );
//			bitreader_swapsymbol( br, 3, 0, 2 );
//			printf("\n");
//			bitreader_stat_symbols( br, 1, 1, 3 );
//			bitreader_swapsymbol( br, 3, 0, 6 );
//			printf("\n");
//			bitreader_stat_symbols( br, 1, 1, 3 );
//			CurBitcount *= 2;

			bitreader_SearchBestSymbolMultilenSwapBiggestLoss( br, BestLen, Gain, Sym1, Sym2 );
			printf("Swapping symbol len %d sym1 %d sym2 %d gain %d\n", BestLen, Sym1, Sym2, Gain  );
			bitreader_swapsymbol( br, BestLen, Sym1, Sym2 );

			br.ReadStartSkipp = 0;
			br.processed_count = 0;
			br.have_symbols = 0;
			bitreader_convert_next_block_xormask_anylen( br, 0, CurBitcount, CurBitcount, CurBitcount / 2, 1 );
		}
	/*	{
			br.processed_count = 0;
			bitreader_stat_symbols( br, 1, 1, 2 );
			printf("\n");
			bitreader_biary_checkpredictable( br, 0 );
		}/**/
		bitreader_stat_symbols( br, 1, 1, 2 );
		printf("\n");
		bitreader_stat_symbols( br, 1, 1, 3 );
		printf("\n");
		bitreader_stat_symbols( br, 1, 1, 4 );
		printf("\n");
		bitreader_stat_symbols( br, 1, 1, 5 );
		printf("\n");
//		bitreader_stat_symbols( br, 1, 1, 6 );
//		printf("\n");

		do{
			bitreader_SearchBestSymbolMultilenSwap( br, BestLen, Gain, Sym1, Sym2 );
			if( Gain > 1000 )
			{
				printf("Swapping symbol len %d sym1 %d sym2 %d gain %d\n", BestLen, Sym1, Sym2, Gain  );
				bitreader_swapsymbol( br, BestLen, Sym1, Sym2 );
				bitreader_stat_symbols( br, 1, 1, 2 );
				printf("\n");
			}
		}while( Gain >= 100000 );
/**/
/*		CurBitcount = 39; //1.12

		br.ReadStartSkipp = 1;
		br.processed_count = 0;
		br.have_symbols = 0;
		bitreader_convert_next_block_xormask_anylen( br, 0, CurBitcount, CurBitcount + 1, CurBitcount / 2 );
		/**/
/*		do{
			bitreader_SearchBestSymbolMultilenSwap( br, BestLen, Gain, Sym1, Sym2 );
			if( Gain > 10000 )
			{
				printf("Swapping symbol len %d sym1 %d sym2 %d gain %d\n", BestLen, Sym1, Sym2, Gain  );
				bitreader_swapsymbol( br, BestLen, Sym1, Sym2 );
				bitreader_stat_symbols( br, 1, 1, 2 );
				printf("\n");
			}
		}while( Gain >= 100000 );

		CurBitcount = CurBitcount / 2;
		/**/
	}
	bitreader_stat_symbols( br, 1, 1, 1 );
	printf("\n");
//	bitreader_stat_symbols( br, 1, 1, 2 );
//	printf("\n");
//	bitreader_stat_symbols( br, 1, 1, 3 );
//	printf("\n");
//	bitreader_stat_symbols( br, 1, 1, 4 );
//	printf("\n");

	fwrite( &br.BitMaskIndex, 4, 1, br.outf );
	fwrite( br.cbitmask, 1, (br.BitMaskIndex+7)/8, br.outf );
	fwrite( br.cbuffer, 1, br.readcount, br.outf );

	br.processed_count = br.readcount;
}

void bitreader_convert_next_block_xormask_specific_( bitreader &br )
{
	int CurBitcount = CountBitsRequiredStore( br.readcount ) - 1;	//every symbol to appear at least twice to make sure we can flip half of the symbols
	// 26 - 6 increased size from 69 to 77
	if( CurBitcount > br.SymbolBitCount )
	{
		printf("!Changing expected bitcount from %d to %d\n",CurBitcount,br.SymbolBitCount);
		CurBitcount = br.SymbolBitCount;
	}
	bitreader_stat_symbols( br, 1, 1, 1 );

	// ex : 19, 9
	br.BitMaskIndex = 0;
	br.ReadStartSkipp = 0;
	for( int i=CurBitcount;i>CurBitcount/2;i-- )
	{
		printf("required bitcount in symbol %d\n", i );
		br.processed_count = 0;
		bitreader_convert_next_block_xormask_specific( br, 0, CurBitcount, CurBitcount, i );
		bitreader_stat_symbols( br, 1, 1, 1 );
	}
	fwrite( &br.BitMaskIndex, 4, 1, br.outf );
	fwrite( br.cbitmask, 1, (br.BitMaskIndex+7)/8, br.outf );
	fwrite( br.cbuffer, 1, br.readcount, br.outf );
}

//THIS IS BAD !!!
void bitreader_convert_next_block_full_inverse_chance(bitreader &br, int ResetCounters )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	int can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, br.SymbolBitCount );
		//what is the chance for this symbol
		int MySymbolAppearChance = 0;
		int MyCount = br.symbol_counts[ Symbol ]; //nicely orders symbols by appearance chance : 0 - max. If you are doing it right, you get exactly same appearance numbers for output as input just reordered
		for( int i=0;i<br.SymbolCount;i++)
			if( MyCount > br.symbol_counts[ i ] ) //if this symbol is more like to appear then our chance counter intuitively increases
				MySymbolAppearChance++;

		br.have_symbols++;
		BitsRead += br.SymbolBitCount;
		br.symbol_counts[ Symbol ]++;
		assert( MySymbolAppearChance < br.SymbolCount );
		br.symbol_counts_encode[ MySymbolAppearChance ]++;
	}
	br.processed_count += can_process_bytes;
}

//the best possible distribution
void bitreader_convert_next_block_symbol_rearrange(bitreader &br, int ResetCounters, int SymbolBitCount, int UpdateSource, int OrderType )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	int SymbolCount = 1 << SymbolBitCount;
	int can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;
	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount );
		br.have_symbols++;
		BitsRead += SymbolBitCount;
		br.symbol_counts[ Symbol ]++;
	}
	br.processed_count += can_process_bytes;

	//build symbol list based on memory. Most frequent are in back
	struct SymStore
	{
		__int64 Count;
		int Sym;
	};
	SymStore *NewSymbolList = ( SymStore *)malloc( sizeof( SymStore ) * SymbolCount );

	for( int i=0;i<SymbolCount;i++)
	{
		NewSymbolList[i].Sym = i;
		NewSymbolList[i].Count = br.symbol_counts[ i ];
	}

	//buble sort list 
	for( int i=0;i<SymbolCount-1;i++)
		for( int j=i+1;j<SymbolCount;j++)
			if( ( OrderType == 0 && NewSymbolList[i].Count < NewSymbolList[j].Count )
				|| ( OrderType == 1 && NewSymbolList[i].Count > NewSymbolList[j].Count ) )
			{
				SymStore t;
				t = NewSymbolList[j];
				NewSymbolList[j] = NewSymbolList[i];
				NewSymbolList[i] = t;
			}

	//what is the new index for this symbol
	for( int i=0;i<SymbolCount;i++)
		br.symbol_counts_encode[ NewSymbolList[i].Sym ] = br.symbol_counts[ i ];

	if( UpdateSource )
	{
		unsigned __int64 BitsRead = br.ReadStartSkipp;
		while( BitsRead < can_process_bytes * 8 )
		{
			int SymbolOld = GetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount );
			int SymbolNew = NewSymbolList[ SymbolOld ].Sym;
			SetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount, SymbolNew );
			BitsRead += SymbolBitCount;
		}
	}
	free( NewSymbolList );
}

void bitreader_convert_next_block_symbol_rearrange_(bitreader &br, int ResetCounters, int SymbolBitCount, int UpdateSource )
{
	bitreader_stat_symbols( br, 1, 1, 1 );
	printf("before first run \n" );

	br.processed_count = 0;
	br.ReadStartSkipp = 1;
	bitreader_convert_next_block_symbol_rearrange( br, 1, 2, 1, 0 );
	bitreader_stat_symbols( br, 1, 1, 1 );
	printf("Ended first run \n" );

	br.processed_count = 0;
	br.ReadStartSkipp = 0;
	bitreader_convert_next_block_symbol_rearrange( br, 1, br.SymbolBitCount - 3, 1, 1 );
	bitreader_stat_symbols( br, 1, 1, 1 );
	printf("Ended second run \n" );

	br.processed_count = 0;
	bitreader_convert_next_block_symbol_rearrange( br, 1, br.SymbolBitCount - 5, 1, 0 );
	bitreader_stat_symbols( br, 1, 1, 1 );
	printf("Ended third run \n" );

	br.processed_count = 0;
	bitreader_convert_next_block_symbol_rearrange( br, 1, br.SymbolBitCount - 7, 1, 1 );
	bitreader_stat_symbols( br, 1, 1, 1 );
	printf("Ended fourth run \n" );
}


//the best possible distribution
void bitreader_convert_next_block_replace1symbol(bitreader &br, int ResetCounters, int SymbolBitCount, int UpdateSource, int Symbol1, int Symbol2 )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	int SymbolCount = 1 << SymbolBitCount;
	int can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;
	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount );
		int NewSymbol;
		if( Symbol == Symbol1 )
			NewSymbol = Symbol2;
		else if( Symbol == Symbol2 )
			NewSymbol = Symbol1;
		else
			NewSymbol = Symbol;
		br.have_symbols++;
		br.symbol_counts[ NewSymbol ]++;
		if( UpdateSource )
			SetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount, NewSymbol );
		BitsRead += SymbolBitCount;
	}
	br.processed_count += can_process_bytes;
}

void bitreader_convert_next_block_replace1symbol_(bitreader &br)
{
	unsigned char *cbuffer = (unsigned char*)malloc( BITREADER_FILE_SEGMENT_BUFFER_SIZE + 8 );
	memcpy( cbuffer, br.cbuffer, BITREADER_FILE_SEGMENT_BUFFER_SIZE );

	bitreader_stat_symbols( br, 1, 1, 1 );

	bitreader_stat_symbols( br, 1, 1, 2 );
	printf("0 : %d\n", br.symbol_counts[0] );
	printf("1 : %d\n", br.symbol_counts[1] );
	printf("2 : %d\n", br.symbol_counts[2] );
	printf("3 : %d\n", br.symbol_counts[3] );
	printf("\n");
	for(int i=0;i<=3-1;i++)
		for(int j=i+1;j<=3;j++)
		{
			memcpy( br.cbuffer, cbuffer, BITREADER_FILE_SEGMENT_BUFFER_SIZE );

			br.processed_count = 0;
			br.ReadStartSkipp = 0;
			bitreader_convert_next_block_replace1symbol( br, 1, 2, 1, i, j );
			printf("0 : %d\n", br.symbol_counts[0] );
			printf("1 : %d\n", br.symbol_counts[1] );
			printf("2 : %d\n", br.symbol_counts[2] );
			printf("3 : %d\n", br.symbol_counts[3] );

			br.processed_count = 0;
			br.ReadStartSkipp = 1;
			bitreader_convert_next_block_replace1symbol( br, 1, 2, 1, i, j );
			printf("0 : %d\n", br.symbol_counts[0] );
			printf("1 : %d\n", br.symbol_counts[1] );
			printf("2 : %d\n", br.symbol_counts[2] );
			printf("3 : %d\n", br.symbol_counts[3] );

			printf("\n");
		}
	free( cbuffer );
	br.have_symbols = 0;
}

void bitreader_convert_next_block_memory_statistical_rearrange(bitreader &br, int ResetCounters )
{
	if( ResetCounters )
	{
		memset( br.symbol_counts, 0 , br.SymbolCounterSize );
		br.have_symbols = 0;
	}

	//the bigger the memory the better it will sort appearance chances. 0 will have biggest appear chance
	// 5x symbol count already seems to provide a decent result
#define MEMORY_WINDOW_MULT			10
//#define MEMORY_WINDOW_VECT_SIZE		( br.SymbolCount * MEMORY_WINDOW_MULT )
//#define MEMORY_WINDOW_VECT_SIZE		( br.SymbolCount / 2 )
#define MEMORY_WINDOW_VECT_SIZE		( 65535 * 2 )
	int	*memory = (int	*)malloc( sizeof( int ) * MEMORY_WINDOW_VECT_SIZE + 65535 );
	memset( memory, 0, sizeof( int ) * MEMORY_WINDOW_VECT_SIZE + 65535 );
	int MemoryWriteInd = 0;
	int *SymbolCountMemoryBased = (int	*)malloc( sizeof( int ) * br.SymbolCount );
	memset( SymbolCountMemoryBased, 0, sizeof( int ) * br.SymbolCount );

	int can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, br.SymbolBitCount );

		//build symbol list based on memory. Most frequent are in back
		struct SymStore
		{
			int Count;
			int Sym;
		};
		SymStore NewSymbolList[32];
		for( int i=0;i<br.SymbolCount;i++)
		{
			NewSymbolList[i].Sym = i;
			NewSymbolList[i].Count = SymbolCountMemoryBased[ i ];
		}
		//buble sort list 
		for( int i=0;i<br.SymbolCount;i++)
			for( int j=i+1;j<br.SymbolCount;j++)
				if( NewSymbolList[i].Count > NewSymbolList[j].Count )
				{
					SymStore t;
					t = NewSymbolList[j];
					NewSymbolList[j] = NewSymbolList[i];
					NewSymbolList[i] = t;
				}

		//what is the new index for this symbol
		int NewSymbol;
		for( int i=0;i<br.SymbolCount;i++)
			if( NewSymbolList[ i ].Sym == Symbol )
			{
				NewSymbol = i;
				break;
			}

		int MemoryWriteIndPrev = MemoryWriteInd;
		if( br.have_symbols > MEMORY_WINDOW_VECT_SIZE && SymbolCountMemoryBased[ memory[ MemoryWriteIndPrev ] ] > 0 )
			SymbolCountMemoryBased[ memory[ MemoryWriteIndPrev ] ]--;

		//memorize what we read
		MemoryWriteInd = MemoryWriteInd - 1;
		if( MemoryWriteInd < 0 )
			MemoryWriteInd = MEMORY_WINDOW_VECT_SIZE - 1;
		memory[ MemoryWriteInd ] = Symbol;

		//update our symbol appearance chances
		SymbolCountMemoryBased[ memory[ MemoryWriteInd ] ]++;
	
		br.have_symbols++;
		BitsRead += br.SymbolBitCount;
		br.symbol_counts[ Symbol ]++;
		assert( NewSymbol < br.SymbolCount );
		br.symbol_counts_encode[ NewSymbol ]++;
	}
	br.processed_count += can_process_bytes;

	free( memory );
}

void BitWriterReaderTest( bitreader &br )
{
	int Sym;
	br.cbitmask = (unsigned char *)malloc( BITREADER_FILE_SEGMENT_BUFFER_SIZE * 8 );
	for( int i=0;i<BITREADER_FILE_SEGMENT_BUFFER_SIZE * 8; i += 2 )
	{
		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 0 );
		Sym = GetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1 );
		assert( Sym == 0 );
		br.BitMaskIndex += 1;
		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 1 );
		Sym = GetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1 );
		assert( Sym == 1 );
		br.BitMaskIndex += 1;
	}
	fwrite( &br.BitMaskIndex, 4, 1, br.outf );
	fwrite( br.cbitmask, 1, (br.BitMaskIndex+7)/8, br.outf );
	br.processed_count = br.readcount;
}

void bitreader_flip_bits_range( bitreader &br, unsigned __int64 StartFlippingAt, unsigned __int64 EndFlippingAt )
{
	unsigned __int64	can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64	BitsRead = StartFlippingAt;
	while( BitsRead < can_process_bytes * 8 && BitsRead < EndFlippingAt )
	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, 1 );
		int		NewSymbol = 1 - Symbol;

		SetSymbolAtLoc( br.cbuffer, BitsRead, 1, NewSymbol );
		BitsRead += 1;
	}
	br.processed_count = br.readcount;
}

void bitreader_get_best_TurningPoint( bitreader &br, unsigned __int64 &StartFlippingAt, unsigned __int64 &EndFlippingAt, unsigned __int64 &BitFlipGain, int &ZonelenCost, int &JumpCost, int &GainCost, int ModCost, int writeback )
{
	if( br.BitmaskSize < br.readcount )
	{
		br.BitmaskSize = br.readcount;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}

	signed __int64		Count0sTotal = br.symbol_counts[0];
	signed __int64		Count1sTotal = br.symbol_counts[1];

	unsigned __int64	Count0sCur = 1;
	unsigned __int64	Count1sCur = 1;
	signed __int64		MaxDiff = 0, MinDiff = 1000000000, MinDiffPrev = 1000000000;
	unsigned __int64	MaxDiffAt = 0, MinDiffAt = 0, MinDiffPrevAt = 0;

	signed __int64		MaxGain = 0, MaxGainMinDiff, MaxGainMinDiffAt, MaxGainMaxDiff, MaxGainMaxDiffAt;

	signed __int64		LastMaxGain = 0, LastMaxGainMinDiff, LastMaxGainMinDiffAt, LastMaxGainMaxDiff, LastMaxGainMaxDiffAt, DetectedZoneCount = 0;
	signed __int64		MaxGainForCost = 0, MaxJumpForCost = 0, MaxZoneLenForCost = 0, LastPrevMaxGainMinDiffAtForCost = 0, Sum0InZones = 0, Sum1InZones = 0;
	signed __int64		MinJump = br.readcount, MinExtractedGain = ZonelenCost + JumpCost + GainCost + ModCost;
	signed __int64		SumGain = 0;

	int Needp1 = 0, Needp0 = 0;

	unsigned __int64	can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64	BitsRead = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, 1 );

		if( BitsRead > 100 )
		{
			signed __int64	CurDiff = Count1sCur - Count0sCur;
			signed __int64	TheoreticalDiffNow = BitsRead * ( Count1sTotal - Count0sTotal ) / ( Count1sTotal + Count0sTotal );
			signed __int64	CurDiffAvg = CurDiff - TheoreticalDiffNow;

			if( CurDiff > MaxDiff )
			{
				if( LastMaxGain >= MinExtractedGain )
				{
					signed __int64	ZoneLen = LastMaxGainMinDiffAt - LastMaxGainMaxDiffAt;
					signed __int64	Jump = LastMaxGainMaxDiffAt - LastPrevMaxGainMinDiffAtForCost;

					if( LastMaxGain > MaxGainForCost )
						MaxGainForCost = LastMaxGain;
					if( Jump > MaxJumpForCost )
						MaxJumpForCost = Jump;
					if( ZoneLen > MaxZoneLenForCost )
						MaxZoneLenForCost = ZoneLen;

					DetectedZoneCount++;
					Sum0InZones += ( ZoneLen - LastMaxGain ) / 2 + ( LastMaxGain );
					Sum1InZones += ( ZoneLen - LastMaxGain ) / 2;
					SumGain += LastMaxGain;

//					printf("%lld)Last Gain started at %lld ended %lld len %lld gain %lld gap %lld\n",DetectedZoneCount, LastMaxGainMaxDiffAt, LastMaxGainMinDiffAt, ZoneLen, LastMaxGain, Jump );
					if( writeback )
					{
						bitreader_flip_bits_range( br, LastMaxGainMaxDiffAt, LastMaxGainMinDiffAt );
//						SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, JumpCost, Jump );
						SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, JumpCost, ~Jump );
						br.BitMaskIndex += JumpCost;
						if( ZonelenCost > 0 )
						{
//							SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, ZonelenCost, ZoneLen );
							SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, ZonelenCost, ~ZoneLen );
							br.BitMaskIndex += ZonelenCost;
						}
						else if( GainCost > 0 )
						{
//							SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, GainCost, LastMaxGain );
							SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, GainCost, ~LastMaxGain );
							br.BitMaskIndex += ZonelenCost;
						}
					}
					LastPrevMaxGainMinDiffAtForCost = LastMaxGainMinDiffAt;
				}

				if( LastMaxGain > MaxGain )
				{
					MaxGain = LastMaxGain;
					MaxGainMinDiff = LastMaxGainMinDiff;
					MaxGainMinDiffAt = LastMaxGainMinDiffAt;
					MaxGainMaxDiff = LastMaxGainMaxDiff;
					MaxGainMaxDiffAt = LastMaxGainMaxDiffAt;
				}

				LastMaxGain = 0;
				MaxDiff = CurDiff;
				MaxDiffAt = BitsRead;
			}
			if( CurDiff < MinDiff || MaxDiffAt >= MinDiffAt )
			{
				MinDiff = CurDiff;
				MinDiffAt = BitsRead;
				signed __int64	GainAtm = MaxDiff - MinDiff;
				if( GainAtm > LastMaxGain )
				{
					LastMaxGain = GainAtm;
					LastMaxGainMinDiff = MinDiff;
					LastMaxGainMinDiffAt = MinDiffAt;
					LastMaxGainMaxDiff = MaxDiff;
					LastMaxGainMaxDiffAt = MaxDiffAt;
				}
			}/**/
		}

		if( Symbol == 1 )
			Count1sCur++;
		else
			Count0sCur++;

		BitsRead += 1;
	}
	br.processed_count = br.readcount;

	printf("Biggest jump %lld with cost %d\n", MaxJumpForCost, CountBitsRequiredStore( MaxJumpForCost ) );
	printf("Biggest zonelen %lld with cost %d\n", MaxZoneLenForCost, CountBitsRequiredStore( MaxZoneLenForCost - MinExtractedGain ) );
	printf("Biggest gain %lld with cost %d\n", MaxGainForCost, CountBitsRequiredStore( MaxGainForCost ) );
	printf("Count 0 %lld Count 1 %lld out of %lld\n", Sum0InZones, Sum1InZones, BitsRead );
	printf("Sum gain %lld in %lld zones\n", SumGain, DetectedZoneCount );
	printf("Pct gain %f of total\n", SumGain * 100.0f / (float) BitsRead );
	printf("Mask cost in bytes %lld in PCT %f\n", ( br.BitMaskIndex + 7 ) / 8, ( br.BitMaskIndex + 7 ) / 8 * 100 / (float)br.readcount );

	printf("Max diff %lld at %lld\n", MaxGainMaxDiff, MaxGainMaxDiffAt );
	printf("Min diff %lld at %lld\n", MaxGainMinDiff, MaxGainMinDiffAt );

	signed __int64 GainToMin = ( MaxGainMaxDiff - MaxGainMinDiff );
	printf("converts %lld from %lld\n", GainToMin, MaxGainMaxDiffAt );
	StartFlippingAt = MaxGainMaxDiffAt;
	EndFlippingAt = MaxGainMinDiffAt;
	BitFlipGain = GainToMin;

	JumpCost = MaxJumpForCost;
	ZonelenCost = MaxZoneLenForCost;
	GainCost = MaxGainForCost;
}


void bitreader_convert_next_block_detect_regions( bitreader &br )
{
	unsigned __int64 StartFlippingAt, EndFlippingAt, Gain;
	int ZoneLenCost = 9, JumpLenCost = 15, GainCost = 0;

	bitreader_stat_symbols( br, 1, 1, 1 );
	br.processed_count = 0;
	bitreader_get_best_TurningPoint( br, StartFlippingAt, EndFlippingAt, Gain, JumpLenCost, ZoneLenCost, GainCost, 0, 1 );

	bitreader_stat_symbols( br, 1, 1, 1 );

	fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	fwrite( br.cbuffer, 1, br.readcount, br.outf );

	printf("\n");
}

void bitreader_biary_checkpredictable_N( bitreader &br, int BitCount, int writeback )
{
	int SymbolCount = ( 1 << BitCount );
	int AllocSize = sizeof( unsigned __int64 ) * SymbolCount;
	signed __int64	*NewSymbolList = (signed __int64	*)malloc( AllocSize );
	signed __int64	*CurSymbolCount = (signed __int64	*)malloc( AllocSize );
	signed __int64	*NewSymbolCount = (signed __int64	*)malloc( AllocSize );
	signed __int64	*OldSymbolCount = (signed __int64	*)malloc( AllocSize );
	memset( CurSymbolCount, 0, AllocSize );
	memset( NewSymbolCount, 0, AllocSize );
	//init new symbol list
	for( int i=0;i<(SymbolCount);i++)
		NewSymbolList[i]=i;
	//order list so most probably have least bits
	for( int i=0;i<(SymbolCount);i++)
		for( int j=i+1;j<(SymbolCount);j++)
			if( Count1s( NewSymbolList[i] ) >= Count1s( NewSymbolList[j] ) )
			{
				unsigned __int64 t = NewSymbolList[i];
				NewSymbolList[i] = NewSymbolList[j];
				NewSymbolList[j] = t;
			}

/*	{
		bitreader_stat_symbols( br, 1, 1, 1, 1 );
		//this will generate almost perfect symbol distribution at 2 bits
//		double Ratio10 = 1 * ( (double)( br.symbol_counts[1] ) / (double)( br.symbol_counts[0] ) - 1.0 ) / ( double)BitCount / 2;
//		double Ratio01 = 1 * ( (double)( br.symbol_counts[0] ) / (double)( br.symbol_counts[1] ) - 1.0 ) / ( double)BitCount / 2;
		bitreader_stat_symbols( br, 1, 1, BitCount, 1 );
		signed __int64	AvgSymbolOccurance = br.readcount * 8 / BitCount / ( 1 << BitCount );
		signed __int64 SumOfOnes = 0;
		for( int i=0;i<(SymbolCount);i++)
		{
			SumOfOnes += BitCount;
			SumOfOnes += Count1s( i );
		}
		for( int i=0;i<(SymbolCount);i++)
		{
			signed __int64 Sym1 = Count1s( i ); //there is a 5% boost per "1"
			signed __int64 Sym0 = BitCount - Sym1;
			OldSymbolCount[i] = AvgSymbolOccurance + AvgSymbolOccurance * ( Sym1 * Ratio10 + Sym0 * Ratio01 );
		}
	}/**/
	{
		bitreader_stat_symbols( br, 1, 1, BitCount, 1 );
		for( int i=0;i<(SymbolCount);i++)
			OldSymbolCount[i] = br.symbol_counts[i];
	}/**/

	signed __int64	ProbabilitySum = 0, ProbabilitySumCount = 0;

	int can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64	BitsRead = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, BitCount );
		//to get a gain for a symbol, we should pick a new symbol that costs less. Ofc, all symbols with less cost will have higher probability

		int		OccuranceLeft = OldSymbolCount[ Symbol ] - CurSymbolCount[ Symbol ];

		int		ProbabilityOrder = 0;
		for( int i=0;i<(SymbolCount);i++)
		{
			if( i == Symbol )
				continue;
			int		OccuranceLeftThis = OldSymbolCount[ i ] - CurSymbolCount[ i ];
			if( OccuranceLeftThis < OccuranceLeft )
				ProbabilityOrder++; //small probability that we will guess this
		}

		ProbabilitySum += ProbabilityOrder;
		ProbabilitySumCount++;

		int		MySymbolGuess = NewSymbolList[ ProbabilityOrder ];
//		int		MySymbolGuess = Symbol;

		CurSymbolCount[ Symbol ]++;
		NewSymbolCount[ MySymbolGuess ]++;

		if( writeback )
			SetSymbolAtLoc( br.cbuffer, BitsRead, BitCount, MySymbolGuess );

		BitsRead += BitCount;
	}
	br.processed_count = br.readcount;

	unsigned __int64	SumOld[2],SumNew[2];
	SumOld[0]=SumOld[1]=SumNew[0]=SumNew[1]=0;

	for(int i=0;i<(SymbolCount);i++)
	{
		printf("count ");
		for(int j=BitCount-1;j>=0;j--)
			printf( "%d", (i & ( 1<<j ) ) != 0 );

		unsigned __int64 t = Count1s( i );
		SumOld[1] += t * OldSymbolCount[ i ];
		SumOld[0] += (BitCount - t) * OldSymbolCount[ i ];
		SumNew[1] += t * NewSymbolCount[ i ];
		SumNew[0] += (BitCount - t) * NewSymbolCount[ i ];
		printf("   : %lld -> %lld\n", OldSymbolCount[ i ], NewSymbolCount[ i ] );
	}

	printf( "Probability guess avg %lld out of %d\n", ProbabilitySum / ProbabilitySumCount, SymbolCount );
	printf( "count old 1 / 0 - %lld - %lld - %f\n", SumOld[0], SumOld[1], (float)SumOld[1] / (float)SumOld[0] );
	printf( "count new 1 / 0 - %lld - %lld - %f\n", SumNew[0], SumNew[1], (float)SumNew[1] / (float)SumNew[0] );

	free( NewSymbolList );
	free( CurSymbolCount );
	free( NewSymbolCount );
	free( OldSymbolCount );
}

void bitreader_convert_next_block_predictable( bitreader &br )
{
	bitreader_stat_symbols( br, 1, 1, 1, 0 );

	br.processed_count = 0;
	bitreader_biary_checkpredictable_N( br, 1, 0 );
	printf("\n");

	br.processed_count = 0;
	bitreader_biary_checkpredictable_N( br, 2, 0 );
	printf("\n");

	br.processed_count = 0;
	bitreader_biary_checkpredictable_N( br, 3, 0 );
	printf("\n");

	br.processed_count = 0;
	bitreader_biary_checkpredictable_N( br, 4, 0 );
	printf("\n"); 

	br.processed_count = 0;
	bitreader_biary_checkpredictable_N( br, 5, 0 );
	printf("\n");

	br.processed_count = 0;
	bitreader_biary_checkpredictable_N( br, 6, 0 );
	printf("\n");

	br.processed_count = 0;
	bitreader_biary_checkpredictable_N( br, 7, 0 );
	printf("\n");

	br.processed_count = 0;
	bitreader_biary_checkpredictable_N( br, 8, 0 );
	printf("\n");

	br.processed_count = 0;
	bitreader_biary_checkpredictable_N( br, 9, 0 );
	printf("\n");

	bitreader_stat_symbols( br, 1, 1, 1, 0 );
}

//6 out of 9
#define MatSize 3
//6 out of 16
//#define MatSize 4

void FillMatFromVal( int *mat, int val )
{
	for( int i=0;i<MatSize;i++ )
		for( int j=0;j<MatSize;j++ )
		{
			int tval = val & 1;
			val = val >> 1;
			mat[ i * 3 + j ] = tval;
		}
}

void FillVectFromMat( int *vect, int *mat )
{
	for( int i=0;i<MatSize;i++)
	{
		vect[0 * 2 + i] = 0;
		for(int j=0;j<MatSize;j++)
			vect[0 * 2 + i] += mat[i*MatSize+j];
	}
	for( int i=0;i<MatSize;i++)
	{
		vect[1 * 2 + i] = 0;
		for(int j=0;j<MatSize;j++)
			vect[1 * 2 + i] += mat[j*MatSize+i];
	}
}

bool CheckVectVal( int *vect, int val )
{
	int mat[MatSize*MatSize];
	int conv[2*MatSize];

	FillMatFromVal( &mat[0], val );
	FillVectFromMat( &conv[0], &mat[0] );
	bool Matches = true;
	for( int i=0;i<2*MatSize;i++)
		if( conv[i] != vect[i] )
		{
			Matches = false;
			break;
		}
	return Matches;
}

void CountAlternatives( int *vect, int val, int *MyIndex )
{
	int Matches = 0;
	for(int i=0;i<(1<<(MatSize*MatSize));i++)
	{
		if( CheckVectVal( vect, i ) )
		{
			if( i == val )
				*MyIndex = Matches;
			else
			{
//				printf("%d)Found duplicate %d to %d\n", Matches, i, val );
/*				printf("%d)Found duplicate ", Matches ); 
					PrintAsBinary( i, MatSize*MatSize );
					printf(" to " );
					PrintAsBinary( val, MatSize*MatSize );
					printf("\n" ); /**/
				Matches++;
			}
		}
	}
}

void bitreader_convert_next_block_rearrange_blockdata( bitreader &br )
{
	br.BitmaskSize = MatSize * 7 * br.readcount * 8;
	br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	br.BitMaskIndex = 0;

	//create an NxN matrix
	int mat[MatSize*MatSize];
	int conv[2*MatSize];
	int Index;
	unsigned __int64	can_process_bytes = br.readcount - br.processed_count;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64	BitsRead = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int		Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, MatSize*MatSize );
		
		FillMatFromVal( &mat[0], Symbol );
		FillVectFromMat( &conv[0], &mat[0] );
		CountAlternatives( &conv[0], Symbol, &Index );

		for( int i=0;i<6;i++)
		{
			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, MatSize, conv[i] );
			br.BitMaskIndex += MatSize;
		}
		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, MatSize, Index );
		br.BitMaskIndex += MatSize;

		BitsRead += MatSize*MatSize;
	}
	br.processed_count = br.readcount;

	fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
//	fwrite( br.cbuffer, 1, br.readcount, br.outf );
}

int bitreader_convert_next_block_corelate_data(bitreader &br, int SymbolBitCount, int Distance, int SymbolSumCount, int WriteBackBitCount )
{
	memset( br.symbol_counts, 0 , br.SymbolCounterSize );

	__int64 *symbol_counts_region = (__int64 *)malloc( (SymbolBitCount+1) * sizeof( __int64 ) );

	unsigned __int64 AllSameCount = 0, WorstSameCount = 0, AvgSameCount = 0, AvgSameCountCount = 0;
	unsigned __int64 UniqueSymbolCount = 0;
	unsigned __int64 RequiredBitsToStore = SymbolBitCount;
	int can_process_bytes = br.readcount - br.processed_count;
	unsigned __int64	BitsRead = br.ReadStartSkipp;
	while( BitsRead < can_process_bytes * 8 )
	{
		unsigned __int64	SymbolSum = 0;
		unsigned __int64	Symbol;
		int LargeCount = 0;
		int SameSymbolCount = 0;
		memset( symbol_counts_region, 0, (SymbolBitCount+1) * sizeof( __int64 ) );
		for( int i=0;i<SymbolSumCount;i++)
		{
			Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead + i * Distance, SymbolBitCount );
			SymbolSum += Symbol;

			if( Symbol >= ( 1 << ( SymbolBitCount - 1 ) ) )
				LargeCount++;

			if( symbol_counts_region[ Symbol ] != 0 )
				SameSymbolCount++;
			symbol_counts_region[ Symbol ];
		}

		if( SameSymbolCount > WorstSameCount )
			WorstSameCount = SameSymbolCount;

		AvgSameCount += SameSymbolCount;
		AvgSameCountCount++;

		unsigned __int64 RequiredBitsToStoreThis = CountBitsRequiredStore( SymbolSum );
		if( RequiredBitsToStoreThis > RequiredBitsToStore )
			RequiredBitsToStore = RequiredBitsToStoreThis;

		if( br.symbol_counts[ SymbolSum ] == 0 )
			UniqueSymbolCount++;
		br.symbol_counts[ SymbolSum ]++;

		if( WriteBackBitCount )
		{
//SetSymbolAtLoc( br.cbuffer, BitsRead, SymbolBitCount, SymbolSum % ( 1 << SymbolBitCount ) );
			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, WriteBackBitCount, SymbolSum % ( 1 << WriteBackBitCount ) );
			br.BitMaskIndex += WriteBackBitCount;
		}
	
		BitsRead += SymbolBitCount;
	}
	printf( "Required bitcount to store data : %lld instead %d\n", RequiredBitsToStore, SymbolBitCount );
	printf( "Unique symbol count %lld instead %d\n", UniqueSymbolCount, 1 << RequiredBitsToStore );
	printf( "avg case region contains same symbol %f aka %lld / %d\n", (float)AvgSameCount * 100.0f / AvgSameCountCount / SymbolSumCount, AvgSameCount / AvgSameCountCount, SymbolSumCount );
	if( RequiredBitsToStore <= 7 )
		for(int i=0;i<(1<<RequiredBitsToStore);i++)
		{
			printf("count ");
			PrintAsBinary( i, RequiredBitsToStore );
			printf("   : %lld\n", br.symbol_counts[ i ]);
		}

	free( symbol_counts_region );

	return WorstSameCount;
}

void bitreader_convert_next_block_corelate_data( bitreader &br )
{
/*	for( int symlen=2;symlen<16;symlen++)
	{
		for( int symcount=2;symcount<10;symcount++)
			for( int dist=2;dist<16;dist++)
			{
				int excpt = bitreader_convert_next_block_corelate_data( br, symlen, dist, symcount );
				printf("Exceptions required for %d %d %d = %d pct %f\n", symlen, dist, symcount, excpt, (float)excpt * 100.0f/ ( br.readcount * 8 / symlen ) );
			}
		printf("\n");
	}/**/
	//get statistics what is the best segment length to get each symbol with similar ocurance count
/*	for( int symlen=2;symlen<=2;symlen++)
	{
		for( int symcount=(1<<symlen);symcount<=30;symcount+=1)
			for( int dist=2;dist<=2;dist++)
			{
				int RepeatCount = bitreader_convert_next_block_corelate_data( br, symlen, dist, symcount, 0 );
				printf("Exceptions required for %d %d %d = %d pct %f\n", symlen, dist, symcount, RepeatCount, (float)RepeatCount * 100.0f/ symcount );
				printf("\n");
			}
		printf("\n");
	}/**/

	{
		bitreader_stat_symbols( br, 1, 1, 1, 0 );

		br.BitmaskSize = 8 * br.readcount * 8;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
		br.BitMaskIndex = 0;
		bitreader_convert_next_block_corelate_data( br, 2, 2, 13, 2 );
//		bitreader_convert_next_block_corelate_data( br, 2, 2, 4, 2 );

		bitreader_stat_symbols( br, 1, 1, 1, 0 );
		bitreader_stat_symbols( br, 1, 1, 2, 0 );
//		bitreader_stat_symbols( br, 1, 1, 7, 0 );

		fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	}

	br.processed_count += br.readcount;
}

void bitreader_convert_next_block_XOR_then_detect_regions( bitreader &br )
{
	unsigned __int64 StartFlippingAt, EndFlippingAt, Gain;
	int ZoneLenCost = 9, JumpLenCost = 15, GainCost = 0;

	bitreader_stat_symbols( br, 1, 1, 1 );

	char *InputBackup = (char*)malloc( br.readcount );
	memcpy( InputBackup, br.cbuffer, br.readcount );

	{
		{
			//356k
			br.processed_count = 0;
			bitreader_convert_next_block_xormask_anylen( br, 0, 70, 70, 70 / 2, 1 );

			br.processed_count = 0;
			JumpLenCost = 9;
			ZoneLenCost = 0;
			GainCost = 5;
			bitreader_get_best_TurningPoint( br, StartFlippingAt, EndFlippingAt, Gain, ZoneLenCost, JumpLenCost, GainCost, -JumpLenCost -ZoneLenCost -GainCost + 5, 1 );
		}

		printf("\n");
		bitreader_stat_symbols( br, 1, 1, 1, 0, 0 );
		printf("\n");
		bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
		printf("\n");
		bitreader_stat_symbols( br, 1, 1, 2, 0, 0 );
		printf("\n");
		bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
	}/**/

/*	{
		int BestSize = br.readcount * 10, BestXor, BestGainC, BestLen, BestJump,BestGain;
		for( int xorlen=20;xorlen<=70;xorlen+=5 )
			for( int gain=1;gain<=5;gain++)
			{
				printf("=======================================\n" );
				printf("Testing combo : xorlen(%d), gain(%d)\n",xorlen,gain );
				memcpy( br.cbuffer, InputBackup, br.readcount );
				br.BitMaskIndex = 0;

				br.processed_count = 0;
				bitreader_convert_next_block_xormask_anylen( br, 0, xorlen, xorlen, xorlen / 2, 1 );

				br.processed_count = 0;
				JumpLenCost = 0;
				ZoneLenCost = 0;
				GainCost = 0;
				bitreader_get_best_TurningPoint( br, StartFlippingAt, EndFlippingAt, Gain, ZoneLenCost, JumpLenCost, GainCost, gain, 0 );

				br.processed_count = 0;
				int tZoneLenCost = ZoneLenCost - gain;
				int tGainCost = GainCost - gain;
				JumpLenCost = CountBitsRequiredStore( JumpLenCost );
				ZoneLenCost = CountBitsRequiredStore( tZoneLenCost );
				GainCost = CountBitsRequiredStore( tGainCost );
				if( GainCost < ZoneLenCost )
					ZoneLenCost = 0;
				else
					GainCost = 0;
				int InLen = ZoneLenCost,InJump = JumpLenCost,InGainC = GainCost;
				bitreader_get_best_TurningPoint( br, StartFlippingAt, EndFlippingAt, Gain, ZoneLenCost, JumpLenCost, GainCost, -JumpLenCost -ZoneLenCost -GainCost + gain, 1 );

				printf("\n");
				bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
				printf("\n");
				int NewByteCountUnCompressed = br.readcount+(br.BitMaskIndex+7)/8;
				float TheoreticCompressionPCT = (float)br.symbol_counts[0]/(float)br.symbol_counts[1]; //not dead sure how close this is
				TheoreticCompressionPCT = 1.0f - ( 1.0f - TheoreticCompressionPCT ) / 2;
				int NewByteCountCompressed = NewByteCountUnCompressed * TheoreticCompressionPCT;
				printf("New file size would be %d b %d Kb %d Mb\n",(int)( NewByteCountUnCompressed ),(int)( NewByteCountUnCompressed )/1024,(int)( NewByteCountUnCompressed )/1024/1024 );
				printf("New file size maybe %d b %d Kb %d Mb\n",(int)( NewByteCountCompressed ),(int)( NewByteCountCompressed )/1024,(int)( NewByteCountCompressed )/1024/1024 );
				BestSize = GetOutArchivedSize( br );
				if( NewByteCountCompressed < BestSize )
				{
					BestSize = NewByteCountCompressed;
					BestXor = xorlen;
					BestGainC = InGainC;
					BestLen = InLen;
					BestJump = InJump;
					BestGain = gain;
				}
			}
		{
			printf("=======================================\n" );
			printf("=======================================\n" );
			printf("Testing combo : xorlen(%d), gain(%d)\n", BestXor, BestGain );
			memcpy( br.cbuffer, InputBackup, br.readcount );
			br.BitMaskIndex = 0;

			br.processed_count = 0;
			bitreader_convert_next_block_xormask_anylen( br, 0, BestXor, BestXor, BestXor / 2, 1 );

			br.processed_count = 0;
			bitreader_get_best_TurningPoint( br, StartFlippingAt, EndFlippingAt, Gain, BestLen, BestJump, BestGainC, -BestLen -BestJump -BestGainC + BestGain, 1 );
		}
	}/**/

	fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	fwrite( br.cbuffer, 1, br.readcount, br.outf );

	printf("\n");

	free( InputBackup );
	br.have_symbols = 0;
}

unsigned __int64 GetSymbolAtLocStart1Larger( unsigned char *cbuffer, unsigned __int64 StartLoc, int &MinLen, signed __int64 MinVal )
{
	unsigned __int64 i;
	for( i=MinLen;i<64;i++ )
	{
		unsigned __int64	Symbol = GetSymbolAtLoc( cbuffer, StartLoc, i );
		if( ( 1 << ( i - 1 ) ) & Symbol )
		{
			if( Symbol > MinVal )
			{
				MinLen = i;
				return Symbol;
			}
		}
	}
	MinLen = 65;
	return 0;
}

unsigned __int64 GetSymbolAtLocStart0Larger( unsigned char *cbuffer, unsigned __int64 StartLoc, int &MinLen, signed __int64 MinVal )
{
	unsigned __int64 i;
	for( i=MinLen;i<64;i++ )
	{
		unsigned __int64	Symbol = GetSymbolAtLoc( cbuffer, StartLoc, i );
		if( ( ( 1 << ( i - 1 ) ) & Symbol ) == 0 )
		{
			if( Symbol > MinVal )
			{
				MinLen = i;
				return Symbol;
			}
		}
	}
	MinLen = 65;
	return 0;
}

void bitreader_convert_next_block_substract_next( bitreader &br )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}

	int can_process_bytes = br.readcount - br.processed_count;
	unsigned __int64	BitsRead = br.ReadStartSkipp;
	unsigned __int64	Symbols[5000];
	unsigned __int64	SymbolsDiff[5000];
	int					SymbolsLen[5000];
	int					LocationsExtracted = 0;
	int					LocationsSkipped = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int SkipInitialN = 0;
ReadNextSeq:
		int SymbolLenThisRun = 1;
		int SymbolsExtractedThisRun = 1;
		int SymbolBitCountSumThisRun = 0;
		Symbols[0] = 0;
		SymbolsLen[0] = 1;
		while( SymbolsLen[ SymbolsExtractedThisRun - 1 ] < 64 
			&& SymbolsExtractedThisRun < 255 
			&& ( BitsRead + SymbolBitCountSumThisRun + SkipInitialN < can_process_bytes * 8 )
			)
		{
			SymbolsLen[ SymbolsExtractedThisRun ] = SymbolsLen[ SymbolsExtractedThisRun - 1 ];
			Symbols[ SymbolsExtractedThisRun ] = GetSymbolAtLocStart1Larger( br.cbuffer, BitsRead + SymbolBitCountSumThisRun + SkipInitialN, SymbolsLen[ SymbolsExtractedThisRun ], Symbols[ SymbolsExtractedThisRun - 1 ] );
			SymbolBitCountSumThisRun += SymbolsLen[ SymbolsExtractedThisRun ];
			SymbolsExtractedThisRun++;
		}
		//last one was tricky
		SymbolsExtractedThisRun--;

		float StoreIncreasePerStep[5000];
		float BiggestIncrease = 0;
		for( int i=1;i<SymbolsExtractedThisRun;i++)
		{
			SymbolsDiff[i] = Symbols[i] - Symbols[i-1];
//			StoreIncreasePerStep[i] = SymbolsLen[i] / (float)i;
			StoreIncreasePerStep[i] = CountBitsRequiredStore( SymbolsDiff[i] ) / (float)i;
			if( StoreIncreasePerStep[i] > BiggestIncrease )
				BiggestIncrease = StoreIncreasePerStep[i];
		}
		int IncreasePerStep = (int)( BiggestIncrease + 0.999 ); //max could be 32

		//bad spot to extract
/*		if( SymbolsExtractedThisRun < 4 || IncreasePerStep > 3 )
		{
//			assert( false );
			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 0 );
			br.BitMaskIndex += 1;
			for( int i=0;i<64;i++)
			{
				int Sym = GetSymbolAtLoc( br.cbuffer, BitsRead + i, 1 );
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, Sym );
				br.BitMaskIndex += 1;
			}
			BitsRead += 64;
			LocationsSkipped++;
			continue;
		}/**/

		{
			if( SymbolsExtractedThisRun < 4 || IncreasePerStep - 1 > 2 )
			{
				SkipInitialN++;
				if( BitsRead + SymbolBitCountSumThisRun + SkipInitialN < can_process_bytes * 8 )
					goto ReadNextSeq;
			}
			//number of data we will need to read
			if( SkipInitialN == 0 )
			{
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 0 );
				br.BitMaskIndex += 1;
			}
			else
			{
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 1 );
				br.BitMaskIndex += 1;
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 9, SkipInitialN );
				br.BitMaskIndex += 9;
				for( int i=0;i<SkipInitialN;i++)
				{
					int Sym = GetSymbolAtLoc( br.cbuffer, BitsRead + i, 1 );
					SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, Sym );
					br.BitMaskIndex += 1;
				}
				LocationsSkipped++;
			}
		}/**/

		//we have data to process
//		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 1 );
//		br.BitMaskIndex += 1;
		//we will store data like this
		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 2, IncreasePerStep - 1 );
		br.BitMaskIndex += 2;
		//number of data we will need to read
//		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 6, SymbolsExtractedThisRun - 1 );
//		br.BitMaskIndex += 8;

		int BitCountSumWrittenFromSource = 0;
		for( int i=1;i<SymbolsExtractedThisRun;i++)
		{
			int LenNow = i * IncreasePerStep;

			if( LenNow >= 64 )
				break;

			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, LenNow, SymbolsDiff[i] );
			br.BitMaskIndex += LenNow;

			BitCountSumWrittenFromSource += SymbolsLen[i];

			if( CountBitsRequiredStore( SymbolsDiff[i] ) > LenNow )
				printf("Rounding error in estimating how much storeage space we need %d / %d %d / %d !\n", i, SymbolsExtractedThisRun, CountBitsRequiredStore( SymbolsDiff[i] ), LenNow );
		}
		LocationsExtracted++;
	
		BitsRead += SkipInitialN;
		BitsRead += BitCountSumWrittenFromSource;
	}
	printf("Locations changed : %d\n", LocationsExtracted );
	printf("Locations skipped : %d\n", LocationsSkipped );
}

unsigned __int64 GetSymbolAtLocStart11Larger( bitreader &br, unsigned __int64 StartLoc, int &MinLen, unsigned __int64 MinVal )
{
	unsigned __int64 i;
	for( i=MinLen+2;i<64;i++ )
	{
		unsigned __int64	Symbol = GetSymbolAtLoc( br.cbuffer, StartLoc, i );
		unsigned __int64	SymStartMask = 0x02;
		if( ( ( SymStartMask << ( i - 2 ) ) & Symbol ) == ( SymStartMask << ( i - 2 ) ) )
		{
			MinLen = i;
			return Symbol;
		}
	}
	MinLen = 65;
	return 0;
}

void bitreader_convert_next_block_substract_next2( bitreader &br )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}

	{
		printf("Sampling all input possibilities when A has 4 bits and B has 3 bits\n");
		for( int a=0;a<(1<<2);a++)
			for( int b=0;b<=1;b++)
			{
				int n1 = ( 2 << 2 ) | a;
				int n2 = ( 2 << 1 ) | b;
				int d = n1 - n2;
				PrintAsBinary( d, 4 );
				printf(" <= ");
				PrintAsBinary( n1, 4 );
				printf(" - ");
				PrintAsBinary( n2, 3 );
				printf(" == %d - %d \n", n1, n2 );
			}
		printf("\n");

		printf("Sampling all input possibilities when A has 5 bits and B has 3 bits\n");
		for( int a=0;a<(1<<3);a++)
			for( int b=0;b<=1;b++)
			{
				int n1 = ( 2 << 3 ) | a;
				int n2 = ( 2 << 1 ) | b;
				int d = n1 - n2;
				PrintAsBinary( d, 5 );
				printf(" i ");
				PrintAsBinary( d >> 3, 3 );
				printf(" <= ");
				PrintAsBinary( n1, 5 );
				printf(" - ");
				PrintAsBinary( n2, 3 );
				printf(" == %d - %d \n", n1, n2 );
			}
//		printf("If 4th bit is 0 read 1 more and consider it a number\n");

		printf("Sampling all input possibilities when A has 6 bits and B has 3 bits\n");
		for( int a=0;a<(1<<3);a++)
			for( int b=0;b<=1;b++)
				for( int c=0;c<=1;c++)
				{
					int n1 = ( 2 << 4 ) | ( c << 3 ) | a;
					int n2 = ( 2 << 1 ) | b;
					int d = n1 - n2;
					PrintAsBinary( d, 6 );
					printf(" i ");
					PrintAsBinary( d >> 3, 3 );
					printf(" <= ");
					PrintAsBinary( n1, 6 );
					printf(" - ");
					PrintAsBinary( n2, 3 );
					printf(" == %d - %d \n", n1, n2 );
				}

		printf("Sampling all input possibilities when A has 6 bits and B has 3 bits\n");
		for( int a=0;a<(1<<3);a++)
			for( int b=0;b<=1;b++)
				for( int c=0;c<=1;c++)
				{
					int n1 = ( 2 << 5 ) | ( c << 4 ) | ( c << 3 ) | a;
					int n2 = ( 2 << 1 ) | b;
					int d = n1 - n2;
					PrintAsBinary( d, 7 );
					printf(" i ");
					PrintAsBinary( d >> 3, 4 );
					printf(" <= ");
					PrintAsBinary( n1, 7 );
					printf(" - ");
					PrintAsBinary( n2, 3 );
					printf(" == %d - %d \n", n1, n2 );
				}

		printf("Sampling all input possibilities when A has 6 bits and B has 3 bits\n");
		for( int a=0;a<(1<<3);a++)
			for( int b=0;b<=1;b++)
				for( int c=0;c<=1;c++)
				{
					int n1 = ( 7 << 5 ) | ( c << 4 ) | ( c << 3 ) | a;
					int n2 = ( 2 << 1 ) | b;
					int d = n1 - n2;
					PrintAsBinary( d, 8 );
					printf(" i ");
					PrintAsBinary( d >> 3, 4 );
					printf(" <= ");
					PrintAsBinary( n1, 8 );
					printf(" - ");
					PrintAsBinary( n2, 3 );
					printf(" == %d - %d \n", n1, n2 );
				}
	}
return;

	int can_process_bytes = br.readcount - br.processed_count;
	unsigned __int64	BitsRead = br.ReadStartSkipp;
	unsigned __int64	Symbols[5000];
	unsigned __int64	SymbolsDiff[5000];
	int					SymbolsLen[5000];
	int					LocationsExtracted = 0;
	int					LocationsSkipped = 0;
	int					BitsSkipped = 0;
	Symbols[0] = 0;
	SymbolsLen[0] = 1;
	while( BitsRead < can_process_bytes * 8 )
	{
		int SkipInitialN = 0;
ReadNextSeq:
		int SymbolLenThisRun = 1;
		int SymbolsExtractedThisRun = 1;
		int SymbolBitCountSumThisRun = 0;
		while( SymbolsLen[ SymbolsExtractedThisRun - 1 ] < 64 
			&& SymbolsExtractedThisRun < 255 
			&& ( BitsRead + SymbolBitCountSumThisRun + SkipInitialN < can_process_bytes * 8 )
			)
		{
			SymbolsLen[ SymbolsExtractedThisRun ] = SymbolsLen[ SymbolsExtractedThisRun - 1 ];
			Symbols[ SymbolsExtractedThisRun ] = GetSymbolAtLocStart11Larger( br, BitsRead + SymbolBitCountSumThisRun + SkipInitialN, SymbolsLen[ SymbolsExtractedThisRun ], Symbols[ SymbolsExtractedThisRun - 1 ] );
			SymbolBitCountSumThisRun += SymbolsLen[ SymbolsExtractedThisRun ];
			SymbolsExtractedThisRun++;
		}
		//last one was tricky
		SymbolsExtractedThisRun--;

		assert( SymbolsExtractedThisRun < 32 );

		{
			if( SymbolsExtractedThisRun < 2 )
			{
				SkipInitialN++;
				if( BitsRead + SymbolBitCountSumThisRun + SkipInitialN < can_process_bytes * 8 )
					goto ReadNextSeq;
			}
			//number of data we will need to read
			if( SkipInitialN == 0 )
			{
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 0 );
				br.BitMaskIndex += 1;
			}
			else
			{
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 1 );
				br.BitMaskIndex += 1;
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 9, SkipInitialN );
				br.BitMaskIndex += 9;
				for( int i=0;i<SkipInitialN;i++)
				{
					int Sym = GetSymbolAtLoc( br.cbuffer, BitsRead + i, 1 );
					SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, Sym );
					br.BitMaskIndex += 1;
				}
				LocationsSkipped++;
			}
		}/**/

		for( int i=1;i<SymbolsExtractedThisRun;i++)
		{
			SymbolsDiff[i] = Symbols[i] - Symbols[i-1];
			assert( SymbolsLen[ i ] >= SymbolsLen[ i - 1 ] );
			assert( SymbolsLen[ i ] < 64 );
		}

		int BitCountSumWrittenFromSource = 0;
		for( int i=1;i<SymbolsExtractedThisRun;i++)
		{
			int LenNow = CountBitsRequiredStore( SymbolsDiff[i] );
			assert( LenNow <= SymbolsLen[i] );
			BitsSkipped += SymbolsLen[i] - LenNow;

			if( LenNow >= 64 )
				break;

			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, LenNow, SymbolsDiff[i] );
			br.BitMaskIndex += LenNow;

			BitCountSumWrittenFromSource += SymbolsLen[i];
		}
		LocationsExtracted++;
	
		BitsRead += SkipInitialN;
		BitsRead += BitCountSumWrittenFromSource;
	}
	printf("Locations changed : %d\n", LocationsExtracted );
	printf("Locations skipped : %d\n", LocationsSkipped );
	printf("Bits skipped : %d\n", BitsSkipped );
	printf("Bits added : %d\n", LocationsExtracted + 9 * LocationsSkipped );
}

void bitreader_convert_next_block_substract_next3( bitreader &br )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}

	//A is 10..
	//B is larger then A
	//B first bit is 0
	//B : 01, 011, 010, 0111,
	//B - A : 00, 010, 001, 0110
	signed __int64		ExceptionCount = 0;
	signed __int64		PossibleBitGain = 0;
	unsigned __int64	can_process_bits = ( br.readcount - br.processed_count ) * 8;
	unsigned __int64	BitsRead = br.ReadStartSkipp;
	unsigned __int64	BitsReadReader = br.ReadStartSkipp;
	while( BitsRead < can_process_bits )
	{

		int MinLenA = 1;
		unsigned __int64 A = GetSymbolAtLocStart1Larger( br.cbuffer, BitsRead, MinLenA, 0 );
		if( MinLenA > 64 )
		{
			BitsRead += 64;
			ExceptionCount++;
			continue;
		}

		int MinLenB = MinLenA;
		unsigned __int64 B = GetSymbolAtLocStart1Larger( br.cbuffer, BitsRead + MinLenA, MinLenB, A );
		if( MinLenB > 64 )
		{
			BitsRead += 64;
			ExceptionCount++;
			continue;
		}

		unsigned __int64 Diff = B - A;
		int DiffSize = CountBitsRequiredStore( Diff );
		int WriteBSize = DiffSize + 1;
		if( WriteBSize < MinLenA + 1)
			WriteBSize = MinLenA + 1;
		assert( MinLenB >= DiffSize );
		PossibleBitGain += MinLenB - WriteBSize;

		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, MinLenA, A );
		br.BitMaskIndex += MinLenA;
		//on decode we will read bits only until A+X >= A
		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, WriteBSize, Diff );
		br.BitMaskIndex += WriteBSize;
	
		//test reader
		{
			int R_MinLenA = 1;
			unsigned __int64 R_A = GetSymbolAtLocStart1Larger( br.cbitmask, BitsReadReader, R_MinLenA, 0 );

			//read bits until A + X > A and next bit is 0
			int R_MinLenB = R_MinLenA - 1;
			unsigned __int64 R_B;
			unsigned __int64 NextBit;
			unsigned __int64 R_MinValB = 1 << ( R_MinLenA - 1 );
			do {
				R_MinLenB += 1;
				R_B = GetSymbolAtLoc( br.cbitmask, BitsReadReader + R_MinLenA, R_MinLenB );
				NextBit = GetSymbolAtLoc( br.cbitmask, BitsReadReader + R_MinLenA + R_MinLenB, 1 );
				R_B += R_A;
			}while( R_B <= R_A || NextBit != 0 || R_B <= R_MinValB );

			assert( A == R_A );
			assert( B == R_B );
			BitsReadReader += R_MinLenA;
			BitsReadReader += R_MinLenB + 1;
		}/**/
		BitsRead += MinLenA;
		BitsRead += MinLenB;
	}
	printf("Exception count : %lld\n", ExceptionCount );
	printf("Maybe win bitcount : %lld\n", PossibleBitGain );
}


void bitreader_convert_next_block_xormask_substract( bitreader &br )
{
	bitreader_stat_symbols( br, 1, 1, 1 );

	int CurBitcount;

	{
		CurBitcount = 20;
		br.ReadStartSkipp = 0;
		br.processed_count = 0;
		br.have_symbols = 0;
//		bitreader_convert_next_block_xormask_anylen( br, 0, CurBitcount, CurBitcount, CurBitcount / 2, 1 );

//		br.processed_count = 0;
		bitreader_convert_next_block_substract_next( br );

		br.processed_count = 0;
//		bitreader_convert_next_block_substract_next3( br );

		fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	}

	bitreader_stat_symbols( br, 1, 1, 1, 0, 2 );
	bitreader_stat_symbols( br, 1, 1, 2, 0, 2 );

	br.have_symbols = 0;
	br.processed_count = br.readcount;
}

void bitreader_convert_next_block_mask_remove( bitreader &br )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}

	for( int i=0;i<16;i++)
	{
		PrintAsBinary( i, 4 );
		printf("\n");
	}
	br.processed_count =  br.readcount;
	return;
}

void bitreader_convert_next_block_random_flips_( bitreader &br, int *flipindexes, int indexlen )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}

	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 index = 0;
	unsigned __int64 BitsRead = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, 1 );
		SetSymbolAtLoc( br.cbuffer, BitsRead, 1, 1 - Symbol );
		BitsRead += flipindexes[ index ];
		index++;
		if( index >= indexlen )
			index = 0;
	}
}

void bitreader_convert_next_block_random_flips( bitreader &br )
{
	int BestCombo[50];
	int Best1Count = 0, Best1CountT;
	int	StackData[50];
	int StackDepth = 0;
	char *InputBackup = (char*)malloc( br.readcount );
	memcpy( InputBackup, br.cbuffer, br.readcount );

	bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
	Best1CountT = Best1Count = br.symbol_counts[1];

#define STEP_MIN_VAL	1
#define STEP_LIMIT		7
#define STACK_LIMIT		4

	for( int i=0;i<=STACK_LIMIT;i++)
		StackData[ i ] = STEP_MIN_VAL;
	while( StackData[STACK_LIMIT] == STEP_MIN_VAL )
	{
		for( int i=0;i<STACK_LIMIT;i++)
			printf( "%d ", StackData[ i ] );
		printf("\n");

		memcpy( br.cbuffer, InputBackup, br.readcount );
		br.BitMaskIndex = 0;
		br.processed_count = 0;
		bitreader_convert_next_block_random_flips_( br, StackData, STACK_LIMIT );
		printf("\n");
		bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );

		if( br.symbol_counts[1] > Best1Count )
		{
			printf("New Best COMBO %d !\n", br.symbol_counts[1] - Best1CountT );
			Best1Count = br.symbol_counts[1];
			memcpy( BestCombo, StackData, sizeof( BestCombo ) );
		}

		//next combo
		StackData[0]++;
		for( int i=0;i<STACK_LIMIT;i++)
			if( StackData[i] >= STEP_LIMIT )
			{
				StackData[ i + 1 ]++;
				StackData[ i ] = STEP_MIN_VAL;
			}
	}/**/
/*
	int FlipMask[50];
	for( int a=1;a<STEP_LIMIT;a++)
	{
		FlipMask[0] = a;
		for( int b=a+1;b<STEP_LIMIT;b++)
		{
			FlipMask[1] = b;
			for( int c=b+1;c<STEP_LIMIT;c++)
			{
				FlipMask[2] = c;
				FlipMask[3] = 0;
				
				memcpy( br.cbuffer, InputBackup, br.readcount );
				br.BitMaskIndex = 0;
				br.processed_count = 0;
				bitreader_convert_next_block_random_flips_( br, FlipMask );
				printf("\n");
				bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
			}
		}
	}
/**/
	br.processed_count =  br.readcount;
	free( InputBackup );
	return;
}

void bitreader_convert_next_block_remove_( bitreader &br, unsigned __int64 WhatSym, int SymSize, unsigned __int64 Step, int WriteBack )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
		br.BitmaskSize2 = br.readcount * 20;
		br.cbitmask2 = (unsigned char*)realloc( br.cbitmask2, br.BitmaskSize2 ); 
		br.BitmaskSize3 = br.readcount * 20;
		br.cbitmask3 = (unsigned char*)realloc( br.cbitmask3, br.BitmaskSize3 ); 
	}

	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 FoundSym = 0, MissedSym = 0;
	unsigned __int64 BitsRead = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );
		if( Symbol == WhatSym )
		{
			if( WriteBack )
			{
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 0 );
				br.BitMaskIndex += 1;
			}
			BitsRead += SymSize;
			FoundSym++;
		}
		else
		{
			if( WriteBack )
			{
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 1 );
				br.BitMaskIndex += 1;

				int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, Step );
				SetSymbolAtLoc( br.cbitmask2, br.BitMaskIndex2, Step, Symbol );
				br.BitMaskIndex2 += Step;
			}
			BitsRead += Step;
			MissedSym++;
		}
	}
	printf("Mask Size         : %lld\n", FoundSym + MissedSym );
	printf("Symbols extracted : %lld\n", FoundSym );
	printf("Theoretical count : %d = %d\n", br.readcount / ( 1 << SymSize ), br.symbol_counts[0] );
	printf("Mask 1 PCT        : %f\n", (float)FoundSym * 100 / ( FoundSym + MissedSym ) );
	printf("Size reduction    : %lld\n", FoundSym * SymSize );
	int NewFileSize = ( FoundSym + MissedSym - FoundSym * SymSize ) / 8 + br.readcount;
	printf("New filesize      : %d %f\n", NewFileSize, (float)NewFileSize / br.readcount );
}
/*
void bitreader_convert_next_block_put_together( bitreader &br, unsigned __int64 WhatSym, int SymSize, unsigned __int64 Step, int WriteBack )
{
	br.BitMaskIndex3 = 0;
	unsigned __int64 BitsRead1 = 0;
	unsigned __int64 BitsRead2 = 0;
	while( BitsRead1 < br.BitMaskIndex )
	{
		int Symbol = GetSymbolAtLoc( br.cbitmask, BitsRead1, 1 );
		BitsRead1 += 1;
		if( Symbol == 1 )
		{
			int Symbol = GetSymbolAtLoc( br.cbitmask2, BitsRead2, Step );
			SetSymbolAtLoc( br.cbitmask3, br.BitMaskIndex3, Step, Symbol );
			br.BitMaskIndex3 += Step;
			BitsRead2 += Step;
		}
		else
		{
			int Symbol = WhatSym;
			SetSymbolAtLoc( br.cbitmask3, br.BitMaskIndex3, SymSize, Symbol );
			br.BitMaskIndex3 += SymSize;
		}
	}
}*/

void bitreader_convert_next_block_remove( bitreader &br )
{
	char *InputBackup = (char*)malloc( br.readcount );
	memcpy( InputBackup, br.cbuffer, br.readcount );

	br.BitMaskIndex = br.BitMaskIndex2 = 0;

	bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );

/*	{
		bitreader_stat_symbols( br, 1, 1, 4, 0, 1 );
//		bitreader_convert_next_block_remove_( br, 0, 4, 1, 1 ); //78% increase. Rar from 334 to 369
//		bitreader_convert_next_block_remove_( br, 0, 4, 2, 1 ); //38% increase. Rar from 334 to 353
//		bitreader_convert_next_block_remove_( br, 0, 4, 3, 1 ); //25% increase. Rar from 334 to 351
//		bitreader_convert_next_block_remove_( br, 0, 4, 4, 1 ); //19% increase. Rar from 334 to 344
//		bitreader_convert_next_block_remove_( br, 0, 4, 5, 1 ); //15% increase. Rar from 334 to 346
//		bitreader_convert_next_block_remove_( br, 0, 4, 6, 1 ); //13% increase. Rar from 334 to 344
//		bitreader_convert_next_block_remove_( br, 0, 4, 7, 1 ); //11% increase. Rar from 334 to 344
		bitreader_convert_next_block_remove_( br, 0, 4, 8, 1 ); //09% increase. Rar from 334 to 340
	}/**/

/*	{
		bitreader_stat_symbols( br, 1, 1, 8, 0, 1 );
//		bitreader_convert_next_block_remove_( br, 0, 8, 1, 1 ); //96% size increase. Rar from 334 to 341
		bitreader_convert_next_block_remove_( br, 0, 8, 4, 1 ); //24% increase. Rar from 334 to 337
//		bitreader_convert_next_block_remove_( br, 0, 8, 8, 1 ); //12% increase. Rar from 334 to 336
//		bitreader_convert_next_block_remove_( br, 0, 8, 12, 1 ); //08% increase. Rar from 334 to 336
	}/**/

	{
		bitreader_stat_symbols( br, 1, 1, 8, 0, 1 );
		bitreader_convert_next_block_remove_( br, 0, 8, 4, 1 ); //24% increase

		printf("\n");
		bitreader_stat_symbols( br, 1, 1, 4, 0, 1 );
		memcpy( br.cbuffer, br.cbitmask2, ( br.BitMaskIndex2 + 7 ) / 8 );
		br.readcount = ( br.BitMaskIndex2 + 7 ) / 8;
		bitreader_convert_next_block_remove_( br, 0, 4, 2, 1 ); //39% increase. Rar from 334 to 620
	}/**/

	fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	fwrite( br.cbitmask2, 1, ( br.BitMaskIndex2 + 7 ) / 8, br.outf );

	br.processed_count =  br.readcount;
	free( InputBackup );
	return;
}

void bitreader_convert_next_block_subnext_( bitreader &br, int SymSize1, int gap, int SymSize2 )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}

	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = 0;
	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol1 = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize1 );
		int Symbol2 = GetSymbolAtLoc( br.cbuffer, BitsRead + gap, SymSize2 );
//		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, SymSize1, Symbol2 - Symbol1 );
		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, SymSize1, Symbol2 ^ Symbol1 );
		br.BitMaskIndex += SymSize1;
		BitsRead += SymSize1;
	}
}

void bitreader_convert_next_block_subnext( bitreader &br )
{
	char *InputBackup = (char*)malloc( br.readcount );
	memcpy( InputBackup, br.cbuffer, br.readcount );

	br.BitMaskIndex = br.BitMaskIndex2 = 0;

/*	for( int j=1;j<3;j++)
	{
		for( int i=0;i<4;i++)
		{
			br.BitMaskIndex = 0;
			bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
			bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
			bitreader_convert_next_block_subnext_( br, i+1, i+1+j );
			memcpy( br.cbuffer, br.cbitmask, br.readcount );
			printf("\n");
		}
	}/**/
	for( int j=1;j<=8;j++)
		for( int i=4;i<=4;i+=4)
		{
			br.BitMaskIndex = 0;
			bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
			bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
			bitreader_convert_next_block_subnext_( br, i+1, (i+1)*j, i+1 );
			memcpy( br.cbuffer, br.cbitmask, br.readcount );
			printf("\n");
		}/**/

	bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
	bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
	bitreader_stat_symbols( br, 1, 1, 3, 0, 1 );
	bitreader_stat_symbols( br, 1, 1, 4, 0, 1 );
	fwrite( br.cbuffer, 1, br.readcount, br.outf );

	br.processed_count =  br.readcount;
	free( InputBackup );
	return;
}

void bitreader_convert_next_block_xor_some_( bitreader &br, int Sym, int SymSize )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}

	int InverseSym = (~Sym) & (( 1 << SymSize ) - 1 );
	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;
	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );
		if( Symbol == Sym )
		{
			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 1 );
			br.BitMaskIndex += 1;
			SetSymbolAtLoc( br.cbuffer, BitsRead, SymSize, InverseSym );
			BitsRead += SymSize;
		}
		else if( Symbol == InverseSym )
		{
			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 0 );
			br.BitMaskIndex += 1;
			BitsRead += SymSize;
		}
		else
			BitsRead += 1;
	}
	printf("Mask Size : %lld as pct %f\n", br.BitMaskIndex, (float)br.BitMaskIndex * 100 / (br.readcount * 8) );
}

void bitreader_convert_next_block_xor_some( bitreader &br )
{
	char *InputBackup = (char*)malloc( br.readcount );
	memcpy( InputBackup, br.cbuffer, br.readcount );

	br.BitMaskIndex = br.BitMaskIndex2 = 0;

	bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );

//	bitreader_convert_next_block_xor_some_( br, 0, 2 );//334 -> 339
	bitreader_convert_next_block_xor_some_( br, 0, 3 );//334 -> 335
//	bitreader_convert_next_block_xor_some_( br, 0, 4 );//334 -> 336
/*	{
		bitreader_convert_next_block_xor_some_( br, 0, 2 );
		br.ReadStartSkipp = 1;
		bitreader_convert_next_block_xor_some_( br, 0, 2 );//334 -> 378
	}/**/
	printf("\n");

	bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );

	bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
	bitreader_stat_symbols( br, 1, 1, 3, 0, 1 );
	bitreader_stat_symbols( br, 1, 1, 4, 0, 1 );

	fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	fwrite( br.cbuffer, 1, br.readcount, br.outf );

	br.processed_count =  br.readcount;
	free( InputBackup );
	return;
}

#if 0
void bitreader_convert_next_block_sum_memory_( bitreader &br, int SymSize, int MemorySize, int MidleVal  )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}

//	int MidleVal = 1 << ( SymSize - 1 );
	unsigned __int64 ReadCount = 0;
	unsigned __int64 MemSum = 0;
	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;
	while( BitsRead < can_process_bytes * 8 )
	{
		int avg = MemSum / MemorySize;

		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );
		int NewSymbol = Symbol;
		int correction = 0;
		MemSum += Symbol;
		if( ReadCount >= MemorySize )
		{
			//remove old
			int SymbolOld = GetSymbolAtLoc( br.cbuffer, BitsRead - SymSize * MemorySize, SymSize );
			MemSum -= SymbolOld; 
			if( avg >= ( 1 << SymSize ) )
				printf("1 memory contains extra value for avg, max is %d, we have %d\n", ( 1 << SymSize ), avg );
			correction = MidleVal - avg;
			NewSymbol = Symbol + correction;
			if( NewSymbol < 0 )
				NewSymbol += ( 1 << SymSize );
			if( NewSymbol >= ( 1 << SymSize ) )
				NewSymbol -= ( 1 << SymSize );
		}

		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, SymSize, NewSymbol );
		br.BitMaskIndex += SymSize;

		{
			int tSymbol = GetSymbolAtLoc( br.cbitmask, br.BitMaskIndex - SymSize, SymSize );
			int tNewSymbol = tSymbol - correction;
			if( tNewSymbol < 0 )
				tNewSymbol += ( 1 << SymSize );
			if( tNewSymbol > ( 1 << SymSize ) - 1 )
				tNewSymbol -= ( 1 << SymSize );
			if( Symbol != tNewSymbol )
				printf( "could not convert back" );
		}

		BitsRead += SymSize;
		ReadCount++;
	}
//	assert( br.BitMaskIndex / 8 == br.readcount );
}

void bitreader_convert_next_block_sum_memory_reverse_( bitreader &br, int SymSize, int MemorySize, int MidleVal )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}

//	int MidleVal = 1 << ( SymSize - 1 );
	unsigned __int64 ReadCount = 0;
	unsigned __int64 MemSum = 0;
	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	signed __int64 BitsRead = can_process_bytes * 8 - SymSize;
	while( BitsRead > 0 )
	{
		int avg = MemSum / MemorySize;
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );
		int NewSymbol = Symbol;
		int correction = 0;
		MemSum += Symbol;
		if( ReadCount > MemorySize )
		{
			//remove old
			int SymbolOld = GetSymbolAtLoc( br.cbuffer, BitsRead + SymSize * MemorySize, SymSize );
			MemSum -= SymbolOld; 
			if( avg >= ( 1 << SymSize ) )
				printf("2 memory contains extra value for avg, max is %d, we have %d\n", ( 1 << SymSize ), avg );
			correction = MidleVal - avg;
			NewSymbol = Symbol + correction;
		}

		SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, SymSize, NewSymbol );
		br.BitMaskIndex += SymSize;

		{
			int tSymbol = GetSymbolAtLoc( br.cbitmask, br.BitMaskIndex - SymSize, SymSize );
			int tNewSymbol = tSymbol - correction;
			if( tNewSymbol < 0 )
				tNewSymbol += ( 1 << SymSize );
			if( tNewSymbol > ( 1 << SymSize ) - 1 )
				tNewSymbol -= ( 1 << SymSize );
			if( Symbol != tNewSymbol )
				printf( "could not convert back" );
		}

		BitsRead -= SymSize;
		ReadCount++;
	}
//	assert( br.BitMaskIndex / 8 == br.readcount );
}

void bitreader_convert_next_block_sum_memory( bitreader &br )
{
	char *InputBackup = (char*)malloc( br.readcount );
	memcpy( InputBackup, br.cbuffer, br.readcount );

	br.BitMaskIndex = br.BitMaskIndex2 = 0;

	bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );

	printf("Starting looped run\n");
/*	{
		int StartSkip = 0;
		for( int i=4;i<12;i++)
		{
			br.BitMaskIndex = 0;
			bitreader_convert_next_block_sum_memory_( br, i, 1 << ( i - 1 ) );
			printf("Done\n");
			memcpy( br.cbuffer, br.cbitmask, br.readcount );
			bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
			bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
			StartSkip += i * ( 1 << ( i - 1 ) );
			br.ReadStartSkipp = StartSkip;
		}
	}/**/
/*	{
//		int UseBitlen = 4;	// 4:08:08 -> 334 -> 328
//		int UseBitlen = 4;	// 4:12:08 -> 334 -> 328
//		int UseBitlen = 4;	// 4:16:08 -> 334 -> 326
//		int UseBitlen = 4;	// 4:32:08 -> 334 -> 327
//		int UseBitlen = 8;	// 8:64:128 -> 334 -> 334
//		int UseBitlen = 4;	// 4:08:16 -> 334 -> 331
//		int UseBitlen = 4;	// 4:08:04 -> 334 -> 313
//		int UseBitlen = 8;	// 8:16:64 -> 334 -> 334
//		int UseBitlen = 8;	// 8:16:32 -> 334 -> 334
		int UseBitlen = 8;	// 8:16:16 -> 334 -> 334
//		int MemLen = 1 << ( UseBitlen - 1 );
//		int MemLen = 1 << ( UseBitlen - 0 );
		int MemLen = 1 << ( UseBitlen - 3 );
		for( int i=0;i<16;i++)
		{
			br.BitMaskIndex = 0;
			bitreader_convert_next_block_sum_memory_( br, UseBitlen, MemLen );
			printf("Done\n");
			memcpy( br.cbuffer, br.cbitmask, br.readcount );

			br.BitMaskIndex = 0;
			bitreader_convert_next_block_sum_memory_reverse_( br, UseBitlen, MemLen );
			printf("Done\n");
			memcpy( br.cbuffer, br.cbitmask, br.readcount );

			bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
			bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
		}
	}/**/
/*	{
		//334 to 246k
		int UseBitlen = 4;
		int MemLen = 3;
		for( int i=0;i<62;i++)
		{
			br.BitMaskIndex = 0;
			bitreader_convert_next_block_sum_memory_( br, UseBitlen, MemLen );
			printf("Done\n");
			memcpy( br.cbuffer, br.cbitmask, br.readcount );

			br.BitMaskIndex = 0;
			bitreader_convert_next_block_sum_memory_reverse_( br, UseBitlen, MemLen );
			printf("Done\n");
			memcpy( br.cbuffer, br.cbitmask, br.readcount );
		}
	}/**/
	{
		//334 to 111k
		int UseBitlen = 4;
		int MemLen = 2;
		for( int i=0;i<7;i++)
		{
			br.BitMaskIndex = 0;
			bitreader_convert_next_block_sum_memory_( br, UseBitlen, MemLen, 1 << ( UseBitlen - 1 ) );
			printf("Done\n");
			memcpy( br.cbuffer, br.cbitmask, br.readcount );

//			br.BitMaskIndex = 0;
//			bitreader_convert_next_block_sum_memory_reverse_( br, UseBitlen, MemLen, 1 << ( UseBitlen - 1 ) );
//			printf("Done\n");
//			memcpy( br.cbuffer, br.cbitmask, br.readcount );
		}
	}/**/
/*	{
		int BestScore = 99999999;
		int BBitLen,BMemLen,BLoop;
		for( int UseBitlen = 3; UseBitlen <= 8; UseBitlen++ )
		{
			for( int MemLen = 1 << ( UseBitlen / 3 ); MemLen < ( 1 << UseBitlen ); MemLen++ )
			{
				for( int LoopLimit = MemLen; LoopLimit < MemLen * 5; LoopLimit++ )
				{
					memcpy( br.cbuffer, InputBackup, br.readcount );
					for( int LoopCount=0;LoopCount<LoopLimit;LoopCount++)
					{
						printf( "Testing %d %d %d\n", UseBitlen, MemLen, LoopCount );
						br.BitMaskIndex = 0;
//						bitreader_convert_next_block_sum_memory_( br, UseBitlen, MemLen, 1 << ( UseBitlen - 1 ) );
						bitreader_convert_next_block_sum_memory_( br, UseBitlen, MemLen, 0 );
	//					printf("Done\n");
						memcpy( br.cbuffer, br.cbitmask, br.readcount );

						br.BitMaskIndex = 0;
//						bitreader_convert_next_block_sum_memory_reverse_( br, UseBitlen, MemLen, 1 << ( UseBitlen - 1 ) );
						bitreader_convert_next_block_sum_memory_reverse_( br, UseBitlen, MemLen, 0 );
	//					printf("Done\n");
						memcpy( br.cbuffer, br.cbitmask, br.readcount );
					}
					br.BitMaskIndex = 0;
					int ScoreNow = GetOutArchivedSize( br );
					printf("Test result %d\n", ScoreNow );
					if( ScoreNow < BestScore )
					{
						printf( "Found New Best Setup %d %d %d with size %d\n", UseBitlen, MemLen, LoopLimit, ScoreNow );
						BBitLen = UseBitlen;
						BMemLen = MemLen;
						BLoop = LoopLimit;
						BestScore = ScoreNow;
					}
					else
						printf( "Best combo was : %d %d %d %d %d\n",BBitLen, BMemLen, BLoop, BestScore, BestScore / 1024 );
				}
			}
		}
	}/**/
/*	{
		int BestScore = 99999999;
		int BBitLen,BMemLen,BLoop;
		int UseBitlen = 4;
		int MemLen = 3;
		{
			for( int LoopLimit = 4;LoopLimit<UseBitlen*16;LoopLimit++)
			{
				int LoopCount;
				memcpy( br.cbuffer, InputBackup, br.readcount );
				for( LoopCount=0;LoopCount<LoopLimit;LoopCount++)
				{
					printf( "Testing %d %d %d\n", UseBitlen, MemLen, LoopCount );
					br.BitMaskIndex = 0;
					bitreader_convert_next_block_sum_memory_( br, UseBitlen, MemLen );
//					printf("Done\n");
					memcpy( br.cbuffer, br.cbitmask, br.readcount );

					br.BitMaskIndex = 0;
					bitreader_convert_next_block_sum_memory_reverse_( br, UseBitlen, MemLen );
//					printf("Done\n");
					memcpy( br.cbuffer, br.cbitmask, br.readcount );
				}
				br.BitMaskIndex = 0;
				int ScoreNow = GetOutArchivedSize( br );
				printf("Test result %d\n", ScoreNow );
				if( ScoreNow < BestScore )
				{
					printf( "Found New Best Setup %d %d %d with size %d\n", UseBitlen, MemLen, LoopCount, ScoreNow );
					BBitLen = UseBitlen;
					BMemLen = MemLen;
					BLoop = LoopCount;
					BestScore = ScoreNow;
				}
			}
		}
		printf( "Best combo : %d %d %d %d %d\n",BBitLen, BMemLen, BLoop, BestScore, BestScore / 1024 );
	}/**/
	printf("Ended looped run\n");

	bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );

	bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
	bitreader_stat_symbols( br, 1, 1, 3, 0, 1 );
	bitreader_stat_symbols( br, 1, 1, 4, 0, 1 );

	fwrite( br.cbuffer, 1, br.readcount, br.outf );

	br.processed_count =  br.readcount;
	free( InputBackup );
	return;
}
#endif

void CalcImpact( bitreader &br, int SampledSymbolCount, int SymSize, int AvgChanged, int ChangeAdvised, signed __int64 *SymCount, int Goal )
{
	SymCount[0] = 0;	// 0 count
	SymCount[1] = 0;	// 1 count
	SymCount[2] = 0;	// old goal sym count 
	SymCount[3] = 0;	// new goal sym count 
	for( int Sym=0;Sym<(1<<SymSize);Sym++)
	{
		int Avg = AvgChanged;
		int MergedSymbol = ( Avg << SymSize ) | Sym;
		if( br.symbol_counts[ MergedSymbol ] == 0 )
			continue;
		int NewSym = Sym + ChangeAdvised;
		if( NewSym < 0 )
			NewSym += SymSize;
		if( NewSym >= ( 1 << SymSize ) )
			NewSym -= ( 1 << SymSize );
//		assert( NewSym >= 0 && NewSym < ( 1 << SymSize ) );
		int Count1 = Count1s( NewSym );
		SymCount[0] += ( SymSize - Count1 ) * br.symbol_counts[ MergedSymbol ];
		SymCount[1] += Count1 * br.symbol_counts[ MergedSymbol ];
		if( Sym == Goal )
			SymCount[2] += br.symbol_counts[ MergedSymbol ];
		if( NewSym == Goal )
			SymCount[3] += br.symbol_counts[ MergedSymbol ];
	}

}

void AddToMem( int &MemSum, int Symbol, int AddType, int SymSize )
{
	if( AddType == 0 )
		MemSum += Symbol;
	else if( AddType == 1 )
		MemSum ^= Symbol;
	else if( AddType == 2 )
		MemSum += Count1s( Symbol );
	else if( AddType == 3 )
		MemSum *= ( Symbol + 1 );
	else if( AddType == 4 )
		MemSum += Symbol;
	else if( AddType == 5 )
		MemSum += Symbol;
	else if( AddType == 6 )
	{
		if( Symbol & 1 )
		{
			if( MemSum + Symbol < ( 1 << SymSize ) )
				MemSum += Symbol;
		}
		else
		{
			if( MemSum > Symbol )
				MemSum -= Symbol;
		}
	}
	else if( AddType == 7 )
	{
		MemSum += ( Symbol & ( ( 1 << ( SymSize / 2 ) ) - 1 ) );
		MemSum += ( Symbol >> ( SymSize / 2 ) );
	}
	else if( AddType == 8 || AddType == 9 )
	{
	}
}

void RemFromMem( int &MemSum, int Symbol, int AddType, int SymSize )
{
	if( AddType == 0 )
		MemSum -= Symbol;
	else if( AddType == 1 )
		MemSum ^= Symbol;
	else if( AddType == 2 )
		MemSum -= Count1s( Symbol );
	else if( AddType == 3 )
		MemSum /= ( Symbol + 1 );
	else if( AddType == 4 )
		MemSum -= Symbol;
	else if( AddType == 5 )
		MemSum -= Symbol;
	else if( AddType == 6 )
	{
		if( Symbol & 1 )
		{
			if( MemSum > Symbol )
				MemSum -= Symbol;
		}
		else
		{
			if( MemSum + Symbol < ( 1 << SymSize ) )
				MemSum += Symbol;
		}
	}
	else if( AddType == 7 )
	{
		MemSum -= ( Symbol & ( ( 1 << ( SymSize / 2 ) ) - 1 ) );
		MemSum -= ( Symbol >> ( SymSize / 2 ) );
	}
	else if( AddType == 8 || AddType == 9 )
	{
	}
}

int AvgFromMem( int MemSum, int MemSize, int AddType, int *mem, int SymSize )
{
	if( AddType == 0 )
		return MemSum / MemSize;
	else if( AddType == 1 )
		return MemSum;
	else if( AddType == 2 )
		return MemSum / MemSize;
	else if( AddType == 3 )
		return sqrt( (float)MemSum );
	else if( AddType == 4 )
		return Count1s( MemSum / MemSize );
	else if( AddType == 5 ) //does not make sense, but why not try it ? :)
	{
		int ret = 0;
		int div = 0;
		for( int i=0;i<MemSize;i++)
		{
			ret += i * mem[i];
			div += i;
		}
		return ret / div;
	}
	else if( AddType == 6 )
		return MemSum / MemSize;
	else if( AddType == 7 )
	{
		int ret = MemSum;
		while( ret >= ( 1 << SymSize ) )
			ret /= 2;
		return ret;
	}
	else if( AddType == 8 )
	{
		unsigned int Min = ~0;
		for( int i=0;i<MemSize;i++)
			if( mem[i] < Min )
				Min = mem[i];
		return Min;
	}
	else if( AddType == 9 )
	{
		unsigned int Max = 0;
		for( int i=0;i<MemSize;i++)
			if( mem[i] > Max )
				Max = mem[i];
		return Max;
	}
	return 0;
}

void bitreader_convert_next_block_sum_memory_statistics_( bitreader &br, int SymSize, int MemorySize, int &BestAvg, int &BestSym, int &BestImpactCount, int Silent, int GoalSymbol, signed __int64 Count1s, int CounterType, int ObjectiveSelectType, int IsReversed )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}
	memset( br.symbol_counts, 0, br.SymbolCounterSize );

	int			*MemoryBuff = (int*)malloc( ( MemorySize + 1 ) * sizeof( int ) );
	int			MemoryBuffWriteIndex = 0;
	int			MemSum = 0;

	signed __int64 can_process_bits = br.readcount * 8;
	signed __int64 BitsRead = br.ReadStartSkipp;
	signed __int64 ReadFromLimit, ReadToLimit;
	signed __int64 ReadAdvance;
	if( IsReversed == 0 )
	{
		ReadAdvance = SymSize;
		ReadFromLimit = br.ReadStartSkipp;
		ReadToLimit = can_process_bits;
		BitsRead = br.ReadStartSkipp;
	}
	else
	{
		ReadAdvance = -SymSize;
		ReadFromLimit = br.ReadStartSkipp + SymSize;
		ReadToLimit = can_process_bits;
		BitsRead = can_process_bits - SymSize;
	}

	for( int i=0;i<MemorySize;i++)
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );
		MemoryBuff[ i ] = Symbol;
		AddToMem( MemSum, Symbol, CounterType, SymSize );
		BitsRead += ReadAdvance;
	}

	while( BitsRead < ReadToLimit && BitsRead > ReadFromLimit )
	{
		int avg = AvgFromMem( MemSum, MemorySize, CounterType, MemoryBuff, SymSize );

		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );

		//get the current avg
		int NewSymbol = ( avg << SymSize ) | Symbol;
		assert( NewSymbol < br.SymbolCount && NewSymbol < ( 1 << SymSize * 2 ) );
		br.symbol_counts[ NewSymbol ]++;

		//remove old
		int SymbolOld = MemoryBuff[ MemoryBuffWriteIndex ];
		RemFromMem( MemSum, SymbolOld, CounterType, SymSize );
		//add new
		MemoryBuff[ MemoryBuffWriteIndex ] = Symbol;
		MemoryBuffWriteIndex = ( MemoryBuffWriteIndex + 1 ) % MemorySize;
		AddToMem( MemSum, Symbol, CounterType, SymSize );

		BitsRead += ReadAdvance;
	}

	int SampledSymbolCount = (1<<(SymSize*2));

	BestAvg = -1;
	BestSym = 0;
	BestImpactCount = 0;
	//we change symbols after markers and try to make sure we maximize "1" count
	if( ObjectiveSelectType == 0 )
	{
		for( int i=0;i<SampledSymbolCount;i++)
			if( br.symbol_counts[ i ] > 0 )
			{
				int Avg = i >> SymSize;
				int	Sym = i & ( ( 1 << SymSize ) - 1 );
				int ChangeAdvised = GoalSymbol - Sym;
				if( ChangeAdvised < 0 )
					ChangeAdvised += ( 1 << SymSize );
				signed __int64 SymCount[4];
				CalcImpact( br, SampledSymbolCount, SymSize, Avg, ChangeAdvised, SymCount, GoalSymbol );
				int DoubleCheckGain = SymCount[1] - Count1s;
				if( DoubleCheckGain > BestImpactCount )
				{
					if( Silent == 0 )
						printf("Best case count now %d   : %d Avg %d Sym %d Mod %d\n", i, DoubleCheckGain, Avg, Sym, ChangeAdvised );
					BestImpactCount = DoubleCheckGain;
					BestAvg = Avg;
					BestSym = ChangeAdvised;
				}
			}
	}
	//we change symbols after markers and try to make sure we maximize selected symbol count
	else
	{
		//can we replace a symbol so our symbol will appear more 
		for( int Avg=0; Avg < ( 1 << SymSize );Avg++)
		{
			int OldSym = ( Avg << SymSize ) | GoalSymbol;
			signed __int64 OldCount = br.symbol_counts[ OldSym ];
			signed __int64 BiggestCount = OldCount;
			signed __int64 BiggestCountSym = -1;
			for( int Sym = 0; Sym < ( 1 << SymSize ); Sym++ )
			{
				int CurSym = ( Avg << SymSize ) | Sym;
				if( br.symbol_counts[ CurSym ] > BiggestCount )
				{
					BiggestCountSym = CurSym;
					BiggestCount = br.symbol_counts[ CurSym ];
				}
				if( BiggestCount - OldCount > BestImpactCount && BiggestCountSym != -1 )
				{
					if( Silent == 0 )
						printf("Best case count now %d %lld Avg %d Sym %d \n", SymSize, BiggestCount - OldCount, Avg, Sym );
					BestImpactCount = BiggestCount - OldCount;
					BestAvg = Avg;
					BestSym = Sym;
				}
			}
		}
	}

	free( MemoryBuff );
}

void bitreader_convert_next_block_sum_memory2_( bitreader &br, int SymSize, int MemorySize, int ModAvg, int ModSym, int GoalSymbol, int CounterType, int IsReversed )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}
	memset( br.symbol_counts, 0, br.SymbolCounterSize );

	int			*MemoryBuff = (int*)malloc( ( MemorySize + 1 ) * sizeof( int ) );
	int			MemoryBuffWriteIndex = 0;
	int			MemoryBuffOldestIndex = 0;
	int			MemSum = 0;

	signed __int64 can_process_bits = br.readcount * 8;
	signed __int64 BitsRead = br.ReadStartSkipp;
	signed __int64 ReadFromLimit, ReadToLimit;
	signed __int64 ReadAdvance;
	if( IsReversed == 0 )
	{
		ReadAdvance = SymSize;
		ReadFromLimit = br.ReadStartSkipp;
		ReadToLimit = can_process_bits;
		BitsRead = br.ReadStartSkipp;
	}
	else
	{
		ReadAdvance = -SymSize;
		ReadFromLimit = br.ReadStartSkipp + SymSize;
		ReadToLimit = can_process_bits;
		BitsRead = can_process_bits - SymSize;
	}

	for( int i=0;i<MemorySize;i++)
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );
		MemoryBuff[ i ] = Symbol;
		AddToMem( MemSum, Symbol, CounterType, SymSize );
		BitsRead += ReadAdvance;
	}

	while( BitsRead < ReadToLimit && BitsRead > ReadFromLimit )
	{
		int Avg = AvgFromMem( MemSum, MemorySize, CounterType, MemoryBuff, SymSize );

		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );

		//do the changes
		if( Avg == ModAvg )
		{
			if( Symbol == ModSym )
				SetSymbolAtLoc( br.cbuffer, BitsRead, SymSize, GoalSymbol );
			br.symbol_counts[ ( Avg << SymSize ) | Symbol ]++;
		}

		//remove old
		int SymbolOld = MemoryBuff[ MemoryBuffWriteIndex ];
		RemFromMem( MemSum, SymbolOld, CounterType, SymSize );
		//add new
		MemoryBuff[ MemoryBuffWriteIndex ] = Symbol;
		MemoryBuffWriteIndex = ( MemoryBuffWriteIndex + 1 ) % MemorySize;
		AddToMem( MemSum, Symbol, CounterType, SymSize );

		BitsRead += ReadAdvance;
	}

	free( MemoryBuff );
}


void bitreader_convert_next_block_sum_memory2( bitreader &br )
{
	char *InputBackup = (char*)malloc( br.readcount );
	memcpy( InputBackup, br.cbuffer, br.readcount );

	br.BitMaskIndex = br.BitMaskIndex2 = 0;

	bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
	bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );

	signed __int64 Count1s = br.symbol_counts[1];
	signed __int64 Count1sNew = Count1s;
//	bitreader_convert_next_block_sum_memory_statistics_( br, 4, 2 );

/*	for(int MarkerType = 0;MarkerType<=4;MarkerType++)
	{
		if( MarkerType == 3 )
			continue;
		printf("Testing Marker type %d\n", MarkerType );
		int SumImpactCount = 0;
		for( int LoopCount = 0; LoopCount < 10; LoopCount++ )
		{
			int BestBitCount, BestMemLen, BestAvg, BestMod, BestImpact, BestGoal;
			BestImpact = BestBitCount = 0;
//			for( int BitCount = 1; BitCount <= 6; BitCount += 1 )
//			for( int BitCount = 3; BitCount <= 5; BitCount += 1 )
			for( int BitCount = 1; BitCount <= 5; BitCount += 1 )
			{
				for( int MemLen = 15; MemLen <= 100; MemLen++ )
				{
					int tBestAvg, tBestMod, tBestImpact;
					bitreader_convert_next_block_sum_memory_statistics_( br, BitCount, MemLen, tBestAvg, tBestMod, tBestImpact, 1, GoalSymbol, Count1sNew, MarkerType );
//					if( tBestImpact * BitCount > BestImpact * BestBitCount )
					if( tBestImpact > BestImpact )
					{
						BestImpact = tBestImpact;
						BestAvg = tBestAvg;
						BestMod = tBestMod;
						BestBitCount = BitCount;
						BestMemLen = MemLen;
						BestGoal = GoalSymbol;
					}
				}
			}
			if( BestImpact < 4 * 8 )
				break;
			printf("%d)Changing input bitc %d, ml %d, avg %d, mod %d, imp %d\n",LoopCount, BestBitCount, BestMemLen, BestAvg, BestMod, BestImpact );
			bitreader_convert_next_block_sum_memory2_( br, BestBitCount, BestMemLen, BestAvg, BestMod, BestGoal, MarkerType );
			Count1sNew += BestImpact;
//			SumImpactCount += BestImpact * BestBitCount;
			SumImpactCount += BestImpact;
//			int OutputSize = GetOutArchivedSize( br );
//			printf("Output File Size now %d\n",OutputSize );

//			bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
		}
		printf("Total number of modifications %d out of %d as pct %f\n", SumImpactCount, br.readcount * 8, (float)SumImpactCount / br.readcount * 8 );
	}/**/
	{
		bitreader_swapsymbol( br, 2, 3, 1 );
		bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
		Count1s = br.symbol_counts[1];
		Count1sNew = Count1s;
		bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );

/*		{
			int BestBitCount = 4, BestMemLen = 11, BestAvg = 9, BestSym = 14, BestImpact = 96, BestGoal = 5;
			int MarkerType = 0;
			bitreader_convert_next_block_sum_memory_statistics_( br, BestBitCount, BestMemLen, BestAvg, BestSym, BestImpact, 1, BestGoal, Count1sNew, MarkerType, 1 );
			bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
			bitreader_convert_next_block_sum_memory2_( br, BestBitCount, BestMemLen, BestAvg, BestSym, BestGoal, MarkerType );
			bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
		}/**/

		int LoopCount = 0;
		bool ChangedSomething = true;
		int MarkerType = 7;
		while( ChangedSomething )
		{
			ChangedSomething = false;
			for( int IsReversed=0;IsReversed<=1;IsReversed++)
			{
	//			for(MarkerType = 0;MarkerType<=9;MarkerType++)
				for(MarkerType = 0;MarkerType<=1;MarkerType++)
				{
					if( MarkerType == 3 )
						continue;
	//				if( MarkerType == 4 )
	//					continue;
					printf("Testing Marker type %d\n", MarkerType );
					int SumImpactCount = 0;
	//				for( LoopCount = 0; LoopCount < 10; LoopCount++ )
					{
						int BestBitCount, BestMemLen, BestAvg, BestSym, BestImpact, BestGoal;
						BestImpact = BestBitCount = 0;
						for( int BitCount = 2; BitCount <= 10; BitCount += 2 )
	//					for( int BitCount = 2; BitCount <= 4; BitCount += 2 )
	//					for( int BitCount = 2; BitCount <= 8; BitCount += 1 ) //step 1 is not compatible with cyclic symbol ( 101010101 )
						{
							int GoalSymbol = 1;
							for( int t = 2; t < BitCount; t += 2 )
								GoalSymbol |= ( 1 << t );
							for( int MemLen = 2; MemLen <= 170; MemLen++ )
	//						for( int MemLen = 2; MemLen <= 70; MemLen++ )
	//						for( int MemLen = 6; MemLen <= 53; MemLen++ )
							{
								int tBestAvg, tBestSym, tBestImpact;
								bitreader_convert_next_block_sum_memory_statistics_( br, BitCount, MemLen, tBestAvg, tBestSym, tBestImpact, 1, GoalSymbol, Count1sNew, MarkerType, 1, IsReversed );
								if( tBestImpact * BitCount > BestImpact * BestBitCount )
			//					if( tBestImpact > BestImpact )
								{
									BestImpact = tBestImpact;
									BestAvg = tBestAvg;
									BestSym = tBestSym;
									BestBitCount = BitCount;
									BestMemLen = MemLen;
									BestGoal = GoalSymbol;
								}
							}
						}
						if( BestImpact * BestBitCount < 4 * 8 * 2 )
	//						break;
							continue;
						printf("%d) %d Changing input bitc %d, ml %d, avg %d, mod %d, imp %d\n", IsReversed, LoopCount, BestBitCount, BestMemLen, BestAvg, BestSym, BestImpact );
						bitreader_convert_next_block_sum_memory2_( br, BestBitCount, BestMemLen, BestAvg, BestSym, BestGoal, MarkerType, IsReversed );
						Count1sNew += BestImpact;
						SumImpactCount += BestImpact * BestBitCount;
						ChangedSomething = true;
			//			int OutputSize = GetOutArchivedSize( br );
			//			printf("Output File Size now %d\n",OutputSize );

						SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 8, BestAvg );
						br.BitMaskIndex += 8;
						SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 8, BestSym );
						br.BitMaskIndex += 8;
						SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 8, BestBitCount );
						br.BitMaskIndex += 8;
						SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 8, BestMemLen );
						br.BitMaskIndex += 8;

						fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
						fwrite( br.cbuffer, 1, br.readcount, br.outf );
						fseek( br.outf, 0L, SEEK_SET);
						fflush( br.outf );

						bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
						bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
						bitreader_stat_symbols( br, 1, 1, BestBitCount, 0, 1 );
					}
					printf("Total number of modifications %d out of %d as pct %f\n", SumImpactCount, br.readcount * 8, (float)SumImpactCount / br.readcount * 8 );
				}
			}
		}
	}/**/
/*	{
		printf("\n");
		for( int BitLen = 3; BitLen <= 8; BitLen++ )
			for( int MemLen=3;MemLen<30;MemLen++)
//				bitreader_convert_next_block_sum_memory2_( br, 4, 2, -1, -1, 8, 0 );
//				bitreader_convert_next_block_sum_memory2_( br, BitLen, MemLen, -1, -1, ( 1 << ( BitLen - 1 ) ), 0 );
//				bitreader_convert_next_block_sum_memory2_( br, BitLen, MemLen, -1, -1, ( 1 << ( BitLen - 0 ) ), 0 );
				bitreader_convert_next_block_sum_memory2_( br, BitLen, MemLen, -1, -1, 0, 0 );
		bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
		bitreader_stat_symbols( br, 1, 1, 4, 0, 1 );
	}/**/

	bitreader_stat_symbols( br, 1, 1, 1, 0, 1 );
	fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	fwrite( br.cbuffer, 1, br.readcount, br.outf );

	br.processed_count =  br.readcount;
	free( InputBackup );
	return;
}
#if 0
void bitreader_convert_next_block_sum_memory_bug_( bitreader &br, int SymSize, int MemorySize, int MidleVal )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}
	memset( br.symbol_counts, 0, br.SymbolCounterSize );

	int			*MemoryBuff = (int*)malloc( ( MemorySize + 1 ) * sizeof( int ) );
	int			MemoryBuffWriteIndex = 0;

	int			UselessChangeCount = 0;

	unsigned __int64 ReadCount = 0;
	unsigned __int64 MemSum = 0;
	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;
	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );
		int NewSymbol = Symbol;

		MemoryBuff[ MemoryBuffWriteIndex ] = Symbol;
		MemoryBuffWriteIndex = ( MemoryBuffWriteIndex + 1 ) % MemorySize;
		MemSum += Symbol;

		if( ReadCount > MemorySize )
		{
			//remove old
			int MemoryBuffOldestIndex = MemoryBuffWriteIndex;
			int SymbolOld = MemoryBuff[ MemoryBuffOldestIndex ];
			MemoryBuffOldestIndex = ( MemoryBuffOldestIndex + 1 ) % MemorySize;
			MemSum -= SymbolOld; 

			//calculate new value and write back
			int avg = MemSum / MemorySize;
			if( avg >= ( 1 << SymSize ) )
				printf("1 memory contains extra value for avg, max is %d, we have %d\n", ( 1 << SymSize ), avg );
			int correction = MidleVal - avg;
			NewSymbol = Symbol + correction;
			if( NewSymbol < 0 )
				NewSymbol += ( 1 << SymSize );
			if( NewSymbol >= ( 1 << SymSize ) )
				NewSymbol -= ( 1 << SymSize );
			if( NewSymbol != Symbol )
				SetSymbolAtLoc( br.cbuffer, BitsRead, SymSize, NewSymbol );
			else
				UselessChangeCount++;
		}

		BitsRead += SymSize;
		ReadCount++;
	}

	printf("Useless changes : %d\n",UselessChangeCount);
	free( MemoryBuff );
}

void bitreader_convert_next_block_sum_memory_bug_back_( bitreader &br, int SymSize, int MemorySize, int MidleVal )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}
	memset( br.symbol_counts, 0, br.SymbolCounterSize );

	int			*MemoryBuff = (int*)malloc( ( MemorySize + 1 ) * sizeof( int ) );
	int			MemoryBuffWriteIndex = 0;

	int			UselessChangeCount = 0;

	unsigned __int64 ReadCount = 0;
	unsigned __int64 MemSum = 0;
	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;
	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );
		int NewSymbol = Symbol;

		if( ReadCount <= MemorySize )
		{
			MemoryBuff[ MemoryBuffWriteIndex ] = Symbol;
			MemoryBuffWriteIndex = ( MemoryBuffWriteIndex + 1 ) % MemorySize;
			MemSum += Symbol;
		}

		if( ReadCount > MemorySize )
		{
			//remove old
			int MemoryBuffOldestIndex = MemoryBuffWriteIndex;
			int SymbolOld = MemoryBuff[ MemoryBuffOldestIndex ];
			MemoryBuffOldestIndex = ( MemoryBuffOldestIndex + 1 ) % MemorySize;
			MemSum -= SymbolOld; 

			//calculate new value and write back
			int avg = MemSum / MemorySize;
			if( avg >= ( 1 << SymSize ) )
				printf("1 memory contains extra value for avg, max is %d, we have %d\n", ( 1 << SymSize ), avg );
			int correction = MidleVal - avg;
			NewSymbol = Symbol - correction;
			if( NewSymbol < 0 )
				NewSymbol += ( 1 << SymSize );
			if( NewSymbol >= ( 1 << SymSize ) )
				NewSymbol -= ( 1 << SymSize );
			if( NewSymbol != Symbol )
				SetSymbolAtLoc( br.cbuffer, BitsRead, SymSize, NewSymbol );
			else
				UselessChangeCount++;
		}

		BitsRead += SymSize;
		ReadCount++;
	}

	printf("Useless changes : %d\n",UselessChangeCount);
	free( MemoryBuff );
}

void bitreader_convert_next_block_sum_memory_bug( bitreader &br )
{
	char *InputBackup = (char*)malloc( br.readcount );
	memcpy( InputBackup, br.cbuffer, br.readcount );

	br.BitMaskIndex = br.BitMaskIndex2 = 0;

/*	{
		for( int i=0;i<19;i++)
		{
			bitreader_convert_next_block_sum_memory_bug_( br, 4, 3, 8, 0 );

			bitreader_stat_symbols( br, 1, 1, 4, 0, 1 );
		}

		bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
		bitreader_stat_symbols( br, 1, 1, 3, 0, 1 );
		bitreader_stat_symbols( br, 1, 1, 4, 0, 1 );
	}/**/

	{
		bitreader_convert_next_block_sum_memory_bug_( br, 4, 3, 8 );
		bitreader_convert_next_block_sum_memory_bug_( br, 4, 3, 8 );
		for( int i=0;i<br.readcount;i++)
			if( InputBackup[i] != br.cbuffer[i] )
			{
				printf("first mismatch at %d\n",i);
			}
	}/**/

	fwrite( br.cbuffer, 1, br.readcount, br.outf );

	br.processed_count =  br.readcount;
	free( InputBackup );
	return;
}
#endif

void bitreader_convert_next_block_stat_sym_dist_( bitreader &br, int SymSize )
{
	if( br.BitmaskSize < br.readcount * 20 )
	{
		br.BitmaskSize = br.readcount * 20;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
	}
	memset( br.symbol_counts, 0, br.SymbolCounterSize );

	int MemBuffSize = ( 1 << SymSize ) * sizeof( unsigned __int64 );
	unsigned __int64	*PrevLocBuff = (unsigned __int64*)malloc( MemBuffSize );
	unsigned __int64	*SumDistBuff = (unsigned __int64*)malloc( MemBuffSize );
	unsigned __int64	*SumCountBuff = (unsigned __int64*)malloc( MemBuffSize );
	memset( PrevLocBuff, 0, MemBuffSize );
	memset( SumDistBuff, 0, MemBuffSize );
	memset( SumCountBuff, 0, MemBuffSize );

	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;

	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );

		unsigned __int64 PrevLoc = PrevLocBuff[ Symbol ];
		unsigned __int64 CurLoc = BitsRead;
		unsigned __int64 Dist = CurLoc - PrevLoc;
		SumDistBuff[ Symbol ] += Dist;
		SumCountBuff[ Symbol ]++;
		PrevLocBuff[ Symbol ] = BitsRead;

		BitsRead += SymSize;
	}

	for( int i=0;i<( 1<<SymSize) ;i++)
		if( SumCountBuff[ i ] > 0 )
			br.symbol_counts[ i ] = SumDistBuff[ i ] / SumCountBuff[ i ];
		else
			br.symbol_counts[ i ] = 0;

	free( PrevLocBuff );
	free( SumDistBuff );
	free( SumCountBuff );
}

void bitreader_convert_next_block_stat_onesym_dist_( bitreader &br, int Sym, int SymSize, int &LastDist )
{
	memset( br.symbol_counts, 0, br.SymbolCounterSize );
	memset( br.symbol_counts2, 0, br.SymbolCounterSize );

						LastDist = 0;
	unsigned __int64	PrevLoc = 0;
	unsigned __int64	SymCountInpected = 0;
	unsigned __int64	SymCountTotal = 0;

//	int					SymDecode = ~Sym & ( ( 1 << SymSize ) - 1 );
//	unsigned __int64	PrevLocDecode = 0;

	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;

	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );

		unsigned __int64 CurLoc = BitsRead;
		unsigned __int64 Dist = CurLoc - PrevLoc;

		if( Symbol == Sym )
		{
			if( Dist > LastDist )
				LastDist = Dist;
			assert( Dist < br.SymbolCount );
			br.symbol_counts[ Dist ]++;
			PrevLoc = BitsRead;
			SymCountInpected++;
		}

		SymCountTotal++;
		BitsRead += SymSize;
	}
	printf( "Dist-  Count - chance - COther - GuessChance - Win BitCount On Flip\n");
	for( int i=0;i<=LastDist;i++)
		if( br.symbol_counts[ i ] > 0 )
		{
			unsigned __int64 SumRemaining = 0;
			for( int j=i+1;j<LastDist;j++)
				if( br.symbol_counts[ j ] > 0 )
					SumRemaining += br.symbol_counts[ j ];
			float ChanceToGuessIfFlipAllAtPos = (float) br.symbol_counts[ i ] * 100.0f / (float)SumRemaining;
			int WinPct = (int)ChanceToGuessIfFlipAllAtPos - 100;
			int BitFlipWinCount = br.symbol_counts[ i ] * WinPct / 100;
//			if( BitFlipWinCount < 0 )
//				BitFlipWinCount = 0;
			printf( "% 3d - % 7lld - % 6f - % 6lld - % 6f - %d\n", i, br.symbol_counts[ i ], (float)br.symbol_counts[ i ] / SymCountInpected, SumRemaining, ChanceToGuessIfFlipAllAtPos, BitFlipWinCount );
		}
}

void bitreader_convert_next_block_stat_majority1_dist_( bitreader &br, int Count1Min, int Count0Min, int SymSize, int &LastDist )
{
	memset( br.symbol_counts, 0, br.SymbolCounterSize );

						LastDist = 0;
	unsigned __int64	PrevLoc = 0;
	unsigned __int64	SymCountInpected = 0;
	unsigned __int64	SymCountTotal = 0;

	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;

	while( BitsRead < can_process_bytes * 8 )
	{
		int Count1 = 0;
		int Count0 = 0;

/*		{
			unsigned __int64 Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize);
			unsigned __int64 tCount1 = Count1s( Symbol );
			Count1 += tCount1;
			Count0 += SymSize - tCount1;
		}/**/

		{
#define ChunkSize 56
			for( int i=0; i<=SymSize-ChunkSize; i+=ChunkSize)
			{
				unsigned __int64 Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead+i, ChunkSize );
				unsigned __int64 tCount1 = Count1s( Symbol );
				Count1 += tCount1;
				Count0 += ChunkSize - tCount1;
			}
			unsigned __int64 Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead+(SymSize/ChunkSize)*ChunkSize, SymSize % ChunkSize);
			unsigned __int64 tCount1 = Count1s( Symbol );
			Count1 += tCount1;
			Count0 += ( SymSize % ChunkSize ) - tCount1;
		}/**/

		if( Count1 >= Count1Min || Count0 >= Count0Min )
		{
			unsigned __int64 CurLoc = BitsRead;
			unsigned __int64 Dist = CurLoc - PrevLoc;
			if( Dist > LastDist )
				LastDist = Dist;
			assert( Dist < br.SymbolCount );
			br.symbol_counts[ Dist ]++;
			PrevLoc = BitsRead;
			SymCountInpected++;
		}

		SymCountTotal++;
		BitsRead += SymSize;
	}
	printf( "Dist-  Count - chance - COther - GuessChance - Win BitCount On Flip\n");
	for( int i=0;i<=LastDist;i++)
		if( br.symbol_counts[ i ] > 0 )
		{
			unsigned __int64 SumRemaining = 0;
			for( int j=i+1;j<LastDist;j++)
				if( br.symbol_counts[ j ] > 0 )
					SumRemaining += br.symbol_counts[ j ];
			float ChanceToGuessIfFlipAllAtPos = (float) br.symbol_counts[ i ] * 100.0f / (float)SumRemaining;
			if( ChanceToGuessIfFlipAllAtPos < 0.00001f )
				ChanceToGuessIfFlipAllAtPos = 0;
			int WinPct = (int)ChanceToGuessIfFlipAllAtPos - 100;
			int BitFlipWinCount = br.symbol_counts[ i ] * WinPct / 100;
//			if( BitFlipWinCount < 0 )
//				BitFlipWinCount = 0;
			float Chance = (float)br.symbol_counts[ i ] / SymCountInpected;
			if( Chance < 0.00001f )
				Chance = 0;
			printf( "% 3d - % 7lld - % 6f - % 8lld - % 7f - %d\n", i, br.symbol_counts[ i ], Chance, SumRemaining, ChanceToGuessIfFlipAllAtPos, BitFlipWinCount );
		}
}

void bitreader_convert_next_block_stat_smallerthen_dist_( bitreader &br, __int64 Pivot, int SymSize, int &LastDist )
{
	memset( br.symbol_counts, 0, br.SymbolCounterSize );

						LastDist = 0;
	unsigned __int64	PrevLoc = 0;
	unsigned __int64	SymCountInpected = 0;
	unsigned __int64	SymCountTotal = 0;

	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;

	while( BitsRead < can_process_bytes * 8 )
	{
		unsigned __int64 Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize);

		if( Pivot > Symbol )
		{
			unsigned __int64 CurLoc = BitsRead;
			unsigned __int64 Dist = CurLoc - PrevLoc;
			if( Dist > LastDist )
				LastDist = Dist;
			assert( Dist < br.SymbolCount );
			br.symbol_counts[ Dist ]++;
			PrevLoc = BitsRead;
			SymCountInpected++;
		}

		SymCountTotal++;
		BitsRead += SymSize;
	}
	printf( "Dist-  Count - chance - COther - GuessChance - Win BitCount On Flip\n");
	for( int i=0;i<=LastDist;i++)
		if( br.symbol_counts[ i ] > 0 )
		{
			unsigned __int64 SumRemaining = 0;
			for( int j=i+1;j<LastDist;j++)
				if( br.symbol_counts[ j ] > 0 )
					SumRemaining += br.symbol_counts[ j ];
			float ChanceToGuessIfFlipAllAtPos = (float) br.symbol_counts[ i ] * 100.0f / (float)SumRemaining;
			if( ChanceToGuessIfFlipAllAtPos < 0.00001f )
				ChanceToGuessIfFlipAllAtPos = 0;
			int WinPct = (int)ChanceToGuessIfFlipAllAtPos - 100;
			int BitFlipWinCount = br.symbol_counts[ i ] * WinPct / 100;
//			if( BitFlipWinCount < 0 )
//				BitFlipWinCount = 0;
			float Chance = (float)br.symbol_counts[ i ] / SymCountInpected;
			if( Chance < 0.00001f )
				Chance = 0;
			float ChanceAll = (float)br.symbol_counts[ i ] / SymCountTotal;
			if( ChanceAll < 0.00001f )
				ChanceAll = 0;
			printf( "% 3d - % 7lld - % 6f - % 8lld - % 7f - %07d - %7f\n", i, br.symbol_counts[ i ], Chance, SumRemaining, ChanceToGuessIfFlipAllAtPos, BitFlipWinCount, ChanceAll );
		}
}

void bitreader_convert_next_block_stat_seq0_dist_( bitreader &br, int SeqLenReq, int SymSize, int &LastDist )
{
	memset( br.symbol_counts, 0, br.SymbolCounterSize );

						LastDist = 0;
	unsigned __int64	PrevLoc = 0;
	unsigned __int64	SymCountInpected = 0;
	unsigned __int64	SymCountTotal = 0;

	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;

	while( BitsRead < can_process_bytes * 8 )
	{
		int SeqNow = 0;
		int SeqBest = 0;
		for( int i=0;i<SymSize;i++)
		{
			unsigned __int64 Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize);
			if( Symbol == 0 )
				SeqNow++;
			else
			{
				if( SeqNow > SeqBest )
					SeqBest = SeqNow;
				SeqNow = 0;
			}
		}
		if( SeqNow > SeqBest )
			SeqBest = SeqNow;

		if( SeqLenReq <= SeqBest )
		{
			unsigned __int64 CurLoc = BitsRead;
			unsigned __int64 Dist = CurLoc - PrevLoc;
			if( Dist > LastDist )
				LastDist = Dist;
			assert( Dist < br.SymbolCount );
			br.symbol_counts[ Dist ]++;
			PrevLoc = BitsRead;
			SymCountInpected++;
		}

		SymCountTotal++;
		BitsRead += SymSize;
	}
	printf( "Dist-  Count - chance - COther - GuessChance - Win BitCount On Flip\n");
	for( int i=0;i<=LastDist;i++)
		if( br.symbol_counts[ i ] > 0 )
		{
			unsigned __int64 SumRemaining = 0;
			for( int j=i+1;j<LastDist;j++)
				if( br.symbol_counts[ j ] > 0 )
					SumRemaining += br.symbol_counts[ j ];
			float ChanceToGuessIfFlipAllAtPos = (float) br.symbol_counts[ i ] * 100.0f / (float)SumRemaining;
			if( ChanceToGuessIfFlipAllAtPos < 0.00001f )
				ChanceToGuessIfFlipAllAtPos = 0;
			int WinPct = (int)ChanceToGuessIfFlipAllAtPos - 100;
			int BitFlipWinCount = br.symbol_counts[ i ] * WinPct / 100;
//			if( BitFlipWinCount < 0 )
//				BitFlipWinCount = 0;
			float Chance = (float)br.symbol_counts[ i ] / SymCountInpected;
			if( Chance < 0.00001f )
				Chance = 0;
			float ChanceAll = (float)br.symbol_counts[ i ] / SymCountTotal;
			if( ChanceAll < 0.00001f )
				ChanceAll = 0;
			printf( "% 3d - % 7lld - % 6f - % 8lld - % 7f - %07d - %7f\n", i, br.symbol_counts[ i ], Chance, SumRemaining, ChanceToGuessIfFlipAllAtPos, BitFlipWinCount, ChanceAll );
		}
}

void bitreader_convert_next_block_stat_onesym_dist_periodic_( bitreader &br, int Sym, int SymSize, int Period, int MyGuessAtPeriond, int &LastDist )
{
	memset( br.symbol_counts, 0, br.SymbolCounterSize );
	memset( br.symbol_counts2, 0, br.SymbolCounterSize );

						LastDist = 0;
	unsigned __int64	PrevLoc = 0;
	unsigned __int64	SymCountInpected = 0,SymCountDecode = 0;
	unsigned __int64	SymCountTotal = 0;
	unsigned __int64	NextPeriodEnd = Period;

	unsigned __int64	GuessCountGood = 0,GuessCountBad = 0;
	int					SymDecode = ~Sym & ( ( 1 << SymSize ) - 1 );
	unsigned __int64	PrevLocDecode = 0;

	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;

	while( BitsRead < can_process_bytes * 8 )
	{
		int Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize );
		int SymbolFlipped = ~Symbol & ( ( 1 << SymSize ) - 1 );

		if( NextPeriodEnd < BitsRead )
		{
			NextPeriodEnd = BitsRead + Period;
			PrevLoc = BitsRead;
			PrevLocDecode = BitsRead;
		}

		unsigned __int64 CurLoc = BitsRead;
		unsigned __int64 Dist = CurLoc - PrevLoc;

		if( Dist == MyGuessAtPeriond )
		{
			if( Symbol == Sym )
				GuessCountGood++;
			else if( Symbol == SymDecode )
				GuessCountBad++;
		}

		if( Symbol == Sym )
		{
			if( Dist > LastDist )
				LastDist = Dist;
			assert( Dist < br.SymbolCount );
			br.symbol_counts[ Dist ]++;
			PrevLoc = BitsRead;
			SymCountInpected++;
		}

		if( Symbol == SymDecode )
			SymCountDecode++;

		SymCountTotal++;
		BitsRead += SymSize;
	}
	printf( "Guesses Good : %lld\n",GuessCountGood);
	printf( "Guesses Bad  : %lld\n",GuessCountBad);
	printf( "Dist-  Count - chance - COther - GuessChance - Win BitCount On Flip\n");
	for( int i=0;i<=LastDist;i++)
		if( br.symbol_counts[ i ] > 0 )
		{
			unsigned __int64 SumRemaining = 0;
			for( int j=i+1;j<=LastDist;j++)
				if( br.symbol_counts[ j ] > 0 )
					SumRemaining += br.symbol_counts[ j ];
			float ChanceToGuessIfFlipAllAtPos = (float) br.symbol_counts[ i ] * 100.0f / (float)SumRemaining;
			int WinPct = (int)ChanceToGuessIfFlipAllAtPos - 100;
			int BitFlipWinCount = br.symbol_counts[ i ] * WinPct / 100;
//			if( BitFlipWinCount < 0 )
//				BitFlipWinCount = 0;
			printf( "% 3d - % 7lld - % 6f - % 6lld - % 6f - %d\n", i, br.symbol_counts[ i ], (float)br.symbol_counts[ i ] / SymCountInpected, SumRemaining, ChanceToGuessIfFlipAllAtPos, BitFlipWinCount );
		}
}

void bitreader_convert_next_block_stat_sym_dist_( bitreader &br )
{
	int LastDist;
/*	for( int BitCount=1;BitCount<=7;BitCount++)
	{
		bitreader_convert_next_block_stat_sym_dist_( br, BitCount );
		bitreader_stat_symbols( br, 0, 1, BitCount, 0, 0 );
	}/**/
//	for( int i=0;i<(1<<3);i++)
//		printf("%d - %d\n",i, br.symbol_counts[i] );
/*	for( int BitCount=1;BitCount<=2;BitCount++)
	{
		printf("Tested bitlen = %d \n",BitCount);
		bitreader_convert_next_block_stat_onesym_dist_( br, ( 1 << BitCount ) - 1, BitCount );
//		bitreader_convert_next_block_stat_onesym_dist_( br, 0, BitCount );
	}/**/
/*	for( int Sym=0;Sym<=3;Sym++)
	{
		printf("Tested bitlen %d Sym %d\n",2, Sym);
		bitreader_convert_next_block_stat_onesym_dist_( br, Sym, 2 );
	}/**/
/*	{
		printf("compare distances and try to pick one where 0 has more then 1( if there is any )\n");
		bitreader_convert_next_block_stat_onesym_dist_( br, 0, 1, LastDist );
		bitreader_convert_next_block_stat_onesym_dist_( br, 1, 1, LastDist );
	}/**/
/*	{
		if( br.BitmaskSize < br.SymbolCount * 8 )
		{
			br.BitmaskSize = br.SymbolCount * 8;
			br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
		}
		printf("compare distances and try to pick one where 0 has more then 1( if there is any )\n");
		bitreader_convert_next_block_stat_onesym_dist_( br, 0, 1, LastDist );

		assert( LastDist < br.SymbolCount );
		__int64	*Sym0DistCount = (__int64	*)br.cbitmask;
		memcpy( Sym0DistCount, br.symbol_counts, LastDist * 8 );

		bitreader_convert_next_block_stat_onesym_dist_( br, 1, 1, LastDist );

		//compare if at any distance we have a higher chance to guess 0 then 1
//		for( int i=0; i<=LastDist; i++ )
//			if( Sym0DistCount[ i ] > br.symbol_counts[ i ] )
	}/**/
/*	{
		printf("compare distances and try to pick one where 0 has more then 1( if there is any )\n");
		for( int Bitlen=3;Bitlen<=64+8;Bitlen+=2)
		{
			int Required0s = (Bitlen+1)/2;
			printf("required zeroes %d for bitlen %d\n",Required0s,Bitlen );
			bitreader_convert_next_block_stat_majority1_dist_( br, 9999, Required0s, Bitlen, LastDist );
		}
	}/**/
/*	{
		printf("compare distances and try to pick one where 0 has more then 1( if there is any )\n");
		for( int Bitlen=2;Bitlen<=56;Bitlen++)
		{
			__int64 Pivot = (__int64)1 << (Bitlen-1);
			printf("smaller then %lld for bitlen %d\n",Pivot,Bitlen );
			bitreader_convert_next_block_stat_smallerthen_dist_( br, Pivot, Bitlen, LastDist );
		}
	}/**/
/*	{
		printf("compare distances and try to pick one where 0 has more then 1( if there is any )\n");
		for( int Bitlen=2;Bitlen<=5;Bitlen++)
		{
			int SeqLen = Bitlen / 2;
			printf("seq len %d for bitlen %d\n",SeqLen,Bitlen );
			bitreader_convert_next_block_stat_smallerthen_dist_( br, SeqLen, Bitlen, LastDist );
		}
	}/**/
	{
		printf("compare distances and try to pick one where 0 has more then 1( if there is any )\n");
		for( int Bitlen=2;Bitlen<=6;Bitlen++)
		{
//			int Extract = ( 1 << Bitlen ) - 1;
			int Extract = 0;
			int Period = ( 1 << Bitlen ) / 2 * Bitlen;
//			int GuessPeriod = Period - Bitlen;
//			int GuessPeriod = 0;
			int GuessPeriod = 75;
			printf("extract symbol %d for bitlen %d and period %d guess period %d\n",Extract, Bitlen, Period, Period - Bitlen );
			bitreader_convert_next_block_stat_onesym_dist_periodic_( br, Extract, Bitlen, Period, GuessPeriod, LastDist );
		}
	}/**/
	br.processed_count =  br.readcount;
}

bool IsPrime( unsigned __int64 Num )
{
	unsigned __int64 NumSQRT = sqrt( (long double )Num );
	for( unsigned __int64 Div =  NumSQRT; Div > 1; Div -= 1 )
		if( Num % Div == 0 )
			return false;
	return true;
}

void SplitNumberToPrimes( unsigned __int64 &A, unsigned __int64 &B )
{
	unsigned __int64 NumSQRT = sqrt( (long double )A );
	for( unsigned __int64 Div =  NumSQRT; Div > 1; Div -= 1 )
		if( A % Div == 0 )
		{
			B = Div;
			A = A / Div;
			return;
		}
	B = A;
	A = 0;
}

void bitreader_convert_next_block_prime_mul_( bitreader &br, int SymSize )
{
	if( br.BitmaskSize < br.readcount * 8 * 8 * 8 * 8 )
	{
		br.BitmaskSize = br.readcount * 8 * 8 * 8 * 8;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
//		br.cbitmask2 = (unsigned char*)realloc( br.cbitmask2, br.BitmaskSize ); 
//		br.cbitmask3 = (unsigned char*)realloc( br.cbitmask3, br.BitmaskSize ); 
		br.BitMaskIndex = 0;
		br.BitMaskIndex2 = 0;
		br.BitMaskIndex3 = 0;
	}

	//gen primes
	int PrimeCount = 0;
	int *Primes = (int*)malloc( sizeof( int ) * (1<<SymSize) );
	for( int i=0;i<(1<<SymSize);i++)
	{
		if( IsPrime( i ) )
		{
			Primes[ PrimeCount ] = i;
			PrimeCount++;
		}
	}
	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;

	while( BitsRead < can_process_bytes * 8 )
	{
		unsigned __int64 Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize);

		int WritesMade = 0;
		unsigned __int64 A, B;
		A = Symbol;
		do{
			SplitNumberToPrimes( A, B );
			//get the index of the prime we will write
			{
				int index = 0;
				for( int i=0;i<PrimeCount;i++)
					if( Primes[i] == B )
					{
						index = i;
						break;
					}
				//write it back
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, SymSize, index );
			}/**/
/*			{
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, SymSize, B );
			}/**/
			br.BitMaskIndex += SymSize;
			WritesMade++;
			assert( WritesMade < SymSize ); // 1 * 2 * 2 .... 2. Ex : 128 = 2 * 64 = 2 * 2 * 32 = ...
			assert( br.BitMaskIndex < br.BitmaskSize * 8 );
		}while( A > 1 );

		BitsRead += SymSize;
	}
	free( Primes );
}

void bitreader_convert_next_block_sqrt_mul_( bitreader &br, int SymSize )
{
	if( br.BitmaskSize < br.readcount * 8 * 8 )
	{
		br.BitmaskSize = br.readcount * 8 * 8;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
		br.BitMaskIndex = 0;
		br.BitMaskIndex2 = 0;
		br.BitMaskIndex3 = 0;
	}

	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;

	while( BitsRead < can_process_bytes * 8 )
	{
		unsigned __int64 Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, SymSize);

		if( Symbol > 1 )
		{
			unsigned __int64 SymbolSqrt = sqrt( (long double) Symbol );
			//8 => 255 - 15 * 15 = 30
			//4 => 31 - 5 * 5 = 6
			unsigned __int64 SymbolMod = Symbol - SymbolSqrt * SymbolSqrt; 
			assert( SymbolSqrt < ( 1 << ( SymSize / 2 ) ) );
			assert( SymbolMod < ( 1 << ( SymSize / 2 + 1) ) );
			int BitsForLarge = CountBitsRequiredStore( SymbolSqrt );
			assert( SymbolMod < ( 1 << ( BitsForLarge + 1) ) && ( BitsForLarge + 1 ) <= ( SymSize / 2 + 1 ) );
			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, SymSize / 2, SymbolSqrt );
			br.BitMaskIndex += SymSize / 2;
			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, BitsForLarge + 1, SymbolMod );
			br.BitMaskIndex += SymSize / 2 + 1;
		}
		else
		{
			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, SymSize / 2, Symbol );
			br.BitMaskIndex += SymSize / 2;
		}

		BitsRead += SymSize;
	}
}

void bitreader_convert_next_block_prime_mul_( bitreader &br )
{
	//break down the number into prime multipliers, store the index of the prime
//	bitreader_convert_next_block_prime_mul_( br, 8);
/*	{
		//375
		bitreader_swapsymbol( br, 4, ( ( 1 << 4 ) - 1 ), 0 );
		bitreader_stat_symbols( br, 0, 1, 2, 0 );
		bitreader_convert_next_block_sqrt_mul_( br, 4);
	}/**/
/*	{
		//374
		bitreader_swapsymbol( br, 8, ( ( 1 << 8 ) - 1 ), 0 );
		bitreader_stat_symbols( br, 0, 1, 2, 0 );
		bitreader_convert_next_block_sqrt_mul_( br, 8);
	}/**/
/*	{
		//355
		bitreader_swapsymbol( br, 16, ( ( 1 << 16 ) - 1 ), 0 );
		bitreader_stat_symbols( br, 0, 1, 2, 0 );
		bitreader_convert_next_block_sqrt_mul_( br, 16);
	}/**/
	{
		//348
		bitreader_swapsymbol( br, 24, ( ( 1 << 24 ) - 1 ), 0 );
		bitreader_stat_symbols( br, 0, 1, 1, 0, 1 );
		bitreader_convert_next_block_sqrt_mul_( br, 24);
		bitreader_stat_symbols( br, 0, 1, 1, 0, 2 );
	}/**/
	//dump it
	fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	//stop looping
	br.processed_count =  br.readcount;
}

void bitreader_convert_next_block_xor_longest_( bitreader &br )
{
	//sample sequances
	// try to remove the ones with biggest impact
/*	{
		int CurBitcount = 128 + 128 + 128 + 128 + 256 + 1;
		bitreader_convert_next_block_xormask_anylen( br, 0, CurBitcount, CurBitcount, CurBitcount / 2, 1 );
	}/**/
	{
		int CurBitcount = 3;
		bitreader_convert_next_block_xormask_anylen( br, 0, CurBitcount, CurBitcount, CurBitcount / 2, 1 );
	}/**/
	//dump it
	fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	fwrite( br.cbuffer, 1, br.readcount, br.outf );
	//stop looping
	br.processed_count =  br.readcount;
	br.have_symbols = 0;
}

void bitreader_convert_next_block_runlength_stat_( bitreader &br, int WriteBack, int WriteBackLen, int Sym )
{
	if( br.BitmaskSize < br.readcount * 8 * 8 )
	{
		br.BitmaskSize = br.readcount * 8 * 8;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
		br.BitMaskIndex = 0;
	}
	memset( br.symbol_counts, 0, br.SymbolCounterSize );

	int MaxLen = 0;
	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;

	while( BitsRead < can_process_bytes * 8 )
	{
		unsigned __int64 Symbol;
		//test runlength ending in 1
		int Len = 0;
		do {
			Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead + Len, 1);
			Len++;
		}while( Symbol == Sym );

		br.symbol_counts[ Len ]++;
		BitsRead += Len;
		if( Len > MaxLen )
			MaxLen = Len;
		if( WriteBack )
		{
			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, WriteBackLen, Len-1 );
			br.BitMaskIndex += WriteBackLen;
			assert( CountBitsRequiredStore( Len-1 ) <= WriteBackLen );
		}
	}
	unsigned __int64 SumLen = 0;
	unsigned __int64 SumLenCount = 0;
	for( int i=0;i<=MaxLen;i++)
	{
		unsigned __int64 RestCount = 0;
		for( int j=i+1;j<=MaxLen;j++)
			RestCount += br.symbol_counts[ j ];
		if( br.symbol_counts[ i ] > 0 )
		{
			unsigned __int64 PCTChanceToFindComparedToRemaining = br.symbol_counts[ i ] * 100 / ( br.symbol_counts[ i ] + RestCount );
			printf( "% 5d : % 8lld : %lld \n", i, br.symbol_counts[ i ], PCTChanceToFindComparedToRemaining );
			SumLen += i * br.symbol_counts[ i ];
			SumLenCount += br.symbol_counts[ i ];
		}
	}
	printf( "avg len * 100 %lld \n", SumLen * 100 / SumLenCount );
}

void bitreader_convert_next_block_dynlen_stat_( bitreader &br, int WriteBack, int WriteBackLen, int Sym, int MinLen, int MaxLen )
{
	if( br.BitmaskSize < br.readcount * 8 * 8 )
	{
		br.BitmaskSize = br.readcount * 8 * 8;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
		br.BitMaskIndex = 0;
	}
	memset( br.symbol_counts, 0, br.SymbolCounterSize );

	unsigned __int64 MaxLenSeen = 0;
	unsigned __int64 CouldNotStatTooLarge = 0;
	unsigned __int64 CouldNotStatTooLong = 0;
	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;

	while( BitsRead < can_process_bytes * 8 )
	{
		unsigned __int64 Symbol;
		//test runlength ending in 1
		int Len = MinLen;
		do {
			Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead + Len, 1);
			Len++;
		}while( Symbol == Sym && Len < MaxLen );

		if( Len >= 56 )
			CouldNotStatTooLong++;
		else
		{
			Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead, Len );

			if( Symbol < br.SymbolCount )
			{
				br.symbol_counts[ Symbol ]++;
				if( Symbol > MaxLenSeen )
					MaxLenSeen = Symbol;
			}
			else 
				CouldNotStatTooLarge++;
		}

		BitsRead += Len;
/*		if( WriteBack )
		{
			SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, WriteBackLen, Symbol );
			br.BitMaskIndex += WriteBackLen;
			assert( CountBitsRequiredStore( Len-1 ) <= WriteBackLen );
		} */
	}
	printf("Could not stat symbol count %lld %lld \n", CouldNotStatTooLarge, CouldNotStatTooLong );
	unsigned __int64 SumLen = 0;
	unsigned __int64 SumLenCount = 0;
	unsigned __int64 UniqueCount = 0;
	for( int i=0;i<=MaxLenSeen;i++)
	{
		if( br.symbol_counts[ i ] > 0 )
		{
			unsigned __int64 RestCount = 0;
			for( int j=i+1;j<=MaxLenSeen;j++)
				RestCount += br.symbol_counts[ j ];
			unsigned __int64 PCTChanceToFindComparedToRemaining = br.symbol_counts[ i ] * 100 / ( br.symbol_counts[ i ] + RestCount );
//			printf( "% 5d : % 8lld : %lld \n", i, br.symbol_counts[ i ], PCTChanceToFindComparedToRemaining );
			printf( "% 8d : % 8lld : % 4lld ", i, br.symbol_counts[ i ], PCTChanceToFindComparedToRemaining ); PrintAsBinary( i , 20 ); printf( "\n" );
			int ReadLen = CountBitsRequiredStore( i );
			if( ReadLen < MinLen )
				ReadLen = MinLen;
			if( ReadLen < 1 )
				ReadLen = 1;
			SumLen += ReadLen * br.symbol_counts[ i ];
			SumLenCount += br.symbol_counts[ i ];
			UniqueCount++;
		}
	}
	printf( "avg len * 100 %lld \n", SumLen * 100 / SumLenCount );
	int RenameBitlen = CountBitsRequiredStore( UniqueCount );
	unsigned __int64 NewBitLen = 0;
	for( int i=0;i<=MaxLenSeen;i++)
		NewBitLen += RenameBitlen * br.symbol_counts[ i ];
	printf( "Unique symbol count %lld, rename cost %d \n", UniqueCount, RenameBitlen );
	printf( "Old bitlen %d\n", br.readcount * 8 );
	printf( "New bitlen %lld\n", NewBitLen );
}

void bitreader_convert_next_block_runlength_stat_bidir( bitreader &br, int WriteBack, int WriteBackLen, int AllowExceptions = 0 )
{
	if( br.BitmaskSize < br.readcount * 8 * 8 )
	{
		br.BitmaskSize = br.readcount * 8 * 8;
		br.cbitmask = (unsigned char*)realloc( br.cbitmask, br.BitmaskSize ); 
		br.BitMaskIndex = 0;
	}
	memset( br.symbol_counts, 0, br.SymbolCounterSize );

	int MaxLen = 0;
	int can_process_bytes = br.readcount;
	can_process_bytes -= ( br.SymbolBitCount + 7 ) / 8; //last bytes are ignored in this chunk
	unsigned __int64 BitsRead = br.ReadStartSkipp;

	while( BitsRead < can_process_bytes * 8 )
	{
		unsigned __int64 Symbol;
		//test runlength ending in 1
		int Len1 = 0;
		do {
			Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead + Len1, 1);
			Len1++;
		}while( Symbol == 0 );
		
		//test runlength ending in 1
		int Len0 = 0;
		do {
			Symbol = GetSymbolAtLoc( br.cbuffer, BitsRead + Len0, 1);
			Len0++;
		}while( Symbol == 1 );

		if( Len1 > Len0 )
		{
			br.symbol_counts[ Len1 ]++;
			BitsRead += Len1;
			if( Len1 > MaxLen )
				MaxLen = Len1;
			if( WriteBack )
			{
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 1 );
				br.BitMaskIndex += 1;
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, WriteBackLen, Len1 - 2 );
				br.BitMaskIndex += WriteBackLen;
				assert( AllowExceptions == 1 || CountBitsRequiredStore( Len1 - 2 ) <= WriteBackLen );
			}
		}
		else
		{
			br.symbol_counts[ Len0 ]++;
			BitsRead += Len0;
			if( Len0 > MaxLen )
				MaxLen = Len0;
			if( WriteBack )
			{
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, 1, 0 );
				br.BitMaskIndex += 1;
				SetSymbolAtLoc( br.cbitmask, br.BitMaskIndex, WriteBackLen, Len0 - 2 );
				br.BitMaskIndex += WriteBackLen;
				assert( AllowExceptions == 1 || CountBitsRequiredStore( Len0-2 ) <= WriteBackLen );
			}
		}
	}
	unsigned __int64 SumLen = 0;
	unsigned __int64 SumLenCount = 0;
	for( int i=0;i<=MaxLen;i++)
	{
		unsigned __int64 RestCount = 0;
		for( int j=i+1;j<=MaxLen;j++)
			RestCount += br.symbol_counts[ j ];
		if( br.symbol_counts[ i ] > 0 )
		{
			unsigned __int64 PCTChanceToFindComparedToRemaining = br.symbol_counts[ i ] * 100 / ( br.symbol_counts[ i ] + RestCount );
			printf( "% 5d : % 8lld : %lld \n", i, br.symbol_counts[ i ], PCTChanceToFindComparedToRemaining );
			SumLen += i * br.symbol_counts[ i ];
			SumLenCount += br.symbol_counts[ i ];
		}
	}
	printf( "avg len * 100 %lld \n", SumLen * 100 / SumLenCount );
}

void bitreader_convert_next_block_runlength_stat_( bitreader &br )
{
	bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
	//sample sequances
/*	{
		bitreader_convert_next_block_runlength_stat_( br, 0, 5, 0 );
		bitreader_convert_next_block_runlength_stat_( br, 0, 5, 1 );
		bitreader_convert_next_block_runlength_stat_bidir( br, 0, 6 );
		bitreader_convert_next_block_dynlen_stat_( br, 0, 6, 0, 2 );
		bitreader_convert_next_block_dynlen_stat_( br, 0, 6, 1, 2 );
	}/**/
	{
		for( int i=0;i<=4;i++)
		{
			printf("\nTesting minlen runlength : %d\n",i);
			bitreader_convert_next_block_dynlen_stat_( br, 0, 6, 0, i, i + 4 );
			bitreader_convert_next_block_dynlen_stat_( br, 0, 6, 1, i, i + 4 );
		}
	}/**/
/*	{
		bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
		bitreader_swapsymbol( br, 2, 0, 2 );
		bitreader_convert_next_block_runlength_stat_( br, 0, 5, 0 );
		bitreader_convert_next_block_runlength_stat_( br, 0, 5, 1 );
		bitreader_convert_next_block_runlength_stat_bidir( br, 0 );
	}/**/
/*	{
		//817->448
		bitreader_convert_next_block_runlength_stat_( br, 1, 5, 1 );
		//dump it
		fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	}/**/
/*	{
		//779->457
		bitreader_convert_next_block_runlength_stat_bidir( br, 1, 6 );
		//dump it
		fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	}/**/
/*	{
		//663->436
		//cheat 1 bit 552->426
		bitreader_swapsymbol( br, 2, 0, 2 );
		bitreader_convert_next_block_runlength_stat_bidir( br, 1, 5 );
		//dump it
		fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	}/**/
/*	{
		//cheat 2 bit 527->414
		bitreader_convert_next_block_xormask_anylen( br, 0, 19, 19, 10, 1 );
		bitreader_stat_symbols( br, 1, 1, 2, 0, 1 );
		bitreader_swapsymbol( br, 2, 0, 2 );
		bitreader_convert_next_block_runlength_stat_bidir( br, 1, 4, 1 );
		//dump it
		fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
	}/**/
	//dump it
//	fwrite( br.cbitmask, 1, ( br.BitMaskIndex + 7 ) / 8, br.outf );
//	fwrite( br.cbuffer, 1, br.readcount, br.outf );
	//stop looping
	br.processed_count =  br.readcount;
	br.have_symbols = 0;
}

void DoBiaryTest(); 
void DoRubicTest();
void DoCostSwapTest();
void DoDistanceConvertTest();
//the idea is to eighter flip symbols at specific positions or flip at specific ranges 
//in order for this to have a gain we need to have symbol X to be present more then symbol Y
//and find a rule at which symbol Y is found more then symbol X. Making the flips would increase the ocurance of X in overall
void main()
{
//	DoBiaryTest();
//	DoAryTest();
//	DoRubicTest();
//    DoCostSwapTest();
	DoDistanceConvertTest();
	printf("Generate symbolocurance statistics\n");
	printf("Zip works on 24 bits!\n");

	bitreader br;
	Costs = (int*)malloc( sizeof( int ) * ( SYMBOL_COUNT_MAX + 1 ) );
	int CurSymBitCountMin=SYMBOL_BITCOUNT;
	int CurSymBitCountMax=SYMBOL_BITCOUNT;
//	for( int CurSymBitCountMax=SYMBOL_MIN_BITCOUNT_SAMPLE;CurSymBitCountMax<SYMBOL_BITCOUNT;CurSymBitCountMax++)
	{
//		bitreader_open(br,"data.zip",CurSymBitCountMax);
		bitreader_open(br,"data.rar",CurSymBitCountMax);
//		bitreader_open(br,"data1.rar",CurSymBitCountMax);
//		bitreader_open(br,"data1.txt",CurSymBitCountMax);
//		bitreader_open(br,"data2.rar",CurSymBitCountMax);
//		bitreader_open(br,"data1.wmv",CurSymBitCountMax);
		printf("Input File size %d MB = %d\n", br.FileSize/1024/1024, br.FileSize );
		printf("Advised minimum file size for this symbol set %f MB\n", br.SymbolCount/1024.0f/1024.0f);
		printf("Bitrange test %d - %d\n", CurSymBitCountMin, CurSymBitCountMax );
		do{
			while( br.readcount && br.readcount > br.processed_count + ( br.SymbolBitCount + 7 ) / 8 )
			{
//				bitreader_convert_next_block_xormask_( br );
//				bitreader_convert_next_block_xormask_specific_( br );
//				bitreader_convert_next_block_full_inverse_chance( br, 0 );
//				bitreader_convert_next_block_symbol_rearrange_( br, 0, br.SymbolBitCount, 0 );
//				bitreader_convert_next_block_replace1symbol_( br );
//				bitreader_convert_next_block_flip_if_avg_( br );
//				bitreader_convert_next_block_flip_if_avg_memory_( br );
//				bitreader_convert_next_block_flip_if_avg_memory2_( br );
//				BitWriterReaderTest( br );
//				bitreader_convert_next_block_detect_regions( br );
//				bitreader_convert_next_block_predictable( br );
//				bitreader_convert_next_block_rearrange_blockdata( br );
//				bitreader_convert_next_block_corelate_data( br );
//				bitreader_convert_next_block_XOR_then_detect_regions( br );
//				bitreader_convert_next_block_xormask_substract( br );
//				bitreader_convert_next_block_mask_remove( br );
//				bitreader_convert_next_block_random_flips( br );
//				bitreader_convert_next_block_remove( br );
//				bitreader_convert_next_block_subnext( br );
//				bitreader_convert_next_block_xor_some( br );
//				bitreader_convert_next_block_sum_memory( br );
//				bitreader_convert_next_block_sum_memory2( br );
//				bitreader_convert_next_block_sum_memory_bug( br );
//				bitreader_convert_next_block_stat_sym_dist_( br );
//				bitreader_convert_next_block_prime_mul_( br );
//				bitreader_convert_next_block_xor_longest_( br );
				bitreader_convert_next_block_runlength_stat_( br );

				if( br.TotalReadCount == br.FileSize && br.have_symbols > 0 )
					PrintStatistics( br );
			}
			// fill buffer initially
			br.readcount = (unsigned int)fread( br.cbuffer, 1, BITREADER_FILE_SEGMENT_BUFFER_SIZE, br.inf);
			if( br.readcount > 0 )
			{
				printf("Reading next buffer %u Kbytes\n",br.readcount/1024);
				br.TotalReadCount += br.readcount;
				// no blocks processed yet 
				br.processed_count = 0;
			}
		}while( br.readcount );

		bitreader_close( br );
	}
//	char c = getch();
	free( Costs );
}
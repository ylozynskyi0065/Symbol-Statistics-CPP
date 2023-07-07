#include "Biary.h"
#include "BitReader.h"
#include "BitWriter.h"
#include <Windows.h>
#include <conio.h>
#include "SymStats.h"

void InitBiary(BiaryStore *bs, int Ones, int Zeros)
{
	bs->Count1Total = Ones;
	bs->Count0Total = Zeros;
	bs->Count1Current = 0;
	bs->Count0Current = 0;
    bs->WrittenCountPattern = 0;
}

void WriteHeaderToFile(FILE *out, BiaryStore *bs)
{

}

int GuessNextSymbol(BiaryStore *bs, int InputSymbol)
{
	int ret;
    int WrittenCount = bs->Count1Current + bs->Count0Current;
    //the output is 01010101, but specific points, extra 1 is going to get inserted
    int PrevSymbol = (WrittenCount - 1) * (bs->Count1Total - bs->Count1Current) / (bs->Count0Total - bs->Count0Current + 1);
    int NextSymbol = WrittenCount * (bs->Count1Total - bs->Count1Current) / (bs->Count0Total - bs->Count0Current + 1);
    if (PrevSymbol == NextSymbol)
		ret = 1;
    else
    {
        static const unsigned char OuputPattern[2] = { 0, 1 };
        ret = OuputPattern[bs->WrittenCountPattern % 2];
        bs->WrittenCountPattern++;
    }
	if(InputSymbol == 1)
		bs->Count1Current++;
	else
		bs->Count0Current++;
	return ret;
}

int ShouldSkipProcessing(int CurBitIndex, int InitialSkips, int SkipCount)
{
	if (CurBitIndex < InitialSkips)
		return 1;
	CurBitIndex -= InitialSkips;
	if (SkipCount == 0 || CurBitIndex % SkipCount == 0)
		return 0;
	return 1;
}

void ProcessFile(const char *Input, const char *Output, int InitialSkips = 0, int SkipCount = 0)
{
	bitreader br;
	BitwriterStore bw;
	BiaryStore bs;
	bitreader_open(br, Input, 1);
    printf("Input File size %d MB = %d B for %s\n", br.FileSize / 1024 / 1024, br.FileSize, Input);
	//init bit writer
	InitBitwriter(&bw, br.FileSize + 8);
	//start reading the input and meantime process it
	do {
		int CountSymbol1 = 0;
		int CountSymbol0 = 0;
		int OutCountSymbol1 = 0;
		while (br.readcount && br.readcount > br.processed_count + (br.SymbolBitCount + 7) / 8)
		{
			//count number of symbols in input buffer
			int KeepNth = SkipCount + 1;
			for (int BitIndex = InitialSkips; BitIndex < br.readcount * 8; BitIndex++)
			{
				if (ShouldSkipProcessing(BitIndex, InitialSkips, KeepNth) == 1)
					continue;
				int Symbol = GetBitFromBuff(br.cbuffer, BitIndex);
				if (Symbol == 1)
					CountSymbol1++;
				else
					CountSymbol0++;
			}

			//init biary with symbol statistics. This should help us make the output have equal amount of symbols
			InitBiary(&bs, (int)CountSymbol1, CountSymbol0);

			//start guessing symbols
			for (int BitIndex = InitialSkips; BitIndex < br.readcount * 8; BitIndex++)
			{
				int StreamInputSymbol = GetBitFromBuff(br.cbuffer, BitIndex);
				int OutputSymbol;
				if (ShouldSkipProcessing(BitIndex, InitialSkips, KeepNth) == 0)
				{
					int GuessedSymbol = GuessNextSymbol(&bs, StreamInputSymbol);
					OutputSymbol = (StreamInputSymbol == GuessedSymbol);
					if (OutputSymbol == 1)
						OutCountSymbol1++;
				}
				else
					OutputSymbol = StreamInputSymbol;
				WriteBit(&bw, OutputSymbol);
			}

			// done with this chunk we just read
			br.processed_count += br.readcount;
		}

		printf("Started with %d ones, ended with %d ones, Diff %d, diff %f\n", (int)CountSymbol1, OutCountSymbol1, (int)CountSymbol1 - OutCountSymbol1, (float)CountSymbol1 / (float)OutCountSymbol1);
		br.readcount = (unsigned int)fread(br.cbuffer, 1, BITREADER_FILE_SEGMENT_BUFFER_SIZE, br.inf);
		if (br.readcount > 0)
		{
			printf("Reading next buffer %u Kbytes\n", br.readcount / 1024);
			br.TotalReadCount += br.readcount;
			// no blocks processed yet 
			br.processed_count = 0;
		}
	} while (br.readcount);
	//we are done processing input
    bitreader_close(br);
    //dump the output buffer into a file
    DumptToFile(&bw, Output);
    //free memory as we tend to run low on it :P
    BitwriterFree(&bw);
}

void DoBiaryTest()
{
	PrintStatisticsf("data.rar", 1);
    for (int i = 0; i < 3; i++)
    {
        ProcessFile("out.dat2", "out.dat2", i & 1);
        PrintStatisticsf("out.dat2", 1);
    }
	_getch();
	exit(0);

    /*
    count 0   : 1339439
    count 1   : 1398345
    Sum        : 2737784
    pct 0 / 1 : 0.957874 1.043978
    pct 0 / 1 : 0.489242 0.510758
    Input File size 0 MB = 342196 B for out.dat2
    Started with 1369373 ones, ended with 1368497 ones, Diff 876, diff 1.000640
    count 0   : 1369067
    count 1   : 1368493
    Sum        : 2737560
    pct 0 / 1 : 1.000419 0.999581
    pct 0 / 1 : 0.500105 0.499895
    Input File size 0 MB = 342196 B for out.dat2
    Started with 1368497 ones, ended with 1367992 ones, Diff 505, diff 1.000369
    count 0   : 1369568
    count 1   : 1367984
    Sum        : 2737552
    pct 0 / 1 : 1.001158 0.998843
    pct 0 / 1 : 0.500289 0.499711
    Input File size 0 MB = 342195 B for out.dat2
    Started with 1367987 ones, ended with 1368479 ones, Diff -492, diff 0.999640
    count 0   : 1369075
    count 1   : 1368477
    Sum        : 2737552
    pct 0 / 1 : 1.000437 0.999563
    pct 0 / 1 : 0.500109 0.499891
    */
}
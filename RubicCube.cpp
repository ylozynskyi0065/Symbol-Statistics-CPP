#include "BitReader.h"
#include <Windows.h>
#include <conio.h>
#include "SymStats.h"
#include <assert.h>

/*
Read symbols from file. At each step, replace the last read symbol with one that costs more bits
Chances are that each symbol will appear the same amount of time. If we keep updating our symbol library, there is a chance that high cost symbols will cost less
*/

#define Cube_Rows 256
#define Cube_Colls 256
#define SymbolBitCount 16
#define SymbolByteCount (SymbolBitCount/8)
#define CubeSizeMemoryBytes (sizeof(unsigned short) * (Cube_Rows + 2) * (Cube_Colls + 2))

int CubeCordToIndex(int x, int y)
{
    if (y >= Cube_Rows || y < -1)
    {
        printf("Invalid row value : %d\n", y);
        return 0;
    }
    if (x >= Cube_Colls || x < -1)
    {
        printf("Invalid column value : %d\n", x);
        return 0;
    }
    return (y * Cube_Colls + x);
}

void InitRubicCube(unsigned short* Cube)
{
	unsigned short* Cube_ = &Cube[CubeCordToIndex(-1, -1)];
    memset(Cube_, 0, CubeSizeMemoryBytes);
	for (int y = 0; y < Cube_Rows; y++)
		for (int x = 0; x < Cube_Colls; x++)
			Cube[CubeCordToIndex(x, y)] = CubeCordToIndex(x, y);
}

int GetBitCost(int Symbol)
{
	int BitCount = 0;
	while (Symbol != 0)
	{
		BitCount += (Symbol & 1);
		Symbol = Symbol >> 1;
	}
	return BitCount;
}

void EvolveCube(unsigned short* Cube, int atx, int aty)
{
	int WorstX = -1, WorstY = -1;
#define DefaultCostValue -1
    int WorstCost = DefaultCostValue;
    assert(atx >= 0 && atx < Cube_Colls);
    assert(aty >= 0 && aty < Cube_Rows);
    for (int y = aty - 1; y < aty + 1; y++)
		for (int x = atx - 1; x < atx + 1; x++)
		{
			int CostNow = GetBitCost(Cube[CubeCordToIndex(x, y)]);
            if (CostNow > WorstCost)
			{
				WorstCost = CostNow;
				WorstX = x;
				WorstY = y;
			}
		}
	//swap current cost with worst cost. Should have a smaller chance to trigger than the one we use now
    if (WorstCost != DefaultCostValue)
	{
		unsigned short OldValue = Cube[CubeCordToIndex(atx, aty)];
		unsigned short NewValue = Cube[CubeCordToIndex(WorstX, WorstY)];
		Cube[CubeCordToIndex(atx, aty)] = NewValue;
		Cube[CubeCordToIndex(WorstX, WorstY)] = OldValue;
	}
}

void SplitSymbolToXY(unsigned char* ReadBuff, int* x, int* y)
{
#if (SymbolBitCount==8)
    *x = ((int)ReadBuff[0] >> 0) & 0x0F;
	*y = ((int)ReadBuff[0] >> 4) & 0x0F;
#endif
#if (SymbolBitCount==16)
    *x = ReadBuff[0];
    *y = ReadBuff[1];
#endif
}

void CheckCubeIntegrity(unsigned short* Cube)
{
    int CubeSizeBytes = CubeSizeMemoryBytes;
    unsigned short *Cube_ = (unsigned short*)malloc(CubeSizeBytes+10);
    memset(Cube_, 0, CubeSizeBytes);
    for (int y = 0; y < Cube_Rows; y++)
        for (int x = 0; x < Cube_Colls; x++)
        {
            unsigned short CubeVal = Cube[CubeCordToIndex(x, y)];
            if (CubeVal >= (1 << SymbolBitCount))
                printf("Cube Integrity check failed 1\n");
            else 
                Cube_[CubeVal]++;
        }
    int LargestSymbol = (1 << SymbolBitCount);
    for (int i = 0; i < LargestSymbol; i++)
        if (Cube_[i] != 1)
            printf("Cube Integrity check failed 1 : Value for symbol %d is %d instead 1\n", i, Cube_[i]);
    free(Cube_);
}

void ProcessFile_Rubic(const char* Input, const char* Output, int InitialSkips = 0, int SkipCount = 0)
{
	FILE *inf, *outf;
	unsigned short* Cube_ = (unsigned short*)malloc(sizeof(unsigned short) * (Cube_Rows + 2) * (Cube_Colls + 2));
	unsigned short* Cube = &Cube_[CubeCordToIndex(1, 1)];
	InitRubicCube(Cube);
	errno_t er = fopen_s(&inf, Input, "rb");
	outf = fopen(Output, "wb");
	int x, y;
	do {
		size_t ReadC;
		unsigned char ReadBuff[16];
		ReadC = fread_s(ReadBuff, sizeof(ReadBuff), 1, SymbolByteCount, inf);
		if (ReadC != SymbolByteCount)
			break;
		SplitSymbolToXY(ReadBuff, &x, &y);
		int OldValue = Cube[CubeCordToIndex(x, y)];
		fwrite(&OldValue, 1, SymbolByteCount, outf);
        //roll the dice for a better world
        EvolveCube(Cube, x, y);
        //we always mess up. Detect and fix as soon as possible
#if (SymbolBitCount==8)
        CheckCubeIntegrity(Cube);
#endif
	} while (1);
	fclose(inf);
	fclose(outf);
	free(Cube_);
}

void DoRubicTest()
{
	PrintStatisticsf("data.rar", 1);
	ProcessFile_Rubic("data.rar", "out2.dat");
	PrintStatisticsf("out2.dat", 1);
	ProcessFile_Rubic("out2.dat", "out3.dat");
	PrintStatisticsf("out3.dat", 1);
	ProcessFile_Rubic("out3.dat", "out4.dat");
	PrintStatisticsf("out4.dat", 1);

	printf("Program will exit on keypress");
	_getch();
	exit(0);

    /*
    count 0   : 1339439
    count 1   : 1398345
    Sum        : 2737784
    pct 0 / 1 : 0.957874 1.043978
    pct 0 / 1 : 0.489242 0.510758
    count 0   : 1349260
    count 1   : 1388524
    Sum        : 2737784
    pct 0 / 1 : 0.971722 1.029100
    pct 0 / 1 : 0.492829 0.507171
    count 0   : 1348675
    count 1   : 1389109
    Sum        : 2737784
    pct 0 / 1 : 0.970892 1.029981
    pct 0 / 1 : 0.492616 0.507384
    count 0   : 1349109
    count 1   : 1388675
    Sum        : 2737784
    pct 0 / 1 : 0.971508 1.029328
    pct 0 / 1 : 0.492774 0.507226
    */
}
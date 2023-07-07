#include "BitReader.h"
#include <Windows.h>
#include <conio.h>
#include "SymStats.h"
#include <assert.h>

/*
Same as biary encoder, just based on 8 bit symbols
We presume each symbol will appear the same amount of times over large intervals. If one symbol appeared more than expected, we presume it has less chance to appear in the future
The lower the chance of a symbol to appear, the higher bitcost it can have. 
Incomming symbols are swapped out based on the chance they have to appear again in the file
*/

int GetBitCost(int Symbol);
namespace SymbolCostBasedSwap
{
    #define SymbolByteCount 1
    #define SymbolBitCount 8
    #define SymbolCount (1<<SymbolBitCount)
    static unsigned char SymbolBitCosts[SymbolCount];

    void FillSymbolCostVector()
    {
        for (int i = 0; i < SymbolCount; i++)
            SymbolBitCosts[i] = GetBitCost(i);
    }

    struct SymbolStatisticsForCost
    {
        int Symbol; //same as index
        int SymbolSwapTo;
        int FoundCount;
    };

    static int TotalSymbolsRead = 0;
    static SymbolStatisticsForCost SymStats[SymbolCount];
    void InitSymStats()
    {
        for (int i = 0; i < SymbolCount; i++)
        {
            SymStats[i].Symbol = i;
            SymStats[i].SymbolSwapTo = i;
            SymStats[i].FoundCount = 0;
        }
    }

    int GetSymbolForSymbol(int Symbol)
    {
        for (int i = 0; i < SymbolCount; i++)
            if (SymStats[i].Symbol == Symbol)
                return SymStats[i].SymbolSwapTo;
        return -1;
    }

    void UpdateSwapVector(int BufferSize, int SymbolRead)
    {
        TotalSymbolsRead++;
        int ExpectedCountForSymbolToAppear = BufferSize / SymbolCount;
        unsigned char SymbolBitCosts2[SymbolCount];
        memcpy(SymbolBitCosts2, SymbolBitCosts, sizeof(SymbolBitCosts2));
        for (int i = 0; i < SymbolCount; i++)
        {
            if (SymStats[i].Symbol == SymbolRead)
                SymStats[i].FoundCount++;
            // times already appeared / total times to appear
//            float ChanceToAppearAgain = (float)(ExpectedCountForSymbolToAppear-SymStats[i].FoundCount) * 100.0f / (float)ExpectedCountForSymbolToAppear;
            // based on chance, we should pick a new cost
            int ExpectedCountForSymbolToAppearRemaining = (ExpectedCountForSymbolToAppear - SymStats[i].FoundCount);
//            if (ExpectedCountForSymbolToAppearRemaining <= 0)
//                ExpectedCountForSymbolToAppearRemaining = 1;
            int TakeNewCostAround = SymbolBitCount * ExpectedCountForSymbolToAppearRemaining / ExpectedCountForSymbolToAppear;
TakeNewCostAround = SymbolBitCount - TakeNewCostAround;
            if (TakeNewCostAround > SymbolBitCount)
                TakeNewCostAround = SymbolBitCount;
            if (TakeNewCostAround < 0)
                TakeNewCostAround = 0;
            int NewSymbolToUse = -1;
            int BestCostDistance = 0xFF;
            for (int StartSearchAt = 0; StartSearchAt < SymbolCount; StartSearchAt++)
            {
                int CostDistance = abs(SymbolBitCosts2[StartSearchAt] - TakeNewCostAround);
                if (CostDistance < BestCostDistance)
                {
                    BestCostDistance = CostDistance;
                    NewSymbolToUse = StartSearchAt;
                }
            }
            assert(NewSymbolToUse != -1);

            //based on the chance it will pop up again in the near future, we will assign a new symbol for it
            SymbolBitCosts2[NewSymbolToUse] = 0xFF;
            SymStats[i].SymbolSwapTo = NewSymbolToUse;
        }
    }

    void ProcessFile_CostSwap(const char* Input, const char* Output, int InitialSkips = 0, int SkipCount = 0)
    {
        FILE *inf, *outf;
        errno_t er = fopen_s(&inf, Input, "rb");
        int FileSize = GetFileSize(inf);
        outf = fopen(Output, "wb");
        int x, y;
        do {
            size_t ReadC;
            unsigned char ReadBuff[16];
            ReadC = fread_s(ReadBuff, sizeof(ReadBuff), 1, SymbolByteCount, inf);
            if (ReadC != SymbolByteCount)
                break;
            int OldValue = GetSymbolForSymbol(ReadBuff[0]);
            fwrite(&OldValue, 1, SymbolByteCount, outf);
            //based on what we have, maybe we can guess the future
            UpdateSwapVector(FileSize, ReadBuff[0]);
        } while (1);
        fclose(inf);
        fclose(outf);
    }
};

void DoCostSwapTest()
{
    SymbolCostBasedSwap::FillSymbolCostVector();
    SymbolCostBasedSwap::InitSymStats();
    PrintStatisticsf("data.rar", 1);
    SymbolCostBasedSwap::ProcessFile_CostSwap("data.rar", "out2.dat");
    PrintStatisticsf("out2.dat", 1);
    SymbolCostBasedSwap::ProcessFile_CostSwap("out2.dat", "out3.dat");
    PrintStatisticsf("out3.dat", 1);
    SymbolCostBasedSwap::ProcessFile_CostSwap("out3.dat", "out4.dat");
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
    count 0   : 1368483
    count 1   : 1369301
    Sum        : 2737784
    pct 0 / 1 : 0.999403 1.000598
    pct 0 / 1 : 0.499851 0.500149
    count 0   : 1370666
    count 1   : 1367118
    Sum        : 2737784
    pct 0 / 1 : 1.002595 0.997411
    pct 0 / 1 : 0.500648 0.499352
    count 0   : 1367918
    count 1   : 1369866
    Sum        : 2737784
    pct 0 / 1 : 0.998578 1.001424
    pct 0 / 1 : 0.499644 0.500356
    */
}
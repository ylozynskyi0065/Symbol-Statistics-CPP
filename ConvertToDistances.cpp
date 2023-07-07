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
namespace ConvertSymbolsToDistances
{
#define SymbolByteCount 1
#define SymbolBitCount 8
#define SymbolCount (1<<SymbolBitCount)
#define MAX_SYMBOL_COUNT_INPUT_BUFFER   655350

    struct SymbolStatisticsForCost
    {
        int Symbol; //same as index
        int SymbolsFound;
        int DistanceUntilNextSymbol[MAX_SYMBOL_COUNT_INPUT_BUFFER];
    };

#if 0 // sadly this is just a random yolo try
    int GetSymbolToLookFor(int BuffLocation, int Symbol, int SymbolInital)
    {
        return (Symbol + 1) & 0xFF;
    }

    void ProcessBufferSwapSymbol(unsigned char* Buff, unsigned int BufferSize, FILE* outf)
    {
        if (BufferSize == 0)
            return;
        //init the temp buffer
        int* SymDistances;
        SymDistances = (int*)malloc(SymbolCount * MAX_SYMBOL_COUNT_INPUT_BUFFER * sizeof(int));
        if (SymDistances == NULL)
        {
            printf("Could not allocate memory.Exiting\n");
            return;
        }
        memset(SymDistances, 0, SymbolCount * MAX_SYMBOL_COUNT_INPUT_BUFFER * sizeof(int));
        int BufferSize2 = BufferSize;
        //process the input buffer
        int i = 0;
        int SymbolsFound = 0;
        int SymbolToLookFor = i;
        //search for the next location of the symbol
        while (BufferSize2 > 0)
        {
            int PrevLocation = 0;
            int SymbolsFoundTemp = 0;
            for (int BuffLocation = 0; BuffLocation < BufferSize2; BuffLocation++)
            {
                //shift remaining buffer to left
                if (SymbolsFound != 0)
                    Buff[BuffLocation - SymbolsFoundTemp] = Buff[BuffLocation];
                //is this a symbol we want to extract the distance for ?
                if (Buff[BuffLocation] == SymbolToLookFor)
                {
                    SymbolToLookFor = GetSymbolToLookFor(BuffLocation, SymbolToLookFor, i);
                    int Distance = BuffLocation - PrevLocation - 1;
                    if (BuffLocation == 0)
                        Distance = 0;
                    SymDistances[SymbolsFound] = Distance;
                    SymbolsFound++;
                    SymbolsFoundTemp++;
                    PrevLocation = BuffLocation;
                }
            }
            if(SymbolsFoundTemp == 0)
                SymbolToLookFor = GetSymbolToLookFor(0, SymbolToLookFor, i);
            //extracted symbols make the buffer smaller
            BufferSize2 = BufferSize - SymbolsFound;
        }

        //number of values we intend to write
        fwrite(&SymbolsFound, 1, 2, outf);
        //how many bytes do we need to write the distances
        int MaxDistance = -1;
        for (int j = 0; j < SymbolsFound; j++)
        {
            assert(SymDistances[j] >= 0);
            if (MaxDistance < SymDistances[j])
                MaxDistance = SymDistances[j];
        }
        int BytesForDistance = 0;
        if (MaxDistance & 0xFF000000)
            BytesForDistance = 4;
        else if (MaxDistance & 0x00FF0000)
            BytesForDistance = 3;
        else if (MaxDistance & 0x0000FF00)
            BytesForDistance = 2;
        else if (MaxDistance & 0x000000FF)
            BytesForDistance = 1;
        //write the distances
        for (int j = 0; j < SymbolsFound; j++)
            fwrite(&SymDistances[j], 1, BytesForDistance, outf);
        free(SymDistances);
    }
#endif

    void ProcessBuffer(unsigned char* Buff, unsigned int BufferSize, FILE*outf)
    {
        if (BufferSize == 0)
            return;
        //init the temp buffer
        SymbolStatisticsForCost *SymDistances;
        SymDistances = (SymbolStatisticsForCost*)malloc(SymbolCount * sizeof(SymbolStatisticsForCost));
        if (SymDistances == NULL)
        {
            printf("Could not allocate memory.Exiting\n");
            return;
        }
        memset(SymDistances, 0, SymbolCount * sizeof(SymbolStatisticsForCost));
        int BufferSize2 = BufferSize;
        //process the input buffer
        for (int i = 0; i < SymbolCount; i++)
        {
            int SymbolToLookFor = i;
            int PrevLocation = 0;
            int SymbolsFound = 0;
            //search for the next location of the symbol
            for (int BuffLocation = 0; BuffLocation < BufferSize2; BuffLocation++)
            {
                //shift remaining buffer to left
                if (SymbolsFound != 0)
                    Buff[BuffLocation - SymbolsFound] = Buff[BuffLocation];
                //is this a symbol we want to extract the distance for ?
                if (Buff[BuffLocation] == SymbolToLookFor)
                {
                    int Distance = BuffLocation - PrevLocation - 1;
                    if (BuffLocation == 0)
                        Distance = 0;
                    SymDistances[SymbolToLookFor].DistanceUntilNextSymbol[SymbolsFound] = Distance;
                    SymbolsFound++;
                    PrevLocation = BuffLocation;
                }
            }
            //extracted symbols make the buffer smaller
            SymDistances[SymbolToLookFor].SymbolsFound = SymbolsFound;
            BufferSize2 -= SymbolsFound;
        }

        //write the temp buffer
        for (int i = 0; i < SymbolCount; i++)
        {
            //number of values we intend to write
            fwrite(&SymDistances[i].SymbolsFound, 1, 2, outf);
            //never seen a case where min distance would not be 0
            {
                int MinDistance = 0x0FFFFFFF;
                for (int j = 0; j < SymDistances[i].SymbolsFound; j++)
                    if (MinDistance > SymDistances[i].DistanceUntilNextSymbol[j])
                        MinDistance = SymDistances[i].DistanceUntilNextSymbol[j];
                fwrite(&MinDistance, 1, 2, outf);
                if (MinDistance > 0 && MinDistance < 0x0FFFFFFF)
                {
                    for (int j = 0; j < SymDistances[i].SymbolsFound; j++)
                    {
                        SymDistances[i].DistanceUntilNextSymbol[j] -= MinDistance;
                    }
                }
            }/**/
            //how many bytes do we need to write the distances
            int MaxDistance = -1;
            for (int j = 0; j < SymDistances[i].SymbolsFound; j++)
            {
                assert(SymDistances[i].DistanceUntilNextSymbol[j] >= 0);
                if (MaxDistance < SymDistances[i].DistanceUntilNextSymbol[j])
                    MaxDistance = SymDistances[i].DistanceUntilNextSymbol[j];
            }
            int BytesForDistance = 0;
            if (MaxDistance & 0xFF000000)
                BytesForDistance = 4;
            else if (MaxDistance & 0x00FF0000)
                BytesForDistance = 3;
            else if (MaxDistance & 0x0000FF00)
                BytesForDistance = 2;
            else if (MaxDistance & 0x000000FF)
                BytesForDistance = 1;
            // Note that last symbol is not necesarry to write. It should be all 0,0,0
            if (i != SymbolCount - 1)
            {
                //write the distances
                for (int j = 0; j < SymDistances[i].SymbolsFound; j++)
                    fwrite(&SymDistances[i].DistanceUntilNextSymbol[j], 1, BytesForDistance, outf);
            }
        }
        free(SymDistances);
    }

    void ProcessFile(const char* Input, const char* Output, int InitialSkips = 0, int SkipCount = 0)
    {
        FILE* inf, * outf;
        errno_t er = fopen_s(&inf, Input, "rb");
        if (inf == NULL)
        {
            printf("Could not open input file %s. Exiting\n", Input);
            return;
        }
        int FileSize = GetFileSize(inf);
        outf = fopen(Output, "wb");
        if (inf == NULL)
        {
            printf("Could not open output file %s. Exiting\n", Output);
            return;
        }
        size_t ReadC;
        do {
            unsigned char ReadBuff[65535];
            ReadC = fread_s(ReadBuff, sizeof(ReadBuff), 1, sizeof(ReadBuff), inf);
            ProcessBuffer(ReadBuff, ReadC, outf);
        } while (ReadC != 0);
        fclose(inf);
        fclose(outf);
    }
};

void DoDistanceConvertTest()
{
    PrintStatisticsf("data.rar", 1);
    for (int i = 0; i < 1; i++)
    {
        char OutputFileName[500];
        sprintf_s(OutputFileName, sizeof(OutputFileName), "out%d.dat", i);
        ConvertSymbolsToDistances::ProcessFile("data.rar", OutputFileName);
        PrintStatisticsf(OutputFileName, 1);
    }

    printf("Program will exit on keypress");
    _getch();
    exit(0);

    /*
    count 0   : 1339439
    count 1   : 1398345
    Sum        : 2737784
    pct 0 / 1 : 0.957874 1.043978
    pct 0 / 1 : 0.489242 0.510758
    count 0   : 3840456
    count 1   : 1181976
    Sum        : 5022432
    pct 0 / 1 : 3.249183 0.307770
    pct 0 / 1 : 0.764661 0.235339
    */
}
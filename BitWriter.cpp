#include "BitWriter.h"
#include <windows.h>
#include <stdio.h>

void InitBitwriter(BitwriterStore* bw, int PreallocSize)
{
    bw->WriteIndex = 0;
    bw->Buffer = (unsigned char*)malloc(PreallocSize);
    memset(bw->Buffer, 0, PreallocSize);
    bw->BufferSize = PreallocSize;
}

void WriteBit(BitwriterStore* bw, int Symbol)
{
    //we already have the buffer zeroed, nothing to write here
    if (Symbol != 1)
    {
        bw->WriteIndex++;
        return;
    }
    unsigned int ByteIndex = bw->WriteIndex / 8;
    unsigned char BitIndex = bw->WriteIndex % 8;
    bw->WriteIndex++;
//    bw->Buffer[ByteIndex] = (bw->Buffer[ByteIndex] & (1 << BitIndex)) | (Symbol << BitIndex); // in case we intend to write multiple times on the same location
    bw->Buffer[ByteIndex] |= (Symbol << BitIndex);
}

void DumptToFile(BitwriterStore* bw, const char* FileName)
{
    unsigned int ByteIndex = bw->WriteIndex / 8;
    FILE *outf;
    outf = fopen(FileName, "wb");
    fwrite(bw->Buffer,1,ByteIndex,outf);
    fclose(outf);
}

void BitwriterFree(BitwriterStore* bw)
{
    free(bw->Buffer);
    bw->Buffer = NULL;
    bw->WriteIndex = -1;
    bw->BufferSize = 0;
}
#pragma once

struct BitwriterStore
{
    unsigned char *Buffer;
    int WriteIndex;
    int BufferSize;
};

void InitBitwriter(BitwriterStore* bw, int PreallocSize);
void WriteBit(BitwriterStore* bw, int Symbol);
void DumptToFile(BitwriterStore* bw, const char *FileName);
void BitwriterFree(BitwriterStore* bw);
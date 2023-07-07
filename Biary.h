#pragma once
#include <stdio.h>

struct BiaryStore
{
	//number of symbols we will have at the end
	int Count1Total;
	int Count0Total;
	//number of symbols we guessed so far
	int Count1Current;
	int Count0Current;
    int WrittenCountPattern;
};

void InitBiary(BiaryStore *bs, int Ones, int Zeros);
void WriteHeaderToFile(FILE *out, BiaryStore *bs);
int GuessNextSymbol(BiaryStore *bs);
void DoBiaryTest();
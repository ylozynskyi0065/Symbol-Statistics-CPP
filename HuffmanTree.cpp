#include <windows.h>
#include <stdio.h>
#include "HuffmanTree.h"

/*
void HuffmanTree::AddSymbol( int Symbol, int SymbolCount )
{
	if( MainNode == NULL )
	{
		MainNode = new HuffmanLeaf;
	}
	HuffmanLeaf *NewNode = new HuffmanLeaf;
	NewNode->ApearanceCount = SymbolCount;
	NewNode->Symbol = Symbol;

	HuffmanLeaf *CurNode = MainNode;
	HuffmanLeaf *PrevNode = NULL;
	while( CurNode )
	{
//		if( CurNode->Left && CurNode->Left->ApearanceCount )
	}
}*/

HuffmanTree::~HuffmanTree()
{
	delete MainNode;
	MainNode = NULL;
}

void HuffmanTree::BuildTree( __int64 *SymbolFrequencyList, int SymbolCount )
{
	//build initial list
	HuffmanLeaf	**LeafList1 = (HuffmanLeaf **)malloc( sizeof(HuffmanLeaf*) * SymbolCount );
	for( int i=0;i<SymbolCount;i++)
		if( SymbolFrequencyList[i] > 0 )
		{
			LeafList1[i] = new HuffmanLeaf;
			LeafList1[i]->ApearanceCount = SymbolFrequencyList[i];
			LeafList1[i]->Symbol = i;
		}
		else
			LeafList1[i] = NULL;

	__int64 Min1, Min1Val;
	__int64 Min2, Min2Val;

	//merge all symbols into 1 tree
	while( true )
	{
		Min1Val = SymbolCount + 1;
		Min2Val = SymbolCount + 1;
		Min1 = Min2 = -1;
		//get the next min
		for( int i=0;i<SymbolCount;i++)
		{
			if( LeafList1[i] != NULL && ( LeafList1[i]->ApearanceCount < Min1Val || Min1 == -1 ) )
			{
				Min1Val = LeafList1[i]->ApearanceCount;
				Min1 = i;
			}
		}
		//get the next min
		for( int i=0;i<SymbolCount;i++)
		{
			if( LeafList1[i] != NULL && ( LeafList1[i]->ApearanceCount < Min2Val || Min2 == -1 ) && i != Min1 )
			{
				Min2Val = LeafList1[i]->ApearanceCount;
				Min2 = i;
			}
		}

		if( Min1 == -1 || Min2 == -1 )
			break;

		//merge the 2 symbols
		HuffmanLeaf *NewLeaf = new HuffmanLeaf;
		NewLeaf->Left = LeafList1[Min1];
		NewLeaf->Right = LeafList1[Min2];
		NewLeaf->ApearanceCount = Min1Val + Min2Val;
		NewLeaf->Left->Owner = NewLeaf;
		NewLeaf->Right->Owner = NewLeaf;

		LeafList1[Min1] = NewLeaf;
		//do not search this symbols again
		LeafList1[Min2] = NULL;
	}
	//remember main node
	for( int i=0;i<SymbolCount;i++)
		if( LeafList1[i] != NULL )
		{
			MainNode = LeafList1[i];
			break;
		}
	free( LeafList1 );
}

void HuffmanTree::BuildTree( int *SymbolFrequencyList, int SymbolCount )
{
	//build initial list
	HuffmanLeaf	**LeafList1 = (HuffmanLeaf **)malloc( sizeof(HuffmanLeaf*) * ( SymbolCount + 1 ) );
	for( int i=0;i<SymbolCount;i++)
		if( SymbolFrequencyList[i] > 0 )
		{
			LeafList1[i] = new HuffmanLeaf;
			LeafList1[i]->ApearanceCount = SymbolFrequencyList[i];
			LeafList1[i]->Symbol = i;
		}
		else
			LeafList1[i] = 0;

	__int64 Min1, Min1Val;
	__int64 Min2, Min2Val;

	//merge all symbols into 1 tree
	while( true )
	{
		Min1Val = SymbolCount + 1;
		Min2Val = SymbolCount + 1;
		Min1 = Min2 = -1;
		//get the next min
		for( int i=0;i<SymbolCount;i++)
		{
			if( LeafList1[i] != NULL && LeafList1[i] != NULL && ( LeafList1[i]->ApearanceCount < Min1Val || Min1 == -1 ) )
			{
				Min1Val = LeafList1[i]->ApearanceCount;
				Min1 = i;
			}
		}
		//get the next min
		for( int i=0;i<SymbolCount;i++)
		{
			if( LeafList1[i] != NULL && LeafList1[i] != NULL && ( LeafList1[i]->ApearanceCount < Min2Val || Min2 == -1 ) && i != Min1 )
			{
				Min2Val = LeafList1[i]->ApearanceCount;
				Min2 = i;
			}
		}

		if( Min1 == -1 || Min2 == -1 )
			break;

		//merge the 2 symbols
		HuffmanLeaf *NewLeaf = new HuffmanLeaf;
		NewLeaf->Left = LeafList1[Min1];
		NewLeaf->Right = LeafList1[Min2];
		NewLeaf->ApearanceCount = Min1Val + Min2Val;
		NewLeaf->Left->Owner = NewLeaf;
		NewLeaf->Right->Owner = NewLeaf;

		LeafList1[Min1] = NewLeaf;
		//do not search this symbols again
		LeafList1[Min2] = NULL;
	}
	//remember main node
	for( int i=0;i<SymbolCount;i++)
		if( LeafList1[i] != NULL )
		{
			MainNode = LeafList1[i];
			break;
		}
	free( LeafList1 );
}

/*
int HuffmanTree::SearchSetSymbolCost( int Symbol )
{
	SearchinForSymbol = Symbol;
	return SearchSetSymbolCostPrivate( MainNode, 0 );
}
void HuffmanTree::SearchSetSymbolCost( int *SymbolCostList )
{
	SearchinForSymbols = SymbolCostList;
	SearchSetSymbolCostPrivate( MainNode, 0 );
}*/

int HuffmanTree::SearchSetSymbolCostPrivate( HuffmanLeaf *CurNode, int Cost )
{
	if( SearchinForSymbols != NULL && CurNode->Symbol != NON_VALID_SYMBOL )
	{
		SearchinForSymbols[ CurNode->Symbol ] = Cost;
	}
	if( CurNode->Symbol == SearchinForSymbol )
	{
		CurNode->BitCost = Cost;
		return Cost;
	}
	int ret = 0;
	if( CurNode->Left )
		ret = SearchSetSymbolCostPrivate( CurNode->Left, Cost + 1 );
	if( ret == 0 && CurNode->Right )
		ret = SearchSetSymbolCostPrivate( CurNode->Right, Cost + 1 );
	return ret;
}

void HuffmanTree::SearchSetSymbolCost( int *SymbolCostList, int SymbolCount )
{
	memset( SymbolCostList, 0, SymbolCount * sizeof( int ) );
	SearchinForSymbols = SymbolCostList;
	char *CallStack = (char *)malloc( SymbolCount );
	memset( CallStack, 0, SymbolCount );
	int CallDepth = 0;
	HuffmanLeaf	*CurNode = MainNode;
#define WALKER_STEP_BACK { CurNode = CurNode->Owner; CallStack[ CallDepth ] = 0; if( CallDepth > 0 ) CallDepth--; }
	while( CurNode != NULL )
	{
		if( SymbolCostList && CurNode->Symbol != NON_VALID_SYMBOL )
			SymbolCostList[ CurNode->Symbol ] = CallDepth;

		if( CallStack[ CallDepth ] == 0 )
		{
			if( CurNode->Left == NULL )
				CallStack[ CallDepth ]++;
			else
			{
				CurNode = CurNode->Left;
				CallStack[ CallDepth ]++;
				CallDepth++;
			}
		}
		else if( CallStack[ CallDepth ] == 1 )
		{
			if( CurNode->Right == NULL )
				CallStack[ CallDepth ]++;
			else
			{
				CurNode = CurNode->Right;
				CallStack[ CallDepth ]++;
				CallDepth++;
			}
		}
		else
		{
			CurNode = CurNode->Owner; 
			CallStack[ CallDepth ] = 0; 
			if( CallDepth > 0 ) 
				CallDepth--;
			else
				break;
		}
	}
	free( CallStack );
}

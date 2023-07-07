#ifndef _HUFMAN_H_
#define _HUFMAN_H_

#define NON_VALID_SYMBOL -1
struct HuffmanLeaf
{
	HuffmanLeaf() 
	{ 
		Symbol = NON_VALID_SYMBOL;
		ApearanceCount = 0;
		Left = Right = Owner = 0;
	}
	~HuffmanLeaf()
	{
		if( Left != NULL )
		{
			delete Left;
			Left = NULL;
		}
		if( Right != NULL )
		{
			delete Right;
			Right = NULL;
		}
	}
	int				Symbol;			//only valid for endnodes
	__int64			ApearanceCount;
	int				BitCost;		//only valid for nodes with a symbol
	HuffmanLeaf		*Left,*Right,*Owner;
};

class HuffmanTree
{
public:
	~HuffmanTree();
	HuffmanTree()
	{
		MainNode = 0;
	}
	void	BuildTree( int *SymbolFrequencyList, int SymbolCount );
	void	BuildTree( __int64 *SymbolFrequencyList, int SymbolCount );
//	int		SearchSetSymbolCost( int Symbol );
//	void	SearchSetSymbolCost( int *SymbolCostList );
	void	SearchSetSymbolCost( int *SymbolCostList, int SymbolCount );
private:
	int				SearchSetSymbolCostPrivate( HuffmanLeaf *CurNode, int Cost );
	HuffmanLeaf		*MainNode;
	int				SearchinForSymbol;
	int				*SearchinForSymbols;
};

#endif
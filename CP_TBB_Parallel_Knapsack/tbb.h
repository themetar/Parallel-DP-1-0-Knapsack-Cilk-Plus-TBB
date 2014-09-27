#pragma once

#include "stdafx.h"
#include "common.h"
#include <tbb\parallel_reduce.h>
#include <tbb\blocked_range.h>
#include <tbb\parallel_for.h>

/*
Functor object
*/
struct Knapsack{
	int maxCapacity,
		numElements,
		maxElements;

	int * indices;

	int	*values,
		*weights;

	int * * profits;

	ReductionNode * reductionTree;

	bool useMapInReduce;

	/*
	constructor
	*/
	Knapsack(int c, int e, int *v, int *w, int **p, bool u=false);

	/*
	spliting constructor
	*/
	Knapsack(Knapsack& s, tbb::split);

	/*
	destructor
	*/
	~Knapsack();

	/*
	'()' operator - implments the map portion
	*/
	void operator()(const tbb::blocked_range<int>& r);

	/*
	join method - implements the reduce portion
	*/
	void join(Knapsack& rhs);

};

/*
Function that implements the mapreduce knapsack algorithm [with TBB]
*/
int mapreduceKnapsackTBB(int size, int capacity, int* values, int* weights, int** profits, int* solution=nullptr);

/*
Function that implements the map knapsack algorithm [with TBB]
*/
void mapKnapsackTBB(int size, int capacity, int* values, int* weights, int** profits);


/*
Function that implements the mapreduce algorithm - and uses another map in the reducing function - [with TBB]
*/
int mapreduceMapKnapsackTBB(int size, int capacity, int* values, int* weights, int** profits, int* solution=nullptr);

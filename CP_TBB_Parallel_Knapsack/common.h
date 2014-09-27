#pragma once

#include <minmax.h>
#include <vector>
using namespace std;

/*
Node for a binary tree that hold the reduction history of knapsack subproblems.
It's needed for solution trackback.
*/
struct ReductionNode{
	int iLength;
	int * reductionIndices;
	int * reductionProfits;
	ReductionNode * left, *right;

	ReductionNode();
	~ReductionNode();
} ;

/*
Solution backtracking from the Reduction Node tree structure and profits matrix.
*/
void treeBacktrackSolution(ReductionNode *root, int **profits, int *weights, int *values, int capacity, int size, int *indices, int *solution);

/*
Serial solution backtracking from the profits matrix. 
*/
void plainBacktrackSolution(int **profits, int *weights, int *values, int capacity, int size, int *solution);
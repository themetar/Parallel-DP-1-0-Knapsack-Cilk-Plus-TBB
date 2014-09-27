#include "stdafx.h"
#include "tbb.h"

Knapsack::Knapsack(int c, int e, int *v, int *w, int **p, bool u) : maxCapacity(c), maxElements(e), values(v), weights(w), profits(p), numElements(0), reductionTree(nullptr), useMapInReduce(u) {
	indices = new int[maxElements];
}

Knapsack::Knapsack(Knapsack& s, tbb::split) : Knapsack(s.maxCapacity, s.maxElements, s.values, s.weights, s.profits) {}

Knapsack::~Knapsack(){
	delete[] indices;
	delete reductionTree;
}

void Knapsack:: operator()(const tbb::blocked_range<int>& r) {
	// three posibilities:
	// 1) this is a fresh Body
	// 2) reused Body, but not previously joined
	// 3) reused Body and previously joined
	bool fresh = numElements == 0;
	bool joined = !fresh && (reductionTree->reductionProfits != nullptr);

	int i = 0;
	for (int currentIndex = r.begin(); currentIndex != r.end(); ++i, ++currentIndex){
		// add index
		indices[numElements] = currentIndex;
		// calculate profits; i.e. "forward phase"
		for (int c = 0; c <= maxCapacity; c++){
			if (i == 0 && (fresh || joined)){
				profits[currentIndex][c] = weights[currentIndex] > c ? 0 : values[currentIndex];
			}
			else{
				int prevIndex = indices[numElements - 1];
				if (weights[currentIndex] > c){
					profits[currentIndex][c] = profits[prevIndex][c];
				}
				else{
					profits[currentIndex][c] = max(profits[prevIndex][c], profits[prevIndex][c - weights[currentIndex]] + values[currentIndex]);
				}
			}
		}
		// increment numElements
		++numElements;
	}

	if (fresh){
		// create leaf node
		reductionTree = new ReductionNode();
		reductionTree->iLength = numElements;
	}

	if (!joined){
		// not fresh, but also not joined
		// update leaf
		reductionTree->iLength = numElements;
	}

	if (joined){
		// Note: not sure if this can happen; the documentation is not quite clear
		// anyway,

		// if body has previously been joined, do another join
		// create root node
		ReductionNode *root = new ReductionNode();
		root->reductionIndices = new int[maxCapacity + 1];
		root->reductionProfits = new int[maxCapacity + 1];
		// select profit sources
		int *leftProfits = reductionTree->reductionProfits;
		int *rightProfits = profits[r.end() - 1]; // last row of profits
		// calculate joint profits (root's profit and index vector)
		for (int c = maxCapacity; c >= 0; --c){
			int max = __sec_reduce_max(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);
			int maxindex = __sec_reduce_max_ind(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);
			root->reductionProfits[c] = max;
			root->reductionIndices[c] = maxindex;
		}
		// update reduction Tree
		root->left = reductionTree;
		root->right = new ReductionNode(); // leaf node
		root->right->iLength = r.size(); // set iLength to leaf
		reductionTree = root;
	}
}

void Knapsack::join(Knapsack& rhs) {
	// get sizes
	int thissize = numElements;
	int rightsize = rhs.numElements;

	// update numElements
	numElements = thissize + rightsize;

	// update indices
	for (int i = thissize; i < numElements; i++) indices[i] = rhs.indices[i - thissize];

	// select profits sources
	int *leftProfits = (reductionTree->reductionProfits == nullptr) ? profits[indices[thissize - 1]] : reductionTree->reductionProfits;
	int *rightProfits = (rhs.reductionTree->reductionProfits == nullptr) ? profits[rhs.indices[rightsize - 1]] : rhs.reductionTree->reductionProfits;

	// update tree node
	ReductionNode * tmp = reductionTree;
	reductionTree = new ReductionNode();
	reductionTree->reductionIndices = new int[maxCapacity + 1];
	reductionTree->reductionProfits = new int[maxCapacity + 1];
	reductionTree->left = tmp;
	reductionTree->right = rhs.reductionTree;
	rhs.reductionTree = nullptr;

	// calculate joint profits
	if (useMapInReduce){
		// mapped
		tbb::parallel_for(tbb::blocked_range<int>(0, maxCapacity / 2 + 1),
			[=](tbb::blocked_range<int> r){	// lambda function
			for (int i = r.begin(); i != r.end(); ++i){
				int c = i;
				int max = __sec_reduce_max(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);
				int maxindex = __sec_reduce_max_ind(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);

				reductionTree->reductionProfits[c] = max;
				reductionTree->reductionIndices[c] = maxindex;
				//
				//
				c = maxCapacity - i;
				max = __sec_reduce_max(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);
				maxindex = __sec_reduce_max_ind(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);

				reductionTree->reductionProfits[c] = max;
				reductionTree->reductionIndices[c] = maxindex;
			}
		}
			);
	}
	else{
		for (int c = maxCapacity; c >= 0; --c){
			int max = __sec_reduce_max(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);
			int maxindex = __sec_reduce_max_ind(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);
			reductionTree->reductionProfits[c] = max;
			reductionTree->reductionIndices[c] = maxindex;
		}
	}
}


int mapreduceKnapsackTBB(int size, int capacity, int* values, int* weights, int** profits, int* solution){
	// setup functor
	Knapsack knapsack(capacity, size, values, weights, profits);
	// call parallel_reduce
	tbb::parallel_reduce(tbb::blocked_range<int>(0, size), knapsack);

	if (solution != nullptr){
		// do backtrack for solution
		treeBacktrackSolution(knapsack.reductionTree, profits, weights, values, capacity, size, knapsack.indices, solution);
	}

	return knapsack.reductionTree->reductionProfits != nullptr ? knapsack.reductionTree->reductionProfits[capacity] : profits[size - 1][capacity];
}

void mapKnapsackTBB(int size, int capacity, int* values, int* weights, int** profits){
	// forward phase:
	// call parallel_for with lambda function for first row
	tbb::parallel_for(tbb::blocked_range<int>(0, capacity),
		[=](tbb::blocked_range<int> r){
		for (int c = r.begin(); c != r.end(); ++c){
			profits[0][c] = weights[0] < c ? values[0] : 0; // init vals, for first item
		}
	});

	for (int k = 1; k < size; k++){		// serially go for every element
		// set profits in parallel
		tbb::parallel_for(tbb::blocked_range<int>(0, capacity),
			[=](tbb::blocked_range<int> r){
			for (int c = r.begin(); c != r.end(); ++c){
				if (weights[k] > c){
					profits[k][c] = profits[k - 1][c];
				}
				else{
					profits[k][c] = max(profits[k - 1][c], profits[k - 1][c - weights[k]] + values[k]);
				}
			}
		}
		);
	}
}


int mapreduceMapKnapsackTBB(int size, int capacity, int* values, int* weights, int** profits, int* solution){
	// setup functor. now with 'true' as last parameter, meaning to use map when joining
	Knapsack knapsack(capacity, size, values, weights, profits, true);
	// call parallel_reduce
	tbb::parallel_reduce(tbb::blocked_range<int>(0, size), knapsack);

	if (solution != nullptr){
		// do backtrack for solution
		treeBacktrackSolution(knapsack.reductionTree, profits, weights, values, capacity, size, knapsack.indices, solution);
	}

	return knapsack.reductionTree->reductionProfits != nullptr ? knapsack.reductionTree->reductionProfits[capacity] : profits[size - 1][capacity];
}
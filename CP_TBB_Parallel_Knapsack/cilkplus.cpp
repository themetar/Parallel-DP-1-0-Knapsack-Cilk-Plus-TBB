#include "stdafx.h"
#include "cilkplus.h"

/* config */

config::config() :useMapInReduce(false){}


config::config(int _maxElements, int _maxCapacity, int ** _profits, int * _values, int * _weights, bool _useMapInReduce) :
maxElements(_maxElements), maxCapacity(_maxCapacity), profits(_profits), values(_values), weights(_weights), useMapInReduce(_useMapInReduce){}


/* Wrapper class */

KnapsackReducer::View::View(config conf) : View(conf.maxElements) { }

KnapsackReducer::View::View(int max_e) : numElements(0){
	indices = new int[max_e];
	reductionTree = new ReductionNode();
}

KnapsackReducer::View :: ~View(){
	delete[] indices;
	delete reductionTree;
}

void KnapsackReducer::View::addElement(int id){
	indices[numElements] = id;				// "push" to array
	++numElements;							// update numElements
	reductionTree->iLength = numElements;	// update iLength
}

/* Monoid struct */

KnapsackReducer::Monoid::Monoid(config &v) : capacity(v.maxCapacity), elements(v.maxElements), values(v.values), weights(v.weights), dpProfits(v.profits), useMap(v.useMapInReduce) {}

void KnapsackReducer::Monoid::identity(View* p) const {
	new(p)View(elements);
}

void KnapsackReducer::Monoid::reduce(View* left, View* right) const {
	// get the sizes of the left and right view
	int leftsize = left->numElements;
	int rightsize = right->numElements;

	// update numElements of the left one - thats the one that the right will be merged in
	left->numElements = leftsize + rightsize;

	// update indices
	for (int i = leftsize; i < leftsize + rightsize; i++) left->indices[i] = right->indices[i - leftsize];

	// select profit sources: depending on whether the left view is already a result of a previous reduction 
	int *leftProfits = (left->reductionTree->reductionProfits == nullptr) ? dpProfits[left->indices[leftsize - 1]] : left->reductionTree->reductionProfits;
	int *rightProfits = (right->reductionTree->reductionProfits == nullptr) ? dpProfits[right->indices[rightsize - 1]] : right->reductionTree->reductionProfits;

	// update tree node
	ReductionNode * tmp = left->reductionTree;
	left->reductionTree = new ReductionNode();
	left->reductionTree->reductionIndices = new int[capacity + 1];
	left->reductionTree->reductionProfits = new int[capacity + 1];
	left->reductionTree->left = tmp;
	left->reductionTree->right = right->reductionTree;
	right->reductionTree = nullptr;
	
	// calculate the combined profits from the two subproblems
	if (useMap){
		// use cilk_for
		// calculate profit for c=0, c=max ; c=1, c=max-1 ; etc... in one iteration to equalize the load in all parallel tasks
		cilk_for(int i = 0; i <= capacity / 2; ++i){
			int c = i;
			int max = __sec_reduce_max(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);				// __sec_reduce_max and array notation tell the compiler that it can use SIMD instructions
			int maxindex = __sec_reduce_max_ind(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);

			left->reductionTree->reductionProfits[c] = max;
			left->reductionTree->reductionIndices[c] = maxindex;
			//
			//
			c = capacity - i;
			max = __sec_reduce_max(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);
			maxindex = __sec_reduce_max_ind(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);

			left->reductionTree->reductionProfits[c] = max;
			left->reductionTree->reductionIndices[c] = maxindex;
		}
	}
	else{
		// use serial for
		for (int c = 0; c <= capacity; ++c){
			int max = __sec_reduce_max(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);
			int maxindex = __sec_reduce_max_ind(leftProfits[0:c + 1 : 1] + rightProfits[c:c + 1 : -1]);

			left->reductionTree->reductionProfits[c] = max;
			left->reductionTree->reductionIndices[c] = maxindex;

		}
	}
}

KnapsackReducer::KnapsackReducer(config &conf) : impl(Monoid(conf), conf) {}

void KnapsackReducer::addElement(int id){
	// get current view and add to it
	impl.view().addElement(id);
}

int * KnapsackReducer::indices(){
	return impl.view().indices;
}

int KnapsackReducer::numElements(){
	return impl.view().numElements;
}

ReductionNode * KnapsackReducer::historyTree(){
	return impl.view().reductionTree;
}



int mapreduceKnapsackCilkplus(int size, int capacity, int* values, int* weights, int** profits, int* solution){
	// setup reducer
	config conf(size, capacity, profits, values, weights);
	KnapsackReducer profit_reducer(conf);

	cilk_for(int k = 0; k < size; ++k){

		int *indices = profit_reducer.indices();
		int numElements = profit_reducer.numElements();

		// calculate profits; i.e. "forward phase"
		if (numElements == 0){
			for (int c = 0; c <= capacity; c++)	profits[k][c] = weights[k] > c ? 0 : values[k];
		}
		else{
			for (int c = 0; c <= capacity; c++){
				if (weights[k] > c){
					profits[k][c] = profits[indices[numElements - 1]][c];
				}
				else{
					profits[k][c] = max(profits[indices[numElements - 1]][c], profits[indices[numElements - 1]][c - weights[k]] + values[k]);
				}
			}
		}
		profit_reducer.addElement(k);
	}

	if (solution != nullptr){
		// do backtrack for solution
		treeBacktrackSolution(profit_reducer.historyTree(), profits, weights, values, capacity, size, profit_reducer.indices(), solution);
	}

	return profit_reducer.historyTree()->reductionProfits != nullptr ? profit_reducer.historyTree()->reductionProfits[capacity] : profits[size - 1][capacity];
}

void mapKnapsackCilkplus(int size, int capacity, int* values, int* weights, int** profits){
	cilk_for(int c = 0; c <= capacity; ++c) profits[0][c] = weights[0] > c ? 0 : values[0]; // first row, i.e. first item

	for (int k = 1; k < size; ++k){
		cilk_for(int c = 0; c <= capacity; ++c){
			if (weights[k] > c){
				profits[k][c] = profits[k - 1][c];
			}
			else{
				profits[k][c] = max(profits[k - 1][c], profits[k - 1][c - weights[k]] + values[k]);
			}
		}
	}
	//
}

int mapreduceMapKnapsackCilkplus(int size, int capacity, int* values, int* weights, int** profits, int* solution){
	//// same as mapreduce except reducer is configured to use second map
	
	config conf(size, capacity, profits, values, weights, true);
	KnapsackReducer profit_reducer(conf);
	cilk_for(int k = 0; k < size; ++k){
		//
		int *indices = profit_reducer.indices();
		int numElements = profit_reducer.numElements();

		// calculate profits; i.e. "forward phase"
		if (numElements == 0){
			for (int c = 0; c <= capacity; c++)	profits[k][c] = weights[k] > c ? 0 : values[k];
		}
		else{
			for (int c = 0; c <= capacity; c++){
				if (weights[k] > c){
					profits[k][c] = profits[indices[numElements - 1]][c];
				}
				else{
					profits[k][c] = max(profits[indices[numElements - 1]][c], profits[indices[numElements - 1]][c - weights[k]] + values[k]);
				}
			}
		}
		profit_reducer.addElement(k);
	}

	if (solution != nullptr){
		// do backtrack for solution
		treeBacktrackSolution(profit_reducer.historyTree(), profits, weights, values, capacity, size, profit_reducer.indices(), solution);
	}

	return profit_reducer.historyTree()->reductionProfits != nullptr ? profit_reducer.historyTree()->reductionProfits[capacity] : profits[size - 1][capacity];
}
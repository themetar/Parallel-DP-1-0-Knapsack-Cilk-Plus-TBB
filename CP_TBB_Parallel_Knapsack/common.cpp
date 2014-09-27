#include "stdafx.h"
#include "common.h"

ReductionNode :: ReductionNode() : reductionIndices(nullptr), reductionProfits(nullptr), left(nullptr), right(nullptr){}
ReductionNode :: ~ReductionNode(){
	delete[] reductionIndices;
	delete[] reductionProfits;
	delete left;
	delete right;
}

void treeBacktrackSolution(ReductionNode *root, int **profits, int *weights, int *values, int capacity, int size, int *indices, int *solution){
	ReductionNode *node = root;				// set current node to root
	int h = 0;								// sub-indices handle to 0
	int ci = capacity;						// current target capacity to final capacity
	vector<int> capacity_stack;				// setup search stacks
	vector<ReductionNode*> node_stack;

	do {
		if (node->reductionIndices != nullptr){		// not a leaf node
			int newci = node->reductionIndices[ci];
			capacity_stack.push_back(ci - newci); // push right index
			node_stack.push_back(node->right); // push right node
			// go to left in next loop
			ci = newci;
			node = node->left;
		}
		else{
			// leaf node
			// backtrack solution
			for (int m = h + node->iLength - 1; m > h; --m){
				if (weights[indices[m]] > ci){
					solution[indices[m]] = 0;
				}
				else{
					if (profits[indices[m - 1]][ci - weights[indices[m]]] == profits[indices[m]][ci] - values[indices[m]]){
						solution[indices[m]] = 1;
						ci -= weights[indices[m]];
					}
					else{
						solution[indices[m]] = 0;
					}
				}

			}
			// for m = h
			if (profits[indices[h]][ci] == values[indices[h]]){ solution[indices[h]] = 1; }
			else { solution[indices[h]] = 0; }

			// set next h
			h += node->iLength;

			// get stack
			if (capacity_stack.size() != 0){
				ci = capacity_stack.back();
				capacity_stack.pop_back();
				node = node_stack.back();
				node_stack.pop_back();
			}
			else{
				node = nullptr;
			}
		}
	} while (node != nullptr);
}

void plainBacktrackSolution(int **profits, int *weights, int *values, int capacity, int size, int *solution){
	for (int k = size - 1; k > 0; --k){
		if (weights[k] > capacity){
			solution[k] = 0;
		}
		else{
			if (profits[k - 1][capacity - weights[k]] == profits[k][capacity] - values[k]){
				solution[k] = 1;
				capacity -= weights[k];
			}
			else{
				solution[k] = 0;
			}
		}
	}
	if (profits[0][capacity] == values[0]){ solution[0] = 1; }
	else { solution[0] = 0; }
}
#include "stdafx.h"
#include "serial.h"

void serialKnapsackDP(int capacity, int size, int *weights, int *values, int **profits, int* solution){
	// forward phase
	for (int c = 0; c <= capacity; ++c) profits[0][c] = weights[0] > c ? 0 : values[0]; // first item, k=0
	for (int k = 1; k < size; ++k){
		for (int c = 0; c <= capacity; ++c){
			if (weights[k] > c){
				profits[k][c] = profits[k-1][c];
			}
			else{
				profits[k][c] = max(profits[k - 1][c], profits[k - 1][c - weights[k]] + values[k]);
			}
		}
	}

	// optional backtrack
	if (solution != nullptr){
		plainBacktrackSolution(profits, weights, values, capacity, size, solution);
	}
}
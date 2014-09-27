#pragma once

#include "stdafx.h"
#include "common.h"

/*
Implements the serial dynamic programming knapsack algorithm.
*/
void serialKnapsackDP(int capacity, int size, int *weights, int *values, int **profits, int* solution = nullptr);
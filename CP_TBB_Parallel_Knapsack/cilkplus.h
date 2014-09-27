#pragma once

#include "stdafx.h"
#include "common.h"
#include <cilk\reducer.h>
#include <cilk\cilk.h>

/*
configuration structure to pass knapsack properties to the reducer class
*/
struct config {
	int	*values,
		*weights;

	int * * profits;

	int maxCapacity,
		maxElements;

	bool useMapInReduce;

	config();
	config(int _maxElements, int _maxCapacity, int ** _profits, int * _values, int * _weights, bool _useMapInReduce = false);
} ;

/*
Wrapper class
*/
class KnapsackReducer {

protected:

	/*
	View struct
	*/
	struct View {

		int numElements;

		int * indices;

		ReductionNode * reductionTree;

		/*
		constructors
		*/
		View(config conf);
		View(int max_e); 
		~View();

		/* 
		adds the knapsack element index, passed as 'id', to the indices array 
		*/
		void addElement(int id);

	};

	/*
	Monoid struct
	*/
	struct Monoid : cilk::monoid_base < View > {
		const int capacity, elements;
		int *values, *weights, **dpProfits;
		bool useMap;

		/* 
		constructor for Monoid
		*/
		Monoid(config &v);

		/*
		creates an identity element at the pointer
		*/
		void identity(View* p) const;

		/* the reduction function, called when two tasks complete and their results are to be merged */
		void reduce(View* left, View* right) const;
	};

	cilk::reducer<Monoid> impl;

public:
	/*
	constructor
	*/
	KnapsackReducer(config &conf);

	/* adds the knapsack element index, passed as 'id', to the current view */
	void addElement(int id);

	/*
	get current view's indices
	*/
	int *indices();

	/*
	get current view's number of elements
	*/
	int numElements();

	/*
	get current view's reduction history tree
	*/
	ReductionNode * historyTree();
};

/*
Function that implements the mapreduce algorithm [with Cilk Plus]
*/
int mapreduceKnapsackCilkplus(int size, int capacity, int* values, int* weigths, int** profits, int* solution = nullptr);

/*
Function that implements the map algorithm [with Cilk Plus]
*/
void mapKnapsackCilkplus(int size, int capacity, int* values, int* weights, int** profits);

/*
Function that implements the mapreduce algorithm - and uses another map in the reducing function - [with Cilk Plus]
*/
int mapreduceMapKnapsackCilkplus(int size, int capacity, int* values, int* weights, int** profits, int* solution = nullptr);
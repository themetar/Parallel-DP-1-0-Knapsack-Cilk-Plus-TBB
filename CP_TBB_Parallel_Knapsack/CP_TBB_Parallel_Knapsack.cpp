// CP_TBB_Paralell_Knapsack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "common.h"
#include "cilkplus.h"
#include "tbb.h"
#include "serial.h"
#include <Windows.h>
#include <fstream>
#include <iostream>

#define MAXK_T 15
#define MAXC_T 750
#define NUMPOINTS 14
#define MAXKC 20000
#define ALGORITHMS 8
#define MAXITERS 25

/* Simple function that prints an array to the standard output. */
void printArray(int * array, int size){
	for (int i = 0; i < size; ++i) cout << array[i];
}


int _tmain(int argc, _TCHAR* argv[])
{
	// types of tests that can be run
	string names[ALGORITHMS] = { "correctness test", "mapreduce CilkPlus", "mapreduce TBB", "map CilkPlus", "map TBB", "serial", "mapreduce+map CilkPlus", "mapreduce+map TBB" };
	// input var to choose test
	int kind;

	// prompt for input
	cout << "Algorithm?\n";
	for (int i = 0; i < ALGORITHMS; ++i){
		cout << i << " ) " << names[i] << "\n";
	}
	cin >> kind;

	// check valid input
	if (kind < 0 || kind >= ALGORITHMS){
		// print explanation
		cout << "You must select 0 to " << ALGORITHMS << "\n";
		return 0;
	}


	/* If the user selected 0, run correctness test*/
	if (kind == 0){
		// allocate memory for dynamic algorithm
		int ** profits_t = new int*[MAXK_T];
		for (int i = 0; i < MAXK_T; i++) profits_t[i] = new int[MAXC_T + 1];

		// Problem A
		int sA = 10;												// size
		int wA[] = { 27, 2, 41, 1, 25, 1, 34, 3, 50, 12 };			// weights
		int vA[] = { 38, 86, 112, 0, 66, 97, 195, 85, 42, 223 };	// values
		int cA = 100;												// maximum knapsack capacity

		// Problem B
		int sB = 15;
		int wB[] = { 70, 73, 77, 80, 82, 87, 90, 94, 98, 106, 110, 113, 115, 118, 120 };
		int vB[] = { 135, 139, 149, 150, 156, 163, 173, 184, 192, 201, 210, 214, 221, 229, 240 };
		int cB = 750;

		// allocate memory for solution array
		int * solution = new int[max(sA, sB)];

		/* run all algorithms for problem A */
		cout << "problem A > n= 10, c= 100; profit= 798, solution= 0111011101 (or 0110011101)\n";

		int p;
		for (int i = 1; i < ALGORITHMS; ++i){
			cout << names[i] << ":\n";
			if (i == 1){
				p = mapreduceKnapsackCilkplus(sA, cA, vA, wA, profits_t, solution);
			}
			if (i == 2){
				p = mapreduceKnapsackTBB(sA, cA, vA, wA, profits_t, solution);
			}
			if (i == 3){
				mapKnapsackCilkplus(sA, cA, vA, wA, profits_t);
				p = profits_t[sA - 1][cA];

			}
			if (i == 4){
				mapKnapsackTBB(sA, cA, vA, wA, profits_t);
				p = p = profits_t[sA - 1][cA];
			}
			if (i == 5){
				serialKnapsackDP(cA, sA, wA, vA, profits_t, solution);
				p = profits_t[sA - 1][cA];
			}
			if (i == 6){
				p = mapreduceMapKnapsackCilkplus(sA, cA, vA, wA, profits_t, solution);
			}
			if (i == 7){
				p = mapreduceMapKnapsackTBB(sA, cA, vA, wA, profits_t, solution);
			}
			cout << "profit= " << p << ", solution= ";
			printArray(solution, sA);
			cout << "\n";
		}
		cout << "\n";

		/* run all algorithms for problem B */
		cout << "problem B > n= 15, c= 750; profit= 1458, solution= 101010111000011\n";
		
		for (int i = 1; i < ALGORITHMS; ++i){
			cout << names[i] << ":\n";
			if (i == 1){
				p = mapreduceKnapsackCilkplus(sB, cB, vB, wB, profits_t, solution);
			}
			if (i == 2){
				p = mapreduceKnapsackTBB(sB, cB, vB, wB, profits_t, solution);
			}
			if (i == 3){
				mapKnapsackCilkplus(sB, cB, vB, wB, profits_t);
				p = profits_t[sB - 1][cB];

			}
			if (i == 4){
				mapKnapsackTBB(sB, cB, vB, wB, profits_t);
				p = p = profits_t[sB - 1][cB];
			}
			if (i == 5){
				serialKnapsackDP(cB, sB, wB, vB, profits_t, solution);
				p = profits_t[sB - 1][cB];
			}
			if (i == 6){
				p = mapreduceMapKnapsackCilkplus(sB, cB, vB, wB, profits_t, solution);
			}
			if (i == 7){
				p = mapreduceMapKnapsackTBB(sB, cB, vB, wB, profits_t, solution);
			}
			cout << "profit= " << p << ", solution= ";
			printArray(solution, sB);
			cout << "\n";
		}
		cout << "\n";

		// release memory
		for (int i = 0; i < MAXK_T; ++i)
			delete[] profits_t[i];
		delete[] profits_t;
		delete[] solution;
	}
	else{
		/* the user has not selected 0.
		masure the runinnig time for the selected algorithm  */

		int testpoints[NUMPOINTS] = { 10, 50, 100, 250, 500, 1000, 2000, 3000, 5000, 7000, 10000, 12000, 16000, MAXKC }; // values for knapsack size & capacity
		
		// memory allocation
		// note: safeguards in case of unavailable memory are not placed.
		int** profits;
		profits = new int*[MAXKC];
		for (int i = 0; i < MAXKC; i++) profits[i] = new int[MAXKC + 1];

		int* weights = new int[MAXKC];
		int* values = new int[MAXKC];

		// setting random quantities for weights and values; we're interested in performance not correctness
		int helper = 0, range = testpoints[helper];
		for (int i = 0; i < MAXKC; ++i){
			if (i>testpoints[helper] && helper < NUMPOINTS - 1) {
				++helper;
				range = testpoints[helper];
			}

			weights[i] = rand() % range;	// weight proportional to range
			values[i] = rand() % MAXKC;		// any value
		}

		// time test variables			| see http://technet.microsoft.com/it-it/sysinternals/dn553408(v=vs.110).aspx
		LARGE_INTEGER startingTime, endingTime, elapsedMicroseconds;
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);

		// .txt file for output
		std::ofstream performanceFile;
		performanceFile.open(names[kind] + ".txt");		// open file

		cout << names[kind] << "\n";					// display algorithm name to console
		
		// run knapsacks:
		for (int sizen = 0; sizen < NUMPOINTS; ++sizen){

			int size = testpoints[sizen];		// for size = 10, 50, 100, .. 

			for (int capacityn = 0; capacityn < NUMPOINTS; ++capacityn){

				int capacity = testpoints[capacityn];		// for capacity = 10, 50, 100, ..

				std::cout << size << "x" << capacity << " ";				// and to screen
				performanceFile << size << "x" << capacity << " ";			// write row title in file

				// calculate the average time of 25 runs:
				__int64 average = 0;
				for (int iteration = 0; iteration < MAXITERS; ++iteration){
					
					QueryPerformanceCounter(&startingTime);		// get starting time
					// run the appropriate algorithm
					if (kind == 1){
						mapreduceKnapsackCilkplus(size, capacity, values, weights, profits);
					}
					else if (kind == 2){
						mapreduceKnapsackTBB(size, capacity, values, weights, profits);
					}
					else if (kind == 3){
						mapKnapsackCilkplus(size, capacity, values, weights, profits);
					}
					else if (kind == 4){
						mapKnapsackTBB(size, capacity, values, weights, profits);
					}
					else if (kind == 5){
						serialKnapsackDP(capacity, size, weights, values, profits);
					}
					else if (kind == 6){
						mapreduceMapKnapsackCilkplus(size, capacity, values, weights, profits);
					}
					else if (kind == 7){
						mapreduceMapKnapsackTBB(size, capacity, values, weights, profits);
					}
					QueryPerformanceCounter(&endingTime);		// get ending time

					// calculate elapsed time
					elapsedMicroseconds.QuadPart = endingTime.QuadPart - startingTime.QuadPart;
					elapsedMicroseconds.QuadPart *= 1000000;
					elapsedMicroseconds.QuadPart /= frequency.QuadPart;

					// write in file
					performanceFile << elapsedMicroseconds.QuadPart << " ";

					// add to average var
					average += elapsedMicroseconds.QuadPart;
				}
				// divide with number of iterations
				average /= MAXITERS;
				// write average in file
				performanceFile << average << "\n";

				cout << "\n";
			}
		}

		// close file
		performanceFile.close();

		// memory release
		for (int i = 0; i < MAXKC; ++i)
			delete[] profits[i];
		delete[] profits;
		delete[] weights;
		delete[] values;

	}

	return 0;
}

